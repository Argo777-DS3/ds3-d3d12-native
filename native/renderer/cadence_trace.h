#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

namespace ds3native::cadence {
// I-0097 tranche 5 (cadence-trace.flag, diagnostics only): the moments of a frame's life on one clock (the performance counter),
// for both renderers, so the engine thread, the presenting thread and the screen's vertical blanks are read on one timeline
// beside PresentMon's presents (Nsight Systems unelevated records neither the vertical blanks nor the context switches on this
// PC). Off by default: a mark returns at once while the trace is off. Each kind keeps its ticks in order until the writer takes
// them, within a reserve (a full kind drops ticks and counts them; nothing allocates on the marking threads).
enum Kind : uint32_t {
    VBlank,              // the screen's vertical blank (D3DKMTWaitForVerticalBlankEvent on the game window's screen)
    GameStep,            // DS3's world update gate (RecordGameStep: both renderers, the same point)
    GameDt,              // its dt argument in microseconds (a value, not a tick: RT-9's dt distribution)
    EngineEndEnter,      // native: EndFrame entered (DS3's frame work and its limiter behind it)
    EnginePostDone,      // native: the post returned (the previous job done): the stamp of clock+0x48
    LatencyWaitStart,    // native worker: the frame-latency object's wait (WaitFrameSlot)
    LatencyWaitEnd,
    PaceStart,           // native worker: PresentPaced entered (before the pacer's wait)
    PresentStart,        // native worker: Present called (after the pacer's wait)
    PresentEnd,
    OriginalPacerEnter,  // original: its pacer D5CB0 entered (its wait and its Present inside)
    OriginalPacerLeave,
    DisplayWaitStart,    // native engine: the display clock's frame-start wait (I-0111 tranche 2, display-clock.flag)
    DisplayWaitEnd,
    Count
};
inline constexpr const char* names[Count]{"vblank","game_step","game_dt_us","engine_end_enter","engine_post_done","latency_wait_start",
    "latency_wait_end","pace_start","present_start","present_end","original_pacer_enter","original_pacer_leave","display_wait_start","display_wait_end"};
inline constexpr size_t capacity=4096;   // a kind's reserve between two takes (taken every second: at most 280 vblanks a second)
struct Store {std::mutex lock;std::vector<int64_t> ticks[Count];uint64_t dropped[Count]{};};
inline std::atomic<bool> on{false};
inline Store& Shared(){static Store s;return s;}
inline int64_t Now() noexcept {LARGE_INTEGER t{};QueryPerformanceCounter(&t);return t.QuadPart;}
inline void MarkValue(Kind kind,int64_t value) noexcept {
    if(!on.load(std::memory_order_relaxed))return;
    auto& s=Shared();const std::lock_guard hold(s.lock);auto& v=s.ticks[kind];
    if(v.size()<v.capacity())v.push_back(value);else ++s.dropped[kind];
}
inline void Mark(Kind kind) noexcept {if(on.load(std::memory_order_relaxed))MarkValue(kind,Now());}
// Two marks that go together (the game step and its dt, step review L7): both kept or both dropped, under one lock.
inline void MarkPair(Kind a,int64_t va,Kind b,int64_t vb) noexcept {
    if(!on.load(std::memory_order_relaxed))return;
    auto& s=Shared();const std::lock_guard hold(s.lock);auto& x=s.ticks[a];auto& y=s.ticks[b];
    if(x.size()<x.capacity() && y.size()<y.capacity()){x.push_back(va);y.push_back(vb);}else{++s.dropped[a];++s.dropped[b];}
}
}
