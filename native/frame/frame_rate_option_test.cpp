// I-0114 tranche 3: the frame rate's parser (rekindled-settings.ini's frame_rate): whole numbers 20 to 1000, « unlimited », 60 as DS3's own (0), case, blanks, the
// byte-order marks, UTF-16, and the refusals (a second word, a sign, a decimal, out of range, empty).
#include "../world/frame_rate_option.h"
#include <cstdio>
#include <stdexcept>
#include <string>

using ds3native::frame_rate_option::Parse;
namespace {
void Require(bool v,const char* m){if(!v)throw std::runtime_error(m);}
bool Is(std::string_view text,float rate){const auto p=Parse(text);return p && *p==rate;}
bool Refused(std::string_view text){return !Parse(text);}
}
int main(){try{
    Require(Is("120",120.f) && Is("240",240.f) && Is("30",30.f) && Is("20",20.f) && Is("1000",1000.f) && Is("144",144.f),"I-0114 frame rate: whole numbers");
    Require(Is("60",0.f) && Is(" 60\r\n",0.f),"I-0114 frame rate: 60 is DS3's own");
    Require(Is("unlimited",1000.f) && Is("Unlimited\n",1000.f) && Is("UNLIMITED",1000.f),"I-0114 frame rate: unlimited");
    Require(Is("\xEF\xBB\xBF" "120\r\n",120.f),"I-0114 frame rate: UTF-8 mark");
    {const std::string utf16("\xFF\xFE" "1\0" "2\0" "0\0",8);Require(Is(utf16,120.f),"I-0114 frame rate: UTF-16");}
    Require(Refused("") && Refused("   ") && Refused("19") && Refused("1001") && Refused("-120") && Refused("120.5") && Refused("120 fps") &&
        Refused("fast") && Refused("99999999999"),"I-0114 frame rate: refusals");
    std::printf("PASS frame rate option: whole numbers 20-1000, unlimited, 60 as DS3's own, marks and UTF-16, refusals\n");
    return 0;
}catch(const std::exception& e){std::printf("FAIL %s\n",e.what());return 1;}}
