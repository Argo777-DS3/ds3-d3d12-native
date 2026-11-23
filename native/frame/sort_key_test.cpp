// I-0113 tranche 15: the model rows' order key (world/sort_key.h) on values whose answers follow from BF330, BF210 and the
// emitters' biases.
#include "../world/sort_key.h"
#include <cmath>
#include <cstdio>
#include <stdexcept>

namespace {
using namespace ds3native::sort_key;
void Expect(bool ok,const char* what){if(!ok)throw std::runtime_error(what);}
bool Near(float a,float b){return std::fabs(a-b)<1e-4f;}
}

int main(){
    try{
        // A camera at (0, 4, -8) looking at the origin (Reference1925's title camera): the row (0, -0.447, 0.894, 8.944).
        const float pose[16]{1,0,0,0, 0,0.894427f,0.447214f,0, 0,-0.447214f,0.894427f,0, 0,4,-8,1};
        float row[4]{};Expect(DepthRow(pose,row),"row");
        Expect(Near(row[0],0.f) && Near(row[1],-0.447214f) && Near(row[2],0.894427f) && Near(row[3],8.944272f),"the title camera's row");
        const float zero[16]{};float none[4]{};Expect(!DepthRow(zero,none),"no forward axis");
        // far 100: near -10, the span 110. The camera's own position: d = 0, key (0 + 10) / 110 * 65535 = 5957.7 -> 5957.
        const float flat[4]{0,0,1,0};const float at[3]{0,0,0};
        Expect(Key(flat,100.f,at,0.f)==5957,"d 0");
        const float far_point[3]{0,0,100};Expect(Key(flat,100.f,far_point,0.f)==65535,"at the far plane");
        const float beyond[3]{0,0,500};Expect(Key(flat,100.f,beyond,0.f)==65535,"clamped");
        const float behind[3]{0,0,-50};Expect(Key(flat,100.f,behind,0.f)==0,"negative: 0");
        const float nan_point[3]{0,0,std::nanf("")};Expect(Key(flat,100.f,nan_point,0.f)==0,"not a number: 0");
        // The bias moves the key as a distance: +10 at d 0 is the key of d 10.
        const float ten[3]{0,0,10};Expect(Key(flat,100.f,at,10.f)==Key(flat,100.f,ten,0.f),"bias as distance");
        // BF210: 0x29 and the other reversed passes far to near, 0x21 near to far.
        Expect(PassKey(0x29,flat,100.f,at,0.f)==0xFFFFu-5957u && PassKey(0x21,flat,100.f,at,0.f)==5957u,"reversal");
        Expect(Reversed(0x1F) && Reversed(0x2D) && Reversed(0x31) && Reversed(0x34) && !Reversed(0x22) && !Reversed(5),"reversed passes");
        Expect(PassKey(0x29,flat,100.f,ten,0.f)<PassKey(0x29,flat,100.f,at,0.f),"0x29: the farther first");
        // The biases: a box 2 x 4 x 6 seen along z: half its depth extent 3; turned 45 degrees about y, (2 + 6) / sqrt 2 / 2.
        const float box[8]{-1,-2,-3,1, 1,2,3,1};
        Expect(Near(ViewExtentBias(flat,box,0.f),3.f) && Near(ViewExtentBias(flat,box,0.5f),3.5f),"view extent");
        const float diagonal[4]{0.707107f,0,0.707107f,0};Expect(Near(ViewExtentBias(diagonal,box,0.f),(2.f+6.f)*0.707107f*0.5f),"turned view");
        Expect(Near(HalfDiagonalBias(box,0.f),std::sqrt(1.f+4.f+9.f)) && Near(HalfDiagonalBias(box,1.f),std::sqrt(14.f)+1.f),"half diagonal");
        float centre[3]{};const float moved[8]{2,0,4,1, 4,2,8,1};Centre(moved,centre);Expect(Near(centre[0],3.f) && Near(centre[1],1.f) && Near(centre[2],6.f),"centre");
        // 57A20's row shifts: the split 0x29 rows +0.5 (pre-pass) and -0.5 (colour), the split 0x22 row +0.5, nothing else.
        Expect(RowBiasShift(0x29,true,true,true)==0.5f && RowBiasShift(0x29,true,true,false)==-0.5f,"split 0x29 rows");
        Expect(RowBiasShift(0x29,false,true,false)==0.f && RowBiasShift(0x29,true,false,false)==0.f,"0x29 without the byte or of category 0x21");
        Expect(RowBiasShift(0x22,true,false,false)==0.5f && RowBiasShift(0x22,false,false,false)==0.f,"0x22");
        Expect(RowBiasShift(0x21,true,false,false)==0.f && RowBiasShift(0x1F,true,true,false)==0.f,"0x21 and 0x1F as given");
        // The order: the subs 0x1F, 0x21, 0x22, 0x29 first, then the key, then the row's index.
        Expect(OrderWord(0x1F,0xFFFF,9)<OrderWord(0x21,0,0) && OrderWord(0x21,0xFFFF,9)<OrderWord(0x22,0,0) && OrderWord(0x22,0xFFFF,9)<OrderWord(0x29,0,0),"subs");
        Expect(OrderWord(0x29,5,9)<OrderWord(0x29,6,0) && OrderWord(0x29,5,1)<OrderWord(0x29,5,2),"key then index");
        Expect((OrderWord(0x29,0x1234,77)>>32&0xFFFF)==0x1234 && (OrderWord(0x29,0x1234,77)&0xFFFFFFFFu)==77 && OrderWord(0x29,0,0)>>48==3,"fields");
        std::puts("sort_key_test: PASS");return 0;
    }catch(const std::exception& e){std::printf("sort_key_test: FAIL %s\n",e.what());return 1;}
}
