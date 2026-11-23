#include "game_quality_object.h"
#include <cstring>
#include <utility>

namespace ds3native::bootstrap {
bool WriteQualityObject(const uint8_t* table,const std::array<int32_t,12>& s,uint8_t* object) noexcept {
    if(!table || !object)return false;
    for(const auto level:s)if(level<0 || level>4)return false;
    const auto copy=[&](size_t from,size_t to,size_t bytes){std::memcpy(object+to,table+from,bytes);};
    copy(size_t(s[0])*8,0x00,8);                      // Texture: two ints (filter mode, anisotropy)
    copy(0x28+size_t(s[1]),0x08,1);                    // Antialiasing
    copy(size_t(s[2])*7+0x2D,0x09,4);                  // SSAO: dword, word, byte
    copy(size_t(s[2])*7+0x31,0x0D,2);
    copy(size_t(s[2])*7+0x33,0x0F,1);
    copy((size_t(s[3])+5)*16,0x10,16);                 // DepthOfField: four dwords
    copy(0xA0+size_t(s[4])*8,0x20,8);                  // MotionBlur
    copy(0xC8+size_t(s[5])*0x18,0x28,0x18);            // Shadow: six dwords
    copy(0x140+size_t(s[6])*8,0x40,8);                 // Lighting
    copy(0x168+size_t(s[7])*0x20,0x48,0x20);           // Effects: flags and seven floats
    copy(0x208+size_t(s[8]),0x68,1);                   // slot 8 (no file key): decal byte
    copy(0x210+size_t(s[9])*8,0x6C,8);                 // Reflection
    copy(0x238+size_t(s[10]),0x74,1);                  // WaterSurface
    copy((size_t(s[11])+0xBF)*3,0x75,2);               // Shade: word, byte
    copy((size_t(s[11])+0xBF)*3+2,0x77,1);
    return true;
}
OptionChangeClass ClassifyOptionChange(const std::array<int32_t,12>& b,const std::array<int32_t,12>& a,bool render_size_changed) noexcept {
    OptionChangeClass out;
    const auto enabled=[](int32_t level){return level>0;};
    for(size_t slot=0;slot<12;++slot){
        if(b[slot]==a[slot])continue;
        out.changed|=1u<<slot;uint8_t c{};
        switch(slot){
        case 0:c=OptionParameter;break;                                  // Texture: new sampler tables only
        case 1:c=OptionGate;break;                                       // Antialiasing: the AA filter pair
        case 2:c=OptionGate;                                             // SSAO: on from LOW, the temporal variants from HIGH
            if(enabled(b[2])!=enabled(a[2]) || (b[2]>=3)!=(a[2]>=3))c|=OptionHistory;break;
        case 3:c=OptionGate;break;                                       // DepthOfField: the draw count, maybe the layers
        case 4:c=OptionGate;if(!enabled(b[4]) && enabled(a[4]))c|=OptionHistory;break;   // MotionBlur: enabling restarts the history
        case 5:c=OptionRecreate;if(enabled(b[5])!=enabled(a[5]))c|=OptionGate;break;     // Shadow: the atlas size
        case 6:c=OptionParameter;break;                                  // Lighting: the fade float (+0x44/+0x45 unread)
        case 7:c=OptionParameter|OptionGate;break;                       // Effects: values, and flag bytes that gate families
        case 8:c=OptionParameter;break;                                  // the decal byte: fixed for the run
        case 9:c=OptionGate|OptionRecreate;break;                        // Reflection: the view and its target divisor
        case 10:c=OptionGate;break;                                      // WaterSurface: the 0x39 draw
        default:c=OptionGate;break;                                      // Shade: the SSS stage
        }
        out.classes|=c;
    }
    if(render_size_changed){out.changed|=1u<<12;out.classes|=OptionRecreate|OptionHistory;}
    return out;
}
void OptionClassLetters(uint8_t classes,char (&letters)[5]) noexcept {
    static constexpr std::pair<uint8_t,char> names[]{{OptionParameter,'P'},{OptionGate,'G'},{OptionRecreate,'R'},{OptionHistory,'H'}};
    size_t n{};for(const auto& [bit,letter]:names)if(classes&bit)letters[n++]=letter;
    letters[n]=0;
}
}
