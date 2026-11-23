#include "yebis_glare.h"
#include <algorithm>
#include <cmath>
#include <cstring>

#pragma fp_contract(off)   // the formulas reproduce the library's float roundings one operation at a time

namespace ds3native::yebis {
namespace {
void Put(GlareDraw& draw,uint32_t word,float value){PutGlareWord(draw,word,value);}
void Common(GlareDraw& draw){Put(draw,20,1.0f);Put(draw,21,-1.0f);}   // fParam_ScreenSpaceScale
float Sigma(const GlareData& d,float sigma){return sigma<=d.sigma_floor?d.sigma_replacement:sigma;}
// FUN_1416D6E80 with its normalisation: g(j) = exp(-j^2 / (2 sigma^2)) / (sqrt(2 pi) sigma); the blur's tap index k is the first
// j >= 2 with g(j) below the sampling threshold (15 when none below 15).
int TapIndex(const GlareData& d,float sigma,float threshold){
    if(!(0.0f<threshold))return 15;
    const float norm=1.0f/(std::sqrt(d.two_pi)*sigma);
    for(int j=2;j<15;++j){const float x=float(j);if(std::exp(-(x*x)/(2.0f*sigma*sigma))*norm<threshold)return j;}
    return 15;}
}
void BlurWeights(const GlareData& d,uint32_t size,float sigma_in,bool analytic,uint32_t distances,std::array<float,15>& offsets,std::array<float,15>& weights){
    if(distances<1 || distances>7)distances=7;
    const float sigma=Sigma(d,sigma_in);
    if(analytic && std::sqrt(d.two_pi)*sigma<1.0f)analytic=false;
    const float inv_two_sigma2=1.0f/(sigma*2.0f*sigma),inv_size=1.0f/float(size);
    std::array<float,7> g{},dist{};
    for(uint32_t j=0;j<distances;++j){dist[j]=float(int(j))+1.0f;const float t=dist[j]*1.0f;g[j]=std::exp(-(t*t*inv_two_sigma2));}
    const float centre=std::exp(-(1.0f*0.0f*1.0f*0.0f*inv_two_sigma2));
    float sum=g[0]*2.0f+centre;for(uint32_t j=1;j<distances;++j)sum=sum+g[j]*2.0f;
    if(analytic)sum=std::sqrt(d.two_pi)*sigma;
    const float norm=1.0f/sum;
    offsets.fill(0.0f);weights.fill(0.0f);
    for(size_t k=0;k<2*size_t(distances)+1;++k){
        if(!k){weights[0]=norm*centre;continue;}
        const size_t j=(k-1)/2;const float o=dist[j]*inv_size;offsets[k]=(k&1)?o:-o;weights[k]=norm*g[j];}
}
// I-0138 tranche 8f: GlareShape 3 (« Standard »), its record, its trim texture and its draws after the bloom combine.
GlareShapeRecord ShapeRecord(const GlareData& d){
    // FUN_141748760 (READ, evidence/I-0138/t8d-readings/glare-shape3-passes.md section 0): the record from the table entry's d0 to d10
    // and its preset's row.
    const auto f=[](uint32_t bits){float v;std::memcpy(&v,&bits,4);return v;};
    const auto& w=d.shape3;GlareShapeRecord r;const float d7=f(w[7]);
    r.bloom=f(w[0]);r.scale=f(w[1]);r.tint=std::fabs(d7)*0.8f;r.tint_levels=4;
    r.ghost=f(w[2]);r.ghost_scale=1.0f;r.ghost_size=f(w[3]);r.lens_flare=f(w[3])>=0.05f;
    r.streak=f(w[4]);r.angle=f(w[6]);r.colour_flag=std::fabs(d7)>=1.0f;r.colour=r.colour_flag?d7*0.36f:d7;
    r.after_time=f(w[8]);r.after_weight=f(w[9]);
    r.arms=int(d.preset3[0]);r.arm_a=f(d.preset3[1]);r.arm_length=f(d.preset3[2]);r.odd_length=f(d.preset3[3]);r.preset_flag=d.preset3[4]!=0;
    return r;}
namespace {
float Bits(uint32_t bits){float v;std::memcpy(&v,&bits,4);return v;}
// 1448E8F70's (d7 >= 0) and 1448E8FC0's (d7 < 0) colours 1 to 3 (written by FUN_1417481E0); colour 0 white.
const std::array<float,4>& StreakColour(float d7,int i){
    static constexpr std::array<std::array<float,4>,4> plus{{{1,1,1,1},{0,0,1,1},{0,1,0,1},{1,0,0,1}}};
    static constexpr std::array<std::array<float,4>,4> minus{{{1,1,1,1},{1,0,0,1},{0,1,0,1},{0,0,1,1}}};
    return (d7>=0.0f?plus:minus)[size_t(i)];}
// The ghost's xorshift128, seeded from immediates at every call (READ, glare-shape3-ghost.md section 1): its eight outputs (the new w).
std::array<uint32_t,8> Xorshift(uint32_t x,uint32_t y,uint32_t z,uint32_t w){std::array<uint32_t,8> out{};
    for(auto& o:out){const uint32_t t=x^(x<<11);x=y;y=z;z=w;w=w^(w>>19)^t^uint32_t(int32_t(t)>>8);o=w;}return out;}
// The streaks, the ghost, the afterimage and the layers' composite after the bloom combine (`combine`), in YebisGlareFrame_1417538e0's
// order (READ: glare-shape3-passes.md sections 1 to 5, glare-shape3-ghost.md); returns the layers' surface (the 2x draw's input), or
// -1 with a reason. `level` is each pyramid level's final blur output (the level record's +0x18).
int8_t AppendStandard(const GlareData& d,const GlareInputs& in,GlarePlan& plan,int8_t bright,const std::vector<int8_t>& level,int8_t combine,std::string& why){
    const auto& rec=in.record;const uint32_t bw=plan.base_width,bh=plan.base_height;const int L=plan.levels;
    const auto surface=[&]{plan.surfaces.push_back({bw,bh,false});return int8_t(plan.surfaces.size()-1);};   // R11G11B10 at the base size
    const auto draw=[&](GlareKind kind,const std::string& technique,int8_t target){GlareDraw g;g.kind=kind;g.technique=technique;g.target=target;
        PutGlareWord(g,20,1.0f);PutGlareWord(g,21,-1.0f);return g;};
    const float g114=in.factor,g118=in.luminance;
    std::vector<int8_t> layers{combine};
    // The streaks (FUN_14174D3A0, CRenderGlare::GenerateGlare_Star), gated as YebisGlareFrame does (+0x248 and +0x250 allocated: +0xE4 = 8).
    constexpr int arm_surfaces=8;constexpr float source_share=0.25f;   // CRenderGlare +0xE4, +0xE8: CPostEffect_Initialize's, in every capture
    if(g118>0.0f && rec.arms>0 && rec.arm_length>0.0f && rec.bloom>0.0f && rec.streak>0.0f){
        // Draw 0, the source mix (+0x210[3]): the bright pass and levels 0 and 1 by (1 - f), f (1 - f), f^2 (f = +0x2EC), point sampled.
        int8_t source=bright;
        if(source_share>0.0f && in.star_softness>0.0f){const float f=in.star_softness;const int8_t mix=surface();
            const bool two=L>1;GlareDraw g=draw(GlareKind::Mix,two?"tech_MadT3":"tech_MadT2",mix);
            g.inputs[0]=bright;g.inputs[1]=level[0];if(two)g.inputs[2]=level[1];g.point[0]=g.point[1]=g.point[2]=true;
            const float w[3]{1.0f-f,f*(two?1.0f-f:1.0f),f*f};
            for(uint32_t i=0;i<(two?3u:2u);++i)for(uint32_t c=0;c<4;++c)PutGlareWord(g,616+4*i+c,w[i]);
            plan.draws.push_back(g);source=mix;}
        const int A=std::min(rec.arms,arm_surfaces);
        const float W=((((g114*0.035f)*rec.bloom)*g118)*0.04f)*rec.streak;   // 0.1 for 0.04 under +0xEE (0 in every capture)
        const float P=std::pow(W,0.2f);
        const float w=float(bw),h=float(bh),M=std::max(w,h);
        const float aspect=(float(bw)/float(bh))/(w/h);
        float base=1.0f/(in.tan_half*2.0f);if(in.star_fov!=1.0f)base=std::exp(std::log(base)*in.star_fov);
        const float F=base*(float(bw)*0.003125f);
        float a0=rec.angle;if(std::fabs(a0)>=d.two_pi)a0=(d.two_pi/float(A))*-0.5f;
        if(rec.preset_flag)a0=a0-in.ev;
        // The colour rows (14174d5d6..14174d83a): row k - 1 (t = k / 8) of tap j: white for j = 0, else ((1 - C) t + C - 1) m + 1.
        const float m=std::fabs(rec.colour)*1.5f;std::array<std::array<std::array<float,4>,4>,8> rows{};
        for(int k=1;k<=8;++k){const float t=float(k)*0.125f;rows[size_t(k-1)][0]={1,1,1,1};
            for(int j=1;j<4;++j){const auto& c=StreakColour(rec.colour,j);
                for(size_t l=0;l<4;++l)rows[size_t(k-1)][size_t(j)][l]=((((1.0f-c[l])*t)+c[l])-1.0f)*m+1.0f;}}
        const float thr=in.star_threshold;   // a float format (R11G11B10) keeps it (the integer formats' raises not served)
        std::vector<int8_t> results(size_t(A),-1);std::vector<float> arm_weight(size_t(A),1.0f);
        const int8_t temp[2]{surface(),surface()};
        const float pi=d.two_pi*0.5f;
        int passes_max=0;
        for(int i=0;i<A;++i){results[size_t(i)]=surface();
            const float a=rec.arm_a;const float len=(rec.odd_length>0.0f && (i&1))?rec.odd_length:rec.arm_length;
            const float step=(((360.0f/float(rec.arms))*pi)/180.0f)*float(i);
            const float theta=a0-step;const float len1=len>1e-5f?len:1e-5f;
            float dx=std::sin(theta)*(a/w),dy=(std::cos(theta)*(a/h))*aspect;
            float x9=((a/F)/6.0f)/len1;
            const float X=(std::log(thr)/std::log(0.5f))/x9;
            if(!std::isfinite(X) || !(X>0.0f)){why="streak passes";return -1;}   // the final review's M1: no int of a non-finite value
            const int N=int(std::ceil(std::log(X)/std::log(4.0f)));
            if(!(N>=1 && N<=8)){why="streak passes";return -1;}
            int last=int(std::ceil((X*4.0f)/std::pow(4.0f,float(N))));if(last<=0)last=4;last=std::min(last,4);
            int pp=0;int8_t from=source;float scale=1.0f;int done=0;
            for(int p=0;p<N;++p){const bool is_last=p==N-1;float mp=0.1875f;
                if(p>=1){mp=P;arm_weight[size_t(i)]=arm_weight[size_t(i)]*(1.0f/P);}
                const float ap=a*scale;
                int n=std::min(4,int(std::ceil(((ap-1.0f)+M)/ap)));n=std::min(n,is_last?last:4);
                if(n<1){why="streak taps";return -1;}
                const int8_t to=(is_last || n<4)?results[size_t(i)]:temp[pp];pp=(pp+1)%2;
                GlareDraw g=draw(GlareKind::Streak,"tech_MadT1_S"+std::to_string(n),to);g.inputs[0]=from;g.border=true;
                for(int j=0;j<n;++j){const uint32_t u=uint32_t(j);
                    PutGlareWord(g,136+4*u,(dx*float(j)+0.0f)*1.0f);PutGlareWord(g,137+4*u,(dy*float(j)+0.0f)*1.0f);
                    const float tap=std::pow(0.5f,float(j)*x9)*mp;const auto& row=rows[size_t(7-p)][size_t(j)];
                    for(uint32_t c=0;c<4;++c)PutGlareWord(g,616+4*u+c,tap*row[c]);}
                plan.draws.push_back(g);from=to;++done;
                dx=dx*4.0f;dy=dy*4.0f;x9=x9*4.0f;scale=scale*4.0f;
                if(n<4)break;}
            passes_max=std::max(passes_max,done);}
        plan.streak_arms=A;plan.streak_passes=passes_max;
        // The arms' maximum (FUN_141735F70, tech_MaxColorTn: gamma +0x1BC = 1), into the streak layer (+0x250), point sampled.
        const int8_t streak=surface();GlareDraw g=draw(GlareKind::StreakMax,"tech_MaxColorT"+std::to_string(A),streak);
        for(int i=0;i<A;++i){g.inputs[size_t(i)]=results[size_t(i)];g.point[size_t(i)]=true;
            for(uint32_t c=0;c<4;++c)PutGlareWord(g,616+4*uint32_t(i)+c,arm_weight[size_t(i)]);}
        const float gain=W*5.33333349f;for(uint32_t c=0;c<4;++c)PutGlareWord(g,744+c,gain);   // [142B631D8] 16/3
        plan.draws.push_back(g);layers.push_back(streak);}
    // The ghost (CRenderGlare_Ghost_14174e390, its non-lens-flare path; the star-assisted rows off: +0x17C 0 in every capture).
    const auto clamp=[&](int m){return std::min(std::max(m,0),L-1);};
    // Yf(m) = 4^min(0, clamp(m) - m) / (G1B0^clamp(m) x G2DC^clamp(m)): both 1 (bloom gamma 1, else refused; +0x2DC 1 for R11G11B10).
    const auto Yf=[&](int m){return std::pow(4.0f,float(std::min(0,clamp(m)-m)));};
    const int b=in.base_level;
    if(g118>0.0f && rec.bloom>0.0f && rec.ghost>0.0f){
        // Draw 1 (FUN_141732E90): eight point-symmetric ghosts of levels b, b + 1, b + 2, b + 2 through the trim texture.
        const auto r1=Xorshift(0x47DB4EB0u,0x8979C4D7u,0xA533A50Cu,0x3C169C8Au);
        std::array<float,8> lane0{-0.33f,-0.56f,-0.75f,-0.4f,-0.85f,-0.25f,-0.5f,-1.0f},lane1{};
        bool distorted=false;
        for(size_t k=0;k<8;++k){const float u=((float(r1[k]&0x7FFFFFFFu)*4.65661287e-10f)*2.0f)-1.0f;
            const float sg=u>0.0f?1.0f:u<0.0f?-1.0f:0.0f;float sv=std::pow(std::fabs(u),0.5f)*sg;if(sv<0.0f)sv=sv*0.5f;
            lane1[k]=std::pow(2.5f,in.ghost_distortion*sv);distorted=distorted || lane1[k]!=1.0f;}
        const float ev=in.ev;
        lane0[1]=lane0[1]*(0.75f+0.25f*std::cos(0.0f-3.0f*ev));lane0[3]=lane0[3]*(1.5f+0.5f*std::cos(0.7f*ev));
        lane0[4]=lane0[4]*(0.7f+0.25f*std::cos(2.1f*ev-0.2f));lane0[6]=lane0[6]*(0.7f+0.3f*std::cos(0.4f-1.4f*ev));
        for(auto& v:lane0)v=std::max(v,-1.0f);
        // The ghosts' offsets: 1448E7400's pairs x param_9 = record +0x20 x (G1A8 == 0 ? 0.5 : 1) x 2 (0: d3 = 0 for shape 3). That table
        // has readers only in the image (filled at run time by a writer not found); its signs are OBSERVED in both oracle captures
        // (draw 41's cb0[61..68] and draw 42's cb0[42..49], reversed, hold -0 and +0 alike): x negative for ghosts 0, 2, 3, 5, 6, 7, y for
        // 1, 2, 4, 6. Its magnitudes unknown, a ghost size other than 0 is refused.
        const float param9=(rec.ghost_size*(in.ghost_distortion==0.0f?0.5f:1.0f))*2.0f;
        if(param9!=0.0f){why="ghost size";return -1;}
        static constexpr bool neg_x[8]{true,false,true,true,false,true,true,true},neg_y[8]{false,true,true,false,true,false,true,false};
        const auto offset=[&](size_t g,bool y){return (y?neg_y[g]:neg_x[g])?-1.0f*param9:1.0f*param9;};
        const int8_t A1=level[size_t(clamp(b))],B1=level[size_t(clamp(b+1))],C1=level[size_t(clamp(b+2))];
        const int8_t ghost1=surface(),ghost0=surface();
        const float Sx=1.0f,Sy=1.0f;   // a surface's uv extent (its +0x88..+0x94 scale and offset: 1 and 0, the captured words)
        {GlareDraw g=draw(GlareKind::Ghost,distorted?"tech_GlareGhost_PointSymmetryDistScaleT4_S8_TrimT1_S8":"tech_GlareGhost_PointSymmetryT4_S8_TrimT1_S8",ghost1);
            g.strip=true;g.border=true;g.inputs={A1,B1,C1,C1,-7,-2,-2,-2};
            const auto& sa=plan.surfaces[size_t(A1)];const float ratio=float(sa.width)/float(sa.height);
            float fW=Sx,fH=Sy;if(ratio<=1.0f)fH=ratio*Sy;else fW=Sx/ratio;
            for(uint32_t k=0;k<8;++k){PutGlareWord(g,136+4*k,Sx/2.0f);PutGlareWord(g,137+4*k,Sy/2.0f);
                PutGlareWord(g,168+4*k,distorted?0.0f:offset(k,false));PutGlareWord(g,169+4*k,distorted?0.0f:offset(k,true));
                if(distorted){PutGlareWord(g,200+4*k,lane1[k]);PutGlareWord(g,244+4*k,offset(k,false));PutGlareWord(g,245+4*k,offset(k,true));}
                PutGlareWord(g,584+4*k,lane0[k]==0.0f?1000.0f:1.0f/lane0[k]);PutGlareWord(g,585+4*k,0.0f);PutGlareWord(g,586+4*k,0.0f);PutGlareWord(g,587+4*k,0.0f);}
            if(distorted){const float v[12]{2.0f/fW,2.0f/fH,-1.0f/fW,-1.0f/fH,fW/2.0f,fH/2.0f,0.5f,0.5f,0.0f,0.0f,Sx,Sy};
                for(uint32_t i=0;i<12;++i)PutGlareWord(g,232+i,v[i]);}
            const float Y0=Yf(b),Y1=Yf(b+1),Y2=Yf(b+2),Y3=Yf(b+2);
            // The words as the code holds them (the immediates and .rdata constants of 14174e896..e9b6): 0.02, 0.03 and 0.015 are not
            // the nearest floats (0x3CA3D70B, 0x3CF5C290, 0x3C75C290; the oracle's draw 41 holds them).
            const float n02=Bits(0x3CA3D70Bu),n03=Bits(0x3CF5C290u),n015=Bits(0x3C75C290u);
            const float base_rows[8][3]{{0.05f,n02,n03},{n015,0.05f,n03},{n03,n015,0.05f},{0.05f,n03,n015},
                {n015,0.05f,0.035f},{0.0225f,0.045f,0.075f},{n015,0.05f,0.025f},{0.175f,0.125f,0.25f}};
            const float Y[8]{Y0,Y0,Y0,Y1,Y1,Y2,Y2,Y3};
            for(uint32_t k=0;k<8;++k){for(uint32_t c=0;c<3;++c){float v=base_rows[k][c]*Y[k];if(k==7)v=v*0.3333f;PutGlareWord(g,616+4*k+c,v);}
                PutGlareWord(g,619+4*k,0.0f);}
            plan.draws.push_back(g);}
        // Draw 2 (FUN_141733B90, tech_GlareGhost_PointSymmetryT3_S8): draw 1's ghosts and levels b + 2 and b again.
        {GlareDraw g=draw(GlareKind::Ghost,"tech_GlareGhost_PointSymmetryT3_S8",ghost0);g.strip=true;g.border=true;g.inputs={ghost1,C1,A1,-2,-2,-2,-2,-2};
            static constexpr std::array<float,8> lane2{1.0f,0.2f,-0.3f,-1.0f,-7.0f,4.0f,2.0f,-1.6f};
            const float FV=(((g114*0.035f)*rec.bloom)*g118)*((rec.ghost*0.05f)*20.0f);
            const float s14=rec.ghost_scale*1.414f;float Z=Yf(b+2);if(s14!=1.0f)Z=Z*std::pow(s14,float(clamp(b+2)-b));
            const float Wl=Yf(b);
            // The .rdata constants (glare-shape3-ghost.md 2.1): 4.08 [142B631D4], 0.16 [142B631A4], 0.02 [142B63184] and 0.01 [142746DC8]
            // are not the nearest floats (0x40828F5D, 0x3E23D70B, 0x3CA3D70B, 0x3C23D70B).
            const float n408=Bits(0x40828F5Du),n016=Bits(0x3E23D70Bu),n02=Bits(0x3CA3D70Bu),n001=Bits(0x3C23D70Bu);
            const float rows2[8][3]{{0.15f,0.375f,1.5f},{1.0f,0.2f,0.5f},{0.6f,1.8f,1.6f},{1.44f,0.72f,n408},{0.032f,0.008f,n016},
                {0.014f,0.002f,n02},{0.001f,0.004f,n02},{n001,0.0005f,0.002f}};
            for(uint32_t k=0;k<8;++k){PutGlareWord(g,136+4*k,Sx/2.0f);PutGlareWord(g,137+4*k,Sy/2.0f);   // the offsets in reversed ghost order
                PutGlareWord(g,168+4*k,offset(7-k,false));PutGlareWord(g,169+4*k,offset(7-k,true));
                // lane 1 is not written by the library (a stack leftover the programme does not read): 0 here.
                PutGlareWord(g,584+4*k,1.0f/lane2[k]);PutGlareWord(g,585+4*k,0.0f);PutGlareWord(g,586+4*k,0.0f);PutGlareWord(g,587+4*k,0.0f);
                for(uint32_t c=0;c<3;++c){float v=rows2[k][c]*FV;if(k==4 || k==5)v=v*Z;else if(k>=6)v=v*Wl;PutGlareWord(g,616+4*k+c,v);}
                PutGlareWord(g,619+4*k,0.0f);}
            plan.draws.push_back(g);}
        layers.push_back(ghost0);}
    // The afterimage (YebisAfterimage_141750000): level 0 over the history kept across frames, tech_MadOffsetT2.
    if(g118>0.0f && rec.bloom>0.0f && rec.after_time>0.0f && rec.after_weight>0.0f){
        const int8_t history=surface();plan.history=history;int8_t current=history;
        if(!in.afterimage_skip && in.frame_time>0.0f){current=surface();
            const float K=(((g114*0.035f)*rec.bloom)*g118)*(rec.after_weight*0.125f);
            const float r=in.frame_time/rec.after_time,c0=1.6f;   // 1.6 for a float format (R11G11B10)
            const float A0=1.0f-std::pow(0.5f,((c0*c0)*r)/0.05f),B0=std::pow(0.5f,(r/c0)/0.05f);
            float Aw=(0.0f<A0 && A0<1.0f)?A0:1.0f,Bw=(0.0f<B0 && B0<1.0f)?B0:0.0f;Aw=std::max(Aw,0.01f);Bw=std::min(Bw,0.999f);
            const float Wc=(Aw*K)*1.0f;   // x 1 / (G1B0^0 x G2DC^0)
            GlareDraw g=draw(GlareKind::Afterimage,"tech_MadOffsetT2",current);g.border=true;g.inputs[0]=level[0];g.inputs[1]=history;g.copy_to=history;
            const uint32_t offset_bits=0xB8000100u;float offset;std::memcpy(&offset,&offset_bits,4);   // [142B631DC] -3.0518509e-05 (format 26)
            for(uint32_t c=0;c<4;++c){PutGlareWord(g,616+c,Wc);PutGlareWord(g,620+c,Bw);PutGlareWord(g,744+c,offset);PutGlareWord(g,748+c,0.0f);PutGlareWord(g,752+c,64000.0f);}
            plan.draws.push_back(g);}
        layers.push_back(current);}
    // The layers' composite (YebisGlareComposite_1417511c0): MadT{n} of the layers, every weight 1, point sampled.
    if(layers.size()<2)return combine;
    const int8_t out=surface();GlareDraw g=draw(GlareKind::Layers,"tech_MadT"+std::to_string(layers.size()),out);
    for(size_t i=0;i<layers.size();++i){g.inputs[i]=layers[i];g.point[i]=true;for(uint32_t c=0;c<4;++c)PutGlareWord(g,616+4*uint32_t(i)+c,1.0f);}
    plan.draws.push_back(g);
    return out;}
}
std::array<uint32_t,2> GlareBaseSize(const GlareData& d,uint32_t width,uint32_t height,int quality){
    if(!width || !height || quality<0 || quality>7)return {0,0};
    // CPostEffect::Initialize: baseW = (int)(W scale sqrt((A / r) / A) + 0.99), baseH = (int)(baseW / (A / r) + 0.499999).
    const float aspect=float(width)/float(height),ratio=aspect/d.glare_aspect;
    const uint32_t base_w=uint32_t(int64_t(float(width)*d.quality_scale[size_t(quality)]*std::sqrt(ratio/aspect)+d.base_ceiling));
    return {base_w,uint32_t(int64_t(float(base_w)/ratio+d.height_rounding))};}
int GlareBaseLevel(uint32_t base_width,uint32_t width){
    // 141753937..14175397f: CVTSI2SS of both, DIVSS, logf, logf(2.0) [143D7EE7C], DIVSS, CVTTSS2SI, + 2. A zero width (a quality the
    // builder refuses) gives 0 here instead of converting -inf.
    if(!base_width || !width)return 0;
    return int(std::log(float(base_width)/float(width))/std::log(2.0f))+2;}
float GlareRotation(float tan_half,int mode,float scale,float offset_degrees){
    // [143D7EDEC] pi/4, [142B55240] pi/8 (not the nearest float), [143D7ED40] pi/180, [143D7EDB4] 0.5 (1416eb490..1416eb5e8, READ).
    const float pi4=Bits(0x3F490FDBu),pi8=Bits(0x3EC90FD0u),radians=Bits(0x3C8EFA35u);
    const float t=std::tan((std::atan(tan_half)*2.0f)*0.5f);
    const float B=offset_degrees*radians;
    float x;
    if(mode==0)x=t*-1.0f+(pi4-std::tan(pi8)*-1.0f);
    else if(mode==1)x=std::log(t)*-1.0f+(pi4-std::log(std::tan(pi8))*-1.0f);
    else if(mode==2)x=(1.0f/t)*0.2f+(pi4-(1.0f/std::tan(pi8))*0.2f);
    else if(mode==3)return scale+B;
    else x=0.0f;
    return x*scale+B;}
GlarePlan BuildGlare(const GlareData& d,const GlareInputs& in){
    GlarePlan plan;
    const auto refuse=[&](const char* why){plan.refused=why;plan.draws.clear();plan.surfaces.clear();plan.output=-1;return plan;};
    if(!in.width || !in.height)return refuse("extent");
    if(in.quality<0 || in.quality>7)return refuse("quality");
    if(in.levels<3 || in.levels>16)return refuse("levels");
    // I-0138 tranche 8f: GlareShape 1 (the bloom) and 3 (« Standard »: the bloom's levels colour-cycled, four-arm streaks, a ghost,
    // an afterimage); the others the library draws are refused (the caller names them first, GlareShapeGate).
    const bool standard=in.shape==3;
    if(in.shape!=1 && !standard)return refuse("shape");
    if(!standard && in.shape_tint>0.0f)return refuse("tinted shape");   // shape 1's record holds no tint
    if(standard && (in.record.lens_flare || in.record.colour_flag || in.record.arms<1 || in.record.tint_levels<0))return refuse("shape record");
    if(in.bloom_gamma!=1.0f)return refuse("bloom gamma");           // the level weight chain under +0x1B0 != 1 is not checked yet
    const int D=2;                                                   // levels kept at the base size (+0xD4)
    const auto [base_w,base_h]=GlareBaseSize(d,in.width,in.height,in.quality);
    if(base_w<16 || base_h<16)return refuse("base size");
    plan.base_width=base_w;plan.base_height=base_h;plan.width=in.width;plan.height=in.height;
    const auto surface=[&](uint32_t w,uint32_t h,bool half){plan.surfaces.push_back({w,h,half});return int8_t(plan.surfaces.size()-1);};
    const auto draw=[&](GlareKind kind,const char* technique,int8_t target){GlareDraw g;g.kind=kind;g.technique=technique;g.target=target;Common(g);return g;};
    // The source chain (FUN_14174BED0, +0x220/+0x228): 2:1 bilinear downsamples of the scene while the halved size stays at least
    // the base (1280x720 -> 640x360 -> 320x180 at a 210x177 base; INFERRED rule, the list is built at initialisation).
    int8_t source=-1;uint32_t sw=in.width,sh=in.height;
    while((sw+1)/2>=base_w && (sh+1)/2>=base_h){sw=(sw+1)/2;sh=(sh+1)/2;const int8_t t=surface(sw,sh,true);
        GlareDraw g=draw(GlareKind::Downsample,"tech_MadT1_S1",t);g.inputs[0]=source;g.border=true;
        for(uint32_t c=0;c<4;++c)Put(g,616+c,1.0f);plan.draws.push_back(g);source=t;}
    // The bright pass (FUN_141736FE0 -> FUN_141722480, remap mode 3): Modulate[0] (e, e, e, 1), [1] (1/F, 1/F, 1/F, 1), [2] t x 4,
    // fParam_BrightPassRemapFactor (4 / p^2 - 2, 1, 1 / p^2, p^2 / 2), from the last source into the base size (RGBA16F).
    // I-0138 tranche 8f: YebisGlareBrightPass_14174b830 (READ, evidence/I-0138/t8f-readings/bright-pass-modes.md) turns mode 3 into
    // mode 2 when the remap factor (+0x124) is exactly 1 (THRESHOLD_LUMINANCE without REMAP, the remap word not written: 32000020 at
    // frame 900), and mode 2 into 0 when the threshold (+0x11C) is 0 (mode 0, not written: refused here; the frame serves the family
    // empty before building, 13b). Mode 3 with another remap factor stays 3 whatever the threshold.
    const bool remap=in.remap!=1.0f;
    if(!remap && in.threshold==0.0f)return refuse("bright pass mode 0");
    const int8_t bright=surface(base_w,base_h,true);
    {const std::string technique=std::string(in.vignette?"tech_BrightPassT1_Vignette_THRESHOLD_LUMINANCE":"tech_BrightPassT1_THRESHOLD_LUMINANCE")+(remap?"_REMAP_S1":"_S1");
        GlareDraw g=draw(GlareKind::BrightPass,technique.c_str(),bright);
        g.inputs[0]=source;g.border=true;if(in.vignette){g.texcoord1=true;g.uv1=in.vignette_uv;}
        const float inv_f=1.0f/in.factor;
        for(uint32_t c=0;c<3;++c){Put(g,616+c,in.exposure);Put(g,620+c,inv_f);}Put(g,619,1.0f);Put(g,623,1.0f);
        for(uint32_t c=0;c<4;++c)Put(g,624+c,in.threshold);
        if(remap){const float p2=in.remap*in.remap,inv_p2=1.0f/(in.remap*in.remap);
            Put(g,836,inv_p2*4.0f-2.0f);Put(g,837,1.0f);Put(g,838,inv_p2);Put(g,839,p2*0.5f);}
        plan.draws.push_back(g);}
    // The pyramid (FUN_14174C160): R = 2^(1 - min(D, N)) C s (+0x1AC); level i: radius rec[+8] R (x sqrt 0.75 after the first),
    // per channel (r, r 540/615, r 465/615) through ((r x lambda) / lambda_red); levels from D on start with a 2:1 downsample.
    // FUN_141748D50: the levels are capped at D - 2 + (int)(log(baseW) / log(2)) (m31: NumLevels 8, base 210: 7 drawn).
    const int N=std::min(in.levels,D-2+int(std::log(float(base_w))/std::log(2.0f)));plan.levels=N;
    const float radius=std::pow(2.0f,float(1-std::min(D,N)))*d.radius_c*in.radius_scale;plan.radius=radius;
    std::vector<int8_t> result(size_t(N),-1),composite_input(size_t(N),-1);
    int8_t previous=bright;uint32_t pw=base_w,ph=base_h;
    for(int i=0;i<N;++i){
        // FUN_141749790: level i scale 2^-(i - D + 1) from D on (else 1), size (uint)(base scale + 0.99) (3 under 4), rec[+8] = 2^i scale.
        const float scale=i<D?1.0f:std::pow(2.0f,-float(i-D+1));
        uint32_t lw=uint32_t(int64_t(float(base_w)*scale+d.base_ceiling)),lh=uint32_t(int64_t(float(base_h)*scale+d.base_ceiling));
        if(lw<4)lw=3;if(lh<4)lh=3;
        const float rec8=std::pow(2.0f,float(i))*scale;   // 1, 2, 2, 2 ... (Reference1649's level records)
        int8_t src=previous;
        if(i>=D){const int8_t t=surface(lw,lh,false);GlareDraw g=draw(GlareKind::Downsample,"tech_MadT1_S1",t);g.inputs[0]=previous;g.border=true;
            for(uint32_t c=0;c<4;++c)Put(g,616+c,1.0f);plan.draws.push_back(g);src=t;}
        float level_radius=rec8*radius;if(i>0)level_radius=level_radius*std::sqrt(d.level_radius);
        const float sigma[4]{(level_radius*d.wavelength[0])/d.wavelength[0],(level_radius*d.wavelength[1])/d.wavelength[0],
            (level_radius*d.wavelength[2])/d.wavelength[0],0.0f};
        const int k=TapIndex(d,sigma[0],in.sampling_threshold);
        // FUN_14173FDF0: k = 2, 4, 6 take the helpers FUN_14173DB50, FUN_14173E6B0, FUN_14173F250 (3, 7, 11 taps normalised over
        // themselves); otherwise 2k - 1 taps (at most 15) of FUN_1416DC220's 15 (the point path: the channels' radii differ).
        const bool own=k==2 || k==4 || k==6;const uint32_t taps=own?uint32_t(2*k-1):uint32_t(std::min(2*k-1,15));
        const uint32_t distances=own?uint32_t(k-1):7u;
        const float weight_root=std::sqrt(1.0f);                        // sqrt(w), w = 1 while bloom gamma is 1 (refused otherwise)
        const int8_t half=surface(lw,lh,false),full=surface(lw,lh,false);
        const std::string technique="tech_MadT1_S"+std::to_string(taps);
        for(int pass=0;pass<2;++pass){
            const uint32_t size=pass?lh:lw;
            GlareDraw g=draw(GlareKind::Blur,technique.c_str(),pass?full:half);g.inputs[0]=pass?half:src;g.point[0]=true;g.border=true;
            for(uint32_t c=0;c<4;++c){std::array<float,15> o{},w{};BlurWeights(d,size,sigma[c],false,distances,o,w);
                // The first eight taps' offsets in afUV_TexCoordOffsetV16 (the VS, cb0[34 + t]), the others in cb0[42 + t] (the PS).
                for(uint32_t t=0;t<taps;++t){if(!c)Put(g,(t<8?136:168)+4*t+(pass?1:0),o[t]);Put(g,616+4*t+c,weight_root*w[t]);}}
            plan.draws.push_back(g);}
        result[size_t(i)]=full;composite_input[size_t(i)]=full;
        // The level's 2x surface (+0x28): levels from 1 whose width is at most +0xA8 >> 4, and the last level; drawn right after
        // the level (FUN_14172A660, four taps at +-0.375 source texel, 0.25 each; FUN_1416D6570 forms them as (1 / size) x 0.375).
        if(i>=2 && (lw<=(in.width>>4) || i==N-1)){const int8_t up=surface(lw*2,lh*2,false);
            GlareDraw g=draw(GlareKind::Upsample,"tech_MadT1_S4",up);g.inputs[0]=full;g.border=true;
            const float ox=(1.0f/float(lw))*d.up_offset,oy=(1.0f/float(lh))*d.up_offset;const float sx[4]{-ox,ox,-ox,ox},sy[4]{-oy,-oy,oy,oy};
            for(uint32_t t=0;t<4;++t){Put(g,136+4*t,sx[t]);Put(g,137+4*t,sy[t]);for(uint32_t c=0;c<4;++c)Put(g,616+4*t+c,0.25f);}
            plan.draws.push_back(g);composite_input[size_t(i)]=up;}
        previous=full;pw=lw;ph=lh;}
    (void)pw;(void)ph;
    // The composite (FUN_14174C9E0 -> FUN_141735980): w_i = (1 / q^i) L with q = 1 (+0x2DC for the R11G11B10 pyramid) and L =
    // F 0.035 shape(+8) luminance shape(+0xC) (FUN_141751DE0, bloom gamma 1); MadLumGammaT{N} with exponent 1 / gamma (word 808).
    const float shape_bloom=standard?in.record.bloom:in.shape_bloom,shape_scale=standard?in.record.scale:in.shape_scale;
    const float luminance=(((in.factor*d.luminance_scale)*shape_bloom)*in.luminance)*shape_scale;plan.luminance=luminance;
    if(N>8)return refuse("composite inputs");                         // eight textures: the post draw's table width
    const int8_t glare=surface(base_w,base_h,false);
    // MadT{N} when the luminance gamma is 1 (m31, m45), MadLumGammaT{N} with exponent 1 / gamma otherwise.
    const bool gamma=in.composite_gamma!=1.0f;
    {GlareDraw g=draw(GlareKind::Composite,"",glare);g.technique=(gamma?"tech_MadLumGammaT":"tech_MadT")+std::to_string(N);
        for(int i=0;i<N;++i){g.inputs[size_t(i)]=composite_input[size_t(i)];const auto& s=plan.surfaces[size_t(composite_input[size_t(i)])];
            g.point[size_t(i)]=s.width==base_w && s.height==base_h;
            const float w=(1.0f/std::pow(1.0f,float(i)))*luminance;std::array<float,4> lane{w,w,w,w};
            // I-0138 tranche 8f: YebisBloomLevelCombine_14174c9e0 under a tinted shape (record +0x10 > 0): from level first =
            // min(+0x2F8 - 1 + record +0x14, levels - 3) (at least 0) on, the level's four equal lanes become blue, green, red in turn
            // (1448E8F70's colours 1 to 3, READ, glare-shape3-passes.md « Side finding »; under a negative d7 another table, 1448E8FC0
            // reversed, INFERRED and unreachable here: d7 is 0.5 at shape 3), (c - 1) tint + 1, scaled so that |r| + |g| + |b| = 3 (READ;
            // the order 14174cc4c..ccf7: (3 / sum) x w, then x lane).
            if(standard && in.record.tint>0.0f){const int first=std::max(0,std::min(in.base_level-1+in.record.tint_levels,N-3));
                if(i>=first){const auto& c=StreakColour(in.record.colour,(i-first)%3+1);
                    for(uint32_t l=0;l<4;++l)lane[l]=(c[l]-1.0f)*in.record.tint+1.0f;
                    const float kw=(3.0f/((std::fabs(lane[0])+std::fabs(lane[1]))+std::fabs(lane[2])))*w;for(auto& v:lane)v=kw*v;}}
            for(uint32_t c=0;c<4;++c)Put(g,616+4*uint32_t(i)+c,lane[c]);}
        if(gamma)Put(g,808,1.0f/in.composite_gamma);plan.draws.push_back(g);}
    int8_t into_output=glare;
    if(standard){std::string why;into_output=AppendStandard(d,in,plan,bright,result,glare,why);if(into_output<0)return refuse(why.c_str());}
    // The tone map's input (the original draws it after the light shafts' MadT2 or a MadT1 copy, absent here): 2x the base width
    // capped at half the extent, the height the base's scaled alike (INFERRED from m37 420x354, m50 640x540 and 32000020 314x264 at
    // bases 210x177, 392x331 and 157x132: the glare aspect's 314 / 1.185 gave 265 there; the surface is CPostEffect's).
    const uint32_t out_w=std::min(base_w*2,in.width/2),out_h=uint32_t(int64_t(float(base_h)*(float(out_w)/float(base_w))+d.height_rounding));
    const int8_t output=surface(out_w,out_h,false);
    {GlareDraw g=draw(GlareKind::Upsample,"tech_MadT1_S4",output);g.inputs[0]=into_output;g.border=true;
        const float ox=(1.0f/float(base_w))*d.up_offset,oy=(1.0f/float(base_h))*d.up_offset;const float sx[4]{-ox,ox,-ox,ox},sy[4]{-oy,-oy,oy,oy};
        for(uint32_t t=0;t<4;++t){Put(g,136+4*t,sx[t]);Put(g,137+4*t,sy[t]);for(uint32_t c=0;c<4;++c)Put(g,616+4*t+c,0.25f);}
        plan.draws.push_back(g);}
    plan.output=output;
    return plan;
}
}
