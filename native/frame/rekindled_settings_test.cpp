// I-0132 R12b: rekindled-settings.ini's model: the defaults (D-0265, D-0266), the file's grammar (case, blanks, comments, sections,
// the UTF-8 mark, CRLF, a decimal comma, a key named twice), what is refused (each entry keeps its default and is named), the
// written text read back to the same settings, and D-0265's rules (Available, Effective).
#include "../renderer/rekindled_settings.h"
#include <cstdio>
#include <stdexcept>
#include <string>

using namespace ds3native::settings;
namespace {void Require(bool v,const char* m){if(!v)throw std::runtime_error(m);}}
int main(){try{
    const Settings d;
    Require(d.level==Level::Rt && d.bounces==2 && d.dlss==Dlss::Off && d.rr && d.reflex==Reflex::Off && d.frame_generation==0 && d.taa &&
        d.sharpen==0.f && d.frame_rate==0.f && d.vsync && !d.borderless && d.vignette,"the defaults (D-0265, D-0266; R12g: the vignette on)");
    {const auto r=Parse("");Require(r.settings==d && r.accepted.empty() && r.unknown==0 && r.refused.empty(),"an empty file: the defaults");}
    {const auto r=Parse("\xEF\xBB\xBF; a comment\r\n# another\r\n[rekindled]\r\n  Level = PT \r\nbounces=3\r\nDLSS=Ultra-Performance\r\nrr=off\r\n"
        "reflex=Boost\r\nframe_generation=x3\r\ntaa=off\r\nsharpen=0,5\r\nframe_rate=120\r\nvsync=off\r\nborderless=on");
        const auto& s=r.settings;
        Require(s.level==Level::Pt && s.bounces==3 && s.dlss==Dlss::UltraPerformance && !s.rr && s.reflex==Reflex::Boost && s.frame_generation==3 &&
            !s.taa && s.sharpen==0.5f && s.frame_rate==120.f && !s.vsync && s.borderless && r.accepted.size()==11 && r.unknown==0 && r.refused.empty(),
            "every key, any case, blanks, comments, a section, the UTF-8 mark, CRLF, a decimal comma, no final line end");}
    {const auto r=Parse("dlss=quality\ndlss=balanced\nframe_rate=60\nframe_generation=on\nrr=1\ntaa=no\nbounces=9\n");
        Require(r.settings.dlss==Dlss::Balanced && r.settings.frame_rate==0.f && r.settings.frame_generation==2 && r.settings.rr && !r.settings.taa &&
            r.settings.bounces==kBouncesMost && r.accepted.size()==7,"a key named twice keeps its last value; 60 is DS3's own; on is x2; the bounces at most kBouncesMost");
        Require(r.Accepted("dlss") && r.Accepted("bounces") && !r.Accepted("level") && !r.Refused("dlss"),"the accepted keys named");}
    {const auto r=Parse("frame_rate=unlimited\nsharpen=4\nsharpen=0.0001\n");
        Require(r.settings.frame_rate==1000.f && r.settings.sharpen==0.f && r.accepted.size()==3,"unlimited is 1000; the sharpening clamped, below 1e-3 none");
        Require(Parse("sharpen=4").settings.sharpen==1.f,"the sharpening at most 1");}
    {const auto r=Parse("level=path\nbounces=0\nbounces=two\ndlss=ultra\nrr=maybe\nreflex=max\nframe_generation=x5\ntaa=\nsharpen=-1\nsharpen=0.5x\n"
        "frame_rate=10\nvsync=sometimes\nborderless=2\ncolour=blue\nno equals sign\n");
        Require(r.settings==d,"every refused value keeps its default");
        Require(r.refused.size()==13 && r.refused[0]=="level" && r.refused[1]=="bounces" && r.refused[12]=="borderless" && r.unknown==2 && r.accepted.empty(),
            "the refused keys named in order, the unknown lines counted");
        Require(r.refused_values.size()==13 && r.RefusedValue("level")=="path" && r.RefusedValue("bounces")=="two" && r.RefusedValue("sharpen")=="0.5x" &&
            r.RefusedValue("taa").empty() && r.RefusedValue("colour").empty(),"the refused values kept as written, the last one of a key");}
    // I-0113 tranche 40: Firelink's loading walls forced off (Thomas): tranche 38's key is an unknown line, dropped by Write.
    {const auto r=Parse("firelink_wall=on\n");Require(r.settings==d && r.unknown==1 && r.accepted.empty() && r.refused.empty() &&
        Write(r.settings).find("firelink")==std::string::npos,"a former firelink_wall key ignored");}
    // I-0132 R12g: the vignette; ROADMAP workstream 2 (D-0273): R12g's gi_paths key, its row removed, an unknown line dropped by Write.
    {const auto r=Parse("vignette=off\n");Require(!r.settings.vignette && r.accepted.size()==1,"the vignette read");
        const auto bad=Parse("vignette=maybe\n");
        Require(bad.settings==d && bad.refused.size()==1 && bad.RefusedValue("vignette")=="maybe","a vignette word refused");
        const auto gi=Parse("gi_paths=4\n");Require(gi.unknown==1 && gi.refused.empty() && gi.settings==d && Write(gi.settings).find("gi_paths")==std::string::npos,
            "a former gi_paths key ignored");
        // The second step review's L5: the anisotropy key a build of 5a4e02501 wrote (the row withdrawn) is an unknown line, dropped by Write.
        const auto old=Parse("anisotropy=x16");Require(old.unknown==1 && old.refused.empty() && old.settings==d && Write(old.settings).find("anisotropy")==std::string::npos,
            "a former anisotropy key ignored");}
    // I-0138 tranche 5: the HDR output's keys.
    {Require(!d.hdr && d.hdr_peak==0 && d.hdr_paper_white==200,"the HDR defaults: off, the screen's own peak, a paper white of 200 nits");
        const auto r=Parse("hdr=on\nhdr_peak=1000\nhdr_paper_white=150\n");
        Require(r.settings.hdr && r.settings.hdr_peak==1000 && r.settings.hdr_paper_white==150 && r.accepted.size()==3 && r.refused.empty(),"the HDR keys read");
        Require(Parse("hdr_peak=auto\n").settings.hdr_peak==0 && Parse("hdr_peak=1000\nhdr_peak=0\n").settings.hdr_peak==0 && Write(d).find("hdr_peak=auto\n")!=std::string::npos,"the automatic peak read and written");
        for(const char* bad:{"hdr=maybe\n","hdr_peak=399\n","hdr_peak=10001\n","hdr_peak=bright\n","hdr_peak=\n","hdr_paper_white=79\n","hdr_paper_white=501\n","hdr_paper_white=auto\n","hdr_paper_white=200nits\n"}){
            const auto b=Parse(bad);Require(b.settings==d && b.refused.size()==1 && b.accepted.empty(),"an HDR value out of its bounds refused");}
        Require(Parse("hdr_peak=400\n").settings.hdr_peak==400 && Parse("hdr_peak=10000\n").settings.hdr_peak==10000 && Parse("hdr_paper_white=80\n").settings.hdr_paper_white==80 &&
            Parse("hdr_paper_white=500\n").settings.hdr_paper_white==500,"the HDR bounds themselves accepted");}
    // I-0138 tranche 7: the colorimetry, the Rekindled look's aberration, the bloom's and the grain's strengths, the light shafts.
    {Require(d.look==Look::Original && d.aberration && d.bloom==100 && d.shafts && d.grain==0,"tranche 7's defaults: the original look, the aberration kept, the bloom whole, the shafts on, no grain");
        const auto r=Parse("look=Rekindled\naberration=off\nbloom=0\nshafts=off\ngrain=100\n");
        Require(r.settings.look==Look::Rekindled && !r.settings.aberration && r.settings.bloom==0 && !r.settings.shafts && r.settings.grain==100 &&
            r.accepted.size()==5 && r.refused.empty(),"tranche 7's keys read");
        for(const char* bad:{"look=vivid\n","aberration=some\n","bloom=101\n","bloom=-1\n","bloom=50%\n","bloom=\n","grain=1000\n","shafts=2\n","fov=79\n","fov=131\n","fov=\n"}){
            const auto b=Parse(bad);Require(b.settings==d && b.refused.size()==1 && b.accepted.empty(),"a tranche 7 value refused");}
        Require(d.fov==100 && Parse("fov=80\n").settings.fov==80 && Parse("fov=130\n").settings.fov==130,"the field of view: the game's by default, its two bounds accepted");}
    {const std::string le("\xFF\xFE" "d\0l\0s\0s\0=\0q\0u\0a\0l\0i\0t\0y\0\r\0\n\0r\0r\0=\0o\0f\0f\0",42);
        std::string be("\xFE\xFF",2);for(size_t i=2;i<le.size();i+=2){be.push_back(le[i+1]);be.push_back(le[i]);}
        for(const auto& text:{le,be}){const auto r=Parse(text);
            Require(r.settings.dlss==Dlss::Quality && !r.settings.rr && r.accepted.size()==2 && r.unknown==0,"a UTF-16 file (either order) read as its text");}}
    {const auto r=Parse("dlss=quality\nDLSS = Bogus\n");   // the step review's L5: a refusal keeps the earlier accepted value
        Require(r.settings.dlss==Dlss::Quality && r.Accepted("dlss") && r.Refused("dlss") && r.RefusedValue("dlss")=="Bogus","a refused value after an accepted one keeps the accepted one");}
    {Settings s;s.level=Level::Pt;s.bounces=4;s.dlss=Dlss::Dlaa;s.rr=false;s.reflex=Reflex::On;s.frame_generation=4;s.taa=false;s.sharpen=0.3333f;
        s.frame_rate=240.f;s.vsync=false;s.borderless=true;s.vignette=false;
        s.hdr=true;s.hdr_peak=1500;s.hdr_paper_white=250;   // I-0138 tranche 5: three more keys (fifteen written)
        s.look=Look::Rekindled;s.aberration=false;s.bloom=40;s.shafts=false;s.grain=30;s.fov=120;   // I-0138 tranche 7: six more (twenty-one)
        Require(Parse(Write(s)).settings==s && Parse(Write(s)).accepted.size()==21 && Parse(Write(d)).settings==d,"the written text read back to the same settings");
        s.frame_rate=1000.f;Require(Parse(Write(s)).settings.frame_rate==1000.f && Write(s).find("frame_rate=unlimited\n")!=std::string::npos,"unlimited written");
        Require(Write(d).find("frame_rate=60\n")!=std::string::npos && Write(d).find("dlss=off\n")!=std::string::npos,"DS3's 60 and no DLSS mode written");}
    // D-0265: without a DLSS mode the PT, RR and frame generation are greyed (and do not run), the TAA and its sharpening offered.
    {Settings s;s.level=Level::Pt;s.frame_generation=2;s.sharpen=0.5f;
        const auto a=Available(s);Require(!a.pt && !a.rr && !a.frame_generation && a.reflex_off && a.taa && a.sharpen && !a.bounces,"no DLSS mode: what is greyed");
        const auto e=Effective(s);Require(e.level==Level::Rt && !e.rr && e.frame_generation==0 && e.reflex==Reflex::Off && e.taa && e.sharpen==0.5f,"no DLSS mode: what runs");
        s.taa=false;Require(!Available(s).sharpen && Effective(s).sharpen==0.f,"no anti-aliasing: no sharpening");}
    // With a DLSS mode: the PT, RR and frame generation offered; the TAA greyed; at the PT RR locked On; with frame generation Reflex Off locked.
    {Settings s;s.dlss=Dlss::Quality;s.rr=false;
        auto a=Available(s);Require(a.pt && a.rr && a.frame_generation && a.reflex_off && !a.taa && a.sharpen && !a.bounces,"a DLSS mode at the RT level (the sharpening kept, Thomas 2026-11-11)");
        s.sharpen=0.5f;auto e=Effective(s);Require(e.level==Level::Rt && !e.rr && !e.taa && e.sharpen==0.5f,"RR Off at the RT level runs Off; the TAA does not run; the sharpening runs after DLSS");s.sharpen=0;
        s.level=Level::Pt;a=Available(s);e=Effective(s);Require(a.pt && !a.rr && a.bounces && e.level==Level::Pt && e.rr,"the PT level: RR locked On, the bounces offered");
        s.frame_generation=3;a=Available(s);e=Effective(s);Require(!a.reflex_off && e.reflex==Reflex::On && e.frame_generation==3,"frame generation: Reflex Off locked, run as On");
        s.reflex=Reflex::Boost;Require(Effective(s).reflex==Reflex::Boost,"frame generation keeps Boost");}
    // I-0132 R12c bis (agreed with the R pilot): a file read, edited as the page and the Graphics rows edit it, written and read again.
    {auto s=Parse("dlss=balanced\nframe_rate=277\nsharpen=0.25\n").settings;
        s.borderless=true;s.vsync=false;s.frame_rate=144.f;s.level=Level::Pt;s.reflex=Reflex::On;
        const auto r=Parse(Write(s));
        Require(r.settings==s && r.refused.empty() && r.unknown==0 && r.accepted.size()==21 && r.settings.dlss==Dlss::Balanced && r.settings.sharpen==0.25f,   // fifteen keys since I-0138 tranche 5 (the three HDR keys), twenty-one since tranche 7
            "an edited file read again: the edits and the untouched keys kept");}
    std::printf("PASS rekindled settings: the defaults, the file's grammar, the refused values named, the written text read back, D-0265's rules, an edit read again\n");
    return 0;
}catch(const std::exception& e){std::printf("FAIL %s\n",e.what());return 1;}}
