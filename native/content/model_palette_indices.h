#pragma once
#include "model_vertex_layout.h"
#include "shader_bank.h"
#include <array>
#include <span>

namespace ds3content {
enum class ModelPaletteIndexStatus { InRange, Unsupported, Invalid, OutOfRange, Empty };
struct ModelPaletteIndexInputs {
    std::span<const ModelVertexElement> elements;
    std::span<const uint8_t> vertices;
    uint32_t vertex_count{},vertex_stride{},palette_base{},object_rows{},cancel_rows{};
};
struct ModelPaletteIndexResult {
    ModelPaletteIndexStatus status{ModelPaletteIndexStatus::Invalid};
    uint64_t vertices_scanned{},indices_scanned{};
    uint32_t max_index{},max_object_index{};
    // Same owned bytes used by internal interface inspection and complete
    // instruction-pattern matching. Retain them with the selected program.
    std::vector<uint8_t> shader_bytecode;
    std::array<char,65> shader_sha256{};
};

// Bounded full-instruction patterns of the two actual selected VS families815.
// No id/hash allowlist, borrowed signature or whole-program interpreter. Only
// BLENDINDICES0/v3 UINT4, slot0/pervertex/step0, exact full CPU vertex slices.
// All four indices are checked even with zero weights: b12 reads index*3+0..2,
// b5 reads (index+explicit palette_base)*3+0..2. Entire slices are conservative;
// an unused bad vertex can refuse a draw whose indexed subset would be safe.
// InRange establishes index boundaries only, never palette kind, actual pose
// availability, constant coverage, geometry/shader binding or draw readiness.
// Caller owns current snapshots and binds this exact inspected bytecode version.
ModelPaletteIndexResult ValidateModelPaletteIndices(const CpuShaderProgram& program,
    const ModelPaletteIndexInputs& inputs) noexcept;
}
