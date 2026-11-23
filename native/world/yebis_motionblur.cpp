// W10 MB-1: see yebis_motionblur.h. The arithmetic keeps the original's single-precision operations and their order.
#include "yebis_motionblur.h"
#include <cmath>
#include <cstring>
#include <utility>

namespace ds3native::yebis {
namespace {
constexpr float one=1.0f,two=2.0f,half=0.5f,minus_one=-1.0f;   // [143D7EE10], [143D7EE7C], [143D7EDB4], [143D7F0C8]
void Put(GlareDraw& g,uint32_t word,float value){PutGlareWord(g,word,value);}
void Put4(GlareDraw& g,uint32_t word,float a,float b,float c,float e){Put(g,word,a);Put(g,word+1,b);Put(g,word+2,c);Put(g,word+3,e);}
}
// FUN_1416D7080 (log, ceil, pow: the CRT's FUN_141FDD060, FUN_141FD2700, FUN_141FD3430, INFERRED from their use).
MotionBlurPlanned PlanMotionBlur(const MotionBlurData& d,float length,int base,int parity,int max_passes,float interleave){
    const float n_base=float(base);
    if(one<interleave && length<=n_base*interleave)interleave=(length/n_base<=one)?one:length/n_base;
    MotionBlurPlanned p;const float spread=length/interleave+one;
    if(spread<=d.no_blur)return p;
    const float ratio=std::log(spread)/std::log(n_base);
    p.passes=int(std::ceil(ratio<=one?one:ratio));
    p.taps=int(std::ceil((spread*n_base)/std::pow(n_base,float(p.passes))));
    if(p.taps==0)p.taps=base;
    if(parity==1 && p.taps%2==0)++p.taps;
    else if(parity==2 && p.taps%2!=0)++p.taps;
    if(base<p.taps)p.taps=base;
    if(0<max_passes && max_passes<p.passes){p.passes=max_passes;p.taps=base;}
    p.step=length/(std::pow(n_base,float(p.passes-1))*float(p.taps)-one);
    return p;}

Matrix4 NormaliseRows3(const Matrix4& m) noexcept {
    Matrix4 out=m;
    for(int r=0;r<3;++r){const float x=m[4*r],y=m[4*r+1],z=m[4*r+2];const float sum=x*x+y*y+z*z;
        if(sum==0.0f || sum==one)continue;
        const float s=one/std::sqrt(sum);out[4*r]=x*s;out[4*r+1]=s*y;out[4*r+2]=s*z;}
    return out;}

Matrix4 Multiply(const Matrix4& a,const Matrix4& b) noexcept {
    Matrix4 out{};
    for(int i=0;i<4;++i){const float a0=a[4*i],a1=a[4*i+1],a2=a[4*i+2],a3=a[4*i+3];
        for(int j=0;j<4;++j)out[4*i+j]=((b[j]*a0+b[4+j]*a1)+b[8+j]*a2)+b[12+j]*a3;}
    return out;}

bool Inverse(const Matrix4& input,Matrix4& out) noexcept {
    float m[16],inv[16]{};std::memcpy(m,input.data(),sizeof(m));inv[0]=inv[5]=inv[10]=inv[15]=one;
    int perm[4]{0,1,2,3};
    for(int k=0;k<4;++k){
        int best=k;for(int j=k+1;j<4;++j)if(std::fabs(m[4*k+perm[best]])<std::fabs(m[4*k+perm[j]]))best=j;
        if(m[4*k+perm[best]]==0.0f)return false;
        std::swap(perm[k],perm[best]);const int pivot=perm[k];
        const float f=one/m[4*k+pivot];
        for(int r=0;r<4;++r){if(k<=r)m[4*r+pivot]=f*m[4*r+pivot];inv[4*r+pivot]=f*inv[4*r+pivot];}
        for(int i=0;i<4;++i){if(i==k)continue;const int c=perm[i];const float g=m[4*k+c]/m[4*k+pivot];
            for(int r=0;r<4;++r){if(k<=r)m[4*r+c]=m[4*r+c]-g*m[4*r+pivot];inv[4*r+c]=inv[4*r+c]-g*inv[4*r+pivot];}}}
    for(int r=0;r<4;++r)for(int j=0;j<4;++j)out[4*r+j]=inv[4*r+perm[j]];
    return true;}

Matrix4 Perspective(float x0,float x1,float y0,float y1,float near_plane,float far_plane) noexcept {
    Matrix4 p{};const float s=one,n2=near_plane*two;
    p[0]=n2/(x1-x0);p[5]=n2/(y1-y0);p[8]=((x0+x1)/(x0-x1))*s;
    const float depth=far_plane-near_plane;
    p[9]=((y0+y1)/(y0-y1))*s;p[11]=s;p[10]=(far_plane/depth)*s;p[14]=-((near_plane*far_plane)/depth);
    return p;}

MotionBlurPlan BuildMotionBlur(const MotionBlurData& d,const MotionBlurInputs& in){
    MotionBlurPlan plan;plan.width=in.width;plan.height=in.height;
    if(!in.width || !in.height || !in.velocity_width || !in.velocity_height){plan.refused="extent";return plan;}
    if(!(in.time_ratio>0.0f) || !(in.max_length>0.0f)){plan.refused="no blur (BlurTimeRatio or MaxBlurLength not positive)";return plan;}
    if(in.depth_channel!=0){plan.refused="depth channel other than R";return plan;}
    // FUN_1416E0160 / FUN_1416E21E0: the current and previous projections from the library's camera (the previous takes the current
    // near and far planes).
    const auto& c=in.camera;
    const auto projection=[&](float tan_half,const std::array<float,2>& shift){
        const float n=c.near_plane,wide=tan_half*c.aspect*n,sx=shift[0]*wide,tall=n*tan_half,sy=shift[1]*tall;
        return Perspective(sx+-wide,sx+wide,-tall+sy,sy+tall,n,c.far_plane);};
    const Matrix4 current_projection=projection(c.tan_half,c.shift),previous_projection=projection(c.previous_tan_half,c.previous_shift);
    // FUN_141730F30: the poses with their first three rows normalised, the previous-clip transform inv(inv(Pc) Ac inv(Ap) Pp).
    const Matrix4 a=NormaliseRows3(c.current),b=NormaliseRows3(c.previous);
    Matrix4 inv_pc{},inv_b{},transform{};
    if(!Inverse(current_projection,inv_pc) || !Inverse(b,inv_b)){plan.refused="singular camera";return plan;}
    if(!Inverse(Multiply(Multiply(inv_pc,a),Multiply(inv_b,previous_projection)),transform)){plan.refused="singular camera transform";return plan;}
    plan.transform=transform;
    // FUN_1416D12C0 (the near and far planes back from the projection) and FUN_1416D15D0 (the aspect (1 / P00) / (1 / P11)).
    {const float s=current_projection[11]>0.0f?one:minus_one,p10=s*current_projection[10],p14=current_projection[14];
        plan.far_factor=-(p14/(p10-one));plan.near_factor=-(p14/p10);}
    const float aspect=(one/current_projection[0])/(one/current_projection[5]);
    // The velocity draw (FUN_141724410, application velocity present, no falloff): the encode pair p17 = (0.5, 0.5) of the
    // application-overwrite source (S+0xA10 = 1), p16 = ((0.5 / s.x) aspect, -0.5 / s.y).
    const std::array<float,2> p16{(half/in.velocity_scale[0])*aspect,-0.5f/in.velocity_scale[1]},p17{half,half};
    plan.surfaces.push_back({in.velocity_width,in.velocity_height,true});   // 0: the internal velocity (RGBA16F)
    {GlareDraw g;g.kind=GlareKind::Composite;g.technique="tech_MotionBlurMakeCameraVelocityOverwrite_R";g.target=0;g.inputs[0]=-4;g.inputs[1]=-6;g.border=true;
        Put4(g,0,one,0.0f,one,0.0f);Put(g,20,one);Put(g,21,minus_one);
        Put4(g,60,plan.near_factor,plan.far_factor,plan.far_factor*plan.near_factor,plan.far_factor-plan.near_factor);
        Put4(g,616,p16[0],p16[1],p17[0],p17[1]);Put4(g,620,in.object_decode[0],in.object_decode[1],0.0f,0.0f);
        Put4(g,624,one/p16[0],one/p16[1],one/p17[0],-(p17[1]/p17[0]));
        for(int col=0;col<4;++col)for(int row=0;row<4;++row)Put(g,896+4*uint32_t(col)+uint32_t(row),transform[4*row+col]);
        // vs_2D_Tex2D3: the covering triangle with TEXCOORD0 the texture coordinate, TEXCOORD1 its clip-space position, TEXCOORD2 = TEXCOORD0.
        g.texcoords=3;g.vertices={-1,1,0,1,-1,-1,0,1, -1,-3,0,-1,-1,3,0,-1, 3,1,2,1,3,-1,2,1};
        plan.draws.push_back(std::move(g));}
    // FUN_14172FBD0 / FUN_1416D7080: L = H x MaxBlurLength, N from NumBaseSamples and the parity, R capped by FUN_1416EADA0.
    int base=in.base_samples<2?5:in.base_samples;const int parity=in.bidirectional?1:2;
    base=parity==1?(base/2)*2+1:((base+1)/2)*2;
    if(base<2 || 8<base){plan.refused="blur taps outside 2 to 8 (the library's techniques)";return plan;}
    const int allowance=in.capability;int max_passes=in.max_recurrences;
    if(allowance<=1){const int cap=allowance==1?2:0;if(max_passes<1 || cap<max_passes)max_passes=cap;}
    const float length=float(in.height)*in.max_length;
    const auto planned=PlanMotionBlur(d,length,base,parity,max_passes,in.interleave);
    plan.base_taps=base;plan.passes=planned.passes;plan.taps=planned.taps;plan.step=planned.step/length;
    if(planned.passes<=0){plan.refused="no blur pass";return plan;}
    if(planned.taps<2 || 8<planned.taps){plan.refused="last pass taps outside 2 to 8 (the library's techniques)";return plan;}
    // FUN_1417301D0's constants: O0 = the velocity decode (2, -1) x s x BlurTimeRatio, the depth test S+0xA04 in the scene alpha's
    // units (FUN_1416F4AF0(2, 0): 1 / its scale), the threshold 2 / H, the rectangle (0, 0, 1, 1) remapped to (2, 2, -1, -1).
    const float sx=in.velocity_scale[0]*in.time_ratio,sy=in.velocity_scale[1]*in.time_ratio,ex=two,ey=minus_one,e=ex*0.0f+ey;
    const std::array<float,4> o0{ex*sx,ex*sy,sx*e+0.0f,sy*e+0.0f};
    float depth_test=0.0f;
    {const float scale=one/in.scene_decode[0],shift=-(in.scene_decode[1]/in.scene_decode[0]);
        if(!(0.0f<in.depth_threshold_far))depth_test=scale*in.depth_threshold;
        else if(in.depth_threshold!=0.0f){plan.refused="far depth threshold (S+0xA08 > 0)";(void)shift;return plan;}}
    const float threshold=two/float(in.height),ratio=one/aspect;   // fStack_488 = (W x 1 + 0) / aspect over the source's unit extent
    float step=plan.step;
    for(int pass=0;pass<planned.passes;++pass){const bool last=pass==planned.passes-1;const int taps=last?planned.taps:base;
        GlareDraw g;g.kind=GlareKind::Blur;g.technique="tech_MotionBlurVelocityDirDepthInnerscreen_SM3_T2_S"+std::to_string(taps);
        g.inputs[0]=pass==0?-1:int8_t(1+(pass-1)%2);g.inputs[1]=0;g.address[0]=3;g.address[1]=2;   // D3D12_TEXTURE_ADDRESS_MODE CLAMP, MIRROR
        g.target=int8_t(1+pass%2);if(plan.surfaces.size()<=size_t(g.target))plan.surfaces.push_back({in.width,in.height,true});
        if(last)plan.output=g.target;
        Put(g,20,one);Put(g,21,minus_one);
        const float w=one*(one/float(taps));float sum[4]{};
        for(int k=0;k<taps;++k){Put4(g,616+4*uint32_t(k),w,w,w,w);for(float& s:sum)s=s+w;}
        Put4(g,616+4*uint32_t(taps),sum[0],sum[1],sum[2],sum[3]);
        const float px=step*ratio,py=step*one;
        Put4(g,744,o0[0],o0[1],o0[2],o0[3]);Put4(g,748,px*o0[0],py*o0[1],px*o0[2]+0.0f,py*o0[3]+0.0f);
        Put4(g,752,px,py,threshold,0.0f);Put(g,756,in.similarity);Put(g,757,depth_test);Put(g,758,0.0f);
        const float rx=two/(one-0.0f),ry=two/(one-0.0f);Put4(g,760,rx,ry,minus_one-rx*0.0f,minus_one-ry*0.0f);
        plan.draws.push_back(std::move(g));
        step=step*float(base);}
    return plan;}
}
