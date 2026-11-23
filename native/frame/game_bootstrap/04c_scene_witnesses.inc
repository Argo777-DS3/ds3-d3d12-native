// Reference and diagnostic witnesses, third part: AO passes, emits and instance records, the sun block, probes, the pop-in
// and LOD observers, light recording, lightmaps, geometry draws, velocity, model rows and pose rings. Part of game_bootstrap.cpp's single translation unit (the order is there): it relies on every earlier part
// (04b_pass_witnesses.inc before it).
#pragma once
#include "04b_pass_witnesses.inc"
namespace {
// Words as rows: their FNV-1a digest and their hexadecimal list.
uint64_t NativeWordsDigest(const uint32_t* words,size_t count) noexcept {
    uint64_t digest=1469598103934665603ull;
    for(size_t i=0;i<count;++i)for(int b=0;b<4;++b){digest^=(words[i]>>(8*b))&0xFF;digest*=1099511628211ull;}
    return digest;}
std::string NativeWordsList(const uint32_t* words,size_t count){std::string out;
    for(size_t i=0;i<count;++i){char cell[16]{};sprintf_s(cell,"%s\"%08X\"",i?",":"",words[i]);out+=cell;}return out;}
// Rows SS1-SS3 (W5c AO-4, with shadow-observe.flag): FUN_1401B26E0 (the AO passes of a layer: argument 1 the layer, argument 3
// the view record of 1401B1960): the layer's record +0x15C..+0x19F, its counter +0x224 before and after, the record's validity
// +0x60 and strength +0x64, and the quality bytes +0x09..+0x0F (the observed original; the native SSAO is removed, I-0126 R7c).
using AoPasses=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t);
AoPasses ao_passes_original{};
void __fastcall ObserveAoPasses(uintptr_t layer,uintptr_t pass,uintptr_t holder) noexcept {
    static volatile LONG64 calls{};const auto n=InterlockedIncrement64(&calls);const bool log=n<=12 || n%300==0;
    std::array<uint8_t,0x44> params{};uint32_t before{},after{};uintptr_t record{},root{},quality{};uint8_t valid{};float strength{};std::array<uint8_t,7> q{};
    if(log){Read(layer+0x15C,params.data(),params.size());Read(layer+0x224,&before,4);
        if(Read(holder,&record,8) && record){Read(record+0x60,&valid,1);Read(record+0x64,&strength,4);}
        if(Read(image+0x4961B50,&root,8) && root && Read(root+0x5360,&quality,8) && quality)Read(quality+0x09,q.data(),q.size());}
    ao_passes_original(layer,pass,holder);
    if(log){Read(layer+0x224,&after,4);
        const std::string row="{\"event\":\"reference_ao_passes\",\"call\":"+std::to_string(n)+",\"params\":\""+HexBytes(params.data(),params.size())+
            "\",\"counter\":["+std::to_string(before)+","+std::to_string(after)+"],\"valid\":"+std::to_string(valid)+",\"strength\":"+std::to_string(strength)+
            ",\"quality\":\""+HexBytes(q.data(),q.size())+"\"}\n";Log(row.c_str());}
}
// Row SL1 (W5a SW-7): FUN_14010A220 (the light recording of a view, third argument the view context the native collect reads)
// and its local-shadow gate context+0x78 == 1 && context+0x3EBF && [[GXRS+0x5360]+0x44], with the pipeline words +0x7C, +0x3EBC
// and +0x2DA8 of SHADOWS-WAVE1-READY 2.1. Bounded: the first 32 calls, then every 300th.
using LightRecording=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
LightRecording light_recording_original{};
// D-0180 C4 (image regression): the original's sun blocks (1496D0: out 0x120 bytes, set, view), light-observe.flag only.
using SunBlockBuild=void(__fastcall*)(void*,uintptr_t,const void*);
SunBlockBuild sun_block_original{};
// I-0141 tranche 18 (m32's 143 models only the native draws): reference-no-occlusion.flag, a witness for Reference runs alone.
// UpdateOcclusionCellStates 1400CA050 (view state, camera) sets each occlusion node's state from the previous frame's queries; its own
// first branch, taken when the view state's byte +0x98 (or +0x99) is clear, sets every node of +0x118..+0x120 to 4 (visible) and,
// with +0x98 clear, returns (READ, GhidraMCP 2026-11-21, evidence/I-0141/artifacts/tranche-18/occlusion-reading.md). The hook clears
// +0x98 for the call and puts it back. READ: the branch runs 14020D240 on the cell hierarchy (+0x90), the root's state +0x78 to 7
// and every cell below to 6, so 208180's cell gate (state above 2) and 142E80's node gate (state above 1) pass on every node and
// cell; and 140147830 on every node, which hands its query handles (+0x48, +0x50, the +0x58 list) back to the pool. Not run in the
// call (READ): slot 8 of the cell hierarchy with the frustum built by 14020E360, then, with +0x9A set, slot 0x60 on each node's
// bounds, which gives it state 0 (hidden by the cells) or 3 (retest); 14020D0E0, the cells' update; 140147390, which writes node
// state 1 (fewer than 11 samples) and queues the node (its owner's +0x138) for the queries pass 0x45 records (its only code caller
// CA050; one DATA reference not read); the camera-motion flag +0x15F, the camera history (+0x164..+0x170, +0x180/+0x188), +0x330
// and +0x334, +0x318/+0x140 and the frame counter +0x160 (frozen: 208180's and 142E80's counter tests are alternatives to the state
// gates). INFERRED: no node's query is recorded (CA6D0's +0x128 is the owner's +0x138 if CA6D0 gets view+0x10; who fills its
// +0x300 list not read), and slot 8 may write the cells' +0x78 too. Outside the call other readers see +0x98 at the game's value and
// +0x15C at 1 (CA050 leaves 0 only when +0x98 and +0x99 are both set; its readers not searched). The traversers' own frustum tests
// still apply. Whether the original then draws the models the native draws is the test. Rows (`qpc_ms` on the pixel-key rows'
// clock; `first`, `cleared_with_nodes`, `periodic`): the first call, the first call that clears the byte while the view has nodes
// (the first such call while CA050 runs on one thread at a time, as E00270's single call site suggests), and every 3600th call,
// with the calls and those cleared so far.
double PixelKeyClockMs() noexcept;   // below, with the pixel-key rows
using OcclusionCellUpdate=void(__fastcall*)(uintptr_t,uintptr_t);
OcclusionCellUpdate occlusion_cell_update_original{};
void __fastcall ReferenceOcclusionCellsVisible(uintptr_t view,uintptr_t camera) noexcept {
    uint8_t enabled{};const bool read=view && Read(view+0x98,&enabled,1);
    static std::atomic<bool> logged{false},logged_nodes{false};static std::atomic<uint64_t> calls{},cleared{};
    const bool clears=read && enabled;const uint64_t call=calls.fetch_add(1,std::memory_order_relaxed)+1;
    const uint64_t done=clears?cleared.fetch_add(1,std::memory_order_relaxed)+1:cleared.load(std::memory_order_relaxed);
    const bool first=!logged.exchange(true),periodic=call%3600==0;
    long long nodes=-1;   // the node list read only when a row may be written (-1 unless both ends read)
    if(first || periodic || (clears && !logged_nodes.load(std::memory_order_relaxed))){uintptr_t begin{},end{};
        if(view && Read(view+0x118,&begin,8) && Read(view+0x120,&end,8) && end>=begin)nodes=static_cast<long long>((end-begin)/8);}
    const bool first_nodes=clears && nodes>0 && !logged_nodes.exchange(true);
    if(first || first_nodes || periodic){
        char row[320]{};sprintf_s(row,"{\"event\":\"reference_no_occlusion\",\"qpc_ms\":%.1f,\"first\":%s,\"cleared_with_nodes\":%s,\"periodic\":%s,\"read\":%s,\"byte_98\":%u,\"nodes\":%lld,\"calls\":%llu,\"cleared\":%llu}\n",
            PixelKeyClockMs(),first?"true":"false",first_nodes?"true":"false",periodic?"true":"false",read?"true":"false",unsigned(enabled),nodes,
            static_cast<unsigned long long>(call),static_cast<unsigned long long>(done));Log(row);}
    if(!clears){occlusion_cell_update_original(view,camera);return;}
    *reinterpret_cast<volatile uint8_t*>(view+0x98)=0;
    occlusion_cell_update_original(view,camera);
    *reinterpret_cast<volatile uint8_t*>(view+0x98)=enabled;
}
// I-0141 tranche 27 (test tooling: reference-share-variant.flag in a bounded Reference run only; 01_state's reference_share_variant):
// the original's own shares of a cutscene's picture switched live at one held moment, as tranche 26's native ones (witness-variant.txt
// read by FollowWitnessShareVariant from the Remo Play_Wait hook; bits 1, 2, 32 and 64, the last two below).
// Bit 1, the light sets' lights 1 to 3 (the Col and Spc rows 5-7 and 9-11 of each 0x120-byte cbLight set), taken out where D-0262 takes
// them out natively: the main view's deferred array (natively the composite's block, scene_pass_lighting) and the forward draws' sets
// (natively 12_scene_collect's forward rows); the effects keep them in both modes (natively 264230's path never loses them).
//   - Deferred: 137910 builds entry i with 1496D0(out, set, view) (call 140137A7D, return 140137A82) or the blend 14A2D0(out, view, a,
//     b, t) (call 140137ABF, return 140137AC4), then uploads the array (READ, its listing): the six rows are zeroed in the entry once
//     built. 1496D0's other callers (264230 the effects, 32650's array of the same 4A3D0 step, its use not read) keep them, counted.
//   - Forward: 5A120 builds each forward draw's set with 149CB0(record, buffer, context, view) (call 14005A267) or 14AB20(buffer,
//     context, view, a, b, t) (call 14005A2BA) on 32A80's records; each computes the 0x120 bytes on its stack and uploads them itself
//     (READ, GhidraMCP 2026-11-23), so it is handed a copy of each record whose lights 1 to 3 intensities are 0 (+0xCC, +0xDC, +0x13C,
//     +0x14C, +0x1AC, +0x1BC: each row is c*c*w with w*w last, READ in both): the same zero rows. Their only callers are those (READ).
//     A record not read whole is handed as it is, counted (path 6: such a draw keeps its lights 1 to 3; the step review's L4).
//   - Every view alike (the step review's L3): 137910's second run for m37's water reflection and 5A120's sets of every view that
//     draws forward rows lose them too, where D-0262 natively acts on the main view alone; 32000010 draws no water reflection.
// Bit 2, the glare family through the library's own skip: E69A0 calls 1416C9690(_, shape), which stores the globals' glare shape at
// CPostEffect+0xBC4, each frame unless that shape is 14, the custom glare, which it hands to 1416C96B0 instead (1400E7179, READ by the
// step review: its M1). Bit 2 does not reach a custom glare (13 cutscene records, I-0138's STUDY): the reference rows carry the globals'
// shape so a variant shot under shape 14 shows (32000010: shape 1 in every clock row of i141-t27sa-orig, OBSERVED). A stored 0 skips the bright pass, the pyramid, the streaks, star, ghost, afterimage, the light shafts and the composite
// (1416E7583) and the tone map takes no glare, while the depth of field and the motion blur run before that gate (I-0138 tranche 8c,
// READ): the hook stores 0. That is what the native's bit 2 serves (13b's share_off).
using ReferenceSunBlock=void(__fastcall*)(void*,uintptr_t,const void*);
using ReferenceSunBlend=void(__fastcall*)(void*,const void*,uintptr_t,uintptr_t,float);
using ReferenceForwardSun=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,const void*);
using ReferenceForwardBlend=void(__fastcall*)(uintptr_t,uintptr_t,const void*,uintptr_t,uintptr_t,float);
using ReferenceGlareShape=uint64_t(__fastcall*)(uint64_t,uint32_t);
ReferenceSunBlock reference_sun_block_original{};
ReferenceSunBlend reference_sun_blend_original{};
ReferenceForwardSun reference_forward_sun_original{};
ReferenceForwardBlend reference_forward_blend_original{};
ReferenceGlareShape reference_glare_shape_original{};
bool ReferenceShareOff(uint32_t bit) noexcept {return (witness_share_variant.load(std::memory_order_relaxed)&bit)!=0;}
void ReferenceShareZeroFill(void* out) noexcept {for(const size_t row:{5,6,7,9,10,11})std::memset(static_cast<uint8_t*>(out)+16*row,0,16);}
void __fastcall ReferenceShareSunBlock(void* out,uintptr_t set,const void* view) noexcept {
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());
    reference_sun_block_original(out,set,view);
    if(!ReferenceShareOff(kShareNoFill))return;
    if(caller==image+0x137A82){ReferenceShareZeroFill(out);reference_share_counts[0].fetch_add(1,std::memory_order_relaxed);}
    else reference_share_counts[4].fetch_add(1,std::memory_order_relaxed);
}
void __fastcall ReferenceShareSunBlend(void* out,const void* view,uintptr_t a,uintptr_t b,float t) noexcept {
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());
    reference_sun_blend_original(out,view,a,b,t);
    if(ReferenceShareOff(kShareNoFill) && caller==image+0x137AC4){ReferenceShareZeroFill(out);reference_share_counts[1].fetch_add(1,std::memory_order_relaxed);}
}
// A record's copy with lights 1 to 3's intensities 0; false (the record handed as it is) when it cannot be read whole. 149CB0 and
// 14AB20 read the record up to +0x28E (READ); 0x2A0 bytes copied.
struct alignas(16) ReferenceShareRecord {std::array<uint8_t,0x2A0> bytes;};
bool ReferenceShareCopy(uintptr_t record,ReferenceShareRecord& copy) noexcept {
    if(!record || !Read(record,copy.bytes.data(),copy.bytes.size()))return false;
    for(const size_t at:{0xCC,0xDC,0x13C,0x14C,0x1AC,0x1BC})std::memset(copy.bytes.data()+at,0,4);
    return true;
}
void __fastcall ReferenceShareForwardSun(uintptr_t record,uintptr_t buffer,uintptr_t context,const void* view) noexcept {
    ReferenceShareRecord copy;
    if(ReferenceShareOff(kShareNoFill) && ReferenceShareCopy(record,copy)){reference_share_counts[2].fetch_add(1,std::memory_order_relaxed);
        reference_forward_sun_original(reinterpret_cast<uintptr_t>(copy.bytes.data()),buffer,context,view);return;}
    if(ReferenceShareOff(kShareNoFill))reference_share_counts[6].fetch_add(1,std::memory_order_relaxed);
    reference_forward_sun_original(record,buffer,context,view);
}
void __fastcall ReferenceShareForwardBlend(uintptr_t buffer,uintptr_t context,const void* view,uintptr_t a,uintptr_t b,float t) noexcept {
    ReferenceShareRecord copy_a,copy_b;
    if(ReferenceShareOff(kShareNoFill) && ReferenceShareCopy(a,copy_a) && ReferenceShareCopy(b,copy_b)){reference_share_counts[3].fetch_add(1,std::memory_order_relaxed);
        reference_forward_blend_original(buffer,context,view,reinterpret_cast<uintptr_t>(copy_a.bytes.data()),reinterpret_cast<uintptr_t>(copy_b.bytes.data()),t);return;}
    if(ReferenceShareOff(kShareNoFill))reference_share_counts[6].fetch_add(1,std::memory_order_relaxed);
    reference_forward_blend_original(buffer,context,view,a,b,t);
}
uint64_t __fastcall ReferenceShareGlareShape(uint64_t self,uint32_t shape) noexcept {
    if(ReferenceShareOff(kShareNoGlare)){reference_share_counts[5].fetch_add(1,std::memory_order_relaxed);shape=0;}
    return reference_glare_shape_original(self,shape);
}
// I-0141 tranche 27, the ambient (bits 32 and 64; the pilot's reading of t27-original-ambient-reading.md, GhidraMCP 2026-11-23):
// the original's ambient is not in cbLight for a deferred character: its Ibl G-buffer programmes (486 of 676, SkinSSSIbl, Dragon_FDP)
// write GBuffer4 = (1 - F0) * max(0, (N, 1) . aSh[3]), aSh the instance record's rows +0x80..+0xAF (cbInstanceData, READ in their
// asm by the reading), and GXLightDeferredAO passes GB4 through (t6, no constant buffer). 5B3D0 (BuildVSModelParams; callers 5CEC0 at
// 14005D0B1, which uploads the 0x110-byte record right after the call, and 5D5C0 at 14005D93F) stores those rows at param_8[0x10..0x15]
// for the pass kinds *param_6 = 0, 3, 4, from one of three sources (READ, the pilot's decompile), and the forward programmes' own cube
// scales envMapColorScale0 and 1 at +0xEC and +0xFC (any kind). Bit 32 zeroes the 0x30 bytes once the record is built (the sun mask
// +0xB0..+0xBF and cbLight untouched); bit 64 the two cube scales, and the deferred specular IBL (SpecularAcc, t7): the region pass
// 675B0(this, context, regions, count) is handed count 0, its draw loop guarded by count (READ by the reading; R9D kept in R13, READ).
// Not reached (the reading's gaps): the lightmapped map pieces' t26 ambient (FC_Dominant*, its CPU writer not found) and the decals'
// GB4 writes; 5D5C0's use of the record after the call (INFERRED: uploaded after it, as 5CEC0's); which of the pass kinds 0, 3 and 4
// are forward passes (not tied). Counts: 7 records' SH zeroed, 8 their cube scales zeroed, 9 region passes handed count 0. Natively bit
// 32 is the traced diffuse ambient, 64 SpecularAcc, the main view's deferred surfaces alone (01_state.inc); here every view the hooks
// reach (the second step review's M1): bit 64's cube scales leave every draw's record, forward ones included (any kind, READ); bit 32's
// SH leaves the kinds 0, 3 and 4, forward draws among them only if those kinds include them (not tied). Natively a forward draw keeps
// its traced probe's SH (ApplyNativeRtProbe) and its cubes. A pair's share is read on the deferred surfaces (the region reading), not
// from the frame's mean alone.
using ReferenceInstanceParams=void(__fastcall*)(uint32_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,const int32_t*,uint32_t,uintptr_t,uintptr_t,uintptr_t,char);
using ReferenceEnvRegions=uint64_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uint32_t);
ReferenceInstanceParams reference_instance_params_original{};
ReferenceEnvRegions reference_env_regions_original{};
// I-0141 tranche 28 (bit 512, the census; no picture changed): which of 5B3D0's three sources gave a draw its instance SH, and the rows
// it wrote, once per (model, pass kind 0, 3 or 4) a census, at most 800 rows a census and 4000 a run. 5B3D0 (the pilot's decompile,
// GhidraMCP 2026-11-23, READ): with s = [group+0x100] (the draw's light-set holder) and q = [group+0x68] (its animation block), the
// source is 0 the global override when [[GXRS+0x4DD0]+0x13E8] != 0 (its rows [GXRS+0x4DD0]+0x190..+0x1BF), else 1 q's own rows
// q+0x130..+0x15F when q is neither 0 nor -0xF0 and [q+0x170] != 0 (its strength q+0x16C goes to the sun mask, not the rows), else 2
// the light-set records of the keys s+0xF8 and s+0x100 (32A80 on [GXRS+0x41E0], rows +0x290..+0x2BF) blended by s+0x108; each row's
// w then times the view's +0x110, +0x114, +0x118 (logged: `scale`). The record's +0x80..+0xBF (aSh[3] and ShLightMask) logged as written
// (a NaN or an infinity kept in the hex: the reader marks the row).
// A row is the first draw of that model name the census meets (the step review's L4: the SH is the draw group's, so other instances of
// the name, with other holders or blocks, are not listed; the first depends on the threads' order). `source` is the choice the same
// inputs make when read again after the call (the second step review's L3: not the original's own read; the update thread may change
// the override byte or q+0x170 in between, INFERRED rare); -1 when GXRS or its +0x4DD0 object is null or the override's, the group's or
// q's read failed (the third step review's L2: a null s under source 2 and a failed key, blend or scale read leave 0, unmarked);
// the blend, the strength and the scales, when not finite, are written 0 and the row marked `nonfinite`. While a census is open every
// kind 0, 3 or 4 draw pays the name's read and one lock (test tooling: no picture changed, the frame's time may be).
void ReferenceShareCensus(uintptr_t view,uintptr_t group,const int32_t* pass,uintptr_t record) noexcept {
    // The step reviews' L2 and L1: once this census is full (or the run's cap reached), nothing more is read on the original's threads
    // (the third step review's L1: the flags are set by the first call that finds the cap reached, so each thread in flight then still
    // reads one kind and one name).
    static std::atomic<uint32_t> full_epoch{UINT32_MAX};static std::atomic<bool> run_full{};
    const uint32_t current=reference_census_epoch.load();if(run_full.load(std::memory_order_relaxed) || full_epoch.load(std::memory_order_relaxed)==current)return;
    int32_t kind{-1};if(!pass || !Read(reinterpret_cast<uintptr_t>(pass),&kind,4) || !(kind==0 || uint32_t(kind)-3u<2u))return;
    static std::mutex lock;static std::set<std::pair<std::string,int32_t>> seen;static uint32_t epoch{};static int rows{},total{};
    uintptr_t model{};Read(reinterpret_cast<uintptr_t>(pass)+0x1B0,&model,8);
    const std::string name=model?NativeModelDataName(model):std::string("?");
    uint32_t census{};   // the step review's L1: the census this row was counted in, printed as it
    {const std::lock_guard held(lock);census=reference_census_epoch.load();
        if(census!=epoch){epoch=census;seen.clear();rows=0;}
        if(rows>=800){full_epoch.store(census);return;}
        if(total>=4000){run_full.store(true);return;}
        if(!seen.insert({name,kind}).second)return;
        ++rows;++total;}
    uintptr_t gxrs{},global{},q{},s{};uint8_t override_on{},q_on{};uint64_t keys[2]{};float blend{},scale[3]{},strength{};
    const bool override_read=Read(image+0x4961B50,&gxrs,8) && gxrs && Read(gxrs+0x4DD0,&global,8) && global && Read(global+0x13E8,&override_on,1);
    const bool group_read=Read(group+0x68,&q,8) && Read(group+0x100,&s,8);
    const bool q_listed=q && q!=uintptr_t(-0xF0);const bool q_read=!q_listed || Read(q+0x170,&q_on,1);
    const bool q_valid=q_listed && q_on;if(q_valid)Read(q+0x16C,&strength,4);
    if(s){Read(s+0xF8,&keys[0],8);Read(s+0x100,&keys[1],8);Read(s+0x108,&blend,4);}
    Read(view+0x110,scale,12);
    const int source=!override_read?-1:override_on?0:!group_read || !q_read?-1:q_valid?1:2;
    std::array<uint8_t,0x40> sh{};const bool sh_read=Read(record+0x80,sh.data(),sh.size());
    bool nonfinite{};
    for(float* f:{&blend,&strength,&scale[0],&scale[1],&scale[2]})if(!std::isfinite(*f)){*f=0;nonfinite=true;}
    std::string clean;for(const auto ch:name.substr(0,80))clean.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');
    char row[640]{};sprintf_s(row,"{\"event\":\"reference_sh_census\",\"epoch\":%u,\"model\":\"%s\",\"kind\":%d,\"source\":%d,\"keys\":[\"%016llx\",\"%016llx\"],"
        "\"blend\":%.4g,\"q_strength\":%.4g,\"scale\":[%.4g,%.4g,%.4g],\"sh\":\"%s\",\"nonfinite\":%s}\n",census,clean.c_str(),kind,source,
        static_cast<unsigned long long>(keys[0]),static_cast<unsigned long long>(keys[1]),blend,strength,scale[0],scale[1],scale[2],sh_read?HexBytes(sh.data(),sh.size()).c_str():"",
        nonfinite?"true":"false");
    Log(row);
}
void __fastcall ReferenceShareInstanceParams(uint32_t k,uintptr_t a2,uintptr_t view,uintptr_t dc,uintptr_t group,const int32_t* pass,uint32_t a7,
    uintptr_t record,uintptr_t a9,uintptr_t a10,char a11) noexcept {
    reference_instance_params_original(k,a2,view,dc,group,pass,a7,record,a9,a10,a11);
    const uint32_t bits=witness_share_variant.load(std::memory_order_relaxed);
    if(record && (bits&kShareCensus))ReferenceShareCensus(view,group,pass,record);   // tranche 28: before any zeroing below
    if(!record || !(bits&(kShareNoGi|kShareNoSpecular)))return;
    if(bits&kShareNoGi){int32_t kind{-1};
        if(pass && Read(reinterpret_cast<uintptr_t>(pass),&kind,4) && (kind==0 || uint32_t(kind)-3u<2u)){
            std::memset(reinterpret_cast<void*>(record+0x80),0,0x30);reference_share_counts[7].fetch_add(1,std::memory_order_relaxed);}}
    if(bits&kShareNoSpecular){*reinterpret_cast<float*>(record+0xEC)=0.f;*reinterpret_cast<float*>(record+0xFC)=0.f;reference_share_counts[8].fetch_add(1,std::memory_order_relaxed);}
}
uint64_t __fastcall ReferenceShareEnvRegions(uintptr_t self,uintptr_t context,uintptr_t regions,uint32_t count) noexcept {
    if(count && ReferenceShareOff(kShareNoSpecular)){reference_share_counts[9].fetch_add(1,std::memory_order_relaxed);count=0;}
    return reference_env_regions_original(self,context,regions,count);
}
// I-0140 tranche 39 (aa-observe.flag, a witness for Reference runs alone): does the original run its GX anti-aliasing filter
// 1B9790 on the menu views' targets (the Status body 560x768, the portraits 512x256), as FB890's enqueue reads (ghidra-research
// 2026-11-22: the menu views' pipelines keep +0x3EBA at 1)? 1B9790 (filter, context, settings), READ in
// decompiled/DS3-1152-F-001B9790.c and .asm.txt: settings+0 the mode, settings+8 the target object, whose +0x10 object holds the
// width +0x50 and the height +0x54. Its first test (`gate`): filter+9 (Enable); then with GXRS = [144961B50] or its quality object
// [GXRS+0x5360] null it runs (with the constant factor at 143D7EE10, not read here); else the quality byte +8 != 0 and the factor
// [[GXRS+0x5368]+0x98] read unguarded, compared by COMISS 0 against it, JNC skipping: a factor <= 0 skips, a NaN runs. After it the
// function draws only with mode < 4, the program objects filter+0x60 and filter+0x68+8*mode giving non-null programmes through their
// vtable +0x10, and a non-null +0x10 object of the target; `ready` here is mode < 4 and those two object pointers and the +0x10
// object non-null (the vtable calls not made: a null programme behind a present object is not seen). So `gate` and `ready` both
// true says the draw is reached up to the programmes' own lookups. `failed` names the first value that could not be read (a value
// decided before it is kept, one not yet decided is null, never guessed); fields not read are null. Rows `reference_aa_filter`:
// one for each new key (width, height, mode, enable, gate, ready; a failed key: failed, enable, gate, ready, mode), at most 64 keys (then the key « overflow »), with the GetTickCount64 tick (the Status
// row's clock); and, at most every 2 s of that clock while the filter is called, one with each key's calls so far (the calls of the
// last 2 s before the game ends are not in a totals row; a Status visit's are if the filter is called again 2 s or more after the
// visit, which the world view's call each frame should give, INFERRED).
using AaFilterRun=void(__fastcall*)(uintptr_t,uintptr_t,uint32_t*);
AaFilterRun aa_filter_run_original{};
void __fastcall ReferenceAaFilterObserved(uintptr_t filter,uintptr_t context,uint32_t* settings) noexcept {
    const char* failed{};int gate=-1,ready=-1;   // -1: not decided (a read failed)
    uint8_t enable{};uint32_t mode{},width{},height{};bool mode_read{},size_read{},quality_read{},factor_read{};uint8_t quality_byte{};float factor{};
    uintptr_t target{},inner{},gxrs{},quality{},factor_block{},program{},mode_program{};
    if(!filter || !Read(filter+9,&enable,1))failed="enable";
    else if(!enable)gate=0;
    else if(!Read(image+0x4961B50,&gxrs,8))failed="gxrs";
    else if(!gxrs)gate=1;
    else if(!Read(gxrs+0x5360,&quality,8))failed="quality_object";
    else if(!quality)gate=1;
    else if(!Read(quality+8,&quality_byte,1))failed="quality_byte";
    else if(quality_read=true;!quality_byte)gate=0;
    else if(!Read(gxrs+0x5368,&factor_block,8) || !factor_block || !Read(factor_block+0x98,&factor,4))failed="factor";
    else {factor_read=true;gate=factor>0.f || std::isnan(factor)?1:0;}   // COMISS 0, factor; JNC skips: a NaN runs
    const auto s=reinterpret_cast<uintptr_t>(settings);
    // The function's own decision after its first test (it reads +0x60 before the mode, then tests the mode, the two programmes,
    // then the target): mode, the two programme objects, then the target, so a value it decides on is kept when a later read fails;
    // a null +0x10 object is a value read (ready 0); a null target too, where the original would fault on [target+0x10] (it reads
    // it unchecked). The size is read whenever the target's +0x10 object is non-null.
    if(!failed){
        if(!settings || !Read(s,&mode,4))failed="mode";
        else if(mode_read=true;mode>=4)ready=0;
        else if(!Read(filter+0x60,&program,8) || !Read(filter+0x68+8ull*mode,&mode_program,8))failed="programmes";
        else if(!program || !mode_program)ready=0;
        else ready=1;
        if(!failed){
            if(!Read(s+8,&target,8))failed="target";
            else if(target && !Read(target+0x10,&inner,8))failed="target_object";
            else if(inner && (!Read(inner+0x50,&width,4) || !Read(inner+0x54,&height,4)))failed="size";
            else {size_read=inner!=0;if(!inner && ready==1)ready=0;}
            if(failed && ready==1)ready=-1;}}
    char key[128]{};
    if(failed)sprintf_s(key,"failed %s e%d g%d r%d m%d",failed,std::strcmp(failed,"enable")?int(enable):-1,gate,ready,mode_read?static_cast<int>(mode):-1);
    else sprintf_s(key,"%ux%u m%u e%u g%d r%d",width,height,mode,unsigned(enable),gate,ready);
    static std::mutex lock;static std::map<std::string,uint64_t> keys;static uint64_t calls{},last_totals{};
    const uint64_t now=GetTickCount64();uint64_t call{};bool fresh{};std::string totals;
    {const std::lock_guard hold(lock);call=++calls;
        const std::string k=keys.size()>=64 && !keys.contains(key)?std::string("overflow"):std::string(key);
        fresh=keys[k]++==0 && k!="overflow";
        if(now>=last_totals+2000){last_totals=now;for(const auto& [name,n]:keys){totals+=totals.empty()?"":",";totals+="\""+name+"\":"+std::to_string(n);}}}
    if(fresh){
        char factor_text[48]{"null"};   // step review M1: %.9g, and a non-finite factor quoted (valid JSON)
        if(factor_read){if(std::isnan(factor))strcpy_s(factor_text,"\"nan\"");else if(std::isinf(factor))strcpy_s(factor_text,factor>0?"\"inf\"":"\"-inf\"");else sprintf_s(factor_text,"%.9g",factor);}
        const auto num=[](bool known,auto v){return known?std::to_string(v):std::string("null");};
        const auto tri=[](int v){return v<0?std::string("null"):v?std::string("true"):std::string("false");};
        const std::string row="{\"event\":\"reference_aa_filter\",\"tick\":"+std::to_string(now)+",\"call\":"+std::to_string(call)+
            ",\"width\":"+num(size_read,width)+",\"height\":"+num(size_read,height)+",\"mode\":"+num(mode_read,mode)+",\"enable\":"+num(!failed || std::strcmp(failed,"enable")!=0,unsigned(enable))+
            ",\"quality\":"+num(quality_read,unsigned(quality_byte))+",\"factor\":"+factor_text+",\"gate\":"+tri(gate)+",\"ready\":"+tri(ready)+
            ",\"failed\":"+(failed?"\""+std::string(failed)+"\"":std::string("null"))+"}\n";
        Log(row.c_str());}
    if(!totals.empty()){const std::string row="{\"event\":\"reference_aa_filter\",\"tick\":"+std::to_string(now)+",\"call\":"+std::to_string(call)+",\"keys\":{"+totals+"}}\n";Log(row.c_str());}
    aa_filter_run_original(filter,context,settings);
}
// D-0180 C4 (casters): the original's map piece emit (D04B90: entity, pass) per pass id, with the entity's category byte
// (+0x20 low byte) and the pass's frame, light-observe.flag only (row reference_disp_emit every 30000 calls).
using DispEmit=void(__fastcall*)(uintptr_t,uintptr_t);
DispEmit disp_emit_original{};
void __fastcall ObserveDispEmit(uintptr_t entity,uintptr_t pass) noexcept {
    disp_emit_original(entity,pass);
    static volatile LONG64 by_pass[0x50]{},by_bits[3][256]{},calls{};
    int32_t id{-1};Read(pass+0x124,&id,4);uint64_t bits{};Read(entity+0x20,&bits,8);
    {static const bool cascades=ModuleFlag(L"cascade-observe.flag");   // cascade observers: map pieces emitted in the cascade passes
        if(cascades && id>=1 && id<=4){const std::lock_guard hold(reference_cascade_lock);++reference_cascades.disp_emits[id-1];}}
    // m50 sky (S1): with cascade-observe.flag, the first 400 map pieces the original emits outside the cascades, once each: their
    // children's model names, the pass id, the category byte and +0x388.
    {static const bool names=ModuleFlag(L"cascade-observe.flag");if(names && !(id>=1 && id<=4)){static std::mutex lock;static std::set<uintptr_t> seen;bool fresh{};
        {const std::lock_guard hold(lock);fresh=seen.size()<400 && seen.insert(entity).second;}
        if(fresh){std::string list;uintptr_t head{},node{};uint32_t walked{};if(Read(entity+0x90,&head,8) && head)Read(head,&node,8);
            for(;node && node!=head && walked<6;++walked){uintptr_t item{},model{};if(Read(node+0x10,&item,8) && item && Read(item+0x58,&model,8) && model){
                for(const auto ch:NativeModelDataName(model))list.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');list+=' ';}if(!Read(node,&node,8))break;}
            uint32_t flags{};Read(entity+0x388,&flags,4);float bounds[8]{};Read(entity+0x40,bounds,sizeof(bounds));
            char row[480]{};sprintf_s(row,"{\"event\":\"reference_disp_names\",\"pass\":%d,\"bits\":%llu,\"flags\":%u,\"bounds\":[%g,%g,%g,%g,%g,%g],\"models\":\"%.240s\"}\n",id,
                static_cast<unsigned long long>(bits&0xFF),flags,bounds[0],bounds[1],bounds[2],bounds[4],bounds[5],bounds[6],list.c_str());Log(row);}}}
    if(id>=0 && id<0x50)InterlockedIncrement64(&by_pass[id]);
    const int group=id>=1 && id<=4?0:(id>=5 && id<=8?1:2);InterlockedIncrement64(&by_bits[group][bits&0xFF]);
    if(InterlockedIncrement64(&calls)%30000==0){std::string row="{\"event\":\"reference_disp_emit\",\"passes\":{";bool first=true;
        for(int i=0;i<0x50;++i)if(by_pass[i]){row+=(first?"\"":",\"")+std::to_string(i)+"\":"+std::to_string(by_pass[i]);first=false;}
        row+="},\"bits\":[";for(int g=0;g<3;++g){row+=g?",{":"{";bool f2=true;for(int b=0;b<256;++b)if(by_bits[g][b]){row+=(f2?"\"":",\"")+std::to_string(b)+"\":"+std::to_string(by_bits[g][b]);f2=false;}row+="}";}
        row+="]}\n";Log(row.c_str());}
}
// D-0180 C4 (image regression): the instance envData the original writes (5B3D0, +0x60 of the record, from the entry index for
// pass state types 6/7), tallied by pass state type (param_4+0x1C) and value, light-observe.flag only.
void LogReferenceInstanceRows(uintptr_t context,uintptr_t descriptor,int* type,uint64_t* out) noexcept;
// I-0141 tranche 17: the pixel-key rows' clock in milliseconds (QPC, the same clock as fog_array's qpc_ms), their 60-second window
// (each window writes every model's states again, so a run's last minutes hold the state each drawn model has then), and a float
// as JSON (null when not finite).
double PixelKeyClockMs() noexcept {LARGE_INTEGER now{},rate{};QueryPerformanceCounter(&now);QueryPerformanceFrequency(&rate);return double(now.QuadPart)*1000.0/double(rate.QuadPart);}
uint64_t PixelKeyWindow(double ms) noexcept {return static_cast<uint64_t>(ms/60000.0);}
std::string PixelKeyNumber(float v) {char c[32]{"null"};if(std::isfinite(v))sprintf_s(c,"%g",v);return c;}
using InstanceRecordBuild=void(__fastcall*)(uint32_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,int*,uint32_t,uint64_t*,uintptr_t,uint64_t*,char);
InstanceRecordBuild instance_record_original{};
void __fastcall ObserveInstanceRecord(uint32_t a1,uintptr_t a2,uintptr_t a3,uintptr_t state,uintptr_t a5,int* a6,uint32_t a7,uint64_t* out,uintptr_t a9,uint64_t* a10,char a11) noexcept {
    instance_record_original(a1,a2,a3,state,a5,a6,a7,out,a9,a10,a11);
    LogReferenceInstanceRows(a3,a5,a6,out);
    static volatile LONG64 tally[16][9]{};static volatile LONG64 calls{};
    int32_t type{-1};uint32_t env{};bool env_read{};if(state)Read(state+0x1C,&type,4);if(out)env_read=Read(reinterpret_cast<uintptr_t>(out)+0x60,&env,4);
    if(type>=0 && type<16)InterlockedIncrement64(&tally[type][env<8?env:8]);
    // I-0113 gap 7 (WHEEL-STUDY 6): per model, the envData written with the pass state type and the group's light key [[a5+0x100]+0xF8]
    // (5B3D0's lookup key for types 6 and 7), once per (model, type, envData, key), at most 600 rows: the wheels against their neighbours.
    // Not logged: 5B3D0's other branch ([root+0x42A0] set: the index from [group+0xF0]); the cap checked first (step review L6).
    static std::atomic<bool> env_rows_full{};
    if(a6 && a5 && !env_rows_full.load(std::memory_order_relaxed)){uintptr_t model{},group{};uint32_t key[2]{};Read(reinterpret_cast<uintptr_t>(a6)+0x1B0,&model,8);
        if(model && Read(a5+0x100,&group,8) && group && Read(group+0xF8,key,8)){const auto name=NativeModelDataName(model);
            static std::mutex lock;static std::set<std::tuple<std::string,int32_t,uint32_t,uint32_t,uint32_t>> seen;bool fresh{};
            {const std::lock_guard held(lock);fresh=seen.size()<600 && seen.insert({name,type,env,key[0],key[1]}).second;if(seen.size()>=600)env_rows_full=true;}
            if(fresh){std::string clean;for(const auto ch:name)clean.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');char row[240]{};
                sprintf_s(row,"{\"event\":\"reference_instance_env_model\",\"model\":\"%.60s\",\"state_type\":%d,\"env\":%u,\"key\":[%u,%d]}\n",clean.c_str(),type,env,key[0],static_cast<int32_t>(key[1]));Log(row);}}}
    // I-0141 tranche 17 (pixel-keys.flag with light-observe.flag, the castle's keys at Archdragon Peak): for the G-buffer rows (types 6
    // and 7), the envData 5B3D0 wrote (the index GBuffer3.w carries, K-0451) beside both of its sources: the branch byte [root+0x42A0]
    // (set: the index from the group's +0xF0 {list, index} through [root+0x4200+8 list]+4+0x30 index; clear: FUN_14014DA10's light-set
    // map [[root+0x41E0]+0xA58] keyed by the light key a +0xF8 alone), and what the composite reads at that index, 137910's entry
    // [[root+0x41F8]+0x88]+0x30 env (+8 light a, +10 light b, +18 fog a, +20 fog b, +28 light t, +2C fog t), read at once (the entries
    // of this frame or of the last), with the group's own key block (+F8, +110, +108, +120) and the group's address ([a5+0x100] is
    // a per-frame copy, not a stable identity, so it is written, not keyed: the final review's M1). Once per 60-second window and
    // (model, the branch byte, the group's key words 0 to 7, the entry's or none): neither the blend weights t nor
    // envData and the count (137EF0 dedups its entries on all ten words, t included, so a blend crossing shifts the indices) are
    // keyed; the first sample's are written (the step reviews' M2 and third M1), so a blend's weights are not followed within a
    // state. Whether envData is below the count stays keyed (an index pushed past it by a shift writes one « entry unread »
    // row). At most 40,000 rows, each with its QPC time (a row whose time falls before the current
    // window is dropped), then one reference_pixel_key_full row; a row is not written unless envData, the model, the group, the root,
    // its keys, the +0xF0 pair, the branch byte, the entries pointer and the count are read.
    {static const bool pixel_keys=ModuleFlag(L"pixel-keys.flag");static std::atomic<bool> full{};
        if(pixel_keys && (type==6 || type==7) && a6 && a5 && out && env_read && !full.load(std::memory_order_relaxed)){uintptr_t model{},group{},root{},entries{};
            Read(reinterpret_cast<uintptr_t>(a6)+0x1B0,&model,8);
            uint32_t own[10]{};uint32_t pair[2]{};uint8_t branch{};int32_t count{};uint32_t entry[12]{};bool entry_read{};
            if(model && Read(a5+0x100,&group,8) && group && Read(image+0x4961B50,&root,8) && root &&
               Read(group+0xF8,own,16) && Read(group+0x110,own+4,16) && Read(group+0x108,own+8,4) && Read(group+0x120,own+9,4) && Read(group+0xF0,pair,8) &&
               Read(root+0x42A0,&branch,1) && Read(root+0x41F8+0x88,&entries,8) && Read(root+0x41F8+0x90,&count,4)){
                if(entries && env<static_cast<uint32_t>(count>0?count:0))entry_read=Read(entries+0x30ull*env,entry,sizeof entry);
                const auto name=NativeModelDataName(model);const double ms=PixelKeyClockMs();
                std::array<uint32_t,17> seen_key{};seen_key[0]=(entry_read?1u:0u)|(uint32_t(branch)<<8);std::memcpy(&seen_key[1],own,32);if(entry_read)std::memcpy(&seen_key[9],&entry[2],32);
                static std::mutex lock;static std::set<std::pair<std::string,std::array<uint32_t,17>>> seen;static uint64_t window{};static size_t written{};bool fresh{},last{};
                {const std::lock_guard held(lock);const auto now=PixelKeyWindow(ms);if(now>window){window=now;seen.clear();}   // forward only (L1)
                    fresh=now>=window && written<40000 && seen.insert({name,seen_key}).second;if(fresh && ++written>=40000){full=true;last=true;}}
                if(fresh){std::string clean;for(const auto ch:name)clean.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');
                    float own_t[2]{},entry_t[2]{};std::memcpy(own_t,own+8,8);std::memcpy(entry_t,entry+10,8);char cell[200]{};
                    if(entry_read)sprintf_s(cell,"[%u,%u,%u,%u,%u,%u,%u,%u],\"entry_t\":[%s,%s]",entry[2],entry[3],entry[4],entry[5],entry[6],entry[7],entry[8],entry[9],
                        PixelKeyNumber(entry_t[0]).c_str(),PixelKeyNumber(entry_t[1]).c_str());
                    else sprintf_s(cell,"null,\"entry_t\":null");
                    char row[760]{};sprintf_s(row,"{\"event\":\"reference_pixel_key\",\"qpc_ms\":%.1f,\"model\":\"%.60s\",\"group\":%llu,\"state_type\":%d,\"env\":%u,\"branch_42a0\":%u,\"list\":[%u,%u],\"count\":%d,"
                        "\"own\":[%u,%u,%u,%u,%u,%u,%u,%u],\"own_t\":[%s,%s],\"entry\":%s}\n",ms,clean.c_str(),static_cast<unsigned long long>(group),type,env,unsigned(branch),pair[0],pair[1],count,
                        own[0],own[1],own[2],own[3],own[4],own[5],own[6],own[7],PixelKeyNumber(own_t[0]).c_str(),PixelKeyNumber(own_t[1]).c_str(),cell);Log(row);
                    if(last){sprintf_s(row,"{\"event\":\"reference_pixel_key_full\",\"qpc_ms\":%.1f,\"rows\":40000}\n",ms);Log(row);}}}}}
    // I-0113 tranche 7 (ZERO-CONSTANTS-CENSUS §5, zero-census.flag): the whole record 5B3D0 wrote (0x110 bytes, VC_InstanceData's one
    // element), once per (model, pass state type), at most 4000 rows, against the native's b4 (native_zero_census).
    {static const bool zero_census=ModuleFlag(L"zero-census.flag");static std::atomic<bool> full{};
        if(zero_census && a6 && out && !full.load(std::memory_order_relaxed)){uintptr_t model{};Read(reinterpret_cast<uintptr_t>(a6)+0x1B0,&model,8);
            if(model){const auto name=NativeModelDataName(model);static std::mutex lock;static std::set<std::pair<std::string,int32_t>> seen;bool fresh{};
                {const std::lock_guard held(lock);fresh=seen.size()<4000 && seen.insert({name,type}).second;if(seen.size()>=4000)full=true;}
                std::array<uint8_t,0x110> record{};
                if(fresh && Read(reinterpret_cast<uintptr_t>(out),record.data(),record.size())){std::string clean;
                    for(const auto ch:name)clean.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');
                    Log(("{\"event\":\"reference_zero_instance\",\"model\":\""+clean.substr(0,80)+"\",\"state_type\":"+std::to_string(type)+
                        ",\"record\":\""+HexBytes(record.data(),record.size())+"\"}\n").c_str());}}}}
    if(InterlockedIncrement64(&calls)%50000==0){std::string row="{\"event\":\"reference_instance_env\",\"types\":{";bool first=true;
        for(int t=0;t<16;++t){std::string cells;LONG64 sum{};for(int e=0;e<9;++e){sum+=tally[t][e];cells+=(e?",":"")+std::to_string(tally[t][e]);}
            if(!sum)continue;row+=(first?"\"":",\"")+std::to_string(t)+"\":["+cells+"]";first=false;}
        row+="}}\n";Log(row.c_str());}
}
// I-0113 tranche 10 (the forward rows' IBL, light-observe.flag, Reference): 75B30 binds t30/t31 from the draw context's env-map keys
// +2F0/+2F8 (the group's +B0/+B8): one row per new key pair (at most 400) with the cubes it binds and the device's defaults +178 (the
// global REM cube) and +170; the REM's name (the device's +13B8 string 41B30 keeps) on the first row and when it changes.
using ForwardIblBind=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t);ForwardIblBind forward_ibl_bind_original{};
void __fastcall ObserveForwardIblBind(uintptr_t device,uintptr_t context,uintptr_t command,uintptr_t extra) noexcept {
    forward_ibl_bind_original(device,context,command,extra);
    // I-0113 tranche 10b: once, the device's kind blends [device+0x2F0 + 8 k] (k 0..4, 72380), which 1A86C0 binds for a forward
    // group whose +0x84 is k (STUDY-TRANSLUCENT-CHAMELEON §1): each one's render target 0 {enable, src, dst, op, src alpha,
    // dst alpha, op alpha, mask}, alpha to coverage and independent blend.
    static std::atomic<bool> kinds_logged{};
    if(!kinds_logged.exchange(true)){std::string row="{\"event\":\"reference_kind_blends\",\"kinds\":[";
        for(uint32_t k=0;k<5;++k){uintptr_t object{};std::string cell="null";
            if(Read(device+0x2F0+8*k,&object,8))if(const auto state=decal_observe::D3d11As(object,__uuidof(ID3D11BlendState))){D3D11_BLEND_DESC d{};
                reinterpret_cast<ID3D11BlendState*>(state)->GetDesc(&d);const auto& t=d.RenderTarget[0];char c[96]{};
                sprintf_s(c,"[%u,%u,%u,%u,%u,%u,%u,%u,%u,%u]",t.BlendEnable,t.SrcBlend,t.DestBlend,t.BlendOp,t.SrcBlendAlpha,t.DestBlendAlpha,t.BlendOpAlpha,
                    t.RenderTargetWriteMask,d.AlphaToCoverageEnable,d.IndependentBlendEnable);cell=c;}
            row+=(k?",":"")+cell;}
        Log((row+"]}\n").c_str());}
    uint32_t keys[4]{};uintptr_t rem{},last{},bound[2]{};
    if(!Read(context+0x2F0,keys,sizeof keys) || !Read(device+0x178,&rem,8) || !Read(device+0x170,&last,8))return;
    Read(command+0x8B0,bound,sizeof bound);
    static std::mutex lock;static std::set<std::array<uint32_t,4>> seen;static std::wstring rem_name;static LONG64 calls{};bool fresh{},renamed{};
    std::wstring name;{uint64_t length{},capacity{};uintptr_t chars=device+0x13B8;
        if(Read(device+0x13C8,&length,8) && Read(device+0x13D0,&capacity,8) && length<128 && (capacity<8 || Read(device+0x13B8,&chars,8))){
            name.resize(static_cast<size_t>(length));if(length && !Read(chars,name.data(),static_cast<size_t>(length)*2))name.clear();}}
    const auto n=InterlockedIncrement64(&calls);
    {const std::lock_guard hold(lock);fresh=seen.size()<400 && seen.insert({keys[0],keys[1],keys[2],keys[3]}).second;if(name!=rem_name){rem_name=name;renamed=true;}}
    if(fresh || renamed || n%20000==0){std::string narrow;for(const auto ch:name)narrow.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');
        // The first slot's texture (width x height, format, mips, array size): the cube a key whose id is -1 takes (+178, else +170).
        char texture[96]{"none"};if(const auto v=decal_observe::D3d11As(bound[0],__uuidof(ID3D11ShaderResourceView))){ID3D11Resource* r{};reinterpret_cast<ID3D11ShaderResourceView*>(v)->GetResource(&r);
            if(r){ID3D11Texture2D* t{};if(SUCCEEDED(r->QueryInterface(__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&t))) && t){D3D11_TEXTURE2D_DESC d{};t->GetDesc(&d);
                    sprintf_s(texture,"%ux%u:f%u:m%u:a%u",d.Width,d.Height,static_cast<unsigned>(d.Format),d.MipLevels,d.ArraySize);t->Release();}r->Release();}}
        // I-0113 tranche 35 (the menu views' hair, natively bound the black cube): each key's 416A0 entry in the region manager's table
        // (root +0x42A8, as NativeEnvMapCube reads it): its cube +0x40, colour scale +0xC and the « %s%04d » name (prefix +0x10, id +8)
        // 14006CED0 binds a missing cube by, so the native can find the same cube by name.
        std::string entries;
        for(uint32_t k=0;k<2;++k){std::string cell="null";uintptr_t root{},table{};
            if(keys[2*k+1]!=0xFFFFFFFFu && Read(image+0x4961B50,&root,8) && root && Read(root+0x42A8,&table,8) && table){
                const auto lookup=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uintptr_t)>(image+0x416A0);
                if(const auto entry=lookup(table,reinterpret_cast<uintptr_t>(keys+2*k))){uintptr_t cube{};float scale{};int32_t id{};uint64_t length{},capacity{};std::wstring prefix;
                    Read(entry+0x40,&cube,8);Read(entry+0xC,&scale,4);Read(entry+8,&id,4);
                    if(Read(entry+0x20,&length,8) && Read(entry+0x28,&capacity,8) && length<64){uintptr_t chars=entry+0x10;prefix.resize(static_cast<size_t>(length));
                        if(capacity>7 && !Read(entry+0x10,&chars,8))chars=0;if(!chars || (length && !Read(chars,prefix.data(),static_cast<size_t>(length)*2)))prefix.clear();}
                    std::string p;for(const auto ch:prefix)p.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');
                    char c[200]{};sprintf_s(c,"{\"cube\":\"%llX\",\"scale\":%g,\"name\":\"%.60s%04d\"}",static_cast<unsigned long long>(cube),scale,p.c_str(),id);cell=c;}
                else cell="\"no entry\"";}
            entries+=(k?",":"")+cell;}
        char row[900]{};sprintf_s(row,"{\"event\":\"reference_forward_ibl\",\"n\":%lld,\"keys\":[[%u,%d],[%u,%d]],\"bound\":[\"%llX\",\"%llX\"],\"texture0\":\"%s\",\"rem\":\"%llX\",\"last\":\"%llX\",\"rem_name\":\"%.80s\",\"entries\":[%s]}\n",
            static_cast<long long>(n),keys[0],static_cast<int32_t>(keys[1]),keys[2],static_cast<int32_t>(keys[3]),static_cast<unsigned long long>(bound[0]),static_cast<unsigned long long>(bound[1]),
            texture,static_cast<unsigned long long>(rem),static_cast<unsigned long long>(last),narrow.c_str(),entries.c_str());Log(row);}
}
// I-0113 tranche 10c (a world object's region IBL, light-observe.flag, Reference): 5C0E0's position input param_5 (16 bytes), its
// view's bucket masks (+0x4B8) and pass id (+0x124), its region gate (the scene context [view+0x4F0]+0x3ECF and the class term, the OR
// of the rows' masks +8 whose +4 is 0, before 5C0E0 ANDs the model's +0x220), the source group param_7[0]'s +0x148 (the instance
// record's origin) and its own keys +0xB0..+0xBF, with the keys 4E2A0 writes into 5C0E0's copy (the thread's last 4E2A0 call, by
// state). Rows: the first call of each source group (at most 64), the first 96 calls that wrote keys, one call every 500 ms in play
// (at most 240; tranche 13: the first-call rows all fell near the load); every 5 s the tallies (calls, the gate open, 4E2A0
// inside 5C0E0, 4E2A0 overall: refs, headless, find no other caller). Past the groups' and samples' budgets no lock is taken.
using ForwardRowSubmitObserved=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uint32_t,uintptr_t,uint32_t,uintptr_t*);ForwardRowSubmitObserved forward_row_submit_observed_original{};
using RegionKeysObserved=void(__fastcall*)(uintptr_t,const void*,const void*,float);RegionKeysObserved region_keys_observed_original{};
thread_local uintptr_t region_keys_state{};thread_local float region_keys_blend{};
std::atomic<int64_t> region_keys_calls{};
void __fastcall ObserveRegionKeys(uintptr_t state,const void* a,const void* b,float blend) noexcept {
    region_keys_observed_original(state,a,b,blend);region_keys_state=state;region_keys_blend=blend;region_keys_calls.fetch_add(1,std::memory_order_relaxed);}
void __fastcall ObserveForwardRowSubmit(uintptr_t a1,uintptr_t view,uintptr_t a3,uint32_t a4,uintptr_t position,uint32_t a6,uintptr_t* a7) noexcept {
    // I-0113 tranche 18 (gap 21, the hair dithered away in a warp): for the hair parts (HR_: the player's, or another character's
    // with the same part), the fade inputs 5C0E0 reads into Q+0x1A9 (00005C0E0.c:101-115: the source group a7[0]'s +0x88, or +0x89
    // for one row of preparation 2, with its opacity +0x124 below the category scalar), read at the call with the first row's pass and
    // preparation: a row each time they change for a model instance and row count (at most 800 rows; the final review's L4: the face
    // parts filled the budget before the fade ended). A model's name is classified once per thread (thread_local, at most 4096
    // models, cleared every second so a model address reused after a load is classified again: the final review's L3).
    {static std::atomic<int> fade_rows{};uintptr_t group{},model{};
        if(fade_rows.load(std::memory_order_relaxed)<800 && a7 && a3 && a4 && Read(reinterpret_cast<uintptr_t>(a7),&group,8) && group &&
           Read(reinterpret_cast<uintptr_t>(a7)+0x18,&model,8) && model){thread_local std::unordered_map<uintptr_t,std::string> part_models;
            thread_local ULONGLONG cleared{};const auto now=GetTickCount64();if(now-cleared>=1000 || part_models.size()>=4096){part_models.clear();cleared=now;}
            auto known=part_models.find(model);
            if(known==part_models.end()){auto n=NativeModelDataName(model);if(!n.starts_with("HR_"))n.clear();known=part_models.emplace(model,std::move(n)).first;}
            if(!known->second.empty()){const auto& name=known->second;std::array<uint8_t,6> b{};float opacity{};uint32_t row[2]{};
                Read(group+0x84,b.data(),6);Read(group+0x124,&opacity,4);Read(a3,row,8);
                char key_text[160]{};sprintf_s(key_text,"%.40s|%llX|%u|%u|%u|%u|%u|%ld",name.c_str(),static_cast<unsigned long long>(model),a4,row[0],row[1],b[4],b[5],std::isfinite(opacity)?std::lround(opacity*20.f):-1l);
                const std::string key=key_text;
                static std::mutex lock;static std::set<std::string> seen;bool fresh{};
                {const std::lock_guard hold(lock);fresh=seen.size()<800 && seen.insert(key).second;}
                if(fresh && fade_rows.fetch_add(1)<800){char op[32]="null";if(std::isfinite(opacity))sprintf_s(op,"%g",opacity);char text[340]{};
                    sprintf_s(text,"{\"event\":\"reference_row_fade_inputs\",\"model\":\"%.40s\",\"rows\":%u,\"first_pass\":%u,\"first_prep\":%u,\"b84\":[%u,%u,%u,%u,%u,%u],\"opacity\":%s,\"group\":\"%llX\"}\n",
                    name.c_str(),a4,row[0],row[1],b[0],b[1],b[2],b[3],b[4],b[5],op,static_cast<unsigned long long>(group));Log(text);}}}}
    region_keys_state=0;
    forward_row_submit_observed_original(a1,view,a3,a4,position,a6,a7);
    static std::atomic<int64_t> calls{},gate_open{},wrote_calls{},wrote_rows{},count_rows{};static std::atomic<ULONGLONG> next_count{};
    static std::atomic<bool> rows_full{};
    const bool wrote=region_keys_state!=0;
    // I-0113 tranche 14: once per model name, keys and blend in hundredths (at most 600 rows), what 4E2A0 wrote into 5C0E0's copy
    // (the keys +0xB0, the blend +0x13C, the scales +0x140/+0x144) with the position, as native_region_ibl.
    if(wrote && a7){static std::mutex lock;static std::set<std::tuple<std::string,uint32_t,uint32_t,int>> seen;static std::atomic<bool> full{};
        uintptr_t model{};uint32_t keys[4]{};float values[3]{},p[4]{};
        if(!full.load(std::memory_order_relaxed) && Read(reinterpret_cast<uintptr_t>(a7)+0x18,&model,8) && model && Read(region_keys_state+0xB0,keys,16) &&
           Read(region_keys_state+0x13C,values,12) && Read(position,p,16)){
            const auto name=NativeModelDataName(model);bool fresh{};
            {const std::lock_guard held(lock);fresh=seen.insert({name,keys[1],keys[3],static_cast<int>(std::lround(values[0]*100.f))}).second;if(seen.size()>=600)full=true;}
            if(fresh){std::string clean;for(const auto ch:name)clean.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');char row[400]{};
                sprintf_s(row,"{\"event\":\"reference_region_ibl\",\"model\":\"%.60s\",\"keys\":[[%u,%d],[%u,%d]],\"blend\":%g,\"scales\":[%g,%g],\"position\":[%g,%g,%g,%g]}\n",
                    clean.c_str(),keys[0],static_cast<int32_t>(keys[1]),keys[2],static_cast<int32_t>(keys[3]),values[0],values[1],values[2],p[0],p[1],p[2],p[3]);Log(row);}}}
    uintptr_t context{};BYTE gate{};uint32_t term{};
    if(Read(view+0x4F0,&context,8) && context)Read(context+0x3ECF,&gate,1);
    if(a3)for(uint32_t i=0;i<a4 && i<4096;++i){uint32_t row[2]{};if(Read(a3+16ull*i+4,row,8) && !row[0])term|=row[1];}
    if(gate && a4 && term)gate_open.fetch_add(1,std::memory_order_relaxed);
    if(wrote)wrote_calls.fetch_add(1,std::memory_order_relaxed);
    const auto n=calls.fetch_add(1,std::memory_order_relaxed)+1;const auto now=GetTickCount64();
    // The tallies every 5 s (at most 720 lines).
    if(auto due=next_count.load(std::memory_order_relaxed);now>=due && next_count.compare_exchange_strong(due,now+5000) && count_rows.fetch_add(1)<720){
        char count[200]{};sprintf_s(count,"{\"event\":\"reference_forward_row_counts\",\"calls\":%lld,\"gate_open\":%lld,\"wrote\":%lld,\"region_keys_calls\":%lld,\"t\":%llu}\n",
        static_cast<long long>(n),static_cast<long long>(gate_open.load()),static_cast<long long>(wrote_calls.load()),static_cast<long long>(region_keys_calls.load()),static_cast<unsigned long long>(now));Log(count);}
    // The writing calls' budget outside the lock; the lock only for the groups and the sample clock, whose budgets alone stop it.
    bool fresh=wrote && wrote_rows.fetch_add(1,std::memory_order_relaxed)<96;
    uintptr_t group{};if(a7)Read(reinterpret_cast<uintptr_t>(a7),&group,8);
    if(!fresh && !rows_full.load(std::memory_order_relaxed)){
        static std::mutex lock;static std::set<uintptr_t> seen;static ULONGLONG next_sample{};static int sampled{};
        const std::lock_guard hold(lock);fresh=group && seen.size()<64 && seen.insert(group).second;
        if(!fresh && group && sampled<240 && now>=next_sample){fresh=true;++sampled;next_sample=now+500;}
        if(seen.size()>=64 && sampled>=240)rows_full=true;}
    if(!fresh || !group)return;
    float p[4]{},origin[3]{};uint32_t masks[2]{},keys[4]{},own[4]{},pass{};Read(position,p,16);Read(view+0x4B8,masks,8);Read(view+0x124,&pass,4);
    Read(group+0x148,origin,12);Read(group+0xB0,own,16);
    const bool written=wrote && Read(region_keys_state+0xB0,keys,16);
    char row[640]{};sprintf_s(row,"{\"event\":\"reference_forward_row_position\",\"position\":[%g,%g,%g,%g],\"origin\":[%g,%g,%g],\"masks\":[\"%X\",\"%X\"],\"pass\":%u,\"gate\":%u,\"term\":\"%X\",\"rows\":%u,\"written\":%s,\"keys\":[[%u,%d],[%u,%d]],\"own\":[[%u,%d],[%u,%d]],\"blend\":%g,\"a6\":%g,\"group\":\"%llX\",\"t\":%llu}\n",
        p[0],p[1],p[2],p[3],origin[0],origin[1],origin[2],masks[0],masks[1],pass,gate,term,a4,written?"true":"false",keys[0],static_cast<int32_t>(keys[1]),keys[2],static_cast<int32_t>(keys[3]),
        own[0],static_cast<int32_t>(own[1]),own[2],static_cast<int32_t>(own[3]),written?region_keys_blend:0.f,
        std::bit_cast<float>(a6),static_cast<unsigned long long>(group),static_cast<unsigned long long>(now));
    Log(row);
}
void __fastcall ObserveSunBlock(void* out,uintptr_t set,const void* view) noexcept {
    sun_block_original(out,set,view);
    static LONG64 calls{};const auto n=InterlockedIncrement64(&calls);
    if(n%600<6){float f[72]{};std::memcpy(f,out,sizeof f);std::string row="{\"event\":\"reference_sun_block\",\"call\":"+std::to_string(n)+",\"set\":"+std::to_string(set)+",\"f\":[";
        for(int i=0;i<72;++i){char v[24]{};sprintf_s(v,"%s%.6g",i?",":"",f[i]);row+=v;}row+="]}\n";Log(row.c_str());}
}
// I-0141 (RESULT « Read for the next tranches », item 2; with light-observe.flag, pass-through; since tranche 3 these hooks are the
// Reference's, the native run's rows written by its own hooks, NativeRemoLightOverride and its siblings): the writes of the
// cutscene act IngameDrawParam (FUN_140CC8180) into the light and the fog banks. FUN_140031A20 (the light sets' bank) and
// FUN_1400380A0 (the fog's) take (bank, key {area, id}, layer, key A, key B, t) and set the record at `key` on `layer` from the
// records at A and B (their blend when t is positive, else A's); FUN_1400320B0 and FUN_1400387D0 take (bank, id from, id to,
// area, layer, a sixth stack word passed on whole) and remove them. A row for each (key, layer) when it is first seen, when its
// A or B changes, and at every 120th call of it between (t moves each frame; the act writes several keys a bank at each update,
// so the rows are kept by key: the merge's final review, L1; 16 keys a bank, a 17th is not logged but for one
// `remo_bank_override_table_full` row a bank); a removal's row the same way
// by (area, ids, layer). No removal was seen in the original's first run (i141-t2b: 0 rows), so the removers' rows are
// unproved. The act runs on the game's thread and is the four functions' one caller (tranche 3's final review, M3).
using BankOverride=void(__fastcall*)(uintptr_t,const uint32_t*,uint32_t,const uint32_t*,const uint32_t*,float);
using BankOverrideRemove=void(__fastcall*)(uintptr_t,uint32_t,uint32_t,uint32_t,uint32_t,uint64_t);
BankOverride light_override_original{},fog_override_original{};BankOverrideRemove light_override_remove_original{},fog_override_remove_original{};
void LogBankOverride(int which,const uint32_t* key,uint32_t layer,const uint32_t* a,const uint32_t* b,float t) noexcept {
    struct Entry {uint32_t words[7];LONG64 calls;};static Entry table[2][16]{};static uint32_t used[2]{};
    uint32_t w[7]{};
    if(!Read(reinterpret_cast<uintptr_t>(key),w,8) || !Read(reinterpret_cast<uintptr_t>(a),w+2,8) || !Read(reinterpret_cast<uintptr_t>(b),w+4,8))return;
    w[6]=layer;Entry* e{};
    for(uint32_t i=0;i<used[which];++i){Entry& c=table[which][i];if(c.words[0]==w[0] && c.words[1]==w[1] && c.words[6]==layer){e=&c;break;}}
    if(!e){const uint32_t n=used[which];
        if(n>=16){static bool full[2]{};if(!full[which]){full[which]=true;Log(which?"{\"event\":\"remo_bank_override_table_full\",\"bank\":\"fog\"}\n":"{\"event\":\"remo_bank_override_table_full\",\"bank\":\"light\"}\n");}return;}
        e=&table[which][n];used[which]=n+1;std::memcpy(e->words,w,sizeof w);e->calls=0;}
    else if(std::memcmp(w,e->words,sizeof w)!=0){std::memcpy(e->words,w,sizeof w);e->calls=0;}
    else if(++e->calls%120)return;
    char row[320]{};sprintf_s(row,"{\"event\":\"remo_bank_override\",\"bank\":\"%s\",\"key\":[%u,%d],\"layer\":%u,\"a\":[%u,%d],\"b\":[%u,%d],\"t\":%.4g}\n",
        which?"fog":"light",w[0],static_cast<int32_t>(w[1]),layer,w[2],static_cast<int32_t>(w[3]),w[4],static_cast<int32_t>(w[5]),std::isfinite(t)?static_cast<double>(t):-1.0);Log(row);}
void LogBankOverrideRemove(int which,uint32_t from,uint32_t to,uint32_t area,uint32_t layer) noexcept {
    struct Entry {uint32_t words[4];LONG64 calls;};static Entry table[2][16]{};static uint32_t used[2]{};
    const uint32_t w[4]{from,to,area,layer};Entry* e{};
    for(uint32_t i=0;i<used[which];++i)if(!std::memcmp(table[which][i].words,w,sizeof w)){e=&table[which][i];break;}
    if(!e){const uint32_t n=used[which];if(n>=16)return;e=&table[which][n];used[which]=n+1;std::memcpy(e->words,w,sizeof w);e->calls=0;}
    else if(++e->calls%120)return;
    char row[224]{};sprintf_s(row,"{\"event\":\"remo_bank_override_removed\",\"bank\":\"%s\",\"area\":%u,\"ids\":[%d,%d],\"layer\":%u}\n",
        which?"fog":"light",area,static_cast<int32_t>(from),static_cast<int32_t>(to),layer);Log(row);}
void __fastcall ObserveLightOverride(uintptr_t bank,const uint32_t* key,uint32_t layer,const uint32_t* a,const uint32_t* b,float t) noexcept {
    LogBankOverride(0,key,layer,a,b,t);light_override_original(bank,key,layer,a,b,t);}
void __fastcall ObserveFogOverride(uintptr_t bank,const uint32_t* key,uint32_t layer,const uint32_t* a,const uint32_t* b,float t) noexcept {
    LogBankOverride(1,key,layer,a,b,t);fog_override_original(bank,key,layer,a,b,t);}
void __fastcall ObserveLightOverrideRemove(uintptr_t bank,uint32_t from,uint32_t to,uint32_t area,uint32_t layer,uint64_t last) noexcept {
    LogBankOverrideRemove(0,from,to,area,layer);light_override_remove_original(bank,from,to,area,layer,last);}
void __fastcall ObserveFogOverrideRemove(uintptr_t bank,uint32_t from,uint32_t to,uint32_t area,uint32_t layer,uint64_t last) noexcept {
    LogBankOverrideRemove(1,from,to,area,layer);fog_override_remove_original(bank,from,to,area,layer,last);}
// The act's update itself (FUN_140CC8180(act): no static caller, a virtual call), with the same flag (natively through
// NativeRemoDrawParamAct, tranche 3): whether it
// runs, from where (the return address, a row a new caller, up to 8) and its target id (the short at act+0x30A; the area comes
// from the act's world). A row at the first call, at each new caller and every 600th call.
using RemoDrawParamAct=void(__fastcall*)(uintptr_t);
RemoDrawParamAct remo_draw_param_act_original{};
void LogRemoDrawParamAct(uintptr_t act,uintptr_t return_address) noexcept {   // I-0141 tranche 3: also the native hook's row
    static LONG64 calls{};static uintptr_t callers[8]{};static uint32_t caller_count{};
    const auto n=InterlockedIncrement64(&calls);const uintptr_t from=return_address-image;
    const uint32_t known=(std::min)(caller_count,8u);bool fresh=known<8;for(uint32_t i=0;i<known;++i)if(callers[i]==from)fresh=false;
    if(fresh){callers[known]=from;caller_count=known+1;}
    if(n==1 || fresh || n%600==0){int16_t id{};Read(act+0x30A,&id,2);char row[200]{};
        sprintf_s(row,"{\"event\":\"remo_draw_param_act\",\"calls\":%lld,\"caller_rva\":%llu,\"target_id\":%d}\n",static_cast<long long>(n),
            static_cast<unsigned long long>(from),static_cast<int>(id));Log(row);}}
void __fastcall ObserveRemoDrawParamAct(uintptr_t act) noexcept {
    LogRemoDrawParamAct(act,reinterpret_cast<uintptr_t>(_ReturnAddress()));remo_draw_param_act_original(act);}
// Fable review (m33: the native YEBIS keys stay at id 0): the original's scene CPU update E006A0 (stage, tick), pass-through
// with shadow-observe.flag: the stage's scene parameter holders (+0x348 own, [+0x500], [+0x508]) with their tokens (+0x190)
// and the keys of their first four category entries (entry i at +0x20 i: A +0x10, B +0x18) and the single key +0x188.
using SceneCpuUpdate=void(__fastcall*)(uintptr_t,uintptr_t);
SceneCpuUpdate scene_cpu_update_original{};
void __fastcall ObserveSceneCpuUpdate(uintptr_t stage,uintptr_t tick) noexcept {
    static LONG64 calls{};const auto n=InterlockedIncrement64(&calls);
    if(n<=8 || n%300==0){uintptr_t view{};uint64_t policy{};Read(stage+0x340,&view,8);if(view)Read(view+0xE0,&policy,8);char head[200]{};
        sprintf_s(head,"{\"event\":\"reference_scene_holders\",\"call\":%lld,\"view_policy\":\"%016llX\",\"holders\":[",static_cast<long long>(n),static_cast<unsigned long long>(policy));std::string row=head;
        uintptr_t set500{},set508{};Read(stage+0x500,&set500,8);Read(stage+0x508,&set508,8);
        const std::array<std::pair<const char*,uintptr_t>,3> holders{{{"own",stage+0x348},{"slot500",set500},{"slot508",set508}}};bool first=true;
        for(const auto& [label,holder]:holders){if(!holder){continue;}
            uint32_t tokens[2]{},single[2]{};Read(holder+0x190,tokens,8);Read(holder+0x188,single,8);std::string entries;
            for(uint32_t i=0;i<4;++i){uint32_t a[2]{},b[2]{};Read(holder+0x20ull*i+0x10,a,8);Read(holder+0x20ull*i+0x18,b,8);char cell[80]{};sprintf_s(cell,"%s[%u,%u,%u,%u]",i?",":"",a[0],a[1],b[0],b[1]);entries+=cell;}
            char cell[320]{};sprintf_s(cell,"%s{\"holder\":\"%s\",\"tokens\":[%u,%u],\"single\":[%u,%u],\"entries\":[%s]}",first?"":",",label,tokens[0],tokens[1],single[0],single[1],entries.c_str());row+=cell;first=false;}
        row+="]}\n";Log(row.c_str());}
    scene_cpu_update_original(stage,tick);
}
// Fable review (m33: characters refused as zero_fade natively): the LOD controller step D1CCE0(record, manager) of MENUS-MAP 62
// section 3, the only writer of the fade weights (+0x7C) and the geometry weight (+0x80 = E+0x170), pass-through in both
// modes with light-observe.flag: the records whose geometry weight is below 1 (at most 96 rows, once per model name and
// weight band), with the manager's camera position (+0x80), extent (+0x110) and scale (+0x150), the record's centre (+0x20)
// and the distance d the step compares.
// Optimisation review (the m33 lake trunk 1.22 after the ambient rows): the original's instance record writer 5B3D0 scales
// the lighting rows' w by the pass context's +0x110/+0x114/+0x118 (natively the scene's pass_row_scale since I-0113 tranche 34) and skips them for an owning
// animation or the render override. With light-observe.flag, once per model name, draw parameter type and context scales
// (300 rows), the inputs it decides on and the rows it wrote (param_8 +0x80..+0xAF).
// Called by ObserveInstanceRecord (the 5B3D0 hook) after the original.
void LogReferenceInstanceRows(uintptr_t context,uintptr_t descriptor,int* type,uint64_t* out) noexcept {
    if(!type || !out || !context || !descriptor)return;
    int draw_type{};float scales[3]{};uintptr_t model{},owner{};uint8_t owned{};
    if(!Read(reinterpret_cast<uintptr_t>(type),&draw_type,4) || !Read(context+0x110,scales,12))return;
    Read(reinterpret_cast<uintptr_t>(type)+0x1B0,&model,8);Read(descriptor+0x68,&owner,8);if(owner && owner!=static_cast<uintptr_t>(-0xF0))Read(owner+0x170,&owned,1);
    const auto name=model?NativeModelDataName(model):std::string("?");
    float rows[16]{};Read(reinterpret_cast<uintptr_t>(out)+0x80,rows,64);
    static std::mutex lock;static std::set<std::tuple<std::string,int,int,int,int,int>> seen;bool fresh{};
    {const std::lock_guard held(lock);fresh=seen.size()<800 && seen.insert({name,draw_type,owned,int(std::lround(rows[3]*1000.f)),int(std::lround(rows[1]*1000.f)),int(std::lround(scales[0]*1000.f))}).second;}
    if(!fresh)return;std::string clean;for(const auto ch:name)clean.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');
    char row[640]{};sprintf_s(row,"{\"event\":\"reference_instance_rows\",\"model\":\"%.60s\",\"type\":%d,\"context_scales\":[%g,%g,%g],\"owned\":%u,\"rows\":[%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g],\"blend\":[%g,%g,%g,%g]}\n",
        clean.c_str(),draw_type,scales[0],scales[1],scales[2],owned,rows[0],rows[1],rows[2],rows[3],rows[4],rows[5],rows[6],rows[7],rows[8],rows[9],rows[10],rows[11],rows[12],rows[13],rows[14],rows[15]);Log(row);
}
// Optimisation review (the per-instance SH: 61950 always gets a null record natively): D40010(draw, src), the draw entity's
// probe source; the pick (D3FFB0) runs only when src and [src+0x90] (the map's baked probe data) exist. Both modes, light-observe.
const char* UiObservationMode() noexcept;
using ProbeSource=void(__fastcall*)(uintptr_t,uintptr_t);
ProbeSource probe_source_original{};
void __fastcall ObserveProbeSource(uintptr_t draw,uintptr_t source) noexcept {
    probe_source_original(draw,source);
    static LONG64 calls{},with_source{},with_data{};const auto n=InterlockedIncrement64(&calls);uintptr_t data{};
    if(source){InterlockedIncrement64(&with_source);if(Read(source+0x90,&data,8) && data)InterlockedIncrement64(&with_data);}
    if(n<=12 || n%500==0){uintptr_t table{};if(source)Read(source,&table,8);char row[300]{};
        sprintf_s(row,"{\"event\":\"probe_source\",\"mode\":\"%s\",\"calls\":%lld,\"with_source\":%lld,\"with_data\":%lld,\"source_table_rva\":%llu,\"data\":%s}\n",UiObservationMode(),n,with_source,with_data,
            static_cast<unsigned long long>(table>=image?table-image:table),data?"true":"false");Log(row);}
}
// and D3FFB0(draw, pos, bbox): the pick 1D650 when draw+0x82C, its record kept at draw+0x820.
using ProbePick=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t);
ProbePick probe_pick_original{};
void __fastcall ObserveProbePick(uintptr_t draw,uintptr_t position,uintptr_t box) noexcept {
    probe_pick_original(draw,position,box);
    static LONG64 calls{},enabled{},found{};const auto n=InterlockedIncrement64(&calls);uint8_t flag{};uintptr_t record{};
    Read(draw+0x82C,&flag,1);Read(draw+0x820,&record,8);if(flag)InterlockedIncrement64(&enabled);if(record)InterlockedIncrement64(&found);
    if(n<=6 || n%500==0){float p[4]{},b[8]{};if(position)Read(position,p,16);if(box)Read(box,b,32);
        float volume[8]{};uintptr_t root{},head{},node{},entry{};if(Read(image+0x4961B50,&root,8) && root && Read(root+0x46E8+8,&head,8) && head && Read(head,&node,8) && node && node!=head && Read(node+0x20,&entry,8) && entry)Read(entry+0x90,volume,32);
        char row[900]{};
        sprintf_s(row,"{\"event\":\"probe_pick\",\"mode\":\"%s\",\"calls\":%lld,\"enabled\":%lld,\"found\":%lld,\"flag\":%u,\"position\":[%g,%g,%g],\"box\":[%g,%g,%g,%g,%g,%g,%g,%g],\"volume\":[%g,%g,%g,%g,%g,%g,%g,%g]}\n",UiObservationMode(),n,enabled,found,flag,p[0],p[1],p[2],
            b[0],b[1],b[2],b[3],b[4],b[5],b[6],b[7],volume[0],volume[1],volume[2],volume[3],volume[4],volume[5],volume[6],volume[7]);Log(row);}
}
// I-0100 T0 (POPIN-STUDY §4 M1, popin-observe.flag, both renderers, diagnostics only): which objects each renderer draws in its
// main view, frame by frame, one row per change (popin_reference, popin_native), so the pop-in and the flicker are measured
// against the original on the same path. Shared here: the camera of the LOD manager the LOD step D1CCE0 last ran with (+0x80,
// what the step compares), an object's world position (the translation of the matrix its draw uses, as the native's candidates
// read it, 09_model_admission: an item's [[item+0x6F8]+0x60], a draw entity's [E+0xB08 + 0x18 (C3C bit 4) + 0x10], a character's
// [E+0xA60 + 0x18 (BB0 bit 3) + 0x10]; the first T0 runs' LOD controller +0x20 was not a position), the fade owner's weights
// (its controller's +0x7C, +0x80: E+0xB0 for a SprjModelDispEntity, E+0xF0 for a SprjModelDrawEntity, MENUS-MAP 62), a model's
// clean name. Distances in metres (DS3's own scale +0x150 not applied).
bool PopinObserve() noexcept {static const bool on=ModuleFlag(L"popin-observe.flag");return on;}
// I-0136 (a test tool): the Velo rows tested against the scene depth with the former bias, not in their own depth.
bool VeloDepthLegacy() noexcept {static const bool on=ModuleFlag(L"velo-depth-legacy.flag");return on;}
std::atomic<uintptr_t> popin_lod_manager{};
float PopinDistance(const float* position) noexcept {
    const auto manager=popin_lod_manager.load(std::memory_order_relaxed);float camera[3]{};
    if(!manager || !Read(manager+0x80,camera,12))return -1.f;
    const float dx=position[0]-camera[0],dy=position[1]-camera[1],dz=position[2]-camera[2];return std::sqrt(dx*dx+dy*dy+dz*dz);
}
struct PopinPlace {float centre[3]{},d{-1.f},weights[2]{-1.f,-1.f};};
constexpr float kPopinNear=8.f;   // I-0115: the near characters' rows (popin_native_near, popin_reference_near), metres from the camera
// position: a 3x4 row-major world matrix's translation (words 3, 7, 11); owner: the fade owner whose weights are read.
PopinPlace PopinPlaceAt(const float* matrix,uintptr_t owner) noexcept {
    PopinPlace p;p.centre[0]=matrix[3];p.centre[1]=matrix[7];p.centre[2]=matrix[11];p.d=PopinDistance(p.centre);
    uintptr_t table{};if(owner){const bool disp=Read(owner,&table,8) && table==image+0x29489A0;Read(owner+(disp?0xB0:0xF0)+0x7C,p.weights,8);}
    return p;
}
// The original's object (kind 0 item, 1 draw entity, 2 character part with its character as owner): its draw's matrix.
PopinPlace PopinReferencePlace(uint8_t kind,uintptr_t object,uintptr_t owner) noexcept {
    uintptr_t matrices{};
    if(kind==0){uintptr_t transforms{};if(Read(object+0x6F8,&transforms,8) && transforms)Read(transforms+0x60,&matrices,8);}
    else if(kind==1){uint8_t enabled{};Read(object+0xC3C,&enabled,1);Read(object+0xB08+((enabled>>4)&1)*0x18+0x10,&matrices,8);}
    else{uint32_t state{};Read(owner+0xBB0,&state,4);Read(owner+0xA60+((state>>3)&1)*0x18+0x10,&matrices,8);}
    float matrix[12]{};if(matrices)Read(matrices,matrix,sizeof matrix);
    return PopinPlaceAt(matrix,kind==2?0:owner);
}
std::string PopinName(uintptr_t model){std::string clean;if(!model)return "?";
    for(const auto ch:NativeModelDataName(model)){if(clean.size()>=60)break;clean.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');}return clean;}
// The original's main-view draws (step review H1, M2): a map piece's item as D04B90 hands it to D3DA20 (only once the piece's
// own tests and its weight passed; D3DA20 then skips an item whose +0x71A is set for passes 5..8), a draw entity as D06E10
// hands it to D079C0 (after its fade test), an assembled character's part as D02A30 hands it to CFDE70; the pass id at
// pass+0x124: 5 the G-buffer view, 0x45 the main view of the category-0x01 pieces (09_model_admission). Each is gathered by
// (object, model) between two world update gates (the game step, ProbeOriginalWorldGate) and compared with the previous
// window's: one in one window and not the other is a row. Every window also writes its emit count and the times of its first
// and last emit after its opening gate (step review M1: DS3's traversal runs on job threads, so a window that cuts one frame's
// traversal shows there, and the reader leaves out the changes around it).
struct PopinReferenceObject {uintptr_t model{},owner{};uint8_t kind{},ids{};float opacity{2.f},by_view[2]{2.f,2.f};};   // opacity: the lowest this window; by_view: the pass-5 and the 0x45 views' (I-0115)
struct PopinReferenceEntry {uint8_t ids{},kind{};std::string name;float centre[3]{};uintptr_t object{};};
struct PopinReference {std::mutex lock;std::unordered_map<uint64_t,PopinReferenceObject> current;std::unordered_map<uint64_t,PopinReferenceEntry> previous;
    uint64_t emits{};int64_t first{},last{},gate{};};
PopinReference popin_reference;
using PopinItemDrawFn=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,int,uint8_t,char);
PopinItemDrawFn popin_item_draw_original{};
using PopinDrawEmitFn=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
PopinDrawEmitFn popin_draw_emit_original{};
using PopinPartDrawFn=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uint32_t,uint8_t);
PopinPartDrawFn popin_part_draw_original{};
void PopinReferenceEmit(uintptr_t object,uintptr_t model,uintptr_t owner,uintptr_t pass,uint8_t kind,bool flagged,float opacity=1.f) noexcept {
    int32_t id{-1};if(!Read(pass+0x124,&id,4) || (id!=5 && id!=0x45))return;
    LARGE_INTEGER now{};QueryPerformanceCounter(&now);
    // I-0100 tranche 2 (T2a): each pass context the main view's draws come through, once (its id, its engine pass +0x128 and key
    // +0x12C, FFX-DRAW), so the rows are known to come from one view, not from another view of the same pass id.
    {static std::mutex lock;static std::set<uintptr_t> seen;bool fresh{};{const std::lock_guard hold(lock);fresh=seen.size()<64 && seen.insert(pass).second;}
        if(fresh){int32_t engine_pass{-1},key{-1};Read(pass+0x128,&engine_pass,4);Read(pass+0x12C,&key,4);char text[200]{};
            sprintf_s(text,"{\"event\":\"popin_reference_pass\",\"pass\":\"%llx\",\"id\":%d,\"engine_pass\":%d,\"key\":%d,\"kind\":%u}\n",static_cast<unsigned long long>(pass),id,engine_pass,key,kind);Log(text);}}
    try{const std::lock_guard hold(popin_reference.lock);auto& o=popin_reference.current[uint64_t(object)^(uint64_t(model)<<1)];
        o.model=model;o.owner=owner;o.kind=kind;o.opacity=(std::min)(o.opacity,opacity);o.by_view[id==5?0:1]=(std::min)(o.by_view[id==5?0:1],opacity);o.ids|=static_cast<uint8_t>((id==5?1u:2u)|(flagged?4u:0u));
        ++popin_reference.emits;if(!popin_reference.first)popin_reference.first=now.QuadPart;popin_reference.last=now.QuadPart;}catch(...){}
}
// D3DA20(item, pass, group, piece, variant, ..): the item drawn unless its +0x71A suppresses passes 5..8 (its own test, read
// after it); a variant above 0 is flagged (D3DA20 also skips it when 812DF0 answers, not read here). Its opacity is the group's
// +0x124 (D04B90 hands a fading piece a copy of its group with +0x124 scaled by the weight, decompiled/DS3-1152-F-00D04B90.c).
void __fastcall PopinItemDraw(uintptr_t item,uintptr_t pass,uintptr_t group,uintptr_t piece,int variant,uint8_t p6,char p7) noexcept {
    popin_item_draw_original(item,pass,group,piece,variant,p6,p7);
    uint32_t id{};if(!Read(pass+0x124,&id,4) || (id!=5 && id!=0x45))return;   // the main view's passes only, tested first
    uint8_t suppressed{};if(id==5 && Read(item+0x71A,&suppressed,1) && suppressed)return;
    uintptr_t model{};Read(item+0x58,&model,8);float opacity{1.f};Read(group+0x124,&opacity,4);PopinReferenceEmit(item,model,piece,pass,0,variant>0,opacity);
    // I-0100 tranche 2 (R9: the objects the native refuses with an empty model): the model's storage (+0xE8) and mesh count (+0xA0)
    // when the original draws its item, by model name at its 1st and 1,000th draw (4,000 models).
    if(model){static std::mutex lock;static std::unordered_map<uintptr_t,uint32_t> seen;uint32_t n{};
        {const std::lock_guard hold(lock);if(seen.contains(model) || seen.size()<4000)n=++seen[model];}
        if(n==1 || n==1000){uintptr_t storage{};int32_t meshes{};Read(model+0xE8,&storage,8);Read(model+0xA0,&meshes,4);char text[200]{};
            sprintf_s(text,"{\"event\":\"popin_reference_model\",\"model\":\"%s\",\"draws\":%u,\"storage\":%s,\"meshes\":%d}\n",PopinName(model).c_str(),n,storage?"true":"false",meshes);Log(text);}}
}
// I-0115 (the Silver Knight at Anor Londo): a draw entity of a character model emitted within kPopinNear metres of the camera, with
// the fade group D06E10 hands to D079C0 (E+0x240, or its copy whose +0x124 it scaled by the weight): +0x84/+0x85/+0x87, +0xDC (the
// words the native's ordinary category reads, 09_model_admission) and +0x124; gathered per window, one popin_reference_near row
// when one changes, « absent » when a near entity stops being emitted (PopinReferenceWindow).
struct PopinReferenceNearEntry {uintptr_t model{};BYTE k[3]{};float dc{-1.f},o{-1.f},d{-1.f};uint64_t code{};std::string name;};
struct PopinReferenceNear {std::mutex lock;std::unordered_map<uintptr_t,PopinReferenceNearEntry> current,previous;};
PopinReferenceNear popin_reference_near;
void __fastcall PopinDrawEmit(uintptr_t entity,uintptr_t a2,uintptr_t a3,uintptr_t a4,uintptr_t a5) noexcept {
    popin_draw_emit_original(entity,a2,a3,a4,a5);
    uintptr_t binding{},model{};if(Read(entity+0xE8,&binding,8) && binding)Read(binding+8,&model,8);
    // I-0115 step review M1: the fade group's +0x124 (D06E10 hands D079C0 its copy scaled by the view's weight), as D3DA20's items.
    float opacity{1.f};if(a3)Read(a3+0x124,&opacity,4);PopinReferenceEmit(entity,model,entity,a2,1,false,opacity);
    int32_t id{-1};if(!model || !a3 || !Read(a2+0x124,&id,4) || (id!=5 && id!=0x45))return;
    const auto place=PopinReferencePlace(1,entity,entity);if(!(place.d>=0.f && place.d<kPopinNear))return;
    PopinReferenceNearEntry e;e.model=model;e.d=place.d;Read(a3+0x84,&e.k[0],1);Read(a3+0x85,&e.k[1],1);Read(a3+0x87,&e.k[2],1);Read(a3+0xDC,&e.dc,4);Read(a3+0x124,&e.o,4);
    e.code=uint64_t(e.k[0])|uint64_t(e.k[1])<<8|uint64_t(e.k[2])<<16|uint64_t(uint8_t(std::lround(std::clamp(e.dc,-1.f,2.f)*50.f)))<<24|
        uint64_t(uint8_t(std::lround(std::clamp(e.o,-1.f,2.f)*50.f)))<<32;
    try{const std::lock_guard hold(popin_reference_near.lock);auto& current=popin_reference_near.current;
        if(const auto found=current.find(entity);found==current.end() || (id==0x45 && found->second.code!=e.code))current.insert_or_assign(entity,std::move(e));}catch(...){}
}
// CFDE70(part, pass, ..): the part's character at +0x18, its index +0x20 in the character's part table [+0xB00] (0x50 bytes a
// row, its model data at [row+0x10]+8, decompiled/DS3-1152-F-00CFDE70.c).
void __fastcall PopinPartDraw(uintptr_t part,uintptr_t pass,uintptr_t a3,uint32_t a4,uint8_t a5) noexcept {
    popin_part_draw_original(part,pass,a3,a4,a5);
    uintptr_t owner{},table{},row{},model{};uint32_t index{};
    if(Read(part+0x18,&owner,8) && owner && Read(part+0x20,&index,4) && Read(owner+0xB00,&table,8) && table && Read(table+uint64_t(index)*0x50+0x10,&row,8) && row)Read(row+8,&model,8);
    PopinReferenceEmit(part,model,owner,pass,2,false);
}
// I-0100 tranche 2 (Thomas's play, 2026-10-26: the distant shadows pop in, not the models): the original's depth rows (63CB0's
// rows of state type 2: the sun cascades' casters and the local lights' tiles, SUN-SHADOWS-WAVE1 7), by model, gathered between
// two world update gates as the main view's draws; one popin_reference_shadow row when a model starts or stops casting (no
// instance or position is known at 63CB0: a model's first instance in, its last out).
struct PopinReferenceShadows {std::mutex lock;std::unordered_map<uintptr_t,uint32_t> current;std::unordered_map<uintptr_t,std::string> previous;};
PopinReferenceShadows popin_reference_shadows;
void PopinReferenceShadow(uintptr_t model) noexcept {
    try{const std::lock_guard hold(popin_reference_shadows.lock);auto& current=popin_reference_shadows.current;if(current.size()<16384 || current.contains(model))++current[model];}catch(...){}}
// At the world update gate (game step n): the window since the previous gate closed. Names and centres are read when an object
// appears and kept, so its « off » row reads nothing of an object that may be gone.
void PopinReferenceWindow(int64_t step) noexcept {
    if(!PopinObserve() || control->mode!=bootstrap::Mode::Reference)return;
    CasterPlacesWindow(step);   // I-0100 tranche 5, caster-place.flag
    try{std::unordered_map<uint64_t,PopinReferenceObject> now;uint64_t emits{};int64_t first_emit{},last_emit{},window_open{};LARGE_INTEGER gate{};QueryPerformanceCounter(&gate);
        {const std::lock_guard hold(popin_reference.lock);now.swap(popin_reference.current);emits=popin_reference.emits;first_emit=popin_reference.first;last_emit=popin_reference.last;
            window_open=popin_reference.gate;popin_reference.emits=0;popin_reference.first=0;popin_reference.last=0;popin_reference.gate=gate.QuadPart;}
        auto& r=popin_reference;
        static const double tick_ms=[]{LARGE_INTEGER f{};QueryPerformanceFrequency(&f);return 1000.0/double(f.QuadPart);}();
        const auto since=[&](int64_t at){return at && window_open?double(at-window_open)*tick_ms:-1.0;};
        // I-0100 tranche 8 (M2): with DS3's texture creations so far and the time since the loading page's window was asked to close.
        {const auto closed=popin_page_closed.load(std::memory_order_relaxed);char text[320]{};
            sprintf_s(text,"{\"event\":\"popin_reference_window\",\"step\":%lld,\"emits\":%llu,\"length_ms\":%.2f,\"first_ms\":%.2f,\"last_ms\":%.2f,\"textures\":%llu,\"page_ms\":%.1f}\n",
            static_cast<long long>(step),static_cast<unsigned long long>(emits),since(gate.QuadPart),since(first_emit),since(last_emit),
            static_cast<unsigned long long>(popin_texture_creations.load(std::memory_order_relaxed)),closed?double(gate.QuadPart-closed)*tick_ms:-1.0);Log(text);}
        // I-0100 tranche 4 (T2a: does the original keep drawing a character on the windows it does not emit it?): per window and
        // character model (c####), the draw entities emitted (D079C0) beside the rows the original executed for that model
        // (63CB0: depth rows of state type 2, the others), one popin_reference_chr row per window with any.
        {std::unordered_map<uintptr_t,std::array<uint32_t,3>> chr;{const std::lock_guard hold(popin_reference_exec.lock);for(const auto& [m,c]:popin_reference_exec.rows)chr[m]={0,c[0],c[1]};popin_reference_exec.rows.clear();}
            for(const auto& [key,o]:now)if(o.kind==1)++chr[o.model][0];
            static std::unordered_map<uintptr_t,std::string> names;std::string list;
            for(const auto& [m,c]:chr){auto found=names.find(m);if(found==names.end()){if(names.size()>4096)names.clear();found=names.emplace(m,PopinName(m)).first;}
                const auto& n=found->second;if(n.size()!=5 || n[0]!='c' || !std::isdigit(static_cast<unsigned char>(n[1])))continue;
                char cell[64]{};sprintf_s(cell,"%s[\"%s\",%u,%u,%u]",list.empty()?"":",",n.c_str(),c[0],c[1],c[2]);list+=cell;}
            if(!list.empty()){const std::string text="{\"event\":\"popin_reference_chr\",\"step\":"+std::to_string(step)+",\"models\":["+list+"]}\n";Log(text.c_str());}}
        static const char* const kinds[]{"item","draw","part"};
        const auto row=[&](bool on,const PopinReferenceEntry& e,const PopinPlace& p){char text[480]{};
            sprintf_s(text,"{\"event\":\"popin_reference\",\"step\":%lld,\"on\":%d,\"entity\":\"%llx\",\"kind\":\"%s\",\"ids\":%u,\"model\":\"%s\",\"centre\":[%.2f,%.2f,%.2f],\"d\":%.2f,\"w\":[%.3f,%.3f]}\n",
                static_cast<long long>(step),on?1:0,static_cast<unsigned long long>(e.object),kinds[e.kind<3?e.kind:0],e.ids,e.name.c_str(),e.centre[0],e.centre[1],e.centre[2],p.d,p.weights[0],p.weights[1]);Log(text);};
        for(const auto& [key,o]:now){
            // A draw at an opacity under 1 (D04B90 scales the group copy's +0x124 by the fade weight, then the LOD cross-fade): one row a window.
            if(o.opacity<0.999f){const auto place=PopinReferencePlace(o.kind,key^(uint64_t(o.model)<<1),o.owner);char text[320]{};
                sprintf_s(text,"{\"event\":\"popin_reference_fade\",\"step\":%lld,\"model\":\"%s\",\"o\":%.3f,\"o5\":%.3f,\"o45\":%.3f,\"centre\":[%.2f,%.2f,%.2f],\"d\":%.2f}\n",static_cast<long long>(step),PopinName(o.model).c_str(),o.opacity,o.by_view[0],o.by_view[1],place.centre[0],place.centre[1],place.centre[2],place.d);Log(text);}
            const auto found=r.previous.find(key);if(found!=r.previous.end()){found->second.ids=o.ids;continue;}
            PopinReferenceEntry e;e.ids=o.ids;e.kind=o.kind;e.object=key^(uint64_t(o.model)<<1);e.name=PopinName(o.model);
            const auto place=PopinReferencePlace(o.kind,key^(uint64_t(o.model)<<1),o.owner);std::memcpy(e.centre,place.centre,sizeof e.centre);row(true,e,place);r.previous.emplace(key,std::move(e));}
        for(auto it=r.previous.begin();it!=r.previous.end();){if(now.contains(it->first)){++it;continue;}
            PopinPlace place;std::memcpy(place.centre,it->second.centre,sizeof place.centre);place.d=PopinDistance(place.centre);
            row(false,it->second,place);it=r.previous.erase(it);}
        // I-0115: the near characters' emits of the window against the previous window's.
        {std::unordered_map<uintptr_t,PopinReferenceNearEntry> near_now;{const std::lock_guard hold(popin_reference_near.lock);near_now.swap(popin_reference_near.current);}
            auto& near_previous=popin_reference_near.previous;
            const auto near_row=[&](const char* why,uintptr_t entity,const PopinReferenceNearEntry& e){char text[360]{};
                sprintf_s(text,"{\"event\":\"popin_reference_near\",\"step\":%lld,\"entity\":\"%llx\",\"model\":\"%s\",\"why\":\"%s\",\"d\":%.2f,\"k\":[%u,%u,%u],\"dc\":%.3f,\"o\":%.3f}\n",
                    static_cast<long long>(step),static_cast<unsigned long long>(entity),e.name.c_str(),why,e.d,e.k[0],e.k[1],e.k[2],e.dc,e.o);Log(text);};
            for(auto& [entity,e]:near_now){const auto found=near_previous.find(entity);
                if(found!=near_previous.end() && found->second.model==e.model){e.name=found->second.name;if(found->second.code==e.code)continue;}
                else e.name=PopinName(e.model);
                if(e.name.size()!=5 || e.name[0]!='c' || !std::isdigit(static_cast<unsigned char>(e.name[1])))continue;
                near_row("emitted",entity,e);}
            for(const auto& [entity,e]:near_previous)if(!near_now.contains(entity) && !e.name.empty() && e.name[0]=='c'){
                PopinReferenceNearEntry gone=e;gone.d=PopinReferencePlace(1,entity,entity).d;near_row(gone.d>=0.f && gone.d<kPopinNear?"absent":"far",entity,gone);}
            for(auto it=near_now.begin();it!=near_now.end();)it=it->second.name.size()==5 && it->second.name[0]=='c'?std::next(it):near_now.erase(it);
            near_previous.swap(near_now);}
        // The depth rows' models of the window (tranche 2), on change.
        std::unordered_map<uintptr_t,uint32_t> casting;{const std::lock_guard hold(popin_reference_shadows.lock);casting.swap(popin_reference_shadows.current);}
        auto& shadows=popin_reference_shadows.previous;
        const auto shadow_row=[&](bool on,const std::string& name,uint32_t rows){char text[200]{};
            sprintf_s(text,"{\"event\":\"popin_reference_shadow\",\"step\":%lld,\"on\":%d,\"model\":\"%s\",\"rows\":%u}\n",static_cast<long long>(step),on?1:0,name.c_str(),rows);Log(text);};
        for(const auto& [model,rows]:casting)if(!shadows.contains(model)){auto name=PopinName(model);shadow_row(true,name,rows);shadows.emplace(model,std::move(name));}
        for(auto it=shadows.begin();it!=shadows.end();){if(casting.contains(it->first)){++it;continue;}shadow_row(false,it->second,0);it=shadows.erase(it);}}
    catch(...){}
}
using LodStep=void(__fastcall*)(uintptr_t,uintptr_t);
LodStep lod_step_original{};
const char* UiObservationMode() noexcept;
// I-0100 tranche 4 (Thomas, 2026-10-26: « et si la distance d'affichage était très basse et surtout fausse avec le natif ? »; the
// LOD threshold nearer after a death, RESULT tranche 2 item 7), popin-observe.flag, both renderers: DS3's own LOD decision, the
// step D1CCE0 above, per controller (a map object's SprjModelDispEntity E+0xB0, a draw entity's E+0xF0; the alternate tracks
// E+0x150/E+0x190 left out), one popin_lod row when its LOD (+0x70, the target +0x74) or its weight (+0x7C) or geometry weight
// (+0x80, the draw distance of passes 1..0x38) changes band (0, between, 1), with the distance the step compares (|centre +0x20
// - manager +0x80| x manager +0x150) and the two fade thresholds it computed from the LodParam row (*record: class +0x2C, the
// distances +0x14/+0x1C, their tables 0x14463F5A0/6A0 and biases 0x144791DF8/DFC); one popin_lod_scale row when the manager's
// scale or the biases (0x144791DF8..E04) or the LOD gate 0x14463F8A0 change. A new controller (+0x91, its first step) starts
// afresh, so a reload's records are rows of their own. At most 400,000 rows a process.
struct PopinLodState {int32_t lod{-1},target{-1};uint8_t weight{9},geometry{9};std::string name;};
struct PopinLodShard {std::mutex lock;std::unordered_map<uintptr_t,PopinLodState> map;};
PopinLodShard popin_lod_shards[16];
std::atomic<int64_t> popin_lod_rows{};
void PopinLodChange(uintptr_t record,uintptr_t manager,uint8_t first) noexcept {
    try{
        static const double tick_ms=[]{LARGE_INTEGER f{};QueryPerformanceFrequency(&f);return 1000.0/double(f.QuadPart);}();
        static const int64_t origin=[]{LARGE_INTEGER t{};QueryPerformanceCounter(&t);return t.QuadPart;}();
        const auto now_ms=[&]{LARGE_INTEGER t{};QueryPerformanceCounter(&t);return double(t.QuadPart-origin)*tick_ms;};
        float scale{};uint32_t globals[5]{};Read(manager+0x150,&scale,4);Read(image+0x4791DF8,globals,16);Read(image+0x463F8A0,&globals[4],4);
        // Step review L4: the six words hashed and compared without a lock (about 4,800 steps a frame on DS3's job threads); the lock
        // only when they changed.
        {static std::atomic<uint64_t> seen{~0ull};uint32_t now[6]{};std::memcpy(now,&scale,4);std::memcpy(now+1,globals,20);
            uint64_t hash=1469598103934665603ull;for(const auto w:now){hash^=w;hash*=1099511628211ull;}if(hash==~0ull)hash=0;
            bool changed{};if(seen.load(std::memory_order_relaxed)!=hash){static std::mutex lock;const std::lock_guard held(lock);changed=seen.load(std::memory_order_relaxed)!=hash;if(changed)seen.store(hash,std::memory_order_relaxed);}
            if(changed && popin_lod_rows.fetch_add(1,std::memory_order_relaxed)<400000){float f[3]{};std::memcpy(f,globals,12);uint32_t width{};Read(manager+0x110,&width,4);
                // The scale's inputs (6EB60: tan(fov/2) / tan(ref/2), divided by height / ref height when the latter is set).
                float fov{},ref_fov{};uint32_t height{},ref_height{};Read(manager+0x140,&fov,4);Read(manager+0x2D4,&ref_fov,4);Read(manager+0x114,&height,4);Read(manager+0x2D0,&ref_height,4);
                char row[480]{};sprintf_s(row,"{\"event\":\"popin_lod_scale\",\"mode\":\"%s\",\"ms\":%.1f,\"scale\":%g,\"width\":%u,\"bias\":[%g,%g,%g],\"lod_bias\":%d,\"lod_gate_bits\":\"%08x\",\"fov\":%g,\"ref_fov\":%g,\"height\":%u,\"ref_height\":%u}\n",
                    UiObservationMode(),now_ms(),scale,width,f[0],f[1],f[2],int32_t(globals[3]),globals[4],fov,ref_fov,height,ref_height);Log(row);}}
        uintptr_t table{};const bool disp=Read(record-0xB0,&table,8) && table==image+0x29489A0;
        if(!disp && ((Read(record-0x150,&table,8) && table==image+0x29489A0) || (Read(record-0x190,&table,8) && table==image+0x2948EF8)))return;   // an alternate track
        int32_t lod[2]{};float weights[2]{};uintptr_t param{};
        if(!Read(record+0x70,lod,8) || !Read(record+0x7C,weights,8) || !Read(record,&param,8) || !param)return;
        const auto band=[](float w)->uint8_t{return w<=0.f?0:w>=1.f?2:1;};
        const uint8_t weight=band(weights[0]),geometry=band(weights[1]);
        auto& shard=popin_lod_shards[(record>>4)&15];std::string name;int32_t was_lod{-1};uint8_t was_weight{9},was_geometry{9};
        {const std::lock_guard held(shard.lock);auto found=shard.map.find(record);
            if(found==shard.map.end()){if(shard.map.size()>=65536)return;found=shard.map.emplace(record,PopinLodState{}).first;}
            auto& s=found->second;if(first){s=PopinLodState{};}
            if(s.lod==lod[0] && s.target==lod[1] && s.weight==weight && s.geometry==geometry)return;
            was_lod=s.lod;was_weight=s.weight;was_geometry=s.geometry;s.lod=lod[0];s.target=lod[1];s.weight=weight;s.geometry=geometry;
            if(s.name.empty()){const auto entity=disp?record-0xB0:record-0xF0;uintptr_t model{},owner{};
                const bool draw=!disp && Read(entity,&owner,8) && owner==image+0x2948EF8;   // step review L5: another owner is named « ? »
                if(disp){uintptr_t head{},node{},item{};if(Read(entity+0x90,&head,8) && head && Read(head,&node,8) && node && node!=head && Read(node+0x10,&item,8) && item)Read(item+0x58,&model,8);}
                else if(draw){uintptr_t binding{};Read(entity+0xE8,&binding,8);if(binding)Read(binding+8,&model,8);}
                s.name=PopinName(model)+(disp?"/disp":"");}
            name=s.name;}
        if(popin_lod_rows.fetch_add(1,std::memory_order_relaxed)>=400000)return;
        float centre[3]{},camera[3]{},fields[5]{};uint8_t kind{},param_class{};Read(record+0x20,centre,12);Read(manager+0x80,camera,12);
        Read(param+0x14,fields,20);Read(param+0x2C,&param_class,1);Read(param+0x10,&kind,1);
        float tables[2]{},bias[2]{};Read(image+0x463F5A0+4ull*param_class,&tables[0],4);Read(image+0x463F6A0+4ull*param_class,&tables[1],4);std::memcpy(bias,globals,8);
        const float dx=centre[0]-camera[0],dy=centre[1]-camera[1],dz=centre[2]-camera[2];const float metres=std::sqrt(dx*dx+dy*dy+dz*dz);
        char row[900]{};sprintf_s(row,"{\"event\":\"popin_lod\",\"mode\":\"%s\",\"ms\":%.1f,\"record\":\"%llx\",\"model\":\"%s\",\"first\":%u,\"lod\":[%d,%d],\"was\":%d,\"w\":[%.3f,%.3f],\"was_w\":[%u,%u],"
            "\"m\":%.6g,\"d\":%.6g,\"scale\":%g,\"class\":%u,\"flags\":%u,\"fade_at\":%.6g,\"draw_at\":%.6g,\"hyst\":[%g,%g],\"centre\":[%.6g,%.6g,%.6g]}\n",
            UiObservationMode(),now_ms(),static_cast<unsigned long long>(record),name.c_str(),first,lod[0],lod[1],was_lod,weights[0],weights[1],was_weight,was_geometry,
            metres,metres*scale,scale,param_class,kind,tables[0]*fields[0]+bias[0],tables[1]*fields[2]+bias[1],fields[1],fields[3],centre[0],centre[1],centre[2]);Log(row);}
    catch(...){}
}
// I-0100 option A (Thomas's choice 2026-10-27, D-0222: by default in a native run, no-lod-launch-scale.flag opts out; in the
// Reference mode with lod-launch-scale.flag only). DS3's views (the class 6E780 builds: the LOD manager, StageRend's own view
// at +0x60, shadow and character views) copy a global reference {height, fov} (14473F6C8) when they are made; 6EB60 scales every
// distance by tan(fov/2) / tan(ref/2) x ref height / height. The global holds {720, 0.959931} until the first frame, then 6E770
// (from D1DE70 and E006A0, natively from UpdateNativeSceneCpu's own call of 6E770) rewrites it every frame with the camera's {height, fov}: so the views made at a launch keep 55° and 720
// (scale 0.757 at a 720 client, 0.378 at 1440), those made at every later load (StageRend makes them anew) the camera's (scale 1):
// every LOD and draw distance nearer after the first death or travel, in the original too (RESULT « The LOD scale study »).
// Option A keeps the global at its launch pair, Elden Ring's mechanism (its reference set once, 141A610B0) with DS3's launch
// value: 6E770's write is skipped, so every view of every load is made as at a launch, at any resolution. The global's only
// reader is the constructor 6E780 (the study's census and its reviewer).
using LodReferenceSet=void(__fastcall*)(uintptr_t);
LodReferenceSet lod_reference_set_original{};
void __fastcall KeepLodReference(uintptr_t pair) noexcept {
    static std::atomic<bool> logged{false};
    if(!logged.exchange(true,std::memory_order_relaxed)){uint32_t kept[2]{},camera[2]{};Read(image+0x473F6C8,kept,8);Read(pair,camera,8);
        char row[200]{};sprintf_s(row,"{\"event\":\"native_lod_reference_kept\",\"ref_height\":%u,\"ref_fov\":%g,\"camera_height\":%u,\"camera_fov\":%g}\n",
            kept[0],std::bit_cast<float>(kept[1]),camera[0],std::bit_cast<float>(camera[1]));Log(row);}
}
void __fastcall ObserveLodStep(uintptr_t record,uintptr_t manager) noexcept {
    uint8_t first{};if(PopinObserve())Read(record+0x91,&first,1);   // the step clears its first-step flag
    lod_step_original(record,manager);
    if(PopinObserve()){popin_lod_manager.store(manager,std::memory_order_relaxed);PopinLodChange(record,manager,first);}   // I-0100 T0: the camera the distances use
    static const bool observe=ModuleFlag(L"light-observe.flag");if(!observe)return;
    static LONG64 calls{};const auto n=InterlockedIncrement64(&calls);
    float weights[2]{};uint8_t flags[2]{};if(!Read(record+0x7C,weights,8) || !Read(record+0x91,&flags[0],1) || !Read(record+0x95,&flags[1],1))return;
    const bool sample=n%20000==0;
    // Optimisation review (the m33 crosses o331204-o331208, zero_fade natively, drawn by the original): a map object's
    // SprjModelDispEntity holds this controller at E+0xB0 (MENUS-MAP 62 section 5); its name is its first child's model and
    // its rows are kept whatever the weight (once per name and band), with the LodParam id byte E+0x84.
    uintptr_t disp_table{};const bool disp=Read(record-0xB0,&disp_table,8) && disp_table==image+0x29489A0;
    if(!sample && !disp && !(weights[1]<1.f))return;
    const auto entity=disp?record-0xB0:record-0xF0;uintptr_t binding{},model{};
    if(disp){uintptr_t head{},node{},item{};if(Read(entity+0x90,&head,8) && head && Read(head,&node,8) && node && node!=head && Read(node+0x10,&item,8) && item)Read(item+0x58,&model,8);}
    else{Read(entity+0xE8,&binding,8);if(binding)Read(binding+8,&model,8);}
    uint8_t lod_param{};if(disp)Read(entity+0x84,&lod_param,1);
    const auto name=(model?NativeModelDataName(model):std::string("?"))+(disp?"/disp"+std::to_string(lod_param):std::string());const int band=weights[1]<=0.f?0:weights[1]<0.5f?1:2;
    static std::mutex lock;static std::set<std::pair<std::string,int>> seen;bool fresh{};
    {const std::lock_guard held(lock);fresh=sample || (seen.size()<(disp?400u:96u) && seen.insert({name,band}).second);}
    if(!fresh)return;
    // I-0120 H4: +0x70..+0x77 (two int32) and +0x110 (two uint32) as PopinLodChange reads them; +0x78 not identified, not read.
    float centre[3]{},camera[3]{},scale{};int32_t lod[2]{};uint32_t extent[2]{};
    Read(record+0x20,centre,12);Read(manager+0x80,camera,12);Read(manager+0x110,extent,8);Read(manager+0x150,&scale,4);Read(record+0x70,lod,8);
    const float dx=centre[0]-camera[0],dy=centre[1]-camera[1],dz=centre[2]-camera[2];const float d=std::sqrt(dx*dx+dy*dy+dz*dz)*scale;
    char row[480]{};sprintf_s(row,"{\"event\":\"lod_step\",\"mode\":\"%s\",\"call\":%lld,\"model\":\"%.60s\",\"weight\":%g,\"geometry_weight\":%g,\"first\":%u,\"enabled\":%u,\"lod\":[%d,%d],\"centre\":[%g,%g,%g],\"camera\":[%g,%g,%g],\"extent\":[%u,%u],\"scale\":%g,\"d\":%g}\n",
        UiObservationMode(),static_cast<long long>(n),name.c_str(),weights[0],weights[1],flags[0],flags[1],lod[0],lod[1],centre[0],centre[1],centre[2],camera[0],camera[1],camera[2],extent[0],extent[1],scale,d);Log(row);
}
void __fastcall ObserveLightRecording(uintptr_t a1,uintptr_t pass,uintptr_t context,uintptr_t a4,uintptr_t a5) noexcept {
    static volatile LONG64 calls{};const auto n=InterlockedIncrement64(&calls);
    {int32_t pipeline{};if(Read(context+0x78,&pipeline,4) && pipeline==1)InterlockedIncrement64(&observed_world_recordings);}
    int32_t pipeline_kind{};Read(context+0x78,&pipeline_kind,4);   // review: the world view (1) sampled on its own cadence, the first 32 calls are the title's
    {   // I-0140 tranche 7: a row at each change of a context's ShadowParam keys, weight or record keys (a fade's every frame), at most 2,000.
        static std::mutex lock;static std::map<uintptr_t,std::array<uint32_t,9>> last;static int rows{};
        std::array<uint32_t,9> now{};Read(context+0x3FA8,now.data(),20);Read(context+0x90+0x20,now.data()+5,16);
        bool changed{};{const std::lock_guard hold(lock);if(rows<2000){auto& seen=last[context];changed=seen!=now;if(changed){seen=now;++rows;}}}
        if(changed){float direction[3]{};Read(context+0x90+0x10,direction,12);char row[400]{};
            sprintf_s(row,"{\"event\":\"reference_shadow_source\",\"call\":%lld,\"context\":%llu,\"w78\":%d,\"keys\":[%u,%u,%u,%u,%g],\"record_keys\":[%u,%u,%u,%u],\"direction\":[%g,%g,%g]}\n",
                static_cast<long long>(n),static_cast<unsigned long long>(context),pipeline_kind,now[0],now[1],now[2],now[3],std::bit_cast<float>(now[4]),now[5],now[6],now[7],now[8],
                direction[0],direction[1],direction[2]);Log(row);}}
    if(n<=32 || n%300==0 || (pipeline_kind==1 && n%120==0)){int32_t w78{},w7c{};uint8_t b3ebc{},b3ebf{},b2da8{};
        Read(context+0x78,&w78,4);Read(context+0x7C,&w7c,4);Read(context+0x3EBC,&b3ebc,1);Read(context+0x3EBF,&b3ebf,1);Read(context+0x2DA8,&b2da8,1);
        // Review (m33 darker picture): the context's 13 gparam key slots A/B and weights (+0x3FD8, +0x4040, +0x40A8), the keys
        // FB890 feeds YEBIS with and the native CollectNativeYebisInputs reads from the same words.
        std::string keys;for(uint32_t i=0;i<13;++i){uint32_t a[2]{},b[2]{};float w{};Read(context+0x3FD8+8ull*i,a,8);Read(context+0x4040+8ull*i,b,8);Read(context+0x40A8+4ull*i,&w,4);
            char cell[96]{};sprintf_s(cell,"%s[%u,%u,%u,%u,%g]",i?",":"",a[0],a[1],b[0],b[1],w);keys+=cell;}
        // I-0140 tranche 7: the ShadowParam keys A +0x3FA8, B +0x3FB0, weight +0x3FB8, the byte +0x3ED0 and 88A10's record +0x90 (its
        // Angle +0x00, direction +0x10, keys +0x20..+0x2C, weight +0x30, Shadow Color +0x8C), what F90F0 takes the sun's shadow from.
        uint32_t shadow_keys[4]{},record_keys[4]{},colour{};float shadow_weight{},record_weight{},angle[2]{},direction[3]{};uint8_t b3ed0{};
        Read(context+0x3FA8,shadow_keys,16);Read(context+0x3FB8,&shadow_weight,4);Read(context+0x3ED0,&b3ed0,1);Read(context+0x90,angle,8);
        Read(context+0x90+0x10,direction,12);Read(context+0x90+0x20,record_keys,16);Read(context+0x90+0x30,&record_weight,4);Read(context+0x90+0x8C,&colour,4);
        char row[2000]{};sprintf_s(row,"{\"event\":\"reference_light_recording\",\"call\":%lld,\"pass\":%llu,\"context\":%llu,\"w78\":%d,\"w7c\":%d,\"b3ebc\":%u,\"b3ebf\":%u,\"b2da8\":%u,"
            "\"shadow_keys\":[%u,%u,%u,%u,%g],\"b3ed0\":%u,\"shadow_record\":{\"angle\":[%g,%g],\"direction\":[%g,%g,%g],\"keys\":[%u,%u,%u,%u,%g],\"colour\":%u},\"gparam_keys\":[%s]}\n",
            static_cast<long long>(n),static_cast<unsigned long long>(pass),static_cast<unsigned long long>(context),w78,w7c,b3ebc,b3ebf,b2da8,
            shadow_keys[0],shadow_keys[1],shadow_keys[2],shadow_keys[3],shadow_weight,b3ed0,angle[0],angle[1],direction[0],direction[1],direction[2],
            record_keys[0],record_keys[1],record_keys[2],record_keys[3],record_weight,colour,keys.c_str());Log(row);}
    light_recording_original(a1,pass,context,a4,a5);
}
// 1186 velocity witnesses (Reference with MotionBlur ENABLE; I-0068 VELOCITY-MOTIONBLUR M1-M4). Pass-through.
// 102380(view, context, pipeline): the velocity pass of group 0x33; 13BFC0(filter, context, .., depth object, current
// view-projection): the camera velocity record; 13D810(filter, context+460, depth object, record, intensity): its draw;
// D079C0(entity, ..): the SprjModelDrawEntity emit that copies the velocity gate E+BA5 && E+BA6 && E+184; 16050: the pose
// ring advance. Bounded rows: first calls, then every 300th.
using VelocityPass=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t);
VelocityPass velocity_pass_original{};
using CameraVelocityRecord=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
CameraVelocityRecord camera_velocity_record_original{};
using CameraVelocityDraw=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,float);
CameraVelocityDraw camera_velocity_draw_original{};
using DrawEntityEmit=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
DrawEntityEmit draw_entity_emit_original{};
using PoseRingUpdate=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uint32_t);
PoseRingUpdate pose_ring_update_original{};
// The GX texture interface's format query (vt+30, as 85540 asks its depth objects); SEH-guarded.
__declspec(noinline) static int GuardedTextureFormat(uintptr_t object) noexcept {
    uintptr_t table{},query{};
    if(!Read(object,&table,8) || !Read(table+0x30,&query,8) || query<image || query>=image+0x2800000)return -1;
    const bool outer=guarded_copy_active;guarded_copy_active=true;int format=-2;
    __try{format=reinterpret_cast<int(__fastcall*)(uintptr_t)>(query)(object);}__except(EXCEPTION_EXECUTE_HANDLER){format=-3;}
    guarded_copy_active=outer;return format;
}
std::string VelocityObjectCell(const char* name,uintptr_t object) {
    uintptr_t table{};Read(object,&table,8);std::array<uint32_t,24> words{};const bool raw=object && Read(object,words.data(),sizeof(words));
    std::string cell="\""+std::string(name)+"\":{\"present\":"+(object?"true":"false");
    if(object){char text[96]{};sprintf_s(text,",\"vtable\":%llu,\"format\":%d,\"words\":[",static_cast<unsigned long long>(table>=image?table-image:table),GuardedTextureFormat(object));cell+=text;
        for(size_t i=0;raw && i<words.size();++i){char w[16]{};sprintf_s(w,"%s%u",i?",":"",words[i]);cell+=w;}cell+="]";}
    return cell+"}";
}
// Lightmap floors (Reference): 19CA80, slot 1 of the draw's GXFlverModelDrawInstanceParam (1A9C30, before the ForLightMap draw):
// the param's lightmap set +0x30 and flag +0x38, the draw parameter bit 13 (+0x18C), each set object's class. Pass-through, first
// calls and every 20000th.
using LightmapBind=void(__fastcall*)(uintptr_t,uintptr_t,uint32_t,uintptr_t);   // review: R9 forwarded unchanged
LightmapBind lightmap_bind_original{};
void __fastcall ObserveLightmapBind(uintptr_t param,uintptr_t draw,uint32_t index,uintptr_t a4) noexcept {
    static LONG64 calls{};const auto n=InterlockedIncrement64(&calls);
    uintptr_t set{};Read(param+0x30,&set,8);static LONG64 with_set{};
    if(n<=4 || n%20000==0 || (set && InterlockedIncrement64(&with_set)<=16)){BYTE flag{};uint32_t bits{};std::array<uintptr_t,5> objects{};std::string cells;Read(param+0x38,&flag,1);Read(draw+0x18C,&bits,4);if(set)Read(set,objects.data(),sizeof(objects));
        for(const auto object:objects){uintptr_t table{},inner{},inner_table{};if(object){Read(object,&table,8);if(table==image+0x29559E8 && Read(object+0x10,&inner,8) && inner)Read(inner,&inner_table,8);}
            char cell[120]{};sprintf_s(cell,"%s[%llu,%llu]",cells.empty()?"":",",static_cast<unsigned long long>(table>image?table-image:0),static_cast<unsigned long long>(inner_table>image?inner_table-image:0));cells+=cell;}
        // Which object holds the set: the nearest map piece item or model draw entity below it, and whether its +8 / +0xD8 is this param.
        long long owner_offset=-1;int owner_kind=0;bool owns_param=false;
        for(uintptr_t k=0;set && k<=0x1000;k+=8){uintptr_t word{};if(!Read(set-k,&word,8))break;
            if(word==image+0x2951FF8 || word==image+0x2948EF8){owner_offset=static_cast<long long>(k);owner_kind=word==image+0x2951FF8?1:2;
                uintptr_t held{};owns_param=Read(set-k+(owner_kind==1?8:0xD8),&held,8) && held==param;break;}}
        char row[560]{};sprintf_s(row,"{\"event\":\"reference_lightmap_bind\",\"call\":%lld,\"index\":%d,\"set\":%s,\"flag38\":%u,\"bit13\":%u,\"owner\":[%d,%lld,%d],\"objects\":[%s]}\n",n,static_cast<int>(index),
            set?"true":"false",flag,(bits>>13)&1u,owner_kind,owner_offset,owns_param?1:0,cells.c_str());Log(row);}
    lightmap_bind_original(param,draw,index,a4);
}// m50 sky (Native1872: FC_DynamicDiffuseColor zero natively, the materials' flag +0x20 clear): 22E930(a1, slot entry, source, a4)
// writes +0x180 of the entry's CPU image [entry+8] only with the source's +0x20; with cascade-observe.flag, the image's +0x180..
// +0x18F after the update for the first 32 distinct images whose source flag is clear (what the original keeps there), and 8 set.
using MatDynUpdate=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t);
MatDynUpdate matdyn_update_original{};
void __fastcall ObserveMatDynUpdate(uintptr_t a1,uintptr_t entry,uintptr_t source,uintptr_t a4) noexcept {
    matdyn_update_original(a1,entry,source,a4);
    static const bool on=ModuleFlag(L"cascade-observe.flag");if(!on)return;
    BYTE flag{};uintptr_t cpu{};if(!Read(source+0x20,&flag,1) || !Read(entry+8,&cpu,8) || !cpu)return;
    static std::mutex lock;static std::set<uintptr_t> seen[2];bool fresh{};
    {const std::lock_guard hold(lock);auto& set=seen[flag?1:0];fresh=set.size()<(flag?8u:32u) && set.insert(cpu).second;}
    if(fresh){float v[4]{};Read(cpu+0x180,v,16);uint32_t word0{};Read(cpu,&word0,4);
        char row[200]{};sprintf_s(row,"{\"event\":\"reference_matdyn_diffuse\",\"flag20\":%u,\"diffuse\":[%g,%g,%g,%g],\"word0\":%u}\n",flag,v[0],v[1],v[2],v[3],word0);Log(row);}
}// Phantoms (Reference1878/Native1878: the NPC phantom at a summon sign shows its inner surfaces natively): 64610 picks the face
// set's cull class through 14473C9F8 row 2 {0,0,0} (draw state +0x180 bit 9, or +0x184 == 2), row 1 {0,2,1} (bit 7, or +0x184
// == 1), else row 0 {0,1,2}. With cascade-observe.flag, the first 64 distinct (state type, row, model name) with a row other than 0.
using GeometryDraw=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
GeometryDraw geometry_draw_original{};
void __fastcall ObserveGeometryDraw(uintptr_t a1,uintptr_t model,uintptr_t state,uintptr_t a4,uintptr_t a5,uintptr_t a6,uintptr_t a7,uintptr_t a8) noexcept {
    static const bool on=ModuleFlag(L"cascade-observe.flag");
    if(on){uint32_t flags{};int32_t mode{},type{};Read(state+0x180,&flags,4);Read(state+0x184,&mode,4);Read(state+0x1C,&type,4);
        const int row=((flags>>9)&1) || mode==2?2:(((flags>>7)&1) || mode==1)?1:0;
        if(row){static std::mutex lock;static std::set<std::tuple<int,int,uintptr_t>> seen;bool fresh{};
            {const std::lock_guard hold(lock);fresh=seen.size()<64 && seen.emplace(type,row,model).second;}
            if(fresh){std::string name;for(const auto ch:NativeModelDataName(model))name.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');
                char row_text[256]{};sprintf_s(row_text,"{\"event\":\"reference_cull_row\",\"type\":%d,\"row\":%d,\"flags180\":%u,\"mode184\":%d,\"model\":\"%.60s\"}\n",type,row,flags,mode,name.c_str());Log(row_text);}}
        // White equipment parts (TestAnim shins): the meshes of the player's part models (FC/HD/BD/AM/LG/WP names) the original emits,
        // with the mesh's material index (+0x20), the material's capability words (+8/+0x10) and the instance's words +0x18..+0x30
        // (63CB0: the class masks and the per-mesh-ordinal mask). First 160 distinct (model, mesh).
        static std::atomic<bool> part_full{};
        if(!part_full.load(std::memory_order_relaxed)){const auto name=NativeModelDataName(model);
            if(name.size()>2 && (name.rfind("FC",0)==0 || name.rfind("HD",0)==0 || name.rfind("BD",0)==0 || name.rfind("AM",0)==0 || name.rfind("LG",0)==0 || name.rfind("WP",0)==0)){
                static std::mutex part_lock;static std::set<std::pair<uintptr_t,uintptr_t>> part_seen;bool fresh{};
                {const std::lock_guard hold(part_lock);fresh=part_seen.size()<160 && part_seen.emplace(model,a4).second;if(part_seen.size()>=160)part_full.store(true);}
                if(fresh){int32_t material_index{-1};uint64_t material_words[2]{},instance_words[4]{};Read(a4+0x20,&material_index,4);Read(a5+8,material_words,16);Read(a6+0x18,instance_words,32);
                    char text[400]{};sprintf_s(text,"{\"event\":\"reference_part_mesh_draw\",\"model\":\"%.40s\",\"type\":%d,\"material_index\":%d,\"material\":[\"%llX\",\"%llX\"],\"instance\":[\"%llX\",\"%llX\",\"%llX\",\"%llX\"],\"index\":%llu}\n",
                        name.c_str(),type,material_index,material_words[0],material_words[1],instance_words[0],instance_words[1],instance_words[2],instance_words[3],static_cast<unsigned long long>(a7));Log(text);}}}}
    geometry_draw_original(a1,model,state,a4,a5,a6,a7,a8);
}void __fastcall ObserveVelocityPass(uintptr_t view,uintptr_t context,uintptr_t pipeline) noexcept {
    static LONG64 calls{};const auto n=InterlockedIncrement64(&calls);
    if(n<=3 || n%300==0){
        uintptr_t root{},options{},quality{},yebis{};BYTE option[2]{},capability[3]{},view_flags[2]{};uint32_t samples{};float factor{},k{};
        Read(image+0x4961B50,&root,8);if(root){Read(root+0x5360,&options,8);Read(root+0x5368,&quality,8);}
        if(options){Read(options+0x20,option,2);Read(options+0x24,&samples,4);}if(quality)Read(quality+0xE0,&factor,4);
        Read(image+0x4961F70,&yebis,8);Read(pipeline+0x3EB9,&capability[0],1);Read(pipeline+0x3ED2,&capability[1],1);Read(pipeline+0x3F70,&capability[2],1);
        Read(view+0x595,&view_flags[0],1);Read(view+0x9D0,&view_flags[1],1);Read(view+0xCA4,&k,4);
        uintptr_t graphics_options{};uint32_t levels[2]{};   // CED9A0's source ([4796298]+0xA0+0x18) and its copy GXRS+0x5330
        if(Read(image+0x4796298,&graphics_options,8) && graphics_options)Read(graphics_options+0xB8,&levels[0],4);if(root)Read(root+0x5330,&levels[1],4);
        std::string objects;
        for(const auto& [name,offset]:{std::pair<const char*,uint32_t>{"e20",0xE20},{"e30",0xE30},{"df0",0xDF0},{"e08",0xE08},{"e50",0xE50},{"e68",0xE68},{"o3980",0x3980},{"fd0",0xFD0}}){
            uintptr_t object{};Read(pipeline+offset,&object,8);objects+=(objects.empty()?"":",")+VelocityObjectCell(name,object);}
        std::string row="{\"event\":\"reference_velocity_pass\",\"call\":"+std::to_string(n);
        char text[480]{};sprintf_s(text,",\"option_20\":%u,\"option_21\":%u,\"sample_cap\":%u,\"factor\":%g,\"yebis\":%s,\"pipeline_3eb9\":%u,\"pipeline_3ed2\":%u,\"pipeline_3f70\":%u,\"view_595\":%u,\"view_9d0\":%u,\"view_ca4\":%g,\"options_level\":%u,\"root_level\":%u,\"objects\":{",
            option[0],option[1],samples,factor,yebis?"true":"false",capability[0],capability[1],capability[2],view_flags[0],view_flags[1],k,levels[0],levels[1]);
        row+=text+objects+"}}\n";Log(row.c_str());
        // D-0180 C4 (image regression, the sun record's set 0): the original's 137910 entries ([root+41F8]+88, count +90), each
        // {+8 light a, +10 light b, +18 fog a, +20 fog b, +28 light t, +2C fog t}; map pixels take set 0 (envData 0).
        if(root){uintptr_t entries{};int32_t count{};Read(root+0x41F8+0x88,&entries,8);Read(root+0x41F8+0x90,&count,4);std::string keys;
            for(int32_t j=0;entries && j<count && j<12;++j){uint32_t e[12]{};if(!Read(entries+0x30ull*j,e,sizeof(e)))break;
                char cell[160]{};sprintf_s(cell,"%s[%u,%u,%u,%u,%u,%u,%u,%u]",j?",":"",e[2],e[3],e[4],e[5],e[6],e[7],e[8],e[9]);keys+=cell;}
            const std::string entry_row="{\"event\":\"reference_light_entries\",\"call\":"+std::to_string(n)+",\"count\":"+std::to_string(count)+",\"keys\":["+keys+"]}\n";Log(entry_row.c_str());
            {uintptr_t banks{},fog_bank{};if(entries && count>0 && Read(root+0x41F8+0x98,&banks,8) && banks && Read(banks+0x28,&fog_bank,8) && fog_bank){
                const auto lookup=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,const void*)>(image+0x38E70);
                for(int32_t j=0;j<count && j<12;++j){uint32_t e[12]{};if(!Read(entries+0x30ull*j,e,sizeof(e)))break;const uintptr_t set=lookup(fog_bank,&e[6]);
                    float f[24]{};if(!set || !Read(set+0x10,f,sizeof f))continue;std::string record="{\"event\":\"reference_fog_set\",\"entry\":"+std::to_string(j)+",\"key\":["+std::to_string(e[6])+","+std::to_string(e[7])+"],\"builtin\":"+(set==fog_bank+0x180?"true":"false")+",\"f\":[";
                    for(int i=0;i<24;++i){char v[24]{};sprintf_s(v,"%s%.6g",i?",":"",f[i]);record+=v;}record+="]}\n";Log(record.c_str());}}}
            {uintptr_t banks{},light_bank{};if(entries && count>0 && Read(root+0x41F8+0x98,&banks,8) && banks && Read(banks+0x20,&light_bank,8) && light_bank){
                const auto lookup=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,const void*)>(image+0x32770);
                for(int32_t j=0;j<count && j<12;++j){uint32_t e[12]{};if(!Read(entries+0x30ull*j,e,sizeof(e)))break;const uintptr_t set=lookup(light_bank,&e[2]);
                    // I-0113 step review M1: whether 32770 gave the built-in default (bank +0x700) or a record node, for a key {area, -1}.
                    float f[64]{};if(!set || !Read(set+0x40,f,sizeof f))continue;std::string record="{\"event\":\"reference_light_set\",\"entry\":"+std::to_string(j)+",\"key\":["+std::to_string(e[2])+","+std::to_string(e[3])+"],\"builtin\":"+(set==light_bank+0x700?"true":"false")+",\"f\":[";
                    for(int i=0;i<64;++i){char v[24]{};sprintf_s(v,"%s%.6g",i?",":"",f[i]);record+=v;}record+="]}\n";Log(record.c_str());}
                // I-0113 (the Handmaid chair: its key {m40, -1} resolved to a registered node with default values here, {1, 0}
                // natively; the built-in offsets light bank +0x700, fog bank +0x180 are 32770's and 38E70's last returns): what 32770
                // gives in play for area 1's ids, the built-in default (bank +0x700) or a record (its first light's colour words).
                std::string area1;for(const uint32_t id:{0u,10u,11u,12u,13u,14u,15u}){const uint32_t key[2]{1u,id};const uintptr_t set=lookup(light_bank,key);float c[4]{};
                    const bool builtin=set==light_bank+0x700;if(set && !builtin)Read(set+0x40+0x10,c,sizeof c);char cell[160]{};
                    sprintf_s(cell,"%s{\"id\":%u,\"builtin\":%s,\"colour\":[%g,%g,%g,%g]}",area1.empty()?"":",",id,builtin?"true":"false",c[0],c[1],c[2],c[3]);area1+=cell;}
                Log(("{\"event\":\"reference_light_area1\",\"call\":"+std::to_string(n)+",\"ids\":["+area1+"]}\n").c_str());}}}
        // Lightmap floors (WAVE1-LAST-EFFECTS-STATIC-PREP 1.4): 102380's third argument is 103510's view context. Its key block
        // +0x3FC0..+0x413F (the 0x3FC0 pair and blend +0x3FD0, the index arrays A +0x3FD8, B +0x4040, weights +0x40A8, the pairs
        // +0x40E0, +0x40F8, the Dominant pair +0x4110/+0x4118 with the weight +0x4120 its setters write, +0x4128) and the
        // Dominant rows 23790 and 103510 read: 134A20([root+0x4680], pair)+8, eleven floats each.
        {std::array<uint32_t,0x60> block{};uintptr_t bank{};
            if(Read(pipeline+0x3FC0,block.data(),sizeof(block)) && root && Read(root+0x4680,&bank,8) && bank){
                const auto lookup=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uintptr_t)>(image+0x134A20);
                const uintptr_t first=lookup(bank,pipeline+0x4110),second=lookup(bank,pipeline+0x4118);
                std::array<float,11> a{},b{};const bool rows=first && second && Read(first+8,a.data(),44) && Read(second+8,b.data(),44);
                std::string dominant="{\"event\":\"reference_dominant_rows\",\"call\":"+std::to_string(n)+",\"block\":["+NativeWordsList(block.data(),block.size())+"],\"rows\":"+(rows?"true":"false");
                if(rows){dominant+=",\"a\":[";for(size_t i=0;i<a.size();++i){char v[24]{};sprintf_s(v,"%s%.6g",i?",":"",a[i]);dominant+=v;}
                    dominant+="],\"b\":[";for(size_t i=0;i<b.size();++i){char v[24]{};sprintf_s(v,"%s%.6g",i?",":"",b[i]);dominant+=v;}dominant+="]";}
                dominant+="}\n";Log(dominant.c_str());}}
        // Lightmap floors (Native1849: the High Wall lightmap names have a registry holder without a texture natively): the same
        // genuine D24490 lookups in the original, once the world runs (diagnostic names of one High Wall piece).
        static const bool lightmap_probe=ModuleFlag(L"cascade-observe.flag");   // review: Reference diagnostics only with cascade-observe.flag
        if(lightmap_probe && (n==600 || n==1800)){static const wchar_t* const probes[]{
                L"n:/fdp/data/model/map/m30_00_00_00/tex/30000000_m000760_gi_0000_00_dol_00.tif",L"n:/fdp/data/model/map/m30_00_00_00/tex/30000000_m000760_gi_0000_00_dol_00_l.tif",
                L"n:/fdp/data/model/map/m30_00_00_00/tex/30000000_m000760_gi_0000_00_dol_00_atlas000.tif",L"n:/fdp/data/model/map/m30_00_00_00/tex/30000000_m000760_gi_0000_00_dol_01.tif",L"SYSTEX_WHITE",L"SYSTEX_DUMMY",L"n:/fdp/data/model/map/m30_00_00_00/tex/30000000_m000760_gi_0000_00_dol_00_atlas001.tif"};
            std::string cells;size_t index{};
            for(const auto* probe:probes){
                // The holder D24490 returns, then the chain of +0x10 objects while their class is the holder's (29559E8), each
                // with its address, its table and its words +0x08..+0x27.
                uintptr_t object=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,const wchar_t*)>(image+0xD24490)(0,probe);std::string chain;
                for(int depth=0;object && depth<4;++depth){uintptr_t table{},next{};uint32_t words[8]{};Read(object,&table,8);Read(object+8,words,sizeof(words));Read(object+0x10,&next,8);
                    char link[320]{};sprintf_s(link,"%s{\"at\":\"%llX\",\"table_rva\":%llu,\"words\":[%s]}",chain.empty()?"":",",static_cast<unsigned long long>(object),
                        static_cast<unsigned long long>(table>image?table-image:0),NativeWordsList(words,8).c_str());chain+=link;
                    if(table!=image+0x29559E8)break;object=next;}
                cells+=(cells.empty()?"":",")+std::string("{\"probe\":")+std::to_string(index++)+",\"chain\":["+chain+"]}";}
            const std::string probe_row="{\"event\":\"reference_lightmap_probe\",\"call\":"+std::to_string(n)+",\"probes\":["+cells+"]}\n";Log(probe_row.c_str());}
        // 1255 QL1 (I-0067 QUALITY-OBJECT-NATIVE): the original's twelve option levels and quality object, for the native rows.
        if(n<=3 && graphics_options && options){std::array<int32_t,12> option_levels{};std::array<uint8_t,0x78> bytes{};
            Read(graphics_options+0xA8,option_levels.data(),sizeof(option_levels));Read(options,bytes.data(),bytes.size());
            std::string list,hex;for(const auto level:option_levels)list+=(list.empty()?"":",")+std::to_string(level);
            for(const auto byte:bytes){char cell[4]{};sprintf_s(cell,"%02x",byte);hex+=cell;}
            const std::string quality_row="{\"event\":\"reference_quality_object\",\"levels\":["+list+"],\"object\":\""+hex+"\"}\n";Log(quality_row.c_str());}}
    velocity_pass_original(view,context,pipeline);
}
void __fastcall ObserveCameraVelocityRecord(uintptr_t filter,uintptr_t context,uintptr_t a3,uintptr_t a4,uintptr_t depth,uintptr_t current) noexcept {
    static LONG64 calls{};static BYTE last_first{0xFF};const auto n=InterlockedIncrement64(&calls);
    BYTE first{};float intensity{};std::array<float,32> before{};Read(filter+0x10,&first,1);Read(filter+0x11C,&intensity,4);Read(filter+0x20,before.data(),sizeof(before));
    camera_velocity_record_original(filter,context,a3,a4,depth,current);
    BYTE after_first{};std::array<float,32> after{};std::array<float,16> now{};Read(filter+0x10,&after_first,1);Read(filter+0x20,after.data(),sizeof(after));Read(current,now.data(),sizeof(now));
    if(n<=4 || n%300==0 || first!=last_first){last_first=first;
        const auto list=[](const float* v,size_t count){std::string s;for(size_t i=0;i<count;++i){char c[24]{};sprintf_s(c,"%s%g",i?",":"",v[i]);s+=c;}return s;};
        const std::string row="{\"event\":\"reference_camera_velocity_record\",\"call\":"+std::to_string(n)+",\"first_before\":"+std::to_string(first)+",\"first_after\":"+std::to_string(after_first)+
            ",\"intensity_11c\":"+std::to_string(intensity)+",\"depth_format\":"+std::to_string(GuardedTextureFormat(depth))+",\"stored_before\":["+list(before.data(),32)+"],\"stored_after\":["+list(after.data(),32)+"],\"current\":["+list(now.data(),16)+"]}\n";
        Log(row.c_str());}
}
void __fastcall ObserveCameraVelocityDraw(uintptr_t filter,uintptr_t context,uintptr_t depth,uintptr_t record,float intensity) noexcept {
    static LONG64 calls{};const auto n=InterlockedIncrement64(&calls);
    if(n<=4 || n%300==0){std::array<float,16> matrix{};float depth_words[2]{},aspect{};uint32_t extent[2]{};uintptr_t camera{};
        Read(record,matrix.data(),sizeof(matrix));Read(context+0xB8,&depth_words[0],4);Read(context+0xC8,&depth_words[1],4);
        if(Read(context+0x210,&camera,8) && camera){Read(camera+0x64,&aspect,4);Read(camera+0x5C,extent,8);}
        std::string m;for(size_t i=0;i<16;++i){char c[24]{};sprintf_s(c,"%s%g",i?",":"",matrix[i]);m+=c;}
        char text[400]{};sprintf_s(text,"{\"event\":\"reference_camera_velocity_draw\",\"call\":%lld,\"intensity\":%g,\"depth_compute\":[%g,%g],\"camera_aspect\":%g,\"camera_extent\":[%u,%u],\"depth_format\":%d,\"record\":[",
            n,intensity,depth_words[0],depth_words[1],aspect,extent[0],extent[1],GuardedTextureFormat(depth));
        const std::string row=std::string(text)+m+"]}\n";Log(row.c_str());}
    camera_velocity_draw_original(filter,context,depth,record,intensity);
}
// 57A20(pass context, view, ...): the view whose +0x595 gates the 0x34 row and whose +0xCA4 scales FC_DifColMul.
using ModelRows=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
ModelRows model_rows_original{};
std::string NativeModelDataName(uintptr_t model);
uintptr_t ModelOfRowSource(uintptr_t source) noexcept {
    uintptr_t table{},getter{};if(!source || !Read(source,&table,8) || !Read(table+8,&getter,8) || !getter)return 0;
    const bool outer=guarded_copy_active;guarded_copy_active=true;uintptr_t model{};
    __try{model=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uint32_t)>(getter)(source,0);}__except(EXCEPTION_EXECUTE_HANDLER){model=0;}
    guarded_copy_active=outer;return model;
}
void __fastcall ObserveModelRows(uintptr_t a1,uintptr_t view,uintptr_t a3,uintptr_t a4,uintptr_t a5,uintptr_t a6,uintptr_t a7,uintptr_t a8,uintptr_t a9,uintptr_t a10) noexcept {
    static LONG64 calls{};static uintptr_t last_view{};const auto n=InterlockedIncrement64(&calls);
    if(n<=3 || n%200000==0 || (view!=last_view && n%1000==0)){last_view=view;BYTE enabled{};float scale{};uintptr_t table{};
        Read(view+0x595,&enabled,1);Read(view+0xCA4,&scale,4);Read(view,&table,8);
        char row[224]{};sprintf_s(row,"{\"event\":\"reference_model_rows_view\",\"call\":%lld,\"view\":%llu,\"vtable\":%llu,\"view_595\":%u,\"view_ca4\":%g}\n",
            n,static_cast<unsigned long long>(view),static_cast<unsigned long long>(table>=image?table-image:table),enabled,scale);Log(row);}
    // L0.1 (NPC-FAMILIES-STATIC §7): the model 57A20 reads its +220/+224 words from (param_9->vtable[1](param_9, 0)), once
    // per character model (data name c....), for the native comparison (native_character_engine7_row).
    // 1199: every model is remembered (map pieces included, up to 65536), so that characters met late are still logged.
    if(const auto model=ModelOfRowSource(a9)){static std::mutex lock;static std::unordered_set<uintptr_t> seen;bool fresh{};
        {const std::lock_guard hold(lock);fresh=seen.size()<65536 && seen.insert(model).second;}
        if(fresh){const auto name=NativeModelDataName(model);
            if(name.size()>1 && (name[0]=='c' || name[0]=='C') && name[1]>='0' && name[1]<='9'){uint32_t words[2]{};Read(model+0x220,words,8);
                char row[256]{};sprintf_s(row,"{\"event\":\"reference_character_row_words\",\"name\":\"%s\",\"model\":%llu,\"flags220\":%u,\"flags224\":%u}\n",
                    name.c_str(),static_cast<unsigned long long>(model),words[0],words[1]);Log(row);}}}
    // Phantoms (the sign's ghost before the summon, Native2069/2070): with light-observe.flag, a phantom draw state (a4 +0x87 = 1, the
    // CFDE70 stack copy of a ChrAsm part) as 57A20 reads it: kind bytes +0x84/+0x85, mulCol +0xD0..+0xDC (its .w already times the
    // row fade), one row per model and alpha change (at most 300), to compare with native_phantom_part_row's group_alpha x fade.
    {static const bool phantoms=ModuleFlag(L"light-observe.flag");BYTE bytes[4]{};
        if(phantoms && a4 && Read(a4+0x84,bytes,4) && bytes[3])if(const auto model=ModelOfRowSource(a9)){std::array<float,4> colour{};Read(a4+0xD0,colour.data(),16);
            static std::mutex lock;static std::unordered_map<uintptr_t,int> last;static int rows{};bool fresh{};const int alpha=static_cast<int>(std::lround(colour[3]*1000.f));
            {const std::lock_guard hold(lock);if(rows<300){const auto found=last.find(model);if(found==last.end() || found->second!=alpha){last[model]=alpha;++rows;fresh=true;}}}
            if(fresh){std::string name;for(const auto ch:NativeModelDataName(model))name.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');int32_t pass{-1};Read(view+0x124,&pass,4);
                char row[320]{};sprintf_s(row,"{\"event\":\"reference_phantom_state\",\"model\":\"%.60s\",\"pass\":%d,\"b84\":[%u,%u,%u,%u],\"mul_col\":[%g,%g,%g,%g]}\n",
                    name.c_str(),pass,bytes[0],bytes[1],bytes[2],bytes[3],colour[0],colour[1],colour[2],colour[3]);Log(row);}}}
    // I-0113 tranche 10d (Hidden Body, the hair; light-observe.flag): the player's part rows (data names FG_, HR_, FC_, BD_, AM_,
    // LG_, WP_) as 57A20 reads them: the draw state a4's fade bytes +0x84..+0x89, +0xDC, +0x124 and +0x98, the row descriptor a8's
    // first 20 bytes and the pass id, a row each time they change for that model and pass (at most 600 rows).
    {static const bool parts=ModuleFlag(L"light-observe.flag");static std::atomic<bool> parts_full{};int32_t pass{-1};
        if(parts && !parts_full.load(std::memory_order_relaxed) && a4 && a8 && Read(view+0x124,&pass,4))if(const auto model=ModelOfRowSource(a9)){const auto name=NativeModelDataName(model);
            if(name.size()>3 && name[2]=='_' && (name.starts_with("FG") || name.starts_with("HR") || name.starts_with("FC") || name.starts_with("BD") ||
                name.starts_with("AM") || name.starts_with("LG") || name.starts_with("WP"))){
                std::array<uint8_t,6> fade{};float alpha{},opacity{};uint32_t flags{};std::array<uint8_t,20> descriptor{};
                Read(a4+0x84,fade.data(),6);Read(a4+0xDC,&alpha,4);Read(a4+0x124,&opacity,4);Read(a4+0x98,&flags,4);Read(a8,descriptor.data(),descriptor.size());
                std::string key(reinterpret_cast<const char*>(fade.data()),6);key.append(reinterpret_cast<const char*>(&alpha),4);key.append(reinterpret_cast<const char*>(&opacity),4);
                key.append(reinterpret_cast<const char*>(descriptor.data()),descriptor.size());
                static std::mutex lock;static std::map<std::pair<uintptr_t,int32_t>,std::string> last;static int rows{};bool fresh{};   // keyed by model and pass: a4 is CFDE70's stack copy
                {const std::lock_guard hold(lock);if(rows<600){auto& previous=last[{model,pass}];if(previous!=key){previous=key;++rows;fresh=true;}}else parts_full=true;}
                if(fresh){std::string d;for(size_t i=0;i<descriptor.size();++i){char cell[8]{};sprintf_s(cell,"%s%02X",i?" ":"",descriptor[i]);d+=cell;}
                    char row[480]{};sprintf_s(row,"{\"event\":\"reference_part_row_state\",\"model\":\"%.40s\",\"state\":%llu,\"pass\":%d,\"b84\":[%u,%u,%u,%u,%u,%u],\"dc\":%g,\"o124\":%g,\"f98\":%u,\"descriptor\":\"%s\"}\n",
                        name.c_str(),static_cast<unsigned long long>(a4),pass,fade[0],fade[1],fade[2],fade[3],fade[4],fade[5],alpha,opacity,flags,d.c_str());Log(row);}}}}
    // Complementary check (Thomas, 2026-10-18, the m33 darkening): with light-observe.flag, the light/fog key block of the draw
    // state a4 (+F8..+120, the group layout 5A120 reads: light a, light b, fog a, fog b, light t, fog t) at the G-buffer pass 5,
    // logged each time an entity's block changes (at most 6,000 rows), to compare with native_model_light_key.
    {static const bool keys=ModuleFlag(L"light-observe.flag");int32_t pass{-1};
        if(keys && a4 && Read(view+0x124,&pass,4))if(const auto model=ModelOfRowSource(a9)){std::array<uint32_t,10> k{};
            if(Read(a4+0xF8,&k[0],16) && Read(a4+0x110,&k[4],16) && Read(a4+0x108,&k[8],4) && Read(a4+0x120,&k[9],4)){
                // Keyed by the draw state a4 (one per entity: several entities share a model with different keys), at most 6,000 rows.
                static std::mutex lock;static std::unordered_map<uintptr_t,std::array<uint32_t,10>> last;static int rows{};bool fresh{};
                {const std::lock_guard hold(lock);if(rows<6000){auto& previous=last[a4];if(previous!=k){previous=k;++rows;fresh=true;}}}
                if(fresh){std::string name;for(const auto ch:NativeModelDataName(model))name.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');
                    float t[2]{};std::memcpy(&t[0],&k[8],4);std::memcpy(&t[1],&k[9],4);char row[640]{};
                    // The instance lighting rows the engine's lookup (32A80 on [root]+41E0, the record's +0x290) gives for key a, as the
                    // native collect reads them (ProduceNativeSelectedInstances): the original's banks hold the zone's files.
                    std::string sh="null,\"strength\":null";
                    {uintptr_t root{},table{};if(Read(image+0x4961B50,&root,8) && root && Read(root+0x41E0,&table,8) && table){
                        const auto lookup=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uintptr_t)>(image+0x32A80);const uintptr_t bank=lookup(table,a4+0xF8);
                        std::array<float,16> one{};if(bank && Read(bank+0x290,one.data(),sizeof(one))){sh="[";for(size_t j=0;j<12;++j){char cell[24]{};sprintf_s(cell,"%s%.5g",j?",":"",one[j]);sh+=cell;}
                            char tail[32]{};sprintf_s(tail,"],\"strength\":%.5g",one[12]);sh+=tail;}}}
                    sprintf_s(row,"{\"event\":\"reference_model_light_key\",\"model\":\"%.60s\",\"group\":%llu,\"pass\":%d,\"light\":[%u,%u,%u,%u],\"light_t\":%g,\"fog\":[%u,%u,%u,%u],\"fog_t\":%g,\"sh\":%s}\n",
                        name.c_str(),static_cast<unsigned long long>(a4),pass,k[0],k[1],k[2],k[3],t[0],k[4],k[5],k[6],k[7],t[1],sh.c_str());Log(row);}}}}
    // m50 sky (S1/S2): with cascade-observe.flag, the first 400 (model, pass id) pairs outside the cascade and G-buffer passes, named.
    {static const bool rows=ModuleFlag(L"cascade-observe.flag");int32_t pass{-1};
        if(rows && Read(view+0x124,&pass,4) && pass>8)if(const auto model=ModelOfRowSource(a9)){static std::mutex lock;static std::set<std::pair<uintptr_t,int32_t>> seen;bool fresh{};
            {const std::lock_guard hold(lock);fresh=seen.size()<400 && seen.emplace(model,pass).second;}
            if(fresh){std::string name;for(const auto ch:NativeModelDataName(model))name.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');uint32_t words[2]{};Read(model+0x220,words,8);
                char row[256]{};sprintf_s(row,"{\"event\":\"reference_model_pass\",\"model\":\"%.80s\",\"pass\":%d,\"flags220\":%u,\"flags224\":%u}\n",name.c_str(),pass,words[0],words[1]);Log(row);}}}
    // Cascade observers: the casters of the cascade passes by kind (cascade-observe.flag).
    {static const bool casters=ModuleFlag(L"cascade-observe.flag");int32_t pass{-1};
        if(casters && Read(view+0x124,&pass,4) && pass>=1 && pass<=8){   // 57A20 reads the pass id at param_2 +0x124
            if(pass>=5){const std::lock_guard hold(reference_cascade_lock);++reference_cascades.gbuffer;}
            else if(const auto model=ModelOfRowSource(a9)){static std::mutex lock;static std::unordered_map<uintptr_t,int> kinds;int kind{-1};
                {const std::lock_guard hold(lock);if(const auto found=kinds.find(model);found!=kinds.end())kind=found->second;}
                if(kind<0){const auto name=NativeModelDataName(model);const char first=name.empty()?'?':static_cast<char>(std::tolower(static_cast<unsigned char>(name[0])));
                    kind=first=='m'?0:first=='c'?1:first=='o'?2:3;const std::lock_guard hold(lock);if(kinds.size()<65536)kinds[model]=kind;}
                const std::lock_guard hold(reference_cascade_lock);++reference_cascades.casters[pass-1][kind];}}}
    // I-0141 tranche 24 (« Suggested next step » 4, m32's 8 models without a G-buffer row): with pixel-keys.flag, once per 60-second
    // window (PixelKeyWindow, as reference_pixel_key's) and (model, the pass id 57A20 reads at view +0x124), a reference_model_pass_window
    // row: every pass the original makes a model's rows for in each window, the forward, depth and shadow passes included, which
    // reference_pixel_key's G-buffer state types leave out; against native_model_pass_window. Keyed by the model's address within a
    // window (the name written); a row whose time falls before the current window is dropped; at most 20,000 rows a window (the step
    // review's M1: one budget for the whole run could run out in the arrival's window), then one reference_model_pass_window_full row
    // naming the window.
    {static const bool pixel_keys=ModuleFlag(L"pixel-keys.flag");int32_t pass{-1};
        if(pixel_keys && Read(view+0x124,&pass,4))if(const auto model=ModelOfRowSource(a9)){const double ms=PixelKeyClockMs();
            static std::mutex lock;static std::set<std::pair<uintptr_t,int32_t>> seen;static uint64_t window{};static size_t written{};bool fresh{},last{};uint64_t at{};
            {const std::lock_guard held(lock);const auto now=PixelKeyWindow(ms);if(now>window){window=now;seen.clear();written=0;}   // forward only
                at=window;fresh=now>=window && written<20000 && seen.emplace(model,pass).second;if(fresh && ++written>=20000)last=true;}
            if(fresh){std::string clean;for(const auto ch:NativeModelDataName(model))clean.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');char row[200]{};
                sprintf_s(row,"{\"event\":\"reference_model_pass_window\",\"qpc_ms\":%.1f,\"model\":\"%.60s\",\"pass\":%d}\n",ms,clean.c_str(),pass);Log(row);
                if(last){sprintf_s(row,"{\"event\":\"reference_model_pass_window_full\",\"qpc_ms\":%.1f,\"window\":%llu,\"rows\":20000}\n",ms,static_cast<unsigned long long>(at));Log(row);}}}}
    model_rows_original(a1,view,a3,a4,a5,a6,a7,a8,a9,a10);
}
// 5A120(view, draw state, ...): the material row preparation; case 5 (Velo) writes FC_DifColMul at state +0x310 from the
// camera aspect and view+0xCA4 (1.0E-5 when not positive). Witness: the view, +0xCA4 and the written words.
using RowPreparation=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
RowPreparation row_preparation_original{};
void __fastcall ObserveRowPreparation(uintptr_t view,uintptr_t state,uintptr_t a3,uintptr_t a4,uintptr_t a5,uintptr_t a6) noexcept {
    row_preparation_original(view,state,a3,a4,a5,a6);
    int32_t pass{};if(!Read(state+0x1C,&pass,4) || pass!=1)return;
    static LONG64 calls{};const auto n=InterlockedIncrement64(&calls);
    if(n<=6 || n%20000==0){float scale{},words[4]{};uintptr_t camera{};Read(view+0xCA4,&scale,4);Read(state+0x310,words,16);Read(view+0x418,&camera,8);
        char row[256]{};sprintf_s(row,"{\"event\":\"reference_velo_preparation\",\"call\":%lld,\"view\":%llu,\"view_ca4\":%g,\"camera\":%s,\"fc_difcolmul\":[%g,%g,%g,%g]}\n",
            n,static_cast<unsigned long long>(view),scale,camera?"true":"false",words[0],words[1],words[2],words[3]);Log(row);}
}
void __fastcall ObserveDrawEntityEmit(uintptr_t entity,uintptr_t a2,uintptr_t a3,uintptr_t a4,uintptr_t a5) noexcept {
    static std::mutex lock;static std::map<uintptr_t,uint64_t> seen;uint64_t k{};
    {const std::lock_guard hold(lock);if(seen.size()<96 || seen.contains(entity))k=++seen[entity];}
    if(k==1 || k==100 || k==1000){uintptr_t table{};BYTE ca{},ba5{},ba6{},f184{},ba7{};
        Read(entity,&table,8);Read(entity+0xCA,&ca,1);Read(entity+0xBA5,&ba5,1);Read(entity+0xBA6,&ba6,1);Read(entity+0x184,&f184,1);Read(entity+0xBA7,&ba7,1);
        char row[256]{};sprintf_s(row,"{\"event\":\"reference_draw_entity_velocity_gate\",\"entity\":%llu,\"sample\":%llu,\"vtable\":%llu,\"e_ca\":%u,\"e_ba5\":%u,\"e_ba6\":%u,\"e_184\":%u,\"e_ba7\":%u}\n",
            static_cast<unsigned long long>(entity),k,static_cast<unsigned long long>(table>=image?table-image:table),ca,ba5,ba6,f184,ba7);Log(row);}
    // Cascade observers (cascade-observe.flag): the draw entities emitted in the cascade passes, with their category byte.
    {static const bool cascades=ModuleFlag(L"cascade-observe.flag");int32_t pass{-1};
        if(cascades && Read(a2+0x124,&pass,4) && pass>=1 && pass<=4){uint64_t bits{};Read(entity+0x20,&bits,8);
            const std::lock_guard hold(reference_cascade_lock);++reference_cascades.entity_emits[pass-1];++reference_cascades.entity_bits[static_cast<uint32_t>(bits&0xFF)];}
        // Fable review (the Crucifixion Woods cross): with cascade-observe.flag, the first 400 distinct draw entities the
        // original emits outside the cascades, with their model name, placement, category word and the +0x20 bits, to compare
        // with the native walk's candidates.
        if(cascades && !(pass>=1 && pass<=4)){static std::mutex names_lock;static std::set<uintptr_t> named;bool fresh{};
            {const std::lock_guard held(names_lock);fresh=named.size()<400 && named.insert(entity).second;}
            if(fresh){uintptr_t binding{},model{},matrices{};uint32_t category{};uint64_t bits{};BYTE enabled{};std::array<float,12> placement{};
                Read(entity+0xE8,&binding,8);if(binding)Read(binding+8,&model,8);Read(entity+0x70,&category,4);Read(entity+0x20,&bits,8);Read(entity+0xC3C,&enabled,1);
                if(Read(entity+0xB08+((enabled>>4)&1)*0x18+0x10,&matrices,8) && matrices)Read(matrices,placement.data(),sizeof(placement));
                char row[360]{};sprintf_s(row,"{\"event\":\"reference_draw_entity\",\"pass\":%d,\"model\":\"%.60s\",\"category\":%u,\"bits\":%llu,\"enabled\":%u,\"at\":[%g,%g,%g]}\n",
                    pass,model?NativeModelDataName(model).c_str():"?",category,static_cast<unsigned long long>(bits),enabled,placement[3],placement[7],placement[11]);Log(row);}}}
    draw_entity_emit_original(entity,a2,a3,a4,a5);
}
void __fastcall ObservePoseRingUpdate(uintptr_t receiver,uintptr_t source,uintptr_t reference,uint32_t worker) noexcept {
    int32_t before[2]{};Read(receiver+0x68,before,8);
    pose_ring_update_original(receiver,source,reference,worker);
    static std::mutex lock;static std::map<uintptr_t,uint64_t> seen;static std::array<uint64_t,3> rings{};uint64_t k{};static LONG64 calls{};
    const auto n=InterlockedIncrement64(&calls);int32_t after{};Read(receiver+0x68,&after,4);
    {const std::lock_guard hold(lock);if(before[1]>=0 && before[1]<=2 && !seen.contains(receiver) && seen.size()<4096)++rings[before[1]];
        if(seen.size()<4096 || seen.contains(receiver))k=++seen[receiver];}
    if((k<=2 && before[1]==2 && n<=4000) || n%20000==0){uintptr_t model{};Read(receiver+0x80,&model,8);
        char row[288]{};sprintf_s(row,"{\"event\":\"reference_pose_ring\",\"call\":%lld,\"receiver\":%llu,\"model\":%llu,\"ring\":%d,\"index_before\":%d,\"index_after\":%d,\"owners_ring1\":%llu,\"owners_ring2\":%llu,\"caller\":%llu}\n",
            n,static_cast<unsigned long long>(receiver),static_cast<unsigned long long>(model),before[1],before[0],after,rings[1],rings[2],
            static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(_ReturnAddress())-image));Log(row);}
}
void __fastcall ObserveAsmModelTick(uintptr_t model,float dt) noexcept {
    ObserveAsmModelUpdate("tick",model,dt,reinterpret_cast<uintptr_t>(_ReturnAddress()));asm_model_tick_original(model,dt);}
// 1073 witness: SprjModelDrawEntity update (vtable 2948EF8+C8 = D07710), for comparison.
void __fastcall ObserveNativeModelEntityUpdate(uintptr_t entity,uintptr_t context) noexcept {
    ffx_observe::Note(ffx_observe::EntityUpdate,reinterpret_cast<uintptr_t>(_ReturnAddress()),entity,context,0,0);   // W4 FW-1
    static LONG64 calls{};const auto n=InterlockedIncrement64(&calls);
    if(n<=6 || n%3000==0){float weight{};Read(context+8,&weight,4);
        char row[256]{};sprintf_s(row,"{\"event\":\"native_model_entity_update\",\"call\":%lld,\"weight\":%g,\"caller\":%llu}\n",
            n,weight,static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(_ReturnAddress())-image));Log(row);}
    model_entity_update_original(entity,context);
}
// I-0140 tranche 28 (D-0301's witness, light-observe.flag, both modes): D0C2F0 adds a menu view's own region (GILM0010, the cube at
// the origin, masks {0x11, 2}) only when GILM0010 is registered (D4ED20 on the registry 144792F58), its key {1, 10} is in the env
// bank (41990 on GXRS+0x42A8) and the manager is on (the original's 65D60 returns 0 otherwise, GXRS+0x42C0; natively D0C2F0's call
// turns it on when it is the registry's first request, so a row natively may read manager_on 0 with a handle); each call's row (64 at
// most) gives the three gates as they stand just before the call, the handle the slot held before and the one stored after
// (STUDY-MENU-ENV-REGIONS).
using MenuRegionAdd=void(__fastcall*)(uintptr_t);MenuRegionAdd menu_region_add_original{};
// Natively (16_offscreen_and_movies.inc, NativeMenuRegionAdd and NativeEnvBankHas): the key gate answered from the native banks.
thread_local bool menu_region_key_from_banks{};   // set by NativeMenuRegionAdd around D0C2F0
thread_local bool menu_region_key_answered{};     // whether the banks answered D0C2F0's own key gate (not the probe below)
using EnvBankHas=bool(__fastcall*)(uintptr_t,const uint32_t*);EnvBankHas env_bank_has_original{};   // natively 41990's genuine entry
void __fastcall ObserveMenuRegionAdd(uintptr_t slot) noexcept {
    uintptr_t root{},registry{},table{},before{},after{},texture{};uint8_t on{};bool known{};
    if(Read(image+0x4961B50,&root,8) && root){Read(root+0x42C0,&on,1);
        if(Read(root+0x42A8,&table,8) && table){const uint32_t key[2]{1,10};
            // key_known: the engine's bank alone in both modes (natively 41990's genuine entry, past the native banks' answer).
            known=(env_bank_has_original?env_bank_has_original:reinterpret_cast<EnvBankHas>(image+0x41990))(table,key);}}
    if(Read(image+0x4792F58,&registry,8) && registry)texture=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,const wchar_t*)>(image+0xD4ED20)(registry,L"GILM0010");
    Read(slot,&before,8);menu_region_key_answered=false;
    menu_region_add_original(slot);
    Read(slot,&after,8);
    static LONG64 calls{};const auto n=InterlockedIncrement64(&calls);
    if(n<=64){char row[320]{};_snprintf_s(row,sizeof row,_TRUNCATE,"{\"event\":\"menu_region_add\",\"call\":%lld,\"manager_on\":%u,\"registered\":%d,\"key_known\":%d,\"key_from_banks\":%d,\"before\":\"%llX\",\"handle\":\"%llX\",\"tick\":%llu,\"thread\":%lu}\n",
        n,unsigned(on),int(texture!=0),int(known),int(menu_region_key_answered),static_cast<unsigned long long>(before),static_cast<unsigned long long>(after),static_cast<unsigned long long>(GetTickCount64()),GetCurrentThreadId());Log(row);}
}
// I-0140 tranche 29 (Thomas's yes 2026-11-21 06:56, the inventory of ENV-BANK-CALLERS; light-observe.flag, both modes): the engine's
// env-map bank GXRS+0x42A8 and its two readers, by call site. 41990(bank, key) answers whether {area, id} is in the bank's two-level
// map, with no fallback; 416A0(bank, key) returns the key's entry, else {1, 0}'s, else the bank's default +0x1A8, a key {-1, -1}
// read as {1, 0} (READ: their disassembly, GhidraMCP 2026-11-21). Per return site (the game's RVA; « module+ » an offset in this
// module, e.g. NativeEnvMapCube's calls; « other+ » an address elsewhere) and reader: the calls; how many pass the bank GXRS+0x42A8
// itself (on_gxrs_bank); 41990's true answers from the engine's bank (natively also given_yes, the native's answer the game
// received: inside D0C2F0's scope it adds the native banks', NativeEnvBankAnswer); 416A0's answers (exact, {1, 0}'s, the default),
// its calls with the key {-1, -1} apart (« minus_one »), its entries' colour scale +0xC range (non-finite scales counted apart) and
// how many carry a cube +0x40; the first 6 distinct keys asked; natively, how many calls ask a key the native banks hold
// (Banks::EnvColorScale on the key 416A0 resolves, {-1, -1} as {1, 0}). One global table of atomics (128 sites), so a call counts
// the moment it is made, whatever its thread (per-thread slots lost a sparse thread's calls). A row env_bank_sites at a site's
// first call (« new_site ») and at the first call once the timer is due (20 s, 60 s once 30 rows of either kind are written, from
// the last call that found it due, a new-site row's included);
// both kinds share 200 rows, the one numbered 200 marked rows_exhausted (found by its flag, rows from two threads may log out of
// order); so while rows remain and the table has room, every site called appears in a row (a 129th site only sets sites_full in
// the rows after it); the counts stop at each site's last row: every site so far, the
// bank GXRS+0x42A8's address and its state: its areas, each one's entry count and first ids (the outer list +0x8 with its count
// +0x10, a node's area +0x10 and inner map +0x18, whose list +0 and count +8 hold the ids at a node's +0x10: as 41990 and 416A0
// walk them, READ; the counts' places INFERRED, MSVC's unordered_map), and the keys +0xD8 and +0xE0 770C0 asks when a group's key
// id is -1. A value that does not apply to a reader or a mode, or that could not be read, is null.
using EnvBankGet=uintptr_t(__fastcall*)(uintptr_t,const uint32_t*);EnvBankGet env_bank_get_original{};
namespace env_bank_witness {
constexpr size_t kKeys=6,kSites=128;constexpr uint64_t kNoKey=~0ull;constexpr int kRows=200;
constexpr uint64_t kWhere=(1ull<<60)-1;   // a site code: its kind in bits 60-61, the RVA, offset or address below
struct Site {std::atomic<uint64_t> id{},calls{},on_gxrs{},yes{},given_yes{},exact{},via_one_zero{},fallback{},minus_one{},cubes{},banks{},scaled{},nonfinite{};
    std::atomic<uint32_t> low{0x7F800000u},high{0xFF800000u};std::atomic<uint64_t> keys[kKeys];
    Site() noexcept {for(auto& k:keys)k.store(kNoKey,std::memory_order_relaxed);}};
Site sites[kSites];std::atomic<bool> full{};std::atomic<uint64_t> next_row_ms{};std::atomic<int> rows{};
bool Enabled() noexcept {static const bool on=ModuleFlag(L"light-observe.flag");return on;}
// The site's code: kind 0 the game, 1 this module, 2 elsewhere (bits 60-61, kept by Find's shift).
uint64_t SiteCode(uintptr_t at) noexcept {
    static const auto end_of=[](uintptr_t base) noexcept {const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        return base+reinterpret_cast<const IMAGE_NT_HEADERS64*>(base+dos->e_lfanew)->OptionalHeader.SizeOfImage;};
    static const uintptr_t image_end=end_of(image);
    static const uintptr_t self=[]{HMODULE m{};GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&Enabled),&m);return reinterpret_cast<uintptr_t>(m);}();
    static const uintptr_t self_end=self?end_of(self):0;
    if(at>=image && at<image_end)return at-image;
    if(self && at>=self && at<self_end)return (1ull<<60)|(at-self);
    return (2ull<<60)|(at&kWhere);}
// The site's slot, claimed by the first call (linear probing; 0 is an empty slot; claimed: this call took it); null once the
// table is full.
Site* Find(uint64_t code,int reader,bool& claimed) noexcept {
    const uint64_t id=((code<<1)|static_cast<uint64_t>(reader))+1;claimed=false;
    size_t h=static_cast<size_t>((id*0x9E3779B97F4A7C15ull)>>57);
    for(size_t i=0;i<kSites;++i,h=(h+1)%kSites){
        uint64_t seen=sites[h].id.load(std::memory_order_acquire);
        if(!seen && sites[h].id.compare_exchange_strong(seen,id,std::memory_order_acq_rel)){claimed=true;return &sites[h];}
        if(seen==id)return &sites[h];}
    full.store(true,std::memory_order_relaxed);return nullptr;}
void Bound(std::atomic<uint32_t>& slot,float value,bool lower) noexcept {
    uint32_t bits{};std::memcpy(&bits,&value,4);uint32_t now=slot.load(std::memory_order_relaxed);
    for(;;){float current{};std::memcpy(&current,&now,4);
        if(lower?!(value<current):!(value>current))return;
        if(slot.compare_exchange_weak(now,bits,std::memory_order_relaxed))return;}}
void AddKey(Site& s,uint64_t key) noexcept {
    for(auto& k:s.keys){uint64_t seen=k.load(std::memory_order_relaxed);if(seen==key)return;
        if(seen==kNoKey && (k.compare_exchange_strong(seen,key,std::memory_order_relaxed) || seen==key))return;}}
std::string Hex(uint64_t v) {char text[24]{};_snprintf_s(text,sizeof text,_TRUNCATE,"%llX",static_cast<unsigned long long>(v));return text;}
// The bank GXRS+0x42A8, read from the global GXRS [0x144961B50]; 0 when unreadable.
uintptr_t GxrsBank() noexcept {uintptr_t root{},bank{};return Read(image+0x4961B50,&root,8) && root && Read(root+0x42A8,&bank,8)?bank:0;}
// The bank's areas and ids (16 areas, 12 ids each at most) and its keys +0xD8, +0xE0.
std::string BankState(uintptr_t bank) {
    std::string out="{\"areas\":[";uintptr_t head{},node{};uint64_t count{};
    const bool counted=Read(bank+0x10,&count,8);
    if(counted && Read(bank+8,&head,8) && head && Read(head,&node,8)){
        for(uint64_t a=0;node && node!=head && a<16 && a<count;++a){uint32_t area{};uintptr_t inner{},inner_head{},inner_node{};uint64_t ids{};
            if(!Read(node+0x10,&area,4) || !Read(node+0x18,&inner,8) || !inner || !Read(inner,&inner_head,8) || !Read(inner+8,&ids,8))break;
            out+=(a?",":"")+std::string("{\"area\":")+std::to_string(area)+",\"count\":"+std::to_string(ids)+",\"ids\":[";
            if(inner_head && Read(inner_head,&inner_node,8))
                for(uint64_t i=0;inner_node && inner_node!=inner_head && i<12 && i<ids;++i){int32_t id{};if(!Read(inner_node+0x10,&id,4))break;
                    out+=(i?",":"")+std::to_string(id);if(!Read(inner_node,&inner_node,8))break;}
            out+="]}";if(!Read(node,&node,8))break;}}
    uint32_t d8[2]{},e0[2]{};const bool keys=Read(bank+0xD8,d8,8) && Read(bank+0xE0,e0,8);
    out+="],\"area_count\":"+(counted?std::to_string(count):std::string("null"));
    if(keys)out+=",\"key_d8\":["+std::to_string(d8[0])+","+std::to_string(static_cast<int32_t>(d8[1]))+"],\"key_e0\":["+std::to_string(e0[0])+","+
        std::to_string(static_cast<int32_t>(e0[1]))+"]}";
    else out+=",\"key_d8\":null,\"key_e0\":null}";
    return out;}
std::string Count(const std::atomic<uint64_t>& v,bool applies) {return applies?std::to_string(v.load(std::memory_order_relaxed)):"null";}
// A row: the bank GXRS+0x42A8, whether the calling table is that bank, and every site.
void Row(uintptr_t table,int row_number,const char* trigger) noexcept {
    try{const bool native=control->mode==bootstrap::Mode::NativeBootstrap;const uintptr_t bank=GxrsBank();
        std::string row="{\"event\":\"env_bank_sites\",\"mode\":\""+std::string(native?"native":"reference")+"\",\"row\":"+std::to_string(row_number)+
            ",\"trigger\":\""+trigger+"\",\"rows_exhausted\":"+(row_number>=kRows?"true":"false")+",\"tick\":"+std::to_string(GetTickCount64())+
            ",\"sites_full\":"+(full.load()?"true":"false")+",\"bank_address\":"+(bank?"\""+Hex(bank)+"\"":std::string("null"))+
            ",\"caller_table_is_bank\":"+(bank?(table==bank?"true":"false"):"null")+",\"bank\":"+(bank?BankState(bank):std::string("null"))+",\"sites\":[";
        bool first=true;
        for(const auto& s:sites){const uint64_t id=s.id.load(std::memory_order_acquire);if(!id)continue;
            const int reader=static_cast<int>((id-1)&1);const uint64_t code=(id-1)>>1;const uint64_t kind=code>>60,where=code&kWhere;
            std::string keys;
            for(const auto& k:s.keys){const uint64_t v=k.load(std::memory_order_relaxed);if(v==kNoKey)continue;
                keys+=(keys.empty()?"":",")+std::string("[")+std::to_string(static_cast<uint32_t>(v))+","+std::to_string(static_cast<int32_t>(v>>32))+"]";}
            std::string scale="null";
            if(reader==1 && s.scaled.load(std::memory_order_acquire)){float low{},high{};const uint32_t lb=s.low.load(),hb=s.high.load();
                std::memcpy(&low,&lb,4);std::memcpy(&high,&hb,4);
                if(std::isfinite(low) && std::isfinite(high)){char text[64]{};_snprintf_s(text,sizeof text,_TRUNCATE,"[%g,%g]",low,high);scale=text;}}
            row+=(first?"":",")+std::string("{\"site\":\"")+(kind==0?"":kind==1?"module+":"other+")+Hex(where)+"\",\"reader\":\""+(reader?"416A0":"41990")+
                "\",\"calls\":"+Count(s.calls,true)+",\"on_gxrs_bank\":"+Count(s.on_gxrs,true)+",\"yes\":"+Count(s.yes,reader==0)+
                ",\"given_yes\":"+Count(s.given_yes,reader==0 && native)+",\"exact\":"+Count(s.exact,reader==1)+",\"via_1_0\":"+
                Count(s.via_one_zero,reader==1)+",\"default\":"+Count(s.fallback,reader==1)+",\"minus_one\":"+Count(s.minus_one,reader==1)+
                ",\"cubes\":"+Count(s.cubes,reader==1)+",\"scale\":"+scale+",\"nonfinite_scales\":"+Count(s.nonfinite,reader==1)+
                ",\"native_banks\":"+Count(s.banks,native)+",\"keys\":["+keys+"]}";
            first=false;}
        row+="]}\n";Log(row.c_str());}
    catch(...){Log("{\"event\":\"env_bank_sites\",\"failed\":true}\n");}}
// One call: reader 0 is 41990 (answer: 1 when the engine's bank holds the key; given: the native's answer the game received, -1 in
// the Reference), reader 1 is 416A0 (answer: 0 exact, 1 {1, 0}'s, 2 the default); the entry's scale and cube for 416A0. Natively,
// whether the native banks hold the key.
void Note(uintptr_t bank,uintptr_t at,int reader,const uint32_t* key,int answer,int given,const float* scale,bool cube) noexcept {
    uint32_t words[2]{0xFFFFFFFFu,0xFFFFFFFFu};if(key)Read(reinterpret_cast<uintptr_t>(key),words,8);
    const bool minus_one=words[0]==0xFFFFFFFFu && words[1]==0xFFFFFFFFu;
    bool banks{};
    if(control->mode==bootstrap::Mode::NativeBootstrap){   // the banks as NativeEngineFogAnswer keeps them: gparam_lock only when a publication changed
        try{thread_local struct {uint64_t serial{~0ull};std::weak_ptr<const ds3light::Banks> banks;} kept;
            const uint64_t published=native_gparam_publishes.load(std::memory_order_acquire);std::shared_ptr<const ds3light::Banks> b;
            if(published==kept.serial)b=kept.banks.lock();
            else{{const std::lock_guard hold(gparam_lock);b=gparam_banks;}kept.banks=b;kept.serial=published;}
            const bool one_zero=reader==1 && minus_one;float s{};banks=b && b->EnvColorScale(one_zero?1u:words[0],one_zero?0u:words[1],s);}
        catch(...){banks=false;}}
    bool claimed{};
    if(Site* s=Find(SiteCode(at),reader,claimed)){
        s->calls.fetch_add(1,std::memory_order_relaxed);
        if(bank && bank==GxrsBank())s->on_gxrs.fetch_add(1,std::memory_order_relaxed);
        if(reader==0){if(answer==1)s->yes.fetch_add(1,std::memory_order_relaxed);if(given==1)s->given_yes.fetch_add(1,std::memory_order_relaxed);}
        else{(answer==0?s->exact:answer==1?s->via_one_zero:s->fallback).fetch_add(1,std::memory_order_relaxed);
            if(minus_one)s->minus_one.fetch_add(1,std::memory_order_relaxed);
            if(cube)s->cubes.fetch_add(1,std::memory_order_relaxed);
            if(scale){if(std::isfinite(*scale)){Bound(s->low,*scale,true);Bound(s->high,*scale,false);s->scaled.fetch_add(1,std::memory_order_release);}
                else s->nonfinite.fetch_add(1,std::memory_order_relaxed);}}
        if(banks)s->banks.fetch_add(1,std::memory_order_relaxed);
        AddKey(*s,static_cast<uint64_t>(words[1])<<32|words[0]);}
    const uint64_t now=GetTickCount64();uint64_t due=next_row_ms.load(std::memory_order_relaxed);
    if(rows.load(std::memory_order_relaxed)>=kRows)return;
    const bool interval=now>=due && next_row_ms.compare_exchange_strong(due,now+(rows.load(std::memory_order_relaxed)<30?20000:60000),std::memory_order_relaxed);
    if(claimed || interval){const int n=rows.fetch_add(1,std::memory_order_relaxed)+1;if(n<=kRows)Row(bank,n,claimed?"new_site":"interval");}}
}  // namespace env_bank_witness
// The Reference's 41990 (natively NativeEnvBankHasObserved notes its own calls).
bool __fastcall ObserveEnvBankHas(uintptr_t table,const uint32_t* key) noexcept {
    const bool answer=env_bank_has_original(table,key);
    env_bank_witness::Note(table,reinterpret_cast<uintptr_t>(_ReturnAddress()),0,key,answer?1:0,-1,nullptr,false);return answer;}
// 416A0 in both modes: the answer told from its result (the default +0x1A8; else 41990's genuine answer on the key it asked, {-1, -1}
// read {1, 0}: exact, else {1, 0}'s; a node another thread's 40DD0 adds between the two calls could make one call read « exact »).
uintptr_t __fastcall ObserveEnvBankGet(uintptr_t table,const uint32_t* key) noexcept {
    const auto entry=env_bank_get_original(table,key);
    uint32_t words[2]{0xFFFFFFFFu,0xFFFFFFFFu};if(key)Read(reinterpret_cast<uintptr_t>(key),words,8);
    if(words[0]==0xFFFFFFFFu && words[1]==0xFFFFFFFFu){words[0]=1;words[1]=0;}
    int answer=2;
    if(entry && entry!=table+0x1A8){const auto has=env_bank_has_original?env_bank_has_original:reinterpret_cast<EnvBankHas>(image+0x41990);answer=has(table,words)?0:1;}
    float scale{};uintptr_t cube{};const bool scaled=entry && Read(entry+0xC,&scale,4);if(entry)Read(entry+0x40,&cube,8);
    env_bank_witness::Note(table,reinterpret_cast<uintptr_t>(_ReturnAddress()),1,key,answer,-1,scaled?&scale:nullptr,cube!=0);return entry;}
// I-0140 tranche 28 (light-observe.flag, Reference): the region pass's selection 67070(manager, pass context, view): the view pair
// (pass +0x4B8/+0x4BC), the manager's listed regions and, of those whose masks are not the world's {0x11, 1}, the key, masks, half
// extents and centre (69940's handle resolution); a row per pair's first 3 calls and each change of its listing (40 rows at most per pair).
using RegionSelect=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t);RegionSelect region_select_original{};
void __fastcall ObserveRegionSelect(uintptr_t manager,uintptr_t pass,uintptr_t view) noexcept {
    region_select_original(manager,pass,view);
    uint32_t pair[2]{},count{};uintptr_t handles{},handles_end{},list{},list_end{},entries{},root{};uint8_t on{};
    if(!Read(pass+0x4B8,pair,8) || !Read(manager+0x10,&on,1) || !Read(manager+0x18,&handles,8) || !Read(manager+0x20,&handles_end,8) ||
       !Read(manager+0x38,&list,8) || !Read(manager+0x40,&list_end,8) || !Read(manager+0x58,&entries,8) || !Read(manager+0x60,&count,4) ||
       handles_end<handles || (handles_end-handles)/8>4096 || list_end<list || (list_end-list)/8>4096)return;
    Read(image+0x4961B50,&root,8);
    std::string listed;uint32_t regions{},world{};
    for(uintptr_t at=list;at<list_end;at+=8){uintptr_t record{};uint32_t key[2]{},masks[2]{};float box[0x50/4]{};
        if(!Read(at,&record,8) || !record || !Read(record,key,8) || !Read(record+0x10,box,0x50))continue;
        ++regions;bool handled{};
        for(uintptr_t h=handles_end;h>handles;h-=8){uintptr_t handle{},region{};int32_t id{};uint16_t generation{},current{};
            if(!Read(h-8,&handle,8) || !handle || !Read(handle+8,&id,4) || id==-1)continue;const uint16_t index=static_cast<uint16_t>(id);
            if(count<index || !Read(handle+0xA,&generation,2) || !Read(entries+0x10+0x18ull*index,&current,2) || generation!=current ||
               !Read(entries+0x18ull*index,&region,8) || region!=record)continue;
            handled=Read(entries+0x18ull*index+8,masks,8);break;}
        if(handled && masks[0]==0x11 && masks[1]==1){++world;continue;}
        if(listed.size()<1200){char item[300]{};_snprintf_s(item,sizeof item,_TRUNCATE,"%s{\"key\":[%u,%d],\"masks\":[%u,%u],\"handle\":%d,\"half\":[%g,%g,%g],\"centre\":[%g,%g,%g]}",
            listed.empty()?"":",",key[0],static_cast<int32_t>(key[1]),masks[0],masks[1],int(handled),box[0],box[1],box[2],box[16],box[17],box[18]);listed+=item;}}
    static std::mutex lock;static std::map<uint64_t,std::pair<uint32_t,std::string>> seen;bool log{};
    {const std::lock_guard hold(lock);auto& [rows,last]=seen[uint64_t(pair[0])<<32|pair[1]];
        const std::string state=std::to_string(regions)+"|"+listed;log=rows<3 || (state!=last && rows<40);last=state;rows+=log;}
    if(log){std::string row="{\"event\":\"reference_region_select\",\"pair\":["+std::to_string(pair[0])+","+std::to_string(pair[1])+"],\"manager_on\":"+std::to_string(on)+
        ",\"is_gxrs\":"+(root && manager==root+0x42B0?"1":"0")+",\"regions\":"+std::to_string(regions)+",\"world\":"+std::to_string(world)+",\"other\":["+listed+"],\"tick\":"+std::to_string(GetTickCount64())+"}\n";Log(row.c_str());}
}
}  // namespace
