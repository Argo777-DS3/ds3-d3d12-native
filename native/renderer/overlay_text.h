#pragma once
// I-0132 R12a: text for the overlay in DS3's own menu fonts (font_outline.h, font_raster.h): a glyph cache over two coverage atlases
// (the glyphs, and their blurred copy for the menus' drop shadow), and a line laid out with the font's advances and kerning into
// overlay quads. Header-only, no D3D12: unit-tested (overlay_text_test).
#include "font_raster.h"
#include "overlay_frame.h"
#include <cmath>
#include <span>
#include <string_view>
#include <unordered_map>

namespace ds3native::overlay {
// UTF-8 to code points; a malformed byte gives U+FFFD and moves on one byte.
inline std::u32string Utf8(std::string_view s) {
    std::u32string out;
    for(size_t i=0;i<s.size();){
        const unsigned char c=static_cast<unsigned char>(s[i]);uint32_t cp{},n{};
        if(c<0x80){cp=c;n=1;}else if((c&0xE0)==0xC0){cp=c&0x1Fu;n=2;}else if((c&0xF0)==0xE0){cp=c&0x0Fu;n=3;}else if((c&0xF8)==0xF0){cp=c&0x07u;n=4;}
        else{out.push_back(0xFFFD);++i;continue;}
        if(i+n>s.size()){out.push_back(0xFFFD);++i;continue;}
        bool ok=true;for(uint32_t k=1;k<n;++k){const unsigned char d=static_cast<unsigned char>(s[i+k]);if((d&0xC0)!=0x80){ok=false;break;}cp=(cp<<6)|(d&0x3Fu);}
        if(!ok){out.push_back(0xFFFD);++i;continue;}
        out.push_back(cp);i+=n;}
    return out;}

// The menus' shadow (MENUS-MAP §78.2): black, blurred, 2 pixels away at 1280x720, at 0.8 rad (down and to the right). `blur` is the
// box's radius in output pixels (1 at 1280x720; the caller scales it with the output, at most GlyphCache::kPad).
struct Shadow {float distance{2.f},angle{0.8f},alpha{1.f};uint32_t blur{1};bool on{true};};

class GlyphCache {
public:
    struct Entry {uint32_t x{},y{},width{},height{};float left{},top{},advance{};bool placed{};};
    static constexpr uint32_t kPad=3;
    GlyphCache(std::shared_ptr<const font::Font> font,uint64_t identity,uint32_t size=1024):font_(std::move(font)),identity_(identity),size_(size),shelf_(size),
        glyphs_(std::make_shared<std::vector<uint8_t>>(size_t(size)*size,uint8_t(0))),shadows_(std::make_shared<std::vector<uint8_t>>(size_t(size)*size,uint8_t(0))){}
    const font::Font& Font() const noexcept {return *font_;}
    // The entry of a code at `pixels` per em; a code the font lacks, or a full atlas, gives an entry without an image (its advance
    // kept, the caller counts it).
    const Entry& Get(uint32_t code,float pixels,uint32_t blur=1) {
        blur=(std::min)(blur,kPad);
        const uint64_t key=(uint64_t(code)<<32)|(uint64_t(blur)<<28)|(uint32_t(std::lround(pixels*16.f))&0x0FFFFFFFu);
        if(const auto it=entries_.find(key);it!=entries_.end())return it->second;
        Entry entry;const font::Glyph* glyph=font_->Find(code);
        if(glyph){entry.advance=glyph->advance*pixels/font::kEmUnits;
            const auto c=font::Rasterize(*glyph,pixels,kPad);
            if(!c.pixels.empty() && shelf_.Place(c.width,c.height,entry.x,entry.y)){
                Writable(glyphs_);Writable(shadows_);
                const auto blurred=font::Blur(c,blur);
                for(uint32_t y=0;y<c.height;++y)for(uint32_t x=0;x<c.width;++x){
                    (*glyphs_w_)[size_t(entry.y+y)*size_+entry.x+x]=c.pixels[size_t(y)*c.width+x];
                    (*shadows_w_)[size_t(entry.y+y)*size_+entry.x+x]=blurred[size_t(y)*c.width+x];}
                entry.width=c.width;entry.height=c.height;entry.left=c.left;entry.top=c.top;entry.placed=true;++version_;}
            else if(!c.pixels.empty())++refused_;}
        else ++missing_;
        return entries_.emplace(key,entry).first->second;}
    // The two atlases as overlay images (identity, identity+1), their version the cache's.
    // Handing an atlas out publishes it (the final review's L7): the next write goes to a copy, the frames keep theirs.
    Image Glyphs() const {glyphs_published_=true;return {identity_,version_,size_,size_,Format::Coverage8,glyphs_};}
    Image Shadows() const {shadows_published_=true;return {identity_+1,version_,size_,size_,Format::Coverage8,shadows_};}
    uint32_t Missing() const noexcept {return missing_;}
    uint32_t Refused() const noexcept {return refused_;}
private:
    // A frame keeps the atlas it was given: a write after it was shared goes to a copy (copy on write).
    void Writable(std::shared_ptr<std::vector<uint8_t>>& atlas){
        const bool glyphs=&atlas==&glyphs_;auto& writer=glyphs?glyphs_w_:shadows_w_;bool& published=glyphs?glyphs_published_:shadows_published_;
        if(published){atlas=std::make_shared<std::vector<uint8_t>>(*atlas);published=false;}
        writer=atlas.get();}
    std::shared_ptr<const font::Font> font_;uint64_t identity_{};uint32_t size_{};font::Shelf shelf_;
    std::shared_ptr<std::vector<uint8_t>> glyphs_,shadows_;std::vector<uint8_t>* glyphs_w_{};std::vector<uint8_t>* shadows_w_{};
    std::unordered_map<uint64_t,Entry> entries_;uint64_t version_{1};uint32_t missing_{},refused_{};mutable bool glyphs_published_{},shadows_published_{};
};

// One line of text at (x, baseline) in output pixels, `pixels` per em: its quads (the shadow's first, then the glyphs') appended to
// `frame`, the two atlases added to its images if absent; returns the line's advance. `glyph_image`/`shadow_image` cache the images'
// indices in the frame (-1 to add them).
struct TextStyle {float pixels{24.f};std::array<float,4> colour{1,1,1,1};Shadow shadow;float scale{1.f};};   // scale: the output height / 720
inline int32_t ImageIndex(Frame& frame,const Image& image){
    for(size_t i=0;i<frame.images.size();++i)if(frame.images[i].identity==image.identity){frame.images[i]=image;return int32_t(i);}
    if(frame.images.size()>=Frame::kImages)return -2;frame.images.push_back(image);return int32_t(frame.images.size()-1);}
// A font chain (the first font that has a code draws it: the English body font MatisseProN lacks the colon, the long dash and the
// guillemets, the first witness i132-r12a4-overlay): kerning only between two codes of one font.
using Chain=std::span<GlyphCache* const>;
inline GlyphCache* Pick(Chain chain,uint32_t code) noexcept {
    for(auto* cache:chain)if(cache && cache->Font().Find(code))return cache;
    for(auto* cache:chain)if(cache)return cache;return nullptr;}
// The advances only (no glyph rasterised): a code no font has advances by 0, as DrawText's.
inline float Measure(Chain chain,std::u32string_view text,float pixels){
    float pen=0;uint32_t previous{};GlyphCache* last{};
    for(const auto cp:text){auto* cache=Pick(chain,cp);if(!cache)break;
        if(previous && cache==last)pen+=cache->Font().Kerning(previous,cp)*pixels/font::kEmUnits;
        if(const auto* glyph=cache->Font().Find(cp))pen+=glyph->advance*pixels/font::kEmUnits;previous=cp;last=cache;}
    return pen;}
inline float Measure(GlyphCache& cache,std::u32string_view text,float pixels){GlyphCache* const one[]{&cache};return Measure(Chain(one),text,pixels);}
inline float DrawText(Frame& frame,Chain chain,std::u32string_view text,float x,float baseline,const TextStyle& style){
    const float pixels=style.pixels;
    // The glyphs first (they may grow the atlases), then the images' current versions into the frame.
    struct Placed {const GlyphCache::Entry* entry;GlyphCache* cache;float x;};std::vector<Placed> placed;placed.reserve(text.size());
    float pen=x;uint32_t previous{};GlyphCache* last{};
    for(const auto cp:text){auto* cache=Pick(chain,cp);if(!cache)break;
        if(previous && cache==last)pen+=cache->Font().Kerning(previous,cp)*pixels/font::kEmUnits;
        const auto& e=cache->Get(cp,pixels,style.shadow.blur);if(e.placed)placed.push_back({&e,cache,pen});pen+=e.advance;previous=cp;last=cache;}
    const float rx=std::round(baseline);
    const auto images=[&](GlyphCache* cache){return std::pair<int32_t,int32_t>{ImageIndex(frame,cache->Glyphs()),style.shadow.on?ImageIndex(frame,cache->Shadows()):-1};};
    if(style.shadow.on){
        const float d=style.shadow.distance*style.scale,dx=d*std::cos(style.shadow.angle),dy=d*std::sin(style.shadow.angle);
        for(const auto& p:placed){const auto [glyphs,shadows]=images(p.cache);if(glyphs<0 || shadows<0)continue;const auto& e=*p.entry;
            const float qx=std::round(p.x+e.left+dx),qy=std::round(rx+e.top+dy);
            frame.quads.push_back({qx,qy,qx+float(e.width),qy+float(e.height),float(e.x),float(e.y),float(e.x+e.width),float(e.y+e.height),{0,0,0,style.shadow.alpha*style.colour[3]},shadows});}}
    for(const auto& p:placed){const auto [glyphs,shadows]=images(p.cache);(void)shadows;if(glyphs<0)continue;const auto& e=*p.entry;
        const float qx=std::round(p.x+e.left),qy=std::round(rx+e.top);
        frame.quads.push_back({qx,qy,qx+float(e.width),qy+float(e.height),float(e.x),float(e.y),float(e.x+e.width),float(e.y+e.height),style.colour,glyphs});}
    return pen-x;}
inline float DrawText(Frame& frame,GlyphCache& cache,std::u32string_view text,float x,float baseline,const TextStyle& style){
    GlyphCache* const one[]{&cache};return DrawText(frame,Chain(one),text,x,baseline,style);}
}
