// I-0137 (I-0120 A1): the controller knows the original executable by its name, size and PE header, in any folder.
// argv[1]: the executable the build pins by SHA-256 (Build-GameBootstrap.ps1).
#include "original_image.h"
#include <winioctl.h>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using ds3native::bootstrap::CheckOriginalImage;using ds3native::bootstrap::OriginalImage;using ds3native::bootstrap::original_image_pin;
namespace {
// What the test made, removed at its end and before a failure's exit (the step review's L3): files, then folders deepest first.
std::vector<std::wstring> made_files,made_folders;
bool Cleanup(){bool clean=true;
    for(const auto& file:made_files)clean=(DeleteFileW(file.c_str()) || GetLastError()==ERROR_FILE_NOT_FOUND || GetLastError()==ERROR_PATH_NOT_FOUND) && clean;
    for(auto folder=made_folders.rbegin();folder!=made_folders.rend();++folder)clean=(RemoveDirectoryW(folder->c_str()) || GetLastError()==ERROR_FILE_NOT_FOUND || GetLastError()==ERROR_PATH_NOT_FOUND) && clean;
    made_files.clear();made_folders.clear();return clean;}
void Require(bool held,const char* what){if(!held){std::printf("FAIL original image: %s\n",what);Cleanup();std::exit(1);}}
void Folder(const std::wstring& path){Require(CreateDirectoryW(path.c_str(),nullptr) || GetLastError()==ERROR_ALREADY_EXISTS,"a test folder made");made_folders.push_back(path);}
// A file of `bytes` bytes whose first bytes are `head` (the rest a sparse hole: nothing is written to the disk).
void Make(const std::wstring& path,const std::vector<BYTE>& head,uint64_t bytes){
    const HANDLE file=CreateFileW(path.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    Require(file!=INVALID_HANDLE_VALUE,"a test file created");
    bool listed=false;for(const auto& known:made_files)listed=listed || known==path;
    if(!listed)made_files.push_back(path);
    DWORD done{};DeviceIoControl(file,FSCTL_SET_SPARSE,nullptr,0,nullptr,0,&done,nullptr);
    const DWORD count=static_cast<DWORD>(head.size()<bytes?head.size():bytes);
    const bool written=!count || (WriteFile(file,head.data(),count,&done,nullptr) && done==count);
    LARGE_INTEGER end{};end.QuadPart=static_cast<LONGLONG>(bytes);
    const bool sized=SetFilePointerEx(file,end,nullptr,FILE_BEGIN) && SetEndOfFile(file);
    CloseHandle(file);Require(written && sized,"a test file's head written and its size set");
}
}
int wmain(int argc,wchar_t** argv){
    Require(argc==2,"usage: original_image_test <the pinned executable>");
    // T1: the pinned executable where it is installed.
    Require(CheckOriginalImage(argv[1])==OriginalImage::Ok,"T1 the pinned executable accepted");
    // T2: another program (this test) is not named DarkSoulsIII.exe; no path, an empty one.
    wchar_t self[MAX_PATH]{};GetModuleFileNameW(nullptr,self,MAX_PATH);
    Require(CheckOriginalImage(self)==OriginalImage::NotNamed,"T2 another program refused by its name");
    Require(CheckOriginalImage(nullptr)==OriginalImage::Unreadable && CheckOriginalImage(L"")==OriginalImage::Unreadable,"T2 no path refused");
    // The original's first 4096 bytes, for the copies below.
    std::vector<BYTE> head(4096);
    {const HANDLE file=CreateFileW(argv[1],GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr);DWORD got{};
        const bool read=file!=INVALID_HANDLE_VALUE && ReadFile(file,head.data(),4096,&got,nullptr) && got==4096;
        if(file!=INVALID_HANDLE_VALUE)CloseHandle(file);Require(read,"the pinned executable's head read");}
    wchar_t temp[MAX_PATH]{};GetTempPathW(MAX_PATH,temp);
    // T3: another folder (a space and a letter outside ASCII in its name), the name in another case, forward slashes: accepted.
    const std::wstring folder=std::wstring(temp)+L"ds3 original image t\u00E9st "+std::to_wstring(GetCurrentProcessId());
    Folder(folder);
    const std::wstring copy=folder+L"\\darksoulsiii.EXE";
    Make(copy,head,original_image_pin.file_bytes);
    Require(CheckOriginalImage(copy.c_str())==OriginalImage::Ok,"T3 the same image in another folder accepted");
    std::wstring slashed=copy;for(auto& c:slashed)if(c==L'\\')c=L'/';
    Require(CheckOriginalImage(slashed.c_str())==OriginalImage::Ok,"T3 forward slashes accepted");
    // T4: a missing file of that name.
    Require(CheckOriginalImage((folder+L"\\missing\\DarkSoulsIII.exe").c_str())==OriginalImage::Unreadable,"T4 a missing file unreadable");
    // T5: another size (the size is asked before the head is read: a shorter file never reaches the head's short read).
    Make(copy,head,original_image_pin.file_bytes-1);Require(CheckOriginalImage(copy.c_str())==OriginalImage::NotOriginal,"T5 one byte shorter refused");
    Make(copy,head,original_image_pin.file_bytes+1);Require(CheckOriginalImage(copy.c_str())==OriginalImage::NotOriginal,"T5 one byte longer refused");
    Make(copy,head,16);Require(CheckOriginalImage(copy.c_str())==OriginalImage::NotOriginal,"T5 a 16-byte file refused");
    Make(copy,{},0);Require(CheckOriginalImage(copy.c_str())==OriginalImage::NotOriginal,"T5 an empty file refused");
    // T6: the right size, each pinned header word changed in turn; no PE header at all.
    IMAGE_DOS_HEADER dos{};std::memcpy(&dos,head.data(),sizeof(dos));
    const size_t nt=static_cast<size_t>(dos.e_lfanew);
    const size_t words[]{nt+offsetof(IMAGE_NT_HEADERS64,FileHeader)+offsetof(IMAGE_FILE_HEADER,TimeDateStamp),
        nt+offsetof(IMAGE_NT_HEADERS64,OptionalHeader)+offsetof(IMAGE_OPTIONAL_HEADER64,SizeOfImage),
        nt+offsetof(IMAGE_NT_HEADERS64,OptionalHeader)+offsetof(IMAGE_OPTIONAL_HEADER64,CheckSum),
        nt+offsetof(IMAGE_NT_HEADERS64,Signature),0};
    for(const auto at:words){auto changed=head;changed[at]^=1;Make(copy,changed,original_image_pin.file_bytes);
        Require(CheckOriginalImage(copy.c_str())==OriginalImage::NotOriginal,"T6 a changed header word refused");}
    {auto changed=head;const LONG far_away=0x7FFFFFF0;std::memcpy(changed.data()+offsetof(IMAGE_DOS_HEADER,e_lfanew),&far_away,4);
        Make(copy,changed,original_image_pin.file_bytes);Require(CheckOriginalImage(copy.c_str())==OriginalImage::NotOriginal,"T6 a header offset outside the head refused");}
    // T7 (the step review's L1): a path past MAX_PATH, given with forward slashes: the extended-form branch. Made and removed
    // through the extended form; asked in the plain one, as QueryFullProcessImageNameW gives it. Only on a lettered drive.
    bool long_path=false;
    if(folder.size()>2 && folder[1]==L':'){
        std::wstring deep=folder;
        for(int level=0;level<4;++level){deep+=L"\\"+std::wstring(70,L'd');Folder(L"\\\\?\\"+deep);}
        const std::wstring plain=deep+L"\\DarkSoulsIII.exe";
        Require(plain.size()>=MAX_PATH+40,"T7 the path is past MAX_PATH");
        Make(L"\\\\?\\"+plain,head,original_image_pin.file_bytes);
        std::wstring asked=plain;for(auto& c:asked)if(c==L'\\')c=L'/';
        Require(CheckOriginalImage(asked.c_str())==OriginalImage::Ok,"T7 the image accepted on a path past MAX_PATH");
        Make(L"\\\\?\\"+plain,head,original_image_pin.file_bytes-1);
        Require(CheckOriginalImage(asked.c_str())==OriginalImage::NotOriginal,"T7 another size refused on a path past MAX_PATH");
        long_path=true;}
    if(!Cleanup()){std::puts("FAIL original image: the test's files and folders removed");return 1;}
    std::printf("PASS original image: T1 the pinned executable accepted where it is; T2 another program and no path refused; T3 the same image accepted in another folder, in another case, with forward slashes; T4 a missing file unreadable; T5 another size, a 16-byte and an empty file refused; T6 each pinned header word changed, and a header offset outside the head, refused; T7 a path past MAX_PATH %s; nothing left behind\n",
        long_path?"accepted and refused alike":"not tried (the temporary folder is not on a lettered drive)");
    return 0;
}
