#pragma once
#include "../renderer/scene_view.h"

namespace ds3native::ingress {
// Existing I-0042 camera producer, connected to the owned startup camera.
// Only marked words are produced; the other cbSceneParam families remain open.
struct SceneCameraConstants {
    uint64_t serial{};
    std::array<uint8_t,960> bytes{};
    std::array<uint8_t,240> written{};
};
bool ProduceCameraConstants(const SceneViewInput& input,SceneCameraConstants& output) noexcept;
struct SceneViewConstants {
    uint64_t serial{};
    std::array<uint8_t,128> bytes{};
};
// Reuse the camera producer's exact projection, then ProduceViewMatrices.
// The unproduced cbMatrixWVP frustum-plane tail is not marked ready.
bool ProduceCameraViewConstants(const SceneViewInput& input,const SceneCameraConstants& camera,
    SceneViewConstants& output) noexcept;
struct RigidPlacement {
    std::array<float,12> shader_rows{};
    uint64_t model{},matrix_record{},matrix_source{};
    uint32_t active{};
};
// Called synchronously at the selected C0E0 CPU input. The renderer receives
// the copied rows, never these diagnostic source identities or engine pointers.
// Version1 admits the acquired single-matrix rigid model only.
bool CopyRigidPlacement(uintptr_t c0e0_inputs,uintptr_t expected_model,RigidPlacement& output) noexcept;
// Called after the accepted camera producer returns in its known caller.
// Copies the initialized pose/lens fields; unsupported projections are refused.
bool CopySceneView(uintptr_t receiver,uintptr_t camera_output,uintptr_t caller,
    uintptr_t image,uint64_t serial,SceneViewInput& output) noexcept;
// Owned scene CPU update invokes the genuine producer directly. This entry
// validates its actual receiver and produced bytes without inventing a caller.
bool CopyProducedSceneView(uintptr_t receiver,uintptr_t camera_output,
    uintptr_t image,uint64_t serial,SceneViewInput& output) noexcept;
}
