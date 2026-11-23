#pragma once
#include "shader_bank.h"
#include <span>

namespace ds3content {
// Native shader intent. Source AA630/AA280 supplies this bounded GBuffer
// recipe; pass membership and constant/texture readiness are separate inputs.
struct ModelProgramIntent {
    uint32_t shader_pass{},layout_type{},deformation{},pixel_variant{};
    bool instanced{},prefer_channel{};
    // C5 (MENUS-MAP 75): the alternate set (mat+0x78, 82E50/A9EF0) a draw takes when its state byte +0x87 is set: its Fwd
    // pixel programme is <family>_Phantom_Fwd[_[A]]; the vertex programme stays <family>_Fwd... Forward pass only.
    bool phantom{};
};
struct ModelProgramName { std::wstring name;ShaderStage stage{}; };
enum class ModelProgramStatus : uint8_t { Resolved,Missing,Ambiguous,Unsupported,Invalid };
struct ModelProgramSelection {
    ModelProgramStatus status{ModelProgramStatus::Invalid};
    size_t vertex{},pixel{};
    bool channel{};
    // 1186 Velo pass (shader pass 1), plain pixel variant: AA630 binds the shared loose programme
    // GXFlver_Common_Velo (repository callback 0) instead of a family programme; `pixel` is then unset.
    bool shared_pixel{};
    // W7 TS-2: a PN recipe (deformation 3): the hull and domain entries (resolved, or the status says why not).
    size_t hull{},domain{};bool tessellated{};
};
inline constexpr const wchar_t* velocity_common_pixel_programme=L"GXFlver_Common_Velo";
// Resolves only against one actual current effect owner's catalogue. The
// returned indices retain that owner's exact entries; no bank-name invention,
// host identifiers, shader substitution or detached resource publication.
ModelProgramSelection SelectModelPrograms(const std::wstring& authored_shader_path,
    const ModelProgramIntent& intent,std::span<const ModelProgramName> programs);
}
