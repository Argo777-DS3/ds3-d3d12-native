#pragma once
// I-0114 (the unlocked frame rate, method B, D-0240; UNLOCK-READY §0-§1): the general laws that make a per-call update of DS3's
// logic correct above 60 frames/s. `k` is the frame's dt over 1/60 s (FrameK: 1 at and under 60, I-0113 gap 27, unless unlock-fix-below-60.flag; exactly 1 at 60 the original's bits: every law returns
// its 60 frames/s expression unchanged when k == 1). Header-only, no game memory: unit-tested alone (logic_rate_test).
#include <cmath>
#include <cstdint>
#include <cstring>

namespace ds3native::logic_rate {

// The k the corrections take from a frame's dt (I-0113 gap 27, I-0132 R12f): dt·60 above 60 frames/s, exactly 1 at 60 and under
// it, where the game's own per-call laws run (the original's at that rate, as with DS3's 60 setting and a slow frame). The laws
// were validated above 60 only (I-0114: k < 1); at k about 3 (a 144 setting with the game at 20 frames/s) the terrain response
// raised the character on stairs (Thomas, 2026-11-11), and the jump's takeoff was already left to the original under 60 (I-0114
// tranche 7). Continuous at k = 1 (every law is its 60 expression there), so a rate wavering about 60 switches nothing.
// `below_60` keeps the corrections at k > 1 (unlock-fix-below-60.flag, the A/B). Not finite or not positive: 1.
inline float FrameK(float dt,bool below_60=false) noexcept {
    if(!(dt>0.f) || !std::isfinite(dt))return 1.f;
    const float k=dt*60.f;
    if(std::abs(k-1.f)<1e-4f)return 1.f;
    return k>1.f && !below_60?1.f:k;}

// x += c per call → x += c·k.
inline float Step(float x,float c,float k) noexcept {return k==1.f?x+c:x+c*k;}
// A per-call quantity (a step, or a divisor of a per-call distance) as k calls of 60 frames/s: c·k.
inline float PerCall(float c,float k) noexcept {return k==1.f?c:c*k;}
// x *= r per call → x *= r^k (a decay or a growth by a constant ratio).
inline float Ratio(float x,float r,float k) noexcept {return k==1.f?x*r:x*std::pow(r,k);}
// x += (y − x)·g per call → the gain 1 − (1 − g)^k. Defined for g in [0, 1]; outside it g is kept (a power of a negative base
// with a non-integer k would be NaN; step review L2).
inline float Gain(float g,float k) noexcept {return k==1.f || !(g>=0.f && g<=1.f)?g:1.f-std::pow(1.f-g,k);}
// A per-call distance compared with a threshold: the distance a 60 frames/s call would have covered.
inline float Distance(float d,float k) noexcept {return k==1.f?d:d/k;}
// x += (y − x)·g·k30 per call, k30 = dt / (1/30) (an Euler step: 0.5·g at 60): the gain g' for which k30·g' is the exact gain
// 1 − (1 − g/2)^k. Defined for g in [0, 2] (the 60 frames/s step not overshooting); outside it g is kept.
inline float Euler30(float g,float k) noexcept {return k==1.f || !(g>=0.f && g<=2.f)?g:2.f*Gain(0.5f*g,k)/k;}
// d = F·(d + δ) per call, δ the call's own change (a deviation of which only the fraction F = g·k30 is kept, the rest dropped):
// the fraction F that keeps 60's steady deviation for a steady rate of change, F/(1 − F) = F60/(k·(1 − F60)), F60 = g/2, returned
// as its g' = F/k30. Defined for F60 in [0, 1); outside it g is kept.
inline float Fraction30(float g,float k) noexcept {
    const float f=0.5f*g;if(k==1.f || !(f>=0.f && f<1.f))return g;
    return 2.f*(f/(f+k*(1.f-f)))/k;}

// A field given a law's value for one call of the follow camera (140515790) and put back after it.
inline bool SameBits(float a,float b) noexcept {return !std::memcmp(&a,&b,sizeof a);}
// A gain the call ramps toward its target by dt, or resets (the focus and view gains): the call was given `given`, `reset_given`
// in place of `kept`, `reset`; from what it left, the true value: the reset, the untouched gain, or the ramp redone on the true
// values ((target − kept)·dt + kept, the call's own expression).
inline float RampedAfter(float after,float kept,float target,float reset,float given,float reset_given,float dt) noexcept {
    if(SameBits(after,reset_given))return reset;
    if(SameBits(after,given))return kept;
    return (target-kept)*dt+kept;}
// The recentre timer r (a gain the call uses once, after its hold `hold` and its decay by dt / span): `used`, the value this call
// will use (the call's own expressions), and `given`, the field to write so that the call uses Gain(used, k) (the rest of the
// timer kept). Out of [0, 1], nothing given (`valid` false).
struct RecentreGiven {float used,given;bool valid;};
inline RecentreGiven GiveRecentre(float r,float hold,float dt,float span,float k) noexcept {
    float h=hold-dt;if(h<0.f)h=0.f;float u=r;
    if(h<=0.f){u=r-dt/span;if(u<0.f)u=0.f;}
    if(!(u>=0.f && u<=1.f))return {u,r,false};
    return {u,Gain(u,k)+(r-u),true};}
// After the call: an input's 1 kept (2518); the field as given means the call did not run its decay (no character, or the hold):
// r back; otherwise the decay ran: `used`.
inline float RecentreAfter(float after,float r,const RecentreGiven& g) noexcept {
    if(after==1.f)return 1.f;
    if(SameBits(after,g.given))return r;
    return g.used;}
// The wall recovery's snap (`|angle a call| < one degree` → 0, 140518989): at 60 it is judged once a 1/60 s on the angle a 60
// frames/s call turns. With the angle kept in the current rate's units (k · the 60 frames/s angle), the threshold for this call:
// one degree · k when a 1/60 s has passed since the last judged call (or a contact starts: `was_zero`), else 0 (no snap), so a
// contact's onset is judged after a full 1/60 s as at 60. `owed` carries the time since the last judged call.
inline float WallSnapThreshold(float one_degree,float dt,float k,bool was_zero,float& owed) noexcept {
    if(k==1.f){owed=0.f;return one_degree;}
    if(was_zero)owed=0.f;
    owed+=dt;
    if(owed>=1.f/60.f-1e-6f){owed-=1.f/60.f;if(owed>1.f/60.f)owed=0.f;return one_degree*k;}
    return 0.f;}

// 1405C72B0 (LTF's site, UNLOCK-SITES; read in GhidraMCP 2026-10-30): the locomotion factor [+0x25C]. With the distance branch
// open, a per-call distance whose 30-fold is under `one` decays it (×0.8, or 0.5 at or under `floor`), otherwise it grows (×1.2, or
// 1.0 at or over `one`). Corrected: the distance as at 60 frames/s, the ratios as powers of k. The constants are DS3's own (read
// from its image by the caller): one [0x143D7EE10], thirty [0x143D7EF84], floor [0x143D7EDB4], decay [0x143D7EDF0], growth
// [0x143D7EE18].
struct LocomotionConstants {float one{},thirty{},floor{},decay{},growth{};};
inline float LocomotionFactor(float x,bool distance_branch,float distance,float k,const LocomotionConstants& c) noexcept {
    // The original's COMISS/JBE pairs take their branch on an unordered compare: a NaN factor is reset to 0.5 or 1.0 (step review L1).
    if(distance_branch && Distance(distance,k)*c.thirty<c.one)return !(x>c.floor)?0.5f:Ratio(x,c.decay,k);
    return !(x<c.one)?1.0f:Ratio(x,c.growth,k);
}

// The terrain response [+0x2DC] (TRTF's site and its two siblings, I-0114 tranche 3; GhidraMCP 2026-10-31): per call, either a
// response added (x + 0.2·m, m the least of the ground probes' answers) or a decay (x·r), with no dt. From the value before the
// call: the response's step times k (Step), the decay r^k (Ratio, r in [0, 1] only); then the functions' clamp to ±0.5.
struct TerrainConstants {float gain{},decay{},half{},minus_half{};};   // [0x143D7ED94] 0.2, [0x143D7EDCC] 0.6, [0x143D7EDB4], [0x143D7F0C0]
// The clamp to ±0.5 both 1405E6580 and 1405D2280 apply after the call (their COMISS/JBE and CMOVBE pairs keep a NaN as it is).
inline float ClampHalf(float v,const TerrainConstants& c) noexcept {return v>c.half?c.half:(c.minus_half>v?c.minus_half:v);}
inline float TerrainResponse(float before,float least,float k,const TerrainConstants& c) noexcept {return Step(before,least*c.gain,k);}
inline bool TerrainDecayDefined(float r) noexcept {return r>=0.f && r<=1.f;}

// The jump's takeoff (1409DEE60, I-0114 tranche 4, series 29): a character's mover sets its proxy's velocity from the call's
// displacement while its ground state [+0x1DA] is set, and once a jump clears it, holds the velocity Havok read back (the last
// step's) through the air. In the original at 60 the takeoff comes about two game steps after the jump's animation reaches it
// (OBSERVED: the held speed 7.47 m/s at 60, 7.05 at 120 and 6.84 at 240 on the same rising root motion); at a shorter step the
// hold starts earlier on that ramp. Tranche 7 (series 29-31's per-step rows): the jump's root motion, aligned on the state's start,
// lags 60's by 3·(1/60 − dt) (24.4 ms at 120, 29-35 at 240, every jump), and the speed the original would hold after a delay of n
// such terms, simulated on six runs' rows, is 60's 7.47 at n = 4 (7.43-7.51; n = 2, tranche 4's, 7.36-7.42), and series 35 held
// 7.44-7.52 at 240 and 7.46-7.47 at 120 with it. The delay that gives the hold the original's game time: 4·(1/60 − dt) from the
// takeoff, the calls inside it handed the ground state (TakeoffHand). Under 60 the expression is negative and nothing is done: a
// step longer than 60's cannot be split, and the hold given the velocity of |delay| earlier (series 35, the mover's own history)
// held 6.84-6.91 m/s at 30 and 7.12 at 20, further from 60's than the original's own 7.56 and 7.67 (the root motion's coarse
// steps, whose samples no single delay maps onto 60's: 38 ms back would fit 30, 18 ms 20).
inline float TakeoffDelay(float dt) noexcept {const float d=4.f*(1.f/60.f-dt);return d>0.f?d:0.f;}
// Whether this call of dt is handed the ground state, the delay still owed consumed: the calls rounded to the nearest (step review
// M1: a call handed only while at least half of it is owed, so a delay of microseconds near 60, or jitter's residue, hands none).
inline bool TakeoffHand(float& owed,float dt) noexcept {if(!(owed>=0.5f*dt))return false;owed-=dt;return true;}

// The 60 frames/s ticks a call of dt owes (one at 60): the whole 1/60 s in what was owed plus dt, the rest kept (a residue over a
// tick, from a frame longer than 64 ticks, dropped).
inline int SixtiethTicks(float& owed,float dt) noexcept {
    owed+=dt;int n=0;while(owed>=1.f/60.f-1e-6f && n<64){owed-=1.f/60.f;++n;}
    if(owed>1.f/60.f || owed<0.f)owed=0.f;return n;}
// A ring of per-call samples averaged over its last `size` (140D71920's; its push: the sample at `next`, next + 1, and start
// moved past next when it meets it or lies outside the ring). Corrected by 60 frames/s ticks, so the ring holds one sample a
// 1/60 s at every rate: the newest sample is a tick in progress (`pending`) after a call that owed none, and the next call's push
// replaces it (RingRewind: next back by one) until a call completes that tick; a call completing more ticks first repeats the
// newest sample for them (sample and hold: RingPush after copying it). At 60 (one tick a call) neither.
struct RingIndex {uint32_t start{},next{},size{};};
inline void RingPush(RingIndex& r) noexcept {r.next=(r.next+1)%r.size;if(r.size<=r.start || r.start==r.next)r.start=(r.next+1)%r.size;}
inline bool RingRewind(RingIndex& r) noexcept {if(r.start==r.next || r.size<2)return false;r.next=(r.next+r.size-1)%r.size;return true;}
struct RingStep {bool rewind;int copies;};
inline RingStep RingByTicks(int ticks,bool pending) noexcept {
    if(pending)return ticks<=1?RingStep{true,0}:RingStep{false,ticks-2};
    return {false,ticks>0?ticks-1:0};}

// A ramp the original steps by a constant c a call inside a function that also uses or copies the stepped value (UNLOCK-READY §1
// method A, the pre-bias): the value to write before the call so that the original's own x + c lands on x + c·k. `below`, when
// given (> 0), keeps the written value under the ramp's end so that the original still takes its step (a ramp guarded by
// x < end, at k > 1).
inline float PreBias(float x,float c,float k,float below=0.f) noexcept {
    if(k==1.f)return x;
    const float v=x+c*(k-1.f);
    return below>0.f && !(v<below)?std::nextafter(below,0.f):v;}

}
