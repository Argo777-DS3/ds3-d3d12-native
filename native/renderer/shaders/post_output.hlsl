// I-0138 tranche 5 (ROADMAP workstream 11): the output of an HDR10 chain. Every frame of such a chain ends in one float picture
// (R16G16B16A16_FLOAT), display-encoded: the power 1/2.2 of the linear light, 1 the paper white (an SDR picture's own encoding,
// continued above 1 by the tone map's highlights). The game's UI and our overlay draw on it in place with the game's own blend
// states, as the SDR chain draws them on its 8-bit picture (a UI drawn apart on a layer cannot give DS3's multiply rows: the
// first witness of the layer, i138-t5c-cycle, left the scene undimmed behind the menus).
//   ps_copy: a frame's 8-bit picture (a world frame without our tone map's HDR entry) into the float picture, value for value,
//   before the UI.
//   ps_scene: the frame's last draw, the float picture into the R10G10B10A2_UNORM back buffer whose colour space is ST 2084 over
//   Rec. 2020 (DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020), or into DLSS-G's HUD-less colour before the UI: decoded to linear
//   light, the largest channel rolled off to the peak above max(paper white, peak / 2) (the hue kept; nothing below changes),
//   taken to Rec. 2020, scaled by the paper white, clamped to the peak, encoded by ST 2084, and (flags bit 0) dithered by one
//   10-bit step of the 64x64 blue noise, moved every frame.
// The decode: the power sdr_gamma (2.2). I-0138 tranche 5b (Thomas, 2026-11-14): the game's brightness option no longer reaches
// this chain (tranche 5 multiplied the power by the option's under the paper white): the paper white is its setting here.
// The SDR chain (the game's brightness pass into an 8-bit back buffer) does not run these programmes.
// Compiled by fxc as shader model 5 (Build-GameBootstrap.ps1) into post_output_vs.dxbc.h, post_output_copy_ps.dxbc.h and
// post_output_scene_ps.dxbc.h.

cbuffer OutputParams:register(b0){   // native/renderer/hdr_output.h, the same layout (hdr_output_test)
    float4 rec2020[3];   // Rec. 709 to Rec. 2020, rows
    float paper_white;   // nits
    float peak;          // nits
    float sdr_gamma;
    uint flags;          // bit 0: the dither
    int2 noise_offset;   // the blue noise's place this frame
    float2 pad;
};

struct Vertex {float4 position:SV_Position;};

Vertex vs(uint id:SV_VertexID){
    static const float2 positions[3]={float2(-1.0,-1.0),float2(-1.0,3.0),float2(3.0,-1.0)};
    Vertex v;v.position=float4(positions[id],0.0,1.0);return v;
}

Texture2D<float4> picture:register(t0);   // ps_copy: the 8-bit picture
Texture2D<float4> scene:register(t1);     // ps_scene: the float picture
Texture2D<float4> noise:register(t2);     // the 64x64 blue noise (R8_UNORM)

float4 ps_copy(Vertex v):SV_Target{
    return float4(picture.Load(int3(int2(v.position.xy),0)).rgb,1.0);
}

// SMPTE ST 2084, nits to signal.
float3 PqEncode(float3 nits){
    const float m1=2610.0/16384.0,m2=2523.0/4096.0*128.0,c1=3424.0/4096.0,c2=2413.0/4096.0*32.0,c3=2392.0/4096.0*32.0;
    const float3 p=pow(saturate(nits/10000.0),m1);
    return pow((c1+c2*p)/(1.0+c3*p),m2);
}
float4 ps_scene(Vertex v):SV_Target{
    const int2 pixel=int2(v.position.xy);
    const float3 encoded=max(scene.Load(int3(pixel,0)).rgb,0.0);
    float3 linear709=pow(encoded,sdr_gamma);   // 1 = the paper white
    const float top=peak/paper_white,start=max(1.0,0.5*top),m=max(linear709.r,max(linear709.g,linear709.b));
    if(m>start && top>start)linear709*=(start+(top-start)*(1.0-exp(-(m-start)/(top-start))))/m;
    const float3 linear2020=float3(dot(linear709,rec2020[0].rgb),dot(linear709,rec2020[1].rgb),dot(linear709,rec2020[2].rgb));
    float3 signal=PqEncode(min(linear2020*paper_white,peak));
    if(flags&1u)signal+=(noise.Load(int3((pixel+noise_offset)&63,0)).r*(255.0/256.0)+(0.5/256.0)-0.5)/1023.0;
    return float4(signal,1.0);
}
