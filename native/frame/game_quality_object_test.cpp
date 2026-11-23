#include "game_quality_object.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace ds3native;
namespace {
void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::vector<uint8_t> Hex(const char* text){std::vector<uint8_t> bytes;std::istringstream in(text);std::string cell;
    while(in>>cell)bytes.push_back(static_cast<uint8_t>(std::stoul(cell,nullptr,16)));return bytes;}
}
// The oracle is I-0067 Build-QualityObject.ps1 over quality-table.tsv (QUALITY-OBJECT-NATIVE section 3 examples).
int main(int argc,char** argv){try{
    Require(argc==2,"usage: game_quality_object_test quality-table.tsv");
    std::vector<uint8_t> table(bootstrap::QualityTableBytes);std::ifstream file(argv[1]);std::string line;size_t rows{};
    while(std::getline(file,line)){const auto tab=line.find('\t');if(tab==std::string::npos)continue;
        const auto offset=std::stoul(line.substr(0,tab),nullptr,16);const auto bytes=Hex(line.c_str()+tab+1);
        Require(offset+bytes.size()<=table.size(),"table row past 0x24C");std::copy(bytes.begin(),bytes.end(),table.begin()+offset);++rows;}
    Require(rows==37,"quality table rows");
    std::vector<uint8_t> object(bootstrap::QualityObjectBytes,0);
    Require(bootstrap::WriteQualityObject(table.data(),{2,3,3,3,3,3,3,3,3,3,3,3},object.data()),"initial levels refused");
    const auto initial=Hex("03 00 00 00 04 00 00 00 01 01 01 01 01 00 00 00 01 00 00 00 03 00 00 00 05 00 00 00 05 00 00 00 "
        "01 01 00 00 05 00 00 00 01 00 00 00 01 00 00 00 01 00 00 00 80 00 00 00 00 10 00 00 00 00 00 00 "
        "00 00 80 3f 01 01 00 00 01 01 01 00 00 00 80 3f 00 00 80 3f 00 00 80 3f 00 00 80 3f 00 00 80 3f "
        "00 00 80 3f 00 00 80 3f 01 00 00 00 01 01 00 00 02 00 00 00 01 01 01 00");
    Require(object==initial,"initial object bytes");
    std::fill(object.begin(),object.end(),uint8_t{0});
    Require(bootstrap::WriteQualityObject(table.data(),{4,4,3,4,0,4,4,4,3,4,3,4},object.data()),"Thomas levels refused");
    const auto thomas=Hex("03 00 00 00 10 00 00 00 01 01 01 01 01 00 00 00 01 00 00 00 05 00 00 00 07 00 00 00 07 00 00 00 "
        "00 00 00 00 00 00 00 00 01 00 00 00 02 00 00 00 01 00 00 00 80 00 00 00 00 20 00 00 02 00 00 00 "
        "9a 99 99 3f 01 01 00 00 01 01 01 01 00 00 80 3f 00 00 80 3f 00 00 80 3f 00 00 80 3f 00 00 80 3f "
        "00 00 80 3f 00 00 80 3f 01 00 00 00 01 01 01 00 02 00 00 00 01 01 01 01");
    Require(object==thomas,"Thomas object bytes");
    // Padding +0x69..+0x6B is never written; a level out of 0..4 is refused without writing.
    std::fill(object.begin(),object.end(),uint8_t{0xEE});
    Require(bootstrap::WriteQualityObject(table.data(),{2,3,3,3,3,3,3,3,3,3,3,3},object.data()) && object[0x69]==0xEE && object[0x6B]==0xEE,"padding written");
    const auto before=object;
    Require(!bootstrap::WriteQualityObject(table.data(),{5,3,3,3,3,3,3,3,3,3,3,3},object.data()) && object==before,"level 5 accepted");
    // O2: the classes of an option change (options sheet section 4).
    using L=std::array<int32_t,12>;const L base{2,3,3,3,3,3,3,3,3,3,3,3};
    const auto classify=[&](L after,bool size=false){return bootstrap::ClassifyOptionChange(base,after,size);};
    const auto letters=[](uint8_t classes){char text[5]{};bootstrap::OptionClassLetters(classes,text);return std::string(text);};
    {const auto none=classify(base);Require(none.classes==0 && none.changed==0,"no change classified");}
    {auto a=base;a[0]=4;const auto c=classify(a);Require(letters(c.classes)=="P" && c.changed==1u,"Texture is P");}
    {auto a=base;a[4]=0;const auto off=classify(a);Require(letters(off.classes)=="G" && off.changed==1u<<4,"MotionBlur off is G");
     const auto on=bootstrap::ClassifyOptionChange(a,base,false);Require(letters(on.classes)=="GH","MotionBlur on is G and H");}
    {auto a=base;a[2]=2;Require(letters(classify(a).classes)=="GH","SSAO across the temporal boundary is G and H");
     auto b=base;b[2]=4;Require(letters(classify(b).classes)=="G","SSAO HIGH to MAX is G");
     auto c=base;c[2]=0;Require(letters(classify(c).classes)=="GH","SSAO off is G and H");}
    {auto a=base;a[5]=4;Require(letters(classify(a).classes)=="R","Shadow size is R");
     a[5]=0;Require(letters(classify(a).classes)=="GR","Shadow off is G and R");}
    {auto a=base;a[6]=4;a[11]=1;const auto c=classify(a);Require(letters(c.classes)=="PG" && c.changed==((1u<<6)|(1u<<11)),"Lighting and Shade");}
    {const auto c=classify(base,true);Require(letters(c.classes)=="RH" && c.changed==1u<<12,"render size is R and H");}
    std::puts("PASS quality object: table 37 rows, initial and Thomas images equal the oracle, padding kept, out-of-range level refused; option change classes");
    return 0;
}catch(const std::exception& e){std::printf("FAIL %s\n",e.what());return 1;}}
