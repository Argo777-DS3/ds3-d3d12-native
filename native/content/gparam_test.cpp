#include "gparam.h"
#include <cstring>
#include <stdexcept>
#include <iostream>

int main() {
    std::vector<uint8_t> bytes(0x54);
    std::memcpy(bytes.data(),"f\0i\0l\0t\0",8);
    const auto word=[&](size_t offset,uint32_t value){std::memcpy(bytes.data()+offset,&value,4);};
    word(8,3);word(24,0x54);
    if(!ds3content::ReadGparam(bytes).groups.empty())return 1;
    const auto refused=[&](){try{ds3content::ReadGparam(bytes);return false;}
        catch(const std::runtime_error&){return true;}};
    word(16,1);if(!refused())return 2;
    word(16,0);word(24,0x55);if(!refused())return 3;
    word(24,0x54);bytes.resize(0x53);if(!refused())return 4;
    std::cout<<"GPARAM empty EOF table and truncated table checks passed\n";
}
