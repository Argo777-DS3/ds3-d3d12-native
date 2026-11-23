// W3 / G0-1 host controls T1 and T2 (GAMMA-COLOUR-PATH-READY 5.1): the display curve against a double-precision pow, and
// the CPU model of the gamma pass (identity at brightness 5 for every 8-bit value).
#include "../renderer/display_curve.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>

using namespace ds3native;
namespace {
void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
std::array<std::array<float,DisplayCurveEntries>,3> Slices(const std::array<float,6>& values){
    std::vector<uint8_t> bytes(DisplayCurveBytes);Require(GenerateDisplayCurve(values,std::span<uint8_t,DisplayCurveBytes>(bytes.data(),DisplayCurveBytes)),"curve refused");
    std::array<std::array<float,DisplayCurveEntries>,3> slices{};DeinterleaveDisplayCurve(std::span<const uint8_t,DisplayCurveBytes>(bytes.data(),DisplayCurveBytes),slices);return slices;}
}
int main(){try{
    // T1: exponent 1 (brightness 5) gives exactly i/1024; the brighter and darker ends follow pow within 1 float ulp.
    const auto identity=Slices(DisplayCurveIdentity);
    for(size_t i=0;i<DisplayCurveEntries;++i)for(size_t c=0;c<3;++c)Require(identity[c][i]==float(i)/1024.f,"T1 identity entry");
    for(const float b:{-1.f,1.f}){const float dst=2.2f+.5f*b;const auto curve=Slices({2.2f,2.2f,2.2f,dst,dst,dst});
        const double exponent=double(2.2f/dst);
        for(size_t i=0;i<DisplayCurveEntries;++i){const double expected=std::min(1.0,std::pow(double(i)/1024.0,exponent));
            const float value=curve[0][i];Require(value>=0.f && value<=1.f,"T1 clamp");
            Require(std::fabs(double(value)-expected)<=double(std::nextafter(float(expected),2.f)-float(expected))*2+1e-9,"T1 pow within two ulps");}}
    {std::vector<uint8_t> bytes(DisplayCurveBytes);const std::span<uint8_t,DisplayCurveBytes> out(bytes.data(),DisplayCurveBytes);
        Require(!GenerateDisplayCurve({2.2f,2.2f,2.2f,0.f,2.2f,2.2f},out) && !GenerateDisplayCurve({2.2f,2.2f,2.2f,NAN,2.2f,2.2f},out),"T1 non-positive values accepted");}
    // T2: the identity curve maps every 8-bit value to itself; the two ends change values within 1 LSB of the exact curve.
    for(int n=0;n<256;++n)Require(DisplayPassModel(identity[0],uint8_t(n))==n,"T2 identity pass");
    size_t changed{};
    for(const float b:{-1.f,1.f}){const float dst=2.2f+.5f*b;const auto curve=Slices({2.2f,2.2f,2.2f,dst,dst,dst});
        for(int n=0;n<256;++n){const double exact=std::pow(n/255.0,double(2.2f/dst))*255.0;const int out=DisplayPassModel(curve[0],uint8_t(n));
            Require(std::fabs(out-exact)<=1.0,"T2 brightness end within 1 LSB");changed+=out!=n;}}
    Require(changed>200,"T2 brightness ends change the image");
    std::puts("PASS display curve: T1 identity exact (i/1024), brightness 0 and 10 within two ulps of pow, non-positive refused; T2 the pass model is the identity at brightness 5 for all 256 values, the ends within 1 LSB");
    return 0;
}catch(const std::exception& e){std::printf("FAIL %s\n",e.what());return 1;}}
