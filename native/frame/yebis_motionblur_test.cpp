// W10 MB-1 (W10-NATIVE-PASSES 10): the motion blur's plan against the original's frame, per zone. Each argument is a Reference run
// with the draws capture and the library at the Alpha_R draw of the same frame (reference-shaft-alpha-library.raw, 0xD00 bytes:
// Reference1676 m37, 1677 m31, 1678 m50), whose frame draws the camera velocity (draw 11) then the blur passes. Checked: the plan of
// yebis::BuildMotionBlur from the library's fields against the captured draws: technique, target and input extents, the samplers of
// the slots the programmes read, every block word the programmes read (the velocity draw's DepthCastScaleOffset, PerspectiveFactor,
// its three Modulate float4 and the transform; the blur's weights, their sum and its five Offset float4), the velocity draw's
// vertices. A named run without its files fails.
#include "mb_capture.h"
#include "glare_library_block.h"
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

using namespace ds3native::yebis;
using ds3native::glare_capture::At;
namespace {
int failures{},checks{};
void Expect(bool ok,const std::string& what){++checks;if(!ok){++failures;std::printf("FAIL %s\n",what.c_str());}}
uint32_t Word(const std::string& hex,uint32_t w){uint32_t v{};if(hex.size()>=8*(w+1))for(int b=3;b>=0;--b)v=(v<<8)|uint32_t(std::stoul(hex.substr(8*w+2*size_t(b),2),nullptr,16));return v;}
// The block words the programmes read (the DXBC's ps_used_globals; vs_2D_Tex2D1/3 read fParam_ScreenSpaceScale).
std::vector<uint32_t> ReadWords(const std::string& t,int taps){
    std::vector<uint32_t> w{20,21};
    if(t.rfind("tech_MotionBlurMakeCameraVelocity",0)==0){for(uint32_t i:{0u,1u,60u,61u,62u,63u})w.push_back(i);for(uint32_t i=616;i<628;++i)w.push_back(i);
        for(uint32_t i=896;i<912;++i)w.push_back(i);}
    else{for(uint32_t i=616;i<616+4*uint32_t(taps+1);++i)w.push_back(i);for(uint32_t i=744;i<764;++i)if(i!=759)w.push_back(i);}
    return w;}
std::vector<std::string> StateRows(const std::string& dir){std::ifstream trace(dir+"/startup.jsonl");std::vector<std::string> rows;std::string line;int current=-1;
    while(std::getline(trace,line)){
        if(line.find("\"event\":\"reference_yebis_draw_capture\"")!=std::string::npos){current=int(std::stoul(line.substr(line.find("\"draw\":")+7)));
            if(size_t(current)>=rows.size())rows.resize(size_t(current)+1);continue;}
        if(current>=0 && line.find("\"what\":\"state\"")!=std::string::npos && rows[size_t(current)].empty())rows[size_t(current)]=line;}
    return rows;}
std::array<int,2> Sampler(const std::string& state,size_t slot){
    const auto at=state.find("\"samplers\":[");if(at==std::string::npos)return {-1,-1};size_t p=at+12;
    for(size_t s=0;s<=slot;++s){p=state.find('[',p);if(p==std::string::npos)return {-1,-1};if(s<slot)p=state.find(']',p);}
    return {std::stoi(state.substr(p+1)),std::stoi(state.substr(state.find(',',p)+1))};}
void Zone(const std::string& dir){
    using ds3native::glare_capture::FileBytes;
    const std::string name=dir.substr(dir.find_last_of("/\\")+1);
    const auto s=FileBytes(dir+"/reference-shaft-alpha-library.raw");const bool state=s.size()>=0xD00;Expect(state,name+" library captured");if(!state)return;
    const auto rows=ds3native::glare_capture::DrawRows(dir);const auto states=StateRows(dir);
    size_t first=rows.size();for(size_t n=0;n<rows.size();++n)if(ds3native::glare_capture::Text(rows[n],"pass").rfind("tech_MotionBlurMakeCameraVelocity",0)==0){first=n;break;}
    Expect(first<rows.size(),name+" the captured frame holds the velocity draw");if(first==rows.size())return;
    const auto in=ds3native::mb_capture::Inputs(s);const auto plan=BuildMotionBlur(MotionBlurData{},in);
    Expect(plan.refused.empty(),name+" plan served: "+plan.refused);if(!plan.refused.empty())return;
    std::printf("%s: N %d, R %d, n %d, step/L %.9g, near %.9g, far %.9g\n",name.c_str(),plan.base_taps,plan.passes,plan.taps,plan.step,plan.near_factor,plan.far_factor);
    std::vector<const GlareDraw*> draws;for(const auto& g:plan.pre)draws.push_back(&g);for(const auto& g:plan.draws)draws.push_back(&g);
    // The draws after the blur passes are not the motion blur's.
    Expect(first+draws.size()<rows.size() && ds3native::glare_capture::Text(rows[first+draws.size()],"pass").rfind("tech_MotionBlur",0)!=0,name+" the pass count equals the captured blur draws");
    for(size_t k=0;k<draws.size() && first+k<rows.size();++k){const auto& g=*draws[k];const auto& row=rows[first+k];const std::string tag=name+" draw "+std::to_string(first+k);
        Expect(ds3native::glare_capture::Text(row,"pass")==g.technique,tag+" technique "+g.technique+" (captured "+ds3native::glare_capture::Text(row,"pass")+")");
        const uint32_t tw=g.target<0?in.width:plan.surfaces[size_t(g.target)].width,th=g.target<0?in.height:plan.surfaces[size_t(g.target)].height;
        char size[64]{};std::snprintf(size,sizeof(size),"\"rt0\":[10,6,%u,%u",tw,th);Expect(row.find(size)!=std::string::npos,tag+" target "+size);
        for(size_t i=0;i<g.inputs.size();++i){const int8_t input=g.inputs[i];if(input==-2)continue;uint32_t w=in.width,h=in.height;
            if(input>=0){w=plan.surfaces[size_t(input)].width;h=plan.surfaces[size_t(input)].height;}else if(input==-6){w=in.velocity_width;h=in.velocity_height;}
            char slot[32]{};std::snprintf(slot,sizeof(slot),"{\"slot\":%zu,",i);char dims[32]{};std::snprintf(dims,sizeof(dims),"\"size\":[%u,%u]",w,h);
            const auto at=row.find(slot);Expect(at!=std::string::npos && row.find(dims,at)==row.find("\"size\"",at),tag+" input "+std::to_string(i)+" "+dims);
            const auto sampler=first+k<states.size()?Sampler(states[first+k],i):std::array<int,2>{-1,-1};
            const int filter=g.point[i]?0:0x14,address=g.address[i]?g.address[i]:g.border?4:3;
            Expect(sampler[0]==filter && sampler[1]==address,tag+" sampler "+std::to_string(i)+" (filter "+std::to_string(filter)+", address "+std::to_string(address)+
                "; captured "+std::to_string(sampler[0])+", "+std::to_string(sampler[1])+")");}
        const auto hex=ds3native::glare_capture::Text(row,"globals");
        const int taps=g.technique.back()-'0';
        Expect(!g.unplaced,tag+" no word without a slot in our block");   // I-0138 tranche 8b
        for(const uint32_t w:ReadWords(g.technique,taps)){const uint32_t ours=ds3native::post_test::LibraryWord(g,w);const uint32_t captured=Word(hex,w);
            char what[128]{};std::snprintf(what,sizeof(what)," word %u (%.9g, captured %.9g)",w,std::bit_cast<float>(ours),std::bit_cast<float>(captured));Expect(ours==captured,tag+what);}
        if(g.texcoords>1){char file[64]{};std::snprintf(file,sizeof(file),"/reference-draw-%02zu-vertices.raw",first+k);const auto v=FileBytes(dir+file);
            const size_t stride=8+8*size_t(g.texcoords);Expect(v.size()>=3*stride && g.vertices.size()==3*stride/4,tag+" vertices captured");
            if(v.size()>=3*stride && g.vertices.size()==3*stride/4)for(size_t i=0;i<g.vertices.size();++i)Expect(At<uint32_t>(v,4*i)==std::bit_cast<uint32_t>(g.vertices[i]),tag+" vertex word "+std::to_string(i));}}
}
}
int main(int argc,char** argv){
    // Without a capture named, the matrix and planner checks below still run (they need none); the zones' are then none.
    // The matrix helpers on known values: the inverse of a scale-and-translate, the product's order.
    {Matrix4 m{2,0,0,0, 0,4,0,0, 0,0,8,0, 1,2,3,1},inv{};Expect(Inverse(m,inv) && inv[0]==0.5f && inv[5]==0.25f && inv[10]==0.125f && inv[12]==-0.5f && inv[13]==-0.5f && inv[14]==-0.375f,"inverse");
        const auto p=Multiply(m,inv);bool identity=true;for(int i=0;i<16;++i)identity=identity && p[i]==((i%5)==0?1.0f:0.0f);Expect(identity,"product with the inverse");}
    // Review: a pose yawed by 90 degrees (the pivot search swaps columns) and a yawed, pitched pose: A inv(A) within 1e-5 of I.
    for(const Matrix4& m:{Matrix4{0,0,-1,0, 0,1,0,0, 1,0,0,0, 390.8f,-261.9f,-906.3f,1},Matrix4{0.6f,0.48f,-0.64f,0, 0,0.8f,0.6f,0, 0.8f,-0.36f,0.48f,0, -12,5,700,1}}){
        Matrix4 inv{};const bool ok=Inverse(m,inv);const auto p=Multiply(m,inv);bool identity=ok;
        for(int i=0;i<16;++i){const float error=std::fabs(p[i]-((i%5)==0?1.0f:0.0f));identity=identity && error<=(i<12?1e-5f:2e-3f);if(error>(i<12?1e-5f:2e-3f))std::printf("  inverse check element %d: %.9g\n",i,p[i]);}
        Expect(identity,"inverse of a rotated pose (pivot swaps; the translation row within 2e-3 at coordinates near 1000)");}
    // Review: the planner's paths the captures do not take (they take the recurrence cap: N 5, R 3 capped to 2, n forced to N).
    {const MotionBlurData d{};const auto check=[&](float l,int n,int parity,int cap,float interleave,int passes,int taps,float step,const char* what){
            const auto p=PlanMotionBlur(d,l,n,parity,cap,interleave);
            char text[160]{};std::snprintf(text,sizeof(text),"planner %s: (%d, %d, %.9g), expected (%d, %d, %.9g)",what,p.passes,p.taps,p.step,passes,taps,step);
            Expect(p.passes==passes && p.taps==taps && p.step==step,text);};
        check(72.0f,5,1,1,1.0f,1,5,18.0f,"default.gparam R 1");                 // L 72, cap 1: one pass of N taps, step 72 / 4
        check(72.0f,5,1,0,1.0f,3,3,72.0f/74.0f,"uncapped R 3");                 // spread 73: R ceil(2.67) 3, n ceil(2.92) 3
        check(72.0f,5,1,0,2.0f,3,3,72.0f/74.0f,"interleave 2");                 // spread 37: R 3, n ceil(1.48) 2, odd 3
        check(72.0f,5,1,2,2.0f,2,5,3.0f,"interleave 2, cap 2");
        check(72.0f,4,2,0,1.0f,4,2,72.0f/127.0f,"even taps (Bidirectional off)");   // N 4: R ceil(3.09) 4, n ceil(1.14) 2
        check(0.05f,5,1,2,1.0f,0,0,0.0f,"no blur at or below 1.1");
        // I-0138 tranche 4 (the final review's M1): with the values every world record holds (NumBaseSamples 4 both ways, so 5 base
        // taps; MaxBlurLength 0.1; MaxRecurrences 2, the option's cap leaving 1 or 2; no interleave), the recurrence cap is taken
        // at every picture height from 360 to 4320, so every pass draws 5 taps: a tap count other than 5 needs other values (a
        // cutscene's record; under a height of 241 the cap of 2 is no longer taken, and the odd parity still gives 5 down to
        // a spread of 15).
        for(const int cap:{1,2})for(const int height:{360,450,720,843,900,1050,1080,1440,2160,4320}){
            const auto p=PlanMotionBlur(d,float(height)*0.1f,5,1,cap,1.0f);
            char text[120]{};std::snprintf(text,sizeof(text),"the world's values at height %d, cap %d: %d passes of %d taps, expected %d of 5",height,cap,p.passes,p.taps,cap);
            Expect(p.passes==cap && p.taps==5,text);}}
    for(int i=1;i<argc;++i)Zone(argv[i]);
    if(failures){std::printf("FAIL yebis motion blur: %d of %d checks failed\n",failures,checks);return 1;}
    std::printf("PASS yebis motion blur: %d checks, the velocity draw and the blur passes equal the captured draws word for word at %d run(s)\n",checks,argc-1);
    return 0;}
