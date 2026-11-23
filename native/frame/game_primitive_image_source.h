#pragma once
#include "primitive_work.h"
#include "primitive_textures.h"
#include <span>

namespace ds3native::bootstrap {
// Physical CPU output of the engine's image decoder. One plane; each supplied
// mip is explicit. Callers must hold the actual CPU owner stable while copied.
// Engine ImageData flags, palettes, queues and GPU mappings are not inputs.
struct CpuPrimitiveImageMip {
    uint32_t width{},height{};
    uint64_t row_pitch{};
    std::span<const uint8_t> bytes;
};
bool ProduceCpuPrimitiveImage(uint64_t identity,uint32_t engine_format,
    std::span<const CpuPrimitiveImageMip> mips,frame::PrimitiveTexture& output);
struct CpuGlyphImageUpdate {
    CpuPrimitiveImageMip source;
    std::array<int32_t,4> destination{};
    uint32_t mip{};
};
// Glyph mode1 supplies a pointer already offset into its genuine RawImage.
// Read only the rectangle's rows, not the inherited whole-image DataSize.
// Output is an atomic owned edit transaction for PrimitiveTextureStore.
bool ProduceCpuGlyphImageUpdates(uint64_t identity,const SceneTexture& current,
    std::span<const CpuGlyphImageUpdate> updates,std::vector<frame::PrimitiveTextureEdit>& output);
}
