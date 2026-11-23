// I-0122 R4 (RT-PT-PLAN §8, R4-PLAN §1): DS3's local lights as one world-space list (RtLight, rt_scene.h) and their shading,
// transcribed from the light programmes' DXBC (evidence/I-0122/artifacts/dxbc: GXLightAcc_DPointLightN, LegacyDPointLightN,
// DSpotLightN; N lights a draw sum the same term). The programmes work in view space; the same terms in world space here (a
// rotation and a translation leave the dot products and distances as they are). The shared reader of the screen's pass
// (rt_lights.hlsl) and, later, of the rays' hits (R6 to R8).
#ifndef RT_LIGHTS_HLSLI
#define RT_LIGHTS_HLSLI
#define RT_LIGHT_POINT 0u    // GXLightAcc_DPointLightN (cbLightAccLightParam PointLight)
#define RT_LIGHT_LEGACY 1u   // GXLightAcc_LegacyDPointLightN (the light's +0xAC == 1)
#define RT_LIGHT_SPOT 2u     // GXLightAcc_DSpotLightN (SpotLight)
#define RT_LIGHT_SHADOWED 0x100u   // R4b: a light its class's `Shadowed` programmes draw (kind's flag; its strength in trans.z, any value)
#define RT_LIGHT_PHYSICAL 0x200u   // I-0132 R11h: the physical falloff (rt_scene.h RtPhysicalLight): the inverse square windowed at r, position.w = 1 / r^2
#define RT_LIGHT_KIND 0xFFu
// position: xyz world; w the point's PositionAndRadius.w (1 / R²) or the spot's PositionAndRange.w (its range R), a physical light's 1 / r²
// (I-0132 R11h: its window, every kind; DS3's R then a half float in kind's bits 16 to 31). diffuse:
// DiffColor (w the gloss clamp). specular: SpecColor.xyz; kind: RT_LIGHT_* with RT_LIGHT_SHADOWED. trans: TransLightParam x, y; z
// the shadow's strength (R4b: the `Shadowed` programmes' ProjParam.w or ShadowParam.x); w the spot's ShadingMode, a shadowed
// point's cube faces' near plane (its major-axis distance from the light: −ProjParam.y / ProjParam.x, the depth the programmes
// compare being (ProjParam.x m + ProjParam.y) / m). to_light: the spot's world-to-light rows x, y, z, w (its
// ViewToLightSpaceMatrix's rows times the camera's world-to-view; q.z > 0 in front of its near plane).
struct RtLight {float4 position;float4 diffuse;float3 specular;uint kind;float4 trans;float4 to_light[4];};
// A pixel's surface as the light programmes read it: P the world position, E the unit direction from the camera to P, N the unit
// world normal (GB0 × 2 − 1), gloss and translucency GB1.x and GB1.y.
struct RtLitSurface {float3 position;float3 eye;float3 normal;float gloss;float translucency;};
// The programmes' power, log2 then exp2 (their `log`, `mul`, `exp`): 0 at a base of 0 for a positive exponent.
float RtPow(float base,float exponent){return exp2(log2(base)*exponent);}
// The Blinn-Phong exponent n = 2^(13 g) clamped by the light's w: n' = max(1, n w / (0.01 n + w)).
float RtLobe(float gloss,float w){const float n=exp2(gloss*13);return max(n*w/(n*0.01+w),1);}
// The translucency all three share: sat(L·E)^(32 TL.y) × ((max(1 − E·N, 0)) / 2)² × TL.x × Diff × 10 t.
float3 RtTranslucency(RtLight light,RtLitSurface s,float3 l){
    const float back=max(dot(-s.eye,s.normal)+1,0)*0.5;
    return RtPow(saturate(dot(l,s.eye)),light.trans.y*32)*(back*back)*light.trans.x*light.diffuse.rgb*(s.translucency*10);}
// The wrapped diffuse where t > 0 (max(N·L + 10 t, 0) / (1 + 20 t)), the plain Lambert sat(N·L) where t = 0, both times Diff.
float3 RtWrapped(RtLight light,RtLitSurface s,float nl){return max(nl+s.translucency*10,0)/(s.translucency*20+1)*light.diffuse.rgb;}
// One light's diffuse and specular at the surface, before the programmes' common factors (the diffuse × (1 − F0 / 2), the
// specular × F0): returned in `diffuse`, `translucent` (the diffuse's translucency term, which the
// `Shadowed` programmes leave unshadowed: R4-PLAN §1) and `specular`. A shadowed light's visibility v multiplies `diffuse` and
// `specular` alone.
void RtLocalLight(RtLight light,RtLitSurface s,out float3 diffuse,out float3 translucent,out float3 specular){
    diffuse=0;translucent=0;specular=0;
    const uint kind=light.kind&RT_LIGHT_KIND;
    if(light.kind&RT_LIGHT_PHYSICAL){
        // I-0132 R11h: one falloff for every kind, I / max(d^2, 1 cm^2) x saturate(1 - (d / r)^4)^2 (Frostbite's window, connaissance
        // générale; LIGHT-UNITS.md), the diffuse and the specular alike; a spot's cone term (behind its plane nothing) kept; each kind's
        // lobe as DS3's (the spot's by its ShadingMode, a legacy point's reflection lobe, a point's half-vector lobe).
        const float3 to=light.position.xyz-s.position;const float d2=dot(to,to);const float3 l=to*rsqrt(max(d2,1e-12));
        const float x=d2*light.position.w,window=saturate(1-x*x);float f=window*window/max(d2,1e-4);
        bool half_vector=kind==RT_LIGHT_POINT;
        if(kind==RT_LIGHT_SPOT){const float4 p=float4(s.position,1);
            const float qx=dot(light.to_light[0],p),qy=dot(light.to_light[1],p),qz=dot(light.to_light[2],p),qw=dot(light.to_light[3],p);
            if(!(qz>0))return;
            const float2 q=float2(qx,qy)/qw;const float cone=max(1-dot(q,q),0);const bool mode=light.trans.w>0;
            f*=mode?cone:cone*cone;half_vector=mode;}
        const float nl=dot(l,s.normal);
        if(s.translucency==0)diffuse=saturate(nl)*light.diffuse.rgb*f;
        else{diffuse=RtWrapped(light,s,nl)*f;translucent=RtTranslucency(light,s,l)*f;}
        const float lobe=RtLobe(s.gloss,light.diffuse.w),norm=lobe*0.125+0.25;
        if(half_vector){const float3 h=normalize(l-s.eye);specular=RtPow(saturate(dot(h,s.normal)),lobe)*light.specular.rgb*RtPow(saturate(nl),0.1)*f*norm;}
        else{const float3 r=s.eye-2*dot(s.eye,s.normal)*s.normal;specular=RtPow(saturate(dot(l,r)),lobe*0.25)*norm*light.specular.rgb*f;}
        return;}
    if(kind==RT_LIGHT_SPOT){
        const float4 p=float4(s.position,1);
        const float qx=dot(light.to_light[0],p),qy=dot(light.to_light[1],p),qz=dot(light.to_light[2],p),qw=dot(light.to_light[3],p);
        if(!(qz>0))return;                                    // `lt r2.z, 0, r2.z` then `and`: nothing behind the light
        const float2 q=float2(qx,qy)/qw;
        const float range=light.position.w;
        const float cone=max(1-dot(q,q),0);
        const float3 to=light.position.xyz-s.position;const float d2=dot(to,to);const float3 l=to*rsqrt(d2);
        const float inv_r2=1/(range*range);
        const bool mode=light.trans.w>0;                      // ShadingMode: the point light's lobe and falloffs
        // mode: x = max(1/d² − 1/R², 0) × cone (diffuse), y = (1 − min(d²/R², 1)) × cone (specular); else both
        // sat(1 − q.w / R)² × cone².
        const float far=saturate(1-qw/range);
        const float fd=mode?max(1/d2-inv_r2,0)*cone:far*far*cone*cone;
        const float fs=mode?(1-min(d2*inv_r2,1))*cone:fd;
        const float nl=dot(l,s.normal);
        if(s.translucency==0)diffuse=saturate(nl)*light.diffuse.rgb*fd;
        else{diffuse=RtWrapped(light,s,nl)*fd;translucent=RtTranslucency(light,s,l)*fd;}
        const float lobe=RtLobe(s.gloss,light.diffuse.w),norm=lobe*0.125+0.25;
        if(mode){const float3 h=normalize(l-s.eye);
            specular=RtPow(saturate(dot(h,s.normal)),lobe)*light.specular.rgb*RtPow(saturate(nl),0.1)*fs*norm;}
        else{const float3 r=s.eye-2*dot(s.eye,s.normal)*s.normal;
            specular=RtPow(saturate(dot(l,r)),lobe*0.25)*norm*light.specular.rgb*fs;}
        return;}
    const float3 to=light.position.xyz-s.position;const float d2=dot(to,to);const float3 l=to*rsqrt(d2);
    const float nl=dot(s.normal,l);
    const float3 t=s.translucency==0?0:RtTranslucency(light,s,l);
    const float lobe=RtLobe(s.gloss,light.diffuse.w),norm=lobe*0.125+0.25;
    if(kind==RT_LIGHT_LEGACY){
        // (1 − sqrt(sat(d² / R²)))² for both terms; the reflection lobe sat(R·L)^(n' / 4), no N·L^0.1.
        const float f=1-sqrt(saturate(d2*light.position.w));const float fall=f*f;
        diffuse=s.translucency==0?saturate(nl)*fall*light.diffuse.rgb:RtWrapped(light,s,nl)*fall;translucent=t*fall;
        const float3 r=s.eye-2*dot(s.eye,s.normal)*s.normal;
        specular=RtPow(saturate(dot(r,l)),lobe*0.25)*norm*light.specular.rgb*fall;
        return;}
    // The point: the diffuse falls off as max(1/d² − 1/R², 0), the specular as 1 − sat(d²/R²); the half-vector lobe times sat(N·L)^0.1.
    const float fd=max(1/d2-light.position.w,0),fs=1-saturate(d2*light.position.w);
    diffuse=s.translucency==0?saturate(nl)*light.diffuse.rgb*fd:RtWrapped(light,s,nl)*fd;translucent=t*fd;
    const float3 h=normalize(l-s.eye);
    specular=RtPow(saturate(dot(h,s.normal)),lobe)*light.specular.rgb*RtPow(saturate(nl),0.1)*fs*norm;
}
#endif
