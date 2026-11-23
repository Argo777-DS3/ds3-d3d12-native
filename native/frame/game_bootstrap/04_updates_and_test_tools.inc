// Reference and diagnostic witnesses, first part (observation hooks behind the Reference mode or a *-observe flag) and the
// test tools: character, model and pose updates, the witness teleport, phantom and flag files, the test tools (invincibility,
// revive, no AI, death), the game step and the world update gate. Part of game_bootstrap.cpp's single translation unit (the order is there): it relies on every earlier part
// (03_engine_cpu_owners.inc before it).
#pragma once
#include "03_engine_cpu_owners.inc"
namespace {
void __fastcall ObserveNativeAsmUpdate(uintptr_t entity,uintptr_t context,uintptr_t third) noexcept {
    static LONG64 calls{};const auto n=InterlockedIncrement64(&calls);
    if(n<=12 || n%60==0){float weight{};uint32_t parts{},state{};Read(context+8,&weight,4);Read(entity+0xAF8,&parts,4);Read(entity+0xBB0,&state,4);
        char row[320]{};sprintf_s(row,"{\"event\":\"native_asm_update\",\"call\":%lld,\"entity\":%llu,\"weight\":%g,\"parts\":%u,\"state\":%u,\"third\":%s,\"caller\":%llu}\n",
            n,static_cast<unsigned long long>(entity),weight,parts,state,third?"true":"false",
            static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(_ReturnAddress())-image));Log(row);}
    asm_update_original(entity,context,third);
}
void ObserveAsmModelUpdate(const char* kind,uintptr_t model,float weight,uintptr_t caller) noexcept {
    static LONG64 calls{};const auto n=InterlockedIncrement64(&calls);
    if(n<=16 || n%60==0){uintptr_t entity{};Read(model+8,&entity,8);
        char row[320]{};sprintf_s(row,"{\"event\":\"native_asm_model_update\",\"kind\":\"%s\",\"call\":%lld,\"model\":%llu,\"entity\":%llu,\"weight\":%g,\"caller\":%llu}\n",
            kind,n,static_cast<unsigned long long>(model),static_cast<unsigned long long>(entity),weight,static_cast<unsigned long long>(caller-image));Log(row);}
}
void __fastcall ObserveChrAsmModelUpdate(uintptr_t model,float weight,uintptr_t third,uintptr_t fourth) noexcept {
    ObserveAsmModelUpdate("chr",model,weight,reinterpret_cast<uintptr_t>(_ReturnAddress()));chr_asm_model_update_original(model,weight,third,fourth);}
void __fastcall ObserveSprjAsmModelUpdate(uintptr_t model,float weight,uintptr_t third,uintptr_t fourth) noexcept {
    ObserveAsmModelUpdate("sprj",model,weight,reinterpret_cast<uintptr_t>(_ReturnAddress()));sprj_asm_model_update_original(model,weight,third,fourth);}
void __fastcall ObservePlayerModelUpdate(uintptr_t chr,float dt) noexcept {
    static LONG64 calls{};const auto n=InterlockedIncrement64(&calls);
    if(n<=16 || n%120==0){uintptr_t model{};Read(chr+0x20B0,&model,8);
        // 1077: the whole return chain (module RVAs) identifies the task group that runs it.
        void* frames[28]{};const auto depth=RtlCaptureStackBackTrace(0,28,frames,nullptr);std::string stack;
        for(USHORT k=0;k<depth;++k){const auto at=reinterpret_cast<uintptr_t>(frames[k]);char item[24]{};
            if(at>=image && at<image+0x6000000)sprintf_s(item,"%s%llu",stack.empty()?"":",",static_cast<unsigned long long>(at-image));
            else sprintf_s(item,"%s-1",stack.empty()?"":",");stack+=item;}
        // 1078: when entered from the task dispatcher (return 23C20AF, call [rax+10] on the
        // delegate at [rsp+40]), name the delegate: vtable RVA, its +10 target, first fields.
        std::string task;
        if(reinterpret_cast<uintptr_t>(_ReturnAddress())-image==0x23C20AF){
            const auto caller_rsp=reinterpret_cast<uintptr_t>(_AddressOfReturnAddress())+8;uintptr_t functor{},table{},method{};std::array<uintptr_t,6> fields{};
            if(Read(caller_rsp+0x40,&functor,8) && functor && Read(functor,&table,8) && table && Read(table+0x10,&method,8)){
                Read(functor,fields.data(),sizeof(fields));char item[400]{};
                // 1079: SprjEzTaskProxy (29BA2D8) forwards to [proxy+10]->vtable+10: name that EzTask.
                uintptr_t inner{},inner_table{},inner_method{},inner_owner{};
                if(Read(functor+0x10,&inner,8) && inner && Read(inner,&inner_table,8) && inner_table)Read(inner_table+0x10,&inner_method,8);
                if(inner)Read(inner+8,&inner_owner,8);
                sprintf_s(item,",\"task_vtable\":%lld,\"task_method\":%lld,\"task_is_chr\":%s,\"task_owner_is_chr\":%s,\"delegate_vtable\":%lld,\"delegate_method\":%lld,\"fields\":[%lld,%lld,%lld,%lld,%lld]",
                    static_cast<long long>(inner_table?inner_table-image:0),static_cast<long long>(inner_method?inner_method-image:0),
                    inner==chr?"true":"false",inner_owner==chr?"true":"false",
                    static_cast<long long>(table-image),static_cast<long long>(method-image),
                    static_cast<long long>(fields[1]>=image && fields[1]<image+0x6000000?fields[1]-image:-1),
                    static_cast<long long>(fields[2]>=image && fields[2]<image+0x6000000?fields[2]-image:-1),
                    static_cast<long long>(fields[3]>=image && fields[3]<image+0x6000000?fields[3]-image:-1),
                    static_cast<long long>(fields[4]>=image && fields[4]<image+0x6000000?fields[4]-image:-1),
                    static_cast<long long>(fields[5]>=image && fields[5]<image+0x6000000?fields[5]-image:-1));task=item;}}
        char row[1024]{};
        sprintf_s(row,"{\"event\":\"native_player_model_update\",\"call\":%lld,\"chr\":%llu,\"model\":%llu,\"dt\":%g,\"caller\":%llu,\"stack\":[%.600s]%s}\n",
            n,static_cast<unsigned long long>(chr),static_cast<unsigned long long>(model),dt,
            static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(_ReturnAddress())-image),stack.c_str(),task.c_str());Log(row);}
    player_model_update_original(chr,dt);}
void LogChrEzTask(const char* what,uintptr_t task,uint64_t step,uintptr_t caller) noexcept {
    static const bool witness=ModuleFlag(L"pose-observe.flag");if(!witness)return;   // review: the task tally (mutex and map per task) only when observed
    uintptr_t table{},action{},owner{};int32_t armed{};
    if(!Read(task,&table,8) || table!=image+0x284F440)return;
    Read(task+0x28,&action,8);Read(task+0x20,&owner,8);Read(task+0x18,&armed,4);
    static std::mutex lock;static std::map<uintptr_t,uint64_t> counts;uint64_t n{};
    {const std::lock_guard hold(lock);n=++counts[(action<<1)^(what[0]==0x72?1:0)];}
    if(n<=3 || n%600==0){char row[320]{};sprintf_s(row,"{\"event\":\"native_chr_eztask\",\"what\":\"%s\",\"count\":%llu,\"action\":%lld,\"owner\":%llu,\"armed\":%d,\"step\":%llu,\"caller\":%lld}\n",
        what,n,static_cast<long long>(action>=image && action<image+0x6000000?action-image:-1),static_cast<unsigned long long>(owner),armed,step,
        static_cast<long long>(caller-image));Log(row);}
}
void __fastcall ObserveChrEzTaskExecute(uintptr_t task) noexcept {
    LogChrEzTask("execute",task,0,reinterpret_cast<uintptr_t>(_ReturnAddress()));chr_eztask_execute_original(task);}
void __fastcall ObserveEzTaskRegister(uintptr_t task,uint64_t step,uint64_t group) noexcept {
    LogChrEzTask("register",task,step,reinterpret_cast<uintptr_t>(_ReturnAddress()));eztask_register_original(task,step,group);}
void __fastcall ObserveChrSchedule(uintptr_t chr,int32_t mode) noexcept {
    uintptr_t manager{},player{};
    if(Read(image+0x477FDB8,&manager,8) && manager && Read(manager+0x80,&player,8) && player==chr){
        static LONG64 calls{};static int32_t last_mode{-99};const auto n=InterlockedIncrement64(&calls);
        if(n<=6 || n%300==0 || mode!=last_mode){last_mode=mode;uintptr_t chr_control{};uint8_t flags{},sleep{};int32_t state{},interval{};
            Read(chr+0x18,&chr_control,8);if(chr_control){Read(chr_control+0x20,&flags,1);Read(chr_control+0x10,&state,4);}
            Read(chr+0x110,&interval,4);Read(chr+0x1EE9,&sleep,1);
            char row[320]{};sprintf_s(row,"{\"event\":\"native_player_schedule\",\"call\":%lld,\"mode\":%d,\"control_flags\":%u,\"control_state\":%d,\"interval\":%d,\"flags_1ee9\":%u,\"caller\":%lld}\n",
                n,mode,flags,state,interval,sleep,static_cast<long long>(reinterpret_cast<uintptr_t>(_ReturnAddress())-image));Log(row);}}
    chr_schedule_original(chr,mode);}
// 1207 (test tooling, both modes): witness positions away from the save's start (NPC-FAMILIES-STATIC §7 stops). The pilot
// writes "x y z" to witness-teleport.txt next to this module; on the game thread (WorldChrMan schedule 8DD2E0) the player's
// physics position ([[[manager+0x80]+0x1F90]+0x68]+0x80, read by 9D9650) takes it and keeps it for the optional 4th value
// in seconds (default 8); the file is removed. The target's collision is not enabled while the player is airborne
// (Native1207-1209 fell through it after short holds), so witnesses hold for their whole stay (Native1210 onward).
bool WriteWitnessPosition(uintptr_t at,const float* xyz) noexcept {
    const bool outer=guarded_copy_active;guarded_copy_active=true;bool ok=true;   // I-0082: an expected fault, not a crash
    __try{std::memcpy(reinterpret_cast<void*>(at),xyz,12);}__except(EXCEPTION_EXECUTE_HANDLER){ok=false;}
    guarded_copy_active=outer;return ok;
}
bool WriteWitnessWord(uintptr_t at,int32_t value) noexcept {
    const bool outer=guarded_copy_active;guarded_copy_active=true;bool ok=true;
    __try{*reinterpret_cast<volatile int32_t*>(at)=value;}__except(EXCEPTION_EXECUTE_HANDLER){ok=false;}
    guarded_copy_active=outer;return ok;
}
// I-0114: the logic's own time (the sum of the dt DS3 hands its systems, BeginFrame), for the witness rows of the unlocked rate.
double native_logic_seconds{};
// I-0114 (series 15: with the follow camera stepped at 60 the 240 path is 60's): the follow camera's logic at a fixed 1/60, its
// view interpolated for the display. The follow camera's update 140515790(this, dt, chr, world; void, four arguments,
// GhidraMCP) keeps its state in this [0x10, 0x3D8) and [0x430, 0x440) (0x440 bytes; [0x3D8, 0x430) is an embedded fade slot of
// heap lists, never copied; the study thread, CAMERA-FOLLOW-SITES §13 on main: no other write outside this but through that
// slot). Each call at an unlocked rate: the state of the last tick (S60) restored, with any 4-byte word written by others since
// our last return carried into it; one original call with dt = 1/60 for each 1/60 owed (collision and fade as the original's,
// on the ticks only), S60 saved after it; then the outputs the copies read (the matrix +0x10..0x4F and the lens +0x50..0x5C,
// 1405103E0's copy into ChrCam, then the render's) written as the blend of the last two ticks at owed / (1/60): the display
// runs up to one tick (16.7 ms) behind, and no law runs on a display frame (step review M1). The heading's target
// (StableViewCopy) reads S60's forward while ChrCam follows this camera. While no state is kept, a dt within k's
// tolerance of 1/60 is the original's call (bits as at 60); once one is, every dt steps the ticks (final review H1). The object's destructor 1405130F0 forgets it
// (L1). One caller thread assumed: the ChrCam update of MoveMapStep's step (INFERRED, CAMERA-FOLLOW-SITES §12; the rows read
// these values from the same thread, ApplyWitnessTeleport). Behind camera-fixed-step.flag with the
// corrections (22_setup): the reference of the camera's method-B laws (D-0243).
namespace camera_step {
using UpdateFn=void(__fastcall*)(uintptr_t,float,uintptr_t,uintptr_t);
UpdateFn original{};
bool active{};
constexpr size_t kLow=0x10,kHigh=0x3D8,kTail=0x430,kEnd=0x440,kOut=0x10,kOutEnd=0x60;
alignas(16) BYTE s60[kEnd]{},left[kEnd]{},before[kOutEnd]{};
uintptr_t kept{};double owed{};LONG64 ticks{};float last_tick_dt{};
constexpr float kStep=1.f/60.f;
void CopyState(BYTE* to,const BYTE* from) noexcept {std::memcpy(to+kLow,from+kLow,kHigh-kLow);std::memcpy(to+kTail,from+kTail,kEnd-kTail);}
void CarryOthers(const BYTE* object) noexcept {   // step review L3: aligned words, a value written over the display's kept whole
    for(size_t i=kLow;i<kEnd;i+=4){if(i==kHigh)i=kTail;if(std::memcmp(object+i,left+i,4))std::memcpy(s60+i,object+i,4);}
}
// The display's outputs (native/world/view_blend.h): the last two ticks' views blended at alpha.
void WriteBlend(BYTE* object,float alpha) noexcept {
    float out[20];ds3native::view_blend::Blend(reinterpret_cast<const float*>(before+kOut),reinterpret_cast<const float*>(s60+kOut),alpha,out);
    std::memcpy(object+kOut,out,sizeof(out));
}
void __fastcall Update(uintptr_t self,float dt,uintptr_t chr,uintptr_t world) noexcept {
    auto* object=reinterpret_cast<BYTE*>(self);
    const bool sixty=std::abs(dt*60.f-1.f)<1e-4f;   // k's own tolerance (BeginFrame)
    if(!self || !(dt>0) || (sixty && self!=kept)){original(self,dt,chr,world);return;}
    if(self!=kept){kept=self;owed=0;original(self,dt,chr,world);last_tick_dt=dt;CopyState(s60,object);std::memcpy(before,object,kOutEnd);CopyState(left,object);return;}
    CarryOthers(object);CopyState(object,s60);
    // Final review H1: once a state is kept, a dt of 1/60 is a tick like any other (one a frame, a steady one-tick lag); a
    // branch of its own flipped the view between the current tick and the blend when dt crossed 1/60 and dropped owed time.
    owed+=dt;
    for(int n=0;owed>=kStep-1e-6 && n<4;++n){owed-=kStep;std::memcpy(before,s60,kOutEnd);original(self,kStep,chr,world);last_tick_dt=kStep;CopyState(s60,object);++ticks;}
    if(owed>=kStep)owed=0;   // a guard: DS3's dt is capped at 50 ms, so no more than four ticks are owed
    WriteBlend(object,std::clamp(static_cast<float>(owed/kStep),0.f,1.f));   // alpha 0 too: the previous tick, so no jump at a tick
    CopyState(left,object);
}
// 140510290(ChrCam, chr) writes the heading's target from ChrCam's forward +0x30 (1405103E0's plain copy of this camera's row,
// through 1404095F0): while ChrCam follows the kept camera ([ChrCam+0x60], step review M2), it is given S60's row for its call.
using ViewCopyFn=void(__fastcall*)(uintptr_t,uintptr_t);
ViewCopyFn view_original{};
uintptr_t chr_cam{};LONG64 copies{};
void __fastcall StableViewCopy(uintptr_t camera_man,uintptr_t chr) noexcept {
    chr_cam=camera_man;++copies;
    uintptr_t follows{};
    if(!kept || !camera_man || !Read(camera_man+0x60,&follows,8) || follows!=kept){view_original(camera_man,chr);return;}
    auto* forward=reinterpret_cast<BYTE*>(camera_man+0x30);BYTE shown[16];std::memcpy(shown,forward,16);
    std::memcpy(forward,s60+0x30,16);view_original(camera_man,chr);std::memcpy(forward,shown,16);
}
using DestroyFn=uintptr_t(__fastcall*)(uintptr_t,uint32_t);
DestroyFn destroy_original{};
uintptr_t __fastcall Destroy(uintptr_t self,uint32_t flags) noexcept {if(self==kept)kept=0;return destroy_original(self,flags);}
bool Install() noexcept {
    if(MH_CreateHook(reinterpret_cast<void*>(image+0x515790),reinterpret_cast<void*>(&Update),reinterpret_cast<void**>(&original))!=MH_OK ||
       MH_CreateHook(reinterpret_cast<void*>(image+0x510290),reinterpret_cast<void*>(&StableViewCopy),reinterpret_cast<void**>(&view_original))!=MH_OK ||
       MH_CreateHook(reinterpret_cast<void*>(image+0x5130F0),reinterpret_cast<void*>(&Destroy),reinterpret_cast<void**>(&destroy_original))!=MH_OK)return false;
    active=true;return true;
}
}
// I-0113 gap 24: Firelink's loading fog walls and their doorway's collision, never made natively (Thomas, 2026-11-11 23:18 and 23:53:
// « Coupé par défaut », then « off forcé »: tranche 38's setting `firelink_wall` removed in tranche 40). A content change for two walls,
// outside D-0161's « a family, never an asset » rule by its nature (both renderers draw the walls alike; RESULT « Tranche 38 »).
// m40's event 14005102 makes, while the cemetery loads, the object-following SFX 840080 on o000401_0000 (entity 4001101, the shrine's
// west entrance, 244.1 -46.8 599.3) and 4 on o000402_0000 (4001102, 283.8 -40.7 588.9) (2006[4] CreateObjectfollowingSFX), and
// deletes them about 19 s after the player leaves for the cemetery (RESULT « Tranche 37 »). The object SFX slot's requests (1407C91C0,
// 1407C9160) and the object effect maker 1407CC4B0 never carried those ids (i113-t37-fogwall8/9-native); ffx-observe.flag
// (i113-t37-fogwall10-native) saw them at SprjSfxImp_CreateSfxIns E6DCD0 (SprjSfxImp, out handle, &id, transform, flags), called
// by the follow effects' update E6A350 at the object's dummy (its call returns to E6A497). A call from there for SFX 840080 or 4 whose
// position (the transform's first three floats, the dummy's translation row) lies within 1 m of its wall runs against an SFX manager
// image whose effect system (+0x58) is null: E6DCD0 then leaves the handle empty and returns it, as when the effect system is absent
// (GhidraMCP 2026-11-11; tranche 38's final review read the manager's only use before the branch). The walls are told by caller, SFX
// id and place (m40's coordinates): no entity id was read from the follow effect (INFERRED absent, not checked). The follow effect
// tries again every frame while its handle is empty (seen for 8 frames). With ffx-observe.flag (which hooks E6DCD0 first) the hook is
// refused: the walls stay. Native runs only.
namespace firelink_wall {
struct Wall {int32_t sfx;float x,y,z;};
constexpr Wall kWalls[2]{{840080,244.1f,-46.8f,599.3f},{4,283.8f,-40.7f,588.9f}};
constexpr float kRadius=1.f;
constexpr uint32_t kCaller=0xE6A497;   // E6A350's call (every logged drop of i113-t37-fogwall11-native, the first 16, came from it)
using Make=uintptr_t(__fastcall*)(uintptr_t,uintptr_t,const int32_t*,uintptr_t,uint32_t);
Make make_original{};
alignas(16) uint8_t no_effect_system[0x100]{};   // an SFX manager image: +0x58 (the effect system) null
volatile LONG64 dropped{},unmatched{},first_seen{};
uintptr_t __fastcall MakeDetour(uintptr_t manager,uintptr_t handle,const int32_t* id,uintptr_t transform,uint32_t flags) noexcept {
    int32_t sfx{-1};
    if(reinterpret_cast<uintptr_t>(_ReturnAddress())==image+kCaller && id && Read(reinterpret_cast<uintptr_t>(id),&sfx,4) &&
       (sfx==kWalls[0].sfx || sfx==kWalls[1].sfx)){
        float at[3]{};const bool placed=transform && Read(transform,at,sizeof at) && std::isfinite(at[0]) && std::isfinite(at[1]) && std::isfinite(at[2]);
        for(size_t i=0;i<2;++i){const auto& wall=kWalls[i];if(wall.sfx!=sfx || !placed)continue;
            const float dx=at[0]-wall.x,dy=at[1]-wall.y,dz=at[2]-wall.z;
            if(dx*dx+dy*dy+dz*dz<=kRadius*kRadius){const auto n=InterlockedIncrement64(&dropped);
                // Its final review's M1 and L4: each wall's first drop, then the total at 600, 6,000 and 60,000 drops.
                const bool first=!(InterlockedOr64(&first_seen,LONG64(1)<<i)&(LONG64(1)<<i));
                if(first || n==600 || n==6000 || n==60000){char row[192]{};_snprintf_s(row,sizeof row,_TRUNCATE,
                    "{\"event\":\"firelink_wall_dropped\",\"sfx\":%d,\"at\":[%.1f,%.1f,%.1f],\"first\":%s,\"total\":%lld}\n",sfx,at[0],at[1],at[2],first?"true":"false",static_cast<long long>(n));Log(row);}
                return make_original(reinterpret_cast<uintptr_t>(no_effect_system),handle,id,transform,flags);}}
        if(InterlockedIncrement64(&unmatched)<=8){char row[192]{};_snprintf_s(row,sizeof row,_TRUNCATE,"{\"event\":\"firelink_wall_unmatched\",\"sfx\":%d,\"placed\":%s,\"at\":[%.1f,%.1f,%.1f]}\n",
            sfx,placed?"true":"false",placed?at[0]:0.f,placed?at[1]:0.f,placed?at[2]:0.f);Log(row);}}
    return make_original(manager,handle,id,transform,flags);}
// I-0113 tranche 40 (Thomas, 2026-11-11 23:18: the doorway's collision removed with the wall): the event's objects o000401_0000 and
// o000402_0000 (entities 4001101, 4001102) carry the doorway's collision (an hkx, no mesh); DeactivateObject's commands set it
// through ObjIns_SetColiEnable 1406660F0 (ObjIns +0x190 bit 1; its callers the command class 14052FB00, the by-entity set 1404D1520,
// the map's object list 1404D2490 and MsbObject_Create; GhidraMCP 2026-11-11). An enable for either entity is turned into a disable.
// The entity read as the map's own entity table reads it (MsbObject_Create through 140660110):
// *(int32_t*)*(*(*(*(ObjIns+0x18)+8)+0x10)+0xB0).
constexpr int32_t kEntities[2]{4001101,4001102};
using SetColi=void(__fastcall*)(uintptr_t,uint8_t);
SetColi coli_original{};
volatile LONG64 coli_held{},coli_first{};
int32_t EntityOf(uintptr_t obj) noexcept {
    uintptr_t p{};int32_t id{-1};
    if(!Read(obj+0x18,&p,8) || !p || !Read(p+8,&p,8) || !p || !Read(p+0x10,&p,8) || !p || !Read(p+0xB0,&p,8) || !p || !Read(p,&id,4))return -1;
    return id;}
void __fastcall ColiDetour(uintptr_t obj,uint8_t enable) noexcept {
    if(enable){const int32_t id=EntityOf(obj);
        for(size_t i=0;i<2;++i){if(id!=kEntities[i])continue;
            const auto n=InterlockedIncrement64(&coli_held);
            if(!(InterlockedOr64(&coli_first,LONG64(1)<<i)&(LONG64(1)<<i)) || n==100 || n==1000){char row[128]{};
                _snprintf_s(row,sizeof row,_TRUNCATE,"{\"event\":\"firelink_wall_collision_held\",\"entity\":%d,\"total\":%lld}\n",id,static_cast<long long>(n));Log(row);}
            enable=0;break;}}
    coli_original(obj,enable);}
bool Install() noexcept {
    return MH_CreateHook(reinterpret_cast<void*>(image+0xE6DCD0),reinterpret_cast<void*>(&MakeDetour),reinterpret_cast<void**>(&make_original))==MH_OK;}
bool InstallCollision() noexcept {
    return MH_CreateHook(reinterpret_cast<void*>(image+0x6660F0),reinterpret_cast<void*>(&ColiDetour),reinterpret_cast<void**>(&coli_original))==MH_OK;}
}
// I-0113 gap 26 (the original's defect, Thomas 2026-11-11: after an Alt+Tab back the camera turns by itself and the inputs may stay
// frozen; he plays with keyboard and mouse, the pad to fix too). DS3's input devices (GhidraMCP 2026-11-11, read whole; the final
// review read their vtables in the executable): each a Device whose update 141895C80 (vtable +0x38) calls its poll (+0xD8) and maps
// the bindings only when the poll returns true (else its failure path, +0x50 and the state arrays cleared). The polls: the DirectInput
// mouse 14183C540 and keyboard 14183BE70 (a failure drops the device pointer +0x198, made again a second later; seen through the native
// input latch's detours, 17_render_step) and the pad 14183D300, which returns false at once while the input manager [0x1449644B8]'s
// foreground-only pad byte +0x24E is set and its window-active byte +0x24D clear (+0x24D: 1 from the manager's constructor 1417DB100,
// cleared by its set-up 1417DC140 when GetActiveWindow differs, written by WM_ACTIVATE through the pad's message hook 14183DDA0 ->
// 1417DC070, the manager's lock held). Each poll returns its bool in AL (the final review's H1: the pad's too, read in its epilogues).
// Witnessed (i113-t36-alttab-native, windowed, three Alt+Tab with the mouse moved on the desktop): no device is polled while the game
// is away; at each return four mouse objects (each with its own DirectInput device) read the desktop's motion and the camera turned;
// four keyboard objects fail their first read back (foreground level, +0x24F set) and are made again at the next.
// The fix (on unless no-input-focus-fix.flag; a game fix, as the unlocked rate's): per device object, a mouse read while the engine's
// window is not in front, the first after a pause of its polls (over 0.3 s: the game away, or a long load whose first motion is then
// lost too) and the one after a failed read hold no motion, and a failed read leaves none behind (the original keeps its last floats);
// a keyboard read while not in front holds no key; while not in front the presses the native latch counted are not forced. The click
// that brings the window back is kept (ER's regain-frame gate drops it; here the original's behaviour, a choice). The pad (unless
// no-pad-focus-fix.flag; untested live, no pad on the PC): skipped while the window reads inactive, its window-active byte set again
// when the engine's window is in front (a WM_ACTIVATE missed), put back if the window left the front meanwhile; skipped, it returns
// false and its update's failure path keeps its bindings off, as the original.
// input-observe.flag (a diagnostic, nothing changed by it): a row at each change of the window in front or of the manager's bytes
// +0x24C..+0x250, at each change of a device's state, and each poll's row for 4 s after any of them or after a pause of the polls.
// I-0132 R12h's L7 (Thomas, 2026-11-12: the keys blocked while the precompile page is up; the provisional choice was « not blocked »):
// while the page is up (16b BuildPrecompileScreen, from its first shown frame through its fade-out), every input the game reads is held
// back as when the game is away: a keyboard read holds no key, a mouse read no motion, wheel or button, the menus' mouse object
// (23891E0) no pressed edge and no wheel (its held state left as read, so a button held across the page's end gives no false edge),
// the native latch forces nothing and its counts are taken as seen (no press replayed after the page), and the pad's poll returns false
// unread (the original's own path while its window is inactive: its update's failure path keeps its bindings off). After the page a
// key or button still held from it stays held back until released (at most 10 s, kAfterPageMs), and the window messages' latch counts
// no press made under it (17). On unless no-precompile-input-block.flag (22_setup); one row at the page's end counts what was held
// back, one more when the 10 s after it are over.
namespace input_focus {
bool fix{},pad_fix{},observe{};   // 22_setup
bool page_block_enabled{},pad_hooked{};   // 22_setup (R12h's L7; the pad's poll detoured, the step review's L2)
std::atomic<bool> page_block{};   // set by the precompile page (16b) while it is up
std::atomic<uint64_t> page_keyboard_held{},page_mouse_held{},page_menu_mouse_held{},page_pad_skipped{};   // polls with an input held back
bool PageBlocks() noexcept {return page_block.load(std::memory_order_relaxed);}
// The page's end (the step review's M2): the game's reads under the page said « released », so a key, a mouse button or a pad button
// still down when it ends would reach the game as a new press. A key or button seen down under the page stays held back after it
// until a read finds it released, and the pad's poll keeps failing while one of its buttons is down, at most kAfterPageMs after the end.
constexpr ULONGLONG kAfterPageMs=10000;
std::atomic<ULONGLONG> page_end_tick{};   // EndPageBlock
std::array<std::atomic<uint8_t>,256> page_keys{};std::array<std::atomic<uint8_t>,8> page_buttons{};std::atomic<bool> page_pad{};
std::atomic<uint64_t> after_page_held{};   // polls after the end with an input still held back
std::atomic<bool> page_block_set{};   // the block set at least once (the page's row; the final review's L2)
void StartPageBlock() noexcept {page_block_set.store(true,std::memory_order_relaxed);page_block.store(true,std::memory_order_relaxed);}   // 16b
void EndPageBlock() noexcept {   // 16b: the page's end, an overlay lost and Step's failure path; the end's tick first (the final review's L5)
    if(page_block.load(std::memory_order_relaxed)){page_end_tick.store(GetTickCount64(),std::memory_order_relaxed);page_block.store(false,std::memory_order_relaxed);}}
std::atomic<bool> after_page_said{};
bool AfterPage() noexcept {
    if(after_page_said.load(std::memory_order_relaxed))return false;   // the final review's L6: nothing more once its window is over
    const ULONGLONG end=page_end_tick.load(std::memory_order_relaxed);if(!end)return false;
    if(GetTickCount64()-end<kAfterPageMs)return true;
    if(!after_page_said.exchange(true)){char row[128]{};   // one row once the window is over
        sprintf_s(row,"{\"event\":\"native_precompile_input_after\",\"held_polls\":%llu,\"ms\":%llu}\n",(unsigned long long)after_page_held.load(),(unsigned long long)kAfterPageMs);Log(row);}
    return false;}
// One device buffer's bytes (bit 7 down): under the page each down byte is marked and cleared; after it a marked byte still down is
// cleared, a released one unmarked. True when a byte was cleared.
template<size_t N> bool HoldBytes(uintptr_t bytes,std::array<std::atomic<uint8_t>,N>& marks,bool page) noexcept {
    bool any=false;
    for(size_t k=0;k<N;++k){const auto byte=reinterpret_cast<volatile uint8_t*>(bytes+k);const bool down=(*byte&0x80)!=0;
        if(page){if(down){marks[k].store(1,std::memory_order_relaxed);any=true;}*byte=0;}
        else if(marks[k].load(std::memory_order_relaxed)){if(down){*byte=0;any=true;}else marks[k].store(0,std::memory_order_relaxed);}}
    return any;}
using PadFn=uint64_t(__fastcall*)(uintptr_t);PadFn pad_original{};
std::atomic<ULONGLONG> burst_until{};std::atomic<uint64_t> focus_key{~0ull};
std::atomic<int> mouse_seen{-1},keyboard_seen{-1},pad_seen{-1};
std::atomic<uint64_t> mouse_dropped{},keyboard_cleared{},pad_reactivated{};
HWND EngineWindow() noexcept {uintptr_t window{};HWND hwnd{};return Read(image+0x489F118,&window,8) && window && Read(window+8,&hwnd,8)?hwnd:nullptr;}
// The engine's window in front (CSWindowImp [0x14489F118]+8, as FollowNativePresentationState compares it; the step review's L3:
// another window of the process, a dialog, is not the game), read once a millisecond tick for the polls of a frame (its L5).
bool InFront() noexcept {
    static std::atomic<ULONGLONG> tick{};static std::atomic<bool> front{};
    const ULONGLONG now=GetTickCount64();if(tick.load(std::memory_order_relaxed)==now)return front.load(std::memory_order_relaxed);
    const HWND top=GetForegroundWindow();bool ours;
    if(const HWND hwnd=EngineWindow())ours=top==hwnd;
    else {DWORD owner{};GetWindowThreadProcessId(top,&owner);ours=owner==GetCurrentProcessId();}
    front.store(ours,std::memory_order_relaxed);tick.store(now,std::memory_order_relaxed);return ours;}
uintptr_t Manager() noexcept {uintptr_t m{};return Read(image+0x49644B8,&m,8)?m:0;}
uint64_t ManagerBytes(uintptr_t m) noexcept {uint64_t b{};if(m)Read(m+0x248,&b,8);return b;}   // +0x248..+0x24F
// The observer's rows: the focus and the manager's bytes at each poll (any device); a change opens a 4 s window of rows.
void Check(bool front) noexcept {
    if(!observe)return;
    const uintptr_t m=Manager();uint8_t setting{};if(m)Read(m+0x250,&setting,1);
    const uint64_t bytes=ManagerBytes(m),key=(bytes&0xFFFFFFFF00000000ull)|(uint64_t(setting)<<8)|(front?1:0);
    if(focus_key.exchange(key)==key)return;
    burst_until.store(GetTickCount64()+4000);
    char row[384]{};sprintf_s(row,"{\"event\":\"input_focus\",\"tick\":%llu,\"front_ours\":%d,\"grab_24c\":%u,\"active_24d\":%u,\"pad_fg_24e\":%u,\"kb_fg_24f\":%u,\"mouse_fg_250\":%u,\"thread\":%lu,\"fix\":%d,\"pad_fix\":%d}\n",
        static_cast<unsigned long long>(GetTickCount64()),front?1:0,unsigned((bytes>>32)&0xFF),unsigned((bytes>>40)&0xFF),unsigned((bytes>>48)&0xFF),unsigned((bytes>>56)&0xFF),unsigned(setting),GetCurrentThreadId(),fix?1:0,pad_fix?1:0);Log(row);
}
bool Burst() noexcept {return GetTickCount64()<burst_until.load();}
// Each device object's last poll and whether it failed, by object (i113-t36-alttab-fix: four mouse objects polled one after the
// other, each with its own DirectInput device, each handing the desktop's motion to its first read back): a pause of an object's polls
// over 0.3 s opens the observer's window and makes its next mouse poll read no motion.
struct Poll {uintptr_t self{};ULONGLONG last{};bool failed{};};
std::array<Poll,16> polls{};SRWLOCK polls_lock=SRWLOCK_INIT;
struct Resume {bool resumed,failed_before;};
Resume Polled(uintptr_t self,bool read) noexcept {
    const ULONGLONG now=GetTickCount64();AcquireSRWLockExclusive(&polls_lock);
    Poll* slot{};for(auto& p:polls){if(p.self==self){slot=&p;break;}if(!slot && !p.self)slot=&p;}
    if(!slot){slot=&polls[0];for(auto& p:polls)if(p.last<slot->last)slot=&p;}   // the least recent, when sixteen objects were seen
    const bool fresh=slot->self!=self;if(fresh)*slot=Poll{self,0,false};
    const Resume r{fresh || slot->last==0 || now-slot->last>300,slot->failed};slot->last=now;slot->failed=!read;
    ReleaseSRWLockExclusive(&polls_lock);return r;}
// After the original mouse poll (17_render_step): true when the latch must not force a press this poll.
// R12h's L7: a successful read under the precompile page holds nothing (DIMOUSESTATE2's lX, lY, lZ and its buttons, the floats).
bool PageMouse(uintptr_t self,uint64_t result) noexcept {
    if(!PageBlocks()){   // M2: a button held across the page's end held back until released
        // Not true for the latch (the final review's L3): a held button sends no new press to count, the other buttons' presses forced as before.
        if(result&0xFF && AfterPage() && HoldBytes(self+0x1AC,page_buttons,false))after_page_held.fetch_add(1,std::memory_order_relaxed);
        return false;}
    if(result&0xFF){int32_t motion[3]{};Read(self+0x1A0,motion,12);
        bool any=motion[0] || motion[1] || motion[2];any=HoldBytes(self+0x1AC,page_buttons,true) || any;
        std::memset(reinterpret_cast<void*>(self+0x1A0),0,12);
        if(any)page_mouse_held.fetch_add(1,std::memory_order_relaxed);}
    std::memset(reinterpret_cast<void*>(self+0x1B8),0,12);return true;}
bool Mouse(uintptr_t self,uint64_t result) noexcept {
    const bool page=PageMouse(self,result);
    if(!fix && !observe)return page;
    const bool front=InFront(),read=(result&0xFF)!=0;const auto polled=Polled(self,read);const bool resumed=polled.resumed,failed_before=polled.failed_before;
    Check(front);if(resumed && observe)burst_until.store(GetTickCount64()+4000);
    float d[3]{};Read(self+0x1B8,d,12);
    const bool drop=fix && (!front || resumed || failed_before || !read);
    if(drop && (d[0]!=0.f || d[1]!=0.f || d[2]!=0.f || read)){
        std::memset(reinterpret_cast<void*>(self+0x1B8),0,12);if(read)std::memset(reinterpret_cast<void*>(self+0x1A0),0,12);   // the floats and DIMOUSESTATE2's lX, lY, lZ
        if(read && !front)std::memset(reinterpret_cast<void*>(self+0x1AC),0,8);   // and its buttons while the game is away
        mouse_dropped.fetch_add(1,std::memory_order_relaxed);}
    if(observe){uintptr_t device{};Read(self+0x198,&device,8);const int state=int(read)|(device?2:0)|(front?4:0);
        if(mouse_seen.exchange(state)!=state || resumed || (Burst() && (d[0]!=0.f || d[1]!=0.f || !read))){char row[512]{};
            sprintf_s(row,"{\"event\":\"input_mouse\",\"tick\":%llu,\"read\":%d,\"device\":%d,\"front\":%d,\"resumed\":%d,\"dx\":%.6g,\"dy\":%.6g,\"dz\":%.6g,\"dropped\":%d,\"dropped_total\":%llu}\n",
                static_cast<unsigned long long>(GetTickCount64()),read?1:0,device?1:0,front?1:0,resumed?1:0,d[0],d[1],d[2],drop?1:0,static_cast<unsigned long long>(mouse_dropped.load()));Log(row);}}
    return (fix && !front) || page;   // the step review's L4: a click during a stall with the game in front still forced
}
bool Keyboard(uintptr_t self,uint64_t result) noexcept {
    bool page=false;
    if(PageBlocks()){page=true;   // R12h's L7: no key under the precompile page
        if(result&0xFF && HoldBytes(self+0x1A0,page_keys,true))page_keyboard_held.fetch_add(1,std::memory_order_relaxed);}
    else if(result&0xFF && AfterPage() && HoldBytes(self+0x1A0,page_keys,false))after_page_held.fetch_add(1,std::memory_order_relaxed);   // M2 (a held key's repeats are not counted: the latch left on, the final review's L3)
    if(!fix && !observe)return page;
    const bool front=InFront(),read=(result&0xFF)!=0,resumed=Polled(self,read).resumed;
    Check(front);if(resumed && observe)burst_until.store(GetTickCount64()+4000);
    uint32_t down{};if(observe)for(uint32_t k=0;k<256;++k)down+=(*reinterpret_cast<const uint8_t*>(self+0x1A0+k)&0x80)?1:0;
    if(fix && read && !front){std::memset(reinterpret_cast<void*>(self+0x1A0),0,256);keyboard_cleared.fetch_add(1,std::memory_order_relaxed);}
    if(observe){uintptr_t device{};Read(self+0x198,&device,8);const int state=int(read)|(device?2:0)|(front?4:0);
        if(keyboard_seen.exchange(state)!=state || resumed || (Burst() && down)){char row[320]{};
            sprintf_s(row,"{\"event\":\"input_keyboard\",\"tick\":%llu,\"read\":%d,\"device\":%d,\"front\":%d,\"resumed\":%d,\"keys_down\":%u,\"cleared_total\":%llu}\n",
                static_cast<unsigned long long>(GetTickCount64()),read?1:0,device?1:0,front?1:0,resumed?1:0,down,static_cast<unsigned long long>(keyboard_cleared.load()));Log(row);}}
    return (fix && !front) || page;
}
// R12h's L7: the menus' mouse object (23891E0, NATIVE-INPUT-LATCH: per button +8 pressed edge, +9 released edge, +0xA held; left,
// right, middle 8 bytes apart; the wheel +0x28, as NativeUiPointerState reads it) after its update under the precompile page: no
// pressed edge and no wheel (the latch forces nothing then: ApplyNativeMouseLatch reads PageBlocks itself).
void PageMenuMouse(uintptr_t mouse) noexcept {
    if(!PageBlocks())return;
    uint8_t edges[3]{};int32_t wheel{};bool any=false;
    for(int b=0;b<3;++b)if(Read(mouse+8+8*b,&edges[b],1) && edges[b]){any=true;*reinterpret_cast<volatile uint8_t*>(mouse+8+8*b)=0;}
    if(Read(mouse+0x28,&wheel,4) && wheel){any=true;*reinterpret_cast<volatile int32_t*>(mouse+0x28)=0;}
    if(any)page_menu_mouse_held.fetch_add(1,std::memory_order_relaxed);}
// The pad's poll (its vtable +0xD8): the original's AL returned on every path. The window-active byte is set by a byte write (the
// original's own write takes the manager's lock from the window thread; from the device update here a lock order device -> manager
// would be new and unrun, the final review's M1), read again after it.
uint64_t __fastcall Pad(uintptr_t self) noexcept {
    if(PageBlocks()){page_pad.store(true,std::memory_order_relaxed);page_pad_skipped.fetch_add(1,std::memory_order_relaxed);return 0;}   // R12h's L7: false unread under the precompile page
    // M2: after the page, the poll read and failed while a button is still down (+0x248, the buttons the observer's row reads).
    const bool pad_after=page_pad.load(std::memory_order_relaxed) && AfterPage();
    const bool front=InFront();Check(front);
    const uintptr_t m=Manager();uint64_t bytes=ManagerBytes(m);
    bool skipped=((bytes>>48)&0xFF) && !((bytes>>40)&0xFF);   // the original's early return
    // Only with the engine's window known (its last step review's L2: not on the process-wide fallback of InFront).
    if(const HWND hwnd=pad_fix && skipped && front && m?EngineWindow():nullptr){std::atomic_ref<uint8_t> active(*reinterpret_cast<uint8_t*>(m+0x24D));active.store(1);
        if(GetForegroundWindow()!=hwnd){uint8_t one=1;active.compare_exchange_strong(one,0);}   // the step review's M2
        pad_reactivated.fetch_add(1,std::memory_order_relaxed);bytes=ManagerBytes(m);skipped=((bytes>>48)&0xFF) && !((bytes>>40)&0xFF);}
    const uint64_t raw=pad_original(self);
    if(pad_after && (raw&0xFF)){uint16_t held{};Read(self+0x248,&held,2);
        if(held){after_page_held.fetch_add(1,std::memory_order_relaxed);return raw&~uint64_t{0xFF};}
        page_pad.store(false,std::memory_order_relaxed);}
    if(!observe)return raw;
    float s[5]{};uint16_t buttons{};int32_t xinput{},other{};Read(self+0x254,s,20);Read(self+0x248,&buttons,2);Read(self+0x24C,&xinput,4);Read(self+0x3C4,&other,4);
    const bool moved=s[0]!=0.f || s[1]!=0.f || s[2]!=0.f || s[3]!=0.f || s[4]!=0.f || buttons;
    const int state=(skipped?1:0)|(moved?2:0)|(xinput>=0?4:0)|(other>=0?8:0)|(front?16:0)|((raw&0xFF)?32:0);
    static std::atomic<ULONGLONG> last_moved{};const ULONGLONG now=GetTickCount64();
    if(pad_seen.exchange(state)!=state || (Burst() && moved) || (moved && now-last_moved.load()>1000)){if(moved)last_moved.store(now);char row[512]{};
        // xinput: +0x24C, the XInput slot it polls; other: +0x3C4, the user index of its other reader (+0x3F0), as 14183D300 reads them
        sprintf_s(row,"{\"event\":\"input_pad\",\"tick\":%llu,\"read\":%d,\"skipped\":%d,\"front\":%d,\"xinput\":%d,\"other\":%d,\"sticks\":[%.6g,%.6g,%.6g,%.6g,%.6g],\"buttons\":%u,\"reactivated\":%llu}\n",
            static_cast<unsigned long long>(now),(raw&0xFF)?1:0,skipped?1:0,front?1:0,xinput,other,s[0],s[1],s[2],s[3],s[4],unsigned(buttons),
            static_cast<unsigned long long>(pad_reactivated.load()));Log(row);}
    return raw;
}
bool Install() noexcept {
    return MH_CreateHook(reinterpret_cast<void*>(image+0x183D300),reinterpret_cast<void*>(&Pad),reinterpret_cast<void**>(&pad_original))==MH_OK;
}
}
// I-0114 (D-0243, method B): the follow camera's per-call laws given their 60 frames/s behaviour above 60 (I-0113 gap 27: k = 1 under it), with k = the call's
// dt / (1/60) (140515790's own dt, XMM1). Each law is a gain, rate or divisor field the call reads (the full decompile,
// evidence/I-0112/ctx/full/140515790.c, line numbers there; CAMERA-FOLLOW-SITES §3, §9-§11): the detour writes the law's value
// into the field for the call and puts the field back after it, keeping whatever the call itself wrote. At k = 1 the original's
// call (its bits at 60). The laws, each with no-camera-law-<name>.flag for the A/B:
// - focus: the focus base +0x90 += k30·[+0x1A8]·(target − it) (1578), Euler: logic_rate::Euler30. [+0x1A8] then ramps toward
//   [+0x1AC] by dt, or is reset to [+0x1B0] on a wall-recovery frame (3391-3397): the three fields are given the law, and after
//   the call the ramp or the reset is redone on the true values.
// - pitch: pitch = [+0x160] + wrap(pitch − [+0x160])·[+0x1B8]·k30 (2579), the frame's change kept by a fraction (E0 is rebuilt
//   from the kept pitch): logic_rate::Fraction30.
// - view: the view position P steps toward F0 by w a call, no dt (3404-3655): horizontal [+0x1BC] (ramped toward [+0x1C0] by dt,
//   reset to [+0x1C4], as the focus gain), vertical [+0x1C8], under lock or aim [+0x1D8] and [+0x1DC]: logic_rate::Gain. The dead
//   band's angle g is the gap accumulated between F0 and the last view (CAMERA-FOLLOW-SITES §11 (a)), a state, not a frame's
//   increment: left as it is.
// - distance: the view's distance blended toward F0's by [+0x1D4] a call (3608): Gain.
// - row: the camera parameter row's values lerped in by [+0x3D0] a call (1185-1216): Gain.
// - lock: the lock-on's yaw and pitch (1996, 2061, 2174, gain [+0x28C]), the pitch toward its target (2468, [+0x288]) and the
//   offset +0x2E0 (2532, [+0x300]), all k30·g: Euler30.
// - speed: [+0x190] from the focus's displacement a call over [+0x180] (2664-2690), a speed read a frame: PerCall on the divisor.
// - recentre: w += [+0x130]·(1 − w) (3565), [+0x130] a timer (decayed by dt / [+0x1F4] after the hold [+0x134], 2247-2256; 1 on
//   an input, 2518) used as a gain a call: the value the call will use given the law, the timer's own decay kept.
// - wall: the automatic wall recovery's angle a call [+0x314] (CAMERA-FOLLOW-SITES §2): its rates [+0x324] and [+0x334] PerCall,
//   its decay [+0x360] Ratio, the angle kept in the current rate's units (scaled by k / the last call's k). Its snap to 0 under
//   one degree a call (140518989, the shared constant 0x143D7ED40 loaded at 14051897C) gates a contact's onset: uncorrected it
//   would need k² times the 60 frames/s rate to start (step review M1). The load's displacement is pointed at a module float
//   (within 2 GB of the image) that logic_rate::WallSnapThreshold sets a call: one degree · k once a 1/60 s (from a contact's
//   start), 0 between, one degree at k = 1. Refused (named) if the instruction's bytes differ; the wall law then runs without it.
// Approximations (step review L4, second order): the view gain [+0x1BC] is read after its own ramp (3398-3408), so the call uses
// the ramp of the laws rather than the law of the ramp; the lock's gain at 2061 is [+0x28C]·(1 − [+0x290]·c/d)·k30, and Euler30
// corrects [+0x28C] alone. k is the call's own dt (step review L6: the frame's k elsewhere; equal while the camera gets the frame's dt).
// UNLOCK-READY §1 method D: a shared constant's RIP-relative float load (movss xmmN,[rip+disp32]) pointed at a module float, a
// 4-byte slot of one page allocated within 2 GB of the game image, holding the constant's value until the module writes the law's;
// applied once at start before the hooks are enabled, never undone (final review L5: written outside MinHook's thread freeze, safe
// because the module is injected shortly after the process starts, before the world runs; the opcode and target checked in
// place are the guard, the launch pins covering the patched functions' entries only). Patch: the instruction's opcode bytes before its disp32 checked
// (and the load's current target), the slot returned; null, nothing patched, when they differ or no page fits.
namespace rip_float {
BYTE* page{};size_t used{};
template<size_t N> float* Patch(uintptr_t at,const BYTE (&opcode)[N],uintptr_t target) noexcept {
    int32_t disp{};const uintptr_t next=at+N+4;
    if(std::memcmp(reinterpret_cast<const void*>(at),opcode,N))return nullptr;
    std::memcpy(&disp,reinterpret_cast<const void*>(at+N),4);
    if(next+static_cast<intptr_t>(disp)!=target)return nullptr;
    for(uintptr_t hint=(image+0x6000000+0xFFFF)&~uintptr_t{0xFFFF};!page && hint<at+0x7FF00000;hint+=0x10000)
        page=static_cast<BYTE*>(VirtualAlloc(reinterpret_cast<void*>(hint),4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if(!page || used+4>4096)return nullptr;
    auto* slot=reinterpret_cast<float*>(page+used);
    const int64_t to=static_cast<int64_t>(reinterpret_cast<uintptr_t>(slot))-static_cast<int64_t>(next);
    if(to>INT32_MAX || to<INT32_MIN)return nullptr;
    std::memcpy(slot,reinterpret_cast<const void*>(target),4);
    DWORD old{};if(!VirtualProtect(reinterpret_cast<void*>(at+N),4,PAGE_EXECUTE_READWRITE,&old))return nullptr;
    const int32_t displacement=static_cast<int32_t>(to);std::memcpy(reinterpret_cast<void*>(at+N),&displacement,4);
    VirtualProtect(reinterpret_cast<void*>(at+N),4,old,&old);FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(at),N+4);
    used+=4;return slot;
}
}
namespace camera_laws {
using UpdateFn=void(__fastcall*)(uintptr_t,float,uintptr_t,uintptr_t);
UpdateFn original{};
bool active{};
enum Law:uint32_t {Focus,Pitch,View,Distance,Row,Lock,Speed,Recentre,Wall,LawCount};
constexpr const char* kNames[LawCount]{"focus","pitch","view","distance","row","lock","speed","recentre","wall"};
uint32_t off{};   // the laws switched off by their flag
uintptr_t self_seen{};LONG64 calls{};float last_dt{};
uintptr_t wall_object{};float wall_k{1.f};
float* snap{};float snap_owed{},one_degree{};   // the wall snap's threshold the patched load reads (Install)
uintptr_t params_seen{};
float& F(uintptr_t self,uint32_t at) noexcept {return *reinterpret_cast<float*>(self+at);}
bool On(Law law) noexcept {return !(off&(1u<<law));}
// I-0138 tranche 7: the field of view's row (the settings' percent, the game's own at 100 %). The call lerps the camera's vertical
// field of view +0x50 toward its param row's (LockCamParam +0x14, degrees, clamped to [0, 180]) times pi / 180 by the weight +0x3D0
// (140515B57..140515B90, READ: evidence/I-0138/artifacts/tranche-7/fov/). The target alone is scaled (the step review's M2: the
// call's own reads of +0x50, the distance pull-back and the lock-on's pitch bound, keep the camera's true angle): the two
// instructions at 140515B76 (movss xmm1,[rax]: the clamped degrees; mulss xmm1,xmm4: times pi / 180) are moved into a stub near
// the module that also multiplies by the scale (fov_scale, written by the camera's thread before each call) and bounds the result
// at 150 degrees, then jumps back to 140515B7E; the site becomes a jump and three nops. Patched once, before the hooks are enabled
// (no camera call yet); false, nothing patched, when the bytes differ or no page fits. The aim and zoom cameras (their own +0x50,
// 14050E9C0, 14050D0C0) and a cutscene with its own lens are left alone; a cutscene's blend from the game camera and one with no
// lens of its own take the scaled value (READ: 14062DDF0, 140CE25C0). camera-fixed-step.flag's update (a test reference) runs
// without camera_laws, so without the patch: the row does nothing there. The stub has no unwind data (a stack walk stopped in its
// four instructions sees a leaf; the second step review's L1), as MinHook's trampolines.
float* fov_scale{};
float FovScaleNow() noexcept {
    const uint32_t percent=(std::clamp)(NativeSettingsCurrent().fov,ds3native::settings::kFovLeast,ds3native::settings::kFovMost);
    return float(percent)/100.f;}
bool PatchFov() noexcept {
    constexpr BYTE kMoved[8]{0xF3,0x0F,0x10,0x08,0xF3,0x0F,0x59,0xCC};   // movss xmm1,[rax]; mulss xmm1,xmm4
    const uintptr_t at=image+0x515B76;
    if(std::memcmp(reinterpret_cast<const void*>(at),kMoved,sizeof kMoved))return false;
    BYTE* code{};float* data{};
    for(uintptr_t hint=(image+0x6000000+0xFFFF)&~uintptr_t{0xFFFF};(!code || !data) && hint<at+0x7FF00000;hint+=0x10000){
        void* page=VirtualAlloc(reinterpret_cast<void*>(hint),4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!page)continue;
        if(!code)code=static_cast<BYTE*>(page);else data=static_cast<float*>(page);}
    const auto refuse=[&]{if(code)VirtualFree(code,0,MEM_RELEASE);if(data)VirtualFree(data,0,MEM_RELEASE);return false;};   // nothing kept (L2)
    if(!code || !data)return refuse();
    data[0]=1.f;data[1]=2.6179939f;   // the scale, and the bound: 150 degrees in radians
    const auto rel=[](uintptr_t from,const void* to,int32_t& out){const int64_t d=static_cast<int64_t>(reinterpret_cast<uintptr_t>(to))-static_cast<int64_t>(from);
        if(d>INT32_MAX || d<INT32_MIN)return false;out=static_cast<int32_t>(d);return true;};
    BYTE* p=code;int32_t d{};
    std::memcpy(p,kMoved,sizeof kMoved);p+=sizeof kMoved;
    constexpr BYTE kMulScale[4]{0xF3,0x0F,0x59,0x0D},kMinBound[4]{0xF3,0x0F,0x5D,0x0D};   // mulss xmm1,[rip+d]; minss xmm1,[rip+d]
    std::memcpy(p,kMulScale,4);p+=4;if(!rel(reinterpret_cast<uintptr_t>(p)+4,&data[0],d))return refuse();std::memcpy(p,&d,4);p+=4;
    std::memcpy(p,kMinBound,4);p+=4;if(!rel(reinterpret_cast<uintptr_t>(p)+4,&data[1],d))return refuse();std::memcpy(p,&d,4);p+=4;
    *p++=0xE9;if(!rel(reinterpret_cast<uintptr_t>(p)+4,reinterpret_cast<const void*>(at+sizeof kMoved),d))return refuse();std::memcpy(p,&d,4);p+=4;
    DWORD old{};if(!VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old))return refuse();FlushInstructionCache(GetCurrentProcess(),code,4096);
    BYTE site[sizeof kMoved]{0xE9,0,0,0,0,0x90,0x90,0x90};if(!rel(at+5,code,d))return refuse();std::memcpy(site+1,&d,4);
    if(!VirtualProtect(reinterpret_cast<void*>(at),sizeof site,PAGE_EXECUTE_READWRITE,&old))return refuse();
    std::memcpy(reinterpret_cast<void*>(at),site,sizeof site);VirtualProtect(reinterpret_cast<void*>(at),sizeof site,old,&old);
    FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(at),sizeof site);
    fov_scale=data;return true;}
// A field the call only reads: the law's value for the call, the original's back unless the call wrote it.
struct Plain {uint32_t at;float kept,given;};
// A gain ramped toward a target by dt in the call, or reset (the focus and view gains): all three given the law; after the call the
// reset or the ramp redone on the true values.
struct Ramped {uint32_t at,target,reset;float kept,target_kept,reset_kept,given,target_given,reset_given;};
void __fastcall Update(uintptr_t self,float dt,uintptr_t chr,uintptr_t world) noexcept {
    self_seen=self;++calls;last_dt=dt;
    if(fov_scale)*fov_scale=FovScaleNow();   // I-0138 tranche 7: the field of view's scale for this call (PatchFov), before every path below
    // I-0132 (world-burst-after-<n>.flag): the follow camera's yaw [+0x144] at each call during the burst's 32 frames, before the call (the
    // turn the burst saw, compared between runs: Get-GiMotion.py); the frame serial as Clear published it (the step review's L2), so a row
    // may name its neighbour.
    if(const uint64_t first=world_burst_first.load(std::memory_order_relaxed),at=world_burst_serial.load(std::memory_order_relaxed);first && self && at>=first && at<first+32){char row[128]{};
        sprintf_s(row,"{\"event\":\"world_burst_yaw\",\"serial\":%llu,\"yaw\":%.6f}\n",(unsigned long long)at,F(self,0x144));Log(row);}
    if(!self || !(dt>0)){original(self,dt,chr,world);return;}
    // I-0132 R12f (the step review's H1): installed at every native start (the rate live), the original's update at DS3's own rate, the
    // wall state given back in the original's units and its snap the original's one degree.
    if(!native_rate_unlocked.load(std::memory_order_relaxed)){
        if(self==wall_object && wall_k!=1.f)F(self,0x314)/=wall_k;
        wall_object=self;wall_k=1.f;   // FG's step review L3: an object first seen here is in the original's units
        if(snap){*snap=one_degree;snap_owed=0.f;}
        original(self,dt,chr,world);return;}
    static const bool below_60=ModuleFlag(L"unlock-fix-below-60.flag");
    const float k=logic_rate::FrameK(dt,below_60);   // BeginFrame's law: 1 at and under 60 frames/s (I-0113 gap 27)
    // Final review L2: the patched load checked every 3600 calls (a run-time guard restoring the bytes would silently bring back the
    // k² onset); one row if it is found reverted. The wall state is kept for one object (final review L3: two follow cameras
    // alternating would restart the snap's 1/60 s at each call; one is seen in play).
    if(snap && !(calls%3600)){int32_t displacement{};std::memcpy(&displacement,reinterpret_cast<const void*>(image+0x518980),4);
        if(image+0x518984+static_cast<intptr_t>(displacement)!=reinterpret_cast<uintptr_t>(snap)){snap=nullptr;Log("{\"event\":\"camera_wall_snap_reverted\"}\n");}}
    // The gains a law depends on, once an object (step review M1: whether the wall snap bites).
    if(self!=params_seen){params_seen=self;char row[384]{};
        sprintf_s(row,"{\"event\":\"camera_laws_params\",\"focus\":%.4f,\"pitch\":%.4f,\"view\":[%.4f,%.4f],\"distance\":%.4f,\"row\":%.4f,\"lock\":[%.4f,%.4f,%.4f],\"speed\":%.4f,"
            "\"wall\":{\"rate\":%.4f,\"rate_far\":%.4f,\"far\":%.4f,\"max\":%.4f,\"decay\":%.4f},\"snap\":%s}\n",F(self,0x1A8),F(self,0x1B8),F(self,0x1BC),F(self,0x1C8),F(self,0x1D4),
            F(self,0x3D0),F(self,0x288),F(self,0x28C),F(self,0x300),F(self,0x180),F(self,0x324),F(self,0x334),F(self,0x340),F(self,0x354),F(self,0x360),snap?"true":"false");Log(row);}
    // The wall recovery's angle is per call: carried into this call's units; its snap judged once a 1/60 s.
    if(On(Wall)){if(self!=wall_object){wall_object=self;wall_k=k;snap_owed=0.f;}
        else if(wall_k!=k){F(self,0x314)*=k/wall_k;wall_k=k;}
        if(snap)*snap=logic_rate::WallSnapThreshold(one_degree,dt,k,F(self,0x314)==0.f,snap_owed);}
    if(k==1.f){original(self,dt,chr,world);return;}
    Plain plain[16];int n=0;
    const auto give=[&](uint32_t at,float value){plain[n++]={at,F(self,at),value};F(self,at)=value;};
    if(On(Pitch))give(0x1B8,logic_rate::Fraction30(F(self,0x1B8),k));
    if(On(View)){for(const uint32_t at:{0x1C8u,0x1D8u,0x1DCu})give(at,logic_rate::Gain(F(self,at),k));}
    if(On(Distance))give(0x1D4,logic_rate::Gain(F(self,0x1D4),k));
    if(On(Row))give(0x3D0,logic_rate::Gain(F(self,0x3D0),k));
    if(On(Lock)){for(const uint32_t at:{0x288u,0x28Cu,0x300u})give(at,logic_rate::Euler30(F(self,at),k));}
    if(On(Speed) && F(self,0x180)>0.f)give(0x180,logic_rate::PerCall(F(self,0x180),k));
    if(On(Wall)){give(0x324,logic_rate::PerCall(F(self,0x324),k));give(0x334,logic_rate::PerCall(F(self,0x334),k));
        if(F(self,0x360)>0.f)give(0x360,logic_rate::Ratio(1.f,F(self,0x360),k));}
    Ramped ramped[2];int m=0;
    const auto ramp=[&](uint32_t at,uint32_t target,uint32_t reset,float(*law)(float,float)){
        Ramped& r=ramped[m++];r={at,target,reset,F(self,at),F(self,target),F(self,reset),law(F(self,at),k),law(F(self,target),k),law(F(self,reset),k)};
        F(self,at)=r.given;F(self,target)=r.target_given;F(self,reset)=r.reset_given;};
    if(On(Focus))ramp(0x1A8,0x1AC,0x1B0,&logic_rate::Euler30);
    if(On(View))ramp(0x1BC,0x1C0,0x1C4,&logic_rate::Gain);
    // The recentre timer: the value this call will use (its hold and decay as 2247-2256 compute them) given the law, the rest of the
    // timer kept; after the call, an input's 1 kept, else the true timer (logic_rate::RecentreAfter: r back when the call skipped
    // its decay, as without a character at 1268, step review L1).
    logic_rate::RecentreGiven recentre{0.f,0.f,false};const float recentre_kept=F(self,0x130);
    if(On(Recentre)){recentre=logic_rate::GiveRecentre(recentre_kept,F(self,0x134),dt,F(self,0x1F4),k);if(recentre.valid)F(self,0x130)=recentre.given;}
    original(self,dt,chr,world);
    for(int i=n-1;i>=0;--i)if(logic_rate::SameBits(F(self,plain[i].at),plain[i].given))F(self,plain[i].at)=plain[i].kept;
    for(int i=0;i<m;++i){const Ramped& r=ramped[i];const float after=F(self,r.at);
        if(logic_rate::SameBits(F(self,r.target),r.target_given))F(self,r.target)=r.target_kept;
        if(logic_rate::SameBits(F(self,r.reset),r.reset_given))F(self,r.reset)=r.reset_kept;
        F(self,r.at)=logic_rate::RampedAfter(after,r.kept,r.target_kept,r.reset_kept,r.given,r.reset_given,dt);}
    if(recentre.valid)F(self,0x130)=logic_rate::RecentreAfter(F(self,0x130),recentre_kept,recentre);
}
using DestroyFn=uintptr_t(__fastcall*)(uintptr_t,uint32_t);
DestroyFn destroy_original{};
uintptr_t __fastcall Destroy(uintptr_t self,uint32_t flags) noexcept {if(self==wall_object)wall_object=0;if(self==self_seen)self_seen=0;return destroy_original(self,flags);}
// The wall snap's load (14051897C: movss xmm4,[rip+disp32], reading 0x143D7ED40; GhidraMCP 2026-10-31) pointed at a module float
// (rip_float, above). Before the hooks are enabled (no camera call yet). False, nothing patched, when the bytes differ or no page fits.
bool PatchSnap() noexcept {
    constexpr BYTE kLoad[4]{0xF3,0x0F,0x10,0x25};
    snap=rip_float::Patch(image+0x51897C,kLoad,image+0x3D7ED40);if(snap)one_degree=*snap;
    return snap!=nullptr;
}
bool Install() noexcept {
    for(uint32_t i=0;i<LawCount;++i){wchar_t name[64]{};swprintf_s(name,L"no-camera-law-%S.flag",kNames[i]);if(ModuleFlag(name))off|=1u<<i;}
    if(On(Wall) && !PatchSnap())Log("{\"event\":\"camera_wall_snap_refused\",\"reason\":\"load_bytes_or_page\"}\n");
    if(!PatchFov())Log("{\"event\":\"camera_fov_refused\",\"reason\":\"site_bytes_or_page\"}\n");   // I-0138 tranche 7: the row then does nothing
    if(MH_CreateHook(reinterpret_cast<void*>(image+0x515790),reinterpret_cast<void*>(&Update),reinterpret_cast<void**>(&original))!=MH_OK ||
       MH_CreateHook(reinterpret_cast<void*>(image+0x5130F0),reinterpret_cast<void*>(&Destroy),reinterpret_cast<void**>(&destroy_original))!=MH_OK)return false;
    std::string row="{\"event\":\"camera_laws\",\"off\":[";
    for(uint32_t i=0;i<LawCount;++i)if(off&(1u<<i)){if(row.back()!='[')row+=",";row+="\"";row+=kNames[i];row+="\"";}
    row+=snap?"],\"wall_snap\":\"per_60\"}\n":"]}\n";Log(row.c_str());
    active=true;return true;
}
}
// I-0114 (CAMERA-FOLLOW-SITES §4.2, locomotion-observe.flag): the follow camera's update 140515790 (its object in RCX, dt in XMM1;
// GhidraMCP 2026-10-31), seen by a thunk at its entry that keeps its last object, its dt and a call count, then jumps to the
// original (no argument or register but RAX and the flags touched), so the position row names the camera's yaw [+0x144], its
// wall-recovery step [+0x314] and its focus [+0x90]. The same thunk on two more entries (the study thread, CAMERA-FOLLOW-SITES
// §8, 2026-10-31): 140510290 (CameraMan in RCX), which copies the camera's view yaw into the player's pad manipulator [+0x90],
// the heading's target in free movement; and 1405E1B80, the manipulator's update that calls the encrypted 1405E2810 (the
// wanted delta [manip+0x20]): their calls a game step, and CameraMan's forward [+0x30..0x38].
namespace follow_camera {
struct Seen {uintptr_t self;LONG64 calls;float dt;};
Seen seen{},view_copy{},pad_update{};
void* original{};void* view_copy_original{};void* pad_update_original{};
bool Thunk(uint32_t rva,Seen* kept,void** next) noexcept {
    auto* memory=static_cast<BYTE*>(VirtualAlloc(nullptr,64,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if(!memory)return false;
    BYTE* p=memory;const auto emit=[&](std::initializer_list<BYTE> b){for(const auto x:b)*p++=x;};
    const auto imm=[&](const void* v){const auto u=reinterpret_cast<uint64_t>(v);std::memcpy(p,&u,8);p+=8;};
    // mov rax,kept; mov [rax],rcx; lock inc qword [rax+8]; movss [rax+0x10],xmm1; mov rax,next; jmp qword [rax]
    emit({0x48,0xB8});imm(kept);emit({0x48,0x89,0x08});emit({0xF0,0x48,0xFF,0x40,0x08});emit({0xF3,0x0F,0x11,0x48,0x10});
    emit({0x48,0xB8});imm(next);emit({0xFF,0x20});
    DWORD old{};if(!VirtualProtect(memory,64,PAGE_EXECUTE_READ,&old))return false;
    FlushInstructionCache(GetCurrentProcess(),memory,64);
    return MH_CreateHook(reinterpret_cast<void*>(image+rva),memory,next)==MH_OK;
}
bool Install() noexcept {
    // The test hooks of series 14-15 (view-delay-<n>, camera-step-4) are gone since method B (D-0243): their record is the RESULT.
    if(camera_step::active)return Thunk(0x5E1B80,&pad_update,&pad_update_original);   // its two entries are camera_step's (rows: Sync)
    if(!camera_laws::active && !Thunk(0x515790,&seen,&original))return false;            // else the entry is camera_laws' (rows: Sync)
    if(!Thunk(0x510290,&view_copy,&view_copy_original))return false;
    return Thunk(0x5E1B80,&pad_update,&pad_update_original);
}
// With camera_step on, the rows take its object, ChrCam and its counts (ticks as the camera's calls); with camera_laws, its object,
// calls and last dt.
void Sync() noexcept {
    if(camera_laws::active){seen.self=camera_laws::self_seen;seen.calls=camera_laws::calls;seen.dt=camera_laws::last_dt;return;}
    if(!camera_step::active)return;
    seen.self=camera_step::kept;seen.calls=camera_step::ticks;seen.dt=camera_step::last_tick_dt;view_copy.self=camera_step::chr_cam;view_copy.calls=camera_step::copies;
}
}
// I-0114 (locomotion-observe.flag): the player's calls of the turn site 1405D0010 (unlock_fix::Turning, 15), told from the other
// characters' by their previous state's heading [previous+4] equal to the player's (the physics module's +0x74, kept each frame
// by ApplyWitnessTeleport): calls, stops (out[9]: the growing-error stop, speed 0), snaps (the turn ended on its target: out[8]
// zero), and the last call's error out[5], step (out[1] - previous[1]), rate and the game's coefficients 0 and 2 (unscaled). Approximate counts: the calls may run on workers.
namespace player_turn {
bool observe{};
std::atomic<float> heading{std::numeric_limits<float>::quiet_NaN()};
volatile LONG64 calls{},stopped{},snapped{};
std::atomic<float> error{},step{},rate{},c0{},c2{};
void Note(const float* out,float,float call_rate,const float* coefficients,const uint64_t* previous) noexcept {
    if(!observe || !previous || !out)return;
    const float was=reinterpret_cast<const float*>(previous)[1];
    if(!(std::abs(was-heading.load(std::memory_order_relaxed))<1e-4f))return;
    InterlockedIncrement64(&calls);
    if(reinterpret_cast<const uint8_t*>(out)[0x24])InterlockedIncrement64(&stopped);
    if(out[8]==0.f)InterlockedIncrement64(&snapped);
    error.store(out[5],std::memory_order_relaxed);step.store(out[1]-was,std::memory_order_relaxed);rate.store(call_rate,std::memory_order_relaxed);
    if(coefficients){c0.store(coefficients[0],std::memory_order_relaxed);c2.store(coefficients[2],std::memory_order_relaxed);}
}
}
// I-0114 tranche 7: the target timer's calls (unlock_fix::TargetTimer, 15; counted with jump-observe.flag), in the jump-observe rows.
namespace target_timer_watch {std::atomic<LONG64> calls{};}
// I-0114 tranche 3 (locomotion-observe.flag): the player's terrain response [+0x2DC] (unlock_fix's terrain detours, 15), told by its
// character [self+8] equal to the player ([WorldChrMan+0x80], kept each frame by ApplyWitnessTeleport): its last value, the site
// and branch that set it, its calls and its total variation (the sum of |after - before| a call: TRTF's hopping measure); and, for
// every character, the corrected calls and the calls whose result matched none of the original's branches (left as they were).
// Approximate counts: the calls may run on workers.
namespace terrain_watch {
enum Branch {Unchanged,Response,Decay,Reset,Bounded,Mismatch};
constexpr const char* site_names[]{"e6580","d2280","ca3f0"};
constexpr const char* branch_names[]{"unchanged","response","decay","reset","bounded","mismatch"};
bool observe{};
std::atomic<uintptr_t> player{};
std::atomic<float> value{};std::atomic<int> site{-1},branch{-1};
std::atomic<double> variation{};
volatile LONG64 calls{},corrected{},mismatched{};
void Note(uintptr_t self,int at,Branch kind,float before,float after,bool wrote) noexcept {
    if(wrote && observe)InterlockedIncrement64(&corrected);   // final review: the count only with its rows
    if(kind==Mismatch){const auto n=InterlockedIncrement64(&mismatched);
        if(n<=3){char row[192]{};sprintf_s(row,"{\"event\":\"unlock_terrain_mismatch\",\"site\":\"%s\",\"before\":%.6f,\"after\":%.6f,\"count\":%lld}\n",site_names[at],before,after,static_cast<long long>(n));Log(row);}}
    if(!observe || *reinterpret_cast<const uintptr_t*>(self+8)!=player.load(std::memory_order_relaxed))return;
    InterlockedIncrement64(&calls);value.store(after,std::memory_order_relaxed);site.store(at,std::memory_order_relaxed);branch.store(kind,std::memory_order_relaxed);
    variation.store(variation.load(std::memory_order_relaxed)+std::abs(double(after)-double(before)),std::memory_order_relaxed);
}
}
uintptr_t WitnessPhysics(uintptr_t manager) noexcept {
    uintptr_t player{},modules{},physics{};
    return Read(manager+0x80,&player,8) && player && Read(player+0x1F90,&modules,8) && modules && Read(modules+0x68,&physics,8)?physics:0;
}
void ApplyWitnessTeleport(uintptr_t manager,LONG64 call) noexcept {
    static std::wstring path;static bool resolved{};static float target[3]{};static ULONGLONG hold_until{};
    if(hold_until){if(GetTickCount64()<hold_until){if(const auto physics=WitnessPhysics(manager))WriteWitnessPosition(physics+0x80,target);return;}
        hold_until=0;float now[4]{};if(const auto physics=WitnessPhysics(manager))Read(physics+0x80,now,16);
        char row[160]{};sprintf_s(row,"{\"event\":\"witness_teleport_released\",\"at\":[%.2f,%.2f,%.2f]}\n",now[0],now[1],now[2]);Log(row);}
    if(!resolved){resolved=true;path=ModuleSibling(L"witness-teleport.txt");}
    // DECALS-WAVE1 (decal-observe.flag): the player's position every 300th call, to aim a teleported witness at an NPC.
    {static const bool observe=ModuleFlag(L"decal-observe.flag") || ModuleFlag(L"chr-positions.flag");if(observe && call%300==0){float now[4]{};if(const auto physics=WitnessPhysics(manager))Read(physics+0x80,now,16);
        char row[160]{};sprintf_s(row,"{\"event\":\"witness_player_position\",\"call\":%lld,\"at\":[%.3f,%.3f,%.3f]}\n",static_cast<long long>(call),now[0],now[1],now[2]);Log(row);}}
    // I-0114 (locomotion-observe.flag): the player's position every 15th call with the logic's time, to compare a path's distances
    // at 60 and unlocked against the logic's clock.
    // With the follow camera seen (follow_camera above): its yaw, wall-recovery step and focus, its calls and last dt; and the
    // physics module's four floats at +0x70 (raw: the character's rotation, INFERRED).
    {static const bool locomotion=ModuleFlag(L"locomotion-observe.flag");
    if(locomotion){player_turn::observe=true;float heading{};if(const auto physics=WitnessPhysics(manager);physics && Read(physics+0x74,&heading,4))player_turn::heading.store(heading,std::memory_order_relaxed);
        uintptr_t player{};if(Read(manager+0x80,&player,8))terrain_watch::player.store(player,std::memory_order_relaxed);terrain_watch::observe=true;}
    // The display's smoothness (camera_step's open point): each frame the follow camera's view position +0x40 and forward yaw
    // (+0x30), and the character's position; every 600 frames the mean per-frame displacement and the mean second difference
    // (|p(n) - 2 p(n-1) + p(n-2)|) of each, and the largest camera second difference. A view that judders at the ticks shows a
    // camera second difference far above the character's.
    if(locomotion){follow_camera::Sync();static double sums[6]{};static float worst{};static int frames{};static float history[3][7]{};static int filled{};
        float view[3]{},forward[3]{},body[4]{};const auto camera=follow_camera::seen.self;
        static uintptr_t smoothed{};if(camera!=smoothed){smoothed=camera;filled=0;}   // step review L6: no difference across a gap
        if(camera && Read(camera+0x40,view,12) && Read(camera+0x30,forward,12)){if(const auto physics=WitnessPhysics(manager))Read(physics+0x80,body,16);
            const float now[7]{view[0],view[1],view[2],std::atan2(forward[0],forward[2]),body[0],body[1],body[2]};
            std::memmove(history[0],history[1],sizeof(history[0])*2);std::memcpy(history[2],now,sizeof(now));
            if(++filled>=3){const auto& a=history[0];const auto& b=history[1];const auto& c=history[2];
                const auto wrap=[](float v){while(v>3.14159265f)v-=6.2831853f;while(v<-3.14159265f)v+=6.2831853f;return v;};
                const float cam1=std::sqrt((c[0]-b[0])*(c[0]-b[0])+(c[1]-b[1])*(c[1]-b[1])+(c[2]-b[2])*(c[2]-b[2]));
                const float cam2=std::sqrt((c[0]-2*b[0]+a[0])*(c[0]-2*b[0]+a[0])+(c[1]-2*b[1]+a[1])*(c[1]-2*b[1]+a[1])+(c[2]-2*b[2]+a[2])*(c[2]-2*b[2]+a[2]));
                const float yaw1=std::abs(wrap(c[3]-b[3])),yaw2=std::abs(wrap(wrap(c[3]-b[3])-wrap(b[3]-a[3])));
                const float chr1=std::sqrt((c[4]-b[4])*(c[4]-b[4])+(c[5]-b[5])*(c[5]-b[5])+(c[6]-b[6])*(c[6]-b[6]));
                const float chr2=std::sqrt((c[4]-2*b[4]+a[4])*(c[4]-2*b[4]+a[4])+(c[5]-2*b[5]+a[5])*(c[5]-2*b[5]+a[5])+(c[6]-2*b[6]+a[6])*(c[6]-2*b[6]+a[6]));
                if(cam1<1.f && chr1<1.f){sums[0]+=cam1;sums[1]+=cam2;sums[2]+=yaw1;sums[3]+=yaw2;sums[4]+=chr1;sums[5]+=chr2;worst=(std::max)(worst,cam2);++frames;}}
            if(frames>=600){char row[320]{};sprintf_s(row,"{\"event\":\"unlock_view_smoothness\",\"call\":%lld,\"frames\":%d,\"view\":[%.6f,%.6f],\"yaw\":[%.7f,%.7f],\"body\":[%.6f,%.6f],\"view_worst\":%.6f}\n",
                    static_cast<long long>(call),frames,sums[0]/frames,sums[1]/frames,sums[2]/frames,sums[3]/frames,sums[4]/frames,sums[5]/frames,worst);Log(row);
                for(auto& s:sums)s=0;worst=0;frames=0;}}}
    // mechanics-observe.flag (UNLOCK-READY §5's mechanics: rolls, stops, turns, attacks): the rows every 1/30 s of the logic's time
    // instead of every 15th call, so 60 and an unlocked rate are sampled alike and a roll is seen at 60 too.
    static const bool mechanics=ModuleFlag(L"mechanics-observe.flag");static double next_row{};
    const bool row_due=mechanics?native_logic_seconds>=next_row:call%15==0;if(mechanics && row_due)next_row=native_logic_seconds+1.0/30.0;
    // I-0132 (ROADMAP workstream 2 step 3, the 60 frames/s play through the corrections installed at every start): the rows with
    // mechanics-observe.flag alone too, so 60 with the corrections is observed (locomotion-observe keeps them out at the rate 0); without
    // locomotion-observe the turn site's counts stay 0 (player_turn not observing), and with no-unlock-fix.flag too the camera's fields
    // (no camera_laws, no observing thunk: no follow camera seen; Get-Mechanics.py notes it), so the original's logic is observed with
    // locomotion-observe as before.
    if((locomotion || mechanics) && row_due){float now[4]{},turn[4]{};
        if(const auto physics=WitnessPhysics(manager)){Read(physics+0x80,now,16);Read(physics+0x70,turn,16);}
        follow_camera::Sync();float yaw{},step{},focus[3]{};const auto camera=follow_camera::seen.self;
        if(camera){Read(camera+0x144,&yaw,4);Read(camera+0x314,&step,4);Read(camera+0x90,focus,12);}
        // The lower-body twist's tracker (the study thread, GhidraMCP 2026-10-31: 1405CFF00 writes its [+0x1C] x 57.2958 into the
        // Havok variable TwistLowerRootAngle): [[[[PlayerCtrl+0x348]+0x40]+0x30]], PlayerCtrl [ChrIns+0x50] (INFERRED, checked here:
        // its vtable 1427CF078 and [PlayerCtrl+8] the player); its target +0x18, value +0x1C, gain +0x20, mode +0x58. -1: not found.
        float twist[3]{-1,-1,-1};int twist_mode=-1;
        {uintptr_t player{},ctrl{},table{},back{},a{},x{},tracker{};
            if(Read(manager+0x80,&player,8) && player && Read(player+0x50,&ctrl,8) && ctrl && Read(ctrl,&table,8) && table==image+0x27CF078 &&
               Read(ctrl+8,&back,8) && back==player && Read(ctrl+0x348,&a,8) && a && Read(a+0x40,&x,8) && x && Read(x+0x30,&tracker,8) && tracker &&
               Read(tracker,&tracker,8) && tracker && Read(tracker+0x18,twist,12))Read(tracker+0x58,&twist_mode,4);}
        char row[640]{};sprintf_s(row,"{\"event\":\"unlock_player_position\",\"call\":%lld,\"t\":%.4f,\"at\":[%.3f,%.3f,%.3f],\"phys70\":[%.4f,%.4f,%.4f,%.4f],"
            "\"cam\":{\"yaw\":%.5f,\"step\":%.5f,\"focus\":[%.3f,%.3f,%.3f],\"calls\":%lld,\"dt\":%.5f},\"twist\":[%.5f,%.5f,%.5f,%d],"
            "\"turn\":{\"calls\":%lld,\"stopped\":%lld,\"snapped\":%lld,\"error\":%.5f,\"step\":%.6f,\"rate\":%.4f,\"c0\":%.4f,\"c2\":%.4f}}\n",static_cast<long long>(call),native_logic_seconds,now[0],now[1],now[2],
            turn[0],turn[1],turn[2],turn[3],yaw,step,focus[0],focus[1],focus[2],static_cast<long long>(follow_camera::seen.calls),follow_camera::seen.dt,
            twist[0],twist[1],twist[2],twist_mode,static_cast<long long>(player_turn::calls),static_cast<long long>(player_turn::stopped),
            static_cast<long long>(player_turn::snapped),player_turn::error.load(std::memory_order_relaxed),player_turn::step.load(std::memory_order_relaxed),
            player_turn::rate.load(std::memory_order_relaxed),player_turn::c0.load(std::memory_order_relaxed),player_turn::c2.load(std::memory_order_relaxed));Log(row);
        // I-0114 tranche 4 (the jump 3-5 % short above 60, series 27): the player's move as PlayerCtrl hands it to the mover
        // (1405CA570, GhidraMCP 2026-11-01): the call's displacement [ctrl+0x1D0..0x1DC] after the virtual +0xD8 (1405E57F0) scaled
        // it by [ctrl+0x384] (= [[owner+0xB8]+0x10] · [manip+0x2D0]), its speed [ctrl+0x260] (|displacement| / dt), the locomotion
        // factor [ctrl+0x25C] (1405C72B0's, applied when under 1); the mover's [+0x1A0..0x1A8] and its time object's dt.
        {uintptr_t player{},ctrl{},table{},back{},owner{},times{},physics{};float disp[4]{},speed{},factor{},scale{},mover[3]{},step_dt{};
            if(Read(manager+0x80,&player,8) && player && Read(player+0x50,&ctrl,8) && ctrl && Read(ctrl,&table,8) && table==image+0x27CF078 &&
               Read(ctrl+8,&back,8) && back==player){Read(ctrl+0x1D0,disp,16);Read(ctrl+0x260,&speed,4);Read(ctrl+0x25C,&factor,4);Read(ctrl+0x384,&scale,4);
                if(Read(player+0x1F90,&owner,8) && owner){if(Read(owner+0x68,&physics,8) && physics)Read(physics+0x1A0,mover,12);
                    if(Read(owner+0x90,&times,8) && times)Read(times+0x1C,&step_dt,4);}
                char move_row[320]{};sprintf_s(move_row,"{\"event\":\"unlock_player_move\",\"call\":%lld,\"t\":%.4f,\"disp\":[%.5f,%.5f,%.5f,%.5f],\"speed\":%.4f,\"factor\":%.5f,\"scale\":%.5f,\"mover\":[%.4f,%.4f,%.4f],\"dt\":%.5f}\n",
                    static_cast<long long>(call),native_logic_seconds,disp[0],disp[1],disp[2],disp[3],speed,factor,scale,mover[0],mover[1],mover[2],step_dt);Log(move_row);}}
        // The player's terrain response (terrain_watch above), when its detours are installed.
        if(terrain_watch::calls){const int at=terrain_watch::site.load(std::memory_order_relaxed),kind=terrain_watch::branch.load(std::memory_order_relaxed);
            char terrain_row[320]{};sprintf_s(terrain_row,"{\"event\":\"unlock_terrain\",\"call\":%lld,\"t\":%.4f,\"x\":%.6f,\"site\":\"%s\",\"branch\":\"%s\",\"calls\":%lld,\"variation\":%.6f,\"corrected\":%lld,\"mismatched\":%lld}\n",
                static_cast<long long>(call),native_logic_seconds,terrain_watch::value.load(std::memory_order_relaxed),at>=0?terrain_watch::site_names[at]:"none",
                kind>=0?terrain_watch::branch_names[kind]:"none",static_cast<long long>(terrain_watch::calls),terrain_watch::variation.load(std::memory_order_relaxed),
                static_cast<long long>(terrain_watch::corrected),static_cast<long long>(terrain_watch::mismatched));Log(terrain_row);}
        // The heading's target (CAMERA-FOLLOW-SITES §8): the pad manipulator [[ChrIns+0x50]+0x318] (else +0x110), its view angles
        // [+0x90..0x9C] (pitch, yaw: 140510290's copy) and wanted delta [+0x20..0x2C] (1405E2810's); CameraMan's forward
        // [+0x30..0x38]; the calls of 140510290 and 1405E1B80 so far (the row's call is the game step's).
        float aim[4]{},wanted[4]{},forward[3]{};
        {uintptr_t player{},ctrl{},manip{};
            if(Read(manager+0x80,&player,8) && player && Read(player+0x50,&ctrl,8) && ctrl && ((Read(ctrl+0x318,&manip,8) && manip) || (Read(ctrl+0x110,&manip,8) && manip))){
                Read(manip+0x90,aim,16);Read(manip+0x20,wanted,16);}}
        if(const auto camera_man=follow_camera::view_copy.self)Read(camera_man+0x30,forward,12);
        char target_row[320]{};sprintf_s(target_row,"{\"event\":\"unlock_heading_target\",\"call\":%lld,\"aim\":[%.5f,%.5f],\"wanted\":[%.5f,%.5f,%.5f,%.5f],\"forward\":[%.4f,%.4f,%.4f],"
            "\"view_copies\":%lld,\"pad_updates\":%lld}\n",static_cast<long long>(call),aim[0],aim[1],wanted[0],wanted[1],wanted[2],wanted[3],forward[0],forward[1],forward[2],
            static_cast<long long>(follow_camera::view_copy.calls),static_cast<long long>(follow_camera::pad_update.calls));Log(target_row);
        // The follow camera's view law (the study thread, 140515790's end, CAMERA-FOLLOW-SITES §9): the wanted position [+0xF0], the
        // previous final position [+0x100], the view's focus [+0xC0]; the weights [+0x1BC] (horizontal) and [+0x1C8] (vertical),
        // the horizontal dead band's angles [+0x1CC] and [+0x1D0], the recentring timer [+0x130].
        // The yaw's own trail (§9 (2)): the lazy position [+0xE0] and the focus [+0xD0] its direction is taken from, the trail's
        // gains [+0x1B4] and [+0x164], and the focus smoothing's gain [+0x1A8].
        if(camera){float wanted_at[3]{},last_at[3]{},view_focus[3]{},lazy[3]{},yaw_focus[3]{},law[8]{};
            Read(camera+0xF0,wanted_at,12);Read(camera+0x100,last_at,12);Read(camera+0xC0,view_focus,12);Read(camera+0xE0,lazy,12);Read(camera+0xD0,yaw_focus,12);
            Read(camera+0x1BC,&law[0],4);Read(camera+0x1C8,&law[1],4);Read(camera+0x1CC,&law[2],4);Read(camera+0x1D0,&law[3],4);Read(camera+0x130,&law[4],4);
            Read(camera+0x1B4,&law[5],4);Read(camera+0x164,&law[6],4);Read(camera+0x1A8,&law[7],4);
            char view_row[640]{};sprintf_s(view_row,"{\"event\":\"unlock_follow_view\",\"call\":%lld,\"wanted\":[%.4f,%.4f,%.4f],\"last\":[%.4f,%.4f,%.4f],\"focus\":[%.4f,%.4f,%.4f],"
                "\"lazy\":[%.4f,%.4f,%.4f],\"yaw_focus\":[%.4f,%.4f,%.4f],\"w_h\":%.5f,\"w_v\":%.5f,\"band\":[%.5f,%.5f],\"recentre\":%.4f,\"trail\":[%.5f,%.5f],\"focus_gain\":%.5f}\n",
                static_cast<long long>(call),wanted_at[0],wanted_at[1],wanted_at[2],last_at[0],last_at[1],last_at[2],view_focus[0],view_focus[1],view_focus[2],
                lazy[0],lazy[1],lazy[2],yaw_focus[0],yaw_focus[1],yaw_focus[2],law[0],law[1],law[2],law[3],law[4],law[5],law[6],law[7]);Log(view_row);}}}
    // I-0114 tranche 4 (jump-observe.flag, with locomotion-observe.flag): every game step for 120 s of the logic's time from the first
    // step whose requested speed [ctrl+0x260] passes 0.5 m/s (tranche 7; before, 30 s from 6.5 m/s, the first jump of firelink-jumps), the mover as 1409DEE60 (GhidraMCP
    // 2026-11-01) sees it: the proxy's velocity read back from Havok [mover+0x120..0x128] (kept in the air: [+0xD0] set and [+0xA2]
    // clear, or [+0x1DB]), those flags and [+0x1D8], [+0x1DA], [+0x1DC]; the position [+0x80]; the requested speed and the step's dt.
    {static const bool jump=ModuleFlag(L"jump-observe.flag") && ModuleFlag(L"locomotion-observe.flag");static double until{-1};
    if(jump){uintptr_t player{},ctrl{},table{},owner{},physics{},times{};float speed{},velocity[4]{},at[4]{},step_dt{};uint8_t flags[6]{};
        if(Read(manager+0x80,&player,8) && player && Read(player+0x50,&ctrl,8) && ctrl && Read(ctrl,&table,8) && table==image+0x27CF078 && Read(ctrl+0x260,&speed,4) &&
           Read(player+0x1F90,&owner,8) && owner && Read(owner+0x68,&physics,8) && physics){
            if(until<0 && speed>0.5f)until=native_logic_seconds+120.0;   // tranche 7: from the first motion, 120 s (the rolls and strafes too)
            if(until>=0 && native_logic_seconds<=until){Read(physics+0x120,velocity,16);Read(physics+0x80,at,16);
                Read(physics+0xD0,&flags[0],1);Read(physics+0xA2,&flags[1],1);Read(physics+0x1DB,&flags[2],1);Read(physics+0x1D8,&flags[3],1);Read(physics+0x1DA,&flags[4],1);Read(physics+0x1DC,&flags[5],1);
                if(Read(owner+0x90,&times,8) && times)Read(times+0x1C,&step_dt,4);
                char jump_row[512]{};_snprintf_s(jump_row,sizeof jump_row,_TRUNCATE,"{\"event\":\"unlock_jump_step\",\"call\":%lld,\"t\":%.5f,\"dt\":%.5f,\"speed\":%.4f,\"v\":[%.4f,%.4f,%.4f],\"at\":[%.4f,%.4f,%.4f],\"d0\":%u,\"a2\":%u,\"1db\":%u,\"1d8\":%u,\"1da\":%u,\"1dc\":%u,\"tt\":%lld}\n",
                    static_cast<long long>(call),native_logic_seconds,step_dt,speed,velocity[0],velocity[1],velocity[2],at[0],at[1],at[2],flags[0],flags[1],flags[2],flags[3],flags[4],flags[5],static_cast<long long>(target_timer_watch::calls.load(std::memory_order_relaxed)));Log(jump_row);}}}}
    if(path.empty() || !WitnessToolsAllowed() || call%15 || GetFileAttributesW(path.c_str())==INVALID_FILE_ATTRIBUTES)return;
    FILE* file{};if(_wfopen_s(&file,path.c_str(),L"rb") || !file)return;
    float hold_seconds{8};const bool parsed=fscanf_s(file,"%f %f %f %f",&target[0],&target[1],&target[2],&hold_seconds)>=3 && hold_seconds>0 && hold_seconds<=600;fclose(file);DeleteFileW(path.c_str());
    float before[4]{};const auto physics=WitnessPhysics(manager);
    const bool found=parsed && std::isfinite(target[0]) && std::isfinite(target[1]) && std::isfinite(target[2]) && physics && Read(physics+0x80,before,16);
    const bool written=found && WriteWitnessPosition(physics+0x80,target);if(written)hold_until=GetTickCount64()+static_cast<ULONGLONG>(hold_seconds*1000);
    char row[256]{};sprintf_s(row,"{\"event\":\"witness_teleport\",\"applied\":%s,\"from\":[%.2f,%.2f,%.2f],\"to\":[%.2f,%.2f,%.2f]}\n",
        written?"true":"false",before[0],before[1],before[2],target[0],target[1],target[2]);Log(row);
}
// Phantom witness (test tooling, both modes; PHANTOMS-SIGNS-NATIVE P15/P16): witness-phantom.txt next to this module holds a
// PhantomParam id; the player's first resolver source chr+0x1F38 (88A950 returns it when it is not negative) takes it on
// every schedule call from then on, so the genuine chain (9CAF30 overrides, slot 214, CFDE70 state bytes) draws the player
// as that phantom in either renderer. The file is removed once read.
void ApplyWitnessPhantom(uintptr_t manager,LONG64 call) noexcept {
    static std::wstring path;static bool resolved{};static int32_t forced{-1};static LONG64 logged{};
    if(!resolved){resolved=true;path=ModuleSibling(L"witness-phantom.txt");}
    uintptr_t player{};
    if(forced>=0 && Read(manager+0x80,&player,8) && player)WriteWitnessWord(player+0x1F38,forced);
    if(path.empty() || !WitnessToolsAllowed() || call%15 || GetFileAttributesW(path.c_str())==INVALID_FILE_ATTRIBUTES)return;
    // A new file replaces the id; 0 releases it (the source goes back to -1, the host's value).
    FILE* file{};if(_wfopen_s(&file,path.c_str(),L"rb") || !file)return;
    int32_t id{-1};const bool parsed=fscanf_s(file,"%d",&id)==1 && id>=0 && id<100000;fclose(file);DeleteFileW(path.c_str());
    int32_t before{};const bool found=parsed && Read(manager+0x80,&player,8) && player && Read(player+0x1F38,&before,4);
    const bool applied=found && WriteWitnessWord(player+0x1F38,id?id:-1);if(applied)forced=id?id:-1;
    if(InterlockedIncrement64(&logged)<=16){char row[160]{};
        sprintf_s(row,"{\"event\":\"witness_phantom\",\"applied\":%s,\"id\":%d,\"before\":%d}\n",applied?"true":"false",id,before);Log(row);}
}
// Menu tests R5 (ENDING-CREDITS-NATIVE 5, test tooling, both modes): witness-flag.txt next to this module holds "id value"; on the game
// thread the game's own event-flag setter 1404C5E20(event flag manager [144752F68], id, value, 0) takes it (flag 20 runs the ending
// chain: 1408F5930, EndingStep, the credits movie); the file is removed once read.
void ApplyWitnessFlag(LONG64 call) noexcept {
    static std::wstring path;static bool resolved{};
    if(!resolved){resolved=true;path=ModuleSibling(L"witness-flag.txt");}
    if(path.empty() || !WitnessToolsAllowed() || call%15 || GetFileAttributesW(path.c_str())==INVALID_FILE_ATTRIBUTES)return;
    FILE* file{};if(_wfopen_s(&file,path.c_str(),L"rb") || !file)return;
    int32_t id{-1},value{1};const bool parsed=fscanf_s(file,"%d %d",&id,&value)>=1 && id>=0 && id<100000000;fclose(file);DeleteFileW(path.c_str());
    uintptr_t manager{};const bool found=parsed && Read(image+0x4752F68,&manager,8) && manager;
    if(found)reinterpret_cast<void(__fastcall*)(uintptr_t,uint64_t,char,char)>(image+0x4C5E20)(manager,static_cast<uint64_t>(id),value?1:0,0);
    char row[160]{};sprintf_s(row,"{\"event\":\"witness_flag\",\"applied\":%s,\"id\":%d,\"value\":%d}\n",found?"true":"false",id,value);Log(row);
}
// I-0139 (the tour: a boss's phases and death, a DLC entrance, read by their event flags; test tooling, both modes): witness-flag-
// watch.txt next to this module holds up to 16 event flag ids, taken once (the file removed, or left and never read again); then,
// sampled every 60th schedule call (a flag set and cleared between two samples is not seen), each is read as the game's own getter
// 1404C5350 reads it (called by 1404AB310 on the manager [144752F68] with r8 0): its manager's +0x228 byte set, the block from
// 1404C7F00(manager, id, 0), then bit 31 - (id % 1000 & 31) of the block's word (id % 1000) >> 5 (OBSERVED in the disassembly,
// GhidraMCP 2026-11-15; the setter 1404C5E20 makes the same lookup). The block is called here, not the getter, so a flag the manager
// holds no block for is written "value":-1, never 0 (the step review's M1). A row at the first sample of each flag and at each change;
// at 512 rows one "capped" row and no more.
void ApplyWitnessFlagWatch(LONG64 call) noexcept {
    static std::wstring path;static bool resolved{},taken{},capped{};static int32_t ids[16]{};static int8_t last[16]{};static int count{},rows{};
    if(!resolved){resolved=true;path=ModuleSibling(L"witness-flag-watch.txt");}
    if(path.empty())return;
    if(!taken){
        if(!WitnessToolsAllowed() || call%15 || GetFileAttributesW(path.c_str())==INVALID_FILE_ATTRIBUTES)return;
        FILE* file{};if(_wfopen_s(&file,path.c_str(),L"rb") || !file)return;
        taken=true;   // read once, removed or not (the step review's L1)
        int32_t id{};while(count<16 && fscanf_s(file,"%d",&id)==1)if(id>=0 && id<100000000){ids[count]=id;last[count]=-2;++count;}
        fclose(file);const bool removed=DeleteFileW(path.c_str())!=FALSE;
        char row[96]{};sprintf_s(row,"{\"event\":\"witness_flag_watch\",\"count\":%d,\"removed\":%s}\n",count,removed?"true":"false");Log(row);
        return;
    }
    if(!count || capped || call%60)return;
    uintptr_t manager{};uint8_t ready{};if(!Read(image+0x4752F68,&manager,8) || !manager || !Read(manager+0x228,&ready,1) || !ready)return;
    const auto block_of=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,int32_t,uint64_t)>(image+0x4C7F00);
    for(int k=0;k<count;++k){
        const uintptr_t block=block_of(manager,ids[k],0);const uint32_t bit=static_cast<uint32_t>(ids[k]%1000);
        uintptr_t words{};uint32_t word{};int8_t value=-1;
        if(block && Read(block,&words,8) && words && Read(words+(bit>>5)*4ull,&word,4))value=static_cast<int8_t>((word>>(31-(bit&31)))&1);
        if(value==last[k])continue;
        if(rows>=512){capped=true;Log("{\"event\":\"witness_flag_watch\",\"capped\":true}\n");return;}
        last[k]=value;++rows;char row[128]{};
        sprintf_s(row,"{\"event\":\"witness_flag_state\",\"id\":%d,\"value\":%d,\"call\":%lld}\n",ids[k],value,static_cast<long long>(call));Log(row);
    }
}
// I-0139 tranche 3 (the tour's cutscenes, test tooling, both modes): witness-cutscene.txt next to this module holds "id [mode]" (a Remo
// cutscene id AABBVVVV, mode 0 by default); on the game thread, once the player exists, the game's own handler of the EMEVD cutscene
// instruction with the player as its entity, 140474280 (RCX unused; EDX the id, R8D the mode, R9D and the 5th argument the Lua
// callback's event and flag, -1 and -1 for none; the 6th the entity, 10000 the player), as EMEVD 2002[3] does: it builds the
// SprjRemoRequest on its own stack and hands it to SprjRemoImp::Request (140CE2810) through the singleton [144791248], which it
// asserts (so it is checked here first); OBSERVED in its disassembly (GhidraMCP, 2026-11-13, evidence/I-0139/artifacts/static/
// remo-request.md). The file is removed once read; the row says whether the request was handed over.
std::atomic<int32_t> witness_cutscene_id{-1};   // I-0138 tranche 8g: the id handed, which the Remo clock keeps to
void ApplyWitnessCutscene(uintptr_t manager,LONG64 call) noexcept {
    static std::wstring path;static bool resolved{};
    if(!resolved){resolved=true;path=ModuleSibling(L"witness-cutscene.txt");}
    if(path.empty() || !WitnessToolsAllowed() || call%15 || GetFileAttributesW(path.c_str())==INVALID_FILE_ATTRIBUTES)return;
    // No world yet, or a singleton the request asserts not there yet: the file waits (the final review's L1: besides SprjRemoImp
    // [144791248], the Ins constructor asserts the FD4 remo manager [1449649A0] and [14489C7A8], remo-request.md §5).
    uintptr_t player{},imp{},fd4{},other{};if(!Read(manager+0x80,&player,8) || !player)return;
    if(!Read(image+0x4791248,&imp,8) || !imp || !Read(image+0x49649A0,&fd4,8) || !fd4 || !Read(image+0x489C7A8,&other,8) || !other)return;
    FILE* file{};if(_wfopen_s(&file,path.c_str(),L"rb") || !file)return;
    // The step review's lows: the id an AABBVVVV of a DS3 map (30 to 51), the mode one the EMEVD data use (0, 2, 8, 10: the other
    // bits' meaning is OPEN), and nothing requested unless the file is gone (a file left behind would queue one cutscene a check).
    int32_t id{-1},mode{0};const bool parsed=fscanf_s(file,"%d %d",&id,&mode)>=1 && id>=30000000 && id<52000000 && (mode==0 || mode==2 || mode==8 || mode==10);
    fclose(file);const bool removed=DeleteFileW(path.c_str())!=FALSE;
    const bool ready=parsed && removed && Read(image+0x4791248,&imp,8) && imp;bool handed{};   // re-read just before the call
    if(ready)handed=reinterpret_cast<bool(__fastcall*)(void*,uint32_t,uint32_t,uint32_t,int32_t,int32_t)>(image+0x474280)(
        nullptr,static_cast<uint32_t>(id),static_cast<uint32_t>(mode),0xFFFFFFFFu,-1,10000);
    if(handed){witness_cutscene_id.store(id,std::memory_order_relaxed);witness_cutscene_handed.store(true,std::memory_order_relaxed);}   // I-0140 tranche 5: pictures-from-cutscene.flag
    char row[192]{};sprintf_s(row,"{\"event\":\"witness_cutscene\",\"parsed\":%s,\"removed\":%s,\"ready\":%s,\"handed\":%s,\"id\":%d,\"mode\":%d}\n",
        parsed?"true":"false",removed?"true":"false",ready?"true":"false",handed?"true":"false",id,mode);Log(row);
}
// I-0138 tranche 8g (D-0299: the two modes compared at the same moment of the game, on its own clock; test tooling, both modes):
// once witness-cutscene.txt's cutscene is handed, the current Remo cut's own clock each game frame, read in SprjRemoIns's Play_Wait
// step just before its Play step (on that step's own thread, so a hold is the frame the step evaluates). READ (GhidraMCP, DarkSoulsIII.exe
// 1.15.2; evidence/I-0138/t8f-readings/remo-cutscene-clock.md, its cut array corrected in tranche 8g by the pilot's reading of the
// Play step FUN_1423b1370: the cuts' pointers are an array AT [Impl+0x270], param_1[0x4e], not inline; the last cut +0x378,
// param_1[0x6f]): SprjRemoImp [144791248], Man [Imp+8], playing Man+0x154 bit 0, the Ins count Man+0xC0, the current Ins
// [[[Man+0xB8]]+0x10], its id +0xC8, the FD4 player W [Ins+0xD0], Impl [W+8]; Impl+0x294 the current cut (-1 none), +0x374 the
// first, +0x378 the last, +0x36C the play speed; Cut = [[Impl+0x270]+8 x index], its frame float +0x68 (the former +0x6C), its data
// [[Cut+0x58]+8] with the start frame int +0x44 and the end int +0x48. The Play step adds rate x dt x speed to +0x68 through
// FUN_1423cd9e0, which then evaluates the cut at that frame (its camera words +0x70..+0xAC), and moves to the next cut past end + 1
// (143D7EE10 holds 1.0f). The scene frame here (ours, not the game's: every cut starts at frame -2, so it steps back about 2 at each cut
// change): the earlier cuts' end + 1 - start summed from the first cut, plus the current frame - its start; the same data in both
// modes give the same sum (pairs compare the cut and its frame).
// Rows: witness_cutscene_cuts once the cuts are read (their starts and ends), witness_cutscene_clock at each cut change, each change
// of the globals' glare shape (+0xD8 of 144961F70, as Play_Wait sees it) or its luminance's sign, and every 30 calls while valid, at
// most 4000.
// Holds (witness-cutscene-holds.txt next to this module, read once and removed: « ms t1 t2 ... », ms 500 to 30000 (20000 before I-0141 tranche 26's share variants), up to 32 targets,
// each a scene frame or « p<percent> » of the scene's whole length): the first frame whose scene frame reaches a target (within 2
// frames past it, in the cut that holds it) sets the play speed to 0 and the cut's frame (+0x68 and +0x6C) to the target's, so the
// Play steps from that one on evaluate the scene at exactly that frame; the speed is restored when the runner drops witness-cutscene-shot.txt
// (its shot taken, the file naming this hold's index) or after ms (a timeout). A witness_cutscene_hold row at each hold (phase "hold"), release ("release", why "shot",
// "timeout" or "scene changed", with the frame read then and the largest drift seen during the hold), a speed restored late
// ("restored") and a target passed, out of reach or left when the scene ended ("missed"). A runner shoots during each hold
// (Run-AlphaCast -CutsceneHolds) and records its shot's ticks: without hold-world-freeze.flag only the Remo clock is held; the world's own simulation (effects,
// cloth, the exposure's adaptation) goes on during a hold, so a shot's offset from the hold's start is part of the moment (step review M3).
// With the flag (below) the clocks fed by the timeline's dt (the effects, the sky, the gparam blends) and the cloth's (+0x26C) are held
// too, from the hold's second frame (INFERRED, below); the exposure's adaptation and the fixed-step tasks go on even then. Step review M1: the clock is valid only with the cut's frame inside its own range (start - 2 to end + 2), so a wrong
// reading of the cuts never holds or writes. Step review L5: the scene frame tagged on the glare rows is Play_Wait's latest (before its step),
// which the render (original) and prepare (native) threads read with their own lag: only rows inside a hold name one moment.
std::atomic<uint64_t> cutscene_clock_tag{};   // bit 63 valid, bits 32-47 the cut, bits 0-31 the scene frame's bits
std::string CutsceneClockCell() {
    const uint64_t tag=cutscene_clock_tag.load(std::memory_order_relaxed);if(!(tag>>63))return {};
    float scene{};const uint32_t bits=uint32_t(tag);std::memcpy(&scene,&bits,4);char cell[96]{};   // scene bounded by the cuts' table (step review M1)
    sprintf_s(cell,",\"scene_cut\":%u,\"scene_frame\":%.3f",unsigned((tag>>32)&0xFFFF),scene);return cell;
}
bool WriteWitnessFloat(uintptr_t at,float value) noexcept {int32_t bits{};std::memcpy(&bits,&value,4);return WriteWitnessWord(at,bits);}
struct RemoClock {uintptr_t impl{},cuts{},cut{};int32_t id{-1},index{-1},first{},last{};float frame{},speed{};bool playing{},valid{};};
// The current Ins: the one Play_Wait was handed (given), else [[[Man+0xB8]]+0x10] while Man+0xC0 counts one. The chain's links are
// also returned for the one-shot witness_cutscene_chain row (the run of 02:52 read no clock: the WorldChrMan update, its only
// caller then, is not armed while a cutscene plays, which the row's journal showed as a 49 s gap between two calls).
struct RemoChain {uintptr_t imp{},man{},p1{},p2{},ins{};uint8_t flags{};int32_t count{};};
RemoChain ReadRemoChain() noexcept {
    RemoChain r;
    if(!Read(image+0x4791248,&r.imp,8) || !r.imp || !Read(r.imp+8,&r.man,8) || !r.man)return r;
    Read(r.man+0x154,&r.flags,1);
    if(!Read(r.man+0xC0,&r.count,4) || r.count<=0 || !Read(r.man+0xB8,&r.p1,8) || !r.p1 || !Read(r.p1,&r.p2,8) || !r.p2)return r;
    Read(r.p2+0x10,&r.ins,8);return r;
}
RemoClock ReadRemoClock(uintptr_t given) noexcept {
    RemoClock c;uintptr_t w{};const RemoChain chain=ReadRemoChain();
    c.playing=chain.man && (chain.flags&1)!=0;
    const uintptr_t ins=given?given:chain.ins;if(!ins)return c;
    Read(ins+0xC8,&c.id,4);
    if(!Read(ins+0xD0,&w,8) || !w || !Read(w+8,&c.impl,8) || !c.impl)return c;
    if(!Read(c.impl+0x270,&c.cuts,8) || !c.cuts || !Read(c.impl+0x294,&c.index,4) || !Read(c.impl+0x374,&c.first,4) || !Read(c.impl+0x378,&c.last,4) ||
        !Read(c.impl+0x36C,&c.speed,4))return c;
    if(c.first<0 || c.last<c.first || c.last>=1024 || c.index<c.first || c.index>c.last)return c;
    if(!Read(c.cuts+8ull*uint32_t(c.index),&c.cut,8) || !c.cut || !Read(c.cut+0x68,&c.frame,4) || !std::isfinite(c.frame))return c;
    c.valid=true;return c;
}
bool ReadRemoCutRange(uintptr_t cuts,int32_t k,int32_t& start,int32_t& end) noexcept {
    uintptr_t cut{},p{},data{};
    return Read(cuts+8ull*uint32_t(k),&cut,8) && cut && Read(cut+0x58,&p,8) && p && Read(p+8,&data,8) && data && Read(data+0x44,&start,4) && Read(data+0x48,&end,4) && end>=start;
}
// Two callers (the tranche's re-reading, GhidraMCP 2026-11-22): SprjRemoIns's Play_Wait step (CD5D80, given its Ins, once a game
// frame while the scene plays, before the step runs: the FD4 player W = [Ins+0xD0] (Play_Wait's param_1[0x1a]), 14235D8E0(W, ctx)
// jumps to [[W+8]]'s slot 0x10, the Play step, which CD5D80 skips when [Ins+0xCC] bit 1 is set or W is null) and the WorldChrMan
// update (ObserveChrManagerSchedule; 8FA3E0's running flag, which its caller 8F2CA0 clears while [[4791248]+8]+154 has bit 0 and
// not bit 5 (the one-shot [step+1F6] forces it for the frame it is set in, its setters not read, the 1086 witness's comment in 01_state.inc). Only Play_Wait builds the cuts' table, tags the
// frame and starts a hold, and only on a call whose Play step runs with nothing before it (step review M1, L3: [Ins+0xCC] bits 0 and
// 1 clear, W set), so a hold is the frame that step evaluates, on its thread; the WorldChrMan caller reads the holds' file, writes
// its clock rows (src "schedule", never valid) and the tag cleared, and closes a scene gone (its missed targets, a release, a late
// restore), and does nothing within a second of a kept Play_Wait call. One lock serialises the two. Play_Wait keeps to the Ins of
// the id handed (+0xC8), the first one met while it is seen within half a second; another Ins's call is ignored whole (the second
// step review's L2: two scenes at once would make the table flap or shut the WorldChrMan caller out); such a call clears the scene
// tag once the kept Ins has not been seen for half a second (the third review's L5: the WorldChrMan caller, which clears it otherwise,
// does not run during the other scene).
bool ApplyWitnessCutsceneClockLocked(bool play,uintptr_t given) noexcept;   // false: a Play_Wait call of an Ins not kept
void ApplyWitnessCutsceneClock(bool play,uintptr_t given) noexcept {
    if(!witness_cutscene_handed.load(std::memory_order_relaxed))return;
    static SRWLOCK lock=SRWLOCK_INIT;static std::atomic<ULONGLONG> last_play{};
    const ULONGLONG now=GetTickCount64();
    if(!play)if(const ULONGLONG seen=last_play.load(std::memory_order_relaxed);seen && now-seen<1000)return;
    AcquireSRWLockExclusive(&lock);const bool kept=ApplyWitnessCutsceneClockLocked(play,given);ReleaseSRWLockExclusive(&lock);
    if(play && kept)last_play.store(now,std::memory_order_relaxed);
}
void FollowWitnessShareVariant(std::string* out=nullptr) noexcept;   // below
// I-0141 tranche 27 (hold-world-freeze.flag, test tooling, a bounded run, both modes): a hold stops the Remo clock alone, so the world's
// own clocks (the effects, the sky's and water's FC_GlobalTime, the gparam blends, the cloth) moved during tranche 26's holds and made
// the storm's and cuts 3 to 5's base shots drift. With the flag, from the frame after a hold starts to its release, the timeline is
// handed dt 0 (the hold's own first frame keeps its dt: Play_Wait, which starts the hold, runs under F0FC90's call of that frame,
// INFERRED from the reading's point that the frame's FD4 tasks run under that call; the second step review's L2). The clocks fed by the
// timeline's dt then hold one state for every shot of a hold, the release frame frozen (INFERRED, the same point; the fourth step
// review's L4); the fixed-step tasks below do not. READ (GhidraMCP, DarkSoulsIII.exe 1.15.2, the pilot's reading of t27-world-clock-reading.md's points): MainLoopTick
// EDAAE0 calls the limiter EED810 at EDABBD, builds the time object {vtable, float dt = [[489DD10]+0x268]} on its stack (EDAC03..EDAC1D)
// and hands it to F0FC90 at EDAC5A (RCX [489F348], RDX the object; F0FC90 jumps to 234CB90 then 234CB00, which copies [RDX+8] to the
// global 47415D8 at 234CB32). 234CB90's other callers (the reading): WmTimerHandler at ED8F84 (a modal loop, the stale +0x268) and the
// ConstDt pump at EDA9BA; the pump calls F0FC90 (I-0112's UNLOCK-SITES), WmTimerHandler's path through F0FC90 INFERRED. INFERRED: that the frame's FD4 tasks run under that one call, and that EED810
// writes +0x268 and +0x26C (its thunk to EED8B0 OPEN; I-0111's runtime captures). Zeroing [RDX+8] at F0FC90's entry reaches the time object's float and its global copy 47415D8 (READ; WmTimerHandler's
// pass only if its path goes through F0FC90, INFERRED); +0x26C (the mean of 16, DtSelector DD4B60's real dt for the cloth's manager, read by MoveMapStep
// 8F2CA0 later in the frame) is zeroed with it, after the limiter wrote it (the limiter rebuilds both from its history each frame).
// The SFX, the sky clock 126EA0 and the gparam blends 0D250F0 take the timeline's dt (READ by the reading); the character mover and the cloth
// step guard dt 0 (READ). Not stopped (INFERRED from I-0112's sites): the tasks with a fixed step (the FFX wind 1/30, YEBIS's 1/60 at
// 16F27E0, so the original's exposure adaptation goes on; the native's adapts on the CPU's time between two records: both modes alike).
// The weather's state machine (EMEVD) was not read. OPEN (the reading): the readers that divide by dt (the Havok world step's seed
// 1/dt, ChildSolver, FollowCam, ValueTracker): no fault in the 36 frozen holds of tranche 27 (four runs), so a bounded run only. A
// witness_hold_world_freeze row at each change (on, off, the frames zeroed).
bool hold_world_freeze{};   // set by 22_setup.inc once F0FC90 is hooked
std::atomic<bool> hold_world_frozen{};
std::atomic<uint64_t> hold_world_frozen_frames{};
std::atomic<ULONGLONG> hold_world_frozen_tick{};
void SetHoldWorldFrozen(bool on,int index) noexcept;   // below
using TimelineRunFn=uint64_t(__fastcall*)(uintptr_t,uintptr_t);
TimelineRunFn timeline_run_original{};
uint64_t __fastcall FreezeTimelineDuringHold(uintptr_t registry,uintptr_t time) noexcept {
    // A hold lasts at most 30 s (the holds' file): a freeze still on after 31 s (its release never seen) is turned off here.
    // seq_cst (the final review's L5); the tick read before the clock (no wrap) and read again before the release (the last step review's
    // L3). Not one atomic step: a hold released and a new one started on a worker between that check and the exchange would lose its
    // freeze (its off row index -1; a Play_Wait running beside F0FC90's entry, against the INFERRED point above).
    if(hold_world_frozen.load()){const ULONGLONG tick=hold_world_frozen_tick.load();
        if(GetTickCount64()-tick>31000 && hold_world_frozen_tick.load()==tick)SetHoldWorldFrozen(false,-1);}
    if(time && hold_world_frozen.load(std::memory_order_relaxed)){
        *reinterpret_cast<volatile float*>(time+8)=0.f;
        uintptr_t clock{};if(Read(image+0x489DD10,&clock,8) && clock)WriteWitnessFloat(clock+0x26C,0.f);
        hold_world_frozen_frames.fetch_add(1,std::memory_order_relaxed);}
    return timeline_run_original(registry,time);
}
void SetHoldWorldFrozen(bool on,int index) noexcept {
    if(!hold_world_freeze)return;
    if(on)hold_world_frozen_tick.store(GetTickCount64());
    if(hold_world_frozen.exchange(on)==on)return;
    char row[160]{};sprintf_s(row,"{\"event\":\"witness_hold_world_freeze\",\"on\":%s,\"index\":%d,\"frames\":%llu}\n",on?"true":"false",index,
        static_cast<unsigned long long>(hold_world_frozen_frames.load(std::memory_order_relaxed)));Log(row);
}
// Step review L1: in a Reference run the variant is read only while a scene's Play_Wait runs; bits left on when the scene ends (a hold
// timed out, the scene closed before the runner's last 0 was read) would light the world and the next scene's first calls without the
// shares. The world's update gate (every frame) puts them back to 0 one second after the last Play_Wait call, with a row. The third
// step review's M1: the reset and the follower's file read and store share reference_share_lock (an SRW lock: its acquire and release
// order the two, per the Win32 documentation; a busy lock: the next frame, or the next Play_Wait call, its count unmoved), the last
// call's tick read again inside it, so a reset never lands over a value a new scene's call has just read. The rows are logged after the
// lock is let go (the fourth review's M1), each with the serial taken under it, which orders them whatever the journal's order (its L1).
// Limits (the fourth review's L6): a scene following another within the second, or a second Ins still calling Play_Wait, keeps the
// bits until the runner writes again; Run-AlphaCast writes 0 at each hold's end and waits for its row, so its holds end at 0.
SRWLOCK reference_share_lock=SRWLOCK_INIT;
std::atomic<ULONGLONG> reference_share_seen{};
uint64_t reference_share_serial{};   // under reference_share_lock
void ReferenceShareVariantIdle() noexcept {
    const ULONGLONG seen0=reference_share_seen.load();
    if(!seen0 || GetTickCount64()-seen0<1000 || !witness_share_variant.load())return;   // the fourth review's L2: no lock while a scene plays
    if(!TryAcquireSRWLockExclusive(&reference_share_lock))return;
    const ULONGLONG seen=reference_share_seen.load();
    const uint32_t was=GetTickCount64()-seen>=1000?witness_share_variant.exchange(0):0u;
    const uint64_t reset_serial=was?++reference_share_serial:0;const ULONGLONG reset_tick=GetTickCount64();   // both under the lock (the last step review's L2)
    ReleaseSRWLockExclusive(&reference_share_lock);
    if(was){char row[192]{};sprintf_s(row,"{\"event\":\"witness_share_variant\",\"reset\":\"no scene\",\"was\":%u,\"serial\":%llu,\"tick\":%llu}\n",was,
        static_cast<unsigned long long>(reset_serial),static_cast<unsigned long long>(reset_tick));Log(row);}   // tick: the final review's L4
}
uintptr_t __fastcall ObserveRemoPlayWait(uintptr_t ins,uintptr_t context) noexcept {
    // I-0141 tranche 27: a Reference run has no BeginFrame of ours; Play_Wait runs once a game frame while the scene plays, a hold
    // included (the hold's own checks run here), which is when its runner writes the variant.
    // Step review L2: two Ins's Play_Wait calls may run at once on two workers; the follower's statics under one lock (a busy lock: the next call).
    if(reference_share_variant){reference_share_seen.store(GetTickCount64());
        std::string rows;if(TryAcquireSRWLockExclusive(&reference_share_lock)){FollowWitnessShareVariant(&rows);ReleaseSRWLockExclusive(&reference_share_lock);}
        if(!rows.empty())Log(rows.c_str());}
    ApplyWitnessCutsceneClock(true,ins);
    return remo_play_wait_original(ins,context);
}
bool ApplyWitnessCutsceneClockLocked(bool play,uintptr_t given) noexcept {
    static uintptr_t latched{};static ULONGLONG latched_tick{};
    if(play){const ULONGLONG now=GetTickCount64();int32_t id{-1};
        if(!Read(given+0xC8,&id,4) || id!=witness_cutscene_id.load(std::memory_order_relaxed)){if(now-latched_tick>=500)cutscene_clock_tag.store(0,std::memory_order_relaxed);return false;}
        if(latched && given!=latched && now-latched_tick<500)return false;latched=given;latched_tick=now;}
    static LONG64 calls{};const LONG64 call=++calls;   // under the lock: one count over both callers' kept calls (the second review's L1)
    // The cuts' table, made once per Impl (offsets[k] the scene frame at cut first + k's start).
    static uintptr_t table_impl{};static int32_t table_first{},table_last{-1};static std::vector<int32_t> starts,ends;static std::vector<double> offsets;static double total{};
    // The holds.
    struct Target {float frame{};float percent{-1.f};bool done{};};
    static std::wstring holds_path;static bool holds_resolved{},holds_read{},holds_resolved_frames{};static Target targets[32]{};static int target_count{};static int hold_ms{};
    static int holding{-1};static ULONGLONG hold_tick{};static float saved_speed{},set_frame{},drift{};static uintptr_t hold_impl{},hold_cut{};
    static int rows{};static int32_t last_index{-2};static int64_t last_shape_key{INT64_MIN};static bool last_valid{};
    static std::wstring shot_path;static bool pending_restore{};static int invalid_calls{};
    const auto finite=[](float v){return std::isfinite(v)?v:-1.0f;};   // step review L4: no nan or inf in a row
    try{
        const RemoClock c=ReadRemoClock(given);
        // The chain's links once, at the first Play_Wait call (the run of 02:52 named no link): the Ins handed and the one the
        // Man chain gives should be the same object.
        static bool chain_row{};if(play && !chain_row){chain_row=true;const RemoChain r=ReadRemoChain();uintptr_t w{};Read(given+0xD0,&w,8);
            char row[520]{};sprintf_s(row,"{\"event\":\"witness_cutscene_chain\",\"call\":%lld,\"thread\":%lu,\"imp\":\"%llx\",\"man\":\"%llx\",\"flags\":%u,\"count\":%d,\"p1\":\"%llx\",\"p2\":\"%llx\",\"chain_ins\":\"%llx\",\"ins\":\"%llx\",\"id\":%d,\"w\":\"%llx\",\"impl\":\"%llx\",\"cuts\":\"%llx\",\"cut\":%d,\"first\":%d,\"last\":%d,\"valid\":%s}\n",
                static_cast<long long>(call),GetCurrentThreadId(),static_cast<unsigned long long>(r.imp),static_cast<unsigned long long>(r.man),unsigned(r.flags),r.count,
                static_cast<unsigned long long>(r.p1),static_cast<unsigned long long>(r.p2),static_cast<unsigned long long>(r.ins),static_cast<unsigned long long>(given),c.id,
                static_cast<unsigned long long>(w),static_cast<unsigned long long>(c.impl),static_cast<unsigned long long>(c.cuts),c.index,c.first,c.last,c.valid?"true":"false");Log(row);}
        if(play && c.valid && c.impl!=table_impl){
            std::vector<int32_t> s,e;std::vector<double> o;double sum{};bool ok=true;
            for(int32_t k=c.first;k<=c.last && ok;++k){int32_t a{},b{};ok=ReadRemoCutRange(c.cuts,k,a,b);if(ok){s.push_back(a);e.push_back(b);o.push_back(sum);sum+=double(b)+1.0-double(a);}}
            if(ok){table_impl=c.impl;table_first=c.first;table_last=c.last;starts=std::move(s);ends=std::move(e);offsets=std::move(o);total=sum;
                std::string cells;for(size_t k=0;k<starts.size() && k<64;++k){char cell[32]{};sprintf_s(cell,"%s[%d,%d]",k?",":"",starts[k],ends[k]);cells+=cell;}
                char head[220]{};sprintf_s(head,"{\"event\":\"witness_cutscene_cuts\",\"call\":%lld,\"id\":%d,\"first\":%d,\"last\":%d,\"total\":%.1f,\"truncated\":%s,\"cuts\":[",
                    static_cast<long long>(call),c.id,c.first,c.last,total,starts.size()>64?"true":"false");
                Log((std::string(head)+cells+"]}\n").c_str());holds_resolved_frames=false;}}
        bool table=play && c.valid && c.impl==table_impl && c.index>=table_first && c.index<=table_last;   // step review M1: the scene's clock only where it plays
        if(table){const size_t k=size_t(c.index-table_first);table=double(c.frame)>=double(starts[k])-2.0 && double(c.frame)<=double(ends[k])+2.0;}
        // Step review M2: a speed a release could not restore is written as soon as the chain reaches its Impl again.
        if(pending_restore && hold_impl && c.impl==hold_impl && WriteWitnessFloat(hold_impl+0x36C,saved_speed)){pending_restore=false;
            char row[128]{};sprintf_s(row,"{\"event\":\"witness_cutscene_hold\",\"phase\":\"restored\",\"call\":%lld}\n",static_cast<long long>(call));Log(row);}
        double scene{-1.0};if(table){const size_t k=size_t(c.index-table_first);scene=offsets[k]+double(c.frame)-double(starts[k]);}
        {uint64_t tag{};if(table){const float f=float(scene);uint32_t bits{};std::memcpy(&bits,&f,4);tag=(1ull<<63)|(uint64_t(uint32_t(c.index)&0xFFFF)<<32)|bits;}
            cutscene_clock_tag.store(tag,std::memory_order_relaxed);}
        // The clock's row.
        int32_t shape{-1};float luminance{};const bool globals=Read(image+0x4961F70+0xD8,&shape,4) && Read(image+0x4961F70+0xDC,&luminance,4);
        const int64_t shape_key=globals?int64_t(shape)*2+(luminance>0.f?1:0):INT64_MIN+1;
        const bool change=c.index!=last_index || shape_key!=last_shape_key || table!=last_valid;
        if(rows<4000 && (change || (table && call%30==0))){++rows;last_index=c.index;last_shape_key=shape_key;last_valid=table;
            char row[360]{};sprintf_s(row,"{\"event\":\"witness_cutscene_clock\",\"call\":%lld,\"src\":\"%s\",\"thread\":%lu,\"tick\":%llu,\"playing\":%s,\"valid\":%s,\"id\":%d,\"cut\":%d,\"frame\":%.3f,\"scene\":%.3f,\"speed\":%.4g,\"shape\":%d,\"luminance\":%.4g,\"holding\":%d}\n",
                static_cast<long long>(call),play?"play":"schedule",GetCurrentThreadId(),static_cast<unsigned long long>(GetTickCount64()),c.playing?"true":"false",table?"true":"false",c.id,c.index,table?c.frame:-1.0f,scene,finite(c.speed),shape,
                std::isfinite(luminance)?luminance:0.f,holding);Log(row);}
        // The holds' file.
        if(!holds_resolved){holds_resolved=true;holds_path=ModuleSibling(L"witness-cutscene-holds.txt");shot_path=ModuleSibling(L"witness-cutscene-shot.txt");}
        if(!holds_read && !holds_path.empty() && call%15==0 && GetFileAttributesW(holds_path.c_str())!=INVALID_FILE_ATTRIBUTES){
            FILE* file{};if(!_wfopen_s(&file,holds_path.c_str(),L"rb") && file){holds_read=true;char text[1024]{};const size_t n=fread(text,1,sizeof(text)-1,file);fclose(file);text[n]=0;
                const bool removed=DeleteFileW(holds_path.c_str())!=FALSE;char* context{};char* token=strtok_s(text," \t\r\n",&context);
                // Step review L1: decimal digits and one point only (no hex, sign or exponent), a digit after p, ms whole; a file that fills the buffer is refused.
                const auto plain=[](const char* s){bool digit{},point{};for(;*s;++s){if(*s>='0' && *s<='9')digit=true;else if(*s=='.' && !point)point=true;else return false;}return digit;};
                char* ms_end{};const long ms=token && plain(token)?strtol(token,&ms_end,10):0;bool parsed=n<sizeof(text)-1 && ms_end && !*ms_end && ms>=500 && ms<=30000;
                while(parsed && (token=strtok_s(nullptr," \t\r\n",&context))!=nullptr){if(target_count>=32){parsed=false;break;}Target t;char* end{};
                    if(token[0]=='p'){if(!plain(token+1)){parsed=false;break;}t.percent=strtof(token+1,&end);if(!end || *end || !(t.percent>=0.f && t.percent<=100.f)){parsed=false;break;}}
                    else{if(!plain(token)){parsed=false;break;}t.frame=strtof(token,&end);if(!end || *end || !(t.frame>=0.f && t.frame<1.0e6f)){parsed=false;break;}}
                    targets[target_count++]=t;}
                if(!parsed)target_count=0;hold_ms=int(ms);
                char row[160]{};sprintf_s(row,"{\"event\":\"witness_cutscene_holds\",\"parsed\":%s,\"removed\":%s,\"ms\":%d,\"targets\":%d}\n",parsed?"true":"false",removed?"true":"false",hold_ms,target_count);Log(row);}}
        if(!target_count)return true;
        if(table && !holds_resolved_frames){holds_resolved_frames=true;   // percentages of this scene's length, once its table is known
            for(int i=0;i<target_count;++i)if(targets[i].percent>=0.f)targets[i].frame=float(total*double(targets[i].percent)/100.0);}
        const auto hold_row=[&](const char* phase,int i,const char* why,float frame){char row[360]{};
            sprintf_s(row,"{\"event\":\"witness_cutscene_hold\",\"phase\":\"%s\",\"index\":%d,\"id\":%d,\"target\":%.3f,\"percent\":%.4g,\"why\":\"%s\",\"call\":%lld,\"tick\":%llu,\"cut\":%d,\"frame\":%.3f,\"scene\":%.3f,\"drift\":%.4g}\n",
                phase,i,c.id,targets[i].frame,targets[i].percent,why,static_cast<long long>(call),static_cast<unsigned long long>(GetTickCount64()),c.index,finite(frame),scene,finite(drift));Log(row);};
        if(holding>=0){
            float now{};const bool same=c.valid && c.impl==hold_impl && c.cut==hold_cut && Read(hold_cut+0x68,&now,4);
            if(same)drift=std::max(drift,std::isfinite(now)?std::fabs(now-set_frame):1.0e9f);
            // The shot file holds the index of the hold its shot was taken in (Run-AlphaCast writes it): a late shot's file found during
            // the next hold is removed with a witness_cutscene_shot_stale row and releases nothing (the run of 04:05: a shot that took
            // 11 s released the next hold at once). A file not opened or still empty (the runner writing it) is left for the next call
            // (the third review's L3).
            bool shot{};
            if(!shot_path.empty() && GetFileAttributesW(shot_path.c_str())!=INVALID_FILE_ATTRIBUTES){
                int index{-1};bool got{};FILE* file{};if(!_wfopen_s(&file,shot_path.c_str(),L"rb") && file){char text[16]{};const size_t n=fread(text,1,sizeof(text)-1,file);fclose(file);text[n]=0;got=n>0;
                    char* end{};const long v=n && text[0]>='0' && text[0]<='9'?strtol(text,&end,10):-1;if(end && !*end && v>=0 && v<32)index=int(v);}
                if(got && DeleteFileW(shot_path.c_str())!=FALSE){if(index==holding)shot=true;
                    else{char row[160]{};sprintf_s(row,"{\"event\":\"witness_cutscene_shot_stale\",\"holding\":%d,\"file\":%d,\"call\":%lld}\n",holding,index,static_cast<long long>(call));Log(row);}}}
            const bool timeout=GetTickCount64()-hold_tick>=ULONGLONG(hold_ms);
            if(!same || shot || timeout){
                // Step review M2: restored whenever the chain still reaches the held Impl, whatever the rest of the clock reads; else later.
                bool restored{};if(c.impl==hold_impl)restored=WriteWitnessFloat(hold_impl+0x36C,saved_speed);pending_restore=!restored;
                const char* why=!same?(restored?"scene changed":"scene changed, speed not written"):shot?(restored?"shot":"shot, speed not written"):(restored?"timeout":"timeout, speed not written");
                hold_row("release",holding,why,same?now:-1.f);SetHoldWorldFrozen(false,holding);targets[holding].done=true;holding=-1;}
            return true;}
        // Step review L2: targets still waiting when the scene has been gone two seconds get their missed row (the runner stops on them).
        if(!table && holds_resolved_frames){if(++invalid_calls==120)for(int i=0;i<target_count;++i)if(!targets[i].done){targets[i].done=true;hold_row("missed",i,"scene ended",-1.f);}}
        else invalid_calls=0;
        if(!table || !holds_resolved_frames || pending_restore)return true;
        {uint32_t flags{};uintptr_t w{};if(!Read(given+0xCC,&flags,4) || (flags&3) || !Read(given+0xD0,&w,8) || !w)return true;}   // step review L3: CD5D80 skips its Play step (bit 1) or runs 14235D810's branch first (bit 0)
        for(int i=0;i<target_count;++i){if(targets[i].done || scene<double(targets[i].frame))continue;
            const size_t k=size_t(c.index-table_first);const double local=double(targets[i].frame)-offsets[k]+double(starts[k]);
            if(scene-double(targets[i].frame)>2.0 || local<double(starts[k])){targets[i].done=true;hold_row("missed",i,local<double(starts[k])?"in an earlier cut":"passed",c.frame);continue;}
            saved_speed=c.speed;set_frame=float(local);drift=0.f;
            const bool ok=WriteWitnessFloat(c.impl+0x36C,0.f) && WriteWitnessFloat(c.cut+0x68,set_frame) && WriteWitnessFloat(c.cut+0x6C,set_frame);
            if(!ok){WriteWitnessFloat(c.impl+0x36C,saved_speed);targets[i].done=true;hold_row("missed",i,"not written",c.frame);continue;}
            holding=i;hold_tick=GetTickCount64();hold_impl=c.impl;hold_cut=c.cut;hold_row("hold",i,"",set_frame);SetHoldWorldFrozen(true,i);break;}
    }catch(...){}
    return true;
}
// I-0141 tranche 26 (RESULT « Suggested next step » 3; test tooling, a bounded run only): witness-variant.txt next to this module holds
// the share variant's bits (01_state.inc's witness_share_variant: 1 the cutscene sets' lights 1 to 3, 2 the glare family, 4, 8 and 16
// the traced rays' sky misses, cache ambient and hits' local lights (R7a's diagnostic bits, ORed directly by the GI's rays, the specular
// rays on the frames NRD denoises (RR off) and the cache's update; bit 16 nothing at the PT level; the probes, the water and the
// specular on the frames NRD does not denoise keep their own misses' sky
// and their hits' local lights, and read cache cells built under the bits once the cache has turned over: the final review's H1, the
// step reviews' M1 and L1); tranche 27's 32: natively the main view's traced diffuse GI (EncodeRtGi's compose skipped), in a
// Reference run the instance SH of the pass kinds 0, 3 and 4 (04c); 64: natively the main view's SpecularAcc (the forward rows keep
// their probes' SH and cubes), in a Reference run the cube scales of every draw the hook reaches and the region passes; tranche 28's
// 128 and 256, native only: the sets' lights 1 to 3 and light 0 out of the traced hits' light (ORed as kRtReflectHitNoFill and
// kRtReflectHitNoSun with the bits 4 to 16, so the same passes); 512, Reference only: the instance SH census (01_state.inc's kShare*).
// A bit outside the mode's set refuses the whole file (one parsed:false row, the previous variant kept); 0 all on), read at the frame's start every 15th call and removed once read. A file last written before the
// module's first call is a killed run's leftover: never read, its removal retried at each check (the final review's L6, the step
// reviews' L2), so the start's flags are latched from a file a runner wrote during a hold. So a runner switches the native's shares off and on during one cutscene hold and shoots each at the same held moment (the
// Remo clock held, D-0299; tranche 25's single runs a variant could not separate the shares from the runs' own spread). The GI's bits
// are ORed on the start's diagnostic flags, never cleared from them. One witness_share_variant row a file read (its bits, the scene
// clock's cut and frame while a scene plays), the runner waiting for it before it lets the picture settle.
// out: given (the Reference caller, under reference_share_lock), the rows are appended there for the caller to log once the lock is
// let go; null (BeginFrame), logged here.
void FollowWitnessShareVariant(std::string* out) noexcept {
    static std::wstring path;static bool resolved{},latched{};static LONG64 calls{};static uint32_t start_diagnostic{};static int rows{};static ULONGLONG started{},consumed{};
    const auto emit=[out](const char* row){if(out)*out+=row;else Log(row);};
    if(!resolved){resolved=true;if(WitnessToolsAllowed())path=ModuleSibling(L"witness-variant.txt");
        FILETIME now{};GetSystemTimeAsFileTime(&now);started=(ULONGLONG(now.dwHighDateTime)<<32)|now.dwLowDateTime;}
    if(path.empty() || ++calls%15 || rows>=2048)return;   // the final review's L7: no bits applied that no row reports
    WIN32_FILE_ATTRIBUTE_DATA data{};if(!GetFileAttributesExW(path.c_str(),GetFileExInfoStandard,&data))return;   // absent, or not reachable now: the next check
    const ULONGLONG written=(ULONGLONG(data.ftLastWriteTime.dwHighDateTime)<<32)|data.ftLastWriteTime.dwLowDateTime;
    if(consumed && written==consumed)return;   // the fourth review's L3, both modes: a file read whose removal failed is not read again
    if(written<started){   // a leftover (the final review's L6): never read
        const bool removed=DeleteFileW(path.c_str())!=FALSE;   // a failed removal (a scanner holding it) retried at the next check
        if(rows<2048){++rows;emit(removed?"{\"event\":\"witness_share_variant\",\"leftover\":true,\"removed\":true}\n":"{\"event\":\"witness_share_variant\",\"leftover\":true,\"removed\":false}\n");}
        return;}
    FILE* file{};if(_wfopen_s(&file,path.c_str(),L"rb") || !file)return;
    char text[16]{};const size_t n=fread(text,1,sizeof(text)-1,file);fclose(file);text[n]=0;
    if(!n)return;   // the runner still writing it: the next check
    // The step review's L1: a file that fills the buffer, or anything but a line end after the number, is refused whole.
    const bool removed=DeleteFileW(path.c_str())!=FALSE;
    consumed=removed?0:written;   // the fifth review's L1: kept only while the read file stays (its time from before the read: INFERRED unchanged by it)
    char* end{};const unsigned long v=text[0]>='0' && text[0]<='9'?strtoul(text,&end,10):~0ul;
    const bool line_end=end && [&]{for(const char* c=end;c<text+n;++c)if(*c!='\r' && *c!='\n')return false;return true;}();   // to n: an embedded NUL refused (the final review's L10)
    // I-0141 tranche 27: a Reference run takes bits 1, 2, 32 and 64 only (the original has no rays), and tranche 28's census bit 512;
    // its rows carry the hooks' counts by path. A native run takes bits 1 to 256 (512 refused: the census is the original's).
    const bool parsed=n<sizeof(text)-1 && line_end && (reference_share_variant?(v&~static_cast<unsigned long>(kShareNoFill|kShareNoGlare|kShareNoGi|kShareNoSpecular|kShareCensus))==0:v<=kShareAll);
    if(parsed){const uint32_t bits=uint32_t(v);
        if(reference_share_variant && (bits&kShareCensus))reference_census_epoch.fetch_add(1);   // tranche 28: a new census (04c), before the bits reach the hook
        witness_share_variant.store(bits);   // under reference_share_lock in a Reference run, as the idle reset (the fourth review's L5)
        if(!reference_share_variant){
            if(!latched){latched=true;start_diagnostic=renderer.rt_gi_diagnostic.load(std::memory_order_relaxed);}   // the start's flags, set long before a hold
            renderer.rt_gi_diagnostic.store(start_diagnostic|((bits&kShareNoSky)?kRtReflectGiNoSky:0u)|((bits&kShareNoAmbient)?kRtReflectGiNoAmbient:0u)|
                ((bits&kShareNoLocal)?kRtReflectGiNoLocal:0u)|((bits&kShareHitNoFill)?kRtReflectHitNoFill:0u)|((bits&kShareHitNoSun)?kRtReflectHitNoSun:0u),std::memory_order_relaxed);
            renderer.share_variant_off.store(((bits&kShareNoGi)?kShareRendererNoGi:0u)|((bits&kShareNoSpecular)?kShareRendererNoSpecular:0u),std::memory_order_relaxed);}}
    if(rows<2048){++rows;std::string counts;
        if(reference_share_variant){counts=",\"mode\":\"reference\",\"paths\":[";
            for(size_t i=0;i<reference_share_counts.size();++i)counts+=(i?",":"")+std::to_string(reference_share_counts[i].load(std::memory_order_relaxed));
            counts+="]";
            int32_t shape{-1};Read(image+0x4961F70+0xD8,&shape,4);counts+=",\"shape\":"+std::to_string(shape);
            if(parsed)counts+=",\"serial\":"+std::to_string(++reference_share_serial);}   // shape: the step review's M1 (a shape 14, a custom glare, is out of bit 2's reach); serial: under reference_share_lock (the fourth review's L1)
        char row[200]{};sprintf_s(row,"{\"event\":\"witness_share_variant\",\"parsed\":%s,\"removed\":%s,\"bits\":%ld,\"tick\":%llu",parsed?"true":"false",
            removed?"true":"false",parsed?long(v):-1L,static_cast<unsigned long long>(GetTickCount64()));
        emit((row+counts+CutsceneClockCell()+"}\n").c_str());}
}
// Test tools (I-0089, Thomas 2026-10-22; the test launcher only, never the play launcher): with test-tools.flag next to this
// module, F4 (Alt not held, game window in front) toggles invincibility (the player's HP refilled on the game thread each
// schedule call, one beep high on / low off, the window title says [TEST]), and the boss-defeated event flags are cleared
// through the game's own setter 1404C5E20 so the zone's boss is back once the zone reloads: test-revive.txt next to the module
// holds "all" or flag ids (only the 26 registered boss flags are accepted, the registry of common.emevd event 970); they are
// applied once the world has run for 180 frames, and F5 applies them again (no id: all). Without the flag file the module does
// nothing here but one cached test of the file's presence.
constexpr int32_t kTestBossFlags[]={13000800,13000830,13000890,13010800,13100800,13200800,13200850,13300800,13300850,13410830,13410860,
    13500800,13700800,13700850,13800800,13800830,13900800,14000800,14000830,14100800,14500800,14500860,15000800,15100800,15100850,15110800};
// The title and the beep run on a thread of their own (review: the window belongs to the game's main thread, and a send to it from
// the game thread could stall the frame): bit 16 = invincible text, bits 0-15 = the tone in Hz (0 = no beep).
DWORD WINAPI TestToolsAnnounceThread(LPVOID argument) noexcept {
    const auto bits=static_cast<uint32_t>(reinterpret_cast<uintptr_t>(argument));
    const HWND window=GetForegroundWindow();DWORD owner{};GetWindowThreadProcessId(window,&owner);
    if(window && owner==GetCurrentProcessId()){DWORD_PTR result{};
        if(!(bits&0x20000))SendMessageTimeoutW(window,WM_SETTEXT,0,reinterpret_cast<LPARAM>((bits&0x10000)?L"DARK SOULS III  [TEST : invincible]":L"DARK SOULS III  [TEST]"),SMTO_ABORTIFHUNG,500,&result);}
    if(bits&0xFFFF)Beep(bits&0xFFFF,140);
    return 0;
}
void TestToolsAnnounce(bool invincible,bool beep,bool title=true,uint32_t tone=0) noexcept {
    const uint32_t bits=(invincible?0x10000u:0u)|(title?0u:0x20000u)|(beep?(tone?tone:(invincible?1000u:500u)):0u);
    if(const HANDLE thread=CreateThread(nullptr,0,&TestToolsAnnounceThread,reinterpret_cast<LPVOID>(static_cast<uintptr_t>(bits)),0,nullptr))CloseHandle(thread);
}
bool TestToolsKeyEdge(int key,bool& was_down) noexcept {
    DWORD owner{};GetWindowThreadProcessId(GetForegroundWindow(),&owner);
    const bool down=owner==GetCurrentProcessId() && (GetAsyncKeyState(key)&0x8000) && !(GetAsyncKeyState(VK_MENU)&0x8000);
    const bool edge=down && !was_down;was_down=down;return edge;
}
// The player's character data module: [[[manager+0x80]+0x1F90]+0x18], HP at +0xD8 and its maximum at +0xDC (INFERRED from the
// community layout, checked on every use: a value out of range skips that step silently (a load or a ring change can show one
// for a frame), and it refuses the switch-on, in the journal and by the low beep, when it is out of range at the key press).
void ApplyTestInvincible(uintptr_t manager,bool toggled,bool& on) noexcept {
    if(toggled){on=!on;TestToolsAnnounce(on,true);}
    if(!on)return;
    uintptr_t player{},modules{},data{};int32_t hp{},hp_max{};
    const bool found=Read(manager+0x80,&player,8) && player && Read(player+0x1F90,&modules,8) && modules && Read(modules+0x18,&data,8) && data &&
        Read(data+0xD8,&hp,4) && Read(data+0xDC,&hp_max,4);
    const bool plausible=found && hp_max>=20 && hp_max<=20000 && hp>=0 && hp<=hp_max;
    if(toggled){char row[200]{};sprintf_s(row,"{\"event\":\"test_invincible\",\"on\":true,\"found\":%s,\"plausible\":%s,\"hp\":%d,\"hp_max\":%d}\n",
        found?"true":"false",plausible?"true":"false",hp,hp_max);Log(row);}
    if(!plausible){if(toggled){on=false;TestToolsAnnounce(false,true);}return;}
    if(hp>0 && hp<hp_max){const bool written=WriteWitnessWord(data+0xD8,hp_max);static LONG64 refills{};const auto n=InterlockedIncrement64(&refills);
        if(n<=3 || n%120==0){char row[160]{};sprintf_s(row,"{\"event\":\"test_invincible_refill\",\"count\":%lld,\"hp_before\":%d,\"hp_max\":%d,\"written\":%s}\n",
            n,hp,hp_max,written?"true":"false");Log(row);}}
}
void ApplyTestRevive(uintptr_t manager,LONG64 call,bool all_again) noexcept {
    static std::vector<int32_t> selected;static bool loaded{},applied{};static LONG64 first_world_call{};
    if(!loaded){loaded=true;const auto path=ModuleSibling(L"test-revive.txt");FILE* file{};
        if(!path.empty() && !_wfopen_s(&file,path.c_str(),L"rb") && file){char word[32]{};
            while(fscanf_s(file,"%31s",word,static_cast<unsigned>(sizeof word))==1){
                if(!_stricmp(word,"all")){selected.assign(std::begin(kTestBossFlags),std::end(kTestBossFlags));break;}
                const int32_t id=atoi(word);if(std::find(std::begin(kTestBossFlags),std::end(kTestBossFlags),id)!=std::end(kTestBossFlags))selected.push_back(id);}
            fclose(file);}
        char row[160]{};sprintf_s(row,"{\"event\":\"test_revive_selection\",\"count\":%d}\n",static_cast<int>(selected.size()));Log(row);}
    // A key press waits (pending) until the flag manager is ready instead of being lost during a load.
    static bool pending{};if(all_again)pending=true;
    uintptr_t player{},flags{};
    if(!Read(manager+0x80,&player,8) || !player)return;
    if(!first_world_call)first_world_call=call;
    if(!pending && (applied || selected.empty() || call-first_world_call<180))return;
    uint8_t ready{};if(!Read(image+0x4752F68,&flags,8) || !flags || !Read(flags+0x228,&ready,1) || !ready)return;
    if(pending && selected.empty())selected.assign(std::begin(kTestBossFlags),std::end(kTestBossFlags));
    for(const auto id:selected)reinterpret_cast<void(__fastcall*)(uintptr_t,uint64_t,char,char)>(image+0x4C5E20)(flags,static_cast<uint64_t>(id),0,0);
    char row[160]{};sprintf_s(row,"{\"event\":\"test_revive\",\"cleared\":%d,\"again\":%s}\n",static_cast<int>(selected.size()),pending?"true":"false");Log(row);
    applied=true;pending=false;TestToolsAnnounce(false,true,false,750);
}
// I-0113 tranche 10d (a boss's death fade, gap 19): F6 kills the character of the largest HP maximum (up to 20,000) within 25 m of the player
// (its character data module's HP +0xD8 written to 0, as ApplyTestDeath does for the player; the game then plays its death).
// The characters: the manager's chr sets (+0x1C8 count, +0x1D0 stride 0x18), each a count +0 and entries [+8] of 0x38 bytes
// whose first word is the character (14089FF60, read in 8DD2E0); the positions the physics module's +0x80 ([+0x1F90]+0x68).
// I-0140 tranche 19 (its step review's L3): the walk F6 and F1's n and p share. Each other character of the sets within 25 m of
// the player whose data module holds a living, bounded HP (i113-t10d-death-native: an unseen character of 60,939 HP near Iudex
// Gundyr's bonfire was taken first by F6: a maximum is kept from 20 to 20,000, as the player's in ApplyTestInvincible) is passed
// to visit with its data module, HP, maximum, position and squared distance; returns the characters looked at (F6's row).
struct TestNearCharacter {uintptr_t chr{},data{};int32_t hp{},hp_max{};float at[3]{},d2{};};
template<class Visit> int32_t VisitTestCharacters(uintptr_t manager,uintptr_t player,Visit&& visit) noexcept {
    const auto position=[](uintptr_t chr,float (&at)[3]){uintptr_t modules{},physics{};
        return Read(chr+0x1F90,&modules,8) && modules && Read(modules+0x68,&physics,8) && physics && Read(physics+0x80,at,12);};
    float origin[3]{};if(!player || !position(player,origin))return 0;
    int32_t sets{};Read(manager+0x1C8,&sets,4);int32_t looked{};
    for(int32_t k=0;k<sets && k<16;++k){uintptr_t set{},entries{};int32_t count{};
        if(!Read(manager+0x1D0+0x18ull*k,&set,8) || !set || !Read(set,&count,4) || !Read(set+8,&entries,8) || !entries)continue;
        for(int32_t i=0;i<count && i<4096;++i){uintptr_t chr{};if(!Read(entries+0x38ull*i,&chr,8) || !chr || chr==player)continue;++looked;
            TestNearCharacter c{};c.chr=chr;if(!position(chr,c.at))continue;const float dx=c.at[0]-origin[0],dy=c.at[1]-origin[1],dz=c.at[2]-origin[2];
            c.d2=dx*dx+dy*dy+dz*dz;if(!(c.d2<=25.f*25.f))continue;uintptr_t modules{};
            if(!Read(chr+0x1F90,&modules,8) || !modules || !Read(modules+0x18,&c.data,8) || !c.data)continue;
            if(!Read(c.data+0xD8,&c.hp,4) || !Read(c.data+0xDC,&c.hp_max,4) || c.hp<=0 || c.hp_max<20 || c.hp_max>20000 || c.hp>c.hp_max)continue;
            visit(c);}}
    return looked;
}
void ApplyTestKill(uintptr_t manager,bool pressed) noexcept {
    if(!pressed)return;
    uintptr_t player{};if(!Read(manager+0x80,&player,8) || !player)return;
    uintptr_t best{};int32_t best_max{},best_hp{};
    const int32_t looked=VisitTestCharacters(manager,player,[&](const TestNearCharacter& c){if(c.hp_max>best_max){best=c.data;best_max=c.hp_max;best_hp=c.hp;}});
    const bool written=best && WriteWitnessWord(best+0xD8,0);
    char row[200]{};sprintf_s(row,"{\"event\":\"test_kill\",\"looked\":%d,\"found\":%s,\"hp\":%d,\"hp_max\":%d,\"written\":%s}\n",looked,best?"true":"false",best_hp,best_max,written?"true":"false");Log(row);
    TestToolsAnnounce(false,true,false,written?1500:300);
}
// I-0113 tranche 11 (Thomas, 2026-11-01: a test save holding every spell, made from a copy): F7 gives the player the items listed
// in test-give.txt next to the module, through the game's own award routine 1407C4080 (the map-item manager [144769240], called
// by the item-lot award 1407C10F0 of the EMEVD handlers 140480820/140480860): a buffer {count 1..6, then {id with its category in
// the top nibble, quantity, durability -1}} and a source {2, 0, 0} as the award path passes (VERIFIED in the decompile; the
// inventory add 1407C2CC0, the popup 140C8CCD0, an excess to the storage box or the ground). Words: g<id> goods (a spell's goods id
// is its Magic id), w<id> weapon (arrows and bolts too: they are weapons), p<id> protector, a<id> accessory (not an arrow: F1's A<id>
// is), one award each; six items a frame, with a player, the game data [1447572B8]
// and the popup sink [14478ECC0] present. I-0140 tranche 17 (the arrows and bolts, which a bow spends): a word may end with
// x<n>, the quantity (1 to 99; 1 without it), the award buffer's own quantity field.
void ApplyTestGive(uintptr_t manager,bool pressed) noexcept {
    static std::vector<std::pair<uint32_t,int32_t>> queue;static size_t next{};
    if(pressed){queue.clear();next=0;const auto path=ModuleSibling(L"test-give.txt");FILE* file{};
        if(!path.empty() && !_wfopen_s(&file,path.c_str(),L"rb") && file){char word[32]{};
            while(queue.size()<4096 && fscanf_s(file,"%31s",word,static_cast<unsigned>(sizeof word))==1){
                const char kind=static_cast<char>(word[0]|0x20);const uint32_t category=kind=='g'?4u:kind=='w'?0u:kind=='p'?1u:kind=='a'?2u:16u;
                char* end{};const unsigned long id=word[1]>='0' && word[1]<='9'?std::strtoul(word+1,&end,10):0;unsigned long quantity=1;   // its third step review's L6: no sign
                if(end && end!=word+1 && (*end|0x20)=='x'){char* digits=end+1;   // tranche 17's step review L6: digits only, no sign
                    quantity=*digits>='0' && *digits<='9'?std::strtoul(digits,&end,10):0;}
                if(category<16 && end && !*end && id && id<0x0FFFFFFF && quantity>=1 && quantity<=99)
                    queue.emplace_back((category<<28)|static_cast<uint32_t>(id),static_cast<int32_t>(quantity));}
            fclose(file);}
        char row[128]{};sprintf_s(row,"{\"event\":\"test_give\",\"listed\":%zu}\n",queue.size());Log(row);
        TestToolsAnnounce(false,true,false,queue.empty()?300:1200);}
    if(next>=queue.size())return;
    uintptr_t player{},items{},game_data{},popups{};
    if(!Read(manager+0x80,&player,8) || !player || !Read(image+0x4769240,&items,8) || !items || !Read(image+0x47572B8,&game_data,8) || !game_data ||
       !Read(image+0x478ECC0,&popups,8) || !popups)return;
    struct Buffer {uint32_t count;struct {uint32_t id;int32_t quantity,durability;} item[6];} buffer{};
    for(;buffer.count<6 && next<queue.size();++next)buffer.item[buffer.count++]={queue[next].first,queue[next].second,-1};
    const int32_t source[3]{2,0,0};
    reinterpret_cast<void(__fastcall*)(uintptr_t,Buffer*,const int32_t*)>(image+0x7C4080)(items,&buffer,source);
    if(next>=queue.size()){char row[128]{};sprintf_s(row,"{\"event\":\"test_give_done\",\"given\":%zu}\n",queue.size());Log(row);TestToolsAnnounce(false,true,false,1500);}
}
// I-0140 (ROADMAP workstream 5 step 5, the spells' audit; STUDY-ATTUNE): F11 attunes the next spell of test-attune.txt (Magic
// ids, a spell's goods id, with or without a leading g) into slot 0 through the game's own setter 140590490(eq, slot, id) (it
// compacts the slots and re-resolves the selection, 140591CD0), then selects it with 140590FC0(eq, id); eq = the game data
// [1447572B8] +0x10 +0x470 (READ, STUDY-ATTUNE). No cost, Attunement or stat check, as the setter has none. It also fills the
// player's FP to its maximum (+0xE4 from +0xE8 of the character data module, INFERRED from the community layout as HP's +0xD8,
// checked: 0 <= FP <= max, max in 10..2000, else left). One test_attune row a press: the id, the slot read back, the selection.
void ApplyTestAttune(uintptr_t manager,bool pressed) noexcept {
    static std::vector<int32_t> spells;static size_t next{};static bool read{},list_opened{};
    if(!pressed)return;
    if(!read){read=true;const auto path=ModuleSibling(L"test-attune.txt");FILE* file{};
        if(!path.empty() && !_wfopen_s(&file,path.c_str(),L"rb") && file){list_opened=true;char word[32]{};
            while(spells.size()<4096 && fscanf_s(file,"%31s",word,static_cast<unsigned>(sizeof word))==1){
                const char* digits=(word[0]|0x20)=='g'?word+1:word;char* end{};const unsigned long id=std::strtoul(digits,&end,10);
                if(end && end!=digits && !*end && id && id<0x0FFFFFFF)spells.push_back(static_cast<int32_t>(id));}
            fclose(file);}
        char row[112]{};sprintf_s(row,"{\"event\":\"test_attune_list\",\"opened\":%s,\"listed\":%zu}\n",list_opened?"true":"false",spells.size());Log(row);}
    uintptr_t game_data{},player_data{},equip{};
    const bool found=Read(image+0x47572B8,&game_data,8) && game_data && Read(game_data+0x10,&player_data,8) && player_data &&
        Read(player_data+0x470,&equip,8) && equip;
    if(next>=spells.size() || !found){char row[160]{};sprintf_s(row,"{\"event\":\"test_attune\",\"listed\":%zu,\"done\":%s,\"found\":%s}\n",
        spells.size(),!spells.empty() && next>=spells.size()?"true":"false",found?"true":"false");Log(row);   // done: a list read and finished
        TestToolsAnnounce(false,true,false,300);return;}
    const int32_t id=spells[next++];
    // Its step review's M1: the setter writes any id (a missing row only makes the slot's second word -1), so the id is first
    // looked up with the setter's own lookup 140E32960 (it writes the id at +0 and the row at +8, and the param file's u16 +8 at +4
    // only when table 0xE exists; the row 0 for a negative id, a missing table, an id not found or a negative row index) and
    // refused without a row. The start values are the setter's own.
    // Its final review's L7: 140E32960 asserts on a null param repository [144798108] before it reads table 0xE: read it first.
    uintptr_t repository{};if(!Read(image+0x4798108,&repository,8) || !repository){char row[128]{};sprintf_s(row,"{\"event\":\"test_attune\",\"k\":%zu,\"id\":%d,\"repository\":false}\n",next,id);
        Log(row);TestToolsAnnounce(false,true,false,300);return;}
    struct {int32_t id{-1},word{};uintptr_t row{};} lookup;
    reinterpret_cast<void(__fastcall*)(void*,int32_t)>(image+0xE32960)(&lookup,id);
    if(!lookup.row){char row[160]{};sprintf_s(row,"{\"event\":\"test_attune\",\"k\":%zu,\"listed\":%zu,\"id\":%d,\"row\":false,\"set\":false}\n",
        next,spells.size(),id);Log(row);TestToolsAnnounce(false,true,false,300);return;}
    const bool set=reinterpret_cast<uint8_t(__fastcall*)(uintptr_t,uint32_t,int32_t)>(image+0x590490)(equip,0,id)!=0;
    reinterpret_cast<void(__fastcall*)(uintptr_t,int32_t)>(image+0x590FC0)(equip,id);
    int32_t slot0{-1},selected{-1};Read(equip+0x18,&slot0,4);Read(equip+0x88,&selected,4);
    const bool attuned=set && slot0==id && selected==0;   // its second step review's M1: the spell in slot 0 and slot 0 selected
    uintptr_t player{},modules{},data{};int32_t fp{-1},fp_max{-1};bool filled{};
    if(Read(manager+0x80,&player,8) && player && Read(player+0x1F90,&modules,8) && modules && Read(modules+0x18,&data,8) && data &&
       Read(data+0xE4,&fp,4) && Read(data+0xE8,&fp_max,4) && fp_max>=10 && fp_max<=2000 && fp>=0 && fp<=fp_max)
        filled=fp==fp_max || WriteWitnessWord(data+0xE4,fp_max);
    char row[288]{};sprintf_s(row,"{\"event\":\"test_attune\",\"k\":%zu,\"listed\":%zu,\"id\":%d,\"row\":true,\"set\":%s,\"slot0\":%d,\"selected\":%d,\"fp\":%d,\"fp_max\":%d,\"filled\":%s,\"attuned\":%s}\n",
        next,spells.size(),id,set?"true":"false",slot0,selected,fp,fp_max,filled?"true":"false",attuned?"true":"false");Log(row);
    TestToolsAnnounce(false,true,false,attuned?900:300);
}
// I-0140 tranche 15 (ROADMAP workstream 5 step 5: the buffs on a weapon, the statuses, a hand for Carthus Flame Arc;
// STUDY-EQUIP-SPEFFECT, the calls re-read by the pilot in GhidraMCP): F1 takes the next word of test-effect.txt beside the
// module, one word a press.
// - s<id>: the SpEffect id on the player through 140888F20(ChrIns, id, scale 1.0 in XMM2), the add the equipment effects use
//   (1408C46B0: the module +0x11C8's own add with its defaults, no broadcast), refused without a SpEffectParam row (the lookup
//   140E3E7E0, repository 0xF: id at +0, the row at +8, 0 when absent; the repository [144798108] read first, as the attune tool).
// - L<id>, R<id>: the weapon (its item id, category 0: with its infusion and level) into the left or the right
//   hand's current slot (ChrAsm = PlayerGameData +0x2B0, its current slots +0xC and +0x10 read as 1409F5BF0 does; slot current
//   x 2, +1 for the right hand, 140868490) by the equipment menu's own sequence (140B064E0): the inventory index found by its
//   id (EquipGameData = PlayerGameData +0x228, its inventory +0x1A8: +0x24 the first block's count, +0x58 and +0x48 the two
//   blocks, +0x88 the last index, 16 bytes an entry, +0 its handle, +4 the id; 14058E5C0), the menu's gate 140B06930(slot), the index's
//   earlier slot cleared (1405842E0, 1405828F0), 140A32290 and 140A323E0 for the value, 140584FF0(EGD, slot, &value, index, 1, 1,
//   0), 1405859A0(EGD), 1408C0930(player) and 1406C1DD0(slot). Left out of the menu's sequence: its record 140765220(0x47,
//   index) (the menu's own state); its extra 1405859A0 between the clear and the equip when the slot's weapon has
//   EquipParamWeapon +0xE2 0xD or 0xE (140582580); its unequip (140B07290) when the value already sits in that slot (the tool
//   then changes nothing and reads it back). Refused when no inventory entry holds the value (F7 gives it).
// - A<id>, B<id> (I-0140 tranche 17, the bows and crossbows): the arrow or the bolt into the right hand's current arrow or bolt
//   slot by the same sequence: 140868490 maps the right hand's arrow (-5) to ChrAsm +0x18's current x 2 + 6 and its bolt (-3)
//   to +0x20's x 2 + 7 (slots 6 to 9; READ, GhidraMCP 2026-11-16; -5 and -3 the right hand's at 1408BB0A7-1408BB149, where -6
//   and -4 are followed by 140868C50(-2) and -5 and -3 by 140868C50(-1)); the menu 140B064E0 runs the same sequence for every
//   slot below 0x16 (0x16 to 0x25 are the quick items, another path), its gate 140B06930 and 1406C1DD0 take any of them (READ),
//   with the menu's extra 1405859A0 after the clear when the target slot held ammunition before it (the menu reads the slot's id
//   with 140582580 before the clear and tests its EquipParamWeapon +0xE2 for 0xD or 0xE; the tool tests the id against the
//   player's ammunition ranges instead). A deliberate rule of the tool, not the game's: the ids are held to the player's
//   ammunition ranges (arrows and greatarrows 400000 to 403999, bolts 404000 to 404999, +0xE2 0xD and 0xE in those rows of
//   evidence/I-0140/e84/weapon-param.tsv; other 0xD and 0xE rows exist, 98000 to 99100, 409900, 419900, 63990000, 67000000), so a bolt never enters
//   an arrow slot.
// - Q<id> (tranche 17, the goods' own use): the goods into quick-item slot 0 (menu slot 0x16) by the menu's quick-item branch
//   (140B064E0 when 1405826A0(slot)): the inventory index (the entry's +4 the goods id with its category nibble 4, 0x40000000 | id),
//   the gate, the value (140A32290, 140A323E0: the entry's handle, read first, which a quick slot's clear leaves alone), the
//   index's earlier quick slot cleared (140583500, 140585A30, 1405859A0), 140585960(EGD, 0, &value, index, 1) (for a quick index below 10 its tail 14058FA90 writes EquipGameData +0x268), 1405859A0
//   and 1408C0930; then slot 0 selected as the next occupied slot after slot 9 (14058F4B0(EGD +0x250, -1), the writer of its
//   +0x68, READ). Read back: +0x68 is 0, slot 0's index (+0x26C) the entry's, the override +0x310 negative; the row's
//   read_back is then the selected quick index, with slot0_index and override beside it. The game's use input (R) reads the
//   goods through 140583DA0 (the use's latch in 1408C7940, READ by a ghidra-research subagent): +0x310 when not negative, else
//   14058ED60, which passes +0x68 to 14058A6F0(*(+0x250 +0xA0), &handle, +0x68) and maps the handle to the inventory index
//   (READ by the pilot in the disassembly, 2026-11-16); the request from the input to the animation is not read. Left out of
//   the menu's sequence: its record 140765220(0x47, index), and its unequip when the item already sits in slot 0 (the tool then
//   changes nothing but the selection). Departures: the selection (14058F4B0) is the tool's, the menu selects nothing; and the menu offers a quick slot only the goods its own list admits; the tool
//   takes any goods the inventory holds (key items included), but refuses ids from 1000000 (the spells, held as goods under
//   their Magic ids, which the menu never puts in a quick slot). Refused (equipped false) when the item is not a goods (its item value's nibble 4,
//   14058C950).
// - n<id> (I-0140 tranche 19, the statuses on NPCs and enemies): the same add on the nearest other character within 25 m of
//   VisitTestCharacters (F6's walk and HP bounds, nearest instead of strongest; the player's own +0x11C8 module test applied to
//   it); refused (applied false) when none is near. The row names the character by its HP, maximum, position and distance (no
//   entity id is read). That 140888F20 takes any ChrIns, not only the player's, is INFERRED from its receiver (a ChrIns method
//   reading +0x11C8); the film says whether the status shows. A character not drawn is not told apart.
// - p<id> (tranche 19, a hit's check; the id, below 0x0FFFFFFF as for every word, is only logged): the same nearest character's
//   row, nothing added, applied true when one was found: the HP between two probes of the same character (its maximum, a position
//   within about 1 m) says whether a swing between them hit it, unless a status of that character took HP meanwhile.
// - f<id> (tranche 20, a target in front: the statuses in view, the hits; the id only logged): the player's heading, the physics
//   module's +0x74 (OBSERVED in I-0114 series 9: it flips by pi at each leg's reversal), written toward the nearest living other
//   character of VisitTestCharacters and held 1.5 s (written each call, as the witness teleport holds a position): the facing
//   is (-sin h, -cos h) in x and z (OBSERVED in i114-cam-f60's rows: a steady walk's atan2(-dx, -dz) equals the heading to
//   0.001 rad at the 90th percentile, 0.01 at most; its step review's L3: headings near 0 and pi only, so another direction is the
//   form's extrapolation, which the release row and the film check), so h = atan2(player x - target x, player z - target z). A test_face_released row reads the heading 0.25 s after the
//   hold (whether the game kept it); the camera is not moved (the runner's middle click puts it behind the player or locks on).
// - h<id> (tranche 22, a target that lives through the audit's hits and statuses; the id only logged): the same nearest character
//   as p, its HP (+0xD8 of its character data module) written to its maximum (+0xDC) once, as F4 refills the player's; the row
//   gives the HP before. Applied false when none is near; a character at 0 HP is not in VisitTestCharacters, so none is raised.
//   After an applied h, a reader of the rows compares the next p with the maximum, not with the p before it (no code does).
// - Without a player, or when the menu's gate refuses (a transient state of the player), the word waits (a test_effect_wait
//   row) for the next press.
// One test_effect row a press (for a hand, an arrow or a bolt the slot's id read back at ChrAsm +0x7C; for Q read_back is the selected
// quick index, slot0_index and override beside it).
void ApplyTestEffect(uintptr_t manager,bool pressed) noexcept {
    static std::vector<std::pair<char,int32_t>> words;static size_t next{};static bool read{},effect_list_opened{};
    // Tranche 20: f's hold (the heading written each call until face_until), then one row 0.25 s after it.
    static float face_heading{};static ULONGLONG face_until{},face_check{};
    if(face_until || face_check){const ULONGLONG now=GetTickCount64();const uintptr_t physics=WitnessPhysics(manager);
        if(face_until && now<face_until){if(physics){int32_t bits{};std::memcpy(&bits,&face_heading,4);WriteWitnessWord(physics+0x74,bits);}}
        else if(face_until){face_until=0;face_check=now+250;}   // its step review's L2: before the runner's next key, 2 s after the press
        else if(now>=face_check){face_check=0;float heading{};const bool kept=physics && Read(physics+0x74,&heading,4);char value[32]{"null"};if(kept)sprintf_s(value,"%.4f",heading);
            char row[160]{};sprintf_s(row,"{\"event\":\"test_face_released\",\"wanted\":%.4f,\"heading\":%s}\n",face_heading,value);Log(row);}}
    if(!pressed)return;
    if(!read){read=true;const auto path=ModuleSibling(L"test-effect.txt");FILE* file{};
        if(!path.empty() && !_wfopen_s(&file,path.c_str(),L"rb") && file){effect_list_opened=true;char word[32]{};
            while(words.size()<4096 && fscanf_s(file,"%31s",word,static_cast<unsigned>(sizeof word))==1){
                const char kind=static_cast<char>(word[0]|0x20);char* end{};const unsigned long id=word[1]>='0' && word[1]<='9'?std::strtoul(word+1,&end,10):0;
                if((kind=='s' || kind=='n' || kind=='p' || kind=='h' || kind=='f' || kind=='l' || kind=='r' || kind=='a' || kind=='b' || kind=='q') && end && end!=word+1 && !*end && id && id<0x0FFFFFFF)words.emplace_back(kind,static_cast<int32_t>(id));}
            fclose(file);}
        char row[112]{};sprintf_s(row,"{\"event\":\"test_effect_list\",\"opened\":%s,\"listed\":%zu}\n",effect_list_opened?"true":"false",words.size());Log(row);}
    if(next>=words.size()){char row[128]{};sprintf_s(row,"{\"event\":\"test_effect\",\"listed\":%zu,\"done\":%s}\n",words.size(),!words.empty()?"true":"false");
        Log(row);TestToolsAnnounce(false,true,false,300);return;}
    const auto [kind,id]=words[next];
    uintptr_t player{},game_data{},player_data{};
    const bool found=Read(manager+0x80,&player,8) && player && Read(image+0x47572B8,&game_data,8) && game_data &&
        Read(game_data+0x10,&player_data,8) && player_data;
    // Its step review's M3: without a player (a load, a warp) the word is not spent; the next press takes it again.
    if(!found){char row[160]{};sprintf_s(row,"{\"event\":\"test_effect_wait\",\"next\":%zu,\"kind\":\"%c\",\"id\":%d,\"found\":false}\n",next+1,kind,id);
        Log(row);TestToolsAnnounce(false,true,false,300);return;}
    ++next;
    if(kind=='f'){TestNearCharacter closest{};closest.d2=-1.f;   // tranche 20: the player turned toward the nearest character
        VisitTestCharacters(manager,player,[&](const TestNearCharacter& c){if(closest.d2<0.f || c.d2<closest.d2)closest=c;});
        const uintptr_t physics=WitnessPhysics(manager);float at[3]{},before{};   // its step review's L1: 0 in the row when not read (no NaN)
        const bool faced=closest.chr && physics && Read(physics+0x80,at,12) && Read(physics+0x74,&before,4);
        if(faced){face_heading=std::atan2(at[0]-closest.at[0],at[2]-closest.at[2]);face_until=GetTickCount64()+1500;face_check=0;}
        char row[320]{};sprintf_s(row,"{\"event\":\"test_effect\",\"k\":%zu,\"listed\":%zu,\"kind\":\"f\",\"id\":%d,\"target\":%s,\"distance\":%.2f,\"hp\":%d,\"hp_max\":%d,\"at\":[%.2f,%.2f,%.2f],\"before\":%.4f,\"heading\":%.4f,\"applied\":%s}\n",
            next,words.size(),id,closest.chr?"true":"false",closest.chr?std::sqrt(closest.d2):-1.f,closest.hp,closest.hp_max,closest.at[0],closest.at[1],closest.at[2],
            faced?before:0.f,faced?face_heading:0.f,faced?"true":"false");Log(row);
        TestToolsAnnounce(false,true,false,faced?900:300);return;}
    if(kind=='p' || kind=='h'){TestNearCharacter closest{};closest.d2=-1.f;   // tranche 19: a probe, nothing added; tranche 22: h refills it
        VisitTestCharacters(manager,player,[&](const TestNearCharacter& c){if(closest.d2<0.f || c.d2<closest.d2)closest=c;});
        const bool applied=closest.chr && (kind=='p' || WriteWitnessWord(closest.data+0xD8,closest.hp_max));
        // Its second step review's M1 and L5: the word's id and applied (a character found) as the runner's word check reads them; 320 bytes.
        char row[320]{};sprintf_s(row,"{\"event\":\"test_effect\",\"k\":%zu,\"listed\":%zu,\"kind\":\"%c\",\"id\":%d,\"target\":%s,\"distance\":%.2f,\"hp\":%d,\"hp_max\":%d,\"at\":[%.2f,%.2f,%.2f],\"applied\":%s}\n",
            next,words.size(),kind,id,closest.chr?"true":"false",closest.chr?std::sqrt(closest.d2):-1.f,closest.hp,closest.hp_max,closest.at[0],closest.at[1],closest.at[2],applied?"true":"false");Log(row);
        TestToolsAnnounce(false,true,false,applied?900:300);return;}
    if(kind=='s' || kind=='n'){
        uintptr_t repository{};if(!Read(image+0x4798108,&repository,8) || !repository){char row[128]{};sprintf_s(row,"{\"event\":\"test_effect\",\"k\":%zu,\"kind\":\"%c\",\"id\":%d,\"repository\":false}\n",next,kind,id);
            Log(row);TestToolsAnnounce(false,true,false,300);return;}
        struct {int32_t id{-1},word{};uintptr_t row{};uint64_t tail{};} lookup;
        reinterpret_cast<void(__fastcall*)(void*,int32_t)>(image+0xE3E7E0)(&lookup,id);
        // Tranche 19: n<id> takes the nearest other living character within 25 m (VisitTestCharacters).
        uintptr_t target=player;TestNearCharacter closest{};closest.d2=-1.f;
        if(kind=='n'){VisitTestCharacters(manager,player,[&](const TestNearCharacter& c){if(closest.d2<0.f || c.d2<closest.d2)closest=c;});target=closest.chr;}
        uintptr_t module{};const bool has_module=target && Read(target+0x11C8,&module,8) && module;
        if(lookup.row && has_module)reinterpret_cast<void(__fastcall*)(uintptr_t,int32_t,float)>(image+0x888F20)(target,id,1.0f);
        char row[320]{};sprintf_s(row,"{\"event\":\"test_effect\",\"k\":%zu,\"listed\":%zu,\"kind\":\"%c\",\"id\":%d,\"row\":%s,\"target\":%s,\"distance\":%.2f,\"hp\":%d,\"hp_max\":%d,\"at\":[%.2f,%.2f,%.2f],\"module\":%s,\"applied\":%s}\n",
            next,words.size(),kind,id,lookup.row?"true":"false",target?"true":"false",closest.chr?std::sqrt(closest.d2):-1.f,closest.hp,closest.hp_max,closest.at[0],closest.at[1],closest.at[2],
            has_module?"true":"false",lookup.row && has_module?"true":"false");Log(row);
        TestToolsAnnounce(false,true,false,lookup.row && has_module?900:300);return;}
    // A hand, an arrow or a bolt: the current slot, the inventory index holding the value, the menu's sequence.
    const uintptr_t equip=player_data+0x228,asm_=player_data+0x2B0,inventory=equip+0x1A8;
    const bool ammo=kind=='a' || kind=='b',quick=kind=='q';
    int32_t current{-1};if(!quick)Read(asm_+(kind=='l'?0xC:kind=='r'?0x10:kind=='a'?0x18:0x20),&current,4);
    // Its step review's L4 and the final review's M1, M2: an arrow only into an arrow slot, a bolt into a bolt slot, no id of the
    // player's ammunition ranges (400000 to 404999) into a hand (the other +0xE2 0xD and 0xE rows are not refused), and no spell
    // (an id from 1000000) into a quick slot.
    const bool ammo_kind=kind=='a'?id>=400000 && id<=403999:kind=='b'?id>=404000 && id<=404999:kind=='q'?id<1000000:!(id>=400000 && id<=404999);
    const int32_t slot=!ammo_kind?-1:quick?0x16:ammo?(current>=0 && current<=1?current*2+(kind=='a'?6:7):-1):current>=0 && current<=2?current*2+(kind=='r'?1:0):-1;
    uint32_t first_count{};int32_t last{-1};uintptr_t block1{},block2{};
    int32_t index=-1;
    if(slot>=0 && Read(inventory+0x24,&first_count,4) && Read(inventory+0x88,&last,4) && Read(inventory+0x58,&block1,8) && Read(inventory+0x48,&block2,8) &&
       last>=0 && last<100000)
        for(int32_t i=0;i<=last && index<0;++i){const uintptr_t base=static_cast<uint32_t>(i)<first_count?block1:block2;if(!base)continue;
            const uintptr_t entry=base+16ull*(static_cast<uint32_t>(i)<first_count?static_cast<uint32_t>(i):static_cast<uint32_t>(i)-first_count);
            // i140-e88 (the first run: every index -1): an entry is {+0 its handle (0x8080xxxx for a weapon, what 140A323E0 gives
            // and ChrAsm +0x24 holds), +4 the item id (ChrAsm +0x7C's), +8 the count}, as the save's slot lays them out; the id is matched.
            int32_t handle{},item_id{};if(Read(entry,&handle,4) && handle && Read(entry+4,&item_id,4) &&
                (quick?static_cast<uint32_t>(item_id)==(0x40000000u|static_cast<uint32_t>(id)):item_id==id))index=i;}   // the final review's L1: a goods entry holds 0x40000000 | id
    bool equipped{},allowed{};int32_t read_back{-1},earlier{-1};
    // Its step review's M1: the menu's own gate at its entry (140B06930(slot): the player's state, 1408BD8F0) refuses as the menu does.
    if(index>=0)allowed=(reinterpret_cast<uint64_t(__fastcall*)(int32_t)>(image+0xB06930)(slot)&0xFF)!=0;
    // Its second step review's L2: a gate's refusal is a transient state of the player, so the word waits as without a player.
    if(index>=0 && !allowed){--next;char row[176]{};sprintf_s(row,"{\"event\":\"test_effect_wait\",\"next\":%zu,\"kind\":\"%c\",\"id\":%d,\"allowed\":false}\n",next+1,kind,id);
        Log(row);TestToolsAnnounce(false,true,false,300);return;}
    int32_t slot0_index{-1},override_id{-1};
    if(index>=0 && allowed && quick){
        // Its second step review's L2: only a goods goes into a quick slot. i140-t17 (every Q word refused): 140A323E0's value is
        // the entry's handle, not a category; the category is the item value's nibble 4 from 14058C950(inventory, &value, index),
        // the read 140583DA0 itself makes before masking 0x0FFFFFFF (READ, GhidraMCP 2026-11-16: 140A32450, 140583B90).
        struct {uintptr_t entry;int32_t index;int32_t pad;} item{};int32_t value_out{};
        reinterpret_cast<void*(__fastcall*)(void*,uintptr_t,int32_t)>(image+0xA32290)(&item,equip,index);
        int32_t* value=reinterpret_cast<int32_t*(__fastcall*)(void*,int32_t*)>(image+0xA323E0)(&item,&value_out);
        uint32_t item_value{0xFFFFFFFFu};reinterpret_cast<void*(__fastcall*)(uintptr_t,uint32_t*,int32_t)>(image+0x58C950)(inventory,&item_value,index);
        const bool goods=item_value!=0xFFFFFFFFu && (item_value>>28)==4u;   // 140A323E0 returns its second argument
        earlier=goods?static_cast<int32_t>(reinterpret_cast<uint32_t(__fastcall*)(uintptr_t,int32_t)>(image+0x583500)(equip,index)):-2;
        if(goods && earlier!=0){
            if(earlier>=0){reinterpret_cast<void(__fastcall*)(uintptr_t,int32_t)>(image+0x585A30)(equip,earlier);reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0x5859A0)(equip);}
            reinterpret_cast<void(__fastcall*)(uintptr_t,int32_t,int32_t*,int32_t,int32_t)>(image+0x585960)(equip,0,value,index,1);
            reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0x5859A0)(equip);
            reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0x8C0930)(player);}
        if(goods)reinterpret_cast<void(__fastcall*)(uintptr_t,int32_t)>(image+0x58F4B0)(equip+0x250,-1);
        // Its L6: the selection, slot 0's index and the use's override (EquipGameData +0x310, which wins over the selection in
        // 140583DA0 when not negative) in the row.
        int32_t selected{-1};Read(equip+0x250+0x68,&selected,4);Read(equip+0x268+4,&slot0_index,4);Read(equip+0x310,&override_id,4);
        read_back=selected;equipped=goods && selected==0 && slot0_index==index && override_id<0;}
    else if(index>=0 && allowed){
        earlier=static_cast<int32_t>(reinterpret_cast<uint32_t(__fastcall*)(uintptr_t,int32_t)>(image+0x5842E0)(equip,index));
        if(earlier!=slot){
            if(earlier>=0){
                // Its step review's L1, its third's L1: the menu's extra 1405859A0 when the target slot held ammunition, read before the clear.
                int32_t held{-1};const bool held_ammo=ammo && Read(asm_+0x7C+4ull*static_cast<uint32_t>(slot),&held,4) && held>=400000 && held<=404999;
                reinterpret_cast<void(__fastcall*)(uintptr_t,int32_t,int32_t)>(image+0x5828F0)(equip,earlier,1);
                if(held_ammo)reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0x5859A0)(equip);}
            struct {uintptr_t entry;int32_t index;int32_t pad;} item{};int32_t value_out{};
            reinterpret_cast<void*(__fastcall*)(void*,uintptr_t,int32_t)>(image+0xA32290)(&item,equip,index);
            int32_t* value=reinterpret_cast<int32_t*(__fastcall*)(void*,int32_t*)>(image+0xA323E0)(&item,&value_out);
            reinterpret_cast<void(__fastcall*)(uintptr_t,int32_t,int32_t*,int32_t,int32_t,char,char)>(image+0x584FF0)(equip,slot,value,index,1,1,0);
            reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0x5859A0)(equip);
            reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0x8C0930)(player);
            reinterpret_cast<void(__fastcall*)(int32_t)>(image+0x6C1DD0)(slot);}   // its step review's M1: the menu's last call (a hand: [[player+0x1F90]+0x20]+0x50 raised)
        Read(asm_+0x7C+4ull*static_cast<uint32_t>(slot),&read_back,4);equipped=read_back==id;}
    char row[320]{};sprintf_s(row,"{\"event\":\"test_effect\",\"k\":%zu,\"listed\":%zu,\"kind\":\"%c\",\"id\":%d,\"current\":%d,\"slot\":%d,\"index\":%d,\"allowed\":%s,\"earlier\":%d,\"read_back\":%d,\"slot0_index\":%d,\"override\":%d,\"equipped\":%s}\n",
        next,words.size(),kind,id,current,slot,index,allowed?"true":"false",earlier,read_back,slot0_index,override_id,equipped?"true":"false");Log(row);
    TestToolsAnnounce(false,true,false,equipped?900:300);
}
void ApplyTestTools(uintptr_t manager,LONG64 call) noexcept {
    static const bool enabled=ModuleFlag(L"test-tools.flag");if(!enabled)return;
    static bool invincible{},f1_down{},f4_down{},f5_down{},f6_down{},f7_down{},f11_down{},titled{};
    if(!titled){titled=true;TestToolsAnnounce(false,false);Log("{\"event\":\"test_tools_enabled\"}\n");}
    ApplyTestInvincible(manager,TestToolsKeyEdge(VK_F4,f4_down),invincible);
    ApplyTestRevive(manager,call,TestToolsKeyEdge(VK_F5,f5_down));
    ApplyTestKill(manager,TestToolsKeyEdge(VK_F6,f6_down));
    ApplyTestGive(manager,TestToolsKeyEdge(VK_F7,f7_down));
    ApplyTestAttune(manager,TestToolsKeyEdge(VK_F11,f11_down));
    ApplyTestEffect(manager,TestToolsKeyEdge(VK_F1,f1_down));
}
// I-0138 tranche 6 (ROADMAP workstream 11): the tone map's look a world frame is asked: the test flags first, then (tranche 7)
// the settings' colorimetry row (below). tone-look-agx.flag,
// tone-look-aces.flag: that candidate's curve in place of the record's, the area's colour matrix and LUT kept; with
// tone-look-neutral.flag neither (the candidate's own look); tone-look-er.flag: the candidate after Elden Ring (always without
// the area's grade). tone-look-cycle.flag (the sheets' runs): the next of the list
// below after each of world-pictures.flag's pictures (02 native_world_picture_mark: the first picture shows the original, the
// k-th the k-th look of the list, wherever the pictures start; one still view gives every look).
// tone-look-keys.flag (a session with Thomas at the keyboard): F2 the next look, F3 the next knee of the HDR entries'
// highlights (0.75 the default; 1: the SDR picture inside the HDR10 chain). A row and a label on the screen (16b) at each
// change. Called once a prepared world frame, on the thread that prepares it; the lock is a guard.
// I-0138 tranche 7: without a test flag, the settings' colorimetry (live: the Advanced page's row), the Rekindled look the candidate
// after Elden Ring with its midtone lift and its vivid variant (tranche 7b, D-0296), its chromatic aberration the settings' sub-row;
// the test flags keep it.
struct NativeToneLookAsked {ds3native::look::ToneLook look{};bool neutral{};float hdr_knee{ds3native::kPostToneHdrKnee};bool aberration{true};};
std::mutex native_test_label_lock;std::string native_test_label;uint64_t native_test_label_tick{};   // the label 16b draws (2.5 s)
NativeToneLookAsked NativeToneLookRequested(uint64_t presentation_serial) noexcept {
    using Look=ds3native::look::ToneLook;
    struct Step {Look look;bool neutral;const char* name;};
    static constexpr Step steps[]{{Look::Original,false,"Original"},{Look::Agx,false,"AgX with the area grade"},{Look::Agx,true,"AgX neutral"},
        {Look::Aces,false,"ACES with the area grade"},{Look::Aces,true,"ACES neutral"},{Look::Er,true,"After Elden Ring"}};
    static constexpr float knees[]{ds3native::kPostToneHdrKnee,0.5f,0.9f,1.0f};
    static constexpr const char* knee_names[]{"normal","strong","soft","off"};   // the label's words: the titles' font has letters only (no digit, plus or bar: i138-t6b-lookkeys' first label)
    static const bool agx=ModuleFlag(L"tone-look-agx.flag"),aces=!agx && ModuleFlag(L"tone-look-aces.flag"),er=!agx && !aces && ModuleFlag(L"tone-look-er.flag"),
        neutral=ModuleFlag(L"tone-look-neutral.flag"),cycle=ModuleFlag(L"tone-look-cycle.flag"),keys=ModuleFlag(L"tone-look-keys.flag");
    if(!agx && !aces && !er && !cycle && !keys){const auto s=NativeSettingsCurrent();
        if(s.look!=ds3native::settings::Look::Rekindled)return {};
        NativeToneLookAsked asked{};asked.look=Look::Er;asked.neutral=true;asked.aberration=s.aberration;return asked;}
    static std::mutex lock;const std::lock_guard hold(lock);
    static size_t step=agx?(neutral?2u:1u):aces?(neutral?4u:3u):er?5u:0u,knee{};static bool said{},f2{},f3{};
    bool changed=!said;said=true;
    if(cycle){const uint64_t mark=native_world_picture_mark.load(std::memory_order_acquire),taken=mark>>40,last=mark&0xFFFFFFFFFFull;
        const size_t now=size_t((taken && presentation_serial<=last?taken-1:taken)%std::size(steps));   // a frame up to the last picture's still shows that picture's look
        if(now!=step){step=now;changed=true;}}
    if(keys){
        if(TestToolsKeyEdge(VK_F2,f2)){step=(step+1)%std::size(steps);changed=true;}
        if(TestToolsKeyEdge(VK_F3,f3)){knee=(knee+1)%std::size(knees);changed=true;}}
    if(changed){char row[224]{};
        sprintf_s(row,"{\"event\":\"native_tone_look\",\"frame\":%llu,\"look\":\"%s\",\"neutral\":%s,\"hdr_knee\":%g}\n",(unsigned long long)presentation_serial,
            ds3native::look::Name(steps[step].look),steps[step].neutral?"true":"false",knees[knee]);Log(row);
        if(keys){char text[96]{};sprintf_s(text,"%s      HDR highlights %s",steps[step].name,knee_names[knee]);
            const std::lock_guard label(native_test_label_lock);native_test_label=text;native_test_label_tick=GetTickCount64();}}
    return {steps[step].look,steps[step].neutral,knees[knee]};}
// I-0097 tranche 5 (Thomas, 2026-10-25: the enemies of the replayed study paths without AI; a test tool, never in the play
// launcher): with test-no-ai.flag next to this module, DS3's own debug byte 14477FE9F is set (a block of debug bytes at
// 14477FE94-14477FEAB, each tested after the block's init check 14477FEC8). INFERRED from the decompile: its AI target and move
// steps 1405CE840 and 1405CE8E0 read it as they read a character's own bit 0x40 of +0x1EE8, so no enemy takes the player as its
// target. Both renderers (called from the world update gate, the original probe's hook too), so the native and original runs of a
// path stay comparable.
// I-0114 tranche 8 (Thomas, 2026-11-02: the enemies still walk under test-no-ai and can push the player off a measured path; « le
// mieux serait de complètement figer l'IA ennemie »): test-freeze-ai.flag also sets the block's next two bytes (GhidraMCP, read-only):
// 14477FEA0, which makes 1405CAB20 zero the move of every character but the local player ([WorldChrMan+0x80], 14088CFF0) and
// one for which 14088D1C0 holds (INFERRED a phantom), as a character's own bit 0x80 of +0x1EE8 does (also read by 1405DF000,
// 1405E08F0 and the mover at 1409A8428; 1405CAB20 still adds its virtual +0x90's vector after); and 14477FEA1, under which 1405C5E70 answers false, so the AI controller's step 1405CEA80
// (its AI object [+0x320], the target and move steps 1405CE840 / 1405CE8E0) is skipped. Not everything stops: in series 39 a leech
// enemy at the top of Anor Londo's stairs still caught the player (what acts there is not established).
void ApplyTestNoAi(LONG64 call) noexcept {
    static const bool freeze=ModuleFlag(L"test-freeze-ai.flag"),enabled=freeze || ModuleFlag(L"test-no-ai.flag");if(!enabled)return;
    auto* const flag=reinterpret_cast<volatile uint8_t*>(image+0x477FE9F);
    if(*flag!=1 || (freeze && (flag[1]!=1 || flag[2]!=1))){*flag=1;if(freeze){flag[1]=1;flag[2]=1;}
        char row[96]{};sprintf_s(row,"{\"event\":\"test_no_ai\",\"call\":%lld,\"freeze\":%d}\n",call,freeze?1:0);Log(row);}
}
// I-0100 tranche 4 (the LOD threshold after a death, Thomas 2026-10-26; a test tool, never in the play launcher): with
// test-death.flag next to this module, the player dies once, 75 s after the first schedule call that reads a plausible HP (the
// character data module of ApplyTestInvincible, HP +0xD8 written to 0), so a run compares the same place after a launch and after
// a death's reload in either renderer (the test save is restored by its runner).
void ApplyTestDeath(uintptr_t manager) noexcept {
    static const bool enabled=ModuleFlag(L"test-death.flag");if(!enabled)return;
    static ULONGLONG first{};static bool done{};if(done)return;
    uintptr_t player{},modules{},data{};int32_t hp{},hp_max{};
    if(!(Read(manager+0x80,&player,8) && player && Read(player+0x1F90,&modules,8) && modules && Read(modules+0x18,&data,8) && data &&
        Read(data+0xD8,&hp,4) && Read(data+0xDC,&hp_max,4)) || hp_max<20 || hp_max>20000 || hp<=0 || hp>hp_max)return;
    const auto now=GetTickCount64();if(!first)first=now;if(now-first<75000)return;
    done=true;const bool written=WriteWitnessWord(data+0xD8,0);
    char row[160]{};sprintf_s(row,"{\"event\":\"test_death\",\"hp\":%d,\"hp_max\":%d,\"written\":%s}\n",hp,hp_max,written?"true":"false");Log(row);
}
// I-0140 tranche 30 (D-0298, Thomas 2026-11-21: E8's idea): with test-region-close.flag next to this module, the native region
// registry is closed once in play (CloseNativeEnvRegions, the Close's own step), 60 s after the first schedule call that reads a
// plausible HP, and the game goes on without regions: the paths written for the close (D0A890's, D0C2F0's and D0CA20's requests
// and 65320 answered 0, 652D0's lookups the closed manager's default record, the snapshot and menu readers taking nothing from a
// manager that is off) run in play instead of only at the game's exit. The menu views keep their {1, 10} cube, which NativeMenuRegionIbl
// takes by name without the manager (its step review's L7): only their region pass and the main view lose the regions. Native mode
// only; the module's Close later finds the registry gone and returns. A test tool, never in the play launcher. The schedule step's
// thread is INFERRED: 8DD2E0 is tail-called by 8DF920, WorldChrManImp's task +31A8 registered at step 0x0E (LEGACY-ROOT-INVENTORY-903,
// Static 1085/1086), which ENGINE-STEP-THREAD-MAP puts on lane 0 (WorldChrMan_PrePhysics), as the main collect's DrawStep (its frame table's row
// 79); the start row names the thread. Its once-guards are atomic all the same (its step reviews' L4 and L3).
void ApplyTestRegionClose(uintptr_t manager) noexcept {
    static const bool flagged=ModuleFlag(L"test-region-close.flag");if(!flagged)return;
    if(control->mode!=bootstrap::Mode::NativeBootstrap){static std::atomic<bool> told{};   // the final review's L5: refused aloud
        if(!told.load(std::memory_order_relaxed) && !told.exchange(true))Log("{\"event\":\"test_region_close_refused\",\"reason\":\"native mode only\"}\n");return;}
    static std::atomic<ULONGLONG> first{};static std::atomic<bool> done{};if(done.load(std::memory_order_acquire))return;
    uintptr_t player{},modules{},data{};int32_t hp{},hp_max{};
    if(!(Read(manager+0x80,&player,8) && player && Read(player+0x1F90,&modules,8) && modules && Read(modules+0x18,&data,8) && data &&
        Read(data+0xD8,&hp,4) && Read(data+0xDC,&hp_max,4)) || hp_max<20 || hp_max>20000 || hp<=0 || hp>hp_max)return;
    const auto now=GetTickCount64();ULONGLONG start{};if(first.compare_exchange_strong(start,now))start=now;   // else the tick stored
    if(now<start || now-start<60000)return;
    if(done.exchange(true,std::memory_order_acq_rel))return;
    // Its step review's L8: whether a registry exists to close, and its wrappers before the close (a request in flight may still add
    // one, which the close retires: the fix's step review's L4).
    bool present{};size_t wrappers{};{const std::lock_guard hold(env_region_cpu_lock);present=static_cast<bool>(env_region_cpu);if(present)wrappers=env_region_cpu->wrappers.size();}
    {char row[200]{};sprintf_s(row,"{\"event\":\"test_region_close\",\"phase\":\"start\",\"tick\":%llu,\"thread\":%lu,\"registry\":%s,\"wrappers_before\":%zu}\n",static_cast<unsigned long long>(now),GetCurrentThreadId(),present?"true":"false",wrappers);Log(row);}
    CloseNativeEnvRegions();
    char row[160]{};sprintf_s(row,"{\"event\":\"test_region_close\",\"phase\":\"closed\",\"tick\":%llu}\n",static_cast<unsigned long long>(GetTickCount64()));Log(row);
}
void __fastcall ObserveChrManagerSchedule(uintptr_t manager,float dt) noexcept {
    static LONG64 calls{};const auto n=InterlockedIncrement64(&calls);
    ApplyWitnessTeleport(manager,n);ApplyWitnessPhantom(manager,n);ApplyWitnessFlag(n);ApplyWitnessFlagWatch(n);ApplyWitnessCutscene(manager,n);ApplyWitnessCutsceneClock(false,0);ApplyTestTools(manager,n);ApplyTestDeath(manager);ApplyTestRegionClose(manager);
    static const bool rows_on=control->mode!=bootstrap::Mode::NativeBootstrap || ModuleFlag(L"update-observe.flag");   // I-0120 H1: the row only in the Reference mode or with update-observe.flag
    if(rows_on && (n<=6 || n%300==0)){int32_t sets{};Read(manager+0x1C8,&sets,4);uintptr_t player{};Read(manager+0x80,&player,8);
        std::string counts;
        for(int32_t k=0;k<sets && k<16;++k){uintptr_t set{};int32_t count{};Read(manager+0x1D0+0x18ull*k,&set,8);if(set)Read(set,&count,4);
            counts+=(k?",":"")+std::to_string(count);}
        char row[400]{};sprintf_s(row,"{\"event\":\"native_chr_manager_schedule\",\"call\":%lld,\"dt\":%g,\"sets\":%d,\"counts\":[%.200s],\"limit_c0\":%d,\"player\":%s,\"caller\":%lld}\n",
            n,dt,sets,counts.c_str(),[&]{int32_t v{};Read(manager+0xC0,&v,4);return v;}(),player?"true":"false",
            static_cast<long long>(reinterpret_cast<uintptr_t>(_ReturnAddress())-image));Log(row);}
    chr_manager_schedule_original(manager,dt);}
void __fastcall ObserveChrModelRequest(uintptr_t chr,uintptr_t second) noexcept {
    static LONG64 calls{};const auto n=InterlockedIncrement64(&calls);
    if(n<=8 || n%600==0){char row[200]{};sprintf_s(row,"{\"event\":\"native_chr_model_request\",\"call\":%lld,\"chr\":%llu,\"caller\":%lld}\n",
        n,static_cast<unsigned long long>(chr),static_cast<long long>(reinterpret_cast<uintptr_t>(_ReturnAddress())-image));Log(row);}
    chr_model_request_original(chr,second);}
bool PopinObserve() noexcept;float PopinDistance(const float* position) noexcept;   // I-0100 tranche 3, below
// I-0100 tranche 3 (R4, R8: distant characters animated at a few frames a second natively): with popin-observe.flag and
// pose-observe.flag, every two seconds the public pose updates (16000) each receiver had in the window and its distance (its
// first pose row's translation against the LOD step's camera), both renderers alike: popin_pose_rates [[d, updates], ...], the
// 128 nearest.
void PopinPoseRate(uintptr_t receiver,uintptr_t rows) noexcept {
    static const bool on=ModuleFlag(L"pose-observe.flag") && PopinObserve();if(!on)return;
    struct Rate {uint32_t updates{};float d{-1.f};};static std::mutex rate_lock;static std::unordered_map<uintptr_t,Rate> rates;static int64_t window_start{};
    float d=-1.f;std::array<float,12> m{};
    if(rows && Read(rows,m.data(),sizeof m)){const float position[3]{m[3],m[7],m[11]};d=PopinDistance(position);}
    LARGE_INTEGER now{},frequency{};QueryPerformanceCounter(&now);QueryPerformanceFrequency(&frequency);std::string row;
    try{const std::lock_guard hold(rate_lock);if(rates.size()<4096 || rates.contains(receiver)){auto& r=rates[receiver];++r.updates;if(d>=0)r.d=d;}
        if(!window_start)window_start=now.QuadPart;
        if(now.QuadPart-window_start>=2*frequency.QuadPart){std::vector<std::pair<float,uint32_t>> cells;for(const auto& [key,r]:rates)if(r.d>=0)cells.emplace_back(r.d,r.updates);
            std::sort(cells.begin(),cells.end());if(cells.size()>128)cells.resize(128);
            char head[160]{};sprintf_s(head,"{\"event\":\"popin_pose_rates\",\"seconds\":%.2f,\"receivers\":%zu,\"rates\":[",double(now.QuadPart-window_start)/double(frequency.QuadPart),rates.size());row=head;
            for(size_t i=0;i<cells.size();++i){char cell[40]{};sprintf_s(cell,"%s[%.1f,%u]",i?",":"",cells[i].first,cells[i].second);row+=cell;}row+="]}\n";
            rates.clear();window_start=now.QuadPart;}}catch(...){}
    if(!row.empty())Log(row.c_str());
}
// I-0100 tranche 5 step 1: the map objects' pose hand-over 14066BBB0 (its +0x3D8 set while the object's item is marked drawn and
// within its draw distance, then 140C4C690), pass-through with popin-observe.flag in every mode: every 600 calls of an animated
// object (+0x2B0 set) one popin_object_syncs row with the calls and those that handed the pose over since the last row, and the
// distinct objects of each.
using ObjectSyncFn=void(__fastcall*)(uintptr_t,uintptr_t);
ObjectSyncFn object_sync_original{};
void __fastcall ObserveObjectSync(uintptr_t object,uintptr_t second) noexcept {
    object_sync_original(object,second);
    uintptr_t animation{};uint8_t synced{};if(!Read(object+0x2B0,&animation,8) || !animation || !Read(object+0x3D8,&synced,1))return;
    static std::mutex lock;static std::unordered_set<uintptr_t> seen,handed;static uint32_t calls{},syncs{};std::string row;
    try{const std::lock_guard hold(lock);++calls;syncs+=synced?1:0;if(seen.size()<4096)seen.insert(object);if(synced && handed.size()<4096)handed.insert(object);
        if(calls>=600){char text[200]{};sprintf_s(text,"{\"event\":\"popin_object_syncs\",\"calls\":%u,\"synced\":%u,\"objects\":%zu,\"objects_synced\":%zu}\n",calls,syncs,seen.size(),handed.size());
            row=text;calls=syncs=0;seen.clear();handed.clear();}}catch(...){}
    if(!row.empty())Log(row.c_str());
}
using PosePublicFn=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uint32_t);
PosePublicFn pose_public_original{};
void __fastcall ObservePosePublic(uintptr_t receiver,uintptr_t source,uintptr_t reference,uint32_t worker) noexcept {   // Reference, pass-through
    PopinPoseRate(receiver,source);pose_public_original(receiver,source,reference,worker);}
void __fastcall ObservePartPoseUpdate(uintptr_t binding,uintptr_t receiver,uintptr_t matrices,uintptr_t retarget,uintptr_t fifth,uintptr_t sixth,uintptr_t seventh) noexcept {
    // Review: the per-receiver tally (a global mutex and a map on the engine's parallel pose path) only with pose-observe.flag.
    static const bool witness=ModuleFlag(L"pose-observe.flag");
    if(!witness){part_pose_update_original(binding,receiver,matrices,retarget,fifth,sixth,seventh);return;}
    static std::mutex lock;static std::map<uintptr_t,uint64_t> counts;uint64_t n{};
    {const std::lock_guard hold(lock);auto it=counts.find(receiver);   // I-0120 H6: at most 4,096 receivers, as its siblings (0: not counted)
        if(it==counts.end() && counts.size()<4096)it=counts.emplace(receiver,0).first;
        if(it!=counts.end())n=++it->second;}
    // 1105: D2D040's own gates: [binding+8] (no model: skip), fifth = retarget map (null: direct 16000 with
    // [matrices+10]), else the retarget path needs [matrices+C] >= 1 before it calls 16000.
    if(n && (n<=2 || n%100==0)){uintptr_t binding_model{};int32_t matrix_count{},retarget_count{};
        Read(binding+8,&binding_model,8);if(matrices)Read(matrices+0xC,&matrix_count,4);if(fifth)Read(fifth+8,&retarget_count,4);
        char row[360]{};sprintf_s(row,"{\"event\":\"native_part_pose_update\",\"receiver\":%llu,\"count\":%llu,\"binding_model\":%s,\"retarget_map\":%s,\"retarget_count\":%d,\"matrix_count\":%d}\n",
        static_cast<unsigned long long>(receiver),static_cast<unsigned long long>(n),binding_model?"true":"false",fifth?"true":"false",retarget_count,matrix_count);Log(row);}
    part_pose_update_original(binding,receiver,matrices,retarget,fifth,sixth,seventh);}
// Optimisation review (ROADMAP C3 item 5): the wall interval between two game frames, measured at the same engine point
// under both renderers, summarised over windows of 120 frames (the original's and the native's pacing side by side). I-0083:
// one copy, called by the world update gate's witness and by the original probe's gate hook (the same point, the same code).
void RecordGameStep(int64_t call,bool native,float dt) noexcept {
    static LARGE_INTEGER last{},frequency{};static std::vector<float> intervals;LARGE_INTEGER now{};
    if(!frequency.QuadPart)QueryPerformanceFrequency(&frequency);QueryPerformanceCounter(&now);
    if(ds3native::cadence::on.load(std::memory_order_relaxed))   // I-0097 tranche 5 (cadence-trace.flag): the step and its dt together
        ds3native::cadence::MarkPair(ds3native::cadence::GameStep,now.QuadPart,ds3native::cadence::GameDt,std::isfinite(dt)?static_cast<int64_t>(std::clamp(double(dt)*1e6,-1e12,1e12)):-1);
    if(last.QuadPart)intervals.push_back(float(1000.0*double(now.QuadPart-last.QuadPart)/frequency.QuadPart));last=now;
    // I-0097: a game step after a pause of a second or more (a load: the world update gate stops while a zone loads) is logged
    // on its own, so a load's length is read alike in both renderers (from the pilot's click to this row).
    if(!intervals.empty() && intervals.back()>=1000.f){char resumed[160]{};sprintf_s(resumed,"{\"event\":\"game_step_resumed\",\"native\":%s,\"gap_ms\":%.0f,\"tick_ms\":%llu}\n",
        native?"true":"false",intervals.back(),static_cast<unsigned long long>(GetTickCount64()));Log(resumed);}
    if(intervals.size()==120){std::vector<float> sorted(intervals);std::sort(sorted.begin(),sorted.end());double sum{},squares{};
        for(const auto v:intervals){sum+=v;squares+=double(v)*v;}const double mean=sum/120.0,deviation=std::sqrt(std::max(0.0,squares/120.0-mean*mean));
        size_t over17{},over25{},over34{};for(const auto v:intervals){over17+=v>17.f;over25+=v>25.f;over34+=v>34.f;}
        char row[320]{};sprintf_s(row,"{\"event\":\"game_frame_times\",\"call\":%lld,\"native\":%s,\"ms_p50_p99_max\":[%.2f,%.2f,%.2f],\"ms_mean_stdev\":[%.2f,%.2f],\"over_17_25_34_ms\":[%zu,%zu,%zu],\"tick_ms\":%llu}\n",
            static_cast<long long>(call),native?"true":"false",sorted[59],sorted[118],sorted[119],mean,deviation,over17,over25,over34,static_cast<unsigned long long>(GetTickCount64()));LogFrameTimes(row,intervals);intervals.clear();}
}
// I-0083: the original probe's only other hook (original-probe.flag): the world update gate timed by RecordGameStep, so the
// original's game step is measured at the point the native's is (I-0081 « open »), nothing else read or written (except DS3's
// debug bytes under test-no-ai.flag and test-freeze-ai.flag, I-0097 tranche 5, I-0114 tranche 8: such a run is never a gate run).
void PopinReferenceWindow(int64_t step) noexcept;
void __fastcall ProbeOriginalWorldGate(uintptr_t step,uint8_t running,float dt,uint8_t p4) noexcept {
    const auto n=InterlockedIncrement64(&world_gate_calls);RecordGameStep(n,false,dt);ApplyTestNoAi(n);PopinReferenceWindow(n);   // I-0100 T0
    world_update_gate_original(step,running,dt,p4);}
void __fastcall ObserveWorldUpdateGate(uintptr_t step,uint8_t running,float dt,uint8_t p4) noexcept {
    static uint64_t last_key{~0ull};const auto n=InterlockedIncrement64(&world_gate_calls);world_gate_step=step;ApplyTestNoAi(n);
    if(reference_share_variant)ReferenceShareVariantIdle();   // I-0141 tranche 27, the step review's L1
    // I-0120 H1: the investigation rows of this hook (frame_pacing, player_skin_colour, player_part_model_masks, world_update_gate) and the
    // reads that only feed them run in the Reference mode or with update-observe.flag; RecordGameStep and the test tools stay unconditional.
    static const bool rows_on=control->mode!=bootstrap::Mode::NativeBootstrap || ModuleFlag(L"update-observe.flag");
    uint8_t a9{},f98{},f1f5{},f1f6{};int32_t f70{};uint32_t remo{};uint8_t event_c{},event_d{};uintptr_t object{},inner{};uint64_t key{};
    if(rows_on){
    Read(step+0xA9,&a9,1);Read(step+0x98,&f98,1);Read(step+0x1F5,&f1f5,1);Read(step+0x1F6,&f1f6,1);Read(step+0x70,&f70,4);
    if(Read(image+0x4791248,&object,8) && object && Read(object+8,&inner,8) && inner)Read(inner+0x154,&remo,4);
    object=0;inner=0;
    if(Read(image+0x4752F80,&object,8) && object && Read(object+8,&inner,8) && inner){Read(inner+0xC,&event_c,1);Read(inner+0xD,&event_d,1);}
    key=running|uint64_t(a9)<<1|uint64_t(f98)<<2|uint64_t(f1f5)<<3|uint64_t(f1f6)<<4|uint64_t(event_c)<<5|uint64_t(event_d)<<6|uint64_t(remo&0xFF)<<8|uint64_t(p4)<<16;}
    // 1118: frame pacing under both renderers (MoveMapStep runs once per game frame): wall time and engine dt
    // over each window of 300 calls. A measurement of the development build, not a performance claim.
    if(rows_on){static LARGE_INTEGER window_start{},frequency{};static double dt_sum{},dt_max{};
        if(!frequency.QuadPart)QueryPerformanceFrequency(&frequency);
        dt_sum+=dt;dt_max=std::max<double>(dt_max,dt);
        if(n%300==1){LARGE_INTEGER now{};QueryPerformanceCounter(&now);
            if(window_start.QuadPart){const double wall=static_cast<double>(now.QuadPart-window_start.QuadPart)/frequency.QuadPart;
                char row[256]{};sprintf_s(row,"{\"event\":\"frame_pacing\",\"call\":%lld,\"native\":%s,\"wall_s\":%.3f,\"fps\":%.2f,\"dt_mean_ms\":%.2f,\"dt_max_ms\":%.2f}\n",
                    n,control->mode==bootstrap::Mode::NativeBootstrap?"true":"false",wall,300.0/wall,dt_sum/300.0*1000.0,dt_max*1000.0);Log(row);}
            window_start=now;dt_sum=0;dt_max=0;}}
    // Optimisation review (the per-instance SH): the probe volumes registered in the root's probe manager G+0x46E8 (1D2D0:
    // count +0x58, probes +0x5C), both modes, every 600 frames.
    if(static const bool probe_witness=ModuleFlag(L"light-observe.flag");probe_witness && n%600==1){uintptr_t root{};int32_t counts[3]{};if(Read(image+0x4961B50,&root,8) && root)Read(root+0x46E8+0x58,counts,12);
        char row[200]{};sprintf_s(row,"{\"event\":\"probe_volumes\",\"call\":%lld,\"native\":%s,\"root\":%s,\"volumes\":%d,\"probes\":%d,\"third\":%d}\n",
            n,control->mode==bootstrap::Mode::NativeBootstrap?"true":"false",root?"true":"false",counts[0],counts[1],counts[2]);Log(row);}
    RecordGameStep(n,control->mode==bootstrap::Mode::NativeBootstrap,dt);
    // 1097: the main player's skin colour candidates, ChrAsmModel+280 and its draw entity group +40 (= entity+280,
    // read as PS cbModelParam FC_SkinMatchColor through draw state +280), under both renderers.
    if(rows_on && (n==30 || n%600==0)){uintptr_t manager{},player{},asm_model{},entity{};std::array<float,4> model_colour{},entity_colour{};
        if(Read(image+0x477FDB8,&manager,8) && manager && Read(manager+0x80,&player,8) && player && Read(player+0x20B0,&asm_model,8) && asm_model){
            Read(asm_model+0x280,model_colour.data(),16);if(Read(asm_model+8,&entity,8) && entity)Read(entity+0x280,entity_colour.data(),16);}
        char row[320]{};sprintf_s(row,"{\"event\":\"player_skin_colour\",\"call\":%lld,\"model_280\":[%g,%g,%g,%g],\"entity_280\":[%g,%g,%g,%g]}\n",n,
            model_colour[0],model_colour[1],model_colour[2],model_colour[3],entity_colour[0],entity_colour[1],entity_colour[2],entity_colour[3]);Log(row);
        // 1102: the player's part models' +220/+224 (57A20 row masks: +224 bit 15 adds the engine7 class-0x100 row).
        uint32_t parts{};uintptr_t descriptors{};std::string cells;
        if(entity && Read(entity+0xAF8,&parts,4) && Read(entity+0xB00,&descriptors,8) && descriptors)
            for(uint32_t i=0;i<parts && i<8;++i){uintptr_t binding{},part_model{};uint32_t w220{},w224{};
                if(Read(descriptors+0x50ull*i+0x10,&binding,8) && binding && Read(binding+8,&part_model,8) && part_model){
                    Read(part_model+0x220,&w220,4);Read(part_model+0x224,&w224,4);}
                char cell[64]{};sprintf_s(cell,"%s[%u,%u,%u]",cells.empty()?"":",",i,w220,w224);cells+=cell;}
        char parts_row[700]{};sprintf_s(parts_row,"{\"event\":\"player_part_model_masks\",\"call\":%lld,\"parts\":[%.600s]}\n",n,cells.c_str());Log(parts_row);}
    if(rows_on && (n<=12 || n%300==0 || key!=last_key)){last_key=key;
        char row[360]{};sprintf_s(row,"{\"event\":\"world_update_gate\",\"call\":%lld,\"running\":%u,\"dt\":%g,\"p4\":%u,\"a9\":%u,\"f98\":%u,\"f1f5\":%u,\"f1f6\":%u,\"f70\":%d,\"remo154\":%u,\"event_c\":%u,\"event_d\":%u,\"caller\":%lld}\n",
            n,running,dt,p4,a9,f98,f1f5,f1f6,f70,remo,event_c,event_d,static_cast<long long>(reinterpret_cast<uintptr_t>(_ReturnAddress())-image));Log(row);}
    world_update_gate_original(step,running,dt,p4);}
}  // namespace
