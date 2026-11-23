// I-0132 R12h step 2: compiled at build time by fxc (Build-GameBootstrap.ps1), with the flags the run-time compilation used;
// moved here byte for byte from its C++ string.
// Histogram and Adapt, cs_5_0.
// I-0132 R11h: the automatic exposure's kernels (cs_5_0, made into pipelines by AutoExposureStage::Open; auto_exposure_test.cpp reads the same built bytes).
// I-0138 tranche 3: b0 is our tone-map pass's parameter block (PostToneParams, post_params.h: the same bytes the pass binds, 224 since I-0141 tranche 11) in
// place of the YEBIS tone map's `$Globals`; the kernels read its exposure, the curve's constants (the Reinhard curve's where its flag 4 is set: I-0138 part 4c), the colour matrix's rows
// and 1 / gamma, in the same expressions as before (the factor's bits are unchanged).
// The two kernels (auto_exposure_math.h is the same math on the CPU). Histogram: 16x16 threads a group, one bin a pixel (log2 of its
// Rec. 709 luminance over [log2_min, log2_min + 256 bin_width); a pixel not positive, NaN or below the range left out, above it in bin 255) into
// the group's bins, then added to the buffer's. Adapt: one group of 256 threads, one bin each: the bins read and cleared for the
// next frame, the kept weights (thread 0), the bisection (22 halvings, a reduction each), the adaptation (thread 0), the factor
// texel and the state. State (u1, bytes): 0 DLSS's exposure (the area's exposure x a; a texture copy's footprint at offset 0), 512
// log2 a, 516 the goal, 520 the mean displayed grey at a, 524 the pixels in range, 528 the initialised mark, 532 the area's exposure,
// 536 the target, 540 a reset waiting for a frame with pixels in range.
// The Tone block: our tone map's PostToneParams (224 bytes since I-0141 tranche 11, the ER vignette's words dropped: tone_rest its other words).
cbuffer Tone:register(b0){float4 tone_colour[3];float4 tone_alpha;float4 tone_vignette[3];float tone_exposure;float tone_curve_a;float tone_curve_b;float tone_inv_gamma;float4 tone_rest[6];};
cbuffer Params:register(b1){uint2 size;float log2_min;float bin_width;float low;float high;float target;float dt;
    float speed_brighter;float speed_darker;float transition;uint reset;float min_log2;float max_log2;float strength;uint pad1;};
Texture2D<float4> scene:register(t0);
RWTexture2D<float4> factor:register(u2);
RWByteAddressBuffer histogram:register(u0);
RWByteAddressBuffer state:register(u1);
groupshared uint bins[256];
groupshared float part[256];
groupshared float weight[256];
groupshared float shared_current;
groupshared uint shared_total;
[numthreads(16,16,1)] void Histogram(uint3 id:SV_DispatchThreadID,uint gi:SV_GroupIndex){
    bins[gi]=0;GroupMemoryBarrierWithGroupSync();
    if(id.x<size.x && id.y<size.y){const float3 c=scene.Load(int3(id.xy,0)).rgb;const float l=dot(c,float3(0.2126,0.7152,0.0722));
        // The step review's M2: a pixel below the range, not positive or NaN is left out (a black frame does not wind the factor up).
        const float x=(log2(l)-log2_min)/bin_width;
        if(l>0 && x>=0)InterlockedAdd(bins[(uint)min(x,255.0)],1u);}
    GroupMemoryBarrierWithGroupSync();
    if(bins[gi])histogram.InterlockedAdd(gi*4,bins[gi]);}
float DisplayGrey(float x){
    x=max(x,0.000061);
    float y;
    // I-0138 part 4c: the Reinhard curve where the block names it (its flag 4, its third constant at curve_c).
    if(asuint(tone_rest[0].w)&4u){const float s=x*tone_curve_b;y=saturate(s*(1+s*tone_rest[1].z)/(1+x*tone_curve_b));}
    else{const float e=exp2(-x*tone_curve_a*1.442695),k=1-e*tone_curve_b;y=saturate((1-e)*(k*k));}
    const float4 v=float4(y,y,y,1);
    float3 o=saturate(float3(dot(v,tone_colour[0]),dot(v,tone_colour[1]),dot(v,tone_colour[2])));
    o=exp2(log2(o)*tone_inv_gamma);
    return dot(o,float3(0.2126,0.7152,0.0722));}
float Sum(uint gi,float value){
    part[gi]=value;GroupMemoryBarrierWithGroupSync();
    [unroll] for(uint s=128;s>0;s>>=1){if(gi<s)part[gi]+=part[gi+s];GroupMemoryBarrierWithGroupSync();}
    const float sum=part[0];GroupMemoryBarrierWithGroupSync();return sum;}
[numthreads(256,1,1)] void Adapt(uint gi:SV_GroupIndex){
    bins[gi]=histogram.Load(gi*4);GroupMemoryBarrierWithGroupSync();
    histogram.Store(gi*4,0);
    if(gi==0){uint total=0;for(uint i=0;i<256;++i)total+=bins[i];shared_total=total;
        const float lo=low*total,hi=high*total;float acc=0;
        for(uint j=0;j<256;++j){const float a0=acc,a1=acc+bins[j];weight[j]=max(0,min(a1,hi)-max(a0,lo));acc=a1;}}
    GroupMemoryBarrierWithGroupSync();
    const float exposure=tone_exposure,lum=exp2(log2_min+(gi+0.5)*bin_width)*exposure,w=weight[gi];
    const float kept=Sum(gi,w);
    float lo2=min_log2,hi2=max_log2;
    [loop] for(uint k=0;k<22;++k){const float mid=0.5*(lo2+hi2);
        const float mean=Sum(gi,w>0?w*DisplayGrey(lum*exp2(mid)):0)/max(kept,1e-6);
        if(mean<target)lo2=mid;else hi2=mid;}
    // The step review's M2: held (the goal the current factor) when under 1 % of the frame's pixels are in range or the tone map's
    // exposure is its guard's (FLT_MIN, a black scene term): a fade, a black frame.
    // The final review's L4: the 1 % test in float (no overflow at any resolution).
    if(gi==0){const bool init=state.Load(528)==0xA5E0u,empty=!(kept>0) || float(shared_total)<0.01*float(size.x)*float(size.y) || !(exposure>1e-20);
        float current=asfloat(state.Load(512));
        // The adaptation's strength (D-0264: 0.5 by default): the solved factor's share in log2, the rest the area's own exposure (a = 1).
        const float goal=empty?(init?current:0):strength*0.5*(lo2+hi2);
        // The final review's L1: a reset on an empty frame waits (540) for the first frame with pixels in range.
        const bool pending=reset!=0 || state.Load(540)!=0;
        if(!init || (pending && !empty))current=goal;
        else{const float diff=goal-current,distance=abs(diff),speed=diff>0?speed_darker:speed_brighter;
            float step=distance>transition?min(speed*dt,distance-transition):distance*(1-exp2(-dt*speed/transition*1.442695));
            step=min(step,distance);current+=diff>0?step:-step;}
        current=clamp(current,min_log2,max_log2);shared_current=current;
        const float a=exp2(current);factor[uint2(0,0)]=float4(a,a,a,1);
        // The first step review's L1: DLSS's exposure is a alone where the tone map's exposure is its guard's FLT_MIN (a black scene term).
        state.Store(0,asuint(exposure>1e-20?exposure*a:a));state.Store(512,asuint(current));state.Store(516,asuint(goal));
        // An uninitialised state on an empty frame waits too (a step review's L4). No comment after code in this file (the reviews' H1s; Build-GameBootstrap.ps1 refuses one).
        state.Store(524,shared_total);state.Store(528,0xA5E0u);state.Store(540,(pending || !init) && empty?1u:0u);state.Store(532,asuint(exposure));state.Store(536,asuint(target));}
    GroupMemoryBarrierWithGroupSync();
    const float shown=Sum(gi,w>0?w*DisplayGrey(lum*exp2(shared_current)):0)/max(kept,1e-6);
    if(gi==0)state.Store(520,asuint(shown));}
