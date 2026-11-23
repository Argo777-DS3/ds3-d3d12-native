#pragma once
#include "game_ui_mesh_output.h"

namespace ds3native::bootstrap {
// Actual CPU tolerance layout from1BD8190/1BDB340: eighteen scalar floats,
// then the authored curve limit. These are explicit CPU renderer-policy inputs.
struct CpuUiShapeTolerances {std::array<uint32_t,19> words;};
static_assert(sizeof(CpuUiShapeTolerances)==76);
// The qualified normal-fill generator consumes only CPU tolerance+60 and
// virtualF0 scratch. Own those two contracts directly; unrelated HAL/glyph/GPU
// construction and legacy readiness are not prerequisites for this algorithm.
class GameUiCpuShapeContext {
public:
    struct State;
    using Failure=void(*)(const char*) noexcept;
    GameUiCpuShapeContext(uintptr_t original_module,const CpuUiShapeTolerances& tolerances,Failure failure);
    ~GameUiCpuShapeContext();
    GameUiCpuShapeContext(const GameUiCpuShapeContext&)=delete;
    GameUiCpuShapeContext& operator=(const GameUiCpuShapeContext&)=delete;
    uintptr_t Interface() const noexcept;
    void Close();
private:
    std::unique_ptr<State> state_;
};
}
