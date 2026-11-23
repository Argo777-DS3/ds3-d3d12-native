// W5d GL-2 T2 (GLOW-WAVE1-READY 7): Banks::GroupWord and Banks::Glow on the admitted default.gparam (argument: a directory holding
// param_drawparam_default.gparam.dcx, the I-0067 extraction; the bank checks skipped with a message without it, the data-free
// PassDominantScale checks always run). default holds the Glow Filter
// group only at id 10 (0, 20, 3, 1, 0, 1); key {1, 0} falls back to the built-in record of 17DBE0 (the same values); key
// {-1, -1} reads as {1, 0}; the 2227C0 blend at t = 0, 1, 0.5 and the epsilon snap, on two records built for the test.
#include "../world/light_producers.h"
#include "../world/producers.h"
#include <algorithm>
#include <array>
#include <bit>
#include <initializer_list>
#include <limits>
#include <cmath>
#include <filesystem>
#include <process.h>
#include <vector>
#include <cstdio>
#include <cstring>
#include <string>

namespace {
int failures{},checks{};
void Expect(bool ok,const std::string& what) {++checks;if(!ok){++failures;std::printf("FAIL %s\n",what.c_str());}}
using Record=ds3light::Banks::GlowRecord;
bool Equal(const Record& a,const Record& b) {
    return a.enable==b.enable && a.blur_scale==b.blur_scale && a.rgb==b.rgb && a.alpha==b.alpha && a.new_glow==b.new_glow && a.final_scale==b.final_scale;}
// I-0113 tranche 33 (gap 16): the effect pass context's +0x110 from the two Dominant Light records (140204C30); no data needed.
void PassDominantScale() {
    float ra[11]{},rb[11]{};for(int i=0;i<11;++i){ra[i]=0.1f*float(i);rb[i]=1.0f+0.2f*float(i);}
    float s[4]{};ds3prod::ProducePassDominantScale(ra,rb,0,s);
    Expect(std::fabs(s[0]-0.8f*0.7f)<1e-6f && std::fabs(s[1]-0.9f*0.7f)<1e-6f && std::fabs(s[2]-1.0f*0.7f)<1e-6f && s[3]==1.0f,"pass dominant scale: record a at t 0, (f8 f7, f9 f7, f10 f7, 1)");
    ds3prod::ProducePassDominantScale(ra,rb,1,s);
    Expect(std::fabs(s[0]-2.6f*2.4f)<1e-5f && std::fabs(s[1]-2.8f*2.4f)<1e-5f && std::fabs(s[2]-3.0f*2.4f)<1e-5f && s[3]==1.0f,"pass dominant scale: record b at t 1");
    ds3prod::ProducePassDominantScale(ra,rb,0.5f,s);const float f7=(0.7f+2.4f)*0.5f;
    Expect(std::fabs(s[0]-(0.8f+2.6f)*0.5f*f7)<1e-5f && std::fabs(s[1]-(0.9f+2.8f)*0.5f*f7)<1e-5f && std::fabs(s[2]-(1.0f+3.0f)*0.5f*f7)<1e-5f && s[3]==1.0f,"pass dominant scale: the blend before the products");
    // 1368D0's snap: within FLT_EPSILON of 0 the record a itself (here ra's products exactly), not the lerp.
    float near0[4]{};ds3prod::ProducePassDominantScale(ra,rb,5e-8f,near0);ds3prod::ProducePassDominantScale(ra,rb,0,s);
    Expect(near0[0]==s[0] && near0[1]==s[1] && near0[2]==s[2],"pass dominant scale: t within epsilon of 0 snaps to a");}
}
// I-0141: the light set that stands for a cutscene's ShadowParam record without one (CutsceneShadowLightSet); no data needed.
void CutsceneShadowSet() {
    const uint32_t none=0xFFFFFFFFu;uint32_t out[2]{7,7};
    {const uint32_t record[2]{232000010u,none},view[2]{232000010u,60u};
        Expect(ds3light::CutsceneShadowLightSet(record,view,out) && out[0]==232000010u && out[1]==60u,"cutscene shadow set: a record without a light set takes the view's key");}
    {uint32_t record[2]{239000020u,none};const uint32_t view[2]{239000020u,40u};
        Expect(ds3light::CutsceneShadowLightSet(record,view,record) && record[0]==239000020u && record[1]==40u,"cutscene shadow set: written in place");}
    {const uint32_t record[2]{1u,none},view[2]{251010010u,10u};out[0]=out[1]=7;
        Expect(ds3light::CutsceneShadowLightSet(record,view,out) && out[0]==251010010u && out[1]==10u,"cutscene shadow set: default's record under a cutscene's key");}
    out[0]=out[1]=7;
    {const uint32_t record[2]{231000010u,22u},view[2]{231000010u,22u};
        Expect(!ds3light::CutsceneShadowLightSet(record,view,out) && out[0]==7 && out[1]==7,"cutscene shadow set: a record that names its light set keeps it");}
    {const uint32_t record[2]{131000000u,none},view[2]{131000000u,0u};
        Expect(!ds3light::CutsceneShadowLightSet(record,view,out) && out[0]==7,"cutscene shadow set: a map's key is not a cutscene's (the record's Angle stays)");}
    {const uint32_t record[2]{1u,none},view[2]{none,none};
        Expect(!ds3light::CutsceneShadowLightSet(record,view,out) && out[0]==7,"cutscene shadow set: the title's key {-1, -1}");}
    {const uint32_t record[2]{232000010u,none},view[2]{232000010u,none};
        Expect(!ds3light::CutsceneShadowLightSet(record,view,out) && out[0]==7,"cutscene shadow set: a view key without an id");}
    {const uint32_t record[2]{232000010u,none},a[2]{199999999u,5u},b[2]{300000000u,5u};
        Expect(!ds3light::CutsceneShadowLightSet(record,a,out) && !ds3light::CutsceneShadowLightSet(record,b,out) && out[0]==7,"cutscene shadow set: the area range's two ends");}
    Expect(ds3light::CutsceneArea(200000000u) && ds3light::CutsceneArea(239000020u) && ds3light::CutsceneArea(299999999u) && !ds3light::CutsceneArea(199999999u) &&
        !ds3light::CutsceneArea(300000000u) && !ds3light::CutsceneArea(1u) && !ds3light::CutsceneArea(none),"a cutscene file's area: 2AABBVVVV");
    Expect(ds3light::CutsceneLightSet(239000020u,239000020u) && ds3light::CutsceneLightSet(232000010u,239000020u) && !ds3light::CutsceneLightSet(139000000u,239000020u) &&
        !ds3light::CutsceneLightSet(239000020u,139000000u) && !ds3light::CutsceneLightSet(139000000u,139000000u) && !ds3light::CutsceneLightSet(none,none) &&
        !ds3light::CutsceneLightSet(1u,1u),"a cutscene's own light set: both keys of the block");
    // Light 0 of a light set's record: its colour at +0x10, its strength at +0x1C (39000020's sets: colour 1, strength 0: off).
    {alignas(16) uint8_t record[0x2A0]{};const auto set=[&](float r,float g,float b,float strength){const float c[4]{r,g,b,strength};std::memcpy(record+0x10,c,16);};
        set(1,1,1,0);const bool off=!ds3light::SunLightOn(record);set(0,0,0,2.5f);const bool black=!ds3light::SunLightOn(record);
        set(1,1,1,0.886f);const bool on=ds3light::SunLightOn(record);set(0,0.2f,0,0.1f);const bool faint=ds3light::SunLightOn(record);
        set(1,1,1,-1);const bool negative=!ds3light::SunLightOn(record);
        Expect(off && black && on && faint && negative,"light 0 of a light set: on with a positive strength and a colour");}
}
int main(int argc,char** argv) {
    PassDominantScale();   // data-free (the tranche 33 step review's L3)
    CutsceneShadowSet();
    if(argc<2){std::printf("glow_bank_test: no gparam directory given, the bank checks skipped (%d checks, %d failures)\n",checks,failures);return failures?1:0;}
    ds3light::Banks banks;if(!banks.Load(argv[1]) || !banks.Files()){std::printf("FAIL cannot load %s\n",argv[1]);return 1;}
    std::string source;
    const Record ten=banks.GlowAt(1,10,&source);
    Expect(source=="default" && ten.enable==0 && ten.blur_scale==20.0f && ten.rgb==3.0f && ten.alpha==1.0f && ten.new_glow==0 && ten.final_scale==1.0f,"default id 10");
    uint32_t word{};Expect(banks.GroupWord(1,10,"Glow Filter","BlurSampleScale",word) && word==0x41A00000u,"GroupWord raw float bits");
    float value{};Expect(banks.GroupFloat(1,10,"Glow Filter","BlurSampleScale",value) && value==20.0f,"GroupFloat unchanged");
    const Record zero=banks.GlowAt(1,0,&source);
    Expect(source=="builtin" && Equal(zero,Record{}),"key {1, 0}: the built-in record");
    const Record minus=banks.GlowAt(0xFFFFFFFFu,0xFFFFFFFFu,&source);
    Expect(source=="builtin" && Equal(minus,Record{}),"key {-1, -1} reads as {1, 0}");
    // The blend on the bank (A = id 10, B = the built-in record): equal records give equal results at any weight.
    Expect(Equal(banks.Glow(1,10,1,0,0.5f,1e-6f),ten),"blend of equal records");
    // The blend rule itself on a key present and one absent is covered by the formula; check the snap and the lerp on scales.
    const Record a=banks.Glow(1,10,1,0,0.0f,1e-6f),b=banks.Glow(1,10,1,0,1.0f,1e-6f);
    Expect(Equal(a,ten) && Equal(b,Record{}),"t = 0 gives A, t = 1 gives B");
    {Record m{};m.enable=1;m.blur_scale=1.0f;m.rgb=1.5f;m.alpha=1.0f;m.new_glow=1;m.final_scale=0.5f;const Record d{};   // mapbase id 0 and the built-in
        const Record h=ds3light::Banks::BlendGlow(m,d,0.5f,1e-6f);
        Expect(h.enable==0 && h.new_glow==0 && h.blur_scale==(20.0f-1.0f)*0.5f+1.0f && h.rgb==(3.0f-1.5f)*0.5f+1.5f && h.alpha==1.0f && h.final_scale==(1.0f-0.5f)*0.5f+0.5f,"lerp of the scales, enables from B");
        Expect(Equal(ds3light::Banks::BlendGlow(m,d,5e-7f,1e-6f),m) && Equal(ds3light::Banks::BlendGlow(m,d,1.0f-5e-7f,1e-6f),d),"epsilon snap");}
    // Lightmap floors: default.gparam has no Dominant Light group: no record for {1, 0} nor {-1, -1} (the caller keeps the built-in).
    {std::array<float,11> row{};row[0]=-1.0f;
        Expect(!banks.DominantAt(1,0,row) && !banks.DominantAt(0xFFFFFFFFu,0xFFFFFFFFu,row) && row[0]==-1.0f,"Dominant: no group, row untouched");}
    // Sun source (BindArea): a bound area reads its bound file (here default, which holds id 10) for every lookup; unbound, or
    // bound to a file Load did not see, it keeps the rule of the area's own name (absent here: the built-in record).
    {const uint32_t area=140000000u;const std::array<float,12> identity{1,0,0,0, 0,1,0,0, 0,0,1,0};
        Expect(Equal(banks.GlowAt(area,10,&source),Record{}) && source=="builtin","unbound area without its file");
        banks.BindArea(area,"m99_00_0000",identity);
        Expect(Equal(banks.GlowAt(area,10,&source),Record{}) && source=="builtin","bound to an absent file: rule unchanged");
        banks.BindArea(area,"default",identity);
        Expect(Equal(banks.GlowAt(area,10,&source),ten) && source=="area","bound area reads the bound file");
        banks.ClearAreaBindings();
        Expect(Equal(banks.GlowAt(area,10,&source),Record{}) && source=="builtin","bindings cleared");}
    // I-0141 tranche 3: a cutscene act's override of a map's light and fog records (Banks::SetRecordOverrides). The blends are the
    // engine's in the module; here stand-ins that copy A (t <= 0) or B and mark the record, so the test sees which ran and with what.
    {const uint32_t area=132000000u;alignas(16) uint8_t own[0x2A0],sa[0x2A0],sb[0x2A0],got[0x2A0];
        const auto marked=[](uint8_t* out,const uint8_t* x,const uint8_t* y,float t,size_t bytes){std::memcpy(out,0.0f<t?y:x,bytes);const float m=100.0f+t;std::memcpy(out+0x40,&m,4);};
        ds3light::UseRecordBlends(nullptr,nullptr);
        uint32_t bid=10;   // B: the first id of default whose light record differs from {1, 0}'s (outside the mark at +0x40)
        if(banks.Sun(1,0,sa,sizeof sa))for(uint32_t k=1;k<400;++k)if(banks.Sun(1,k,sb,sizeof sb) && (std::memcmp(sa,sb,0x40) || std::memcmp(sa+0x44,sb+0x44,sizeof sa-0x44))){bid=k;break;}
        Expect(banks.Sun(area,0,own,sizeof own) && banks.Sun(1,0,sa,sizeof sa) && banks.Sun(1,bid,sb,sizeof sb),"override: the records without one");
        // The integrity review's L3: the checks that tell A from B need two different records (outside the mark at +0x40).
        Expect(std::memcmp(sa,sb,0x40)!=0 || std::memcmp(sa+0x44,sb+0x44,sizeof sa-0x44)!=0,"override: the test's sources A and B differ");
        // The removers' range (320B0's loop): from..to inclusive, wrapping; none when to - from is negative as a signed word.
        Expect(ds3light::Banks::RemovedId(5,5,5) && ds3light::Banks::RemovedId(6,5,7) && !ds3light::Banks::RemovedId(8,5,7) && !ds3light::Banks::RemovedId(4,5,7) &&
            !ds3light::Banks::RemovedId(0,0,0xFFFFFFFFu) && !ds3light::Banks::RemovedId(5,7,5) && ds3light::Banks::RemovedId(0xFFFFFFFFu,0xFFFFFFFFu,0xFFFFFFFFu) &&
            ds3light::Banks::RemovedId(0,0xFFFFFFFFu,0) && !ds3light::Banks::RemovedId(1,0xFFFFFFFFu,0),"override: the removers' id range");
        const uint64_t before=banks.BindingGeneration();
        ds3light::Banks::RecordOverrides table;table[{0,area,0}]=ds3light::Banks::RecordOverride{{1,0},{1,bid},0.0f,11};
        table[{1,area,0}]=ds3light::Banks::RecordOverride{{1,0},{1,bid},0.0f,11};
        banks.SetRecordOverrides(table);const uint64_t after=banks.BindingGeneration();
        Expect(after!=before && banks.Overridden(0,area,0) && banks.Overridden(1,area,0) && !banks.Overridden(0,area,1) && !banks.Overridden(1,area,1),"override: set, its generation moved");
        banks.SetRecordOverrides(table);Expect(banks.BindingGeneration()==after,"override: the same table again keeps the generation");
        Expect(!banks.Sun(area,0,got,sizeof got),"override: no light blend installed, no record");
        ds3light::UseRecordBlends([](uint8_t* out,const uint8_t* x,const uint8_t* y,float t){std::memcpy(out,0.0f<t?y:x,0x2A0);const float m=100.0f+t;std::memcpy(out+0x40,&m,4);},
            [](uint8_t* out,const uint8_t* x,const uint8_t* y,float t){std::memcpy(out,0.0f<t?y:x,0x60);const float m=200.0f+t;std::memcpy(out+0x40,&m,4);});
        alignas(16) uint8_t want[0x2A0];marked(want,sa,sb,0.0f,sizeof want);
        Expect(banks.Sun(area,0,got,sizeof got) && !std::memcmp(got,want,sizeof got),"override: the key answers A's record through the light blend at t 0");
        {alignas(16) uint8_t short_record[0x260];Expect(banks.Sun(area,0,short_record,sizeof short_record) && !std::memcmp(short_record,want,sizeof short_record),"override: a 0x260-byte read takes its part");}
        Expect(banks.Sun(area,1,got,sizeof got) && !std::memcmp(got,own,sizeof got),"override: another id of the area keeps its own record");
        table[{0,area,0}].t=0.25f;banks.SetRecordOverrides(table);marked(want,sa,sb,0.25f,sizeof want);
        Expect(banks.BindingGeneration()!=after && banks.Sun(area,0,got,sizeof got) && !std::memcmp(got,want,sizeof got),"override: t > 0 blends A and B, the generation moved");
        // Fog: A's 0x60 bytes at t 0 (no blend called), the node's own area at +0x64.
        alignas(16) uint8_t fa[0x80],fgot[0x80];Expect(banks.Fog(1,0,fa,sizeof fa) && banks.Fog(area,0,fgot,sizeof fgot),"override: the fog records");
        uint32_t at{};std::memcpy(&at,fgot+0x64,4);
        Expect(!std::memcmp(fgot,fa,0x60) && at==area,"override: fog at t 0 is A's, the key's area at +0x64");
        {alignas(16) uint8_t fown[0x80],fb[0x80];uint32_t fbid=bid;   // the fog's B: the first id of default whose fog record differs from A's (tranche 3's fixes' step review, L2)
            for(uint32_t k=1;k<400;++k)if(banks.Fog(1,k,fb,sizeof fb) && (std::memcmp(fb,fa,0x40) || std::memcmp(fb+0x44,fa+0x44,0x1C))){fbid=k;break;}
            Expect(banks.Fog(area,1,fown,sizeof fown) && banks.Fog(1,fbid,fb,sizeof fb) && (std::memcmp(fb,fa,0x40) || std::memcmp(fb+0x44,fa+0x44,0x1C)),"override: the fog's own record, and B's differing from A's");
            table[{1,area,0}].b[1]=fbid;table[{1,area,0}].t=0.5f;banks.SetRecordOverrides(table);float mark{};
            Expect(banks.Fog(area,0,fgot,sizeof fgot) && (std::memcpy(&mark,fgot+0x40,4),mark==200.5f) && !std::memcmp(fgot,fb,0x40) && !std::memcmp(fgot+0x44,fb+0x44,0x1C),
                "override: fog at t > 0 is the fog blend's (here B's bytes and the mark)");
            std::memcpy(&at,fgot+0x64,4);Expect(at==area,"override: the blended fog keeps the key's area at +0x64");
            table[{1,area,0}].t=0.0f;
            // {area, -1}: the key the original's node holds; the bank answers it overridden like any id.
            table[{1,area,0xFFFFFFFFu}]=ds3light::Banks::RecordOverride{{1,0},{1,0},0.0f,11};banks.SetRecordOverrides(table);
            Expect(banks.Overridden(1,area,0xFFFFFFFFu) && banks.Fog(area,0xFFFFFFFFu,fgot,sizeof fgot) && !std::memcmp(fgot,fa,0x60),"override: {area, -1} answers its override");
            // An override whose source is itself overridden reads the source by its own rule, one level (the engine would read A's node,
            // which holds A's own override if it had one: the act's sources are a cutscene file's keys, never overridden in the runs read).
            table[{0,1,0}]=ds3light::Banks::RecordOverride{{1,bid},{1,bid},0.0f,11};table[{0,area,0}].t=0.0f;banks.SetRecordOverrides(table);marked(want,sa,sb,0.0f,sizeof want);
            Expect(banks.Sun(area,0,got,sizeof got) && !std::memcmp(got,want,sizeof got),"override: a source's own override is not followed");
            // I-0141 tranche 5 (D-0282): a light key is a cutscene's own set by its key or by an override from a cutscene file's set.
            {ds3light::Banks::RecordOverrides cut;cut[{0,area,7}]=ds3light::Banks::RecordOverride{{232000000u,35},{232000000u,35},0.0f,11};
                cut[{0,area,8}]=ds3light::Banks::RecordOverride{{232000000u,35},{132000000u,1},0.0f,11};cut[{0,area,9}]=ds3light::Banks::RecordOverride{{232000000u,35},{132000000u,1},0.5f,11};
                cut[{0,area,10}]=ds3light::Banks::RecordOverride{{1,0},{1,0},0.0f,11};cut[{1,area,11}]=ds3light::Banks::RecordOverride{{232000000u,35},{232000000u,35},0.0f,11};
                cut[{0,area,13}]=ds3light::Banks::RecordOverride{{232000000u,35},{232000000u,36},0.5f,11};cut[{0,area,14}]=ds3light::Banks::RecordOverride{{132000000u,1},{232000000u,35},0.5f,11};
                banks.SetRecordOverrides(cut);
                Expect(banks.AuthoredLightKey(232000000u,35) && banks.AuthoredLightKey(area,7) && banks.AuthoredLightKey(area,8) && !banks.AuthoredLightKey(area,9) &&
                    !banks.AuthoredLightKey(area,10) && !banks.AuthoredLightKey(area,11) && !banks.AuthoredLightKey(area,12) && banks.AuthoredLightKey(area,13) &&
                    !banks.AuthoredLightKey(area,14),
                    "authored light key: a cutscene's, or a map key overridden from a cutscene file's set (B too when blending), the light group only");
                banks.SetRecordOverrides(table);}
            const uint64_t set=banks.BindingGeneration();banks.SetRecordOverrides({});
            Expect(banks.BindingGeneration()!=set && !banks.Overridden(0,area,0) && banks.Sun(area,0,got,sizeof got) && !std::memcmp(got,own,sizeof got),"override: cleared, the key's own record again, the generation moved");
            Expect(banks.Fog(area,0,fgot,sizeof fgot) && !std::memcmp(fgot,fown,sizeof fgot),"override: cleared, the fog's own record again");}
        ds3light::UseRecordBlends(nullptr,nullptr);}
    // I-0141 tranche 6's witness: the record's +0x280 is the set's LightMask Influence (default's {1, 0}: 0, where a set without the
    // parameter would keep 14B3A0's 1).
    {alignas(16) uint8_t r[0x2A0];float influence{-1.0f};
        Expect(banks.Sun(1,0,r,sizeof r) && (std::memcpy(&influence,r+0x280,4),influence==0.0f),"light set: LightMask Influence at +0x280 (default's {1, 0}: 0)");}
    // I-0141 tranche 7, on the per-value path since tranche 24: an area's fade from its earlier layers' values to its current ones'
    // (Banks::SetAreaFades, AdvanceFades), on a test clock. The earlier layers bind default (which holds the light set `bid` and the
    // fog id `fid`) with a quarter-turn transform; the current ones none, so the area's keys fall back to {1, 0}. Each value moves by
    // its class (FadeValue: a float (1 - w) A + V w, V when equal; a vector so on every lane, V when every lane is equal; LightSet's
    // Angle pair through AngleLerp; a bool at once), then the record is derived as from a file's, its directions turned by the current
    // layers' rotation. The record blends (UseRecordBlends) take no part: set to stand-ins that mark their output, which must not
    // appear in a fade's answer (an override still uses them).
    {const uint32_t area=133000000u;const std::array<float,12> identity{1,0,0,0, 0,1,0,0, 0,0,1,0},turn{0,0,1,0, 0,1,0,0, -1,0,0,0};static double now{};
        const auto F=[](const uint8_t* p,uint32_t at){float v;std::memcpy(&v,p+at,4);return v;};
        const auto lerp=[](float a,float b,float w){return a==b?b:(1.0f-w)*a+b*w;};   // 1401EEC30's order
        alignas(16) uint8_t sa[0x2A0],sb[0x2A0],sid[0x2A0],got[0x2A0];uint32_t bid=0,fid=0;
        for(uint32_t k=1;k<400 && !bid;++k)if(banks.Sun(1,k,sa,sizeof sa) && banks.Sun(1,0,sb,sizeof sb) && std::memcmp(sa,sb,0x290))bid=k;
        {alignas(16) uint8_t x[0x80],y[0x80];for(uint32_t k=1;k<400 && !fid;++k)if(banks.Fog(1,k,x,sizeof x) && banks.Fog(1,0,y,sizeof y) && std::memcmp(x,y,0x58))fid=k;}
        Expect(bid!=0 && fid!=0,"fade: default holds a light set and a fog id other than {1, 0}'s");
        alignas(16) uint8_t fa[0x80],fb[0x80],fgot[0x80];
        banks.SetAreaLayers({{area,{{"default",identity}}}});const bool unturned=banks.Sun(area,bid,sid,sizeof sid);   // the earlier values, unturned
        banks.SetAreaLayers({{area,{{"default",turn}}}});
        Expect(unturned && banks.Sun(area,bid,sa,sizeof sa) && banks.Fog(area,fid,fa,sizeof fa),"fade: the earlier layers' records");
        banks.SetAreaLayers({});
        Expect(banks.Sun(area,bid,sb,sizeof sb) && banks.Fog(area,fid,fb,sizeof fb) && std::memcmp(sa,sb,0x290)!=0 && std::memcmp(sa+0x290,sb+0x290,16)!=0 &&
            [&]{for(const uint32_t at:{0x00u,0x04u,0x08u,0x0Cu,0x10u,0x14u,0x18u,0x20u,0x24u,0x30u,0x34u,0x38u,0x3Cu,0x40u,0x44u,0x48u})if(std::memcmp(fa+at,fb+at,4))return true;return false;}() && !std::memcmp(sid+0x290,sb+0x290,16),"fade: the current records, their words, a faded fog value and the light's rotation differing from the earlier; the identity binding's rotation the unbound one's");
        ds3light::UseFadeClock([]{return now;});
        ds3light::UseRecordBlends([](uint8_t* out,const uint8_t* x,const uint8_t* y,float t){std::memcpy(out,t<0.5f?x:y,0x2A0);const float m=100.0f+t;std::memcpy(out+0x40,&m,4);},
            [](uint8_t* out,const uint8_t* x,const uint8_t* y,float t){std::memcpy(out,t<0.5f?x:y,0x58);const float m=200.0f+t;std::memcpy(out+0x40,&m,4);});
        const std::map<uint32_t,ds3light::Banks::AreaFade> fades{{area,ds3light::Banks::AreaFade{{{"default",turn}},10.0,4.0f}}};
        now=9.0;const uint64_t before=banks.RecordGeneration();banks.SetAreaFades(fades);const uint64_t after=banks.RecordGeneration();banks.SetAreaFades(fades);
        Expect(after!=before && banks.RecordGeneration()==after,"fade: set, its generation moved; the same fades again keep it");
        // The light record at w: each light's two colours (vectors), Sharpness and SsaoWeight, the Angle pair twice, the hemisphere
        // colours, giOcclusionRate and LightMask Influence by their classes; the three bools and the constants the current's; the
        // directions and the rotation are checked apart.
        const auto light_at=[&](float w,uint8_t* want){std::memcpy(want,sb,0x2A0);
            const auto vec=[&](uint32_t at){bool same=true;for(uint32_t c=0;c<4;++c)same=same && F(sa,at+4*c)==F(sb,at+4*c);
                for(uint32_t c=0;c<4;++c){const float x=F(sa,at+4*c),y=F(sb,at+4*c),v=same?y:(1.0f-w)*x+y*w;std::memcpy(want+at+4*c,&v,4);}};
            const auto one=[&](uint32_t at){const float v=lerp(F(sa,at),F(sb,at),w);std::memcpy(want+at,&v,4);};
            for(uint32_t k=0;k<4;++k){const uint32_t b=0x70u*k;vec(b+0x10);vec(b+0x20);one(b+0x40);one(b+0x50);}
            vec(0x1E0);vec(0x1F0);one(0x248);one(0x280);};
        const auto light_fields=[&](const uint8_t* x,const uint8_t* y){for(uint32_t k=0;k<4;++k){const uint32_t b=0x70u*k;
                if(std::memcmp(x+b+0x10,y+b+0x10,0x20) || std::memcmp(x+b+0x40,y+b+0x40,4) || std::memcmp(x+b+0x50,y+b+0x50,4))return false;}
            return !std::memcmp(x+0x1E0,y+0x1E0,0x20) && !std::memcmp(x+0x220,y+0x220,0x30) && !std::memcmp(x+0x248,y+0x248,8) && !std::memcmp(x+0x280,y+0x280,4);};
        // Before its start the weight is 0: the earlier values, turned by the current rotation (sid's, every word but the bools at once).
        {alignas(16) uint8_t want[0x2A0];std::memcpy(want,sid,sizeof want);std::memcpy(want+0x24C,sb+0x24C,3);
            Expect(banks.Fading(area) && banks.Sun(area,bid,got,sizeof got) && !std::memcmp(got,want,sizeof got),
                "fade: before its start, the weight 0: the earlier values under the current rotation, the bools the current's (no blend called)");}
        {alignas(16) uint8_t want[0x80];std::memcpy(want,fa,0x4C);std::memcpy(want+0x1C,fb+0x1C,4);std::memcpy(want+0x4C,fb+0x4C,sizeof want-0x4C);
            Expect(banks.Fog(area,fid,fgot,sizeof fgot) && !std::memcmp(fgot,want,sizeof fgot),"fade: the fog at weight 0, the earlier values, ToUpper (a bool) and the position the current's");}
        now=11.0;{alignas(16) uint8_t want[0x2A0];std::memcpy(want,sid,sizeof want);std::memcpy(want+0x24C,sb+0x24C,3);
            Expect(banks.Sun(area,bid,got,sizeof got) && !std::memcmp(got,want,sizeof got),"fade: the weight moves only at AdvanceFades (the whole record)");}
        {alignas(16) uint8_t want[0x80];std::memcpy(want,fa,0x4C);std::memcpy(want+0x1C,fb+0x1C,4);std::memcpy(want+0x4C,fb+0x4C,sizeof want-0x4C);
            Expect(banks.Fog(area,fid,fgot,sizeof fgot) && !std::memcmp(fgot,want,sizeof fgot),"fade: the fog's weight moves only at AdvanceFades too (the whole record)");}
        float half_lo{},half_hi{},pi_lo{},pi_hi{};
        const bool periods=ds3light::Rdata(0x3D7F0D4,&half_lo,4) && ds3light::Rdata(0x3D7EE40,&half_hi,4) && ds3light::Rdata(0x3D7F0EC,&pi_lo,4) && ds3light::Rdata(0x3D7EEC0,&pi_hi,4);
        const auto angles_at=[&](float w){bool ok=true;int checked=0;for(uint32_t k=0;k<4;++k){const uint32_t b=0x70u*k;
                const float t0=F(sa,b+0x30),t1=F(sb,b+0x30),p0=F(sa,b+0x34),p1=F(sb,b+0x34);
                const bool inside=t0>=half_lo && t1>=half_lo && std::fabs(t0-t1)<std::fabs(half_lo-half_hi) && p0>=pi_lo && p1>=pi_lo && std::fabs(p0-p1)<std::fabs(pi_lo-pi_hi);
                if(!inside || (t0==t1 && p0==p1))continue;   // AngleLerp's wrap: its cases are the value-class test's below
                ++checked;
                ok=ok && F(got,b+0x30)==(t1-t0)*w+t0 && F(got,b+0x34)==(p1-p0)*w+p0 && !std::memcmp(got+b+0x38,got+b+0x30,8);
                const float x=F(got,b),y=F(got,b+4),z=F(got,b+8);ok=ok && std::fabs(x*x+y*y+z*z-1.0f)<1e-5f && F(got,b+0x44)==x && F(got,b+0x48)==z && F(got,b+0x4C)==y;}
            return ok && checked>0;};   // the step review's L3: at least one light's pair moves inside its periods
        {const uint64_t g=banks.RecordGeneration();banks.AdvanceFades();alignas(16) uint8_t want[0x2A0];light_at(0.25f,want);
            Expect(banks.RecordGeneration()!=g && banks.Fading(area) && banks.Sun(area,bid,got,sizeof got) && light_fields(got,want) && !std::memcmp(got+0x24C,sb+0x24C,3) &&
                !std::memcmp(got+0x290,sb+0x290,16),"fade: a quarter through, each light value by its class, the bools and the rotation the current's, the generation moved");
            Expect(!periods || angles_at(0.25f),"fade: a quarter through, the Angle pairs (b - a) t + a (AngleLerp inside its periods), each direction derived from them (unit, its {x, z, y} copy)");}
        const auto fog_at=[&](float w,uint8_t* want){std::memcpy(want,fb,0x80);
            const auto vec=[&](uint32_t at){bool same=true;for(uint32_t c=0;c<4;++c)same=same && F(fa,at+4*c)==F(fb,at+4*c);
                for(uint32_t c=0;c<4;++c){const float x=F(fa,at+4*c),y=F(fb,at+4*c),v=same?y:(1.0f-w)*x+y*w;std::memcpy(want+at+4*c,&v,4);}};
            const auto one=[&](uint32_t at){const float v=lerp(F(fa,at),F(fb,at),w);std::memcpy(want+at,&v,4);};
            vec(0x00);vec(0x30);for(const uint32_t at:{0x10u,0x14u,0x18u,0x20u,0x24u,0x40u,0x44u,0x48u})one(at);};
        {alignas(16) uint8_t want[0x80];fog_at(0.25f,want);
            Expect(banks.Fog(area,fid,fgot,sizeof fgot) && !std::memcmp(fgot,want,sizeof fgot),"fade: the fog a quarter through, each value by its class, ToUpper, the position and the area the current's");}
        {float mark{};now=13.0;banks.AdvanceFades();alignas(16) uint8_t want[0x2A0];light_at(0.75f,want);
         Expect(banks.Sun(area,bid,got,sizeof got) && light_fields(got,want) && (!periods || angles_at(0.75f)),"fade: three quarters through, the light values by their classes");
         {alignas(16) uint8_t fw[0x80];fog_at(0.75f,fw);Expect(banks.Fog(area,fid,fgot,sizeof fgot) && !std::memcmp(fgot,fw,sizeof fgot),"fade: the fog three quarters through");}
         // An override of a fading key still answers first (tranche 3's rule before the fade's), through the engine's record blend.
         ds3light::Banks::RecordOverrides table;table[{0,area,bid}]=ds3light::Banks::RecordOverride{{1,0},{1,0},0.0f,11};banks.SetRecordOverrides(table);
         Expect(banks.Sun(area,bid,got,sizeof got) && (std::memcpy(&mark,got+0x40,4),mark==100.0f),"fade: an override of the key answers before the fade");
         banks.SetRecordOverrides({});}
        Expect(!banks.Fading(area+10000u) && banks.Sun(area+10000u,bid,got,sizeof got),"fade: another area does not fade");
        now=14.0;{const uint64_t g=banks.RecordGeneration();banks.AdvanceFades();
            Expect(banks.RecordGeneration()!=g && !banks.Fading(area) && banks.Sun(area,bid,got,sizeof got) && !std::memcmp(got,sb,sizeof got) && banks.Fog(area,fid,fgot,sizeof fgot) && !std::memcmp(fgot,fb,sizeof fgot),
                "fade: at its end, the current records alone, the generation moved once more");
            const uint64_t h=banks.RecordGeneration();banks.AdvanceFades();Expect(banks.RecordGeneration()==h,"fade: none left, AdvanceFades keeps the generation");}
        now=11.0;banks.SetAreaFades({{area,ds3light::Banks::AreaFade{{},10.0,4.0f}}});
        Expect(!banks.Fading(area) && banks.Sun(area,bid,got,sizeof got) && !std::memcmp(got,sb,sizeof got),"fade: without earlier layers, none");
        now=20.0;banks.SetAreaFades(fades);Expect(!banks.Fading(area),"fade: one already over when given is dropped");
        banks.SetAreaFades({});ds3light::UseFadeClock(nullptr);ds3light::UseRecordBlends(nullptr,nullptr);}
    // I-0141 tranche 23: the other readers fade value by value (FadeValue: the original's value classes, READ). The earlier layers
    // bind default, whose id 11 differs from {1, 0}'s in ShadowParam, SSAO and Setting Info; the current ones none (the area's keys
    // read {1, 0}). Before the first AdvanceFades the weight is 0: the earlier values, except those declared without the fade.
    {const uint32_t area=133000000u,id=11;const std::array<float,12> identity{1,0,0,0, 0,1,0,0, 0,0,1,0};static double now{};
        const auto F=[](const uint8_t* p,uint32_t at){float v;std::memcpy(&v,p+at,4);return v;};
        const auto U=[](const uint8_t* p,uint32_t at){uint32_t v;std::memcpy(&v,p+at,4);return v;};
        const auto lerp=[](float a,float b,float w){return a==b?b:(1.0f-w)*a+b*w;};   // 1401EEC30's order
        alignas(16) uint8_t sa[0x94],sb[0x94],got[0x94];std::vector<ds3light::Banks::BankEntry> ea,eb,eg;float fa{},fb{},fg{};uint32_t wa{},wb{},wg{};
        banks.SetAreaLayers({{area,{{"default",identity}}}});
        const bool earlier=banks.Shadow(area,id,sa,sizeof sa) && banks.GroupEntries(area,id,"SSAO",ea) && banks.GroupFloat(area,id,"SSAO","Ver2 AO Scale",fa) &&
            banks.GroupWord(area,id,"Setting Info","DepthOfFieldQuality",wa);
        banks.SetAreaLayers({});
        const bool current=banks.Shadow(area,id,sb,sizeof sb) && banks.GroupEntries(area,id,"SSAO",eb) && banks.GroupFloat(area,id,"SSAO","Ver2 AO Scale",fb) &&
            banks.GroupWord(area,id,"Setting Info","DepthOfFieldQuality",wb);
        Expect(earlier && current && U(sa,0x20)==area && U(sb,0x20)==1 && F(sa,0x34)==2.0f && F(sb,0x34)==0.1f && U(sa,0x50)==2 && U(sb,0x50)==1 && U(sa,0x84)==4 && U(sb,0x84)==0 &&
            fa!=fb && wa==3 && wb==7,"value fade: the two ends (default's id 11 through the layers, {1, 0} without)");
        float ignored{};Expect(!banks.EnvColorScale(area,id,ignored),"value fade: EnvColorScale of an area without its file answers nothing (its caller reads {1, 0})");
        ds3light::UseFadeClock([]{return now;});
        now=9.0;banks.SetAreaFades({{area,ds3light::Banks::AreaFade{{{"default",identity}},10.0,4.0f}}});
        Expect(banks.Shadow(area,id,got,sizeof got) && F(got,0x34)==F(sa,0x34) && F(got,0x00)==F(sa,0x00) && U(got,0x84)==4 && U(got,0x50)==1 && U(got,0x90)==U(sb,0x90) && U(got,0x20)==1,
            "value fade: the shadow record at weight 0, the earlier values; FilterMode and Resolution (enumerations) and the bank tag the current's");
        Expect(banks.GroupWord(area,id,"Setting Info","DepthOfFieldQuality",wg) && wg==7,"value fade: a value declared without the fade, the current's at weight 0");
        now=11.0;banks.AdvanceFades();   // w = 0.25
        float half_lo{},half_hi{};const bool angles=ds3light::Rdata(0x3D7F0D4,&half_lo,4) && ds3light::Rdata(0x3D7EE40,&half_hi,4);
        Expect(banks.Shadow(area,id,got,sizeof got) && F(got,0x34)==lerp(F(sa,0x34),F(sb,0x34),0.25f) && F(got,0x40)==lerp(F(sa,0x40),F(sb,0x40),0.25f) &&
            F(got,0x74)==lerp(F(sa,0x74),F(sb,0x74),0.25f) && U(got,0x84)==3 && U(got,0x50)==1 && U(got,0x24)==U(sb,0x24) && U(got,0x8C)==U(sb,0x8C) && got[0x70]==sb[0x70] && got[0x71]==sb[0x71],
            "value fade: the shadow record at 0.25, floats (1 - w) A + V w, Blur Count 4 to 0 truncated to 3, the enumeration and the bools the current's");
        Expect(!angles || (F(got,0x00)==(F(sb,0x00)-F(sa,0x00))*0.25f+F(sa,0x00) && F(got,0x04)==(F(sb,0x04)-F(sa,0x04))*0.25f+F(sa,0x04) && half_lo<0.0f && half_hi>0.0f),
            "value fade: the shadow angle pair through AngleLerp ((b - a) t + a inside the periods)");
        Expect(banks.GroupFloat(area,id,"SSAO","Ver2 AO Scale",fg) && fg==lerp(fa,fb,0.25f),"value fade: GroupFloat at 0.25");
        Expect(banks.GroupEntries(area,id,"SSAO",eg) && eg.size()==eb.size(),"value fade: GroupEntries keeps the current list");
        {bool ok=eg.size()==ea.size();for(size_t k=0;ok && k<eg.size();++k){const auto& g=eg[k];const auto& from=ea[k];const auto& to=eb[k];
                if(g.type==9)ok=std::bit_cast<float>(g.raw[0])==lerp(std::bit_cast<float>(from.raw[0]),std::bit_cast<float>(to.raw[0]),0.25f);
                else if(g.type==0xB)ok=g.raw[0]==to.raw[0];}
            Expect(ok,"value fade: each SSAO entry, floats lerped, bools (UseReProj, PreventGhost, EnableUpScale 1 to 0) the current's");}
        now=11.2;banks.AdvanceFades();   // w = 0.3: Blur Count 4 to 0 reads 2.8, truncated to 2 (rounding would give 3)
        Expect(banks.Shadow(area,id,got,sizeof got) && U(got,0x84)==2 && U(got,0x50)==1 && U(got,0x24)==U(sb,0x24) && U(got,0x90)==U(sb,0x90),
            "value fade: an integer truncated toward zero (cvttss2si), the declared-at-once fields (Light Id, FilterMode, Resolution) the current's");
        now=14.0;banks.AdvanceFades();
        Expect(banks.Shadow(area,id,got,sizeof got) && !std::memcmp(got,sb,sizeof got) && banks.GroupFloat(area,id,"SSAO","Ver2 AO Scale",fg) && fg==fb,"value fade: at its end, the current values alone");
        banks.SetAreaFades({});ds3light::UseFadeClock(nullptr);}
    // I-0141 tranche 23, the final review's M2: each value class on synthetic words (the expected numbers by hand from the slots'
    // listings, RESULT « Tranche 23 »).
    {const auto V=[](const char* g,const char* p,uint8_t type,std::initializer_list<uint32_t> a,std::initializer_list<uint32_t> v,float w){
            std::array<uint32_t,4> x{},y{},o{};std::copy(a.begin(),a.end(),x.begin());std::copy(v.begin(),v.end(),y.begin());
            ds3light::FadeValueForTest(g,p,type,static_cast<unsigned>(a.size()),x.data(),y.data(),w,o.data());return o;};
        const auto B=[](float f){return std::bit_cast<uint32_t>(f);};const auto Fl=[](uint32_t u){return std::bit_cast<float>(u);};
        const float nan=std::numeric_limits<float>::quiet_NaN();
        Expect(V("SSAO","x",9,{B(1)},{B(3)},0.25f)[0]==B(1.5f) && V("SSAO","x",9,{B(1)},{B(3)},0.0f)[0]==B(1) && V("SSAO","x",9,{B(1)},{B(3)},-1.0f)[0]==B(1) &&
            V("SSAO","x",9,{B(1)},{B(3)},1.0f)[0]==B(3) && V("SSAO","x",9,{B(1)},{B(3)},2.0f)[0]==B(3) && std::isnan(Fl(V("SSAO","x",9,{B(1)},{B(3)},nan)[0])) &&
            V("SSAO","x",9,{B(0.1f)},{B(0.1f)},0.1f)[0]==B(0.1f),"value classes: a float at 0.25, w <= 0, w >= 1, NaN lerped, equal ends V");
        {const float w=0.1f,lane=(1.0f-w)*0.1f+0.1f*w;const auto o=V("SSAO","x",0xD,{B(0.1f),B(1),B(2)},{B(0.1f),B(3),B(2)},w);
            Expect(lane!=0.1f && o[0]==B(lane) && o[1]==B((1.0f-w)*1.0f+3.0f*w) && o[2]==B((1.0f-w)*2.0f+2.0f*w),"value classes: a vector with lanes unequal lerps its equal lanes too (cmpeqps, movmskps)");}
        Expect(V("SSAO","x",0xC,{B(0.0f),B(1)},{B(-0.0f),B(1)},0.5f)[0]==B(-0.0f),"value classes: a vector equal on every lane, V's words");
        Expect(V("Dof","x",3,{4},{0},0.3f)[0]==2 && V("Dof","x",3,{static_cast<uint32_t>(-5)},{0},0.5f)[0]==static_cast<uint32_t>(-2),"value classes: s32 truncated toward zero");
        Expect(V("Dof","x",7,{0xFFFFFFFFu},{0},0.25f)[0]==0xC0000000u,"value classes: u32 through 64-bit conversions");
        Expect(V("LightScattering","x",2,{0xFFFFFFF6u},{0},0.25f)[0]==0xFFFFFFF9u && V("LightScattering","x",2,{0xFFFFFFF6u},{0x0000FFF6u},0.5f)[0]==0x0000FFF6u,
            "value classes: s16 sign-extended, truncated, its equality on 16 bits");
        Expect(V("Bloom","x",5,{200},{10},0.5f)[0]==105 && V("Bloom","x",5,{255},{0},0.3f)[0]==178 && V("Bloom","x",1,{0xF6},{0},0.25f)[0]==0xF9,"value classes: u8 and s8");
        Expect(V("Bloom","x",0xF,{0x10C8FF40u},{0x00100A00u},0.5f)[0]==0x086C8420u,"value classes: a colour per byte, truncated");
        Expect(V("SSAO","x",0xB,{0},{1},0.1f)[0]==1 && V("Tone Map","ToneType",3,{4},{0},0.5f)[0]==0,"value classes: a bool and an enumeration the current's at once");
        float half_lo{},half_hi{},pi_lo{},pi_hi{};
        const bool periods=ds3light::Rdata(0x3D7F0D4,&half_lo,4) && ds3light::Rdata(0x3D7EE40,&half_hi,4) && ds3light::Rdata(0x3D7F0EC,&pi_lo,4) && ds3light::Rdata(0x3D7EEC0,&pi_hi,4);
        Expect(periods && std::fabs(half_lo+1.5707964f)<1e-5f && std::fabs(half_hi-1.5707964f)<1e-5f && std::fabs(pi_lo+3.1415927f)<1e-5f && std::fabs(pi_hi-3.1415927f)<1e-5f,
            "value classes: the angle periods in the executable (-pi/2, pi/2; -pi, pi)");
        {const auto o=V("ShadowParam","Angle",0xC,{B(-2.0f),B(-3.5f)},{B(1.0f),B(3.0f)},0.25f),p=V("LightSet","Angle",0xC,{B(1.6f),B(0)},{B(-1.55f),B(0)},0.5f);
            const auto q=V("Snow","Angle",0xC,{B(-1.55f),B(0)},{B(1.6f),B(0)},0.5f),r=V("Snow","Angle",0xC,{B(1.0f),B(0)},{B(-2.0f),B(0)},0.5f);
            Expect(std::fabs(Fl(o[0])-1.1061945f)<1e-5f && std::fabs(Fl(o[1])-2.8373890f)<1e-5f && Fl(p[1])==0.0f && std::fabs(Fl(p[0])-1.5957963f)<1e-5f &&
                std::fabs(Fl(q[0])+1.5457963f)<1e-5f && std::fabs(Fl(r[0])-1.0707963f)<1e-5f,
                "value classes: the angle pair's A and V below the period's start moved by the period; across it both ways");}}
    // I-0105 (the surface snow): 23790's rows from 131780's records (m31_00_0000's `Snow` group: elevation -pi/2, azimuth 0).
    {const float sa[16]{-1.5707964f,0, 1,1, 1, 1,1, 0,0.5f, 0.02f, 0.8f,0.8f,0.8f,1, 0.04f, 0.02f};
        float sb[16];std::memcpy(sb,sa,sizeof sb);sb[0]=-1.22173f;sb[1]=0.628319f;sb[2]=2;sb[4]=0.75f;sb[7]=0.22f;sb[8]=0.44f;
        uint8_t out[0xB0]{};uint8_t written[0xB0/4]{};float f[0xB0/4];
        const float rate=ds3prod::ProduceSnow(sa,sb,0,out,sizeof out,written);std::memcpy(f,out,sizeof f);
        Expect(rate==1 && std::fabs(f[0x60/4])<1e-6f && std::fabs(f[0x64/4]-1)<1e-6f && std::fabs(f[0x68/4])<1e-6f && f[0x6C/4]==0 &&
            f[0x70/4]==1 && f[0x78/4]==0 && f[0x80/4]==0.8f && f[0x90/4]==0.04f && f[0x94/4]==0 && f[0x98/4]==2 && f[0x9C/4]==0.02f && f[0xA8/4]==0 &&
            written[0x60/4] && written[0xAC/4] && !written[0x5C/4],"snow: record a at t 0, the direction up");
        std::memset(out,0,sizeof out);const float one=ds3prod::ProduceSnow(sa,sb,1,out,sizeof out,written);std::memcpy(f,out,sizeof f);
        Expect(one==0.75f && f[0x70/4]==2 && f[0x78/4]==-0.5f && std::fabs(f[0x98/4]-1/0.22f)<1e-4f && std::fabs(f[0x64/4]-1)<1e-6f,"snow: record b at t 1, the direction still a's (131780)");
        std::memset(out,0,sizeof out);const float half=ds3prod::ProduceSnow(sa,sb,0.5f,out,sizeof out,written);std::memcpy(f,out,sizeof f);
        const float e=(-1.22173f+-1.5707964f)*0.5f,z=0.628319f*0.5f;
        Expect(std::fabs(half-0.875f)<1e-6f && std::fabs(f[0x60/4]-std::cos(e)*std::sin(z))<1e-5f && std::fabs(f[0x64/4]+std::sin(e))<1e-5f &&
            std::fabs(f[0x68/4]-std::cos(e)*std::cos(z))<1e-5f && f[0x70/4]==1.5f,"snow: the blend, angles through 1316D0");
        uint8_t model[0x50]{};uint8_t marks[0x50/4]{};ds3prod::ProduceSnowRate(1,0,0.25f,0.75f,model,sizeof model,marks);float r{};std::memcpy(&r,model+0x40,4);
        Expect(r==0.5625f && marks[0x40/4],"snow rate: the model's blend times the scene's");
        // Review L4: 1316D0's two wraps (azimuth range [-pi, pi], width 2 pi): y below lo takes the width; |x - y| >= width takes it off y.
        constexpr float width=6.2831855f;float wa[16],wb[16];std::memcpy(wa,sa,sizeof wa);std::memcpy(wb,sa,sizeof wb);
        wa[0]=wb[0]=0;wa[1]=3.0f;wb[1]=-3.2f;std::memset(out,0,sizeof out);ds3prod::ProduceSnow(wa,wb,0.5f,out,sizeof out,written);std::memcpy(f,out,sizeof f);
        const float z1=((-3.2f+width)-3.0f)*0.5f+3.0f;
        Expect(std::fabs(f[0x60/4]-std::sin(z1))<1e-5f && std::fabs(f[0x68/4]-std::cos(z1))<1e-5f && std::fabs(f[0x64/4])<1e-6f,"snow: an azimuth below -pi takes the width");
        wa[1]=-3.1f;wb[1]=3.2f;std::memset(out,0,sizeof out);ds3prod::ProduceSnow(wa,wb,0.5f,out,sizeof out,written);std::memcpy(f,out,sizeof f);
        const float z2=((3.2f-width)-(-3.1f))*0.5f+(-3.1f);
        Expect(std::fabs(f[0x60/4]-std::sin(z2))<1e-5f && std::fabs(f[0x68/4]-std::cos(z2))<1e-5f,"snow: |x - y| past the width, the short way round");
        // default.gparam's Snow and Snow Model groups hold no value: no record, the caller keeps the engine's built-in.
        std::array<float,16> row{};row[0]=-1.0f;float model_rate=-1.0f;
        Expect(!banks.SnowAt(1,0,row) && row[0]==-1.0f && !banks.SnowModelAt(1,0,model_rate) && model_rate==-1.0f,"snow: empty groups, no record");}
    // I-0138 tranche 4 part 4b: a cutscene's file sAA_BB_VVVV answers its area 2AABBVVVV (GetGparamArea_140d27790's inverse);
    // before, the area was looked for as mAA_BB_VVVV and every cutscene key fell to {1, 0}. A folder holding default.gparam
    // twice, the second under the name of Vordt's intro file (the binary cutscene files are not extracted on this PC).
    {const auto dir=std::filesystem::temp_directory_path()/("glow_bank_test_"+std::to_string(_getpid()));std::error_code ec;
        std::filesystem::create_directories(dir,ec);const auto from=std::filesystem::path(argv[1])/"param_drawparam_default.gparam.dcx";
        std::filesystem::copy_file(from,dir/"param_drawparam_default.gparam.dcx",std::filesystem::copy_options::overwrite_existing,ec);
        std::filesystem::copy_file(from,dir/"param_drawparam_s30_00_0030.gparam.dcx",std::filesystem::copy_options::overwrite_existing,ec);
        ds3light::Banks cut;const bool loaded=cut.Load(dir) && cut.Files()==2;Expect(loaded,"cutscene folder loaded (two files)");
        if(loaded){std::vector<ds3light::Banks::BankEntry> entries;
            Expect(cut.GroupEntries(230000030u,10,"Glow Filter",entries,&source) && source=="s30_00_0030" && !entries.empty(),"cutscene area 230000030 reads s30_00_0030, labelled by its name");
            Expect(Equal(cut.GlowAt(230000030u,10,&source),ten) && source=="s30_00_0030","cutscene area through GroupWord, the same label");
            Expect(!cut.GroupEntries(230000031u,10,"Glow Filter",entries,&source),"another cutscene's area: no file, default's {1, 0} lacks id 10's group record");
            Expect(!cut.GroupEntries(130000000u,10,"Glow Filter",entries,&source),"a map area without its file: the same fallback as before");}
        std::filesystem::remove_all(dir,ec);}
    // I-0138 tranche 6 (K-0496; its final review's L2): mapbase answers map areas only. A folder holding default.gparam twice, the
    // second under mapbase's name (so mapbase holds the Glow Filter group at id 10): a map area without its own file reads it, a
    // cutscene's area without its own file does not (the engine binds mapbase under map files alone). Not checked here: that the
    // key {1, 0} reads default alone (both files hold the same groups, so no value could tell; the fixes' step review's L1).
    {const auto dir=std::filesystem::temp_directory_path()/("glow_bank_test_mapbase_"+std::to_string(_getpid()));std::error_code ec;
        std::filesystem::create_directories(dir,ec);const auto from=std::filesystem::path(argv[1])/"param_drawparam_default.gparam.dcx";
        std::filesystem::copy_file(from,dir/"param_drawparam_default.gparam.dcx",std::filesystem::copy_options::overwrite_existing,ec);
        std::filesystem::copy_file(from,dir/"param_drawparam_mapbase.gparam.dcx",std::filesystem::copy_options::overwrite_existing,ec);
        ds3light::Banks base;const bool loaded=base.Load(dir) && base.Files()==2;Expect(loaded,"mapbase folder loaded (two files)");
        if(loaded){std::vector<ds3light::Banks::BankEntry> entries;
            Expect(base.GroupEntries(130000000u,10,"Glow Filter",entries,&source) && source=="mapbase" && !entries.empty(),"a map area without its file reads mapbase");
            Expect(Equal(base.GlowAt(130000000u,10,&source),ten) && source=="mapbase","a map area through GroupWord, from mapbase");
            Expect(!base.GroupEntries(230000031u,10,"Glow Filter",entries,&source),"a cutscene's area without its file: mapbase does not answer");
            Expect(Equal(base.GlowAt(230000031u,10,&source),Record{}) && source=="builtin","a cutscene's area through GroupWord: the built-in record, not mapbase's");
            Expect(Equal(base.GlowAt(1,10,&source),ten) && source=="default","the key {1, 10}: default itself");
            Expect(Equal(base.GlowAt(130000000u,0,&source),Record{}) && source=="builtin","a map area's id that no file holds: the built-in record");}
        std::filesystem::remove_all(dir,ec);}
    // I-0141 tranche 16: Banks::UnboundAnswers. default.gparam copied as m99_00_0000 (area 199000000): a light set and a fog lookup of
    // that area with no binding answer the file by its name and are each noted once, unturned (no mapoffset.txt in the folder); a
    // second copy m99_01_0000 (area 199010000), bound before any lookup, adds nothing for keys never noted, while the same id unbound
    // does; default's own key {1, 0} is never noted. The record is process-wide (32 entries): this block needs it empty at its start (no unbound
    // lookup of any area before it in the process; checked), and it fills the list: it stays the last user of UnboundAnswers in this test.
    {const auto dir=std::filesystem::temp_directory_path()/("glow_bank_test_unbound_"+std::to_string(_getpid()));std::error_code ec;
        std::filesystem::create_directories(dir,ec);const auto from=std::filesystem::path(argv[1])/"param_drawparam_default.gparam.dcx";
        std::filesystem::copy_file(from,dir/"param_drawparam_default.gparam.dcx",std::filesystem::copy_options::overwrite_existing,ec);
        std::filesystem::copy_file(from,dir/"param_drawparam_m99_00_0000.gparam.dcx",std::filesystem::copy_options::overwrite_existing,ec);
        for(int m=1;m<=11;++m){char n[48];std::snprintf(n,sizeof n,"param_drawparam_m99_%02d_0000.gparam.dcx",m);   // 01 bound below, 02 to 11 fill the list
            std::filesystem::copy_file(from,dir/n,std::filesystem::copy_options::overwrite_existing,ec);}
        ds3light::Banks un;const bool loaded=un.Load(dir) && un.Files()==13;Expect(loaded,"unbound folder loaded (default, m99_00 to m99_11: thirteen files)");
        if(loaded){const uint32_t area=199000000u;alignas(16) uint8_t r[0x2A0],f[0x80];
            const auto noted=[&](uint32_t group){std::vector<ds3light::Banks::UnboundAnswer> seen;ds3light::Banks::UnboundAnswers(&seen);size_t n{};
                for(const auto& u:seen)if(u.area==area && u.id==0 && u.group==group){++n;if(u.offset || u.fade_view)return size_t{99};}return n;};
            const uint64_t before=ds3light::Banks::UnboundAnswers(nullptr);
            Expect(un.Sun(1,0,r,sizeof r) && un.Fog(1,0,f,sizeof f) && ds3light::Banks::UnboundAnswers(nullptr)==before,"default's {1, 0}: not an unbound answer");
            Expect(un.Sun(area,0,r,sizeof r) && un.Fog(area,0,f,sizeof f) && noted(0)==1 && noted(1)==1,"an unbound area's light set and fog: each noted once, unturned");
            const uint64_t once=ds3light::Banks::UnboundAnswers(nullptr);Expect(before==0 && once==2,"the list empty before this block, two entries now (the cap's count needs it)");
            Expect(un.Sun(area,0,r,sizeof r) && ds3light::Banks::UnboundAnswers(nullptr)==once,"the same key again: no new entry");
            const uint32_t bound_area=199010000u;const std::array<float,12> identity{1,0,0,0, 0,1,0,0, 0,0,1,0};un.BindArea(bound_area,"m99_01_0000",identity);
            Expect(un.Sun(bound_area,0,r,sizeof r) && un.Fog(bound_area,0,f,sizeof f) && un.Sun(bound_area,10,r,sizeof r) && ds3light::Banks::UnboundAnswers(nullptr)==once,
                "bound: keys never noted add nothing");
            Expect(un.Sun(area,10,r,sizeof r) && ds3light::Banks::UnboundAnswers(nullptr)==once+1,"the same id unbound (10, which default holds): noted");
            {std::vector<ds3light::Banks::UnboundAnswer> seen;ds3light::Banks::UnboundAnswers(&seen);size_t at_ten{},of_bound{};
                for(const auto& u:seen){if(u.area==area && u.id==10 && u.group==0 && !u.offset && !u.fade_view)++at_ten;if(u.area==bound_area)++of_bound;}
                Expect(at_ten==1 && of_bound==0,"the new entry is {199000000, 10, light set}, none of the bound area");}
            // The cap: three entries so far; areas 199020000 to 199100000, three keys each (light sets 0 and 10, fog 0, which
            // default holds), and 199110000's two light sets make 32; its fog key, the 33rd, is not listed.
            bool answered=true;for(uint32_t m=2;m<=10;++m){const uint32_t fill=199000000u+m*10000u;
                answered=un.Sun(fill,0,r,sizeof r) && un.Sun(fill,10,r,sizeof r) && un.Fog(fill,0,f,sizeof f) && answered;}
            const uint32_t last=199110000u;answered=un.Sun(last,0,r,sizeof r) && un.Sun(last,10,r,sizeof r) && answered;
            {std::vector<ds3light::Banks::UnboundAnswer> seen;const uint64_t full=ds3light::Banks::UnboundAnswers(&seen);
                Expect(answered && full==32 && seen.size()==32,"the list full at exactly 32 entries, each key answered");
                const bool past=un.Fog(last,0,f,sizeof f);seen.clear();ds3light::Banks::UnboundAnswers(&seen);size_t fog_last{};
                for(const auto& u:seen)if(u.area==last && u.group==1)++fog_last;
                Expect(past && seen.size()==32 && fog_last==0,"the 33rd key (199110000's fog) answered, not listed");}}
        std::filesystem::remove_all(dir,ec);}
    Expect(ds3light::AreaFileName(0)=="default" && ds3light::AreaFileName(1)=="default" && ds3light::AreaFileName(230000030u)=="s30_00_0030" &&
        ds3light::AreaFileName(245000000u)=="s45_00_0000" && ds3light::AreaFileName(130000000u)=="m30_00_0000" && ds3light::AreaFileName(151010000u)=="m51_01_0000",
        "AreaFileName: default, sAA_BB_VVVV for 2AABBVVVV, mAA_BB_CCCC otherwise");
    std::printf("glow_bank_test: %s (%d checks, %d failures)\n",failures?"FAILED":"passed",checks,failures);
    return failures?1:0;
}
