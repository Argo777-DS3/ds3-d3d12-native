#pragma once
#include "game_primitive_mesh_source.h"

namespace ds3native::bootstrap {
struct CpuUiMeshOutput {
    CpuPrimitiveVertexFormat format;
    uint32_t vertices{},indices{};
    std::array<uint32_t,2> fills{};
    uint32_t flags{},mesh_ordinal{};
    std::array<float,8> transform{};
    std::vector<uint8_t> vertex_bytes,index_bytes;
};
enum class CpuUiMeshOutputStatus : uint8_t { Ready,Missing,Unsupported,Invalid };
// The real five-method CPU VertexOutput interface. The generator receives our
// destinations; no game mesh/cache fields or original mapped buffers are used.
// Current single-fill triangle data only. Unknown/partial output is refused.
class NativeCpuMeshOutput {
public:
    struct State;
    NativeCpuMeshOutput();
    ~NativeCpuMeshOutput();
    NativeCpuMeshOutput(const NativeCpuMeshOutput&)=delete;
    NativeCpuMeshOutput& operator=(const NativeCpuMeshOutput&)=delete;
    uintptr_t Interface() const noexcept;
    CpuUiMeshOutputStatus Resolve(CpuUiMeshOutput& output) const;
private:
    std::unique_ptr<State> state_;
};
using CpuUiMeshGenerator=void(*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uint32_t);
CpuUiMeshOutputStatus ProduceNativeCpuUiMesh(CpuUiMeshGenerator qualified_cpu_generator,
    uintptr_t receiver,uintptr_t renderer,uintptr_t mesh,uint32_t flags,CpuUiMeshOutput& output);
}
