// I-0132 R12a: font_outline.h and font_raster.h on synthetic movies: a DefineFont3 tag written here bit by bit (two glyphs, a
// square with a square hole and a curved triangle, wide codes, a layout with advances and a kerning pair), read back; the
// rasteriser's exact coverage (a pixel-aligned square full, its hole empty, the area of a triangle, a half-pixel edge at half),
// the blur's conservation, the shelf packer's refusal.
#include "../renderer/font_raster.h"
#include <cstdio>
#include <stdexcept>
#include <string>

using namespace ds3native::font;
namespace {
void Require(bool v,const char* m){if(!v)throw std::runtime_error(m);}
struct BitWriter {
    std::vector<uint8_t> bytes;uint32_t bit{};
    void U(uint32_t v,uint32_t n){for(uint32_t i=n;i-->0;){if(!(bit&7))bytes.push_back(0);if((v>>i)&1u)bytes.back()|=uint8_t(0x80u>>(bit&7));++bit;}}
    void S(int32_t v,uint32_t n){U(uint32_t(v)&((1u<<n)-1u),n);}
};
// A shape: moves and lines and curves in twentieths of an em unit, absolute moves, relative edges, 16-bit fields.
struct ShapeWriter {
    BitWriter w;
    ShapeWriter(){w.U(1,4);w.U(0,4);}
    void Move(int x,int y){w.U(0,1);w.U(1|4,5);w.U(16,5);w.S(x,16);w.S(y,16);w.U(1,1);}   // MoveTo and FillStyle1 = 1
    void Line(int dx,int dy){w.U(1,1);w.U(1,1);w.U(14,4);w.U(1,1);w.S(dx,16);w.S(dy,16);}
    void Curve(int cx,int cy,int ax,int ay){w.U(1,1);w.U(0,1);w.U(14,4);w.S(cx,16);w.S(cy,16);w.S(ax,16);w.S(ay,16);}
    std::vector<uint8_t> End(){w.U(0,1);w.U(0,5);return w.bytes;}
};
void U16(std::vector<uint8_t>& o,uint32_t v){o.push_back(uint8_t(v));o.push_back(uint8_t(v>>8));}
std::vector<uint8_t> Movie(const std::vector<std::vector<uint8_t>>& shapes,const std::vector<uint16_t>& codes,const std::vector<int16_t>& advances,
    uint16_t kern_left,uint16_t kern_right,int16_t kern){
    std::vector<uint8_t> tag;U16(tag,7);tag.push_back(0x80|0x04);tag.push_back(1);const std::string name="Test Font";tag.push_back(uint8_t(name.size()+1));
    tag.insert(tag.end(),name.begin(),name.end());tag.push_back(0);U16(tag,uint32_t(shapes.size()));
    uint32_t offset=uint32_t(2*(shapes.size()+1));
    for(const auto& s:shapes){U16(tag,offset);offset+=uint32_t(s.size());}U16(tag,offset);
    for(const auto& s:shapes)tag.insert(tag.end(),s.begin(),s.end());
    for(const auto c:codes)U16(tag,c);
    U16(tag,900*20);U16(tag,124*20);U16(tag,0);
    for(const auto a:advances)U16(tag,uint16_t(a));
    for(size_t i=0;i<shapes.size();++i)tag.push_back(0);   // empty RECTs: Nbits 0, one byte each
    U16(tag,1);U16(tag,kern_left);U16(tag,kern_right);U16(tag,uint16_t(kern));
    // The header (« GFX », a version, a length left 0), a 5-bit RECT of zeros (one byte), the rate (8.8), the frame count.
    std::vector<uint8_t> movie={'G','F','X',11,0,0,0,0, 0, 0,24, 1,0};
    const uint16_t head=uint16_t((75<<6)|63);U16(movie,head);const uint32_t n=uint32_t(tag.size());
    movie.push_back(uint8_t(n));movie.push_back(uint8_t(n>>8));movie.push_back(uint8_t(n>>16));movie.push_back(uint8_t(n>>24));
    movie.insert(movie.end(),tag.begin(),tag.end());U16(movie,0);
    return movie;}
uint64_t Sum(const std::vector<uint8_t>& p){uint64_t s=0;for(const auto v:p)s+=v;return s;}
}
int main(){try{
    // Glyph 0, « O »-like: a square 0..1024 em units (scaled ×20) with a hole 256..768 drawn the other way round. Glyph 1: a triangle
    // with one curved side.
    ShapeWriter o;o.Move(0,-20480);o.Line(20480,0);o.Line(0,20480);o.Line(-20480,0);o.Line(0,-20480);
    o.Move(5120,-15360);o.Line(0,10240);o.Line(10240,0);o.Line(0,-10240);o.Line(-10240,0);
    ShapeWriter t;t.Move(0,0);t.Line(20480,0);t.Curve(-7680,-10240,-2560,-10240);t.Line(-10240,20480);
    const auto movie=Movie({o.End(),t.End()},{0x4F,0xE9},{int16_t(1100*20),int16_t(1000*20)},0x4F,0xE9,int16_t(-50*20));
    const auto fonts=ReadFonts(movie);
    Require(fonts.size()==1,"one DefineFont3 read");const auto& f=fonts[0];
    Require(f.name=="Test Font" && f.id==7 && f.has_layout && f.glyphs.size()==2 && f.ascent==900*20.f && f.descent==124*20.f,"the font's name, id, layout");
    Require(f.Find(0x4F) && f.Find(0xE9) && !f.Find(0x41) && f.Find(0xE9)->advance==1000*20.f && f.Kerning(0x4F,0xE9)==-50*20.f && f.Kerning(0xE9,0x4F)==0.f,
        "codes (wide), advances, the kerning pair");
    Require(f.Find(0x4F)->contours.size()==2 && f.Find(0x4F)->contours[0].segments.size()==4 && f.Find(0xE9)->contours[0].segments.back().curve==false &&
        f.Find(0xE9)->contours[0].segments[1].curve,"the contours and their segments");
    // 16 pixels an em: the square is 16x16 pixels, aligned, its hole 8x8.
    const auto c=Rasterize(*f.Find(0x4F),16.f,0);
    Require(c.width==16 && c.height==16 && c.left==0.f && c.top==-16.f,"the image's extent and origin");
    Require(c.pixels[0]==255 && c.pixels[15*16+15]==255 && c.pixels[8*16+8]==0 && c.pixels[4*16+4]==0 && c.pixels[11*16+11]==0 && c.pixels[3*16+3]==255 && c.pixels[4*16+3]==255 && c.pixels[4*16+12]==255 && c.pixels[12*16+12]==255,
        "full outside the hole, empty inside");
    Require(Sum(c.pixels)==uint64_t(16*16-8*8)*255,"the coverage's sum is the area");
    // A half-pixel edge: the square at 8.5 pixels an em: its right column half covered, its top right corner a quarter.
    {const auto h=Rasterize(*f.Find(0x4F),8.5f,0);Require(h.width==9 && h.pixels[9+8]>=120 && h.pixels[9+8]<=135 && h.pixels[8]>=56 && h.pixels[8]<=72,"a half-covered pixel at half");}
    // The padded glyph and its blur: the blur keeps the coverage's sum within rounding.
    {const auto p=Rasterize(*f.Find(0x4F),16.f,2);Require(p.width==20 && p.height==20 && p.left==-2.f && p.top==-18.f && Sum(p.pixels)==Sum(c.pixels),"the padding");
        const auto b=Blur(p,1);const double s0=double(Sum(p.pixels)),s1=double(Sum(b));Require(std::abs(s1-s0)<=s0*0.01,"the blur keeps the area");
        Require(b[2*20+2]<255 && b[2*20+2]>0,"the blur softens the corner");}
    // The triangle: a straight-sided triangle of base 16 and height 16 at 16 pixels an em minus its curved bite; between the two.
    {const auto tri=Rasterize(*f.Find(0xE9),16.f,0);const double area=double(Sum(tri.pixels))/255.;Require(area>80. && area<127.,"the curved triangle's area");}
    // A space (no contour): an empty image.
    {Glyph space;Require(Rasterize(space,16.f,2).pixels.empty(),"an empty glyph");}
    // The shelf: places in rows, refuses past its size.
    {Shelf s(32);uint32_t x{},y{};Require(s.Place(20,10,x,y) && x==0 && y==0,"first place");Require(s.Place(20,10,x,y) && x==0 && y==11,"the next row");
        Require(s.Place(10,10,x,y) && x==21 && y==11,"beside");Require(!s.Place(10,20,x,y),"full");Require(!s.Place(40,1,x,y),"too wide");}
    // A malformed tag (cut short) is not read.
    {auto cut=movie;cut.resize(cut.size()-20);Require(ReadFonts(cut).empty(),"a cut movie: no font");}
    // The step review's M2: the tag's own bounds. The tag (its long header at byte 13, its body from 19) cut inside at every length,
    // its declared length rewritten so the stream stays whole: never read (each section needs its whole extent); then every body byte
    // set to 0, to 0xFF and flipped: whatever is read is rasterised, and nothing reads past the end (bounded loops, checked offsets).
    {const size_t body=19;const uint32_t length=uint32_t(movie[15])|(uint32_t(movie[16])<<8)|(uint32_t(movie[17])<<16)|(uint32_t(movie[18])<<24);
        Require(body+length+2==movie.size(),"the synthetic tag's layout");
        for(uint32_t keep=0;keep<length;++keep){std::vector<uint8_t> m(movie.begin(),movie.begin()+body+keep);
            m[15]=uint8_t(keep);m[16]=uint8_t(keep>>8);m[17]=uint8_t(keep>>16);m[18]=uint8_t(keep>>24);m.push_back(0);m.push_back(0);
            Require(ReadFonts(m).empty(),"a tag cut inside: no font");}
        uint32_t read{};
        for(size_t i=body;i<body+length;++i)for(const int mode:{0,1,2}){auto m=movie;m[i]=mode==0?uint8_t(0):mode==1?uint8_t(0xFF):uint8_t(m[i]^0x55);
            for(const auto& font:ReadFonts(m)){++read;for(const auto& g:font.glyphs)(void)Rasterize(g,16.f,2);}}
        Require(read>0,"some changed bytes still read (a code, an advance): the loop reached the rasteriser");}
    std::printf("PASS font raster: DefineFont3 read back, exact coverage, the hole, the blur, the shelf\n");
    return 0;
}catch(const std::exception& e){std::printf("FAIL %s\n",e.what());return 1;}}
