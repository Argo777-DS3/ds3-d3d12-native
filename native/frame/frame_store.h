#pragma once
// I-0091 (ER-STUDY §5: GXBS::GXLinearAllocator, a frame arena kept and reset at each scene render, 141B452B0; per-context
// chunked bump allocators without lock, 141AD1FC0 -> 141C4ACD0): the collect's per-frame arrays take their memory from blocks
// kept between frames. A scene's arrays return their blocks when the scene is destroyed, on whichever thread releases it (the
// render worker, the frame ring's release thread, NativeReleaseLater), and a later collect takes them again, so the pages a
// frame writes stay mapped: the system is asked for memory only when no kept block of the class is free. ER's spin lock per
// chunk refill is not copied: a short lock per block (a few a frame), never per record.
#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace ds3native::frame_store {
// Blocks by power-of-two class from 64 KiB; smaller requests (and larger than the last class) are the general heap's.
constexpr size_t min_block=size_t(64)<<10;
constexpr unsigned class_count=15;   // 64 KiB .. 1 GiB
constexpr unsigned kept_per_class=8;
constexpr size_t held_limit=size_t(160)<<20;   // the kept blocks' total; a block past it goes back to the system

class Store {
public:
    struct Counters {uint64_t reused{},fresh{},kept{},freed{},held_bytes{},held_blocks{};};
    // `keep` false (no-frame-store.flag, the A/B): every released block goes back to the system, so each frame's arrays take fresh
    // pages as before I-0091 (their sizes still rounded to the classes; the pass rows' kept chunks are not covered by the flag).
    void Keep(bool keep) noexcept {keep_.store(keep,std::memory_order_relaxed);}
    // Memory for `bytes`; `granted` receives its usable size (the class's size for a kept-block class).
    void* Acquire(size_t bytes,size_t& granted) {
        const unsigned c=bytes<min_block?class_count:ClassOf(bytes);
        if(c>=class_count){granted=bytes;return ::operator new(bytes);}
        granted=min_block<<c;
        {const std::lock_guard hold(lock_);auto& list=free_[c];
            if(!list.empty()){void* block=list.back();list.pop_back();held_-=granted;++counters_.reused;return block;}
            ++counters_.fresh;}
        return ::operator new(granted);
    }
    // `granted` as Acquire gave it: a class block is kept (or given back to the system), any other freed.
    void Release(void* block,size_t granted) noexcept {
        if(!block)return;
        const unsigned c=granted>=min_block && std::has_single_bit(granted)?ClassOf(granted):class_count;
        if(c<class_count){const std::lock_guard hold(lock_);auto& list=free_[c];
            if(keep_.load(std::memory_order_relaxed) && list.size()<kept_per_class && held_+granted<=held_limit){
                list.push_back(block);held_+=granted;++counters_.kept;return;}   // the list's room is reserved: no allocation
            ++counters_.freed;}
        ::operator delete(block);
    }
    Counters Read() const {const std::lock_guard hold(lock_);Counters out=counters_;out.held_bytes=held_;
        for(const auto& list:free_)out.held_blocks+=list.size();return out;}
    // Each class's list has room reserved up front, so keeping a block never allocates.
    Store() {for(auto& list:free_)list.reserve(kept_per_class);}
private:
    static unsigned ClassOf(size_t bytes) noexcept {
        if(bytes<=min_block)return 0;
        return static_cast<unsigned>(std::bit_width(bytes-1)-std::bit_width(min_block-1));}
    mutable std::mutex lock_;
    std::array<std::vector<void*>,class_count> free_;
    size_t held_{};Counters counters_;
    std::atomic<bool> keep_{true};
};
// The module's store, never destroyed (a scene released by a detached thread at process exit still returns its blocks).
inline Store& Shared() {static Store* const store=new Store;return *store;}

// A frame array: a vector's subset over the store's blocks, whose places can also be constructed by several threads at once
// (Uninitialized, then Commit): an element is built once where it stays, never value-initialised first and assigned after.
template<class T> class Array {
    static_assert(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_destructible_v<T>);
    static_assert(alignof(T)<=__STDCPP_DEFAULT_NEW_ALIGNMENT__);
public:
    using value_type=T;using size_type=size_t;using iterator=T*;using const_iterator=const T*;
    using reference=T&;using const_reference=const T&;
    Array()=default;
    Array(const Array&)=delete;Array& operator=(const Array&)=delete;
    Array(Array&& other) noexcept:data_(std::exchange(other.data_,nullptr)),size_(std::exchange(other.size_,0)),
        capacity_(std::exchange(other.capacity_,0)),granted_(std::exchange(other.granted_,0)),pending_(std::exchange(other.pending_,0)) {}
    Array& operator=(Array&& other) noexcept {if(this!=&other){Free();data_=std::exchange(other.data_,nullptr);size_=std::exchange(other.size_,0);
        capacity_=std::exchange(other.capacity_,0);granted_=std::exchange(other.granted_,0);pending_=std::exchange(other.pending_,0);}return *this;}
    ~Array() {Free();}
    size_t size() const noexcept {return size_;}
    size_t capacity() const noexcept {return capacity_;}
    bool empty() const noexcept {return !size_;}
    T* data() noexcept {return data_;}
    const T* data() const noexcept {return data_;}
    T* begin() noexcept {return data_;}
    T* end() noexcept {return data_+size_;}
    const T* begin() const noexcept {return data_;}
    const T* end() const noexcept {return data_+size_;}
    T& operator[](size_t i) noexcept {return data_[i];}
    const T& operator[](size_t i) const noexcept {return data_[i];}
    T& at(size_t i) {if(i>=size_)throw std::out_of_range("frame array");return data_[i];}
    const T& at(size_t i) const {if(i>=size_)throw std::out_of_range("frame array");return data_[i];}
    T& back() noexcept {return data_[size_-1];}
    const T& back() const noexcept {return data_[size_-1];}
    void reserve(size_t wanted) {Settled();if(wanted>capacity_)Regrow(wanted);}
    template<class... Args> T& emplace_back(Args&&... args) {
        Settled();
        if(size_==capacity_)Regrow(size_+1>capacity_+capacity_/2?size_+1:capacity_+capacity_/2);
        T* at=::new(static_cast<void*>(data_+size_)) T(std::forward<Args>(args)...);++size_;return *at;}
    void push_back(T&& value) {emplace_back(std::move(value));}
    void push_back(const T& value) {emplace_back(value);}
    // Shrinks (the tail destroyed); grows by value-initialised elements.
    void resize(size_t wanted) {Settled();
        if(wanted<=size_){std::destroy(data_+wanted,data_+size_);size_=wanted;return;}
        reserve(wanted);while(size_<wanted){::new(static_cast<void*>(data_+size_)) T();++size_;}}
    void clear() {Settled();std::destroy(data_,data_+size_);size_=0;}
    template<class Pred> size_t EraseIf(Pred pred) {Settled();
        T* kept=std::remove_if(begin(),end(),pred);const size_t removed=static_cast<size_t>(end()-kept);std::destroy(kept,end());size_-=removed;return removed;}
    // The room for `count` more elements, after the last: the caller constructs each of them (from any thread, each place once)
    // before Commit(count); nothing else may be called on the array in between.
    T* Uninitialized(size_t count) {
        Settled();reserve(size_+count);pending_=count;return data_+size_;}
    void Commit(size_t count) {
        if(count!=pending_)throw std::logic_error("frame array: commit of other places");
        size_+=count;pending_=0;}
private:
    // Places pending (Uninitialized without its Commit): no other change of the array.
    void Settled() const {if(pending_)throw std::logic_error("frame array: places pending");}
    void Regrow(size_t wanted) {
        size_t granted{};T* fresh=static_cast<T*>(Shared().Acquire(wanted*sizeof(T),granted));
        std::uninitialized_move(data_,data_+size_,fresh);std::destroy(data_,data_+size_);
        Shared().Release(data_,granted_);data_=fresh;granted_=granted;capacity_=granted/sizeof(T);}
    // Places left pending (never reached: their constructions cannot throw) are not known one by one: their block is kept away.
    void Free() noexcept {if(pending_)return;std::destroy(data_,data_+size_);size_=0;Shared().Release(data_,granted_);data_=nullptr;capacity_=granted_=0;}
    T* data_{};size_t size_{},capacity_{},granted_{},pending_{};
};
}  // namespace ds3native::frame_store
