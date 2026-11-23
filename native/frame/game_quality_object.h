#pragma once
#include <array>
#include <cstdint>

namespace ds3native::bootstrap {
// I-0067 QUALITY-OBJECT-NATIVE: the GX quality object [GXRS+0x5360] (0x78 bytes) rewritten from the quality table
// [GXRS+0x5358] (0x24C bytes, filled by 1400219B0) at the twelve slot levels, as 140023790 does when a level changed.
constexpr size_t QualityTableBytes=0x24C,QualityObjectBytes=0x78;
// Writes each field in place, in the original's order; padding +0x69..+0x6B is left as it is. Levels must be 0..4.
bool WriteQualityObject(const uint8_t* table,const std::array<int32_t,12>& levels,uint8_t* object) noexcept;
// O2 (options sheet section 4): what a change of the graphics options recreates, by class. P parameters only; G a pass
// appears or disappears (resources and PSOs on first enable, the frame graph's plan key changes); R an existing resource
// changes size or count (ring-wide wait, recreation); H a temporal history is reset.
enum OptionClass : uint8_t {OptionParameter=1,OptionGate=2,OptionRecreate=4,OptionHistory=8};
// The classes of a change of the twelve slot levels and of the configured render size: the union over the changed slots,
// and the changed slots as bits 0..11 (bit 12: the render size).
struct OptionChangeClass {uint8_t classes{};uint32_t changed{};};
OptionChangeClass ClassifyOptionChange(const std::array<int32_t,12>& before,const std::array<int32_t,12>& after,
    bool render_size_changed) noexcept;
// "PGRH" letters of a class union, for the rows.
void OptionClassLetters(uint8_t classes,char (&letters)[5]) noexcept;
}
