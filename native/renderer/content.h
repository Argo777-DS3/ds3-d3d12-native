#pragma once
#include <cstdint>
#include <span>
#include <vector>

namespace ds3native {

// Offsets are byte offsets into the model's source bytes. Vertex indices are
// local to the slice's vertex range and index data is little-endian R16.
struct MeshSlice {
    uint32_t vertex_offset{};
    uint32_t vertex_count{};
    uint32_t stride{};
    uint32_t index_offset{};
    uint32_t index_count{};
    uint32_t material_id{};
    // 1063: second vertex stream of a two-buffer FLVER mesh (0 = one stream);
    // same vertex count, input slot 1.
    uint32_t vertex_offset2{};
    uint32_t stride2{};
    // 1184: the selected face set's culling class (face set +0x0C: 0 none, 1 front, 2 back (measured, Native1184); FLVER CullBackfaces,
    // read by 140064610 through the table 14473C9F8; MENUS-MAP 64).
    uint32_t cull_class{1};
    // 1202: index width in bytes, 2 (R16_UINT, face set format 0x39) or 4 (R32_UINT, 0x2A); 1FA030 picks it from the FLVER
    // face set's index size (16 -> 0x39, otherwise 0x2A) and stores the view start in elements of that width.
    uint32_t index_width{2};
    bool operator==(const MeshSlice&) const=default;   // I-0117 T2: a skinned instance's slices compared walk to walk
};

// Supported input: float3 POSITION at byte 0 of each vertex, followed by an
// R16 or R32 triangle-list index slice for each mesh.
constexpr size_t kMaximumContentMeshes=1024;
// I-0097: the checks on bytes and slices held elsewhere (a model storage's), without a copy of the whole source.
bool ValidContent(std::span<const uint8_t> source,std::span<const MeshSlice> meshes) noexcept;

} // namespace ds3native
