// I-0097 tranche 5 control: the GPARAM bank folder's listing after DS3's reloads. ds3hostio::PublishFolder, the host step
// of PublishNativeGparamBanks, is run on ten files admitted and retired fifty times, as DS3 reloads the same map parts: each
// publication lists each current file once, Banks::Load loads the ten files, and a withdrawn file is neither listed nor
// loaded. Before the fix, a file was listed once more at every re-admission.
#include "../world/host_input.h"
#include "../world/light_producers.h"
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <set>
#include <string>

namespace {
int failures{},checks{};
void Expect(bool ok,const std::string& what) {++checks;if(!ok){++failures;std::printf("FAIL %s\n",what.c_str());}}
// The smallest GPARAM ReadGparam accepts (host_input_test's): "filt", game 3, header 0x54, no group.
std::shared_ptr<const std::vector<uint8_t>> Bytes() {
    auto bytes=std::make_shared<std::vector<uint8_t>>(0x54);std::memcpy(bytes->data(),"f\0i\0l\0t\0",8);
    const uint32_t game=3,header=0x54;std::memcpy(bytes->data()+8,&game,4);std::memcpy(bytes->data()+24,&header,4);return bytes;}
std::set<std::filesystem::path> published;
std::shared_ptr<const ds3light::Banks> banks;
// PublishNativeGparamBanks' publication of the files `current` into the folder `folder`, then its bank load.
void Publish(const std::filesystem::path& folder,const std::map<std::filesystem::path,std::shared_ptr<const std::vector<uint8_t>>>& current) {
    ds3hostio::PublishFolder(folder,published,current);
    auto next=std::make_shared<ds3light::Banks>();next->Load(folder,banks.get());banks=next;}
}
int main() {
    const std::filesystem::path root=L"gparam-listing-test-root";   // a memory-only root: nothing is read from or written to disk
    ds3hostio::MemoryOnly(root);
    ds3light::UseReader(&ds3hostio::ReadForProducers);   // as PublishNativeGparamBanks: the banks' text files read from memory too
    ds3light::UseSharedReader(&ds3hostio::SharedForProducers);
    ds3light::UseListingReader(&ds3hostio::List);
    const auto folder=root/L"banks";
    std::map<std::filesystem::path,std::shared_ptr<const std::vector<uint8_t>>> zone;
    zone[folder/L"default.gparam"]=Bytes();
    for(int i=0;i<9;++i)zone[folder/(L"m40_00_000"+std::to_wstring(i)+L".gparam")]=Bytes();
    for(int reload=0;reload<50;++reload) {
        // DS3 admits the map part's files one by one (a publication each), then retires all but default at the reload.
        std::map<std::filesystem::path,std::shared_ptr<const std::vector<uint8_t>>> current;
        for(const auto& [path,bytes]:zone){current[path]=reload%2?Bytes():bytes;Publish(folder,current);}
        Expect(ds3hostio::List(folder).size()==10,"reload "+std::to_string(reload)+": ten files listed, not "+std::to_string(ds3hostio::List(folder).size()));
        Expect(banks && banks->Files()==10,"reload "+std::to_string(reload)+": ten files loaded");
        current.erase(std::next(current.begin()),current.end());Publish(folder,current);
        Expect(ds3hostio::List(folder).size()==1,"reload "+std::to_string(reload)+": the retired files leave the listing");
        Expect(banks && banks->Files()==1,"reload "+std::to_string(reload)+": the retired files are not loaded");
        Expect(ds3hostio::Bytes(folder/L"m40_00_0000.gparam").empty(),"reload "+std::to_string(reload)+": a retired file keeps its tombstone");
    }
    Expect(!ds3hostio::disk_reads,"no disk read");
    if(failures){std::printf("gparam_listing_test: %d of %d checks failed\n",failures,checks);return 1;}
    std::printf("gparam_listing_test: %d checks passed: fifty reloads of ten GPARAMs keep one listing entry a file\n",checks);
}
