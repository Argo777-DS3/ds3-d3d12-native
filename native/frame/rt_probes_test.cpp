// I-0129 R9a: the traced probes' CPU side (rt_probes.h): the rows from a frame's sums (a uniform sky, an upper hemisphere, a coloured
// direction, too few rays, a value not finite), the sun's share, the cell keys and their boxes, the states' first answer, blend,
// frame bits and drop.
#include "../renderer/rt_probes.h"
#include <cmath>
#include <cstdio>
#include <random>
#include <stdexcept>

using namespace ds3native;
namespace {
void Require(bool v,const char* m){if(!v)throw std::runtime_error(m);}
bool Near(float a,float b,float e){return std::abs(a-b)<=e;}
// The GPU's sums for N uniform directions and a radiance function (rgb) of the direction.
template<class F> RtProbeResult Sums(uint32_t n,F radiance,uint32_t seed=7){
    std::mt19937 g(seed);std::uniform_real_distribution<float> u(0.f,1.f);RtProbeResult r;
    for(uint32_t i=0;i<n;++i){const float z=1.f-2.f*u(g),phi=6.2831853f*u(g),s=std::sqrt(std::max(0.f,1.f-z*z));
        const float w[3]{s*std::cos(phi),z,s*std::sin(phi)};   // y up
        const auto l=radiance(w);for(int c=0;c<3;++c)r.sum[c]+=l[c];r.sum[3]+=1.f;
        for(int k=0;k<3;++k){r.red[k]+=l[0]*w[k];r.green[k]+=l[1]*w[k];r.blue[k]+=l[2]*w[k];}}
    return r;}
float Eval(const std::array<float,12>& rows,int c,const float n[3]){return std::max(0.f,rows[4*c]*n[0]+rows[4*c+1]*n[1]+rows[4*c+2]*n[2]+rows[4*c+3]);}
}
int main(){try{
    std::array<float,12> rows{};
    // A uniform sky of 1: the rows (0, 0, 0, 1) within the sampling's noise.
    Require(RtProbeRowsOf(Sums(200000,[](const float*){return std::array<float,3>{1,1,1};}),rows),"uniform: rows");
    for(int c=0;c<3;++c)Require(Near(rows[4*c+3],1.f,1e-6f) && Near(rows[4*c],0,0.01f) && Near(rows[4*c+1],0,0.01f) && Near(rows[4*c+2],0,0.01f),"uniform: (0, 0, 0, 1)");
    // An upper hemisphere of 1: 1 up, 0 down, 0.5 sideways (the exact cosine-weighted means).
    Require(RtProbeRowsOf(Sums(400000,[](const float* w){return w[1]>0?std::array<float,3>{1,1,1}:std::array<float,3>{0,0,0};}),rows),"hemisphere: rows");
    const float up[3]{0,1,0},down[3]{0,-1,0},side[3]{1,0,0};
    Require(Near(Eval(rows,0,up),1.f,0.01f) && Near(Eval(rows,1,down),0.f,0.01f) && Near(Eval(rows,2,side),0.5f,0.01f),"hemisphere: 1 up, 0 down, 0.5 sideways");
    // A red light from +x only: the red rows lean to +x, green and blue zero.
    Require(RtProbeRowsOf(Sums(400000,[](const float* w){return w[0]>0?std::array<float,3>{2,0,0}:std::array<float,3>{0,0,0};}),rows),"red: rows");
    Require(Near(Eval(rows,0,side),2.f,0.02f) && Near(rows[4],0,1e-6f) && Near(rows[11],0,1e-6f),"red: +x lit, the others zero");
    // Too few valid rays, a value not finite: no rows.
    {RtProbeResult r=Sums(kRtProbeRaysLeast-1,[](const float*){return std::array<float,3>{1,1,1};});Require(!RtProbeRowsOf(r,rows),"too few rays");
     r=Sums(64,[](const float*){return std::array<float,3>{1,1,1};});r.red[0]=NAN;Require(!RtProbeRowsOf(r,rows),"not finite");}
    // The sun's share.
    {RtProbeResult r;r.red[3]=16;r.green[3]=64;Require(RtProbeSunOf(r) && Near(*RtProbeSunOf(r),0.25f,1e-6f),"sun: 16 of 64");
     r.green[3]=0;Require(!RtProbeSunOf(r),"sun: no rays");}
    // The cell keys: a cell's points share a key, its box holds them, neighbours differ, negative coordinates, the object tag.
    {const float a[3]{10.2f,-3.7f,0.f},b[3]{10.9f,-3.1f,0.99f},c[3]{11.0f,-3.7f,0.f};
     Require(RtProbeCellKey(a)==RtProbeCellKey(b) && RtProbeCellKey(a)!=RtProbeCellKey(c) && (RtProbeCellKey(a)>>63),"cells: shared, apart, tagged");
     const auto box=RtProbeCellRequest(RtProbeCellKey(a),3);
     Require(box.lo[0]==10 && box.hi[0]==11 && box.lo[1]==-4 && box.hi[1]==-3 && box.lo[2]==0 && box.hi[2]==1 && box.set==3,"cells: the box");}
    // The states: the first answer whole, then the blend; the frame's bits decide what Find gives; the drop after kRtProbeKeep.
    {auto states=std::make_shared<RtProbeStates>();
     const auto one=Sums(64,[](const float*){return std::array<float,3>{1,1,1};}),three=Sums(64,[](const float*){return std::array<float,3>{3,3,3};});
     RtProbeResult sun=one;sun.red[3]=64;sun.green[3]=64;
     const uint64_t keys[2]{5,6};const RtProbeResult first[2]{sun,one};
     states->Deliver(keys,first,kRtProbeAmbient|kRtProbeSun,1000,states->Epoch());
     auto s=states->Find(5,1000);Require(s && s->ambient && Near(s->rows[3],1.f,1e-5f) && s->sun_valid && Near(s->sun,1.f,1e-6f),"states: the first answer whole");
     Require(states->Find(6,1000) && !states->Find(6,1000)->sun_valid,"states: no sun rays, no sun");
     const RtProbeResult second[1]{three};states->Deliver({keys,1},second,kRtProbeAmbient|kRtProbeSun,1000,states->Epoch());
     s=states->Find(5,1000);Require(s && Near(s->rows[3],1.f+kRtProbeBlend*2.f,1e-5f) && !s->sun_valid,"states: blended; a sun frame without its rays clears the share (the final review's H1)");
     states->Deliver({keys,1},second,kRtProbeSun,1000,states->Epoch());
     {const RtProbeResult lit[1]{sun};states->Deliver({keys,1},lit,kRtProbeSun,1000,states->Epoch());}
     s=states->Find(5,1000);Require(s && !s->ambient && s->sun_valid,"states: a frame without the GI gives no ambient");
     states->Deliver({keys,1},second,kRtProbeAmbient|kRtProbeSun,1000,states->Epoch());
     Require(states->Find(5,1000) && states->Find(5,1000)->ambient && Near(states->Find(5,1000)->rows[3],1.f+kRtProbeBlend*2.f+kRtProbeBlend*(3.f-1.f-kRtProbeBlend*2.f),1e-4f),"states: the GI again, the rows kept");
     states->Deliver({},{},0,1000,states->Epoch());Require(!states->Find(5,1000),"states: a frame tracing nothing gives none");
     {auto edge=std::make_shared<RtProbeStates>();const uint64_t k6[1]{6};const RtProbeResult r6[1]{one};const uint64_t k7[1]{7};
      edge->Deliver(k6,r6,kRtProbeAmbient,1000,edge->Epoch());   // key 6 delivered, then key 7 alone
      for(uint64_t i=0;i<kRtProbeRecent-1;++i)edge->Deliver(k7,r6,kRtProbeAmbient,1000,edge->Epoch());
      Require(edge->Find(6,1000).has_value(),"recent: a key delivered kRtProbeRecent - 1 deliveries ago answers");
      edge->Deliver(k7,r6,kRtProbeAmbient,1000,edge->Epoch());Require(edge->Find(6,1000).has_value(),"recent: kRtProbeRecent ago answers");
      edge->Deliver(k7,r6,kRtProbeAmbient,1000,edge->Epoch());Require(!edge->Find(6,1000),"recent: kRtProbeRecent + 1 ago does not");}
     for(uint64_t i=0;i<kRtProbeRecent+1;++i)states->Deliver({keys,1},second,kRtProbeAmbient,1000,states->Epoch());
     Require(states->Find(5,1000) && !states->Find(6,1000) && states->Size()==2,"states: a key not delivered lately answers nothing (kept a while)");
     for(uint64_t i=0;i<kRtProbeKeep+32;++i)states->Deliver({keys,1},second,kRtProbeAmbient,1000,states->Epoch());
     Require(states->Find(5,1000) && !states->Find(6,1000) && states->Size()==1,"states: the unseen dropped");
     Require(states->Find(5,1000+kRtProbeStaleMs) && !states->Find(5,1001+kRtProbeStaleMs),"states: stale past kRtProbeStaleMs");
     const uint64_t before=states->Epoch();states->Clear();Require(!states->Find(5,1000) && states->Size()==0 && states->Epoch()==before+1,"states: cleared");
     states->Deliver({keys,1},second,kRtProbeAmbient,1000,before);Require(!states->Find(5,1000) && states->Size()==0,"states: an answer from before the clear dropped");}
    // The hysteresis (the step review's M2): small moves of the average leave what a state gives; a larger one moves it.
    {auto states=std::make_shared<RtProbeStates>();const uint64_t key[1]{9};
     const auto at=[](float l){return Sums(64,[l](const float*){return std::array<float,3>{l,l,l};});};
     const RtProbeResult a[1]{at(1.f)},b[1]{at(1.1f)},c[1]{at(2.f)};
     states->Deliver(key,a,kRtProbeAmbient,1000,states->Epoch());
     for(int i=0;i<2;++i)states->Deliver(key,b,kRtProbeAmbient,1000,states->Epoch());   // the average about 1.01: under 3 %
     auto s=states->Find(9,1000);Require(s && s->rows[3]==1.f && s->mean[3]>1.f,"hysteresis: a small move kept");
     states->Deliver(key,c,kRtProbeAmbient,1000,states->Epoch());s=states->Find(9,1000);Require(s && s->rows[3]>1.05f && s->rows[3]==s->mean[3],"hysteresis: a large move given");}
    // The quantization: within 0.1 %, idempotent, zero and a value not finite.
    for(const float x:{1.f,0.3333f,-2.7183f,1e-4f,123.456f}){const float q=RtProbeQuantize(x);Require(std::abs(q-x)<=std::abs(x)*1e-3f && RtProbeQuantize(q)==q,"quantize: near and idempotent");}
    Require(RtProbeQuantize(0.f)==0.f && RtProbeQuantize(NAN)==0.f && RtProbeQuantize(1.0004f)==RtProbeQuantize(1.0002f),"quantize: zero, not finite, a small move");
    std::printf("PASS rt probes: the rows (a uniform sky, an upper hemisphere, a coloured side, too few rays, not finite), the sun's share, the cells, the states (first, blend, bits, the sun cleared, recent, drop, stale, clear and its epoch, hysteresis), the quantization\n");
    return 0;
}catch(const std::exception& e){std::printf("FAIL %s\n",e.what());return 1;}}
