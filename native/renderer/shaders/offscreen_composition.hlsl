// I-0132 R12h step 2: compiled at build time by fxc (Build-GameBootstrap.ps1), with the flags the run-time compilation used;
// moved here byte for byte from its C++ string.
// The offscreen composition (SceneRenderer::OffscreenCompositionPipeline, scene_pass.cpp; menu_tonemap.h holds its CPU reference):
// VS, PS and BackgroundPS, vs_5_0 / ps_5_0.
// The programme (menu_tonemap_test reads the same built bytes, the final review's M4 of I-0113 tranche 27): the HLSL
// of menu_tonemap.h's Apply() for a tabled view, the sRGB encode otherwise; root constants b0 = {tabled, mode, scale, contrast, gamma,
// scaler.rgb}, t0 the view's lit HDR. BackgroundPS (tranche 29) writes the background quad's colour from the same b0: rgb in scaler.rgb, alpha in the gamma word
// (BackgroundQuadColour's packed alpha; the step review's L1).
Texture2D<float4> materialColor:register(t0);
cbuffer Tone:register(b0) {uint tabled;uint mode;float scale;float contrast;float gamma;float3 scaler;};
float Curve(float u) {
 float x=0.2*u/(1.0-u);
 if(mode==7){float a=1.0-exp2(-x),b=1.0-exp2(-3.089127*x);return pow(a+contrast*(b*b*b-a),1.0/gamma);}
 float f=max(x-0.004,0.0);return pow(f*(6.2*f+0.5)/(f*(6.2*f+1.7)+0.06),2.2/gamma);
}
float Entry(uint i) {float v=Curve((float(i)+0.5)/1024.0);if(isnan(v))v=0.0;return round(saturate(v)*255.0)/255.0;}
float Table(float y) {float f=clamp(y*1024.0-0.5,0.0,1023.0);uint i=uint(f);return lerp(Entry(i),Entry(min(i+1,1023u)),f-float(i));}
float4 VS(uint id:SV_VertexID):SV_Position {
 float2 uv=float2((id<<1)&2,id&2); return float4(uv*float2(2,-2)+float2(-1,1),1,1);
}
float4 PS(float4 position:SV_Position):SV_Target {
 float3 c=max(0,materialColor.Load(int3(position.xy,0)).rgb);
 if(tabled){float3 x=c*scale,y=x/(x+0.2);return float4(Table(y.r)*scaler.r,Table(y.g)*scaler.g,Table(y.b)*scaler.b,1);}
 float3 encoded=lerp(12.92*c,1.055*pow(c,1.0/2.4)-0.055,step(0.0031308,c));
 return float4(encoded,1);
}
float4 BackgroundPS(float4 position:SV_Position):SV_Target {return float4(scaler,gamma);}
