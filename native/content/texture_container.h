#pragma once
#include "../renderer/scene_pass.h"
#include <filesystem>
#include <optional>
#include <span>

namespace ds3content {
struct CpuTextureFloatMetadata {
    uint32_t tag{};
    // Exact TPF words, including nonfinite IEEE values. Interpretation belongs
    // to a selected consumer; the container assigns no HDR meaning or default.
    std::vector<uint32_t> words;
};
struct CpuContainerTexture {
    std::string name;
    uint8_t format_tag{},type{},mip_count{};
    std::vector<uint8_t> dds;
    std::optional<CpuTextureFloatMetadata> float_metadata;
};
// PC TPF and the qualified DS3 DCX/DFLT envelope only. All returned names and
// payloads and optional metadata are owned values. Other encodings/compressions
// fail explicitly. A present empty metadata block differs from an absent block.
std::vector<CpuContainerTexture> DecodeCpuTextureContainer(const std::vector<uint8_t>& bytes);
// I-0074: one entry read in place, with the same validation of the whole container (none when the container has fewer
// entries): its DDS bytes are a view into `bytes`
// (a raw TPF) or into `expanded` (a DCX, inflated there), valid while that storage lives and is unchanged. No container copy.
struct CpuContainerEntryView {
    std::string name;
    uint8_t format_tag{},type{},mip_count{};
    std::span<const uint8_t> dds;
    std::optional<CpuTextureFloatMetadata> float_metadata;
};
std::optional<CpuContainerEntryView> ViewCpuTextureContainerEntry(std::span<const uint8_t> bytes,size_t index,std::vector<uint8_t>& expanded);
}
