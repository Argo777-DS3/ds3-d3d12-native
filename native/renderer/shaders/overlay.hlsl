// I-0132 R12h step 2: compiled at build time by fxc (Build-GameBootstrap.ps1), with the flags the run-time compilation used;
// moved here byte for byte from its C++ string.
// The native overlay (overlay_pass.cpp): VS, PS and PSScreen, vs_5_1 / ps_5_1.
struct Quad {float4 rect;float4 uv;float4 colour;int image;uint coverage;int image2;uint mode;float4 uv2;};
StructuredBuffer<Quad> quads:register(t0);
Texture2D images[16]:register(t1);
SamplerState linear_clamp:register(s0);
SamplerState linear_wrap:register(s1);   // R12h: the fire's noise, scrolling
struct V {float4 position:SV_Position;float2 uv:TEXCOORD0;float4 colour:COLOR0;nointerpolation int image:IMAGE;nointerpolation uint coverage:COVERAGE;
    float2 uv2:TEXCOORD1;nointerpolation int image2:IMAGE2;nointerpolation uint mode:MODE;};
V VS(uint id:SV_VertexID){
    const Quad q=quads[id/6];const uint c=id%6;
    const float2 corner=float2(c==1 || c==2 || c==4?1:0,c==2 || c==4 || c==5?1:0);
    V v;v.position=float4(lerp(q.rect.xy,q.rect.zw,corner),0,1);v.uv=lerp(q.uv.xy,q.uv.zw,corner);
    v.colour=q.colour;v.image=q.image;v.coverage=q.coverage;v.uv2=lerp(q.uv2.xy,q.uv2.zw,corner);v.image2=q.image2;v.mode=q.mode;return v;}
float4 PS(V v):SV_Target{
    if(v.image<0)return v.colour;
    float4 t=images[NonUniformResourceIndex(v.image)].Sample(linear_clamp,v.uv);
    if(v.mode==1){   // R12h: the fire: the heat (the image's alpha) broken up by the bonfire's noise, coloured by the fire's ramp
        const float n=images[NonUniformResourceIndex(v.image2)].Sample(linear_wrap,v.uv2).r;
        const float heat=saturate(t.a*(0.25+1.5*n));   // the step review's M1: the quad's alpha scales the output, not the heat
        const float3 ramp=float3(smoothstep(0.0,0.32,heat),0.88*smoothstep(0.22,0.72,heat),0.55*smoothstep(0.7,1.0,heat));
        return float4(ramp*v.colour.rgb,smoothstep(0.02,0.7,heat)*v.colour.a);}
    if(v.coverage==1)return float4(v.colour.rgb,v.colour.a*t.r);
    if(v.coverage==2){const float3 c=saturate(t.rgb);t.rgb=c<=0.0031308?c*12.92:1.055*pow(c,1.0/2.4)-0.055;}   // the target holds encoded values
    return v.colour*t;}
// I-0132 R12e: the screen layer's output, premultiplied (its blend: target + colour x (1 - target), light over dark, nothing over white).
// Its second step review's L7: half a code value of dither where it draws (the smoke spans a few 8-bit codes over black: no bands).
float4 PSScreen(V v):SV_Target{const float4 c=PS(v);const float n=frac(52.9829189*frac(dot(v.position.xy,float2(0.06711056,0.00583715))))-0.5;
    return float4(max(c.rgb*c.a+(c.a>0.001?n/255.0:0.0),0.0),c.a);}
