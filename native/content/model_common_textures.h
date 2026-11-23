#pragma once
#include "dds_resource.h"
#include <array>

namespace ds3content {
//7ECB0's deterministic CPU initialization, with the independently recovered
//seed and SIMD math. No old renderer, texture wrapper or GPU allocation runs.
std::array<CpuTextureResource,16> ProduceModelDitherTextures();
//FF630's pair of 4x4 A8_UNORM ordered-dither textures ([GXRS+4DE8]+8D0 and +940): the model programmes' g_DitherTexture (t38),
//which 13B450 binds as min(Q+1A6, 1) when Q+1A9; t38 held a 4x4 A8_UNORM one-mip texture at every named row of
//i113-t22-warp3-original, FC_DitherEnable 0 or 1 (I-0113 tranche 22: the alpha test of the _[A] programmes reads it too); the second is 255 minus the first.
std::array<CpuTextureResource,2> ProduceModelFadeDitherTextures();
//06C00 common bank entry1, selected by5EDA0/6FFA0 for absent decal images.
//Selection of that branch remains the caller's current instance contract.
CpuTextureResource ProduceCommonDecalTexture();
struct SystemTexture { std::string name;CpuTextureResource image; };
//06C00 system textures0..13 (SYSTEX_WHITE..CYAN), exact CPU contents.
std::array<SystemTexture,14> ProduceSystemTextures();
}
