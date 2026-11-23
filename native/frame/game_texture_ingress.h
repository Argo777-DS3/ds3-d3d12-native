#pragma once
#include "../renderer/backend.h"
#include "../content/dds_resource.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <list>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace ds3native::bootstrap {
struct TextureDescription {
    uint32_t width{},height{},mips{};
    DXGI_FORMAT format{};
    uint32_t slices{1};
    bool cube{};
};
// I-0097 tranche 4 (D-0211): the key of an encoded texture's content (TextureIngress's kept contents): the hash of its engine
// bytes, their size, its entry name and its family word.
struct TextureContentKey {uint64_t hash{},bytes{};std::string name;uint32_t family{};bool operator==(const TextureContentKey&) const=default;};
struct NativeTextureRecord {
    TextureDescription description;
    // B3a: upload_fence is the ingress serial of the recorded upload (nonzero once queued ahead of every later
    // submission of the queue), not a fence ticket; resident means usable by any later submission.
    uint64_t generation{},content_bytes{},upload_fence{};
    bool resident{};
    bool render_target{};   // 1169: drawn by the native renderer after admission (menu offscreen views)
    // I-0070: set once the ingress stops returning this record (its receiver retired or admitted again, or the close); a
    // consumer that kept it checks this when Withdrawals() moved, instead of dropping everything it kept.
    std::atomic<bool> withdrawn{};
    std::shared_ptr<const ds3content::CpuTextureResource> content;   // I-0096: the description; its subresources carry no bytes
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    CpuDescriptor view;   // B3b: pooled CPU-only SRV slot
    std::shared_ptr<const TextureContentKey> kept_key;   // D-0211: the kept content it uses (read and written under the ingress lock)
};
// CPU asset ingress at the GX texture construction boundary. GPU handles
// are owned here and never published in the original graphics wrapper slots.
// D3D12 review R5 (B3a): uploads, FaceGen compositions and offscreen target
// clears are recorded on the queue's ingress context (DeviceQueues::RecordIngress)
// without the frame lock and without a CPU wait; the next submission of the queue
// carries them ahead of its own list.
class TextureIngress {
public:
    TextureIngress(DeviceQueues& gpu,std::mutex& submissions):gpu_(gpu),submissions_(submissions){}
    std::shared_ptr<const NativeTextureRecord> Admit(uintptr_t receiver,TextureDescription description,uintptr_t source);
    std::shared_ptr<const NativeTextureRecord> AdmitOwned(uintptr_t receiver,const ds3content::CpuTextureResource& source);
    // I-0074: the same, the CPU image moved into the record instead of copied.
    std::shared_ptr<const NativeTextureRecord> AdmitOwned(uintptr_t receiver,ds3content::CpuTextureResource&& source);
    // I-0097 tranche 4 (D-0211, the modern method: prepared once, found in a cache in play): an encoded texture admitted with
    // its key keeps its uploaded content (resource, description) under that key. While a record uses it, it costs nothing more;
    // once no record does (its receivers retired), it stays kept, up to 1 GB of placed bytes, the least recently retired released
    // first. When DS3 loads the same content again (a map part reloaded as the player moves), FindKept returns it and AdmitKept
    // installs the receiver's record at once, resident, sharing the kept resource with a view of its own: no decode, no upload, no
    // stand-in (D-0197). TrimKept releases kept contents no record uses, the least recently retired first, at least `bytes` of
    // them (the video memory budget) or all (the title); Close releases everything.
    using ContentKey=TextureContentKey;
    struct KeptContent {TextureDescription description;std::shared_ptr<const ds3content::CpuTextureResource> content;
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;uint64_t content_bytes{},upload_fence{},placed_bytes{};};
    std::shared_ptr<const NativeTextureRecord> AdmitOwned(uintptr_t receiver,ds3content::CpuTextureResource&& source,const ContentKey& keep);
    std::optional<KeptContent> FindKept(const ContentKey& key);
    std::shared_ptr<const NativeTextureRecord> AdmitKept(uintptr_t receiver,const ContentKey& key,const KeptContent& kept);
    uint64_t TrimKept(uint64_t bytes=UINT64_MAX);   // the placed bytes released
    struct KeptCensus {uint64_t hits{},misses{},entries{},retired{},retired_bytes{},placed_bytes{};};
    KeptCensus KeptCount();
    // FaceGen composition target (836E40): the engine draws its current CPU
    // face image over a whole width x height R8G8B8A8_UNORM render target. The
    // native owner executes that draw on its own queue and keeps the target.
    // The mark layer (836E40's second draw, 142379C90): a quad over the target, its corners in clip space in the
    // strip order (A, A + size, B, B + size; texcoords (0,0), (1,0), (0,1), (1,1)), the mark texture times the
    // constant colour, blended SRC_ALPHA / INV_SRC_ALPHA, the target's alpha replaced by the source's (ONE / ZERO).
    struct FaceMarkLayer {std::shared_ptr<const NativeTextureRecord> texture;std::array<float,8> corners{};std::array<float,4> colour{};};
    std::shared_ptr<const NativeTextureRecord> AdmitComposedFace(uintptr_t receiver,const NativeTextureRecord& base,uint32_t width,uint32_t height,
        const FaceMarkLayer* mark=nullptr);
    // Menu offscreen model render target (I-0066 1165): width x height R8G8B8A8_UNORM render target cleared to
    // `clear`, left shader-readable, with its description (no CPU bytes, I-0096) so the UI image ingress can share it.
    std::shared_ptr<const NativeTextureRecord> AdmitOffscreenTarget(uintptr_t receiver,uint32_t width,uint32_t height,const std::array<float,4>& clear);
    // A zero-filled structured buffer (elements x stride) with its SRV, for shader
    // buffers whose native producer is an explicit empty policy.
    std::shared_ptr<const NativeTextureRecord> AdmitStructuredZero(uintptr_t receiver,uint32_t stride,uint32_t elements);
    std::shared_ptr<const NativeTextureRecord> Acquire(uintptr_t receiver) const;
    // I-0132 R12a's second tranche: the first resident record whose content has this entry name (DS3's menu textures, MENU_* of
    // 01_common.tpf, as the menus loaded them), none before; the holder keeps the record (and its resource) alive.
    std::shared_ptr<const NativeTextureRecord> FindByName(std::string_view name) const {
        std::shared_lock<std::shared_mutex> hold(mutex_);if(closed_)return nullptr;   // the step review's L2: none once closing
        for(const auto& [receiver,record]:records_){(void)receiver;   // `resident` first: written under the exclusive lock after `resource`
            if(record && record->resident && !record->render_target && record->resource && record->content && record->content->name==name &&
               record->description.slices==1 && !record->description.cube)return record;}   // L3: a plain 2D texture only
        return nullptr;}
    void Retire(uintptr_t receiver);
    // I-0097 E5: a record replaced earlier (its withdrawal already counted) is now superseded where its holders look: the
    // withdrawals counted once more, so consumers that took the withdrawn record meanwhile check their records again.
    void NoteRepublished() {std::lock_guard<std::shared_mutex> hold(mutex_);Withdrawn();Changed();}
    // I-0087 (the residual slopes of the night endurance test): the records held, those that are render targets (the menu
    // offscreen targets; a composed FaceGen face is not flagged as one), and the committed content bytes of the render targets.
    // I-0096: and the bytes of the CPU images the records hold (every subresource of `content`); the records withdrawn (retired
    // or replaced) that a holder outside the ingress still keeps alive, their GPU content bytes and the largest ones' names.
    struct Census {size_t records{},render_targets{},withdrawn_held{};uint64_t render_target_bytes{},cpu_image_bytes{},withdrawn_held_bytes{};
        std::string withdrawn_largest;};
    Census Count() {Census c;std::vector<std::shared_ptr<const NativeTextureRecord>> alive;   // step review 2: released after the lock
        {std::lock_guard<std::shared_mutex> hold(mutex_);c.records=records_.size();
            for(const auto& [receiver,record]:records_){(void)receiver;if(record->render_target){++c.render_targets;c.render_target_bytes+=record->content_bytes;}
                if(record->content)for(const auto& sub:record->content->subresources)c.cpu_image_bytes+=sub.bytes.size();}
            std::erase_if(withdrawn_,[&](const std::weak_ptr<const NativeTextureRecord>& weak){auto record=weak.lock();if(!record)return true;alive.push_back(std::move(record));return false;});}
        std::vector<std::pair<uint64_t,std::string>> held;
        for(const auto& record:alive){++c.withdrawn_held;c.withdrawn_held_bytes+=record->content_bytes;held.emplace_back(record->content_bytes,record->content?record->content->name:std::string("(no content)"));}
        std::partial_sort(held.begin(),held.begin()+std::min<size_t>(8,held.size()),held.end(),[](const auto& a,const auto& b){return a.first>b.first;});
        for(size_t i=0;i<std::min<size_t>(8,held.size());++i){std::string name=held[i].second.substr(0,48);
            for(auto& ch:name)if(ch=='"' || ch=='\\' || static_cast<unsigned char>(ch)<32 || static_cast<unsigned char>(ch)>126)ch='?';
            c.withdrawn_largest+=(i?",":"")+std::string("[\"")+name+"\","+std::to_string(held[i].first>>10)+"]";}
        return c;}
    // Workstream K (K-COLLECT-IMPLEMENTATION-SHEET 2.4 point 2): changes whenever what Acquire returns can change: an
    // admission, a record becoming resident, a retirement, the close. A record's own generation numbers admissions only
    // (it is set before the upload, while resident is still false), so a consumer that kept a choice across frames
    // compares this value instead.
    uint64_t Version() const noexcept {return version_.load(std::memory_order_acquire);}
    // I-0070: changes only when a record Acquire returned can stop being returned (a retirement, an admission replacing a
    // receiver's record, the close). An admission of a new receiver or a record becoming resident leaves it: a consumer that
    // kept a resident record stays right across those: when this value moved, it checks its records' withdrawn flags.
    uint64_t Withdrawals() const noexcept {return withdrawals_.load(std::memory_order_acquire);}
    // Withdraw admission first, wait for in-flight admissions, submit what they
    // queued and drain before clearing. Must precede DeviceQueues::Close and not
    // hold submissions_.
    void Close();
private:
    // Counts an admission between its closed_ check and its end; Close waits for zero.
    struct Admission {TextureIngress& owner;explicit Admission(TextureIngress& o);~Admission();};
    DeviceQueues& gpu_;
    std::mutex& submissions_;
    mutable std::shared_mutex mutex_;   // I-0097 tranche 2: Acquire takes it shared (the collect's workers look up at once)
    std::condition_variable_any idle_;
    uint32_t admitting_{};
    std::mutex face_lock_;   // face_root_/face_pso_ creation
    std::unordered_map<uintptr_t,std::shared_ptr<NativeTextureRecord>> records_;
    uint64_t generation_{};
    std::atomic<uint64_t> version_{};
    void Changed() noexcept {version_.fetch_add(1,std::memory_order_acq_rel);}   // under mutex_, after the change
    std::atomic<uint64_t> withdrawals_{};
    void Withdrawn() noexcept {withdrawals_.fetch_add(1,std::memory_order_acq_rel);}   // under mutex_, before Changed
    // Under mutex_: the receiver's record replaced (the former one withdrawn), then the version moved.
    void Install(uintptr_t receiver,const std::shared_ptr<NativeTextureRecord>& candidate) {
        auto& slot=records_[receiver];if(slot){slot->withdrawn.store(true,std::memory_order_release);withdrawn_.push_back(slot);ReleaseKeptUser(*slot);Withdrawn();}
        slot=candidate;Changed();}
    std::vector<std::weak_ptr<const NativeTextureRecord>> withdrawn_;   // I-0096 census, pruned by Count()
    // D-0211: the kept contents by key, with the records using each; those no record uses in the retired list (most recently
    // retired first) and their placed bytes. kept_lock_ is taken alone or inside mutex_, never the other way; released contents
    // wait in kept_released_ and are destroyed outside both (FlushKept).
    struct KeptKeyHash {size_t operator()(const ContentKey& k) const noexcept {return static_cast<size_t>(k.hash^(k.bytes*0x9E3779B97F4A7C15ull)^k.family);}};
    struct KeptEntry {KeptContent content;uint32_t users{};std::list<ContentKey>::iterator retired{};};
    static constexpr uint64_t kept_limit=1ull<<30;
    std::mutex kept_lock_;std::unordered_map<ContentKey,KeptEntry,KeptKeyHash> kept_;std::list<ContentKey> kept_retired_;
    std::vector<KeptContent> kept_released_;
    uint64_t kept_retired_bytes_{},kept_placed_bytes_{},kept_hits_{},kept_misses_{};bool kept_closed_{};
    void ReleaseKeptUser(const NativeTextureRecord& record);   // under mutex_
    void EvictKept(uint64_t bytes);   // under kept_lock_
    void FlushKept();   // outside both locks
    std::shared_ptr<const NativeTextureRecord> AdmitOwnedKept(uintptr_t receiver,ds3content::CpuTextureResource&& source,const ContentKey* keep);
    bool closed_{};
    Microsoft::WRL::ComPtr<ID3D12RootSignature> face_root_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> face_pso_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> face_mark_root_;   // the mark layer's (under face_lock_)
    Microsoft::WRL::ComPtr<ID3D12PipelineState> face_mark_pso_;
};
}
