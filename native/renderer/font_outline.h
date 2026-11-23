#pragma once
// I-0132 R12a (STUDY-R12-SETTINGS-PAGE §3.2): the menus' own fonts for our Rekindled page, read from the game's font.gfx
// (data1/font/<family>/font.gfx): a GFX movie (« GFX » plain or « CFX » zlib, SWF's tag stream) whose DefineFont3 tag (75) holds
// each glyph's outline (SWF shape records: lines and quadratic curves, in units of 1/20 of a 1024-unit em, y down), its code, and
// with HasLayout its advance, the font's ascent and descent and the kerning pairs. Read once by the module, nothing copied beside it.
// Header-only, unit-tested alone (font_raster_test); no Windows and no D3D12 here. The zlib inflate is the caller's (the module
// has zlib; the test feeds a plain « GFX » movie).
#include <algorithm>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace ds3native::font {
inline constexpr float kEmUnits=1024.f*20.f;   // DefineFont3: twentieths of a 1024 em

struct Point {float x{},y{};};
// One closed or open contour: a start point, then segments; a segment with `curve` has a quadratic control point.
struct Segment {Point control,to;bool curve{};};
struct Contour {Point start;std::vector<Segment> segments;};
struct Glyph {
    uint32_t code{};
    float advance{};               // em units (kEmUnits an em); 0 without a layout
    std::vector<Contour> contours; // em units, y down, the baseline at 0
};
struct Font {
    std::string name;
    uint16_t id{};
    bool bold{},italic{},has_layout{};
    float ascent{},descent{},leading{};   // em units
    std::vector<Glyph> glyphs;
    std::unordered_map<uint32_t,uint32_t> by_code;        // code -> glyph index
    std::unordered_map<uint64_t,float> kerning;           // (left << 32 | right) -> adjustment, em units
    const Glyph* Find(uint32_t code) const noexcept {const auto it=by_code.find(code);return it==by_code.end()?nullptr:&glyphs[it->second];}
    float Kerning(uint32_t left,uint32_t right) const noexcept {const auto it=kerning.find((uint64_t(left)<<32)|right);return it==kerning.end()?0.f:it->second;}
};

namespace detail {
struct Bits {
    std::span<const uint8_t> data;size_t bit{};bool overrun{};
    uint32_t U(uint32_t n) noexcept {uint32_t v{};for(uint32_t i=0;i<n;++i){const size_t byte=bit>>3;if(byte>=data.size()){overrun=true;return 0;}
        v=(v<<1)|((data[byte]>>(7-(bit&7)))&1u);++bit;}return v;}
    int32_t S(uint32_t n) noexcept {if(!n)return 0;const uint32_t v=U(n);return (v&(1u<<(n-1)))?int32_t(v|(~0u<<n)):int32_t(v);}
};
inline uint16_t U16(std::span<const uint8_t> d,size_t at) noexcept {return at+2<=d.size()?uint16_t(d[at]|(d[at+1]<<8)):0;}
inline uint32_t U32(std::span<const uint8_t> d,size_t at) noexcept {return at+4<=d.size()?uint32_t(d[at]|(d[at+1]<<8)|(d[at+2]<<16)|(uint32_t(d[at+3])<<24)):0;}
// One glyph's SHAPE (NumFillBits, NumLineBits, then the records to the end record). False on a malformed shape.
inline bool Shape(std::span<const uint8_t> bytes,std::vector<Contour>& out) {
    Bits b{bytes};uint32_t fill_bits=b.U(4),line_bits=b.U(4);Point pen{};Contour* current{};
    for(uint32_t guard=0;guard<1u<<20;++guard){
        if(b.overrun)return false;
        if(!b.U(1)){   // a style change, or the end
            const uint32_t flags=b.U(5);if(!flags)return !b.overrun;
            if(flags&0x10)return false;   // StateNewStyles: never in a font's glyph
            if(flags&1){const uint32_t n=b.U(5);const float x=float(b.S(n)),y=float(b.S(n));pen={x,y};out.push_back({pen,{}});current=&out.back();}
            if(flags&2)(void)b.U(fill_bits);if(flags&4)(void)b.U(fill_bits);if(flags&8)(void)b.U(line_bits);
            continue;}
        if(!current){out.push_back({pen,{}});current=&out.back();}
        const bool straight=b.U(1)!=0;const uint32_t n=b.U(4)+2;
        if(straight){float dx=0,dy=0;
            if(b.U(1)){dx=float(b.S(n));dy=float(b.S(n));}else if(b.U(1))dy=float(b.S(n));else dx=float(b.S(n));
            const Point to{pen.x+dx,pen.y+dy};current->segments.push_back({to,to,false});pen=to;}
        else{const float cx=float(b.S(n)),cy=float(b.S(n)),ax=float(b.S(n)),ay=float(b.S(n));
            const Point control{pen.x+cx,pen.y+cy},to{control.x+ax,control.y+ay};current->segments.push_back({control,to,true});pen=to;}
    }
    return false;}
}

// The SWF tag stream of an inflated movie (« GFX », « FWS », or a « CFX »/« CWS » header followed by the inflated body).
struct Tag {uint16_t code{};size_t at{},length{};};
inline std::vector<Tag> Tags(std::span<const uint8_t> movie) {
    std::vector<Tag> tags;if(movie.size()<9)return tags;
    const uint32_t rect_bits=5+4*(movie[8]>>3);size_t at=8+(rect_bits+7)/8+4;
    while(at+2<=movie.size()){
        const uint16_t head=detail::U16(movie,at);at+=2;const uint16_t code=uint16_t(head>>6);size_t length=head&63u;
        if(length==63){if(at+4>movie.size())break;length=detail::U32(movie,at);at+=4;}
        if(at+length>movie.size())break;
        tags.push_back({code,at,length});at+=length;if(!code)break;}
    return tags;}

// The movie's DefineFont3 tags (the menus' font.gfx has one); a malformed tag is skipped, never half read.
inline std::vector<Font> ReadFonts(std::span<const uint8_t> movie) {
    std::vector<Font> fonts;
    for(const auto& tag:Tags(movie)){
        if(tag.code!=75)continue;const auto d=movie.subspan(tag.at,tag.length);
        if(d.size()<5)continue;
        Font font;font.id=detail::U16(d,0);const uint8_t flags=d[2];const size_t name_length=d[4];
        const bool wide_offsets=(flags&0x08)!=0,wide_codes=(flags&0x04)!=0;font.has_layout=(flags&0x80)!=0;font.italic=(flags&0x02)!=0;font.bold=(flags&0x01)!=0;
        if(5+name_length+2>d.size())continue;
        font.name.assign(reinterpret_cast<const char*>(d.data()+5),name_length);while(!font.name.empty() && !font.name.back())font.name.pop_back();
        size_t at=5+name_length;const uint32_t count=detail::U16(d,at);at+=2;
        const size_t table=at,width=wide_offsets?4:2;
        if(table+width*(size_t(count)+1)>d.size())continue;
        std::vector<uint32_t> offsets(size_t(count)+1);
        for(uint32_t i=0;i<=count;++i)offsets[i]=wide_offsets?detail::U32(d,table+width*i):detail::U16(d,table+width*i);
        const size_t codes=table+offsets[count];const size_t code_width=wide_codes?2:1;
        if(codes+code_width*count>d.size())continue;
        bool ok=true;font.glyphs.resize(count);
        for(uint32_t i=0;i<count && ok;++i){
            const size_t from=table+offsets[i],to=i+1<count?table+offsets[i+1]:codes;
            if(from>to || to>d.size()){ok=false;break;}
            ok=detail::Shape(d.subspan(from,to-from),font.glyphs[i].contours);
            font.glyphs[i].code=wide_codes?detail::U16(d,codes+2*i):d[codes+i];}
        if(!ok)continue;
        at=codes+code_width*count;
        if(font.has_layout){
            if(at+6+2*size_t(count)>d.size())continue;
            font.ascent=float(detail::U16(d,at));font.descent=float(detail::U16(d,at+2));font.leading=float(int16_t(detail::U16(d,at+4)));at+=6;
            for(uint32_t i=0;i<count;++i)font.glyphs[i].advance=float(int16_t(detail::U16(d,at+2*i)));
            at+=2*size_t(count);
            for(uint32_t i=0;i<count && ok;++i){   // the bounds table: RECTs, skipped
                if(at>=d.size()){ok=false;break;}
                const uint32_t n=d[at]>>3;at+=(5+4*n+7)/8;}
            if(!ok || at+2>d.size())continue;
            const uint32_t pairs=detail::U16(d,at);at+=2;const size_t pair=2*code_width+2;
            if(at+pair*pairs>d.size())continue;
            for(uint32_t i=0;i<pairs;++i,at+=pair){
                const uint32_t left=wide_codes?detail::U16(d,at):d[at],right=wide_codes?detail::U16(d,at+2):d[at+1];
                font.kerning[(uint64_t(left)<<32)|right]=float(int16_t(detail::U16(d,at+2*code_width)));}}
        for(uint32_t i=0;i<count;++i)font.by_code.emplace(font.glyphs[i].code,i);
        fonts.push_back(std::move(font));}
    return fonts;}
}
