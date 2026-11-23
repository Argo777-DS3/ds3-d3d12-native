// W10 DF-2/DF-3 tests: the depth of field's native frame from a Reference run's captured inputs (the camera record E69A0 takes,
// reference-driver-camera.raw; the globals block after it, reference-glare-globals.raw), with the executable's words, and the DoF
// table its frame draws with (CustomDofLutCopy's g_Coc in both rows). Header-only, for the offline tests.
#pragma once
#include "glare_capture.h"
#include "../world/yebis_dof.h"
#include "../world/light_producers.h"

namespace ds3native::dof_capture {
struct Data {yebis::DofData draw;yebis::DofSetupData setup;yebis::DofConstants k;};
inline bool Load(Data& d){
    const auto read=[](uint32_t rva,void* out,size_t bytes){return ds3light::Rdata(rva,out,uint32_t(bytes));};
    auto& k=d.k;
    return yebis::LoadDofDrawData(d.draw,read) && yebis::LoadDofSetupData(d.setup,read) &&
        read(0x3D7EE10,&k.one,4) && read(0x3D7EDB4,&k.half,4) && read(0x3D7EE7C,&k.two,4) && read(0x3D7EEC0,&k.pi,4) && read(0x3D7EFE8,&k.degrees,4) &&
        read(0x3D7ED10,&k.milli,4) && read(0x3D7F088,&k.full_hd,4) && read(0x3D7EF48,&k.quality_steps,4);}
struct Frame {yebis::DofSetup setup;yebis::DofLens lens;yebis::DofFrame frame;std::vector<float> table;};
// The table: ds3light::CustomDofTable on the frame's lens and the globals 144962244..26C, its 2048 values in each of the two rows.
inline std::vector<float> Table(const yebis::DofLens& lens,const std::vector<uint8_t>& globals,uint32_t width,uint32_t height){
    ds3light::CustomDofInput t;if(!ds3light::CustomDofConstants(t) || width!=2048)return {};
    t.width=width;t.height=height;t.near_plane=lens.near_plane;t.far_plane=lens.far_plane;
    t.lens={lens.focus,lens.words[0],lens.words[1],1.0f/lens.words[0],-(lens.words[1]/lens.words[0])};t.exposure=lens.enable?lens.scale:0.0f;
    for(uint32_t w=0;w<11;++w)t.dof[w]=glare_capture::At<uint32_t>(globals,0x2D4+4*w);
    std::vector<uint8_t> cb(0x2010);ds3light::CustomDofTable(t,cb.data());
    std::vector<float> out(size_t(width)*height);for(uint32_t y=0;y<height;++y)std::memcpy(out.data()+size_t(y)*width,cb.data()+0x10,size_t(width)*4);
    return out;}
// I-0138 tranche 4: with an extent given, the same camera and globals at that extent (the aspect then the extent's, as a
// frame without an explicit aspect), for the comparisons away from the captures.
inline bool Native(const Data& d,const std::string& dir,Frame& out,std::string& why,uint32_t at_width=0,uint32_t at_height=0){
    using glare_capture::At;
    const auto g=glare_capture::FileBytes(dir+"/reference-glare-globals.raw"),c=glare_capture::FileBytes(dir+"/reference-driver-camera.raw");
    if(g.size()<0x370 || c.size()<0x80){why="camera or globals not captured";return false;}
    const bool sized=at_width && at_height;const uint32_t width=sized?at_width:At<uint32_t>(c,0x5C),height=sized?at_height:At<uint32_t>(c,0x60);
    out.setup=yebis::BuildDofSetup(d.setup,width,height,At<int>(g,0x4C),7);if(!out.setup.refused.empty()){why=out.setup.refused;return false;}
    yebis::DofLensInputs in;in.fov=At<float>(c,0x50);in.near_plane=At<float>(c,0x54);in.far_plane=At<float>(c,0x58);in.aspect=sized?0.0f:At<float>(c,0x64);
    in.extent_width=width;in.extent_height=height;in.quality=At<int>(g,0x4C);in.focus=At<float>(g,0x1B0);in.f_number=At<float>(g,0x1B4);in.p6d0=At<float>(g,0x1B8);
    in.angle=At<float>(g,0x1BC);in.diagonal=At<float>(g,0x1C0);in.enable=At<int>(g,0x1AC)!=0;
    out.lens=yebis::DofLensState(d.k,in);
    out.frame=yebis::NativeDofFrame(out.setup,out.lens,At<int>(g,0x1D0),At<int>(g,0x1D4),At<int>(g,0x240)==2?2u:0xFFFFFFFFu);
    out.table=Table(out.lens,g,out.frame.table_width,out.frame.table_height);if(out.table.empty()){why="DoF table";return false;}
    return true;}
}
