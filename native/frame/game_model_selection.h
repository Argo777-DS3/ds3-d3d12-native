#pragma once
#include "../content/acquire.h"
#include "../content/model_parameters.h"

namespace ds3native::ingress {
struct ModelFadeTrack {
    int32_t current{},previous{},current_variant{},previous_variant{};
    float interpolation{},weight{};
};
// Owned scalar inputs, separated from legacy 400-byte state and its pointers.
// Geometry passes use a different weight; pass 0x39 uses its own track.
struct ModelFadeState {
    ModelFadeTrack normal,alternate;
    float geometry_weight{},group_opacity{};
    uint32_t group_flags{};
    bool geometry_suppressed{},fade_enabled{},alternate_fade_enabled{};
};
struct ModelFadeIntent {
    int32_t variant{};
    uint32_t branch{};
    float weight{},opacity{};
    bool fade_enabled{},alternate_fade_enabled{};
};
struct ModelFadeSelection { std::array<ModelFadeIntent,2> intents{};uint32_t count{}; };
bool ReadModelFadeState(uintptr_t entity,ModelFadeState& output) noexcept;
// Field layout of a fade owner: SprjModelDrawEntity (defaults, D06E10) or
// SprjModelDispEntity (D04B90: tracks +120/+1C0, weight +130, group +1F0, no
// geometry suppression byte). Group-relative fields are shared (+88/+89/+98/+124).
struct ModelFadeLayout { uint32_t normal{0x160},alternate{0x200},geometry_weight{0x170},group{0x240};int32_t suppressed{0xA5D}; };
bool ReadModelFadeStateAt(uintptr_t owner,const ModelFadeLayout& layout,ModelFadeState& output) noexcept;
struct ModelWetDrawInputs { float a{},b{},c{}; };
// Selected group = entity+240.1A8580 copies all400 bytes to draw state+240;
//22E930 reads group+90/+8C/+94.4E180's fade copies preserve these values.
// Own current CPU values, without constructing the old draw state or reading
// its constructor defaults. Material wet activation/base/updates are separate.
bool ReadModelWetDrawInputsAt(uintptr_t group,ModelWetDrawInputs& output) noexcept;
//82CC0/22B550 construct the genuine CPU prefix retained by the native material
//registry. Own its CURRENT wet fields (material+D0+140..17F), never GPU buffer
//contents or constructor constants. Caller holds the material lifetime lock.
bool ReadModelWetMaterialParameters(uintptr_t material,ds3content::ModelWetMaterialParameters& output) noexcept;
//2DD60 compares43 current UTF16 shader names,512 bytes apart, at144731BE0.
// Copy at selection, not from executable-file initial bytes or a cached default.
bool ReadModelWetShaderCatalog(uintptr_t image,std::vector<std::wstring>& output);
enum class ModelDecalChoice : uint8_t { None,Common,InstanceProvider };
// D079C0's descriptor+98 is the selected entity+A60->+50 identity already
// owned by ReadCurrentModelInputs.57A20 preserves a non-null provider;
// otherwise model+224 bit2 and current14473C7E5 choose the6FFA0 common binder.
// Only its t41/t42 common image is currently implemented; custom providers
// and the distinct t43 bank entry require their own current resource contracts.
bool ReadModelDecalChoice(uintptr_t image,uintptr_t model,uintptr_t provider,
                         ModelDecalChoice& output) noexcept;
//57A20 ASM140058474 compares the CURRENT dword14473CBAC with2.
//This setting is not a boolean byte. Model+220 bit2 gates the split separately.
bool ReadModelSplitPassSetting(uintptr_t image,bool& output) noexcept;
// D06E10's pass-sensitive branch selection. Does not call 4E180/4E270 or
// publish a fake legacy instance record. Failure leaves output unchanged.
bool SelectModelFade(const ModelFadeState& state,uint32_t engine_pass,ModelFadeSelection& output) noexcept;
// I-0115: whether the main walk draws an entity of category 0x01 (category: its E+0x20 byte; precondition: category&1, which the
// caller tests) through the 0x45 view's selection
// instead of the pass-5 one. Without a pass-5 intent (or a 0x02/0x04 bit), the former rule: the 0x45 view when it has an intent or
// when the entity is of category 0x01 alone. With both, the pass-5 view, unless view_weight_fix and the 0x45 view is strictly
// more opaque (no-view-weight-fix.flag: the former rule).
bool PreferMainViewFade(uint8_t category,const ModelFadeSelection& geometry,const ModelFadeSelection& main_view,bool view_weight_fix) noexcept;
// D079C0 descriptor+20 = entity+A40; 5C0E0 -> 5A120 -> 140850 routes
// its CPU0x90-row collection to 1FF910. Owns the table and names synchronously;
// no legacy draw record, material factory or GPU object is created.
bool ReadModelParameterAnimations(uintptr_t owner,ds3content::ModelParameterAnimations& output);
struct CurrentModelPreparationFlags { bool skip_kind8{},special_kind5{}; };
// D079C0 supplies entity+3D0 as description+90; single5CEC0/5B3D0
// sets F8 from block+E1==0 and F9 from block+630!=0. No provider callback
// is invoked. Other callback/instanced flag mutations remain separate inputs.
bool ReadModelPreparationFlagsAt(uintptr_t animation,CurrentModelPreparationFlags& output) noexcept;
// Current engine CPU associations. No legacy renderer object is invoked and
// these borrowed identities are resolved to native leases before publication.
struct CurrentModelMesh {
    uint32_t ordinal{},flags{},layout_type{};
    uint16_t material_id{},patch_index{},lod_mask{};
    uintptr_t material{},layout{};
};
struct CurrentModelInputs {
    uintptr_t model{},storage{},pose{},patch{};
    bool instance_streams{};
    std::vector<CurrentModelMesh> meshes;
};
// D079C0/59040/57A20 + 63CB0/64610 associations; layout membership comes
// from 144340's actual +40 publication, not the old virtual provider at +20.
// This recovers selection inputs, not material pass eligibility or readiness.
bool ReadCurrentModelInputs(uintptr_t image,uintptr_t entity,uintptr_t model,
                            CurrentModelInputs& output) noexcept;
// Same associations with explicit owner fields: the dynamic stream source
// pointer (its +50 is the patch), the optional instance vector pointer field
// (0 = none) and the pose identity. SprjFD4ModelItem: source +740, pose
// [+710]+A0 (D3C4D0/D16F60/D3DA20).
bool ReadCurrentModelInputsAt(uintptr_t image,uintptr_t model,uintptr_t source_field,uintptr_t instance_field,
                              uintptr_t pose,CurrentModelInputs& output) noexcept;
}
