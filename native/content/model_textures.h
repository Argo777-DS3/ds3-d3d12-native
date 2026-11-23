#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>
#include "shader_interface.h"
#include "shader_constants.h"
#include "texture_tiling.h"

namespace ds3content {
// A97F0/1B0BC0/1B0E80: owned current CPU declarations, before relocation.
// Slot is the actual resource register used by1AD990, not an authored ordinal.
struct ModelTextureDeclaration {
    std::wstring name;
    uint32_t slot{},kind{};
    uint8_t flags{};
    int32_t remap{-1};
};
std::vector<ModelTextureDeclaration> ReadModelTextureDeclarations(std::span<const uint8_t> bytes);
struct ModelTextureInput {
    std::wstring path,type;
    std::array<float,2> scale{};
    uint32_t authored_ordinal{};
    uint16_t metadata{}; // FLVER+10/+11; semantics remain opaque.
};
struct ModelTextureBinding {
    ModelTextureDeclaration declaration;
    std::array<float,2> scale{};
    uint16_t metadata{};
    uint32_t first_authored_ordinal{},current_authored_ordinal{};
    std::wstring path;
};
// Exact-name SPX first, then83340's two pinned builtin tables. An existing
// SPX owner with a miss uses82220's observed zero flags, not builtin flags.
// Duplicate slots keep first metadata and replace the current path/ordinal.
// Missing names are omitted; no missing/default texture is manufactured.
bool PrepareModelTextureBindings(std::span<const ModelTextureInput> inputs,
    bool spx_owner,std::span<const ModelTextureDeclaration> declarations,
    std::span<const ModelTextureDeclaration> builtins,std::vector<ModelTextureBinding>& output);
enum class ModelTextureStatus : uint8_t { Ready,Missing,Unsupported,Invalid };
enum class ModelTextureSource : uint8_t { Material,Scene };
struct ModelSceneTextureBinding {uint32_t slot{};};
struct ModelShaderTexture { uint32_t slot{};size_t binding{};ModelTextureSource source{ModelTextureSource::Material}; };
// Reuses material admission's bounded PS/2D/count1/float SRV envelope. Actual
//1AD990 register associations replace diagnostic shader-name aliases. Ready
// means declarations match; residency, samplers and submission are separate.
ModelTextureStatus SelectModelShaderTextures(const ShaderInterface& shader,
    std::span<const ModelTextureBinding> bindings,std::vector<ModelShaderTexture>& output);
//Joint current bindings, supplied only for actual owned scene resources.
//Ambiguous material/scene slots refuse; no missing resource is substituted.
ModelTextureStatus SelectModelShaderTextures(const ShaderInterface& shader,
    std::span<const ModelTextureBinding> bindings,std::span<const ModelSceneTextureBinding> scene,
    std::vector<ModelShaderTexture>& output);
// The first texture or structured resource outside that envelope (the cause of an Unsupported status), or null.
const ShaderResourceDeclaration* UnsupportedModelShaderTexture(const ShaderInterface& shader);

struct ModelTextureTilingInputs {
    int32_t buffer_register{-1};
    bool skip_kind8{},special_kind5{};
    TextureUvOffsets offsets;
};
struct ModelTextureTiling {
    int32_t buffer_register{-1};
    std::array<bool,3> stages{}; // VS, DS, PS; original texture-row bind gates.
    TextureTilingValues values;
};
// Exact authored rows plus explicit current selection inputs, independent of
// legacy draw-state packing. Negative authored remaps follow1AE3F0's -1 gate.
// No guessed name/register, identity fill or missing animated pair is supplied.
bool ProduceModelTextureTiling(std::span<const ModelTextureBinding> bindings,
    const ModelTextureTilingInputs& inputs,ModelTextureTiling& output) noexcept;
// Actual RDEF USED extents govern readiness. Only current written words are
// passed to the existing packer; a whole USED array requires its whole extent.
ShaderRegisterBlock PrepareModelTextureTiling(const ShaderInterface& shader,
    const ModelTextureTiling& tiling);
}
