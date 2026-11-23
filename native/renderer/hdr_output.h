// I-0138 tranche 5 (ROADMAP workstream 11): the HDR10 output's numbers, without D3D: what the frame asks for, what the screen
// says, the ST 2084 curve and the Rec. 709 to Rec. 2020 matrix as the output programme (shaders/post_output.hlsl) applies them,
// the parameter block of that programme and its model. The published standards' constants (SMPTE ST 2084, ITU-R BT.2087),
// written here from their definitions.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ds3native::hdr {
// Off: the 8-bit SDR chain, as ever. Auto: HDR10 where Windows' HDR is on for the window's screen. Force (a test tool): HDR10 on
// any screen (the compositor then converts the picture for an SDR screen).
enum class Mode:uint8_t {Off,Auto,Force};
struct Request {
    Mode mode{Mode::Off};
    float paper_white{200.0f};   // nits of the SDR white (the UI, a diffuse white)
    float peak{};                // nits of the brightest value written; 0: the screen's own maximum
};
// IDXGIOutput6::GetDesc1 of the window's screen (FramePresenter's probe). hdr: its colour space is ST 2084 over Rec. 2020,
// which is how DXGI says that Windows' HDR is on for it.
struct Display {
    bool found{},hdr{};
    float max_nits{},max_full_frame_nits{},min_nits{};
    uint32_t color_space{},bits{};
};
// The peak the output clamps to: the asked one, else the screen's when it is above the paper white, else 1,000 nits (a forced
// HDR10 chain on an SDR screen, whose own maximum is its SDR white); never under the paper white.
inline float Peak(const Request& request,const Display& display) noexcept {
    const float paper=(std::max)(request.paper_white,1.0f);
    const float peak=request.peak>0?request.peak:display.max_nits>paper?display.max_nits:1000.0f;
    return (std::min)((std::max)(peak,paper),10000.0f);
}
// SMPTE ST 2084: the signal (0 to 1) of a luminance in nits (0 to 10,000), and back.
inline constexpr double kPqM1=2610.0/16384.0,kPqM2=2523.0/4096.0*128.0,kPqC1=3424.0/4096.0,kPqC2=2413.0/4096.0*32.0,kPqC3=2392.0/4096.0*32.0;
inline double PqEncode(double nits) noexcept {
    const double y=(std::min)((std::max)(nits/10000.0,0.0),1.0),p=std::pow(y,kPqM1);
    return std::pow((kPqC1+kPqC2*p)/(1.0+kPqC3*p),kPqM2);
}
inline double PqDecode(double signal) noexcept {
    const double e=std::pow((std::min)((std::max)(signal,0.0),1.0),1.0/kPqM2),n=(std::max)(e-kPqC1,0.0);
    return 10000.0*std::pow(n/(kPqC2-kPqC3*e),1.0/kPqM1);
}
// ITU-R BT.2087-0: linear Rec. 709 to linear Rec. 2020 (rows; each sums to 1, so a white stays white).
inline constexpr float kRec709ToRec2020[3][3]{{0.6274f,0.3293f,0.0433f},{0.0691f,0.9195f,0.0114f},{0.0164f,0.0880f,0.8956f}};
// The decode of a display-encoded picture to linear light: the power 2.2 of the SDR screen the game's picture is made for
// (the screen's part, not the record's gamma, which the tone map applies itself and which is 2.2 in every area); not sRGB's
// piecewise curve, which would lift the shadows.
inline constexpr float kSdrGamma=2.2f;
// The highlights of the original colorimetry in HDR (shaders/post_tone.hlsl, ps_hdr): our tone map's SDR value v (0 to 1, the
// largest channel of the curve's output with its glare, before the colour matrix) is kept up to the knee and, above it, replaced by
// knee + (1 - knee) ln((1 - knee) / (1 - v)): the continuation, at the knee's slope, of a curve whose shoulder closes on 1 as
// an exponential of the scene's luminance (the sensitometric curve's does), so a highlight keeps rising with the scene instead
// of closing on the paper white. The colour takes the gain (value over v) on its three channels: its hue and the picture
// under the knee are the SDR picture's. The knee is the tone map's (post_params.h, kPostToneHdrKnee).
inline double Highlight(double v,double knee) noexcept {
    return v<=knee?v:knee+(1.0-knee)*std::log((1.0-knee)/(std::max)(1.0-v,1e-7));
}
// The roll-off of the output (the programme's Encode): linear values, 1 the paper white, top = peak / paper white. Nothing
// changes up to max(1, top / 2); above, the largest channel closes on the top as 1 - exp.
inline double RollOff(double m,double top) noexcept {
    const double start=(std::max)(1.0,0.5*top);
    return m>start && top>start?start+(top-start)*(1.0-std::exp(-(m-start)/(top-start))):m;
}
// b0 of the output programme (shaders/post_output.hlsl, the same layout).
enum OutputFlags:uint32_t {kOutputDither=1u};
struct OutputParams {
    float rec2020[3][4]{};   // kRec709ToRec2020's rows (.w unused)
    float paper_white{};     // nits
    float peak{};            // nits
    float sdr_gamma{};       // kSdrGamma
    uint32_t flags{};        // OutputFlags
    int32_t noise_offset[2]{};   // the blue noise's place this frame (texels)
    float pad[2]{};              // I-0138 tranche 5b: the game's brightness option no longer reaches this chain (one power)
};
static_assert(sizeof(OutputParams)==80);
inline OutputParams BuildOutputParams(float paper_white,float peak) noexcept {
    OutputParams p;
    for(int r=0;r<3;++r)for(int c=0;c<3;++c)p.rec2020[r][c]=kRec709ToRec2020[r][c];
    p.paper_white=paper_white;p.peak=peak;p.sdr_gamma=kSdrGamma;return p;
}
// What the programme writes, without its dither, for one display-encoded texel (the SDR picture's 0 to 1, or the scene's
// values above 1), as 10-bit codes: the reference of hdr_output_test.
inline void EncodedToHdr10(const float encoded[3],const OutputParams& p,double codes[3]) noexcept {
    double linear[3];for(int c=0;c<3;++c){const double e=(std::max)(double(encoded[c]),0.0);
        linear[c]=std::pow(e,double(p.sdr_gamma));}
    const double m=(std::max)(linear[0],(std::max)(linear[1],linear[2])),rolled=RollOff(m,double(p.peak)/double(p.paper_white));
    if(rolled!=m)for(double& value:linear)value*=rolled/m;
    for(int r=0;r<3;++r){
        const double v=double(p.rec2020[r][0])*linear[0]+double(p.rec2020[r][1])*linear[1]+double(p.rec2020[r][2])*linear[2];
        codes[r]=1023.0*PqEncode((std::min)(v*double(p.paper_white),double(p.peak)));}
}
}
