#include "game_primitive_geometry.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <thread>

using ds3native::bootstrap::PrimitiveGeometryStorage;
void Require(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
template<class F> void Refuses(F action) {bool refused{};try{action();}catch(const std::runtime_error&){refused=true;}Require(refused,"unsupported transaction accepted");}
int main() {
    try {
        PrimitiveGeometryStorage storage;
        // First source-qualified startup write in reference10 is272 vertex
        // bytes and120 index bytes. These controls are synthetic CPU payloads,
        // never production assets or a claim to recover that run's geometry.
        auto first=storage.Allocate(17,16,17,60);
        auto* borrowed_vertices=first->Vertices();auto* borrowed_indices=first->Indices();
        for(size_t i=0;i<272;++i)borrowed_vertices[i]=static_cast<uint8_t>((i*37+11)&255);
        for(size_t i=0;i<60;++i){borrowed_indices[i*2]=static_cast<uint8_t>(i%17);borrowed_indices[i*2+1]=0;}
        Require(!storage.Acquire(17),"unsealed data published");
        bool other_thread_refused{};
        std::thread worker([&]{try{first->Vertices();}catch(const std::runtime_error&){other_thread_refused=true;}});worker.join();
        Require(other_thread_refused,"foreign writer accepted");
        auto lease=storage.Seal(first);Require(lease->vertices.identity!=lease->indices.identity,"buffer identities alias");
        std::memset(borrowed_vertices,0,272);std::memset(borrowed_indices,255,120);
        for(size_t i=0;i<272;++i)Require(lease->vertices.bytes[i]==static_cast<uint8_t>((i*37+11)&255),"borrowed mutation changed sealed vertices");
        for(size_t i=0;i<60;++i)Require(lease->indices.bytes[i*2]==i%17 && lease->indices.bytes[i*2+1]==0,"borrowed mutation changed sealed indices");
        Refuses([&]{storage.Seal(first);});
        auto replacement=storage.Allocate(17,8,16,30);Require(!storage.Acquire(17),"old incarnation remained published");
        Require(replacement->Incarnation()!=lease->incarnation,"incarnation reused");
        replacement->Indices()[0]=16;Refuses([&]{storage.Seal(replacement);});
        Require(!storage.Acquire(17),"bad index published");replacement->Indices()[0]=0;
        storage.Seal(replacement);storage.Withdraw(17);Require(!storage.Acquire(17),"withdrawal retained publication");
        Require(lease->vertices.bytes[0]==11,"withdrawal invalidated held lease");
        auto pending=storage.Allocate(19,8,16,30);auto* destination=pending->Vertices();
        storage.Withdraw(19);destination[0]=93;Refuses([&]{storage.Seal(pending);});
        auto stale=storage.Allocate(21,8,16,30);storage.Allocate(21,8,16,30);Refuses([&]{storage.Seal(stale);});
        Refuses([&]{storage.Allocate(0,8,16,30);});Refuses([&]{storage.Allocate(23,8,65537,30);});
        Refuses([&]{storage.Allocate(23,0xffffffffu,65536,30);});
        std::puts("PASS CPU primitive geometry: immutable lease, distinct buffers, thread ownership, replacement, withdrawal with outstanding writer, bounds and bad index refusal");return 0;
    }catch(const std::exception& error){std::fprintf(stderr,"FAIL: %s\n",error.what());return 1;}
}
