#pragma once
#include <array>
#include <cstdint>
#include <vector>

namespace ds3native {
// W10 YB-5 (YEBIS-WAVE1-READY 2, 4.7): the three GX programmes around the YEBIS library, the original's DXBC from the
// gxposteffect bundle (loaded by the engine, resolved by name like the glow and AO programmes), each checked against its pin
// (bytes and SHA-256 of the whole container, verified on the I-0063 programme cache): the scene copy before the library
// (PointSampleColorClamp1: max(t0.SampleLevel(ClampPoint), 0)), the colour-grading LUT copy (LutCopy2DTo3DCs, [4,16,1]: a
// 16x256 strip into the 16^3 UAV) and the depth-of-field table (CustomDofLutCopy, [128,1,1]: cbComputeCustomDof into the 2048x2
// R32F table the next frame's first draw reads). Until I-0138 tranche 8 the module resolved the LUT copy alone (the other two
// pins the record of what the bundle holds, I-0120 F5); since tranche 8 none is read by the
// module any more (the LUT copy is our own compute, post_tone.hlsl cs_lut_copy); the pins stay for the offline tests (the oracle
// GradingStage in post_tone_stage_test and grading_stage_test) and the build's check against the I-0063 programme cache.
struct YebisGxPin {const wchar_t* name;uint32_t type;uint32_t bytes;const char* sha256;};   // type: D3D11_SHVER (0 pixel, 5 compute)
inline constexpr std::array<YebisGxPin,3> yebis_gx_pins{{
    {L"PointSampleColorClamp1",0u,736,"91462FDC7E630E371557714D81F27C6C460160D26FFE8E778536C6DD943E37EB"},
    {L"LutCopy2DTo3DCs",5u,1040,"BDDD16E09DAFA473B5604D355679C41BDD9F6FEBE7DFFFF50F080D4E2085534E"},
    {L"CustomDofLutCopy",5u,1856,"3D3D289A9697BBA54D3A89735AC1FD9C36DE91D0C0D22E0CA282CB71387F35B0"},
}};
}
