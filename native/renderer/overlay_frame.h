#pragma once
// I-0132 R12a (R12-PLAN « R12a's design »): what the native overlay draws above the game's UI on one frame: textured quads with a
// colour, from a few images (the glyph atlas, its shadow atlas, DS3's menu pieces). Built on the engine thread by our screens (the
// shader precompile screen and the title screen, R12-PLAN's course change), handed whole to the renderer (OverlayPass), which uploads
// an image of bytes only when its version changes and binds a texture the native already holds as it is. Header-only, no D3D12:
// unit-tested with the text layout (overlay_text_test).
#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace ds3native::overlay {
// One image the quads sample: R8 coverage (the glyphs, the shadow, R12e's ember and cloud), or R8G8B8A8 or BC7 bytes of our own (no
// producer yet: R12e made its images as coverage; their sizes unit-tested), or an external texture (below); `version` changes whenever its bytes do (the
// renderer uploads it again then). The bytes are shared: a frame is a cheap copy.
enum class Format : uint8_t {Coverage8,Rgba8,Bc7};
struct Image {
    uint64_t identity{},version{};
    uint32_t width{},height{};
    Format format{Format::Coverage8};
    std::shared_ptr<const std::vector<uint8_t>> bytes;   // the top mip only, rows packed (a BC7 row: 4 pixel rows of 16-byte blocks)
    // R12a's second tranche: a texture the native already holds (DS3's menu pieces, from its texture ingress), sampled as it is: its
    // resource (an ID3D12Resource*, readable by pixel shaders, never written) and the owner that keeps it alive while a frame names it;
    // `bytes` is then empty and `format` unused (the resource's own format is read).
    void* external{};
    std::shared_ptr<const void> owner;
};
inline uint64_t ImageBytes(uint32_t width,uint32_t height,Format format) noexcept {
    switch(format){case Format::Coverage8:return uint64_t(width)*height;case Format::Rgba8:return uint64_t(width)*height*4;
        default:return uint64_t((width+3)/4)*((height+3)/4)*16;}}
inline bool Valid(const Image& image) noexcept {
    if(image.external)return image.identity && image.owner && image.width && image.height && !image.bytes;
    return image.identity && image.width && image.height && image.width<=8192 && image.height<=8192 && image.bytes &&
        image.bytes->size()==ImageBytes(image.width,image.height,image.format) &&
        (image.format!=Format::Bc7 || ((image.width%4)==0 && (image.height%4)==0));}
// A quad in output pixels (x right, y down), its image's rectangle in texels, its colour (straight RGBA, 0 to 1, the target's
// encoding: DS3's UI colours as its films write them), the image's index in the frame (-1: a plain colour).
struct Quad {
    float x0{},y0{},x1{},y1{};
    float u0{},v0{},u1{},v1{};
    std::array<float,4> colour{1,1,1,1};
    int32_t image{-1};
    // I-0132 R12h (the precompile page's fire with the bonfire's own textures): mode 1, a fire: the image's alpha is a heat field, the
    // second image (sampled repeating, its rectangle in its texels, beyond them to scroll) a noise whose red channel breaks the heat up,
    // the colour from the fire's ramp of the modulated heat times the quad's rgb, the quad's alpha a gain on the output. 0 (the
    // default): the quad as before.
    int32_t image2{-1};
    float u2_0{},v2_0{},u2_1{},v2_1{};
    uint32_t mode{};
};
struct Frame {
    uint32_t width{},height{};   // the output the quads were laid out for (the renderer skips a frame laid out for another size)
    std::vector<Image> images;   // at most kImages
    std::vector<Quad> quads;     // drawn in order, alpha-blended (straight alpha)
    static constexpr size_t kImages=16,kQuads=16384;
};
inline bool Valid(const Frame& frame) noexcept {
    if(!frame.width || !frame.height || frame.images.size()>Frame::kImages || frame.quads.size()>Frame::kQuads)return false;
    for(const auto& image:frame.images)if(!Valid(image))return false;
    for(size_t i=0;i<frame.images.size();++i)for(size_t j=i+1;j<frame.images.size();++j)if(frame.images[i].identity==frame.images[j].identity)return false;
    for(const auto& q:frame.quads)if(q.image>=int32_t(frame.images.size()) || q.image<-1 || q.image2>=int32_t(frame.images.size()) || q.image2<-1 || q.mode>1 || (q.mode==1 && (q.image<0 || q.image2<0)))return false;
    return true;}
}
