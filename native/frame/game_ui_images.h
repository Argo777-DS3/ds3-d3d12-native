#pragma once
#include "game_primitive_image_source.h"
#include "game_texture_ingress.h"

namespace ds3native::bootstrap {
struct NativeUiImage {
    uint64_t identity{};
    // The engine address only associates its CPU image with this native owner.
    // Each GPU upload has a separate token retained by immutable consumers.
    std::shared_ptr<uint8_t> upload_token;
    std::shared_ptr<const NativeTextureRecord> gpu;
};
struct NativeUiImages {
    frame::PrimitiveTextureVersion version;
    frame::PrimitiveTextureStore::Lease textures;
    std::unordered_map<uintptr_t,NativeUiImage> images;
};
// Native CPU images, atlas edits and actual fenced D3D12 uploads. No original
// texture/view/device or GPU mapping is an input. Game ABI delivery is separate.
class UiImageIngress {
public:
    using Lease=std::shared_ptr<const NativeUiImages>;
    UiImageIngress(TextureIngress& uploads,uint64_t incarnation);
    ~UiImageIngress(){Close();}
    bool Create(uintptr_t engine_image,frame::PrimitiveTextureVersion expected,uint32_t engine_format,
        std::span<const CpuPrimitiveImageMip> mips);
    bool Import(uintptr_t engine_image,frame::PrimitiveTextureVersion expected,
        std::shared_ptr<const NativeTextureRecord> texture);
    bool UpdateGlyphs(uintptr_t engine_image,frame::PrimitiveTextureVersion expected,
        std::span<const CpuGlyphImageUpdate> updates);
    bool Retire(uintptr_t engine_image,frame::PrimitiveTextureVersion expected);
    Lease Acquire() const;
    void Close();
private:
    bool Publish(uintptr_t engine_image,frame::PrimitiveTextureVersion expected,
        const frame::PrimitiveTexture& image,std::span<const frame::PrimitiveTextureEdit> edits);
    TextureIngress& uploads_;
    mutable std::mutex mutex_;
    Lease current_;
    uint64_t last_identity_{};
};
}
