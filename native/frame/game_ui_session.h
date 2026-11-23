#pragma once
#include <optional>
#include <string>
#include <vector>
#include "game_ui_image_owner.h"
#include "game_ui_mesh_owner.h"
#include "game_ui_shaders.h"

namespace ds3native::bootstrap {
// Persistent engine-facing CPU HAL and loader backed by native image/mesh owners.
// Rendering callbacks are added at their actual consumer; unknown methods refuse.
class GameUiSession {
public:
    struct State;
    using Failure=void(*)(const char*) noexcept;
    GameUiSession(uintptr_t module,uintptr_t manager,TextureIngress& textures,
        DeviceQueues& gpu,std::mutex& submissions,uint64_t incarnation,Failure failure);
    ~GameUiSession();
    GameUiSession(const GameUiSession&)=delete;
    GameUiSession& operator=(const GameUiSession&)=delete;
    uintptr_t Hal() const noexcept;
    uintptr_t Loader() const noexcept;
    uintptr_t TextureManager();
    uintptr_t ImportTexture(std::shared_ptr<const NativeTextureRecord> texture,const char** not_taken=nullptr);   // GameUiImageOwner::ImportTexture
    // Actual CPU movie captures in authored order. GPU submission remains a
    // separate native responsibility; unknown render callbacks still refuse.
    uint32_t PrepareFrame(const frame::PrimitiveProgramBank& programs,uint64_t serial,uint32_t width,uint32_t height);
    const frame::PrimitiveWorkPacket* PreparedFrame() const noexcept;
    std::optional<frame::PrimitiveWorkPacket> TakePreparedFrame() noexcept;   // the prepared frame, moved out (none after)
    // I-0067 SCALEFORM-FILTERS-NATIVE: JSON trace rows (filters first seen: plan, fields; sampled frame counts).
    std::vector<std::string> TakeFilterReports();
    // D-0167 measurement (measure runs only, SetProfiling): QPC ticks in PrepareFrame as a whole, in the movies' display
    // calls (the game's Scaleform CPU work and the native HAL callbacks under them), in the draw submissions (shader
    // uniforms and primitive packets of each UI draw), in the final frame packet, the submissions and the calls, taken and reset.
    struct Profile {uint64_t frame_ticks{},display_ticks{},submit_ticks{},packet_ticks{},submissions{},calls{},shader_ticks{},primitive_ticks{},image_bytes{},reused_image_bytes{};};   // shader: uniforms and image bindings, primitive: the draw packets; image bytes taken by draw packets, those already in the frame
    static void SetProfiling(bool enabled) noexcept;
    // W1 F8a: cacheAsBitmap primitives (an empty FilterSet) drawn through a target and the cached quad (opt-in, ui-cache.flag,
    // until its page is witnessed); off keeps the former path (the subtree drawn in place).
    static void SetUiCache(bool enabled) noexcept;
    static Profile TakeProfile() noexcept;
    void Close();
private:
    std::unique_ptr<State> state_;
};
}
