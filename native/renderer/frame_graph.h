#pragma once
// Phase E E1 (D3D12-PHASE-E-FRAME-GRAPH-SHEET 5): what the frame graph derives from a compiled FramePlan, as pure CPU
// data: the state of every subresource at each use and the transitions that bring it there (StateTracker and
// BarrierPlanner), the first-use action of each transient, and a placement of the transients in heaps by lifetime
// (PlacementPlanner). No device call: sizes and alignments come from the caller (GetResourceAllocationInfo); the
// executor that records the batches is E3. States are D3D12_RESOURCE_STATES values.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "frame_plan.h"
#include <d3d12.h>
#include <array>
#include <cstdint>
#include <vector>

namespace ds3native {
// Transient: allocated by the graph, home state at the end of its activation (G2). Persistent: kept across frames, never
// placed (history, scene copy, shadow atlases). External: owned outside (swap chain, ingress records); the graph never
// allocates it. Persistent and external resources start and end in their declared boundary state.
enum class FrameLifetime : uint8_t {Transient,Persistent,External};
// Tier 1 heaps separate render-target and depth textures, other textures and buffers; Tier 2 needs one class.
enum class FrameHeapClass : uint8_t {Target,Texture,Buffer};
enum class FirstUse : uint8_t {None,Clear,Discard,CopyInit};
struct FrameResourceDesc {
    ResourceKey key;
    FrameLifetime lifetime{};
    FrameHeapClass heap_class{};
    uint32_t subresources{1};                  // planes x mips x slices (a depth-stencil texture: 2 planes)
    D3D12_RESOURCE_STATES home{};              // transient: the state between activations; others: the boundary state
    uint64_t size{},alignment{65536};          // GetResourceAllocationInfo of the resource (transients only)
    // E5: a persistent resource's state at the start of this frame when it differs from its boundary state (the first frame
    // after its creation starts from the creation state); the frame ends in `home`.
    bool start_declared{};D3D12_RESOURCE_STATES start{};
};
struct GraphTransition {ResourceKey key;uint32_t subresource{};D3D12_RESOURCE_STATES before{},after{};};   // subresource NoPlanIndex: all
struct BarrierBatch {std::vector<GraphTransition> transitions;std::vector<ResourceKey> uav;};
struct GraphFirstUse {ResourceKey key;uint32_t pass{};FirstUse action{};};
enum class GraphError : uint8_t {None,Plan,Description,BoundaryState,UnknownState,Placement};
struct GraphStatus {
    GraphError error{};
    uint32_t pass{NoPlanIndex},resource{NoPlanIndex};
    explicit operator bool() const noexcept {return error==GraphError::None;}
};
// One batch before each pass, one after each pass (G2: a transient goes back to its home state at the end of its
// activation, after its last pass, before its memory can be another resource's), one at the end (persistent and
// external resources back to their boundary state), and the first-use actions.
struct BarrierPlan {
    GraphStatus status;
    std::vector<BarrierBatch> before_pass,after_pass;
    BarrierBatch end;
    std::vector<GraphFirstUse> first_uses;
    // The state each tracked use found (after its batch), in pass and use order, for the shadow-mode comparison (E2).
    struct UseState {uint32_t pass{},use{},subresource{};D3D12_RESOURCE_STATES state{};};
    std::vector<UseState> use_states;
    size_t TransitionCount() const noexcept;
};
// A read-only state combination satisfies a read requirement it contains; any other requirement needs equality.
bool GraphStateSatisfies(D3D12_RESOURCE_STATES current,D3D12_RESOURCE_STATES required) noexcept;
// Every resource of the plan needs exactly one description; a use that is not `tracked` is left to its encoder. Transients
// start in their home state; persistent and external resources in their declared boundary state, and end in it. Refused
// at plan time (G8): an unknown state bit, a write state combined with any other bit, a write use with a read-only state,
// a range past the resource's subresources (UnknownState, Description).
BarrierPlan PlanFrameBarriers(const CompiledFramePlan& plan,const std::vector<FrameResourceDesc>& descs);

struct GraphPlacement {ResourceKey key;FrameHeapClass heap_class{};uint64_t offset{};};
struct PlacementPlan {
    GraphStatus status;
    std::vector<GraphPlacement> placements;     // transients only
    std::array<uint64_t,3> heap_size{};         // by FrameHeapClass (with one class under Tier 2: index 0)
    std::array<uint64_t,3> separate_size{};     // the same resources unplaced (each its own allocation), for comparison
};
// Greedy by decreasing size: each transient takes the lowest aligned offset free of every placed transient of its class
// whose lifetime (first to last pass, inclusive) overlaps its own. `tier2`: one heap for every class.
PlacementPlan PlanFramePlacement(const CompiledFramePlan& plan,const std::vector<FrameResourceDesc>& descs,bool tier2);
}
