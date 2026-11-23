#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <vector>

namespace ds3content {
using ModelPoseRow=std::array<float,12>;
enum class ModelPoseReference { Unknown, Null, Present };
enum class ModelPoseStatus {
    // Explicit values (the journals log them as numbers; 2 was InvalidInput until I-0113 tranche 10d).
    Ready=0, MissingInput=1, Absent=3, Empty=4,
    UnsupportedNongated=5, UnsupportedFast=6
};
struct ModelPoseInputs {
    uint32_t row_count{};
    bool gated{},compose{},auxiliary_present{},fast{};
    ModelPoseReference reference{ModelPoseReference::Unknown};
    bool gate_available{};
    std::span<const ModelPoseRow> current,reference_rows,gate_rows;
};
struct ModelPosePalette {
    ModelPoseStatus status{ModelPoseStatus::MissingInput};
    std::vector<ModelPoseRow> rows;
    bool Available() const noexcept { return status==ModelPoseStatus::Ready; }
};
struct ModelPoseVersion {
    uint64_t generation{};
    uint32_t row_count{};
    bool gated{},compose{},auxiliary_present{},fast{},gate_available{},cloth{};
    ModelPoseReference reference{ModelPoseReference::Unknown};
    std::vector<ModelPoseRow> current_source,reference_source,gate_source;
    ModelPosePalette current,auxiliary,cancel;
};

//16050's gated full-row CPU semantics. M=A/A*G and the auxiliary/cancel pair
// keep independent availability. A missing Q is never treated as explicit null.
// Fast15FC0 and nongated per-mesh packing are explicitly unsupported here.
// n is the meaningful row count, with no constructor max(n,2) padding or RDEF
// clipping. Missing/nonfinite inputs invalidate only dependent outputs; invalid
// metadata/count or allocation failure leaves output unchanged. No GPU work,
// shader palette-kind choice or vertex index-range admission occurs here.
bool ProduceModelPose(const ModelPoseInputs& inputs,ModelPoseVersion& output) noexcept;

// I-0113 gap 14: a model particle's palettes, mesh by mesh, as 64B30 (140064B30) makes them from the bone array A 636F0 receives
// (no pose object): with the model's +0x1E0 set (full rows), a mesh takes A (Given) or, when Dynamic (the mesh record's +4), A_i x G_i
// over every row (Composed, G the model's [+0x1D0] bind rows, 17CE0's order); with it clear, its own rows (Own): its list's entries
// up to a negative one, each A_k or A_k x G_k, or for a mesh not Dynamic with an empty list the one row of its default bone. `composed`
// holds the full A x G rows when a mesh takes them. False, with `reason` named, when G is short for a Dynamic mesh or an index is past A.
struct EffectModelMesh { bool dynamic{}; uint16_t default_bone{}; std::vector<int32_t> bones; };
enum class EffectMeshPaletteKind { Given, Composed, Own };
struct EffectMeshPalette { EffectMeshPaletteKind kind{EffectMeshPaletteKind::Given}; std::vector<ModelPoseRow> rows; };
bool ProduceEffectModelPalettes(bool full_rows,std::span<const ModelPoseRow> a,std::span<const ModelPoseRow> g,
    std::span<const EffectModelMesh> meshes,std::vector<EffectMeshPalette>& out,std::vector<ModelPoseRow>& composed,const char*& reason);

// One native engine pose owner publishes immutable versions. CPU ring rotation
// and reference Q are caller inputs with distinct roles; no previous version is
// used as Q. Recording/completion retains the exact acquired shared version.
// Withdrawal blocks future selection/update while held versions stay valid.
class ModelPoseOwner {
public:
    explicit ModelPoseOwner(uint32_t row_count) noexcept : row_count_(row_count) {}
    std::shared_ptr<const ModelPoseVersion> Acquire() const;
    std::shared_ptr<const ModelPoseVersion> Update(const ModelPoseInputs& inputs) noexcept;
    void Withdraw();
private:
    const uint32_t row_count_;
    mutable std::mutex lock_;
    std::shared_ptr<const ModelPoseVersion> current_;
    bool withdrawn_{};
};
}
