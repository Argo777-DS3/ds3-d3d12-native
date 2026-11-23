#pragma once
#include <array>
#include <cstdint>

namespace ds3content {
// Character paint (DECALS-PAINT-READY D2, the kind-1 decal request painted into a struck character's GXPaintRenderTarget):
// the CPU matrices of the box paint draw, transcribed from 5FCD0 (the decal frame W), 1AC640 (its rigid inverse V), 1AA540
// (the depth bias B and the projection P of a box, range > 1 diverging, else orthographic) and 1AB370 (F = V B P, uploaded as
// VS b0 cbMatrixWVP), with the original's summation orders (compiled /fp:strict). Pure CPU.
//
// Conventions of the original: 16 floats, row-major, row-vector convention (v' = v M); the buffer holds the transpose (register
// j = column j of F).

constexpr unsigned kPaintDescriptorSize = 0x118;   // the kind-1 request's descriptor (the GXPaintDecal object's +0x18..+0x12F)

struct PaintProjector {
    std::array<float,16> frame{};   // W (decal space -> world): rows D, m x D (m = -P1, i.e. D x P1), -P1, (P0, 1)
    std::array<float,16> view{};    // V = W^-1 (1AC640)
    std::array<float,16> bias{},projection{};   // B, P (1AA540)
    std::array<float,16> wvp{};     // the VS b0 upload: F transposed (S[4r+c] = F[c][r])
    bool diverging{};               // range > the minimum range (1.0): the truncated pyramid
};
// From the descriptor bytes (d+0x00 P0, d+0x0C P1, d+0x18 D, d+0x24 width s, d+0x2C depth z, d+0x30 range); false when the box
// cannot be built (a zero width or depth, a non-finite value).
bool ComputePaintProjector(const uint8_t* descriptor,PaintProjector& out);
}
