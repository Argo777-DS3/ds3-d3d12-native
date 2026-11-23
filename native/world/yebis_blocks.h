// W5f YB-1 (YEBIS-WAVE1-READY 1.2, 1.3, 4.2): the three bank blocks the first YEBIS converters read, built from the entries of
// a gparam group at a key (Banks::GroupEntries), and FB890's blends between two keys. Pure functions; the golden is
// evidence/I-0068/queries/yebis-ready/bank-blocks.tsv (Get-YebisBankBlocks.ps1, the executable specification).
#pragma once
#include "light_producers.h"
#include <array>
#include <cstdint>
#include <vector>

namespace ds3native::yebis {
using Entries=std::vector<ds3light::Banks::BankEntry>;
// FUN_1400E42A0's float[35]: the first 35 `Tone Map` entries in file order, one word each. Refused (false) when the group has
// fewer than 35 entries. A positive AutoExposure word (9) is not refused here: YB-4b.4's guard (ToneAutoFlags) logs it and the
// feed keeps the file's exposure (MapOverride_0001 and _0003 set it, YEBIS-SECOND-TRANCHE-READY 2.5).
bool ToneBlock(const Entries& entries,std::array<uint32_t,35>& out);
// FUN_1400E4460's float[42] in the engine's order: the first 36 `Bloom` entries expanded (float4 entries give four words) to the
// file words f[0..41], then engine words 0-3 = f[0..3], 4 = the record-only 1.0, 5-12 = f[4..11], 13-15 = f[30..32],
// 16-23 = f[33..40], 24-41 = f[12..29]. Refused when the expansion is not 42 words.
bool BloomBlock(const Entries& entries,std::array<uint32_t,42>& out);
// FUN_1400E4070's int[21]: the type-3 entries of `Setting Info` in file order as words 0..17, SceneLuminanceScale's bits as
// word 20, words 18 and 19 zero.
bool SettingBlock(const Entries& entries,std::array<uint32_t,21>& out);
// The blends at weight t (epsilon [143D7ECE8]): |t| <= eps -> a, |t - 1| <= eps -> b; otherwise Tone Map (140218BA0) copies
// words 2, 9, 28, 29, 34 from b and lerps the others (b - a) t + a in float; Bloom (1402148D0) copies 8, 9, 13, 27, 31, 33,
// 36, 39 and lerps the others (word 4 too); Setting Info (1401D8220) is b with word 20 lerped.
std::array<uint32_t,35> BlendTone(const std::array<uint32_t,35>& a,const std::array<uint32_t,35>& b,float t,float eps);
std::array<uint32_t,42> BlendBloom(const std::array<uint32_t,42>& a,const std::array<uint32_t,42>& b,float t,float eps);
std::array<uint32_t,21> BlendSetting(const std::array<uint32_t,21>& a,const std::array<uint32_t,21>& b,float t,float eps);
// W10 YB-4b (FFX-WAVE1-READY 4.1): the 28-word GXLightShaftFilterParam block (record +0x10) FUN_1400E4700 reads, from the 15
// `LightShaft` entries: 0 Enable, 4-7 ColorScale, 8 ShaftLength, 9 the record-only 0, 10 the record-only 0x3F7FFF58, 11
// GlareRatio, 12-21 entries 4-13, 24-27 RingOuterColor; words 1-3, 22, 23 padding (zero). Refused without the two float4.
bool LightShaftBlock(const Entries& entries,std::array<uint32_t,28>& out);
// W10 YB-4b: the `Anti-Alias` record (the 9 entries in file order: Enable, AntiAliasType, then 7 floats; refused on another
// layout) and FUN_140220B50: weight 0 (snap) gives a; otherwise words 0 and 1 from b and, at the snap on 1, b's words 2-8,
// else words 2-8 lerped. FB890 derives 144962190 from words 0 and 1 (and the quality and factor gates).
bool AntiAliasBlock(const Entries& entries,std::array<uint32_t,9>& out);
std::array<uint32_t,9> BlendAntiAlias(const std::array<uint32_t,9>& a,const std::array<uint32_t,9>& b,float t,float eps);
// The `FeedBackBlur` record: Enable and FeedbackWeight as words 0 and 1, a record-only word 2 = 0.5 (FB890's stack default
// 0x3F000000), entries 2-6 as words 3-7 (INFERRED from the original's globals 144962178..188 = 0, 0, 0.5, 0, 0 at m37,
// Reference1603, where the file order put FeedbackRate 0.8 in word 4). FUN_140221640 as FB890 calls it: weight <= 0 gives a; otherwise
// word 0 from b and words 1-7 from b at weight >= 1, else lerped.
bool FeedbackBlock(const Entries& entries,std::array<uint32_t,8>& out);
std::array<uint32_t,8> BlendFeedback(const std::array<uint32_t,8>& a,const std::array<uint32_t,8>& b,float t);
// W10 YB-4b.1 (YEBIS-SECOND-TRANCHE-READY 2.2): the 34-word `Dof` record FUN_1400E4890 reads: word k = file entry k except
// word 12 = entry 13 (the second Enable), words 13-32 = entries 14-33, word 33 = entry 12 (BackgroundMaskThreshold);
// FUN_14021E720: the snaps, otherwise words 0, 8-13, 18, 24, 25 from b and the others lerped.
bool DofBlock(const Entries& entries,std::array<uint32_t,34>& out);
std::array<uint32_t,34> BlendDof(const std::array<uint32_t,34>& a,const std::array<uint32_t,34>& b,float t,float eps);
// YB-4b.2 (2.3): the 21-word `MotionBlur` record: w0 Enable, w1 the mode 2 (not in the file), w2 BlurTimeRatio, w3
// MaxBlurLength, w4 NumBaseSamples, w5 MaxRecurrences, w6-w8 entries 6-8, w9 0, w10-w11 entries 9-10, w12 MotionBlurScale,
// w13 Bidirectional; the GX half: w14 the enable byte, w15-w17 the three intensities, w18 ReccurenceCount, w19 the three flag
// bytes, w20 Base Fps. FUN_1401D9910: the snaps, otherwise words 0, 1, 4, 5, 9, 13, 14, 18, 19 from b and the others lerped
// (word 12 lerped here: the original leaves word 12, the top bytes of word 14 and byte 3 of word 19 as the caller's buffer
// holds them, in FB890 the DoF record just built; the feed copies them from it in that branch).
bool MotionBlurBlock(const Entries& entries,std::array<uint32_t,21>& out);
std::array<uint32_t,21> BlendMotionBlur(const std::array<uint32_t,21>& a,const std::array<uint32_t,21>& b,float t,float eps);
// W10 YB-4c (FFX-WAVE1-READY 4.2): the integer part of the colour-grading block (Enable, LutSourceId, the two fade ids -1, the
// fade progress 0; the names and texture fields are not built) and the integer table of FUN_14021C150 for records without a
// fade in progress: w <= 0 {A0, A1, A1, B1, 0}, w >= 1 {B0, B1, B1, B1, 1.0}, otherwise {B0, A1, A1, B1, w}. FB890 writes the
// five words to 144962228..238 (144962228: the LUT-copy trigger).
bool GradingBlock(const Entries& entries,std::array<uint32_t,5>& out);
std::array<uint32_t,5> BlendGrading(const std::array<uint32_t,5>& a,const std::array<uint32_t,5>& b,float t);
// FUN_14021D690: the snaps, otherwise words 0 and 9 from b, words 4-8, 10-21, 24-27 lerped, the padding not written (zero).
std::array<uint32_t,28> BlendLightShaft(const std::array<uint32_t,28>& a,const std::array<uint32_t,28>& b,float t,float eps);
// W10 YB-4b.4 (YEBIS-SECOND-TRANCHE-READY 2.5, 4.3): auto exposure and auto focus. The flags compare as signed integers (> 0):
// the Tone Map record's word 9 (AutoExposure, file entry 9) and word 0x30 (`Enable AutoExposure`, the GX half, found by name),
// and the DoF record's word 24 (file entry 25, the auto-focus Enable). The stored exposures are the record-only words 0x35 and
// 0x36 (FB890 writes 144962008 into word 0x35 of both records before the blend).
// Record words, by number: w0 = word 0, w9 = word 9, w23 = word 0x23 (35), w30 = word 0x30 (48), w35 = word 0x35 (53), w36 =
// word 0x36 (54); the last four names are hexadecimal word numbers.
struct ToneStored {uint32_t w0{},w9{},w23{},w30{},w35{},w36{};};
// FUN_140218BA0's two exposure words {out word 0, out word 0x23}: at a snap the record's word, replaced by its stored word when its
// flag is positive; in between, flags (a, b) both positive: lerp of the stored words; only a: lerp(a stored, b plain); only b:
// lerp(a plain, b stored); neither: the generic lerp.
std::array<uint32_t,2> BlendToneStored(const ToneStored& a,const ToneStored& b,float t,float eps);
struct AutoFlags {bool exposure{},exposure_gx{},focus{};bool Any() const noexcept {return exposure || exposure_gx || focus;}};
// The flags of one Tone Map record (word 9 and `Enable AutoExposure`); the feed checks both records of the blend (2.5).
AutoFlags ToneAutoFlags(const Entries& tone);
bool DofAutoFocus(const std::array<uint32_t,34>& dof);
// The read-back the HAL owes (YB-8, a `Readback` lifetime class; 4.3): the library's adapted exposure (FUN_1416CA160,
// [S+0x1F0] + slot x 0x68 + 0x54, into 144962008) and measured focus distance (FUN_1416CA2E0, [S+0x1F8] + slot x 0x48 + 0x34,
// into 144962120), CPU values the library fills from a GPU result (INFERRED), mapped `latency` frames after the copy (PROPOSED 2,
// the swap-chain depth). Until the host exists (YB-7) both features are refused with a row and nothing is read back.
struct ReadbackDecl {const char* name;uint32_t global;uint32_t bytes;uint32_t latency;};
inline constexpr ReadbackDecl kReadbacks[]{{"exposure",0x4962008u,4u,2u},{"focus_distance",0x4962120u,4u,2u}};
// The camera-cut rule of CollectNativeVelocityInputs: the camera moved more than 10 units or its forward vectors' dot is below
// 0.5 (poses are row-major 4x4, translation in 12-14, forward in 8-10).
bool CameraCut(const float* previous_pose,const float* pose) noexcept;
}
