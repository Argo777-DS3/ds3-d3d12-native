#pragma once
// I-0137 (I-0120 A1): the original executable is known by what it is, not by where it is installed. The controller injects
// only into Dark Souls III 1.15.2.0; it used to compare the process's image path with the default Steam folder, so a game in
// another Steam library could never be started natively. The image is now checked by its file name, its size and its PE
// header's words (no path is compared). The header alone is read: the controller injects as soon as the process exists and
// a hash of the whole 85 MB file would delay it; the launcher hashes the file before the game starts, and the build checks
// the words below against the executable it pins by SHA-256 (Build-GameBootstrap.ps1), so the three cannot drift apart.
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <cwchar>

namespace ds3native::bootstrap {
struct OriginalImagePin {uint64_t file_bytes;uint32_t time_date_stamp,size_of_image,checksum;};
inline constexpr OriginalImagePin original_image_pin{88960032,0x639C4DDD,0x056F4000,0x054D6C82};
enum class OriginalImage {Ok,NotNamed,Unreadable,NotOriginal};
inline const char* OriginalImageName(OriginalImage result) noexcept {
    switch(result){case OriginalImage::Ok:return "ok";case OriginalImage::NotNamed:return "not_named_darksoulsiii_exe";
    case OriginalImage::Unreadable:return "image_file_unreadable";default:return "not_the_1_15_2_executable";}
}
inline OriginalImage CheckOriginalImage(const wchar_t* path) noexcept {
    if(!path || !*path)return OriginalImage::Unreadable;
    const wchar_t* name=path;
    for(const wchar_t* at=path;*at;++at)if(*at==L'\\' || *at==L'/')name=at+1;
    if(_wcsicmp(name,L"DarkSoulsIII.exe"))return OriginalImage::NotNamed;
    // A path past MAX_PATH (a deep Steam library) opens only in its extended form.
    wchar_t extended[32768+8]{};const auto length=wcslen(path);
    if(length>=MAX_PATH && length<32768 && path[1]==L':'){wcscpy_s(extended,L"\\\\?\\");wcscat_s(extended,path);
        for(auto* at=extended+4;*at;++at)if(*at==L'/')*at=L'\\';path=extended;}
    // The running image is mapped by the loader: it is opened for reading whatever access the others hold (only its head is read).
    const HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)return OriginalImage::Unreadable;
    struct Close {HANDLE h;~Close(){CloseHandle(h);}} const close{file};
    LARGE_INTEGER size{};
    if(!GetFileSizeEx(file,&size))return OriginalImage::Unreadable;
    if(static_cast<uint64_t>(size.QuadPart)!=original_image_pin.file_bytes)return OriginalImage::NotOriginal;
    BYTE head[4096]{};DWORD got{};
    if(!ReadFile(file,head,sizeof(head),&got,nullptr))return OriginalImage::Unreadable;
    if(got!=sizeof(head))return OriginalImage::NotOriginal;
    IMAGE_DOS_HEADER dos{};std::memcpy(&dos,head,sizeof(dos));
    if(dos.e_magic!=IMAGE_DOS_SIGNATURE || dos.e_lfanew<0 || static_cast<size_t>(dos.e_lfanew)+sizeof(IMAGE_NT_HEADERS64)>sizeof(head))return OriginalImage::NotOriginal;
    IMAGE_NT_HEADERS64 nt{};std::memcpy(&nt,head+dos.e_lfanew,sizeof(nt));
    const bool original=nt.Signature==IMAGE_NT_SIGNATURE && nt.FileHeader.Machine==IMAGE_FILE_MACHINE_AMD64 &&
        nt.OptionalHeader.Magic==IMAGE_NT_OPTIONAL_HDR64_MAGIC && nt.FileHeader.TimeDateStamp==original_image_pin.time_date_stamp &&
        nt.OptionalHeader.SizeOfImage==original_image_pin.size_of_image && nt.OptionalHeader.CheckSum==original_image_pin.checksum;
    return original?OriginalImage::Ok:OriginalImage::NotOriginal;
}
}
