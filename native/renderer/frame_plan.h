#pragma once
#include <cstdint>
#include <limits>
#include <vector>

namespace ds3native {

// Native-owned whole-resource intent, independent of graphics API call streams.
// These are backend contracts, not recovered DS3 layouts or complete game coverage.
struct ResourceKey {
    uint64_t identity{},generation{};
    bool operator==(const ResourceKey&) const = default;
};
enum class ResourceKind : uint8_t { Buffer,Texture,Color,Depth,Presentation };
// Phase E E1: PostProcess (master plan S2) before Present, which stays the last kind.
enum class PassKind : uint8_t { Upload,Clear,Depth,Shadow,Material,Lighting,Effect,SceneResolve,Composition,Ui,FinalDisplay,Readback,Environment,PrimitiveStencilClear,PostProcess,Present };
enum class ResourceAccess : uint8_t { Read,Write,ReadWrite };
struct FrameResource {
    ResourceKey key;
    ResourceKind kind{};
    // Only the native owner may attest to completed uploads/preserved values.
    bool initialized{};
};
// Phase E E1 (D3D12-PHASE-E-FRAME-GRAPH-SHEET 5.1): a use names a range of subresources (default: all of them) and may
// carry the state the frame graph must give it (below). Two uses of one resource in one pass are
// accepted when their ranges are disjoint (the depth plane read as a shader resource while the stencil plane stays
// writable through a read-only DSV); overlapping ranges are a DuplicateUse.
struct SubresourceRange {
    uint32_t first{},count{std::numeric_limits<uint32_t>::max()};   // count max: from `first` to the last subresource
    bool Overlaps(const SubresourceRange& o) const noexcept {
        const uint64_t a_end=count==std::numeric_limits<uint32_t>::max()?UINT64_MAX:uint64_t(first)+count;
        const uint64_t b_end=o.count==std::numeric_limits<uint32_t>::max()?UINT64_MAX:uint64_t(o.first)+o.count;
        return first<b_end && o.first<a_end;}
    bool operator==(const SubresourceRange&) const = default;
};
// `tracked`: the frame graph brings the range to `required_state` (a D3D12_RESOURCE_STATES value, 0 = COMMON/PRESENT) before
// the pass; untracked uses are left to their encoder.
struct PassResource { ResourceKey key; ResourceAccess access{}; SubresourceRange range{}; uint32_t required_state{}; bool tracked{}; };
struct FramePass {
    uint32_t identity{};
    PassKind kind{};
    std::vector<uint32_t> after;
    std::vector<PassResource> resources;
};
struct FramePlan {
    uint64_t serial{};
    std::vector<FrameResource> resources;
    // Explicit semantic order. Compilation checks dependency edges, not an
    // inferred legacy pass order. Native encoders consume this immutable copy.
    std::vector<FramePass> passes;
};
constexpr uint32_t NoPlanIndex=std::numeric_limits<uint32_t>::max();
// Phase E E1 (master plan S2): passes per plan and dependencies per pass; the largest pass count compiled since the last call.
constexpr size_t MaxPlanPasses=256,MaxPassDependencies=256;
uint64_t TakePlanPassesPeak() noexcept;
enum class PlanError : uint8_t {
    None,Extent,ResourceIdentity,PassIdentity,Kind,Dependency,ResourceVersion,
    DuplicateUse,UninitializedRead,MissingDependency,Presentation
};
struct PlanStatus {
    PlanError error{};
    uint32_t pass{NoPlanIndex},resource{NoPlanIndex};
    explicit operator bool() const noexcept {return error==PlanError::None;}
};
struct ResourceLifetime { ResourceKey key; uint32_t first{},last{}; };
class CompiledFramePlan {
public:
    const FramePlan& Plan() const noexcept {return plan_;}
    const std::vector<ResourceLifetime>& Lifetimes() const noexcept {return lifetimes_;}
    PlanStatus Status() const noexcept {return status_;}
    explicit operator bool() const noexcept {return static_cast<bool>(status_);}
private:
    friend CompiledFramePlan CompileFramePlan(const FramePlan&);
    FramePlan plan_;
    std::vector<ResourceLifetime> lifetimes_;
    PlanStatus status_{PlanError::Extent};
};
// Checks versioned inputs, RAW/WAR/WAW dependencies and one final presentation.
// It allocates CPU metadata only; physical residency/barriers/fences belong to
// the native execution owner and are not proved by a successfully compiled plan.
CompiledFramePlan CompileFramePlan(const FramePlan& input);
} // namespace ds3native
