#pragma once
#include "texture_container.h"

namespace ds3content {
struct CpuTextureSubresource {
    uint32_t width{},height{},row_bytes{},rows{};
    std::vector<uint8_t> bytes;
};
struct CpuTextureResource {
    std::string name;
    uint8_t tpf_format_tag{};
    uint32_t width{},height{},mip_levels{},array_slices{};
    DXGI_FORMAT format{};
    bool cube{};
    // Face/array-major, then mip. Format is the DDS-declared interpretation;
    // no original engine SRGB/view policy is inferred from a TPF format tag.
    std::vector<CpuTextureSubresource> subresources;
    // Owned with this resource and copied with its content generation. No
    // shader binding or semantic interpretation follows from its presence.
    std::optional<CpuTextureFloatMetadata> float_metadata;
};
// Decodes owned file content into native allocation/upload descriptions.
// This is asset-format admission, not a legacy graphics call translation.
// Unsupported resource families fail; no synthetic replacement is returned.
CpuTextureResource DecodeNativeTextureDDS(const CpuContainerTexture& texture);
// I-0074: the same decode from an entry read in place (its DDS bytes are copied once, into the subresources).
CpuTextureResource DecodeNativeTextureDDS(const CpuContainerEntryView& texture);
// Exact physical extent for the engine's pointer-only legacy DDS source.
// DX10 arrays require an explicit producer extent and remain unsupported here.
size_t NativeLegacyDdsByteCount(const std::vector<uint8_t>& header);
bool ValidCpuTextureResource(const CpuTextureResource& texture) noexcept;
}
