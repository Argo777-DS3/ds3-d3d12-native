// R14 cR4 host control T-R6 (D3D12-R13-R14-RENDER-THREAD-PACING-SHEET 10.1): the present pacer on a fake clock, then a
// short run on the system clock.
#include "../renderer/frame_pacer.h"
#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <vector>
#include <windows.h>

using namespace ds3native;
namespace {void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}}
int main(){try{
    int64_t time=1000;std::vector<int64_t> waits;
    FramePacer pacer(FramePacer::Clock{[&]{return time;},6000,[&](int64_t deadline){waits.push_back(deadline);time=deadline;}});   // 6000 ticks a second: 100 per 1/60 s
    // Interval 0 passes through, without anchoring or waiting.
    Require(pacer.Pace(0)==0 && waits.empty(),"T-R6 interval 0 passes through");
    // I-0097 tranche 4: one rule on every screen. Paced by hand, then Present(1) (V-sync, the default): the first call presents
    // at once; a frame 30 ticks later waits until anchor + 100.
    Require(pacer.Pace(1)==1 && waits.empty(),"T-R6 first frame at once, synced");
    time+=30;Require(pacer.Pace(1)==1 && waits.size()==1 && waits[0]==1100,"T-R6 paced deadline, then Present(1)");
    // A late frame (past its deadline) presents at once, without waiting, and re-anchors there.
    time+=150;Require(pacer.Pace(1)==1 && waits.size()==1,"T-R6 late frame presents without waiting");
    time+=10;Require(pacer.Pace(1)==1 && waits.size()==2 && waits[1]==1250+100,"T-R6 re-anchored at the late frame");
    // Intervals 2 and 3: two and three sixtieths (still Present(1)); above 4 is clamped to 4.
    time+=10;Require(pacer.Pace(2)==1 && waits.size()==3 && waits[2]==1350+200,"T-R6 interval 2");
    time+=10;Require(pacer.Pace(3)==1 && waits.size()==4 && waits[3]==1550+300,"T-R6 interval 3");
    time+=10;Require(pacer.Pace(9)==1 && waits.size()==5 && waits[4]==1850+400 && pacer.SyncedPresents()==7,"T-R6 interval above 4 clamped");
    // Reset: the next frame presents at once.
    pacer.Reset();time+=1;Require(pacer.Pace(1)==1 && waits.size()==5,"T-R6 reset");
    // Without V-sync (the launcher's option): paced the same, Present(0), anchored at the wake (the original's rule, K-0009).
    {std::vector<int64_t> w;int64_t overshoot{};FramePacer off(FramePacer::Clock{[&]{return time;},6000,[&](int64_t deadline){w.push_back(deadline);time=deadline+overshoot;}});
        off.SetSync(false);Require(!off.Sync() && off.Pace(1)==0 && w.empty(),"I-0097 V-sync off: first frame at once, Present(0)");
        overshoot=3;time+=10;Require(off.Pace(1)==0 && w.size()==1,"I-0097 V-sync off: paced");
        overshoot=0;const int64_t woke=time;time+=10;Require(off.Pace(1)==0 && w.back()==woke+100 && off.SyncedPresents()==0,"I-0097 V-sync off: anchored at the wake");}
    // Step review (tranche 3): a late synced frame (at once, re-anchored), and the synced anchor: the deadline when the wake
    // overshoots under 1 ms, the wake itself beyond.
    {std::vector<int64_t> w;int64_t overshoot{};FramePacer bounds(FramePacer::Clock{[&]{return time;},6000,[&](int64_t deadline){w.push_back(deadline);time=deadline+overshoot;}});
        Require(bounds.Pace(1)==1 && w.empty(),"I-0097 synced, first frame at once");
        time+=500;Require(bounds.Pace(1)==1 && w.empty(),"I-0097 a late synced frame presents at once");
        overshoot=3;time+=10;Require(bounds.Pace(1)==1 && w.size()==1,"I-0097 synced wait");   // 0.5 ms late: anchored at the deadline
        overshoot=0;const int64_t anchored=w.back();time+=10;Require(bounds.Pace(1)==1 && w.back()==anchored+100,"I-0097 synced anchor at the deadline");
        overshoot=12;time+=10;Require(bounds.Pace(1)==1 && w.size()==3,"I-0097 synced wait, 2 ms late");
        overshoot=0;const int64_t woke=time;time+=10;Require(bounds.Pace(1)==1 && w.back()==woke+100,"I-0097 synced anchor at the wake beyond 1 ms");}
    // Final review (tranche 3), widened in tranche 4: a synced frame steps by the whole refreshes nearest its interval when within
    // 2 % (59.94 Hz: 1001/60000 s = 1001 ticks here; 119.88 Hz: two of 500.5, presented with that interval), 1/60 s otherwise
    // (279.96 Hz, a VRR screen: 4.67 refreshes; 144 Hz: 2.4; Present(1)), a screen slower than the cadence and without V-sync
    // (Present(0)).
    {std::vector<int64_t> w;FramePacer exact(FramePacer::Clock{[&]{return time;},60000,[&](int64_t deadline){w.push_back(deadline);time=deadline;}});
        exact.SetRefreshPeriod(1001.0/60000.0);const int64_t start=time;
        Require(exact.Pace(1)==1 && exact.Pace(1)==1 && w.size()==1 && w[0]==start+1001,"I-0097 59.94 Hz: the exact period");
        exact.SetRefreshPeriod(1001.0/120000.0);Require(exact.Pace(1)==2 && w.size()==2 && w[1]==start+1001+1001,"I-0097 119.88 Hz: two exact refreshes, Present(2) (step review 1)");
        exact.SetRefreshPeriod(1.0/279.96);Require(exact.Pace(1)==1 && w.size()==3 && w[2]==start+2002+1000,"I-0097 279.96 Hz: 1/60 s");
        exact.SetRefreshPeriod(1.0/144.0);Require(exact.Pace(1)==1 && w.size()==4 && w[3]==start+3002+1000,"I-0097 144 Hz: 1/60 s");
        exact.SetRefreshPeriod(1.0/50.0);Require(exact.Pace(1)==0 && w.size()==5 && w[4]==start+4002+1000,"I-0097 a screen slower than the cadence (50 Hz): 1/60 s, Present(0) (step review 2)");
        exact.SetRefreshPeriod(1001.0/60000.0);exact.SetSync(false);Require(exact.Pace(1)==0 && w.size()==6 && w[5]==start+5002+1000,"I-0097 V-sync off: 1/60 s");}
    // The system clock: 30 frames at interval 1 take at least 29/60 s (the invariant); the upper bound and the wait count
    // tolerate a loaded machine (review: a preempted frame is late and re-anchors).
    FramePacer system;LARGE_INTEGER frequency{},start{},end{};QueryPerformanceFrequency(&frequency);QueryPerformanceCounter(&start);
    for(int frame=0;frame<30;++frame)Require(system.Pace(1)==1,"T-R6 system clock interval");
    QueryPerformanceCounter(&end);const double seconds=double(end.QuadPart-start.QuadPart)/double(frequency.QuadPart);
    Require(seconds>=29.0/60.0-0.002 && seconds<29.0/60.0+0.25 && system.Waits()>=25,"T-R6 system clock cadence");
    // I-0097: the display count. One frame a refresh: no repeat; a late frame shown on the third refresh then a quick one
    // (PresentCount +2 over 3 refreshes, a sample the earlier count dropped): one repeat; a frame held three refreshes: two; a
    // failed read restarts without bridging; Take returns the counts since the previous Take.
    DisplayRefreshes display;display.Add(true,10,100);display.Add(true,11,101);display.Add(true,11,101);display.Add(true,12,102);
    auto counts=display.Take();Require(counts.samples==2 && counts.presented==2 && counts.refreshes==2 && counts.repeated==0 && counts.hitches==0,"I-0097 display steady");
    display.Add(true,14,105);display.Add(true,15,108);counts=display.Take();
    Require(counts.samples==2 && counts.presented==3 && counts.refreshes==6 && counts.repeated==3 && counts.hitches==2,"I-0097 display repeats in multi-frame samples");
    display.Add(false,0,0);display.Add(true,30,400);display.Add(true,31,401);counts=display.Take();
    Require(counts.samples==1 && counts.presented==1 && counts.refreshes==1 && counts.repeated==0,"I-0097 display restarts after a failed read");
    // A sample with fewer refreshes than frames (a frame never shown) is a shortfall and offsets no later repeat.
    display.Add(false,0,0);display.Add(true,40,500);display.Add(true,42,501);display.Add(true,43,503);counts=display.Take();
    Require(counts.presented==3 && counts.refreshes==3 && counts.repeated==1 && counts.hitches==1 && counts.shortfalls==1,"I-0097 display shortfall apart from repeats");
    Require(counts.holds.empty(),"I-0098 no holds kept without KeepHolds");
    // I-0098: with KeepHolds each sample's frames and refreshes in order (a 279.96 Hz cadence 5, 4, then two frames over 9); Take
    // moves them out and keeps the next window's in the spare it is given (none given: none kept), Reset keeps the request.
    DisplayRefreshes held;held.KeepHolds(true);held.Add(true,1,10);held.Add(true,2,15);held.Add(true,3,19);held.Add(true,5,28);counts=held.Take(DisplayRefreshes::SpareHolds(true));
    Require((counts.holds==std::vector<uint32_t>{1,5,1,4,2,9}) && counts.samples==3 && counts.repeated==4+3+7,"I-0098 holds in order");
    held.Add(true,6,33);held.Reset();held.Add(true,10,50);held.Add(true,11,54);counts=held.Take(DisplayRefreshes::SpareHolds(true));
    Require((counts.holds==std::vector<uint32_t>{1,4}),"I-0098 holds after Take and Reset");
    for(uint64_t i=0;i<400;++i)held.Add(true,12+i,60+5*i);counts=held.Take();
    Require(counts.holds.size()>=512 && counts.holds.size()<1024 && counts.holds.size()%2==0 && counts.samples==400,"I-0098 holds bounded by the reserve, the counts complete");
    held.Add(true,500,3000);held.Add(true,501,3005);counts=held.Take();Require(counts.holds.empty() && counts.samples==2,"I-0098 no spare given, no holds kept");
    // I-0111 tranche 2: the display clock (form B) on a fake clock (6000 ticks a second) and an object whose count comes back only
    // with a scripted take (the compositor taking a present).
    {int64_t now=0;int stored=0;std::vector<int64_t> takes;size_t next=0;uint32_t calls=0;
        const auto object=[&](uint32_t bound){++calls;
            while(next<takes.size() && takes[next]<=now){++stored;++next;}   // takes already past are counts waiting
            if(stored>0){--stored;return DisplayClock::Release::Signalled;}
            if(next<takes.size() && takes[next]<=now+int64_t(bound)*6){now=takes[next++];return DisplayClock::Release::Signalled;}
            now+=int64_t(bound)*6;return DisplayClock::Release::Timeout;};
        DisplayClock clock(DisplayClock::Source{[&]{return now;},object});
        // A take: the frame starts on it; the lead is the deadline minus that moment; one count taken.
        takes={120};now=100;auto o=clock.Wait(130);Require(o.released && !o.stored && now==120 && o.lead==10 && next==1,"I-0111 T2 the frame starts on the take");
        // A count already there: at once, no display event, never a second wait for this frame.
        stored=1;calls=0;now=200;o=clock.Wait(250);Require(o.stored && !o.released && now==200 && calls==1,"I-0111 T2 a stored count is no display event");
        // No take within the bound: 34 ms (204 ticks), no count taken, one owed; its late take is taken first at the next frame (a
        // 0 ms probe), then that frame blocks on its own take again (second review H1).
        takes={510,600};next=0;stored=0;now=300;o=clock.Wait(400);Require(o.timeout && now==504 && clock.Owed()==1,"I-0111 T2 a bounded wait owes its count");
        now=520;o=clock.Wait(650);Require(o.owed_taken==1 && clock.Owed()==0 && o.released && now==600,"I-0111 T2 the owed count taken, then the frame's own take");
        // A count a failed Present never gave: no wait at all.
        takes={750};next=0;stored=0;calls=0;now=700;o=clock.Wait(800,true);Require(o.skipped && calls==0 && next==0 && now==700,"I-0111 T2 a skipped object");
        // The first frame after the clock was off: a probe only (no count there: no wait).
        takes={950};next=0;stored=0;now=900;o=clock.Wait(1000,false,true);Require(o.probed && !o.released && now==900 && next==0,"I-0111 T2 a probe-only frame");
        // A failed wait (the probe and the wait both failing), and no deadline (no lead).
        {DisplayClock failing(DisplayClock::Source{[&]{return now;},[](uint32_t){return DisplayClock::Release::Failed;}});
            now=1100;o=failing.Wait(0);Require(o.failed && o.lead==0 && failing.Owed()==0,"I-0111 T2 a failed wait");}
        // Several frames: one count a frame, taken as each previous present is (a 100-tick refresh), the frame start on the take.
        takes={2000,2100,2200};next=0;stored=0;now=1950;
        for(int f=0;f<3;++f){o=clock.Wait(now+100);Require(o.released && now==2000+100*f,"I-0111 T2 one count a frame on the takes");now+=40;}
        // The screens: 60 and 59.94 Hz one refresh a frame, 120 and 119.88 Hz two; 60.02, 144, 165, 279.96, 50 Hz and unknown none.
        Require(DisplayClock::Fits(1.0/60)==1 && DisplayClock::Fits(1001.0/60000)==1 && DisplayClock::Fits(1.0/120)==2 && DisplayClock::Fits(1001.0/120000)==2,"I-0111 T2 fitting screens");
        Require(!DisplayClock::Fits(1.0/60.02) && !DisplayClock::Fits(1.0/144) && !DisplayClock::Fits(1.0/165) && !DisplayClock::Fits(1.0/279.96) && !DisplayClock::Fits(1.0/50) && !DisplayClock::Fits(0),"I-0111 T2 other screens");}
    std::printf("PASS frame pacer: T-R6 interval 0 through, first frame at once, manual deadline anchor + interval/60, late frame re-anchors, intervals 2-4 and the clamp, paced by hand then the fitting refreshes as the interval, Present(1) on other rates, Present(0) on slower screens or without V-sync (I-0097 tranche 4), the synced anchor, the exact refreshes within 2 percent, reset; the display count (repeats in multi-frame samples, restart after a failed read, a shortfall counted apart); I-0098 each sample's frames and refreshes kept in order with KeepHolds, bounded by the reserve; 30 frames on the system clock in %.3f s\n",seconds);
    return 0;
}catch(const std::exception& e){std::printf("FAIL %s\n",e.what());return 1;}}
