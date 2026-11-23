#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "frame_pacer.h"
#include <windows.h>
#include <algorithm>
#include <vector>
#include <cmath>

namespace ds3native {
namespace {
int64_t Now(){LARGE_INTEGER t{};QueryPerformanceCounter(&t);return t.QuadPart;}
int64_t Frequency(){LARGE_INTEGER f{};QueryPerformanceFrequency(&f);return f.QuadPart;}
// The coarse part on a high-resolution waitable timer (one per thread), the last millisecond as a spin on the counter.
void WaitUntil(int64_t deadline){
    const int64_t frequency=Frequency();
    thread_local HANDLE timer=CreateWaitableTimerExW(nullptr,nullptr,CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,TIMER_ALL_ACCESS);
    const int64_t remaining=deadline-Now();
    const int64_t coarse=remaining-frequency/1000;
    if(timer && coarse>0){LARGE_INTEGER due{};due.QuadPart=-(coarse*10000000/frequency);   // relative, 100 ns units
        if(SetWaitableTimer(timer,&due,0,nullptr,nullptr,FALSE))WaitForSingleObject(timer,INFINITE);}
    while(Now()<deadline)YieldProcessor();
}
}
FramePacer::FramePacer():clock_{Now,Frequency(),WaitUntil}{}
uint32_t FramePacer::Pace(uint32_t interval){
    if(!interval)return 0;
    if(interval>4)interval=4;   // the original's valid range (larger values assert in FUN_1400D5DA0)
    // I-0097 tranche 3 (K-0479, a labelled deviation from the original, which lets DXGI pace at 60 Hz): every frame is paced by
    // hand first, so each Present leaves at its deadline. Presented as soon as the frame was ready, the native's presents spread
    // over 14-19 ms and a composed 60 Hz screen dropped 110-125 of them a run (late display 75-101 frame periods, the original 28);
    // paced, then synced: 11-19 dropped, late display 12-17 (i97-t3d6). Tranche 4 (one rule on every screen): with V-sync, a
    // screen whose refreshes fit the interval (within 2 %: 59.94, 60, 119.88, 240 Hz) keeps them as the DXGI interval (a frame
    // ready early still waits its refreshes, step review 1); any other (279.96 Hz, 144 Hz, a VRR screen, unknown) gets Present(1);
    // a screen slower than the cadence (50 Hz) Present(0), as the original (step review 2: Present(1) would queue the game down
    // to the screen's rate). Without V-sync, Present(0).
    // Final review (tranche 3): a synced frame's step is its refreshes at the screen's exact rate where known (SetRefreshPeriod: a
    // 59.94 Hz screen would otherwise fall one frame behind a 60.000 pacer every 16.7 s, its presents queueing until the latency
    // wait spread them again): the whole refreshes nearest the interval, when within 2 % of it; 1/60 s otherwise.
    const double nominal=double(interval)/60.0;
    const double refreshes=period_seconds_>0?std::round(nominal/period_seconds_):0;
    const bool fits=refreshes>=1 && refreshes<=4 && std::abs(refreshes*period_seconds_/nominal-1.0)<0.02;
    const bool slower=period_seconds_>0 && !fits && period_seconds_>nominal*1.02;   // final review 3: against the cadence itself
    const bool synced=sync_ && !slower;
    const uint32_t present_interval=synced?(fits?uint32_t(refreshes):1u):0u;
    if(synced)++synced_;
    const int64_t now=clock_.now();
    if(!anchor_){anchor_=now;return present_interval;}
    const bool exact=synced && fits;
    const int64_t step=exact?int64_t(refreshes*period_seconds_*double(clock_.frequency)+0.5):int64_t(interval)*clock_.frequency/60;
    const int64_t deadline=anchor_+step;
    // Step review: a synced frame anchors at its deadline when it woke within a millisecond of it (the wake's overshoot would
    // otherwise slow the cadence below 60 frames/s against the screen); a Present(0) keeps the original's rule (the moment it
    // presented, K-0009).
    if(now<deadline){clock_.wait_until(deadline);++waits_;const int64_t woke=clock_.now();anchor_=synced && woke-deadline<clock_.frequency/1000?deadline:woke;}
    else anchor_=now;
    return present_interval;
}
DisplayClock::Outcome DisplayClock::Wait(int64_t deadline,bool skip_object,bool probe_only){
    Outcome o;o.start=source_.now();
    while(owed_ && source_.wait_object(0)==Release::Signalled){--owed_;++o.owed_taken;}   // a timed-out frame's late count
    if(skip_object)o.skipped=true;
    else if(source_.wait_object(0)==Release::Signalled)o.stored=true;   // a count already there: no display event
    else if(probe_only)o.probed=true;
    else{const auto r=source_.wait_object(object_bound_ms);
        if(r==Release::Signalled)o.released=true;else if(r==Release::Timeout){o.timeout=true;++owed_;}else o.failed=true;}
    o.end=source_.now();o.lead=deadline?deadline-o.end:0;return o;
}
uint32_t DisplayClock::Fits(double period_seconds) noexcept {
    if(!(period_seconds>0))return 0;
    const double nominal=1.0/60.0,refreshes=std::round(nominal/period_seconds);
    if(refreshes<1 || refreshes>2)return 0;
    const double frame=refreshes*period_seconds;
    return frame>=nominal*(1.0-10e-6) && frame<=nominal*1.002?uint32_t(refreshes):0u;
}
double RefreshPeriodOf(void* window) noexcept {
    try {
        MONITORINFOEXW monitor{};monitor.cbSize=sizeof(monitor);
        if(!window || !GetMonitorInfoW(MonitorFromWindow(static_cast<HWND>(window),MONITOR_DEFAULTTONEAREST),&monitor))return 0;
        UINT32 path_count{},mode_count{};
        if(GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS,&path_count,&mode_count)!=ERROR_SUCCESS)return 0;
        std::vector<DISPLAYCONFIG_PATH_INFO> paths(path_count);std::vector<DISPLAYCONFIG_MODE_INFO> modes(mode_count);
        if(QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS,&path_count,paths.data(),&mode_count,modes.data(),nullptr)!=ERROR_SUCCESS)return 0;
        for(UINT32 i=0;i<path_count;++i){DISPLAYCONFIG_SOURCE_DEVICE_NAME source{};source.header.type=DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
            source.header.size=sizeof(source);source.header.adapterId=paths[i].sourceInfo.adapterId;source.header.id=paths[i].sourceInfo.id;
            if(DisplayConfigGetDeviceInfo(&source.header)!=ERROR_SUCCESS || _wcsicmp(source.viewGdiDeviceName,monitor.szDevice))continue;
            const auto rate=paths[i].targetInfo.refreshRate;
            return rate.Numerator && rate.Denominator?double(rate.Denominator)/double(rate.Numerator):0;}
    }catch(...){}
    return 0;
}bool PlaceWindowOnScreen(void* window,const wchar_t* device) noexcept {
    struct Find {const wchar_t* device;RECT area{};bool found{};} find{device};
    if(!window || !device || !*device)return false;
    EnumDisplayMonitors(nullptr,nullptr,[](HMONITOR monitor,HDC,LPRECT,LPARAM data)->BOOL{auto& f=*reinterpret_cast<Find*>(data);
        MONITORINFOEXW info{};info.cbSize=sizeof(info);
        if(GetMonitorInfoW(monitor,&info) && !_wcsicmp(info.szDevice,f.device)){f.area=info.rcMonitor;f.found=true;return FALSE;}
        return TRUE;},reinterpret_cast<LPARAM>(&find));
    return find.found && SetWindowPos(static_cast<HWND>(window),nullptr,find.area.left,find.area.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
}
uint32_t RefreshRateOf(void* window) noexcept {
    MONITORINFOEXW monitor{};monitor.cbSize=sizeof(monitor);DEVMODEW mode{};mode.dmSize=sizeof(mode);
    if(!window || !GetMonitorInfoW(MonitorFromWindow(static_cast<HWND>(window),MONITOR_DEFAULTTONEAREST),&monitor) ||
       !EnumDisplaySettingsW(monitor.szDevice,ENUM_CURRENT_SETTINGS,&mode) || mode.dmDisplayFrequency<=1)return 0;
    return mode.dmDisplayFrequency;
}
}
