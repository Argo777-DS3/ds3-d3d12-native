#pragma once
// I-0113 tranche 15 (PASS-CHUNKS-STUDY « the key »): the order key of a model row, as 57A20 gives it to 5C0E0 (param_6, the
// position; param_7, the bias) and 5C0E0 to 1400BF330 / 1400BF210. BF330: d = dot((position.xyz, 1), the pass context's depth row
// +0x250), far the camera's +0x58, near = far * -0.1 (143D7F0B0), key = (d - near + bias) / (far - near) * 65535 (143D7F078),
// truncated, 0 when negative or not a number, 65535 at most; BF210 reverses it (0xFFFF - key) for the passes whose byte
// ctx+0x426+pass is 1 (natively 0x1F, 0x29, 0x2D, 0x31, 0x34, as 21_effects sets the effects' context), so those draw far to near.
// The bias by emitter (headless, evidence/I-0113/t15/key-decomp-1.txt with the exports): D3DA20 (map items and objects) half the
// view-space depth extent of the box at the item's instance +0x80 (286010 with the pass's view +0x10); D079C0 (draw entities)
// the same of its box +0xB80 (9E840) plus the entity's +0xBA0; CFDE70 (character parts) the half diagonal's length of the owner's
// box +0xAD0 plus the owner's +0xAF0. 286010 and 9E840 transform the box as a row vector times the view (Arvo's bounds), so the
// depth extent is the sum over the axes of |view[axis][2]| times the box's extent; the view's third column is the depth row's
// direction (an orthonormal view, INFERRED). Header-only: unit-tested alone (sort_key_test).
#include <cmath>
#include <cstdint>

namespace ds3native::sort_key {
inline bool Reversed(uint32_t pass) noexcept {return pass==0x1F || pass==0x29 || pass==0x2D || pass==0x31 || pass==0x34;}
// BF330 (row: the depth row's xyz and w; centre's w is taken as 1, as BF330 masks it).
inline uint32_t Key(const float* row,float far_plane,const float* centre,float bias) noexcept {
    const float near_plane=far_plane*-0.1f;
    const float d=centre[0]*row[0]+centre[1]*row[1]+centre[2]*row[2]+row[3];
    const float k=((d-near_plane)+bias)/(far_plane-near_plane)*65535.0f;
    if(!(0.0f<=k))return 0;
    return static_cast<uint32_t>(static_cast<int32_t>(k<=65535.0f?k:65535.0f));
}
// BF210's order for the pass.
inline uint32_t PassKey(uint32_t pass,const float* row,float far_plane,const float* centre,float bias) noexcept {
    const uint32_t k=Key(row,far_plane,centre,bias);return Reversed(pass)?0xFFFFu-k:k;
}
// The box (minimum xyz w, maximum xyz w) and its centre.
inline void Centre(const float* box,float* centre) noexcept {for(int i=0;i<3;++i)centre[i]=(box[4+i]+box[i])*0.5f;}
// 57A20's shift of the bias for one row: a draw with its descriptor's byte +0x10 records, at 0x29 (category 2 or 0x29), its depth
// pre-pass at +0.5 and its colour rows at -0.5 (57A20:550-569, 657-720), and its 0x22 row (category 0x21) at +0.5 (57A20:640-660);
// every other row takes the bias as given.
inline float RowBiasShift(uint32_t pass,bool split_byte,bool split_category,bool prepass) noexcept {   // split_category: category 2 or 0x29
    if(pass==0x29 && split_byte && split_category)return prepass?0.5f:-0.5f;
    if(pass==0x22 && split_byte)return 0.5f;
    return 0.0f;
}
// The forward subs' order in the scene view's key (the sub in its bits 16 to 21, the list sorted ascending): 0x1F, 0x21, 0x22,
// then 0x29 (the water's 0x24 and 0x27 between 0x22 and 0x29 are drawn apart). The sort word: the rank, the key, then the row's
// index (equal keys in the rows' order).
inline uint64_t ForwardRank(uint32_t pass) noexcept {return pass==0x1F?0:pass==0x21?1:pass==0x22?2:3;}
inline uint64_t OrderWord(uint32_t pass,uint32_t key,uint32_t index) noexcept {return ForwardRank(pass)<<48|uint64_t(key&0xFFFF)<<32|index;}
// D3DA20 and D079C0: half the box's extent along the depth row, plus `extra` (D079C0's +0xBA0; 0 for D3DA20).
inline float ViewExtentBias(const float* row,const float* box,float extra) noexcept {
    float extent=0.0f;for(int i=0;i<3;++i)extent+=std::fabs(row[i])*(box[4+i]-box[i]);
    return extent*0.5f+extra;
}
// CFDE70: the length of the half diagonal, plus `extra` (the owner's +0xAF0).
inline float HalfDiagonalBias(const float* box,float extra) noexcept {
    const float x=(box[4]-box[0])*0.5f,y=(box[5]-box[1])*0.5f,z=(box[6]-box[2])*0.5f;
    return std::sqrt(x*x+z*z+y*y)+extra;
}
// The depth row of a camera pose (row-vector world matrix: +0x20 the forward axis, +0x30 the position): the forward axis
// normalised and w = -dot(forward, position), as the effects' pass context takes it (21_effects; Reference1925). False when the
// axis has no length.
inline bool DepthRow(const float* pose,float* row) noexcept {
    const float l=std::sqrt(pose[8]*pose[8]+pose[9]*pose[9]+pose[10]*pose[10]);
    if(!(l>0.0f))return false;
    for(int i=0;i<3;++i)row[i]=pose[8+i]/l;
    row[3]=-(row[0]*pose[12]+row[1]*pose[13]+row[2]*pose[14]);
    return true;
}
}
