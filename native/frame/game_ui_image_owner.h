#pragma once
#include "game_ui_bindings.h"
#include <functional>

namespace ds3native::bootstrap {
// Real engine CPU manager/image prefixes with native GPU callbacks. The first
// admitted families are the witnessed mode1 R8 glyph source and the actual
// CPU RGBA gradient decoder. Other image families remain unsupported.
class GameUiImageOwner {
public:
    struct State;
    using Failure=void(*)(const char*) noexcept;
    GameUiImageOwner(uintptr_t original_module,TextureIngress& uploads,DeviceQueues& gpu,
        std::mutex& submissions,uint64_t incarnation,Failure failure);
    ~GameUiImageOwner();
    GameUiImageOwner(const GameUiImageOwner&)=delete;
    GameUiImageOwner& operator=(const GameUiImageOwner&)=delete;
    void* ConstructManager();
    // 0 when the image is not taken; `not_taken` then says why, for the caller's choice between a skip and a refusal: the data's
    // ("format": a cube, an array or a format the UI does not sample; "set_full": the set's PrimitiveTextureLimit images) or
    // the module's own ("owner_closed", "texture_not_resident", "set_refused").
    uintptr_t ImportTexture(std::shared_ptr<const NativeTextureRecord> texture,const char** not_taken=nullptr);
    void ConsumeCpuPending();
    UiImageIngress::Lease Acquire() const;
    // Execute one genuine current CPU prepare while native image +68 delivers
    // owned texture/sampler intent. Synchronous, nested/foreign owners refused.
    bool PrepareBindings(uint64_t serial,uint64_t draw_identity,const std::function<void()>& prepare,UiDrawImageBinding& output);
    void Close();
private:
    std::shared_ptr<State> state_;
    void* manager_{}; // One actual engine CPU ref owned by this class.
};
}
