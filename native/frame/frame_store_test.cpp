// I-0091 control: the collect's frame store keeps a released block of a class and gives it back to the next request of that
// class (no new memory), frees it when keeping is off or the class's list is full, and leaves small requests to the heap; the
// frame array keeps its elements across a regrowth, constructs places once (Uninitialized then Commit, from several threads),
// erases in order and returns its block when destroyed.
#include "frame_store.h"
#include <cstdio>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace ds3native::frame_store;
namespace {
int failures{};
void Expect(bool condition,const char* what) {if(!condition){++failures;std::printf("FAIL %s\n",what);}}
struct Counted {
    static inline std::atomic<int> alive{};
    std::shared_ptr<int> value;std::array<uint8_t,1000> bytes{};
    Counted() {++alive;}
    explicit Counted(int v):value(std::make_shared<int>(v)) {++alive;bytes[0]=static_cast<uint8_t>(v);}
    Counted(Counted&& o) noexcept:value(std::move(o.value)),bytes(o.bytes) {++alive;}
    Counted& operator=(Counted&& o) noexcept {value=std::move(o.value);bytes=o.bytes;return *this;}
    ~Counted() {--alive;}
};
}
int main() {
    auto& store=Shared();
    // A class block released and taken again: the same memory, counted as reused.
    {size_t granted{};void* a=store.Acquire(100000,granted);Expect(granted==size_t(128)<<10,"100,000 bytes take the 128 KiB class");
        const auto before=store.Read();store.Release(a,granted);
        size_t again{};void* b=store.Acquire(70000,again);const auto after=store.Read();
        Expect(b==a && again==granted,"the released block of the class is given back");
        Expect(after.kept==before.kept+1 && after.reused==before.reused+1 && after.fresh==before.fresh,"kept then reused, nothing fresh");
        store.Release(b,again);}
    // Small requests are the heap's: never kept.
    {size_t granted{};void* a=store.Acquire(1000,granted);Expect(granted==1000,"a small request keeps its size");
        const auto before=store.Read();store.Release(a,granted);Expect(store.Read().kept==before.kept,"a small block is not kept");}
    // The class boundary: exactly 64 KiB is class 0, one byte more class 1.
    {size_t g0{},g1{};void* a=store.Acquire(min_block,g0);void* b=store.Acquire(min_block+1,g1);
        Expect(g0==min_block && g1==2*min_block,"64 KiB is class 0, one byte more class 1");store.Release(a,g0);store.Release(b,g1);}
    // A full class list frees the next released block.
    {std::vector<std::pair<void*,size_t>> blocks;for(unsigned i=0;i<kept_per_class+2;++i){size_t g{};void* p=store.Acquire(size_t(3)<<20,g);blocks.push_back({p,g});}
        const auto before=store.Read();for(const auto& [p,g]:blocks)store.Release(p,g);const auto after=store.Read();
        Expect(after.freed>=before.freed+2,"a full class list gives the extra blocks back");
        // Keeping off (no-frame-store.flag): a released class block goes back to the system.
        std::vector<std::pair<void*,size_t>> again;for(unsigned i=0;i<kept_per_class;++i){size_t g{};void* p=store.Acquire(size_t(3)<<20,g);again.push_back({p,g});}
        store.Keep(false);const auto off=store.Read();for(const auto& [p,g]:again)store.Release(p,g);
        Expect(store.Read().kept==off.kept && store.Read().freed==off.freed+kept_per_class,"keeping off frees every block");store.Keep(true);}
    // The array: growth keeps the elements; push_back, back, EraseIf in order; destruction returns the block.
    {Array<Counted> array;for(int i=0;i<300;++i)array.push_back(Counted(i));   // 300 KB: grows through classes
        bool ordered=true;for(int i=0;i<300;++i)ordered=ordered && array[i].value && *array[i].value==i && array[i].bytes[0]==static_cast<uint8_t>(i);
        Expect(ordered && array.size()==300 && Counted::alive==300,"growth keeps every element, once");
        Expect(array.capacity()*sizeof(Counted)>=min_block,"a large array is on a class block");
        array.EraseIf([](const Counted& c){return *c.value%3==0;});
        bool kept=array.size()==200;for(size_t i=0;kept && i<array.size();++i)kept=*array[i].value%3!=0 && (i==0 || *array[i].value>*array[i-1].value);
        Expect(kept && Counted::alive==200,"EraseIf keeps the others in order and destroys the erased");
        array.resize(50);Expect(array.size()==50 && Counted::alive==50,"resize shrinks and destroys the tail");
        array.resize(60);Expect(array.size()==60 && !array[59].value && Counted::alive==60,"resize grows with value-initialised elements");}
    Expect(Counted::alive==0,"the destroyed array destroys every element");
    // Places constructed once by several threads, then committed; nothing else allowed meanwhile.
    {Array<Counted> array;array.push_back(Counted(-1));
        constexpr size_t count=4000;Counted* places=array.Uninitialized(count);
        bool refused=false;try{array.push_back(Counted(-2));}catch(const std::logic_error&){refused=true;}
        Expect(refused,"a push while places are pending is refused");
        bool regrow=false;try{array.reserve(count*4);}catch(const std::logic_error&){regrow=true;}
        Expect(regrow,"a reserve while places are pending is refused (the workers' block stays)");
        std::vector<std::thread> threads;for(unsigned t=0;t<4;++t)threads.emplace_back([&,t]{for(size_t i=t;i<count;i+=4)::new(static_cast<void*>(places+i)) Counted(static_cast<int>(i));});
        for(auto& t:threads)t.join();
        bool other=false;try{array.Commit(count-1);}catch(const std::logic_error&){other=true;}
        Expect(other,"a commit of another count is refused");
        array.Commit(count);
        bool placed=array.size()==count+1 && *array[0].value==-1;for(size_t i=0;placed && i<count;++i)placed=*array[i+1].value==static_cast<int>(i);
        Expect(placed && Counted::alive==static_cast<int>(count+1),"every place constructed once and committed");}
    Expect(Counted::alive==0,"the committed places are destroyed with the array");
    // A block a destroyed array returned is taken by the next array of its class (the next frame's).
    {const void* first{};{Array<Counted> a;a.reserve(500);first=a.data();}
        Array<Counted> b;b.reserve(500);Expect(b.data()==first,"the next frame's array takes the kept block");}
    if(failures){std::printf("frame_store_test: %d failure(s)\n",failures);return 1;}
    std::printf("frame_store_test: ok\n");return 0;
}
