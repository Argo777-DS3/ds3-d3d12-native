#include "producers.h"
#include <DirectXMath.h>
#include <cmath>
#include <cstdlib>
#include <atomic>
#include <immintrin.h>

namespace ds3prod {
namespace {
using namespace DirectX;
void Put(uint8_t* out,uint32_t offset,const float* v,unsigned n) { std::memcpy(out+offset,v,4u*n); }
// Stores a row-vector matrix as rows for column vectors (its transpose), n rows.
void PutT(uint8_t* out,uint32_t offset,FXMMATRIX m,unsigned rows) { XMFLOAT4X4 t; XMStoreFloat4x4(&t,XMMatrixTranspose(m)); Put(out,offset,&t.m[0][0],4u*rows); }
}
namespace { std::atomic<uint64_t> g_render_size{0}; }
void UseRenderSize(uint32_t width,uint32_t height) { g_render_size=width && height?(static_cast<uint64_t>(width)<<32)|height:0; }
void ProduceSceneCamera(const Camera& k,uint8_t out[960]) {
    const uint64_t rs=g_render_size; const uint32_t rw=rs?static_cast<uint32_t>(rs>>32):k.width, rh=rs?static_cast<uint32_t>(rs):k.height;
    const float w=static_cast<float>(rw),h=static_cast<float>(rh);
    const float screen[4]={w,h,1.0f/w,1.0f/h};
    Put(out,16,screen,4);
    // Left-handed perspective, depth 0 at near, 1 at far: z' = A z + B, w' = z.
    const float ty=std::tan(k.fovy*0.5f), tx=ty*k.aspect;
    const float sy=1.0f/ty, sx=1.0f/tx;
    const float a=k.far_z/(k.far_z-k.near_z), b=-k.near_z*a;
    const float depth[4]={a,-k.near_z/(k.far_z-k.near_z),b,0.0f};
    Put(out,32,depth,4);
    const float pos[4]={k.position[0],k.position[1],k.position[2],1.0f};
    Put(out,48,pos,4);
    const float clip[4]={0,0,0,1};
    Put(out,64,clip,4);
    // Row-vector matrices (v' = v M), as the engine's pose is stored.
    const XMMATRIX pose(k.right[0],k.right[1],k.right[2],0, k.up[0],k.up[1],k.up[2],0, k.forward[0],k.forward[1],k.forward[2],0, k.position[0],k.position[1],k.position[2],1);
    // Off-center form: the engine stores -0 for the zero centre offsets (m[2][0], m[2][1]); I-0111: the upscaler's jitter there
    // (m20 = 2 jx / W, m21 = -2 jy / H), so every matrix derived below (InvProj, InvViewProj, VC_MatrixViewProj) carries it.
    const bool jittered=k.jitter[0]!=0 || k.jitter[1]!=0;
    const float m20=jittered?2.0f*k.jitter[0]/w:-0.0f, m21=jittered?-2.0f*k.jitter[1]/h:-0.0f;
    const XMMATRIX proj(sx,0,0,0, 0,sy,0,0, m20,m21,a,1, 0,0,b,0);
    const XMMATRIX view=XMMatrixInverse(nullptr,pose);
    PutT(out,80,view,3);
    PutT(out,256,proj,4);
    const XMMATRIX inv_proj=XMMatrixInverse(nullptr,proj);
    PutT(out,192,inv_proj,4);
    PutT(out,320,pose,4);
    PutT(out,128,XMMatrixMultiply(inv_proj,pose),4);
    // Camera-relative: the view's translation cleared, its w kept (the generic inverse can leave 1 +- 1 ulp there, and the engine's
    // VC_MatrixViewProj carries it: i63-live-24 frame 150, word 107 one ulp off and 737 texels when w was forced to 1).
    XMMATRIX rotation=view; rotation.r[3]=XMVectorSet(0,0,0,XMVectorGetW(view.r[3]));
    PutT(out,384,XMMatrixMultiply(rotation,proj),4);
    Put(out,880,&k.near_z,1);
}
}
namespace ds3prod {
namespace {
float F(const uint8_t* p,uint32_t o) { float v; std::memcpy(&v,p+o,4); return v; }
// 14001CE40: row-major A x B, each output row (a.z B2 + a.x B0) + (a.w B3 + a.y B1) (SSE order, as light_producers Multiply).
void EngineMultiply(const float* a,const float* b,float* out) { for(int i=0;i<4;++i) for(int j=0;j<4;++j) { const float* r=a+4*i; out[4*i+j]=(r[2]*b[8+j]+r[0]*b[j])+(r[3]*b[12+j]+r[1]*b[4+j]); } }
uint32_t U(const uint8_t* p,uint32_t o) { uint32_t v; std::memcpy(&v,p+o,4); return v; }
// 140004BCA0: the engine's 4x4 inverse (cofactors, one reciprocal of the determinant), same operation order.
void EngineInverse(const float* m,float* r) {
    const float f7=m[8],f16=m[9],f8=m[10],f9=m[11],f3=m[14],f5=m[15],f1=m[12],f13=m[13],f10=m[0],f17=m[1],f11=m[2],f12=m[3],f4=m[6],f6=m[7],f2=m[4],f15=m[5];
    const float f26=f9*f11-f8*f12, f27=f5*f11-f3*f12, f28=f9*f4-f8*f6, f29=f5*f4-f3*f6;
    const float f30=f10*f16-f7*f17, f31=f10*f13-f1*f17, f32=f2*f16-f7*f15, f33=f2*f13-f1*f15;
    const float f18=f11*f6-f4*f12, f20=f8*f5-f3*f9, f21=f10*f15-f2*f17, f23=f7*f13-f1*f16;
    const float f19=(f15*f20-f16*f29)+f28*f13, f22=(f17*f29-f15*f27)+f18*f13;
    const float f24=(f16*f27-f13*f26)-f20*f17, f25=(f15*f26-f16*f18)-f28*f17;
    const float det=f2*f24+f1*f25+f10*f19+f7*f22;
    const float q=1.0f/det;
    r[4]=((f7*f29-f1*f28)-f2*f20)*q; r[5]=((f10*f20-f7*f27)+f1*f26)*q; r[6]=((f2*f27-f1*f18)-f10*f29)*q; r[7]=((f10*f28-f2*f26)+f7*f18)*q;
    r[8]=((f6*f23-f9*f33)+f32*f5)*q; r[9]=((f9*f31-f5*f30)-f23*f12)*q; r[10]=((f12*f33-f6*f31)+f21*f5)*q; r[11]=((f6*f30-f9*f21)-f32*f12)*q;
    r[0]=q*f19; r[1]=q*f24; r[2]=q*f22; r[3]=q*f25;
    r[12]=((f33*f8-f32*f3)-f23*f4)*q; r[13]=((f23*f11-f31*f8)+f30*f3)*q; r[14]=((f31*f4-f21*f3)-f33*f11)*q; r[15]=((f32*f11-f30*f4)+f21*f8)*q;
}
}
// I-0044: the 140004BCA0 transcription for the lighting producers.
void InverseMatrix(const float* m,float* r) { EngineInverse(m,r); }
void ProduceInstanceRecord(const InstanceRecordInputs& s,uint8_t out[instance_record_bytes],uint8_t written[instance_record_bytes/4]) {
    auto put=[&](uint32_t o,float v) { std::memcpy(out+o,&v,4); written[o/4]=1; };
    auto putu=[&](uint32_t o,uint32_t v) { std::memcpy(out+o,&v,4); written[o/4]=1; };
    auto copy=[&](uint32_t o,const uint8_t* p,uint32_t bytes) { std::memcpy(out+o,p,bytes); for(uint32_t i=0;i<bytes;i+=4) written[(o+i)/4]=1; };
    // Lighting rows (+0x80..+0xAF) and blend vector (+0xB0), for draw parameter types 0, 3, 4.
    if(s.lighting) {
        const auto& light=*s.lighting;
        float rows[12]; float f46=1,f47=1,f48=1,f49=1,f41=1;
        if(light.state_owned) { for(int k=0;k<12;++k) rows[k]=light.animation_rows[k]; f46=light.state_scale[0];f47=light.state_scale[1];f48=light.state_scale[2];f49=light.state_scale[3];f41=light.state_strength; }
        else if(!light.render_override) {
            const float t=light.blend;
            if(!light.animation_owned && light.banks_present)
                for(int k=0;k<12;++k) { const float l1=light.bank_rows[0][k],l2=light.bank_rows[1][k]; rows[k]=(l2-l1)*t+l1; }
            else if(light.animation_owned) { for(int k=0;k<12;++k) rows[k]=light.animation_rows[k]; f46=f47=f48=f49=light.animation_strength; }
            else for(float& r:rows) r=0;
            if(light.banks_present) {
                const float l1=light.bank_strength[0],l2=light.bank_strength[1];
                f41=1.1920929e-07f<t?(t<1.0f?(l2-l1)*t+l1:l2):l1;
            }
        } else for(int k=0;k<12;++k) rows[k]=light.override_rows[k];
        // Scale: x, y, z by 1; w of row r by ctx+0x110+4r.
        for(int r=0;r<3;++r) { rows[4*r]*=1.0f; rows[4*r+1]*=1.0f; rows[4*r+2]*=1.0f; rows[4*r+3]*=light.row_scale[r]; }
        for(int k=0;k<12;++k) put(0x80+4u*k,rows[k]);
        put(0xB0,(f46-1.0f)*f41+1.0f); put(0xB4,(f47-1.0f)*f41+1.0f); put(0xB8,(f48-1.0f)*f41+1.0f); put(0xBC,(f49-1.0f)*f41+1.0f);
    }
    // World matrix (+0x00) and palette base (+0x50).
    if(s.object_rows) {
        copy(0,reinterpret_cast<const uint8_t*>(s.object_rows->data()),48);
        putu(0x50,s.palette_base);
    }
    // +0x40: group flag 2 transforms the group vector +0xE0 by the inverse of the context matrix +0x90.
    if(s.clip && !s.clip->enabled) { put(0x40,0); put(0x44,0); put(0x48,0); put(0x4C,0); }
    else if(s.clip) {
        float inv[16];EngineInverse(s.clip->context.data(),inv);
        const float x=s.clip->vector[0],y=s.clip->vector[1],z=s.clip->vector[2],w=s.clip->vector[3];
        for(int r=0;r<4;++r) put(0x40+4u*r,w*inv[4*r+3]+z*inv[4*r+2]+y*inv[4*r+1]+x*inv[4*r]);
    }
    if(s.animation_colour)copy(0x30,reinterpret_cast<const uint8_t*>(s.animation_colour->data()),16);
    if(s.animation_auxiliary)copy(0xC0,reinterpret_cast<const uint8_t*>(s.animation_auxiliary->data()),16);
    copy(0x70,reinterpret_cast<const uint8_t*>(s.group_colour.data()),16);
    // Bounds (+0xE0, +0xF0): group +0x154.. unless the instance has +0x110/+0x118 set.
    if(!s.bounds_suppressed) { copy(0xE0,reinterpret_cast<const uint8_t*>(s.bounds_min.data()),12); copy(0xF0,reinterpret_cast<const uint8_t*>(s.bounds_max.data()),12); }
    else { put(0xE0,0); put(0xE4,0); put(0xE8,0); put(0xF0,0); put(0xF4,0); put(0xF8,0); }
    copy(0xD0,reinterpret_cast<const uint8_t*>(s.origin.data()),12);put(0xDC,s.origin_w);put(0xEC,s.bounds_min_w);put(0xFC,s.bounds_max_w);
    if(s.type_value)putu(0x60,*s.type_value);
    put(0x100,s.opacity);
    putu(0x64,s.instance_flag?1u:0u);
}
void ProduceInstanceRecord(const InstanceState& s,uint8_t out[instance_record_bytes],uint8_t written[instance_record_bytes/4]) {
    // Historical diagnostic decoder only. Both entries use the same native
    // producer; the integrated engine supplies owned values directly.
    InstanceRecordInputs input;const auto* g=s.group;const auto* a=s.anim;
    std::array<float,12> world{};
    if(s.object_matrix){std::memcpy(world.data(),s.object_matrix_bytes,48);
        input.palette_base=s.palette_destination?s.object_bones*s.index:0u;}
    else {for(int r=0;r<3;++r)for(int c=0;c<4;++c)world[4*r+c]=F(s.default_matrix,16u*c+4u*r);input.palette_base=s.index;}
    input.object_rows=world;
    InstanceClipInputs clip;clip.enabled=(U(g,0x98)&2u)!=0;
    if(clip.enabled){std::memcpy(clip.context.data(),s.ctx90,64);std::memcpy(clip.vector.data(),g+0xE0,16);}input.clip=clip;
    if(a){std::array<float,4> colour{},auxiliary{};
        const bool e0=a[0xE0-0x80]!=0,e1=a[0xE1-0x80]!=0;
        if(!e0 && !e1)std::memcpy(colour.data(),a+0xB8-0x80,16);else colour={1,1,0,0};
        std::memcpy(auxiliary.data(),e1?s.default_light:a+0xD0-0x80,16);
        input.animation_colour=colour;input.animation_auxiliary=auxiliary;}
    if(s.params_type==0 || static_cast<uint32_t>(s.params_type-3)<2u){
        InstanceLightingInputs light;light.render_override=s.render_override;light.blend=F(g,0x108);
        light.banks_present=s.light[0] && s.light[1];light.animation_owned=a && a[0x170-0x80];
        if(light.banks_present)for(size_t i=0;i<2;++i){std::memcpy(light.bank_rows[i].data(),s.light[i],48);light.bank_strength[i]=F(s.light[i],0x30);}
        if(light.animation_owned){std::memcpy(light.animation_rows.data(),a+0x130-0x80,48);light.animation_strength=F(a,0x16C-0x80);}
        if(light.render_override)std::memcpy(light.override_rows.data(),s.render190,48);
        std::memcpy(light.row_scale.data(),s.ctx110,12);input.lighting=light;
    }
    std::memcpy(input.group_colour.data(),g+0xD0,16);std::memcpy(input.origin.data(),g+0x148,12);
    std::memcpy(input.bounds_min.data(),g+0x154,12);std::memcpy(input.bounds_max.data(),g+0x160,12);
    input.origin_w=F(g,0x13C);input.bounds_min_w=F(g,0x140);input.bounds_max_w=F(g,0x144);input.opacity=F(g,0x124);
    uint64_t first{},second{};std::memcpy(&first,s.instance+0x110,8);std::memcpy(&second,s.instance+0x118,8);
    input.bounds_suppressed=first || second;input.type_value=(s.state_type==6 || s.state_type==7)?s.type_value:0u;
    input.instance_flag=s.instance[0x128]!=0;
    ProduceInstanceRecord(input,out,written);
}
}
namespace ds3prod {
namespace {
#define SH(a,b,i) _mm_shuffle_ps(a,b,i)
// 1400179C0 transcribed instruction by instruction (SSE): inverse of [m (3 rows); 0 0 0 1], three output rows.
void EngineAffineInverseSse(const float* m,float* out) {
    auto mask=[](int lane) { alignas(16) uint32_t v[4]{}; v[lane]=0x80000000u; return _mm_load_ps(reinterpret_cast<const float*>(v)); };
    const __m128 m2850=mask(3), m2860=mask(2), m2880=mask(1), m28c0=mask(0);
    __m128 x5=m2860, x4=m2880;
    __m128 x6=_mm_loadu_ps(m+4), x10=_mm_setr_ps(0,0,0,1), x13=_mm_loadu_ps(m+8), x14=_mm_loadu_ps(m);
    __m128 x11=x6, x0=x10; x0=SH(x0,x10,0x50); x11=SH(x11,x6,0xee);
    __m128 x1=x13, x8=x10; x1=SH(x1,x13,0xee); x8=SH(x8,x10,0xee);
    __m128 x12=x6, x3=x5; x1=_mm_mul_ps(x1,x0); x12=SH(x12,x6,0x26);
    x0=x13; __m128 x9=x10, x2=x10; x0=SH(x0,x13,0x50); x9=SH(x9,x6,0xdd); x2=SH(x2,x10,0xcc);
    __m128 x7=x13; x5=_mm_xor_ps(x5,x2); x8=_mm_mul_ps(x8,x0); x7=SH(x7,x13,0x36);
    x0=x6; x7=_mm_xor_ps(x7,m2880); x8=_mm_sub_ps(x8,x1); x0=SH(x0,x6,0x50);
    x1=x14; x1=SH(x1,x14,0xee); x1=_mm_mul_ps(x1,x0);
    x0=x14; x0=SH(x0,x14,0x50); x11=_mm_mul_ps(x11,x0);
    x0=x10; x0=SH(x0,x6,0x88); x11=_mm_sub_ps(x11,x1);
    x1=x13; x1=SH(x1,x14,0xdd); x1=_mm_mul_ps(x1,x0);
    x0=x13; x0=SH(x0,x14,0x88); x9=_mm_mul_ps(x9,x0);
    x0=x6; x0=SH(x0,x6,0xcc); x6=SH(x6,x6,0x79); x9=_mm_sub_ps(x9,x1);
    x1=x14; x3=_mm_xor_ps(x3,x0); x0=x9; x0=SH(x0,x8,0xd4); x1=SH(x1,x14,0x36);
    x4=_mm_xor_ps(x4,x1); x1=x14; x12=_mm_mul_ps(x12,x0);
    x0=x8; x0=SH(x0,x8,0x4c); x6=_mm_mul_ps(x6,x0);
    x0=x8; x0=SH(x0,x9,0x1a); x12=_mm_sub_ps(x12,x6); x0=_mm_mul_ps(x0,x3);
    x3=x14; x3=SH(x3,x14,0xc8); x12=_mm_add_ps(x12,x0);
    x0=x8; x0=SH(x0,x9,0x1e); x3=_mm_mul_ps(x3,x0); x12=_mm_xor_ps(x12,m28c0);
    x1=SH(x1,x14,0x6d); x6=x10; x6=SH(x6,x10,0x26);
    x0=x8; x2=x13; x0=SH(x0,x8,0x58); x2=SH(x2,x13,0xc8); x10=SH(x10,x10,0x79);
    x1=_mm_mul_ps(x1,x0); x12=SH(x12,x12,0x39); x13=SH(x13,x13,0x6d);
    x0=x9; x3=_mm_sub_ps(x3,x1); x0=SH(x0,x8,0xc4);
    x0=_mm_mul_ps(x0,x4); x3=_mm_add_ps(x3,x0);
    x0=x9; x3=_mm_xor_ps(x3,m2850); x0=SH(x0,x11,0xde); x6=_mm_mul_ps(x6,x0);
    x0=x11; x0=SH(x0,x11,0x4c); x3=SH(x3,x3,0x39); x10=_mm_mul_ps(x10,x0);
    x0=x11; x0=SH(x0,x9,0xba); x6=_mm_sub_ps(x6,x10); x0=_mm_mul_ps(x0,x5); x6=_mm_add_ps(x6,x0);
    x0=x11; x6=_mm_xor_ps(x6,m28c0); x0=SH(x0,x9,0xbe); x9=SH(x9,x11,0xce);
    x2=_mm_mul_ps(x2,x0); x9=_mm_mul_ps(x9,x7); x6=SH(x6,x6,0x39);
    x0=x11; x0=SH(x0,x11,0x58); x13=_mm_mul_ps(x13,x0);
    x0=x14; x0=_mm_mul_ps(x0,x12); x2=_mm_sub_ps(x2,x13); x14=SH(x14,x0,0x40); x2=_mm_add_ps(x2,x9);
    x2=_mm_xor_ps(x2,m2850); x14=_mm_add_ps(x14,x0); x2=SH(x2,x2,0x39);
    x0=SH(x0,x14,0x30); x0=_mm_add_ps(x0,x14); x0=SH(x0,x0,0xaa);
    x1=_mm_set1_ps(1.0f); x1=_mm_div_ps(x1,x0);
    x4=x1; x0=x1; x0=_mm_mul_ps(x0,x3); x3=x1; x4=_mm_mul_ps(x4,x12); x1=_mm_mul_ps(x1,x2); x3=_mm_mul_ps(x3,x6);
    x2=x4; x2=SH(x2,x0,0x44); x4=SH(x4,x0,0xee); x0=x3; x0=SH(x0,x1,0x44); x3=SH(x3,x1,0xee);
    x1=x2; x1=SH(x1,x0,0x88); x2=SH(x2,x0,0xdd); x4=SH(x4,x3,0x88);
    _mm_storeu_ps(out,x1); _mm_storeu_ps(out+4,x2); _mm_storeu_ps(out+8,x4);
}
#undef SH
}
void ProduceClothPalette(const float (*bones)[12],const float (*reference)[12],uint32_t count,float (*out)[12]) {
    constexpr float k[4]={0,0,0,1};
    for(uint32_t i=0;i<count;++i) {
        float m[12]; EngineAffineInverseSse(reference[i],m);
        const float* a=bones[i];
        for(int row=0;row<3;++row) for(int c=0;c<4;++c)
            out[i][4*row+c]=((a[4*row+1]*m[4+c]+a[4*row]*m[c])+a[4*row+2]*m[8+c])+a[4*row+3]*k[c];   // SSE order at 1400164CD
    }
}
}
namespace ds3prod {
bool DecodeMtdNumericParam(std::string name,uint32_t type,std::span<const uint32_t> words,MtdParam& output) {
    if(name.empty() || name.find('\0')!=std::string::npos || type>=12 || words.size()!=type%4+1)return false;
    MtdParam result{std::move(name),{}};
    for(const auto word:words){
        double value{};
        if(type<4){if(word>255)return false;value=word;}
        else if(type<8){int32_t integer{};std::memcpy(&integer,&word,4);value=integer;}
        else {float number{};std::memcpy(&number,&word,4);if(!std::isfinite(number))return false;value=number;}
        result.values.push_back(value);
    }
    output=std::move(result);return true;
}
std::vector<std::string> ProduceMtdParams(const std::vector<MtdParam>& mtd,const std::vector<ConstantVariable>& layout,uint8_t* out,uint32_t bytes,uint8_t* written) {
    auto find=[&](const char* name)->const MtdParam* { for(const auto& p:mtd) if(p.name==name) return &p; return nullptr; };
    auto value=[&](const char* name,size_t i,float fallback) { const MtdParam* p=find(name); return p && i<p->values.size()?static_cast<float>(p->values[i]):fallback; };
    std::vector<std::string> unknown;
    for(const auto& v:layout) {
        float x[4]{}; unsigned n=0;
        if(v.name=="FC_DifMapMulCol") { for(unsigned i=0;i<3;++i) x[i]=value("g_DiffuseMapColor",i,1); n=3; }
        else if(v.name=="FC_SpcMapMulCol") { for(unsigned i=0;i<3;++i) x[i]=value("g_SpecularMapColor",i,1); n=3; }
        else if(v.name=="FC_SpcParam") { x[0]=value("g_SpecularPower",0,0); x[1]=value("g_BaseReflectance",0,0); x[2]=value("g_AmbientReflectionRate",0,0); n=3; }
        else if(v.name=="FC_AlphaRef") {   // 0 disables the test (-100); otherwise (ref - 0.0001) / 255, second component / 255
            const float a=value("g_AlphaRef",0,0); x[0]=a==0?-100.0f:(a-0.0001f)/255.0f; x[1]=value("g_AlphaRef",1,0)/255.0f; x[2]=0; n=3; }
        else if(v.name=="FC_BloodAmount") { x[0]=value("g_BloodAmount",0,0); n=1; }
        else if(v.name=="FC_GlowScale") { for(unsigned i=0;i<4;++i) x[i]=value("g_GlowScale",i,1); n=4; }
        // I-0064 (forward hair, P[ChrCustom_Hair]): the pairs of the hair specular parameters, the rim lighting set, the SSAO factor
        else if(v.name=="FC_HairSpecularParam0" && find("g_HairSpecularOffset") && find("g_HairSpecularRange")) { x[0]=value("g_HairSpecularOffset",0,0); x[1]=value("g_HairSpecularOffset",1,0); x[2]=value("g_HairSpecularRange",0,0); x[3]=value("g_HairSpecularRange",1,0); n=4; }
        else if(v.name=="FC_HairSpecularParam1" && find("g_HairSpecularIntensity") && find("g_HairSpecularPower")) { x[0]=value("g_HairSpecularIntensity",0,0); x[1]=value("g_HairSpecularIntensity",1,0); x[2]=value("g_HairSpecularPower",0,0); x[3]=value("g_HairSpecularPower",1,0); n=4; }
        else if(v.name=="FC_HairSpecularParam2" && find("g_HairSpecularShininess") && find("g_HairSpecularDynLightSpecFactor")) { x[0]=value("g_HairSpecularShininess",0,0); x[1]=value("g_HairSpecularDynLightSpecFactor",0,0); n=2; }
        else if(v.name=="FC_RimLightingParam" && find("g_RimLightingIntensity")) { x[0]=value("g_RimLightingIntensity",0,0); x[1]=value("g_RimLightingPower",0,0); x[2]=value("g_RimLightingIblIntensity",0,0); x[3]=value("g_RimLightingIblPower",0,0); n=4; }
        else if(v.name=="FC_SSAOParam" && find("g_SSAOFactor")) { x[0]=value("g_SSAOFactor",0,0); n=1; }
        else if(const MtdParam* same=find(v.name.c_str())) { n=static_cast<unsigned>((std::min<size_t>)(same->values.size(),4)); for(unsigned i=0;i<n;++i) x[i]=static_cast<float>(same->values[i]); }   // same name in MTD and RDEF
        else { unknown.push_back(v.name); continue; }
        for(unsigned i=0;i<n && 4*i<v.bytes && v.offset+4*i+4<=bytes;++i) { std::memcpy(out+v.offset+4*i,&x[i],4); written[(v.offset+4*i)/4]=1; }
    }
    return unknown;
}
}
namespace ds3prod {
void ProduceTexHdrScale(const std::vector<float>& floats,uint32_t offset,uint8_t* out,uint32_t bytes,uint8_t* written) {
    for(size_t i=0;i<floats.size() && i<4 && offset+4*i+4<=bytes;++i) { std::memcpy(out+offset+4*i,&floats[i],4); written[(offset+4*i)/4]=1; }
}
void ProduceClothCancel(const float (*reference)[12],const float (*gate_rows)[12],uint32_t count,float (*out)[12]) {
    constexpr float k[4]={0,0,0,1};
    for(uint32_t i=0;i<count;++i) { const float* q=reference[i]; const float* g=gate_rows[i];
        for(int row=0;row<3;++row) for(int c=0;c<4;++c)   // 17CE0: ((q0*g0 + q1*g1) + q2*g2) + q3*k per row
            out[i][4*row+c]=((q[4*row+1]*g[4+c]+q[4*row]*g[c])+q[4*row+2]*g[8+c])+q[4*row+3]*k[c]; }
}
void ProduceIdentityRows(uint32_t count,float (*out)[12]) {
    for(uint32_t i=0;i<count;++i) for(int q=0;q<12;++q) out[i][q]=(q==0 || q==5 || q==10)?1.0f:0.0f;
}
}
namespace ds3prod {
namespace {
void Put(uint8_t* out,uint32_t bytes,uint8_t* written,uint32_t o,const void* p,uint32_t n) {
    for(uint32_t i=0;i<n && o+i+4<=bytes;i+=4) { std::memcpy(out+o+i,static_cast<const uint8_t*>(p)+i,4); written[(o+i)/4]=1; }
}
}
void ProduceTexScroll(const uint8_t* state,uint8_t* out,uint32_t bytes,uint8_t* written) { Put(out,bytes,written,0,state+0x240,0x40); }
void ProduceModelParamPs(const uint8_t* state,uint8_t* out,uint32_t bytes,uint8_t* written,const uint8_t* default_row) {
    uint32_t flags; std::memcpy(&flags,state+0x200,4);
    const float dither=(flags>>20)&1u?1.0f:0.0f, c30[4]={1,0.5f,2,0.5f}, c50[4]={8192,0.2f,2048,32}, zero=0, one=1;
    Put(out,bytes,written,0x00,state+0x310,16); Put(out,bytes,written,0x10,state+0x37C,4); Put(out,bytes,written,0x20,&dither,4);
    Put(out,bytes,written,0x30,c30,16); Put(out,bytes,written,0x44,&zero,4); Put(out,bytes,written,0x48,&one,4); Put(out,bytes,written,0x50,c50,16);
    Put(out,bytes,written,0x60,state+0x280,16);
    // I-0064: the parallax-corrected cubemap boxes (1A86C0 when state +0x426 / +0x427): box A from the scale +0x440 and the rows
    // +0x450..+0x48F, box B from +0x490 and +0x4A0..+0x4DF; each M = 1CE40(the scale rows with the .data row 0x144929B20 last, the
    // rows), stored transposed with its 4BCA0 inverse (FC_ParallaxCorrectedCubemapObbA +0xA0, InvObbA +0xE0, ObbB +0x120, InvObbB
    // +0x160); the origins +0x428 / +0x434 at +0x80 / +0x90 (w 0); the enables +0x1A0 / +0x1A4 (1.0 when the box is set).
    if(!default_row) return;
    const bool a=state[0x426]!=0, b=state[0x427]!=0; const float on=1.0f, off=0.0f;
    Put(out,bytes,written,0x1A0,a?&on:&off,4); Put(out,bytes,written,0x1A4,b?&on:&off,4); if(!a && !b) return;
    auto box=[&](uint32_t scale,uint32_t rows,uint32_t at_m,uint32_t at_inv) {
        float s[16]{},r[16],m[16],inv[16],tm[16],ti[16]; for(int i=0;i<3;++i) s[5*i]=F(state,scale+4*i); std::memcpy(s+12,default_row,16); std::memcpy(r,state+rows,64);
        EngineMultiply(s,r,m); InverseMatrix(m,inv);
        for(int i=0;i<4;++i) for(int j=0;j<4;++j) { tm[4*i+j]=m[4*j+i]; ti[4*i+j]=inv[4*j+i]; }
        Put(out,bytes,written,at_m,tm,64); Put(out,bytes,written,at_inv,ti,64); };
    box(0x440,0x450,0xA0,0xE0); box(0x490,0x4A0,0x120,0x160);
    const float zero4=0.0f; Put(out,bytes,written,0x80,state+0x428,12); Put(out,bytes,written,0x8C,&zero4,4); Put(out,bytes,written,0x90,state+0x434,12); Put(out,bytes,written,0x9C,&zero4,4);
}
void ProduceFrameTime(float time,uint8_t* out,uint32_t bytes,uint8_t* written) {
    Put(out,bytes,written,0xB0,&time,4);
}
void ProduceDitherAlpha(uint8_t* out,uint32_t bytes,uint8_t* written) {
    const uint32_t one=1;Put(out,bytes,written,0,&one,4);
}
void ProduceMatDynScalars(uint32_t byte_offset,const uint32_t* words,uint32_t count,
    uint8_t* out,uint32_t bytes,uint8_t* written) {
    if(byte_offset>bytes || byte_offset%4 || !words)return;
    const auto extent=(std::min)(count,(bytes-byte_offset)/4);
    Put(out,bytes,written,byte_offset,words,extent*4);
}
void ProduceMatDynWet(const MatDynWet& input,uint8_t* out,uint32_t bytes,uint8_t* written) {
    const auto pw=[](float x){return std::pow(x,2.2f);};
    const auto& d=input.diffuse;const auto& r=input.reflectance;
    const float w0[4]={pw(d[0]),pw(d[1]),pw(d[2]),input.b*d[3]},
        w1[4]={input.a,input.c,0,0},
        w2[4]={pw(r[0]),pw(r[1]),pw(r[2]),input.b*r[3]},
        w3[4]={0,0,input.b*input.scale,0};
    Put(out,bytes,written,0x230,w0,16);Put(out,bytes,written,0x240,w1,16);
    Put(out,bytes,written,0x250,w2,16);Put(out,bytes,written,0x260,w3,16);
    Put(out,bytes,written,0x260,&input.type,4);
}
// 1368D0: the two Dominant Light records' eleven floats blended (a at t 0, b at t 1, else (b - a) * t + a).
static void BlendDominant(const float row_a[11],const float row_b[11],float t,float r[11]) {
    constexpr float eps=1.1920929e-07f;   // 0x143D7ECE8
    for(int i=0;i<11;++i) r[i]=std::fabs(t-0.0f)<=eps?row_a[i]:std::fabs(t-1.0f)<=eps?row_b[i]:(row_b[i]-row_a[i])*t+row_a[i];
}
void ProducePerFrame(float time,const float row_a[11],const float row_b[11],float t,uint8_t* out,uint32_t bytes,uint8_t* written) {
    float r[11];BlendDominant(row_a,row_b,t,r);
    const float one=1;
    ProduceFrameTime(time,out,bytes,written); Put(out,bytes,written,0xC0,r,28); Put(out,bytes,written,0xDC,&one,4);
}
void ProducePassDominantScale(const float row_a[11],const float row_b[11],float t,float out[4]) {
    float r[11];BlendDominant(row_a,row_b,t,r);
    out[0]=r[8]*r[7];out[1]=r[9]*r[7];out[2]=r[10]*r[7];out[3]=1.0f;   // 140204C30: MULSS by [RSP+0x4C] (f7), the w 1.0 (0x143D7EE10)
}
float ProduceSnow(const float a[16],const float b[16],float t,uint8_t* out,uint32_t bytes,uint8_t* written) {
    constexpr float eps=1.1920929e-07f,half_pi=1.5707964f,pi=3.1415927f;   // 0x143D7ECE8, 0x143D7EE40 / F0D4, 0x143D7EEC0 / F0EC
    // 1316D0: an angle's blend over the range [lo, hi] (its width added below lo, the short way round).
    const auto angle=[](float x,float y,float w,float lo,float hi){const float width=std::fabs(lo-hi);
        if(x<lo)x+=width;if(y<lo)y+=width;
        if(width<=std::fabs(x-y)){if(y<=x)return ((width+y)-x)*w+x;y-=width;}
        return (y-x)*w+x;};
    float r[16],e,z;
    if(std::fabs(t)<=eps || std::fabs(t-1.0f)<=eps){std::memcpy(r,std::fabs(t)<=eps?a:b,sizeof r);e=a[0];z=a[1];}
    else{for(int i=0;i<16;++i)r[i]=(b[i]-a[i])*t+a[i];e=angle(a[0],b[0],t,-half_pi,half_pi);z=angle(a[1],b[1],t,-pi,pi);}
    const float dir[4]={std::cos(e)*std::sin(z),-std::sin(e),std::cos(e)*std::cos(z),0},
        tiling[4]={r[2],r[3],(1-r[2])*0.5f,(1-r[3])*0.5f},
        heights[4]={r[14],r[7],1/(r[8]-r[7]),r[9]},
        sparkle[4]={r[5],r[6],(1-r[5])*0.5f,(1-r[6])*0.5f};
    Put(out,bytes,written,0x60,dir,16);Put(out,bytes,written,0x70,tiling,16);Put(out,bytes,written,0x80,r+10,16);
    Put(out,bytes,written,0x90,heights,16);Put(out,bytes,written,0xA0,sparkle,16);
    return r[4];
}
void ProduceSnowRate(float a,float b,float t,float scene,uint8_t* out,uint32_t bytes,uint8_t* written) {
    const float rate=((b-a)*t+a)*scene;Put(out,bytes,written,0x40,&rate,4);
}
}
namespace ds3prod {
void ProduceViewMatrices(const float pose[16],const float proj[16],uint8_t out[128]) {
    std::memcpy(out,pose,64);
    // the rotation's inverse: the pose with its translation row replaced by (0, 0, 0, 1), inverted (XMMatrixInverse); exact on the
    // main view and the cascades of five zones, where the engine inverse (4BCA0) of the full pose, its stored view (+0x10) or the
    // transpose differ by one ulp on some words
    XMMATRIX rotation(pose); rotation.r[3]=XMVectorSet(0,0,0,1); rotation=XMMatrixInverse(nullptr,rotation);
    PutT(out,64,XMMatrixMultiply(rotation,XMMATRIX(proj)),4);
}
}
