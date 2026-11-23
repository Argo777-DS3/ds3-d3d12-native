// I-0132 R12c (D-0271): the model of our rows in the game's Graphics tab and Advanced Settings page (graphics_rows.h): our text ids free
// of the game's, answered in English and French (our English in other languages), the Screen Mode values (Full screen, Borderless,
// Windowed), the frame-rate choices, and the RT rows over the settings file: D-0265's rules (an unavailable row greyed, showing what
// runs; Reflex's Off not offered with frame generation), Thomas's Anti-aliasing row and its DLSS mode row (2026-11-11), each row's
// choices and writes, the writes read back through the file.
#include "graphics_rows.h"
#include <cstdio>
#include <cwchar>
#include <stdexcept>

using namespace ds3native;
namespace {void Require(bool v,const char* m){if(!v)throw std::runtime_error(m);}}
int main(){try{
    namespace gr=graphics_rows;using settings::Settings;using gr::RtRow;
    Require(gr::Ours(gr::kBorderless) && gr::Ours(gr::kBorderless+gr::kHelp) && gr::Ours(gr::kRtLabel) && gr::Ours(gr::kRtWord+61) && !gr::Ours(720100) &&
        !gr::Ours(820100) && !gr::Ours(722000) && !gr::Ours(722001) && !gr::Ours(728999) && !gr::Ours(730000),"our ids are 729000-729999 and their helps, not the game's");
    Require(!gr::Ours(gr::kPresetLabel) && !gr::Ours(gr::kPresetCustomWord) && !gr::Ours(gr::kAdvancedLabel),"the preset's and the button's words are the game's");
    Require(!std::wcscmp(gr::Text(gr::kBorderless,gr::Language::French),L"Sans bordures") && !std::wcscmp(gr::Text(gr::kBorderless,gr::Language::English),L"Borderless"),
        "Borderless in both languages");
    Require(gr::LanguageOf(2)==gr::Language::French && gr::LanguageOf(8)==gr::Language::English && gr::LanguageOf(1)==gr::Language::English &&
        gr::LanguageOf(4)==gr::Language::Other && gr::LanguageOf(0)==gr::Language::Other,"fraFR French, engGB and engUS English, the others Other");
    // Every label, help line and choice word our rows use has words in each language, the help lines saying when they take effect.
    const auto has=[](int id){for(const auto l:{gr::Language::English,gr::Language::French,gr::Language::Other}){const wchar_t* t=gr::Text(id,l);if(!t || !*t)return false;}return true;};
    Require(has(gr::kVsync) && has(gr::kVsync+gr::kHelp) && has(gr::kFrameRate) && has(gr::kFrameRate+gr::kHelp),"V-sync and the frame rate, labels and help lines");
    for(uint8_t i=0;i<gr::kFrameRateCount;++i)Require(has(gr::kFrameRateChoice+i),"each frame-rate choice has words");
    const Settings defaults{};
    for(uint32_t i=0;i<gr::kRtRows;++i){
        Require(has(gr::kRtLabel+int(i)) && has(gr::kRtLabel+int(i)+gr::kHelp),"each RT row's label and help line");
        // I-0132 R12f: the settings live; no help line says the next launch any more.
        Require(!std::wcsstr(gr::Text(gr::kRtLabel+int(i)+gr::kHelp,gr::Language::English),L"next launch") &&
            !std::wcsstr(gr::Text(gr::kRtLabel+int(i)+gr::kHelp,gr::Language::French),L"prochain lancement"),"no RT help line says the next launch");
        // The help box held about 95 characters of an English line (the run r12g-pages3 cut the Anti-aliasing line there): 88 at most.
        Require(std::wcslen(gr::Text(gr::kRtLabel+int(i)+gr::kHelp,gr::Language::English))<=88 && std::wcslen(gr::Text(gr::kRtLabel+int(i)+gr::kHelp,gr::Language::French))<=88,
            "each RT help line fits the help box");
        for(const Settings& s:{defaults,settings::Parse("dlss=quality\nlevel=pt\nframe_generation=x2\n").settings,settings::Parse("dlss=dlaa\nlevel=pt\n").settings}){
            const auto c=gr::RtChoices(RtRow(i),s);Require(c.count>=2 && c.count<=gr::kMostChoices,"each RT row offers 2 to kMostChoices choices");
            for(uint32_t k=0;k<c.count;++k)Require(has(c.items[k].word),"each RT choice has words");
            Require(c.Has(gr::RtValue(RtRow(i),s)),"each RT row's value is among its choices");}}
    // The page's last row is always offered, as the game's own pages end: the vignette since I-0132 R12g (Thomas, 2026-11-11 15:19: « Mets
    // Vignette tout en bas »), after Reflex, both offered whatever the settings.
    // I-0138 tranche 7: the post-processing rows between Reflex and the vignette.
    Require(uint32_t(RtRow::Vignette)==gr::kRtRows-1 && uint32_t(RtRow::Look)==uint32_t(RtRow::Reflex)+1 && uint32_t(RtRow::Grain)==gr::kRtRows-3 && uint32_t(RtRow::Fov)==gr::kRtRows-2,
        "the vignette the last row, the post-processing rows between Reflex and it");
    {Settings s=defaults;
        Require(gr::RtValue(RtRow::Look,s)==0 && !gr::RtShown(RtRow::Aberration,s) && !gr::RtAvailable(RtRow::Aberration,s) && gr::SubRow(RtRow::Aberration),
            "the original look by default, the aberration's sub-row hidden with it");
        Require(gr::RtGoverns(RtRow::Look) && gr::RtWrite(RtRow::Look,1,s) && s.look==settings::Look::Rekindled && gr::RtShown(RtRow::Aberration,s) &&
            gr::RtValue(RtRow::Aberration,s)==1 && gr::RtWrite(RtRow::Aberration,0,s) && !s.aberration,"Rekindled shows its aberration, kept by default, off");
        Require(gr::RtValue(RtRow::Bloom,s)==100 && gr::RtWrite(RtRow::Bloom,30,s) && s.bloom==30 && gr::RtWrite(RtRow::Bloom,0,s) && s.bloom==0 &&
            !gr::RtWrite(RtRow::Bloom,35,s) && !gr::RtWrite(RtRow::Bloom,110,s),"the bloom in percent by tens, 100 % by default, down to none");
        Require(gr::RtValue(RtRow::Shafts,s)==1 && gr::RtWrite(RtRow::Shafts,0,s) && !s.shafts,"the light shafts on by default, off");
        Require(gr::RtValue(RtRow::Grain,s)==0 && gr::RtChoices(RtRow::Grain,s).items[0].word==gr::kWordOff && gr::RtWrite(RtRow::Grain,100,s) && s.grain==100,
            "no grain by default (its Off first), up to 100 %");
        s.bloom=45;Require(gr::RtValue(RtRow::Bloom,s)==40,"a file's bloom between two tens shown as the ten below");
        Require(gr::RtValue(RtRow::Fov,s)==100 && !gr::RtWrite(RtRow::Fov,70,s) && !gr::RtWrite(RtRow::Fov,140,s) && gr::RtWrite(RtRow::Fov,130,s) && s.fov==130 &&
            gr::RtWrite(RtRow::Fov,80,s) && s.fov==80,"the field of view: 100 % by default, 80 to 130 %");
        {Settings odd=s;odd.fov=125;Require(gr::RtValue(RtRow::Fov,odd)==120,"a file's field of view between two tens shown as the ten below");}
        for(const RtRow r:{RtRow::Sharpen,RtRow::Bloom,RtRow::Grain,RtRow::Fov}){const auto k=gr::RtSlider(r);Require(k && (k->max-k->min)%k->step==0,"the sliders' steps");
            for(int32_t v=k->min;v<=k->max;v+=k->step)Require(gr::RtChoices(r,s).Has(uint8_t(v)),"each slider step a value the row offers");}
        Require(!gr::RtSlider(RtRow::Look) && !gr::RtSlider(RtRow::Shafts) && !gr::RtSlider(RtRow::Vignette),"the On/Off rows stay choice rows");
        Require(!std::wcscmp(gr::Text(gr::kWordStrength,gr::Language::English),L"0 %") && !std::wcscmp(gr::Text(gr::kWordStrength+10,gr::Language::French),L"100 %") &&
            gr::kWordStrength>gr::kHdrPaperWhiteLabel && gr::kWordRekindled<gr::kWordStrength,"the strengths' words, apart from the HDR ids");
        const auto back=settings::Parse(settings::Write(s));Require(back.settings==s && back.refused.empty(),"tranche 7's writes read back through the file");}
    // I-0138 tranche 5b: the HDR rows of the Graphics tab (tranche 5's three choice rows of the Advanced page are gone with their
    // tests): the labels, help lines and the Brightness row's word in each language; the two sliders' steps, the peak's automatic
    // value one step under 400 nits, a file's value off the steps shown as the nearest step and kept until the slider moves.
    for(const int id:{gr::kHdrLabel,gr::kHdrPeakLabel,gr::kHdrPaperWhiteLabel})Require(gr::Ours(id) && has(id) && has(id+gr::kHelp) &&
        std::wcslen(gr::Text(id+gr::kHelp,gr::Language::English))<=88 && std::wcslen(gr::Text(id+gr::kHelp,gr::Language::French))<=88,"each HDR row's label and help line, within the help box");
    Require(has(gr::kWordHdrOff),"the Brightness row's word");   // tranche 5d: the greyed sliders' rows and their two ids are gone (the rows are hidden while the output is off)
    for(uint32_t i=0;i<gr::kRtRows;++i)Require(gr::kRtLabel+int(i)!=gr::kHdrLabel && gr::kRtWord+95==gr::kWordHdrOff && gr::kWordSharpen+10<gr::kWordHdrOff,"the HDR ids apart from the RT rows' ids and words");
    {Settings s=defaults;wchar_t text[24]{};
        Require(gr::HdrPeakSliderValue(s.hdr_peak)==gr::kHdrPeakAuto && gr::HdrPaperWhiteSliderValue(s.hdr_paper_white)==200,"the peak automatic and the paper white 200 nits by default");
        Require(!gr::HdrPeakWrite(gr::kHdrPeakAuto,s) && !gr::HdrPaperWhiteWrite(200,s) && s==defaults,"a slider that has not moved writes nothing");
        Require(gr::HdrPeakWrite(400,s) && s.hdr_peak==400 && gr::HdrPeakWrite(1000,s) && s.hdr_peak==1000 && gr::HdrPeakSliderValue(1000)==1000 &&
            gr::HdrPeakWrite(4000,s) && s.hdr_peak==4000 && gr::HdrPeakWrite(gr::kHdrPeakAuto,s) && s.hdr_peak==0,"the peak slider: 400 to 4000 nits, automatic at its least");
        Require(gr::HdrPaperWhiteWrite(80,s) && s.hdr_paper_white==80 && gr::HdrPaperWhiteWrite(500,s) && s.hdr_paper_white==500 && gr::HdrPaperWhiteWrite(210,s) && s.hdr_paper_white==210,
            "the paper white slider: the file's bounds by 10");
        // Every step of both sliders is a value the file takes and gives back; the big step (ten steps) stays on the steps.
        for(int32_t v=gr::kHdrPeakSlider.min;v<=gr::kHdrPeakSlider.max;v+=gr::kHdrPeakSlider.step){Settings t=defaults;t.hdr_peak=gr::HdrPeakOfSlider(v);
            Require((v==gr::kHdrPeakAuto?t.hdr_peak==0:t.hdr_peak==uint32_t(v)) && gr::HdrPeakSliderValue(t.hdr_peak)==v && settings::Parse(settings::Write(t)).settings==t,"each peak step through the file");}
        for(int32_t v=gr::kHdrPaperWhiteSlider.min;v<=gr::kHdrPaperWhiteSlider.max;v+=gr::kHdrPaperWhiteSlider.step){Settings t=defaults;t.hdr_paper_white=gr::HdrPaperWhiteOfSlider(v);
            Require(t.hdr_paper_white==uint32_t(v) && gr::HdrPaperWhiteSliderValue(t.hdr_paper_white)==v && settings::Parse(settings::Write(t)).settings==t,"each paper white step through the file");}
        // A file's value off the steps: shown as the nearest step, kept while the slider stays there, replaced once it moves.
        s.hdr_peak=1532;s.hdr_paper_white=263;
        Require(gr::HdrPeakSliderValue(1532)==1550 && gr::HdrPaperWhiteSliderValue(263)==260 && !gr::HdrPeakWrite(1550,s) && s.hdr_peak==1532 && !gr::HdrPaperWhiteWrite(260,s) && s.hdr_paper_white==263 &&
            gr::HdrPeakWrite(1500,s) && s.hdr_peak==1500 && gr::HdrPaperWhiteWrite(270,s) && s.hdr_paper_white==270,"a file's value off the steps");
        Require(gr::HdrPeakSliderValue(10000)==4000 && gr::HdrPeakSliderValue(401)==400 && gr::HdrPeakSliderValue(374)==400 && gr::HdrPeakOfSlider(399)==0 && gr::HdrPeakOfSlider(4050)==4000 &&
            gr::HdrPaperWhiteOfSlider(0)==80 && gr::HdrPaperWhiteOfSlider(900)==500,"values outside a slider are brought inside; a set peak is never shown as automatic");
        Require(!std::wcscmp(gr::HdrSliderText(true,1000,text),L"1000") && !std::wcscmp(gr::HdrSliderText(true,gr::kHdrPeakAuto,text),L"Auto") &&
            !std::wcscmp(gr::HdrSliderText(false,200,text),L"200") && !std::wcscmp(gr::HdrSliderText(false,350,text),L"350") && !std::wcscmp(gr::HdrSliderText(true,4000,text),L"4000"),
            "what a slider shows: the number, « Auto » for the peak's automatic value");
        for(const int id:{gr::kHdrPeakLabel,gr::kHdrPaperWhiteLabel})for(const auto l:{gr::Language::English,gr::Language::French})Require(std::wcsstr(gr::Text(id,l),L"(nits)"),"the sliders' labels carry the unit");
        for(int32_t v=gr::kHdrPeakSlider.min;v<=gr::kHdrPeakSlider.max;v+=gr::kHdrPeakSlider.step)Require(std::wcslen(gr::HdrSliderText(true,v,text))<=5,"each peak value within the slider's field (five letters)");}
    for(const char* file:{"","dlss=quality\nlevel=pt\nframe_generation=x2\n","taa=off\n","dlss=dlaa\nframe_generation=x4\nreflex=boost\n"})
        Require(gr::RtAvailable(RtRow::Vignette,settings::Parse(file).settings) && gr::RtAvailable(RtRow::Reflex,settings::Parse(file).settings),"the last rows offered whatever the settings");
    Require(!gr::Text(729999,gr::Language::English) && !gr::Text(gr::kBorderless+gr::kHelp,gr::Language::French),"no words for ids we do not use");
    Require(gr::ScreenModeValue(1,false)==gr::kFullScreen && gr::ScreenModeValue(1,true)==gr::kBorderlessValue && gr::ScreenModeValue(0,true)==gr::kWindowed &&
        gr::ScreenModeValue(0,false)==gr::kWindowed,"the Screen Mode value from the applied mode and the borderless switch");
    Require(gr::FrameRateValue(0.f)==1 && gr::FrameRateValue(60.f)==1 && gr::FrameRateValue(1000.f)==7 && gr::FrameRateValue(277.f)==gr::kFrameRateCustomValue &&
        gr::FrameRateOf(3)==120.f && !gr::FrameRateOf(gr::kFrameRateCustomValue),"the frame-rate choices, DS3's 60 and a custom rate");
    // D-0265 on the defaults (no DLSS mode, the TAA on): the Anti-aliasing row on TAA, the DLSS mode row absent, Reflex and the
    // sharpening open; the PT, RR and frame generation greyed, showing what runs, the bounces absent (Ray tracing, RR and frame generation off).
    Require(gr::RtValue(RtRow::AntiAliasing,defaults)==gr::kAaTaa && !gr::RtShown(RtRow::DlssMode,defaults) && gr::RtShown(RtRow::Sharpen,defaults),
        "without DLSS: TAA chosen, the DLSS mode row absent");
    Require(gr::RtAvailable(RtRow::AntiAliasing,defaults) && gr::RtAvailable(RtRow::Reflex,defaults) && gr::RtAvailable(RtRow::Sharpen,defaults),
        "without DLSS: the anti-aliasing, Reflex and the sharpening open");
    Require(!gr::RtAvailable(RtRow::Level,defaults) && !gr::RtAvailable(RtRow::Bounces,defaults) && !gr::RtAvailable(RtRow::Rr,defaults) &&
        !gr::RtAvailable(RtRow::FrameGeneration,defaults) && !gr::RtShown(RtRow::Bounces,defaults),"without DLSS: the PT, RR and frame generation greyed, the bounces absent");
    Require(gr::RtValue(RtRow::Level,defaults)==0 && gr::RtValue(RtRow::Rr,defaults)==0 && gr::RtValue(RtRow::FrameGeneration,defaults)==0,"a greyed row shows what runs");
    {Settings s=defaults;Require(!gr::RtWrite(RtRow::Level,1,s) && s==defaults,"a greyed row writes nothing");
        Require(gr::RtWrite(RtRow::AntiAliasing,gr::kAaDlss,s,settings::Dlss::Quality) && s.dlss==settings::Dlss::Quality,"DLSS chosen with the page's last mode");
        Require(gr::RtShown(RtRow::DlssMode,s) && gr::RtAvailable(RtRow::DlssMode,s) && gr::RtValue(RtRow::DlssMode,s)==2,"the DLSS mode row appears, on Quality");
        Require(gr::RtWrite(RtRow::DlssMode,4,s) && s.dlss==settings::Dlss::Performance && !gr::RtWrite(RtRow::DlssMode,1,s) && !gr::RtWrite(RtRow::DlssMode,0,s),
            "the DLSS mode Quality to Ultra Performance only (NVIDIA DLAA its own choice, Thomas 2026-11-11 05:29)");
        {Settings d=s;Require(gr::RtWrite(RtRow::AntiAliasing,gr::kAaDlaa,d) && d.dlss==settings::Dlss::Dlaa && gr::RtValue(RtRow::AntiAliasing,d)==gr::kAaDlaa &&
            !gr::RtShown(RtRow::DlssMode,d) && gr::RtAvailable(RtRow::Level,d) && gr::RtAvailable(RtRow::Sharpen,d),"NVIDIA DLAA: no mode row, the PT and the sharpening open");
            const auto dlaa=settings::Parse(settings::Write(d));Require(dlaa.settings==d && dlaa.refused.empty(),"NVIDIA DLAA read back through the file");
            Require(gr::RtWrite(RtRow::AntiAliasing,gr::kAaDlss,d,settings::Dlss::Balanced) && d.dlss==settings::Dlss::Balanced,"DLSS after DLAA takes the page's last mode");}
        Require(gr::RtAvailable(RtRow::Level,s) && gr::RtAvailable(RtRow::Rr,s) && gr::RtAvailable(RtRow::FrameGeneration,s) && gr::RtAvailable(RtRow::Sharpen,s),
            "with DLSS: the PT, RR, frame generation and the sharpening open (Thomas, 2026-11-11 01:05)");
        Require(gr::RtWrite(RtRow::Level,1,s) && s.level==settings::Level::Pt && gr::RtAvailable(RtRow::Bounces,s) && !gr::RtAvailable(RtRow::Rr,s) &&
            gr::RtValue(RtRow::Rr,s)==1,"at the PT level: the bounces open, RR greyed and shown on");
        Require(gr::RtShown(RtRow::Bounces,s) && !gr::RtShown(RtRow::Bounces,defaults),"the light bounces shown at the PT level only");
        Require(gr::RtWrite(RtRow::Bounces,3,s) && s.bounces==3 && !gr::RtWrite(RtRow::Bounces,5,s) && s.bounces==3,"the bounces 1 to 4 only");
        Require(gr::RtChoices(RtRow::Reflex,s).Has(0) && gr::RtWrite(RtRow::FrameGeneration,2,s) && s.frame_generation==2,"frame generation x2");
        Require(!gr::RtChoices(RtRow::Reflex,s).Has(0) && gr::RtValue(RtRow::Reflex,s)==1 && !gr::RtWrite(RtRow::Reflex,0,s) && gr::RtWrite(RtRow::Reflex,2,s) &&
            s.reflex==settings::Reflex::Boost,"with frame generation Reflex offers On and On + Boost, shown On while off");
        Require(gr::RtWrite(RtRow::AntiAliasing,gr::kAaDlss,s,settings::Dlss::Balanced) && s.dlss==settings::Dlss::Performance,"DLSS chosen again keeps the held mode");
        Require(gr::RtGoverns(RtRow::AntiAliasing) && gr::RtGoverns(RtRow::Level) && gr::RtGoverns(RtRow::FrameGeneration) && !gr::RtGoverns(RtRow::DlssMode) &&
            !gr::RtGoverns(RtRow::Sharpen) && !gr::RtGoverns(RtRow::Rr),"the rows whose writes change others' availability or presence");
        const auto back=settings::Parse(settings::Write(s));Require(back.settings==s && back.refused.empty(),"the rows' writes read back through the file");
        Require(gr::RtWrite(RtRow::AntiAliasing,gr::kAaTaa,s) && s.dlss==settings::Dlss::Off && s.taa && !gr::RtShown(RtRow::DlssMode,s),"TAA chosen: DLSS off, its mode row gone");}
    {Settings s=defaults;Require(gr::RtWrite(RtRow::Sharpen,5,s) && s.sharpen==0.5f && gr::RtValue(RtRow::Sharpen,s)==5 && gr::RtChoices(RtRow::Sharpen,s).count==11,
            "the sharpening in tenths, Off and 10 % to 100 % (Thomas, 2026-11-11 02:20)");
        Require(gr::RtWrite(RtRow::Sharpen,3,s) && settings::Parse(settings::Write(s)).settings.sharpen==s.sharpen && gr::RtValue(RtRow::Sharpen,settings::Parse(settings::Write(s)).settings)==3,
            "a tenth read back through the file");
        {Settings q=defaults;q.sharpen=0.25f;Require(gr::RtValue(RtRow::Sharpen,q)==3,"a quarter of an older file shown at its nearest tenth");}
        Require(gr::RtWrite(RtRow::AntiAliasing,gr::kAaOff,s) && !s.taa && s.dlss==settings::Dlss::Off && gr::RtValue(RtRow::AntiAliasing,s)==gr::kAaOff &&
            !gr::RtAvailable(RtRow::Sharpen,s) && gr::RtValue(RtRow::Sharpen,s)==0,"no anti-aliasing greys the sharpening, shown off");}
    {auto s=settings::Parse("dlss=quality\nframe_rate=277\n").settings;s.borderless=true;
        const auto back=settings::Parse(settings::Write(s));
        Require(back.settings==s && back.settings.borderless && back.settings.frame_rate==277.f && back.refused.empty(),"the borderless edit read back, the rest kept");}
    // I-0132 R12f: the sub-rows' left separator a quarter shorter from its left end, its right end and the label where they were.
    {const auto n=gr::RowCaption(false),s=gr::RowCaption(true);const auto close=[](double a,double b){return std::abs(a-b)<1e-9;};   // not « near » (windows.h)
        const double left_n=n.caption_x-gr::kSeparatorHalf*n.caption_xscale/100,right_n=n.caption_x+gr::kSeparatorHalf*n.caption_xscale/100;
        const double left_s=s.caption_x-gr::kSeparatorHalf*s.caption_xscale/100,right_s=s.caption_x+gr::kSeparatorHalf*s.caption_xscale/100;
        Require(close(right_s,right_n) && close(right_s-left_s,0.75*(right_n-left_n)),"the sub-row's line: its right end kept, three quarters of its length");
        Require(close(s.caption_x+s.text_x*s.caption_xscale/100,n.caption_x+n.text_x) && close(s.caption_xscale*s.text_xscale/100,100),"the sub-row's label at its place and size");
        Require(gr::SubRow(gr::RtRow::DlssMode) && gr::SubRow(gr::RtRow::Bounces) && !gr::SubRow(gr::RtRow::AntiAliasing) && !gr::SubRow(gr::RtRow::Level),"the « └ » rows");
        // I-0138 tranche 5d: the same on the options film's places (the Graphics tab's HDR sliders).
        const auto on=gr::RowCaption(false,gr::kCaptionX,gr::kOptionsCaptionTextX),os=gr::RowCaption(true,gr::kCaptionX,gr::kOptionsCaptionTextX);
        Require(close(os.caption_x+gr::kSeparatorHalf*os.caption_xscale/100,on.caption_x+gr::kSeparatorHalf) && close(os.caption_xscale,75) &&
            close(os.caption_x+os.text_x*os.caption_xscale/100,on.caption_x+on.text_x) && close(os.caption_xscale*os.text_xscale/100,100),"the options film's sub-row: the line's right end, three quarters, the label at its place and size");}
    // I-0132 R12g: the new rows (Thomas, 2026-11-11 06:25 to 06:34).
    {Settings s=defaults;Require(gr::RtAvailable(RtRow::Vignette,s) && gr::RtValue(RtRow::Vignette,s)==1 && gr::RtWrite(RtRow::Vignette,0,s) && !s.vignette,"the vignette offered, on by default, off");
        s.dlss=settings::Dlss::Dlaa;s.level=settings::Level::Pt;
        const auto back=settings::Parse(settings::Write(s));Require(back.settings==s && back.refused.empty(),"the new rows' writes read back through the file");}
    // I-0132 R12g: our scroll over the Advanced page's rows (fourteen slots).
    {bool on[20];for(bool& b:on)b=true;
        Require(!gr::ScrollAfterMove(on,14,0,13,0),"fourteen rows: the game's own wrap");
        Require(!gr::ScrollAfterMove(on,16,0,4,5) && !gr::ScrollAfterMove(on,16,0,5,4),"a move inside the window stands");
        auto m=gr::ScrollAfterMove(on,16,0,13,0);Require(m && m->first==1 && m->cursor==13,"down past the window: one row further, the cursor on the last slot");
        m=gr::ScrollAfterMove(on,16,2,13,0);Require(m && m->first==0 && m->cursor==0,"down past the list's last row: back to its first");
        m=gr::ScrollAfterMove(on,16,2,0,13);Require(m && m->first==1 && m->cursor==0,"up past the window: one row back, the cursor on the first slot");
        m=gr::ScrollAfterMove(on,16,0,0,13);Require(m && m->first==2 && m->cursor==13,"up past the list's first row: to its last");
        on[14]=false;m=gr::ScrollAfterMove(on,16,0,13,0);Require(m && m->first==2 && m->cursor==13,"a greyed row below skipped: the next selectable one shown");
        on[14]=true;on[13]=false;m=gr::ScrollAfterMove(on,16,0,12,0);Require(m && m->first==1 && m->cursor==13,"the window's last row greyed: the wrap from its last selectable one");
        Require(!gr::ScrollAfterMove(on,16,0,12,3),"a jump that is no wrap stands (a click)");
        // The mouse wheel (Thomas, 2026-11-17): the window moved by the notches, clamped; the cursor kept on its row, else the nearest selectable.
        {bool w[20];for(bool& b:w)b=true;
            Require(!gr::ScrollByWheel(w,14,0,3,1) && !gr::ScrollByWheel(w,20,0,3,0) && !gr::ScrollByWheel(w,20,0,3,-1) && !gr::ScrollByWheel(w,20,6,3,2),"the wheel: no move at the list's ends or with fourteen rows");
            auto k=gr::ScrollByWheel(w,20,0,5,1);Require(k && k->first==1 && k->cursor==4,"a notch down: one row further, the cursor on its row");
            k=gr::ScrollByWheel(w,20,0,0,3);Require(k && k->first==3 && k->cursor==0,"the cursor's row left above: the window's first row");
            k=gr::ScrollByWheel(w,20,6,13,-4);Require(k && k->first==2 && k->cursor==13,"up: the cursor's row left below, the window's last row");
            k=gr::ScrollByWheel(w,20,4,0,-9);Require(k && k->first==0 && k->cursor==4,"up past the top: clamped, the cursor on its row");
            w[3]=false;k=gr::ScrollByWheel(w,20,0,0,3);Require(k && k->first==3 && k->cursor==1,"a greyed row skipped: the nearest selectable");
            // The step review's L1: a first row beyond the list's end, a negative cursor (no row focused), every row greyed.
            k=gr::ScrollByWheel(w,20,40,5,-1);Require(k && k->first==5 && k->cursor==6,"a first row beyond the end clamped before the notch (to 6, the cursor kept on row 11)");
            k=gr::ScrollByWheel(w,20,4,-1,-2);Require(k && k->first==2 && k->cursor==2,"no focused row: the old window's first row kept (row 4)");
            bool none[20]{};k=gr::ScrollByWheel(none,20,0,4,2);Require(k && k->first==2 && k->cursor==0,"every row greyed: the window still moves, the cursor at its top");}
        Require(gr::FirstRowClamped(5,16)==2 && gr::FirstRowClamped(5,10)==0,"the window's first row clamped");
        // The step review's M1: 16 rows scrolled to the bottom (first 2), the cursor on row 9; two rows below it go (14 rows): the window
        // clamped to 0 keeps row 9 (slot 9, not slot 7); a row kept past the window moves it.
        Require(gr::FirstRowKeeping(2,14,9)==0 && 9-int(gr::FirstRowKeeping(2,14,9))==9 && gr::FirstRowKeeping(0,20,17)==4 && gr::FirstRowKeeping(5,20,3)==3,
            "a rebuild keeps the cursor's row of the list in the window");
        // The scroll bar's thumb as the game places it: at the top a window from the list's first row, at the bottom from its last
        // window, its length 14 rows of the total.
        const auto top=gr::ScrollThumb(0,16,10.f,684.f),bottom=gr::ScrollThumb(2,16,10.f,684.f);
        Require(top.y==10 && bottom.y==10+int(2.f/16.f*684.f) && top.yscale==14.f/16.f && bottom.y+int(bottom.yscale*684.f)<=10+684 && bottom.y+int(bottom.yscale*684.f)>=10+683,
            "the thumb from the top to the bottom of the track, 14 rows of 16 long");
        Require(gr::ScrollThumb(0,14,10.f,684.f).yscale==1.f && gr::ScrollThumb(0,0,10.f,684.f).y==10 && gr::ScrollThumb(0,0,10.f,684.f).yscale==1.f,
            "fourteen rows or none: the whole track");}
    std::puts("graphics_rows_test passed");return 0;}
catch(const std::exception& e){std::fprintf(stderr,"graphics_rows_test FAILED: %s\n",e.what());return 1;}}
