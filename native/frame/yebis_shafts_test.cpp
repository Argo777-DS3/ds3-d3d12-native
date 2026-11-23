// W10 LS-1 (W10-NATIVE-PASSES 7): Alpha_R's block computed natively against the original's frame, per zone. Each argument is a
// Reference run with the draws capture and the shaft state (CaptureYebisShaftState: Reference1660 m37): the library's depth words
// at the Alpha_R draw (reference-shaft-alpha-library.raw, -dof.raw) give the inputs, the focus distance is checked against the
// globals block as E69A0 read it (reference-driver-globals.raw +0x1B0, 144962120) when the run has that file (i138-t8i-oracle-shafts-*,
// 1705, 1706), else against the one captured at the bright pass (reference-glare-globals.raw; the runs 1660 to 1666), and the block is compared bit for bit with
// the captured draw's `$Globals` words 616-623 and 744-747. Also: FUN_1416F4AF0's directions, inverse and scale against hand
// values. A named run without its files fails; a run without the depth-of-field branch (S+0xC33 clear) is checked against
// NearDepthSpaces (I-0138 tranche 8d; no such capture yet). An
// argument ending in .exe is the executable (DS3_EXE, for the projection's constants).
#include "glare_capture.h"
#include "glare_library_block.h"
#include "../world/yebis_shafts.h"
#include "../world/light_producers.h"
#include <algorithm>
#include <bit>
#include <cmath>
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
void Zone(const std::string& dir){
    using ds3native::glare_capture::FileBytes;
    const auto library=FileBytes(dir+"/reference-shaft-alpha-library.raw"),dof=FileBytes(dir+"/reference-shaft-alpha-dof.raw"),globals=FileBytes(dir+"/reference-glare-globals.raw");
    const std::string name=dir.substr(dir.find_last_of("/\\")+1);
    const bool state=library.size()>=0xD00 && dof.size()>=0x400 && globals.size()>=0x370;Expect(state,name+" shaft state captured");if(!state)return;
    const auto rows=ds3native::glare_capture::DrawRows(dir);size_t draw=rows.size();
    for(size_t n=0;n<rows.size();++n)if(ds3native::glare_capture::Text(rows[n],"pass")=="tech_Alpha_R"){draw=n;break;}
    Expect(draw<rows.size(),name+" Alpha_R draw captured");if(draw==rows.size())return;
    // I-0138 tranche 8d: outside the depth-of-field branch (S+0xC33 clear) the words are NearDepthSpaces' (c48 the near plane, the
    // blur's or a shaft's case); no such capture yet, the branch's own checks unchanged.
    const bool branch=At<uint8_t>(library,0xC33)==1;
    // I-0138 tranche 8i: the frame's globals as E69A0 read them (reference-driver-globals.raw, captured before its call) when the run
    // has them. In i138-t8i-oracle-shafts-1250 the two blocks differ in word 0x1B0 alone (the focus: 3.8820 in the bright pass's block,
    // 3.8919 in the driver block and in the library, OBSERVED); that the bright pass's block holds a later frame's focus (the cutscene's
    // camera moving) is an INTERPRETATION, not read. In 1705 and 1706, the runs read here that have both files, the two blocks are
    // byte-identical, so their reference does not change; 1667 to 1669 (shaftdriver) are not read by Zone. A run without the driver
    // block keeps the bright pass's.
    const auto driver_globals=FileBytes(dir+"/reference-driver-globals.raw");const auto& frame_globals=driver_globals.size()>=0x370?driver_globals:globals;
    std::printf("%s: depth-of-field branch (S+0xC33) %d\n",name.c_str(),branch?1:0);
    if(branch)Expect(At<uint32_t>(library,0x6C0)==At<uint32_t>(frame_globals,0x1B0) && At<uint32_t>(library,0xC48)==At<uint32_t>(frame_globals,0x1B0),name+" focus distance = globals 144962120");
    else Expect(At<uint32_t>(library,0xC48)==At<uint32_t>(library,0x564),name+" outside the branch, S+0xC48 = the near plane");
    AlphaRInputs in;
    in.spaces=branch?DofDepthSpaces(At<float>(library,0x564),At<float>(library,0x568),At<float>(frame_globals,0x1B0),{At<float>(dof,0x1B4),At<float>(dof,0x1B8),At<float>(dof,0x1BC),At<float>(dof,0x1C0)}):
        NearDepthSpaces(At<float>(library,0x564),At<float>(library,0x568));
    in.spaces.source=At<int32_t>(library,0x43C);
    for(uint32_t o:{0xC38u,0xC3Cu,0xC40u,0xC44u}){const float mine=o==0xC38?in.spaces.c38:o==0xC3C?in.spaces.c3c:o==0xC40?in.spaces.c40:in.spaces.c44;
        char what[64]{};std::snprintf(what,sizeof(what)," S+0x%X",o);Expect(Bits(mine)==At<uint32_t>(library,o),name+what);}
    in.source_object=At<uint64_t>(library,0x430)!=0 || At<uint64_t>(library,0x50)!=0;in.source_scale=At<float>(library,0x448);in.source_bias=At<float>(library,0x44C);
    in.colour=(At<uint8_t>(library,0x454)?At<float>(library,0x6E8)*At<float>(library,0x69C):At<float>(library,0x698))/At<float>(library,0x698);
    const auto block=AlphaR(in);const std::string hex=ds3native::glare_capture::Text(rows[draw],"globals");
    for(uint32_t c=0;c<4;++c){Expect(Word(hex,616+c)==Bits(block.scale),name+" word "+std::to_string(616+c)+" (s)");
        Expect(Word(hex,620+c)==Bits(block.colour[c]),name+" word "+std::to_string(620+c)+" (colour)");
        Expect(Word(hex,744+c)==Bits(block.offset),name+" word "+std::to_string(744+c)+" (o)");}
    std::printf("%s: s %.9g o %.9g (near %g far %g focus %g)\n",name.c_str(),block.scale,block.offset,in.spaces.near_plane,in.spaces.far_plane,in.spaces.c48);
}
// LS-2: the shaft plan from the mask-draw state (record, glare object, surfaces; the glare shape from the bright-pass capture) against
// the captured draws from the mask on: technique, the words the programmes read, and the covering triangle's vertices, bit for bit.
void Plan(const std::string& dir){
    using ds3native::glare_capture::FileBytes;
    const std::string name=dir.substr(dir.find_last_of("/\\")+1);
    const auto records=FileBytes(dir+"/reference-shaft-mask-records.raw"),glare=FileBytes(dir+"/reference-shaft-mask-glare.raw"),
        surfaces=FileBytes(dir+"/reference-shaft-mask-surfaces.raw"),shape=FileBytes(dir+"/reference-glare-shape.raw");
    const bool state=records.size()>=0x50 && glare.size()>=0x400 && surfaces.size()>=4*0xC0 && shape.size()>=0x70;
    Expect(state,name+" shaft plan state captured");if(!state)return;
    const auto rows=ds3native::glare_capture::DrawRows(dir);size_t first=rows.size();
    for(size_t n=0;n<rows.size();++n)if(ds3native::glare_capture::Text(rows[n],"pass")=="tech_LightShaftMask_SM3"){first=n;break;}
    Expect(first<rows.size(),name+" mask draw captured");if(first==rows.size())return;
    ShaftInputs in;std::memcpy(in.record.data(),records.data(),0x50);
    in.base_width=At<uint32_t>(glare,0xB0);in.factor=At<float>(glare,0x114);in.luminance=At<float>(glare,0x118);in.fov=At<float>(glare,0x134);
    in.tan_half=At<float>(glare,0x138);in.exposure=At<float>(glare,0x148);in.shape_bloom=At<float>(shape,8);
    const auto surface=[&](size_t i){ShaftSurface s{At<uint32_t>(surfaces,i*0xC0+0x5C),At<uint32_t>(surfaces,i*0xC0+0x60)};
        Expect(s.width && s.height && At<uint32_t>(surfaces,i*0xC0+0x88)==Bits(1.0f/float(s.width)) && At<uint32_t>(surfaces,i*0xC0+0x8C)==Bits(1.0f/float(s.height)) &&
            At<uint32_t>(surfaces,i*0xC0+0x90)==0 && At<uint32_t>(surfaces,i*0xC0+0x94)==0,name+" surface "+std::to_string(i)+" scale 1/extent, bias 0");return s;};
    in.shaft=surface(0);in.output=surface(1);in.pingpong={surface(2),surface(3)};
    {const auto& row=rows[first];const auto at=row.find("\"size\":[");Expect(at!=std::string::npos,name+" mask source size");
        if(at!=std::string::npos){in.mask_source.width=uint32_t(std::stoul(row.substr(at+8)));in.mask_source.height=uint32_t(std::stoul(row.substr(row.find(',',at+8)+1)));}}
    Expect(Bits(in.tan_half)==Bits(std::tan(in.fov*0.5f)),name+" +0x138 = tan(+0x134 / 2)");
    const auto plan=BuildShaft(in);Expect(plan.refused.empty(),name+" plan served: "+plan.refused);
    for(size_t k=0;k<plan.draws.size();++k){const auto& d=plan.draws[k];const size_t n=first+k;const std::string at=name+" draw "+std::to_string(n);
        if(n>=rows.size()){Expect(false,at+" captured");break;}
        Expect(ds3native::glare_capture::Text(rows[n],"pass")==d.technique,at+" technique "+d.technique);
        const auto hex=ds3native::glare_capture::Text(rows[n],"globals");
        for(const auto& [w,v]:d.words){char what[96]{};std::snprintf(what,sizeof(what)," word %u (%.9g, captured %.9g)",w,v,std::bit_cast<float>(Word(hex,w)));
            Expect(Word(hex,w)==Bits(v),at+what);}
        char file[64]{};std::snprintf(file,sizeof(file),"/reference-draw-%02zu-vertices.raw",n);const auto vertices=FileBytes(dir+file);
        const size_t stride=4*d.vertices[0].size();Expect(vertices.size()>=3*stride,at+" vertices captured");if(vertices.size()<3*stride)continue;
        for(size_t v=0;v<3;++v)for(size_t f=0;f<d.vertices[v].size();++f){const uint32_t captured=At<uint32_t>(vertices,v*stride+4*f);char what[96]{};
            std::snprintf(what,sizeof(what)," vertex %zu float %zu (%.9g, captured %.9g)",v,f,d.vertices[v][f],std::bit_cast<float>(captured));Expect(captured==Bits(d.vertices[v][f]),at+what);}}
    if(first+plan.draws.size()<rows.size())std::printf("%s: after the plan, %s\n",name.c_str(),ds3native::glare_capture::Text(rows[first+plan.draws.size()],"pass").c_str());
    std::printf("%s: shaft plan %zu draws, %d passes, last %d taps\n",name.c_str(),plan.draws.size(),plan.passes,plan.last_taps);
    // LS-3: the glare plan from the same frame's glare state with the shafts appended: the whole draw sequence from the first source
    // downsample to the 2x draw equals the captured techniques, and every appended draw's words (MadT2's included) and the 2x
    // draw's input change are checked.
    GlareData data;const auto glare_state=ds3native::glare_capture::Load(dir);
    if(!ds3native::glare_capture::Data(data) || !glare_state.Ok()){Expect(false,name+" glare data and state (DS3_EXE, reference-glare-*.raw)");return;}
    const auto glare_in=ds3native::glare_capture::Inputs(glare_state);const bool standard3=glare_in.shape==3;
    auto merged=BuildGlare(data,glare_in);Expect(merged.refused.empty(),name+" glare plan served");
    const size_t glare_draws=merged.draws.size();std::string refused;
    // I-0138 tranche 8i: a capture of GlareShape 3 with its shafts (i138-t8i-oracle-shafts-*): the shafts go before the layers'
    // composite (AppendShafts), so the appended draws start there; on shape 1 they start where the 2x draw was.
    size_t appended=glare_draws-1;if(standard3)for(size_t k=0;k<merged.draws.size();++k)if(merged.draws[k].kind==GlareKind::Layers)appended=k;
    Expect(!standard3 || appended+2==glare_draws,name+" shape 3: the layers' composite before the 2x draw");
    Expect(AppendShafts(merged,in,plan,refused),name+" shafts appended: "+refused);
    size_t bright=rows.size(),sources=0;for(size_t n=0;n<rows.size();++n)if(ds3native::glare_capture::Text(rows[n],"pass").rfind("tech_BrightPass",0)==0){bright=n;break;}
    while(sources<merged.draws.size() && merged.draws[sources].kind==GlareKind::Downsample)++sources;
    Expect(bright<rows.size() && bright>=sources,name+" bright pass captured");if(bright==rows.size() || bright<sources)return;
    const size_t start=bright-sources;
    for(size_t k=0;k<merged.draws.size();++k){const auto& g=merged.draws[k];const size_t n=start+k;const std::string at=name+" merged draw "+std::to_string(n);
        if(n>=rows.size()){Expect(false,at+" captured");break;}
        Expect(ds3native::glare_capture::Text(rows[n],"pass")==g.technique,at+" technique "+g.technique);
        if(k>=appended && k+1<merged.draws.size()){const auto hex=ds3native::glare_capture::Text(rows[n],"globals");   // the appended draws (shape 3: and the widened composite)
            Expect(!g.unplaced,at+" no word without a slot in our block");   // I-0138 tranche 8b: the library's words read back through our block
            for(uint32_t w=0;w<ds3native::post_test::library_block_bytes/4;++w){const uint32_t mine=ds3native::post_test::LibraryWord(g,w);if(!mine)continue;
                char what[96]{};std::snprintf(what,sizeof(what)," word %u",w);Expect(Word(hex,w)==mine,at+what);}}}
    if(!standard3)Expect(merged.draws.back().inputs[0]==int8_t(merged.surfaces.size()-1),name+" the 2x draw reads MadT2");
    // Shape 3: that the 2x draw reads the layers' composite is not asserted here: AppendShafts keeps the plan's last draw reading the last
    // layer's target or refuses (its "glare plan shape" check), and the appended draws' techniques were compared with the capture above.
    // I-0138 tranche 8g: the same frame's state as GlareShape 3 (its record from the executable's tables): the shafts become the layers'
    // composite's last layer (YebisGlareComposite_1417511c0 section 5 of t8d-readings/glare-shape3-passes.md): the same shaft draws as
    // shape 1's, just before the layers' composite, which reads their output as one more layer of weight 1; no MadT2, the 2x draw
    // still reads the layers' composite. The captures that carry the shafts' state with shape 1 (the runs above) have no shape-3 plan to
    // compare with: this block builds the shape-3 plan from their state and checks its structure only, not the texels; a capture with
    // shape 3 and its shafts (i138-t8i-oracle-shafts-*) goes through the first branch above, against the captured draws' techniques and words.
    if(!standard3){auto in3=glare_in;in3.shape=3;in3.record=ShapeRecord(data);
        auto standard=BuildGlare(data,in3);Expect(standard.refused.empty(),name+" shape 3 plan served: "+standard.refused);
        if(standard.refused.empty()){size_t at=standard.draws.size();for(size_t k=0;k<standard.draws.size();++k)if(standard.draws[k].kind==GlareKind::Layers)at=k;
            Expect(at+2==standard.draws.size(),name+" shape 3: the layers' composite before the 2x draw");
            if(at+2==standard.draws.size()){const auto before=standard.draws[at];size_t n{};while(n<8 && before.inputs[n]!=-2)++n;
                const size_t surface_count=standard.surfaces.size();std::string why;
                Expect(AppendShafts(standard,in,plan,why),name+" shape 3 shafts appended: "+why);
                const size_t shaft_draws=merged.draws.size()-glare_draws-1;   // shape 1's appended draws but its MadT2
                Expect(standard.draws.size()==at+2+shaft_draws && standard.shafts==1 && standard.surfaces.size()==surface_count+4,name+" shape 3: the shaft draws and surfaces, no MadT2");
                if(standard.draws.size()==at+2+shaft_draws){
                    for(size_t k=0;k<shaft_draws;++k){const auto& a=standard.draws[at+k];const auto& b=merged.draws[glare_draws-1+k];
                        Expect(a.technique==b.technique && a.block==b.block && a.vertices==b.vertices,name+" shape 3: shaft draw "+std::to_string(k)+" as shape 1's");}
                    const auto& layers=standard.draws[at+shaft_draws];const int8_t output=standard.draws[at+shaft_draws-1].target;
                    bool kept=layers.kind==GlareKind::Layers && layers.technique=="tech_MadT"+std::to_string(n+1) && layers.target==before.target && layers.inputs[n]==output && layers.point[n];
                    for(size_t i=0;i<n;++i)kept=kept && layers.inputs[i]==before.inputs[i];
                    for(uint32_t w=0;w<=n;++w)for(uint32_t c=0;c<4;++c){float v{};const uint32_t word=616+4*w+c;const int slot=glare_block_slots[word/4];
                        if(slot>=0)std::memcpy(&v,layers.block.data()+16*size_t(slot)+4*(word%4),4);kept=kept && slot>=0 && v==1.0f;}
                    Expect(kept,name+" shape 3: MadT"+std::to_string(n+1)+" of the layers and the shafts' output, every weight 1");
                    Expect(standard.draws.back().inputs[0]==before.target,name+" shape 3: the 2x draw reads the layers' composite");}}}}
    std::printf("%s: merged plan %zu draws (%zu glare + %zu shaft), %zu surfaces\n",name.c_str(),merged.draws.size(),glare_draws,merged.draws.size()-glare_draws,merged.surfaces.size());
}
// LS-2b: the record from the library's slot (E69A0's output, reference-driver-slots.raw) and the library's depth words at the mask
// draw, against the record CRenderGlare draws with (reference-shaft-mask-records.raw).
void Driver(const std::string& dir){
    using ds3native::glare_capture::FileBytes;
    const std::string name=dir.substr(dir.find_last_of("/\\")+1);
    const auto slots=FileBytes(dir+"/reference-driver-slots.raw"),records=FileBytes(dir+"/reference-shaft-mask-records.raw"),library=FileBytes(dir+"/reference-shaft-mask-library.raw");
    const bool state=slots.size()>=0x58 && records.size()>=0x50 && library.size()>=0xD00;Expect(state,name+" driver state captured");if(!state)return;
    std::array<float,22> slot{};std::memcpy(slot.data(),slots.data(),0x58);
    DepthSpaces s;s.near_plane=At<float>(library,0x564);s.far_plane=At<float>(library,0x568);s.c38=At<float>(library,0xC38);s.c3c=At<float>(library,0xC3C);
    s.c40=At<float>(library,0xC40);s.c44=At<float>(library,0xC44);s.c48=At<float>(library,0xC48);s.source=At<int32_t>(library,0x43C);
    // E69A0 -> the slot: the camera record, the globals and the post object captured before the call.
    {const auto camera=FileBytes(dir+"/reference-driver-camera.raw"),globals=FileBytes(dir+"/reference-driver-globals.raw"),post=FileBytes(dir+"/reference-driver-post.raw");
        const bool inputs=camera.size()>=0x80 && globals.size()>=0x370 && post.size()>=0x2210;Expect(inputs,name+" driver inputs captured");
        ShaftDriverInputs d;
        const bool projected=inputs && ds3light::CameraProjection4B070(camera.data(),camera.size(),d.projection.data());
        if(inputs)Expect(projected,name+" projection (14004B070; DS3_EXE)");
        if(projected){
            std::memcpy(d.pose.data(),camera.data(),64);std::memcpy(d.globals.data(),globals.data(),0x370);
            std::memcpy(d.colour.data(),post.data()+0x21E0,16);std::memcpy(d.colour2.data(),post.data()+0x2200,16);
            // The native glare words from the camera's field of view: +0x138 = tan(fov / 2), +0x134 = atan(+0x138) x 2 (AppendNativeShafts).
            {const auto glare_words=FileBytes(dir+"/reference-shaft-mask-glare.raw");float fov{};std::memcpy(&fov,camera.data()+0x50,4);
                const float tan_half=std::tan(fov*0.5f),fov2=std::atan(tan_half)*2.0f;
                Expect(glare_words.size()>=0x140 && Bits(tan_half)==At<uint32_t>(glare_words,0x138) && Bits(fov2)==At<uint32_t>(glare_words,0x134),
                    name+" glare +0x134/+0x138 from the camera's field of view");}
            const auto mine=DriverSlot(d);
            for(uint32_t w=0;w<22;++w){char what[96]{};std::snprintf(what,sizeof(what)," slot word %u (%.9g, captured %.9g)",w,mine[w],At<float>(slots,4*w));
                Expect(Bits(mine[w])==At<uint32_t>(slots,4*w),name+what);}
            std::printf("%s: slot from E69A0: sun (%.9g, %.9g) depth %.9g, colour (%.9g, %.9g, %.9g, %.9g)\n",name.c_str(),mine[17],mine[18],mine[19],mine[0],mine[1],mine[2],mine[3]);}}
    const auto record=ShaftRecordFromSlot(slot,s);
    // I-0138 tranche 8i (the step review's M1, c7e0ac515's M1, 0cbb83ac9's L2): the record's depth word (19) is the slot's depth through
    // the depth spaces, whose c48 is the focus in the depth-of-field branch; the library converts it in FUN_1416EA000 before
    // FUN_1416E9050 stores this frame's words (READ in tranche 8d), so with this frame's near and far and the previous frame's c48
    // (RecordDepthSpaces). The mask library's words are this frame's and no file holds the previous frame's focus. So when the branch is
    // on and the focus moved between E69A0 (reference-driver-globals.raw +0x1B0) and the bright pass (reference-glare-globals.raw +0x1B0),
    // word 19 must be the conversion with a focus on the side the focus came from: from this frame's to two of its steps back
    // (now + 2 (now - bright pass); the bright pass's block holding the next frame's focus is an INTERPRETATION); whether this frame's own
    // focus gives it is reported. i138-t8i-oracle-shafts-1250 (OBSERVED, this test's output): the captured 0.9981461 is given by every
    // focus from 3.89206 (this frame's 3.89189 gives 0.9981499: the cancelled 1 - c48 / near leaves a coarse grid), the previous frame's
    // extrapolated 3.90176 inside. Otherwise word 19 is required equal.
    const auto driver_globals=FileBytes(dir+"/reference-driver-globals.raw"),bright_globals=FileBytes(dir+"/reference-glare-globals.raw");
    const bool branch=At<uint8_t>(library,0xC33)==1;
    const bool focus_moved=branch && driver_globals.size()>=0x370 && bright_globals.size()>=0x370 && At<uint32_t>(driver_globals,0x1B0)!=At<uint32_t>(bright_globals,0x1B0);
    for(uint32_t w=0;w<20;++w){char what[96]{};std::snprintf(what,sizeof(what)," record word %u (%.9g, captured %.9g)",w,record[w],At<float>(records,4*w));
        if(w==19 && focus_moved){const uint32_t captured=At<uint32_t>(records,4*19);
            const auto with_focus=[&](float focus){DepthSpaces t=s;t.c48=focus;return Bits(ShaftRecordFromSlot(slot,t)[19]);};
            const float now=At<float>(driver_globals,0x1B0),next=At<float>(bright_globals,0x1B0),back=now+2.0f*(now-next);
            const float lo=std::min(now,back),hi=std::max(now,back);float first{},last{};bool found=false;
            const bool finite=std::isfinite(lo) && std::isfinite(hi);Expect(finite,name+what+": finite focuses");
            // The step review of 050f3c888 (L5): the scan bounded, the band refused when positive focuses span more than 2^22 floats
            // (the one band scanned here, i138-t8i-oracle-shafts-1250's, about 82 800).
            const bool bounded=finite && lo>0.0f && Bits(hi)-Bits(lo)<=(1u<<22);Expect(!finite || bounded,name+what+": a positive band of at most 2^22 floats");
            if(bounded)for(float c=lo;c<=hi;c=std::nextafter(c,INFINITY))if(with_focus(c)==captured){if(!found)first=c;found=true;last=c;}
            Expect(found,name+what+": a focus from this frame's to two of its steps back (the previous frame's words)");
            // The step review of 050f3c888 (M1): where the grid tells this frame's focus from the band's far end, this frame's own
            // conversion must not give the word (the previous frame's words, not this frame's).
            if(finite && with_focus(now)!=with_focus(back))Expect(with_focus(now)!=captured,name+what+": not this frame's own conversion");
            if(found)std::printf("%s:%s: the conversion with a focus in [%.6g, %.6g] %s (this frame %.6g, its own conversion %s; one step back %.6g)\n",
                name.c_str(),what,first,last,last==hi?", the band clipped at its end":"",now,with_focus(now)==captured?"equal":"not equal",now+(now-next));
            continue;}
        Expect(Bits(record[w])==At<uint32_t>(records,4*w),name+what);}
}
}
int main(int argc,char** argv){
    // FUN_1416F4AF0 by hand: device depth to space 1 and back, the inverse, the scale and the identity.
    DepthSpaces d;d.near_plane=0.5f;d.far_plane=100.0f;d.c48=2.0f;
    const auto f=DepthConversion(d,0,1),r=DepthConversion(d,1,0),id=DepthConversion(d,2,2),sc=DepthConversion(d,1,1,2.0f,1.0f);
    Expect(f[0]==((99.5f*2.0f)/(100.0f*0.5f)) && f[1]==1.0f-4.0f,"0 -> 1");
    Expect(r[0]==1.0f/f[0] && r[1]==-(f[1]/f[0]),"1 -> 0 is the inverse");
    Expect(id[0]==1.0f && id[1]==0.0f && id[2]==1.0f,"identity");
    Expect(sc[0]==0.5f && sc[1]==-0.5f,"scale and bias");
    d.source=1;Expect(DepthConversion(d,6,0)[0]==r[0],"6 names the source space");
    // I-0138 tranche 8d: FUN_1416E9050 outside the depth-of-field branch (c38 1, c3c 0, c40 1, c44 -0, c48 the near plane) and
    // Alpha_R's block in it: s = (far - near) near / (far near), o = 1 - near / near = 0 (device depth 0 at the near plane, alpha 0).
    {const auto n=NearDepthSpaces(0.08f,10000.0f);
        Expect(Bits(n.c38)==Bits(1.0f) && Bits(n.c3c)==0 && Bits(n.c40)==Bits(1.0f) && Bits(n.c44)==0x80000000u && Bits(n.c48)==Bits(0.08f) && n.source==0,"near spaces' words");
        AlphaRInputs a;a.spaces=n;a.source_bias=-0.0f;const auto b=AlphaR(a);
        Expect(Bits(b.scale)==Bits(((10000.0f-0.08f)*0.08f)/(10000.0f*0.08f)) && b.offset==0.0f,"Alpha_R outside the branch");}
    // I-0138 tranche 8i (0cbb83ac9's step review, M1): the record's spaces keep this frame's near, far, source and space-3 factor and
    // take the previous frame's conversion words; without a previous frame, this frame's.
    {DepthSpaces now=DofDepthSpaces(0.08f,1000.0f,3.9f,{1.0f,0.0f,1.0f,0.0f}),before=DofDepthSpaces(0.1f,10000.0f,4.0f,{2.0f,0.5f,1.0f,0.0f});now.source=1;before.source=2;now.space3=3.0f;
        const auto rs=RecordDepthSpaces(now,&before),o=RecordDepthSpaces(now,nullptr);
        Expect(rs.near_plane==now.near_plane && rs.far_plane==now.far_plane && rs.source==1 && rs.space3==3.0f && rs.c48==before.c48 && rs.c38==before.c38 && rs.c3c==before.c3c
            && rs.c40==before.c40 && rs.c44==before.c44,"the record's spaces: this frame's planes, the previous frame's words");
        Expect(o.c48==now.c48 && o.far_plane==now.far_plane && o.c38==now.c38,"the record's spaces without a previous frame");
        // The step review of 050f3c888 (L2): the previous spaces apply only when stored at the serial just before this one.
        Expect(PreviousSpacesApply(41,42) && !PreviousSpacesApply(~0ull,0) && !PreviousSpacesApply(40,42) && !PreviousSpacesApply(42,42) && !PreviousSpacesApply(43,42)
            && !PreviousSpacesApply(~0ull-1,~0ull-1) && PreviousSpacesApply(~0ull-1,~0ull),"the previous spaces at the serial just before only");}
    // I-0138 tranche 8i: a GlareShape 3 capture with its shafts (« -shafts- »): Alpha_R's block, the shaft plan and the merged chain, and
    // the driver's slot, the cutscene camera's glare words and the mask record (Driver, as for the shaftdriver runs).
    for(int i=1;i<argc;++i){const std::string dir=argv[i];if(dir.size()>4 && dir.substr(dir.size()-4)==".exe"){_putenv_s("DS3_EXE",argv[i]);continue;}if(dir.find("shaftplan")!=std::string::npos)Plan(dir);else if(dir.find("shaftdriver")!=std::string::npos){Plan(dir);Driver(dir);}
        else if(dir.find("-shafts-")!=std::string::npos || dir.find("-sun-")!=std::string::npos){Zone(dir);Plan(dir);Driver(dir);}else Zone(dir);}
    std::printf("%s yebis shafts: LS-1 Alpha_R block, LS-2 shaft plan, %d checks, %d failures\n",failures?"FAIL":"PASS",checks,failures);
    return failures?1:0;
}
