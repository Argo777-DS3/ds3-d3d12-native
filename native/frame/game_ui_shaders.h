#pragma once
#include "primitive_constants.h"
#include <functional>
#include <thread>

namespace ds3native::bootstrap {
struct UiCpuPrepareInputs {
    uintptr_t shader_manager{},fill{},vertex_format{},view{},mesh_nodes{};
    uint32_t flags{},draw_type{},meshes{};
    uintptr_t texture_matrices{};
    uint32_t texture_matrix_count{};
};
struct UiCpuPreparedShader {
    uint64_t serial{},draw_identity{};
    uint32_t flags{},uniform_writes{};
    frame::PrimitiveProgramPair programs;
    frame::PrimitiveDrawConstants values;
    std::vector<SceneConstantBinding> constants;
};
using UiCpuPrepare=uintptr_t(*)(uintptr_t,uintptr_t,uint32_t*,uint32_t,uintptr_t,uint32_t,uintptr_t,uintptr_t,uintptr_t);
using UiCpuFailure=void(*)(const char*) noexcept;
// Stateless CPU profile methods qualified at3AD6690. Own the policy interface
// and manager+28 prefix directly; no old HAL/huge shader manager is constructed.
class GameUiCpuShaderPolicy {
public:
    GameUiCpuShaderPolicy(uintptr_t module,UiCpuFailure failure);
    GameUiCpuShaderPolicy(const GameUiCpuShaderPolicy&)=delete;
    GameUiCpuShaderPolicy& operator=(const GameUiCpuShaderPolicy&)=delete;
    uintptr_t Manager() const;
    // I-0088 P1: the frame's preparing thread takes the policy at the frame's start (the render step's, or the UI display
    // worker's: one at a time, under the module's UI preparation lock); Manager refuses any other thread inside the frame.
    void Adopt() noexcept {thread_=std::this_thread::get_id();}
private:
    struct Profile {uintptr_t table{};UiCpuFailure failure{};} profile_;
    std::array<uintptr_t,14> table_{};
    std::array<uintptr_t,6> manager_{};
    std::thread::id thread_{std::this_thread::get_id()};
    static void Unsupported(uintptr_t profile) noexcept;
};
// Synchronous CPU shader-interface callbacks for the qualified1F784C0 algorithm.
// Caller must own/qualify current CPU manager/profile/fill/mesh/view dispatch and
// supply native images under GameUiImageOwner::PrepareBindings when textured.
// Current2D single-mesh path only; no old shader/input-layout/CB wrapper exists.
// Selection's legacy GPU-ready result stays false. This function returns owned
// CPU preparation separately; only actual PrimitivePass PSOs can qualify GPU work.
frame::PrimitiveConstantStatus PrepareNativeUiCpu(UiCpuPrepare producer,const frame::PrimitiveProgramBank& bank,
    frame::PrimitiveProgramVersion current,uint32_t mode,uint64_t serial,uint64_t draw_identity,
    const UiCpuPrepareInputs& inputs,UiCpuPreparedShader& output,UiCpuFailure failure,const char** data_case=nullptr);
// I-0137 tranche 5: `data_case`, when given, names the case of the game's data behind a non-Ready answer (a pair whose uniform
// layout the native values do not take); null for every other cause (the native's own
// constants, a 3D view, a contract), which the caller still treats as a failure (its integrity review's H1).
// I-0067 SCALEFORM-FILTERS-NATIVE F5: one Scaleform filter pass. The pass programme `selector` (a 1F75440 key) is
// selected through the same CPU shader-interface object, then `writes(writer,metadata)` runs the original CPU uniform
// writes (1F78AD0, 1F770A0, 1F81EB0) against it; their values are owned and packed here. The original GPU commit
// (1F80640) is never run. Mode-3 programmes only.
frame::PrimitiveConstantStatus PrepareNativeUiFilterCpu(const frame::PrimitiveProgramBank& bank,frame::PrimitiveProgramVersion current,
    uint64_t serial,uint64_t draw_identity,uint32_t selector,const std::function<void(uintptr_t writer,uintptr_t metadata)>& writes,
    UiCpuPreparedShader& output,UiCpuFailure failure);
}
