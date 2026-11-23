#pragma once
#include <atomic>
#include <cstdint>
#include <functional>
#include <vector>

namespace ds3native {
// R14 cR4 (D3D12-R13-R14-RENDER-THREAD-PACING-SHEET 6.1-6.2): the original's present pacer (FUN_1400D5CB0 / D5E30, K-0009)
// on the thread that presents. Interval 0 passes through (Present(0)). Every valid interval (1 to 4) is paced by hand:
// deadline = anchor + interval / 60 s; the caller waits until the deadline, then presents. I-0097 tranche 4 (Thomas, 2026-10-24:
// one rule on every screen, as modern games do): with V-sync (the default) the screen's refreshes that fit the interval as the
// DXGI interval (within 2 %: Present(1) at 59.94-60 Hz as in tranche 3, K-0479; Present(2) at 119.88-120 Hz), Present(1) on any
// other rate (a variable-refresh screen, G-Sync, shows each frame as it arrives; a fixed one at the next refresh), Present(0) on
// a screen slower than the cadence (50 Hz, the original's rule); without V-sync (SetSync(false), the launcher's option) Present(0), the caller allowing tearing where the
// chain can (the screen's VRR and the driver's V-sync then decide). The moment it presented becomes the next anchor (a synced
// frame: its deadline, when it woke within 1 ms); a frame that arrives after its deadline presents at once and re-anchors.
// The first call presents at once and anchors. No assumption of a fixed cadence beyond the interval itself (D-0167, D-0169:
// the game clock is not touched).
class FramePacer {
public:
    // Ticks of a monotonic clock (QueryPerformanceCounter by default) and its frequency; `wait_until(ticks)` blocks until
    // that tick (by default a high-resolution waitable timer for the coarse part, then a spin on the clock, as the
    // original's Sleep plus spin).
    struct Clock {std::function<int64_t()> now;int64_t frequency{};std::function<void(int64_t)> wait_until;};
    FramePacer();   // the system clock
    explicit FramePacer(Clock clock):clock_(std::move(clock)){}
    // The interval to pass to Present after any wait this call made (0 to 4: the fitting refreshes, 1, or 0; see above).
    uint32_t Pace(uint32_t interval);
    void Reset() noexcept {anchor_=0;}   // a resize or a long stop: the next frame presents at once
    // Tranche 4: V-sync on (the default) or off; I-0132 R12f: changed live by another thread than the presenting one (atomic).
    void SetSync(bool sync) noexcept {sync_.store(sync,std::memory_order_relaxed);}
    bool Sync() const noexcept {return sync_.load(std::memory_order_relaxed);}
    // Final review (I-0097 tranche 3): the screen's exact refresh period in seconds (RefreshPeriodOf). A synced frame steps by
    // the whole refreshes nearest its interval when they are within 2 % of it (59.94 Hz, 119.88 Hz), 1/60 s otherwise (a VRR
    // screen at 279.96 Hz, 144 Hz, unknown: 0).
    void SetRefreshPeriod(double seconds) noexcept {period_seconds_=seconds;}
    uint64_t Waits() const noexcept {return waits_;}
    uint64_t SyncedPresents() const noexcept {return synced_;}   // presents given a DXGI sync interval (I-0097: also paced, so they overlap Waits)
private:
    Clock clock_;
    int64_t anchor_{};
    uint64_t waits_{},synced_{};double period_seconds_{};std::atomic<bool> sync_{true};
};
// I-0111 tranche 2 (the modern cadence, form B: MODERN-CADENCE-STUDY §6.7, CADENCE-REFLEX-READY §2, D-0240): the frame-start wait
// on the engine thread, at the end of the native EndFrame, before DS3's limiter: one wait of the swap chain's frame-latency object
// a frame, at a maximum latency of 1 on a fitting screen, so the object is released when the compositor takes the previous present
// (the display's event: the frame then starts on it, DS3's limiter spinning only what is left to its own deadline). A count already
// there returns at once and is no display event, counted apart (`stored`; step review H-A). One count a frame only: a second wait
// could need a present the engine has not made yet (H1 of the first review). A wait that timed out took no count while its present
// gives one later: the clock owes it and takes it first at the next frame with a 0 ms probe, so the frames pair with their presents
// again (second review H1). `probe_only` (the first frame after the clock was off: the present it pairs with may never be taken,
// a minimised window's) takes a count only if one is there. `skip_object` (a count a failed Present never gave) takes none. The
// wait is bounded at 34 ms (I-0097 tranche 5). `deadline` (DS3's limiter's next one, ticks; 0 unknown) only measures how far
// before it the frame starts (`lead`).
class DisplayClock {
public:
    enum class Release {Signalled,Timeout,Failed};
    struct Source {std::function<int64_t()> now;std::function<Release(uint32_t)> wait_object;};
    struct Outcome {bool stored{},released{},timeout{},failed{},skipped{},probed{};uint32_t owed_taken{};int64_t start{},end{},lead{};};
    explicit DisplayClock(Source source):source_(std::move(source)){}
    Outcome Wait(int64_t deadline,bool skip_object=false,bool probe_only=false);
    uint32_t Owed() const noexcept {return owed_;}
    void Reset() noexcept {owed_=0;}
    // The screens it applies to while the logic is locked at 60 (§6.2 point 5): V-Sync on and one or two refreshes a frame not
    // shorter than 1/60 s by more than 10 ppm and at most 0.2 % longer (59.94-60.000 Hz, 119.88-120 Hz); not a VRR screen, not
    // 144, 165 or 279.96 Hz, not 60.02 Hz. The refreshes a frame (1 or 2: the Present interval), 0 when it does not fit.
    static uint32_t Fits(double period_seconds) noexcept;
    static constexpr uint32_t object_bound_ms=34;
private:
    Source source_;uint32_t owed_{};
};
// cR4: the refresh rate of the display the window (an HWND) is on, 0 when unknown (the presenter follows a change of screen with
// it). I-0097: also the original's, for its display rows.
uint32_t RefreshRateOf(void* window) noexcept;
// The exact refresh period (seconds) of the display the window is on, from its display path's rational rate (59.94 Hz:
// 1001/60000 s); 0 when unknown.
double RefreshPeriodOf(void* window) noexcept;
// I-0097 tranche 3 (Thomas, 2026-10-24: the screen chosen at launch, as modern games offer): the window (an HWND) moved to the
// top-left of the screen whose device name is `device` (\\.\DISPLAY2), its size kept, before the swap chain is made there; false
// when no attached screen has that name or the move failed.
bool PlaceWindowOnScreen(void* window,const wchar_t* device) noexcept;
// I-0097 (I-0095 §4, §8): what the display showed, from a swap chain's frame statistics read after each Present (the
// statistics of the last frame it showed): every advance of PresentCount is counted with the refreshes between the vblanks
// those frames reached the screen on (PresentRefreshCount; SyncRefreshCount is only when DXGI sampled its clock, which jitters
// by a refresh); the refreshes beyond the frames shown repeated a frame on screen, counted per sample (final review: a sample with
// fewer refreshes than frames, a frame never shown, is counted apart as a shortfall and cancels no repeat). A sample spanning several frames (a late frame then a quick one) still counts its
// repeats, which the earlier count, keeping only advances of exactly one, missed. A failed read (occluded, a disjoint
// statistics stream) restarts from the next one instead of bridging the gap. Used for the native chain and for the original's
// (its probe), so both renderers are counted alike. At 60 frames/s on a 120 Hz display every frame holds two refreshes: the
// rows carry the refresh rate. Presents discarded unseen (interval 0, tearing) count as shown and hide repeats.
// I-0098: with KeepHolds (frame-dump.flag) each sample's frames and refreshes are also kept in order (holds: frames, refreshes,
// frames, refreshes, ...), so a reader counts the refreshes beyond any screen's cadence (4 or 5 a frame at 60 frames/s on a
// 279.96 Hz screen, where every frame « repeats »), not only beyond one a frame.
struct DisplayRefreshes {
    // repeated: refreshes beyond the frames shown; hitches: samples with a repeat; shortfalls: samples with fewer refreshes than frames.
    struct Counts {uint64_t samples{},presented{},refreshes{},repeated{},hitches{},shortfalls{};std::vector<uint32_t> holds;};
    void KeepHolds(bool keep){keep_holds_=keep;if(keep)counts_.holds.reserve(512);}
    void Add(bool shown,uint64_t present_count,uint64_t present_refresh_count) noexcept {
        if(!shown){last_present_=0;return;}
        if(present_count==last_present_)return;   // nothing new shown since the previous read
        if(last_present_ && present_count>last_present_ && present_refresh_count>=last_refresh_){
            const uint64_t frames=present_count-last_present_,refreshes=present_refresh_count-last_refresh_;
            ++counts_.samples;counts_.presented+=frames;counts_.refreshes+=refreshes;
            if(refreshes>frames){counts_.repeated+=refreshes-frames;++counts_.hitches;}else if(refreshes<frames)++counts_.shortfalls;
            // Within the reserve only (no allocation under the presenting thread's lock); a window's samples fit it.
            if(keep_holds_ && counts_.holds.size()+2<=counts_.holds.capacity()){counts_.holds.push_back(uint32_t(frames<UINT32_MAX?frames:UINT32_MAX));
                counts_.holds.push_back(uint32_t(refreshes<UINT32_MAX?refreshes:UINT32_MAX));}}
        last_present_=present_count;last_refresh_=present_refresh_count;}
    // Since the previous Take. Step review: the next window's holds go into `spare`, reserved by the caller outside any lock
    // (SpareHolds); without a reserve no holds are kept (Add never allocates).
    Counts Take(std::vector<uint32_t> spare={}) noexcept {Counts taken=std::move(counts_);counts_={};spare.clear();counts_.holds=std::move(spare);return taken;}
    bool KeepsHolds() const noexcept {return keep_holds_;}
    static std::vector<uint32_t> SpareHolds(bool keep) noexcept {std::vector<uint32_t> spare;if(keep)try{spare.reserve(512);}catch(...){}return spare;}
    void Reset() noexcept {std::vector<uint32_t> holds=std::move(counts_.holds);holds.clear();counts_={};counts_.holds=std::move(holds);last_present_=last_refresh_=0;}
private:
    Counts counts_;uint64_t last_present_{},last_refresh_{};bool keep_holds_{};
};
}
