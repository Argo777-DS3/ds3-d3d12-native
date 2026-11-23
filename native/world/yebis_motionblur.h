// W10 MB-1 (W10-NATIVE-PASSES 10): the motion blur's draws and constants (mode 2, the engine's object velocity overwriting the
// library's camera velocity), computed natively with the original's single-precision order. The library's stage FUN_1416E21E0:
// the camera velocity FUN_1416E0160 -> FUN_141730F30 -> FUN_141724410 (tech_MotionBlurMakeCameraVelocityOverwrite_<depth channel>
// into the internal velocity surface S+0x140), then the blur FUN_1417301D0 (the planner FUN_14172FBD0 -> FUN_1416D7080, the passes
// FUN_141724840: tech_MotionBlurVelocityDirDepthInnerscreen_SM3_T2_S<taps>). Checked against the captured draws 11-13 of
// Reference1676-1678 (m37, m31, m50) by yebis_motionblur_test.
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include "yebis_glare.h"

namespace ds3native::yebis {
using Matrix4=std::array<float,16>;   // row-major, the library's row-vector convention (v' = v M)
// FUN_1416DD810 on the first three rows (x, y, z; w kept): s = 1 / sqrt((x x + y y) + z z) unless that sum is 0 or 1.
Matrix4 NormaliseRows3(const Matrix4& m) noexcept;
// FUN_1416DD9B0: a b, each term ((b0j a_i0 + b1j a_i1) + b2j a_i2) + b3j a_i3.
Matrix4 Multiply(const Matrix4& a,const Matrix4& b) noexcept;
// FUN_1416FE370: Gauss-Jordan with partial pivoting on the row's columns; false (out untouched) on a zero pivot.
bool Inverse(const Matrix4& m,Matrix4& out) noexcept;
// FUN_1416D0F00(out, 0, 1, x0, x1, y0, y1, near, far): the off-centre perspective the stage builds from the library's camera.
Matrix4 Perspective(float x0,float x1,float y0,float y1,float near_plane,float far_plane) noexcept;

// The library's fields the stage reads (S = [1448E5E38]), set by E69A0's setters: the camera (FUN_1416C9450/9490 near, far, tan of
// half the field of view, FUN_1416C95x0 the poses), the motion blur's setters (A3-A9 of YEBIS-MOTIONBLUR-NATIVE 1.2).
struct MotionBlurCamera {
    float near_plane{},far_plane{};         // +0x564, +0x568 (the previous projection takes them too)
    float tan_half{},aspect{};              // +0x570, +0x328
    std::array<float,2> shift{};            // +0x574, +0x578 (the lens offsets)
    float previous_tan_half{};              // +0x588
    std::array<float,2> previous_shift{};   // +0x58C, +0x590
    Matrix4 current{},previous{};           // +0x594, +0x614: the camera poses (the camera's world matrices)
};
struct MotionBlurInputs {
    uint32_t width{},height{};              // the scene (the blur's source and targets)
    uint32_t velocity_width{},velocity_height{};   // the internal velocity surface S+0x140
    MotionBlurCamera camera;
    float time_ratio{},max_length{};        // +0x9D0 BlurTimeRatio, +0x9D4 MaxBlurLength
    int base_samples{},max_recurrences{};   // +0x9D8 NumBaseSamples (S+0x21694), +0x9DC MaxRecurrences
    bool bidirectional{true};               // +0x9CC (the tap parity S+0x21698 = 2 - bidirectional)
    float interleave{1.0f};                 // +0x9E0 SampleInterleaved
    int capability{2};                      // +0x340 (the recurrence allowance of FUN_1416EADA0)
    float similarity{},depth_threshold{},depth_threshold_far{};   // +0xA00, +0xA04, +0xA08
    std::array<float,2> velocity_scale{};   // +0xA24 (the application velocity's {s, s}, s = MotionBlurScale x factor x MaxBlurLength)
    int depth_channel{};                    // FUN_1416F4FA0 (0: the R value of a depth-format view)
    std::array<float,4> object_decode{1.0f,0.0f,1.0f,-0.0f};   // FUN_1416F4AF0(source, 0, S+0x440, S+0x444): the velocity draw's depth map
    std::array<float,4> scene_decode{1.0f,0.0f,1.0f,-0.0f};    // FUN_1416F4AF0(2, 0, S+0x440, S+0x444): the blur's scene-alpha depth map
};
// The data words the stage reads: [1427CDAB0] 1.1 (no blur at or below), [143D7EE7C] 2 and [143D7F0C8] -1 (the rectangle remap).
struct MotionBlurData {float no_blur{1.1f};};
// The plan: draws[0] the camera velocity (target surface 0, the internal velocity; input -4 the scene depth's plane 0, -6 the
// engine's object velocity), then the blur passes (t0 the scene for the first, the previous pass's surface after; t1 surface 0):
// pass i into surface 1 + i % 2. The library ping-pongs between its slots +0x4C4 and +0x4C0 (the scene) and draws the last pass into
// +0x4C8; each pass reads the previous one's output either way, and output names the last pass's surface, which the renderer copies
// back into the scene (as the depth of field's output).
struct MotionBlurPlan : GlarePlan {
    float near_factor{},far_factor{};       // FUN_1416D12C0 of the current projection: the velocity draw's PerspectiveFactor
    Matrix4 transform{};                    // the previous-clip transform (am44_TransformMatrix[0], words 896-911)
    int base_taps{},passes{},taps{};float step{};   // the planner: N, R, n and step / L
};
// FUN_1416D7080: the passes R, the last pass's taps n and the tap step (pixels at full velocity) for a length L, N base taps, the tap
// parity (1 odd, 2 even), the recurrence cap (0: none) and SampleInterleaved; R = 0 when L / interleave + 1 <= 1.1.
struct MotionBlurPlanned {int passes{},taps{};float step{};};
MotionBlurPlanned PlanMotionBlur(const MotionBlurData& d,float length,int base,int parity,int max_passes,float interleave);
MotionBlurPlan BuildMotionBlur(const MotionBlurData& d,const MotionBlurInputs& in);
}
