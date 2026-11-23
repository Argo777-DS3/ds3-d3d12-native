#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <vector>

// The memory facade (first written for the former standalone live host), used by the native
// startup source. Inline definitions retain one publication table per module.
namespace ds3hostio {
namespace fs=std::filesystem;
// I-0051: an entry may be lazy (`make` builds its text lines on first read: the live inputs arrive as binary records and
// their text form is built only for a reader that still wants it, e.g. a frame saved for offline replay) and may carry
// typed records (`typed`, of type `type`) that binary readers use instead of the text.
struct Entry { std::shared_ptr<const std::vector<uint8_t>> bytes; std::shared_ptr<const std::vector<std::string>> lines; std::function<std::vector<std::string>()> make;
    std::shared_ptr<const void> typed; const std::type_info* type{}; uint64_t serial{}; bool exists{}; };
inline std::recursive_mutex mu;   // I-0051: the tables are shared by the render thread and the composer thread (no file is replaced while the other reads it)
#define IO_LOCK std::lock_guard<std::recursive_mutex> io_lock_(mu)
inline std::unordered_map<std::wstring,Entry> files; inline std::map<std::wstring,std::vector<fs::path>> listings;
inline uint64_t serial=0, disk_reads=0, disk_bytes=0;
inline std::wstring Key(const fs::path& p) { std::wstring s=p.lexically_normal().wstring(); for(auto& c:s) { if(c==L'/') c=L'\\'; c=static_cast<wchar_t>(towlower(c)); } return s; }
// A native source publishes all of its current inputs. Missing entries under
// its root remain missing; they cannot acquire a previous capture from disk.
inline std::set<std::wstring> memory_roots;
inline bool MemoryKey(const std::wstring& key) { for(const auto& root:memory_roots)
    if(key==root || (key.size()>root.size() && key[root.size()]==L'\\' && !key.compare(0,root.size(),root))) return true;
    return false; }
inline void MemoryOnly(const fs::path& root) { IO_LOCK; memory_roots.insert(Key(root)); }
inline void Materialize(Entry& e) { if(e.bytes || !e.make) return; auto lines=std::make_shared<std::vector<std::string>>(e.make()); auto b=std::make_shared<std::vector<uint8_t>>();
    size_t n=0; for(const auto& l:*lines) n+=l.size()+1; b->reserve(n); for(const auto& l:*lines) { b->insert(b->end(),l.begin(),l.end()); b->push_back('\n'); }
    e.lines=std::move(lines); e.bytes=std::move(b); }
// Find: the entry, read from disk on first use, lazy text not built (existence, serial, typed records).
inline Entry& Find(const fs::path& p) { IO_LOCK; const auto k=Key(p); if(auto it=files.find(k); it!=files.end()) return it->second;
    Entry e; e.serial=++serial; auto v=std::make_shared<std::vector<uint8_t>>();
    if(MemoryKey(k)) { e.bytes=v; return files.emplace(k,std::move(e)).first->second; }
    if(std::ifstream in(p,std::ios::binary); in) { e.exists=true; v->assign(std::istreambuf_iterator<char>(in),{}); ++disk_reads; disk_bytes+=v->size(); }
    e.bytes=v; return files.emplace(k,std::move(e)).first->second; }
inline Entry& Get(const fs::path& p) { IO_LOCK; Entry& e=Find(p); Materialize(e); return e; }
inline const std::vector<uint8_t>& Bytes(const fs::path& p) { return *Get(p).bytes; }
// I-0120 D7: the shared pointer taken while the lock is held (a Put or Remove on another thread may replace the entry after it).
// Bytes, Lines and List still return references into the entry: their callers read on the one thread that writes (gparam_lock).
inline std::shared_ptr<const std::vector<uint8_t>> Shared(const fs::path& p) { IO_LOCK; return Get(p).bytes; }
inline uint64_t Serial(const fs::path& p) { return Find(p).serial; }
inline bool Exists(const fs::path& p) { return Find(p).exists; }
inline const std::vector<std::string>& Lines(const fs::path& p) { IO_LOCK; Entry& e=Get(p);
    if(!e.lines) { auto out=std::make_shared<std::vector<std::string>>(); const auto& b=*e.bytes; size_t a=0;
        for(size_t i=0;i<=b.size();++i) if(i==b.size() || b[i]=='\n') { size_t z=i; if(z>a && b[z-1]=='\r') --z; if(z>a) out->emplace_back(reinterpret_cast<const char*>(b.data())+a,z-a); a=i+1; }
        e.lines=out; }
    return *e.lines; }
// Typed records of `p` (null when it has none: a text file read from disk).
template<class T> std::shared_ptr<const T> Typed(const fs::path& p) { IO_LOCK; const Entry& e=Find(p); return e.typed && e.type && *e.type==typeid(T) ? std::static_pointer_cast<const T>(e.typed) : nullptr; }
// PutLazy: a file whose text is built by `make` on first read, with its typed records.
template<class T> void PutLazy(const fs::path& p,std::shared_ptr<const T> typed,std::function<std::vector<std::string>()> make) {
    IO_LOCK; Entry e; e.serial=++serial; e.exists=true; e.make=std::move(make); e.typed=std::move(typed); e.type=&typeid(T); const auto k=Key(p); files[k]=std::move(e); }
inline void Put(const fs::path& p,std::shared_ptr<const std::vector<uint8_t>> b) { IO_LOCK; Entry e; e.serial=++serial; e.exists=true; e.bytes=std::move(b); const auto k=Key(p); files[k]=std::move(e); }
// Tombstone virtual content so a missing current incarnation cannot reload an old disk/file cache entry.
inline void Remove(const fs::path& p) { IO_LOCK; Entry e; e.serial=++serial; e.bytes=std::make_shared<const std::vector<uint8_t>>(); const auto k=Key(p); files[k]=std::move(e); }
inline void Put(const fs::path& p,std::vector<uint8_t> b) { Put(p,std::make_shared<const std::vector<uint8_t>>(std::move(b))); }
inline const std::vector<fs::path>& List(const fs::path& dir) { IO_LOCK; const auto k=Key(dir); if(auto it=listings.find(k); it!=listings.end()) return it->second;
    if(MemoryKey(k)) return listings.emplace(k,std::vector<fs::path>{}).first->second;
    std::vector<fs::path> v; std::error_code ec; for(const auto& e:fs::directory_iterator(dir,ec)) v.push_back(e.path()); std::sort(v.begin(),v.end());
    {   // LH_LIST_NUMERIC (control, I-0051): numbered files (models, packages) in number order, the live publication order
        static const bool numeric=[]{ size_t n=0; return !getenv_s(&n,nullptr,0,"LH_LIST_NUMERIC") && n; }();
        if(numeric) std::stable_sort(v.begin(),v.end(),[](const fs::path& a,const fs::path& b) { const auto na=a.filename().wstring(), nb=b.filename().wstring(); const bool da=!na.empty() && iswdigit(na[0]), db=!nb.empty() && iswdigit(nb[0]);
            if(da!=db) return da; if(!da) return false; return std::wcstoull(na.c_str(),nullptr,10)<std::wcstoull(nb.c_str(),nullptr,10); }); } return listings.emplace(k,std::move(v)).first->second; }
// Virtual folders (live mode): files published into a folder are listed in publication order.
inline void AddListing(const fs::path& dir,const fs::path& file) { IO_LOCK; listings[Key(dir)].push_back(file); }
// I-0097 tranche 5: a withdrawn file leaves its folder's listing (its tombstone stays in `files`), so a file admitted
// again is listed once, not once more at every (re-)admission.
inline void Unlist(const fs::path& dir,const fs::path& file) { IO_LOCK; const auto it=listings.find(Key(dir)); if(it==listings.end()) return;
    const auto k=Key(file); std::erase_if(it->second,[&](const fs::path& f) { return Key(f)==k; }); }
// A virtual folder's files set to exactly `current`: a file no longer current is tombstoned and unlisted, a new one listed
// once, every current one put; `published` keeps the folder's current paths between calls.
inline void PublishFolder(const fs::path& dir,std::set<fs::path>& published,const std::map<fs::path,std::shared_ptr<const std::vector<uint8_t>>>& current) {
    IO_LOCK; for(const auto& path:published) if(!current.contains(path)) { Remove(path); Unlist(dir,path); }
    for(const auto& [path,bytes]:current) { if(!published.contains(path)) AddListing(dir,path); Put(path,bytes); }
    published.clear(); for(const auto& entry:current) published.insert(entry.first); }
inline std::vector<uint8_t> ReadForProducers(const fs::path& p) { IO_LOCK; return Bytes(p); }   // D7: copied under the lock
inline std::shared_ptr<const std::vector<uint8_t>> SharedForProducers(const fs::path& p) { return Shared(p); }   // I-0052
}
#undef IO_LOCK
