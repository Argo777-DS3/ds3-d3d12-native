#pragma once
// I-0132 R12c (D-0271, Thomas 2026-11-10 23:06: « Vas y », the game's own pages as the base; R12C-STATIC §8): the game's Graphics
// tab and its Advanced Settings page keep their films, their cursor, sounds and inputs, and get our rows through the game's own row
// functions. The Graphics tab: Screen Mode (Full screen > Borderless > Windowed), Resolution, V-sync, the frame rate, the HDR
// output with its peak and its paper white (I-0138 tranche 5b, sliders), the Advanced button. The Advanced page: the Quality Setting preset, DS3's five rows still in use, then our RT rows. This header holds what can be
// tested alone: our text ids and their words, the Screen Mode values, the frame-rate choices, and the RT rows' model over the settings
// file (rekindled_settings.h: D-0265's rules, an unavailable row greyed and locked, the DLSS mode and the light bounces shown only where they apply). The game-side glue is
// game_bootstrap/02b_rekindled_graphics.inc. Header-only, unit-tested (graphics_rows_test).
#include "../renderer/rekindled_settings.h"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cwchar>
#include <optional>

namespace ds3native::graphics_rows {
// Our FMG ids: 729000-729999 and their help lines at +100000 (FUN_140AB6E50 reads a row's help at id + 100000), none of which
// exists in the game's menu FMGs (engUS, fraFR; R12C-STATIC §1.3).
inline constexpr int kHelp=100000;
inline constexpr int kBorderless=729000,kVsync=729003,kFrameRate=729004,kFrameRateChoice=729020,kFrameRateCustom=729039;
inline constexpr bool Ours(int id) noexcept {return (id>=729000 && id<=729999) || (id>=829000 && id<=829999);}
// The game's words our rows reuse (D-0153): the preset's Low to Max and Custom (722021 to 722024, 722020), its Quality Setting label
// (720103) and its Advanced Settings label (720104). Our RT rows say On and Off in our words (the game's 722040 and 722041 are « Oui » and
// « Non » in French).
inline constexpr int kPresetLabel=720103,kPresetCustomWord=722020,kAdvancedLabel=720104;

// The Screen Mode row's values: the game's 1 (full screen) and 0 (windowed), and ours 2 (borderless: DS3's full screen with
// CSWindowImp +0x40 set, its screen mode 2). The list's order is Thomas's (2026-11-10 19:23): Full screen > Borderless > Windowed.
inline constexpr uint8_t kFullScreen=1,kWindowed=0,kBorderlessValue=2;
inline constexpr uint8_t ScreenModeValue(int32_t applied_mode,bool borderless_switch) noexcept {
    return applied_mode==1?(borderless_switch?kBorderlessValue:kFullScreen):kWindowed;}

// The frame-rate row: frame_rate_option's values (0 DS3's own 60, else 20 to 1000, 1000 « unlimited »). The choices in order, the
// row's byte the index; a value the settings file holds outside the list (the launcher's -FrameRate 277) is shown as one more choice.
inline constexpr float kFrameRates[]{30.f,0.f,90.f,120.f,144.f,165.f,240.f,1000.f};
inline constexpr uint8_t kFrameRateCount=uint8_t(sizeof(kFrameRates)/sizeof(kFrameRates[0])),kFrameRateCustomValue=100;
inline bool SameRate(float a,float b) noexcept {   // 0 is DS3's own 60
    const auto rate=[](float r){return r<=0.f?60.f:r;};return std::fabs(rate(a)-rate(b))<0.5f;}
inline uint8_t FrameRateValue(float rate) noexcept {
    for(uint8_t i=0;i<kFrameRateCount;++i)if(SameRate(kFrameRates[i],rate))return i;
    return kFrameRateCustomValue;}
inline std::optional<float> FrameRateOf(uint8_t value) noexcept {
    if(value<kFrameRateCount)return kFrameRates[value];
    return std::nullopt;}   // the custom choice keeps the file's value

// The RT rows of the Advanced page, after DS3's five (D-0265, RT-PT-PLAN §6), in Thomas's order (2026-11-11 00:46 and 00:47: one
// « Anti-aliasing » row above Rendering, Off, TAA, NVIDIA DLAA, NVIDIA DLSS since 05:29, the DLSS mode in a row that appears with NVIDIA DLSS). Each row's byte is the
// value the game's row edits; its choices are (value, FMG word) pairs. A row unavailable under the current settings is built greyed and
// locked (the game's GrayoutItem, the row constructor's « enabled » argument, R12C-STATIC §8.3) and shows what the renderer would run
// (settings::Effective). They take effect at once (I-0132 R12f: the module's readers take the settings live, at the next engine frame;
// frame generation's swap chain made again when it is switched).
// The sharpening right under the anti-aliasing it follows, and Reflex, then the vignette (R12g), always offered, last (the game's own pages
// end with an offered row; the faults once put on a greyed last row were the test runs' stop, I-0132 RESULT « R12c », the RCA).
// I-0132 R12g (Thomas, 2026-11-11 06:25 to 06:34): the vignette (DS3's, On or Off), last (Thomas, 15:19: « Mets Vignette tout en bas »).
// ROADMAP workstream 2 (D-0273, 2026-11-13): the GI quality row removed, its gain in motion not measurable (I-0132 RESULT « The measures
// left »); the GI's paths stay a test tool (rt-gi-paths-N.flag).
// I-0138 tranche 5b: the HDR output's rows are on the Graphics tab (below, « The HDR rows »), no longer here.
// I-0138 tranche 7 (PLAN §5, D-0285): the colorimetry and its sub-row the Rekindled look's chromatic aberration, the bloom's strength, the
// light shafts and the film grain's strength, after Reflex and before the vignette, which stays last.
enum class RtRow : uint8_t {AntiAliasing,DlssMode,Sharpen,Level,Bounces,Rr,FrameGeneration,Reflex,Look,Aberration,Bloom,Shafts,Grain,Fov,Vignette,Count};
inline constexpr uint32_t kRtRows=uint32_t(RtRow::Count);
inline constexpr int kRtLabel=729200,kRtWord=729300;   // a row's label kRtLabel + row, its help + kHelp; our choice words from kRtWord
inline constexpr int kWordRayTracing=kRtWord,kWordPathTracing=kRtWord+1,kWordDlaa=kRtWord+10,kWordQuality=kRtWord+11,kWordBalanced=kRtWord+12,
    kWordPerformance=kRtWord+13,kWordUltraPerformance=kRtWord+14,kWordX2=kRtWord+20,kWordX3=kRtWord+21,kWordX4=kRtWord+22,kWordBoost=kRtWord+30,
    kWordSharpen=kRtWord+80,kWordBounce=kRtWord+50,kWordOn=kRtWord+60,kWordOff=kRtWord+61,kWordTaa=kRtWord+70,kWordDlss=kRtWord+71,
    kWordHdrOff=kRtWord+95,   // the game's Brightness row on an HDR10 chain (below)
    kWordOriginal=kRtWord+96,kWordRekindled=kRtWord+97,kWordStrength=kRtWord+150;   // I-0138 tranche 7: the looks; kWordStrength + 0..13: « 0 % » to « 130 % »
    // kWordSharpen + 1..10: 10 % to 100 % (Thomas, 2026-11-11 02:20: a finer setting); kWordBounce + 1..4: « 1 » to « 4 »
// I-0138 tranche 5b (Thomas, 2026-11-14 12:05: « Je pr\u00E9f\u00E8re que les r\u00E9glages du HDR soient dans le menu graphics et pas advanced. Aussi, je
// pr\u00E9f\u00E8re que \u00E7a soit des curseurs plut\u00F4t que des s\u00E9lections. \u00C7a peut aller par tranche de 10 ou de 50 (au choix de R) »): the HDR rows,
// on the Graphics tab after the frame rate: the HDR output (the game's On/Off choice row), then its peak and its paper white as
// the game's slider rows (FUN_140AA43C0; the slider keeps an int and shows it), sub-rows of it (their « └ », their separator a
// quarter shorter) built only while the HDR output is on (Thomas, 2026-11-14 20:00: hidden as the DLSS mode's row is). The peak: 400 to 4000 nits by 50, and one step under 400 the automatic value (the screen's own maximum, the file's 0). The
// paper white: the file's bounds by 10. A file's value off the steps is shown as the nearest step and kept until the slider moves.
// The tab needs the film's three added slots (options_film.h).
inline constexpr int kHdrLabel=729400,kHdrPeakLabel=729401,kHdrPaperWhiteLabel=729402;   // their help lines at + kHelp
struct SliderSteps {int32_t min,max,step;};
inline constexpr int32_t kHdrPeakAuto=350;   // the peak slider's least value: automatic
inline constexpr SliderSteps kHdrPeakSlider{kHdrPeakAuto,4000,50},kHdrPaperWhiteSlider{int32_t(settings::kHdrPaperWhiteLeast),int32_t(settings::kHdrPaperWhiteMost),10};
static_assert(kHdrPeakAuto+kHdrPeakSlider.step==int32_t(settings::kHdrPeakLeast) && kHdrPeakSlider.max<=int32_t(settings::kHdrPeakMost) &&
    (kHdrPeakSlider.max-kHdrPeakSlider.min)%kHdrPeakSlider.step==0 && (kHdrPaperWhiteSlider.max-kHdrPaperWhiteSlider.min)%kHdrPaperWhiteSlider.step==0);
// A value on the slider's steps: the nearest step, within the slider.
inline constexpr int32_t OnSteps(const SliderSteps& s,int64_t value) noexcept {
    if(value<=s.min)return s.min;if(value>=s.max)return s.max;
    return int32_t(s.min+(value-s.min+s.step/2)/s.step*s.step);}
inline constexpr int32_t HdrPeakSliderValue(uint32_t nits) noexcept {return nits?(std::max)(OnSteps(kHdrPeakSlider,nits),int32_t(settings::kHdrPeakLeast)):kHdrPeakAuto;}
inline constexpr uint32_t HdrPeakOfSlider(int32_t value) noexcept {return value<int32_t(settings::kHdrPeakLeast)?0u:uint32_t(OnSteps(kHdrPeakSlider,value));}
inline constexpr int32_t HdrPaperWhiteSliderValue(uint32_t nits) noexcept {return OnSteps(kHdrPaperWhiteSlider,nits);}
inline constexpr uint32_t HdrPaperWhiteOfSlider(int32_t value) noexcept {return uint32_t(OnSteps(kHdrPaperWhiteSlider,value));}
// The settings after a slider's move (false: nothing changed: the slider still shows the file's value, on or off the steps).
inline bool HdrPeakWrite(int32_t value,settings::Settings& s) noexcept {
    if(value==HdrPeakSliderValue(s.hdr_peak))return false;s.hdr_peak=HdrPeakOfSlider(value);return true;}
inline bool HdrPaperWhiteWrite(int32_t value,settings::Settings& s) noexcept {
    if(value==HdrPaperWhiteSliderValue(s.hdr_paper_white))return false;s.hdr_paper_white=HdrPaperWhiteOfSlider(value);return true;}
// The Anti-aliasing row's values, and the DLSS mode it gives when DLSS is chosen without one held (the page keeps the last one chosen).
// NVIDIA DLAA its own choice beside the TAA and DLSS (Thomas, 2026-11-11 05:29), the DLSS mode row offering the scaled modes only.
inline constexpr uint8_t kAaTaa=0,kAaDlss=1,kAaOff=2,kAaDlaa=3;
inline constexpr settings::Dlss kDlssModeFirst=settings::Dlss::Quality;
inline constexpr uint32_t kMostChoices=12;
struct Choice {uint8_t value;int32_t word;};
struct Choices {Choice items[kMostChoices]{};uint32_t count{};
    void Add(uint8_t value,int32_t word) noexcept {if(count<kMostChoices)items[count++]={value,word};}
    bool Has(uint8_t value) const noexcept {for(uint32_t i=0;i<count;++i)if(items[i].value==value)return true;return false;}};

// Whether the row is on the page: the DLSS mode only while NVIDIA DLSS is chosen (Thomas: « un sous menu apparaît »), the light bounces
// only at the path-traced level (Thomas, 2026-11-11 05:09: « Possible de la cacher plutôt ? »), every other row always.
inline bool RtShown(RtRow row,const settings::Settings& s) noexcept {
    if(row==RtRow::DlssMode)return s.dlss!=settings::Dlss::Off && s.dlss!=settings::Dlss::Dlaa;
    if(row==RtRow::Bounces)return settings::Available(s).bounces;
    if(row==RtRow::Aberration)return s.look==settings::Look::Rekindled;   // I-0138 tranche 7: the Rekindled look's own (none in the original's)
    return true;}
inline bool RtAvailable(RtRow row,const settings::Settings& s) noexcept {
    const auto a=settings::Available(s);
    switch(row){
    case RtRow::AntiAliasing:return true;
    case RtRow::DlssMode:return s.dlss!=settings::Dlss::Off && s.dlss!=settings::Dlss::Dlaa;
    case RtRow::Level:return a.pt;                 // the PT only with NVIDIA DLAA or DLSS: without them the row is locked on Ray tracing
    case RtRow::Bounces:return a.bounces;
    case RtRow::Vignette:return true;
    case RtRow::Look:case RtRow::Bloom:case RtRow::Shafts:case RtRow::Grain:case RtRow::Fov:return true;
    case RtRow::Aberration:return s.look==settings::Look::Rekindled;
    case RtRow::Rr:return a.rr;                    // with NVIDIA DLAA or DLSS, and not at the PT level (On and locked there)
    case RtRow::FrameGeneration:return a.frame_generation;
    case RtRow::Reflex:return true;                // with frame generation its Off is not offered (D-0265: On or On + Boost)
    case RtRow::Sharpen:return a.sharpen;          // with the TAA or DLSS (Thomas, 2026-11-11 01:05: « garde le actif avec DLSS »)
    default:return false;}}
inline uint8_t AntiAliasingValue(const settings::Settings& s) noexcept {
    return s.dlss==settings::Dlss::Dlaa?kAaDlaa:s.dlss!=settings::Dlss::Off?kAaDlss:s.taa?kAaTaa:kAaOff;}
// The row's value: the settings' own while the row can be changed, what runs (settings::Effective) while it is greyed.
inline uint8_t RtValue(RtRow row,const settings::Settings& held) noexcept {
    const settings::Settings s=RtAvailable(row,held)?held:settings::Effective(held);
    switch(row){
    case RtRow::AntiAliasing:return AntiAliasingValue(s);
    case RtRow::DlssMode:return uint8_t(s.dlss!=settings::Dlss::Off && s.dlss!=settings::Dlss::Dlaa?s.dlss:kDlssModeFirst);
    case RtRow::Level:return s.level==settings::Level::Pt?1:0;
    case RtRow::Bounces:return uint8_t((std::clamp)(s.bounces,1u,settings::kBouncesMost));
    case RtRow::Vignette:return s.vignette?1:0;
    case RtRow::Look:return uint8_t(s.look);
    case RtRow::Aberration:return s.aberration?1:0;
    case RtRow::Bloom:return uint8_t((std::min)(s.bloom,settings::kStrengthMost)/10u*10u);   // percent, by tens (a file's value between, the ten below)
    case RtRow::Shafts:return s.shafts?1:0;
    case RtRow::Grain:return uint8_t((std::min)(s.grain,settings::kStrengthMost)/10u*10u);
    case RtRow::Fov:return uint8_t((std::clamp)(s.fov,settings::kFovLeast,settings::kFovMost)/10u*10u);
    case RtRow::Rr:return s.rr?1:0;
    case RtRow::FrameGeneration:return uint8_t(s.frame_generation>=2?(std::min)(s.frame_generation,4u):0u);
    case RtRow::Reflex:return uint8_t(settings::Effective(held).reflex);   // Off shown as On while frame generation runs
    case RtRow::Sharpen:return uint8_t(std::lround((std::clamp)(s.sharpen,0.f,1.f)*10.f));
    default:return 0;}}
inline Choices RtChoices(RtRow row,const settings::Settings& s) noexcept {
    Choices c;
    switch(row){
    case RtRow::AntiAliasing:c.Add(kAaOff,kWordOff);c.Add(kAaTaa,kWordTaa);c.Add(kAaDlaa,kWordDlaa);c.Add(kAaDlss,kWordDlss);break;   // Thomas (2026-11-11 01:28): « D\u00E9sactiv\u00E9 en premier »
    case RtRow::DlssMode:c.Add(2,kWordQuality);c.Add(3,kWordBalanced);c.Add(4,kWordPerformance);c.Add(5,kWordUltraPerformance);break;
    case RtRow::Level:c.Add(0,kWordRayTracing);c.Add(1,kWordPathTracing);break;
    case RtRow::Bounces:for(uint8_t n=1;n<=settings::kBouncesMost;++n)c.Add(n,kWordBounce+n);break;
    case RtRow::Vignette:c.Add(1,kWordOn);c.Add(0,kWordOff);break;
    case RtRow::Look:c.Add(0,kWordOriginal);c.Add(1,kWordRekindled);break;
    case RtRow::Aberration:case RtRow::Shafts:c.Add(1,kWordOn);c.Add(0,kWordOff);break;
    case RtRow::Bloom:for(uint8_t n=0;n<=10;++n)c.Add(uint8_t(n*10),kWordStrength+n);break;
    case RtRow::Grain:c.Add(0,kWordOff);for(uint8_t n=1;n<=10;++n)c.Add(uint8_t(n*10),kWordStrength+n);break;
    case RtRow::Fov:for(uint8_t n=uint8_t(settings::kFovLeast/10u);n<=settings::kFovMost/10u;++n)c.Add(uint8_t(n*10),kWordStrength+n);break;
    case RtRow::Rr:c.Add(1,kWordOn);c.Add(0,kWordOff);break;
    case RtRow::FrameGeneration:c.Add(0,kWordOff);c.Add(2,kWordX2);c.Add(3,kWordX3);c.Add(4,kWordX4);break;
    case RtRow::Reflex:if(settings::Available(s).reflex_off)c.Add(0,kWordOff);c.Add(1,kWordOn);c.Add(2,kWordBoost);break;
    case RtRow::Sharpen:c.Add(0,kWordOff);for(uint8_t n=1;n<=10;++n)c.Add(n,kWordSharpen+n);break;
    default:break;}
    (void)s;return c;}
// A value the row's choices offer written into the settings (false, nothing changed, for a greyed row or a value it does not offer).
// `mode`: the DLSS mode the Anti-aliasing row gives when DLSS is chosen (the page's last one, kDlssModeFirst at first).
inline bool RtWrite(RtRow row,uint8_t value,settings::Settings& s,settings::Dlss mode=kDlssModeFirst) noexcept {
    if(!RtAvailable(row,s) || !RtChoices(row,s).Has(value))return false;
    switch(row){
    case RtRow::AntiAliasing:
        if(value==kAaDlaa)s.dlss=settings::Dlss::Dlaa;
        else if(value==kAaDlss){if(s.dlss==settings::Dlss::Off || s.dlss==settings::Dlss::Dlaa)
            s.dlss=mode!=settings::Dlss::Off && mode!=settings::Dlss::Dlaa?mode:kDlssModeFirst;}
        else {s.dlss=settings::Dlss::Off;s.taa=value==kAaTaa;}
        break;
    case RtRow::DlssMode:s.dlss=settings::Dlss(value);break;
    case RtRow::Level:s.level=value?settings::Level::Pt:settings::Level::Rt;break;
    case RtRow::Bounces:s.bounces=value;break;
    case RtRow::Vignette:s.vignette=value!=0;break;
    case RtRow::Look:s.look=value?settings::Look::Rekindled:settings::Look::Original;break;
    case RtRow::Aberration:s.aberration=value!=0;break;
    case RtRow::Bloom:s.bloom=value;break;
    case RtRow::Shafts:s.shafts=value!=0;break;
    case RtRow::Grain:s.grain=value;break;
    case RtRow::Fov:s.fov=value;break;
    case RtRow::Rr:s.rr=value!=0;break;
    case RtRow::FrameGeneration:s.frame_generation=value;break;
    case RtRow::Reflex:s.reflex=settings::Reflex(value);break;
    case RtRow::Sharpen:s.sharpen=float(value)/10.f;break;
    default:return false;}
    return true;}
// The rows drawn as the game's slider while they can be changed (Thomas, 2026-11-11 02:21 for the sharpening: « C'est mieux un curseur »;
// 2026-11-14 12:05: the numeric settings as sliders): their range and step, in the row's own values (the sharpening 0 to 10, the bloom
// and the film grain in percent by tens, the field of view 80 to 130 % by tens); the slider shows the number, the unit in the label.
inline std::optional<SliderSteps> RtSlider(RtRow row) noexcept {
    switch(row){case RtRow::Sharpen:return SliderSteps{0,10,1};case RtRow::Bloom:case RtRow::Grain:return SliderSteps{0,100,10};
    case RtRow::Fov:return SliderSteps{int32_t(settings::kFovLeast),int32_t(settings::kFovMost),10};default:return std::nullopt;}}
// Whether a write of this row can change another row's availability or presence (the page then builds its rows again, 02b).
inline bool RtGoverns(RtRow row) noexcept {return row==RtRow::AntiAliasing || row==RtRow::Level || row==RtRow::FrameGeneration || row==RtRow::Look;}

// I-0132 R12g: our scroll over the Advanced page's rows. The film has fourteen slots (Item_0_0 to Item_13_0) and the game's list does not
// move rows between slots (a row takes the slot of the list's size when it is made; the R12g Ghidra study of FUN_140A90E20,
// FUN_140A913B0, FUN_140A41A40 and FUN_140A8DCB0, INFERRED for the scroll itself; a fifteenth row went off the film in r12g-pages1). So
// the page shows a window of kPageSlots rows of its list and builds its rows again when the cursor leaves it: the game's cursor wraps
// within the window (from its last selectable row down to its first, and back up), which is when the window moves. `enabled` the rows'
// selectability (a greyed row is skipped by the game's cursor), `total` their count, `first` the window's first row, `before` and
// `after` the cursor in the window before and after the game's update. The answer: the new first row and the cursor in the new window,
// or nothing when the game's own move stands (no wrap, or a wrap that lands where it should).
inline constexpr uint32_t kPageSlots=14;
struct PageWindow {uint32_t first;uint32_t cursor;};
inline uint32_t FirstRowClamped(uint32_t first,uint32_t total) noexcept {return total>kPageSlots?(std::min)(first,total-kPageSlots):0u;}
// The window's first row that keeps the list's row `row` in it (the step review's M1: a rebuild keeps the cursor on its row of the list,
// not on its slot, when rows appear or go below it and the window is clamped), clamped.
inline uint32_t FirstRowKeeping(uint32_t first,uint32_t total,uint32_t row) noexcept {
    if(row<first)first=row;else if(row>=first+kPageSlots)first=row-kPageSlots+1;
    return FirstRowClamped(first,total);}
// The film's scroll bar (ScrollBarV) shown over our list: its thumb (the clip `Bar`, a mask) placed as the game places it
// (FUN_140A53700, the R12g Ghidra study: its top at the list's first row over the total along the track, its length the visible rows over
// the total, the x scale 1), from the track's top `y0` and length `travel` the game read from the film. Its y in pixels and y scale (1 the
// whole track).
struct ThumbPlace {int32_t y;float yscale;};
inline ThumbPlace ScrollThumb(uint32_t first,uint32_t total,float y0,float travel) noexcept {
    if(!total)return {int32_t(y0),1.f};
    return {int32_t(float(first)/float(total)*travel+y0),float((std::min)(kPageSlots,total))/float(total)};}
inline std::optional<PageWindow> ScrollAfterMove(const bool* enabled,uint32_t total,uint32_t first,int32_t before,int32_t after) noexcept {
    if(!enabled || total<=kPageSlots || before==after || before<0 || after<0)return std::nullopt;
    first=FirstRowClamped(first,total);const uint32_t end=first+kPageSlots;
    int64_t low=-1,high=-1,all_low=-1,all_high=-1;
    for(uint32_t v=0;v<total;++v){if(!enabled[v])continue;if(all_low<0)all_low=v;all_high=v;
        if(v>=first && v<end){if(low<0)low=v;high=v;}}
    const int64_t vb=int64_t(first)+before,va=int64_t(first)+after;
    if(low<0 || vb>=int64_t(end) || va>=int64_t(end))return std::nullopt;
    int64_t target=-1;
    if(vb==high && va==low && va<vb){for(int64_t v=vb+1;v<int64_t(total);++v)if(enabled[v]){target=v;break;}if(target<0)target=all_low;}   // down past the window
    else if(vb==low && va==high && va>vb){for(int64_t v=vb-1;v>=0;--v)if(enabled[v]){target=v;break;}if(target<0)target=all_high;}   // up past it
    else return std::nullopt;
    uint32_t next=first;
    if(target<int64_t(next))next=uint32_t(target);else if(target>=int64_t(next+kPageSlots))next=uint32_t(target)-kPageSlots+1;
    next=FirstRowClamped(next,total);
    if(next==first && target==va)return std::nullopt;
    return PageWindow{next,uint32_t(target-int64_t(next))};}

// The mouse wheel over the page (Thomas, 2026-11-17: « molette qui ne fait pas défiler la barre latérale du menu options »): the game's
// list holds only the window's fourteen rows, so it has nothing to scroll; the window moves by `notches` rows (negative: up), clamped to
// the list, and the cursor keeps its row of the list while that row is in the new window and selectable, else takes the window's
// selectable row nearest to it. `cursor` the cursor in the window. Nothing when the window does not move.
inline std::optional<PageWindow> ScrollByWheel(const bool* enabled,uint32_t total,uint32_t first,int32_t cursor,int32_t notches) noexcept {
    if(!enabled || total<=kPageSlots || !notches)return std::nullopt;
    first=FirstRowClamped(first,total);
    const uint32_t next=uint32_t((std::clamp)(int64_t(first)+notches,int64_t(0),int64_t(total-kPageSlots))),end=next+kPageSlots;
    if(next==first)return std::nullopt;
    const int64_t row=(std::clamp)(int64_t(first)+(std::max)(cursor,0),int64_t(next),int64_t(end)-1);
    for(int64_t d=0;d<int64_t(kPageSlots);++d){
        if(row-d>=int64_t(next) && enabled[row-d])return PageWindow{next,uint32_t(row-d-int64_t(next))};
        if(row+d<int64_t(end) && enabled[row+d])return PageWindow{next,uint32_t(row+d-int64_t(next))};}
    return PageWindow{next,0u};}

// The words, in the game's language (its ordinal [14479BA10]: 1 engGB and 8 engUS English, 2 fraFR French). Another language: our
// English (the game never named these rows); the game's own words (the preset, Screen Mode's two) stay the game's, in its language.
enum class Language : uint8_t {English,French,Other};
// I-0132 R12f (Thomas, 2026-11-11 05:11 and 05:12, his mock-up subrow-separator-thomas-2026-11-11.png): a sub-row's left separator a
// quarter shorter, from its left end. The film 02_042_pc_graphicsetting.gfx (R12f's static study, OBSERVED): a slot's
// Widgets/ComboBox/Caption (sprite 51) sits at x -456 px and holds the line (an unnamed sprite, 49, centred on Caption's origin, 206 px
// each side, 3 px high) and Text_0 (the label) at x -198 px; the line has no name of its own. So Caption is scaled to 75 % in x and moved
// right by the quarter it loses (its right end stays), and Text_0 scaled back and placed so the label stays where it was, at its size.
// The values each slot's two clips take (pixels and percent, Scaleform's DisplayInfo X and XScale) from their unscaled places, read in
// the game (the run r12f-live4 read -456 and -200; the film's PlaceObject says -198 for Text_0); a normal row gets those back.
inline constexpr double kCaptionX=-456.0,kCaptionTextX=-200.0,kSeparatorHalf=206.0,kSubSeparator=0.75;
// I-0138 tranche 5d: the options film (02_040_optionsetting.gfx, the Graphics tab's slots) has the same Caption in a slot's Slider
// widget (the sprite 63 at x -456 px: its line the shape 60, 206 px each side, and Text_0), its Text_0 at x -300 px as the game
// holds it (the run i138-t5d1-tab's `slider_caption` row; the film's PlaceObject says -298, read by the pilot's reader); the game's
// own place is read in the slot before it is used, as for the page's rows.
inline constexpr double kOptionsCaptionTextX=-300.0;
struct CaptionLayout {double caption_x,caption_xscale,text_x,text_xscale;};
inline constexpr CaptionLayout RowCaption(bool sub,double caption_x=kCaptionX,double text_x=kCaptionTextX) noexcept {
    if(!sub)return {caption_x,100.0,text_x,100.0};
    const double cut=(1.0-kSubSeparator)*kSeparatorHalf;
    return {caption_x+cut,100.0*kSubSeparator,(text_x-cut)/kSubSeparator,100.0/kSubSeparator};}
inline constexpr bool SubRow(RtRow row) noexcept {return row==RtRow::DlssMode || row==RtRow::Bounces || row==RtRow::Aberration;}   // the « └ » rows
inline Language LanguageOf(int32_t ordinal) noexcept {return ordinal==2?Language::French:(ordinal==1 || ordinal==8)?Language::English:Language::Other;}
inline const wchar_t* Text(int id,Language language) noexcept {
    const bool fr=language==Language::French;
    switch(id){
    case kBorderless:return fr?L"Sans bordures":L"Borderless";
    case kVsync:return fr?L"Synchronisation verticale":L"V-sync";
    case kVsync+kHelp:return fr?L"Synchroniser les images avec l'\u00E9cran.":L"Synchronise frames with the display.";
    case kFrameRate:return fr?L"Fr\u00E9quence d'images maximale":L"Maximum frame rate";
    case kFrameRate+kHelp:return fr?L"Limiter le nombre d'images par seconde.":L"Limit the number of frames per second.";
    case kFrameRateChoice+0:return L"30";
    case kFrameRateChoice+1:return L"60 (Dark Souls III)";
    case kFrameRateChoice+2:return L"90";
    case kFrameRateChoice+3:return L"120";
    case kFrameRateChoice+4:return L"144";
    case kFrameRateChoice+5:return L"165";
    case kFrameRateChoice+6:return L"240";
    case kFrameRateChoice+7:return fr?L"Illimit\u00E9e":L"Unlimited";
    case kRtLabel+int(RtRow::AntiAliasing):return fr?L"Anticr\u00E9nelage":L"Anti-Aliasing";   // Thomas (2026-11-11 04:30)
    case kRtLabel+int(RtRow::AntiAliasing)+kHelp:return fr?L"TAA, NVIDIA DLAA ou NVIDIA DLSS (mise \u00E0 l'\u00E9chelle).":
        L"TAA, NVIDIA DLAA or NVIDIA DLSS (upscaling).";
    case kRtLabel+int(RtRow::DlssMode):return fr?L"\u2514 Mode DLSS":L"\u2514 DLSS mode";   // a sub-row, marked (Thomas, 2026-11-11 04:29: the sub-menus better seen)
    case kRtLabel+int(RtRow::DlssMode)+kHelp:return fr?L"La qualit\u00E9 de la mise \u00E0 l'\u00E9chelle de NVIDIA DLSS.":
        L"The quality of NVIDIA DLSS's upscaling.";
    case kRtLabel+int(RtRow::Level):return fr?L"Rendu":L"Rendering";
    case kRtLabel+int(RtRow::Level)+kHelp:return fr?L"Ray tracing, ou path tracing avec DLAA ou DLSS.":
        L"Ray tracing, or path tracing with DLAA or DLSS.";
    case kRtLabel+int(RtRow::Bounces):return fr?L"\u2514 Rebonds de lumi\u00E8re":L"\u2514 Light bounces";
    case kRtLabel+int(RtRow::Bounces)+kHelp:return fr?L"Rebonds de chaque chemin, en path tracing.":
        L"The bounces of each path, with path tracing.";
    case kRtLabel+int(RtRow::Vignette):return fr?L"Vignettage":L"Vignette";
    case kRtLabel+int(RtRow::Vignette)+kHelp:return fr?L"L'assombrissement des bords de l'image de Dark Souls III.":
        L"Dark Souls III's darkening of the picture's edges.";
    case kRtLabel+int(RtRow::Rr):return L"NVIDIA Ray Reconstruction";   // Thomas (2026-11-11 02:48): « NVIDIA » in front
    case kRtLabel+int(RtRow::Rr)+kHelp:return fr?L"D\u00E9bruitage NVIDIA ; toujours actif en path tracing.":
        L"NVIDIA's denoising; always on with path tracing.";
    case kRtLabel+int(RtRow::FrameGeneration):return fr?L"G\u00E9n\u00E9ration d'images NVIDIA":L"NVIDIA Frame Generation";
    case kRtLabel+int(RtRow::FrameGeneration)+kHelp:return fr?L"Images g\u00E9n\u00E9r\u00E9es par NVIDIA, avec NVIDIA DLAA ou DLSS.":
        L"Frames generated by NVIDIA, with NVIDIA DLAA or DLSS.";
    case kRtLabel+int(RtRow::Reflex):return L"NVIDIA Reflex";
    case kRtLabel+int(RtRow::Reflex)+kHelp:return fr?L"Moins de latence ; requis avec les images g\u00E9n\u00E9r\u00E9es.":
        L"Lower latency; required with frame generation.";
    case kRtLabel+int(RtRow::Sharpen):return fr?L"Nettet\u00E9":L"Sharpening";
    case kRtLabel+int(RtRow::Sharpen)+kHelp:return fr?L"NVIDIA Image Scaling (NIS) apr\u00E8s le TAA, NVIDIA DLAA ou DLSS.":L"NVIDIA Image Scaling (NIS) after TAA, NVIDIA DLAA or DLSS.";   // I-0132 R12f
    // I-0138 tranche 7.
    case kRtLabel+int(RtRow::Look):return fr?L"Colorim\u00E9trie":L"Colour grading";
    case kRtLabel+int(RtRow::Look)+kHelp:return fr?L"Celle de Dark Souls III, ou Rekindled, d'apr\u00E8s Elden Ring.":L"Dark Souls III's own, or Rekindled, after Elden Ring.";
    case kRtLabel+int(RtRow::Aberration):return fr?L"\u2514 Aberration chromatique":L"\u2514 Chromatic aberration";
    case kRtLabel+int(RtRow::Aberration)+kHelp:return fr?L"La l\u00E9g\u00E8re dispersion des couleurs vers les bords, du style Rekindled.":L"The Rekindled look's slight colour fringing towards the edges.";
    case kRtLabel+int(RtRow::Bloom):return fr?L"Lueur, bloom (%)":L"Bloom (%)";
    case kRtLabel+int(RtRow::Bloom)+kHelp:return fr?L"La lueur autour des lumi\u00E8res vives ; 100 % : celle de Dark Souls III.":L"The glow around bright lights; 100 %: Dark Souls III's own.";
    case kRtLabel+int(RtRow::Shafts):return fr?L"Rayons de lumi\u00E8re":L"Light shafts";
    case kRtLabel+int(RtRow::Shafts)+kHelp:return fr?L"Les rayons du soleil \u00E0 travers l'image, avec le bloom.":L"The sun's rays across the picture, with the bloom.";
    case kRtLabel+int(RtRow::Grain):return fr?L"Grain de film (%)":L"Film grain (%)";
    case kRtLabel+int(RtRow::Grain)+kHelp:return fr?L"Un grain de pellicule sur l'image ; absent de Dark Souls III.":L"A film grain over the picture; not in Dark Souls III.";
    case kRtLabel+int(RtRow::Fov):return fr?L"Champ de vision (%)":L"Field of view (%)";
    case kRtLabel+int(RtRow::Fov)+kHelp:return fr?L"L'angle de la cam\u00E9ra en jeu ; 100 % : celui de Dark Souls III.":L"The camera's angle in play; 100 %: Dark Souls III's own.";
    case kWordOriginal:return L"Dark Souls III";
    case kWordRekindled:return L"Rekindled";
    case kWordStrength+0:return L"0 %";
    case kWordTaa:return L"TAA";
    case kWordDlss:return L"NVIDIA DLSS";
    case kWordRayTracing:return L"Ray tracing";
    case kWordPathTracing:return L"Path tracing";
    case kWordDlaa:return L"NVIDIA DLAA";
    case kWordQuality:return fr?L"Qualit\u00E9":L"Quality";
    case kWordBalanced:return fr?L"\u00C9quilibr\u00E9":L"Balanced";
    case kWordPerformance:return L"Performance";
    case kWordUltraPerformance:return L"Ultra Performance";
    case kWordX2:return L"x2";
    case kWordX3:return L"x3";
    case kWordX4:return L"x4";
    case kWordBoost:return fr?L"Activ\u00E9 + Boost":L"On + Boost";
    case kWordSharpen+1:case kWordStrength+1:return L"10 %";
    case kWordSharpen+2:case kWordStrength+2:return L"20 %";
    case kWordSharpen+3:case kWordStrength+3:return L"30 %";
    case kWordSharpen+4:case kWordStrength+4:return L"40 %";
    case kWordSharpen+5:case kWordStrength+5:return L"50 %";
    case kWordSharpen+6:case kWordStrength+6:return L"60 %";
    case kWordSharpen+7:case kWordStrength+7:return L"70 %";
    case kWordSharpen+8:case kWordStrength+8:return L"80 %";
    case kWordSharpen+9:case kWordStrength+9:return L"90 %";
    case kWordSharpen+10:case kWordStrength+10:return L"100 %";
    case kWordStrength+11:return L"110 %";
    case kWordStrength+12:return L"120 %";
    case kWordStrength+13:return L"130 %";
    case kWordBounce+1:return L"1";
    case kWordBounce+2:return L"2";
    case kWordBounce+3:return L"3";
    case kWordBounce+4:return L"4";
    // I-0138 tranche 5b: the HDR rows of the Graphics tab, and the game's Brightness row on an HDR10 chain.
    case kHdrLabel:return fr?L"Sortie HDR":L"HDR output";
    case kHdrLabel+kHelp:return fr?L"HDR10, l\u00E0 o\u00F9 le HDR de Windows est activ\u00E9 pour l'\u00E9cran.":L"HDR10, where Windows HDR is on for the display.";
    case kHdrPeakLabel:return fr?L"\u2514 Luminance maximale (nits)":L"\u2514 Peak brightness (nits)";   // the unit here: the slider's field holds five letters (the run i138-t5b1-tab: « 450 ni », « Autom »)
    case kHdrPeakLabel+kHelp:return fr?L"La luminance la plus forte envoy\u00E9e \u00E0 l'\u00E9cran. Auto : le maximum de l'\u00E9cran.":L"The brightest value sent to the display. Auto: the display's own maximum.";
    case kHdrPaperWhiteLabel:return fr?L"\u2514 Blanc papier (nits)":L"\u2514 Paper white (nits)";
    case kHdrPaperWhiteLabel+kHelp:return fr?L"La luminance du blanc de l'interface et d'une image SDR.":L"The brightness of the interface's white and of an SDR picture.";
    case kWordHdrOff:return fr?L"Sans effet en HDR":L"No effect in HDR";
    case kWordOn:return fr?L"Activ\u00E9":L"On";
    case kWordOff:return fr?L"D\u00E9sactiv\u00E9":L"Off";
    default:return nullptr;}
}
// What an HDR slider shows beside its bar: the number, as the game writes it, and « Auto » for the peak's automatic value (the
// same word in French; the field holds five letters, the unit is in the row's label).
inline const wchar_t* HdrSliderText(bool peak,int32_t value,wchar_t (&buffer)[24]) noexcept {
    if(peak && value<int32_t(settings::kHdrPeakLeast))std::swprintf(buffer,24,L"Auto");
    else std::swprintf(buffer,24,L"%d",value);
    return buffer;}
// The custom frame-rate choice's words (« 277 »), written into the caller's buffer.
inline const wchar_t* CustomRateText(float rate,wchar_t (&buffer)[16]) noexcept {
    std::swprintf(buffer,16,L"%u",static_cast<unsigned>(rate+0.5f));return buffer;}
}
