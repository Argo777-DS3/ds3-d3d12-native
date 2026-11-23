#include "acquire.h"
#include <bcrypt.h>
#include <atomic>
#include <limits>
namespace ds3content {
namespace {
bool ReadOwnProcess(uintptr_t address,void* out,size_t bytes) noexcept {
    SIZE_T got{};
    return ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void*>(address),out,bytes,&got) && got==bytes;
}
std::atomic<ProcessRead> process_read{&ReadOwnProcess};
}
void SetProcessRead(ProcessRead read) noexcept {process_read.store(read?read:&ReadOwnProcess,std::memory_order_release);}
bool Read(uintptr_t address,void* out,size_t bytes) noexcept {
    if (!address || bytes>32u*1024u*1024u || address>UINTPTR_MAX-bytes) return false;
    return process_read.load(std::memory_order_acquire)(address,out,bytes);
}
bool Sha256(const void* data,size_t bytes,char (&hex)[65]) noexcept {
    if(bytes>ULONG_MAX) return false;
    unsigned char digest[32]{};
    if(BCryptHash(BCRYPT_SHA256_ALG_HANDLE,nullptr,0,
        reinterpret_cast<PUCHAR>(const_cast<void*>(data)),static_cast<ULONG>(bytes),digest,sizeof(digest))) return false;
    constexpr char digits[]="0123456789ABCDEF";
    for(unsigned i=0;i<32;++i) { hex[i*2]=digits[digest[i]>>4]; hex[i*2+1]=digits[digest[i]&15]; }
    hex[64]=0; return true;
}
}
