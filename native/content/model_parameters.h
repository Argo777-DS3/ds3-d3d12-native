#pragma once
#include <cstdint>
#include <array>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ds3content {
// CPU SPX material parameter declarations (1B0BC0/1B0D80/1FEEC0).
// Loaded 1FEEC0 ASM + loaded 1FF700/1FF910 distinguish the output vector,
// animation lookup key, producer kind and RGB transform flag. No CBV binding
// or shader-used coverage is established by these fields alone.
struct ModelParameterDeclaration {
    std::wstring name;
    uint32_t vector_index{},lookup_key{};
    uint8_t producer_kind{};
    bool transform_rgb{};
    // SPX generic route supplies zero for these. A7E40 built-in definitions
    // separately supply scalar lane and animation-category selection.
    uint32_t scalar_offset{},animation_selector{};
};
// Raw, unrelocated SPX bytes copied before A97F0. Owns every returned name;
// no pointer into the source, old template object, file I/O or GPU work.
std::vector<ModelParameterDeclaration> ReadModelParameterDeclarations(std::span<const uint8_t> bytes);
// Actual MTD type0..11 and raw CPU getter words. Retain the type: bool words
// and signed integer-to-float conversion have different shader semantics.
struct ModelNumericParameter {
    std::string name;
    uint32_t type{};
    std::array<uint32_t,4> words{};
};
struct PreparedModelParameter {
    ModelParameterDeclaration declaration;
    uint32_t components{};
    std::array<uint32_t,4> base_words{};
};
// Owned immutable producer under current material + effect leases. This is
// the source-selected generic numeric route; missing/default/special routes
// are not substituted. Failure leaves output unchanged.
bool PrepareModelNumericParameter(const ModelParameterDeclaration& declaration,
    const ModelNumericParameter& value,PreparedModelParameter& output);
struct ModelBuiltinParameterDeclaration {
    std::wstring name;
    uint32_t kind{},animation_selector{},vector_index{},scalar_offset{};
    bool transform_rgb{};
};
// Same generic numeric producer from a source-selected A7E40 definition.
// Kinds0/1/3 only; specialized cases remain unavailable. This entry applies
// only after no current SPX declaration matches; no authored default is added.
bool PrepareModelBuiltinParameter(const ModelBuiltinParameterDeclaration& declaration,
    const ModelNumericParameter& value,PreparedModelParameter& output);
struct ModelParameterAnimation {
    // True only after the current lookup is established. Empty target then
    // means the source-selected base branch; false cannot mean no animation.
    bool lookup_complete{};
    std::optional<std::array<float,4>> target;
    float strength{};
};
struct ModelParameterAnimationRow {
    std::wstring material_name;
    uint32_t lookup_key{};
    bool universal{};
    std::array<float,4> target{};
    float strength{};
    uint32_t kind{};
    //22D620's outer gate is distinct from146850/146920 name/key selection.
    int32_t material_index{-1};
};
struct ModelParameterAnimations {
    bool available{};
    std::vector<ModelParameterAnimationRow> rows;
};
// 146920 selects by key; nonzero category selects through146850. Both prefer
// the first exact material-name row, then the first universal row. Table order
// is semantic. An unavailable snapshot is not an empty one.
bool SelectModelParameterAnimation(const ModelParameterAnimations& inputs,
    const std::wstring& material_name,uint32_t lookup_key,ModelParameterAnimation& output,uint32_t selector=0);
struct ModelParameterWrite {
    uint32_t byte_offset{},bytes{};
    std::array<uint32_t,4> words{};
};
struct ShaderInterface;
// Native per-material feature-off contract for ordinary pixel preparation.
// Positive current disabled states only; unavailable/active states add nothing.
// Exact named float4 b2 layout is required for each independent field group.
// This defines owned native values, not old shared-arena initialization/parity.
std::vector<ModelParameterWrite> ProduceOrdinaryInactiveModelParameters(const ShaderInterface& shader,
    std::optional<bool> damage_enabled,std::optional<bool> wet_enabled);
// 141070 -> 22C4B0 -> 22E930: FLVER GXMD values use the SPX scalar
// descriptor table, separately from the generic MTD parameter table/b3.
// Own scalar words and the destination register. Packed colour groups (type 11)
// are emitted as 22E930 does (1128); MTD provider descriptors (type 0) stay
// unavailable. This bounded route has no legacy object.
struct ModelGeneratedParameters {
    uint32_t byte_offset{};
    std::vector<uint32_t> words;
    // 22E5C0 (the animated generated parameters): each SPX descriptor's type (+0x38), gamma flag (+0x39), component (+0x30)
    // and animation lookup key (+0x3C), and the raw scalar words before 22E930's emission (one per descriptor).
    struct Descriptor {uint8_t type{},gamma{};uint32_t component{},animation_key{};};
    std::vector<Descriptor> descriptors;
    std::vector<uint32_t> raw;
};
// 22E5C0 then 22E930's emission: for each descriptor with component 0, a nonzero animation key and type 1..4 or 11, the row
// 146920 selects by that key and the material name moves the descriptor's 1..4 raw values (type 11: four) towards the row's
// target by its strength (v += (t - v) * s); the words are then emitted again (type 11 colour groups packed as at parsing).
// Returns false (output untouched) when no descriptor is animated.
bool AnimateModelGeneratedParameters(const ModelGeneratedParameters& parameters,const ModelParameterAnimations& animations,
    const std::wstring& material_name,std::vector<uint32_t>& words);
enum class ModelGeneratedStatus { Ready, Missing, Unsupported, Invalid };
// Raw SPX before relocation, ordered FLVER records including their terminator.
// The source's matching layout copies row components in order; other layouts
// select each scalar by its lookup key/component, then its SPX default word.
// The initial route starts at270: lower generated ranges may be overwritten
// by currently unavailable fixed-field writers and therefore refuse.
// Failure leaves output unchanged. Animation must be qualified separately.
ModelGeneratedStatus ReadModelGeneratedParameters(std::span<const uint8_t> spx,
    std::span<const uint8_t> gx,ModelGeneratedParameters& output);
//141070 GX80 and22BB90/22BC90 GX81/GX82, owned by the current FLVER,
//MTD and SPX owners. Missing provider values leave their UV range absent.
//22E930 uses GX81's override flag for BOTH UV branches (actual ASM).
struct ModelDamageParameters {
    bool enabled{};
    uint32_t rate{};
    std::array<std::optional<std::array<uint32_t,4>>,2> uv;
};
ModelGeneratedStatus ReadModelDamageParameters(std::span<const uint8_t> gx,
    std::span<const ModelParameterDeclaration> declarations,
    std::span<const ModelBuiltinParameterDeclaration> builtins,
    std::span<const ModelNumericParameter> values,bool provider_values_available,
    ModelDamageParameters& output);
//A rate update requires an available animation snapshot and the current
//material+60 index for a nonempty table. The rate writes FOUR bytes only;
//unwritten lanes and disabled fields never become known through zero-fill.
std::vector<ModelParameterWrite> ProduceModelDamageParameters(const ModelDamageParameters& parameters,
    const ModelParameterAnimations& animations,const std::wstring& material_name,
    std::optional<int32_t> material_index);
// Current CPU material fields, owned separately from its old GPU buffer.
// Overrides use source160/17C/164 order a,b,c. No constructor values supplied.
struct ModelWetMaterialParameters {
    std::array<float,4> diffuse{},reflectance{};
    uint32_t type{};
    float scale{};
    std::optional<std::array<float,3>> override_abc;
};
//2DD60's exact shader-family/catalog match OR141070's current SPX header bit2.
// A null catalog is unavailable, not empty. A proved positive branch suffices.
bool SelectModelWetActivation(std::wstring_view shader_family,std::span<const uint8_t> spx,
    const std::vector<std::wstring>* current_catalog,bool& output);
//22D620 category15/16 and CRC fallback update wet colors. Category17 changes
//only override a/c. Reuses22E930's native wet calculation and owns four16-byte
//ranges, after current lookup/finite values are established. Disabled wet
//writes nothing; an unavailable input never becomes creation/default coverage.
std::vector<ModelParameterWrite> ProduceModelWetParameters(bool enabled,
    const ModelWetMaterialParameters& parameters,const std::optional<std::array<float,3>>& draw_abc,
    const ModelParameterAnimations& animations,const std::wstring& material_name,
    std::optional<int32_t> material_index);
// Phantom block of the material dynamic object (source = material+D0): flag +22, IsEdgeSubtract +70, IsFrontSubtract +71,
// Edge +80, Front +90, Diff +A0, Spec +B0, Light +C0 (rgb and a). 22D620 overrides them from the draw's ExParam_Phantom*
// rows (categories 8-14, else the key globals 4962354, 4962344, 4962340, 496234C, 4962348, 4962334, 4962330), then
// 22E930 writes cbMatDynParam rows 26-30 (+1A0..+1EF) = (r*r*a*s, g*g*a*s, b*b*a*s, a), s = -1 for a subtracting
// Edge or Front, else 1 (I-0067 PHANTOMS-SIGNS-NATIVE §2). Nothing is written when the flag is clear.
struct ModelPhantomParameters {
    bool enabled{};
    std::array<uint8_t,2> subtract{};
    std::array<std::array<float,4>,5> colours{};
};
std::vector<ModelParameterWrite> ProduceModelPhantomParameters(const ModelPhantomParameters& parameters,
    const std::array<uint32_t,7>& fallback_keys,const ModelParameterAnimations& animations,const std::wstring& material_name,
    std::optional<int32_t> material_index);
// 1FF910: scalar=4 bytes, vector=16 bytes (including source's zero padding),
// current interpolation first, otherwise optional RGB power2.2. Produces an
// owned range, not a legacy CPU object or a zero-filled ready constant buffer.
bool ProduceModelParameter(const PreparedModelParameter& parameter,
    const ModelParameterAnimation& animation,ModelParameterWrite& output);
// Native policy from current authored parameters/declarations. A7DD0/2D8E0
// establish zero alpha/transparency and material type1. A7E40 and2DD60 supply
// authored flags; vtables759 identify their actual alpha/blend callbacks.
// Runtime shader catalogs controlling bits26/27 are separate current inputs.
struct ModelMaterialPolicy {
    uint32_t material_type{1},flags{},known_flags_mask{0xF3FFFFFFu},alpha_ref{};
    // Raw MTD blend mode (builtin kind 6, the value whose nonzero sets transparent).
    uint32_t blend_mode{};
    bool forward{},transparent{},disable_decal{},disable_depth_write{};
};
bool PrepareModelMaterialPolicy(std::wstring_view shader_family,
    std::span<const ModelParameterDeclaration> declarations,
    std::span<const ModelBuiltinParameterDeclaration> builtins,
    std::span<const ModelNumericParameter> values,ModelMaterialPolicy& output);
// Exact142130 precedence, consuming native policy and current CPU special flag.
// This classification does not establish pass masks, constants or draw readiness.
uint32_t ClassifyModelMaterial(const ModelMaterialPolicy& policy,bool special);
}
