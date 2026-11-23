#include "spatial_exchange_pairing.hpp"

#include <cstdio>

namespace {
SpatialEvent Event(std::uint64_t sequence,SpatialSite site) {
    SpatialEvent value{};
    value.sequence=sequence; value.tid=7; value.update_index=11; value.site=site;
    value.owner=0x1000; value.descriptor=0x2000; value.selected=0x3040;
    value.pointer_chain_valid=true; value.selected48_valid=true;
    value.payload48_valid=true; value.elapsed={1015580809u,0,0,0};
    return value;
}
bool Equal(PairResult actual,PairResult expected,const char* label) {
    if (actual==expected) return true;
    std::fprintf(stderr,"FAIL: %s actual=%u expected=%u\n",label,
        static_cast<unsigned>(actual),static_cast<unsigned>(expected));
    return false;
}
}

int main() {
    SpatialPairing valid;
    if (!Equal(valid.Process(Event(1,SpatialSite::forward_entry)),PairResult::entry_opened,"forward entry") ||
        !Equal(valid.Process(Event(2,SpatialSite::reverse_entry)),PairResult::entry_opened,"reverse entry") ||
        !Equal(valid.Process(Event(3,SpatialSite::forward_write)),PairResult::pair_accepted,"forward pair") ||
        !Equal(valid.Process(Event(4,SpatialSite::reverse_write)),PairResult::pair_accepted,"reverse pair") ||
        valid.OpenCount()!=0) return 1;

    SpatialPairing mismatch;
    auto bad_elapsed=Event(2,SpatialSite::forward_write); bad_elapsed.elapsed[0]=1;
    if (!Equal(mismatch.Process(Event(1,SpatialSite::forward_entry)),PairResult::entry_opened,"mismatch entry") ||
        !Equal(mismatch.Process(bad_elapsed),PairResult::invalid_payload,"elapsed mismatch") ||
        mismatch.OpenCount()!=0) return 2;

    SpatialPairing identities;
    auto wrong=Event(2,SpatialSite::reverse_write); wrong.selected=0x9999;
    if (!Equal(identities.Process(Event(1,SpatialSite::reverse_entry)),PairResult::entry_opened,"identity entry") ||
        !Equal(identities.Process(wrong),PairResult::orphan_write,"identity mismatch") ||
        identities.OpenCount()!=1) return 3;

    SpatialPairing duplicates;
    if (!Equal(duplicates.Process(Event(1,SpatialSite::reverse_entry)),PairResult::entry_opened,"duplicate first") ||
        !Equal(duplicates.Process(Event(2,SpatialSite::reverse_entry)),PairResult::duplicate_entry,"duplicate second") ||
        !Equal(duplicates.Process(Event(1,SpatialSite::reverse_write)),PairResult::non_monotonic,"sequence order")) return 4;

    auto invalid=Event(1,SpatialSite::forward_entry); invalid.pointer_chain_valid=false;
    SpatialPairing guards;
    if (!Equal(guards.Process(invalid),PairResult::invalid_common,"pointer guard")) return 5;

    std::puts("PASS: native spatial pairing accepts both interleaved directions and rejects elapsed, identity, duplicate, ordering and pointer failures.");
    return 0;
}
