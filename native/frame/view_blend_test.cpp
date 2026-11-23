// I-0114 (camera_step): the display's view blend between two ticks: its ends, its orthonormal rows (both handednesses), the
// position and field of view halfway, the other floats the current tick's.
#include "../world/view_blend.h"
#include <cmath>
#include <cstdio>
#include <stdexcept>

using namespace ds3native::view_blend;
namespace {
void Require(bool v,const char* m){if(!v)throw std::runtime_error(m);}
bool Near(float a,float b,float e=1e-5f){return std::abs(a-b)<=e;}
// A view turned by yaw about +Y: right, up, forward, position, lens; `mirrored` flips the right row (the other handedness).
void View(float yaw,float x,bool mirrored,float fov,float* v){
    const float c=std::cos(yaw),s=std::sin(yaw),side=mirrored?-1.f:1.f;
    const float rows[20]{side*c,0,-side*s,0, 0,1,0,0, s,0,c,0, x,1.5f,-4,1, fov,0.1f,1000,1.7778f};
    for(int i=0;i<20;++i)v[i]=rows[i];
}
float Dot(const float* a,const float* b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
}
int main(){try{
    for(const bool mirrored:{false,true}){
        float a[20],b[20],out[20];View(0.1f,0,mirrored,0.8f,a);View(0.5f,2,mirrored,1.0f,b);b[17]=0.2f;
        Blend(a,b,0,out);Require(Near(out[8],a[8]) && Near(out[10],a[10]) && Near(out[12],0) && Near(out[16],0.8f),"alpha 0 is the previous tick");
        Require(Near(out[17],0.2f),"the near plane is the current tick's");
        Blend(a,b,1,out);for(int i=0;i<17;++i)Require(Near(out[i],b[i]),"alpha 1 is the current tick");
        Blend(a,b,0.5f,out);
        Require(Near(Dot(out,out),1) && Near(Dot(out+4,out+4),1) && Near(Dot(out+8,out+8),1),"unit rows");
        Require(Near(Dot(out,out+4),0) && Near(Dot(out,out+8),0) && Near(Dot(out+4,out+8),0),"orthogonal rows");
        float expected[20];View(0.3f,1,mirrored,0.9f,expected);
        for(int i=0;i<16;++i)if(i%4!=3)Require(Near(out[i],expected[i],1e-4f),"halfway: the rows turned by half and the position");
        Require(Near(out[16],0.9f),"halfway: the field of view");
        Require(Dot(out,b)>0,"right on the current tick's side");}
    std::printf("PASS view blend: ends, orthonormal rows in both handednesses, halfway rows, position and field of view, the lens's rest\n");
    return 0;
}catch(const std::exception& e){std::printf("FAIL %s\n",e.what());return 1;}}
