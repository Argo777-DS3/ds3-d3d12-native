// I-0132 R12h step 2: compiled at build time by fxc (Build-GameBootstrap.ps1), with the flags the run-time compilation used;
// moved here byte for byte from its C++ string.
// The half-size object velocity (scene_pass_velocity.cpp): VS and PS, vs_5_0 / ps_5_0.
// W10 MB-4 (W10-NATIVE-PASSES 10, D-0162): the engine's half-size object velocity for the motion blur. The original rasterises the
// Velo rows into a half-size R8G8B8A8_UNORM target (pipeline+0xDF0, cleared to (127, 127, 255, 0) / 255) and runs the GX dilation
// twice, whose shipped length (MotionBlurScale x MaxBlurLength x BlurTimeRatio texels) keeps each texel and whose zero shift
// (+0.5 / 255 - 3e-7) leaves every 8-bit value (STATIC arithmetic, YEBIS-MOTIONBLUR-NATIVE 4.1). Natively the rows draw into the
// full-size R16G16B16A16_UNORM target; each half-size texel takes, of its 2x2 block, the texel with the largest alpha (an object
// row), ties by the smallest blue (the nearest), converted to 8 bits by the output merger as the original's rows are. I-0111 tranche
// 3: the target is half the presentation's extent (the original's half of the scene it blurs); in a scaled DLSS mode the block is the
// render-size velocity's under the output pixels it covers (b0: render / output; 1 keeps the block 2 x the texel).
cbuffer Map : register(b0) { float2 ratio; };
Texture2D<float4> velocity : register(t0);
float4 VS(uint id : SV_VertexID) : SV_Position { const float2 t=float2((id<<1)&2,id&2); return float4(t*float2(2,-2)+float2(-1,1),0,1); }
float4 PS(float4 position : SV_Position) : SV_Target {
    const int2 base=int2(floor(position.xy)*2.0*ratio);
    float4 best=velocity.Load(int3(base,0));
    [unroll] for(int i=1;i<4;++i){ const float4 v=velocity.Load(int3(base+int2(i&1,i>>1),0)); if(v.w>best.w || (v.w==best.w && v.z<best.z)) best=v; }
    return best;
}
