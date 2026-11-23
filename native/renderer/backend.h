#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include "hdr_output.h"
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <array>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <memory>
#include <functional>
#include <mutex>
#include <thread>
#include <utility>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <span>
#include <vector>
#include "content.h"

namespace ds3native {
struct FrameReceipt {
    uint64_t serial{}, fence{}, presentation_fence{}, pixel_hash{};
    uint32_t changed_pixels{}, debug_errors{};
    uint64_t content_incarnation{};
    uint32_t content_draw_count{};
    uint32_t primitive_draw_count{};
    HRESULT present_status{};
    bool present_lost{};   // I-0101: present_status is a failed Present admitted (the device not lost); the frame completed
    // 1129 witness: CPU milliseconds of recording, of the GPU completion wait, and of present/drain/diagnostics.
    float record_ms{},gpu_wait_ms{},present_ms{};
};

// D3D12 phase D cD0 (D3D12-PHASE-D-IMPLEMENTATION-SHEET 9.1): process audit counters read by the frame owner's audit row.
// Drains and completion waits by call site, GPU objects created (committed resources and descriptor heaps, counted at
// every creation of the game path), depth target recreations, lighting programme changes, dynamic stream uploads.
enum WaitSite : uint8_t {WaitOther,WaitMainFrame,WaitOffscreen,WaitMovie,WaitDepthRecreate,WaitMoviePlanes,WaitStartup,
    WaitResize,WaitClose,WaitFixture,WaitPostLayout,WaitSiteCount};   // WaitPostLayout: no drain since the optimisation review (the stages renew their heaps), kept so the audit row keeps its columns
struct GpuAudit {
    std::array<std::atomic<uint64_t>,WaitSiteCount> drains{},completes{};
    std::atomic<uint64_t> resources_created{},heaps_created{},depth_recreations{},lighting_programme_changes{},
        dynamic_stream_updates{},dynamic_stream_bytes{};
    // Phase E E0 (D3D12-PHASE-E-FRAME-GRAPH-SHEET 7): hand-written transition barriers recorded through the Transition and
    // DepthPlaneTransition helpers of scene_pass.cpp and primitive_pass.cpp (dynamic calls; raw ResourceBarrier calls of the
    // stream and texture ingress are not counted; a helper called from a lambda counts at the lambda's line) and the
    // distinct call sites seen since start; the allocation bytes of the committed render and depth targets of the main
    // view and of the menu-view contexts, their resource count, the number of view extents and the largest placement
    // alignment the device reports for them (set by the recording thread when a target generation changes).
    std::atomic<uint64_t> barrier_calls{},barrier_sites{},target_bytes_main{},target_bytes_views{},target_resources{},target_extents{},target_alignment{};
    std::atomic<uint64_t> target_priority_raised{};   // I-0084 (D1): targets of the last generation at high residency priority
};
inline GpuAudit gpu_audit;
// Phase E (sheet 6.15, E8): GPU-based validation, enabled with the debug layer before the device is created
// (gpu-validation.flag); it serialises the GPU, so timings are judged without it.
inline std::atomic<bool> gpu_based_validation{};
// I-0082 (DESIGN 4): DRED breadcrumbs and page faults, enabled before the device is created unless no-dred.flag; and the GPU
// hang the completion wait saw (its 5 s deadline passed), read by the module's fault exit.
inline std::atomic<bool> device_removal_data{true};
inline std::atomic<bool> gpu_hang_detected{};
// E0: a barrier site is (file, line): file 0 scene_pass.cpp, 1 primitive_pass.cpp, 2 to 6 the scene renderer's other parts
// (scene_pass_internal.h ScenePassFile).
inline std::array<std::atomic<uint8_t>,7*4096> barrier_site_seen{};
inline void NoteBarrier(uint32_t file,uint32_t line) noexcept {
    gpu_audit.barrier_calls.fetch_add(1,std::memory_order_relaxed);const uint32_t site=file*4096+line;
    if(line<4096 && site<barrier_site_seen.size() && !barrier_site_seen[site].load(std::memory_order_relaxed) &&
       !barrier_site_seen[site].exchange(1,std::memory_order_relaxed))gpu_audit.barrier_sites.fetch_add(1,std::memory_order_relaxed);}
inline ID3D12Device* CountedResource(ID3D12Device* device) noexcept {gpu_audit.resources_created.fetch_add(1,std::memory_order_relaxed);return device;}
inline ID3D12Device* CountedResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device) noexcept {return CountedResource(device.Get());}
inline ID3D12Device* CountedHeap(ID3D12Device* device) noexcept {gpu_audit.heaps_created.fetch_add(1,std::memory_order_relaxed);return device;}
inline ID3D12Device* CountedHeap(const Microsoft::WRL::ComPtr<ID3D12Device>& device) noexcept {return CountedHeap(device.Get());}

// I-0109 (R0): the device's capabilities for the ray-tracing path, CheckFeatureSupport's answers; feature_level is the device's
// highest (the raster passes need no more than 11_0 until their traced replacements remove them, D-0237).
struct DeviceCapabilities {
    D3D_FEATURE_LEVEL feature_level{};D3D_SHADER_MODEL shader_model{};D3D12_RAYTRACING_TIER raytracing{D3D12_RAYTRACING_TIER_NOT_SUPPORTED};
    D3D12_RESOURCE_BINDING_TIER binding{};D3D12_MESH_SHADER_TIER mesh{D3D12_MESH_SHADER_TIER_NOT_SUPPORTED};bool enhanced_barriers{};
    bool native_16bit{};   // I-0126 R7b: OPTIONS4's native 16-bit shader ops (the rt_reflect programmes' SHaRC types)
    // I-0132 R11f: the D3D12 runtime's SDK version (the Agility SDK's through the device factory, else the inbox runtime's: 0 when the
    // device does not say) and OPTIONS16's GPU upload heaps.
    UINT sdk_version{};bool gpu_upload{};
};
// I-0132 R11f (R11-PLAN §8; Thomas, 2026-11-09 04:48: « Tu peux prendre l'Agility SDK d'ailleurs »): the Agility SDK's runtime for the
// native device, through ID3D12SDKConfiguration1::CreateDeviceFactory (no D3D12SDKVersion export: the module lives in DS3's process).
// agility_sdk_folder: the folder holding its D3D12Core.dll and d3d12SDKLayers.dll, absolute, with its trailing separator, set before
// DeviceQueues::Open (the module's D3D12 folder by default since I-0132 R11i, empty under no-agility.flag, WARP and PIX); empty: the inbox runtime. kAgilitySdkVersion is the
// package's D3D12_SDK_VERSION (Microsoft.Direct3D.D3D12 1.620.1, the DLLs Build-GameBootstrap.ps1 links beside the module).
inline std::wstring agility_sdk_folder;
inline constexpr UINT kAgilitySdkVersion=620;
// I-0132 R11f: GPU upload heaps (D3D12_HEAP_TYPE_GPU_UPLOAD: video memory the CPU writes through Resizable BAR) for the frame's upload
// arena and the dynamic streams' versions, where the device offers them (OPTIONS16) unless no-gpu-upload.flag (gpu_upload_allowed
// false, set before Open); gpu_upload_heaps set by Open. The CPU only writes those buffers (write-combined either way).
inline std::atomic<bool> gpu_upload_allowed{true},gpu_upload_heaps{};
// I-0132 R11f: the Agility SDK 1.620 debug layer's notice at every classic CheckFeatureSupport (« superseded by
// ID3D12Device16::CheckFeatureSupport1 »), a warning without a named ID in either the Windows Kits 10.0.28000 or the Agility 1.620
// headers, so matched by its text: not a fault. Its reads stay sound because the module compiles against the 10.0.28000 headers
// (D3D12_SDK_VERSION 617): the levels, shader models, structures and tiers it asks are that SDK's; a capability read added later
// must be checked against that (CheckFeatureSupport1 would clamp the answers to the caller's SDK version).
inline bool AgilityCapabilityNotice(D3D12_MESSAGE_SEVERITY severity,const char* text) noexcept {
    return severity==D3D12_MESSAGE_SEVERITY_WARNING && text && std::strstr(text,"CheckFeatureSupport is superseded by ID3D12Device16::CheckFeatureSupport1");}
// I-0132 R12f (the run r12f-live2): DLSS-G loaded while the game runs, the debug layer warned at its first present that a queue waited on
// a fence at 0 (« waiting for a fence value of zero will always be satisfied »): the module never waits on a fence value of 0 (its only
// ID3D12CommandQueue::Wait is test-gpu-hang.flag's, at 1), so the wait is Streamline's own queue's (INFERRED), harmless by the warning's own words; not a fault while DLSS-G is loaded (the flag set by
// the renderer then), a fault otherwise.
inline std::atomic<bool> frame_generation_loaded_now{};inline std::atomic<uint64_t> frame_generation_fence_notices{};
inline bool FrameGenerationFenceNotice(D3D12_MESSAGE_SEVERITY severity,const char* text,bool counted=true) noexcept {   // counted once: Diagnostics()
    if(!frame_generation_loaded_now.load(std::memory_order_relaxed) || severity!=D3D12_MESSAGE_SEVERITY_WARNING || !text ||
       !std::strstr(text,"ID3D12CommandQueue1::Wait: Fence values can never be less than zero"))return false;
    if(counted)frame_generation_fence_notices.fetch_add(1,std::memory_order_relaxed);return true;}
inline D3D12_HEAP_TYPE UploadHeapType() noexcept {return gpu_upload_heaps.load(std::memory_order_relaxed)?D3D12_HEAP_TYPE_GPU_UPLOAD:D3D12_HEAP_TYPE_UPLOAD;}

// Explicit bootstrap/queues and submission-completion ownership.
// D3D12 review R2/R3 (phase B): the queue whose fence returns pooled transient memory (constant chunks, UI uploads,
// UI targets), set by the renderer that records the frames and withdrawn on its close. `owner` is released at once when
// no queue is set or nothing was submitted yet, else after the queue completes the ticket submitted last.
class DeviceQueues;
void SetRetirementQueue(DeviceQueues* queues) noexcept;
void WithdrawRetirementQueue(DeviceQueues* queues) noexcept;
void RetireWhenSubmittedWorkCompletes(std::shared_ptr<void> owner);
// Pre-merge review (axis 2): an object the list being recorded on this thread may already reference (a stage's renewed heap,
// a retired surface): kept until the next list this thread submits has completed (SubmitWithIngress with a list), never against
// the previous submission; without a retirement queue kept for the process's life as RetireWhenSubmittedWorkCompletes does.
void RetireAfterCurrentRecording(std::shared_ptr<void> owner);
class DeviceQueues;
void RetireRecordingOwners(DeviceQueues& queues,uint64_t ticket);   // SubmitWithIngress, after a list of this thread
// Phase D complement 4.4: while a frame slot whose ticket has completed is released, owners reaching
// RetireWhenSubmittedWorkCompletes on that thread are released at once (every list that referenced them through the
// slot has completed; a later list still referencing them holds them itself), not one frame later.
struct ReleaseAfterCompletion {
    ReleaseAfterCompletion() noexcept;
    ~ReleaseAfterCompletion();
    ReleaseAfterCompletion(const ReleaseAfterCompletion&)=delete;
    ReleaseAfterCompletion& operator=(const ReleaseAfterCompletion&)=delete;
private:
    bool previous_{};
};
// D3D12 review R5 (phase B3a): the ingress copy context. A loader thread records an upload into its own pooled list
// (no lock held while recording, never the caller's frame lock) and commits it; the next Execute submits every committed
// list ahead of its own list in one ExecuteCommandLists call, in commit order, so every later list on this queue sees the
// copies without a CPU wait. The list and what it keeps return after that submission's ticket completes. A recording
// dropped without Commit is discarded (it was never submitted). Lock order: the caller's submission lock, then ingress.
class DeviceQueues;
class IngressRecording {
public:
    IngressRecording(IngressRecording&& other) noexcept;
    IngressRecording& operator=(IngressRecording&&)=delete;
    ~IngressRecording();
    ID3D12GraphicsCommandList* List() const noexcept;
    void Keep(std::shared_ptr<void> owner);   // released once the submission carrying this list completes
    uint64_t Commit();                        // closes and queues the list; returns its nonzero ingress serial
private:
    friend class DeviceQueues;
    struct Pending;
    IngressRecording(DeviceQueues& owner,std::unique_ptr<Pending> pending) noexcept;
    DeviceQueues* owner_{};
    std::unique_ptr<Pending> pending_;
};
// D3D12 review R5 (phase B3b): one CBV/SRV/UAV descriptor from a pooled non-shader-visible heap, in place of a heap
// per texture. It is read only by the CPU (copied into shader-visible tables when a draw is prepared), so the slot
// returns to the pool as soon as its holder lets go; every reader holds the record that owns it while it copies.
struct CpuDescriptor {
    D3D12_CPU_DESCRIPTOR_HANDLE handle{};
    std::shared_ptr<void> lease;
    explicit operator bool() const noexcept {return handle.ptr!=0;}
};
// I-0079: the creation memory of streamed resources (textures and static buffers with initial data). A committed resource
// of its own waited on the paging fence at its creation (and freed its allocation at its release): about 1 ms a texture on
// DS3's loading threads, which the engine thread joins while the world streams (Firelink walk, i79-04). After ER's placed arm
// (D3D12-DESIGN-REVIEW E7) and its fence-stamped upload ring (I-0073 ER-STUDY 2): the resource is placed in a pooled DEFAULT
// heap of its kind (64 MB, ALLOW_ONLY_*, not zeroed where the runtime allows), its range returned when the resource object is
// destroyed (a lease in its private data, so every holder's fencing still applies); its bytes go through a range of a pooled,
// persistently mapped UPLOAD chunk whose lease the copy's recording keeps until that submission completes. A resource above
// the placed bound, or when no pool is open, is committed as before. Any thread.
enum class PlacedKind : uint8_t {Texture,Buffer};
struct StagingRange {
    uint8_t* cpu{};ID3D12Resource* resource{};uint64_t offset{},bytes{};
    std::shared_ptr<void> lease;   // keep it until the submission that reads the range has completed
};
// I-0096: per pool (0 textures, 1 small textures, 2 buffers) the heaps' bytes, the live placed bytes and the heaps with every
// range free, so a pool's free but pinned memory reads as heap - live.
struct StreamedPoolCounters {uint64_t heaps{},heap_bytes{},live_bytes{},empty_heaps{};};
struct StreamedMemoryCounters {uint64_t heaps{},placed{},committed{},placed_live_bytes{},staging_chunks{},staging_dedicated{},staging_bytes{};
    std::array<StreamedPoolCounters,3> pools{};};
class DeviceQueues {
public:
    CpuDescriptor AllocateCpuDescriptor();
    size_t CpuDescriptorsInUse() const;
    // I-0079 (above): `desc.Alignment` is chosen here; `initial` is the state the resource is created in.
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateStreamed(D3D12_RESOURCE_DESC desc,D3D12_RESOURCE_STATES initial,PlacedKind kind);
    // `align`: a power of two (512 for a texture copy footprint). The range's bytes are not cleared.
    StagingRange AllocateStaging(uint64_t bytes,uint64_t align=D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
    StreamedMemoryCounters StreamedMemory() const;
    // I-0079: releases one fully free placed heap of each kind beyond the one kept (ER keeps one empty heap a list); the
    // count released. Called by the frame owner at a low rate. I-0084 (lead D1): bytes over the local or non-local budget
    // release free heaps (the kept spare last) or idle staging chunks until they are covered, the memory the backend holds on
    // its own account.
    size_t TrimStreamedMemory(uint64_t local_over=0,uint64_t nonlocal_over=0,bool no_world=false);
    struct PlacedPool;struct StagingPool;   // defined in backend.cpp (public for the range leases there)
    DeviceQueues();   // out of line: the ingress members hold an incomplete type
    DeviceQueues(const DeviceQueues&)=delete;
    DeviceQueues& operator=(const DeviceQueues&)=delete;
    IngressRecording RecordIngress();
    // Submits the committed ingress lists alone; 0 when none is waiting.
    uint64_t SubmitIngress();
    // I-0082: `debug` asks for the debug layer; without the Graphics Tools the device opens without it (DebugLayerActive).
    void Open(bool warp, bool debug);
    bool DebugLayerActive() const noexcept {return debug_layer_active_;}
    bool RemovalDataActive() const noexcept {return removal_data_active_;}
    // I-0132 R11f: why the Agility SDK's device factory was not used (empty when it was, or when none was asked).
    const std::string& AgilityFailure() const noexcept {return agility_failure_;}
    bool DxgiDebug() const noexcept {return dxgi_debug_;}   // the final review's L7: the DXGI factory made with its debug flag
    // I-0109 (R0, RT-PT-PLAN §3.1): what the device offers the ray-tracing path, read once by Open (start row native_device_caps).
    const DeviceCapabilities& Capabilities() const noexcept {return capabilities_;}
    uint64_t Execute(ID3D12CommandList* list);
    // I-0074 point 3c: closed lists submitted in this order, after the committed ingress lists, in one ExecuteCommandLists.
    uint64_t Execute(std::span<ID3D12CommandList* const> lists);
    uint64_t Drain(uint8_t site=WaitOther);
    // Phase D cD6: a fence signal after what was submitted so far (Present included), without waiting: the ticket of a frame
    // slot when frames are in flight.
    uint64_t Signal();
    void Complete(uint64_t value,uint8_t site=WaitOther);
    // Phase D cD4: the same wait on the caller's own event (FrameRing::WaitNext), so a concurrent Complete on the shared
    // event can neither consume nor be consumed by its wake-up (the R4 race of the phase D sheet for this one waiter).
    void CompleteOn(uint64_t value,HANDLE event,uint8_t site=WaitOther);
    void Close();
    // D3D12 review R3 (phase B): an owner retired after a submission ticket is kept until the queue fence passes that
    // ticket, then released by the next CollectCompleted (called by the owner of the frame). No wait. Close releases all
    // after its drain; a quarantine keeps them to process exit.
    void RetireAfter(uint64_t ticket,std::shared_ptr<void> owner);
    size_t CollectCompleted();
    size_t PendingRetirements() const;
    // A lower bound under concurrency (another thread may be submitting): retiring at it is safe only because every list
    // that uses an owner holds it itself (rule L2).
    uint64_t LastSubmitted() const noexcept {return submitted_;}
    uint64_t CompletedValue() const;   // the queue fence's completed value (throws when the device is removed)
    void QuarantineToProcessExit() noexcept;
    ~DeviceQueues();
    Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    Microsoft::WRL::ComPtr<ID3D12Device5> device5;   // I-0109: the ray-tracing interface; null on a runtime without it
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
    Microsoft::WRL::ComPtr<ID3D12InfoQueue> diagnostics;
    // D3D12 review phase A (D3D12-PHASES-A-C-PREP 3.2): submissions and CPU waits on this queue since start.
    std::atomic<uint64_t> executes{},drains{},completes{};
    std::atomic<uint64_t> ingress_lists{};   // B3a: ingress lists submitted
private:
    friend class IngressRecording;
    struct IngressPool;
    struct DescriptorPool;

    std::shared_ptr<DescriptorPool> descriptors_;
    std::shared_ptr<PlacedPool> placed_textures_,placed_small_,placed_buffers_;   // placed_small_: 4 KB-placed textures (ER keeps them apart)
    std::shared_ptr<StagingPool> staging_;
    uint64_t SubmitWithIngress(std::span<ID3D12CommandList* const> lists);
    void Wait(uint64_t value);   // the completion wait of Drain and Complete (5 s, a timeout is a fault)
    static void RecycleIngress(const std::weak_ptr<IngressPool>& pool,std::unique_ptr<IngressRecording::Pending> pending) noexcept;
    std::shared_ptr<IngressPool> ingress_;
    std::mutex ingress_lock_;
    std::vector<std::unique_ptr<IngressRecording::Pending>> committed_;   // under ingress_lock_, in commit order
    uint64_t ingress_serial_{};
    Microsoft::WRL::ComPtr<ID3D12InfoQueue1> callbacks_;
    DWORD callback_cookie_{};
    bool callback_registered_{},debug_layer_active_{},removal_data_active_{};
    DeviceCapabilities capabilities_{};std::string agility_failure_;bool dxgi_debug_{};
    uint64_t completion_deadline_ms_{5000};   // I-0084: 120 s on WARP, whose frames can take seconds
    Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
    // R13 cR1 (= phase D cD-F; R13/R14 sheet 5.2): any thread may submit and wait. One submission mutex covers the ingress
    // swap, ExecuteCommandLists, the ticket, the Signal, the submitted ticket and the retirement of the ingress lists, and
    // the ticket and Signal of Drain and Signal(), so tickets reach the fence in order and ingress lists precede the list
    // of the same submission. Each wait takes its own event from a pool (no waiter consumes another's wake-up) and waits
    // again until its deadline after a wake-up below its value; an event whose wait expired is never reused or closed.
    // Lock order: submit_mutex_ before ingress_lock_ and retire_lock_; events_lock_ is a leaf; no wait under submit_mutex_.
    std::mutex submit_mutex_;
    std::mutex events_lock_;
    std::vector<HANDLE> free_events_;
    void WaitOn(uint64_t value,HANDLE event);   // the shared wait loop (5 s deadline, a timeout is a fault)
    std::atomic<uint64_t> next_{1},submitted_{};   // read by retirement from other threads
    mutable std::mutex retire_lock_;
    std::vector<std::pair<uint64_t,std::shared_ptr<void>>> retired_;
};

// D3D12 review R2 (phase B2c): linear upload ranges for data written once per submission (lighting, environment, SSS
// and velocity constants, point-light batches), replacing buffers rewritten in place that were safe only because every
// frame drained. Persistently mapped UPLOAD chunks (I-0132 R11f: GPU upload heaps where the device offers them, UploadHeapType;
// the CPU only writes them); a range is aligned (256 bytes unless asked for more) and zero-filled
// to its requested size unless its writer fills it whole. Retire(queues, ticket) hands every chunk used since the previous
// call to the queue: it returns to the free list of its size class when the fence passes the ticket (CollectCompleted).
// Phase D cD3 (sheet A2): chunks come in size classes of 1, 4, 16 and 64 MB, each class pooled up to 16, 4, 2 and 1
// chunks (128 MB at most); only a range above 64 MB takes a chunk of its own, never pooled. One recording thread; chunks
// may return from any thread.
class UploadArena {
public:
    // `resource` and `offset` locate the range in its chunk (a copy source, B2d); `gpu` is the resource address + offset.
    struct Range {uint8_t* cpu{};D3D12_GPU_VIRTUAL_ADDRESS gpu{};uint64_t bytes{};ID3D12Resource* resource{};uint64_t offset{};};
    static constexpr uint64_t chunk_bytes=1ull<<20,alignment=256;
    static constexpr std::array<uint64_t,4> class_bytes{1ull<<20,4ull<<20,16ull<<20,64ull<<20};
    static constexpr std::array<size_t,4> class_pooled{16,4,2,1};
    UploadArena();
    ~UploadArena();
    UploadArena(const UploadArena&)=delete;
    UploadArena& operator=(const UploadArena&)=delete;
    // `align`: a power of two, at least 256 (512 for texture copy footprints). `zero` false: the writer fills every byte.
    Range Allocate(ID3D12Device* device,uint64_t bytes,uint64_t align=alignment,bool zero=true);
    void Retire(DeviceQueues& queues,uint64_t ticket);
    void Reset() noexcept;                     // after the queue drained: chunks in use and the free list released
    void QuarantineToProcessExit() noexcept;   // unproved completion: chunks in use kept to process exit
    size_t FreeChunks() const;                 // pooled chunks of every class
    size_t FreeChunks(size_t size_class) const;
    size_t UsedChunks() const noexcept {return used_.size();}   // handed out since the last Retire
private:
    struct Chunk;struct Pool;
    std::shared_ptr<Pool> pool_;
    std::vector<std::unique_ptr<Chunk>> used_;   // since the last Retire; back() takes the next range
};
// D3D12 phase D cD5 (sheet section 5, B5 in its pooled form): immutable versions of the dynamic streams (physical model
// streams, cloth copies). An engine thread publishes a version: its bytes go into a persistently mapped UPLOAD buffer (I-0132
// R11f: a GPU upload heap's where offered, UploadHeapType)
// taken from a pool keyed by size, created only when the pool has none (warm-up, a new size). A version is immutable;
// whoever binds its buffer for drawing holds the version (the geometry binding keeps it), so the buffer returns to the
// pool only when the last holder is gone and, through RetireWhenSubmittedWorkCompletes, after the queue completed what
// was submitted by then (at once inside a completed slot's release). Any thread; the pool is internally locked.
struct UploadVersion {
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    uint64_t bytes{};   // the published size (the buffer is at least this, rounded to its size class)
    UploadVersion()=default;
    UploadVersion(const UploadVersion&)=delete;
    UploadVersion& operator=(const UploadVersion&)=delete;
    ~UploadVersion();
private:
    friend class UploadVersionPool;
    struct Pool;std::weak_ptr<Pool> pool_;uint8_t* memory_{};uint64_t capacity_{};ID3D12Device* device_{};
};
class UploadVersionPool {
public:
    static constexpr uint64_t granularity=4096;          // buffer sizes are rounded to it (the size class)
    static constexpr size_t pooled_per_size=64;           // free buffers kept per size class (several streams share one)
    static constexpr uint64_t pooled_bytes=64ull<<20;     // and in all
    UploadVersionPool();
    std::shared_ptr<const UploadVersion> Publish(ID3D12Device* device,const void* data,size_t bytes);
    size_t FreeBuffers() const;
    uint64_t Created() const noexcept;
    void Reset() noexcept;   // after the queue drained (close): free buffers released; live versions return nowhere
private:
    std::shared_ptr<UploadVersion::Pool> pool_;
};
// Process totals of all arenas (pacing row): chunks created and bytes handed out.
uint64_t UploadArenaChunksCreated() noexcept;
uint64_t UploadArenaBytesAllocated() noexcept;

// I-0072 (D-0172, D-0195; ER-PIPELINE-STUDY): the device's pipeline cache, after Elden Ring's. ER keeps one cache per GX
// device (a map read without lock and a pending map behind the cache mutex), a pipeline library on disk checked against the
// adapter, global root signatures held by the device, and a precompile list ("PSC") of descriptions that a loading step
// creates through the library. DS3 keeps that shape and differs where ER stutters: a miss never holds the cache lock while the
// driver compiles (one thread builds a given name, the others wait for that name only), the precompile list is written by the
// cache itself from every description it creates (ER's capture tooling, always on), and the precompile runs on worker threads
// from the device's creation and keeps its pipelines in memory, so a pipeline already seen on this machine is not compiled on
// the frame path.
// I-0113 gap 4 (D-0241): DS3 does not keep ER's library. On this PC (NVIDIA, driver 32.0.16.1714), in a start that runs the
// I-0109 ray-tracing self-test, a pipeline loaded from an ID3D12PipelineLibrary samples through its root signature's static
// samplers as if they were point samplers: the grading's 16-cube LUT read at its nearest cell (the dark areas posterised, red and
// blue patches) and the glow in blocks (K-0486); a library also gave I-0100's grey title. The precompile compiles every listed
// description instead, through the driver's own shader cache: 227 pipelines in 40 to 72 ms against 32 ms loaded, still on the
// workers and off the frame path.
// A pipeline is named by its whole description (its root signature's content (I-0132: the RTS0 chunk), programme bytes, every fixed-function field, the input
// layout), so a name never stands for another description; the list (`native-pipelines.v3.psc`) holds the descriptions, the
// root signature blobs and the programme bytes (the game's own programmes, copied from what the engine loaded on this machine),
// with a checksum. The records name themselves, so a list made by another module, adapter or driver stays valid. Without Open
// (or no-pipeline-library.flag, a historical name) the pipelines are cached in memory only; a root signature not created by Root, or a description
// the list cannot hold (stream output, cached blobs), is created directly.
// I-0132 R11i: the Agility SDK 1.620 debug layer refuses a graphics description that names a depth-stencil format it does not use
// (« DSVFormat should be DXGI_FORMAT_UNKNOWN when depth-stencil is not used »: the depth and stencil tests off and the pixel shader
// not writing SV_Depth), which the inbox layer accepts. One rule for every description, where the store makes them (Acquire, the
// precompile's included): such a format becomes DXGI_FORMAT_UNKNOWN, so the pipeline is named and compiled without it.
// PixelShaderWritesDepth reads the programme's output signature (a DXBC or DXIL container's OSGN, OSG5 or OSG1 chunk: an
// SV_Depth, SV_DepthGreaterEqual or SV_DepthLessEqual element); an unreadable container counts as writing depth (the format kept).
bool PixelShaderWritesDepth(const D3D12_SHADER_BYTECODE& ps) noexcept;
bool UnusedDepthStencilFormat(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& d) noexcept;
class PipelineStore {
public:
    PipelineStore()=default;
    PipelineStore(const PipelineStore&)=delete;
    PipelineStore& operator=(const PipelineStore&)=delete;
    // Opens the precompile list `path` for `device`; `enabled` false keeps it closed (memory cache only).
    void Open(ID3D12Device* device,std::wstring path,bool enabled=true);
    bool IsOpen() const;
    // The root signature of a serialised description (a bare root or a DXIL programme embedding one), one object per root and
    // device (ER's roots held by the device), the root named by its content (the RTS0 chunk, I-0132: never by the blob carrying it,
    // which changes with a programme's code): the pipelines of a root made here are cached and listed.
    HRESULT Root(ID3D12Device* device,const void* blob,size_t size,ID3D12RootSignature** out);
    // `created` (optional): the driver compiled the pipeline in this call (the memory cache did not have it).
    HRESULT Graphics(ID3D12Device* device,const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc,ID3D12PipelineState** out,bool* created=nullptr);
    HRESULT Compute(ID3D12Device* device,const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc,ID3D12PipelineState** out,bool* created=nullptr);
    // Compiles every listed pipeline on `workers` threads below normal priority; a frame asking for a pipeline being built waits
    // for that pipeline only. Close cancels it.
    void Precompile(unsigned workers);
    // Writes the list when it changed since the last save; returns whether the file was written.
    bool Save();
    // Save on the store's own thread (one at a time; a save still running is left to finish): the pacing window's call. Close
    // joins it, so no save runs into the process's end (review: a detached thread would).
    void SaveAsync();
    void Close() noexcept;   // cancels the precompile, joins a running save, saves, then releases everything (the device may go afterwards)
    struct Counters {uint64_t created{},saves{},unsaved{},memory_hits{},direct{},waits{},records{},list_bytes{},
        precompile_total{},precompiled{},precompile_failed{},precompile_ms{},frame_created{},pruned{},roots_renamed{},records_merged{},root_aliases{};
        bool list_loaded{},open{},precompile_done{};};   // frame_created: by requests other than the precompile's; roots_renamed and
        // records_merged: a version-3 list's roots renamed by content at Open, and its records made one by it; root_aliases: one root
        // object returned for two contents (expected 0, I-0132)
    Counters Stats() const;
    struct Name {uint64_t high{},low{};bool operator==(const Name&) const=default;};
    struct NameHash {size_t operator()(const Name& n) const noexcept {return static_cast<size_t>(n.high^(n.low*0x9E3779B97F4A7C15ULL));}};
private:
    struct RootEntry {std::vector<uint8_t> blob;Microsoft::WRL::ComPtr<ID3D12RootSignature> root;};
    // idle: starts since a request other than the precompile's asked for it (a record idle for max_idle starts leaves the list:
    // descriptions no build asks for any more).
    struct Record {std::vector<uint8_t> encoding;uint8_t idle{};};
    static constexpr uint8_t max_idle=8;
    HRESULT Acquire(ID3D12Device* device,const D3D12_GRAPHICS_PIPELINE_STATE_DESC* graphics,const D3D12_COMPUTE_PIPELINE_STATE_DESC* compute,
        ID3D12PipelineState** out,bool* created,bool precompile,uint8_t idle_if_new=0);   // idle_if_new: a migrated record's age (I-0132 R11i)
    // Under lock_; `stages` are the five (graphics) or one (compute) programme names, hashed before the lock (review L1).
    bool Encode(const D3D12_GRAPHICS_PIPELINE_STATE_DESC* graphics,const D3D12_COMPUTE_PIPELINE_STATE_DESC* compute,const std::array<Name,5>& stages,std::vector<uint8_t>& out);
    void PrecompileWorker();
    void NameRoot(ID3D12RootSignature* root,const Name& name);   // under lock_
    mutable std::mutex lock_;std::condition_variable built_;
    Microsoft::WRL::ComPtr<ID3D12Device> device_;bool open_{};
    std::unordered_map<Name,RootEntry,NameHash> roots_;std::unordered_map<ID3D12RootSignature*,Name> root_names_;
    std::unordered_map<Name,std::shared_ptr<const std::vector<uint8_t>>,NameHash> programmes_;
    std::unordered_map<Name,Record,NameHash> records_;   // the list
    std::unordered_map<Name,Microsoft::WRL::ComPtr<ID3D12PipelineState>,NameHash> pipelines_;   // the memory cache (ER's main map)
    std::unordered_set<Name,NameHash> building_;   // names a thread is compiling (each name built once)
    std::wstring path_;Counters counters_;uint64_t unsaved_{};
    std::thread saver_;std::atomic<bool> saving_{};   // SaveAsync's thread (joined by SaveAsync when done, and by Close)
    std::vector<std::thread> precompilers_;std::vector<std::pair<Name,std::vector<uint8_t>>> precompile_list_;bool precompiling_{};
    std::atomic<size_t> precompile_next_{},precompile_left_{};std::atomic<bool> precompile_cancel_{};LARGE_INTEGER precompile_start_{};
};
// The process's store (process lifetime by design, as the model draw caches). I-0084 (lead A2): every pipeline and root
// signature of the game_bootstrap path goes through it (the fixture paths keep direct creations), the stages' and passes' included (they compiled at their first frame on every
// start, outside the precompile list), as ER keeps one cache per device.
PipelineStore& Pipelines();
struct FrameRecorder {
    void Open(ID3D12Device* device);
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list;
};
// D3D12 phase D cD1 (D3D12-PHASE-D-IMPLEMENTATION-SHEET 3.1, complement 2.1 and 4.4): the frames being recorded and in
// flight. A slot owns its allocator and list and holds what its list references (model draws, and the GPU-referenced
// frame inputs the caller does not own: movie and view targets, environment cubes) until the queue passes its ticket.
// Owner thread only. With one slot (cD1 to cD5) End waits for the slot's ticket and releases it at once, as before the
// ring; with more, Begin waits only for the slot it reuses.
constexpr uint32_t max_frame_slots=3;
struct FrameSlot {
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list;
    uint64_t ticket{};   // the ticket covering this slot's last submission (0 = nothing in flight)
    // Rule L2 (sheet 3.3): every later list that references an object holds it itself; ReleaseAfterCompletion relies on it,
    // so nothing GPU-referenced through a raw pointer or descriptor alone may be put here without that holder.
    std::vector<std::shared_ptr<const void>> draws,kept;
    // Phase D cD3 (complement 2.1(b), T-D11): resources the caller passed as raw pointers into this slot's list (movie and
    // view targets); each must be held in `kept` before the list is submitted (FrameRing::RequireHeld).
    std::vector<const ID3D12Resource*> referenced;
    // Phase D cD4 (sheet G1-G4, R6): work that needs this slot's GPU results (witness readbacks, statistics, diagnostics),
    // run in order when the slot is released, i.e. after its ticket completed (at End with one slot, at the slot's next
    // Begin or WaitNext with more), before its draws and kept objects go.
    std::vector<std::function<void()>> completions;
    // I-0074 point 3c: further lists of this slot's frame (the recording chunks and the list that continues after them), each
    // with its own allocator, reset only when taken again in a later recording of the slot (the slot's ticket then passed).
    struct Extra {Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list;};
    std::vector<Extra> extras;size_t extras_used{};
};
// A COM resource held as a slot's kept object (released with the slot).
std::shared_ptr<const void> HoldResource(ID3D12Resource* resource);
// Optimisation review axis 2: any COM object (a descriptor heap, a resource) held for RetireWhenSubmittedWorkCompletes, so a
// stage replaces a heap or a surface an earlier frame may still read without draining the queue.
std::shared_ptr<void> HoldObject(IUnknown* object);
class FrameRing {
public:
    FrameRing();   // I-0090: out of line (the release thread's type is complete in backend.cpp)
    FrameRing(const FrameRing&)=delete;
    FrameRing& operator=(const FrameRing&)=delete;
    void Open(ID3D12Device* device,uint32_t slots);
    uint32_t Slots() const noexcept {return static_cast<uint32_t>(slots_.size());}
    bool Recording() const noexcept {return recording_;}
    // The next slot: its previous ticket waited for (a `Complete` at `site` only when the queue has not passed it yet), its
    // contents released, its allocator and list reset (the list is open, with `initial` as its pipeline). Refused while
    // a slot is recording (a recording that threw stays in its slot until the renderer closes or quarantines).
    FrameSlot& Begin(DeviceQueues& queues,uint8_t site,ID3D12PipelineState* initial=nullptr);
    // Phase D cD3: the slot being recorded (one list per frame: the movie, the menu views and the frame record into it).
    FrameSlot& Current();
    // I-0074 point 3c: the slot's next extra list, open (reset on its allocator, or created), for the recording in progress.
    ID3D12GraphicsCommandList* ExtraList(ID3D12Device* device,FrameSlot& slot);
    uint32_t CurrentIndex() const noexcept {return static_cast<uint32_t>(next_);}   // the recording (or next) slot
    uint64_t Frames() const noexcept {return frames_;}                              // Begin calls since Open
    // C4-2a: every Begin numbered over the ring's life (not reset by Close), and the frames known finished: a Begin waits for
    // the frame recorded Slots() earlier in its slot (in queue order, so for every earlier one), and Close follows a drain.
    uint64_t Numbered() const noexcept {return numbered_;}
    uint64_t CompletedThrough() const noexcept {return numbered_>Slots()?numbered_-Slots():0;}
    // Refuses a slot whose `referenced` resources are not all held in `kept` (complement 2.1(b)); before its Execute.
    static void RequireHeld(const FrameSlot& slot);
    // The slot's submission ticket (non-zero, after its Execute or a later Signal). With one slot the ticket is waited for
    // (as Begin) and the slot released here.
    void End(DeviceQueues& queues,FrameSlot& slot,uint64_t ticket,uint8_t site);
    void WaitIdle(DeviceQueues& queues,uint8_t site);   // every slot waited for and released
    // Phase D cD4 (sheet R5): the wait of the next Begin, done ahead (before the caller's submission lock): the slot Begin will
    // reuse is waited for on the ring's own event (no other waiter can consume its wake-up); Begin releases it. Refused while
    // recording.
    void WaitNext(DeviceQueues& queues,uint8_t site);
    // R13 cR3: atomic, so the owner thread's pacing row takes them while the render worker waits and releases.
    double TakeWaitMs() noexcept {return wait_ms_.exchange(0);}   // slot_wait_ms since the last call
    double TakeReleaseMs() noexcept {return release_ms_.exchange(0);}   // cD6: time spent releasing slots
    // I-0090 (the frame's per-frame objects off the recording thread): with deferred release, a retired slot's holders (its
    // draws and kept objects, the frame's per-frame objects) are swapped out to the ring's release thread, which destroys them
    // (their tickets have passed); the recording thread no longer runs their destructors. Off by default (the release stays in
    // Retire); with four batches waiting (five frames' holders with the one being destroyed) the next is released in Retire. Flush waits until every handed batch is destroyed; WaitIdle and
    // Close flush.
    void SetDeferredRelease(bool deferred) noexcept {deferred_release_=deferred;}
    void Flush() noexcept;
    double TakeReleaseThreadMs() noexcept {return release_thread_ms_.exchange(0);}   // the release thread's destruction time
    void Close() noexcept;                              // after the queue drained: everything released
    void QuarantineToProcessExit() noexcept;            // unproved completion: lists, allocators and contents kept
private:
    void Retire(FrameSlot& slot);
    bool HandOff(FrameSlot& slot) noexcept;
    void StopReleaser() noexcept;
    struct Releaser;std::unique_ptr<Releaser> releaser_;bool deferred_release_{};
    std::vector<FrameSlot> slots_;
    size_t next_{};
    uint64_t frames_{},numbered_{};
    bool recording_{};
    std::atomic<double> wait_ms_{},release_ms_{},release_thread_ms_{};
    HANDLE event_{};   // WaitNext's own completion event
    void WaitSlot(DeviceQueues& queues,FrameSlot& slot,uint8_t site,bool release=true);
public:
    ~FrameRing();
};
// I-0101: the presentation's statuses that mean a lost device (a refusal with its DRED rows); any other failure is followed.
inline bool IsDeviceLoss(HRESULT status) noexcept {
    return status==DXGI_ERROR_DEVICE_REMOVED || status==DXGI_ERROR_DEVICE_RESET || status==DXGI_ERROR_DEVICE_HUNG || status==DXGI_ERROR_DRIVER_INTERNAL_ERROR;
}
// I-0138 tranche 5: IDXGIOutput6::GetDesc1 of the screen the window is on, through a factory made for the call (a factory
// made earlier keeps the screens' state of its creation); `found` false when no output of any adapter holds the window's monitor.
// `factory`: that factory, kept by the caller to ask it later whether it is still current (IDXGIFactory1::IsCurrent: false once a
// screen was plugged or Windows' HDR switched).
hdr::Display ProbeHdrDisplay(HWND window,IDXGIFactory1** factory=nullptr) noexcept;
struct FramePresenter {
    void Open(DeviceQueues& owner, HWND window, uint32_t width, uint32_t height);
    void Resize(DeviceQueues& owner, uint32_t width, uint32_t height);
    // I-0101 (the live run i101-fs-alttab-1: ResizeBuffers answered E_ACCESSDENIED after a focus loss): ResizeBuffers at the
    // current size; on a failure the chain is unchanged and its buffers are taken again (or stand-ins when it refuses them,
    // I-0111, below), the caller deciding (ER FUN_141E9F100
    // goes on after a failed resize, as here). That the old buffers survive a failed ResizeBuffers is ER's premise, not documented
    // by DXGI; a lost device returns before they are taken again.
    HRESULT TryResizeBuffers(DeviceQueues& owner);
    // I-0111: the chain's buffers taken again without throwing (false: the chain refused one, or, I-0137, they are not of this
    // presenter's size after a refused resize to another one), and, when they are not taken, committed textures of the
    // presenter's size and the chain's format in their place (`stand_in`), until a resize succeeds.
    bool TryAcquire(ID3D12Device* device);
    void StandIn(ID3D12Device* device);
    bool stand_in{};
    HRESULT ResizeChain(uint32_t width, uint32_t height);   // ResizeBuffers with the creation's flags, the buffers released
    // I-0138 tranche 5 (HDR10): `hdr_wanted`, set before Open and before any resize, asks for an R10G10B10A2_UNORM chain whose
    // colour space is ST 2084 over Rec. 2020 (DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020); Open and every resize that DXGI
    // accepts leave `format` (the buffers', the stand-ins' too) and `hdr10` (the colour space taken). A chain that refuses the
    // colour space is made 8-bit again, `hdr_failure` saying why. The frames choose their last pass by `format` (Hdr10Format).
    // No HDR metadata is set (IDXGISwapChain4::SetHDRMetaData: Microsoft's page advises against it; the output is tone-mapped
    // to the screen here).
    bool hdr_wanted{},hdr10{};
    // The step review's L1: set when this chain refused the colour space (Open clears it): later resizes ask 8-bit at once. When
    // the 8-bit resize that follows a refusal is itself refused, `format` is 8-bit all the same and TryAcquire, which does not
    // take buffers of another format, leaves 8-bit stand-ins until a resize succeeds.
    bool hdr_refused{};
    DXGI_FORMAT format{DXGI_FORMAT_R8G8B8A8_UNORM};
    std::string hdr_failure;
    bool Hdr10Format() const noexcept {return format==DXGI_FORMAT_R10G10B10A2_UNORM;}
    void ApplyColorSpace();
    void Acquire(ID3D12Device* device);
    Microsoft::WRL::ComPtr<IDXGISwapChain3> swapchain;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtv;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> buffers;
    uint32_t width{}, height{}, stride{};
    // D3D12 phase D cD8 (R14a-b): set before Open. `count` back buffers (2 = the former chain); `waitable` creates the chain
    // with a frame-latency waitable object (`latency`, maximum latency `max_latency`): a frame waits on it before it records
    // instead of blocking later in Present or on the queue.
    uint32_t count{2}, max_latency{1};
    bool waitable{};
    // I-0088 P0 (measurement only, with original-unlock.flag): set before Open to ask for a chain that may tear; Open leaves it
    // true only when the factory supports DXGI_FEATURE_PRESENT_ALLOW_TEARING, so an interval-0 present in a window is not held
    // to the compositor's cadence (flip discard on a 60 Hz desktop otherwise caps the measured rate near 120).
    bool tearing{};
    HANDLE latency{};
    HWND presented{};   // the presented window (a hidden or minimized window retires no present: no latency wait then)
    // I-0111 (R1): set by Open: the chain is Streamline's proxy (DLSS ready), or why its factory upgrade was refused.
    bool upgraded{};std::string upgrade_failure;
    FramePresenter()=default;
    FramePresenter(const FramePresenter&)=delete;
    FramePresenter& operator=(const FramePresenter&)=delete;
    ~FramePresenter();
};

} // namespace ds3native
