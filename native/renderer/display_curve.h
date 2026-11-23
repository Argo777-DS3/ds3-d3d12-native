#pragma once
#include <array>
#include <cstdint>
#include <span>

namespace ds3native {
// W3 / G0-1 (GAMMA-COLOUR-PATH-READY 3.3): the display curve of the original's gamma pass. FUN_1400C83D0 writes 1025 RGB
// float entries (0x300C bytes, interleaved): for channel c, entry i = clamp(powf(i/1024, src[c]/dst[c]), 0, 1) with the
// literal 1.0 above one and 0 below zero. In the game the engine's own generator is called (bit-identical powf); this pure
// reimplementation serves the unit tests and the gamma-verify cross-check.
constexpr size_t DisplayCurveEntries=1025,DisplayCurveBytes=DisplayCurveEntries*3*sizeof(float);
// values = {src R, G, B, dst R, G, B}; false when a component is not finite and positive.
bool GenerateDisplayCurve(const std::array<float,6>& values,std::span<uint8_t,DisplayCurveBytes> interleaved) noexcept;
// The three 1025-float slices (R, G, B) of an interleaved curve, as the Texture1DArray rows.
void DeinterleaveDisplayCurve(std::span<const uint8_t,DisplayCurveBytes> interleaved,std::array<std::array<float,DisplayCurveEntries>,3>& slices) noexcept;
// The brightness values the render step produces: src 2.2, dst 2.2 + 0.5 b per channel (b = the engine's brightness term).
constexpr std::array<float,6> DisplayCurveIdentity{2.2f,2.2f,2.2f,2.2f,2.2f,2.2f};
// CPU model of the pass on one 8-bit channel value: Load of the 8-bit value c, a linear sample of the 1025-entry slice at
// texel coordinate c * 1025 - 0.5 (the half-texel offset the original's shader has), with the hardware's 8-bit
// filter-weight quantisation, rounded to 8 bits.
uint8_t DisplayPassModel(const std::array<float,DisplayCurveEntries>& slice,uint8_t value) noexcept;
}
