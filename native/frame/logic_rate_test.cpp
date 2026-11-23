// I-0114 (UNLOCK-READY §5, unit controls): the frame-rate laws: k = 1 gives the 60 frames/s expression bit for bit; two calls at
// k/2 equal one at k (exactly for a step, within rounding for the powers); a timer that expires at call n at 60 expires at the same
// game time at 120; the locomotion factor of 1405C72B0 at 60 and its equivalence at 120 and 30.
#include "../world/logic_rate.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <vector>

using namespace ds3native::logic_rate;
namespace {
void Require(bool v,const char* m){if(!v)throw std::runtime_error(m);}
bool Same(float a,float b){return !std::memcmp(&a,&b,4);}
bool Near(float a,float b,float tolerance){return std::fabs(a-b)<=tolerance*(std::fabs(b)>1.f?std::fabs(b):1.f);}
}
int main(){try{
    // I-0113 gap 27: the corrections' k from dt, 1 at and under 60 frames/s (unless below_60), dt·60 above; continuous at 60.
    {Require(FrameK(1.f/60.f)==1.f && FrameK(1.f/59.f)==1.f && FrameK(1.f/20.f)==1.f && FrameK(0.05f)==1.f,"gap 27 k at and under 60");
        Require(Near(FrameK(1.f/120.f),0.5f,1e-6f) && Near(FrameK(1.f/240.f),0.25f,1e-6f) && Near(FrameK(1.f/61.f),60.f/61.f,1e-6f),"gap 27 k above 60");
        Require(Near(FrameK(1.f/20.f,true),3.f,1e-5f) && FrameK(1.f/60.f,true)==1.f,"gap 27 k under 60 kept by the A/B");
        Require(FrameK(0.f)==1.f && FrameK(-1.f)==1.f && FrameK(std::nanf(""))==1.f && FrameK(INFINITY)==1.f,"gap 27 k of a bad dt");
        // A rate wavering about 60 (59 to 61 frames/s): k within 2 % of 1 whatever the side, no jump.
        for(float hz=59.f;hz<=61.f;hz+=0.25f){const float k=FrameK(1.f/hz);Require(k<=1.f && k>=60.f/61.f-1e-6f,"gap 27 k about 60");}}
    // k = 1: the original's bits.
    for(const float x:{0.f,0.3f,0.75f,1.f,-2.5f,123.456f}){
        Require(Same(Step(x,1.f/30,1.f),x+1.f/30) && Same(Ratio(x,0.8f,1.f),x*0.8f) && Same(Gain(0.1f,1.f),0.1f) && Same(Distance(x,1.f),x),"I-0114 k = 1 bit for bit");}
    // Two calls at k/2 equal one at k.
    {const float k=2.f;float a=0.25f,b=0.25f;a=Step(Step(a,1.f/30,k/2),1.f/30,k/2);b=Step(b,1.f/30,k);Require(Near(a,b,1e-6f),"I-0114 step halves");
        a=b=0.9f;a=Ratio(Ratio(a,0.8f,k/2),0.8f,k/2);b=Ratio(b,0.8f,k);Require(Near(a,b,1e-5f),"I-0114 ratio halves");
        // A lerp with the gain law: two half-steps equal one whole step toward a fixed target.
        float y=1.f;a=b=0.f;const float g=0.2f;a+=(y-a)*Gain(g,k/2);a+=(y-a)*Gain(g,k/2);b+=(y-b)*Gain(g,k);Require(Near(a,b,1e-5f),"I-0114 gain halves");}
    // The same on the power path on both sides (step review L7): k 0.5 against two calls at 0.25, and a non-integer k.
    for(const float k:{0.5f,0.7f,1.3f}){float a=0.9f,b=0.9f;a=Ratio(Ratio(a,1.2f,k/2),1.2f,k/2);b=Ratio(b,1.2f,k);Require(Near(a,b,1e-5f),"I-0114 ratio halves, power path");
        float y=2.f;a=b=0.f;a+=(y-a)*Gain(0.3f,k/2);a+=(y-a)*Gain(0.3f,k/2);b+=(y-b)*Gain(0.3f,k);Require(Near(a,b,1e-5f),"I-0114 gain halves, power path");}
    // A timer that expires at call n at 60 expires at the same game time at 120: 1 s of -1/60 steps.
    {float t60=1.f,t120=1.f;int n60=0,n120=0;while(t60>0){t60=Step(t60,-1.f/60,1.f);++n60;}while(t120>0){t120=Step(t120,-1.f/60,0.5f);++n120;}
        Require(n120==2*n60 || n120==2*n60-1 || n120==2*n60+1,"I-0114 timer expires at the same game time");}
    // The locomotion factor: at 60 the original's rule; at 120 and 30 (k 0.5 and 2) the same factor after the same game time when
    // the character moves at the same speed (0.025 m a 60 frames/s call: under the threshold 1/30, so it decays).
    {const LocomotionConstants c{1.f,30.f,0.5f,0.8f,1.2f};
        Require(Same(LocomotionFactor(0.9f,true,0.02f,1.f,c),0.9f*0.8f) && Same(LocomotionFactor(0.45f,true,0.02f,1.f,c),0.5f) &&
            Same(LocomotionFactor(0.9f,true,0.05f,1.f,c),0.9f*1.2f) && Same(LocomotionFactor(1.f,true,0.05f,1.f,c),1.f) &&
            Same(LocomotionFactor(0.9f,false,0.f,1.f,c),0.9f*1.2f),"I-0114 locomotion at 60");
        // A NaN factor is reset as the original's unordered compares do (0.5 decaying, 1.0 growing), at any k.
        const float nan=std::nanf("");
        Require(Same(LocomotionFactor(nan,true,0.01f,0.5f,c),0.5f) && Same(LocomotionFactor(nan,true,0.05f,0.5f,c),1.f) && Same(LocomotionFactor(nan,false,0.f,1.f,c),1.f),"I-0114 locomotion NaN");
        float x60=1.f,x120=1.f,x30=1.f;
        for(int i=0;i<2;++i)x60=LocomotionFactor(x60,true,0.025f,1.f,c);
        for(int i=0;i<4;++i)x120=LocomotionFactor(x120,true,0.0125f,0.5f,c);
        x30=LocomotionFactor(x30,true,0.05f,2.f,c);   // above the floor all along (its reset to 0.5 is a per-call event, kept)
        Require(Near(x120,x60,1e-5f) && Near(x30,x60,1e-5f),"I-0114 locomotion at 120 and 30 as at 60");
        // A run at 2.4 m/s (0.04 m a 60 frames/s call, 30-fold 1.2: it grows) moves 0.02 m a call at 120: read uncorrected (30-fold
        // 0.6) it would decay as a stop (LTF's defect); corrected it grows as at 60.
        Require(LocomotionFactor(0.9f,true,0.04f,1.f,c)>0.9f && LocomotionFactor(0.9f,true,0.02f,0.5f,c)>0.9f && LocomotionFactor(0.9f,true,0.02f,1.f,c)<0.9f,
            "I-0114 the threshold read per 60 frames/s call");}
    // The follow camera's laws (140515790, D-0243): Euler30 and Fraction30 keep g at k = 1; four calls at k 0.25 (240) equal one
    // at 60 for the Euler step toward a fixed target; Fraction30 keeps 60's steady deviation under a steady drift.
    for(const float g:{0.f,0.1f,0.6f,1.9f,2.5f,-0.1f})Require(Same(Euler30(g,1.f),g) && Same(Fraction30(g,1.f),g),"I-0114 camera laws, k = 1 bit for bit");
    Require(Same(Euler30(2.5f,0.25f),2.5f) && Same(Fraction30(2.f,0.25f),2.f),"I-0114 camera laws kept outside their domain");
    for(const float k:{0.25f,0.5f,2.f,0.37f}){
        const float g=0.1f,y=3.f;float a=0.f,b=0.f;const int n=static_cast<int>(1.f/k+0.5f);
        if(std::fabs(n*k-1.f)<1e-6f){for(int i=0;i<n;++i)a+=(y-a)*(Euler30(g,k)*0.5f*k);b+=(y-b)*(g*0.5f);Require(Near(a,b,1e-5f),"I-0114 Euler30: n calls at k equal one at 60");}
        float d60=0.f,d=0.f;const float rate=0.3f,f60=0.25f,gf=2.f*f60;   // a drift of 0.3 rad/s, 60 keeping a quarter
        for(int i=0;i<600;++i)d60=f60*(d60+rate/60.f);
        for(int i=0;i<static_cast<int>(600/k);++i)d=(Fraction30(gf,k)*0.5f*k)*(d+rate*k/60.f);
        Require(Near(d,d60,1e-4f),"I-0114 Fraction30: 60's steady deviation at any rate");}
    {float u=0.f;for(int i=0;i<4;++i)u+=PerCall(0.01f,0.25f);Require(Same(PerCall(0.01f,1.f),0.01f) && Near(u,0.01f,1e-6f),"I-0114 PerCall");}
    // Step review L3: Euler30 at k = 2 (30 frames/s) equals two 60 frames/s calls; Fraction30's deviation within 0.1 % (relative).
    {const float g=0.3f,y=1.f;float a=0.f,b=0.f;a+=(y-a)*(Euler30(g,2.f)*0.5f*2.f);for(int i=0;i<2;++i)b+=(y-b)*(g*0.5f);Require(Near(a,b,1e-6f),"I-0114 Euler30 at k = 2");}
    for(const float k:{0.25f,0.5f,2.f,0.37f}){float d60=0.f,d=0.f;for(int i=0;i<600;++i)d60=0.25f*(d60+0.3f/60.f);
        for(int i=0;i<static_cast<int>(600/k);++i)d=(Fraction30(0.5f,k)*0.5f*k)*(d+0.3f*k/60.f);Require(std::fabs(d-d60)<=1e-3f*d60,"I-0114 Fraction30 relative");}
    // Step review L2: Gain keeps a g outside [0, 1] (no NaN); Euler30 and Fraction30 keep a negative g.
    Require(Same(Gain(1.5f,0.5f),1.5f) && Same(Gain(-0.1f,0.5f),-0.1f) && Same(Euler30(-0.2f,0.5f),-0.2f) && Same(Fraction30(-0.2f,0.5f),-0.2f),"I-0114 laws outside their domain");
    // RampedAfter: the reset, the untouched gain, the ramp redone on the true values.
    {const float kept=0.1f,target=0.3f,reset=0.05f,dt=1.f/240,given=Gain(kept,0.25f),reset_given=Gain(reset,0.25f);
        Require(Same(RampedAfter(reset_given,kept,target,reset,given,reset_given,dt),reset),"I-0114 RampedAfter reset");
        Require(Same(RampedAfter(given,kept,target,reset,given,reset_given,dt),kept),"I-0114 RampedAfter untouched");
        Require(Same(RampedAfter(0.12345f,kept,target,reset,given,reset_given,dt),(target-kept)*dt+kept),"I-0114 RampedAfter ramp");}
    // GiveRecentre and RecentreAfter: the hold, the decay, the call that skips its decay, an input's 1, and the use as at 60.
    {const float dt=1.f/240,span=2.f,k=0.25f;
        const auto held=GiveRecentre(0.8f,0.5f,dt,span,k);Require(held.valid && Same(held.used,0.8f) && Same(held.given,Gain(0.8f,k)),"I-0114 recentre held");
        Require(Same(RecentreAfter(held.given,0.8f,held),0.8f),"I-0114 recentre held, no decay in the call");
        const auto decays=GiveRecentre(0.8f,0.f,dt,span,k);Require(decays.valid && Same(decays.used,0.8f-dt/span),"I-0114 recentre decays");
        const float called=decays.given-dt/span;Require(Near(called,Gain(decays.used,k),1e-6f),"I-0114 recentre: the call uses Gain(used)");
        Require(Same(RecentreAfter(called,0.8f,decays),decays.used) && Same(RecentreAfter(decays.given,0.8f,decays),0.8f) && Same(RecentreAfter(1.f,0.8f,decays),1.f),"I-0114 recentre after");
        const auto ends=GiveRecentre(0.001f,0.f,dt,span,k);Require(ends.valid && Same(ends.used,0.f) && Same(ends.given,0.001f),"I-0114 recentre at its end");}
    // WallSnapThreshold: one degree at k = 1; at 240 judged once a 1/60 s from a contact's start (0 between), at one degree · k.
    {const float one=0.0174533f;float owed=0.f;Require(Same(WallSnapThreshold(one,1.f/60,1.f,false,owed),one),"I-0114 snap at 60");
        owed=0.f;float t[8];for(int i=0;i<8;++i)t[i]=WallSnapThreshold(one,1.f/240,0.25f,i==0,owed);
        Require(t[0]==0.f && t[1]==0.f && t[2]==0.f && Near(t[3],one*0.25f,1e-6f) && t[4]==0.f && t[6]==0.f && Near(t[7],one*0.25f,1e-6f),"I-0114 snap once a 1/60 s at 240");}
    // The terrain response (1405E6580 and its siblings, tranche 3): at k = 1 the original's expressions (m·0.2 + x, r·x, its
    // clamp); the clamp (a NaN kept); a steady response reaches the clamp at the same game
    // time at 240 as at 60; a decay after 1/60 s at 240 equals one call at 60.
    {const TerrainConstants c{0.2f,0.6f,0.5f,-0.5f};
        for(const float x:{0.f,0.1f,-0.3f,0.49f})for(const float m:{0.f,0.05f,-0.2f,1.f}){
            Require(Same(ClampHalf(TerrainResponse(x,m,1.f,c),c),ClampHalf(m*0.2f+x,c)) && Same(Ratio(x,0.6f,1.f),0.6f*x),"I-0114 terrain k = 1 bit for bit");}
        const float nan=std::nanf("");
        Require(Same(ClampHalf(0.7f,c),0.5f) && Same(ClampHalf(-0.7f,c),-0.5f) && Same(ClampHalf(0.2f,c),0.2f) && Same(ClampHalf(-0.2f,c),-0.2f) && std::isnan(ClampHalf(nan,c)),"I-0114 terrain clamp (a NaN kept, as the originals)");
        int n60=0,n240=0;float a=0.f,b=0.f;
        while(a<0.5f){a=ClampHalf(TerrainResponse(a,0.1f,1.f,c),c);++n60;}while(b<0.5f){b=ClampHalf(TerrainResponse(b,0.1f,0.25f,c),c);++n240;}
        Require(std::abs(n240-4*n60)<=4,"I-0114 terrain response reaches the clamp at the same game time");
        a=b=0.4f;a=Ratio(a,0.6f,1.f);for(int i=0;i<4;++i)b=Ratio(b,0.6f,0.25f);Require(Near(a,b,1e-6f),"I-0114 terrain decay at 240");
        Require(TerrainDecayDefined(0.f) && TerrainDecayDefined(1.f) && !TerrainDecayDefined(-0.1f) && !TerrainDecayDefined(1.1f) && !TerrainDecayDefined(nan),"I-0114 terrain decay's domain");}
    // The pre-bias (the ramps 1407CB390, 1407F5260, 1409B6D80): k = 1 leaves x; the original's own step lands on x + c·k; a ramp
    // guarded by x < 1 at k = 2 still takes its step and ends at 1; a 1/30 ramp from 0 ends at the same game time at 240 as at 60.
    {Require(Same(PreBias(0.3f,0.1f,1.f),0.3f) && Same(PreBias(0.3f,0.1f,1.f,1.f),0.3f),"I-0114 pre-bias k = 1");
        for(const float k:{0.25f,0.5f,2.f})Require(Near(PreBias(0.3f,0.1f,k)+0.1f,0.3f+0.1f*k,1e-6f),"I-0114 pre-bias lands on x + c·k");
        const float guarded=PreBias(0.98f,1.f/30,2.f,1.f);Require(guarded<1.f && std::fmin(guarded+1.f/30,1.f)==1.f,"I-0114 pre-bias under the ramp's end");
        int n60=0,n240=0;float a=0.f,b=0.f;
        while(a<1.f){a=std::fmin(PreBias(a,1.f/30,1.f,1.f)+1.f/30,1.f);++n60;}while(b<1.f){b=std::fmin(PreBias(b,1.f/30,0.25f,1.f)+1.f/30,1.f);++n240;}
        Require(std::abs(n240-4*n60)<=4,"I-0114 pre-bias ramp at 240");}
    // The jump's takeoff delay (tranche 7): none at 60 and under; at 120 and 240 the hold starts four 60 frames/s steps less four
    // of its own later (33.3 and 50 ms).
    {Require(TakeoffDelay(1.f/60.f)==0.f && TakeoffDelay(1.f/30.f)==0.f && TakeoffDelay(1.f/20.f)==0.f,"I-0114 takeoff delay at 60, 30 and 20");
        Require(Near(TakeoffDelay(1.f/120.f),1.f/30.f,1e-6f) && Near(TakeoffDelay(1.f/240.f),0.05f,1e-6f),"I-0114 takeoff delay at 120 and 240");
        // The handed calls: their time within half a call of the delay, with a jittered dt (±10 %, alternating) too; none for a delay
        // of microseconds (an unlocked rate running a hair under 60).
        for(const float base:{1.f/120.f,1.f/240.f,1.f/1000.f})for(const float jitter:{0.f,0.1f}){
            float owed=TakeoffDelay(base),handed=0.f,widest=0.f;
            for(int i=0;i<4000;++i){const float dt=base*(1.f+((i&1)?jitter:-jitter));widest=std::fmax(widest,dt);if(!TakeoffHand(owed,dt))break;handed+=dt;}
            Require(std::abs(handed-TakeoffDelay(base))<=0.5f*widest+1e-6f,"I-0114 takeoff delay in calls");}
        {float owed=TakeoffDelay(1.f/60.f-1e-5f);Require(!TakeoffHand(owed,1.f/60.f-1e-5f),"I-0114 takeoff delay near 60 hands no call");}}
    // The 60 frames/s ticks: one a call at 60, one every four at 240, two at 30, three at 20, the time kept over 600 calls.
    for(const float dt:{1.f/60.f,1.f/240.f,1.f/30.f,1.f/20.f,0.0071f}){float owed=0.f;int ticks=0;
        for(int i=0;i<600;++i){const int n=SixtiethTicks(owed,dt);ticks+=n;
            if(dt==1.f/60.f)Require(n==1,"I-0114 one tick a call at 60");
            if(dt==1.f/30.f)Require(n==2,"I-0114 two ticks a call at 30");
            if(dt==1.f/20.f)Require(n==3,"I-0114 three ticks a call at 20");
            if(dt==1.f/240.f)Require(n==((i%4==3)?1:0),"I-0114 one tick every four calls at 240");}
        Require(std::abs(ticks-int(std::lround(600.0*dt*60.0)))<=1,"I-0114 ticks keep the time");}
    // The ring by ticks against a ring stepped at 60: at each call that completes a tick, the averaged window (the original's walk
    // from start to next, at most size) holds the samples 60 holds then.
    {struct Ring {RingIndex r;float s[32]{};bool pending{};
            void Push(float v){s[r.next]=v;RingPush(r);}
            void Call(float v,int ticks){const auto step=RingByTicks(ticks,pending);
                if(step.rewind)RingRewind(r);
                if(r.start!=r.next)for(int j=0;j<step.copies;++j)Push(s[(r.next+r.size-1)%r.size]);
                Push(v);pending=ticks==0;}
            std::vector<float> Window() const {std::vector<float> w;uint32_t i=r.start,n=0;if(i==r.next)return w;
                do{if(n++>=r.size)break;w.push_back(s[i]);i=(i+1)%r.size;}while(i!=r.next);return w;}};
        // 240 (and an irregular 0.0071 s): 60 takes each tick's last value.
        for(const float dt:{1.f/240.f,0.0071f}){Ring a{{0,0,20}},b{{0,0,20}};float owed=0.f;bool same=true;
            for(int i=0;i<400;++i){const float v=0.25f*float(i+1);const int n=SixtiethTicks(owed,dt);b.Call(v,n);
                for(int j=0;j<n;++j)a.Push(v);
                if(n>0 && n<2)same=same && a.Window()==b.Window();}
            Require(same,"I-0114 ring by ticks under 1/60");}
        // A hitch after calls that completed no tick (the step review's L3: pending with 3 ticks, then 1/240 calls again): 60 holds
        // the previous call's sample for every tick but the last.
        {Ring a{{0,0,20}},b{{0,0,20}};float owed=0.f,previous=0.f;bool same=true,hitched=false;
            for(int i=0;i<300;++i){const float dt=(i%3==2)?0.05f:1.f/240.f,v=float(i+1);const int n=SixtiethTicks(owed,dt);b.Call(v,n);
                if(n>0){for(int j=1;j<n;++j)a.Push(previous);a.Push(v);same=same && a.Window()==b.Window();}
                hitched=hitched || (n>=2 && i>2);previous=v;}
            Require(same && hitched,"I-0114 ring by ticks after a hitch");}
        // 30: two ticks a call; 60 holds the last sample for the first tick.
        Ring c{{0,0,20}},d{{0,0,20}};float owed=0.f;
        for(int i=0;i<200;++i){const float v=float(i);c.Call(v,SixtiethTicks(owed,1.f/30.f));if(i)d.Push(float(i-1));d.Push(v);}
        Require(c.Window()==d.Window(),"I-0114 ring by ticks at 30");}
    std::printf("PASS logic rate: k = 1 bit for bit, two half-calls equal one (step, ratio, gain), a timer's game time at 120, the locomotion factor at 60, 120 and 30, the follow camera's Euler30, Fraction30 and PerCall laws, the terrain response, the pre-bias, the jump's takeoff delay, the 60 frames/s ticks and the ring by ticks\n");
    return 0;
}catch(const std::exception& e){std::printf("FAIL %s\n",e.what());return 1;}}
