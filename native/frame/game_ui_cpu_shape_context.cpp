#include "game_ui_cpu_shape_context.h"
#include "../content/acquire.h"   // C7-0 rule: in-process field reads through the installed guarded copy
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace ds3native::bootstrap {
namespace {
template<class T>T Load(uintptr_t p){T result{};
    if(!p || !ds3content::Read(p,&result,sizeof(result)))
        throw std::runtime_error("native shape CPU field unreadable");return result;
}
constexpr size_t ScratchBytes=0xAA8;
void Validate(const CpuUiShapeTolerances& policy){
    for(size_t n=0;n<18;++n){float value{};std::memcpy(&value,&policy.words[n],4);
        if(!std::isfinite(value) || value<0)throw std::invalid_argument("native CPU shape tolerance scalar");
    }
    if(!policy.words[18] || policy.words[18]>256)throw std::invalid_argument("native CPU shape curve limit");
}
}
struct GameUiCpuShapeContext::State {
    struct CpuInterface {uintptr_t table;State* owner;std::array<uint8_t,0x50> reserved;CpuUiShapeTolerances policy;} cpu{};
    static_assert(offsetof(CpuInterface,policy)==0x60);
    std::array<uintptr_t,31> table{};
    uintptr_t module{},heap{},scratch{},free{};Failure failure{};
    std::thread::id thread{std::this_thread::get_id()};
    bool initialized{},closed{};
    [[noreturn]] void Fail(const char* why) const noexcept {failure(why);std::terminate();}
};
namespace {
auto& Owner(uintptr_t object){return *reinterpret_cast<GameUiCpuShapeContext::State::CpuInterface*>(object)->owner;}
[[noreturn]] void Unsupported(uintptr_t object){Owner(object).Fail("unqualified_native_cpu_shape_context_method");}
uintptr_t Scratch(uintptr_t object){auto& state=Owner(object);
    if(state.closed || state.thread!=std::this_thread::get_id() || !state.initialized || !state.scratch)
        state.Fail("native_cpu_shape_context_scope");return state.scratch;
}
}
GameUiCpuShapeContext::GameUiCpuShapeContext(uintptr_t module,const CpuUiShapeTolerances& policy,Failure failure){
    if(!module || !failure)throw std::invalid_argument("native CPU shape services");Validate(policy);
    state_=std::make_unique<State>();auto& state=*state_;state.module=module;state.failure=failure;
    state.table.fill(reinterpret_cast<uintptr_t>(&Unsupported));state.table[0xF0/8]=reinterpret_cast<uintptr_t>(&Scratch);
    state.cpu.table=reinterpret_cast<uintptr_t>(state.table.data());state.cpu.owner=&state;state.cpu.policy=policy;
    state.heap=Load<uintptr_t>(module+0x4936CC0);const auto heap_table=Load<uintptr_t>(state.heap);
    const auto allocate=Load<uintptr_t>(heap_table+0x50);state.free=Load<uintptr_t>(heap_table+0x60);
    const uint32_t hint=0x48;
    state.scratch=reinterpret_cast<uintptr_t(*)(uintptr_t,size_t,const uint32_t*)>(allocate)(state.heap,ScratchBytes,&hint);
    if(!state.scratch)throw std::bad_alloc();std::memset(reinterpret_cast<void*>(state.scratch),0,ScratchBytes);
    // Genuine CPU tessellator/stroker pools/math constructors; no HAL, font
    // image cache, original device or shader wrapper is constructed here.
    reinterpret_cast<uintptr_t(*)(uintptr_t,uintptr_t)>(module+0x1BD8E70)(state.scratch,state.heap);
    state.initialized=true;
}
GameUiCpuShapeContext::~GameUiCpuShapeContext(){Close();}
uintptr_t GameUiCpuShapeContext::Interface() const noexcept {return state_ && !state_->closed?reinterpret_cast<uintptr_t>(&state_->cpu):0;}
void GameUiCpuShapeContext::Close(){if(!state_ || state_->closed)return;auto& state=*state_;
    if(state.thread!=std::this_thread::get_id())state.Fail("native_cpu_shape_retirement_thread");
    if(state.scratch){
        //1BD9570's scratch retirement consists of these four actual CPU page
        // pools. Their math/array views borrow the pools and own no extra refs.
        // Glyph/cache/queue destructors are irrelevant to this native context.
        if(state.initialized)for(size_t n=4;n>0;--n)
            reinterpret_cast<void(*)(uintptr_t)>(state.module+0x1BD5F10)(state.scratch+(n-1)*0x28);
        reinterpret_cast<void(*)(uintptr_t,uintptr_t)>(state.free)(state.heap,state.scratch);state.scratch=0;
    }
    state.closed=true;
}
}
