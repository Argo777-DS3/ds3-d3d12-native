// I-0138 tranche 2 (ROADMAP workstream 11): the glare family's programmes of our own (the bloom's source chain, bright pass,
// levels, 2x draws and composite; the light shafts' mask, radial blurs and add; Alpha_R), in place of YEBIS's read from the
// game's image. The draws, their targets, samplers and constants stay the plan's (yebis::BuildGlare, BuildShaft, AlphaRDraw:
// our transcription of the library's CPU formulas); these programmes only compute what the library's did, operation for
// operation, so that the same inputs give the same bytes (post_draws_test T13 and glare_stage_test T14 on them, against the
// original's captured draws).
//
// One shape a define, a count where the shape has one (Build-GameBootstrap.ps1 compiles each with fxc as shader model 5 with
// IEEE strictness, the original's compiler family; the originals are shader model 4 without the refactoring flag):
//   VS_OFFSET n (1 to 8)   vs_2D_OffsetTex2D{n}: n coordinates, the vertex's plus an offset each
//   VS_GIVEN n (1 to 4)    vs_2D_Tex2D{n}: the vertex's n coordinates as they are
//   PS_TAPS n (1 to 15)    ps_MadT1_S{n} under vs_2D_OffsetTex2D: the weighted taps of one texture; from the ninth tap on the
//                          coordinate is the first one's plus an offset, added here
//   PS_GIVEN n (2 to 4)    ps_MadT1_S{n} under vs_2D_Tex2D{n} (the shafts' radial blurs): the same sum at the given coordinates
//   PS_ADD n (2 to 8)      ps_MadT{n}: n textures at one coordinate, a weight each; with GAMMA, ps_MadLumGammaT{n}
//   PS_BRIGHT              with REMAP ps_BrightPassT1_THRESHOLD_LUMINANCE_REMAP_S1, without it (I-0138 tranche 8f: RemapFactor 1)
//                          ps_BrightPassT1_THRESHOLD_LUMINANCE_S1; with VIGNETTE, their _Vignette_ forms
//   PS_SHAFT_MASK          ps_LightShaftMask_SM3
//   PS_CORONA              ps_Corona (under vs_2D_Tex2D2; I-0138 tranche 8b): the shafts' spectral ring
//   PS_ALPHA               ps_Alpha_R
// I-0138 tranche 4 (part 4a), the depth of field's shapes:
//   PS_TAPS n up to 25     ps_MadT1_S16, S20 and S25 too (the layers' downsamples)
//   PS_DOF_FACTOR          ps_DepthOfField_BlurFactorAlphaFromDepthTextureTypeCastLut_RVALUE
//   PS_DOF_FRONT_FACTOR    ps_DepthOfField_FrontBlurBlendFactor
//   PS_DOF_COMPOSITE       ps_DepthOfField_CompositeBlurLevelsFrontBlur_TS{DOF_BACK}_{DOF_FRONT} (0 to 4, 0 to 7)
// I-0138 tranche 4 (part 4b), the composite under holder flags 0:
//   PS_DOF_COMPOSITE DOF_ABS with DOF_BACK n - 1 (0 to 7), DOF_FRONT 0: ps_DepthOfField_CompositeBlurredLevels_T{n}, the same
//                          blends with the scene's factor taken absolute
// and the motion blur's:
//   PS_MB_VELOCITY         ps_MotionBlurMakeCameraVelocityOverwrite_R (under vs_2D_Tex2D3)
//   PS_MB_BLUR n (2 to 8)  ps_MotionBlurVelocityDirDepthInnerscreen_SM3_T2_S{n}: the centre and n - 1 taps along the velocity
// I-0138 tranche 8f, GlareShape 3 (« Standard »):
//   PS_MAX n (2 to 8)      ps_MaxColorT{n}: the largest of n weighted textures, scaled (the streak arms' layer)
//   PS_MAD_OFFSET          ps_MadOffsetT2: two weighted textures, an offset, a floor and a ceiling (the afterimage)
//   VS_POINT_SYMMETRY      vs_2D_PointSymmetryTex2D8: eight coordinates reflected through each ghost's centre and scaled
//   PS_GHOST               ps_GlareGhost_PointSymmetryT4_S8_TrimT1_S8: eight ghosts of four textures through the trim (t4); with
//                          DIST, ps_GlareGhost_PointSymmetryDistScaleT4_S8_TrimT1_S8: each ghost's radius raised to its exponent
//   PS_GHOST_T3            ps_MadT3_S3S3S2: the second ghost draw, eight coordinates over three textures (4, 2, 2)
// The literals are the originals' bits (read with fxc /dumpbin /Lx: the listings print them rounded): 0x3eaaaaab (1/3),
// 0x34800000 (2^-22), 0x477a0000 (64000), the falloff's four exponents -1.442695 x (1, 2, 4, 8), 0x3d888889 (1/15).
// A for over the taps is marked [unroll]: under IEEE strictness fxc otherwise keeps it as a loop.
// The operands of a product, a sum, a min or a max are written in the order that gives the original's instruction (fxc emits
// them reversed: `a*b` is `mul b, a`), so that a NaN or a signed zero meets the same operand first;
// investigations/I-0138-modern-post-chain/Compare-Programmes.py checks it (the listings, then the values; with --glare the
// first family's shapes).

// The block is the plan's, in our layout (I-0138 tranche 8b; yebis_glare.h glare_block_registers: 91 float4, 1,456 bytes): slot
// i holds the library's register glare_block_registers[i], the comments name each field's library register.
cbuffer PostGlareBlock:register(b0){
    float4 depth_decode:packoffset(c0);         // (library c0) the depth of field's factor: .x and .y decode the depth
    float4 screen_scale:packoffset(c1);         // (c5) .xy: the position's scale (1, -1)
    float4 shaft_grain:packoffset(c2);          // (c10) the shaft mask: .y the angle's scale into the film grain, .z its offset
#if defined(PS_MB_VELOCITY) || defined(PS_MB_BLUR)
    // The motion blur's words in slots other shapes name otherwise (yebis_motionblur.cpp puts them).
    float4 mb_planes:packoffset(c3);            // (c15) the camera velocity: .y the far factor, .z far x near, .w far - near
    float4 mb_decode:packoffset(c80);           // (c186) the blur: a velocity texel's .xy into a direction (scale .xy, offset .zw)
    float4 mb_step:packoffset(c81);             // (c187) the blur: a velocity texel's .xy into one tap's step in coordinates
    float4 mb_range:packoffset(c82);            // (c188) the blur: .z the direction's smallest |x| + |y| that blurs
    float4 mb_test:packoffset(c83);             // (c189) the blur: .x the similarity factor, .y the depth test's margin
    float4 mb_rect:packoffset(c84);             // (c190) the blur: a coordinate into the inner rectangle's [-1, 1]
    float4 mb_previous[4]:packoffset(c87);      // (c224..c227) the camera velocity: the previous-clip transform's rows
#else
    float4 shaft_mask:packoffset(c3);           // (c15) the shaft mask: .x the grain's strength, .y and .z the colour's scale and pivot, .w the depth threshold
    float4 offset0:packoffset(c80);             // (c186) Alpha_R: .x the offset; MaxColorT{n}: the scale; MadOffsetT2: the offset
    float4 offset1:packoffset(c81);             // (c187) MadOffsetT2: the floor
    float4 offset2:packoffset(c82);             // (c188) MadOffsetT2: the ceiling
#endif
    float4 dof_factor:packoffset(c4);           // (c17) the depth of field's factor: .x and .y scale the table's value into the alpha
    float4 dof_back[7]:packoffset(c5);          // (c19..c25) the depth of field's composite: a back level's remap of the scene's factor (.x, .y)
    float4 vertex_offsets[8]:packoffset(c12);   // (c34..c41) .xy: the first eight taps' coordinate offsets; the ghosts' centres
    float4 ghost_offsets[8]:packoffset(c20);    // (c42..c49) .xy: the ghosts' offsets (afUV_TexCoordOffsetV16's second half)
    // (c50..c68) .xy: taps 8 to 24's; [0].x also the depth of field's table row; the DistScale ghosts: [g].x ghost g's exponent,
    // [8] and [9] the coordinate's map into the centred square and back, [11 + g].xy ghost g's distortion offset
    float4 pixel_offsets[19]:packoffset(c28);
    float4 scalers[8]:packoffset(c47);          // (c146..c153) the ghosts' scales: .x the reflection's factor, .zw the centre's pull
    float4 weights[25]:packoffset(c55);         // (c154..c178) a tap's or an input's weight; the bright pass: [0] the scene's factor, [1] 1/F, [2].w the threshold
    float4 gamma:packoffset(c85);               // (c202) the composite: .x 1 / the luminance gamma
    float4 remap:packoffset(c86);               // (c209) the bright pass's remap: .x and .w
};
SamplerState s0:register(s0);SamplerState s1:register(s1);SamplerState s2:register(s2);SamplerState s3:register(s3);
SamplerState s4:register(s4);SamplerState s5:register(s5);SamplerState s6:register(s6);SamplerState s7:register(s7);
Texture2D<float4> t0:register(t0);Texture2D<float4> t1:register(t1);Texture2D<float4> t2:register(t2);Texture2D<float4> t3:register(t3);
Texture2D<float4> t4:register(t4);Texture2D<float4> t5:register(t5);Texture2D<float4> t6:register(t6);Texture2D<float4> t7:register(t7);

#if defined(VS_OFFSET)
struct Offsets {float2 uv[VS_OFFSET]:TEXCOORD0;float4 position:SV_Position;};
Offsets main(float2 position:POSITION,float2 uv:TEXCOORD0){
    Offsets v;[unroll] for(int i=0;i<VS_OFFSET;++i)v.uv[i]=uv+vertex_offsets[i].xy;
    v.position=float4(position*screen_scale.xy,0.1,1.0);return v;}
#elif defined(VS_GIVEN)
struct Given {float2 uv[VS_GIVEN]:TEXCOORD0;float4 position:SV_Position;};
Given main(float2 position:POSITION,float2 uv[VS_GIVEN]:TEXCOORD0){
    Given v;[unroll] for(int i=0;i<VS_GIVEN;++i)v.uv[i]=uv[i];
    v.position=float4(position*screen_scale.xy,0.1,1.0);return v;}
#elif defined(PS_TAPS)
#if PS_TAPS<8
#define COORDS PS_TAPS
#else
#define COORDS 8
#endif
float4 main(float2 uv[COORDS]:TEXCOORD0):SV_Target{
#if PS_TAPS==1
    return t0.Sample(s0,uv[0])*weights[0];
#else
    float4 s=t0.Sample(s0,uv[1])*weights[1];
    s=mad(t0.Sample(s0,uv[0]),weights[0],s);
    [unroll] for(int i=2;i<PS_TAPS;++i){
        float2 c;if(i<8)c=uv[i];else c=uv[0]+pixel_offsets[i-8].xy;
        s=mad(t0.Sample(s0,c),weights[i],s);}
    return s;
#endif
}
#elif defined(PS_GIVEN)
float4 main(float2 uv[PS_GIVEN]:TEXCOORD0):SV_Target{
    float4 s=t0.Sample(s0,uv[1])*weights[1];
    s=mad(t0.Sample(s0,uv[0]),weights[0],s);
    [unroll] for(int i=2;i<PS_GIVEN;++i)s=mad(t0.Sample(s0,uv[i]),weights[i],s);
    return s;}
#elif defined(PS_ADD)
float4 main(float2 uv:TEXCOORD0):SV_Target{
    float4 s=t1.Sample(s1,uv)*weights[1];
    s=mad(t0.Sample(s0,uv),weights[0],s);
#if PS_ADD>2
    s=mad(t2.Sample(s2,uv),weights[2],s);
#endif
#if PS_ADD>3
    s=mad(t3.Sample(s3,uv),weights[3],s);
#endif
#if PS_ADD>4
    s=mad(t4.Sample(s4,uv),weights[4],s);
#endif
#if PS_ADD>5
    s=mad(t5.Sample(s5,uv),weights[5],s);
#endif
#if PS_ADD>6
    s=mad(t6.Sample(s6,uv),weights[6],s);
#endif
#if PS_ADD>7
    s=mad(t7.Sample(s7,uv),weights[7],s);
#endif
#ifdef GAMMA
    const float mean=dot(s.rgb,float3(0.3333333432674408,0.3333333432674408,0.3333333432674408));
    const float lifted=mean+2.384185791015625e-7;
    const float power=exp2(log2(min(max(mean,0.0),64000.0))*gamma.x);
    return float4((s.rgb/lifted)*power,s.a);
#else
    return s;
#endif
}
#elif defined(PS_BRIGHT)
// The coordinates as an array, a register each, as the vertex programme gives them (two separate float2 would share one).
#ifdef VIGNETTE
float4 main(float2 uvs[2]:TEXCOORD0):SV_Target{
    const float2 uv=uvs[0],vignette=uvs[1];
#else
float4 main(float2 uv:TEXCOORD0):SV_Target{
#endif
    const float4 texel=t0.Sample(s0,uv);
    float3 c=texel.rgb*weights[0].rgb;
    const float mean=dot(c,float3(0.3333333432674408,0.3333333432674408,0.3333333432674408));
    const float above=mean-weights[2].w;
    c=c/(mean+2.384185791015625e-7);
    const float d=max(above,0.0);
#ifdef REMAP
    const float curve=(d+sqrt(mad(d+remap.x,d,1.0))+-1.0)*remap.w;
    c=0.0<d?c*curve:c*d;
#else
    c=c*d;
#endif
#ifdef VIGNETTE
    const float r=dot(vignette,vignette);
    float w=saturate(mad(-r,2.0,r+1.0));w=w*w;w=w*w;c=c*w;
#endif
    return float4(min(max(c*weights[1].rgb,0.0),64000.0),texel.a);
}
#elif defined(PS_SHAFT_MASK) || defined(PS_CORONA)
// atan2(d.y, d.x) as the original's compiler expanded it (its polynomial's steps are mad; today's fxc under IEEE strictness would
// split them): the smaller over the larger magnitude, a ninth-degree odd polynomial, then the octant's folds.
float Angle(float2 d){
    const float2 magnitude=abs(d);
    const float ratio=min(magnitude.y,magnitude.x)*(1.0/max(magnitude.y,magnitude.x));
    const float square=ratio*ratio;
    float poly=mad(square,0.02083509974181652,-0.08513300120830536);
    poly=mad(square,poly,0.18014100193977356);
    poly=mad(square,poly,-0.3302994966506958);
    poly=mad(square,poly,0.9998660087585449);
    const float fold=magnitude.x<magnitude.y?mad(ratio*poly,-2.0,1.5707963705062866):0.0;
    float angle=mad(ratio,poly,fold);
    angle=angle+(d.x<-d.x?-3.1415927410125732:0.0);
    const float low=min(d.y,d.x),high=max(d.y,d.x);
    return (low<-low && high>=-high)?-angle:angle;}
#if defined(PS_SHAFT_MASK)
// uv[0]: the bright pass's coordinate; uv[2]: the direction from the sun, in the vignette's space.
float4 main(float2 uv[3]:TEXCOORD0):SV_Target{
    const float angle=Angle(uv[2]);
    const float base=angle*shaft_grain.y;
    const float3 around=mad(shaft_grain.zzz,float3(-0.25,0.5,-0.75),base);
    float4 grain;
    grain.x=t1.Sample(s1,float2(mad(angle,shaft_grain.y,shaft_grain.z),0.0)).x;
    grain.y=t1.Sample(s1,float2(around.x,0.0)).y;
    grain.z=t1.Sample(s1,float2(around.y,0.0)).z;
    grain.w=t1.Sample(s1,float2(around.z,0.0)).w;
    grain=grain+-0.5;
    grain=saturate(grain+grain);
    grain=saturate(mad(-grain,shaft_mask.x,1.0));
    float streak=grain.x*grain.y;streak=streak*grain.z;streak=streak*grain.w;
    const float4 texel=t0.Sample(s0,uv[0]);
    const float lit=texel.a>=shaft_mask.w?1.0:0.0;
    float3 colour=texel.rgb-shaft_mask.z;
    colour=mad(shaft_mask.y,colour,shaft_mask.z);
    colour=(colour*lit)*streak;
    const float4 fall=exp2(sqrt(dot(uv[2],uv[2]))*float4(-1.4426950216293335,-2.885390043258667,-5.770780086517334,-11.541560173034668));
    float falloff=mad(fall.y,2.0,fall.x);falloff=mad(fall.z,4.0,falloff);falloff=mad(fall.w,8.0,falloff);
    falloff=falloff*0.06666667014360428;
    return float4(colour*falloff,0.0);
}
#else
// ps_Corona: uv[0] the last radial blur's coordinate, uv[1] the position from the shaft's source; t0 that blur (the shafts),
// t1 the library's spectral ramp (256x1, built on the CPU), t2 the film grain. The ring's radius is the distance stretched by the
// grain at the angle (shaft_grain: .y and .z the angle into the grain, .x the grain's strength, .w the ramp alpha's exponent);
// shaft_mask is the corona's here (.x its strength, also the grain's damping, .y the falloff's base, .z the ramp coordinate's scale,
// .w the radius's offset); weights[0] the record's tint (c154: the ring's base, toward which the falloff fades), [1] the ramp
// colour's scale (c155: 1.5), [2] the output's factor (c156: (1, 1, 1, 0), the output's alpha 0).
float4 main(float2 uv[2]:TEXCOORD0):SV_Target{
    const float angle=Angle(uv[1]);
    const float grain=t2.Sample(s2,float2(mad(angle,shaft_grain.y,shaft_grain.z),0.0)).x+-0.5;
    const float stretch=mad(grain*saturate(-shaft_mask.x+1.0),shaft_grain.x,1.0);   // fxc emits add_sat 1, -x whichever order is written (1.0 - x and 1.0 + -x tried): the one commuted add
    const float distance=sqrt(dot(uv[1],uv[1]));
    const float place=mad(distance,stretch,shaft_mask.w);
    float edge=max(mad(distance,stretch,-1.0),0.0);edge=edge*edge;
    const float4 ramp=t1.Sample(s1,float2(max(place,1.0)*shaft_mask.z,0.5));
    const float3 colour=mad(ramp.rgb,weights[1].rgb,-weights[0].rgb);
    const float strength=exp2(log2(ramp.a)*shaft_grain.w)*shaft_mask.x;
    const float falloff=strength*exp2(edge*log2(shaft_mask.y));
    const float3 ring=mad(falloff.xxx,colour,weights[0].rgb);
    const float4 texel=t0.Sample(s0,uv[0]);
    return float4(ring*texel.rgb,texel.a)*weights[2];
}
#endif
#elif defined(PS_DOF_FACTOR)
// The depth of field's factor: the depth decoded, the circle of confusion read in the table (t2, its row at pixel_offsets[0].x),
// scaled into the alpha; the colour zero (the draw writes the alpha alone).
float4 main(float2 uv:TEXCOORD0):SV_Target{
    const float depth=mad(t0.Sample(s0,uv).x,depth_decode.x,depth_decode.y);
    const float alpha=mad(dof_factor.x,t2.Sample(s2,float2(depth,pixel_offsets[0].x)).x,dof_factor.y);
    return float4(0.0,0.0,0.0,alpha);
}
#elif defined(PS_DOF_FRONT_FACTOR)
// The front blur's blend factor: the colour kept, the alpha remapped, squared and quartered, at most 1.
float4 main(float2 uv:TEXCOORD0):SV_Target{
    const float4 texel=t0.Sample(s0,uv);
    float w=max(mad(texel.a,weights[0].w,weights[1].w),0.0);
    w=w*w;w=w*0.25;
    return float4(texel.rgb,min(w,1.0));}
#elif defined(PS_DOF_COMPOSITE)
// The depth of field's composite, DOF_BACK back levels (0 to 4; 0 to 7 with DOF_ABS) then DOF_FRONT front levels (0 to 7) over the scene (t0, its
// alpha the depth factor): a back level is blended by the scene's factor remapped for it, a front level by the root of four
// times its own alpha less the next level's. The same textures t1.. serve both. The original's compiler put the two operands
// of some differences in the other order at eleven of the forty counts (those without a front level or with four back
// levels: it followed its own order of the samples); today's does not follow the source there. Compare-Programmes.py lists
// them, and the stage tests compare every count's bytes with the original programme's.
#if DOF_BACK>DOF_FRONT
#define DOF_LEVELS DOF_BACK
#else
#define DOF_LEVELS DOF_FRONT
#endif
// Part 4b: CompositeBlurredLevels_T{n} reads the scene's factor absolute (mad_sat r, |t0.a|, ...), up to seven back levels.
#if defined(DOF_ABS)
#define DOF_FACTOR abs(base.a)
#else
#define DOF_FACTOR base.a
#endif
#define DOF_LEVEL(n) const float4 l##n=t##n.Sample(s##n,uv[1]);
float4 main(float2 uv[2]:TEXCOORD0):SV_Target{
    const float4 base=t0.Sample(s0,uv[0]);
#if DOF_LEVELS>=1
    DOF_LEVEL(1)
#endif
#if DOF_LEVELS>=2
    DOF_LEVEL(2)
#endif
#if DOF_LEVELS>=3
    DOF_LEVEL(3)
#endif
#if DOF_LEVELS>=4
    DOF_LEVEL(4)
#endif
#if DOF_LEVELS>=5
    DOF_LEVEL(5)
#endif
#if DOF_LEVELS>=6
    DOF_LEVEL(6)
#endif
#if DOF_LEVELS>=7
    DOF_LEVEL(7)
#endif
    float3 c=base.rgb;
#if DOF_BACK>=1
    c=mad(saturate(mad(DOF_FACTOR,dof_back[0].x,dof_back[0].y)),l1.rgb-c,c);
#endif
#if DOF_BACK>=2
    c=mad(saturate(mad(DOF_FACTOR,dof_back[1].x,dof_back[1].y)),l2.rgb-c,c);
#endif
#if DOF_BACK>=3
    c=mad(saturate(mad(DOF_FACTOR,dof_back[2].x,dof_back[2].y)),l3.rgb-c,c);
#endif
#if DOF_BACK>=4
    c=mad(saturate(mad(DOF_FACTOR,dof_back[3].x,dof_back[3].y)),l4.rgb-c,c);
#endif
#if DOF_BACK>=5
    c=mad(saturate(mad(DOF_FACTOR,dof_back[4].x,dof_back[4].y)),l5.rgb-c,c);
#endif
#if DOF_BACK>=6
    c=mad(saturate(mad(DOF_FACTOR,dof_back[5].x,dof_back[5].y)),l6.rgb-c,c);
#endif
#if DOF_BACK>=7
    c=mad(saturate(mad(DOF_FACTOR,dof_back[6].x,dof_back[6].y)),l7.rgb-c,c);
#endif
#if DOF_FRONT>1
    c=mad(sqrt(saturate((l1.a-l2.a)*4.0)),l1.rgb-c,c);
#elif DOF_FRONT==1
    c=mad(sqrt(saturate(l1.a*4.0)),l1.rgb-c,c);
#endif
#if DOF_FRONT>2
    c=mad(sqrt(saturate((l2.a-l3.a)*4.0)),l2.rgb-c,c);
#elif DOF_FRONT==2
    c=mad(sqrt(saturate(l2.a*4.0)),l2.rgb-c,c);
#endif
#if DOF_FRONT>3
    c=mad(sqrt(saturate((l3.a-l4.a)*4.0)),l3.rgb-c,c);
#elif DOF_FRONT==3
    c=mad(sqrt(saturate(l3.a*4.0)),l3.rgb-c,c);
#endif
#if DOF_FRONT>4
    c=mad(sqrt(saturate((l4.a-l5.a)*4.0)),l4.rgb-c,c);
#elif DOF_FRONT==4
    c=mad(sqrt(saturate(l4.a*4.0)),l4.rgb-c,c);
#endif
#if DOF_FRONT>5
    c=mad(sqrt(saturate((l5.a-l6.a)*4.0)),l5.rgb-c,c);
#elif DOF_FRONT==5
    c=mad(sqrt(saturate(l5.a*4.0)),l5.rgb-c,c);
#endif
#if DOF_FRONT>6
    c=mad(sqrt(saturate((l6.a-l7.a)*4.0)),l6.rgb-c,c);
#elif DOF_FRONT==6
    c=mad(sqrt(saturate(l6.a*4.0)),l6.rgb-c,c);
#endif
#if DOF_FRONT==7
    c=mad(sqrt(saturate(l7.a*4.0)),l7.rgb-c,c);
#endif
    return float4(c,base.a);}
#elif defined(PS_MB_VELOCITY)
// The camera's velocity at a pixel, the object's over it. uv[0]: the depth's coordinate; uv[1]: the pixel's clip-space position;
// uv[2]: the object velocity's coordinate. The depth decoded and clamped (weights[1]), the clip position at that depth carried
// to the previous frame's clip space (mb_previous), the difference encoded (weights[0]) and held to a unit length; where the
// object velocity's alpha says so its decoded value (weights[2]) replaces the camera's. The alpha: the previous depth.
float4 main(float2 uv[3]:TEXCOORD0):SV_Target{
    const float depth=mad(t0.Sample(s0,uv[0]).x,depth_decode.x,depth_decode.y);
    const float z=saturate(mad(weights[1].x,depth,weights[1].y));
    const float q=-mb_planes.z/mad(mb_planes.w,z,-mb_planes.y);
    const float4 p=float4(float3(uv[1],z)*q,q);
    const float below=max(dot(p,mb_previous[3]),0.00097655999707058072);   // 0x3a7fffd5
    const float3 previous=float3(dot(p,mb_previous[0]),dot(p,mb_previous[1]),dot(p,mb_previous[2]))/below;
    float2 v=previous.xy-uv[1];
    const float4 object=t1.Sample(s1,uv[2]);
    float2 own=mad(object.xy,weights[2].zz,weights[2].ww);
    own=mad(own,weights[2].xy,-v);
    v=mad(saturate(object.w),own,v);
    v=v*weights[0].xy;
    const float square=dot(v,v);
    const float2 unit=v*rsqrt(square);
    v=1.0<sqrt(square)?unit:v;
    return float4(mad(v,weights[0].zz,weights[0].ww),0.0,previous.z);}
#elif defined(PS_MB_BLUR)
// The blur along the velocity: the centre and PS_MB_BLUR - 1 taps, a tap `m` steps away (an odd count: -1 to -(n - 1) / 2 then
// +1 to +(n - 1) / 2; an even count: +1 to n - 1; the sum is made in that order). A tap is added when it is inside the inner
// rectangle and either moves alike (its direction's dot with the centre's above the centre's own times mb_test.x) or is not
// nearer than the centre by more than mb_test.y (the scene's alpha); the sum is divided by the taps counted (weights[n] over
// the count). A centre slower than mb_range.z is left as it is.
float4 main(float2 uv:TEXCOORD0):SV_Target{
    const float2 velocity=t1.Sample(s1,uv).xy;
    const float2 step=mad(velocity,mb_step.xy,mb_step.zw);
    const float2 direction=mad(velocity,mb_decode.xy,mb_decode.zw);
    const float4 centre=t0.Sample(s0,uv);
    const float limit=centre.a+mb_test.y;
    const float alike=dot(direction,direction)*mb_test.x;
    float4 sum=centre;float count=1.0;
    [unroll] for(int i=1;i<PS_MB_BLUR;++i){
        const int side=(PS_MB_BLUR-1)/2;
        const int m=(PS_MB_BLUR-1)%2==0?(i<=side?-i:i-side):i;
        float2 at;if(m==1)at=step+uv;else if(m==-1)at=-step+uv;else at=mad(step,float(m),uv);
        const float4 texel=t0.SampleLevel(s0,at,0.0);
        const float2 moved=mad(t1.SampleLevel(s1,at,0.0).xy,mb_decode.xy,mb_decode.zw);
        const bool2 within=1.0>=abs(mad(at,mb_rect.xy,mb_rect.zw));
        const bool taken=(alike<dot(moved,direction)) || (texel.a>=limit);
        const float4 added=sum+texel;
        const float counted=count+1.0;
        sum=(within.y && within.x)?(taken?added:sum):sum;
        count=(within.y && within.x)?(taken?counted:count):count;}
    const float4 mean=sum*(weights[PS_MB_BLUR]/count);
    return (abs(direction.x)+abs(direction.y)>=mb_range.z)?mean:centre;}
#elif defined(PS_ALPHA)
float4 main(float2 uv:TEXCOORD0):SV_Target{return mad(t0.Sample(s0,uv).rrrr,weights[0].xxxx,offset0.xxxx);}
#elif defined(PS_MAX)
// The streak arms' layer: the largest of the arms' weighted colours, scaled by offset0 (the arm chain's gain), the alpha 0.
float4 main(float2 uv:TEXCOORD0):SV_Target{
    float3 m=t0.Sample(s0,uv).rgb*weights[0].rgb;
    m=max(m,m);
    m=max(m,t1.Sample(s1,uv).rgb*weights[1].rgb);
#if PS_MAX>2
    m=max(m,t2.Sample(s2,uv).rgb*weights[2].rgb);
#endif
#if PS_MAX>3
    m=max(m,t3.Sample(s3,uv).rgb*weights[3].rgb);
#endif
#if PS_MAX>4
    m=max(m,t4.Sample(s4,uv).rgb*weights[4].rgb);
#endif
#if PS_MAX>5
    m=max(m,t5.Sample(s5,uv).rgb*weights[5].rgb);
#endif
#if PS_MAX>6
    m=max(m,t6.Sample(s6,uv).rgb*weights[6].rgb);
#endif
#if PS_MAX>7
    m=max(m,t7.Sample(s7,uv).rgb*weights[7].rgb);
#endif
    return float4(m*offset0.rgb,0.0);}
#elif defined(PS_MAD_OFFSET)
// The afterimage: level 0 (t0) and the history (t1) weighted, an offset, then held between a floor and a ceiling.
float4 main(float2 uv:TEXCOORD0):SV_Target{
    float4 s=t1.Sample(s1,uv)*weights[1];
    s=mad(t0.Sample(s0,uv),weights[0],s);
    s=s+offset0;
    s=max(s,offset1);
    return min(s,offset2);}
#elif defined(VS_POINT_SYMMETRY)
// Ghost g's coordinate: the vertex's pulled toward the centre (vertex_offsets[g]) by scalers[g].zw, the rest of the way scaled by
// scalers[g].x (a reflection through the centre where it is negative), shifted by ghost_offsets[g] times that rest's length.
struct Ghosts {float2 uv[8]:TEXCOORD0;float4 position:SV_Position;};
Ghosts main(float2 position:POSITION,float2 uv:TEXCOORD0){
    Ghosts v;
    [unroll] for(int g=0;g<8;++g){
        const float2 r=uv-vertex_offsets[g].xy;
        float2 p=mad(scalers[g].zw,r,vertex_offsets[g].xy);
        const float2 q=uv-p;
        p=mad(q,scalers[g].xx,p);
        v.uv[g]=mad(ghost_offsets[g].xy,sqrt(dot(q,q)),p);}
    v.position=float4(position*screen_scale.xy,0.1,1.0);return v;}
#elif defined(PS_GHOST)
// Ghost g reads its level (ghosts 0 to 2 t0, 3 and 4 t1, 5 and 6 t2, 7 t3) through the trim's alpha (t4), weighted; ghost 1 first,
// then 0, 2 ... 7 (the original's order). With DIST its coordinate is first carried into the centred square (pixel_offsets[8]),
// its radius raised to the ghost's exponent (pixel_offsets[g].x), shifted (pixel_offsets[11 + g]) and carried back (pixel_offsets[9]).
float2 GhostAt(float2 uv,int g){
#ifdef DIST
    const float2 p=mad(uv,pixel_offsets[8].xy,pixel_offsets[8].zw);
    const float2 r=mad(scalers[g].zw,-p,p);
    const float d=sqrt(dot(r,r));
    const float e=exp2(log2(d)*pixel_offsets[g].x);
    const float k=e/d;
    const float2 q=mad(p,k,e*pixel_offsets[11+g].xy);
    return mad(q,pixel_offsets[9].xy,pixel_offsets[9].zw);
#else
    return uv;
#endif
}
float4 Ghost(Texture2D<float4> level,SamplerState s,float2 uv){
    const float4 texel=level.Sample(s,uv);const float trim=t4.Sample(s4,uv).w;
#ifdef DIST
    return texel*trim;
#else
    return trim*texel;
#endif
}
float4 main(float2 uv[8]:TEXCOORD0):SV_Target{
    float4 s=Ghost(t0,s0,GhostAt(uv[1],1))*weights[1];
    s=mad(Ghost(t0,s0,GhostAt(uv[0],0)),weights[0],s);
    s=mad(Ghost(t0,s0,GhostAt(uv[2],2)),weights[2],s);
    s=mad(Ghost(t1,s1,GhostAt(uv[3],3)),weights[3],s);
    s=mad(Ghost(t1,s1,GhostAt(uv[4],4)),weights[4],s);
    s=mad(Ghost(t2,s2,GhostAt(uv[5],5)),weights[5],s);
    s=mad(Ghost(t2,s2,GhostAt(uv[6],6)),weights[6],s);
    s=mad(Ghost(t3,s3,GhostAt(uv[7],7)),weights[7],s);
    return s;}
#elif defined(PS_GHOST_T3)
// The second ghost draw: the first's ghosts (t0) at coordinates 0 to 3, level b + 2 (t1) at 4 and 5, level b (t2) at 6 and 7.
float4 main(float2 uv[8]:TEXCOORD0):SV_Target{
    float4 s=t0.Sample(s0,uv[1])*weights[1];
    s=mad(t0.Sample(s0,uv[0]),weights[0],s);
    s=mad(t0.Sample(s0,uv[2]),weights[2],s);
    s=mad(t0.Sample(s0,uv[3]),weights[3],s);
    s=mad(t1.Sample(s1,uv[4]),weights[4],s);
    s=mad(t1.Sample(s1,uv[5]),weights[5],s);
    s=mad(t2.Sample(s2,uv[6]),weights[6],s);
    s=mad(t2.Sample(s2,uv[7]),weights[7],s);
    return s;}
#endif
