#include "host_input.h"
#include "light_producers.h"
#include <cstring>
#include <iostream>

int main() {
    // This existing source directory proves missing virtual inputs cannot
    // fall through to real disk files. No files are written or removed.
    const std::filesystem::path root=L"native/content";
    if(!std::filesystem::exists(root/L"gparam.cpp"))return 1;
    ds3hostio::MemoryOnly(root);
    if(ds3hostio::Exists(root/L"gparam.cpp") || !ds3hostio::List(root).empty())return 2;
    auto bytes=std::make_shared<std::vector<uint8_t>>(0x54);
    std::memcpy(bytes->data(),"f\0i\0l\0t\0",8);
    const uint32_t game=3,header=0x54;
    std::memcpy(bytes->data()+8,&game,4);std::memcpy(bytes->data()+24,&header,4);
    const auto file=root/L"default.gparam";
    ds3hostio::AddListing(root,file);ds3hostio::Put(file,bytes);
    ds3light::UseReader(&ds3hostio::ReadForProducers);
    ds3light::UseListingReader(&ds3hostio::List);
    auto banks=std::make_shared<ds3light::Banks>();
    if(!banks->Load(root) || banks->Files()!=1 || ds3hostio::disk_reads)return 3;
    const std::shared_ptr<const ds3light::Banks> owned=banks;
    ds3hostio::PutLazy<ds3light::Banks>(root/L"current-banks",owned,[]{return std::vector<std::string>{};});
    const auto retained=ds3hostio::Typed<ds3light::Banks>(root/L"current-banks");
    const auto version=ds3hostio::Serial(file);
    ds3hostio::Remove(file);
    ds3light::Banks next;
    if(next.Load(root) || next.Files() || ds3hostio::Serial(file)<=version || !retained || retained->Files()!=1)return 4;
    if(ds3hostio::disk_reads || !ds3hostio::Bytes(file).empty())return 5;
    std::cout<<"Host memory source -> existing Banks: no disk fallback, real GPARAM parsing, withdrawal and retained snapshot passed\n";
}
