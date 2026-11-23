// I-0101 (STUDY §2): the image of the original's GX MTD object (`GXFLV::GXFlverMaterial` slot 7, 0x140140410, returns
// [material+0x80]) as far as DS3's CPU readers use it: FUN_140667430, a burning map object's material pass, reads the wide
// string at +0x48 in the MSVC layout (inline under 8 characters, else a pointer; size +0x58, capacity +0x60) and compares it
// whole with its Burn MTDs. The name is the stem of the MTD's shader name (FUN_14002DD60). The image is built once with the
// material and never moves (a long name's pointer is into `name`).
#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

namespace ds3native::bootstrap {
struct GxMtdImage {alignas(16) std::array<uint8_t,0x100> bytes{};std::wstring name;};
inline std::shared_ptr<GxMtdImage> MakeGxMtdImage(std::wstring name) {
    auto made=std::make_shared<GxMtdImage>();made->name=std::move(name);
    const uint64_t size=made->name.size(),capacity=size<8?7:size;
    if(size<8)std::memcpy(made->bytes.data()+0x48,made->name.c_str(),(size+1)*sizeof(wchar_t));
    else{const auto text=reinterpret_cast<uintptr_t>(made->name.c_str());std::memcpy(made->bytes.data()+0x48,&text,8);}
    std::memcpy(made->bytes.data()+0x58,&size,8);std::memcpy(made->bytes.data()+0x60,&capacity,8);
    return made;
}
}
