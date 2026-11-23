#pragma once
// Phase E E2 (D3D12-PHASE-E-FRAME-GRAPH-SHEET 5.3, 7): the shadow mode. While a frame is recorded with its hand-written
// barriers, the encoders also declare the frame graph's plan (passes and their uses with the state each needs) and every
// hand-written transition is recorded. At the end of the frame the plan is compiled and planned (PlanFrameBarriers), and
// for every pass and subresource the state the graph gives the use is compared with the state the hand-written code has
// there; a hand-written transition whose `before` is not the recorded state, and a resource not back in its declared
// state at the end of the frame, are mismatches too. Recording thread only; no effect on the commands.
#include "frame_graph.h"
#include <cstdint>
#include <stdexcept>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace ds3native {
class GraphShadow {
public:
    // `list`: the frame's command list, where an executed plan records its barriers (E3).
    void Begin(uint64_t serial,ID3D12GraphicsCommandList* list=nullptr);
    bool Active() const noexcept {return active_;}
    // I-0074 point 3c: the list the plan's later barriers go to (the frame's recording continued in another list).
    void SetList(ID3D12GraphicsCommandList* list) noexcept {list_=list;}
    // E3 (the default since D-0171): after the declarations, `structure` (the flags that select the frame's passes) and the
    // declarations form the plan key. A plan the shadow compared with 0 mismatches for that key is then executed: each pass
    // opening records the previous pass's after-batch and its own before-batch (one ResourceBarrier call each), the end
    // records the last pass's and the end batch, and the hand-written transitions of declared resources are suppressed.
    // Without such a plan the frame keeps its hand-written barriers (and may become the cached plan). A pass that differs
    // from the executed plan's is an error (the key must select the passes).
    void SetExecutor(bool enabled) noexcept {executor_=enabled;}
    void Execute(uint64_t structure);
    bool Executing() const noexcept {return executing_!=nullptr;}
    bool Suppress(ID3D12Resource* resource) const noexcept {return executing_ && tracked_.contains(resource);}
    uint64_t ExecutedFrames() const noexcept {return executed_frames_;}
    uint64_t GraphBarriers() const noexcept {return graph_barriers_;}
    // A resource the plan tracks: its subresource count, its state at the start and end of the frame, its lifetime class.
    // `output`: the recording's presented resource (the back buffer, a menu view's or the movie's target), read by its
    // final Present pass in its boundary state.
    void Declare(ID3D12Resource* resource,uint32_t subresources,D3D12_RESOURCE_STATES home,FrameLifetime lifetime,const char* name,bool output=false);
    // E5: a resource whose state at the start of this recording is `start`, and `home` at its end (a persistent resource's
    // first recording after its creation).
    void DeclareStarting(ID3D12Resource* resource,D3D12_RESOURCE_STATES start,D3D12_RESOURCE_STATES home,FrameLifetime lifetime,const char* name);
    // A hand-written transition (subresource NoPlanIndex: all), from the Transition helpers.
    void Transition(ID3D12Resource* resource,uint32_t subresource,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after,uint32_t line);
    // Opens the next pass (in recording order); the uses that follow are its own.
    void Pass(const char* name,PassKind kind);
    void Use(ID3D12Resource* resource,ResourceAccess access,D3D12_RESOURCE_STATES required,SubresourceRange range={});
    // Compiles and plans, compares, and returns the mismatch rows of this frame (empty when equal).
    std::vector<std::string> End();
    uint64_t Frames() const noexcept {return frames_;}
    uint64_t Uses() const noexcept {return uses_;}
    // Since start: how many frames opened each pass, and the hand-written transitions of resources the plan does not declare
    // by call line (a plan that misses a resource shows here, since a zero-mismatch result cannot see a missing use).
    std::string Summary() const;
private:
    struct Tracked {uint32_t index{};std::vector<D3D12_RESOURCE_STATES> state;};
    struct Recorded {uint32_t pass{},use{},subresource{};D3D12_RESOURCE_STATES hand{};};
    bool active_{};uint64_t serial_{},frames_{},uses_{};
    std::vector<FrameResourceDesc> descs_;std::vector<std::string> names_;
    std::unordered_map<ID3D12Resource*,Tracked> tracked_;
    FramePlan plan_;std::vector<std::string> pass_names_;
    std::vector<Recorded> recorded_;
    std::vector<std::string> mismatches_;
    std::vector<std::pair<std::string,uint64_t>> pass_hits_;
    // E3 review: the key holds the declaration slots (name, subresources, home, lifetime), not the resource pointers; the
    // cached plan's pointers are those of the frame that cached it (`slots`), replaced by this frame's at execution.
    // Optimisation review axis 3: `verified` counts the executed frames of this plan whose full comparison found no mismatch;
    // once it reaches `verified_before_sampling` the comparison (the plan compiled and planned again, every use compared) runs
    // on one executed frame in `sampling_period` only; the other frames record no use and compile nothing.
    // `executions` counts the plan's own executed frames (pre-merge review D2: the global frame count alternates between the
    // frame's recordings, so one recording's plan would never be sampled).
    struct CachedPlan {std::vector<uint64_t> key;std::vector<std::string> pass_names;BarrierPlan barriers;std::vector<uint64_t> slots;uint64_t last_used{},verified{},executions{};};
    std::vector<CachedPlan> cache_;std::vector<uint64_t> key_;
    bool executor_{};CachedPlan* executing_{};ID3D12GraphicsCommandList* list_{};size_t executed_passes_{};
    uint64_t executed_frames_{},graph_barriers_{},barrier_calls_{},evicted_{};
    bool verify_{true};uint64_t compared_frames_{},sampled_out_frames_{};
    void Apply(const BarrierBatch& first,const BarrierBatch* second=nullptr);
public:
    static constexpr uint64_t verified_before_sampling=8,sampling_period=64;
    uint64_t BarrierCalls() const noexcept {return barrier_calls_;}   // ResourceBarrier calls of executed frames
    uint64_t EvictedPlans() const noexcept {return evicted_;}        // plans dropped after an executed frame mismatched
    uint64_t ComparedFrames() const noexcept {return compared_frames_;}       // frames whose plan was compiled and compared
    uint64_t SampledOutFrames() const noexcept {return sampled_out_frames_;}  // executed frames that skipped the comparison
private:
    std::vector<std::pair<uint32_t,uint64_t>> untracked_lines_;
    void Mismatch(std::string row);
};
// The shadow of the frame being recorded on this thread (null outside a shadowed frame).
inline thread_local GraphShadow* active_graph_shadow{};
}
