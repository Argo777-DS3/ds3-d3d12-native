// W5f YB-1 T1/T2 (YEBIS-WAVE1-READY 6): the three bank blocks built from the decoded gparam dumps (argument 1: the zone-gparam
// directory of TSV dumps, the input of Get-YebisBankBlocks.ps1) equal the golden bank-blocks.tsv (argument 2) word for word
// for default and the seven zone files; the blends between m31 and m40; Banks::GroupEntries on the admitted default.gparam.dcx
// (argument 3, a directory) equals the dump's entries at id 0 for the three groups.
#include "../world/yebis_blocks.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

using namespace ds3native;
namespace {
int failures{},checks{};
void Expect(bool ok,const std::string& what) {++checks;if(!ok){++failures;std::printf("FAIL %s\n",what.c_str());}}
std::vector<std::string> Split(const std::string& line,char c) {std::vector<std::string> v;std::stringstream ss(line);std::string x;while(std::getline(ss,x,c))v.push_back(x);return v;}
// The script's rule: an entry is a maximal run of rows with the same parameter name and strictly increasing ids.
std::map<std::string,yebis::Entries> ReadDump(const std::filesystem::path& path,int32_t id) {
    std::map<std::string,yebis::Entries> groups;std::map<std::string,std::pair<std::string,int32_t>> previous;std::map<std::string,std::vector<bool>> has;
    std::ifstream in(path);std::string line;
    while(std::getline(in,line)){
        if(!line.empty() && line.back()=='\r')line.pop_back();
        if(line.empty() || line[0]=='#' || line.rfind("group",0)==0)continue;
        const auto c=Split(line,'\t');if(c.size()<6)continue;
        const std::string& g=c[0];const std::string& name=c[1];const int type=std::stoi(c[2]);const int32_t rid=std::stoi(c[3]);
        auto& list=groups[g];auto& prev=previous[g];
        if(list.empty() || prev.first!=name || rid<=prev.second){ds3light::Banks::BankEntry e;e.name=name;e.type=static_cast<uint8_t>(type);list.push_back(e);}
        const auto words=Split(c[5],',');auto& e=list.back();
        if(rid==id){e.words=static_cast<uint8_t>(words.size());for(size_t k=0;k<words.size() && k<4;++k)e.raw[k]=static_cast<uint32_t>(std::stoul(words[k],nullptr,16));}
        else if(words.size()>1)e.words=static_cast<uint8_t>(words.size());
        prev={name,rid};
    }
    return groups;
}
// W10 YB-5 T6: the body of the snapshot producer CustomDof as it stood before the core was extracted (commit 3e640a18,
// light_producers.cpp, exact against the I-0064 capture), on named inputs: the oracle of ds3light::CustomDofTable.
void FrozenCustomDof(const ds3light::CustomDofInput& in,uint8_t* out) {
    const float one=in.one,step=in.step,tiny=in.tiny;const float fallback[4]{0,0,in.fallback,0};
    auto G=[&](uint32_t a){ float v; std::memcpy(&v,&in.dof[(a-0x244)/4],4); return v; }; auto GU=[&](uint32_t a){ return in.dof[(a-0x244)/4]; };
    const float n=in.near_plane, f=in.far_plane, c48=in.lens[0], c38=in.lens[1], c3c=in.lens[2], c40=in.lens[3], c44=in.lens[4], e=in.exposure;
    float a=f, nf=f*n; a=a-n; a=a*c48; const float k=c48/n, b0=one-k; a=a/nf; float b=a*0.0f; b=b+b0;
    a=a*c38; b=c38*b; b=b+c3c;
    a=a*c40; b=c40*b; b=b+c44; b=b*e; a=a*e; b=b+0.0f;
    auto pick=[&](float v){ return v<=0.0f?fallback[2]:v; };
    const float f24c=pick(G(0x24C)), f250=pick(G(0x250)), f25c=pick(G(0x25C)), f260=pick(G(0x260)), f26c=pick(G(0x26C));
    const uint32_t eps=0x33D6BF95u; float epsf; std::memcpy(&epsf,&eps,4);
    auto clamp01=[&](float x){ return 0.0f<=x?(x<=one?x:1.0f):0.0f; };
    std::memset(out,0,0x2010); std::memcpy(out,&in.width,4); std::memcpy(out+4,&in.height,4); std::memcpy(out+0xC,&in.word_cc,4);
    for(int i=0;i<0x800;++i) {
        float t=static_cast<float>(i)*step; t=0.0f<=t?(t<=one?t:1.0f):0.0f; if(i>0x7FD) t=one;
        float c=a*t; c=c+b;
        float d=n*f, q=f-n; q=q*t; q=q-f; d=d/q; { uint32_t u; std::memcpy(&u,&d,4); u^=0x80000000u; std::memcpy(&d,&u,4); }
        bool scale_near=true;
        if(GU(0x244)) { c=0.0f; scale_near=false;
            if(d<f250) { float den=f250-f24c; if(den<=tiny) den=epsf; const float u=clamp01((d-f24c)/den); c=(one-u)*G(0x254); uint32_t x; std::memcpy(&x,&c,4); x^=0x80000000u; std::memcpy(&c,&x,4); scale_near=true; } }
        if(scale_near && c<0.0f) c=c*G(0x258);
        if(GU(0x248) && f25c<d) { float den=f260-f25c; if(den<=tiny) den=epsf; c=G(0x264)*clamp01((d-f25c)/den); }
        if(0.0f<c) c=c*G(0x268);
        if(f26c<d) c=0.0f;
        std::memcpy(out+0x10+4*i,&c,4); }
}
}
int main(int argc,char** argv) {
    if(argc<3){std::printf("usage: yebis_blocks_test zone-gparam-dir bank-blocks.tsv [default-gparam-dir]\n");return 2;}
    // golden: file -> block -> word -> hex
    std::map<std::string,std::map<std::string,std::map<int,uint32_t>>> golden;
    {std::ifstream in(argv[2]);std::string line;std::getline(in,line);
        while(std::getline(in,line)){if(!line.empty() && line.back()=='\r')line.pop_back();const auto c=Split(line,'\t');if(c.size()<6)continue;
            golden[c[0]][c[2]][std::stoi(c[3])]=static_cast<uint32_t>(std::stoul(c[5],nullptr,16));}}
    std::map<std::string,std::array<uint32_t,35>> tones;std::map<std::string,std::array<uint32_t,42>> blooms;
    for(const auto& file:std::filesystem::directory_iterator(argv[1])){
        if(file.path().extension()!=".tsv")continue;const std::string stem=file.path().stem().string();
        const auto groups=ReadDump(file.path(),0);
        if(groups.contains("Tone Map")){std::array<uint32_t,35> t{};const bool built=yebis::ToneBlock(groups.at("Tone Map"),t);
            Expect(built,stem+" Tone Map built");tones[stem]=t;
            for(const auto& [word,hex]:golden[stem]["ToneMap35"])Expect(t[word]==hex,stem+" ToneMap35 word "+std::to_string(word));}
        if(groups.contains("Bloom")){std::array<uint32_t,42> b{};const bool built=yebis::BloomBlock(groups.at("Bloom"),b);
            Expect(built,stem+" Bloom built");blooms[stem]=b;
            for(const auto& [word,hex]:golden[stem]["Bloom42"])Expect(b[word]==hex,stem+" Bloom42 word "+std::to_string(word));}
        if(groups.contains("Setting Info") && golden[stem].contains("Setting21")){std::array<uint32_t,21> s{};
            Expect(yebis::SettingBlock(groups.at("Setting Info"),s),stem+" Setting Info built");
            for(const auto& [word,hex]:golden[stem]["Setting21"])Expect(s[word]==hex,stem+" Setting21 word "+std::to_string(word));}
    }
    Expect(tones.size()>=8 && blooms.size()>=8,"default and the seven zones dumped");
    // T2: the blends between m31 and m40.
    if(tones.contains("m31_00_0000") && tones.contains("m40_00_0000")){
        const auto& a=tones["m31_00_0000"];const auto& b=tones["m40_00_0000"];const float eps=1.1920929e-7f;
        Expect(yebis::BlendTone(a,b,0.0f,eps)==a && yebis::BlendTone(a,b,1.0f,eps)==b,"Tone Map snaps");
        const auto h=yebis::BlendTone(a,b,0.5f,eps);bool ok=true;
        for(size_t i=0;i<35;++i){const bool copied=i==2 || i==9 || i==28 || i==29 || i==34;float x,y,v;std::memcpy(&x,&a[i],4);std::memcpy(&y,&b[i],4);const float e=(y-x)*0.5f+x;std::memcpy(&v,&h[i],4);
            ok=ok && (copied?h[i]==b[i]:(v==e || (std::isnan(v) && std::isnan(e))));}
        Expect(ok,"Tone Map lerp with words 2, 9, 28, 29, 34 from B");
        const auto& ba=blooms["m31_00_0000"];const auto& bb=blooms["m40_00_0000"];const auto hb=yebis::BlendBloom(ba,bb,0.5f,eps);ok=true;
        for(size_t i=0;i<42;++i){const bool copied=i==8 || i==9 || i==13 || i==27 || i==31 || i==33 || i==36 || i==39;float x,y,v;std::memcpy(&x,&ba[i],4);std::memcpy(&y,&bb[i],4);
            const float e=(y-x)*0.5f+x;std::memcpy(&v,&hb[i],4);ok=ok && (copied?hb[i]==bb[i]:v==e);}
        Expect(ok && hb[4]==0x3F800000u,"Bloom lerp with the eight words from B, word 4 stays 1.0");
        std::array<uint32_t,35> auto_exposure=a;auto_exposure[9]=1;std::array<uint32_t,35> out{};yebis::Entries e;
        for(size_t i=0;i<35;++i){ds3light::Banks::BankEntry x;x.raw[0]=auto_exposure[i];e.push_back(x);}
        // YB-4b.4: the block keeps a positive AutoExposure word; the guard (ToneAutoFlags) flags it.
        Expect(yebis::ToneBlock(e,out) && out[9]==1 && yebis::ToneAutoFlags(e).exposure,"AutoExposure kept by the block, flagged by the guard");}
    // W10 YB-4b.4 T2g (YEBIS-SECOND-TRANCHE-READY 2.5, 6): FUN_140218BA0's two exposure words on synthetic records, the nine
    // flag combinations (-1, 0, 1 as signed integers) for word 9 and word 0x30, the snaps, and the guard's conditions.
    {const auto bits=[](float f){uint32_t u;std::memcpy(&u,&f,4);return u;};const float eps=1.1920929e-7f;
        const uint32_t flags[3]{0xFFFFFFFFu,0u,1u};int table_failures{};
        // The word-9 flags (fa, fb) and the word-0x30 flags (ga, gb) vary independently: 81 combinations.
        for(const uint32_t fa:flags)for(const uint32_t fb:flags)for(const uint32_t ga:flags)for(const uint32_t gb:flags){
            const yebis::ToneStored a{bits(1.0f),fa,bits(2.0f),ga,bits(3.0f),bits(4.0f)},b{bits(5.0f),fb,bits(6.0f),gb,bits(7.0f),bits(8.0f)};
            const bool pa=fa==1u,pb=fb==1u,qa=ga==1u,qb=gb==1u;
            // 140219340..14021941A written out case by case: word 0 then word 0x23 at t = 0.25.
            const float w0=pa && pb?4.0f:pa?3.5f:pb?2.5f:2.0f,w23=qa && qb?5.0f:qa?4.5f:qb?3.5f:3.0f;
            const auto mid=yebis::BlendToneStored(a,b,0.25f,eps),lo=yebis::BlendToneStored(a,b,0.0f,eps),hi=yebis::BlendToneStored(a,b,1.0f,eps);
            if(mid[0]!=bits(w0) || mid[1]!=bits(w23) || lo[0]!=bits(pa?3.0f:1.0f) || lo[1]!=bits(qa?4.0f:2.0f) ||
               hi[0]!=bits(pb?7.0f:5.0f) || hi[1]!=bits(qb?8.0f:6.0f))++table_failures;}
        Expect(!table_failures,"stored-exposure table: the flags of words 9 and 0x30 independently (81 combinations), both words, snaps");
        yebis::Entries tone(51);tone[46].name="Enable AutoExposure";tone[46].raw[0]=1;
        const auto gx=yebis::ToneAutoFlags(tone);tone[46].raw[0]=0xFFFFFFFFu;tone[9].raw[0]=0xFFFFFFFFu;const auto negative=yebis::ToneAutoFlags(tone);
        std::array<uint32_t,34> dof{};dof[24]=1;const bool focus=yebis::DofAutoFocus(dof);dof[24]=0xFFFFFFFFu;
        Expect(gx.exposure_gx && !gx.exposure && !negative.Any() && focus && !yebis::DofAutoFocus(dof),"auto guards: positive flags only");
        const auto groups=ReadDump(std::filesystem::path(argv[1])/"default.tsv",0);std::array<uint32_t,34> d{};
        Expect(groups.contains("Tone Map") && !yebis::ToneAutoFlags(groups.at("Tone Map")).Any() && groups.contains("Dof") &&
            yebis::DofBlock(groups.at("Dof"),d) && !yebis::DofAutoFocus(d),"default: no auto exposure, no auto focus");}
    // The camera-cut rule (CollectNativeVelocityInputs): a still and a move of exactly 10 units kept, a move above 10 units and a
    // 90-degree turn cut.
    {std::array<float,16> a{};a[0]=a[5]=a[10]=a[15]=1.0f;auto b=a;bool ok=!yebis::CameraCut(a.data(),b.data());
        b[12]=10.0f;ok=ok && !yebis::CameraCut(a.data(),b.data());
        b[12]=20.5f;ok=ok && yebis::CameraCut(a.data(),b.data());
        b=a;b[8]=1.0f;b[10]=0.0f;ok=ok && yebis::CameraCut(a.data(),b.data());
        Expect(ok,"camera cut: moves and turns");}
    // Banks::GroupEntries on the binary default file against its dump.
    if(argc>=4){ds3light::Banks banks;
        if(!banks.Load(argv[3]) || !banks.Files())Expect(false,"default.gparam loaded");
        else{const auto dump=ReadDump(std::filesystem::path(argv[1])/"default.tsv",0);
            for(const char* group:{"Tone Map","Bloom","Setting Info"}){std::vector<ds3light::Banks::BankEntry> entries;std::string source;
                const bool found=banks.GroupEntries(1,0,group,entries,&source);const auto it=dump.find(group);
                bool same=found && source=="default" && it!=dump.end() && entries.size()==it->second.size();
                const auto trim=[](std::string s){while(!s.empty() && s.back()==' ')s.pop_back();while(!s.empty() && s.front()==' ')s.erase(s.begin());return s;};
                for(size_t i=0;same && i<entries.size();++i){const auto& x=entries[i];const auto& y=it->second[i];
                    same=trim(x.name)==trim(y.name) && x.type==y.type && x.words==y.words;
                    for(uint8_t k=0;same && k<x.words && k<4;++k)same=x.raw[k]==y.raw[k];}
                Expect(same,std::string("GroupEntries default ")+group+" equals the dump");}
        }}
    // W10 YB-4b T2b: the light-shaft block (FFX-WAVE1-READY 4.1) against evidence/I-0068/queries/ffx-ready/bank-blocks.tsv
    // (argument 4) for default and the seven zones, and its blend between m31 and m50.
    if(argc>=5){std::map<std::string,std::map<int,uint32_t>> shafts;
        {std::ifstream in(argv[4]);std::string line;std::getline(in,line);
            while(std::getline(in,line)){if(!line.empty() && line.back()=='\r')line.pop_back();const auto c=Split(line,'\t');
                if(c.size()<6 || c[2]!="LightShaft28")continue;shafts[c[0]][std::stoi(c[3])]=static_cast<uint32_t>(std::stoul(c[5],nullptr,16));}}
        std::map<std::string,std::array<uint32_t,28>> built;
        for(const auto& file:std::filesystem::directory_iterator(argv[1])){
            if(file.path().extension()!=".tsv")continue;const std::string stem=file.path().stem().string();
            const auto groups=ReadDump(file.path(),0);if(!groups.contains("LightShaft"))continue;
            std::array<uint32_t,28> s{};Expect(yebis::LightShaftBlock(groups.at("LightShaft"),s),stem+" LightShaft built");built[stem]=s;
            const auto golden_file=shafts.find(stem);
            Expect(golden_file!=shafts.end() && golden_file->second.size()==28,stem+" LightShaft28 golden present");
            if(golden_file!=shafts.end())for(const auto& [word,hex]:golden_file->second)Expect(s[word]==hex,stem+" LightShaft28 word "+std::to_string(word));}
        Expect(built.size()>=8,"LightShaft for default and the seven zones");
        if(built.contains("m31_00_0000") && built.contains("m50_00_0000")){const auto& a=built["m31_00_0000"];const auto& b=built["m50_00_0000"];const float eps=1.1920929e-7f;
            Expect(yebis::BlendLightShaft(a,b,0.0f,eps)==a && yebis::BlendLightShaft(a,b,1.0f,eps)==b,"LightShaft snaps");
            const auto h=yebis::BlendLightShaft(a,b,0.5f,eps);bool ok=h[0]==b[0] && h[9]==b[9];
            for(size_t i:{1u,2u,3u,22u,23u})ok=ok && h[i]==0;
            for(size_t i=0;i<28;++i){if(i<4 || i==9 || i==22 || i==23)continue;float x,y,v;std::memcpy(&x,&a[i],4);std::memcpy(&y,&b[i],4);std::memcpy(&v,&h[i],4);ok=ok && v==(y-x)*0.5f+x;}
            Expect(ok,"LightShaft lerp with words 0 and 9 from B, padding unwritten");}
        // Synthetic records whose every word differs (the zones share words 9 and 10): 0 and 9 from B, 10 lerped, padding zero.
        {std::array<uint32_t,28> a{},b{};for(size_t i=0;i<28;++i){const float x=1.0f+float(i),y=-3.0f-2.0f*float(i);std::memcpy(&a[i],&x,4);std::memcpy(&b[i],&y,4);}
            const auto h=yebis::BlendLightShaft(a,b,0.25f,1.1920929e-7f);float w10{},w8{};std::memcpy(&w10,&h[10],4);std::memcpy(&w8,&h[8],4);
            float a10,b10,a8,b8;std::memcpy(&a10,&a[10],4);std::memcpy(&b10,&b[10],4);std::memcpy(&a8,&a[8],4);std::memcpy(&b8,&b[8],4);
            Expect(h[0]==b[0] && h[9]==b[9] && w10==(b10-a10)*0.25f+a10 && w8==(b8-a8)*0.25f+a8 && !h[1] && !h[2] && !h[3] && !h[22] && !h[23],"LightShaft blend on distinct synthetic records");}}
    // W10 YB-4c T2c: the grading globals 144962228..238 at weight 0 (A = B = the file's id 0) against ffx-ready/bank-blocks.tsv
    // (Grading-globals) for default and the seven zones, and the three weight branches of the integer table.
    if(argc>=5){std::map<std::string,std::map<int,uint32_t>> grading;
        {std::ifstream in(argv[4]);std::string line;std::getline(in,line);int row{};std::string last;
            while(std::getline(in,line)){if(!line.empty() && line.back()=='\r')line.pop_back();const auto c=Split(line,'\t');
                if(c.size()<6 || c[2]!="Grading-globals")continue;if(c[0]!=last){last=c[0];row=0;}grading[c[0]][row++]=static_cast<uint32_t>(std::stoul(c[5],nullptr,16));}}
        size_t files{};
        for(const auto& file:std::filesystem::directory_iterator(argv[1])){
            if(file.path().extension()!=".tsv")continue;const std::string stem=file.path().stem().string();
            const auto groups=ReadDump(file.path(),0);if(!groups.contains("ColorGrading") || !grading.contains(stem))continue;++files;
            std::array<uint32_t,5> g{};Expect(yebis::GradingBlock(groups.at("ColorGrading"),g),stem+" ColorGrading built");
            const auto h=yebis::BlendGrading(g,g,0.0f);for(int i=0;i<5;++i)Expect(h[i]==grading[stem][i],stem+" grading global "+std::to_string(i));}
        Expect(files>=8,"grading goldens for default and the seven zones");
        const std::array<uint32_t,5> a{1,3,0xFFFFFFFFu,0xFFFFFFFFu,0},b{0,7,0xFFFFFFFFu,0xFFFFFFFFu,0};
        Expect(yebis::BlendGrading(a,b,1.0f)==std::array<uint32_t,5>{0,7,7,7,0x3F800000u} && yebis::BlendGrading(a,b,0.25f)==std::array<uint32_t,5>{0,3,3,7,0x3E800000u},"grading table branches");
        Expect(yebis::BlendGrading(a,b,0.0f)==std::array<uint32_t,5>{1,3,3,7,0} && yebis::BlendGrading(a,b,-0.5f)==std::array<uint32_t,5>{1,3,3,7,0},"grading table at w <= 0 with distinct records");}
    // W10 YB-4b: the Anti-Alias and FeedBackBlur records of the default dump (file order), their blends' snaps and splits.
    {const auto groups=ReadDump(std::filesystem::path(argv[1])/"default.tsv",0);std::array<uint32_t,9> aa{};std::array<uint32_t,8> fb{};
        Expect(groups.contains("Anti-Alias") && yebis::AntiAliasBlock(groups.at("Anti-Alias"),aa) && aa[0]==1 && aa[1]==4 && aa[2]==0x3F800000u && aa[8]==0x3DF5C28Fu,"default Anti-Alias record");
        Expect(groups.contains("FeedBackBlur") && yebis::FeedbackBlock(groups.at("FeedBackBlur"),fb) && fb[0]==0 && fb[2]==0x3F000000u && fb[4]==0 && fb[5]==0x3F4CCCCDu && fb[7]==0x40900000u,"default FeedBackBlur record (record-only word 2)");
        std::array<uint32_t,9> ab=aa;ab[0]=0;ab[1]=0;float x=3.0f;std::memcpy(&ab[2],&x,4);const float eps=1.1920929e-7f;
        const auto h=yebis::BlendAntiAlias(aa,ab,0.5f,eps);float v;std::memcpy(&v,&h[2],4);
        Expect(yebis::BlendAntiAlias(aa,ab,0.0f,eps)==aa && yebis::BlendAntiAlias(aa,ab,1.0f,eps)==ab && h[0]==0 && h[1]==0 && v==2.0f,"Anti-Alias blend");
        std::array<uint32_t,8> fb2=fb;fb2[0]=1;float y=1.8f;std::memcpy(&fb2[5],&y,4);const auto g=yebis::BlendFeedback(fb,fb2,0.5f);float u;std::memcpy(&u,&g[5],4);
        Expect(yebis::BlendFeedback(fb,fb2,0.0f)==fb && yebis::BlendFeedback(fb,fb2,1.0f)==fb2 && g[0]==1 && u==(1.8f-0.8f)*0.5f+0.8f,"FeedBackBlur blend");}
    // W10 YB-4b.1/.2 T2d/T2e: the Dof and MotionBlur records of the default dump against yebis-second/second-blocks.tsv
    // (argument 5; the golden of YEBIS-SECOND-TRANCHE-READY, file "default"), and the blends' copy sets on distinct records.
    if(argc>=6){std::map<std::string,std::map<int,uint32_t>> second;
        {std::ifstream in(argv[5]);std::string line;std::getline(in,line);
            while(std::getline(in,line)){if(!line.empty() && line.back()=='\r')line.pop_back();const auto c=Split(line,'\t');
                if(c.size()<6 || c[0]!="default")continue;second[c[2]][std::stoi(c[3])]=static_cast<uint32_t>(std::stoul(c[5],nullptr,16));}}
        const auto groups=ReadDump(std::filesystem::path(argv[1])/"default.tsv",0);std::array<uint32_t,34> dof{};std::array<uint32_t,21> motion{};
        Expect(groups.contains("Dof") && yebis::DofBlock(groups.at("Dof"),dof) && second["Dof34"].size()==34,"default Dof record built");
        for(const auto& [word,hex]:second["Dof34"])Expect(dof[word]==hex,"default Dof34 word "+std::to_string(word));
        Expect(groups.contains("MotionBlur") && yebis::MotionBlurBlock(groups.at("MotionBlur"),motion) && second["MotionBlur21"].size()==21,"default MotionBlur record built");
        for(const auto& [word,hex]:second["MotionBlur21"])Expect(motion[word]==hex,"default MotionBlur21 word "+std::to_string(word));
        std::array<uint32_t,34> da{},db{};for(size_t i=0;i<34;++i){const float x=1.0f+float(i),y=-2.0f-float(i);std::memcpy(&da[i],&x,4);std::memcpy(&db[i],&y,4);}
        const auto dh=yebis::BlendDof(da,db,0.5f,1.1920929e-7f);bool ok=true;
        for(size_t i=0;i<34;++i){const bool copied=i==0 || (i>=8 && i<=13) || i==18 || i==24 || i==25;float x,y,v;std::memcpy(&x,&da[i],4);std::memcpy(&y,&db[i],4);std::memcpy(&v,&dh[i],4);
            ok=ok && (copied?dh[i]==db[i]:v==(y-x)*0.5f+x);}
        Expect(ok,"Dof blend: words 0, 8-13, 18, 24, 25 from B");
        std::array<uint32_t,21> ma{},mb{};for(size_t i=0;i<21;++i){const float x=1.0f+float(i),y=-2.0f-float(i);std::memcpy(&ma[i],&x,4);std::memcpy(&mb[i],&y,4);}
        const auto mh=yebis::BlendMotionBlur(ma,mb,0.5f,1.1920929e-7f);ok=true;
        for(size_t i=0;i<21;++i){const bool copied=i<=1 || i==4 || i==5 || i==9 || i==13 || i==14 || i==18 || i==19;float x,y,v;std::memcpy(&x,&ma[i],4);std::memcpy(&y,&mb[i],4);std::memcpy(&v,&mh[i],4);
            ok=ok && (copied?mh[i]==mb[i]:v==(y-x)*0.5f+x);}
        Expect(ok,"MotionBlur blend: words 0, 1, 4, 5, 9, 13, 14, 18, 19 from B");}
    // W10 YB-5 T6: ds3light::CustomDofTable equals the frozen producer body for three DoF settings (near and far custom ranges
    // off / on, a zero setting taking the .rdata fallback), with the executable's constants when Rdata reaches it.
    {ds3light::CustomDofInput base;const bool constants=ds3light::CustomDofConstants(base);
        if(!constants){base.one=1.0f;base.step=1.0f/2046.0f;base.tiny=1e-6f;base.fallback=1.0f;}
        base.width=2048;base.height=2;base.word_cc=0x12345678u;base.near_plane=0.1f;base.far_plane=10000.0f;base.lens={35.0f,0.5f,0.25f,2.0f,-0.125f};
        const auto bits=[](float f){uint32_t u;std::memcpy(&u,&f,4);return u;};
        std::array<ds3light::CustomDofInput,3> cases{base,base,base};
        cases[0].exposure=1.0f;cases[0].dof={0,1,bits(0.0f),bits(2.0f),bits(1.0f),bits(1.0f),bits(50.0f),bits(10000.0f),bits(1.0f),bits(1.0f),bits(20000.0f)};
        cases[1].exposure=0.8f;cases[1].dof={1,1,bits(0.5f),bits(3.0f),bits(0.75f),bits(1.5f),bits(40.0f),bits(400.0f),bits(2.0f),bits(0.5f),bits(9000.0f)};
        cases[2].exposure=0.0f;cases[2].dof={1,0,bits(-1.0f),bits(0.0f),bits(1.0f),bits(1.0f),bits(0.0f),bits(0.0f),bits(1.0f),bits(1.0f),bits(0.0f)};
        int differ{};
        for(const auto& c:cases){std::vector<uint8_t> x(0x2010),y(0x2010);ds3light::CustomDofTable(c,x.data());FrozenCustomDof(c,y.data());
            if(x!=y)++differ;uint32_t w{},h{},cc{};std::memcpy(&w,x.data(),4);std::memcpy(&h,x.data()+4,4);std::memcpy(&cc,x.data()+0xC,4);
            if(w!=2048 || h!=2 || cc!=0x12345678u)++differ;}
        size_t given{};getenv_s(&given,nullptr,0,"DS3_EXE");   // the build passes the executable: its constants are then required
        Expect(!differ && (constants || !given),std::string("CustomDof core equals the frozen producer (constants ")+(constants?"from the executable)":"literal)"));
        std::printf("T6 CustomDof: constants %s\n",constants?"from the executable":"literal (no executable)");}
    std::printf("yebis_blocks_test: %s (%d checks, %d failures)\n",failures?"FAILED":"passed",checks,failures);
    return failures?1:0;
}
