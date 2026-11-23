// I-0101 (STUDY §2): the GX MTD image read back as DS3's FUN_140667430 reads it (0x14066755A: `CMP [RAX+0x60],8`, the
// inline buffer at +0x48 under 8, else the pointer there; then a whole compare, terminator included, with its Burn MTDs).
#include "gx_mtd_image.h"
#include <cstdio>
#include <cwchar>
#include <stdexcept>

namespace {
void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
const wchar_t* AsDs3Reads(const ds3native::bootstrap::GxMtdImage& image){
    uint64_t capacity{};std::memcpy(&capacity,image.bytes.data()+0x60,8);
    if(capacity<8)return reinterpret_cast<const wchar_t*>(image.bytes.data()+0x48);
    uintptr_t text{};std::memcpy(&text,image.bytes.data()+0x48,8);return reinterpret_cast<const wchar_t*>(text);
}
bool Ds3Equal(const wchar_t* read,const wchar_t* target){const auto n=std::wcslen(target)+1;return !std::wmemcmp(read,target,n);}
}
int main(){try{
    const auto burn=ds3native::bootstrap::MakeGxMtdImage(L"GXFlver_ColDifSpcBumpIbl_Burn");
    Require(Ds3Equal(AsDs3Reads(*burn),L"GXFlver_ColDifSpcBumpIbl_Burn"),"a long stem is read through the pointer, whole");
    Require(!Ds3Equal(AsDs3Reads(*burn),L"GXFlver_ColDifSpcBumpRNM_Burn"),"another Burn MTD does not match");
    uint64_t size{},capacity{};std::memcpy(&size,burn->bytes.data()+0x58,8);std::memcpy(&capacity,burn->bytes.data()+0x60,8);
    Require(size==29 && capacity==29,"size and capacity of a long stem");
    const auto copy=burn;Require(AsDs3Reads(*copy)==AsDs3Reads(*burn),"the same pointer every call");
    const auto cloth=ds3native::bootstrap::MakeGxMtdImage(L"BurnCloth_cloth");
    Require(Ds3Equal(AsDs3Reads(*cloth),L"BurnCloth_cloth"),"BurnCloth_cloth (15 characters)");
    const auto inline_name=ds3native::bootstrap::MakeGxMtdImage(L"Water");
    std::memcpy(&capacity,inline_name->bytes.data()+0x60,8);
    Require(capacity==7 && AsDs3Reads(*inline_name)==reinterpret_cast<const wchar_t*>(inline_name->bytes.data()+0x48),"a short stem is inline");
    Require(Ds3Equal(AsDs3Reads(*inline_name),L"Water") && !Ds3Equal(AsDs3Reads(*inline_name),L"BurnCloth_cloth"),"inline, whole, no Burn match");
    const auto empty=ds3native::bootstrap::MakeGxMtdImage({});
    Require(*AsDs3Reads(*empty)==0 && !Ds3Equal(AsDs3Reads(*empty),L"GXFlver_ColDifSpcBumpIbl_Burn"),"the empty image matches nothing");
    std::printf("PASS GX MTD image: read back as FUN_140667430 reads it (inline under 8, else the pointer; whole compare)\n");
    return 0;
}catch(const std::exception& e){std::printf("FAIL GX MTD image: %s\n",e.what());return 1;}}
