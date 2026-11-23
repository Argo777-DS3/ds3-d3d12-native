#pragma once
// I-0111 tranche 4 step 2 (final review L2): the DLSS mode named by rekindled-settings.ini's dlss (I-0132 R12b).
// One word, whole, any case, surrounding blanks and a UTF-8 or UTF-16 byte-order mark ignored (a UTF-16 file has its zero bytes
// dropped): dlaa, quality, balanced, performance, ultra-performance (or ultra_performance, ultraperformance). Anything else:
// none. I-0132 R11j (D-0265): « off » names no DLSS mode (the TAA then), as an absent key or an unrecognised word does. Header-only:
// unit-tested alone (dlss_mode_test).
#include <cctype>
#include <optional>
#include <string>
#include <string_view>

namespace ds3native::dlss_mode {
enum class Parsed {Dlaa,Quality,Balanced,Performance,UltraPerformance,Off};
inline std::optional<Parsed> Parse(std::string_view bytes) noexcept {
    std::string word;bool seen_blank=false;
    for(const char c:bytes){const auto u=static_cast<unsigned char>(c);
        if(!u || u==0xEF || u==0xBB || u==0xBF || u==0xFF || u==0xFE)continue;   // UTF-16's zero bytes, the marks
        if(std::isspace(u)){if(!word.empty())seen_blank=true;continue;}
        if(seen_blank)return std::nullopt;   // step review L-d: one word, whole: a second word refuses
        if(word.size()>=32)return std::nullopt;
        word.push_back(char(std::tolower(u)));}
    if(word=="off")return Parsed::Off;
    if(word=="dlaa")return Parsed::Dlaa;
    if(word=="quality")return Parsed::Quality;
    if(word=="balanced")return Parsed::Balanced;
    if(word=="performance")return Parsed::Performance;
    if(word=="ultra-performance" || word=="ultra_performance" || word=="ultraperformance")return Parsed::UltraPerformance;
    return std::nullopt;
}
}
