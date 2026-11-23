#pragma once
#include "../renderer/scene_pass.h"
#include "../content/shader_interface.h"

namespace ds3native::frame {
// Owned prepared primitives, not a graphics-call stream or recovered ER layout.
// Engine producers must supply a stable CPU transaction. No deferred pointers
// to the original primitive/batch/cache intrusive lists enter this contract.
enum class PrimitiveBlend : uint8_t { NoColor,StraightAlpha,AdditiveAlpha,PremultipliedAlpha,Multiply,Screen,PremultipliedAdditive };
enum class PrimitiveStencil : uint8_t { Disabled,ReplaceZero,IncrementEqualZero,ReadOne,WriteMaskLevel,ReadMaskLevel,ClampToReference };
enum class PrimitiveSampler : uint8_t { None,LinearWrap,LinearClamp,PointWrap,PointClamp };
struct PrimitiveProgram {
    uint64_t identity{};
    ds3content::CpuShaderProgram program;
};
struct PrimitiveBuffer {
    uint64_t identity{};
    std::vector<uint8_t> bytes;
};
struct PrimitiveTexture {
    uint64_t identity{};
    SceneTexture texture;
    // 1169: a native render target the UI samples directly (menu offscreen views): bound as is, never uploaded
    // from `texture` (whose bytes only describe its extent and format).
    Microsoft::WRL::ComPtr<ID3D12Resource> external;
    // F1 (I-0067 SCALEFORM-FILTERS-NATIVE): the packet target of that identity, sampled as earlier draws left it; `texture`
    // then describes its shape only (R8G8B8A8_UNORM, one empty mip entry).
    uint64_t target{};
};
// F1: an intermediate render target of one packet (Scaleform filter and cache targets): R8G8B8A8_UNORM with its own
// D32S8 stencil, cleared to transparent (stencil 0) at its first reference in draw order (written or sampled), sampled
// afterwards through a PrimitiveTexture naming it. Draws name their target in PrimitiveDraw::target (0: the frame
// target). Each identity is one use: its life runs from its first to its last referencing draw, and the encoder may
// back identities whose lives do not overlap with the same resource (the original's pooled temporary targets).
struct PrimitiveTarget {
    uint64_t identity{};
    uint32_t width{},height{};
};
struct PrimitiveViewport {
    float x{},y{},width{},height{},min_depth{},max_depth{};
};
struct PrimitiveImageBinding {
    uint64_t texture{};
    PrimitiveSampler sampler{};
    float mip_bias{},min_lod{},max_lod{};
};
struct PrimitiveDraw {
    uint64_t identity{},vertex_program{},pixel_program{},vertices{},indices{},texture{};
    std::vector<SceneVertexElement> elements;
    std::vector<SceneConstantBinding> constants;
    uint32_t stride{},vertex_offset{},index_offset{},index_width{},count{},start{},instances{1},first_instance{};
    int32_t base_vertex{};
    bool indexed{},scissor_enabled{};
    std::array<int32_t,4> scissor{};
    PrimitiveViewport viewport;
    PrimitiveBlend blend{};
    PrimitiveStencil stencil{};
    PrimitiveSampler sampler{};
    // Genuine observed sampler semantics. Native policy does not infer these
    // from a legacy object ID or silently select a default sampler.
    float mip_bias{},min_lod{},max_lod{};
    PrimitiveImageBinding secondary;
    uint32_t stencil_reference{};
    bool clear_stencil_before{};
    uint64_t target{};   // F1: 0 = the frame target, else a PrimitiveTarget of the packet
};
struct PrimitiveWorkSource {
    uint64_t content_incarnation{},generation{},texture_generation{},serial{};
    uint32_t width{},height{};
    std::vector<PrimitiveProgram> programs;
    std::vector<PrimitiveBuffer> buffers;
    std::vector<PrimitiveTexture> textures;
    std::vector<PrimitiveDraw> draws;
    std::vector<PrimitiveTarget> targets;
};
struct PrimitiveWorkPacket {
    uint64_t content_incarnation{},generation{},texture_generation{},serial{};
    uint32_t width{},height{};
    std::vector<PrimitiveProgram> programs;
    std::vector<PrimitiveBuffer> buffers;
    std::vector<PrimitiveTexture> textures;
    std::vector<PrimitiveDraw> draws;
    std::vector<PrimitiveTarget> targets;
};
enum class PrimitiveWorkStatus : uint8_t { Ready,MissingProgram,MissingBuffer,MissingTexture,MissingConstants,Unsupported,Invalid };
struct PrimitiveTextureLayout {
    uint32_t row_bytes{},rows{},block_width{},block_height{},block_bytes{};
};
// Exact physical storage for qualified single-plane primitive image formats.
// No format conversion, implicit mip generation or shader/view policy.
bool GetPrimitiveTextureLayout(uint32_t width,uint32_t height,DXGI_FORMAT format,PrimitiveTextureLayout& output) noexcept;
bool ValidPrimitiveTexturePayload(const SceneTexture& texture) noexcept;
// 1250: an external texture (a native render target bound as is) keeps one mip entry per level for its shape; each entry
// is empty or exact (its bytes are never read), so the UI texture set does not copy target-sized buffers.
bool ValidPrimitiveTexture(const PrimitiveTexture& texture) noexcept;
struct PrimitiveWorkResult {
    PrimitiveWorkStatus status{PrimitiveWorkStatus::Invalid};
    uint64_t draw_identity{};
};
// One complete, order-preserving CPU transaction. Refusal keeps prior output
// untouched. Output owns all programmes/resources/current constants. Target
// initialization/production and GPU residency are separate owner obligations.
PrimitiveWorkResult ProducePrimitiveWork(const PrimitiveWorkSource& input,PrimitiveWorkPacket& output);
// The same admission; a Ready source moves its payloads into the packet (a refused source is left intact).
PrimitiveWorkResult ProducePrimitiveWork(PrimitiveWorkSource&& input,PrimitiveWorkPacket& output);
}
