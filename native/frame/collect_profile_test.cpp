// Workstream K1 control (K-COLLECT-IMPLEMENTATION-SHEET 1.1): the collect profile attributes self time. A nested Scope
// counts its time once, as the inner phase's; Switch moves along regions; calls count entries, not returns; nothing is
// accumulated while disabled or outside any phase.
#include "collect_profile.h"
#include <cstdio>

using namespace ds3native;
namespace {
int failures{};
double frequency{};
void Spin(double ms) {const auto start=kprofile::Now();while((kprofile::Now()-start)*1000.0/frequency<ms){}}
double Ms(kprofile::Phase phase) {return kprofile::totals.ticks[phase]*1000.0/frequency;}
void Reset() {for(auto& t:kprofile::totals.ticks)t=0;for(auto& c:kprofile::totals.calls)c=0;for(auto& c:kprofile::totals.counters)c=0;}
void Expect(bool condition,const char* what) {if(!condition){++failures;std::printf("FAIL %s\n",what);}}
void Near(double value,double expected,const char* what) {
    // Scheduling can only add time; allow 1.5 ms over and 0.05 ms under.
    if(value<expected-0.05 || value>expected+1.5){++failures;std::printf("FAIL %s: %.3f ms, expected %.3f\n",what,value,expected);}
}
}
int main() {
    LARGE_INTEGER f{};QueryPerformanceFrequency(&f);frequency=static_cast<double>(f.QuadPart);
    // Disabled: no accumulation.
    Reset();kprofile::enabled=false;
    {const kprofile::Scope outer(kprofile::Passes);Spin(2);kprofile::Add(kprofile::AdmittedMeshes);}
    Expect(kprofile::totals.ticks[kprofile::Passes]==0 && kprofile::totals.calls[kprofile::Passes]==0,"disabled scope accumulates nothing");
    Expect(kprofile::totals.counters[kprofile::AdmittedMeshes]==0,"disabled counter stays zero");
    // Three levels: SceneSetup 2+1, Traversal 2+1, EntityFilter 3.
    Reset();kprofile::enabled=true;
    {const kprofile::Scope setup(kprofile::SceneSetup);Spin(2);
        {const kprofile::Scope traversal(kprofile::Traversal);Spin(2);
            {const kprofile::Scope filter(kprofile::EntityFilter);Spin(3);}
            Spin(1);}
        Spin(1);}
    Near(Ms(kprofile::SceneSetup),3,"outer self time");
    Near(Ms(kprofile::Traversal),3,"middle self time");
    Near(Ms(kprofile::EntityFilter),3,"inner self time");
    Expect(kprofile::totals.calls[kprofile::SceneSetup]==1 && kprofile::totals.calls[kprofile::Traversal]==1 &&
        kprofile::totals.calls[kprofile::EntityFilter]==1,"one call per scope entry, none per return");
    // Outside any phase: no attribution after the outermost scope returns.
    Spin(2);Near(Ms(kprofile::SceneSetup),3,"no attribution after the outermost scope");
    // Switch along regions inside a scope (the mesh loop), then back to the scope's phase.
    Reset();
    {const kprofile::Scope model(kprofile::ModelAdmission);Spin(1);
        for(int mesh=0;mesh<2;++mesh){kprofile::Switch(kprofile::MeshLookups);Spin(1);kprofile::Switch(kprofile::MeshBlocks);Spin(2);
            kprofile::Switch(kprofile::ModelAdmission);}
        Spin(1);}
    Near(Ms(kprofile::MeshLookups),2,"switched region (lookups)");
    Near(Ms(kprofile::MeshBlocks),4,"switched region (blocks)");
    Near(Ms(kprofile::ModelAdmission),2,"scope phase around switched regions");
    Expect(kprofile::totals.calls[kprofile::MeshLookups]==2 && kprofile::totals.calls[kprofile::MeshBlocks]==2,"one call per switch");
    kprofile::Add(kprofile::ProgramsBuilt,3);Expect(kprofile::totals.counters[kprofile::ProgramsBuilt]==3,"counter adds");
    kprofile::enabled=false;
    std::printf("collect profile failures %d\n",failures);
    return failures?1:0;
}
