// W10 DoF: see yebis_dof.h. The arithmetic keeps the original's single-precision operations and their order.
#include "yebis_dof.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>

#pragma fp_contract(off)

namespace ds3native::yebis {
std::vector<DofLayerWords> DofLayerRamps(const DofHolder& h,const std::vector<DofLayer>& layers) noexcept {
    // [143D7EE10] 1, [143D7ED9C] 0.25, [143D7ED00] 0.0001, [142B658B0] 1.15, [142B65898] 0.61, [1429D9B10] 0.51, [142944890] 0.98.
    constexpr float one=1.0f,quarter=0.25f,first=0.0001f,near_scale=1.15f,near_scale4=0.61f,far_scale4=0.51f,far_scale=0.98f;
    std::vector<DofLayerWords> out;out.reserve(layers.size());
    for(size_t i=0;i<layers.size();++i){
        const float s=layers[i].size;float prev=i<1?s*first:layers[i-1].size;
        float near_b=prev,far_b=s,weight=quarter,far_a=s,near_a=prev;
        if(h.flags&4){far_b=s*far_scale4;if(i==0){near_b=prev*0.0f;weight=0.0f;}else near_b=prev*near_scale4;}
        if(h.flags&2){near_a=prev*near_scale;far_a=s*far_scale;}
        const float base=layers[0].size*quarter,base_b=layers[0].size*weight;
        near_a=near_a+base;near_b=near_b+base_b;
        const float span_a=(far_a+base)-near_a,inv_a=one/span_a,span_b=(far_b+base_b)-near_b,inv_b=one/span_b;
        DofLayerWords w;w.c8=inv_a;w.cc=-(near_a/span_a);w.c0=inv_b;w.c4=-(near_b/span_b);
        w.d4=w.c4;w.d0=inv_b*h.scale1b0;w.dc=w.cc;w.d8=inv_a*h.scale1b0;
        const float inv_r=one/layers[i].radius,height=float(layers[i].height);
        w.e8=h.scale1b0*one*height*inv_r;w.ec=((h.scale1b0*0.0f+0.0f)*height+0.0f)*inv_r+0.0f;
        float scale=one,offset=0.0f;if(h.mode154==1){scale=h.scale1bc;offset=h.offset1c0;}
        w.f4=w.e8*offset+w.ec;w.f0=w.e8*scale;
        out.push_back(w);}
    return out;
}

namespace {
constexpr float one=1.0f,two=2.0f,half=0.5f;   // [143D7EE10], [143D7EE7C], [143D7EDB4]
void Put(GlareDraw& g,uint32_t word,float value){PutGlareWord(g,word,value);}
GlareDraw Draw(GlareKind kind,const std::string& technique,int8_t target){GlareDraw g;g.kind=kind;g.technique=technique;g.target=target;
    Put(g,20,1.0f);Put(g,21,-1.0f);return g;}   // fParam_ScreenSpaceScale
// FUN_1416D63B0 (n < 8, the bilinear flag clear): tap i at ((float)i x (2n / n) - (n - 1)) x 1 x (1 / size).
std::vector<float> AxisTaps(uint32_t size,int n){
    std::vector<float> out;const float fn=float(n),step=(fn*two)/fn,inv=one/float(size),centre=fn-one;
    for(int i=0;i<n;++i)out.push_back((float(i)*step-centre)*one*inv);
    return out;}
// FUN_1416D6570 in FUN_14172A660's call (the bilinear flag clear, at most 5 per axis, step 1): per axis n = ceil((src x 0.5) / dst),
// one less when (n - 1) dst 2 < src and src - (2n - 2) <= (n - 1) dst 2 (dst >= 8); the taps row by row.
int AxisCount(float src,float dst){
    int n=int(std::ceil((src*half)/dst+(-0.0f)));
    if(1<n && 8.0f<=dst){const float covered=float(n-1)*dst*two;if(covered<src && src-float(n*2-2)<=covered)--n;}
    return std::min(n,5);}
// FUN_14172A660 (weights (1, 1, 1, 1), the plain column): the taps' offsets (words 136 + 4t for t < 8, else 168 + 4t) and weights
// 1 / n (words 616 + 4t + c).
bool Downsample(GlareDraw& g,uint32_t sw,uint32_t sh,uint32_t dw,uint32_t dh){
    const int nx=AxisCount(float(sw),float(dw)),ny=AxisCount(float(sh),float(dh));if(nx<1 || ny<1)return false;
    const auto xs=AxisTaps(sw,nx),ys=AxisTaps(sh,ny);const int n=nx*ny;const float weight=one/float(n);
    g.technique="tech_MadT1_S"+std::to_string(n);
    int t=0;for(int y=0;y<ny;++y)for(int x=0;x<nx;++x,++t){const uint32_t at=(t<8?136u:168u)+4u*uint32_t(t);Put(g,at,xs[size_t(x)]);Put(g,at+1,ys[size_t(y)]);
        for(uint32_t c=0;c<4;++c)Put(g,616+4*uint32_t(t)+c,weight*1.0f);}
    return true;}
// The tap blur's words: offsets as above, weights[t] x (1, 1, 1, 1).
void Taps(GlareDraw& g,const std::vector<std::array<float,2>>& offsets,const std::vector<float>& weights){
    g.technique="tech_MadT1_S"+std::to_string(offsets.size());
    for(size_t t=0;t<offsets.size();++t){const uint32_t at=(t<8?136u:168u)+4u*uint32_t(t);Put(g,at,offsets[t][0]);Put(g,at+1,offsets[t][1]);
        for(uint32_t c=0;c<4;++c)Put(g,616+4*uint32_t(t)+c,1.0f*weights[t]);}}
// FUN_14173B2C0 -> FUN_14173C3F0 (bilinear) -> FUN_1416D7300: the 3x3 kernel (0.4, 1, 0.4 / 1, 1, 1 / 0.4, 1, 0.4) folded into five
// bilinear taps (the centre, then the four edges each with its corner), normalised by their sum.
void Blur3(GlareDraw& g,uint32_t w,uint32_t h){
    float k[9];const uint32_t corner=0x3ECCCCCDu;std::memcpy(&k[0],&corner,4);k[2]=k[6]=k[8]=k[0];k[1]=k[3]=k[4]=k[5]=k[7]=1.0f;
    const float x=one/float(w),y=one/float(h),xm=x*-1.0f,x0=x*0.0f,ym=y*-1.0f,y0=y*0.0f;
    std::vector<std::array<float,2>> o(5,{0.0f,0.0f});std::vector<float> wt(5);wt[0]=k[4];
    const auto fold=[&](float a,float b,float& t){const float s=b+a;t=0.0f;if(0.0f<s)t=a/s;return (s*half)*two;};
    float t{};
    wt[1]=fold(k[0],k[1],t);o[1]={(xm-x0)*t+x0,ym};
    wt[2]=fold(k[2],k[5],t);o[2]={x,(ym-y0)*t+y0};
    wt[3]=fold(k[3],k[6],t);o[3]={xm,(y0-y)*t+y};
    wt[4]=fold(k[7],k[8],t);o[4]={(x0-x)*t+x,y};
    float sum=0.0f;sum=sum+wt[0]+wt[1]+wt[2]+wt[3];sum=sum+wt[4];
    const float norm=one/sum;for(auto& v:wt)v=norm*v;
    Taps(g,o,wt);}
// FUN_14173B3D0 -> FUN_1416D7540: nine taps, the centre (1), the axes at [142B4D0A8] texels (1.8) and the diagonals at [142B4D0A0]
// (2.0306249), normalised by 1 + 4 x 1.8 + 3 x 2.0306249 + [142B4D118].
void Blur9(GlareDraw& g,const DofData& d,uint32_t w,uint32_t h){
    const float x=one/float(w),y=one/float(h),xa=x*d.axis,ya=y*d.axis,xd=x*d.diagonal,yd=y*d.diagonal,x0=x*0.0f,y0=y*0.0f;   // +0 both (tap 4 x is (1/H) x 0)
    std::vector<std::array<float,2>> o{{0.0f,0.0f},{-xa,y0},{xa,y0},{x0,-ya},{x0,ya},{-xd,-yd},{xd,-yd},{-xd,yd},{xd,yd}};
    const uint32_t axis_bits=0x3FE66666u,diagonal_bits=0x4001F5C2u;float a{},dg{};std::memcpy(&a,&axis_bits,4);std::memcpy(&dg,&diagonal_bits,4);
    std::vector<float> wt{1.0f,a,a,a,a,dg,dg,dg,dg};
    float sum=wt[0]+0.0f;for(size_t i=1;i<8;++i)sum=sum+wt[i];sum=sum+d.norm_last;
    const float norm=one/sum;for(auto& v:wt)v=norm*v;
    Taps(g,o,wt);}
// W10 closure (m40, Reference1705 draws 6 and 8): FUN_14173B850 -> FUN_1416D77E0, the nine-tap level's 5x5 kernel: 25 taps at the
// distances of kernel25 (one texel = 1/W, 1/H), weights 1, 2 x4, 4 x4, 1.75 x4, 0.92160004 x4, 2.6883662 x8, normalised by their sum
// accumulated row by row of five; the ±0 components are the decompile's c x 0.0 x texel products.
void Blur25(GlareDraw& g,const DofData& d,uint32_t w,uint32_t h){
    const auto& c=d.kernel25;const float x=one/float(w),y=one/float(h);
    const float c38=c[0],cf0=c[1],c48=c[2],cb0=c[3],c3c=c[4],c8c=c[5],ca8=c[6],cac=c[7],ca4=c[8],c44=c[9];
    const float x38=x*c38,xf0=x*cf0,y38=y*c38,yf0=y*cf0,x38z=(c38*0.0f)*x,y38z=(c38*0.0f)*y,x48z=(c48*0.0f)*x,y48z=(c48*0.0f)*y;
    std::vector<std::array<float,2>> o{{0.0f,0.0f},
        {xf0,y38z},{x38,y38z},{x38z,yf0},{x38z,y38},
        {xf0,yf0},{x38,yf0},{xf0,y38},{x38,y38},
        {x*cb0,y48z},{x*c48,y48z},{x48z,y*cb0},{x48z,y*c48},
        {x*ca8,y*ca8},{x*c3c,y*ca8},{x*ca8,y*c3c},{x*c3c,y*c3c},
        {x*c8c,y*cac},{x*ca4,y*cac},{x*c44,y*c8c},{x*c44,y*ca4},{x*ca4,y*c44},{x*c8c,y*c44},{x*cac,y*ca4},{x*cac,y*c8c}};
    const float a175=1.75f;
    const float w92=0.92160004f,w26=2.6883662f;
    std::vector<float> wt{1.0f,2.0f,2.0f,2.0f,2.0f,4.0f,4.0f,4.0f,4.0f,a175,a175,a175,a175,w92,w92,w92,w92,w26,w26,w26,w26,w26,w26,w26,w26};
    float sum=0.0f;for(size_t r=0;r<5;++r)sum=sum+wt[5*r]+wt[5*r+1]+wt[5*r+2]+wt[5*r+3]+wt[5*r+4];
    const float norm=one/sum;for(auto& v:wt)v=v*norm;
    Taps(g,o,wt);}
}
bool LoadDofDrawData(DofData& d,const std::function<bool(uint32_t,void*,size_t)>& read){
    static constexpr uint32_t k25[10]{0x3D7EE38,0x3D7F0CC,0x2B4D148,0x2B4D1B0,0x2B4D13C,0x2B4D18C,0x2B4D1A8,0x2B4D1AC,0x2B4D0A4,0x2B4D144};
    bool ok=read(0x2B4D0A8,&d.axis,4) && read(0x2B4D0A0,&d.diagonal,4) && read(0x2B4D118,&d.norm_last,4);
    for(size_t i=0;i<10 && ok;++i)ok=read(k25[i],&d.kernel25[i],4);
    return ok;}

DofCounts DofLevelCounts(const DofFrame& f) noexcept {
    DofCounts c;
    float near_bound=f.near_override;if(near_bound<f.near_plane || (f.far_plane<=near_bound && near_bound!=f.far_plane))near_bound=f.near_plane;
    float far_bound=f.far_override;if(f.far_plane<far_bound || far_bound<=0.0f)far_bound=f.far_plane;
    for(size_t i=0;i<f.levels.size();++i){const auto& r=f.levels[i].ramps;
        if(far_bound<=0.0f || f.focus<=0.0f || 0.0f<=((far_bound-f.focus)/far_bound)*r.d0+r.d4)c.back=int(i)+1;
        if(near_bound<=0.0f || f.focus<=0.0f || 0.0f<=(-((near_bound-f.focus)/near_bound))*r.d8+r.dc)c.front=int(i)+1;}
    if(-1<f.limits[0])c.front=std::min(c.front,f.limits[0]);
    if(-1<f.limits[1])c.back=std::min(c.back,f.limits[1]);
    c.both=std::max(c.front,c.back);
    return c;
}

GlarePlan BuildDof(const DofData& d,const DofFrame& f){
    GlarePlan plan;
    const auto refuse=[&](const char* why){plan.refused=why;plan.draws.clear();plan.surfaces.clear();plan.pre.clear();plan.output=-1;return plan;};
    if(!f.width || !f.height)return refuse("extent");
    // I-0138 part 4b: holder flags 0 (any record whose edge quality maps to 0: -1 at DoF quality 0 or 1, FUN_1416cbfe0, or 0 and
    // 1; the cutscene records and five map records, m21_00_0000, m22_00_0000, m30_00_0200, m32_00_0100, m38_00_0002) draw the same
    // frame without the front blur's blend factors, then CompositeBlurredLevels_T{levels + 1} (YebisDofComposite_14175f850,
    // CompositeBlurredLevels_Tn_1417263b0); the masked flags (6 and up) are not served.
    if(f.flags!=2 && f.flags!=0)return refuse("holder flags other than 0 and the front blur's 2 (+0x140)");
    const bool front_blur=f.flags==2;
    if(f.mode154!=0)return refuse("the USNORM composite (+0x154): no witness");
    if(f.depth_format!=0)return refuse("a depth format other than the R value");
    if(f.lookup_space==6 || f.factor_space==6)return refuse("the table's source space (6): no witness");
    if(!f.table_width || !f.table_height)return refuse("table extent");
    if(f.source_column || f.level_column)return refuse("a downsample technique column");
    plan.width=f.width;plan.height=f.height;plan.table_width=f.table_width;plan.table_height=f.table_height;
    // The depth factor (pre draw): t0 the depth (point, border), t2 the table (point, wrap); the factor into the scene's alpha.
    {GlareDraw g=Draw(GlareKind::Downsample,"tech_DepthOfField_BlurFactorAlphaFromDepthTextureTypeCastLut_RVALUE",-1);g.write_mask=8;
        g.inputs[0]=-4;g.point[0]=true;g.border=true;g.inputs[2]=-5;g.point[2]=true;g.address[2]=1;   // D3D12_TEXTURE_ADDRESS_MODE_WRAP
        const auto lookup=DepthConversion(f.spaces,6,f.lookup_space,1.0f,0.0f);
        const float tw=float(f.table_width),texel=(tw-one)/tw;
        const float a=lookup[0]*texel,b=lookup[1]*texel+half/tw;
        const float scale=f.source_object?f.source_scale:1.0f,bias=f.source_object?f.source_bias:0.0f;
        Put(g,0,scale*a);Put(g,1,a*bias+b);Put(g,2,1.0f);Put(g,3,0.0f);
        const auto factor=DepthConversion(f.spaces,f.factor_space,1,1.0f,0.0f);
        float fa=factor[0],fb=factor[1]+0.0f;fa=fa*f.conversion[0];fb=f.conversion[0]*fb+f.conversion[1];
        Put(g,68,fa);Put(g,69,fb);Put(g,70,one/fa);Put(g,71,-(fb/fa));
        Put(g,200,(f.table_row+half)/float(f.table_height));
        plan.pre.push_back(g);}
    const auto counts=DofLevelCounts(f);
    if(counts.both<1)return refuse("no level drawn");
    if(size_t(counts.both)>f.levels.size())return refuse("level count");
    const auto surface=[&](uint32_t w,uint32_t h){plan.surfaces.push_back({w,h,true});return int8_t(plan.surfaces.size()-1);};
    // The source chain: each drawn level's surface, downsampled from the previous one (the scene first).
    std::map<int,int8_t> sources;int8_t current=-1;int current_id=-1;uint32_t cw=f.width,ch=f.height;
    for(int i=0;i<counts.both;++i){const auto& level=f.levels[size_t(i)];
        if(level.kind!=0)return refuse("an aperture filter other than the tap blurs");
        if(level.width!=level.surface_width || level.height!=level.surface_height)return refuse("a level smaller than its surface: no witness");
        if(level.surface==current_id)continue;
        const int8_t t=surface(level.surface_width,level.surface_height);sources[level.surface]=t;
        GlareDraw g=Draw(GlareKind::Downsample,"",t);g.inputs[0]=current;if(current==-1)g.address[0]=3;else g.border=true;   // the scene's object: clamp
        if(!Downsample(g,cw,ch,level.surface_width,level.surface_height))return refuse("downsample taps");
        plan.draws.push_back(g);current=t;current_id=level.surface;cw=level.surface_width;ch=level.surface_height;}
    // The aperture filter of each drawn level: under flags 2 the front levels' blend factor (point, mirror) into a level surface, then the blur
    // (bilinear, mirror) into the level's result.
    std::vector<int8_t> results;
    for(int i=0;i<counts.both;++i){const auto& level=f.levels[size_t(i)];int8_t source=sources[level.surface];
        if(front_blur && i<counts.front){const int8_t t=surface(level.width,level.height);
            GlareDraw g=Draw(GlareKind::Blur,"tech_DepthOfField_FrontBlurBlendFactor",t);g.inputs[0]=source;g.point[0]=true;g.address[0]=2;   // MIRROR
            float scale=-level.ramps.d8,offset=level.ramps.dc;
            if(f.mode154==1){const float m=scale;scale=m*f.scale1bc;offset=offset+m*f.offset1c0;}
            for(uint32_t c=0;c<3;++c){Put(g,616+c,1.0f);Put(g,620+c,0.0f);}Put(g,619,scale);Put(g,623,offset);
            plan.draws.push_back(g);source=t;}
        const int8_t r=surface(level.width,level.height);
        GlareDraw g=Draw(GlareKind::Blur,"",r);g.inputs[0]=source;g.address[0]=2;
        if(level.taps==3)Blur3(g,level.width,level.height);else if(level.taps==5)Blur9(g,d,level.width,level.height);
        else if(level.taps==9)Blur25(g,d,level.width,level.height);   // FUN_14173C2B0: 3, 5 and 9 taps; another returns E_NOTIMPL
        else return refuse("a level blur other than 3, 5 or 9 taps (FUN_14173C2B0 draws none)");
        plan.draws.push_back(g);results.push_back(r);}
    // The composite (mode 2, and flags 0 the same inputs and words): t0 the scene (point, clamp), t1.. the levels' results (bilinear, clamp); words 76 + 4k the level k's
    // (+0xD0, +0xD4) (the library's c18 before them holds zeros); TEXCOORD1 the levels' coordinates (their surfaces are wholly used: equal to TEXCOORD0).
    if(results.size()>7)return refuse("composite inputs");
    const int8_t output=surface(f.width,f.height);
    {GlareDraw g=Draw(GlareKind::Composite,front_blur?"tech_DepthOfField_CompositeBlurLevelsFrontBlur_TS"+std::to_string(counts.back)+"_"+std::to_string(counts.front):
            "tech_DepthOfField_CompositeBlurredLevels_T"+std::to_string(results.size()+1),output);
        g.inputs[0]=-1;g.point[0]=true;g.address[0]=3;
        for(size_t k=0;k<results.size();++k){g.inputs[k+1]=results[k];g.address[k+1]=3;}
        // Words 72..75 (c18), the scene's pair, are the library's zeros: no programme of ours reads them, so our block has no slot.
        for(size_t k=0;k<results.size();++k){const auto& r=f.levels[k].ramps;const uint32_t at=76+4*uint32_t(k);Put(g,at,r.d0);Put(g,at+1,r.d4);Put(g,at+2,0.0f);Put(g,at+3,0.0f);}
        g.texcoord1=true;g.uv1={{{0.0f,1.0f},{0.0f,-1.0f},{2.0f,1.0f}}};
        plan.draws.push_back(g);}
    plan.output=output;
    return plan;
}
}

namespace ds3native::yebis {
bool LoadDofSetupData(DofSetupData& d,const std::function<bool(uint32_t,void*,size_t)>& read){
    bool ok=true;for(uint32_t k=0;k<d.quality.size() && ok;++k)ok=read(0x469B510+0x1C*k,d.quality[k].data(),0x1C);
    // FUN_141755ED0's preset table, seven presets of seven (a, b) pairs, from the .rdata quadwords it copies (in its stack order);
    // pair 21 is (0, render aspect < [143D7EE18]), written by BuildDofSetup.
    static constexpr uint32_t rows[49]{0x2B65910,0x2B65918,0x2B59B40,0x2B59B48,0x3D802E0,0x3D802E8,0x27BCEE0,
        0x27BCEE8,0x2B3A640,0x2B3A648,0x2B659F0,0x2B659F8,0x3D802E0,0x3D802E8,
        0x3D7F810,0x3D7F818,0x2B3A640,0x2B3A648,0x3D802E0,0x3D802E8,0,
        0,0x2B65950,0x2B65958,0x2B659F0,0x2B659F8,0x3D802E0,0x3D802E8,
        0x3D7F810,0x3D7F818,0x2B65950,0x2B65958,0x3D802E0,0x3D802E8,0x27BCEE0,
        0x27BCEE8,0x2B65930,0x2B65938,0x2B65970,0x2B65978,0x3D802E0,0x3D802E8,
        0x3D7F810,0x3D7F818,0x2B65960,0x2B65968,0x3D802E0,0x3D802E8,0};
    for(size_t i=0;i<49 && ok;++i){if(!rows[i]){d.presets[i]={-1,-1};continue;}ok=read(rows[i],d.presets[i].data(),8);}
    d.presets[21]={0,0};
    ok=ok && read(0x2B659C0,d.taps.data(),20) && read(0x29C86F0,&d.radius,4) && read(0x2B65858,d.small_sizes.data(),16) &&
        read(0x3D7EE00,&d.base_ceiling,4) && read(0x2B55250,&d.height_rounding,4) && read(0x3D7EDB4,&d.half,4) && read(0x3D7EE18,&d.aspect_limit,4);
    const uint32_t aspect_bits=0x3F2AAAE3u;std::memcpy(&d.dof_aspect,&aspect_bits,4);   // CPostEffect::Initialize's +0x388 (TM-3's m)
    return ok;
}
DofSetup BuildDofSetup(const DofSetupData& d,uint32_t width,uint32_t height,int quality,int levels){
    DofSetup s;s.width=width;s.height=height;
    if(!width || !height){s.refused="extent";return s;}
    if(quality<0 || quality>=int(d.quality.size())){s.refused="DoF quality";return s;}
    if(levels<1 || levels>7){s.refused="level count";return s;}
    const auto& row=d.quality[size_t(quality)];float scale{};std::memcpy(&scale,&row[0],4);
    // The base size (FUN_1416EBF80): W x scale x sqrt((A / k) / A) + 0.99, then W / (A / k) + 0.499999, A = W / H (S+0x328), k = S+0x388.
    const float aspect=float(width)/float(height),ratio=aspect/d.dof_aspect;
    const float root=std::sqrt(ratio/aspect);
    s.base_width=uint32_t(int64_t(float(width)*scale*root+d.base_ceiling));
    if(s.base_width>=width)s.base_width=width;
    s.base_height=uint32_t(int64_t(float(s.base_width)/ratio+d.height_rounding));
    if(s.base_width<16 || s.base_height<16){s.refused="DoF base size below 16";return s;}
    static constexpr uint32_t flags[5]{0,0,2,6,6};   // FUN_1416FDD80's table (its code's immediates)
    if(row[2]>4){s.refused="creation flags index";return s;}
    s.creation_flags=flags[row[2]];s.levels=levels;s.preset=int(row[4]);s.small_width=row[5];std::memcpy(&s.small_scale,&row[6],4);
    if(s.preset<0){s.refused="custom preset (+0xDC): no witness";return s;}
    // FUN_141755ED0.
    auto presets=d.presets;presets[21]={0,(float(width)/float(s.base_width))<d.aspect_limit?1:0};
    const auto* p=&presets[size_t(std::min(s.preset,6))*7];
    const float scene_width=float(width);uint32_t w=s.base_width,h=s.base_height;int small_index=0,b=0;
    const auto is_small=[&](uint32_t x){return x<=s.small_width || float(x)<=scene_width*s.small_scale;};
    for(int i=0;i<levels;++i){
        const int at=i>5?6:i;bool past=true;if(i<7 && -1<p[at][0]){b=p[at][1];past=false;}
        if(i<1){if(is_small(w))small_index=1;}
        else if(past || p[i-1][0]!=p[i][0]){
            w=uint32_t(int64_t(float(w)*d.half+d.base_ceiling));if(w<4)w=3;
            h=uint32_t(int64_t(float(h)*d.half+d.base_ceiling));if(h<4)h=3;
            if(is_small(w))small_index=std::min(small_index+1,3);}
        DofSetupLevel v;v.width=w;v.height=h;v.small_index=small_index;v.size_scale=d.small_sizes[size_t(small_index)];v.surface_width=w;v.surface_height=h;
        if(0.0f<v.size_scale){v.surface_width=width<=w*2?width:w*2;v.surface_height=height<=h*2?height:h*2;}
        v.kind=0;v.preset_b=b;
        // FUN_141754D30 (kind 0): the taps by b, the radius taps x 1.05, +0x24 the radius plus the small table's term.
        if(b<0 || b>4){s.refused="preset taps index";return s;}
        v.taps=d.taps[size_t(b)];const float taps=float(v.taps);v.radius=taps*d.radius;v.size=v.radius;
        if(0.0f<v.size_scale)v.size=v.radius+v.size_scale;
        s.level.push_back(v);}
    return s;
}
DofLens DofLensState(const DofConstants& k,const DofLensInputs& in){
    DofLens l;
    // FUN_1416C9450 and FUN_141763320.
    l.near_plane=in.near_plane<=0.0f?k.one:in.near_plane;l.far_plane=in.far_plane<=0.0f?1000.0f:in.far_plane;
    l.tan_half=std::tan(in.fov*k.half);if(l.tan_half<=0.0f)l.tan_half=std::tan(0.39269909f);
    l.fov=std::atan(l.tan_half)*k.two;
    // E69A0 -> FUN_1416C98D0: S+0x6C4 (the film's height from the diagonal at the render aspect), S+0x6CC (E5110's quality factor x
    // the f-number), S+0x6D0, S+0x6D4.
    const float aspect=in.aspect==0.0f?float(in.extent_width)/float(in.extent_height):in.aspect;
    l.sensor=(in.diagonal*k.milli)/std::sqrt(aspect*aspect+k.one);
    const float q=float(in.quality)/k.quality_steps;
    const float factor=(k.one/(float(uint32_t(in.extent_width*in.extent_height))/k.full_hd))*q+(k.one-q);
    l.f_number=factor*in.f_number;l.focus=in.focus;
    // FUN_141762CF0, aperture type 0 (the f-number as given; [+0x388] 0, [+0x38C] 0).
    const float t=std::tan(l.fov*k.half)*k.two,x=t*k.half;
    l.focal=0.0f<l.focus?((l.focus*l.sensor)*k.half)/(x*l.focus+l.sensor*k.half):l.sensor/(x*k.two);
    float aperture=0.0f;if(0.0f<l.f_number)aperture=l.focal/l.f_number;
    const bool valid=0.0f<l.focus && 0.0f<l.f_number && 0.0f<aperture && 0.0f<l.sensor;
    if(valid){l.aperture=aperture;l.coc=(aperture*l.focal)/(l.focus-l.focal);l.scale=l.coc/l.sensor;}
    l.enable=valid && 0.0f<l.fov && l.fov<k.pi;
    l.branch=0.0f<l.f_number && 0.0f<l.near_plane && l.near_plane<l.far_plane && l.enable && in.enable;
    return l;
}
DofFrame NativeDofFrame(const DofSetup& setup,const DofLens& lens,int front_limit,int back_limit,uint32_t flags){
    DofFrame f;f.width=setup.width;f.height=setup.height;f.flags=flags;f.mode154=0;
    f.near_plane=lens.near_plane;f.far_plane=lens.far_plane;f.focus=lens.focus;f.limits={front_limit,back_limit,-1,-1};
    std::vector<DofLayer> inputs;for(const auto& v:setup.level)inputs.push_back({v.height,v.radius,v.size/float(v.height)});
    const auto ramps=DofLayerRamps({flags,0,lens.scale,lens.words[2],lens.words[3]},inputs);
    int surface=-1;uint32_t sw{},sh{};
    for(size_t i=0;i<setup.level.size();++i){const auto& v=setup.level[i];DofLevel level;level.width=v.width;level.height=v.height;
        // FUN_141747320 hands the levels their surfaces from a pool by extent: consecutive levels of one extent share it.
        if(surface<0 || v.surface_width!=sw || v.surface_height!=sh){++surface;sw=v.surface_width;sh=v.surface_height;}
        level.surface=surface;level.surface_width=sw;level.surface_height=sh;level.kind=v.kind;
        level.taps=v.taps;level.ramps=ramps[i];f.levels.push_back(level);}
    f.spaces=DofDepthSpaces(lens.near_plane,lens.far_plane,lens.focus,lens.words);f.spaces.source=0;f.spaces.space3=lens.enable?lens.scale:0.0f;
    f.source_object=true;f.source_scale=1.0f;f.source_bias=-0.0f;f.depth_format=0;f.lookup_space=0;f.factor_space=3;f.table_row=0.0f;
    f.conversion={lens.words[0],lens.words[1],1.0f/lens.words[0],-(lens.words[1]/lens.words[0])};
    return f;
}
}
