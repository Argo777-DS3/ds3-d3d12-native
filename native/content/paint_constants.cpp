#include "paint_constants.h"
#include <cmath>
#include <cstring>

namespace ds3content {
namespace {
float At(const uint8_t* bytes,unsigned offset){float value;std::memcpy(&value,bytes+offset,4);return value;}
}
bool ComputePaintProjector(const uint8_t* d,PaintProjector& out) {
    out={};
    for(unsigned offset=0;offset<0x34;offset+=4)if(!std::isfinite(At(d,offset)))return false;
    const float s=At(d,0x24),z=At(d,0x2C),range=At(d,0x30);
    if(s==0.0f || z==0.0f)return false;
    // 5FCD0: the frame W of the kind-1 object (+0x18 = d): rows D (+0x30), m x D with m = 0 - P1 (its shuffle order) (+0x24, the constant K at
    // 1439F2490 is zero; the mask 143D1C0E0 clears w), m, and (P0 (+0x18), 1).
    const float D[3]{At(d,0x18),At(d,0x1C),At(d,0x20)},m[3]{0.0f-At(d,0x0C),0.0f-At(d,0x10),0.0f-At(d,0x14)};
    auto& W=out.frame;
    W={D[0],D[1],D[2],0.0f,
       D[2]*m[1]-D[1]*m[2],D[0]*m[2]-D[2]*m[0],D[1]*m[0]-D[0]*m[1],0.0f,
       m[0],m[1],m[2],0.0f,
       At(d,0x00),At(d,0x04),At(d,0x08),1.0f};
    // 1AC640: the transposed rotation, the translation dotted with each row (its order) and negated ([143D80010] = (-1,-1,-1,1)).
    auto& V=out.view;
    const float x=W[13]*W[1]+W[15]*W[3]+W[12]*W[0]+W[14]*W[2];
    const float y=W[13]*W[5]+W[15]*W[7]+W[12]*W[4]+W[14]*W[6];
    const float zt=W[13]*W[9]+W[15]*W[11]+W[12]*W[8]+W[14]*W[10];
    V={W[0],W[4],W[8],0.0f, W[1],W[5],W[9],0.0f, W[2],W[6],W[10],0.0f, -1.0f*x,-1.0f*y,-1.0f*zt,1.0f*1.0f};
    // 1AA540 (projection 0, the box): range above [143D7EE10] = 1 diverges from a = s z / 2 / (s range - s / 2) ([143D7EDB4] =
    // 0.5), else orthographic ([143D7FCA0]: depth bias 0.1, [143D7EE7C] = 2, [143D7ED70] = 0.1, [143D7F0B0] = -0.1).
    auto& B=out.bias;auto& P=out.projection;
    B={1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    if(1.0f<range){out.diverging=true;
        const float a=(s*z*0.5f)/(s*range-s*0.5f);
        const float t=(a+a)/s;const float q0=z+a;const float q=q0/(q0-a);const float r=-(q*a);
        B[14]=a;
        P={t,0,0,0, 0,t,0,0, 0,0,q,1, 0,0,r,0};}
    else{B[14]=0.1f;
        const float p=2.0f/s;const float f=1.0f/((z+0.1f)-0.1f);const float g=f*-0.1f;
        P={p,0,0,0, 0,p,0,0, 0,0,f,0, 0,0,g,1};}
    // 1AB370: VB = V B, then F = VB P, in its summation orders (rows 0, 1, 3 alike, row 2 its own), stored transposed.
    float VB[16];
    for(int r=0;r<4;++r)for(int c=0;c<4;++c)VB[4*r+c]=V[4*r+3]*B[12+c]+V[4*r+1]*B[4+c]+V[4*r+2]*B[8+c]+V[4*r+0]*B[c];
    for(int r=0;r<4;++r)for(int c=0;c<4;++c){const float* v=VB+4*r;
        out.wvp[4*c+r]=r==2?v[2]*P[8+c]+v[0]*P[c]+v[3]*P[12+c]+v[1]*P[4+c]:v[3]*P[12+c]+v[1]*P[4+c]+v[2]*P[8+c]+v[0]*P[c];}
    for(const float value:out.wvp)if(!std::isfinite(value))return false;
    return true;
}
}
