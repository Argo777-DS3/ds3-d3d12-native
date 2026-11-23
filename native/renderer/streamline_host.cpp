// I-0111 (R1): Streamline 2.14.16 in manual-hooking mode (streamline_host.h). The interposer is loaded dynamically: the module
// never links sl.interposer.lib, so a missing or unsigned interposer only leaves DLSS off.
#include "streamline_host.h"
#include <sl.h>
#include <sl_dlss.h>
#include <sl_dlss_d.h>
#include <sl_dlss_g.h>
#include <sl_nis.h>
#include <sl_reflex.h>
#include <sl_pcl.h>
#include <sl_security.h>
#include <algorithm>
#include <unordered_map>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <mutex>

namespace ds3native::streamline {
namespace {
struct Api {
    HMODULE module{};
    PFun_slInit* init{};PFun_slShutdown* shutdown{};PFun_slSetD3DDevice* set_device{};PFun_slIsFeatureSupported* supported{};
    PFun_slUpgradeInterface* upgrade{};PFun_slGetNewFrameToken* token{};PFun_slSetConstants* constants{};
    PFun_slEvaluateFeature* evaluate{};PFun_slGetFeatureFunction* feature_function{};PFun_slFreeResources* free{};
    PFun_slDLSSSetOptions* dlss_options{};PFun_slDLSSGetOptimalSettings* dlss_optimal{};
    PFun_slDLSSDSetOptions* rr_options{};PFun_slDLSSDGetOptimalSettings* rr_optimal{};   // I-0123
    PFun_slReflexSetOptions* reflex_options{};PFun_slReflexSleep* reflex_sleep{};PFun_slReflexGetState* reflex_state{};PFun_slPCLSetMarker* pcl_marker{};
    PFun_slSetTagForFrame* tag{};PFun_slDLSSGSetOptions* fg_options{};PFun_slDLSSGGetState* fg_state{};   // I-0131 R10b
    PFun_slSetFeatureLoaded* set_loaded{};   // I-0132 R12f: DLSS-G loaded and unloaded live (ProgrammingGuideDLSS_G §18)
    PFun_slNISSetOptions* nis_options{};   // I-0132 R12f: NVIDIA Image Scaling's sharpening
};
std::mutex lock;   // Initialise, SetDevice, UpgradeFactory, Shutdown and the evaluation's option state
std::mutex token_lock;   // slGetNewFrameToken's calls (engine and render threads)
Api api;
std::atomic<bool> initialised{},device_set{},dlss_supported{},evaluation_failed{},reflex_supported{},pcl_ready{},rr_supported{},rr_failed{};
std::atomic<int> reflex_mode{-1};   // the mode last sent (-1: none yet)
// I-0131 R10b: DLSS-G requested at Initialise, supported on the adapter (SetDevice), latched off after a refusal; the mode last sent
// (-1: none yet; the presenting thread's, under `lock`).
std::atomic<bool> fg_requested{},fg_supported{},fg_failed{};int fg_mode{-1};bool fg_released{};
std::atomic<bool> fg_loaded{};   // I-0132 R12f: the plugin loaded now (SetFrameGenerationLoaded; all requested features load at slInit)
std::atomic<bool> nis_supported{},nis_failed{};float nis_sharpness{-1.f};   // I-0132 R12f: NIS on the adapter, latched off; the sharpness last sent (under `lock`)
std::atomic<uint32_t> fg_multiplier{2};uint32_t fg_generate{};   // the multiplier asked; the generated frames last sent (under `lock`)
bool reflex_adapter{};uint32_t latency_attempts{};   // under `lock`
// Final review M1: Reflex given up for the session (no Reflex on the adapter once PCL answered, its functions unanswered after 600
// tries, or its options refused): SetReflexMode then returns at once, without the lock EvaluateDlss holds on the worker.
std::atomic<bool> reflex_given_up{};
std::atomic<int> low_latency{-1};   // slReflexGetState's lowLatencyAvailable, read when the options are accepted
// I-0111 tranche 3 (final review H1, M2): test-dlss-refuse.flag beside the module refuses the 600th evaluation once, so the latch
// path (DLAA and the scaled modes) can be witnessed; 0 without the flag.
uint64_t test_refusal_at{},evaluations{};
void (*log_sink)(const char*){};
std::wstring log_folder;
struct Options {uint32_t output_width{},output_height{};DlssMode mode{};bool exposure{},set{};} options,rr_options;   // I-0123: RR's own
// I-0140 tranche 40 (D-0305): the menu views' viewports' options (under `lock`) and their latch.
std::unordered_map<uint32_t,Options> menu_options;std::atomic<bool> menu_failed{};
const char* ResultName(sl::Result r) {
    switch(r){case sl::Result::eOk:return "ok";case sl::Result::eErrorIO:return "io";case sl::Result::eErrorDriverOutOfDate:return "driver_out_of_date";
        case sl::Result::eErrorOSOutOfDate:return "os_out_of_date";case sl::Result::eErrorOSDisabledHWS:return "hardware_scheduling_off";
        case sl::Result::eErrorDeviceNotCreated:return "device_not_created";case sl::Result::eErrorNoSupportedAdapterFound:return "no_supported_adapter";
        case sl::Result::eErrorAdapterNotSupported:return "adapter_not_supported";case sl::Result::eErrorNoPlugins:return "no_plugins";
        case sl::Result::eErrorNGXFailed:return "ngx_failed";case sl::Result::eErrorMissingResourceState:return "missing_resource_state";
        case sl::Result::eErrorInvalidIntegration:return "invalid_integration";case sl::Result::eErrorMissingInputParameter:return "missing_input";
        case sl::Result::eErrorNotInitialized:return "not_initialised";case sl::Result::eErrorInvalidParameter:return "invalid_parameter";
        case sl::Result::eErrorMissingConstants:return "missing_constants";case sl::Result::eErrorFeatureMissing:return "feature_missing";
        case sl::Result::eErrorFeatureNotSupported:return "feature_not_supported";case sl::Result::eErrorFeatureFailedToLoad:return "feature_failed_to_load";
        case sl::Result::eErrorInvalidState:return "invalid_state";case sl::Result::eWarnOutOfVRAM:return "out_of_vram";
        default:return "error";}
}
std::string Failed(const char* call,sl::Result r) {char text[160]{};sprintf_s(text,"%s: %s (%d)",call,ResultName(r),int(r));return text;}
std::atomic<uint64_t> warnings{},errors{};
void OnMessage(sl::LogType type,const char* message) {
    if(type==sl::LogType::eInfo || !log_sink || !message)return;
    // The first 32 of each type, then one in 600 (step review: errors keep their own budget; `n` shows what was left out).
    const auto n=(type==sl::LogType::eWarn?warnings:errors).fetch_add(1,std::memory_order_relaxed)+1;if(n>32 && n%600)return;
    char row[640]{};std::string text(message,strnlen(message,480));
    for(auto& c:text)if(c=='"' || c=='\\' || c<32)c='\'';
    sprintf_s(row,"{\"event\":\"native_streamline_message\",\"type\":\"%s\",\"n\":%llu,\"text\":\"%s\"}\n",type==sl::LogType::eWarn?"warn":"error",
        static_cast<unsigned long long>(n),text.c_str());
    log_sink(row);
}
sl::DLSSMode SlMode(DlssMode mode) noexcept {
    switch(mode){case DlssMode::Quality:return sl::DLSSMode::eMaxQuality;case DlssMode::Balanced:return sl::DLSSMode::eBalanced;
        case DlssMode::Performance:return sl::DLSSMode::eMaxPerformance;case DlssMode::UltraPerformance:return sl::DLSSMode::eUltraPerformance;
        default:return sl::DLSSMode::eDLAA;}
}
// A feature function of DLSS (I-0123: or of Ray Reconstruction), asked once (it exists once the plugin has the device and a swap
// chain: i111-dlss-1, asked right after slInit, « 'kFeatureDLSS' has not been initialized yet »). Under `lock`.
template<class T> bool FeatureFunction(T*& out,const char* name,std::string& reason,sl::Feature feature=sl::kFeatureDLSS) {
    if(out)return true;void* function{};
    if(const auto r=api.feature_function(feature,name,function);r!=sl::Result::eOk || !function){reason=Failed((std::string("slGetFeatureFunction(")+name+")").c_str(),r);return false;}
    out=reinterpret_cast<T*>(function);return true;
}
template<class T> bool Resolve(T*& out,const char* name) {out=reinterpret_cast<T*>(GetProcAddress(api.module,name));return out!=nullptr;}
sl::float4x4 Rows(const Matrix4& m) {
    sl::float4x4 r;for(uint32_t i=0;i<4;++i)r.setRow(i,sl::float4(m[i*4],m[i*4+1],m[i*4+2],m[i*4+3]));return r;
}
}
bool Initialise(const std::wstring& folder,void (*sink)(const char*),std::string& reason,bool frame_generation) noexcept {
    const std::lock_guard hold(lock);
    if(initialised)return true;
    try{
        const std::wstring path=folder+L"\\sl.interposer.dll";
        if(GetFileAttributesW(path.c_str())==INVALID_FILE_ATTRIBUTES){reason="sl.interposer.dll absent beside the module";return false;}
        // Security audit L2 (2026-11-23): the interposer held open without write or delete sharing from before its signature check to
        // after slInit, so the file itself cannot be replaced, renamed or deleted between the check and the load. Out of reach, as the
        // module itself is (game_bootstrap.dll is unsigned): a process that can write the module's folder or a junction above it (it
        // can re-point the path between check and load, write the file in place, or swap a plugin, which Streamline checks and loads by
        // name). No comparison after the load: the build folders' interposers are hard links of one file, whose mapped name is the
        // first link opened, and the checks tried (by name, by final path) refused good loads or reopened the path.
        const HANDLE held=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(held==INVALID_HANDLE_VALUE){reason="sl.interposer.dll cannot be held for its check";return false;}
        struct Close {HANDLE h;~Close(){CloseHandle(h);}} const close{held};
        if(!sl::security::verifyEmbeddedSignature(path.c_str())){reason="sl.interposer.dll not signed by NVIDIA";return false;}
        // Its own folder first for the libraries it loads (its plugins come from pathsToPlugins below).
        api={};api.module=LoadLibraryExW(path.c_str(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);
        if(!api.module){char text[96]{};sprintf_s(text,"sl.interposer.dll load failed (%lu)",GetLastError());reason=text;return false;}
        if(!Resolve(api.init,"slInit") || !Resolve(api.shutdown,"slShutdown") || !Resolve(api.set_device,"slSetD3DDevice") ||
           !Resolve(api.supported,"slIsFeatureSupported") || !Resolve(api.upgrade,"slUpgradeInterface") || !Resolve(api.token,"slGetNewFrameToken") ||
           !Resolve(api.constants,"slSetConstants") || !Resolve(api.evaluate,"slEvaluateFeature") ||
           !Resolve(api.feature_function,"slGetFeatureFunction") || !Resolve(api.free,"slFreeResources") ||
           (frame_generation && (!Resolve(api.tag,"slSetTagForFrame") || !Resolve(api.set_loaded,"slSetFeatureLoaded")))){
            FreeLibrary(api.module);api={};reason="sl.interposer.dll exports";return false;}
        log_sink=sink;log_folder=folder;
        test_refusal_at=GetFileAttributesW((folder+L"/test-dlss-refuse.flag").c_str())!=INVALID_FILE_ATTRIBUTES?600:0;evaluations=0;
        const wchar_t* plugins[]{folder.c_str()};
        // I-0123: RR; I-0131 R10b: DLSS-G last, loaded only when asked.
        // I-0132 R12f: NIS (sl.nis, the SDK's) for the sharpening, before DLSS-G (which stays last: loaded only when asked or offered).
        const sl::Feature features[]{sl::kFeatureDLSS,sl::kFeatureDLSS_RR,sl::kFeatureReflex,sl::kFeaturePCL,sl::kFeatureNIS,sl::kFeatureDLSS_G};
        sl::Preferences preferences{};
        preferences.showConsole=false;preferences.logLevel=sl::LogLevel::eDefault;
        preferences.pathsToPlugins=plugins;preferences.numPathsToPlugins=1;preferences.pathToLogsAndData=log_folder.c_str();
        preferences.logMessageCallback=OnMessage;
        // No over-the-air update or downloaded plugin of Streamline (the defaults allow both); NGX may still take the NVIDIA App's
        // DLSS override (streamline_host.h).
        preferences.flags=sl::PreferenceFlags::eDisableCLStateTracking|sl::PreferenceFlags::eUseManualHooking|sl::PreferenceFlags::eDisableDebugText;
        if(frame_generation)preferences.flags|=sl::PreferenceFlags::eUseFrameBasedResourceTagging;   // I-0131 R10b: slSetTagForFrame's global tags
        preferences.featuresToLoad=features;preferences.numFeaturesToLoad=uint32_t(std::size(features))-(frame_generation?0:1);
        preferences.engine=sl::EngineType::eCustom;preferences.engineVersion="DS3 Rekindled native D3D12";
        preferences.projectId="5f2c9a71-4e0b-4d36-9a8e-3b1d7c64e2a9";   // the project's own identifier (no NVIDIA application id)
        preferences.renderAPI=sl::RenderAPI::eD3D12;
        if(const auto r=api.init(preferences,sl::kSDKVersion);r!=sl::Result::eOk){reason=Failed("slInit",r);FreeLibrary(api.module);api={};return false;}
        options={};rr_options={};menu_options.clear();menu_failed=false;evaluation_failed=false;rr_failed=false;fg_requested=frame_generation;fg_supported=false;fg_loaded=false;fg_failed=false;fg_mode=-1;fg_released=false;
        initialised=true;return true;
    }catch(...){reason="Streamline initialisation: exception";return false;}
}
bool Initialised() noexcept {return initialised;}
bool SetDevice(ID3D12Device* device,const LUID& adapter,std::string& reason) noexcept {
    const std::lock_guard hold(lock);
    if(!initialised || !device){reason="Streamline not initialised";return false;}
    if(const auto r=api.set_device(device);r!=sl::Result::eOk){reason=Failed("slSetD3DDevice",r);return false;}
    device_set=true;
    LUID luid=adapter;sl::AdapterInfo info{};info.deviceLUID=reinterpret_cast<uint8_t*>(&luid);info.deviceLUIDSizeInBytes=sizeof(luid);
    // Reflex (I-0111 tranche 2): whether the adapter offers it; its functions and PCL's are asked later (SetReflexMode), as DLSS's,
    // once the plugins have the device and a swap chain. A refusal leaves them off, DLSS unaffected.
    reflex_adapter=api.supported(sl::kFeatureReflex,info)==sl::Result::eOk;
    // I-0123: Ray Reconstruction (its plugin and nvngx_dlssd beside the module); a refusal leaves DLSS Super Resolution.
    rr_supported=api.supported(sl::kFeatureDLSS_RR,info)==sl::Result::eOk;
    nis_supported=api.supported(sl::kFeatureNIS,info)==sl::Result::eOk;nis_failed=false;nis_sharpness=-1.f;   // I-0132 R12f
    // I-0131 R10b: DLSS-G on the adapter (its OS and scheduling requirements included: slIsFeatureSupported answers them, §1.0).
    if(fg_requested){const auto r=api.supported(sl::kFeatureDLSS_G,info);fg_supported=r==sl::Result::eOk;fg_loaded=fg_supported.load();
        if(!fg_supported && log_sink){char row[160]{};sprintf_s(row,"{\"event\":\"native_frame_generation\",\"supported\":false,\"why\":\"%s\"}\n",ResultName(r));log_sink(row);}}
    if(const auto r=api.supported(sl::kFeatureDLSS,info);r!=sl::Result::eOk){reason=Failed("slIsFeatureSupported(DLSS)",r);return false;}
    dlss_supported=true;return true;
}
bool UpgradeFactory(IDXGIFactory4* factory,IDXGIFactory4** proxy,std::string& reason) noexcept {
    const std::lock_guard hold(lock);
    if(!initialised || !factory || !proxy){reason="Streamline not initialised";return false;}
    // slUpgradeInterface replaces the pointer by its proxy, which takes its own reference on the native factory (step review L2,
    // dxgiFactory.cpp); with the interposer disabled it returns the pointer unchanged, which then gets the caller's reference.
    void* object=factory;
    if(const auto r=api.upgrade(&object);r!=sl::Result::eOk || !object){reason=Failed("slUpgradeInterface(factory)",r);return false;}
    if(object==factory)factory->AddRef();
    *proxy=static_cast<IDXGIFactory4*>(object);return true;
}
bool DlssReady() noexcept {return initialised && device_set && dlss_supported && !evaluation_failed;}
bool RrReady() noexcept {return DlssReady() && rr_supported && !rr_failed;}
bool MenuDlssReady() noexcept {return DlssReady() && !menu_failed;}
void FreeMenuDlss(uint32_t viewport) noexcept {
    const std::lock_guard hold(lock);if(viewport<2 || !menu_options.erase(viewport) || !initialised || !device_set)return;
    try{(void)api.free(sl::kFeatureDLSS,sl::ViewportHandle(viewport));}catch(...){}}
void RrDisable() noexcept {rr_failed=true;}
const char* DlssModeName(DlssMode mode) noexcept {
    switch(mode){case DlssMode::Quality:return "quality";case DlssMode::Balanced:return "balanced";case DlssMode::Performance:return "performance";
        case DlssMode::UltraPerformance:return "ultra_performance";default:return "dlaa";}
}
bool DlssRenderExtent(DlssMode mode,uint32_t output_width,uint32_t output_height,uint32_t& width,uint32_t& height,std::string& reason,bool rr) noexcept {
    if(mode==DlssMode::Dlaa){width=output_width;height=output_height;return true;}
    const std::lock_guard hold(lock);
    if(!DlssReady() || !output_width || !output_height){reason="DLSS not ready";return false;}
    if(rr && !RrReady()){reason="Ray Reconstruction not ready";return false;}
    try{
        uint32_t w{},h{};const char* call=rr?"slDLSSDGetOptimalSettings":"slDLSSGetOptimalSettings";
        if(rr){
            if(!FeatureFunction(api.rr_optimal,call,reason,sl::kFeatureDLSS_RR))return false;
            sl::DLSSDOptions o{};o.mode=SlMode(mode);o.outputWidth=output_width;o.outputHeight=output_height;o.colorBuffersHDR=sl::Boolean::eTrue;
            sl::DLSSDOptimalSettings settings{};
            if(const auto r=api.rr_optimal(o,settings);r!=sl::Result::eOk){reason=Failed(call,r);return false;}
            w=settings.optimalRenderWidth;h=settings.optimalRenderHeight;}
        else{
            if(!FeatureFunction(api.dlss_optimal,call,reason))return false;
            sl::DLSSOptions o{};o.mode=SlMode(mode);o.outputWidth=output_width;o.outputHeight=output_height;o.colorBuffersHDR=sl::Boolean::eTrue;
            sl::DLSSOptimalSettings settings{};
            if(const auto r=api.dlss_optimal(o,settings);r!=sl::Result::eOk){reason=Failed(call,r);return false;}
            w=settings.optimalRenderWidth;h=settings.optimalRenderHeight;}
        if(!w || !h || w>output_width || h>output_height){
            char text[160]{};sprintf_s(text,"%s: %ux%u for %ux%u",call,w,h,output_width,output_height);reason=text;return false;}
        width=w;height=h;return true;
    }catch(...){reason="DLSS optimal settings: exception";return false;}
}
void Disable() noexcept {evaluation_failed=true;}
const char* ReflexModeName(ReflexMode mode) noexcept {return mode==ReflexMode::On?"on":mode==ReflexMode::Boost?"boost":"off";}
bool ReflexReady() noexcept {return initialised && device_set && reflex_supported;}
namespace {
// PCL's marker and Reflex's sleep and options, asked until they answer (a few hundred frames at most). Under `lock`.
void ResolveLatency() noexcept {
    if((pcl_ready && (reflex_supported || !reflex_adapter)) || latency_attempts>=600)return;
    ++latency_attempts;
    try{
        void* marker{};if(!pcl_ready && api.feature_function(sl::kFeaturePCL,"slPCLSetMarker",marker)==sl::Result::eOk && marker){
            api.pcl_marker=reinterpret_cast<PFun_slPCLSetMarker*>(marker);pcl_ready=true;}
        void* sleep{};void* setter{};void* state{};
        if(reflex_adapter && !reflex_supported && api.feature_function(sl::kFeatureReflex,"slReflexSleep",sleep)==sl::Result::eOk && sleep &&
           api.feature_function(sl::kFeatureReflex,"slReflexSetOptions",setter)==sl::Result::eOk && setter &&
           api.feature_function(sl::kFeatureReflex,"slReflexGetState",state)==sl::Result::eOk && state){
            api.reflex_sleep=reinterpret_cast<PFun_slReflexSleep*>(sleep);api.reflex_options=reinterpret_cast<PFun_slReflexSetOptions*>(setter);
            api.reflex_state=reinterpret_cast<PFun_slReflexGetState*>(state);reflex_supported=true;}
    }catch(...){}
}
}
bool SetReflexMode(ReflexMode mode,std::string& reason) noexcept {
    if(ReflexReady() && reflex_mode==int(mode))return true;   // step review L3: no lock once sent (EvaluateDlss holds it on the worker)
    if(reflex_given_up){reason="Reflex given up";return false;}
    const std::lock_guard hold(lock);
    if(!initialised || !device_set){reason="Streamline not ready";return false;}
    ResolveLatency();
    if(!ReflexReady()){
        if((pcl_ready && !reflex_adapter) || latency_attempts>=600)reflex_given_up=true;
        reason=!reflex_adapter?"Reflex not supported on this adapter":latency_attempts>=600?"Reflex functions not answered after 600 tries":"Reflex functions not answered yet";
        return false;}
    if(reflex_mode==int(mode))return true;
    try{
        sl::ReflexOptions o{};o.mode=mode==ReflexMode::On?sl::ReflexMode::eLowLatency:mode==ReflexMode::Boost?sl::ReflexMode::eLowLatencyWithBoost:sl::ReflexMode::eOff;
        o.frameLimitUs=0;   // DS3's limiter and the display clock stay the frame's clocks (D-0240); a driver cap is a later option
        // A refusal latches once PCL answered (its marker is resolved here too, step review L-c).
        if(const auto r=api.reflex_options(o);r!=sl::Result::eOk){if(pcl_ready)reflex_given_up=true;reason=Failed("slReflexSetOptions",r);return false;}
        reflex_mode=int(mode);
    }catch(...){if(pcl_ready)reflex_given_up=true;reason="Reflex options: exception";return false;}
    // Final review M2: the driver's low-latency flag read once here (slReflexGetState makes two driver calls), not each frame; a later
    // loss (sl.reflex clears it when a driver sleep fails) is not reported (step review L-a). Its own try (step review L-b).
    int low=-1;try{if(api.reflex_state){sl::ReflexState state{};if(api.reflex_state(state)==sl::Result::eOk)low=state.lowLatencyAvailable?1:0;}}catch(...){}
    low_latency=low;return true;
}
int ReflexLowLatencyAvailable() noexcept {return ReflexReady()?low_latency.load():-1;}
namespace {
// The frame token of `frame` (one index space with DLSS's, the job serial); null when Streamline refuses.
// slGetNewFrameToken is documented not thread-safe (ProgrammingGuide): every call goes through token_lock (step review L2). Its
// tokens come from a ring of six slots; a token is used at once, but DLSS's is held between slSetConstants and slEvaluateFeature
// while the engine thread may ask others: six alternations of index in that span would overwrite it (not met: three a frame).
sl::FrameToken* Token(uint64_t frame) noexcept {
    sl::FrameToken* token{};const uint32_t index=uint32_t(frame);
    try{const std::lock_guard hold(token_lock);if(api.token(token,&index)!=sl::Result::eOk)return nullptr;}catch(...){return nullptr;}
    return token;
}
}
void ReflexSleep(uint64_t frame) noexcept {
    if(!ReflexReady() || reflex_mode<0 || !api.reflex_sleep)return;
    if(auto* token=Token(frame)){try{(void)api.reflex_sleep(*token);}catch(...){}}
}
void Marker(PclMarker marker,uint64_t frame) noexcept {
    if(!initialised || !device_set || !pcl_ready || !api.pcl_marker || !frame)return;
    static constexpr sl::PCLMarker kMarkers[]{sl::PCLMarker::eSimulationStart,sl::PCLMarker::eSimulationEnd,sl::PCLMarker::eRenderSubmitStart,
        sl::PCLMarker::eRenderSubmitEnd,sl::PCLMarker::ePresentStart,sl::PCLMarker::ePresentEnd};
    if(auto* token=Token(frame)){try{(void)api.pcl_marker(kMarkers[size_t(marker)],*token);}catch(...){}}
}
void Shutdown() noexcept {
    const std::lock_guard hold(lock);
    if(!initialised)return;
    try{
        (void)api.free(sl::kFeatureDLSS,sl::ViewportHandle(0));
        if(rr_supported)(void)api.free(sl::kFeatureDLSS_RR,sl::ViewportHandle(0));   // I-0123
        if(fg_supported && fg_loaded)(void)api.free(sl::kFeatureDLSS_G,sl::ViewportHandle(0));   // I-0131 R10b (I-0132 R12f: while loaded)
        for(const auto& [viewport,sent]:menu_options){(void)sent;(void)api.free(sl::kFeatureDLSS,sl::ViewportHandle(viewport));}   // I-0140 tranche 40
        (void)api.shutdown();
    }catch(...){}
    initialised=false;device_set=false;dlss_supported=false;options={};rr_options={};menu_options.clear();menu_failed=false;rr_supported=false;rr_failed=false;reflex_supported=false;pcl_ready=false;reflex_mode=-1;reflex_adapter=false;latency_attempts=0;
    fg_requested=false;fg_supported=false;fg_loaded=false;fg_failed=false;fg_mode=-1;fg_released=false;api.tag=nullptr;api.fg_options=nullptr;api.fg_state=nullptr;api.set_loaded=nullptr;   // I-0131 R10b
    nis_supported=false;nis_failed=false;nis_sharpness=-1.f;api.nis_options=nullptr;   // I-0132 R12f
    api.reflex_options=nullptr;api.reflex_sleep=nullptr;api.reflex_state=nullptr;api.pcl_marker=nullptr;reflex_given_up=false;low_latency=-1;
}
namespace {
bool Evaluate(ID3D12GraphicsCommandList* list,const DlssFrame& f,std::string& reason) noexcept {
    if(!DlssReady() || !list || !f.color || !f.depth || !f.motion || !f.output || !f.render_width || !f.render_height || !f.output_width || !f.output_height){
        reason="DLSS frame incomplete";return false;}
    if(f.rr && (!RrReady() || !f.albedo || !f.specular_albedo || !f.normal_roughness)){reason="Ray Reconstruction frame incomplete";return false;}
    // I-0140 tranche 40: a menu view's viewport (2 and up) runs DLAA alone, without Ray Reconstruction or the world's exposure.
    if(f.viewport && (f.viewport<2 || f.rr || f.exposure || f.mode!=DlssMode::Dlaa || menu_failed)){reason="menu DLSS frame invalid";return false;}
    try{
        const sl::ViewportHandle viewport(f.viewport);
        sl::FrameToken* token{};const uint32_t index=f.frame;
        {const std::lock_guard tokens(token_lock);
            if(const auto r=api.token(token,&index);r!=sl::Result::eOk || !token){reason=Failed("slGetNewFrameToken",r);return false;}}
        const bool exposure=f.exposure!=nullptr && !f.rr;
        // I-0123: Ray Reconstruction keeps its own options (its plugin, its instance): a switch between the two restarts the history.
        auto& sent=f.rr?rr_options:f.viewport?menu_options[f.viewport]:options;   // I-0140 tranche 40: a menu view's own
        const bool changed=!sent.set || sent.output_width!=f.output_width || sent.output_height!=f.output_height || sent.mode!=f.mode;
        // Step review L4: RR's options every RR frame (the camera's rows change each frame; the plugin rebuilds its instance only on a
        // mode, size, preset or roughness change).
        if(f.rr){
            if(!FeatureFunction(api.rr_options,"slDLSSDSetOptions",reason,sl::kFeatureDLSS_RR))return false;
            // RR-PLAN §2 (its guide §3.13): HDR, the roughness packed in the normal's w, the camera's
            // world-to-view rows for the specular hit distance R6 will give (no tag reads them before).
            sl::DLSSDOptions o{};
            o.mode=SlMode(f.mode);o.outputWidth=f.output_width;o.outputHeight=f.output_height;o.colorBuffersHDR=sl::Boolean::eTrue;
            o.normalRoughnessMode=sl::DLSSDNormalRoughnessMode::ePacked;
            // Preset F in every mode (Thomas, 2026-11-11 07:25: « Preset F pour RR »): Streamline 2.14.16's latest transformer model, the
            // default of DLSS 310.9.16's RR, written so a later DLL's default does not change it.
            o.dlaaPreset=o.qualityPreset=o.balancedPreset=o.performancePreset=o.ultraPerformancePreset=o.ultraQualityPreset=sl::DLSSDPreset::ePresetF;
            o.worldToCameraView=Rows(f.world_to_view);o.cameraViewToWorld=Rows(f.view_to_world);
            if(const auto r=api.rr_options(viewport,o);r!=sl::Result::eOk){reason=Failed("slDLSSDSetOptions",r);return false;}
            rr_options={f.output_width,f.output_height,f.mode,false,true};}
        // Step review M1: sl.dlss reads useAutoExposure and the exposure tag only when it rebuilds its instance (a mode, size or
        // preset change), so the frame always gives the exposure (1 without a tone-map input, scene_pass_frame.cpp); presence changes
        // only if the texture's creation fails (latched), when the options are sent again though DLSS keeps its first choice.
        const bool exposure_changed=!f.rr && sent.set && sent.exposure!=exposure;
        if(!f.rr && (changed || exposure_changed)){
            // Asked at the first evaluation (FeatureFunction). NVIDIA's default preset for the mode (Thomas, 2026-10-30 08:17).
            if(!FeatureFunction(api.dlss_options,"slDLSSSetOptions",reason))return false;
            sl::DLSSOptions o{};
            o.mode=SlMode(f.mode);o.outputWidth=f.output_width;o.outputHeight=f.output_height;
            // The lit HDR is linear and not pre-exposed (YEBIS applies the exposure at its tone map): DS3's exposure when the frame
            // gives it (D-0242), else DLSS's own auto-exposure.
            o.colorBuffersHDR=sl::Boolean::eTrue;o.useAutoExposure=exposure?sl::Boolean::eFalse:sl::Boolean::eTrue;
            if(const auto r=api.dlss_options(viewport,o);r!=sl::Result::eOk){reason=Failed("slDLSSSetOptions",r);return false;}
            sent={f.output_width,f.output_height,f.mode,exposure,true};}
        sl::Constants c{};
        c.cameraViewToClip=Rows(f.view_to_clip);c.clipToCameraView=Rows(f.clip_to_view);
        Matrix4 identity{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};c.clipToLensClip=Rows(identity);
        c.clipToPrevClip=Rows(f.clip_to_previous_clip);c.prevClipToClip=Rows(f.previous_clip_to_clip);
        c.jitterOffset={f.jitter_x,f.jitter_y};
        c.mvecScale={1.0f/float(f.render_width),1.0f/float(f.render_height)};   // the vectors are in render pixels
        c.cameraPinholeOffset={0,0};
        c.cameraPos={f.position[0],f.position[1],f.position[2]};c.cameraUp={f.up[0],f.up[1],f.up[2]};
        c.cameraRight={f.right[0],f.right[1],f.right[2]};c.cameraFwd={f.forward[0],f.forward[1],f.forward[2]};
        c.cameraNear=f.near_plane;c.cameraFar=f.far_plane;c.cameraFOV=f.vertical_fov;c.cameraAspectRatio=f.aspect;
        c.depthInverted=sl::Boolean::eFalse;c.cameraMotionIncluded=sl::Boolean::eTrue;c.motionVectors3D=sl::Boolean::eFalse;
        c.reset=f.reset || changed?sl::Boolean::eTrue:sl::Boolean::eFalse;   // a new extent or mode restarts the history too
        c.orthographicProjection=sl::Boolean::eFalse;
        c.motionVectorsDilated=sl::Boolean::eFalse;c.motionVectorsJittered=sl::Boolean::eFalse;
        if(const auto r=api.constants(c,*token,viewport);r!=sl::Result::eOk){reason=Failed("slSetConstants",r);return false;}
        sl::Resource color(sl::ResourceType::eTex2d,f.color,uint32_t(f.color_state)),depth(sl::ResourceType::eTex2d,f.depth,uint32_t(f.depth_state));
        sl::Resource motion(sl::ResourceType::eTex2d,f.motion,uint32_t(f.motion_state)),output(sl::ResourceType::eTex2d,f.output,uint32_t(f.output_state));
        sl::Resource exposure_texture(sl::ResourceType::eTex2d,f.exposure,uint32_t(f.exposure_state));
        // I-0123: Ray Reconstruction's guides (RR-PLAN §1), at the render extent.
        sl::Resource albedo(sl::ResourceType::eTex2d,f.albedo,uint32_t(f.guide_state)),specular(sl::ResourceType::eTex2d,f.specular_albedo,uint32_t(f.guide_state));
        sl::Resource normal(sl::ResourceType::eTex2d,f.normal_roughness,uint32_t(f.guide_state)),before(sl::ResourceType::eTex2d,f.before_transparency,uint32_t(f.guide_state));
        sl::Resource hit(sl::ResourceType::eTex2d,f.specular_hit_distance,uint32_t(f.guide_state));   // I-0125 R6c (RR guide §4.1.9)
        const sl::Extent render{0,0,f.render_width,f.render_height},full{0,0,f.output_width,f.output_height},one{0,0,1,1};
        sl::ResourceTag tags[]{
            sl::ResourceTag(&color,sl::kBufferTypeScalingInputColor,sl::ResourceLifecycle::eValidUntilEvaluate,&render),
            sl::ResourceTag(&output,sl::kBufferTypeScalingOutputColor,sl::ResourceLifecycle::eValidUntilEvaluate,&full),
            sl::ResourceTag(&depth,sl::kBufferTypeDepth,sl::ResourceLifecycle::eValidUntilEvaluate,&render),
            sl::ResourceTag(&motion,sl::kBufferTypeMotionVectors,sl::ResourceLifecycle::eValidUntilEvaluate,&render),
            sl::ResourceTag(&exposure_texture,sl::kBufferTypeExposure,sl::ResourceLifecycle::eValidUntilEvaluate,&one),
            sl::ResourceTag(&albedo,sl::kBufferTypeAlbedo,sl::ResourceLifecycle::eValidUntilEvaluate,&render),
            sl::ResourceTag(&specular,sl::kBufferTypeSpecularAlbedo,sl::ResourceLifecycle::eValidUntilEvaluate,&render),
            sl::ResourceTag(&normal,sl::kBufferTypeNormalRoughness,sl::ResourceLifecycle::eValidUntilEvaluate,&render),
            sl::ResourceTag(&before,sl::kBufferTypeColorBeforeTransparency,sl::ResourceLifecycle::eValidUntilEvaluate,&render),
            sl::ResourceTag(&hit,sl::kBufferTypeSpecularHitDistance,sl::ResourceLifecycle::eValidUntilEvaluate,&render)};
        // The tags travel with the evaluation (nothing is copied for a later use); the exposure only when the frame has it. No
        // current-colour bias (I-0111 tranche 4's reactive mask, removed by I-0116 tranche 2: DLSS 310.9's models other than the
        // deprecated preset F do not read it, series 5).
        const sl::BaseStructure* inputs[10]{&viewport,&tags[0],&tags[1],&tags[2],&tags[3]};uint32_t count=5;
        if(f.rr){inputs[count++]=&tags[5];inputs[count++]=&tags[6];inputs[count++]=&tags[7];if(f.before_transparency)inputs[count++]=&tags[8];
            if(f.specular_hit_distance)inputs[count++]=&tags[9];}
        else if(f.exposure)inputs[count++]=&tags[4];
        const auto feature=f.rr?sl::kFeatureDLSS_RR:sl::kFeatureDLSS;
        if(const auto r=api.evaluate(feature,*token,inputs,count,list);r!=sl::Result::eOk){
            reason=Failed(f.rr?"slEvaluateFeature(DLSS_RR)":"slEvaluateFeature(DLSS)",r);return false;}
        return true;
    }catch(...){reason="DLSS evaluation: exception";return false;}
}
}
// I-0132 R12f (Thomas, 2026-11-11 03:53: NVIDIA's NIS for the sharpening): NVSharpen, NIS's sharpen-only mode, on viewport 1 (its own
// options; DLSS keeps viewport 0), the frame's colour already tone mapped (8-bit, no HDR mode): `input` read, `output` written (unordered
// access) at width x height. Its common constants given each evaluation (identity matrices: NIS reads none of them, Streamline asks for
// the frame's constants of every evaluated viewport). Any refusal latches NIS off for the session (our own sharpening then, the caller).
bool NisReady() noexcept {return initialised && device_set && nis_supported && !nis_failed;}
void NisDisable() noexcept {nis_failed=true;}
// Its step review's H1 (ProgrammingGuideNIS §3: tagged resources destroyed, slFreeResources): Streamline caches its views by the
// resource's address, so before the targets are made again (a new extent) the cache of viewport 1 is dropped. Its guide asks that no
// list with a pending NIS evaluation be in flight then (ProgrammingGuide): held because the presentation's extent changes only through
// SceneRenderer::Resize, which waits for every slot and drains the queue before the next frame makes the targets (the final review's L3).
void NisFreeResources() noexcept {
    const std::lock_guard hold(lock);if(!initialised || !device_set || !nis_supported)return;
    try{(void)api.free(sl::kFeatureNIS,sl::ViewportHandle(1));}catch(...){}nis_sharpness=-1.f;}
bool EvaluateNisSharpen(ID3D12GraphicsCommandList* list,uint64_t frame,ID3D12Resource* input,uint32_t input_state,ID3D12Resource* output,uint32_t output_state,
    uint32_t width,uint32_t height,float sharpness,std::string& reason) noexcept {
    const std::lock_guard hold(lock);
    if(!NisReady() || !list || !input || !output || !width || !height){reason="NIS not ready";return false;}
    const bool done=[&]{
        try{
            const sl::ViewportHandle viewport(1);
            const float s=std::clamp(sharpness,0.0f,1.0f);
            if(s!=nis_sharpness){
                if(!FeatureFunction(api.nis_options,"slNISSetOptions",reason,sl::kFeatureNIS))return false;
                sl::NISOptions o{};o.mode=sl::NISMode::eSharpen;o.hdrMode=sl::NISHDR::eNone;o.sharpness=s;
                if(const auto r=api.nis_options(viewport,o);r!=sl::Result::eOk){reason=Failed("slNISSetOptions",r);return false;}
                nis_sharpness=s;}
            sl::FrameToken* token{};const uint32_t index=uint32_t(frame);
            {const std::lock_guard tokens(token_lock);
                if(const auto r=api.token(token,&index);r!=sl::Result::eOk || !token){reason=Failed("slGetNewFrameToken",r);return false;}}
            Matrix4 identity{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
            sl::Constants c{};c.cameraViewToClip=c.clipToCameraView=c.clipToLensClip=c.clipToPrevClip=c.prevClipToClip=Rows(identity);
            c.jitterOffset={0,0};c.mvecScale={1,1};c.cameraPinholeOffset={0,0};c.cameraPos={0,0,0};c.cameraUp={0,1,0};c.cameraRight={1,0,0};c.cameraFwd={0,0,1};
            c.cameraNear=0.1f;c.cameraFar=1000.0f;c.cameraFOV=1.0f;c.cameraAspectRatio=float(width)/float(height);
            c.depthInverted=c.cameraMotionIncluded=c.motionVectors3D=c.reset=c.orthographicProjection=c.motionVectorsDilated=c.motionVectorsJittered=sl::Boolean::eFalse;
            if(const auto r=api.constants(c,*token,viewport);r!=sl::Result::eOk){reason=Failed("slSetConstants(NIS)",r);return false;}
            sl::Resource in(sl::ResourceType::eTex2d,input,input_state),out(sl::ResourceType::eTex2d,output,output_state);
            const sl::Extent full{0,0,width,height};
            sl::ResourceTag tags[]{sl::ResourceTag(&in,sl::kBufferTypeScalingInputColor,sl::ResourceLifecycle::eValidUntilEvaluate,&full),
                sl::ResourceTag(&out,sl::kBufferTypeScalingOutputColor,sl::ResourceLifecycle::eValidUntilEvaluate,&full)};
            const sl::BaseStructure* inputs[]{&viewport,&tags[0],&tags[1]};
            if(const auto r=api.evaluate(sl::kFeatureNIS,*token,inputs,3,list);r!=sl::Result::eOk){reason=Failed("slEvaluateFeature(NIS)",r);return false;}
            return true;
        }catch(...){reason="NIS evaluation: exception";return false;}}();
    if(!done)nis_failed=true;
    return done;
}
// Any refusal of the world's evaluation latches DLSS off for the session (DlssReady false: the game thread stops the jitter from its
// next frame); a menu view's (viewport 2 and up) latches only the menu views' DLSS off (MenuDlssReady false; I-0140 tranche 40).
bool EvaluateDlss(ID3D12GraphicsCommandList* list,const DlssFrame& f,std::string& reason) noexcept {
    const std::lock_guard hold(lock);
    const bool refused=!f.viewport && test_refusal_at && ++evaluations==test_refusal_at;if(refused)reason="test refusal (test-dlss-refuse.flag)";   // I-0140 tranche 40: the world's evaluations counted
    const bool evaluated=!refused && Evaluate(list,f,reason);
    // I-0123: RR's refusal leaves DLSS ready; I-0140 tranche 40: a menu view's latches only the menu views' DLSS off.
    if(!evaluated){if(f.viewport)menu_failed=true;else if(f.rr && !refused)rr_failed=true;else evaluation_failed=true;}
    return evaluated;
}
// I-0131 R10b (R10-PLAN §3).
bool FgLoaded() noexcept {return initialised && device_set && fg_supported && fg_loaded;}
bool FgSupported() noexcept {return initialised && device_set && fg_supported;}
// I-0132 R12f: DLSS-G loaded or unloaded with no chain alive (the caller released it, the queue drained; slSetFeatureLoaded is not
// thread safe and needs the device, sl_core_api.h): its functions asked again once loaded, its mode unknown.
bool SetFrameGenerationLoaded(bool loaded,std::string& reason) noexcept {
    const std::lock_guard hold(lock);
    if(!initialised || !device_set || !fg_supported){reason="frame generation not supported";return false;}
    if(fg_loaded==loaded)return true;
    if(!api.set_loaded){reason="slSetFeatureLoaded absent";return false;}
    try{
        if(const auto r=api.set_loaded(sl::kFeatureDLSS_G,loaded);r!=sl::Result::eOk){reason=Failed("slSetFeatureLoaded(DLSS_G)",r);return false;}
        fg_loaded=loaded;fg_mode=-1;fg_released=false;api.fg_options=nullptr;api.fg_state=nullptr;return true;
    }catch(...){reason="slSetFeatureLoaded: exception";return false;}
}
bool FgReady() noexcept {return FgLoaded() && !fg_failed && DlssReady();}
void FgDisable() noexcept {fg_failed=true;}
namespace {
// DLSS-G's two functions, asked once the plugin has the device and a chain (as DLSS's). Under `lock`.
bool FgFunctions(std::string& reason) noexcept {
    if(api.fg_options && api.fg_state)return true;
    void* setter{};void* state{};
    if(const auto r=api.feature_function(sl::kFeatureDLSS_G,"slDLSSGSetOptions",setter);r!=sl::Result::eOk || !setter){reason=Failed("slGetFeatureFunction(slDLSSGSetOptions)",r);return false;}
    if(const auto r=api.feature_function(sl::kFeatureDLSS_G,"slDLSSGGetState",state);r!=sl::Result::eOk || !state){reason=Failed("slGetFeatureFunction(slDLSSGGetState)",r);return false;}
    api.fg_options=reinterpret_cast<PFun_slDLSSGSetOptions*>(setter);api.fg_state=reinterpret_cast<PFun_slDLSSGGetState*>(state);return true;
}
// The tags of frame `frame` (its token: DLSS's, the same index), on `list` (eOnlyValidNow: Streamline records its copies there).
bool Tag(ID3D12GraphicsCommandList* list,uint32_t frame,const sl::ResourceTag* tags,uint32_t count,std::string& reason) noexcept {
    if(!FgReady() || !api.tag || !list){reason="frame generation not ready";return false;}
    try{
        sl::FrameToken* token{};const uint32_t index=frame;
        {const std::lock_guard tokens(token_lock);if(const auto r=api.token(token,&index);r!=sl::Result::eOk || !token){reason=Failed("slGetNewFrameToken",r);return false;}}
        if(const auto r=api.tag(*token,sl::ViewportHandle(0),tags,count,list);r!=sl::Result::eOk){reason=Failed("slSetTagForFrame",r);return false;}
        return true;
    }catch(...){reason="frame generation tags: exception";return false;}
}
}
bool TagFrameGenerationDepth(ID3D12GraphicsCommandList* list,uint32_t frame,ID3D12Resource* depth,D3D12_RESOURCE_STATES depth_state,
    ID3D12Resource* motion,D3D12_RESOURCE_STATES motion_state,uint32_t render_width,uint32_t render_height,std::string& reason) noexcept {
    if(!depth || !motion || !render_width || !render_height){reason="frame generation depth and motion incomplete";return false;}
    sl::Resource d(sl::ResourceType::eTex2d,depth,uint32_t(depth_state)),m(sl::ResourceType::eTex2d,motion,uint32_t(motion_state));
    const sl::Extent render{0,0,render_width,render_height};
    const sl::ResourceTag tags[]{sl::ResourceTag(&d,sl::kBufferTypeDepth,sl::ResourceLifecycle::eOnlyValidNow,&render),
        sl::ResourceTag(&m,sl::kBufferTypeMotionVectors,sl::ResourceLifecycle::eOnlyValidNow,&render)};
    return Tag(list,frame,tags,2,reason);
}
bool TagFrameGenerationHudless(ID3D12GraphicsCommandList* list,uint32_t frame,ID3D12Resource* hudless,D3D12_RESOURCE_STATES state,
    uint32_t width,uint32_t height,std::string& reason) noexcept {
    if(!hudless || !width || !height){reason="frame generation HUD-less colour incomplete";return false;}
    sl::Resource h(sl::ResourceType::eTex2d,hudless,uint32_t(state));const sl::Extent full{0,0,width,height};
    const sl::ResourceTag tag(&h,sl::kBufferTypeHUDLessColor,sl::ResourceLifecycle::eOnlyValidNow,&full);
    return Tag(list,frame,&tag,1,reason);
}
bool SetFrameGeneration(bool on,std::string& reason) noexcept {
    if(!FgLoaded()){reason="frame generation not loaded";return false;}
    const std::lock_guard hold(lock);
    const bool want=on && !fg_failed;
    if(fg_mode==int(want))return true;
    try{
        // The step review's L2: an Off before the functions answer has nothing to turn off (not a refusal).
        if(!FgFunctions(reason)){if(!want)return true;fg_failed=true;return false;}
        // §6.2: the multiplier's generated frames, at most the adapter's (numFramesToGenerateMax: 1 on an RTX 40, 3 on an RTX 50).
        uint32_t generate=1;if(want){sl::DLSSGState state{};const uint32_t asked=std::clamp<uint32_t>(fg_multiplier.load(),2,4)-1;
            if(api.fg_state(sl::ViewportHandle(0),state,nullptr)==sl::Result::eOk && state.numFramesToGenerateMax)generate=std::min(asked,state.numFramesToGenerateMax);}
        sl::DLSSGOptions o{};o.mode=want?sl::DLSSGMode::eOn:sl::DLSSGMode::eOff;o.numFramesToGenerate=generate;
        o.flags=sl::DLSSGFlags::eRetainResourcesWhenOff;   // §6.4: off in the menus and loads without freeing (no stutter when it returns)
        if(const auto r=api.fg_options(sl::ViewportHandle(0),o);r!=sl::Result::eOk){fg_failed=true;reason=Failed("slDLSSGSetOptions",r);return false;}
        fg_mode=int(want);if(want)fg_generate=generate;return true;
    }catch(...){fg_failed=true;reason="frame generation options: exception";return false;}
}
void ReleaseFrameGeneration() noexcept {
    if(!FgLoaded() || !fg_failed)return;
    const std::lock_guard hold(lock);
    if(fg_released || fg_mode!=0)return;
    try{(void)api.free(sl::kFeatureDLSS_G,sl::ViewportHandle(0));}catch(...){}
    fg_released=true;
}
// I-0132 R12f (FG's step review M2): a new multiplier sent at the next On (its mode marked unknown, under `lock`).
void SetFrameGenerationMultiplier(uint32_t multiplier) noexcept {
    const std::lock_guard hold(lock);const uint32_t m=std::clamp<uint32_t>(multiplier,2,4);if(m!=fg_multiplier.load()){fg_multiplier=m;fg_mode=-1;}}
bool ReflexOn() noexcept {return ReflexReady() && reflex_mode>0;}
bool FrameGenerationState(FgState& out) noexcept {
    if(!FgLoaded())return false;
    const std::lock_guard hold(lock);
    try{
        std::string reason;if(!FgFunctions(reason))return false;
        sl::DLSSGState state{};if(api.fg_state(sl::ViewportHandle(0),state,nullptr)!=sl::Result::eOk)return false;
        out.generate=fg_generate;out.presented=state.numFramesActuallyPresented;out.status=uint32_t(state.status);out.generate_max=state.numFramesToGenerateMax;
        out.min_size=state.minWidthOrHeight;out.vsync=state.bIsVsyncSupportAvailable==sl::Boolean::eTrue;return true;
    }catch(...){return false;}
}
}
