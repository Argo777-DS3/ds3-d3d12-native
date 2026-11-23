#pragma once
#include "game_primitive_geometry.h"
#include "game_ui_cpu_shape_context.h"

namespace ds3native::bootstrap {
struct UiMeshCacheConfiguration {
    std::array<uint64_t,6> bytes;
    std::array<uint32_t,5> limits;
};
static_assert(sizeof(UiMeshCacheConfiguration)==72); //68 authored bytes, tail alignment.
// Genuine engine CPU cache/item prefixes and mesh associations, with native
// CPU allocation/map/seal/retirement. No original GPU buffer/fence wrapper.
// Its bounded whole writer generates normal CPU shapes independently. It does
// not initialize a HAL, run unknown mesh providers or claim GPU readiness;
// preparation and complete native packets remain separate.
class GameUiMeshOwner {
public:
    struct State;
    using Failure=void(*)(const char*) noexcept;
    GameUiMeshOwner(uintptr_t original_module,Failure failure);
    ~GameUiMeshOwner();
    GameUiMeshOwner(const GameUiMeshOwner&)=delete;
    GameUiMeshOwner& operator=(const GameUiMeshOwner&)=delete;
    // Bind a genuine, newly constructed empty CPU cache embedded in the native
    // HAL. Caller owns its storage/lifetime; no tail fields or GPU owner enter.
    void* AdoptCache(void* constructed_cpu_cache);
    // Explicit current CPU policy, independent of an original HAL instance.
    void SetShapeTolerances(const CpuUiShapeTolerances& tolerances);
    std::shared_ptr<const PrimitiveGeometry> Acquire(uintptr_t item) const;
    void Close();
private:
    std::shared_ptr<State> state_;
};
// I-0137 (I-0120 G5): no bound on the live items; instead this witness, when set, is called the first time a cache's live items reach a
// power of two from 4,096 up (the bootstrap writes a journal row): a growth without end shows in a session's journal.
void SetUiMeshItemsWitness(void(*witness)(size_t live)) noexcept;
}
