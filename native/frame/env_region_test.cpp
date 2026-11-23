// I-0113 tranche 14: the region choice (world/env_region.h) on boxes whose answers follow from 66C80, 66DB0 and 5C0E0's search.
#include "../world/env_region.h"
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace {
using namespace ds3native::env_region;
void Expect(bool ok,const char* what){if(!ok)throw std::runtime_error(what);}
bool Near(float a,float b){return std::fabs(a-b)<1e-5f;}
// A box at `centre` with half extents e, its axes the world's turned by 90 degrees about y when `turned`.
Region Box(const float* centre,const float* e,float fade,uint32_t priority,uint32_t id,bool turned=false){
    Region r;r.key[0]=137000000;r.key[1]=id;r.fade=fade;r.priority=priority;r.masks[0]=1;r.masks[1]=1;r.handle=true;
    for(int k=0;k<3;++k)r.box[k]=e[k];
    const float axes[2][9]{{1,0,0,0,1,0,0,0,1},{0,0,-1,0,1,0,1,0,0}};
    for(int k=0;k<3;++k)for(int i=0;i<3;++i)r.box[4+4*k+i]=axes[turned][3*k+i];
    for(int i=0;i<3;++i)r.box[16+i]=centre[i];
    return r;}
}

int main(){
    try{
        const float origin[3]{0,0,0},e[3]{2,3,4};
        const auto r=Box(origin,e,1.f,1,350);
        {const float p[3]{0,0,0},q[3]{1.5f,0,0},s[3]{2.5f,0,0},t[3]{0,2.75f,0};
            Expect(Inside(r.box,p) && Inside(r.box,q) && !Inside(r.box,s),"inside");
            Expect(Near(Blend(r,p),0.f),"deep inside: the nearest face beyond the fade");
            Expect(Near(Blend(r,q),0.5f),"half the fade from the x face");
            Expect(Near(Blend(r,s),1.f),"outside the box: 1");
            Expect(Near(Blend(r,t),0.75f),"a quarter of the fade from the y face");}
        {auto flat=r;flat.fade=0.f;const float q[3]{1.5f,0,0};Expect(Near(Blend(flat,q),0.f),"no fade: 0");}
        {const auto turned=Box(origin,e,1.f,1,350,true);const float q[3]{0,0,1.5f},s[3]{0,0,2.5f},w[3]{3.5f,0,0};   // its x axis is the world's z
            Expect(Inside(turned.box,q) && !Inside(turned.box,s) && Inside(turned.box,w),"turned box");
            Expect(Near(Blend(turned,q),0.5f),"turned box: half the fade from its face");}
        {auto odd=r;odd.fade=std::nanf("");const float q[3]{1.5f,0,0};Expect(std::isnan(Blend(odd,q)),"a NaN fade: the face ratio kept unclamped");}
        {const float big[3]{10,10,10},small[3]{1,1,1},c[3]{0,0,0},none[3]{0,0,0},away[3]{1000,1000,1000};
            const std::vector<Region> listed{Box(c,big,2.f,1,350),Box(c,small,0.5f,2,351)};
            // The default record: no extents (it holds only its own centre), here away from every point tried, key {0, -1}.
            auto fallback=Box(away,none,0.f,0,0);fallback.key[0]=0;fallback.key[1]=static_cast<uint32_t>(-1);
            const uint32_t view[2]{1,1};
            const float deep[3]{0,0,0},edge[3]{0.9f,0,0},out[3]{5,0,0},far[3]{20,0,0};
            auto x=Choose(listed,fallback,deep,view);Expect(x.first==&listed[1] && x.second==&listed[1] && Near(x.blend,0.f),"deep in the inner region");
            x=Choose(listed,fallback,edge,view);Expect(x.first==&listed[1] && x.second==&listed[0] && Near(x.blend,0.8f),"near the inner face: the outer second");
            x=Choose(listed,fallback,out,view);Expect(x.first==&listed[0] && x.second==&listed[0] && Near(x.blend,0.f),"in the outer region only");
            x=Choose(listed,fallback,far,view);Expect(x.first==&fallback && x.second==&fallback && Near(x.blend,1.f),"no region: the default's, blend 1");
            const uint32_t other[2]{2,1};
            x=Choose(listed,fallback,deep,other);Expect(x.first==&fallback,"the view's masks refuse every region");
            const uint32_t second_word[2]{1,2};
            x=Choose(listed,fallback,deep,second_word);Expect(x.first==&fallback,"the second mask word refuses every region");
            auto unhandled=listed;unhandled[1].handle=false;
            x=Choose(unhandled,fallback,edge,view);Expect(x.first==&unhandled[0],"a region without a handle is skipped");
            const float outer_edge[3]{9.5f,0,0};
            x=Choose(listed,fallback,outer_edge,view);Expect(x.first==&listed[0] && x.second==&fallback && Near(x.blend,0.75f),"near the outer face: no lower priority, the default second");
            auto tie=listed;tie[0].priority=2;
            x=Choose(tie,fallback,edge,view);Expect(x.first==&tie[1] && x.second==&fallback,"an equal priority is not lower: the default second");}
        std::puts("PASS env region choice: the box test, the face blend (plain and turned boxes, no fade, outside), a NaN fade, the priority search (an equal priority, none lower), both view mask words, a region without a handle");
        return 0;
    }catch(const std::exception& e){std::printf("FAIL env region: %s\n",e.what());return 1;}
}
