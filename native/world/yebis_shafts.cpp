// W10 LS-1: see yebis_shafts.h. The arithmetic keeps the original's single-precision operations and their order.
#include "yebis_shafts.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <emmintrin.h>
#include "producers.h"

namespace ds3native::yebis {
std::array<float,4> DepthConversion(const DepthSpaces& d,int from,int to,float scale,float bias) noexcept {
    if(from==6)from=d.source;
    if(to==6)to=d.source;
    constexpr float one=1.0f;
    float a=one,b=0.0f;
    if(from!=to){
        const int lo=std::min(from,to),hi=std::max(from,to);
        if(lo<1 && hi>0){a=((d.far_plane-d.near_plane)*d.c48)/(d.far_plane*d.near_plane);b=a*0.0f+(one-d.c48/d.near_plane);}
        if(lo<2 && hi>1){a=a*d.c38;b=d.c38*b+d.c3c;}
        if(lo<3 && hi>2){a=a*d.c40*d.space3;b=(d.c40*b+d.c44)*d.space3+0.0f;}
        if(lo==to){b=-(b/a);a=one/a;}   // the inverse direction
    }
    if(scale!=one || bias!=0.0f){const float shift=-(bias/scale)*a;a=a*(one/scale);b=b+shift;}
    return {a,b,one/a,-(b/a)};
}
DepthSpaces DofDepthSpaces(float near_plane,float far_plane,float focus,const std::array<float,4>& dof_words) noexcept {
    DepthSpaces d;d.near_plane=near_plane;d.far_plane=far_plane;d.c48=focus;d.c38=dof_words[0];d.c3c=dof_words[1];
    d.c40=1.0f/d.c38;d.c44=-(d.c3c/d.c38);
    return d;
}
DepthSpaces NearDepthSpaces(float near_plane,float far_plane) noexcept {
    DepthSpaces d;d.near_plane=near_plane;d.far_plane=far_plane;d.c48=near_plane;d.c38=1.0f;d.c3c=0.0f;d.c40=1.0f/d.c38;d.c44=-(d.c3c/d.c38);
    return d;
}
DepthSpaces RecordDepthSpaces(const DepthSpaces& current,const DepthSpaces* previous) noexcept {
    DepthSpaces d=current;
    if(previous){d.c38=previous->c38;d.c3c=previous->c3c;d.c40=previous->c40;d.c44=previous->c44;d.c48=previous->c48;}
    return d;
}
AlphaRBlock AlphaR(const AlphaRInputs& in) noexcept {
    const auto d=DepthConversion(in.spaces,6,2,1.0f,0.0f);
    const float scale=in.source_object?in.source_scale:1.0f,bias=in.source_object?in.source_bias:0.0f;
    AlphaRBlock block;block.scale=d[0]*scale;block.offset=d[1]+bias*d[0];block.colour={in.colour,in.colour,in.colour,1.0f};
    return block;
}
}

namespace ds3native::yebis {
namespace {
constexpr float one=1.0f,two=2.0f,half=0.5f,quarter=0.25f,four=4.0f;   // [143D7EE10], [143D7EE7C], [143D7EDB4], [143D7ED9C], [143D7EED8]
void Put(ShaftDraw& d,uint32_t word,float value){d.words.emplace_back(word,value);}
// FUN_141718C00's covering triangle (the flag bit 16 clear): the target's rectangle in clip space, the right edge and the bottom
// pushed out by its width and height (x' = (x1 - x0) 2 + x0).
void Positions(ShaftDraw& d,const ShaftSurface& target){
    const float w=float(target.width),h=float(target.height),sx=two/w,sy=two/h;
    float left=0.0f-w*half,top=0.0f-h*half;float right=left+w,bottom=top+h;
    right=(right-left)*two+left;top=(top-bottom)*two+bottom;
    const float x0=(left+0.0f)*sx,x1=(right+0.0f)*sx,y0=(bottom+0.0f)*sy,y1=(top+0.0f)*sy;
    d.vertices[0]={x0,y0};d.vertices[1]={x0,y1};d.vertices[2]={x1,y0};}
// One rectangle of texture coordinates from (start) to (end) on the same triangle: (sx, ey), (sx, (sy - ey) 2 + ey), ((ex - sx) 2 + sx, ey).
void Texcoords(ShaftDraw& d,float sx,float sy,float ex,float ey){
    const float x1=(ex-sx)*two+sx,y1=(sy-ey)*two+ey;
    d.vertices[0].insert(d.vertices[0].end(),{sx,ey});d.vertices[1].insert(d.vertices[1].end(),{sx,y1});d.vertices[2].insert(d.vertices[2].end(),{x1,ey});
    ++d.texcoords;}
const ShaftSurface& Surface(const ShaftInputs& in,ShaftSlot s){
    switch(s){case ShaftSlot::MaskSource:return in.mask_source;case ShaftSlot::Ping0:return in.pingpong[0];case ShaftSlot::Ping1:return in.pingpong[1];
    case ShaftSlot::Output:return in.output;default:return in.shaft;}}
}
ShaftPlan BuildShaft(const ShaftInputs& in){
    ShaftPlan plan;const auto& r=in.record;
    if(r[9]!=0.0f){plan.refused="record +0x24 (the FUN_141737480 composite): no witness";return plan;}
    if(!in.float_surface){plan.refused="an 8-bit shaft surface (the 1/255 base): no witness";return plan;}
    for(const auto* s:{&in.mask_source,&in.shaft,&in.output,&in.pingpong[0],&in.pingpong[1]})
        if(!s->width || !s->height){plan.refused="a shaft surface without an extent";return plan;}
    const float sun_x=r[17],sun_y=r[18];
    // The mask (FUN_1417290B0): $Globals c15 (handle +0xDC) and c10 (+0x11C); t0 the glare source, t1 the film grain.
    {ShaftDraw d;d.technique="tech_LightShaftMask_SM3";d.inputs={ShaftSlot::MaskSource,ShaftSlot::FilmGrain};d.target=ShaftSlot::Shaft;
        Put(d,20,1.0f);Put(d,21,-1.0f);
        Put(d,60,r[5]);Put(d,61,r[4]);Put(d,62,in.exposure/in.factor);Put(d,63,r[19]);
        Put(d,40,256.0f);Put(d,41,r[6]);Put(d,42,sun_y+sun_x+0.0f);Put(d,43,0.0f);
        const float W=float(in.shaft.width),H=float(in.shaft.height),aspect=W/H;
        Positions(d,in.shaft);
        const auto& src=in.mask_source;
        Texcoords(d,0.0f,0.0f,float(src.width)*(one/float(src.width))+0.0f,float(src.height)*(one/float(src.height))+0.0f);
        Texcoords(d,0.0f,0.0f,aspect*four,four);
        float size=std::tan(in.fov*half);size=r[8]*0.125f*size;
        float sx_scale=size*aspect;float sx_bias=-sx_scale;const float sy_scale=(size-(-size))/(one-0.0f);
        sx_scale=(sx_scale-sx_bias)/(one-0.0f);sx_bias=sx_bias-sx_scale*0.0f;const float sy_bias=-size-sy_scale*0.0f;
        Texcoords(d,sx_scale*((0.0f-sun_x)+half)+sx_bias,sy_scale*((0.0f-sun_y)+half)+sy_bias,
            sx_scale*((one-sun_x)+half)+sx_bias,sy_scale*((one-sun_y)+half)+sy_bias);
        plan.draws.push_back(std::move(d));}
    // The radial blurs (FUN_141750620): the pass count, the last pass's taps and the tap weights.
    const float reach=(one/(in.tan_half*two))*(float(in.base_width)*0.003125f);           // FUN_141751CB0(glare, 1)
    float spacing=(0.0625f/reach)/r[7];
    const float base=0.001f;                                                              // [143D7ED10]: a float shaft surface
    float length=std::log(base);length=length/std::log(half);length=length/spacing;
    // Review: a degenerate length (rec+0x1C 0, an infinite or non-positive spacing) has no pass to compute; refused before the integer
    // conversions. Lengths up to 4^8 x 4 keep the conversions in range.
    if(!(length>1.0f) || !(length<262144.0f)){plan.refused="no radial pass";return plan;}
    int passes=int(std::ceil(std::log(length)/std::log(four)));if(passes>=8)passes=8;   // [142B63164]
    int last=int(std::ceil((length*four)/std::pow(four,float(passes))));last=last==0?4:std::min(last,4);
    if(passes<1){plan.refused="no radial pass";return plan;}
    if(last<2){plan.refused="a one-tap last pass (tech_TexCoordMadT1_S1, unpinned): no witness";return plan;}
    plan.passes=passes;plan.last_taps=last;
    const float scale=std::pow(in.tan_half*quarter,one/float(passes));
    const float factor=((((in.factor*0.035f)*in.shape_bloom)*in.luminance)*quarter)*four;   // [142899374] 0.035
    std::array<float,4> colour{r[0],r[1],r[2],r[3]};
    if(r[13]!=one || r[14]!=one || r[15]!=one){colour[0]=colour[0]*r[13];colour[1]=colour[1]*r[14];colour[2]=colour[2]*r[15];}
    ShaftSlot source=ShaftSlot::Shaft;int ping=0;
    for(int pass=0;pass<passes;++pass){
        const bool final_pass=pass==passes-1;
        const float w0=pass==0?scale*quarter:scale;
        std::array<float,4> weight{};for(int k=0;k<4;++k)weight[size_t(k)]=std::pow(half,float(k)*spacing)*w0;
        const int taps=std::min(final_pass?last:4,4);
        const bool into_output=final_pass || taps<4;
        ShaftDraw d;d.technique="tech_TexCoordMadT1_S"+std::to_string(taps);d.inputs={source,ShaftSlot::None};
        d.target=into_output?ShaftSlot::Output:(ping==0?ShaftSlot::Ping0:ShaftSlot::Ping1);
        Put(d,20,1.0f);Put(d,21,-1.0f);
        const auto& src=Surface(in,source);const auto& dst=Surface(in,d.target);
        const float end_x=float(src.width)*(one/float(src.width))+0.0f,end_y=float(src.height)*(one/float(src.height))+0.0f;
        const float height=float(dst.height);
        const float sx=((float(src.width)*sun_x)*(one/float(src.width)))+0.0f,sy=((float(src.height)*sun_y)*(one/float(src.height)))+0.0f;
        const float step=float(int(std::pow(four,float(pass))));
        const float to_x0=0.0f-sx,to_y0=0.0f-sy,to_x1=end_x-sx,to_y1=end_y-sy;
        Positions(d,dst);
        for(int k=0;k<taps;++k){
            std::array<float,4> v{weight[size_t(k)],weight[size_t(k)],weight[size_t(k)],weight[size_t(k)]};
            if(into_output)for(size_t c=0;c<4;++c)v[c]=factor*(colour[c]*v[c]);
            for(uint32_t c=0;c<4;++c)Put(d,616+4*uint32_t(k)+c,v[c]);
            const float t=one/((float(k)*step+height)/height);
            Texcoords(d,sx+to_x0*t,sy+to_y0*t,sx+to_x1*t,sy+to_y1*t);}
        plan.draws.push_back(std::move(d));
        source=plan.draws.back().target;ping^=1;spacing=spacing*four;
        if(taps<4)break;}
    return plan;
}
}
namespace ds3native::yebis {
std::array<float,20> ShaftRecordFromSlot(const std::array<float,22>& slot,const DepthSpaces& spaces) noexcept {
    std::array<float,20> record{};for(size_t i=0;i<19;++i)record[i]=slot[i];
    const auto d=DepthConversion(spaces,6,2,1.0f,0.0f);record[19]=slot[19]*d[0]+d[1];
    return record;
}
}
namespace ds3native::yebis {
namespace {
// SHUFPS a,b,imm: lanes 0-1 from a, lanes 2-3 from b.
inline __m128 Shuf(__m128 a,__m128 b,int imm){
    alignas(16) float x[4],y[4],r[4];_mm_store_ps(x,a);_mm_store_ps(y,b);
    r[0]=x[imm&3];r[1]=x[(imm>>2)&3];r[2]=y[(imm>>4)&3];r[3]=y[(imm>>6)&3];return _mm_load_ps(r);}
inline __m128 Row(const std::array<float,16>& m,int r){return _mm_loadu_ps(m.data()+4*r);}
inline float GlobalWord(const ShaftDriverInputs& in,uint32_t offset){float v{};std::memcpy(&v,&in.globals[offset/4],4);return v;}
}
std::array<float,22> DriverSlot(const ShaftDriverInputs& in) noexcept {
    const __m128 k2500=_mm_setr_ps(0,0,0,1),k480=_mm_setr_ps(1,1,-1,1),k4A0=_mm_setr_ps(1,-1,1,1),k080=_mm_setr_ps(0,0,0,1),
        kF90=_mm_setr_ps(1,0,0,0),kFA0=_mm_setr_ps(0,1,0,0),kF370=_mm_setr_ps(0,1,0,0),k120=_mm_set1_ps(1.0f),
        kF6F0=_mm_setr_ps(0.5f,-0.5f,1.0f,0.0f),kF2F0=_mm_setr_ps(0.5f,0.5f,0.0f,0.0f),k170=_mm_castsi128_ps(_mm_set1_epi32(0x7FC00000)),
        k160=_mm_castsi128_ps(_mm_set1_epi32(0x7F800000)),k720=_mm_castsi128_ps(_mm_setr_epi32(-1,-1,-1,0)),
        sign=_mm_castsi128_ps(_mm_set1_epi32(int(0x80000000u)));
    // [143D7ED40] degrees to radians, [143D7EEC0] pi, [143D7EDA0] the visibility bias, [143D7EE0C] the depth cap.
    constexpr float degrees=0.0174532924f,pi=3.14159274f,bias=0.3f,depth_cap=0.999970019f;
    // FUN_14004BCA0: the view (the pose's inverse), rows 0..2.
    std::array<float,16> view{};ds3prod::InverseMatrix(in.pose.data(),view.data());
    const __m128 x15=Row(view,0),x14=Row(view,1),x13=Row(view,2),x12=k2500;
    // The direction's rotations: the azimuth (+ pi) about y, the elevation about x (1400E7463..1400E75FA).
    __m128 x8=_mm_set_ss(GlobalWord(in,0x164)*degrees+pi);
    __m128 x11=_mm_set_ss(std::sin(_mm_cvtss_f32(x8)));
    __m128 x6=_mm_set_ss(std::cos(_mm_cvtss_f32(x8)));
    x11=Shuf(x11,x6,0xCC);
    x6=_mm_set_ss(GlobalWord(in,0x160)*degrees);
    x8=Shuf(x11,x11,0xC6);x8=_mm_mul_ps(x8,k480);
    __m128 x10=_mm_set_ss(std::cos(_mm_cvtss_f32(x6)));
    __m128 x7=_mm_set_ss(std::sin(_mm_cvtss_f32(x6)));
    x6=k080;__m128 x2=kF90;const __m128 x5=kFA0;
    __m128 x9=x2,x0=x2;
    x10=Shuf(x10,x7,0xC3);x0=Shuf(x0,x2,0x55);x9=Shuf(x9,x2,0xFF);
    __m128 x1=x2,x3=x10;x7=x10;
    x0=_mm_mul_ps(x0,x5);x9=_mm_mul_ps(x9,x6);x9=_mm_add_ps(x9,x0);
    x0=x10;x1=Shuf(x1,x2,0x00);x3=Shuf(x3,x10,0xD8);x0=Shuf(x0,x10,0x00);
    x3=_mm_mul_ps(x3,k4A0);x1=_mm_mul_ps(x1,x8);x0=_mm_mul_ps(x0,x8);
    __m128 x4=x3;x4=Shuf(x4,x3,0x00);x7=Shuf(x7,x10,0xAA);x2=Shuf(x2,x2,0xAA);
    x7=_mm_mul_ps(x7,x11);x2=_mm_mul_ps(x2,x11);x4=_mm_mul_ps(x4,x8);
    x1=_mm_add_ps(x1,x2);x7=_mm_add_ps(x7,x0);x0=x3;x9=_mm_add_ps(x9,x1);
    x0=Shuf(x0,x3,0xAA);x1=x10;x1=Shuf(x1,x10,0xFF);x0=_mm_mul_ps(x0,x11);x10=Shuf(x10,x10,0x55);
    x4=_mm_add_ps(x4,x0);x0=x6;x1=_mm_mul_ps(x1,x6);x10=_mm_mul_ps(x10,x5);x0=Shuf(x0,x6,0x55);
    x1=_mm_add_ps(x1,x10);x7=_mm_add_ps(x7,x1);x1=x3;x1=Shuf(x1,x3,0xFF);x3=Shuf(x3,x3,0x55);
    x3=_mm_mul_ps(x3,x5);x1=_mm_mul_ps(x1,x6);x1=_mm_add_ps(x1,x3);x3=x6;x3=Shuf(x3,x6,0xFF);
    x4=_mm_add_ps(x4,x1);x3=_mm_mul_ps(x3,x6);x0=_mm_mul_ps(x0,x5);x1=x6;x1=Shuf(x1,x6,0x00);x6=Shuf(x6,x6,0xAA);
    x3=_mm_add_ps(x3,x0);
    x0=_mm_set_ss(GlobalWord(in,0x24C));x0=_mm_xor_ps(x0,sign);
    x1=_mm_mul_ps(x1,x8);x8=_mm_setzero_ps();x2=x8;x2=_mm_unpacklo_ps(x2,x0);x6=_mm_mul_ps(x6,x11);x2=_mm_unpacklo_ps(x2,kF370);
    x1=_mm_add_ps(x1,x6);x3=_mm_add_ps(x3,x1);
    __m128 d=x2;x0=x2;x1=x2;x0=Shuf(x0,x2,0xAA);x1=Shuf(x1,x2,0x55);d=Shuf(d,x2,0xFF);
    x0=_mm_mul_ps(x0,x4);d=_mm_mul_ps(d,x3);x1=_mm_mul_ps(x1,x7);d=_mm_add_ps(d,x0);x2=Shuf(x2,x2,0x00);x4=_mm_setzero_ps();
    x2=_mm_mul_ps(x2,x9);x1=_mm_add_ps(x1,x2);d=_mm_add_ps(d,x1);                 // the world direction (CPostEffect +0x21A0)
    // View space, then the projection and the screen (1400E7632..1400E76D0).
    x3=d;x0=d;x1=d;x0=Shuf(x0,d,0xAA);x3=Shuf(x3,d,0xFF);x1=Shuf(x1,d,0x55);
    x0=_mm_mul_ps(x0,x13);x3=_mm_mul_ps(x3,x12);x1=_mm_mul_ps(x1,x14);x3=_mm_add_ps(x3,x0);
    x0=d;x0=Shuf(x0,d,0x00);x0=_mm_mul_ps(x0,x15);x1=_mm_add_ps(x1,x0);x3=_mm_add_ps(x3,x1);
    x2=x3;x0=x3;x1=x3;x0=Shuf(x0,x3,0xAA);x1=Shuf(x1,x3,0x55);x2=Shuf(x2,x3,0xFF);
    x0=_mm_mul_ps(x0,Row(in.projection,2));x1=_mm_mul_ps(x1,Row(in.projection,1));x2=_mm_mul_ps(x2,Row(in.projection,3));x2=_mm_add_ps(x2,x0);
    x0=k120;x3=Shuf(x3,x3,0x00);x3=_mm_mul_ps(x3,Row(in.projection,0));x1=_mm_add_ps(x1,x3);x2=_mm_add_ps(x2,x1);
    x1=x2;x1=Shuf(x1,x2,0xFF);x1=Shuf(x1,x1,0x00);x0=_mm_div_ps(x0,x1);x0=_mm_mul_ps(x0,x2);x0=_mm_mul_ps(x0,kF6F0);x0=_mm_add_ps(x0,kF2F0);
    alignas(16) float screen[4];_mm_store_ps(screen,x0);
    // The visibility: the normalised direction (w masked) against the pose's third row (1400E76D7..1400E77EC).
    x0=k720;d=_mm_and_ps(d,x0);x0=_mm_andnot_ps(x0,x4);d=_mm_or_ps(d,x0);
    x3=d;x3=_mm_mul_ps(x3,d);x0=x3;x0=Shuf(x0,x3,0xEE);x3=_mm_add_ps(x3,x0);x3=Shuf(x3,x3,0x40);x0=Shuf(x0,x3,0xF0);
    x2=k160;x3=_mm_add_ps(x3,x0);x3=Shuf(x3,x3,0xAA);x1=_mm_sqrt_ps(x3);x0=x4;x2=_mm_cmpneq_ps(x2,x3);d=_mm_div_ps(d,x1);
    x0=_mm_cmpneq_ps(x0,x1);d=_mm_and_ps(d,x0);d=_mm_and_ps(d,x2);x2=_mm_andnot_ps(x2,k170);d=_mm_or_ps(d,x2);
    const __m128 forward=Row(in.pose,2);
    x2=forward;x0=d;x2=Shuf(x2,x4,0xDC);x2=Shuf(x2,forward,0xE8);x0=Shuf(x0,x4,0xDC);x0=Shuf(x0,d,0xE8);
    x2=_mm_mul_ps(x2,x0);x1=x2;x1=Shuf(x1,x2,0x99);x2=_mm_add_ss(x2,x1);x0=x1;x0=Shuf(x0,x1,0x55);x2=_mm_add_ss(x2,x0);
    x2=Shuf(x2,x2,0x00);x2=_mm_add_ss(x2,_mm_set_ss(bias));
    const float dot=_mm_cvtss_f32(x2);const float clamped=0.0f>dot?0.0f:(dot<=1.0f?dot:1.0f);
    const float v=(1.0f-clamped)*0.0f+clamped;
    // The setters' words (slot +4 x index).
    std::array<float,22> slot{};
    const float z=screen[2];const float depth=0.0f>z?0.0f:(z<=depth_cap?z:depth_cap);
    const float f=in.colour[3];slot[0]=v*(f*in.colour[0]);slot[1]=v*(f*in.colour[1]);slot[2]=v*(f*in.colour[2]);slot[3]=v*f;
    slot[7]=GlobalWord(in,0x144);slot[4]=GlobalWord(in,0x148);slot[19]=depth;std::memcpy(&slot[21],&in.globals[0x150/4],4);
    slot[8]=GlobalWord(in,0x154);slot[5]=GlobalWord(in,0x158);slot[6]=GlobalWord(in,0x15C);
    const float f2=in.colour2[3];slot[13]=v*(f2*in.colour2[0]);slot[14]=v*(f2*in.colour2[1]);slot[15]=v*(f2*in.colour2[2]);slot[16]=v*f2;
    slot[9]=GlobalWord(in,0x168);slot[12]=GlobalWord(in,0x16C);slot[10]=GlobalWord(in,0x170);slot[11]=GlobalWord(in,0x174);
    slot[17]=screen[0];slot[18]=screen[1];
    const uint32_t enable=in.globals[0x130/4]!=0?1u:0u;std::memcpy(&slot[20],&enable,4);
    return slot;
}
}
namespace ds3native::yebis {
std::vector<std::vector<uint8_t>> FilmGrainLevels(const std::vector<uint8_t>& level0){
    std::vector<std::vector<uint8_t>> levels;if(level0.size()!=size_t(film_grain_size)*film_grain_size*4)return levels;
    static constexpr float dither[16]{0.0f,2.0f,0.5f,2.5f,3.0f,1.0f,3.5f,1.5f,0.75f,2.75f,0.25f,2.25f,3.75f,1.75f,3.25f,1.25f};
    levels.push_back(level0);uint32_t w=film_grain_size,h=film_grain_size;
    while(w>1 || h>1){
        const uint32_t dw=std::max(w>>1,1u),dh=std::max(h>>1,1u);const bool dithered=std::max(dw,dh)>=4;
        const auto& src=levels.back();std::vector<uint8_t> dst(size_t(dw)*dh*4);
        for(uint32_t y=0;y<dh;++y)for(uint32_t x=0;x<dw;++x){
            const uint32_t x0=dw<w?2*x:x,x1=dw<w?2*x+1:x,y0=dh<h?2*y:y,y1=dh<h?2*y+1:y;
            const float offset=dithered?dither[(y&3)+4*(x&3)]:2.0f;
            for(uint32_t c=0;c<4;++c){
                const uint32_t sum=uint32_t(src[(size_t(y1)*w+x1)*4+c])+src[(size_t(y1)*w+x0)*4+c]+src[(size_t(y0)*w+x1)*4+c]+src[(size_t(y0)*w+x0)*4+c];
                dst[(size_t(y)*dw+x)*4+c]=uint8_t(int64_t((offset+float(sum))*0.25f));}}
        levels.push_back(std::move(dst));w=dw;h=dh;}
    return levels;
}
}
namespace ds3native::yebis {
namespace {
void PutWord(GlareDraw& g,uint32_t word,float value){PutGlareWord(g,word,value);}
}
GlareDraw AlphaRDraw(const AlphaRBlock& block){
    GlareDraw g;g.kind=GlareKind::Downsample;g.technique="tech_Alpha_R";g.inputs[0]=-4;g.target=-1;g.write_mask=8;g.point[0]=true;g.border=true;
    PutWord(g,20,1.0f);PutWord(g,21,-1.0f);
    for(uint32_t c=0;c<4;++c){PutWord(g,616+c,block.scale);PutWord(g,620+c,block.colour[c]);PutWord(g,744+c,block.offset);}
    return g;
}
bool AppendShafts(GlarePlan& plan,const ShaftInputs& in,const ShaftPlan& shafts,std::string& refused){
    if(!shafts.refused.empty()){refused=shafts.refused;return false;}
    // I-0138 tranche 8g: a plan with a layers' composite (GlareShape 3: bloom, streaks, ghost, afterimage, GlareKind::Layers) takes the
    // shafts as that composite's last layer, as YebisGlareComposite_1417511c0 does (+0x2B0 after the other layers, MadT{n+1}, every
    // weight 1; READ, evidence/I-0138/t8d-readings/glare-shape3-passes.md section 5); without one (shape 1, or shape 3 with fewer than two
    // layers) the bloom combine and the shafts make that composite alone, MadT2.
    int bright=-1,composite=-1,layers=-1;
    for(size_t k=0;k<plan.draws.size();++k){const auto& g=plan.draws[k];if(g.kind==GlareKind::BrightPass && bright<0)bright=g.target;if(g.kind==GlareKind::Composite)composite=g.target;
        if(g.kind==GlareKind::Layers)layers=int(k);}
    const int8_t last=layers>=0?plan.draws[size_t(layers)].target:int8_t(composite);
    if(bright<0 || composite<0 || plan.draws.empty() || plan.draws.back().kind!=GlareKind::Upsample || plan.draws.back().inputs[0]!=last ||
        (layers>=0 && size_t(layers)+2!=plan.draws.size())){refused="glare plan shape";return false;}
    size_t layer_count{};if(layers>=0){while(layer_count<8 && plan.draws[size_t(layers)].inputs[layer_count]!=-2)++layer_count;
        if(layer_count>=8){refused="layers' composite full";return false;}}
    if(plan.surfaces[size_t(bright)].width!=in.mask_source.width || plan.surfaces[size_t(bright)].height!=in.mask_source.height){refused="mask source extent";return false;}
    const auto add=[&](const ShaftSurface& s){plan.surfaces.push_back({s.width,s.height,false});return int8_t(plan.surfaces.size()-1);};
    const int8_t shaft=add(in.shaft),ping0=add(in.pingpong[0]),ping1=add(in.pingpong[1]),output=add(in.output);
    const auto slot=[&](ShaftSlot s)->int8_t{switch(s){case ShaftSlot::MaskSource:return int8_t(bright);case ShaftSlot::FilmGrain:return -3;case ShaftSlot::Shaft:return shaft;
        case ShaftSlot::Ping0:return ping0;case ShaftSlot::Ping1:return ping1;case ShaftSlot::Output:return output;default:return -2;}};
    std::vector<GlareDraw> added;
    for(const auto& d:shafts.draws){GlareDraw g;g.kind=GlareKind::Blur;g.technique=d.technique;g.border=true;g.target=slot(d.target);
        for(size_t i=0;i<d.inputs.size();++i){g.inputs[i]=slot(d.inputs[i]);g.film_grain[i]=d.inputs[i]==ShaftSlot::FilmGrain;}
        for(const auto& [w,v]:d.words)PutWord(g,w,v);
        g.texcoords=uint8_t(d.texcoords);
        for(const auto& v:d.vertices)g.vertices.insert(g.vertices.end(),v.begin(),v.end());
        added.push_back(std::move(g));}
    if(layers>=0){auto& g=plan.draws[size_t(layers)];const size_t i=layer_count;g.technique="tech_MadT"+std::to_string(i+1);g.inputs[i]=output;g.point[i]=true;
        for(uint32_t c=0;c<4;++c)PutWord(g,616+4*uint32_t(i)+c,1.0f);
        plan.draws.insert(plan.draws.begin()+layers,added.begin(),added.end());++plan.shafts;return true;}
    const int8_t madt2=add({plan.surfaces[size_t(composite)].width,plan.surfaces[size_t(composite)].height});
    {GlareDraw g;g.kind=GlareKind::Composite;g.technique="tech_MadT2";g.inputs[0]=int8_t(composite);g.inputs[1]=output;g.point[0]=g.point[1]=true;g.target=madt2;
        PutWord(g,20,1.0f);PutWord(g,21,-1.0f);for(uint32_t w=616;w<624;++w)PutWord(g,w,1.0f);added.push_back(std::move(g));}
    plan.draws.insert(plan.draws.end()-1,added.begin(),added.end());plan.draws.back().inputs[0]=madt2;++plan.shafts;
    return true;
}
}
