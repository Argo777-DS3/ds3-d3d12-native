// W10 GL-2/GL-4 (W10-NATIVE-PASSES 5): the native glare plan against the original's frame, per zone. Each argument after the
// executable is a Reference run with the draws capture and the glare state (yebis-draws-capture.flag: Reference1649 m37, 1650 m31,
// 1651 m50, 1652 m45): the plan is built from the library's globals and state captured at the bright-pass draw
// (glare_capture.h) and compared with the captured draws of that frame (the reference_yebis_draw rows: technique, target, inputs and
// the `$Globals` block): the source downsamples, the bright pass and every draw up to the composite in order, then the 2x draw into
// the tone map's t1 (the original draws the light shafts in between: on shape 1 after the bloom composite, with their MadT2 or a MadT1 copy;
// on shape 3 before the layers' composite, which reads them as its last layer; the zones named « -shafts- » (tranche 8i) append them
// to the plan, and their draws' words are not checked here: yebis_shafts_test's LS-2, every other draw's still are). Checked:
// techniques, target sizes, input sizes, and bit for bit every block word the draw's programmes read (fParam_ScreenSpaceScale, the VS's
// afUV_TexCoordOffsetV16 taps and the PS's extra taps at cb0[42 + t], the afRGBA_Modulate taps, the bright pass's remap words, the
// composite's gamma word), and the vignette bright pass's TEXCOORD1 against the captured vertices. Skipped with a message without
// the executable or a run's files.
#include "glare_capture.h"
#include "glare_library_block.h"
#include "../world/yebis_tonemap.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

using namespace ds3native::yebis;
using ds3native::glare_capture::At;
namespace {
int failures{},checks{},rotations{},rotations_exact{};
using ds3native::glare_capture::Text;
void Fail(const std::string& what){++failures;std::printf("FAIL %s\n",what.c_str());}
// The words a draw's programmes read.
std::vector<uint32_t> ReadWords(const std::string& p){
    std::vector<uint32_t> words{20,21};
    const auto taps=[&](uint32_t n,bool offsets){for(uint32_t i=0;i<n;++i){if(offsets){const uint32_t at=(i<8?136:168)+4*i;words.push_back(at);words.push_back(at+1);}
        for(uint32_t c=0;c<4;++c)words.push_back(616+4*i+c);}};
    if(p.rfind("tech_MadT1_S",0)==0)taps(uint32_t(std::stoul(p.substr(12))),true);
    else if(p.rfind("tech_BrightPass",0)==0){for(uint32_t w:{616u,617u,618u,620u,621u,622u,627u})words.push_back(w);
        if(p.find("_REMAP_")!=std::string::npos){words.push_back(836);words.push_back(839);}}
    else if(p.rfind("tech_MadLumGammaT",0)==0){taps(uint32_t(std::stoul(p.substr(17))),false);words.push_back(808);}
    else if(p.rfind("tech_MadT",0)==0)taps(uint32_t(std::stoul(p.substr(9))),false);
    // I-0138 tranche 8f: shape 3's programmes (their cb0 reads, fxc /dumpbin of the DXBC: evidence/I-0138/t8d-readings/MaxColorT4.asm,
    // MadOffsetT2.asm, t8e-readings/dxbc): MaxColorT{n} the weights and cb0[186].rgb; MadOffsetT2 the two weights and cb0[186..188];
    // the ghosts' VS (vs_2D_PointSymmetryTex2D8) cb0[34..49].xy and cb0[146..153].x, .zw, their PS the weights, and DistScale's
    // cb0[50..57].x, cb0[58], cb0[59], cb0[61..68].xy.
    else if(p.rfind("tech_MaxColorT",0)==0){taps(uint32_t(std::stoul(p.substr(14))),false);for(uint32_t w:{744u,745u,746u})words.push_back(w);}
    else if(p=="tech_MadOffsetT2"){taps(2,false);for(uint32_t w=744;w<756;++w)words.push_back(w);}
    else if(p.rfind("tech_GlareGhost_",0)==0){taps(8,false);
        for(uint32_t g=0;g<8;++g)for(uint32_t w:{136+4*g,137+4*g,168+4*g,169+4*g,584+4*g,586+4*g,587+4*g})words.push_back(w);
        if(p.find("DistScale")!=std::string::npos){for(uint32_t w=232;w<240;++w)words.push_back(w);
            for(uint32_t g=0;g<8;++g)for(uint32_t w:{200+4*g,244+4*g,245+4*g})words.push_back(w);}}
    return words;}
uint32_t Word(const std::string& hex,uint32_t w){uint32_t v{};if(hex.size()>=8*(w+1))for(int b=3;b>=0;--b)v=(v<<8)|uint32_t(std::stoul(hex.substr(8*w+2*size_t(b),2),nullptr,16));return v;}
void Zone(const GlareData& d,const std::string& dir){
    const auto state=ds3native::glare_capture::Load(dir);const auto rows=ds3native::glare_capture::DrawRows(dir);
    const std::string name=dir.substr(dir.find_last_of("/\\")+1);
    if(!state.Ok() || rows.empty()){Fail(name+": no state or draws in the named capture");return;}
    const auto in=ds3native::glare_capture::Inputs(state);const auto& object=state.object;
    {++checks;if(At<float>(object,0x118)!=in.luminance || At<float>(object,0x11C)!=in.threshold || At<float>(object,0x2E4)!=in.radius_scale ||
        At<float>(object,0x1B8)!=in.composite_gamma || At<float>(object,0x124)!=in.remap || At<float>(object,0x2E0)!=in.sampling_threshold ||
        At<float>(object,0x148)!=in.exposure)Fail(name+": the glare object's fields differ from the globals they are read from");}
    // I-0138 tranche 8f: shape 3's globals as the object holds them, and its record as FUN_141748760 makes it from the executable's tables.
    if(in.shape==3){++checks;if(At<float>(object,0x2E8)!=in.star_threshold || At<float>(object,0x1B4)!=in.star_fov || At<float>(object,0x2EC)!=in.star_softness ||
            At<float>(object,0x1A8)!=in.ghost_distortion)Fail(name+": the glare object's shape-3 fields differ from the globals they are read from");
        const auto made=ShapeRecord(d);const auto& r=in.record;++checks;
        if(made.bloom!=r.bloom || made.scale!=r.scale || made.tint!=r.tint || made.tint_levels!=r.tint_levels || made.ghost!=r.ghost ||
            made.ghost_scale!=r.ghost_scale || made.ghost_size!=r.ghost_size || made.lens_flare!=r.lens_flare || made.streak!=r.streak || made.angle!=r.angle ||
            made.colour!=r.colour || made.colour_flag!=r.colour_flag || made.after_time!=r.after_time || made.after_weight!=r.after_weight || made.arms!=r.arms ||
            made.arm_a!=r.arm_a || made.arm_length!=r.arm_length || made.odd_length!=r.odd_length || made.preset_flag!=r.preset_flag)
            Fail(name+": GlareShape 3's record made from the executable's tables differs from the captured record (CRenderGlare +0x150)");}
    auto plan=BuildGlare(d,in);
    if(!plan.refused.empty()){Fail(name+": the plan is refused: "+plan.refused);return;}
    // I-0138 tranche 8i: a GlareShape 3 capture with its shafts (« -shafts- » or « -sun- », i138-t8i-oracle-shafts-*, -sun-*): the shafts appended from
    // the mask draw's state (AppendShafts: before the layers' composite, which reads their output as one more layer; without one, before
    // the 2x draw, with MadT2); the shaft draws' own words (the appended range, MadT2 included when there is one) are yebis_shafts_test's
    // (LS-2), their techniques, targets and inputs' sizes checked here with the rest. The layers' composite keeps its own words checked.
    size_t shafts_from=0,shafts_to=0;
    if(name.find("-shafts-")!=std::string::npos || name.find("-sun-")!=std::string::npos){ShaftInputs shaft_in;size_t mask{};++checks;
        if(in.shape!=3 || !ds3native::glare_capture::ShaftInputsOf(dir,rows,shaft_in,mask)){Fail(name+": not a shape-3 capture with the shafts' state");return;}
        // The appended draws start where AppendShafts inserts them: before the layers' composite when the plan has one, else before the 2x
        // draw (the MadT2 path: the shaft draws and MadT2 are inserted there, whichever the plan's other draws are).
        const size_t before=plan.draws.size();shafts_from=before-1;
        for(size_t k=0;k<plan.draws.size();++k)if(plan.draws[k].kind==GlareKind::Layers)shafts_from=k;std::string refused;++checks;
        if(!AppendShafts(plan,shaft_in,BuildShaft(shaft_in),refused)){Fail(name+": the shafts are not appended: "+refused);return;}
        shafts_to=shafts_from+(plan.draws.size()-before);   // the inserted draws only: [shafts_from, shafts_to)
        if(plan.draws.size()-before==0){Fail(name+": the shafts appended no draw");return;}}
    ++checks;if(plan.levels!=At<int32_t>(object,0x2B8))Fail(name+": the level count (+0x2B8)");
    ++checks;if(plan.radius!=At<float>(object,0x1AC))Fail(name+": the radius base R (+0x1AC)");
    ++checks;if(plan.base_width!=At<uint32_t>(object,0xB0) || plan.base_height!=At<uint32_t>(object,0xB4))Fail(name+": the base size (+0xB0, +0xB4)");
    // I-0138 tranche 8f: the native sources of the object's +0x2F8, +0x15C and +0x158 (the renderer derives them; the plan above
    // took the captured words): the base level exact, the rotation from the globals' DiaphragmRotate words within one float ulp
    // (logf and tanf of another library), the frame time the globals' word +4.
    ++checks;if(GlareBaseLevel(plan.base_width,in.width)!=At<int32_t>(object,0x2F8))Fail(name+": the base level (+0x2F8) from the base width");
    {const float made=GlareRotation(At<float>(object,0x138),At<int32_t>(state.globals,0x1CC),At<float>(state.globals,0x1C4),At<float>(state.globals,0x1C8));
        const float captured=At<float>(object,0x15C);int32_t a{},b{};std::memcpy(&a,&made,4);std::memcpy(&b,&captured,4);++checks;
        if(std::abs(int64_t(a)-int64_t(b))>1)Fail(name+": the rotation (+0x15C) from the globals' DiaphragmRotate words");
        if(a==b)++rotations_exact;++rotations;}
    ++checks;if(At<uint32_t>(object,0x158)!=At<uint32_t>(state.globals,4))Fail(name+": the frame time (+0x158) is not the globals' word +4");
    const auto original=ds3native::glare_capture::OriginalDraws(plan,rows);
    ++checks;if(original.size()!=plan.draws.size()){Fail(name+": the capture does not hold the plan's draws");return;}
    for(size_t k=0;k<plan.draws.size();++k){const auto& g=plan.draws[k];const auto& row=original[k]<rows.size()?rows[original[k]]:std::string();
        const std::string tag=name+" draw "+std::to_string(original[k]);
        ++checks;if(Text(row,"pass")!=g.technique){Fail(tag+" technique "+g.technique+" against "+Text(row,"pass"));continue;}
        const auto& t=plan.surfaces[size_t(g.target)];char size[64]{};std::snprintf(size,sizeof(size),"\"rt0\":[%u,6,%u,%u",t.half_float?10u:26u,t.width,t.height);
        ++checks;if(row.find(size)==std::string::npos)Fail(tag+" target "+size);
        for(size_t s=0;s<g.inputs.size();++s){if(g.inputs[s]==-2)continue;uint32_t w=in.width,h=in.height;if(g.inputs[s]==-7)w=h=64;else if(g.inputs[s]==-3)w=h=film_grain_size;/* tranche 8i: the shafts mask's film grain */else if(g.inputs[s]>=0){w=plan.surfaces[size_t(g.inputs[s])].width;h=plan.surfaces[size_t(g.inputs[s])].height;}
            char input[64]{};std::snprintf(input,sizeof(input),"{\"slot\":%zu,\"format\":",s);char dims[32]{};std::snprintf(dims,sizeof(dims),"\"size\":[%u,%u]",w,h);
            const auto at=row.find(input);++checks;if(at==std::string::npos || row.find(dims,at)!=row.find("\"size\"",at))Fail(tag+" input "+std::to_string(s)+" size");}
        const auto hex=Text(row,"globals");
        if(k>=shafts_from && k<shafts_to)continue;   // tranche 8i: a shaft draw's words are LS-2's
        ++checks;if(g.unplaced)Fail(tag+" "+g.technique+": a word without a slot in our block");   // I-0138 tranche 8b
        for(const uint32_t w:ReadWords(g.technique)){const uint32_t captured=Word(hex,w);const uint32_t ours=ds3native::post_test::LibraryWord(g,w);++checks;
            if(ours!=captured){float a{},b{};std::memcpy(&a,&ours,4);std::memcpy(&b,&captured,4);char line[240]{};
                std::snprintf(line,sizeof(line),"%s %s word %u: %08X (%.9g), captured %08X (%.9g), %d ulps",tag.c_str(),g.technique.c_str(),w,ours,a,captured,b,int(ours)-int(captured));Fail(line);}}
        if(g.texcoord1){char file[64]{};std::snprintf(file,sizeof(file),"/reference-draw-%02zu-vertices.raw",original[k]);const auto v=ds3native::glare_capture::FileBytes(dir+file);
            ++checks;if(v.size()<3*24){Fail(tag+" vertices");continue;}
            bool same=true;for(size_t i=0;i<3;++i)for(size_t c=0;c<2;++c){float captured{};std::memcpy(&captured,v.data()+i*24+16+c*4,4);same=same && !std::memcmp(&captured,&g.uv1[i][c],4);}
            if(!same)Fail(tag+" TEXCOORD1 differs from the captured vertices");}}
    std::printf("zone %s: %zu draws (%d levels, base %ux%u, R %.9g, L %.9g%s%s) checked\n",name.c_str(),plan.draws.size(),plan.levels,plan.base_width,plan.base_height,
        plan.radius,plan.luminance,in.vignette?", vignette bright pass":"",in.composite_gamma!=1.0f?", luminance gamma":"");
}
}
int main(int argc,char** argv){
    if(argc<3){std::printf("usage: yebis_glare_test <DarkSoulsIII.exe> <Reference run directory>...\n");return 1;}
    _putenv_s("DS3_EXE",argv[1]);
    // I-0138 tranche 8c: the library's gate on the family (no executable needed): the luminance's reason named first (our order: the library branches on the shape first, the skip the same), then shape 0, shape 1 drawn.
    {using G=ds3native::yebis::GlareGate;constexpr auto gate=ds3native::yebis::GlareShapeGate;const float nan=std::numeric_limits<float>::quiet_NaN();
        const struct {int shape;float luminance;G want;} cases[]{{1,3.0f,G::Draw},{0,3.0f,G::SkipShape0},{3,3.0f,G::Draw},{2,3.0f,G::RefuseShape},{4,3.0f,G::RefuseShape},{14,1.0f,G::RefuseShape},
            {1,0.0f,G::SkipLuminance},{1,-1.0f,G::SkipLuminance},{1,nan,G::SkipLuminance},{0,0.0f,G::SkipLuminance},{3,0.0f,G::SkipLuminance}};
        int bad{};for(const auto& c:cases){++checks;if(gate(c.shape,c.luminance)!=c.want){++bad;Fail("glare gate shape "+std::to_string(c.shape)+" luminance "+std::to_string(c.luminance));}}
        if(bad)return 1;
        std::printf("PASS yebis glare gate: the family skipped at a glare luminance not positive (NaN included) or shape 0, drawn at shapes 1 and 3, refused at the other shapes (%zu cases)\n",std::size(cases));}
    GlareData d;if(!ds3native::glare_capture::Data(d)){std::printf("PASS yebis glare: skipped (no executable)\n");return 0;}
    for(int a=2;a<argc;++a)Zone(d,argv[a]);
    if(failures){std::printf("FAIL yebis glare: %d of %d checks\n",failures,checks);return 1;}
    std::printf("PASS yebis glare: the native plans equal the original's glare draws (techniques, targets, input sizes) and every block word their programmes read, bit for bit, at %d zone captures (the words of the shaft draws appended in the -shafts- zones are yebis_shafts_test's, not counted here) (%d checks); the native rotation (+0x15C) bit-equal at %d of %d (one ulp allowed)\n",argc-2,checks,rotations_exact,rotations);
    return 0;
}
