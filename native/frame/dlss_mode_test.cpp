// I-0111 tranche 4 step 2 (final review L2): the DLSS words' parser (rekindled-settings.ini's dlss): the five words, case, blanks, the byte-order marks, UTF-16,
// and what is refused (a sentence, an unknown word, a long word, an empty file).
#include "../renderer/dlss_mode.h"
#include <cstdio>
#include <stdexcept>
#include <string>

using namespace ds3native::dlss_mode;
namespace {void Require(bool v,const char* m){if(!v)throw std::runtime_error(m);}}
int main(){try{
    Require(Parse("dlaa")==Parsed::Dlaa && Parse("Quality")==Parsed::Quality && Parse("BALANCED")==Parsed::Balanced &&
        Parse("performance")==Parsed::Performance && Parse("ultra-performance")==Parsed::UltraPerformance && Parse("Ultra_Performance")==Parsed::UltraPerformance,"the five words");
    Require(Parse("off")==Parsed::Off && Parse(" OFF\r\n")==Parsed::Off && !Parse("off quality"),"off (I-0132 R11j)");
    Require(Parse("  quality\r\n")==Parsed::Quality && Parse("\xEF\xBB\xBFquality")==Parsed::Quality,"blanks and the UTF-8 mark");
    const std::string utf16("\xFF\xFEq\0u\0a\0l\0i\0t\0y\0\r\0\n\0",20);
    Require(Parse(utf16)==Parsed::Quality,"UTF-16 with its mark");
    Require(!Parse("") && !Parse("   ") && !Parse("dlaa, not quality") && !Parse("quality please") && !Parse("dlaa mode") && Parse("ultraperformance")==Parsed::UltraPerformance && !Parse("ultra") && !Parse(std::string(64,'q')),"refused");
    std::printf("PASS dlss mode: the five words and off, case, blanks, the UTF-8 and UTF-16 marks, a sentence, an unknown or long word and an empty file refused\n");
    return 0;
}catch(const std::exception& e){std::printf("FAIL %s\n",e.what());return 1;}}
