// W5d GL-1 (GLOW-WAVE1-READY 3): the glow chain's constant images as pure functions, the cores of the snapshot producers
// GlowTaps and GlowParam (native/world/light_producers) and of the native glow stage.
#pragma once
#include <array>
#include <cstdint>

namespace ds3native::glow {
// The 13 tap offsets of FUN_14007FED0 (.rdata 143D7FB10, 7D0, 80250, 7FA10, 7FA20, 80210 pairs, then {-0.5, [143D7F0BC]}), as
// the bit images read from the executable; the lower pair keeps the original's y = -0.707101 (BF35048B) against +0.707107.
inline constexpr std::array<uint32_t,26> tap_bits{
    0x00000000,0x00000000, 0xBF000000,0x3F3504E6, 0x3F000000,0x3F3504E6, 0xBF800000,0x00000000, 0x3F800000,0x00000000,
    0xBF000000,0xBF35048B, 0x3F000000,0xBF35048B, 0x3F000000,0x3E715BEF, 0x00000000,0x3EF15BEF, 0xBF000000,0x3E715BEF,
    0x3F000000,0xBE715BEF, 0x00000000,0xBEF15BEF, 0xBF000000,0xBE715BEF};
// k = [143D7F0C0] = -0.5 and 2 pi = [143D7EEFC] (bit images).
inline constexpr uint32_t k_bits=0xBF000000,two_pi_bits=0x40C90FDB;
// cbSampleOffset (256 bytes): entry i = (x_i s, y_i s, w_i / sum w, 0), w_i = expf((y y + x x) k) / sqrtf(2 pi) summed in index
// order; entries 13..15 zero. `taps` and the two constants are passed so the snapshot producer can hand the words it read.
void TapImage(float scale,uint8_t out[256],const uint32_t* taps=tap_bits.data(),uint32_t k=k_bits,uint32_t two_pi=two_pi_bits);
// cbFilterParam (32 bytes): (w/64, h/64, 0, 0), (1/w, 1/h, 0, 0) for the target's integer size.
void FilterParamImage(uint32_t width,uint32_t height,uint8_t out[32]);
// cbGlowParam (16 bytes): ((0 - g) s + g, the raw AlphaScale dword, 0, 0) with s = clamp(1 - factor, 0, 1) as FadeSelect
// computes it (one - factor, then 0 below 0 and 1 above 1).
void ParamImage(float g,uint32_t alpha_bits,float factor,uint8_t out[16]);
// The fade select of 1BB810 (s = clamp(one - fade)): `select` for a given one and value.
float FadeSelect(float one,float value);
// Level i of the chain (0 = L0 a quarter of the source, then halves): ceil(W / (4 2^i)) x ceil(H / (4 2^i)).
struct Size {uint32_t width{},height{};};
Size LevelSize(uint32_t width,uint32_t height,unsigned level);
}
