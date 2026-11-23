// I-0132 R12c (D-0271, Thomas 2026-11-10 23:06: « Vas y », the game's own pages as the base, after D-0270; R12C-STATIC §8): the
// game's Graphics tab and its Advanced Settings page keep their films (cursor, sounds, mouse, pad, help line; the tab's Defaults row) and
// get our rows through the game's own row functions, in the slots of the right kind:
// - the Graphics tab (FUN_140AB07D0, six fixed slots in the shipped film, R12C-STATIC §5; nine with the three our observer adds,
//   options_film.h): Screen Mode (Full screen > Borderless > Windowed: our three functions on the game's value row), Resolution (the
//   game's own row, its three functions), V-sync (the game's On/Off choice row, the slot Auto-detect had), the frame rate (a value
//   row), with the nine slots the HDR output and its two sliders (I-0138 tranche 5b, below), the Advanced Settings button (the game's
//   own, its functor);
// - the Advanced Settings page (AdvancedSettingsPage_BuildRows FUN_140AAFD10, fourteen slots and a scroll bar): the Quality Setting
//   preset, DS3's five rows still in use (the game's labels, choices and state bytes), then our RT rows (graphics_rows.h), each built
//   greyed and locked when D-0265's rules make it unavailable (the DLSS mode and the light bounces left out instead), built again when a row that governs others changes; its Defaults row
//   left out (fourteen slots).
// Auto-detect is no longer offered and is kept OFF (Thomas, 2026-11-10 20:40). The rows run on the game's menu task and touch bytes,
// atomics, two short copy locks (the settings, the RT snapshot, never held across a disk write) and the journal there; the settings file, the screen mode's switch and the game's config writer run on the engine frame's thread
// (owner_thread, StepRekindledGraphics), which they check. Part of game_bootstrap.cpp's single translation unit (the order is there):
// it relies on every earlier part (02_frame_lifecycle.inc before it: Log, Read, the display request).
#pragma once
#include "02_frame_lifecycle.inc"
#include "../graphics_rows.h"
#include "../options_film.h"
#include <io.h>   // _get_osfhandle (the settings file flushed to the disk)
namespace {
namespace gr=ds3native::graphics_rows;
// The settings as the page edits them (the second R12 pilot, R12c's door). A copy under a lock, from the file read at start; each edit
// writes the whole file (settings::Write) atomically: a temporary beside it, then MoveFileExW REPLACE_EXISTING | WRITE_THROUGH.
// NativeSettings() stays the start's values (the file's row); the module's readers take this copy (01_state.inc, I-0132 R12f: the
// settings live, applied at the next engine frame); the play launcher rewrites the file only before a launch. The edits are serialized
// by native_settings_write_lock (the disk), the copy (01_state.inc) read under native_settings_lock only (a reader never waits for the disk).
std::mutex native_settings_write_lock;
bool EditNativeSettings(const std::function<void(ds3native::settings::Settings&)>& edit) noexcept {
    try{
        const std::lock_guard writing(native_settings_write_lock);
        auto next=NativeSettingsCurrent();edit(next);
        if(next==NativeSettingsCurrent())return true;   // nothing to write
        // I-0132 R12f (its step reviews' L5 and L1): applied live first, whether or not the file can be written (the row says so; the next
        // edit writes the whole file again).
        {const std::lock_guard hold(native_settings_lock);NativeSettingsLiveLocked()=next;}
        const std::string text=ds3native::settings::Write(next);
        const auto refused=[&]{char row[96]{};sprintf_s(row,"{\"event\":\"native_settings_written\",\"ok\":false,\"bytes\":%zu}\n",text.size());Log(row);return false;};
        const auto path=ModuleSibling(L"rekindled-settings.ini");if(path.empty())return refused();
        const std::wstring temporary=path+L".tmp";
        FILE* file{};if(_wfopen_s(&file,temporary.c_str(),L"wb") || !file)return refused();
        const bool written=fwrite(text.data(),1,text.size(),file)==text.size();
        const bool flushed=fflush(file)==0 && FlushFileBuffers(reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(file))));
        const bool closed=fclose(file)==0 && flushed;
        bool moved=written && closed && MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
        if(!moved)DeleteFileW(temporary.c_str());
        char row[96]{};sprintf_s(row,"{\"event\":\"native_settings_written\",\"ok\":%s,\"bytes\":%zu}\n",moved?"true":"false",text.size());Log(row);
        return moved;}
    catch(...){return false;}}

// The menu text reader FUN_140E1A060(repository, id) -> const wchar_t*: our ids answered in the game's language.
using MenuTextFn=const wchar_t*(__fastcall*)(uintptr_t,int32_t);MenuTextFn menu_text_original{};
wchar_t custom_rate_text[16]{};
const wchar_t* __fastcall RekindledMenuText(uintptr_t repository,int32_t id) noexcept {
    if(!gr::Ours(id))return menu_text_original(repository,id);
    if(id==gr::kFrameRateCustom)return gr::CustomRateText(NativeSettingsCurrent().frame_rate,custom_rate_text);
    int32_t ordinal{};Read(image+0x479BA10,&ordinal,4);
    const wchar_t* text=gr::Text(id,gr::LanguageOf(ordinal));
    return text?text:L"";}

// The game's helpers (R12C-STATIC §1.1, §8), RVAs.
constexpr uintptr_t kTextOfId=0xA67060,kStringCopy=0x2B50,kStringLiteral=0x7E060,kTextAllocator=0xA67240,kListAppend=0xA9B560,
    kEntryDestroy=0xA95570,kRowLabel=0xAB6E50,kLabelDestroy=0xA49F80,kFunctorRow=0xAB1C20,kEmptyText=0x3D1DA80,
    kOnOffRow=0xAA40F0,kOnOffChoices=0xAAB920,kOnOffEntryDestroy=0xAA6360,kButtonRow=0xA8F920,kAdvancedRow=0xAA3D30,kGraphicsDefaults=0xC20E20,
    kResolutionRead=0x28C7AE8,kResolutionWrite=0x28C7B28,kResolutionList=0x28C7B68,kAdvancedButton=0x28C7C68;
// A game string (0x30 bytes: a 16-byte buffer, size, capacity, allocator, one more word) freed as the game frees it.
void FreeGameString(uintptr_t string) noexcept {
    const uint64_t capacity=*reinterpret_cast<uint64_t*>(string+0x18);
    if(capacity>=8){const uintptr_t allocator=*reinterpret_cast<uintptr_t*>(string+0x20);
        reinterpret_cast<void(__fastcall*)(uintptr_t,void*)>(*reinterpret_cast<uintptr_t*>(*reinterpret_cast<uintptr_t*>(allocator)+0x68))(allocator,*reinterpret_cast<void**>(string));}
    *reinterpret_cast<uint64_t*>(string+0x18)=7;*reinterpret_cast<uint64_t*>(string+0x10)=0;*reinterpret_cast<uint16_t*>(string)=0;}
// A choice list (the game's: 0x78-byte entries, at most 32, the count at +0xF08), one entry made as FUN_140A9A220 makes its own and
// appended (FUN_140A9B560 copies it).
uint64_t& ChoiceCount(uintptr_t list) noexcept {return *reinterpret_cast<uint64_t*>(list+0xF08);}
void AppendChoice(uintptr_t list,uint8_t value,int text_id) noexcept {
    alignas(16) uint8_t text[0x40]{};alignas(16) uint8_t entry[0x80]{};
    const auto out=reinterpret_cast<uintptr_t>(reinterpret_cast<void*(__fastcall*)(void*,int)>(image+kTextOfId)(text,text_id));
    entry[0]=value;*reinterpret_cast<uint64_t*>(entry+8)=*reinterpret_cast<uint64_t*>(out);
    reinterpret_cast<void(__fastcall*)(void*,const void*)>(image+kStringCopy)(entry+0x10,reinterpret_cast<void*>(out+8));
    *reinterpret_cast<uint64_t*>(entry+0x40)=0;
    reinterpret_cast<void(__fastcall*)(void*,const void*,void*)>(image+kStringLiteral)(entry+0x48,reinterpret_cast<void*>(image+kEmptyText),
        reinterpret_cast<void*(__fastcall*)()>(image+kTextAllocator)());
    reinterpret_cast<void(__fastcall*)(uintptr_t,const void*)>(image+kListAppend)(list,entry);
    reinterpret_cast<void(__fastcall*)(void*)>(image+kEntryDestroy)(entry);
    FreeGameString(out+8);}
// A choice list on the stack, its entries destroyed as the game's builder destroys them (FUN_140A95570 each).
struct ChoiceList {
    alignas(16) uint8_t bytes[0xF10]{};
    uintptr_t Get() noexcept {return reinterpret_cast<uintptr_t>(bytes);}
    ~ChoiceList(){const uint64_t n=(std::min)(ChoiceCount(Get()),uint64_t(32));for(uint64_t i=0;i<n;++i)reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+kEntryDestroy)(Get()+i*0x78);}};
// A row label (the label's text, then its help at id + 100000, FUN_140AB6E50), destroyed once the row has copied it.
struct RowLabel {
    alignas(16) uint8_t bytes[0x80]{};
    explicit RowLabel(int id) noexcept {reinterpret_cast<void*(__fastcall*)(void*,int)>(image+kRowLabel)(bytes,id);}
    ~RowLabel(){reinterpret_cast<void(__fastcall*)(void*)>(image+kLabelDestroy)(bytes);}
    RowLabel(const RowLabel&)=delete;RowLabel& operator=(const RowLabel&)=delete;};

// A std::function of the game's (0x20 bytes, the impl pointer at +0x18, as the game's own copies show; the impl's vtable: copy, move,
// do_call, target type, delete_this, destructor, get). Ours are made in place (their impl pointer their own space, so that the game
// copies them in place: FUN_140AA4A10 passes `where` only then), hold at most two words after their vtable, and are never freed. The
// game's own (Resolution's three, the Advanced button's, R12C-STATIC §8.1) are made the same way with the game's vtables.
struct GameCallable {const void* const* vtable;uintptr_t word[2];};
struct GameFunction {void* space[3];void* impl;};
void* __fastcall CallableCopy(GameCallable* self,void* where) noexcept {
    if(!where)return self;
    *static_cast<GameCallable*>(where)=*self;return where;}
const std::type_info& __fastcall CallableType(GameCallable*) noexcept {return typeid(GameCallable);}
void __fastcall CallableDelete(GameCallable*,bool) noexcept {}
void* __fastcall CallableDestroy(GameCallable* self,uint32_t) noexcept {return self;}
const void* __fastcall CallableGet(GameCallable* self) noexcept {return self;}
void InitGameFunction(GameFunction& f,const void* const* vtable,uintptr_t first=0,uintptr_t second=0) noexcept {
    f.space[0]=const_cast<void*>(static_cast<const void*>(vtable));f.space[1]=reinterpret_cast<void*>(first);f.space[2]=reinterpret_cast<void*>(second);
    f.impl=&f.space[0];}
const void* const* GameVtable(uintptr_t rva) noexcept {return reinterpret_cast<const void* const*>(image+rva);}
#define GAME_CALLABLE_VTABLE(name,do_call) const void* const name[7]{reinterpret_cast<const void*>(&CallableCopy),reinterpret_cast<const void*>(&CallableCopy),\
    reinterpret_cast<const void*>(&do_call),reinterpret_cast<const void*>(&CallableType),reinterpret_cast<const void*>(&CallableDelete),\
    reinterpret_cast<const void*>(&CallableDestroy),reinterpret_cast<const void*>(&CallableGet)}

// The engine thread's work the rows hand over (atomics only on the menu task; -1 nothing pending).
std::atomic<int32_t> pending_screen_mode{-1},pending_frame_rate{-1},pending_preset{-1};
std::atomic<uint32_t> rt_rows_changed{};   // a bit for each RT row the player changed (the page's settings in rt_page_snapshot)
std::atomic<bool> rekindled_hooks_installed{};
bool RekindledOnGameThread() noexcept {
    if(GetCurrentThreadId()==owner_thread)return true;
    static LONG once{};if(!InterlockedExchange(&once,1))Log("{\"event\":\"native_rekindled_graphics\",\"refused\":\"not_game_thread\"}\n");
    return false;}

// DS3's window and graphics config: CSWindowImp [14489F118], the applied config at +0x68 and the requested one at +0xB0 (each: the
// windowed resolution +0, the full-screen one +8, the mode +0x10, Auto-detect +0x14, the preset +0x18, the eleven levels
// +0x1C..+0x44); the window applies the request at its next step (FUN_140EF5590).
uintptr_t GameWindow() noexcept {uintptr_t window{};return Read(image+0x489F118,&window,8)?window:0;}
bool rekindled_save_pending{};   // the engine thread's
void RequestGraphicsConfigSave() noexcept {rekindled_save_pending=true;}

// The screen mode (live: the window's next step; Full screen to Borderless and back, which keep the applied mode 1, switched by
// the native as I-0076's start does, R12C-STATIC §1.2). Borderless is kept in the settings file (GraphicsConfig.xml has two tokens).
// Windowed keeps the full-screen kind the player chose last (+0x40 and the file's borderless unchanged): going back to full screen
// from the game's own paths gives that kind again.
uint8_t ReadScreenModeValue() noexcept {
    const uintptr_t window=GameWindow();int32_t mode{};uint8_t borderless{};
    if(!window || !Read(window+0x78,&mode,4) || !Read(window+0x40,&borderless,1))return gr::kWindowed;
    return gr::ScreenModeValue(mode,borderless!=0);}
bool WriteScreenModeValue(uint8_t want) noexcept {
    if(!RekindledOnGameThread())return false;
    const uintptr_t window=GameWindow();if(!window)return false;
    if(want==gr::kWindowed){*reinterpret_cast<uint32_t*>(window+0xC0)=0;RequestGraphicsConfigSave();
        Log("{\"event\":\"native_rekindled_display\",\"screen_mode\":\"windowed\"}\n");return true;}
    const bool borderless=want==gr::kBorderlessValue;int32_t mode{};Read(window+0x78,&mode,4);
    const bool switched=(*reinterpret_cast<uint8_t*>(window+0x40)!=0)!=borderless;
    *reinterpret_cast<uint8_t*>(window+0x40)=borderless?1:0;
    if(mode==1 && switched){uintptr_t state{};HWND hwnd{};uint64_t resolution{};
        if(Read(window+0x38,&state,8) && state)reinterpret_cast<void(__fastcall*)(uintptr_t,uint32_t)>(image+0xEF1C50)(state,borderless?2u:1u);
        Read(window+8,&hwnd,8);Read(window+0x70,&resolution,8);
        const std::lock_guard hold(native_display_lock);
        native_display_request=NativeDisplayRequest{!borderless,borderless,uint32_t(resolution),uint32_t(resolution>>32),hwnd};}
    *reinterpret_cast<uint32_t*>(window+0xC0)=1;RequestGraphicsConfigSave();
    const bool saved=NativeSettingsCurrent().borderless==borderless || EditNativeSettings([&](ds3native::settings::Settings& s){s.borderless=borderless;});
    char row[160]{};sprintf_s(row,"{\"event\":\"native_rekindled_display\",\"screen_mode\":\"%s\",\"switched\":%s,\"saved\":%s}\n",borderless?"borderless":"full_screen",
        mode==1 && switched?"true":"false",saved?"true":"false");Log(row);
    return saved;}

// The Graphics tab's own functions (the menu task): Screen Mode with Borderless, the frame rate. Their writes hand the value to the
// engine thread.
uint8_t __fastcall ScreenModeRead(GameCallable*) noexcept {return ReadScreenModeValue();}
void __fastcall ScreenModeWrite(GameCallable*,const uint8_t* value) noexcept {if(value && *value<=gr::kBorderlessValue)pending_screen_mode.store(*value);}
void* __fastcall ScreenModeList(GameCallable*,uintptr_t list) noexcept {
    ChoiceCount(list)=0;AppendChoice(list,gr::kFullScreen,722000);AppendChoice(list,gr::kBorderlessValue,gr::kBorderless);AppendChoice(list,gr::kWindowed,722001);
    return reinterpret_cast<void*>(list);}
// The chosen rate until the engine thread has written it (the final review's L8: the saved one lags a frame or more).
uint8_t __fastcall FrameRateRead(GameCallable*) noexcept {
    if(const int32_t rate=pending_frame_rate.load();rate>=0)return uint8_t(rate);
    return gr::FrameRateValue(NativeSettingsCurrent().frame_rate);}
void __fastcall FrameRateWrite(GameCallable*,const uint8_t* value) noexcept {if(value && gr::FrameRateOf(*value))pending_frame_rate.store(*value);}
void* __fastcall FrameRateList(GameCallable*,uintptr_t list) noexcept {
    ChoiceCount(list)=0;
    for(uint8_t i=0;i<gr::kFrameRateCount;++i)AppendChoice(list,i,gr::kFrameRateChoice+i);
    if(gr::FrameRateValue(NativeSettingsCurrent().frame_rate)==gr::kFrameRateCustomValue)AppendChoice(list,gr::kFrameRateCustomValue,gr::kFrameRateCustom);
    return reinterpret_cast<void*>(list);}
GAME_CALLABLE_VTABLE(screen_read_vtable,ScreenModeRead);GAME_CALLABLE_VTABLE(screen_write_vtable,ScreenModeWrite);GAME_CALLABLE_VTABLE(screen_list_vtable,ScreenModeList);
GAME_CALLABLE_VTABLE(rate_read_vtable,FrameRateRead);GAME_CALLABLE_VTABLE(rate_write_vtable,FrameRateWrite);GAME_CALLABLE_VTABLE(rate_list_vtable,FrameRateList);

// A value row (FUN_140AB1C20(window, label, read, write, list): the game's Screen Mode and Resolution rows, R12C-STATIC §8.1).
void AddValueRow(uintptr_t window,int label_id,const void* const* read,const void* const* write,const void* const* list) noexcept {
    RowLabel label(label_id);
    GameFunction r,w,l;InitGameFunction(r,read);InitGameFunction(w,write);InitGameFunction(l,list);
    reinterpret_cast<void(__fastcall*)(uintptr_t,void*,GameFunction*,GameFunction*,GameFunction*)>(image+kFunctorRow)(window,label.bytes,&r,&w,&l);}
// V-sync's byte, edited by the game's On/Off row (FUN_140AA40F0(window, label, byte, its On/Off list FUN_140AAB920, the default, enabled)),
// read by the engine thread (StepRekindledGraphics).
uint8_t vsync_byte{1};std::atomic<bool> vsync_row_built{};
// The rows' defaults (the Defaults row's values) are read by the game as 4 bytes from where they point (the Advanced page's row keeps an
// int, FUN_140A9BE10): each one padded.
alignas(4) uint8_t vsync_default[4]{1},preset_default[4]{3},rt_defaults[gr::kRtRows][4]{};
// The game's On/Off row on a byte (V-sync's, the HDR output's).
void AddOnOffRow(uintptr_t window,int label_id,uint8_t* byte,uint8_t* fallback) noexcept {
    alignas(16) uint8_t choices[0x110]{};reinterpret_cast<void(__fastcall*)(void*)>(image+kOnOffChoices)(choices);
    RowLabel label(label_id);
    reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,void*,uint8_t*,void*,uint8_t*,uint8_t)>(image+kOnOffRow)(window,label.bytes,byte,choices,fallback,1);
    const uint64_t n=(std::min)(*reinterpret_cast<uint64_t*>(choices+0xF8),uint64_t(2));
    for(uint64_t i=0;i<n;++i)reinterpret_cast<void(__fastcall*)(void*)>(image+kOnOffEntryDestroy)(choices+i*0x78);}

// The Advanced Settings page. Its state (page+0x10F0, the Options' graphics state: Auto-detect +0x63, the preset +0x64, the eleven
// levels +0x65..+0x6F in GraphicsConfig.xml's order) is the game's: its rows edit those bytes and the page's change callback
// (FUN_1428C83E8's do_call: page+0x1154 = 5, then the page's vfunc 0x90 with the state) carries them on as in the game. Ours replaces
// that callback on each row and calls it after keeping the preset and the levels together (a preset writes its eleven levels from the
// game's table, ApplyGraphicsPreset FUN_140EF7340; a level makes the preset Custom), and for our RT rows hands the value to the
// engine thread (the settings file) and keeps the page's own copy of the settings, from which the page builds its rows again when a
// row that governs others changed (Thomas, 2026-11-11 00:44: « Il faut que l'anti aliasing se grise quand DLSS s'active »).
// The game's Antialiasing (state +0x66, 3 for On: GraphicsSettingsFillDefaults_140C20E20 maps the config's 1 to 3) has no row here: our
// DLSS and TAA rows own the anti-aliasing, and the native runs them only while the game's Antialiasing is on (FollowNativeDlssOption):
// kept On in the state the page applies (at its build and after a preset) and in the window's config (StepRekindledGraphics).
constexpr uint32_t kStatePreset=0x64,kStateLevels=0x65,kStateAntialiasing=0x66,kPresetCustom=5;constexpr uint8_t kStateAntialiasingOn=3;
constexpr uint8_t kPresetLevels[4][11]{{1,0,0,0,0,0,1,1,0,0,1},{2,1,2,2,2,2,2,2,3,3,2},{3,1,3,3,3,3,3,3,3,3,3},{4,1,3,4,3,4,4,4,4,3,4}};
// DS3's five rows still in use (STUDY-R12-SETTINGS-PAGE §1.2): the game's label (FMG id), its choice-list builder and its state byte.
struct GameRow {int label;uintptr_t choices;uint32_t level;};   // level: the index in the eleven (state +0x65 + level)
constexpr GameRow kGameRows[5]{{721100,0xA99B50,0},{721103,0xA99310,3},{721104,0xA990A0,4},{721107,0xA99B50,7},{721110,0xA997F0,9}};
constexpr uint32_t kTagPreset=0x100,kTagLevel=0x200,kTagRt=0x300;   // the change callback's second word: its row
uint8_t rt_bytes[gr::kRtRows]{};   // our RT rows' values, edited by the game's rows on the menu task
ds3native::settings::Settings page_settings{};   // the menu task's: the settings as the open Advanced page shows them
ds3native::settings::Dlss page_dlss_mode{gr::kDlssModeFirst};   // the menu task's: the DLSS mode the Anti-aliasing row gives back
std::mutex rt_snapshot_lock;ds3native::settings::Settings rt_page_snapshot{};   // the page's settings as last changed, for the engine thread
std::atomic<bool> rebuild_pending{};   // a governing row changed: the page's rows built again at its next update
uint8_t page_preset{};   // the menu task's: the preset the player chose on the open page (5 Custom)
// I-0132 R12g: our scroll (graphics_rows.h ScrollAfterMove): the page's rows as a list (the preset, DS3's five, our RT rows shown), of
// which a window of gr::kPageSlots is built in the film's slots; the menu task's.
struct PageRow {uint8_t kind;uint8_t index;bool enabled;};   // kind 0 the preset, 1 a DS3 row (kGameRows' index), 2 an RT row (RtRow)
constexpr uint32_t kPageRowsMost=1+5+gr::kRtRows;   // at most: the preset, DS3's rows, every RT row
PageRow page_rows[kPageRowsMost]{};bool page_enabled[kPageRowsMost]{};uint32_t page_row_count{},page_first{};
// An RT row's new value (the menu task): into the page's settings (and its DLSS mode), handed to the engine thread; a governing
// row's write builds the rows again at the page's next update.
void RtRowChanged(uint32_t i) noexcept {
    const auto r=static_cast<gr::RtRow>(i);
    const bool taken=gr::RtWrite(r,rt_bytes[i],page_settings,page_dlss_mode);
    if(taken && r==gr::RtRow::DlssMode)page_dlss_mode=page_settings.dlss;
    {const std::lock_guard hold(rt_snapshot_lock);rt_page_snapshot=page_settings;}rt_rows_changed.fetch_or(1u<<i);
    if(taken && gr::RtGoverns(r))rebuild_pending.store(true);}
// The game's callback turns a chosen preset into Custom (after the preset's own report, after the five rows' reports of its levels,
// r12g-wrap2 and r12g-pages8, and after the slider's own callback, the same vfunc 0x90; what it compares is not read): the page's
// chosen preset put back in its state while the levels are its table's, and in the window's config by the engine thread; a level
// that leaves the table makes it Custom. Called after each of our rows' callbacks and at each update of the page (the slider).
void KeepChosenPreset(uintptr_t page) noexcept {
    uint8_t* const state=reinterpret_cast<uint8_t*>(page+0x10F0);
    if(page_preset<1 || page_preset>4)return;
    for(uint32_t k=0;k<11;++k)if(k!=1 && state[kStateLevels+k]!=kPresetLevels[page_preset-1][k]){page_preset=uint8_t(kPresetCustom);return;}
    if(state[kStatePreset]!=page_preset){state[kStatePreset]=page_preset;pending_preset.store(page_preset);}}
std::atomic<uintptr_t> rekindled_advanced_page{},rekindled_defaults_skip{},rekindled_rows_begin{};   // the Advanced page our rows were built in; its Defaults row to skip
void __fastcall RowChanged(GameCallable* self) noexcept {
    const uintptr_t page=self->word[0];const uint32_t tag=uint32_t(self->word[1]);
    // I-0138 tranche 5b (its final review's L1): the Brightness page's greyed row (and, until tranche 5d, the Graphics tab's) is made by the same
    // row maker and carry this callback; a call for another page than the Advanced one (none seen: a locked row reports no change;
    // a Defaults press there was not witnessed) must not write the preset's state nor run the page's apply.
    if(page!=rekindled_advanced_page.load())return;
    uint8_t* const state=reinterpret_cast<uint8_t*>(page+0x10F0);
    if(tag==kTagPreset){const uint8_t preset=state[kStatePreset];
        if(preset>=1 && preset<=4){std::memcpy(state+kStateLevels,kPresetLevels[preset-1],sizeof(kPresetLevels[0]));state[kStateAntialiasing]=kStateAntialiasingOn;}
        }   // the five rows follow their bytes themselves (r12g-pages6: Low's levels shown at once)
    else if((tag & 0xF00)==kTagLevel){const uint8_t preset=state[kStatePreset];bool same=preset>=1 && preset<=4;
        for(uint32_t k=0;same && k<11;++k)if(k!=1 && state[kStateLevels+k]!=kPresetLevels[preset-1][k])same=false;   // k 1: the Antialiasing, kept On
        if(!same && preset!=kPresetCustom){state[kStatePreset]=uint8_t(kPresetCustom);rebuild_pending.store(true);}}   // the rows report the preset's own writes (r12g-pages5)
    else if((tag & 0xF00)==kTagRt && (tag & 0xFF)<gr::kRtRows)RtRowChanged(tag & 0xFF);
    char row[200]{};sprintf_s(row,"{\"event\":\"native_rekindled_graphics\",\"changed\":%u,\"preset\":%u,\"levels\":[%u,%u,%u,%u,%u]%s}\n",tag,state[kStatePreset],
        state[kStateLevels],state[kStateLevels+3],state[kStateLevels+4],state[kStateLevels+7],state[kStateLevels+9],(tag & 0xF00)==kTagRt?",\"rt\":true":"");Log(row);
    const uint8_t chosen=state[kStatePreset];
    *reinterpret_cast<uint8_t*>(page+0x1154)=5;   // FUN_1428C83E8's do_call, as the game's own rows call it
    reinterpret_cast<void(__fastcall*)(uintptr_t)>(*reinterpret_cast<uintptr_t*>(*reinterpret_cast<uintptr_t*>(page)+0x90))(page);
    // The preset row's report with the cursor on it (row 0, page+0x9D8+0xB4) is the player's choice, Custom included (the second step
    // review's M1); with the cursor elsewhere it is the game's Custom reaching the row (the slider's callback, the final review's M1).
    if(tag==kTagPreset && page_first==0 && *reinterpret_cast<const int32_t*>(page+0x9D8+0xB4)==0)page_preset=chosen;   // R12g: the list's row 0
    KeepChosenPreset(page);}
GAME_CALLABLE_VTABLE(row_changed_vtable,RowChanged);
#undef GAME_CALLABLE_VTABLE
// A row of the Advanced page's kind (FUN_140AA3D30(window, label, byte, choices, default, change callback, an empty function, enabled),
// the call AddRowWithLabelAndChoices FUN_140AA4210 makes with enabled 1; R12C-STATIC §8.2, §8.3).
// A greyed row's constructor writes its text into the slot's GrayoutItem (Widgets/ComboBox/GrayoutItem) but never shows it, and an
// enabled row hides it: a slot an enabled row held before a rebuild showed a greyed row without its words (the run r12g-pages2). Shown
// here: the slot of the row about to be added (FUN_140A90E20(page, out): Item_<rows>_0), its child by name (FUN_140A51C20), shown by
// ShowHideWindow_140A3D720, each handle released as the game releases them (FUN_140E63410 on its +0x28).
constexpr uintptr_t kSlotOfRow=0xA90E20,kChildOf=0xA51C20,kShowHide=0xA3D720,kHandleRelease=0xE63410,kComboName=0x28C0A90,kGrayoutName=0x28C3AC0,kSliderName=0x28C0A80,kButtonName=0x28C0AD8;   // « Widgets/ComboBox », « GrayoutItem », « Widgets/Slider », « Widgets/Button »
struct UiHandle {alignas(16) uint8_t bytes[0x80]{};~UiHandle(){reinterpret_cast<void(__fastcall*)(void*)>(image+kHandleRelease)(bytes+0x28);}};
// I-0132 R12f: a clip's X (pixels) and X scale (percent) through Scaleform's DisplayInfo, as the game's own setters do it (FUN_140E659A0,
// FUN_140A3CBA0, FUN_140E65520: get, modify the flagged fields, set; the R12f Ghidra study, OBSERVED): the handle's Value at +0x28 (its
// ObjectInterface at +0x40, its data at +0x50, its type at +0x48: a display object is 10), GetDisplayInfo its vfunc 0xD8 and
// SetDisplayInfo 0xE0 (ObjectInterface, data, DisplayInfo*); the DisplayInfo (0xD8 bytes) its X the double at +0x00 (flag 0x1), its
// XScale at +0x18 (flag 0x8), its flags word at +0xD4, its two matrices made identity first as the game does. `seen` gets what Get read.
bool SetClipX(UiHandle& h,double x,double xscale,double seen[2],bool read_only=false) noexcept {
    uint8_t* const v=h.bytes+0x28;
    if((*reinterpret_cast<const uint32_t*>(v+0x20) & 0x8F)!=10)return false;
    const uintptr_t oi=*reinterpret_cast<const uintptr_t*>(v+0x18),data=*reinterpret_cast<const uintptr_t*>(v+0x28);if(!oi || !data)return false;
    alignas(16) uint8_t info[0xD8]{};float* const m3=reinterpret_cast<float*>(info+0x60);m3[0]=m3[5]=m3[10]=1.f;
    float* const m4=reinterpret_cast<float*>(info+0x90);m4[0]=m4[5]=m4[10]=m4[15]=1.f;
    const auto* const slots=*reinterpret_cast<const uintptr_t* const*>(oi);
    if(!reinterpret_cast<bool(__fastcall*)(uintptr_t,uintptr_t,void*)>(slots[0xD8/8])(oi,data,info))return false;   // the final review's L2: its answer
    double& at=*reinterpret_cast<double*>(info);double& scale=*reinterpret_cast<double*>(info+0x18);seen[0]=at;seen[1]=scale;
    if(read_only || (at==x && scale==xscale))return true;
    at=x;scale=xscale;*reinterpret_cast<uint16_t*>(info+0xD4)|=0x1|0x8;
    reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t,const void*)>(slots[0xE0/8])(oi,data,info);return true;}
// I-0138 tranche 5d (its step review's High): a slot's own widget shown once its row is made. The row makers hide the slot's four
// other widgets and never show their own (R12C-STATIC §8.4), and since tranche 5d a slot of the Graphics tab changes its row's kind
// when the HDR output is switched (the sixth and the seventh: a slider, or the Advanced Settings button and Defaults): a button made
// in a slot that held a slider stayed hidden. `slot`: taken before the row is made (kSlotOfRow answers the slot of the list's size).
void ShowSlotWidget(UiHandle& slot,uintptr_t name) noexcept {
    UiHandle widget;reinterpret_cast<void*(__fastcall*)(void*,void*,const char*)>(image+kChildOf)(slot.bytes,widget.bytes,reinterpret_cast<const char*>(image+name));
    reinterpret_cast<void(__fastcall*)(void*,uint8_t)>(image+kShowHide)(widget.bytes,1);}
// I-0132 R12f: the Caption of a slot's widget (its label and the row's left separator) placed as a sub-row's, a quarter shorter
// (graphics_rows.h RowCaption), or as any other row's, the film's layout again (a slot keeps its clips across a rebuild). `places`:
// the film's unscaled places of Caption and its Text_0, as last read from a slot of that film not yet changed (a changed slot
// keeps the ones read before; the two films differ: the Graphics tab's sliders hold their own pair, and AddAdvancedRow's one pair
// serves the Advanced page's film and the Brightness page's row of the options film, read again at each page's first, unscaled
// row before any sub-row uses it). `seen`: what was read.
bool PlaceRowCaption(UiHandle& widget,bool sub,double places[2],double seen[4]) noexcept {
    const auto child=reinterpret_cast<void*(__fastcall*)(void*,void*,const char*)>(image+kChildOf);
    UiHandle caption;child(widget.bytes,caption.bytes,"Caption");UiHandle text;child(caption.bytes,text.bytes,"Text_0");
    bool set=SetClipX(caption,0,0,seen,true) && SetClipX(text,0,0,seen+2,true);
    if(set && seen[1]==100.0 && seen[3]==100.0){places[0]=seen[0];places[1]=seen[2];}
    const auto placed=gr::RowCaption(sub,places[0],places[1]);double after[2]{};
    return set && SetClipX(caption,placed.caption_x,placed.caption_xscale,after) && SetClipX(text,placed.text_x,placed.text_xscale,after);}
// `label`: a label object of the game's (0x38 bytes of text, its help after it), the caller's; `sub`: a sub-row (above).
void AddAdvancedRow(uintptr_t page,void* label,bool sub,uint8_t* byte,uintptr_t choices,uint8_t* fallback,uint32_t tag,bool enabled) noexcept {
    const auto add=[&]{GameFunction changed,none{};InitGameFunction(changed,row_changed_vtable,page,tag);
        reinterpret_cast<void(__fastcall*)(uintptr_t,void*,uint8_t*,uintptr_t,uint8_t*,GameFunction*,GameFunction*,uint8_t)>(image+kAdvancedRow)(
            page,label,byte,choices,fallback,&changed,&none,enabled?1:0);};
    UiHandle slot;reinterpret_cast<void*(__fastcall*)(uintptr_t,void*)>(image+kSlotOfRow)(page,slot.bytes);   // before the row: the slot of the list's size
    add();
    // The row makers hide the slot's four other widgets but never show their own: a slot that held the sharpening's slider before a
    // rebuild showed a choice row with nothing (the run r12g-pages4). Its ComboBox shown here, and a greyed row's GrayoutItem.
    const auto child=reinterpret_cast<void*(__fastcall*)(void*,void*,const char*)>(image+kChildOf);
    UiHandle combo;child(slot.bytes,combo.bytes,reinterpret_cast<const char*>(image+kComboName));
    reinterpret_cast<void(__fastcall*)(void*,uint8_t)>(image+kShowHide)(combo.bytes,1);
    {static double places[2]{gr::kCaptionX,gr::kCaptionTextX};double seen[4]{};
        const bool set=PlaceRowCaption(combo,sub,places,seen);
        static bool named=false;if(!named){named=true;char row[224]{};
            sprintf_s(row,"{\"event\":\"native_rekindled_graphics\",\"caption\":{\"set\":%s,\"caption_x\":%.6g,\"caption_xscale\":%.6g,\"text_x\":%.6g,\"text_xscale\":%.6g}}\n",
                set?"true":"false",seen[0],seen[1],seen[2],seen[3]);Log(row);}}   // the film's values as first read (the constants' check)
    if(enabled)return;
    UiHandle grey;child(combo.bytes,grey.bytes,reinterpret_cast<const char*>(image+kGrayoutName));
    reinterpret_cast<void(__fastcall*)(void*,uint8_t)>(image+kShowHide)(grey.bytes,1);}
void AddAdvancedRow(uintptr_t page,int label_id,uint8_t* byte,uintptr_t choices,uint8_t* fallback,uint32_t tag,bool enabled) noexcept {
    const int rt=label_id-gr::kRtLabel;RowLabel label(label_id);   // the final review's L1: a sub-row only among the RT rows
    AddAdvancedRow(page,label.bytes,rt>=0 && rt<int(gr::kRtRows) && gr::SubRow(gr::RtRow(rt)),byte,choices,fallback,tag,enabled);}
using AdvancedRowsFn=void(__fastcall*)(uintptr_t,uintptr_t);AdvancedRowsFn advanced_rows_original{};
// The page's Defaults row (OpenRestoreDefaultsBox_140AAE950(page), called by CreateOptionSubPageByName after the rows) is left out of
// our Advanced page: the film has fourteen slots, our rows fill them, and a fifteenth row did not scroll (the run r12g-pages1; the
// fault at its end was the run's stop, the RCA). The Graphics tab keeps its own Defaults row.
using DefaultsRowFn=void(__fastcall*)(uintptr_t);DefaultsRowFn advanced_defaults_original{};
void __fastcall RekindledAdvancedDefaults(uintptr_t page) noexcept {
    uintptr_t expected=page;
    if(rekindled_hooks_installed.load() && page && rekindled_defaults_skip.compare_exchange_strong(expected,0)){Log("{\"event\":\"native_rekindled_graphics\",\"page\":\"advanced\",\"defaults_row\":false}\n");return;}
    advanced_defaults_original(page);}
// The game's slider row (AddSliderRow FUN_140AA43C0(page, label, byte, range, default): the game's Brightness page's, R12C-STATIC §8.5; its range
// {min, max, step, a byte}; its change callback the game's own FUN_1428C8368, which only calls the page's vfunc 0x90): its byte watched
// at the page's update (RekindledWindowUpdate) instead.
constexpr uintptr_t kSliderRow=0xAA43C0;
struct SliderRange {int32_t min,max,step;uint8_t flag;uint8_t pad[3];};
static_assert(sizeof(SliderRange)==16);
uint32_t slider_rows{};uint8_t slider_seen[gr::kRtRows]{};   // the menu task's: the rows built as sliders (gr::RtSlider, I-0138 tranche 7), their values seen
SliderRange advanced_slider_ranges[gr::kRtRows]{};   // the menu task's too: each slider row's range, kept for the game while the page lives (the step review's L5)

// I-0138 tranche 5b (graphics_rows.h « the HDR rows »; Thomas, 2026-11-14 12:05): the Graphics tab (FUN_140AB07D0(window, state)).
// With the film's nine slots (options_film.h: three added by 16b's observer as the game takes the film): Screen Mode, Resolution,
// V-sync, the frame rate, the HDR output (the game's On/Off row), its peak and its paper white (the game's slider rows, built only while the HDR
// output is on since tranche 5d: seven rows with it off), the Advanced Settings button; the tab's own Defaults row
// follows in the ninth. A film without the added slots (not the shipped one, or no observer) keeps the five rows of before in its
// six slots (value, value, choice, value, button, R12C-STATIC §5; Defaults in the sixth): the HDR output then follows the settings
// file alone.
// A slider row keeps its value as an int in its widget (the row + 0x98, its value at +0xBC8: FUN_140AA5940, FUN_140A53C20) and writes
// only the low byte through the row's pointer (140AB4E40), so each slider is given a scratch byte: the widget's int is set once the row
// is made (FUN_140A538D0(slider, value, no callback, a sound request that plays nothing), then FUN_140A537B0: the value its baseline)
// and read at each update of the tab. The game draws the bare int beside the bar (« %d », FUN_140A51590) at each change and when it
// sets the baseline, so our word for the peak's automatic value (« Auto ») is written at each update, the way the game writes its own (the
// slider's clip at +0x9D8, its child Text_0, FUN_140E65770 on the handle's value). A row's default is a byte (FUN_140AAC470): the
// paper white's 200, the peak's 0, which the widget brings to its least value, automatic.
std::atomic<uint32_t> options_film_slots{};   // the slots of the tab's film as the game last took it (16b's observer; 0: not seen)
constexpr uintptr_t kSliderSet=0xA538D0,kSliderBaseline=0xA537B0,kSilentSound=0x3D77E60,kTextFieldName=0x28AD090,kSetText=0xE65770,kTabDefaultsRow=0xAAEC10,
    kRowsRelease=0xA90390,kRowsBind=0xA913B0,kSubPageVtable=0x28C7558,kSliderRowVtable=0x28C7868;
struct SoundRequest {int32_t id{-1},priority{-1},channel{-1};uint32_t pad{};const void* name{};};   // FUN_140AAC470's: an id under 0 plays nothing (FUN_140A67CA0)
static_assert(sizeof(SoundRequest)==24 && ds3native::settings::kHdrPaperWhiteDefault<=255);
struct TabSlider {uintptr_t slider{};uint32_t index{};};   // the row's widget and the row's place in the tab's list
struct HdrFields {bool hdr{};uint32_t peak{},paper_white{};};
// The menu task's: the tab's bytes and sliders, its copy of the settings as its rows show them, its rows' count without Defaults.
uint8_t tab_hdr_byte{},tab_scratch[2]{};alignas(4) uint8_t tab_hdr_default[4]{},tab_peak_default[4]{},tab_paper_default[4]{uint8_t(ds3native::settings::kHdrPaperWhiteDefault)};
TabSlider tab_peak{},tab_paper{};ds3native::settings::Settings tab_settings{};bool tab_hdr_rows{},tab_begin_known{};uint32_t tab_row_count{};
std::atomic<uintptr_t> rekindled_graphics_tab{},rekindled_tab_rows_begin{};std::atomic<bool> tab_rebuild_pending{};
// The tab's HDR settings as last changed, for the engine thread: the snapshot under rt_snapshot_lock, and the tick from which it
// is taken (0 nothing to take; the step review's L1: a slider held or dragged changes at every update, and each taking writes the
// settings file to the disk and quiesces the renderer for the chain, so the value is taken once the slider has been still for
// kTabSettleMs, a time and not a count of frames, the second step review's Low; the HDR output's switch at once). The engine
// thread waits, so a tab left meanwhile loses nothing; a snapshot taken twice writes nothing the second time (EditNativeSettings).
HdrFields tab_hdr_snapshot{};std::atomic<uint64_t> tab_hdr_due{};constexpr uint64_t kTabSettleMs=250;
void SetSliderText(uintptr_t slider,const wchar_t* text) noexcept {
    UiHandle field;reinterpret_cast<void*(__fastcall*)(uintptr_t,void*,const char*)>(image+kChildOf)(slider+0x9D8,field.bytes,reinterpret_cast<const char*>(image+kTextFieldName));
    if((*reinterpret_cast<const uint32_t*>(field.bytes+0x28+0x20) & 0x8F)>1)reinterpret_cast<void(__fastcall*)(void*,const wchar_t*)>(image+kSetText)(field.bytes+0x28,text);}
// One of the two HDR slider rows, built only while the HDR output is on (I-0138 tranche 5d, Thomas 2026-11-14 20:00): a sub-row of
// the HDR output's, its separator a quarter shorter as the DLSS mode's.
void AddHdrSliderRow(uintptr_t window,bool peak) noexcept {
    TabSlider& held=peak?tab_peak:tab_paper;held={};const uint32_t k=peak?0:1;
    const int32_t value=peak?gr::HdrPeakSliderValue(tab_settings.hdr_peak):gr::HdrPaperWhiteSliderValue(tab_settings.hdr_paper_white);
    uint8_t* const fallback=peak?tab_peak_default:tab_paper_default;tab_scratch[k]=0;
    static const SliderRange ranges[2]{{gr::kHdrPeakSlider.min,gr::kHdrPeakSlider.max,gr::kHdrPeakSlider.step,1,{}},   // the flag: the ten-step keys (FUN_140A53AF0)
        {gr::kHdrPaperWhiteSlider.min,gr::kHdrPaperWhiteSlider.max,gr::kHdrPaperWhiteSlider.step,1,{}}};
    const uintptr_t* const rows=reinterpret_cast<const uintptr_t*>(window+0x1070);const uintptr_t count=(rows[1]-rows[0])>>7;
    {RowLabel label(peak?gr::kHdrPeakLabel:gr::kHdrPaperWhiteLabel);
        UiHandle slot;reinterpret_cast<void*(__fastcall*)(uintptr_t,void*)>(image+kSlotOfRow)(window,slot.bytes);
        reinterpret_cast<void(__fastcall*)(uintptr_t,void*,uint8_t*,const SliderRange*,uint8_t*)>(image+kSliderRow)(window,label.bytes,&tab_scratch[k],&ranges[k],fallback);
        UiHandle widget;reinterpret_cast<void*(__fastcall*)(void*,void*,const char*)>(image+kChildOf)(slot.bytes,widget.bytes,reinterpret_cast<const char*>(image+kSliderName));
        reinterpret_cast<void(__fastcall*)(void*,uint8_t)>(image+kShowHide)(widget.bytes,1);   // its own widget shown (AddAdvancedRow's note)
        static double places[2]{gr::kCaptionX,gr::kOptionsCaptionTextX};double seen[4]{};const bool set=PlaceRowCaption(widget,true,places,seen);
        static bool named=false;if(!named){named=true;char row[224]{};
            sprintf_s(row,"{\"event\":\"native_rekindled_graphics\",\"slider_caption\":{\"set\":%s,\"caption_x\":%.6g,\"caption_xscale\":%.6g,\"text_x\":%.6g,\"text_xscale\":%.6g}}\n",
                set?"true":"false",seen[0],seen[1],seen[2],seen[3]);Log(row);}}   // the options film's values as first read
    if(((rows[1]-rows[0])>>7)!=count+1)return;   // no row was made
    const uintptr_t row=*reinterpret_cast<const uintptr_t*>(rows[1]-0x80+0x70);if(!row)return;   // the list's entry: its label, then the row
    const uintptr_t slider=row+0x98;const SoundRequest silent{-1,-1,-1,0,reinterpret_cast<const void*>(image+kSilentSound)};
    reinterpret_cast<void(__fastcall*)(uintptr_t,int32_t,uint8_t,const SoundRequest*)>(image+kSliderSet)(slider,value,0,&silent);
    reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+kSliderBaseline)(slider);
    wchar_t text[24]{};SetSliderText(slider,gr::HdrSliderText(peak,value,text));
    held={slider,uint32_t(count)};}
void BuildGraphicsTabRows(uintptr_t window,const char* why) noexcept {
    const uintptr_t* const rows=reinterpret_cast<const uintptr_t*>(window+0x1070);
    AddValueRow(window,720100,screen_read_vtable,screen_write_vtable,screen_list_vtable);   // Screen Mode, ours
    AddValueRow(window,720101,GameVtable(kResolutionRead),GameVtable(kResolutionWrite),GameVtable(kResolutionList));   // Resolution, the game's own
    AddOnOffRow(window,gr::kVsync,&vsync_byte,vsync_default);
    AddValueRow(window,gr::kFrameRate,rate_read_vtable,rate_write_vtable,rate_list_vtable);
    tab_peak={};tab_paper={};
    if(tab_hdr_rows){tab_hdr_byte=tab_settings.hdr?1:0;AddOnOffRow(window,gr::kHdrLabel,&tab_hdr_byte,tab_hdr_default);
        if(tab_settings.hdr){AddHdrSliderRow(window,true);AddHdrSliderRow(window,false);}}   // tranche 5d: its two sub-rows only while it is on
    {RowLabel label(gr::kAdvancedLabel);GameFunction press,none{};InitGameFunction(press,GameVtable(kAdvancedButton),window);   // the game's own button (FUN_140AA8750's chain)
        UiHandle slot;reinterpret_cast<void*(__fastcall*)(uintptr_t,void*)>(image+kSlotOfRow)(window,slot.bytes);const uintptr_t count=(rows[1]-rows[0])>>7;
        reinterpret_cast<void(__fastcall*)(uintptr_t,void*,GameFunction*,GameFunction*)>(image+kButtonRow)(window,label.bytes,&press,&none);
        if(((rows[1]-rows[0])>>7)==count+1)ShowSlotWidget(slot,kButtonName);}   // tranche 5d: its slot may have held a slider; only when the row was made (the merge's final review, L3)
    tab_row_count=uint32_t((rows[1]-rows[0])>>7);
    char row[288]{};sprintf_s(row,"{\"event\":\"native_rekindled_graphics\",\"page\":\"graphics\",\"built\":\"%s\",\"rows\":%u,\"film_slots\":%u,\"vsync\":%u,\"screen_mode\":%u,\"hdr\":%s,\"hdr_peak\":%u,\"hdr_paper_white\":%u,\"sliders\":%u}\n",
        why,tab_row_count,options_film_slots.load(),vsync_byte,ReadScreenModeValue(),tab_settings.hdr?"true":"false",tab_settings.hdr_peak,tab_settings.hdr_paper_white,
        uint32_t(tab_peak.slider!=0)+uint32_t(tab_paper.slider!=0));Log(row);}
using GraphicsRowsFn=void(__fastcall*)(uintptr_t,uintptr_t);GraphicsRowsFn graphics_rows_original{};
void __fastcall RekindledGraphicsTab(uintptr_t window,uintptr_t state) noexcept {
    if(!rekindled_hooks_installed.load()){graphics_rows_original(window,state);return;}
    rekindled_graphics_tab.store(0);tab_rebuild_pending.store(false);tab_begin_known=false;
    tab_settings=NativeSettingsCurrent();tab_hdr_rows=options_film_slots.load()>=ds3native::options_film::kSlotsWanted;
    vsync_byte=tab_settings.vsync?1:0;vsync_row_built.store(true);
    BuildGraphicsTabRows(window,"open");
    // The final review's L1: the game adds the tab's Defaults row next, in the slot after ours: its Button shown beforehand (a
    // maker hides only the slot's four other widgets), in case the film's clips outlived a page whose slot held a slider.
    {UiHandle slot;reinterpret_cast<void*(__fastcall*)(uintptr_t,void*)>(image+kSlotOfRow)(window,slot.bytes);ShowSlotWidget(slot,kButtonName);}
    rekindled_graphics_tab.store(window);}   // after its rows; the game adds the tab's Defaults row next, then binds the list
// Our tab: its address, the sub-page's class, its rows' count (ours and Defaults) and its list (taken at the first update after the
// build: the Defaults row is pushed after our rows and may move the list).
bool IsRekindledGraphicsTab(uintptr_t window) noexcept {
    uintptr_t vtable{},begin{},end{};
    if(!window || window!=rekindled_graphics_tab.load() || !Read(window,&vtable,8) || vtable!=image+kSubPageVtable || !Read(window+0x1070,&begin,8) ||
       !Read(window+0x1078,&end,8) || !begin || ((end-begin)>>7)!=tab_row_count+1)return false;
    if(!tab_begin_known){tab_begin_known=true;rekindled_tab_rows_begin.store(begin);}
    return begin==rekindled_tab_rows_begin.load();}
// A slider we made is still the row at its place in the tab's list (guarded reads: nothing of a freed page is followed).
bool TabSliderAlive(uintptr_t window,const TabSlider& s) noexcept {
    uintptr_t begin{},end{},row{},vtable{};
    return s.slider && Read(window+0x1070,&begin,8) && Read(window+0x1078,&end,8) && begin && ((end-begin)>>7)>s.index &&
        Read(begin+uintptr_t(s.index)*0x80+0x70,&row,8) && row && row+0x98==s.slider && Read(row,&vtable,8) && vtable==image+kSliderRowVtable;}
// The tab's rows built again (the HDR output switched: its two sub-rows come or go), as RebuildAdvancedRows does it below: the list
// emptied as the page's destructor empties it, our rows, the tab's Defaults row as the game adds it (FUN_140AAEC10(page): the
// defaults function the options' tab dispatcher passes for « PCGraphic », 140AB3850), bound again with the cursor where it was.
void RebuildGraphicsTab(uintptr_t window) noexcept {
    uintptr_t* const rows=reinterpret_cast<uintptr_t*>(window+0x1070);const int32_t cursor=*reinterpret_cast<const int32_t*>(window+0x9D8+0xB4);
    reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t)>(image+kRowsRelease)(window+0x1070,rows[0],rows[1]);rows[1]=rows[0];
    BuildGraphicsTabRows(window,"rebuilt");
    {UiHandle slot;reinterpret_cast<void*(__fastcall*)(uintptr_t,void*)>(image+kSlotOfRow)(window,slot.bytes);const uintptr_t count=(rows[1]-rows[0])>>7;
        reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+kTabDefaultsRow)(window);
        if(((rows[1]-rows[0])>>7)==count+1)ShowSlotWidget(slot,kButtonName);}   // tranche 5d: as the Advanced Settings button's
    const int32_t built=int32_t((rows[1]-rows[0])>>7);
    reinterpret_cast<void(__fastcall*)(uintptr_t,int32_t)>(image+kRowsBind)(window,(std::clamp)(cursor,0,(std::max)(built-1,0)));
    rekindled_tab_rows_begin.store(rows[0]);tab_begin_known=true;}
// After each update of the tab (the menu task): the HDR output's byte and the sliders' values taken into the tab's settings and
// handed to the engine thread; the sliders' words written at every update (the step review's L2 asked for a write at a change
// only: the game writes its bare number again when it sets a slider's baseline, with no change of value to see, so the words
// are written each time: two child lookups and two texts an update, while the tab is open); a switch of the HDR output builds the
// rows again at the next update.
void StepGraphicsTab(uintptr_t window) noexcept {
    if(!tab_hdr_rows)return;
    bool changed=false,switched=false;
    if(const bool on=tab_hdr_byte!=0;on!=tab_settings.hdr){tab_settings.hdr=on;changed=switched=true;tab_rebuild_pending.store(true);}
    for(const bool peak:{true,false}){TabSlider& s=peak?tab_peak:tab_paper;if(!s.slider)continue;
        int32_t value{};if(!TabSliderAlive(window,s) || !Read(s.slider+0xBC8,&value,4)){s={};continue;}
        if(peak?gr::HdrPeakWrite(value,tab_settings):gr::HdrPaperWhiteWrite(value,tab_settings))changed=true;
        wchar_t text[24]{};SetSliderText(s.slider,gr::HdrSliderText(peak,value,text));}
    if(!changed)return;
    {const std::lock_guard hold(rt_snapshot_lock);tab_hdr_snapshot={tab_settings.hdr,tab_settings.hdr_peak,tab_settings.hdr_paper_white};}
    tab_hdr_due.store(GetTickCount64()+(switched?0:kTabSettleMs));
    char row[160]{};sprintf_s(row,"{\"event\":\"native_rekindled_graphics\",\"hdr_rows\":{\"hdr\":%s,\"hdr_peak\":%u,\"hdr_paper_white\":%u}}\n",
        tab_settings.hdr?"true":"false",tab_settings.hdr_peak,tab_settings.hdr_paper_white);Log(row);}

// I-0138 tranche 5b (Thomas, 2026-11-14 12:05: « Il faudrait également que le réglage de luminosité du jeu de base se désactive si une
// sortie HDR est détectée, pour pas faire doublon ni perturber »): the game's Brightness row. Its page « BrightnessSetting » is built
// by FUN_140AAC6A0(page, state) (one slider on state + 2, 0 to 10, the label FUN_140AAA370; the options' tab and the brightness
// screen of a new game, FUN_140BE2DC0, both go through it; read in the executable). While the chain is HDR10 the options' tab
// does not get its slider: a greyed locked row with the game's own label says it has no effect (the output pass no longer reads
// the option either, scene_pass_frame.cpp), in the tab's first slot, of the generic kind 92 (the
// film's sprite « BrightnessSetting »). The options keep each tab's page while they are open (the run i138-t5b1-tab: one build of
// the Graphics tab over three visits, and a Brightness page built before the HDR output was switched kept its slider), so the
// page's rows are built again at its next update when the chain's kind is no longer the one they were built for: the list
// emptied, the row, the page's Defaults row as the game adds it (FUN_140AAD1B0, what CreateOptionSubPageByName calls for a page
// without its own defaults function), bound again. The new game's screen keeps its slider (the step review's M1: its film's
// slot is not read; CreateOptionSubPageByName's seventh argument, kept at page + 0x1161, is 1 for it and 0 for the options' tabs):
// there the slider moves and changes nothing on an HDR10 chain.
std::atomic<bool> native_hdr10_chain{};   // the engine thread's: the swap chain is HDR10 now (15_cpu_service_and_input.inc)
constexpr uintptr_t kBrightnessLabel=0xAAA370,kPageDefaultsRow=0xAAD1B0;
uint8_t brightness_locked_byte{};alignas(4) uint8_t brightness_locked_default[4]{};   // the menu task's: the locked row's value, never changed
std::atomic<uintptr_t> brightness_page{},brightness_rows_begin{};bool brightness_built_locked{},brightness_begin_known{};   // the options' Brightness page, its list and what its row was built for (the menu task's)
using BrightnessRowsFn=void(__fastcall*)(uintptr_t,uintptr_t);BrightnessRowsFn brightness_rows_original{};
void BuildBrightnessRow(uintptr_t page,uintptr_t state,bool locked,const char* why) noexcept {
    // The final review's High (tranche 5d): the game's slider maker does not show its own widget either, and the locked row's maker
    // hid it in this slot: the slider stayed hidden once the chain was SDR again (i138-t5b2-tab, i138-t5d1-tab).
    if(!locked){UiHandle slot;reinterpret_cast<void*(__fastcall*)(uintptr_t,void*)>(image+kSlotOfRow)(page,slot.bytes);
        const uintptr_t* const rows=reinterpret_cast<const uintptr_t*>(page+0x1070);const uintptr_t count=(rows[1]-rows[0])>>7;
        brightness_rows_original(page,state);
        if(((rows[1]-rows[0])>>7)==count+1)ShowSlotWidget(slot,kSliderName);}   // only when the builder made its one row (the fixes' step review)
    else {alignas(16) uint8_t label[0x80]{};reinterpret_cast<void*(__fastcall*)(void*)>(image+kBrightnessLabel)(label);
        {ChoiceList list;AppendChoice(list.Get(),0,gr::kWordHdrOff);brightness_locked_byte=0;
            AddAdvancedRow(page,label,false,&brightness_locked_byte,list.Get(),brightness_locked_default,0,false);}
        reinterpret_cast<void(__fastcall*)(void*)>(image+kLabelDestroy)(label);}
    char row[128]{};sprintf_s(row,"{\"event\":\"native_rekindled_graphics\",\"page\":\"brightness\",\"built\":\"%s\",\"locked\":%s}\n",why,locked?"true":"false");Log(row);}
void __fastcall RekindledBrightnessRows(uintptr_t page,uintptr_t state) noexcept {
    uint8_t new_game_screen{1};
    brightness_page.store(0);brightness_begin_known=false;   // whatever page this is, the one kept before is gone or built again (the second step review's High)
    if(!rekindled_hooks_installed.load() || !Read(page+0x1161,&new_game_screen,1) || new_game_screen){brightness_rows_original(page,state);return;}
    brightness_built_locked=native_hdr10_chain.load();
    BuildBrightnessRow(page,state,brightness_built_locked,"open");
    brightness_page.store(page);}
// The options' Brightness page: its address, the sub-page's class, an options' tab (never the new game's screen), its two rows (the
// row and Defaults) and its list (taken at the first update after a build, as the Graphics tab's: Defaults is pushed after the row).
bool IsBrightnessPage(uintptr_t window) noexcept {
    uintptr_t vtable{},begin{},end{};uint8_t new_game_screen{1};
    if(!window || window!=brightness_page.load() || !Read(window,&vtable,8) || vtable!=image+kSubPageVtable || !Read(window+0x1161,&new_game_screen,1) || new_game_screen ||
       !Read(window+0x1070,&begin,8) || !Read(window+0x1078,&end,8) || !begin || ((end-begin)>>7)!=2)return false;
    if(!brightness_begin_known){brightness_begin_known=true;brightness_rows_begin.store(begin);}
    return begin==brightness_rows_begin.load();}
void RebuildBrightnessPage(uintptr_t window,bool locked) noexcept {
    uintptr_t* const rows=reinterpret_cast<uintptr_t*>(window+0x1070);const int32_t cursor=*reinterpret_cast<const int32_t*>(window+0x9D8+0xB4);
    reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t)>(image+kRowsRelease)(window+0x1070,rows[0],rows[1]);rows[1]=rows[0];
    BuildBrightnessRow(window,window+0x10F0,locked,"rebuilt");brightness_built_locked=locked;
    reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+kPageDefaultsRow)(window);
    const int32_t built=int32_t((rows[1]-rows[0])>>7);
    reinterpret_cast<void(__fastcall*)(uintptr_t,int32_t)>(image+kRowsBind)(window,(std::clamp)(cursor,0,(std::max)(built-1,0)));
    brightness_rows_begin.store(rows[0]);brightness_begin_known=true;}
// The rows (the menu task): the preset, DS3's five, our RT rows shown from `held`; I-0132 R12g: only the window [page_first,
// page_first + gr::kPageSlots) of that list is built in the film's slots (graphics_rows.h ScrollAfterMove).
// `keep` (R12g's step review M1): the list's row the window keeps in it (the cursor's), -1 none.
void BuildAdvancedRows(uintptr_t page,uintptr_t state,const ds3native::settings::Settings& held,const char* why,int32_t keep=-1) noexcept {
    uint8_t* const bytes=reinterpret_cast<uint8_t*>(state);bytes[kStateAntialiasing]=kStateAntialiasingOn;
    alignas(16) uint8_t defaults[32]{};reinterpret_cast<void(__fastcall*)(void*)>(image+kGraphicsDefaults)(defaults);   // the eleven levels' defaults
    const ds3native::settings::Settings start{};uint32_t greyed{};slider_rows=0;
    page_row_count=0;
    page_rows[page_row_count++]={0,0,true};
    const auto rt_row=[&](uint32_t i){const auto r=static_cast<gr::RtRow>(i);if(!gr::RtShown(r,held))return;
        rt_bytes[i]=gr::RtValue(r,held);rt_defaults[i][0]=gr::RtValue(r,start);const bool enabled=gr::RtAvailable(r,held);if(!enabled)greyed|=1u<<i;
        page_rows[page_row_count++]={2,uint8_t(i),enabled};};
    for(uint8_t g=0;g<5;++g)page_rows[page_row_count++]={1,g,true};
    for(uint32_t i=0;i<gr::kRtRows;++i)rt_row(i);
    for(uint32_t v=0;v<page_row_count;++v)page_enabled[v]=page_rows[v].enabled;
    page_first=keep>=0?gr::FirstRowKeeping(page_first,page_row_count,(std::min)(uint32_t(keep),page_row_count?page_row_count-1:0u)):gr::FirstRowClamped(page_first,page_row_count);
    const uint32_t end=(std::min)(page_first+gr::kPageSlots,page_row_count);
    for(uint32_t v=page_first;v<end;++v){const PageRow& row=page_rows[v];
        if(row.kind==0){ChoiceList list;for(int p=1;p<=4;++p)AppendChoice(list.Get(),uint8_t(p),722020+p);AppendChoice(list.Get(),uint8_t(kPresetCustom),gr::kPresetCustomWord);
            AddAdvancedRow(page,gr::kPresetLabel,bytes+kStatePreset,list.Get(),preset_default,kTagPreset,true);continue;}
        if(row.kind==1){const auto& g=kGameRows[row.index];ChoiceList list;reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+g.choices)(list.Get());
            AddAdvancedRow(page,g.label,bytes+kStateLevels+g.level,list.Get(),defaults+g.level,kTagLevel+g.level,true);continue;}
        const uint32_t i=row.index;const auto r=static_cast<gr::RtRow>(i);const bool enabled=row.enabled;
        // A greyed row shows its list's first entry (the run r12g-pages1): its list is the one value it shows.
        const auto choices=gr::RtChoices(r,held);ChoiceList list;
        for(uint32_t c=0;c<choices.count;++c)if(enabled || choices.items[c].value==rt_bytes[i])AppendChoice(list.Get(),choices.items[c].value,choices.items[c].word);
        // Thomas (2026-11-11 02:21: « C'est mieux un curseur »; 2026-11-14 12:05: the numeric settings as sliders): the game's slider row
        // for the sharpening (0 to 10), the bloom, the film grain and the field of view (percent by tens, I-0138 tranche 7); each range
        // kept for the game (advanced_slider_ranges, the menu task's).
        if(const auto steps=gr::RtSlider(r);steps && enabled){
            RowLabel label(gr::kRtLabel+int(i));SliderRange& range=advanced_slider_ranges[i];range={steps->min,steps->max,steps->step,0,{}};
            UiHandle slot;reinterpret_cast<void*(__fastcall*)(uintptr_t,void*)>(image+kSlotOfRow)(page,slot.bytes);
            reinterpret_cast<void(__fastcall*)(uintptr_t,void*,uint8_t*,const SliderRange*,uint8_t*)>(image+kSliderRow)(page,label.bytes,&rt_bytes[i],&range,rt_defaults[i]);
            UiHandle slider;reinterpret_cast<void*(__fastcall*)(void*,void*,const char*)>(image+kChildOf)(slot.bytes,slider.bytes,reinterpret_cast<const char*>(image+kSliderName));
            reinterpret_cast<void(__fastcall*)(void*,uint8_t)>(image+kShowHide)(slider.bytes,1);   // its own widget shown (above)
            slider_seen[i]=rt_bytes[i];slider_rows|=1u<<i;continue;}
        AddAdvancedRow(page,gr::kRtLabel+int(i),&rt_bytes[i],list.Get(),rt_defaults[i],kTagRt+i,enabled);}
    char row[320]{};sprintf_s(row,"{\"event\":\"native_rekindled_graphics\",\"page\":\"advanced\",\"built\":\"%s\",\"rows\":%u,\"first\":%u,\"slots\":%u,\"auto_detect\":%u,\"preset\":%u,\"levels\":[%u,%u,%u,%u,%u],\"greyed\":%u}\n",
        why,page_row_count,page_first,end-page_first,bytes[0x63],bytes[kStatePreset],bytes[kStateLevels],bytes[kStateLevels+3],bytes[kStateLevels+4],bytes[kStateLevels+7],bytes[kStateLevels+9],greyed);Log(row);}
// I-0132 R12g (Thomas, 2026-11-11 15:19: « il n'y a pas de barre de défilement sur la droite? »): the film's scroll bar over our list. The
// game's list sees only the fourteen rows we build, so it hides its bar (FUN_140A41A40: shown only when its rows exceed its slots); we
// show it (ShowHideWindow on the bar's object, the list's +0x180) and place its thumb (the list's +0x238, the clip `Bar`) with the game's
// own setters (FUN_140A3D640: x and y in pixels; FUN_140A3D6A0: x and y scales, 1 the whole track), from the track the game read from
// the film (the list's +0x2EC its top, +0x2F0 its x, +0x2F4 its length; FUN_140A52F20). The list's own scroll fields (+0x2F8 to +0x300)
// are never written: the cursor's moves read them (the R12g Ghidra study, R12G-STATIC §3). The bar is shown and its thumb placed at the
// page's next update after each build (the game's bind hides it and places the thumb full length: the second step review's M1; its plain
// cursor moves do not, `r12g-rows2`'s stills) and when the window or the list changes.
constexpr uintptr_t kWidgetPosition=0xA3D640,kWidgetScale=0xA3D6A0;
// The mouse wheel (Thomas, 2026-11-17): the wheel's delta the game window received (WM_MOUSEWHEEL, counted by NativeInputLatchHook,
// 17_render_step.inc), taken by our page's update (RekindledWindowUpdate) and emptied when the page opens.
std::atomic<int32_t> rekindled_wheel_delta{};int32_t rekindled_wheel_rest{};   // the menu task's: a delta under a notch, kept for the next update (the step review's L2: emptied with the delta)
uint64_t scroll_bar_key{UINT64_MAX};   // the menu task's: the page, the window's first row and the list's size the thumb was set for
void ShowRekindledScrollBar(uintptr_t page) noexcept {
    const uintptr_t list=page+0x9D8;
    if(page_row_count<=gr::kPageSlots){scroll_bar_key=UINT64_MAX;return;}   // the game's own state (hidden) stands
    const uint64_t key=(uint64_t(page)<<16)^(uint64_t(page_first)<<8)^page_row_count;
    if(key==scroll_bar_key)return;   // the final review's L2: shown and placed after each build only (only our binds hide it)
    reinterpret_cast<void(__fastcall*)(uintptr_t,uint8_t)>(image+kShowHide)(list+0x180,1);
    float y0{},x{},travel{};
    if(!Read(list+0x2EC,&y0,4) || !Read(list+0x2F0,&x,4) || !Read(list+0x2F4,&travel,4) || !std::isfinite(y0) || !std::isfinite(x) || !std::isfinite(travel) || !(travel>0))return;
    const auto thumb=gr::ScrollThumb(page_first,page_row_count,y0,travel);
    reinterpret_cast<void(__fastcall*)(uintptr_t,int32_t,int32_t)>(image+kWidgetPosition)(list+0x238,int32_t(x),thumb.y);
    reinterpret_cast<void(__fastcall*)(uintptr_t,float,float)>(image+kWidgetScale)(list+0x238,1.0f,thumb.yscale);
    scroll_bar_key=key;   // the second step review's L2: kept once placed (a track not read yet is tried again at the next update)
    char row[160]{};sprintf_s(row,"{\"event\":\"native_rekindled_graphics\",\"scroll_bar\":{\"first\":%u,\"rows\":%u,\"y\":%d,\"yscale\":%.4f,\"track\":[%g,%g]}}\n",
        page_first,page_row_count,thumb.y,double(thumb.yscale),double(y0),double(travel));Log(row);}
void __fastcall RekindledAdvancedPage(uintptr_t page,uintptr_t state) noexcept {
    if(!rekindled_hooks_installed.load() || state!=page+0x10F0){advanced_rows_original(page,state);return;}
    rekindled_advanced_page.store(0);rekindled_defaults_skip.store(page);rebuild_pending.store(false);page_settings=NativeSettingsCurrent();page_first=0;scroll_bar_key=UINT64_MAX;rekindled_wheel_delta.store(0);rekindled_wheel_rest=0;
    page_preset=reinterpret_cast<const uint8_t*>(state)[kStatePreset];
    if(page_settings.dlss!=ds3native::settings::Dlss::Off && page_settings.dlss!=ds3native::settings::Dlss::Dlaa)page_dlss_mode=page_settings.dlss;
    BuildAdvancedRows(page,state,page_settings,"open");rekindled_rows_begin.store(*reinterpret_cast<uintptr_t*>(page+0x1070));
    rekindled_advanced_page.store(page);}   // after its rows (R12c's last step review: an update re-entered during the build never clears it)
// Our page: its address, the sub-page's class (CreateOptionSubPageByName's vtable 1428C7558) and the rows' list our build made (the
// final review's L1: a freed page's address reused by another sub-page has another list).
bool IsRekindledAdvancedPage(uintptr_t window) noexcept {
    uintptr_t vtable{},begin{};
    return window && window==rekindled_advanced_page.load() && Read(window,&vtable,8) && vtable==image+0x28C7558 && Read(window+0x1070,&begin,8) &&
        begin==rekindled_rows_begin.load();}
// The rows built again (Thomas's ask above): at the page's next update, before any of its rows runs (the generic window update
// FUN_140A4CA90(window, ...) entered for our page; a row's own change is never on the stack then), the rows' list (page+0x1070, entries
// of 0x80 bytes) emptied as the page's destructor empties it (FUN_140A90390: each row released, its label destroyed), our rows built in
// the same slots (a row takes the slot of the list's size, FUN_140A90E20), then bound again with the cursor where it was
// (FUN_140A913B0(page, the cursor), as CreateOptionSubPageByName binds them at 0; the cursor at page+0x9D8+0xB4, FUN_140A42AD0).
// Its second argument is the frame's time in XMM1 (its prologue keeps XMM1 in XMM6): a float here, forwarded as one; RDX, the second
// integer register, is never read by it (its prologue saves RCX in RSI and R8 in RBX only; the final review's L5).
using WindowUpdateFn=void(__fastcall*)(uintptr_t,float,uintptr_t,uintptr_t);WindowUpdateFn window_update_original{};
// The page's rows built again with the window from `first` and the cursor on the list's row `row` (the window moved to keep it, the
// step review's M1; the cursor clamped to the rows built): the rows' list (page+0x1070, entries of 0x80 bytes) emptied as the page's
// destructor empties it, our rows built in the same slots, bound again with the cursor (below).
void RebuildAdvancedRows(uintptr_t window,uint32_t first,uint32_t row_kept,const char* why) noexcept {
    uintptr_t* const rows=reinterpret_cast<uintptr_t*>(window+0x1070);
    reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t)>(image+0xA90390)(window+0x1070,rows[0],rows[1]);rows[1]=rows[0];
    page_first=first;scroll_bar_key=UINT64_MAX;BuildAdvancedRows(window,window+0x10F0,page_settings,why,int32_t(row_kept));   // the bar placed again after the bind
    const int32_t built=int32_t((rows[1]-rows[0])>>7);int32_t cursor=int32_t(row_kept)-int32_t(page_first);cursor=(std::clamp)(cursor,0,(std::max)(built-1,0));
    reinterpret_cast<void(__fastcall*)(uintptr_t,int32_t)>(image+0xA913B0)(window,cursor);rekindled_rows_begin.store(rows[0]);
    char row[128]{};sprintf_s(row,"{\"event\":\"native_rekindled_graphics\",\"rebuilt_cursor\":%d,\"first\":%u}\n",cursor,page_first);Log(row);}
// The faults first put on the live rebuild and on the slider (the runs r12g-pages4 to r12g-pages6, r12g-wrap1, r12g-wrap2) were the
// bounded runs' stop arriving with a menu open (the RCA, I-0132 RESULT « R12c »: the known close fault at +0xE6037D, the native HAL
// withdrawn by Close while the menu re-imports its Status face), each at its run's deadline; the rebuild itself was survived.
void __fastcall RekindledWindowUpdate(uintptr_t window,float dt,uintptr_t input,uintptr_t r9) noexcept {
    // The page's class checked too (CreateOptionSubPageByName's vtable 1428C7558): a freed page's address reused by another window is
    // never emptied.
    const bool ours=IsRekindledAdvancedPage(window);
    // I-0138 tranche 5b: the Graphics tab, its rows built again before its update when the HDR output was switched, its sliders
    // read after it (StepGraphicsTab); its address under another window forgets it.
    const bool tab=IsRekindledGraphicsTab(window);
    if(!tab && window && window==rekindled_graphics_tab.load() && tab_begin_known){rekindled_graphics_tab.store(0);tab_rebuild_pending.store(false);tab_peak={};tab_paper={};}
    if(tab && tab_rebuild_pending.exchange(false))RebuildGraphicsTab(window);
    // The options' Brightness page: its row built again when the chain's kind changed since it was built (above).
    if(IsBrightnessPage(window)){if(const bool locked=native_hdr10_chain.load();locked!=brightness_built_locked)RebuildBrightnessPage(window,locked);}
    else if(window && window==brightness_page.load() && brightness_begin_known)brightness_page.store(0);   // its address, another window now
    if(ours){
        for(uint32_t s=0;s<gr::kRtRows;++s)if((slider_rows>>s)&1u && rt_bytes[s]!=slider_seen[s]){slider_seen[s]=rt_bytes[s];RtRowChanged(s);   // a slider's value, seen changed
            char row[112]{};sprintf_s(row,"{\"event\":\"native_rekindled_graphics\",\"slider\":%u,\"row\":%u}\n",rt_bytes[s],s);Log(row);}
        KeepChosenPreset(window);}
    else if(window && window==rekindled_advanced_page.load()){   // our page's address, another window now (the final review's L10)
        rekindled_advanced_page.store(0);rebuild_pending.store(false);slider_rows=0;}
    if(ours && rebuild_pending.exchange(false)){
        // R12g: the cursor kept on its row of the list (a row that appears or goes is below the governing row; the step review's M1: the
        // window moved to keep it, not the slot).
        const int32_t cursor=*reinterpret_cast<int32_t*>(window+0x9D8+0xB4);
        RebuildAdvancedRows(window,page_first,page_first+uint32_t((std::max)(cursor,0)),"rebuilt");}
    const int32_t before=ours?*reinterpret_cast<const int32_t*>(window+0x9D8+0xB4):-1;
    window_update_original(window,dt,input,r9);
    if(tab && IsRekindledGraphicsTab(window))StepGraphicsTab(window);
    // I-0132 R12g: our scroll. The game's update moved the cursor within the fourteen slots; a wrap past the window's edge moves the
    // window instead (built again here, after the game's update: no row of the page is on the stack, as at its entry above).
    if(ours && IsRekindledAdvancedPage(window)){const int32_t after=*reinterpret_cast<const int32_t*>(window+0x9D8+0xB4);
        if(const auto moved=gr::ScrollAfterMove(page_enabled,page_row_count,page_first,before,after)){
            char row[128]{};sprintf_s(row,"{\"event\":\"native_rekindled_graphics\",\"scrolled\":{\"from\":%u,\"to\":%u,\"cursor\":%u}}\n",page_first,moved->first,moved->cursor);Log(row);
            RebuildAdvancedRows(window,moved->first,moved->first+moved->cursor,"scrolled");}
        // The mouse wheel: a notch a row (a wheel delta of 120; forward, a positive delta, scrolls up), the rest kept for the next update.
        else{const int32_t delta=rekindled_wheel_delta.exchange(0)+rekindled_wheel_rest;rekindled_wheel_rest=delta%120;
            if(const auto wheel=gr::ScrollByWheel(page_enabled,page_row_count,page_first,after,-(delta/120))){
                char row[128]{};sprintf_s(row,"{\"event\":\"native_rekindled_graphics\",\"wheel\":{\"from\":%u,\"to\":%u,\"cursor\":%u}}\n",page_first,wheel->first,wheel->cursor);Log(row);
                RebuildAdvancedRows(window,wheel->first,wheel->first+wheel->cursor,"wheel");}}
        ShowRekindledScrollBar(window);}}

// Each engine frame (game thread): the rows' changes carried into the settings file and the window, Auto-detect kept OFF, and
// GraphicsConfig.xml written by the game (FUN_140EF5AB0, from +0x68) once the window applied every requested field (FUN_140EF6760
// copies the 0x48 bytes).
constexpr uint32_t kAutoDetect=0x14;
void StepRekindledGraphics() noexcept {
    if(!rekindled_hooks_installed.load() || !RekindledOnGameThread())return;
    if(const int32_t mode=pending_screen_mode.exchange(-1);mode>=0)WriteScreenModeValue(uint8_t(mode));
    // The pending rate cleared only once written (R12c's last step review: the row reads it until then), a newer choice kept.
    if(int32_t rate=pending_frame_rate.load();rate>=0){const auto value=gr::FrameRateOf(uint8_t(rate));
        bool ok=false;if(value)ok=EditNativeSettings([&](ds3native::settings::Settings& s){s.frame_rate=*value;});
        pending_frame_rate.compare_exchange_strong(rate,-1);if(value){
        char row[128]{};sprintf_s(row,"{\"event\":\"native_rekindled_graphics\",\"frame_rate\":%.0f,\"saved\":%s}\n",double(*value),ok?"true":"false");Log(row);}}
    static int vsync_tried=-1;   // M1 (the step review): one write per value the row takes, a refused write not tried each frame
    if(vsync_row_built.load()){const bool on=std::atomic_ref<uint8_t>(vsync_byte).load(std::memory_order_relaxed)!=0;
        if(int(on)!=vsync_tried && (vsync_tried=int(on),on!=NativeSettingsCurrent().vsync)){const bool ok=EditNativeSettings([&](ds3native::settings::Settings& s){s.vsync=on;});
            char row[128]{};sprintf_s(row,"{\"event\":\"native_rekindled_graphics\",\"vsync\":%s,\"saved\":%s}\n",on?"true":"false",ok?"true":"false");Log(row);}}
    if(const uint32_t changed=rt_rows_changed.exchange(0)){ds3native::settings::Settings page{};{const std::lock_guard hold(rt_snapshot_lock);page=rt_page_snapshot;}
        const bool ok=EditNativeSettings([&](ds3native::settings::Settings& s){s.level=page.level;s.bounces=page.bounces;s.dlss=page.dlss;s.rr=page.rr;s.reflex=page.reflex;
            s.frame_generation=page.frame_generation;s.taa=page.taa;s.sharpen=page.sharpen;s.vignette=page.vignette;
            s.look=page.look;s.aberration=page.aberration;s.bloom=page.bloom;s.shafts=page.shafts;s.grain=page.grain;s.fov=page.fov;});   // I-0138 tranche 7
        char row[640]{};sprintf_s(row,"{\"event\":\"native_rekindled_graphics\",\"rt_rows\":%u,\"saved\":%s,\"settings\":{\"level\":\"%s\",\"dlss\":\"%s\",\"taa\":%s,\"sharpen\":%.2f,\"rr\":%s,\"fg\":%u,\"reflex\":\"%s\",\"bounces\":%u,\"vignette\":%s,"
            "\"look\":\"%s\",\"aberration\":%s,\"bloom\":%u,\"shafts\":%s,\"grain\":%u,\"fov\":%u}}\n",
            changed,ok?"true":"false",ds3native::settings::Name(page.level),ds3native::settings::Name(page.dlss),page.taa?"true":"false",double(page.sharpen),page.rr?"true":"false",
            page.frame_generation,ds3native::settings::Name(page.reflex),page.bounces,page.vignette?"true":"false",
            ds3native::settings::Name(page.look),page.aberration?"true":"false",page.bloom,page.shafts?"true":"false",page.grain,page.fov);Log(row);}
    // I-0138 tranche 5b: the Graphics tab's HDR rows (the HDR10 chain follows the settings live, FollowNativeLiveSettings).
    if(uint64_t due=tab_hdr_due.load();due && GetTickCount64()>=due && tab_hdr_due.compare_exchange_strong(due,0)){   // a newer change wins the exchange
        HdrFields tab{};{const std::lock_guard hold(rt_snapshot_lock);tab=tab_hdr_snapshot;}
        const bool ok=EditNativeSettings([&](ds3native::settings::Settings& s){s.hdr=tab.hdr;s.hdr_peak=tab.peak;s.hdr_paper_white=tab.paper_white;});
        char row[192]{};sprintf_s(row,"{\"event\":\"native_rekindled_graphics\",\"hdr_rows\":{\"saved\":%s,\"hdr\":%s,\"hdr_peak\":%u,\"hdr_paper_white\":%u}}\n",
            ok?"true":"false",tab.hdr?"true":"false",tab.peak,tab.paper_white);Log(row);}
    const uintptr_t w=GameWindow();if(!w)return;
    if(const int32_t preset=pending_preset.exchange(-1);preset>=1 && preset<=4){*reinterpret_cast<int32_t*>(w+0xB0+0x18)=preset;RequestGraphicsConfigSave();
        char row[96]{};sprintf_s(row,"{\"event\":\"native_rekindled_graphics\",\"preset_kept\":%d}\n",preset);Log(row);}
    int32_t auto_detect{};   // Auto-detect kept OFF (Thomas, 2026-10-31 12:39 and 2026-11-10 20:40; options sheet G §7a)
    if(Read(w+0xB0+kAutoDetect,&auto_detect,4) && auto_detect!=0){*reinterpret_cast<int32_t*>(w+0xB0+kAutoDetect)=0;RequestGraphicsConfigSave();
        Log("{\"event\":\"native_rekindled_graphics\",\"edge\":\"auto_detect_off\"}\n");}
    int32_t antialiasing{};   // the game's Antialiasing kept On (above, kStateAntialiasing): its level at +0x1C + 4
    if(Read(w+0xB0+0x20,&antialiasing,4) && antialiasing==0){*reinterpret_cast<int32_t*>(w+0xB0+0x20)=1;RequestGraphicsConfigSave();
        Log("{\"event\":\"native_rekindled_graphics\",\"edge\":\"antialiasing_on\"}\n");}
    if(!rekindled_save_pending)return;
    uint8_t applied[0x48]{},requested[0x48]{};
    if(!Read(w+0x68,applied,sizeof(applied)) || !Read(w+0xB0,requested,sizeof(requested)) || std::memcmp(applied,requested,sizeof(applied)))return;
    rekindled_save_pending=false;
    reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0xEF5AB0)(w);
    Log("{\"event\":\"native_rekindled_graphics\",\"edge\":\"graphics_config_saved\"}\n");}
}
