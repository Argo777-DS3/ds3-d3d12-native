#pragma once
// I-0129 R9 (R9-PLAN §2): the traced probes. DS3 lights a forward object (the instance record's aSh rows and ShLightMask.x) and a lit
// effect (1DB60's SH rows and mask) from one probe at its position; R9 traces that probe: rays from the object's box lit as the GI's
// (rt_reflect.hlsl's `probe`), read back once the frame's work is done, averaged by object over frames here, and written by the
// bootstrap into the same constants. This header is the CPU's side, pure (rt_probes_test.cpp): the GPU's records, the rows from the
// sums, the states by key.
// The rows: for a uniform direction ω and the radiance L met along it, the cosine-weighted mean of L about a normal n, which is the
// L1 SH irradiance over π, is f(n) = mean(L) + 2 mean(L ω) · n (Y00² = 1 / 4π, the band-1 kernel 2/3 times Y1² = 3 / 4π); DS3's rows are
// that form, `max(dp4((n, 1), row), 0)` a channel (R9-PLAN §1), in the unit the GI pass adds (`gi`: albedo (1 − F0) L of a cosine ray).
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <span>
#include <unordered_map>
#include <vector>
namespace ds3native {
inline constexpr uint32_t kRtProbesMost=1024;    // probes a frame (the rest wait for a later frame: the list's order, the CPU's choice)
// The rays a probe a frame are rt_reflect.hlsl's RT_PROBE_RAYS (128, one a thread of its group; the C++ side reads only the sums).
inline constexpr float kRtProbeBlend=0.05f;      // a frame's weight in a probe's state (a time constant of about twenty frames)
inline constexpr uint64_t kRtProbeRecent=8;       // a state answers only while its key was delivered within this many deliveries (the final review's H1)
// The step review's M2: what a state gives (its `rows` and `sun`) moves only once its average has moved past these from it (the rows:
// their largest change against their band 0's largest magnitude; the sun's share: absolute), so a still light gives the same record
// bytes frame after frame (the carried instance tables, the kept draws) and the rays' noise does not shimmer on the object.
inline constexpr float kRtProbeStepRows=0.03f,kRtProbeStepSun=0.03f;
enum : uint32_t {kRtProbeRequestNoSun=1};   // a request's flags: no sun rays (a model whose deferred meshes share its record: R9a's step review H1)
inline constexpr float kRtProbeBoxMost=8.f,kRtProbeMixedBoxMost=4.f;   // metres a box's side at most; a model with G-buffer rows (the final review's M3)
inline constexpr uint32_t kRtProbeRaysLeast=8;   // the rays a frame's ambient needs (the others met a back face: the box in a wall)
inline constexpr uint64_t kRtProbeKeep=120;      // deliveries a state outlives without one of its own (then dropped)
inline constexpr float kRtProbeCell=1.f;         // metres: an effect's probe's cell (R9b; the cell its box)
inline constexpr uint64_t kRtProbeStaleMs=500;   // a last delivery older than this (the rays or the probes stopped): no state answers
// A request (32 bytes, rt_reflect.hlsl's RtProbeRequest): the object's world box and the light set its misses take (Unmet).
struct RtProbeRequest {std::array<float,3> lo{};uint32_t set{};std::array<float,3> hi{};uint32_t flags{};};
static_assert(sizeof(RtProbeRequest)==32);
// A result (64 bytes, rt_reflect.hlsl's `probe` writes it): the sums over the frame's valid rays of L (rgb) and their count, of L ω a
// channel (xyz) with the sun's visible rays (red.w) and its traced rays (green.w); blue.w unused.
struct RtProbeResult {std::array<float,4> sum{};std::array<float,4> red{},green{},blue{};};
static_assert(sizeof(RtProbeResult)==64);
enum : uint32_t {kRtProbeAmbient=1,kRtProbeSun=2};   // a frame's bits: its ambient traced with the cache (a GI frame), its sun traced
// The frame's requests (the bootstrap's) with their keys, and where their answers go.
class RtProbeStates;
struct RtProbeRequests {std::vector<RtProbeRequest> requests;std::vector<uint64_t> keys;std::shared_ptr<RtProbeStates> states;uint64_t epoch{};};
// The rows (three float4, DS3's form: xyz the band-1 vector, w the band 0) of a frame's sums; false below kRtProbeRaysLeast valid rays.
inline bool RtProbeRowsOf(const RtProbeResult& r,std::array<float,12>& rows) noexcept {
    const float n=r.sum[3];if(!(n>=float(kRtProbeRaysLeast)))return false;
    const std::array<const std::array<float,4>*,3> ch{&r.red,&r.green,&r.blue};
    for(int c=0;c<3;++c){for(int k=0;k<3;++k)rows[4*c+k]=2.f*(*ch[c])[k]/n;rows[4*c+3]=r.sum[c]/n;}
    for(const float x:rows)if(!std::isfinite(x))return false;
    return true;}
// The sun's visible share of a frame's rays (r.red[3] visible of r.green[3] traced); none without rays.
inline std::optional<float> RtProbeSunOf(const RtProbeResult& r) noexcept {
    if(!(r.green[3]>0.f) || !std::isfinite(r.red[3]))return std::nullopt;
    return std::clamp(r.red[3]/r.green[3],0.f,1.f);}
// An effect's cell key (R9b): the position's metre cell, 21 bits an axis (±1,048 km), tagged apart from an object's address (bit 63).
inline uint64_t RtProbeCellKey(const float p[3]) noexcept {
    uint64_t key=1ull<<63;
    for(int k=0;k<3;++k){const double c=std::floor(double(p[k])/kRtProbeCell);const int64_t i=std::clamp<int64_t>(int64_t(c),-(1<<20),(1<<20)-1);
        key|=uint64_t(i+(1<<20))<<(21*k);}
    return key;}
inline RtProbeRequest RtProbeCellRequest(uint64_t key,uint32_t set) noexcept {
    RtProbeRequest r;r.set=set;
    for(int k=0;k<3;++k){const int64_t i=int64_t((key>>(21*k))&((1u<<21)-1))-(1<<20);r.lo[k]=float(i)*kRtProbeCell;r.hi[k]=r.lo[k]+kRtProbeCell;}
    return r;}
// A value with its mantissa rounded to 10 bits (a relative step under 0.1 %): a state's rows move by less between most frames, so the
// records the bootstrap writes stay the same bytes (its carried instance tables and kept draws reused) while the light is still.
inline float RtProbeQuantize(float x) noexcept {
    if(!std::isfinite(x) || x==0.f)return std::isfinite(x)?x:0.f;
    uint32_t u{};std::memcpy(&u,&x,4);u=(u+(1u<<12))&~((1u<<13)-1);float y{};std::memcpy(&y,&u,4);return std::isfinite(y)?y:x;}
// One probe's state: its rows and sun share averaged over the deliveries that had them (`mean`, `sun_mean`), what it gives (`rows`,
// `sun`: the averages once they moved past kRtProbeStep*), the last delivery that touched it.
struct RtProbeState {std::array<float,12> rows{},mean{};float sun{1.f},sun_mean{1.f};bool ambient{},sun_valid{};uint64_t seen{};};
inline void RtProbeSettle(RtProbeState& s) noexcept {
    float scale=0.f,moved=0.f;for(int c=0;c<3;++c)scale=std::max(scale,std::abs(s.mean[4*c+3]));
    for(size_t k=0;k<12;++k)moved=std::max(moved,std::abs(s.mean[k]-s.rows[k]));
    if(moved>kRtProbeStepRows*std::max(scale,1e-4f))s.rows=s.mean;}
// The states by key (an object's draw group, or an effect's cell), shared by the renderer's completions (Deliver, the render side)
// and the bootstrap's readers (Find, the collect's workers and the engine's threads): a shared lock.
class RtProbeStates {
public:
    // A frame's answers, in its requests' order (keys[i] answered by results[i]), and the frame's bits (kRtProbe*: what it traced).
    // A state's first answer is taken whole, a later one blends into its average by kRtProbeBlend; a frame that traced the ambient but met too few
    // rays for one probe keeps its rows. Then the states unseen for kRtProbeKeep deliveries are dropped.
    // The final review's H1: a frame that traced the sun but gave a key no sun rays (its request flagged: a model with G-buffer rows
    // now) clears that key's share at once. L2: the answers of requests made before the last Clear (their `epoch`) are dropped.
    void Deliver(std::span<const uint64_t> keys,std::span<const RtProbeResult> results,uint32_t bits,uint64_t now_ms,uint64_t epoch) {
        const std::unique_lock hold(lock_);if(epoch!=epoch_)return;++deliveries_;bits_=bits;last_ms_=now_ms;
        for(size_t i=0;i<keys.size() && i<results.size();++i){
            std::array<float,12> rows{};const bool ambient=(bits&kRtProbeAmbient) && RtProbeRowsOf(results[i],rows);
            const auto sun=(bits&kRtProbeSun)?RtProbeSunOf(results[i]):std::nullopt;
            auto& s=states_[keys[i]];s.seen=deliveries_;
            if(ambient){if(!s.ambient)s.rows=s.mean=rows;else{for(size_t k=0;k<12;++k)s.mean[k]+=kRtProbeBlend*(rows[k]-s.mean[k]);RtProbeSettle(s);}s.ambient=true;}
            if(sun){if(!s.sun_valid)s.sun=s.sun_mean=*sun;else{s.sun_mean+=kRtProbeBlend*(*sun-s.sun_mean);if(std::abs(s.sun_mean-s.sun)>kRtProbeStepSun)s.sun=s.sun_mean;}s.sun_valid=true;}
            else if(bits&kRtProbeSun)s.sun_valid=false;}
        if(deliveries_%16==0)for(auto it=states_.begin();it!=states_.end();)it=deliveries_-it->second.seen>kRtProbeKeep?states_.erase(it):std::next(it);}
    // A key's state as the last delivery's frame allows it: its ambient only if that frame traced the ambient, its sun only if it
    // traced the sun (a frame without the GI: the baked rows again, at once), none once that delivery is kRtProbeStaleMs old or the key
    // was not delivered within kRtProbeRecent deliveries (no longer asked).
    std::optional<RtProbeState> Find(uint64_t key,uint64_t now_ms) const {const std::shared_lock hold(lock_);
        if(now_ms>last_ms_+kRtProbeStaleMs)return std::nullopt;
        const auto it=states_.find(key);if(it==states_.end() || deliveries_-it->second.seen>kRtProbeRecent)return std::nullopt;auto s=it->second;s.ambient&=(bits_&kRtProbeAmbient)!=0;s.sun_valid&=(bits_&kRtProbeSun)!=0;
        if(!s.ambient && !s.sun_valid)return std::nullopt;return s;}
    // Drops every state (the rays turned off, a load): the baked probes again until new answers.
    void Clear() {const std::unique_lock hold(lock_);states_.clear();bits_=0;++epoch_;}
    uint64_t Epoch() const {const std::shared_lock hold(lock_);return epoch_;}
    size_t Size() const {const std::shared_lock hold(lock_);return states_.size();}
    uint64_t Deliveries() const {const std::shared_lock hold(lock_);return deliveries_;}
private:
    mutable std::shared_mutex lock_;std::unordered_map<uint64_t,RtProbeState> states_;uint64_t deliveries_{},last_ms_{},epoch_{};uint32_t bits_{};
};
}
