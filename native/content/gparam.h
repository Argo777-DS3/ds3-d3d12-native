#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace ds3content {
// I-0045: DS3 GPARAM ("filt", game 3) graphics parameter files (param/drawparam/*.gparam.dcx): groups of params, each
// a typed value list with one value ID per value. Read-only; layout as documented by SoulsFormats' GPARAM.
struct GparamParam {
    std::string name1, name2;
    uint8_t type{};                                // 1 byte, 2 short, 3/7 int, 5/0xB bool, 9 float, 0xC/0xD/0xE float2/3/4, 0xF byte4
    std::vector<int32_t> ids;                      // value ID of each value
    std::vector<std::array<uint32_t,4>> values;    // raw 32-bit words of each value (floats as bits; ints/bools widened)
    unsigned Components() const noexcept;          // words a value carries (float2 = 2, float4 = 4, scalars 1)
    int IndexOf(int32_t id) const noexcept;        // value index of an ID, -1 if absent
    float Float(size_t value,unsigned component=0) const noexcept;
};
struct GparamGroup { std::string name1, name2; std::vector<GparamParam> params; };
struct Gparam {
    uint32_t game{}; std::vector<GparamGroup> groups;
    const GparamGroup* Group(const std::string& name2) const noexcept;   // by short name (e.g. "LightSet", "Fog")
};
Gparam ReadGparam(const std::vector<uint8_t>& bytes);   // raw or DCX/DFLT
}
