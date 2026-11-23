// W10 DF-1 (W10-NATIVE-PASSES 9): the depth of field's plan against the original's frame, per zone. Each argument is a Reference run
// with the draws capture and the DoF state at the depth factor draw (CaptureYebisDofState: Reference1676 m37, 1677 m31, 1678 m50:
// reference-dof-holder.raw the CDepthOfField holder, -layers.raw its level records 0x108 each) and the library at the Alpha_R draw of
// the same frame (reference-shaft-alpha-library.raw). Checked: FUN_141756580's ramps of every level bit for bit; the level counts
// (+0x1D8, +0x1DC, +0x1D4); then the plan of yebis::BuildDof from the holder's and the library's fields against the captured draws
// 0-9: technique, target extent, input extents, every block word the programmes read, the samplers of the slots they read, the write
// mask, the composite's TEXCOORD1. A named run without its files fails. An argument ending in .exe is the executable (DS3_EXE).
#include "glare_capture.h"
#include "glare_library_block.h"
#include "../world/yebis_dof.h"
#include "../world/light_producers.h"
#include <algorithm>
#include <bit>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

using namespace ds3native::yebis;
using ds3native::glare_capture::At;
namespace {
int failures{},checks{};
void Expect(bool ok,const std::string& what){++checks;if(!ok){++failures;std::printf("FAIL %s\n",what.c_str());}}
uint32_t Bits(float v){return std::bit_cast<uint32_t>(v);}
uint32_t Word(const std::string& hex,uint32_t w){uint32_t v{};if(hex.size()>=8*(w+1))for(int b=3;b>=0;--b)v=(v<<8)|uint32_t(std::stoul(hex.substr(8*w+2*size_t(b),2),nullptr,16));return v;}
// The block words a technique's programmes read (the DXBC's ps_used_globals, and the vertex programmes' tap offsets).
std::vector<uint32_t> ReadWords(const std::string& t,size_t levels){
    std::vector<uint32_t> w{20,21};
    if(t.rfind("tech_MadT1_S",0)==0){const uint32_t n=uint32_t(std::stoul(t.substr(12)));
        for(uint32_t i=0;i<n;++i){const uint32_t at=(i<8?136:168)+4*i;w.push_back(at);w.push_back(at+1);for(uint32_t c=0;c<4;++c)w.push_back(616+4*i+c);}}
    else if(t=="tech_DepthOfField_FrontBlurBlendFactor")for(uint32_t i=616;i<624;++i)w.push_back(i);
    else if(t.rfind("tech_DepthOfField_BlurFactorAlpha",0)==0){for(uint32_t i:{0u,1u,2u,3u,68u,69u,70u,71u,200u,201u,202u,203u})w.push_back(i);}
    else if(t.rfind("tech_DepthOfField_Composite",0)==0)for(uint32_t i=72;i<72+4*uint32_t(levels+1);++i)w.push_back(i);
    return w;}
// The state row (samplers) of each captured draw, by index.
std::vector<std::string> StateRows(const std::string& dir){std::ifstream trace(dir+"/startup.jsonl");std::vector<std::string> rows;std::string line;int current=-1;
    while(std::getline(trace,line)){
        if(line.find("\"event\":\"reference_yebis_draw_capture\"")!=std::string::npos){current=int(std::stoul(line.substr(line.find("\"draw\":")+7)));
            if(size_t(current)>=rows.size())rows.resize(size_t(current)+1);continue;}
        if(current>=0 && line.find("\"what\":\"state\"")!=std::string::npos && rows[size_t(current)].empty())rows[size_t(current)]=line;}
    return rows;}
std::array<int,2> Sampler(const std::string& state,size_t slot){
    const auto at=state.find("\"samplers\":[");if(at==std::string::npos)return {-1,-1};size_t p=at+12;
    for(size_t s=0;s<=slot;++s){p=state.find('[',p);if(p==std::string::npos)return {-1,-1};if(s<slot)p=state.find(']',p);}
    const int filter=std::stoi(state.substr(p+1));const int address=std::stoi(state.substr(state.find(',',p)+1));return {filter,address};}
// The plan of a frame against the captured draws 0-9 of the run: technique, target, inputs, samplers, every word read, TEXCOORD1.
GlarePlan CheckPlan(const DofData& data,const DofFrame& f,const std::string& dir,const std::string& name){
    using ds3native::glare_capture::FileBytes;
    const auto rows=ds3native::glare_capture::DrawRows(dir);const auto states=StateRows(dir);const auto counts=DofLevelCounts(f);
    const auto plan=BuildDof(data,f);Expect(plan.refused.empty(),name+" plan served: "+plan.refused);if(!plan.refused.empty())return plan;
    std::vector<const GlareDraw*> draws;for(const auto& g:plan.pre)draws.push_back(&g);for(const auto& g:plan.draws)draws.push_back(&g);
    Expect(rows.size()>draws.size(),name+" the captured frame holds the plan's draws");
    for(size_t k=0;k<draws.size() && k<rows.size();++k){const auto& g=*draws[k];const auto& row=rows[k];const std::string tag=name+" draw "+std::to_string(k);
        Expect(ds3native::glare_capture::Text(row,"pass")==g.technique,tag+" technique "+g.technique+" (captured "+ds3native::glare_capture::Text(row,"pass")+")");
        const uint32_t tw=g.target<0?f.width:plan.surfaces[size_t(g.target)].width,th=g.target<0?f.height:plan.surfaces[size_t(g.target)].height;
        char size[64]{};std::snprintf(size,sizeof(size),"\"rt0\":[10,6,%u,%u",tw,th);Expect(row.find(size)!=std::string::npos,tag+" target "+size);
        for(size_t i=0;i<g.inputs.size();++i){const int8_t in=g.inputs[i];if(in==-2)continue;uint32_t w=f.width,hh=f.height;
            if(in>=0){w=plan.surfaces[size_t(in)].width;hh=plan.surfaces[size_t(in)].height;}else if(in==-5){w=f.table_width;hh=f.table_height;}
            char slot[32]{};std::snprintf(slot,sizeof(slot),"{\"slot\":%zu,",i);char dims[32]{};std::snprintf(dims,sizeof(dims),"\"size\":[%u,%u]",w,hh);
            const auto at=row.find(slot);Expect(at!=std::string::npos && row.find(dims,at)==row.find("\"size\"",at),tag+" input "+std::to_string(i)+" "+dims);
            const auto sampler=k<states.size()?Sampler(states[k],i):std::array<int,2>{-1,-1};
            const int filter=g.point[i]?0:0x14,address=g.address[i]?g.address[i]:g.border?4:3;
            Expect(sampler[0]==filter && sampler[1]==address,tag+" sampler "+std::to_string(i)+" (filter "+std::to_string(filter)+", address "+std::to_string(address)+
                "; captured "+std::to_string(sampler[0])+", "+std::to_string(sampler[1])+")");}
        const auto hex=ds3native::glare_capture::Text(row,"globals");
        Expect(!g.unplaced,tag+" no word without a slot in our block");   // I-0138 tranche 8b
        for(const uint32_t w:ReadWords(g.technique,size_t(counts.both))){const uint32_t ours=ds3native::post_test::LibraryWord(g,w);const uint32_t captured=Word(hex,w);
            char what[128]{};std::snprintf(what,sizeof(what)," word %u (%.9g, captured %.9g)",w,std::bit_cast<float>(ours),std::bit_cast<float>(captured));Expect(ours==captured,tag+what);}
        if(g.texcoord1){char file[64]{};std::snprintf(file,sizeof(file),"/reference-draw-%02zu-vertices.raw",k);const auto v=FileBytes(dir+file);
            Expect(v.size()>=3*24,tag+" vertices captured");
            if(v.size()>=3*24)for(size_t i=0;i<3;++i)for(size_t c=0;c<2;++c)Expect(At<uint32_t>(v,i*24+16+c*4)==Bits(g.uv1[i][c]),tag+" TEXCOORD1");}}
    return plan;}
void Zone(const DofData& data,const std::string& dir){
    using ds3native::glare_capture::FileBytes;
    const std::string name=dir.substr(dir.find_last_of("/\\")+1);
    const auto h=FileBytes(dir+"/reference-dof-holder.raw"),l=FileBytes(dir+"/reference-dof-layers.raw"),s=FileBytes(dir+"/reference-shaft-alpha-library.raw");
    const bool state=h.size()>=0x400 && l.size()>=0x108 && s.size()>=0xD00;Expect(state,name+" DoF state captured");if(!state)return;
    // FUN_141756580: the ramps of every level from the holder and the levels' inputs.
    DofHolder H{At<uint32_t>(h,0x140),At<int>(h,0x154),At<float>(h,0x1B0),At<float>(h,0x1BC),At<float>(h,0x1C0)};
    std::vector<DofLayer> L;const size_t n=l.size()/0x108;for(size_t i=0;i<n;++i)L.push_back({At<uint32_t>(l,i*0x108+4),At<float>(l,i*0x108+0x20),At<float>(l,i*0x108+0x30)});
    const auto ramps=DofLayerRamps(H,L);
    for(size_t i=0;i<n;++i){const float* p=&ramps[i].c0;const uint32_t offs[12]={0xC0,0xC4,0xC8,0xCC,0xD0,0xD4,0xD8,0xDC,0xE8,0xEC,0xF0,0xF4};
        for(int k=0;k<12;++k){char what[96]{};std::snprintf(what,sizeof(what)," level %zu +0x%X (%.9g, captured %.9g)",i,offs[k],p[k],At<float>(l,i*0x108+offs[k]));
            Expect(Bits(p[k])==At<uint32_t>(l,i*0x108+offs[k]),name+what);}}
    // The frame from the holder (+0x140.., the levels) and the library (the depth factor's words).
    DofFrame f;f.width=At<uint32_t>(h,0x144);f.height=At<uint32_t>(h,0x148);f.flags=H.flags;f.mode154=H.mode154;f.scale1bc=H.scale1bc;f.offset1c0=H.offset1c0;
    f.near_plane=At<float>(h,0x168);f.far_plane=At<float>(h,0x16C);f.near_override=At<float>(h,0x1E8);f.far_override=At<float>(h,0x1EC);f.focus=At<float>(h,0x188);
    f.limits={At<int>(h,0x1C4),At<int>(h,0x1C8),At<int>(h,0x1CC),At<int>(h,0x1D0)};f.source_column=At<int>(h,0x1F0);f.level_column=At<int>(h,0x1F4);
    for(size_t i=0;i<n;++i){DofLevel v;const size_t b=i*0x108;v.width=At<uint32_t>(l,b);v.height=At<uint32_t>(l,b+4);v.surface_width=At<uint32_t>(l,b+8);v.surface_height=At<uint32_t>(l,b+0xC);
        const uint64_t surface=At<uint64_t>(l,b+0x38);v.surface=int(i);for(size_t j=0;j<i;++j)if(At<uint64_t>(l,j*0x108+0x38)==surface){v.surface=f.levels[j].surface;break;}
        v.kind=At<int>(l,b+0x10);v.taps=At<int>(l,b+0x18);v.ramps=ramps[i];f.levels.push_back(v);}
    const auto counts=DofLevelCounts(f);
    Expect(counts.front==At<int>(h,0x1D8) && counts.back==At<int>(h,0x1DC) && counts.both==At<int>(h,0x1D4),name+" level counts (+0x1D8 "+std::to_string(counts.front)+
        ", +0x1DC "+std::to_string(counts.back)+", +0x1D4 "+std::to_string(counts.both)+")");
    f.spaces=DofDepthSpaces(At<float>(s,0x564),At<float>(s,0x568),At<float>(s,0xC48),{At<float>(h,0x1B4),At<float>(h,0x1B8),At<float>(h,0x1BC),At<float>(h,0x1C0)});
    f.spaces.source=At<int32_t>(s,0x43C);f.spaces.space3=At<uint8_t>(s,0xC34)?At<float>(h,0x1B0):0.0f;
    Expect(At<uint8_t>(s,0xC35)==0 && At<uint8_t>(s,0xC33)==1,name+" library: the depth-of-field branch, no C35 path");
    f.source_object=At<uint64_t>(s,0x430)!=0 || At<uint64_t>(s,0x50)!=0;f.source_scale=At<float>(s,0x448);f.source_bias=At<float>(s,0x44C);
    f.depth_format=At<int>(s,0x438);f.lookup_space=At<int>(s,0x74);f.factor_space=At<int>(s,0x78);f.table_row=float(At<int>(s,0x80));
    f.conversion={At<float>(s,0xC38),At<float>(s,0xC3C),At<float>(s,0xC40),At<float>(s,0xC44)};
    const auto rows=ds3native::glare_capture::DrawRows(dir);
    {const auto& row=rows.empty()?std::string():rows[0];const auto at=row.find("{\"slot\":2,");Expect(at!=std::string::npos,name+" draw 0's table");
        if(at!=std::string::npos){const auto size=row.find("\"size\":[",at);f.table_width=uint32_t(std::stoul(row.substr(size+8)));f.table_height=uint32_t(std::stoul(row.substr(row.find(',',size+8)+1)));}}
    const auto plan=CheckPlan(data,f,dir,name);
    std::printf("%s: %zu draws from the holder, levels front %d back %d, focus %.9g, scale +0x1B0 %.9g\n",name.c_str(),plan.pre.size()+plan.draws.size(),counts.front,counts.back,f.focus,H.scale1b0);
}
// DF-2: the holder natively, from the executable's tables, the camera record E69A0 takes (reference-driver-camera.raw), the globals block
// after it (reference-glare-globals.raw, the bright pass's: the measured focus written back) and the render extent: the creation
// (+0xC0..+0x11C, the level records), the lens (+0x168..+0x1C0, S+0x6C4, S+0x6CC, the branch), then the plan of that native frame
// against the captured draws, and the DoF table (CustomDofLutCopy's cbComputeCustomDof, ds3light::CustomDofTable) against the
// captured t2 of the depth factor.
int flags0_zones=0;   // part 4b: the zones whose flags-0 frame draws levels (its composite checks ran)
void Native(const DofData& data,const DofSetupData& setup_data,const DofConstants& k,const std::string& dir){
    using ds3native::glare_capture::FileBytes;
    const std::string name=dir.substr(dir.find_last_of("/\\")+1)+" (native)";
    const auto h=FileBytes(dir+"/reference-dof-holder.raw"),l=FileBytes(dir+"/reference-dof-layers.raw"),s=FileBytes(dir+"/reference-shaft-alpha-library.raw"),
        g=FileBytes(dir+"/reference-glare-globals.raw"),c=FileBytes(dir+"/reference-driver-camera.raw"),t2=FileBytes(dir+"/reference-draw-00-t2.raw");
    const bool state=h.size()>=0x400 && l.size()>=7*0x108 && s.size()>=0xD00 && g.size()>=0x370 && c.size()>=0x80 && t2.size()>=2048*2*4;
    Expect(state,name+" native inputs captured");if(!state)return;
    const uint32_t width=At<uint32_t>(c,0x5C),height=At<uint32_t>(c,0x60);
    const auto setup=BuildDofSetup(setup_data,width,height,At<int>(g,0x4C),7);Expect(setup.refused.empty(),name+" setup served: "+setup.refused);if(!setup.refused.empty())return;
    const auto same=[&](const std::vector<uint8_t>& b,size_t at,uint32_t mine,const char* what){char line[128]{};
        std::snprintf(line,sizeof(line)," %s (%08X, captured %08X)",what,mine,At<uint32_t>(b,at));Expect(At<uint32_t>(b,at)==mine,name+line);};
    same(h,0xC0,setup.base_width,"+0xC0");same(h,0xC4,setup.base_height,"+0xC4");same(h,0xC8,setup.creation_flags,"+0xC8");
    same(h,0xCC,uint32_t(setup.levels),"+0xCC");same(h,0xD8,uint32_t(setup.preset),"+0xD8");same(h,0x118,setup.small_width,"+0x118");same(h,0x11C,Bits(setup.small_scale),"+0x11C");
    for(size_t i=0;i<setup.level.size();++i){const auto& v=setup.level[i];const size_t b=i*0x108;char what[48]{};
        const auto at=[&](size_t o,uint32_t mine){std::snprintf(what,sizeof(what),"level %zu +0x%zX",i,o);same(l,b+o,mine,what);};
        at(0,v.width);at(4,v.height);at(8,v.surface_width);at(0xC,v.surface_height);at(0x10,uint32_t(v.kind));at(0x14,uint32_t(v.preset_b));at(0x18,uint32_t(v.taps));
        at(0x20,Bits(v.radius));at(0x24,Bits(v.size));at(0x28,0.0f<v.size_scale?1u:0u);at(0x2C,Bits(v.size_scale));at(0x30,Bits(v.size/float(v.height)));}
    DofLensInputs in;in.fov=At<float>(c,0x50);in.near_plane=At<float>(c,0x54);in.far_plane=At<float>(c,0x58);in.aspect=At<float>(c,0x64);
    in.extent_width=width;in.extent_height=height;in.quality=At<int>(g,0x4C);in.focus=At<float>(g,0x1B0);in.f_number=At<float>(g,0x1B4);in.p6d0=At<float>(g,0x1B8);
    in.angle=At<float>(g,0x1BC);in.diagonal=At<float>(g,0x1C0);in.enable=At<int>(g,0x1AC)!=0;
    const auto lens=DofLensState(k,in);
    same(h,0x168,Bits(lens.near_plane),"+0x168");same(h,0x16C,Bits(lens.far_plane),"+0x16C");same(h,0x174,Bits(lens.tan_half),"+0x174");same(h,0x170,Bits(lens.fov),"+0x170");
    same(s,0x6C4,Bits(lens.sensor),"S+0x6C4");same(s,0x6CC,Bits(lens.f_number),"S+0x6CC");same(h,0x190,Bits(lens.sensor),"+0x190");same(h,0x18C,Bits(lens.f_number),"+0x18C");
    same(h,0x188,Bits(lens.focus),"+0x188");same(h,0x180,Bits(lens.focal),"+0x180");same(h,0x184,Bits(lens.aperture),"+0x184");same(h,0x19C,Bits(lens.coc),"+0x19C");
    same(h,0x1B0,Bits(lens.scale),"+0x1B0");for(uint32_t w=0;w<4;++w)same(h,0x1B4+4*w,Bits(lens.words[w]),"+0x1B4..");
    Expect((At<uint8_t>(h,0x164)!=0)==lens.enable && (At<uint8_t>(s,0xC33)!=0)==lens.branch,name+" the enable (+0x164) and the branch (S+0xC33)");
    auto f=NativeDofFrame(setup,lens,At<int>(g,0x1D0),At<int>(g,0x1D4),At<int>(g,0x240)==2?2u:0xFFFFFFFFu);
    same(h,0x140,f.flags,"+0x140");
    const auto plan=CheckPlan(data,f,dir,name);
    // I-0138 part 4b: the frame these inputs give under holder flags 0, built as the native frame builds it (NativeDofFrame with
    // flags 0: bit 2 clear, its near ramps unscaled, so its counts may differ from flags 2's; YebisDofComposite_14175f850's
    // flags < 2 branch, read on the decompilation; the cutscene capture is its oracle): the far ramps (d0, d4) those of flags
    // 2 (bit 2 does not touch them); no blend factor; one blur a drawn level; then CompositeBlurredLevels_T{levels + 1} over the
    // scene and the drawn levels' results, words 76 + 4k the levels' (d0, d4), TEXCOORD1 as the FrontBlur composite's.
    if(plan.refused.empty() && !plan.draws.empty()){const auto f0=NativeDofFrame(setup,lens,At<int>(g,0x1D0),At<int>(g,0x1D4),0);
        Expect(f0.flags==0 && f0.levels.size()==f.levels.size(),name+" flags 0: the frame");
        bool far_same=f0.levels.size()==f.levels.size();for(size_t i=0;far_same && i<f.levels.size();++i)far_same=Bits(f0.levels[i].ramps.d0)==Bits(f.levels[i].ramps.d0) && Bits(f0.levels[i].ramps.d4)==Bits(f.levels[i].ramps.d4);
        Expect(far_same,name+" flags 0: the far ramps (d0, d4) those of flags 2");
        const auto p0=BuildDof(data,f0);const auto counts=DofLevelCounts(f0);
        Expect(p0.refused.empty() || (p0.refused=="no level drawn" && counts.both<1),name+" flags 0: plan served: "+p0.refused);
        if(p0.refused.empty() && !p0.draws.empty()){++flags0_zones;size_t blends=0,blurs=0;for(const auto& d:p0.draws){blends+=d.technique=="tech_DepthOfField_FrontBlurBlendFactor";blurs+=d.kind==GlareKind::Blur;}
            Expect(blends==0 && blurs==size_t(counts.both),name+" flags 0: no blend factor, one blur a drawn level");
            const auto& a=p0.draws.back();const auto& b=plan.draws.back();size_t ia=0;for(const auto i:a.inputs)ia+=i!=-2;
            Expect(a.technique=="tech_DepthOfField_CompositeBlurredLevels_T"+std::to_string(counts.both+1),name+" flags 0: composite "+a.technique);
            const auto word=[](const GlareDraw& d,uint32_t w){return ds3native::post_test::LibraryWord(d,w);};
            bool words=true;for(uint32_t lv=0;lv<uint32_t(counts.both);++lv){const auto& r=f0.levels[lv].ramps;words=words && word(a,76+4*lv)==Bits(r.d0) && word(a,77+4*lv)==Bits(r.d4) && word(a,78+4*lv)==0 && word(a,79+4*lv)==0;}
            // c18 (words 72..75, the library's zeros): without a slot in our block the builder must not have tried to write it,
            // with one it must hold the zeros (the captured zeros are checked by CheckPlan's ReadWords).
            words=words && !a.unplaced;if(glare_block_slots[18]>=0)for(uint32_t w=72;w<76;++w)words=words && word(a,w)==0;
            Expect(ia==size_t(counts.both)+1 && a.inputs[0]==-1 && a.point[0] && words && a.texcoord1==b.texcoord1 && a.uv1==b.uv1,name+" flags 0: the composite's inputs, words and coordinates");}}
    // The DoF table: CustomDofTable on the native inputs (the texture 2048x2, the lens, the exposure +0x1B0 under S+0xC34, the DoF globals
    // 144962244..26C) against row 0 and row 1 of the captured t2 (CustomDofLutCopy writes both from g_Coc).
    ds3light::CustomDofInput t;Expect(ds3light::CustomDofConstants(t),name+" CustomDof constants");
    t.width=2048;t.height=2;t.near_plane=lens.near_plane;t.far_plane=lens.far_plane;t.lens={lens.focus,lens.words[0],lens.words[1],1.0f/lens.words[0],-(lens.words[1]/lens.words[0])};
    t.exposure=lens.enable?lens.scale:0.0f;for(uint32_t w=0;w<11;++w)t.dof[w]=At<uint32_t>(g,0x2D4+4*w);
    std::vector<uint8_t> cb(0x2010);ds3light::CustomDofTable(t,cb.data());
    size_t differ0=0,differ1=0;for(size_t i=0;i<2048;++i){const uint32_t mine=At<uint32_t>(cb,0x10+4*i);differ0+=At<uint32_t>(t2,4*i)!=mine;differ1+=At<uint32_t>(t2,2048*4+4*i)!=mine;}
    Expect(differ0==0,name+" DoF table row 0: "+std::to_string(differ0)+" of 2048 differ");
    std::printf("%s: setup %ux%u (quality %d, preset %d), lens focal %.9g scale %.9g, plan %zu draws, table row 1 differs at %zu of 2048\n",name.c_str(),setup.base_width,setup.base_height,
        in.quality,setup.preset,lens.focal,lens.scale,plan.pre.size()+plan.draws.size(),differ1);
}
}
int main(int argc,char** argv){
    DofData data{};DofSetupData setup{};DofConstants k{};bool exe=false;
    for(int a=1;a<argc;++a){const std::string arg=argv[a];if(arg.size()>4 && arg.substr(arg.size()-4)==".exe"){_putenv_s("DS3_EXE",argv[a]);
        const auto read=[](uint32_t rva,void* out,size_t bytes){return ds3light::Rdata(rva,out,uint32_t(bytes));};
        exe=LoadDofDrawData(data,read) && LoadDofSetupData(setup,read) &&
            read(0x3D7EE10,&k.one,4) && read(0x3D7EDB4,&k.half,4) && read(0x3D7EE7C,&k.two,4) && read(0x3D7EEC0,&k.pi,4) && read(0x3D7EFE8,&k.degrees,4) &&
            read(0x3D7ED10,&k.milli,4) && read(0x3D7F088,&k.full_hd,4) && read(0x3D7EF48,&k.quality_steps,4);}}
    if(!exe){std::printf("FAIL yebis dof: the executable's words (an argument ending in .exe)\n");return 1;}
    int zones=0;for(int a=1;a<argc;++a){const std::string arg=argv[a];if(arg.size()>4 && arg.substr(arg.size()-4)==".exe")continue;Zone(data,arg);Native(data,setup,k,arg);++zones;}
    std::printf("flags 0: the composite checks ran at %d of %d zones (the others draw no level at their focus)\n",flags0_zones,zones);
    Expect(flags0_zones>0,"flags 0: the composite checks ran at one zone at least");
    if(failures){std::printf("FAIL yebis dof: %d of %d checks\n",failures,checks);return 1;}
    std::printf("PASS yebis dof: the level ramps, the level counts, the plan of the ten DoF draws from the holder and from the native holder, and the DoF table equal the original's, bit for bit, at %d zone captures (%d checks)\n",zones,checks);
    return 0;
}
