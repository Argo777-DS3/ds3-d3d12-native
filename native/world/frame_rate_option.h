#pragma once
// I-0114 tranche 3 (the rate in the options, until the menus' redesign: ROADMAP's two sliders): the frame rate named in
// rekindled-settings.ini's frame_rate beside the module, written by the play launcher (-FrameRate). One word, whole, any case, surrounding blanks and a
// UTF-8 or UTF-16 byte-order mark ignored: a whole number of frames a second from 20 to 1000, or « unlimited » (a requested 1000,
// the frame its own bound, as unlock-unlimited.flag). 60 is DS3's own rate: 0, nothing written. Anything else: none (the caller
// keeps DS3's 60 and says so). Header-only: unit-tested alone (frame_rate_option_test).
#include <cctype>
#include <optional>
#include <string>
#include <string_view>

namespace ds3native::frame_rate_option {
inline std::optional<float> Parse(std::string_view bytes) noexcept {
    std::string word;bool seen_blank=false;
    for(const char c:bytes){const auto u=static_cast<unsigned char>(c);
        if(!u || u==0xEF || u==0xBB || u==0xBF || u==0xFF || u==0xFE)continue;   // UTF-16's zero bytes, the marks
        if(std::isspace(u)){if(!word.empty())seen_blank=true;continue;}
        if(seen_blank)return std::nullopt;   // one word, whole
        if(word.size()>=16)return std::nullopt;
        word.push_back(char(std::tolower(u)));}
    if(word=="unlimited")return 1000.f;
    if(word.empty())return std::nullopt;
    int rate=0;for(const char c:word){if(c<'0' || c>'9')return std::nullopt;rate=rate*10+(c-'0');if(rate>1000)return std::nullopt;}
    if(rate<20)return std::nullopt;
    return rate==60?0.f:float(rate);
}
}
