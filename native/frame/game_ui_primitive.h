#pragma once
#include "game_ui_shaders.h"
#include "game_primitive_geometry.h"
#include "game_ui_bindings.h"

namespace ds3native::bootstrap {
struct UiPrimitiveState {
    frame::PrimitiveViewport viewport;
    frame::PrimitiveBlend blend{};
    frame::PrimitiveStencil stencil{};
    uint32_t stencil_reference{};
};
// Actual CPU mode/ref/blend intent, qualified by1F79550/1F795B0 and original
// static tables. Solid/no-cull2D family, including native nested-mask levels.
// Unsupported depth modes and blend equations refuse; no previous-state fallback.
bool DecodeUiPrimitiveState(uint32_t depth_mode,uint32_t stencil_ref,uint32_t blend_mode,
    uint8_t premultiplied,uint8_t target_alpha,const frame::PrimitiveViewport& viewport,UiPrimitiveState& output);
// One same-call untextured geometry/shader/state transaction. Static CPU input
// metadata and every current constant/resource are owned. No legacy geometry,
// layout wrapper or setter stream enters the packet. Target initialization and
// complete ordered frame composition are independent caller obligations.
frame::PrimitiveWorkResult ProduceNativeUiPrimitive(const UiCpuPreparedShader& shader,
    const PrimitiveGeometry& geometry,frame::PrimitiveProgramVersion current,uint64_t serial,
    uint32_t width,uint32_t height,const UiPrimitiveState& state,frame::PrimitiveWorkPacket& output);
frame::PrimitiveWorkResult ProduceNativeUiPrimitive(const UiCpuPreparedShader& shader,
    const PrimitiveGeometry& geometry,frame::PrimitiveProgramVersion current,uint64_t serial,
    uint32_t width,uint32_t height,const UiPrimitiveState& state,const UiDrawImageBinding& image,frame::PrimitiveWorkPacket& output,
    const UiResidentImage& resident={});
}
