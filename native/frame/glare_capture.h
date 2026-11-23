// W10 GL-2/GL-3/GL-4 tests: the glare plan's inputs from the executable (DS3_EXE, Rdata) and from a Reference capture of the library's
// state at the bright-pass draw (CaptureYebisGlareState: reference-glare-globals.raw, the globals block 144961F70; -object.raw,
// CRenderGlare; -shape.raw, its shape record), and the captured draws of that frame (yebis-draws-capture.flag). Header-only, for
// the offline tests.
#pragma once
#include "../world/yebis_glare.h"
#include "../world/yebis_tonemap.h"
#include "../world/yebis_shafts.h"
#include "../world/light_producers.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace ds3native::glare_capture {
inline std::vector<uint8_t> FileBytes(const std::string& path){std::ifstream file(path,std::ios::binary);if(!file)return {};
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());}
template<class T> T At(const std::vector<uint8_t>& b,size_t offset){T v{};if(offset+sizeof(T)<=b.size())std::memcpy(&v,b.data()+offset,sizeof(T));return v;}
// The executable's words of yebis::GlareData (the glare aspect 1.5 is CPostEffect's init-flag value, not a data word).
inline bool Data(yebis::GlareData& d){
    bool ok=ds3light::Rdata(0x2899374,&d.luminance_scale,4) && ds3light::Rdata(0x3D7EE90,&d.radius_c,4) && ds3light::Rdata(0x3D7EDE8,&d.level_radius,4) &&
        ds3light::Rdata(0x2B63170,&d.wavelength[0],4) && ds3light::Rdata(0x2B6316C,&d.wavelength[1],4) && ds3light::Rdata(0x2B63168,&d.wavelength[2],4) &&
        ds3light::Rdata(0x3D7EEFC,&d.two_pi,4) && ds3light::Rdata(0x27D5950,&d.sigma_floor,4) && ds3light::Rdata(0x2B4CF88,&d.sigma_replacement,4) &&
        ds3light::Rdata(0x3D7EE00,&d.base_ceiling,4) && ds3light::Rdata(0x2B55250,&d.height_rounding,4) && ds3light::Rdata(0x2B60CA0,&d.up_offset,4);
    for(uint32_t q=0;q<8 && ok;++q)ok=ds3light::Rdata(0x469B4D0+8*q,&d.quality_scale[q],4);
    // I-0138 tranche 8f: GlareShape 3's table entry (14469BD70 + 3 x 0x38, d0 to d10 after the name) and its preset's row (14469BC90 +
    // d5 x 0x20, after the name).
    ok=ok && ds3light::Rdata(0x469BD70+3*0x38+8,d.shape3.data(),4*d.shape3.size()) && d.shape3[5]<7 &&
        ds3light::Rdata(0x469BC90+d.shape3[5]*0x20+8,d.preset3.data(),4*d.preset3.size());
    d.glare_aspect=1.5f;return ok;}
struct State {std::vector<uint8_t> globals,object,shape;bool Ok() const {return globals.size()>=0x370 && object.size()>=0x400 && shape.size()>=0x70;}};
inline State Load(const std::string& dir){return {FileBytes(dir+"/reference-glare-globals.raw"),FileBytes(dir+"/reference-glare-object.raw"),FileBytes(dir+"/reference-glare-shape.raw")};}
// The inputs from the globals block (offsets from 144961F70), F and the shape record from the glare object; the bright pass's
// vignette (CRenderGlare +0x160) with the tone map's coordinates from Vignette Power (144962040) and TM-3's data words.
inline yebis::GlareInputs Inputs(const State& s){
    yebis::GlareInputs in;in.width=At<uint32_t>(s.object,0xA8);in.height=At<uint32_t>(s.object,0xAC);
    in.quality=At<int32_t>(s.globals,0x48);in.exposure=At<float>(s.globals,0x98);in.luminance=At<float>(s.globals,0xDC);in.threshold=At<float>(s.globals,0xE0);
    in.levels=At<int32_t>(s.globals,0xE4);in.radius_scale=At<float>(s.globals,0xF8);in.composite_gamma=At<float>(s.globals,0xFC);in.bloom_gamma=At<float>(s.object,0x1B0);
    in.remap=At<float>(s.globals,0x244);in.sampling_threshold=At<float>(s.globals,0x304);in.factor=At<float>(s.object,0x114);
    in.shape_bloom=At<float>(s.shape,8);in.shape_scale=At<float>(s.shape,0xC);in.shape_tint=At<float>(s.shape,0x10);
    in.vignette=At<uint8_t>(s.object,0x160)!=0;
    // I-0138 tranche 8f: the shape (globals +0xD8, GlareShape) and, for a shape other than 1, its record as captured (CRenderGlare +0x150,
    // the offsets of yebis::GlareShapeRecord); the streaks' and the ghost's globals; the object's EV (+0x15C), base level (+0x2F8),
    // tan_half (+0x138) and frame time (+0x158), whose native sources the caller gives.
    in.shape=At<int32_t>(s.globals,0xD8);
    {auto& r=in.record;const auto& b=s.shape;r.bloom=At<float>(b,0x08);r.scale=At<float>(b,0x0C);r.tint=At<float>(b,0x10);r.tint_levels=At<int32_t>(b,0x14);
        r.ghost=At<float>(b,0x18);r.ghost_scale=At<float>(b,0x1C);r.ghost_size=At<float>(b,0x20);r.lens_flare=At<uint8_t>(b,0x24)!=0;
        r.streak=At<float>(b,0x28);r.angle=At<float>(b,0x2C);r.colour=At<float>(b,0x30);r.colour_flag=At<uint8_t>(b,0x34)!=0;
        r.after_time=At<float>(b,0x38);r.after_weight=At<float>(b,0x3C);r.arms=At<int32_t>(b,0x50);r.arm_a=At<float>(b,0x54);
        r.arm_length=At<float>(b,0x58);r.odd_length=At<float>(b,0x5C);r.preset_flag=At<uint8_t>(b,0x60)!=0;}
    in.star_threshold=At<float>(s.globals,0xEC);in.star_fov=At<float>(s.globals,0xF0);in.star_softness=At<float>(s.globals,0xF4);
    in.ghost_distortion=At<float>(s.globals,0x100);
    in.tan_half=At<float>(s.object,0x138);in.ev=At<float>(s.object,0x15C);in.base_level=At<int32_t>(s.object,0x2F8);in.frame_time=At<float>(s.object,0x158);
    if(in.vignette){yebis::VignetteData vd;const uint32_t tan_bits=0x3ED413CDu,k_bits=0x3FAAAAA8u,m_bits=0x3F2AAAE3u;std::memcpy(&vd.tan_eighth_pi,&tan_bits,4);
        std::memcpy(&vd.k,&k_bits,4);std::memcpy(&vd.m,&m_bits,4);
        in.vignette_uv=yebis::VignetteCoordinates(yebis::VignetteRect(vd,At<float>(s.globals,0xD0),float(in.width)/float(in.height)));}
    return in;}
// The captured draw rows of the frame the draws capture dumped (the reference_yebis_draw row before each capture row), by index.
inline std::vector<std::string> DrawRows(const std::string& dir){std::ifstream trace(dir+"/startup.jsonl");std::vector<std::string> rows;std::string line,last;
    while(std::getline(trace,line)){if(line.find("\"event\":\"reference_yebis_draw\"")!=std::string::npos)last=line;
        else if(line.find("\"event\":\"reference_yebis_draw_capture\"")!=std::string::npos){const auto n=std::stoul(line.substr(line.find("\"draw\":")+7));
            if(n>=rows.size())rows.resize(n+1);rows[n]=last;}}
    return rows;}
inline std::string Text(const std::string& row,const std::string& name){const auto key=row.find("\""+name+"\":\"");if(key==std::string::npos)return {};
    const auto from=key+name.size()+4;return row.substr(from,row.find('"',from)-from);}
// The original draw index of each plan draw: the bright pass by its technique, the source downsamples just before it, the draws in
// order up to the composite, then the draw into the output surface after it (the light shafts and their MadT2, or a MadT1 copy,
// come between); empty when the capture does not hold them.
inline std::vector<size_t> OriginalDraws(const yebis::GlarePlan& plan,const std::vector<std::string>& rows){
    size_t bright=rows.size();for(size_t n=0;n<rows.size();++n)if(Text(rows[n],"pass").rfind("tech_BrightPass",0)==0){bright=n;break;}
    size_t sources=0;while(sources<plan.draws.size() && plan.draws[sources].kind==yebis::GlareKind::Downsample)++sources;
    if(bright==rows.size() || bright<sources || plan.output<0 || plan.draws.empty())return {};
    std::vector<size_t> original;for(size_t k=0;k+1<plan.draws.size();++k)original.push_back(bright-sources+k);
    const auto& out=plan.surfaces[size_t(plan.output)];char final_rt[64]{};std::snprintf(final_rt,sizeof(final_rt),"\"rt0\":[26,6,%u,%u",out.width,out.height);
    for(size_t n=original.back()+1;n<rows.size();++n)if(rows[n].find(final_rt)!=std::string::npos){original.push_back(n);return original;}
    return {};}
// LS-2/LS-3: the shaft plan's inputs from a run's state at the mask draw (CaptureYebisShaftState, Reference1664 on: the record, the
// glare object, the surfaces; the glare shape from the bright-pass capture; the mask source's extent from the mask draw's row);
// `first` is the mask draw's index. False when a file or the mask draw is missing.
inline bool ShaftInputsOf(const std::string& dir,const std::vector<std::string>& rows,yebis::ShaftInputs& in,size_t& first){
    const auto records=FileBytes(dir+"/reference-shaft-mask-records.raw"),glare=FileBytes(dir+"/reference-shaft-mask-glare.raw"),
        surfaces=FileBytes(dir+"/reference-shaft-mask-surfaces.raw"),shape=FileBytes(dir+"/reference-glare-shape.raw");
    if(records.size()<0x50 || glare.size()<0x400 || surfaces.size()<4*0xC0 || shape.size()<0x70)return false;
    first=rows.size();for(size_t n=0;n<rows.size();++n)if(Text(rows[n],"pass")=="tech_LightShaftMask_SM3"){first=n;break;}
    if(first==rows.size())return false;
    std::memcpy(in.record.data(),records.data(),0x50);
    in.base_width=At<uint32_t>(glare,0xB0);in.factor=At<float>(glare,0x114);in.luminance=At<float>(glare,0x118);in.fov=At<float>(glare,0x134);
    in.tan_half=At<float>(glare,0x138);in.exposure=At<float>(glare,0x148);in.shape_bloom=At<float>(shape,8);
    const auto surface=[&](size_t i){return yebis::ShaftSurface{At<uint32_t>(surfaces,i*0xC0+0x5C),At<uint32_t>(surfaces,i*0xC0+0x60)};};
    in.shaft=surface(0);in.output=surface(1);in.pingpong={surface(2),surface(3)};
    const auto& row=rows[first];const auto at=row.find("\"size\":[");if(at==std::string::npos)return false;
    in.mask_source.width=uint32_t(std::stoul(row.substr(at+8)));in.mask_source.height=uint32_t(std::stoul(row.substr(row.find(',',at+8)+1)));
    return true;}
}
