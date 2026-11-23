#pragma once
#include "game_ui_images.h"
#include <functional>

namespace ds3native::bootstrap {
struct UiDrawImageBinding {
    uint64_t serial{},draw_identity{};
    UiImageIngress::Lease images;
    uint64_t texture{};
    uint8_t sampler_index{};
    frame::PrimitiveSampler sampler{frame::PrimitiveSampler::None};
    float mip_bias{},min_lod{},max_lod{};
    frame::PrimitiveImageBinding secondary;
    uint8_t secondary_sampler_index{};
};
// Authored CPU sampler byte from1F82AA0: bit0 clamps U/V, bit1 selects
// linear filtering; W clamps, bias=-.75, minLOD=0, maxLOD=FLT_MAX.
// Native GPU objects are created by PrimitivePass. Other bytes are unsupported.
bool DecodeUiSampler(uint8_t index,frame::PrimitiveSampler& output) noexcept;
class UiDrawImageSource {
public:
    bool Begin(UiImageIngress::Lease current,uint64_t serial,uint64_t draw_identity);
    // Qualified single-plane images at t0/s0 and t1/s1 are admitted.
    // Current snapshot/engine address identify native CPU data, never old GPU
    // views. A failed bind poisons this transaction; no previous draw defaults.
    bool Bind(UiImageIngress::Lease current,uintptr_t engine_image,uint32_t shader_slot,uint8_t sampler);
    bool Resolve(frame::PrimitiveTextureVersion current,uint64_t serial,UiDrawImageBinding& output) const;
private:
    UiDrawImageBinding binding_;
    bool valid_{};
    uint32_t bound_mask_{};
};
// Add an independently owned current CPU image and sampler to one native draw.
// A refusal leaves draw and textures untouched; retained binding also owns its fenced native resource.
// `resident`, when given, may supply an image's payload instead of the copy: it receives the image object of the
// binding's immutable snapshot and moves an owned payload of exactly that object into `taken` (returns true).
using UiResidentImage=std::function<bool(const frame::PrimitiveTexture& origin,frame::PrimitiveTexture& taken)>;
bool ApplyUiDrawImageBinding(const UiDrawImageBinding& binding,frame::PrimitiveTextureVersion current,
    uint64_t serial,frame::PrimitiveDraw& draw,std::vector<frame::PrimitiveTexture>& textures,const UiResidentImage& resident={});
}
