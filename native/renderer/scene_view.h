#pragma once
#include <array>
#include <cstdint>

namespace ds3native {
using Matrix4 = std::array<float,16>;

// Values owned by the native scene. Row-vector matrices; no engine addresses.
// Version1 supports the observed default finite-perspective branch only.
struct SceneViewInput {
    uint32_t version{1};
    uint64_t serial{};
    Matrix4 camera_pose{};
    float vertical_fov{}, near_plane{}, far_plane{}, explicit_aspect{};
    uint32_t width{}, height{}, projection_flags{}, tile_count{};
    // I-0111 (R1): the temporal upscaler's sub-pixel jitter of this view's rasterisation, in render pixels (+x right, +y down);
    // the camera producer puts it in the projection words (ProduceCameraConstants). BuildSceneView's matrices stay unjittered.
    float jitter_x{}, jitter_y{};
};
struct SceneView {
    uint64_t serial{};
    Matrix4 view{}, projection{}, view_projection{}, camera_relative_view_projection{};
    std::array<float,4> camera_position{};
    uint32_t width{}, height{};
};

// Failure leaves output unchanged. Unsupported lens branches are refused.
bool BuildSceneView(const SceneViewInput& input, SceneView& output) noexcept;
Matrix4 Multiply(const Matrix4& left,const Matrix4& right) noexcept;
Matrix4 Transpose(const Matrix4& input) noexcept;
std::array<float,4> Transform(const std::array<float,4>& row,const Matrix4& matrix) noexcept;
}
