#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

// I-0044: native producers of DS3's deferred-lighting constant blocks, fed by the engine state copied at the entry
// of each writer (adapter v19, transport 28; development snapshots, D-0119 limits). A producer recomputes the bytes
// a writer uploads from those inputs only; the oracle block is never read here.
namespace ds3light {
// One region of a writer entry snapshot: key = spec region index (| 0x8000 for a vector's pointer array,
// | element << 16 for its elements, element starting at 1).
struct Region { uint32_t key{}; uint64_t address{}; const uint8_t* data{}; uint32_t bytes{}; };
struct Child { uint32_t writer{}; std::vector<Region> regions; };   // transport 30: a helper called inside a persistent record's call
struct Snapshot {
    uint32_t id{}, writer{}, parent{}, gen{}; std::vector<uint8_t> raw; std::vector<Region> regions; std::vector<Child> children;   // parent: transport 29; gen: transport 31 record generation
    uint32_t seq{}; int64_t frame{};   // I-0051: capture sequence and camera frame of the call (snaps.jsonl "seq", "frame")
    const uint8_t* data{}; size_t size{}; std::shared_ptr<const void> keep;   // I-0051: the bytes regions point into: raw, or the capture pool itself (kept alive by keep) when loaded with its owner
    const Region* Find(uint32_t key) const { for(const auto& r:regions) if(r.key==key) return &r; return nullptr; }
    unsigned Elements(uint32_t region) const { unsigned n=0; for(const auto& r:regions) if((r.key&0xFFFFu)==region && (r.key>>16)) ++n; return n; }
};
// I-0051: the rows Snapshots::Load reads from resources.jsonl (2D textures), origins.jsonl and snaps.jsonl, in file order;
// the live host gives them from the transport's binary records, the text loader parses them from the files.
struct TextureRow { uint64_t object{}; uint32_t w{}, h{}; };
struct OriginRow { uint32_t ref{}, snap{}, caller{}, stack1{}, gen{}; uint64_t handle{}; };   // stack1: second return address (generic helper uploads); handle (v47): the GX buffer pooled
struct SnapRow { uint32_t id{}, writer{}, parent{}, gen{}, slot{}, seq{}; int64_t frame{}; uint64_t offset{}, bytes{}; bool has_slot{}; };
struct SnapshotRows { std::vector<TextureRow> textures; std::vector<OriginRow> origins; std::vector<SnapRow> snaps; };
class Snapshots {
public:
    // capture dir with snaps.jsonl, origins.jsonl, persist/; pool = its capture-pool.bin.
    void Load(const std::filesystem::path& capture,const std::vector<uint8_t>& pool);
    // The same from rows (persistent records still read from capture/persist/).
    // pool_owner: when given, snapshots point into `pool` (which must not move while they are used) instead of copying it.
    void Load(const std::filesystem::path& capture,const std::vector<uint8_t>& pool,const SnapshotRows& rows,std::shared_ptr<const void> pool_owner=nullptr);
    static SnapshotRows ReadRows(const std::filesystem::path& capture);
    const Snapshot* OfRef(uint32_t ref) const;   // snapshot of the writer call that uploaded the pooled block `ref`
    uint32_t SnapOf(uint32_t ref) const { const auto it=snap_of_.find(ref); return it==snap_of_.end()?0:it->second; }
    uint32_t CallerOf(uint32_t ref) const { const auto it=caller_of_.find(ref); return it==caller_of_.end()?0:it->second; }   // last-update return address (RVA)
    uint64_t HandleOf(uint32_t ref) const { const auto it=handle_of_.find(ref); return it==handle_of_.end()?0:it->second; }   // v47: the GX buffer the block was pooled from (0 before)
    // I-0064: the rank of the pooled block `ref` among the uploads of its writer call (uploads are pooled in call order, so in
    // increasing references), whether a draw binds them or not; -1 when the block has no call. same_caller: among the uploads with the
    // block's return address only (CADD0: one call uploads other blocks between its cbwvpmatrix ones).
    int UploadOrdinal(uint32_t ref,bool same_caller=false) const;
    unsigned UploadCount(uint32_t snap) const { const auto it=refs_of_.find(snap); return it==refs_of_.end()?0u:static_cast<unsigned>(it->second.size()); }
    // Size of a D3D11 texture the renderer created (capture resource table), by its address.
    bool TextureSize(uint64_t object,uint32_t& w,uint32_t& h) const { const auto it=texture_size_.find(object); if(it==texture_size_.end()) return false; w=it->second.first; h=it->second.second; return true; }
    const std::map<uint32_t,Snapshot>& All() const { return snaps_; }
    // I-0064 (review): indexes built by Load: the in-window calls made inside `id` (parent = id), and every call of a writer, in id order
    const std::vector<const Snapshot*>& ChildrenOf(uint32_t id) const { static const std::vector<const Snapshot*> none; const auto it=children_.find(id); return it==children_.end()?none:it->second; }
    const std::vector<const Snapshot*>& OfWriter(uint32_t writer) const { static const std::vector<const Snapshot*> none; const auto it=by_writer_.find(writer); return it==by_writer_.end()?none:it->second; }
    // Transport 31: blocks whose upload generation differs from the persistent record's (unattributed: SnapOf = 0), and
    // blocks checked (upload and record both carry a generation).
    unsigned GenMismatches() const { return gen_mismatch_; }
    unsigned GenChecked() const { return gen_checked_; }
private:
    std::unordered_map<uint32_t,uint32_t> snap_of_, caller_of_;   // I-0052: hashed (rebuilt every live frame)
    std::unordered_map<uint32_t,uint64_t> handle_of_;
    std::unordered_map<uint32_t,std::vector<uint32_t>> refs_of_;   // I-0064: each call's uploads, ascending
    std::unordered_map<uint64_t,std::pair<uint32_t,uint32_t>> texture_size_;   // D3D11 resource address -> (width, height), resources.jsonl
    std::map<uint32_t,Snapshot> snaps_;
    std::unordered_map<uint32_t,std::vector<const Snapshot*>> children_, by_writer_;
    unsigned gen_mismatch_{}, gen_checked_{};
};
uint32_t WriterRva(uint32_t writer);   // adapter v19 writer index -> RVA
// The block `ordinal` (0-based, upload order) of `family` uploaded by the call `snap`; `bytes` = block size.
// Returns false when no producer exists for that family and writer.
bool Produce(const std::string& family,const Snapshot& snap,unsigned ordinal,uint32_t caller,uint8_t* out,uint32_t bytes,const Snapshots* all=nullptr);
// Producer class of a family and writer: 'A' computed from engine objects or game data, 'B' copy of a CPU image of the
// block prepared upstream (its origin is not yet identified), 0 none.
char Kind(const std::string& family,uint32_t writer);
// Bytes of the executable image at `rva` (game/DarkSoulsIII.exe, or the DS3_EXE path), from its section table.
bool Rdata(uint32_t rva,void* out,size_t bytes);
// 1401130F0 (I-0066 1141, shared with the native SSS stage): the 17-tap separable kernel of a profile of strength s.
bool SssProfileKernel(const float strength[3],float kernel[17][4]);
// W10 YB-5 (YEBIS-WAVE1-READY 4.7 c): cbComputeCustomDof (E5110 at 1400E61BE, 0x2010 bytes: g_nTexInfo, then g_Coc[2048]) on
// named inputs, the core of the snapshot producer (I-0064, exact against the capture), so the library host can feed it from the
// live library and the native DoF globals: the LUT texture size and arg0+0xCC; the library's near and far (Y+0x564, +0x568),
// lens words (Y+0xC48, +0xC38, +0xC3C, +0xC40, +0xC44) and the exposure [[Y+0x110]+0x1B0] (0 unless Y+0xC34); the DoF globals
// 144962244..26C (E4890's); the executable constants [143D7EE10], [143D7ED04], [143D7ECE4] and lane 2 of [1439F2500].
struct CustomDofInput {
    uint32_t width{},height{},word_cc{};
    float near_plane{},far_plane{};std::array<float,5> lens{};float exposure{};
    std::array<uint32_t,11> dof{};
    float one{},step{},tiny{},fallback{};
};
void CustomDofTable(const CustomDofInput& in,uint8_t out[0x2010]);
bool CustomDofConstants(CustomDofInput& in);   // the four executable constants by Rdata
// I-0045: the sun (1496D0) and fog (14E900) parameter-bank records that 137910 hands to its builders, built from game
// data: the record of key {area, value ID} takes the LightSet / Fog values of that ID from GPARAM m<aa>_<bb>_<cccc>
// (area = 1aabbcccc; area 0 = default.gparam), the light directions from each Angle pair rotated by the yaw (Degree)
// of the map's MSB MapOffset event, the fog map position from its Position, and the engine's record defaults (.rdata).
// Only the record words the builders read are produced (sun 0x00..0x24F, fog 0x00..0x57); the others stay zero.
// The bank file name of an area: default (0, 1), sAA_BB_VVVV (a cutscene's, 2AABBVVVV), mAA_BB_CCCC (light_producers.cpp).
std::string AreaFileName(uint32_t area);
class Banks {
public:
    // dir: *.gparam.dcx (param_drawparam_<name>.gparam.dcx) and mapoffset.txt (one line per map's MapOffset; no tool writes it today).
    // I-0079: with `previous`, a file whose bytes are the same object the previous bank read (the shared reader's view) keeps
    // its parsed form instead of being parsed again (each admission or release of one GPARAM reparsed every file: 4-6 ms).
    bool Load(const std::filesystem::path& dir,const Banks* previous=nullptr);
    bool Sun(uint32_t area,uint32_t id,uint8_t* record,uint32_t bytes) const;   // bytes >= 0x260; with 0x2A0, the record's rotation (a quaternion) at +0x290
    // I-0063: the area's GPARAM file (m<aa>_<bb>_<cccc> of the same map: state variants and partial overlays) whose sun (or fog) record of `id` equals the engine's
    // copied record on every word the builders read; remembered for every bank lookup of that area. False when none does.
    bool SelectVariant(uint32_t area,uint32_t id,const uint8_t* copied,bool fog=false,uint32_t limit=0x1000) const;   // limit: only words below it are compared (a blended set's copy ends at 0x220)   // fog: the fog record (14E900), group 1
    bool Fog(uint32_t area,uint32_t id,uint8_t* record,uint32_t bytes) const;   // bytes >= 0x80
    // Sun source (I-0066 2026-10-13, NATIVE-LIGHTING-MAP "Sun source"): the area's own file the engine binds through
    // FUN_140D275F0 (kind 7, bank 5: the map's GPARAM or the state variant the area controller selected) and the 3x4 transform
    // that binding passes to FUN_140049D00 (rows {r0, tx}, {r1, ty}, {r2, tz}; FUN_140037340 takes the records' rotation from
    // it). While an area is bound, every lookup of that area reads the bound file and Sun rotates by the bound rows instead of
    // the file named after the area and the MapOffset yaw (the rows applied transposed: chosen by measurement, Native1543-1547
    // against the original at m37, m31, m40; not read in 37340 or 362B0). `file` is Load's key (lower case, no "drawparam_"
    // prefix, no extension); a file Load did not see leaves the area on its previous rule.
    void BindArea(uint32_t area,const std::string& file,const std::array<float,12>& transform) const;
    void ClearAreaBindings() const;
    // Every area's binding replaced at once (one lock): no lookup sees a partial set.
    void SetAreaBindings(const std::map<uint32_t,std::pair<std::string,std::array<float,12>>>& bindings) const;
    // Every live kind-7 binding of each area, latest first: a lookup reads the latest layer that holds its value ID (INFERRED rule:
    // m45 binds m45_00_0000 then the partial overlay m45_00_0200; Reference1751 against Native1752), the first is the bound file.
    void SetAreaLayers(const std::map<uint32_t,std::vector<std::pair<std::string,std::array<float,12>>>>& layers) const;
    // Counts every change of the bindings or layers (pre-merge review): a cache keyed by the bank must key by this too.
    uint64_t BindingGeneration() const;
    // I-0141 tranche 7 (STUDY-LAYER-FADE): the original fades an area's draw-parameter records over the transition time its area
    // controller (140CE6350) gives an unbinding and a binding (FUN_140092B50: a weight linear in the time, clamped to [0, 1]); the
    // values move from those of the layers before the change to those after. Here, per area, the layers before the change (in
    // SetAreaLayers' form, latest first), the start on FadeClock()'s seconds and the duration: while the weight w = (now - start) /
    // duration is below 1, Sun and Fog of that area are derived from values faded one by one (tranche 24), as the original fades
    // its values before its records are derived (READ in tranche 23, FadeValue's classes): each parameter of the current layers'
    // file between the earlier layers' value and the current one, then the record built from them (OwnSun, OwnFog). The record
    // blends (UseRecordBlends) serve only the act's overrides.
    // Its first end differs where a fade restarts during another: the original starts from the values blended then, this from the
    // earlier layers' values (a jump at the restart; tranche 7's step review, L7).
    // I-0141 tranche 23: Shadow, GroupWord and GroupFloat (so GlowAt), GroupEntries (so DominantAt, SnowAt, SnowModelAt and the post
    // settings) and EnvColorScale of a fading area answer each value between the earlier layers' and the current layers', by the
    // original's value classes (READ: floats and vectors lerped, integers lerped and truncated, colours by byte, angle pairs by
    // AngleLerp, bools and the values declared without the fade at once). BackgroundColor's BG Color is one of those (at once); Env
    // (the MSB boxes) is no layer's.
    struct AreaFade { std::vector<std::pair<std::string,std::array<float,12>>> from; double start{}; float duration{}; };
    // The weights move only in AdvanceFades (once a world collect, CollectNativeSelectedScene), which renews
    // RecordGeneration while a fade runs and once at its end: a cache keyed on it never serves a fade's answer past its step. SetAreaFades replaces them all, each fade at its weight on FadeClock() then; a fade already over is dropped.
    void SetAreaFades(const std::map<uint32_t,AreaFade>& fades) const;
    void AdvanceFades() const;
    // BindingGeneration plus the fades' own counter: what a cache of any fading reader's answers keys on (Sun, Fog, and since
    // tranche 23 the readers above; a fade's step leaves BindingGeneration: tranche 7's step review's M1).
    uint64_t RecordGeneration() const;
    bool Fading(uint32_t area) const;   // a fade of the area at the last AdvanceFades or SetAreaFades
    // I-0141 tranche 16 (I-0138 part 4b's MapOffset rule): a Sun or Fog lookup of an area other than default (after Resolve) with
    // no layer and no loaded bound file (an area bound to a file missing from the bank counts) answers the file File() finds for
    // it: the area's variant (one of its map's files, or a global state file, eclipse, mapoverride_000N, mapbase or default, that
    // SelectVariant set, a candidate it is still trying included; a Fog lookup without a fog variant takes the sun's), else the
    // file named after the area. Its directions are turned by the map's MapOffset yaw and its fog position is the MapOffset's,
    // from mapoffset.txt. The game's bank folder is memory-only and nothing publishes that file, so in play `offset` is false and
    // the answer is unturned, with fog position 0; the original's bank holds the bound files' records only (a key absent from it
    // reads as {1, 0}, 140032770). The first 32 distinct (area, id, group, fade_view) answered so, process-wide: the module writes
    // a row for each; the generation is their count. For an overridden key the keys noted are its sources' (A's, and B's when
    // blending), after Resolve, never the one asked for. A record is noted only once it is whole, but a Sun or Fog built from two
    // (an override's sources) may still be refused after the first is noted; a fade's earlier end is noted after the current
    // record is whole, even when its file lacks parameters (those keep the current value); SelectVariant's
    // trial lookups note like any other, so a key can be listed whose own lookups never answer so (the trial's variant restored,
    // or the key resolving to {1, 0}). `fade_view`: the lookup ran inside a fade's view of the earlier layers whose first file is
    // not in the bank. A key already listed, or the list full, costs no lock: entries are published before the count and never
    // rewritten; only a new key takes the lock. In the module SelectVariant never runs (its callers sit behind UseBanks, which
    // nothing calls), so the variant map stays empty there and the MapOffset branch is dead code.
    struct UnboundAnswer { uint32_t area{}, id{}, group{}; bool offset{}, fade_view{}; };
    static uint64_t UnboundAnswers(std::vector<UnboundAnswer>* out);
    // I-0141 tranche 3: the records a cutscene's IngameDrawParam act writes over a map's (FUN_140CC8180 through FUN_140031A20 for
    // the light sets and FUN_1400380A0 for the fog, each (bank, key, layer, A, B, t): the record at `key` becomes A's, or the
    // blend of A's and B's at t when t > 0; READ, GhidraMCP, 2026-11-15). In the engine the record node of `key` is overwritten
    // on the act's layer (kind 9, MapOverride_forRemo) at each update of the act and the map's comes back when that layer is
    // withdrawn; natively the act writes the same overrides (OBSERVED, i141-t2c) into engine banks the renderer does not read.
    // Here every lookup of an overridden key (Sun, Fog, and so every reader of the banks) answers the override, before the key's
    // own resolution: A's and B's records are this bank's own (their usual rule, without a second override), their blend the
    // engine's own function (UseRecordBlends: 140147ED0 for a light set, which also makes its record at t <= 0, and 14014E5D0 for a
    // fog record, at t > 0 only; without the blend it needs an overridden key has no record). One override a (group, key): group 0
    // the light sets, 1 the fog. Unlike the engine's node, which holds the blend made at the act's last write, the blend here is
    // made at each lookup from the table's A, B and t (the same values: the act writes them at each of its updates).
    struct RecordOverride { uint32_t a[2]{}, b[2]{}; float t{}; uint32_t layer{}; };
    using RecordOverrides=std::map<std::tuple<uint32_t,uint32_t,uint32_t>,RecordOverride>;   // (group, area, id)
    void SetRecordOverrides(const RecordOverrides& overrides) const;   // replaces them all; the generation moves when they change
    // The ids FUN_1400320B0 and FUN_1400387D0 remove for (from, to): to - from + 1 ids from `from` on (unsigned, wrapping), none
    // when to - from is negative as a signed word (their loop, READ, GhidraMCP 2026-11-15).
    static constexpr bool RemovedId(uint32_t id,uint32_t from,uint32_t to) noexcept { return static_cast<int32_t>(to-from)>=0 && id-from<=to-from; }
    bool Overridden(uint32_t group,uint32_t area,uint32_t id) const;
    // I-0141 tranche 5 (D-0282, Thomas 2026-11-15, provisional like D-0279 until the light sources are set by us, ROADMAP workstream
    // 12): a light key whose set is a cutscene's own: a cutscene file's key (CutsceneArea), or a map key the act overrides by a
    // cutscene file's set (A's area a cutscene's, and B's too when the override blends, t > 0).
    bool AuthoredLightKey(uint32_t area,uint32_t id) const;
    // IBL region record (675B0 argument 2 entries) of the MSB EnvironmentMapEffectBox Env_Box<id> of the map of `area`
    // (envregions.txt, Dump-EnvRegions.ps1); only the words 675B0 and the lighting programmes read (0x00..0x5C,
    // 0xC8..0xDC) are produced; bytes >= 0xE0.
    bool Env(uint32_t area,uint32_t id,uint8_t* record,uint32_t bytes) const;
    // 1400416A0 lookup result +0xC: GPARAM `Env Map` ColorScale of value ID `id` in the GPARAM of `area`.
    bool EnvColorScale(uint32_t area,uint32_t id,float& out) const;
    // I-0066 1173: `Setting Info` BG Color of exactly {area, id} (no fallback: 140087FB0 keeps the pipeline's set-up clear
    // unless the Setting Info bank contains the key, 14015EC10).
    bool BackgroundColor(uint32_t area,uint32_t id,float out[4]) const;
    // I-0045 cycle 7: ShadowParam payload R (0x94 bytes, 18AF40 layout: Angle +0x00, Near +0x34, Far Fade Start +0x38,
    // Far Fade Dist +0x3C, cascades +0x40..+0x48, FilterMode +0x50, Depth Offset +0x54, Volume Depth +0x64, +0x68,
    // Depth Smooth +0x6C, CullFlip/AlignTexel +0x70/+0x71, DepthRate..DepthOffsetScale +0x74..+0x80, Blur +0x84/+0x88,
    // Shadow Color +0x8C, Resolution +0x90) of value `id` for `area`: the map GPARAM's ShadowParam group, or
    // mapbase.gparam's when the map leaves the group empty (observed on m40, i45-cap-05); unregistered words keep the
    // 839F0 defaults.
    // I-0140 tranche 7: the key rule of the ShadowParam bank's lookup 14018C590 ({-1, -1} -> {1, 0}; a key the bank lacks ->
    // {1, 0}; a hit writes its area into the record's +0x20); false when {1, 0} misses too (the bank then returns its built-in
    // record, ShadowDefaults with +0x20 and +0x24 at -1: the caller takes that).
    bool Shadow(uint32_t area,uint32_t id,uint8_t* payload,uint32_t bytes) const;
    // I-0066 1186: one float parameter (short name) of a group (short name) for {area, id}: the area's GPARAM, or mapbase.gparam's
    // group when the area leaves it empty (as Shadow); a key the group lacks falls back to {1, 0} (default.gparam), as the
    // engine's bank lookups do. `source` (optional) names the file used.
    bool GroupFloat(uint32_t area,uint32_t id,const char* group,const char* param,float& out,std::string* source=nullptr) const;
    // W5d GL-2 (GLOW-WAVE1-READY 5.1): the same lookup returning the parameter's raw first word (an integer, a boolean or the
    // float's bits).
    bool GroupWord(uint32_t area,uint32_t id,const char* group,const char* param,uint32_t& out,std::string* source=nullptr) const;
    // W5d GL-2: the record of the `Glow Filter` group FUN_14017DBE0's bank returns for a key ({-1, -1} -> {1, 0}; the area file,
    // else mapbase; a key the group lacks -> {1, 0} of default), the built-in record (Enable 0, BlurSampleScale 20, Blur RgbScale 3,
    // Blur AlphaScale 1, New Glow Enable 0, NewGlow Final ColorScale 1) where that lookup fails too; then FUN_1402227C0's blend
    // between keys A and B at weight t: |t| <= eps -> A, |t - 1| <= eps -> B, else the floats (b - a) t + a and the two enables B.
    struct GlowRecord {uint32_t enable{};float blur_scale{20.0f},rgb{3.0f},alpha{1.0f};uint32_t new_glow{};float final_scale{1.0f};};
    GlowRecord GlowAt(uint32_t area,uint32_t id,std::string* source=nullptr) const;
    static GlowRecord BlendGlow(const GlowRecord& a,const GlowRecord& b,float weight,float epsilon);
    // W5f YB-1 (YEBIS-WAVE1-READY 4.1): every entry of a group at a key, in file order (duplicate names stay separate entries):
    // its short name, type, word count (1, or the components of a float2/3/4) and raw words; the key rule of GroupWord (the area
    // file's group, else mapbase's; an id the group lacks falls back to default {1, 0}).
    struct BankEntry {std::string name;uint8_t type{};uint8_t words{1};std::array<uint32_t,4> raw{};bool present{};};   // present: the row holds the id (raw else zero)
    bool GroupEntries(uint32_t area,uint32_t id,const char* group,std::vector<BankEntry>& out,std::string* source=nullptr) const;
    // Lightmap floors (WAVE1-LAST-EFFECTS-STATIC-PREP 1.4): the record +8 that 134A20 ([root+0x4680], the view context's Dominant
    // pair +0x4110/+0x4118) returns, the eleven floats of the `Dominant Light` group in file order (Dominant Sharpness Min Value,
    // Offset, Exponant, Color Scale, R, G, B Scale, SH Color Scale, SH R, G, B Scale; Reference1844 at m37: the file's row), with
    // GroupEntries' key rule; a parameter the group lacks keeps `out`'s row, one under an unknown name is taken at its place (I-0141).
    // False when no layer holds the group, none of its parameters is found, or a named one is not one float: the caller keeps the engine
    // bank's built-in record (0.5, 0, 1.25, then ones).
    bool DominantAt(uint32_t area,uint32_t id,std::array<float,11>& out,std::string* source=nullptr) const;
    // I-0105 (the surface snow): the record 12F400 ([root+0x45F8], the view context's pair +0x3FC0/+0x3FC8) returns, from the
    // `Snow` group with GroupEntries' key rule, in the order ds3prod::ProduceSnow reads: Angle (2: elevation, azimuth), Snow Tiling
    // Scale (2), Scene Snow Rate, Sparkle Tiling Scale (2), Ground Plane Height, Snow Plane Height, Normal Scale, Albedo Scale (4),
    // Reflectance, Normal Blur Radius. False when no layer holds the group or a parameter is missing or of another size: the
    // caller keeps the engine bank's built-in record.
    // I-0105 tranche 10: `texture_id` the group's `Texture:ID` (131EA0 loads the record's textures from it), when given.
    bool SnowAt(uint32_t area,uint32_t id,std::array<float,16>& out,std::string* source=nullptr,int32_t* texture_id=nullptr) const;
    // The `Snow Model` group's Snow Rate at a key (46300's record +8, [root+0x4600], a draw state's pair +0x2B0/+0x2B8).
    bool SnowModelAt(uint32_t area,uint32_t id,float& rate,std::string* source=nullptr) const;
    GlowRecord Glow(uint32_t a_area,uint32_t a_id,uint32_t b_area,uint32_t b_id,float weight,float epsilon,std::string* sources=nullptr) const;
    // Env_Box numbers of the map of `area`, ascending.
    std::vector<uint32_t> EnvIds(uint32_t area) const;
    // SSS profile objects (vector [0x1449622F8], read by 1141B0), from sssprofiles.txt (Dump-SssProfiles.ps1):
    // {strength xyz, width} (float bits) of the unlinked profile at vector `position`: 0 = the 113D80 constructor
    // defaults (code immediates), k = the k-th MTD of the material bank with g_SSSWidth (g_SSSStrength, g_SSSWidth).
    bool SssBase(unsigned position,uint32_t out[4]) const;
    // Strength of a profile linked (22BF70, FLVER material item GX15) to the base at `base_position`: the GX15 data of
    // the FLVER materials using that base's MTD; false unless every such material carries the same values.
    bool SssInstance(unsigned base_position,uint32_t strength[3]) const;
    size_t Files() const;
    struct Impl; ~Banks(); Banks(); Banks(const Banks&)=delete; Banks& operator=(const Banks&)=delete;
private:
    // I-0141 tranche 3: the key's record without an override; tranche 24: with `fade`, its values faded one by one from the earlier
    // layers' (FadeValue) before the record is derived.
    struct LayerFade;
    bool OwnSun(uint32_t area,uint32_t id,uint8_t* record,uint32_t bytes,const LayerFade* fade=nullptr) const;
    bool OwnFog(uint32_t area,uint32_t id,uint8_t* record,uint32_t bytes,const LayerFade* fade=nullptr) const;
    bool OwnEntries(uint32_t area,uint32_t id,const char* group,std::vector<BankEntry>& out,std::string* source) const;   // I-0141 tranche 23: GroupEntries without a fade
    Impl* impl_;
};
// I-0141 tranche 3: the engine's record blends Banks applies to an override with t > 0. Light: `out` and `a`, `b` 0x2A0 bytes in
// Banks::Sun's layout (the lookup record's +0x40), 16-byte aligned; the module's form makes `out` as 31A20 does (140148F50 on its
// four lights, then 140147ED0). Fog: 0x60 bytes in Banks::Fog's layout (the record's +0x10), 16-byte aligned (14014E5D0).
using RecordBlend=void(*)(uint8_t* out,const uint8_t* a,const uint8_t* b,float t);
void UseRecordBlends(RecordBlend light,RecordBlend fog);
// I-0141 tranche 7: the seconds a layer fade is timed in (Banks::AreaFade's start): the steady clock's (QueryPerformanceCounter on Windows) by default; a test sets
// its own (null restores the default).
double FadeClock();
void UseFadeClock(double(*clock)());
// When set, cbLight / cbFog are produced from the bank records built for each 137910 entry key instead of the records
// copied at builder entry (the entry keys and the view stay engine inputs). Mismatches against the copied records are
// counted per read word (diagnostic).
void UseBanks(const Banks* banks);
// I-0045: the CED670 camera output of the frame (24 floats). When set, cbSceneParam (13AA60, 764D0)
// is produced from it and the cascaded-shadow state object instead of the copied image (class A).
void UseCamera(const float* camera);
// I-0045: shadow-map size (graphics configuration: 4096 for resolution 3, x cfg+0x2C / cfg+0x30, clamped to cfg+0x34..+0x38)
// and random-texture size of the backend; with UseBanks, the cbSceneParam shadow fields come from the ShadowParam record.
void UseShadowSetup(uint32_t map_size,uint32_t random_texture);
struct BankStats { unsigned records{}, words{}, mismatched_words{}; std::string first; };
const BankStats& BankCheck();
// I-0064: cbLight (fog = false, 0x120 bytes) or cbFog (fog = true, 0x60 bytes) of a forward material draw from the light or fog
// keys {area, value ID} of its draw state (a, b and the factor t), the view as the frame's sun and fog builders received it.
// `frame`: the draw's camera frame (the record's), whose sun and fog calls are preferred.
bool ProduceKeyed(bool fog,const uint32_t a[2],const uint32_t b[2],float t,const Snapshots* all,uint8_t* out,uint32_t bytes,int64_t frame,uint32_t before_seq=0);
// I-0064: post-chain blocks that depend only on a texture size (a frame input, D-0151: CBDlaaParam the draw's source texture,
// cbFilterParam its render target) and executable constants: CBDlaaParam (1B9790's 40 offsets: {-1.5, 1.5, -7.5, -5.5, -3.5, 3.5, 5.5, 7.5, -1, 1} / size)
// and cbFilterParam (7F950: {W k, H k, 0, 0, 1 / W, 1 / H, 0, 0}, k = [0x143D7ED38]). False for another family.
bool ProduceSized(const std::string& family,uint32_t width,uint32_t height,uint8_t* out,uint32_t bytes);
// Engine data written to a structured buffer (EvBufferData) by the call `snap`, `ordinal` = its write order in that call.
bool ProduceBuffer(const Snapshot& snap,unsigned ordinal,uint8_t* out,uint32_t bytes);
// I-0048: route the file reads of Snapshots::Load, Banks::Load and Rdata through `reader` (the live host's in-memory inputs); null = disk.
using Reader=std::vector<uint8_t>(*)(const std::filesystem::path&);
void UseReader(Reader reader);
// Native startup and the live host may enumerate the same virtual memory
// directory as their byte reader. Null preserves standalone disk loading.
using ListingReader=const std::vector<std::filesystem::path>&(*)(const std::filesystem::path&);
void UseListingReader(ListingReader reader);
using RdataReader=bool(*)(uint32_t,void*,size_t);
void UseRdataReader(RdataReader reader);
// I-0141 tranche 23: the value fade of one parameter (group and short name, the file's type, n words, the earlier end a, the
// current v, the weight w), for glow_bank_test.
void FadeValueForTest(const char* group,const char* param,uint8_t type,unsigned n,const uint32_t* a,const uint32_t* v,float w,uint32_t* out);
// I-0052: the same files viewed without a copy (persistent writer records, read every live frame).
using SharedReader=std::shared_ptr<const std::vector<uint8_t>>(*)(const std::filesystem::path&);
void UseSharedReader(SharedReader reader);
// W10 LS-2b: 14004B070 (the projection) on a camera record laid out as E69A0's; false on the orthographic flag or a short record.
bool CameraProjection4B070(const uint8_t* camera,size_t bytes,float proj[16]);
// I-0140 tranche 7 (the traced sun's shadow source, F90F0's chain; R's readings re-read by the E pilot, GhidraMCP 2026-11-14):
// ShadowDefaults: 1400839F0's ShadowParam record (0x94 bytes; +0x20 and +0x24 at -1, +0x8C 0xFF000000).
void ShadowDefaults(uint8_t* record);
// 1401DAF40(out, a, b, t): the view context's record +0x90 from the records of its keys A and B at weight t (88A10). |t| <= eps:
// a whole (1400841B0), |t - 1| <= eps: b whole, else the fields' lerps and copies of b as the listing writes them (+0x08, +0x0C,
// +0x4C, +0x58..+0x60 and +0x68 keep `out`'s); +0x20/+0x24 A's area and Shadow Light Id, +0x28/+0x2C B's (A's or B's twice when one record
// is taken whole), +0x30 t; +0x8C always keeps `out`'s (the record's Shadow Color is never taken); +0x10 the Angle's direction.
// `out` is the persistent record of the view (ShadowDefaults at first).
void BlendShadowRecord(uint8_t* out,const uint8_t* a,const uint8_t* b,float t,float eps);
// 1400F90F0's sun direction from that record: A's area or id -1 -> the Angle's direction (+0x10); B's -1 -> light 0 of the light set
// A (140032770 lookup, its record +0x40 = Banks::Sun +0x00); both keys -> light 0's Angle (+0x70 plus +0xA0, Banks::Sun +0x30 and
// +0x60, which Banks::Sun leaves 0: the node's +0xA0 writer unread) of A and B interpolated by 1400F8FD0 (theta on [-pi/2, pi/2], phi on [-pi, pi]) at the record's +0x30, turned by the slerp
// (1400FEA40, XMQuaternionSlerpV) of their rotations (+0x2D0, Banks::Sun +0x290).
// I-0141 (ROADMAP workstream 7 step 7; the coordinator's recommendation of 2026-11-14 while Thomas was away, his choice B prolonged,
// kept by his choice of the same evening, D-0279): a cutscene whose ShadowParam record names no light set (Shadow Light Id -1: 32000010's cuts, 39000020's)
// has, in the original, its shadow sun along the record's Angle, a horizontal direction; the original still lights each draw by its
// own light set, where the native's one traced sun then leaves the scene in shadow (Siegward dark, I-0140 `i140-e17`). Such a view's
// traced sun takes light 0 of the light set at the view's own ShadowParam key: in both cutscenes the key {2AABBVVVV, the cut's id}
// is the light set the cut's characters carry (OBSERVED in `i140-e16` and `i140-e17`: `native_model_light_key` beside
// `native_shadow_state`'s keys). CutsceneShadowLightSet: the light set that stands for a record's missing one; false unless the
// record's id is -1 and the view's key names a cutscene file's area (2AABBVVVV, GetGparamArea's range) and an id. `out` may be
// `record_keys`. It stands only where the bank holds it and its light 0 is on (SunLightOn; the step review's M1): 39000020's
// sets have their light 0 off (a zero strength at the default direction), and the record's Angle stays there, as the original's.
// A cutscene file's area (sAA_BB_VVVV answers 2AABBVVVV, GetGparamArea_140D27790's inverse).
inline constexpr bool CutsceneArea(uint32_t area) noexcept { return area>=200000000u && area<300000000u; }
// I-0141 (variant B, Thomas's provisional choice of 2026-11-14, D-0279: until the light sources are set by us): a light set of a cutscene's own file: both keys of a draw's light key block (a blend
// with a map's set keeps D-0262's removal: the map's fills would come back beside the bounce; the step review's L2). Its lights 1
// to 3 are kept on a traced frame, unshadowed. OBSERVED in 39000020: the characters' sets have their light 0 off and light them by
// their lights 1 to 3 (the original's cbLight blocks, `i140-e16-c39000020-orig`): the cut's authored lights. In 32000010 the
// sets' light 0 is on and their lights 1 to 3 are weaker (fills beside it, which then add to the traced bounce): INFERRED for
// the other cutscenes that the family mixes both.
inline constexpr bool CutsceneLightSet(uint32_t area_a,uint32_t area_b) noexcept { return CutsceneArea(area_a) && CutsceneArea(area_b); }
// Whether light 0 of a light set's record (Banks::Sun's, its colour +0x10 and its strength +0x1C) lights anything.
inline bool SunLightOn(const uint8_t* sun_record) noexcept {
    float c[4]; std::memcpy(c,sun_record+0x10,16); return c[3]>0.f && (c[0]>0.f || c[1]>0.f || c[2]>0.f);
}
inline bool CutsceneShadowLightSet(const uint32_t record_keys[2],const uint32_t view_keys[2],uint32_t out[2]) noexcept {
    if(record_keys[1]!=0xFFFFFFFFu || view_keys[1]==0xFFFFFFFFu || !CutsceneArea(view_keys[0])) return false;
    const uint32_t area=view_keys[0],id=view_keys[1]; out[0]=area; out[1]=id; return true;
}
// `view_keys`: the view's ShadowParam keys A and B (the context's +0x3FA8 and +0x3FB0; zeros when not read). CutLightSet, CutBlend: the
// light set or the blend taken through CutsceneShadowLightSet; a cut's set the bank lacks, or whose light 0 is off, leaves the
// record as it is (its Angle). A record's key is matched to either view key (BlendShadowRecord's whole-record branches write one
// record's keys into both: the step review's L1).
enum class ShadowSunSource : uint8_t { Missing, Angle, LightSet, Blend, CutLightSet, CutBlend };
ShadowSunSource ShadowSunDirection(const Banks& banks,const uint8_t* record,const uint32_t view_keys[4],float out[4]);
}
