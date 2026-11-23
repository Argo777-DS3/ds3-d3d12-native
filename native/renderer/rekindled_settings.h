#pragma once
// I-0132 R12b (D-0265, D-0266, STUDY-R12-SETTINGS-PAGE §4): the remaster's settings, one file beside the module,
// rekindled-settings.ini, in place of the one-word files of each option (dlss-mode.txt, rr-mode.txt, rt-level.txt, rt-bounces.txt,
// reflex-mode.txt, fg-mode.txt, frame-rate.txt, sharpen.txt). The play launcher writes it (and moves the old files' words into it
// once); R12c's page will write it too. The module reads it once at start; a test run's *.flag files keep their priority there.
//
// The file: one « key = value » a line, any case, blanks around both, « ; » or « # » comment lines and « [section] » lines skipped,
// a UTF-8 mark (or a UTF-16 one, either order: its zero bytes dropped), CR and LF, a decimal comma accepted; a key named twice keeps its last value; an unknown key is counted and
// ignored, a refused value keeps that entry's earlier value (its default if none) and is named in `refused`. Header-only: unit-tested alone
// (rekindled_settings_test).
//
// D-0265's rules live here, not in the page's drawing: Available() says which entries can be changed (an unavailable one is greyed
// and locked; the page hides only the DLSS mode without NVIDIA DLSS and the bounces below the PT level, D-0271), Effective() what the renderer would run: the PT level, Ray Reconstruction and frame generation only with
// a DLSS mode; Ray Reconstruction On, and locked, at the PT level (RT-PT-PLAN §6: the PT runs only on the frames RR denoises);
// Reflex Off locked while frame generation runs (Off then runs as On); the TAA only without a DLSS mode (the anti-aliasing without
// DLSS); the sharpening with the TAA or a DLSS mode (Thomas, 2026-11-11 01:05: « garde le actif avec DLSS »; its pass after either);
// the bounces and the GI's paths (I-0132 R12g) at the PT level only.
#include "dlss_mode.h"
#include "../world/frame_rate_option.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

namespace ds3native::settings {
enum class Level : uint8_t {Rt,Pt};
enum class Dlss : uint8_t {Off,Dlaa,Quality,Balanced,Performance,UltraPerformance};
enum class Reflex : uint8_t {Off,On,Boost};
// I-0138 tranche 7 (D-0285): the colorimetry, DS3's own (the default) or the Rekindled look after Elden Ring.
enum class Look : uint8_t {Original,Rekindled};
// The PT level's traced vertices a path (rt_scene.h's kRtPathVerticesDefault and kRtPathVerticesMost; the module checks they match).
inline constexpr uint32_t kBouncesDefault=2,kBouncesMost=4;
inline constexpr uint32_t kStrengthMost=100;   // I-0138 tranche 7: the bloom's and the film grain's strengths, in percent
inline constexpr uint32_t kFovLeast=80,kFovMost=130,kFovDefault=100;   // I-0138 tranche 7: the field of view, percent of the game's
// I-0138 tranche 5: the HDR10 output's bounds, in nits (200: the usual paper white of a desktop HDR screen).
inline constexpr uint32_t kHdrPeakLeast=400,kHdrPeakMost=10000,kHdrPaperWhiteLeast=80,kHdrPaperWhiteMost=500,kHdrPaperWhiteDefault=200;

// The defaults are D-0265's and D-0266's: the RT level, no DLSS mode (the TAA on), RR On once a mode runs, Reflex and frame
// generation off, no sharpening, DS3's 60 frames a second, the V-sync on, the window as DS3 makes it (Borderless off); I-0132 R12g:
// DS3's vignette on. ROADMAP workstream 2 (D-0273, 2026-11-13): R12g's GI quality removed with its row (no gain measured in motion), so
// `gi_paths` is an unknown line now (the GI's paths a test tool, rt-gi-paths-N.flag). I-0113 tranche 40 (Thomas, 2026-11-11 23:53:
// « off forcé »): Firelink's loading walls have no key
// any more (tranche 38's `firelink_wall`, now an unknown line): the native never makes them (firelink_wall, 04_updates_and_test_tools.inc).
struct Settings {
    Level level{Level::Rt};
    uint32_t bounces{kBouncesDefault};         // 1 to kBouncesMost
    Dlss dlss{Dlss::Off};
    bool rr{true};
    Reflex reflex{Reflex::Off};
    uint32_t frame_generation{};               // 0 off, else the multiplier 2, 3 or 4
    bool taa{true};
    float sharpen{};                           // 0 (none) to 1
    float frame_rate{};                        // frame_rate_option's: 0 DS3's 60, 20 to 1000 (1000 « unlimited »)
    bool vsync{true};
    bool borderless{};
    bool vignette{true};                       // I-0132 R12g (D-0266): DS3's tone-map vignette where the area's Vignette Power asks it
    // I-0138 tranche 5: the HDR10 output (where Windows' HDR is on for the game's screen: an SDR screen keeps the SDR chain), the
    // brightest value written (nits; 0: the screen's own maximum) and the paper white (nits of the SDR white: the UI, a diffuse white).
    bool hdr{};
    uint32_t hdr_peak{};                       // 0, or kHdrPeakLeast to kHdrPeakMost
    uint32_t hdr_paper_white{kHdrPaperWhiteDefault};   // kHdrPaperWhiteLeast to kHdrPaperWhiteMost
    // I-0138 tranche 7 (PLAN §5, D-0276, D-0285): the colorimetry (the original the default), the Rekindled look's lateral
    // chromatic aberration (kept by default: Thomas, 2026-11-12 12:34; none in the original's), the bloom's strength (percent of
    // DS3's own), the light shafts, and the film grain's strength (a new effect, none by default).
    Look look{Look::Original};
    bool aberration{true};
    uint32_t bloom{kStrengthMost};             // 0 to kStrengthMost (percent)
    bool shafts{true};
    uint32_t grain{};                          // 0 to kStrengthMost (percent)
    uint32_t fov{kFovDefault};                 // kFovLeast to kFovMost (percent of the follow camera's param row, LockCamParam)
    bool operator==(const Settings&) const = default;
};

struct Availability {bool pt,rr,frame_generation,reflex_off,taa,sharpen,bounces;};
inline Availability Available(const Settings& s) noexcept {
    const bool dlss=s.dlss!=Dlss::Off,pt=dlss && s.level==Level::Pt,fg=dlss && s.frame_generation>=2;
    return {dlss,dlss && !pt,dlss,!fg,!dlss,dlss || s.taa,pt};}
inline Settings Effective(Settings s) noexcept {
    const bool dlss=s.dlss!=Dlss::Off;
    if(!dlss){s.level=Level::Rt;s.rr=false;s.frame_generation=0;}
    else {s.taa=false;if(s.level==Level::Pt)s.rr=true;}
    if(!dlss && !s.taa)s.sharpen=0;
    if(s.frame_generation>=2 && s.reflex==Reflex::Off)s.reflex=Reflex::On;
    return s;}

namespace detail {
inline std::string Lower(std::string_view text) {
    std::string out;out.reserve(text.size());
    for(const char c:text)out.push_back(char(std::tolower(static_cast<unsigned char>(c))));
    return out;}
inline std::string_view Trim(std::string_view text) noexcept {
    while(!text.empty() && std::isspace(static_cast<unsigned char>(text.front())))text.remove_prefix(1);
    while(!text.empty() && std::isspace(static_cast<unsigned char>(text.back())))text.remove_suffix(1);
    return text;}
inline bool Switch(const std::string& v,bool& out) noexcept {
    if(v=="on" || v=="true" || v=="yes" || v=="1"){out=true;return true;}
    if(v=="off" || v=="false" || v=="no" || v=="0"){out=false;return true;}
    return false;}
}

struct ParseResult {
    Settings settings;
    std::vector<std::string> accepted;   // the known keys read with an accepted value, in the file's order (a key twice, twice)
    uint32_t unknown{};                  // the lines with an unknown key, or without « = »
    std::vector<std::string> refused;    // the known keys whose value was refused (their earlier value kept, the default if none)
    std::vector<std::string> refused_values;   // the refused values as written (blanks trimmed), parallel to `refused`
    bool Accepted(std::string_view key) const noexcept {for(const auto& k:accepted)if(k==key)return true;return false;}
    bool Refused(std::string_view key) const noexcept {for(const auto& k:refused)if(k==key)return true;return false;}
    // The last refused value of a key as written (empty when none was refused).
    std::string RefusedValue(std::string_view key) const {for(size_t i=refused.size();i-->0;)if(refused[i]==key && i<refused_values.size())return refused_values[i];return {};}
};

inline ParseResult Parse(std::string_view bytes) {
    ParseResult result;
    // The final review's L1: a UTF-16 file (Windows PowerShell's > or Out-File) read by dropping its zero bytes after its mark, as
    // dlss_mode.h does: the keys and values are ASCII.
    std::string narrowed;
    if(bytes.size()>=2 && ((static_cast<unsigned char>(bytes[0])==0xFF && static_cast<unsigned char>(bytes[1])==0xFE) ||
        (static_cast<unsigned char>(bytes[0])==0xFE && static_cast<unsigned char>(bytes[1])==0xFF))){
        for(size_t i=2;i<bytes.size();++i)if(bytes[i])narrowed.push_back(bytes[i]);
        bytes=narrowed;}
    if(bytes.size()>=3 && static_cast<unsigned char>(bytes[0])==0xEF && static_cast<unsigned char>(bytes[1])==0xBB &&
        static_cast<unsigned char>(bytes[2])==0xBF)bytes.remove_prefix(3);
    auto& s=result.settings;
    while(!bytes.empty()){
        const size_t end=bytes.find_first_of("\r\n");
        std::string_view line=detail::Trim(bytes.substr(0,end));
        bytes.remove_prefix(end==std::string_view::npos?bytes.size():end+1);
        if(line.empty() || line.front()==';' || line.front()=='#' || line.front()=='[')continue;
        const size_t eq=line.find('=');
        if(eq==std::string_view::npos){++result.unknown;continue;}
        const std::string key=detail::Lower(detail::Trim(line.substr(0,eq)));
        const std::string raw(detail::Trim(line.substr(eq+1)));const std::string value=detail::Lower(raw);
        bool ok=true;
        if(key=="level"){if(value=="rt")s.level=Level::Rt;else if(value=="pt")s.level=Level::Pt;else ok=false;}
        else if(key=="bounces"){char* stop{};const long n=value.empty()?0:std::strtol(value.c_str(),&stop,10);
            if(!value.empty() && stop && !*stop && n>=1 && n<=9)s.bounces=(std::min)(uint32_t(n),kBouncesMost);else ok=false;}
        else if(key=="dlss"){using dlss_mode::Parsed;const auto p=dlss_mode::Parse(value);
            if(!p)ok=false;
            else switch(*p){case Parsed::Off:s.dlss=Dlss::Off;break;case Parsed::Dlaa:s.dlss=Dlss::Dlaa;break;case Parsed::Quality:s.dlss=Dlss::Quality;break;
                case Parsed::Balanced:s.dlss=Dlss::Balanced;break;case Parsed::Performance:s.dlss=Dlss::Performance;break;
                case Parsed::UltraPerformance:s.dlss=Dlss::UltraPerformance;break;}}
        else if(key=="rr")ok=detail::Switch(value,s.rr);
        else if(key=="reflex"){if(value=="off")s.reflex=Reflex::Off;else if(value=="on")s.reflex=Reflex::On;else if(value=="boost")s.reflex=Reflex::Boost;else ok=false;}
        else if(key=="frame_generation"){if(value=="off" || value=="0")s.frame_generation=0;else if(value=="on" || value=="x2" || value=="2")s.frame_generation=2;
            else if(value=="x3" || value=="3")s.frame_generation=3;else if(value=="x4" || value=="4")s.frame_generation=4;else ok=false;}
        else if(key=="taa")ok=detail::Switch(value,s.taa);
        else if(key=="sharpen"){std::string number=value;for(auto& c:number)if(c==',')c='.';char* stop{};
            const float v=number.empty()?0.f:std::strtof(number.c_str(),&stop);
            if(!number.empty() && stop && !*stop && std::isfinite(v) && v>=0.f)s.sharpen=v>=1e-3f?(std::min)(1.0f,v):0.f;else ok=false;}
        else if(key=="frame_rate"){const auto rate=frame_rate_option::Parse(value);if(rate)s.frame_rate=*rate;else ok=false;}
        else if(key=="vsync")ok=detail::Switch(value,s.vsync);
        else if(key=="borderless")ok=detail::Switch(value,s.borderless);
        else if(key=="vignette")ok=detail::Switch(value,s.vignette);
        else if(key=="hdr")ok=detail::Switch(value,s.hdr);
        else if(key=="hdr_peak" || key=="hdr_paper_white"){const bool peak=key=="hdr_peak";char* stop{};const long n=value.empty()?-1:std::strtol(value.c_str(),&stop,10);
            if(peak && (value=="auto" || value=="0"))s.hdr_peak=0;
            else if(value.empty() || !stop || *stop)ok=false;
            else if(peak){if(n>=long(kHdrPeakLeast) && n<=long(kHdrPeakMost))s.hdr_peak=uint32_t(n);else ok=false;}
            else if(n>=long(kHdrPaperWhiteLeast) && n<=long(kHdrPaperWhiteMost))s.hdr_paper_white=uint32_t(n);else ok=false;}
        else if(key=="look"){if(value=="original")s.look=Look::Original;else if(value=="rekindled")s.look=Look::Rekindled;else ok=false;}
        else if(key=="aberration")ok=detail::Switch(value,s.aberration);
        else if(key=="shafts")ok=detail::Switch(value,s.shafts);
        else if(key=="bloom" || key=="grain"){char* stop{};const long n=value.empty()?-1:std::strtol(value.c_str(),&stop,10);
            if(value.empty() || !stop || *stop || n<0 || n>long(kStrengthMost))ok=false;else (key=="bloom"?s.bloom:s.grain)=uint32_t(n);}
        else if(key=="fov"){char* stop{};const long n=value.empty()?-1:std::strtol(value.c_str(),&stop,10);
            if(value.empty() || !stop || *stop || n<long(kFovLeast) || n>long(kFovMost))ok=false;else s.fov=uint32_t(n);}
        else {++result.unknown;continue;}
        (ok?result.accepted:result.refused).push_back(key);if(!ok)result.refused_values.push_back(raw.substr(0,64));
    }
    return result;}

inline const char* Name(Level v) noexcept {return v==Level::Pt?"pt":"rt";}
inline const char* Name(Look v) noexcept {return v==Look::Rekindled?"rekindled":"original";}
inline const char* Name(Reflex v) noexcept {return v==Reflex::Boost?"boost":v==Reflex::On?"on":"off";}
inline const char* Name(Dlss v) noexcept {
    switch(v){case Dlss::Dlaa:return "dlaa";case Dlss::Quality:return "quality";case Dlss::Balanced:return "balanced";
        case Dlss::Performance:return "performance";case Dlss::UltraPerformance:return "ultra-performance";default:return "off";}}
// The file's text, every key with its value (LF lines), which Parse reads back to the same settings.
inline std::string Write(const Settings& s) {
    char rate[16]{};
    if(s.frame_rate<=0.f)std::snprintf(rate,sizeof(rate),"60");
    else if(s.frame_rate>=1000.f)std::snprintf(rate,sizeof(rate),"unlimited");
    else std::snprintf(rate,sizeof(rate),"%u",static_cast<unsigned>(s.frame_rate+0.5f));
    char sharpen[16]{};std::snprintf(sharpen,sizeof(sharpen),"%.9g",double(s.sharpen));
    std::string text="; Dark Souls III: Rekindled, its settings (I-0132 R12b). Written by the launcher and the Rekindled page.\n";
    const auto line=[&](const char* key,const std::string& value){text+=key;text+='=';text+=value;text+='\n';};
    line("level",Name(s.level));line("bounces",std::to_string(s.bounces));line("dlss",Name(s.dlss));line("rr",s.rr?"on":"off");
    line("reflex",Name(s.reflex));line("frame_generation",s.frame_generation>=2?"x"+std::to_string(s.frame_generation):std::string("off"));
    line("taa",s.taa?"on":"off");line("sharpen",sharpen);line("frame_rate",rate);line("vsync",s.vsync?"on":"off");
    line("borderless",s.borderless?"on":"off");line("vignette",s.vignette?"on":"off");
    line("hdr",s.hdr?"on":"off");line("hdr_peak",s.hdr_peak?std::to_string(s.hdr_peak):std::string("auto"));line("hdr_paper_white",std::to_string(s.hdr_paper_white));
    line("look",Name(s.look));line("aberration",s.aberration?"on":"off");line("bloom",std::to_string(s.bloom));line("shafts",s.shafts?"on":"off");
    line("grain",std::to_string(s.grain));line("fov",std::to_string(s.fov));
    return text;}
}
