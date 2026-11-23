// I-0132 R12h step 2: compiled at build time by fxc (Build-GameBootstrap.ps1), with the flags the run-time compilation used;
// moved here byte for byte from its C++ string.
// Resolve, Sharpen and Mirror, cs_5_0 (strictness, IEEE strictness, level 3).
// I-0132 R11j (R11-PLAN §11): the TAA's resolve, ER's TempAA mode of GXAntiAliasingFilter (ER-TAA-STUDY) written on DS3's inputs. One
// compute pass at the render extent (equal to the output's: no upscaling), 8x8 groups. t0 the lit HDR (jittered), t1 the depth (the
// R32 of D32S8, 1 far), t2 the native motion vectors (previous minus current position, render pixels, unjittered), t3 the history (the
// previous frame's result); u0 the output (the post chain's source), u1 the new history (the result, unsharpened, as ER's).
// - ER's DilateMotionVector inside the pass: the motion read at the 3x3's nearest depth (an edge takes the foreground's motion).
// - The current colour at the pixel's unjittered centre: the 3x3 samples weighted by a Gaussian fit of Blackman-Harris over their
//   scene distance to it (the frame is drawn shifted by +jitter pixels), each also by the perceptual weight below.
//   I-0140 tranche 41 (D-0306): the Gaussian's coefficient `kernel` (2.29 the fit, the main view's; a menu view's 8, chosen by measure
//   against the original's DLAA); with `own_reset` a reset frame (drawn unjittered: a menu view's reset as its collect sees it, its first collect, a cut or a collect after one not drawn)
//   is the pixel's own sample, not a reconstruction (the fit filters even an unjittered frame: 0.10 on each side, 0.01 on the diagonals).
// - The history read with Catmull-Rom (5 bilinear taps), clipped (not clamped) toward the neighbourhood's YCoCg variance box (mean
//   +- gamma sigma, inside the min-max box): no hue shift, no ghost past a disocclusion.
// - The blend: the history's weight `feedback` at rest, both sides weighted by 1 / (1 + exposed max channel) (bright samples weigh as
//   dim ones once tone-mapped: no flicker from a firefly); the current colour alone on a reset or a reprojection off the screen.
// - Thomas's sharpening (2026-11-10 04:28; ER's SharpenFilter.ppo, its history left unsharpened): with `sharpen` above 0 the resolve
//   writes the history alone and Sharpen reads it (t3) into the output, a contrast-adaptive sharpening of the cross (AMD's CAS idea,
//   written here): each channel's weight -amp x sharpen / 5, amp = sqrt(min(lo, 1 - hi) / hi) of the cross's tone-mapped
//   (c e / (1 + c e)) minimum and maximum, so flat areas and strong edges take little, soft detail the most; 0 none, 1 CAS's strongest;
//   the result kept within the cross's range (no overshoot: the final review's H1).
// - I-0132 R12c: the same sharpening on DLSS's output (Thomas, 2026-11-11): Mirror copies the target (u1, read as a UAV) into the
//   stage's scratch (u0), then Sharpen reads the scratch (t3) into the target (u0).
// - I-0113 tranche 39 (STUDY-REACTIVE-MASK; FSR 2's « auto-reactive » rule, as I-0111 tranche 4 drew it for DLSS): with `reactive` set,
//   t4 is the lit HDR copied before the 0x29 rows and the effects (the transparent rows, the effect draws and the effect layer's
//   composite). Where they changed the pixel, the history's weight falls: where the largest channel difference between the current
//   colour and that base, both exposed and through the reversible tone map c / (1 + max(c)), passes 0.01 to 0.03 (a smoothstep: no
//   flicker on a hard threshold, the R pilot's review), up to 0.9 of `feedback` is taken away (FSR 2's thresholded form, softened; an
//   animated effect, a fading body: no motion vector of their own, their past frames otherwise blended in). The first form (nothing under 0.02, then the difference itself, at most 0.9) moved the mist's gradient little (RESULT « Tranche 39 », g24n).
//   I-0113 tranche 41: the difference is the largest over the 3x3 (FSR 2 dilates its reactive mask the same way): a pixel the rows
//   left as it was beside one they changed (a dissolving body's holes, the old edge of a moving fading arm) no longer keeps its
//   history, which the neighbourhood's box, spanning both colours, did not clip. `reactive` 2 (taa-reactive-view.flag, a test view)
//   paints the weight taken red in the output (not the history; not with the sharpening).
// - I-0140 (gap 23, a fading body in a fast travel): with `fade`, t5 is the fade mask, 0 where a dissolving model's row drew
//   (scene_pass.h, 13b's gate). Over its 3x3 (the dissolve's holes beside its dots, as tranche 41's difference), the history's whole
//   weight is taken and the pixel's own sample replaces the reconstruction: the output there is the current frame, which breaks into
//   the original's hard dots (i140-e64; the reconstruction with no history kept a veil, i140-e62, and so did the own sample with the
//   weight's last tenth, I-0113 tranche 41's trial). Not where only the reactive difference holds (a fog, an effect): the own sample
//   there would crawl with the jitter (+72 % frame to frame over Archdragon Peak's cloud bank, i140-e62), as the reconstruction does not.
cbuffer Taa:register(b0){
    uint2 size;float2 inv_size;
    float2 jitter;float exposure;float feedback;
    uint reset;float clip_gamma;float sharpen;uint reactive;uint fade;
    float kernel;uint own_reset;   // I-0140 tranche 41 (D-0306): the reconstruction's Gaussian coefficient; a reset frame's own sample
};
Texture2D<float4> current:register(t0);Texture2D<float> depth:register(t1);Texture2D<float2> motion:register(t2);Texture2D<float4> history:register(t3);
Texture2D<float4> base:register(t4);Texture2D<float> fade_mask:register(t5);
RWTexture2D<float4> output:register(u0);RWTexture2D<float4> next_history:register(u1);
SamplerState linear_clamp:register(s0);
float3 Finite(float3 c){return min(max(c,0),65504.0);}   // the step review's M2: NaN to 0, +Inf to fp16's largest
float3 ToYCoCg(float3 c){return float3(dot(c,float3(0.25,0.5,0.25)),dot(c,float3(0.5,0,-0.5)),dot(c,float3(-0.25,0.5,-0.25)));}
float3 FromYCoCg(float3 c){return float3(c.x+c.y-c.z,c.x+c.z,c.x-c.y-c.z);}
float Weight(float3 c){return rcp(1.0+exposure*max(c.r,max(c.g,c.b)));}
float3 History(float2 uv){
    const float2 position=uv*float2(size),centre=floor(position-0.5)+0.5,f=position-centre,f2=f*f,f3=f2*f;
    const float2 w0=-0.5*f3+f2-0.5*f,w1=1.5*f3-2.5*f2+1.0,w2=-1.5*f3+2.0*f2+0.5*f,w3=0.5*f3-0.5*f2,w12=w1+w2;
    const float2 t0=(centre-1.0)*inv_size,t3=(centre+2.0)*inv_size,t12=(centre+w2/w12)*inv_size;
    float4 r=0;
    r+=float4(history.SampleLevel(linear_clamp,float2(t12.x,t0.y),0).rgb,1)*(w12.x*w0.y);
    r+=float4(history.SampleLevel(linear_clamp,float2(t0.x,t12.y),0).rgb,1)*(w0.x*w12.y);
    r+=float4(history.SampleLevel(linear_clamp,t12,0).rgb,1)*(w12.x*w12.y);
    r+=float4(history.SampleLevel(linear_clamp,float2(t3.x,t12.y),0).rgb,1)*(w3.x*w12.y);
    r+=float4(history.SampleLevel(linear_clamp,float2(t12.x,t3.y),0).rgb,1)*(w12.x*w3.y);
    return Finite(r.rgb/max(r.w,1e-4));
}
[numthreads(8,8,1)]
void Resolve(uint3 id:SV_DispatchThreadID){
    if(id.x>=size.x || id.y>=size.y)return;
    const int2 p=int2(id.xy),last=int2(size)-1;
    float3 m1=0,m2=0,lo=1e30,hi=-1e30,sum=0;float wsum=0,closest=2,changed=0,faded=0;int2 nearest=p;
    [unroll]for(int y=-1;y<=1;++y)[unroll]for(int x=-1;x<=1;++x){
        const int2 q=clamp(p+int2(x,y),int2(0,0),last);
        const float3 c=Finite(current[q].rgb),ycc=ToYCoCg(c);
        m1+=ycc;m2+=ycc*ycc;lo=min(lo,ycc);hi=max(hi,ycc);
        const float2 d=float2(x,y)-jitter;
        const float w=exp(-kernel*dot(d,d))*Weight(c);sum+=c*w;wsum+=w;
        const float z=depth[q];if(z<closest){closest=z;nearest=q;}
        if(fade!=0)faded=max(faded,saturate(1.0-fade_mask[q]));
        if(reactive!=0){const float3 a=c*exposure,b=Finite(base[q].rgb)*exposure;
            const float3 delta=abs(a/(1.0+max(a.r,max(a.g,a.b)))-b/(1.0+max(b.r,max(b.g,b.b))));changed=max(changed,max(delta.r,max(delta.g,delta.b)));}}
    const float3 now=reset!=0 && own_reset!=0?Finite(current[p].rgb):sum/max(wsum,1e-6);   // I-0140 tranche 41: an unjittered reset frame as drawn
    const float2 previous=(float2(p)+0.5+motion[nearest])*inv_size;
    float3 result=now;float taken=0;
    if(reset==0 && previous.x>0 && previous.y>0 && previous.x<1 && previous.y<1){
        const float3 mean=m1/9.0,sigma=sqrt(max(m2/9.0-mean*mean,0));
        const float3 box_lo=max(lo,mean-clip_gamma*sigma),box_hi=min(hi,mean+clip_gamma*sigma);
        float3 h=ToYCoCg(History(previous));
        const float3 centre=0.5*(box_lo+box_hi),extent=max(0.5*(box_hi-box_lo),1e-5),offset=h-centre,units=abs(offset/extent);
        const float beyond=max(units.x,max(units.y,units.z));if(beyond>1)h=centre+offset/beyond;
        const float3 old=max(FromYCoCg(h),0);
        float keep=feedback;
        float3 cur=now;
        if(reactive!=0){taken=0.9*smoothstep(0.01,0.03,changed);keep=feedback*(1.0-taken);}
        if(faded>0){taken=max(taken,faded);keep*=1.0-faded;cur=lerp(now,Finite(current[p].rgb),faded);}   // I-0140
        const float wh=keep*Weight(old),wc=(1.0-keep)*Weight(cur);
        result=(old*wh+cur*wc)/max(wh+wc,1e-6);}
    else if(faded>0)result=lerp(now,Finite(current[p].rgb),faded);   // its step review's L1: a reset or an off-screen reprojection too
    if(any(isnan(result)) || any(isinf(result)))result=0;
    if(sharpen<=0)output[p]=float4(reactive==2?lerp(result,float3(1.0/exposure,0,0),taken):result,1);   // tranche 41's view: the weight taken, red
    next_history[p]=float4(result,1);
}
float3 Tone(float3 c){return c*exposure/(1.0+c*exposure);}
float3 Untone(float3 t){return t/(exposure*max(1.0-t,1e-4));}
[numthreads(8,8,1)]
void Sharpen(uint3 id:SV_DispatchThreadID){
    if(id.x>=size.x || id.y>=size.y)return;
    const int2 p=int2(id.xy),last=int2(size)-1;
    const float3 c=Tone(Finite(history.Load(int3(p,0)).rgb));
    const float3 n=Tone(Finite(history.Load(int3(clamp(p+int2(0,-1),int2(0,0),last),0)).rgb)),s=Tone(Finite(history.Load(int3(clamp(p+int2(0,1),int2(0,0),last),0)).rgb));
    const float3 w=Tone(Finite(history.Load(int3(clamp(p+int2(-1,0),int2(0,0),last),0)).rgb)),e=Tone(Finite(history.Load(int3(clamp(p+int2(1,0),int2(0,0),last),0)).rgb));
    const float3 lo=min(c,min(min(n,s),min(w,e))),hi=max(c,max(max(n,s),max(w,e)));
    const float3 amp=sqrt(saturate(min(lo,1.0-hi)/max(hi,1e-5)));
    const float3 k=-amp*(0.2*saturate(sharpen));
    // The final review's H1: bounded to the cross's own range (tone space), so an edge is steepened but never overshoots: a highlight
    // is not pushed toward the inverse tone's limit, a dark side not to black.
    const float3 r=clamp((c+(n+s+w+e)*k)/(1.0+4.0*k),lo,hi);
    float3 result=Untone(r);
    if(any(isnan(result)) || any(isinf(result)))result=0;
    output[p]=float4(result,1);
}
[numthreads(8,8,1)]
void Mirror(uint3 id:SV_DispatchThreadID){
    if(id.x>=size.x || id.y>=size.y)return;
    output[id.xy]=next_history[id.xy];
}
