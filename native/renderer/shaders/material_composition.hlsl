// I-0132 R12h step 2: compiled at build time by fxc (Build-GameBootstrap.ps1), with the flags the run-time compilation used;
// moved here byte for byte from its C++ string.
// The material colour composition (SceneRenderer::MaterialColorCompositionPipeline, scene_pass.cpp): VS and PS, vs_5_0 / ps_5_0.
Texture2D<float4> materialColor:register(t0);
float4 VS(uint id:SV_VertexID):SV_Position {
 float2 uv=float2((id<<1)&2,id&2); return float4(uv*float2(2,-2)+float2(-1,1),0,1);
}
float4 PS(float4 position:SV_Position):SV_Target {
 float3 c=max(0,materialColor.Load(int3(position.xy,0)).rgb);
 float3 encoded=lerp(12.92*c,1.055*pow(c,1.0/2.4)-0.055,step(0.0031308,c));
 return float4(encoded,1);
}
