#pragma once
#include "model_programs.h"
#include <optional>

namespace ds3content {
// Native render policy, kept separate from the pass scheduling family. The
// numeric1/2 meanings mirror the observed Crender714 consumer, not a mapping
// from an external AA preset. Unknown is never silently Default.
enum class NativeModelPixelPolicy : uint8_t { Default=0,AlphaCoverage=1,ForcePlain=2,Unknown=0xFF };
struct NativeModelPixelInputs {
    uint32_t template_value{}; // Current value corresponding to T+7C.
    bool global_variant_enabled{}; // Current global473D39E!=0.
    bool fade_dither{}; // Current Q+1A9 -> S200 bit20.
    NativeModelPixelPolicy policy{NativeModelPixelPolicy::Unknown};
};
enum class NativeModelPixelStatus : uint8_t { Selected,Unavailable,Unsupported };
struct NativeModelPixelSelection {
    NativeModelPixelStatus status{NativeModelPixelStatus::Unavailable};
    uint32_t variant{};
};
NativeModelPixelSelection SelectNativeModelPixelVariant(uint32_t shader_pass,const NativeModelPixelInputs& input) noexcept;

// OrdinaryForward: 57A20 rows (engine pass 0x29, preparation 0, class mask 0x12; and, for a model with +224 bit 12,
// engine pass 0x1F, preparation 0, class mask 0x80),
// shader pass 0 (Fwd), Reference1114.
// OrdinaryVelocity (1186, I-0067 VELOCITY-SKINNING §7): 57A20's single row recorded through 5C060 after the per-kind
// rows: engine pass 0x34, preparation 5 (shader pass 1, Velo), class mask 0xFFFF. The caller adds it only when the view,
// the quality option +0x21 and the draw's velocity byte (descriptor byte 1) allow it.
// OrdinaryShadow (D-0180 C2): the cascade passes 1..4's row (preparation 2, class mask 0x107, shader pass 2 Dpt/DptA).
enum class NativeModelPassFamily : uint8_t { Unknown,OrdinaryDeferred,OrdinaryForward,OrdinaryVelocity,OrdinaryShadow };
enum class NativeModelPassFlag : uint32_t { DisableDeformation=1,Instanced=2,SplitEngine5=4 };
enum class NativeModelPassStatus : uint8_t { Eligible,Filtered,Unsupported,Invalid };
enum class NativeModelPassReason : uint8_t {
    None,Family,Category,Selectors,RequiredMeshMask,RequestedLod,Flags,Deformation,
    ModelClassMask,MeshMask,MeshLod,MaterialClass,MaterialSecondary,Capabilities
};
struct OrdinaryModelPassInput {
    NativeModelPassFamily family{NativeModelPassFamily::Unknown};
    uint32_t category{},engine_pass{},preparation{},shader_pass{};
    uint32_t required_mesh_mask{},requested_lod{},flags{};
    // 63CB0: model+B4 is the loaded LOD mask; an absent requested LOD falls
    // back to the highest loaded LOD below it (LOD0 always remains).
    uint32_t model_lod_mask{1};
    // 57A20 split engine5 (dword14473CBAC==2, model+220 bit2): two rows,
    // class masks 1 then 4, replace the single mask5 row. 0 = pass default.
    uint32_t row_class_mask{};
    // nullopt is AFFIRMED absence of M, never unavailable/failed acquisition.
    // A selected nonnull model requires its actual owned current M+220 value.
    std::optional<uint32_t> model_material_mask;
    uint32_t mesh_flags{};
    uint16_t mesh_lod_mask{};
    // Use existing ClassifyModelMaterial; do not derive class from recipes.
    uint32_t material_class{},material_secondary{};
    uint64_t material_capabilities_first{},material_capabilities_second{};
    uint64_t group_capabilities_first{},group_capabilities_second{};
    uint32_t layout_type{},base_deformation{};
    bool prefer_channel{};
    NativeModelPixelInputs pixel;
};
struct OrdinaryModelPassDecision {
    // Eligible means row predicates only, never a shader or game draw ready.
    NativeModelPassStatus status{NativeModelPassStatus::Invalid};
    NativeModelPassReason reason{NativeModelPassReason::None};
    uint32_t material_class_mask{},secondary_mask{},effective_deformation{};
    NativeModelPixelSelection pixel;
    // Missing pixel policy cannot accidentally manufacture variant0 intent.
    // Nonzero deformation is retained here; existing SelectModelPrograms owns
    // the bounded recipe refusal rather than substituting deformation0.
    uint32_t effective_lod{};
    std::optional<ModelProgramIntent> intent;
};
uint32_t EffectiveOrdinaryModelDeformation(uint32_t base,bool disabled,uint32_t requested_lod) noexcept;
// 63CB0 descriptor+18 adjustment, ahead of the mesh +14 LOD bit test.
uint32_t SelectOrdinaryModelLod(uint32_t requested,uint32_t loaded_mask) noexcept;
OrdinaryModelPassDecision EvaluateOrdinaryModelPass(const OrdinaryModelPassInput& input) noexcept;

struct ModelPreparationPair {
    const void* material{}; // Identity of a retained native CPU material owner.
    uint32_t layout_selector{};
    bool operator==(const ModelPreparationPair&) const = default;
};
// Local to ONE ordered preparation invocation. Begin is called only after row
// admission. On a changed pair it withdraws the previous active preparation;
// failure leaves no active pair. UV/contributions and end/begin callbacks are
// caller-owned and are not cleared or performed here. Commit only real success.
class ModelPreparationPairState {
public:
    bool Begin(ModelPreparationPair candidate);
    bool RememberSuccess(ModelPreparationPair candidate) noexcept;
    const std::optional<ModelPreparationPair>& Current() const noexcept {return current_;}
    void Reset() noexcept {current_.reset();pending_.reset();}
private:
    std::optional<ModelPreparationPair> current_,pending_;
};
}
