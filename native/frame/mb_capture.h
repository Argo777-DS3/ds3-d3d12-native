// W10 MB-1/MB-2 tests: the motion blur's inputs from a Reference capture of the library at the Alpha_R draw of the captured frame
// (reference-shaft-alpha-library.raw, 0xD00 bytes; the frame's motion blur runs after it). Header-only, for the offline tests.
#pragma once
#include "glare_capture.h"
#include "../world/yebis_motionblur.h"
#include "../world/yebis_shafts.h"

namespace ds3native::mb_capture {
using glare_capture::At;
// The library's fields the stage reads (S+0x328 .. S+0xC48), the scene 1280x720 and the internal velocity 640x360 of the captures.
inline yebis::MotionBlurInputs Inputs(const std::vector<uint8_t>& s){
    yebis::MotionBlurInputs in;in.width=1280;in.height=720;in.velocity_width=640;in.velocity_height=360;
    auto& c=in.camera;c.near_plane=At<float>(s,0x564);c.far_plane=At<float>(s,0x568);c.tan_half=At<float>(s,0x570);c.aspect=At<float>(s,0x328);
    c.shift={At<float>(s,0x574),At<float>(s,0x578)};c.previous_tan_half=At<float>(s,0x588);c.previous_shift={At<float>(s,0x58C),At<float>(s,0x590)};
    for(size_t i=0;i<16;++i){c.current[i]=At<float>(s,0x594+4*i);c.previous[i]=At<float>(s,0x614+4*i);}
    in.time_ratio=At<float>(s,0x9D0);in.max_length=At<float>(s,0x9D4);in.base_samples=At<int32_t>(s,0x9D8);in.max_recurrences=At<int32_t>(s,0x9DC);
    in.bidirectional=At<uint8_t>(s,0x9CC)!=0;in.interleave=At<float>(s,0x9E0);in.capability=At<int32_t>(s,0x340);
    in.similarity=At<float>(s,0xA00);in.depth_threshold=At<float>(s,0xA04);in.depth_threshold_far=At<float>(s,0xA08);
    in.velocity_scale={At<float>(s,0xA24),At<float>(s,0xA28)};
    yebis::DepthSpaces d;d.near_plane=c.near_plane;d.far_plane=c.far_plane;d.c38=At<float>(s,0xC38);d.c3c=At<float>(s,0xC3C);d.c40=At<float>(s,0xC40);
    d.c44=At<float>(s,0xC44);d.c48=At<float>(s,0xC48);d.source=At<int32_t>(s,0x43C);
    const bool object=At<uint64_t>(s,0x430)!=0 || At<uint64_t>(s,0x50)!=0;
    const float scale=object?At<float>(s,0x440):1.0f,bias=object?At<float>(s,0x444):0.0f;
    in.object_decode=yebis::DepthConversion(d,6,0,scale,bias);in.scene_decode=yebis::DepthConversion(d,2,0,scale,bias);
    return in;}
}
