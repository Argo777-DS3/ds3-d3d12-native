#pragma once
// I-0117 (R2 of RT-PT-PLAN, tranche T0; R2-PLAN §3): the ray-tracing scene's bottom levels. One BLAS a model storage, one
// triangle geometry a mesh slice (a ray query's GeometryIndex() is the slice's index), built from the storage's own buffer on an
// ingress list after that buffer's upload, compacted once its build completed, released with the storage. The rays' scene, always on since I-0132 R11e;
// the passes that trace it (RtSunShadow below and the others) draw in production (I-0120 F8: « nothing here draws » was stale).
#include "backend.h"
#include "content.h"
#include "raytracing_base.h"
#include "scene_view.h"
#include "rt_probes.h"
#include "rt_denoise.h"   // I-0132 R11: kRtDenoiseInputs (EncodeNrdSignal's targets)
#include <array>
#include <cstddef>
#include <atomic>
#include <functional>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <span>
#include <unordered_map>
#include <vector>

namespace ds3native {

// D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT.
constexpr uint64_t RtAlign(uint64_t bytes) noexcept {return (bytes+255)&~uint64_t(255);}
// A slice the BLAS takes: a triangle list (at least one triangle, a whole number of them), vertices of float3 at byte 0 with a
// 4-byte multiple stride, the vertex start 4-byte and the index start index-width aligned (DXR's address rules).
bool RtSliceEligible(const MeshSlice& slice) noexcept;
// Its geometry against the buffer's GPU address (opaque: an alpha-tested slice's instance is FORCE_NON_OPAQUE, T3).
D3D12_RAYTRACING_GEOMETRY_DESC RtSliceGeometry(D3D12_GPU_VIRTUAL_ADDRESS base,const MeshSlice& slice) noexcept;

// A model's BLAS through its life: building (its list recorded), built (its list completed: the compacted size readable),
// compacting (the copy recorded), ready (the compacted structure), failed (none: the model stays out of the rays).
// The BLAS alive now (T0 final review M1): counted when built, their placed bytes moved by the compaction, released when the
// model's BLAS goes.
struct RtLiveBlas {std::atomic<uint64_t> models{},placed_bytes{};};
// A structure's placed size: the pooled buffer heaps place at 64 KB (DeviceQueues::CreateStreamed).
constexpr uint64_t RtPlaced(uint64_t bytes) noexcept {return (bytes+65535)&~uint64_t(65535);}
struct RtModelBlas {
    enum class State : int {building,built,compacting,ready,failed};
    std::atomic<State> state{State::building};
    uint32_t postbuild_slot{UINT32_MAX};                 // its compacted size's slot (RtBlasRegistry::kPostbuildSlots)
    uint32_t geometries{},skipped{};uint64_t triangles{},built_bytes{},compacted_bytes{};
    // The structure the rays use, read by any thread (a frame's TLAS keeps the one it read until that frame completed): the built
    // one, swapped for the compacted one by the frame owner (T0 final review L4).
    Microsoft::WRL::ComPtr<ID3D12Resource> Structure() const {const std::lock_guard hold(structure_lock_);return structure_;}
    void SetStructure(Microsoft::WRL::ComPtr<ID3D12Resource> structure,uint64_t placed);
    RtModelBlas()=default;RtModelBlas(const RtModelBlas&)=delete;RtModelBlas& operator=(const RtModelBlas&)=delete;
    ~RtModelBlas();
private:
    friend class RtBlasRegistry;
    mutable std::mutex structure_lock_;Microsoft::WRL::ComPtr<ID3D12Resource> structure_;
    std::shared_ptr<RtLiveBlas> live_;uint64_t placed_{};   // counted in live_ once built
};

// What the registry has done (the native_rt_blas row). Since the start: models to pending; alive now: live_models, live_placed_bytes.
struct RtBlasCounters {
    // skipped: the ineligible slices (none in the game: ValidContent refuses such a model before its BLAS); compacted_bytes counts
    // the structures' 256-byte sizes, live_placed_bytes the 64 KB placed ranges they take; slots_free: the postbuild slots left.
    uint64_t models{},geometries{},skipped{},triangles{},built_bytes{},compacted_bytes{},ready{},failed{},pending{},slots_free{};
    uint64_t live_models{},live_placed_bytes{};
};

// The BLAS of every admitted model storage. Build: any thread (the storage queue); Advance: the frame owner, once a frame.
class RtBlasRegistry {
public:
    static constexpr uint32_t kPostbuildSlots=16384;   // models compacting at once
    // The model's BLAS recorded on its own ingress list (submitted ahead of the next frame list, after the buffer's upload, which
    // was committed earlier on the same queue). `buffer` must be in a state that includes NON_PIXEL_SHADER_RESOURCE. Null when no
    // slice is eligible; throws on a device failure.
    std::shared_ptr<RtModelBlas> Build(DeviceQueues& gpu,ID3D12Resource* buffer,std::span<const MeshSlice> meshes);
    // The compaction of at most `budget` BLAS whose build completed (one copy list for them; T0 final review M2: a zone's load
    // spread over frames), the large structures kept until that list completed; the compacted ones marked ready once their copy
    // completed. Returns how many advanced.
    static constexpr size_t kCompactionsPerAdvance=32;
    size_t Advance(DeviceQueues& gpu,size_t budget=kCompactionsPerAdvance);
    RtBlasCounters Counters() const;
    void Close() noexcept;   // drops the pending list and the postbuild buffers (after the queue's drain)
private:
    struct Postbuild;
    mutable std::mutex lock_;
    // Building or compacting, with its postbuild slot, given back when the model is reaped or was destroyed before that.
    // I-0120 F1: `built`, set when the build's list completed (or was dropped uncommitted): a destroyed model's slot is freed only then.
    struct Pending {std::weak_ptr<RtModelBlas> blas;uint32_t slot;std::shared_ptr<const std::atomic<bool>> built;};
    std::vector<Pending> pending_;
    std::shared_ptr<Postbuild> postbuild_;
    RtBlasCounters counters_{};
    std::shared_ptr<RtLiveBlas> live_=std::make_shared<RtLiveBlas>();
};

// I-0119 (R3a, R3-PLAN §3): one slice's material as its G-buffer programme writes it (rt_material.hlsli), with T3's alpha test
// (I-0117, R2-PLAN §3) as DS3's alpha-tested programmes make it. Its triangles: the indices (start in bytes, width) and the
// positions (float3 at byte 0 of slot 0) in the model storage's buffer; its UV0 element (first byte, stride, RtUv format); the
// NORMAL and TANGENT elements (R8G8B8A8_UINT, (v - 127) / 127, as the vertex programmes decode them; a stride 0: none). Its textures
// by role (the G-buffer programme's g_DiffuseTexture, g_SpecularTexture, g_BumpmapTexture, g_EmissiveTexture: the raster's CPU-only
// SRVs, kept by `textures`), each with its FC_TextureTilingScale row; FC_DifMapMulCol in `albedo` with the gloss scale
// (g_SpecularPower / 256)² in its w; the emissive colour. The alpha test (`alpha`): the diffuse texture's alpha when
// `alpha_texture`, times the vertex colour COLOR1.w where the DptA vertex programme reads it (T3 step review M1), against `ref`
// (FC_AlphaRef.x); none tested otherwise. `family`: the programme family's index (RtMaterialFamilies) for the control view's rows.
enum : uint32_t {RtUvSint16=0,RtUvFloat2=1,RtUvHalf2=2,RtUvFloat2Scaled=3,RtUvHalf2Scaled=4};   // 3, 4: x 1/2048 (a map blend's, R3c)
enum : uint32_t {RtAlphaColourNone=0,RtAlphaColourUnorm8=1,RtAlphaColourFloat4=2,RtAlphaColourBgra8=3};   // 3: B8G8R8A8_UNORM (its rgb read, R3c's face)
enum : uint32_t {RtTextureDiffuse=0,RtTextureSpecular=1,RtTextureBump=2,RtTextureEmissive=3,RtTextureRoles=4};
struct RtMaterialSlice {
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE,RtTextureRoles> views{};std::array<std::shared_ptr<const void>,RtTextureRoles> textures;
    std::array<std::array<float,4>,RtTextureRoles> tiling{{{1,1,0,0},{1,1,0,0},{1,1,0,0},{1,1,0,0}}};
    uint32_t index_offset{},index_width{2},position_offset{},position_stride{},uv_offset{},uv_stride{},uv_format{};
    uint32_t normal_offset{},normal_stride{},tangent_offset{},tangent_stride{};
    std::array<float,4> albedo{1,1,1,0},emissive{0,0,0,0};
    bool material{};   // its programme set read (the records of the others: a triangle and the alpha test alone)
    bool alpha{},alpha_texture{};float ref{};
    uint32_t colour_offset{},colour_stride{},colour_format{RtAlphaColourNone};
    uint32_t keep{0xFF};   // T5: the ray bits (RtMask*) the slice answers (RtSliceKeep): a candidate of a ray with none is passed over
    uint32_t family{};
    bool unscrolled{};   // R3c: UV0 without the instance's VC_TexScroll (a map blend's vertex programmes read none)
    bool normal_half{},tangent_half{};   // R3c: NORMAL, TANGENT in R16G16B16A16_FLOAT (read as is), else R8G8B8A8_UINT
    // I-0119 R3c: a map blend's layers 1 to 3 (MultiBlend, FourBlend: GBUF-FAMILIES §16), each a record after the set's slices
    // (rt_material.hlsli): its diffuse, specular and bump textures in the reference roles with their rows; the first also the mask
    // (the emissive role's place), the vertex weights (its UV element: TEXCOORD1 or TEXCOORD2, RtWeight* format), the vertex colour
    // COLOR0 (its colour element) and the blend's parameters P0, P1, P2 and the layers' count in its albedo. Empty: one layer.
    // R3c (`mul`): a two-layer piece (ColDifSpcBumpMul and MulMask, GBUF-FAMILIES §3 and §4): `layers` holds its second layer, its
    // g_DiffuseTexture2, g_SpecularTexture2, g_BumpmapTexture2 with their rows, its UV (TEXCOORD0.zw, scrolled by VC_TexScroll_1), its
    // vertex frame's TANGENT1, the weight's COLOR1 (.w); with `mul_mask` its g_BlendMaskTexture in the emissive role, the mask's UV
    // element (TEXCOORD1.xy, or .zw in the lightmapped bodies; UV16, scrolled by VC_TexScroll_3) in its normal fields, and
    // FC_BlendMaskParam's .x and .z in its albedo.
    std::vector<RtMaterialSlice> layers;
    bool mul{},mul_mask{};
    // R3c: the skin's albedo curve (GXFlver_SkinIbl, SkinIbl_Bloody, SkinSSSIbl_Bloody: GBUF-FAMILIES §17), the diffuse texel t made
    // saturate(sqrt(t) + FC_SkinMatchColor)² before FC_DifMapMulCol; the colour the instance's (RtInstance::skin). `hair`: the hair's
    // (GXFlver_HairIbl and its variants), saturate(sqrt(t) × FC_SkinMatchColor)².
    bool skin{},hair{};
    // R3c (`face`): the FaceGen face (GXFlver_FgFaceIbl, FgFaceIbl_Bloody, FgFaceSSSIbl_Bloody: GBUF-FAMILIES §17), a `mul` piece whose
    // second diffuse is the composed face image, lerped by COLOR0.w (the second layer's colour element) and the result times COLOR0.rgb;
    // `skin` curves layer 0 before the lerp, `face_both` the face image too (the _Bloody bodies).
    bool face{},face_both{};
    // I-0117 T6: its G-buffer vertex programme moves it by the wind (it reads VC_ForceParam): TEXCOORD1's format (RtWind*) and byte
    // in the vertex (slot 0); not part of its record (the wind moves the positions, RtSkinner).
    uint32_t wind{},wind_offset{};
    // I-0125 R6a (R6-PLAN §1): the ambient its G-buffer programme writes into GB4 (RtAmbient*): the lightmapped bodies' (t26 and t27,
    // the instance's lightmaps, sampled at `lightmap_offset`, `lightmap_stride` in `lightmap_format`: the lightmap UV element's two
    // values / 2048) or the others' (the instance's SH rows); a lightmapped slice whose element is not read has a stride 0 (its hits'
    // ambient counted missing).
    uint32_t ambient{},lightmap_offset{},lightmap_stride{},lightmap_format{};   // the final review's H1: its UV's format (RtUv*)
    // I-0125 R6d (R6-PLAN §7): an unlit forward slice (drawn forward only, by one of the sky's three unlit forward programmes) and its
    // row's blend (RtForwardBlend's two factors). The reflection rays that meet no surface compose these slices. `forward_kind` its
    // programme (RtForward*): GXFlver_Col_Fwd's (the dome: its diffuse in the diffuse role with its row 0, COLOR1 in the colour element,
    // apart from the alpha test's, T3's DptA element: the fixes' step review L-b; FC_DynamicDiffuseColor in `albedo`), the scrolling
    // zenith's MultiBlend2_et1_nolight_MuitiBlendMask_Fwd or the cloud sheets' Fujir_Nolight_OpacityBlend_Fwd (their parts in its
    // fields and its layer record: 12_scene_collect.inc).
    bool forward{};uint32_t forward_blend{},forward_colour_offset{},forward_colour_stride{},forward_colour_format{},forward_kind{};
};
enum : uint32_t {RtForwardCol=0,RtForwardBlendMask=1,RtForwardOpacity=2};   // 2: the cloud sheets' Fujir_Nolight_OpacityBlend_Fwd
// I-0125 R6d: a forward slice's blend as its row's (NativeOrdinaryForwardBlend; colour ADD only): the source factor in bits 0-3, the
// destination's in 4-7 (RtFactor*); opaque (ONE, ZERO) for a row drawn unblended. The record's word adds the kind (RtForward*) in bits 8-11.
enum : uint32_t {RtFactorZero=0,RtFactorOne=1,RtFactorSrcAlpha=2,RtFactorInvSrcAlpha=3};
constexpr uint32_t RtForwardBlend(uint32_t source,uint32_t destination) noexcept {return source|destination<<4;}
enum : uint32_t {RtAmbientNone=0,RtAmbientDol=1,RtAmbientSh=2};
enum : uint32_t {RtWeightUnorm8=1,RtWeightFloat4=2,RtWeightHalf4=3,RtWeightUnorm16=4};
// An instance's slices (by GeometryIndex) and the buffer their triangles and vertices are read from (the model storage's).
// `non_opaque`: a slice alpha-tested, or slices answering different rays (T5's mixed sets): its instances FORCE_NON_OPAQUE.
// `skin`: a slice with the skin's or the hair's curve (its instances read their FC_SkinMatchColor). `wind` (T6): a slice moved by
// the wind (its instances skinned while their wind blows).
// I-0125 R6a: `dol`, `sh`: a slice of each ambient (its instances read their rows and, for `dol`, their lightmaps).
// I-0125 R6d: `forward`, a forward slice (its instances read their forward block: RtForwardRow).
struct RtMaterialSet {Microsoft::WRL::ComPtr<ID3D12Resource> buffer;std::vector<RtMaterialSlice> slices;bool non_opaque{},skin{},wind{};
    bool dol{},sh{},forward{};};
// Whether two sets give the same records (the same buffer, textures and views, every field): a set made again equal to the one it
// replaces keeps the former, so its records stay in the table (a texture still at its stand-in remakes its set every 30 walks).
bool RtSameMaterials(const RtMaterialSet& a,const RtMaterialSet& b) noexcept;
// One slice's record for the shader (rt_material.hlsli's RtMaterialRecord): 224 bytes, in the material table (RtMaterialTable).
// I-0125 R6a: its ambient (RtAmbient*) and its lightmap UV's first byte, stride and format (RtUv*). R6d: a forward slice's blend
// (RtRecordForward: RtForwardBlend's word).
enum : uint32_t {RtRecordAlpha=1,RtRecordAlphaTexture=2,RtRecordNormal=4,RtRecordTangent=8,RtRecordMaterial=16,RtRecordLayers=32,RtRecordUnscrolled=64,RtRecordNormalHalf=128,RtRecordTangentHalf=256,
    RtRecordMul=512,RtRecordMulMask=1024,RtRecordSkin=2048,RtRecordHair=4096,RtRecordFace=8192,RtRecordFaceBoth=16384,RtRecordForward=32768};
struct RtMaterialRecord {
    uint32_t buffer,index_offset,index_width,keep,uv_offset,uv_stride,uv_format,flags;
    uint32_t position_offset,position_stride,normal_offset,normal_stride,tangent_offset,tangent_stride,family;float ref;
    uint32_t colour_offset,colour_stride,colour_format,extension;   // R3c: the first layer record's index in the table (RtRecordLayers: a blend's; RtRecordMul: a Mul piece's second layer)
    std::array<uint32_t,RtTextureRoles> textures;
    std::array<std::array<float,4>,RtTextureRoles> tiling;
    std::array<float,4> albedo,emissive;
    uint32_t ambient,lightmap_offset,lightmap_stride,lightmap_format;
    uint32_t forward,forward_colour_offset,forward_colour_stride,forward_colour_format;};
static_assert(sizeof(RtMaterialRecord)==224);
// One instance's row for the shader (rt_material.hlsli's RtInstanceRow), by its index in the top level: its scroll row
// (cbTexScroll VC_TexScroll_0), a skinned instance's positions (their raw view's slot, ~0 for a placed instance) and its slices'
// first vertices there (the byte where they start in the frame's rows buffer), its first record and its records' count (0: none);
// R3c: the scroll rows VC_TexScroll_1 and _3 (a Mul piece's second layer and mask), the skin's FC_SkinMatchColor; I-0117 T6: a
// skinned instance's vertex frame rows (RtInstance::frame; zero: none). I-0125 R6a: its ambient rows (RtInstanceAmbient: atlasUvScaleOffset,
// vDominantRotateParam, the SH rows and ShLightMask), whether its SH rows were read (lit.x 1), its t26 and t27 lightmaps' slots (lit.y,
// lit.z; ~0 none) and cbTexHdrScaleParam's rgb (256 bytes). R6d: `forward` the byte of its forward block (RtForwardRow) in the
// frame's rows buffer, after the rows and the skinned bases (~0: none; R6d's final review: the block for its forward instances only, not
// in every row). The offsets rt_material.hlsli's RtRowOf reads asserted (R6d's final review).
struct RtInstanceRow {std::array<float,4> scroll;uint32_t positions,bases,first,count;std::array<float,4> scroll1,scroll3,skin;std::array<float,12> frame;
    std::array<float,4> atlas,rotate;std::array<float,16> sh;std::array<uint32_t,4> lit;std::array<float,3> hdr;uint32_t forward;};
static_assert(sizeof(RtInstanceRow)==256 && offsetof(RtInstanceRow,positions)==16 && offsetof(RtInstanceRow,scroll1)==32 &&
    offsetof(RtInstanceRow,scroll3)==48 && offsetof(RtInstanceRow,skin)==64 && offsetof(RtInstanceRow,frame)==80 && offsetof(RtInstanceRow,atlas)==128 &&
    offsetof(RtInstanceRow,rotate)==144 && offsetof(RtInstanceRow,sh)==160 && offsetof(RtInstanceRow,lit)==224 && offsetof(RtInstanceRow,hdr)==240 &&
    offsetof(RtInstanceRow,forward)==252);
// I-0125 R6d: a forward instance's block (rt_material.hlsli's RtForwardRowOf): its mulCol (cbInstanceData +0x70, the group's +0xD0 as
// ProduceInstanceRecord writes it) and its model's forward fog entry (cbFog's first 0x60 bytes: FogParam, FogCol, ExtFogParam,
// HeightFogParam, HeightFogCol, FogType; ProduceNativeForwardLight's). An instance has one only when both were read: the raster refuses
// the instance otherwise (R6d's final review: an unread mulCol was taken white), and the rays pass its forward slices over.
struct RtForwardRow {std::array<float,4> colour{};std::array<float,24> fog{};};
static_assert(sizeof(RtForwardRow)==112 && offsetof(RtForwardRow,fog)==16);
// I-0125 R6a: the cbInstanceData rows an instance's ambient reads, as the raster's record holds them (5B3D0: +0x30 atlasUvScaleOffset,
// +0xC0 vDominantRotateParam, +0x80..+0xBF the SH rows and ShLightMask); `sh_read` false when its lighting inputs were not read. Its
// lightmaps (a lightmapped set's): the t26 and t27 textures of its draw's lightmap set and their views (null: none), cbTexHdrScaleParam.
// I-0125 R6b: `light_set` its light and fog set (envData, NativeLightSetIndex of its group's keys), the hits' directional lights.
// I-0125 R6d: a forward set's block (RtForwardRow; null: none), its mulCol and fog read once the frame's light blocks are made, one
// block a draw group (the collect's CollectNativeRtForwardBlocks, before the input is published).
struct RtInstanceAmbient {std::array<float,4> atlas{1,1,0,0},rotate{0,0,0,1};std::array<float,16> sh{};bool sh_read{};uint32_t light_set{};
    std::array<std::shared_ptr<const void>,2> lightmaps;std::array<D3D12_CPU_DESCRIPTOR_HANDLE,2> views{};std::array<float,4> hdr{1,1,1,1};
    std::shared_ptr<const RtForwardRow> forward;};
// The draw group's four scroll rows (cbTexScroll VC_TexScroll_0 to _3, its first 64 bytes), the identity when unread.
using RtScrollRows=std::array<float,16>;
constexpr RtScrollRows kRtScrollIdentity{1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0};

// I-0117 (R2 T1): one instance of a model's BLAS in the top level.
struct RtInstance {
    std::array<float,12> transform{};   // 3x4 row-major, translation at [3], [7], [11]: DS3's placement as it keeps it, DXR's layout
    Microsoft::WRL::ComPtr<ID3D12Resource> blas;   // the structure read at the collect, held until the frame that traces it completed
    uint32_t id{};        // InstanceID (24 bits)
    uint8_t mask{};       // InstanceMask (RtMask*)
    // I-0119 R3a: its slices' materials (null: none read; every slice opaque) and its scroll rows (cbTexScroll VC_TexScroll_0 to _3,
    // the draw group's first 64 bytes; R3c reads _1 and _3); R3c: its FC_SkinMatchColor (PS cbModelParam +0x60: the draw group's
    // +0x40, or the part's appearance colour; read for a set with the skin's or hair's curve; unread: rgb 0, w -1, each curve's identity in the shader).
    std::shared_ptr<const RtMaterialSet> materials;RtScrollRows scroll=kRtScrollIdentity;std::array<float,4> skin{};
    RtInstanceAmbient ambient;   // I-0125 R6a
    // A skinned instance's positions (its BLAS's vertices, in the world) and each slice's first vertex there; null for a placed one.
    Microsoft::WRL::ComPtr<ID3D12Resource> positions;std::shared_ptr<const std::vector<uint32_t>> bases;
    // I-0117 T6: a skinned instance whose slices keep their vertex frame (a placed model moved by the wind: the Grass programmes
    // rotate NORMAL and TANGENT by the placement alone): that placement's rows, by which its records' frames are rotated; zero
    // for the others (their triangle's normal).
    std::array<float,12> frame{};
};
// The instance mask bits (R2-PLAN §3 T1), from the engine's categories (T0's walks): the main view's, the sun casters' 0x02, the
// local casters' 0x04, the reflection's 0x10; 0x80 an instance in none of them. T5: 0x10 an instance of the main view with a slice
// that is not a surface (RtInstanceMask; forward-only, transparent, an overlay): seen by the eye, not a G-buffer surface.
// I-0125 R6c: 0x20 a water slice's (a g_GXFT_PlanarReflection material: the water's own rays find its surface), on an instance the
// eye sees (its view or forward bit).
enum : uint8_t {RtMaskView=1,RtMaskSun=2,RtMaskLocal=4,RtMaskReflection=8,RtMaskForward=0x10,RtMaskWater=0x20,RtMaskOther=0x80};
// I-0117 (R2 T5): what an instance's slices are, by DS3's material classes (ClassifyModelMaterial, 142130: 1 opaque, 4 alpha-tested,
// 0x100 emissive, the G-buffer rows' classes of 57A20; 2 transparent, 0x80 forward, 8 and 0x10 decals, the forward rows'), over its
// LOD-0 slices; whether one of them casts (RtSliceCasts: the cascade row's rule). Not known while a slice's programme set is not
// built (the masks then stay the engine's).
struct RtSurfaceClasses {bool known{};uint32_t classes{};bool casts{};
    bool surfaces{};   // a slice is a surface: of a G-buffer class and writing depth (no g_GX_bDisableDepthWrite)
    bool mixed{};      // its slices answer different rays (RtSliceKeep): their records pass the others over
    bool overlays{};   // a slice is not a surface
    bool translucent{};   // I-0118 R5c: a transparent slice casts (RtSliceCasts)
    bool water{};};       // I-0125 R6c: a slice is water (RtMaskWater: g_GXFT_PlanarReflection)
constexpr uint32_t kRtGbufferClasses=0x105;   // 1 | 4 | 0x100
constexpr uint32_t kRtTransparentClass=2;
// I-0118 R5c: whether a slice casts, by the cascade row's rule (57A20's passes 1..4, class mask 0x107; 140680 refuses a Decal and
// its material's no-cast byte; a water or refraction material, g_GXFT_PlanarReflection or g_GXFT_Refraction (its policy's flags
// 0x30), is a kind-0x27 draw with the rows 0x24 and 0x27 alone, WT-1; the local faces take the same row, I-0106): a surface (a
// G-buffer class, its depth written), or a transparent slice (class 2: the hair's `HR_A` slices of the row 0x29, FaceGen's `FG_A`
// pieces), which the cascades draw whatever its depth write. Explicit policy: a G-buffer slice drawn without a depth write (the
// High Wall's sky, m45's grime cards) does not cast, though the cascades would draw it (T5 final review L1). The original cuts a
// caster at the cascades' extent, which the rays do not have for any class; the engine's category byte keeps the forward-only sky
// pieces seen so far out (category 1: no sun or local bit).
bool RtSliceCasts(uint32_t cls,bool surface,bool decal,bool no_cast,bool water) noexcept;
// One slice's ray bits: a surface (a G-buffer class, its depth written) answers every ray but RtMaskForward's, the sun's and the
// local lights' only when it casts; any other slice (forward-only, transparent, an overlay drawn without a depth write)
// RtMaskForward's and the reflection's, never the view's, and the casters' when it casts (R5c: a transparent slice).
// I-0125 R6c: a water slice also answers RtMaskWater's rays.
uint8_t RtSliceKeep(bool surface,bool casts,bool water=false) noexcept;
// The instance's mask from its engine bits (the categories' RtMask*) and its slices: with no surface slice (the sky's dome and
// clouds, glass, an overlay's own piece) the view bit becomes RtMaskForward, with some it gains it; with no slice that casts,
// no sun or local bit; an empty mask RtMaskOther.
uint8_t RtInstanceMask(uint8_t engine_bits,const RtSurfaceClasses& surface) noexcept;
// The instance records (D3D12_RAYTRACING_INSTANCE_DESC, 64 bytes each) of `instances` at `out`; each names its first record in
// the material table (InstanceContributionToHitGroupIndex: `firsts`, one an instance, RtMaterialTable::Encode's), and one whose
// set is non-opaque is FORCE_NON_OPAQUE (its candidates reach the ray query's alpha test); with no table (empty `firsts`) every
// instance is opaque.
void RtFillInstanceDescs(std::span<const RtInstance> instances,D3D12_RAYTRACING_INSTANCE_DESC* out,std::span<const uint32_t> firsts={}) noexcept;

// I-0117 (R2 T2, R2-PLAN §3): the skinned geometry. A model posed by a palette of more than one row (an assembled character's
// parts, a map item or an object with bones) has its own BLAS a placed instance, over its world positions, which a compute pass
// (rt_skin.hlsl) writes from the model storage's vertices and the pose's rows as DS3's vertex programmes do, again whenever the
// pose changed; the BLAS is then refitted in place (PERFORM_UPDATE), rebuilt every kRebuildAfter refits. Its slices keep the
// storage's order, so a ray query's GeometryIndex() is still the slice. T6: a placed model whose Grass programmes move it by the
// wind (its wind block set) is such an instance too, its slices placed by the world row (mode 3) and moved as the programmes move
// them, skinned again whenever its wind block changed (its time: every frame while it blows).
// One slice of a skinned instance: the storage's slice and how its vertices are placed (the vertex layout's elements).
struct RtSkinSlice {
    MeshSlice slice;
    // 0 excluded (an empty geometry: the mesh not drawn, or its layout not supported), 1 rigid (the bone in NORMAL.w), 2 blended,
    // 3 the instance's world row (T2c: a cloth mesh's simulated copy in model space, which the _cloth programmes place by VC_World).
    uint32_t mode{};
    uint32_t index_offset{},index_format{};     // the bone indices' byte offset in the vertex; 0 R8G8B8A8_UINT, 1 R16G16B16A16_UINT
    uint32_t weight_offset{},weight_format{};   // 0 R8G8B8A8_UNORM, 1 R16G16B16A16_UNORM, 2 R16G16B16A16_SNORM, 3 R32G32B32A32_FLOAT
    uint32_t palette{};   // 0 the pose's current rows, 1 its auxiliary rows (the cloth route)
    uint32_t cancel{};    // 1: its cancel rows applied first (a programme reading VS b12), 0 none
    // T2b: where its vertices are read: 0 the storage (at the slice's vertex offset), k > 0 the instance's sources[k - 1] (a physics
    // stream, the engine's rewrite of that mesh's vertex slot 0) from `source_offset`; the indices always the storage's.
    uint32_t source{},source_offset{};
    // T5: 1 when a blended slice's BLENDINDICES and BLENDWEIGHT are in the mesh's second stream (slot 1: the slice's vertex_offset2
    // and stride2, the storage's; the face's FG_A models, i117-t5d), their offsets then in that stream's vertex.
    uint32_t blend_stream{};
    // T6: the wind (RtWind*: the Grass programmes' VC_ForceParam displacement after the placement) and its TEXCOORD1's byte in the
    // vertex (slot 0); none without the instance's wind block.
    uint32_t wind{},wind_offset{};
    bool operator==(const RtSkinSlice&) const=default;
};
// TEXCOORD1 as R32G32B32(A32)_FLOAT, R16G16B16A16_FLOAT, R8G8B8A8_UNORM (the grass's FLVER UV1 Byte4C, i117-t6a) or B8G8R8A8_UNORM.
enum : uint32_t {RtWindNone=0,RtWindFloat3=1,RtWindHalf4=2,RtWindUnorm8=3,RtWindBgra8=4};
// I-0117 T6: an instance's wind block as the Grass vertex programmes read cbvModelParam: VC_ForceParam (its +0x80..+0x48F, the
// animation block [anim+210] when anim+620 is set: 13b_prepare_model_frame.inc) then VC_ModelSizeParam (its +0x4B0: group+0x60).
constexpr uint32_t kRtWindForceBytes=0x410,kRtWindBytes=kRtWindForceBytes+16;
using RtWindBlock=std::array<uint8_t,kRtWindBytes>;
// The shader's record of a slice (rt_skin.hlsl): 16 words; T5: the blend elements' stream (its first vertex's byte in the
// storage and its stride; a stride 0: the vertex's own stream); T6: the wind and its TEXCOORD1's byte in the vertex.
struct RtSkinRecord {uint32_t source_offset,stride,vertices,first,mode,index_offset,index_format,weight_offset,weight_format,palette,cancel,source,
    blend_offset,blend_stride,wind,wind_offset;};
static_assert(sizeof(RtSkinRecord)==64);
// The records of `slices` (the excluded ones too, with no vertex) and the vertices they need in all; `source_bytes`: the storage's
// size, then each further source's. A slice that cannot be skinned (its vertices past its source's bytes or its indices past the
// storage's, an element past the vertex or not 4-byte aligned, a source not given) is excluded; T6: a wind element so placed, or
// of an unknown format, leaves the slice without its wind (its rest pose).
uint64_t RtSkinRecords(std::span<const RtSkinSlice> slices,std::span<const uint64_t> source_bytes,std::vector<RtSkinRecord>& out);
using RtPoseRows=std::vector<std::array<float,12>>;
// The palette rows `slices` index, by role (0 the current rows, 1 the auxiliary rows, 2 the cancel rows), read from the storage's
// bytes as rt_skin.hlsl reads them: a rigid slice's NORMAL.w, a blended slice's four BLENDINDICES (a zero weight still reads its
// row: 0 times a row not finite is not finite, in the programme as in the shader), the cancel's role with the cancel; a world-row
// slice (mode 3) none. `all`: a role some slice reads from another source (a physics stream, a cloth copy: indices only the GPU
// sees) or past `bytes`: every row. Sorted, each row once.
struct RtSkinRows {std::array<std::vector<uint32_t>,3> rows;std::array<bool,3> all{};};
RtSkinRows RtSkinRowsOf(std::span<const uint8_t> bytes,std::span<const RtSkinSlice> slices);
// A skinned instance's GPU state: its positions, its BLAS (ALLOW_UPDATE) and when it was last written. The recording thread only.
struct RtSkinnedBlas {
    Microsoft::WRL::ComPtr<ID3D12Resource> positions,structure;
    uint64_t positions_bytes{},structure_bytes{},update_scratch{},build_scratch{};
    std::array<uint64_t,2> generation{};bool built{};uint32_t updates{};   // the pose and stream generations last written; refits since the last build
    bool failed{};   // no vertex it can skin (its slices fixed): left out of the rays (T6: a placed model moved by the wind traced in its rest pose)
    uint32_t retry{};   // frames before its resources are tried again after a placement was refused (T2 step review L2)
    std::vector<uint32_t> vertices;   // each slice's vertices at its first build (a refit must keep its geometries)
    std::shared_ptr<const std::vector<uint32_t>> bases;   // I-0119 R3a: each slice's first vertex in `positions` (its record's `first`)
};
struct RtSkinnedInstance {
    std::shared_ptr<RtSkinnedBlas> blas;              // kept across frames for this instance by the collect
    Microsoft::WRL::ComPtr<ID3D12Resource> source;    // the model storage's buffer (its vertices and indices)
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> sources;   // T2b: its physics streams' buffers (RtSkinSlice::source - 1)
    // What keeps the sources' memory until the frame completed: their upload versions (a pooled upload buffer goes back to its pool
    // when its version's last holder goes, not its resource's).
    std::vector<std::shared_ptr<const void>> owners;
    std::shared_ptr<const std::vector<RtSkinSlice>> slices;
    std::array<std::shared_ptr<const RtPoseRows>,3> palettes;   // current, auxiliary, cancel (null: not bound)
    std::array<float,12> world{1,0,0,0, 0,1,0,0, 0,0,1,0};      // T2c: VC_World for the mode-3 slices (the cloth instance frame since I-0113 tranche 37)
    std::shared_ptr<const RtWindBlock> wind;                    // T6: its wind (null: its slices' wind not applied)
    bool framed{};   // T6: a placed model moved by the wind: its vertex frame by `world` (RtInstance::frame)
    Microsoft::WRL::ComPtr<ID3D12Resource> rest;   // T6: its storage's BLAS, traced at `world` when it cannot be skinned (null: left out)
    // The pose's (a new one each pose update), and its physics streams' and cloth copies' with the cloth frame when a mode-3 slice
    // reads it and its wind block (T6) when it has one (0: none).
    std::array<uint64_t,2> generation{};
    uint32_t id{};uint8_t mask{};
    bool hold{};   // its pose has a row not finite: not skinned again, its last positions traced (T2 final review M2)
    std::shared_ptr<const RtMaterialSet> materials;RtScrollRows scroll=kRtScrollIdentity;std::array<float,4> skin{};   // R3a, R3c: as RtInstance's
    RtInstanceAmbient ambient;   // I-0125 R6a: as RtInstance's
};
// What the skinning did in a frame (the native_rt_walk row's T2 part).
// bytes: the positions and structures of the instances skinned so far (the frame's scratch apart); T6: winds, the instances skinned
// with their wind, of them, and their vertices.
struct RtSkinCounters {uint64_t instances{},builds{},refits{},vertices{},failed{},bytes{},winds{},wind_vertices{};};
// The skinning of a frame's skinned instances, on the frame's list before its top level: the ones whose pose changed (or never
// written) skinned, then built (first, or after kRebuildAfter refits) or refitted; each one built becomes an instance of the top
// level with the identity transform (its positions are in the world); T6: one left out with a `rest` structure traced in its rest
// pose at `world` instead (counted with the left out). Throws on a device failure.
class RtSkinner {
public:
    static constexpr uint32_t kRebuildAfter=120;   // refits before a rebuild (a refit keeps the first build's tree: quality drifts)
    using Upload=std::function<UploadArena::Range(uint64_t bytes)>;
    void Encode(DeviceQueues& gpu,ID3D12GraphicsCommandList4* list,const Upload& upload,std::span<const RtSkinnedInstance> skinned,
        std::vector<RtInstance>& out,RtSkinCounters& counters);
    uint64_t Bytes() const noexcept {return scratch_bytes_;}
    void Close() noexcept {root_.Reset();pipeline_.Reset();scratch_.Reset();scratch_bytes_=0;}
private:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_;Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_;
    Microsoft::WRL::ComPtr<ID3D12Resource> scratch_;uint64_t scratch_bytes_{};
};

// The top level, rebuilt each frame on the frame's own list from that frame's instance records (one direct queue: a rebuild runs
// after the previous frame's reads). Its structure and scratch grow by half again when a frame needs more, the former retired
// after the list being recorded (RetireAfterCurrentRecording). The recording thread only.
class RtTlas {
public:
    // `descs`: `count` records in the frame's upload memory (16-byte aligned). Returns the structure's address (0: no instance);
    // the structure is readable by ray queries after this call (a UAV barrier closes the build). Throws on a device failure.
    D3D12_GPU_VIRTUAL_ADDRESS Build(ID3D12Device5* device,ID3D12GraphicsCommandList4* list,D3D12_GPU_VIRTUAL_ADDRESS descs,uint32_t count);
    ID3D12Resource* Resource() const noexcept {return structure_.Get();}
    uint64_t Bytes() const noexcept {return structure_bytes_+scratch_bytes_;}
    void Close() noexcept {structure_.Reset();scratch_.Reset();structure_bytes_=scratch_bytes_=0;}
private:
    Microsoft::WRL::ComPtr<ID3D12Resource> structure_,scratch_;uint64_t structure_bytes_{},scratch_bytes_{};
};

// I-0119 (R3a, R3-PLAN §3): the material table, in place of T3's per-frame alpha records. Each material set an instance names gets
// its records once, in a GPU table kept across frames (a default buffer, the records copied in on the frame's list when a set is
// first named, the table grown by half again when full: its former buffer retired after the list being recorded); a set's range is
// freed when the set is gone or no frame named it for kRetireFrames frames (the queue runs the lists in order, so a later copy into
// it follows every read of an earlier frame). The views the records name in the bindless heap (one raw view a buffer, the raster's
// own view copied for a texture, a skinned instance's positions) are counted by the records and frames that name them and released
// against the ticket of the last submission that may read them; a view the full heap cannot take leaves that texture out (or its
// slice opaque without a buffer: counted in `full`). Each frame also writes the instances' rows (RtInstanceRow, by their index in
// the top level), the skinned instances' slice bases and, I-0125 R6d, the forward blocks (RtForwardRow) into one frame buffer. Encode with no group still counts the frame and
// sweeps (a frame with no rays, a load). The recording thread only (the heap's frame owner).
class RtMaterialTable {
public:
    static constexpr uint64_t kRetireFrames=120,kSweepFrames=16;   // the sets' retirement, swept every kSweepFrames-th frame
    using Upload=std::function<UploadArena::Range(uint64_t bytes)>;
    struct Frame {
        D3D12_GPU_VIRTUAL_ADDRESS table{},rows{};   // the records (RtMaterialRecord) and the rows buffer (0: none)
        uint32_t records{},instances{};std::vector<uint32_t> firsts;   // each instance's first record (RtFillInstanceDescs)
    };
    // The records and rows of `groups` (each a span of instances, in the order RtFillInstanceDescs numbers them), the copies and
    // their barriers on `list` (the table and rows readable by any shader after it). `ticket`: the last submitted ticket (every
    // list that may read a released view is submitted); `completed`: the queue's completed fence value. Throws on a device failure.
    Frame Encode(ID3D12Device* device,ID3D12GraphicsCommandList* list,BindlessHeap& heap,const Upload& upload,uint64_t ticket,uint64_t completed,
        std::initializer_list<std::span<const RtInstance>> groups);
    // The sets, records and views alive now; the records written and the views refused since the start; the table's records; the
    // last call's time and the mean of the calls that named instances (ms, final review M3).
    struct Counters {uint64_t sets{},records{},buffers{},textures{},written{},full{},capacity{};double encode_ms_last{},encode_ms_mean{};};
    Counters Totals() const noexcept;
    ID3D12Resource* TableResource() const noexcept {return table_.Get();}   // the unit test's views
    ID3D12Resource* RowsResource() const noexcept {return rows_.Get();}
    RtMaterialRecord RecordAt(uint32_t index) const {return shadow_.at(index);}   // the unit test's: a record as written
    void Close(BindlessHeap* heap,uint64_t ticket) noexcept;
private:
    // A view: counted by the entries naming it (a storage's buffer, a texture), or last named by a frame (a skinned instance's
    // positions, named by the frame's rows alone, released kRetireFrames frames after).
    struct View {uint32_t slot{BindlessIndices::none};uint64_t uses{},named{};Microsoft::WRL::ComPtr<ID3D12Resource> buffer;std::weak_ptr<const void> texture;};
    struct Entry {std::weak_ptr<const RtMaterialSet> set;uint32_t first{},count{};uint64_t named{};std::vector<const void*> textures;ID3D12Resource* buffer{};};
    static uint32_t RawView(ID3D12Device* device,BindlessHeap& heap,uint64_t completed,View& view,ID3D12Resource* buffer);
    uint32_t TextureView(ID3D12Device* device,BindlessHeap& heap,uint64_t completed,const std::shared_ptr<const void>& texture,D3D12_CPU_DESCRIPTOR_HANDLE view);
    void Retire(BindlessHeap& heap,uint64_t ticket,const Entry& entry) noexcept;   // its views unused, its range freed
    void Free(uint32_t first,uint32_t count);
    uint32_t Allocate(uint32_t count);   // UINT32_MAX: the table must grow
    std::unordered_map<ID3D12Resource*,View> buffers_,positions_;std::unordered_map<const void*,View> textures_;
    std::unordered_map<const void*,View> lightmaps_;   // I-0125 R6a: the instances' lightmaps by texture, named by the frames that read them
    std::unordered_map<const RtMaterialSet*,Entry> entries_;
    std::vector<std::pair<uint32_t,uint32_t>> free_;   // free ranges (first, count), sorted by first, adjacent ones joined
    std::vector<RtMaterialRecord> shadow_;              // the table's records on the CPU (a grown table is written whole from it)
    Microsoft::WRL::ComPtr<ID3D12Resource> table_;uint32_t capacity_{};
    Microsoft::WRL::ComPtr<ID3D12Resource> rows_;uint64_t rows_bytes_{};
    uint64_t frame_{},written_{},full_{},records_{};
    double encode_ms_last_{},encode_ms_sum_{};uint64_t encode_calls_{};
};

// I-0117 (R2 T4, R2-PLAN §3): the control view (rt_view.hlsl): the main view's primary rays through the frame's top level against
// the raster depth after the G-buffer. Its constants (rt_view.hlsl's View): the camera's right, up, forward and position rows in
// the world with the near plane, the far plane, sx and sy in their fourth words; the jitter in NDC (2 jx / W, -2 jy / H) and 1 / W,
// 1 / H; the extent; the slots of the depth's SRV (R32 of the D32S8) and of the two pictures' UAVs (R8G8B8A8_UNORM, the extent);
// the pass (1: clear the counts); the instance mask; 1 with a material table (I-0119 R3a: the traced picture the hit's albedo).
struct RtViewConstants {
    std::array<float,4> right{},up{},forward{},position{},jitter{};
    uint32_t width{},height{},depth{},traced{},classes{},clear{},mask{RtMaskView},materials{};
    // I-0119 R3b: the materials picture's UAV and GB0 to GB3's SRVs (BindlessIndices::none: no comparison).
    uint32_t differences{UINT32_MAX},gb0{UINT32_MAX},gb1{UINT32_MAX},gb2{UINT32_MAX},gb3{UINT32_MAX};
    uint32_t anisotropy{1};float texture_bias{};uint32_t pad2{};   // R3c: the raster's material samplers' (NativeRtSceneInput)
    // I-0125 R6a: GB4's SRV (the ambient compared) and the frame's dominant rows (NativeRtSceneInput::dominant).
    uint32_t gb4{UINT32_MAX};std::array<uint32_t,3> pad3{};std::array<float,4> dominant{};
};
static_assert(sizeof(RtViewConstants)==176);
// From the frame's camera (BuildSceneView's lens, the projection ProduceSceneCamera gives the raster with the upscaler's jitter):
// false when the camera is refused (BuildSceneView's checks).
bool RtViewCameraOf(const SceneViewInput& camera,RtViewConstants& out) noexcept;
// The pixels of a view by class (the counts' words in order): the traced distance within 2 cm + 1 % of the raster's, within 5 %,
// nearer, farther; no hit on a raster surface; no surface in either; a hit where the raster has none; T5: of the nearer, those
// whose hit triangle is clockwise as DXR sees it (the raster's back face for an unmirrored instance; not a class of its own).
// The disagreeing pixels (close, nearer, farther, missed) whose ray met an alpha-tested candidate (tested, kept or not): where the
// alpha test (its top mip, no dither) and the wind's grass in its rest pose (R2-PLAN T2) can explain them; not a class of its own.
struct RtViewCounts {uint64_t agree{},close{},nearer{},farther{},missed{},sky{},extra{},nearer_cw{},alpha_met{};
    uint64_t Surfaces() const noexcept {return agree+close+nearer+farther+missed;}};
RtViewCounts RtViewCountsOf(const uint32_t words[16]) noexcept;
// I-0119 R3b: one programme family's comparison with the G-buffer at the agreeing pixels whose hit has a material (rt_view.hlsl):
// the pixels compared, those within tolerance by channel (albedo, F0, gloss within 8 steps of 255 as the G-buffer stores them,
// the normal within 10 degrees), the summed errors (albedo and gloss in steps, the normal in tenths of a degree); R3c: the signed
// albedo errors by channel and the signed gloss error (traced less raster, steps), the pixels whose second-layer weight is at least
// one half and their summed albedo error.
constexpr uint32_t kRtViewFamilies=256;   // the families' indices (RtMaterialSlice::family) counted apart; past it with index 0
constexpr uint32_t kRtViewFamilyWords=20;   // the counts' words a family (rt_view.hlsl)
// I-0125 R6a: the hit's ambient against GB4 at those pixels whose family writes one: the pixels compared, those within 5 % (its
// luminance, 1/1024 apart allowed), the signed and absolute sums of 256 log2 of the luminances' ratio (traced over raster), the
// pixels whose ambient's inputs were missing (a lightmap without a view, an instance without its SH rows).
struct RtMaterialCheck {uint64_t pixels{},albedo{},f0{},gloss{},normal{},albedo_error{},gloss_error{},normal_error{};
    std::array<int64_t,3> albedo_bias{};int64_t gloss_bias{};uint64_t layered{},layered_error{};
    uint64_t ambient{},ambient_close{};int64_t ambient_bias{};uint64_t ambient_error{},ambient_missing{};
    void Add(const RtMaterialCheck& o) noexcept {pixels+=o.pixels;albedo+=o.albedo;f0+=o.f0;gloss+=o.gloss;normal+=o.normal;albedo_error+=o.albedo_error;
        gloss_error+=o.gloss_error;normal_error+=o.normal_error;for(size_t k=0;k<3;++k)albedo_bias[k]+=o.albedo_bias[k];gloss_bias+=o.gloss_bias;
        layered+=o.layered;layered_error+=o.layered_error;
        ambient+=o.ambient;ambient_close+=o.ambient_close;ambient_bias+=o.ambient_bias;ambient_error+=o.ambient_error;ambient_missing+=o.ambient_missing;}};
// The samples read back since the start: how many, the last one (its frame serial) and their sum; R3b: by family, summed.
struct RtViewTotals {uint64_t samples{},serial{};RtViewCounts last{},sum{};std::array<RtMaterialCheck,kRtViewFamilies> families{};};
class RtView {
public:
    static constexpr uint64_t kCountBytes=64+4*kRtViewFamilyWords*kRtViewFamilies;   // the counts' buffer (16 words, 9 used; R3b: a family's words)
    // The pass on `list`: the counts cleared, then one ray a pixel; the bindless heap set on the list (the caller's later passes set
    // their own heaps). `scene`: the top level; `table`, `rows`: the frame's material table and rows (RtMaterialTable::Frame; a table 0:
    // every candidate opaque); `counts`: a buffer of
    // kCountBytes with UNORDERED_ACCESS allowed, in COMMON or UNORDERED_ACCESS, left in UNORDERED_ACCESS with a UAV barrier after
    // the pass; the depth and the pictures in their read and UAV states. Throws on a device failure.
    void Encode(ID3D12Device* device,ID3D12GraphicsCommandList* list,BindlessHeap& heap,const std::function<UploadArena::Range(uint64_t)>& upload,
        D3D12_GPU_VIRTUAL_ADDRESS scene,D3D12_GPU_VIRTUAL_ADDRESS table,D3D12_GPU_VIRTUAL_ADDRESS rows,RtViewConstants constants,ID3D12Resource* counts);
    void Close() noexcept {root_.Reset();pipeline_.Reset();}
private:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_;Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_;
};

// I-0117 R5 (RT-PT-PLAN §8, D-0237; on with the rays): the sun's shadow traced into the shadow and AO mask's w in place of the
// original's mask layer (rt_shadow.hlsl). Its constants (rt_shadow.hlsl's Sun): the camera as RtViewConstants' first rows, the
// extent, the depth's slot (R32 of the D32S8), 1 with a material table; the direction toward the sun (world, unit; the LightSet's
// light 0 as the cascade fit takes it); the ray's offset: x metres and y a share of the distance pulled back toward the eye, z the
// ray's TMin; w 1 for the orientation witness (a surface facing away from the sun written 0.5: rt-shadow-witness.flag, a test tool).
// I-0122 R4b (final review L7): the shadow rays' offset the traced sun and the local lights share.
inline constexpr std::array<float,4> kRtShadowRayOffset{0.02f,0.002f,0.01f,0};
// I-0123 RR-b (RR-PLAN §3): the penumbra, on the frames Ray Reconstruction denoises: the cosine of the sun's angular radius (0: one
// sharp ray toward the direction) and the frame's seed for the white noise of the cone's sample (step review L3: a uint, not float bits).
struct RtSunConstants {
    std::array<float,4> right{},up{},forward{},position{},jitter{};
    uint32_t width{},height{},depth{},materials{};
    // I-0140 tranche 31: `neutral` 1 writes the mask's RGB white (a Player menu view's, whose lighting reads its xyz as the neutral
    // white: no composite there) in place of the « no shadow » packed depth (0, 0, 1).
    std::array<float,4> sun{},offset{kRtShadowRayOffset};float cone{};uint32_t seed{};uint32_t neutral{};float pad3{};
    // I-0140 tranche 35: `neutral` 2, a target a blur pass reads next (xyz white on the G-buffer's pixels, black off it); `source`
    // the bindless slot of a blur pass's source, `blur` its scale (pixels a tap unit).
    uint32_t source{};float blur{};uint32_t pad4{},pad5{};
};
static_assert(sizeof(RtSunConstants)==160);
// I-0123 RR-b: the sun's angular radius on the frames RR denoises, wider than the physical 0.27°, toward the original's blurred
// cascades, and a traced local light's sphere: a share of its range, at most 0.5 m (rt_lights.hlsl's RtLightRadiusMax; DS3's lights are
// points). Thomas's choice on the pictures (2026-11-04 02:13, I-0122 R4c): 2° and 10 %, the first radii (1°, 5 %, 0.25 m) hard to see.
inline constexpr float kRtSunAngularRadius=0.034906585f;
inline constexpr float kRtLightRadiusShare=0.10f;
// The camera rows of `view` (RtViewCameraOf's) and the sun's direction, normalised: false when it is not finite or zero.
bool RtSunConstantsOf(const RtViewConstants& view,const std::array<float,4>& sun,RtSunConstants& out) noexcept;
class RtSunShadow {
public:
    // The two draws on `list` (the full-screen triangle twice: OutputColor's value where the G-buffer stencil bit 0x08 is clear,
    // the traced term where it is set) into the mask through `mask_rtv` with the read-only depth view `stencil` bound; the mask a
    // render target, the depth readable by pixel shaders (its depth plane) and its stencil plane tested. The bindless heap set
    // on the list; the viewport and scissor the caller's. `scene` a top level (not 0). Throws on a device failure, and on a mask
    // format other than the first one's (the pipelines are made once). Close only after the queue's drain.
    void Encode(ID3D12Device* device,ID3D12GraphicsCommandList* list,BindlessHeap& heap,const std::function<UploadArena::Range(uint64_t)>& upload,
        D3D12_GPU_VIRTUAL_ADDRESS scene,D3D12_GPU_VIRTUAL_ADDRESS table,D3D12_GPU_VIRTUAL_ADDRESS rows,RtSunConstants constants,D3D12_CPU_DESCRIPTOR_HANDLE mask_rtv,
        D3D12_CPU_DESCRIPTOR_HANDLE stencil,DXGI_FORMAT mask_format);
    // I-0140 tranche 35: one pass of the original's screen-space shadow blur (rt_shadow.hlsl's `blur`) after Encode made the
    // pipelines: OutputColor's value (`fill`) where the stencil bit 0x08 is clear, the blur of `constants.source` (a mask Encode or
    // an earlier pass wrote, shader-readable) where it is set, into the target `target_rtv` (a render target of the mask's format)
    // with the same read-only depth view. `constants.depth` the depth's slot, `constants.blur` positive and finite. The blur's
    // pipeline made on the first call. Throws on a missing input, on a device failure, or before Encode made the pipelines.
    void EncodeBlur(ID3D12Device* device,ID3D12GraphicsCommandList* list,BindlessHeap& heap,const std::function<UploadArena::Range(uint64_t)>& upload,
        D3D12_GPU_VIRTUAL_ADDRESS scene,const RtSunConstants& constants,D3D12_CPU_DESCRIPTOR_HANDLE target_rtv,D3D12_CPU_DESCRIPTOR_HANDLE stencil);
    void Close() noexcept {root_.Reset();fill_.Reset();trace_.Reset();blur_.Reset();format_=DXGI_FORMAT_UNKNOWN;}
private:
    void MakePipeline(ID3D12Device* device,const unsigned char* pixel,size_t size,bool written,DXGI_FORMAT format,
        Microsoft::WRL::ComPtr<ID3D12PipelineState>& out,const char* what);
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_;Microsoft::WRL::ComPtr<ID3D12PipelineState> fill_,trace_,blur_;DXGI_FORMAT format_{};
};

// I-0122 R4 (R4-PLAN §4): DS3's local lights as one world-space list, a record a light (rt_lights.hlsli's RtLight): position (xyz
// world; w the point's 1 / R², the spot's range R), diffuse (DiffColor, w the gloss clamp), specular (SpecColor), the kind (an
// integer: step review L2; R4b: kRtLightShadowed for a shadowed light), trans (TransLightParam x, y; z a shadowed light's strength;
// w the spot's ShadingMode, a shadowed point's cube faces' near plane), the spot's world-to-light rows x, y, z, w.
enum : uint32_t {kRtLightPoint=0,kRtLightLegacy=1,kRtLightSpot=2,kRtLightShadowed=0x100,kRtLightPhysical=0x200};   // R4b: the shadowed flag beside the kind; I-0132 R11h: the physical falloff
struct RtLight {std::array<float,4> position{},diffuse{};std::array<float,3> specular{};uint32_t kind{};std::array<float,4> trans{};std::array<float,16> to_light{};};
static_assert(sizeof(RtLight)==128);
// A point light's record from its batch row (cbLightAccLightParam PointLight, 16 floats: the view-space position and 1 / R², DiffColor,
// SpecColor, TransLightParam) and its world position (the light's +0x150). False when a value is not finite.
bool RtPointLightOf(const std::array<float,16>& row,const std::array<float,3>& world,bool legacy,RtLight& out) noexcept;
// A spot's from its batch row (SpotLight, 32 floats: PositionAndRange, DiffColor, SpecColor, TransLightParam and ShadingMode in
// view space, then the ViewToLightSpaceMatrix's rows, each applied as dot(row, (p, 1)) to a view-space p), the camera's pose
// (view to world) and its inverse (world to view), both row-major with the translation in the last row (row vectors). False
// when a value is not finite.
bool RtSpotLightOf(const std::array<float,32>& row,const float pose[16],const float view[16],RtLight& out) noexcept;
// R4b: a shadowed light's record from its `Shadowed` rows (the shadow words after its batch row: a point's ProjParam, ShadowCenterPos
// and faces; a spot's ShadowParam and rectangle): kRtLightShadowed, the strength (ProjParam.w, ShadowParam.x) and a point's cube
// faces' near plane −ProjParam.y / ProjParam.x (0, unknown, unless ProjParam.x > 0 and the plane positive and finite). False when the
// strength is not finite or the rows too short (a point's four words, a spot's one).
// I-0132 R11h (R11-PLAN section 9 step 2, a test tool: rt-local-physical-*.flag): the record in physical terms, one universal rule for
// every local light (LIGHT-UNITS.md): its intensity (the illuminance at 1 m, DS3's units) from DS3's colours, a point's as they are (its
// diffuse falloff max(1/d^2 - 1/R^2, 0) is already the inverse square at 1 m), a legacy point's and a spot's without its ShadingMode
// (falloffs that are not the inverse square) matched to it at half their range (x R^2 / 16), then times `scale` (the ratio to the
// sun); the falloff 1 / max(d^2, 1 cm^2) for the diffuse and the specular alike, windowed at r by Frostbite's saturate(1 - (d / r)^4)^2
// instead of DS3's cut at R, r = sqrt(the intensity's luminance before `scale` / e_min) (a reach that grows with the light's
// brightness, from the brighter of the diffuse and the specular); kRtLightPhysical and position.w = 1 / r^2 for every kind (the grid's
// and the shader's support a sphere), DS3's range R as a half float in the kind's bits 16 to 31 (the soft shadow's sphere and the hits'
// stop keep it). False (the record unchanged) when r is not finite and positive.
bool RtPhysicalLight(RtLight& light,float scale,float e_min) noexcept;
// The reach's threshold (DS3's units at x1): DS3's torch-like light (0.12) reaches 8 m (its DS3 range 2 m), the bonfire's (0.6) 18 m (6 m).
inline constexpr float kRtLocalPhysicalMinimum=0.0019f;
bool RtShadowedOf(std::span<const float> shadow,RtLight& record) noexcept;
// The frame's list: the records, and per 128-pixel tile of the view (columns × rows) its {first word, count} then the records'
// indices (the batches' binning and order), as rt_lights.hlsl reads them.
// I-0128 R8c: `grid`, the lights binned in a world grid about the camera (rt_light_grid.h's words; empty: none, the hits loop over
// the whole list).
struct RtLightList {std::vector<RtLight> lights;std::vector<uint32_t> tiles;uint32_t columns{},rows{},edge{128};std::vector<uint32_t> grid;};
// The pass's constants (rt_lights.hlsl's Lights): the camera as RtViewConstants' first rows, the extent, the bindless slots of the
// depth (R32 of the D32S8), GB0, GB1, GB3, a word unused since I-0126 R7c (the SSAO term's slot, D-0251), the tiles' columns and edge; R4b: the shadowed lights traced (1),
// shaded unshadowed (2: no top level; I-0131 R10a), whether a material table is bound (both set by Encode), the shadow ray's offset.
struct RtLightsConstants {
    std::array<float,4> right{},up{},forward{},position{},jitter{};
    uint32_t width{},height{},depth{},gb0{},gb1{},gb3{},pad{},columns{},tile{},traced{},materials{},seed{};   // I-0123 RR-b: the frame's seed
    std::array<float,4> offset{kRtShadowRayOffset};   // I-0123 RR-b: w the shadowed lights' sphere radius as a share of their range (0: sharp)
};
static_assert(sizeof(RtLightsConstants)==144);
class RtLocalLights {
public:
    // I-0132 R12d: the RT and PT levels' pipelines made once through the device's pipeline store (a start-up worker, rt_scene.cpp), so
    // that Encode finds them there; `done` counts each one tried; the number made returned.
    static uint32_t Warm(ID3D12Device* device,const std::atomic<bool>& cancel,std::atomic<uint32_t>& done) noexcept;
    static constexpr uint32_t kWarmPipelines=2;
    static D3D12_GRAPHICS_PIPELINE_STATE_DESC Describe(bool pt,ID3D12RootSignature* root) noexcept;   // Encode's and Warm's
    // One draw on `list` (the full-screen triangle) added (ONE/ONE) into the diffuse and specular accumulation targets `rtvs`
    // (R11G11B10, as the light programmes' pair); the G-buffer and depth readable by pixel shaders through the bindless heap,
    // which is set on the list (the caller sets its own heap, root signature and bindings again after it). `lights` and `tiles` the
    // frame's list (RtLightList) in buffers the GPU reads. R4b: `scene` the frame's top level (0: none, the shadowed lights shaded unshadowed: final review L5, I-0131 R10a),
    // `table` and `rows` its material table and instance rows (0: none, every instance opaque). I-0122 R4c: `pt` with a top level, the PT level's programme
    // (rt_lights.hlsl `pt`: every light of the tile casts, one sampled a pixel) in place of `shade`. Throws on a device failure. Close
    // only after the queue's drain.
    void Encode(ID3D12Device* device,ID3D12GraphicsCommandList* list,BindlessHeap& heap,const std::function<UploadArena::Range(uint64_t)>& upload,
        D3D12_GPU_VIRTUAL_ADDRESS lights,D3D12_GPU_VIRTUAL_ADDRESS tiles,D3D12_GPU_VIRTUAL_ADDRESS scene,D3D12_GPU_VIRTUAL_ADDRESS table,
        D3D12_GPU_VIRTUAL_ADDRESS rows,RtLightsConstants constants,const D3D12_CPU_DESCRIPTOR_HANDLE rtvs[2],bool pt=false);
    void Close() noexcept {root_.Reset();pipeline_.Reset();pt_.Reset();pt_refused_=false;}
    bool PtRefused() const noexcept {return pt_refused_.load(std::memory_order_relaxed);}   // R4c: the PT pipeline refused (the RT level drawn instead)
private:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_;Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_,pt_;
    std::atomic<bool> pt_refused_{};   // read by the game thread's row
};

// I-0125 R6b (R6-PLAN §3): the deferred surfaces' specular ambient traced (rt_reflect.hlsl), in place of the region draws. Its
// constants (rt_reflect.hlsl's Reflect): the camera as RtViewConstants' first rows, the extent, the slots of the depth, GB0, GB1, GB3,
// whether a material table is bound, the frame's seed, the counts of the regions, the lights and the light sets, the mode
// (kRtReflect*), the raster's material samplers' anisotropy and mip bias, `gi_paths` (I-0132 R12g: the GI's paths a pixel at the PT level, 1
// to kRtGiPathsMost; the former `cut`, unread since I-0132 R11d: no roughness cut), the sun's
// cone (its cosine; 0 sharp), the shadow rays' offset, `dominant` and `shadow_colour` (unread since R11d, D-0262: no baked ambient, no
// FC_ShadowColor floor; the layout kept), the
// frame's sun (toward it, world: the traced shadow's direction, with kRtReflectSun) and in its w kRtLightRadiusShare; R6c (R6-PLAN
// §6): the water's plane (n·X + w = 0) and the reflection target's extent, read by the water's programme only.
// I-0125 R6d: kRtReflectSkyWitness (rt-sky-witness.flag, a diagnostic): the water's texels coloured by what their ray met (rt_reflect.hlsl's
// `water`: magenta the forward sky, the other colours there).
enum : uint32_t {kRtReflectTraced=1,kRtReflectPt=4,   // 2 free since I-0132 R11d (kRtReflectLobe: every reflection samples its lobe)
    kRtReflectSun=8,kRtReflectSkyWitness=16,
    kRtReflectGiNoAmbient=64,kRtReflectGiNoLocal=128,   // I-0126 R7a's diagnostics (the GI pass and the cache's update)
    kRtReflectCache=256,kRtReflectCacheClear=512,       // I-0126 R7b: the cache resolved this frame (the hits' ambient its own); its clear
    kRtReflectGiNoSky=1024,kRtReflectCacheWitness=2048,   // R7b's diagnostics: no misses (the GI and the update); the cells' answers coloured (the GI pass)
    kRtReflectGiMean=4096,kRtReflectGiMeanRestart=8192,   // I-0126 R7d's bias check (rt-gi-mean.flag): the GI's estimate accumulated (constants.mean), restarted
    kRtReflectGiNoGlossy=16384,    // R7d's diagnostic: the GI hits without their specular (rt-gi-no-glossy.flag)
    kRtReflectPathNoCache=32768,   // I-0128 R8b's reference (rt-pt-no-cache.flag): the PT level's paths 8 vertices without the cache
    kRtReflectLightGrid=65536,     // I-0128 R8c: the hits' lights from the frame's grid (RtLightList::grid, right after the records: Bind)
    kRtReflectRoulette=131072,     // I-0132 R11a: Russian roulette past a path's first vertex (the default; no-rt-roulette.flag the test tool; never with the reference)
    kRtReflectEmissiveX4=262144,kRtReflectEmissiveX16=524288,   // I-0132 R11d's test tools: the hits' emission x4, x16 (rt-emissive-x4.flag, -x16)
    kRtReflectHitNoFill=1u<<20,kRtReflectHitNoSun=1u<<21};   // I-0141 tranche 28's share variant (bits 128, 256): the sets' lights 1 to 3, light 0, out of the hits' light
// I-0128 R8b (R8-PLAN §3): the PT level's traced vertices a path (RtReflectConstants::bounces; the settings' bounces, 1 to the most), 1 the
// R7 paths (the cache at the first hit).
inline constexpr uint32_t kRtPathVerticesDefault=2,kRtPathVerticesMost=4;
// I-0132 R12g: the GI's paths a pixel at the PT level (RtReflectConstants::gi_paths; rt-gi-paths-N.flag, a test tool since its settings row went, ROADMAP workstream 2): ReSTIR GI's initial candidates,
// or the one-ray GI's rays averaged where ReSTIR does not run; 1 off the PT level.
inline constexpr uint32_t kRtGiPathsDefault=1,kRtGiPathsMost=4;
// I-0126 R7b (R7-PLAN §3): NVIDIA's radiance cache (SHaRC 1.6.5, rt_reflect.hlsl's cache_update and cache_resolve): its entries
// (rt_reflect.hlsl's RT_CACHE_ENTRIES: SHaRC's guide's baseline, 40 bytes each, 160 MiB), the update's block (one path a block of
// kRtCacheBlock x kRtCacheBlock pixels: RT_CACHE_BLOCK) and the resolve's group (256 threads).
inline constexpr uint32_t kRtCacheEntries=1u<<22,kRtCacheBlock=5,kRtCacheGroup=256;
inline constexpr float kRtCacheJump=100.f;   // metres between two GI frames' cameras that clear the cache (a warp, a map change)
// A region in the pass's buffer: CB_ENVMAP_REGION (400 bytes), its cube's bindless slot, 1 when parallax-corrected (_Pcc).
inline constexpr uint32_t kRtReflectRegionBytes=416;
struct RtReflectConstants {
    std::array<float,4> right{},up{},forward{},position{},jitter{};
    uint32_t width{},height{},depth{},gb0{},gb1{},gb3{},materials{},seed{},regions{},lights{},sets{},mode{},anisotropy{1};float texture_bias{};uint32_t gi_paths{1};float cone{};   // I-0132 R12g: gi_paths in the former `cut`'s place (unread since R11d)
    std::array<float,4> offset{kRtShadowRayOffset},dominant{},shadow_colour{},sun{};
    std::array<float,4> plane{};uint32_t water_width{},water_height{};float time{};uint32_t gb2{};   // R6d: FC_GlobalTime; I-0126 R7a: GB2's slot (the GI's albedo)
    std::array<float,4> previous{};   // I-0126 R7b: the camera's position at the cache's last resolve, kRtCacheEntries's bits in w (Bind fills them)
    // I-0126 R7c (R7-PLAN §7): nrd_signal's motion (the velocity target's slot, its rows' scale, the camera's current-to-previous
    // screen rows, row vectors: the velocity resolve's), the composes' denoised radiance (its slot: gi_compose the diffuse's, I-0132 R11
    // spec_compose the specular's), REBLUR's hit distance parameters.
    // I-0126 R7d: the bias check's accumulation (its UAV slot, with kRtReflectGiMean).
    uint32_t velocity{UINT32_MAX},denoised{UINT32_MAX},mean{UINT32_MAX},bounces{1};   // I-0128 R8b: the PT level's path vertices
    std::array<float,4> velocity_scale{};std::array<float,16> current_to_previous{};std::array<float,4> hit_distance{};
    // I-0126 R7d (R7-PLAN §8): ReSTIR GI's, the last ReSTIR frame's eye and the reservoirs' layout (RTXDI's block row pitch, its array
    // pitch, kRtRestir* bits, the surfaces' half): EncodeGiRestir fills them but the bits and the eye.
    std::array<float,4> eye_previous{};std::array<uint32_t,4> restir{};
    std::array<uint32_t,4> probe{};   // I-0129 R9a: the probes' requests' and results' bindless slots, their count (EncodeProbes fills it)
    // I-0132 R11b (R11-PLAN §13): shader execution reordering's scratch targets (RtReflections::MakeSer): the colour's and the hit
    // distance's UAV slots, then their SRV slots; UINT32_MAX: the frame's path programmes run as pixel shaders.
    std::array<uint32_t,4> ser{UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX};
};
static_assert(sizeof(RtReflectConstants)==432);
// I-0126 R7d: ReSTIR GI's bits (rt_reflect.hlsl's RT_RESTIR_*): the temporal and the spatial resampling on, the last frame's history
// this extent's and serial's, this frame's surfaces in the second half.
enum : uint32_t {kRtRestirTemporal=1,kRtRestirSpatial=2,kRtRestirHistory=4,kRtRestirOdd=8,kRtRestirWide=16};   // R7d: Wide, the Jacobian's window [0.1, 10] (rt-gi-jacobian-wide.flag)
// I-0128 R8d (R8-PLAN §2): the temporal neighbour permuted (rt-restir-no-permute.flag off); the last frame's M at most (bits 8 to 15, 0 the
// shader's default 8; rt-restir-history-N.flag) and the share of the pixels shaded from their initial sample (bits 16 to 23, in 255ths;
// 102 unless rt-restir-no-decorrelate.flag): rt_reflect.hlsl's RT_RESTIR_PERMUTE, RT_RESTIR_CAP, RT_RESTIR_SHARE.
enum : uint32_t {kRtRestirPermute=32};
// I-0128 R8d (R8's final review L8): whether a frame's paths go past their first hit (rt_reflect.hlsl's PathVertices() > 1): the
// programmes with PathFrom compiled in then (RT_PATHS 1), the others without it.
inline bool RtPathsOf(const RtReflectConstants& c) noexcept {return (c.mode&kRtReflectPt) && ((c.mode&kRtReflectPathNoCache) || c.bounces>1);}
inline constexpr uint32_t kRtRestirCapShift=8,kRtRestirShareShift=16;
inline constexpr uint32_t RtRestirCapOf(uint32_t stages) noexcept {return (stages>>kRtRestirCapShift)&0xFFu;}
inline constexpr uint32_t RtRestirShareOf(uint32_t stages) noexcept {return (stages>>kRtRestirShareShift)&0xFFu;}
// R6c: RR's specular hit distance guide (R16_FLOAT, the render extent) and the water's reflection target (RGBA16F, half the output).
inline constexpr DXGI_FORMAT kRrHitDistanceFormat=DXGI_FORMAT_R16_FLOAT,kRtWaterFormat=DXGI_FORMAT_R16G16B16A16_FLOAT;
class RtReflections {
public:
    // I-0132 R12d: every kind (Make's 0 to 12) made once through the device's pipeline store at start (rt_scene.cpp), so that a kind's
    // first frame finds it there; `done` counts each kind tried; the number made returned.
    static constexpr int kWarmKinds=13;   // Make's kinds 0 to 12: a new kind raises it (Make's ternary and Describe)
    static constexpr uint32_t kWarmPipelines=uint32_t(kWarmKinds)+1;   // the kinds and SER's state object
    static constexpr int kSerApplyKind=12;   // SER's own kind (its apply draw), warmed with SER only
    // The warm-up's count: every kind and SER's state object with SER, without both SER's (kSerApplyKind and the state object).
    static constexpr uint32_t WarmTotal(bool ser) noexcept {return kWarmPipelines-(ser?0u:2u);}
    // `ser`: SER's state object too (kept for MakeSer); DropWarmed releases one MakeSer never took (before the device closes).
    static uint32_t Warm(ID3D12Device* device,const std::atomic<bool>& cancel,std::atomic<uint32_t>& done,bool ser) noexcept;
    static uint32_t WarmSer(ID3D12Device* device,const std::atomic<bool>& cancel,std::atomic<uint32_t>& done) noexcept;   // SER's, after the rest
    static void DropWarmed() noexcept;
    // One draw on `list` (the full-screen triangle) writing the specular accumulation `rtv` (R11G11B10) where the G-buffer stencil bit
    // 0x08 is set (the read-only depth view `dsv` bound, the stencil reference the caller's 0xFF); the depth and G-buffer readable by
    // pixel shaders and every cube through the bindless heap, which is set on the list (the caller sets its own heap and bindings again
    // after it). `scene` the frame's top level (0: none, the cubes alone), `table` and `rows` its material table and rows, `lights`
    // the frame's list, `regions` the regions' buffer (kRtReflectRegionBytes each), `sets` cbLight's sets (288 bytes each); R6d: `fogs` cbFog's (96 bytes each, the same set index). R6c: with
    // `distance` (a kRrHitDistanceFormat target), the specular hit distance there too. Throws on a device failure. Close only after
    // the queue's drain.
    void Encode(ID3D12Device* device,ID3D12GraphicsCommandList* list,BindlessHeap& heap,const std::function<UploadArena::Range(uint64_t)>& upload,
        D3D12_GPU_VIRTUAL_ADDRESS scene,D3D12_GPU_VIRTUAL_ADDRESS table,D3D12_GPU_VIRTUAL_ADDRESS rows,D3D12_GPU_VIRTUAL_ADDRESS lights,
        D3D12_GPU_VIRTUAL_ADDRESS regions,D3D12_GPU_VIRTUAL_ADDRESS sets,D3D12_GPU_VIRTUAL_ADDRESS fogs,RtReflectConstants constants,D3D12_CPU_DESCRIPTOR_HANDLE rtv,D3D12_CPU_DESCRIPTOR_HANDLE dsv,
        D3D12_CPU_DESCRIPTOR_HANDLE distance={});
    // R6c (R6-PLAN §6): one draw writing the water's reflection target `rtv` (kRtWaterFormat, constants.water_width × water_height: the
    // viewport and scissor set here), the same bindings as Encode's without the G-buffer, depth or stencil (the caller's camera rows,
    // regions, sets and plane in `constants`). Throws on a device failure.
    void EncodeWater(ID3D12Device* device,ID3D12GraphicsCommandList* list,BindlessHeap& heap,const std::function<UploadArena::Range(uint64_t)>& upload,
        D3D12_GPU_VIRTUAL_ADDRESS scene,D3D12_GPU_VIRTUAL_ADDRESS table,D3D12_GPU_VIRTUAL_ADDRESS rows,D3D12_GPU_VIRTUAL_ADDRESS lights,
        D3D12_GPU_VIRTUAL_ADDRESS regions,D3D12_GPU_VIRTUAL_ADDRESS sets,D3D12_GPU_VIRTUAL_ADDRESS fogs,RtReflectConstants constants,D3D12_CPU_DESCRIPTOR_HANDLE rtv);
    // I-0126 R7a (R7-PLAN §1): one draw adding the traced diffuse ambient into `rtv` (GXLightCmbAlbedo's target, R16G16B16A16, its RGB
    // blended ONE + ONE), every pixel with a surface (no depth or stencil bound); the depth, GB0, GB2 (constants.gb2) and GB3 readable
    // by pixel shaders; the same bindings as Encode's. Throws on a device failure.
    void EncodeGi(ID3D12Device* device,ID3D12GraphicsCommandList* list,BindlessHeap& heap,const std::function<UploadArena::Range(uint64_t)>& upload,
        D3D12_GPU_VIRTUAL_ADDRESS scene,D3D12_GPU_VIRTUAL_ADDRESS table,D3D12_GPU_VIRTUAL_ADDRESS rows,D3D12_GPU_VIRTUAL_ADDRESS lights,
        D3D12_GPU_VIRTUAL_ADDRESS regions,D3D12_GPU_VIRTUAL_ADDRESS sets,D3D12_GPU_VIRTUAL_ADDRESS fogs,RtReflectConstants constants,D3D12_CPU_DESCRIPTOR_HANDLE rtv);
    // I-0126 R7c (R7-PLAN §7), I-0132 R11 (D-0252): on a GI frame RR does not denoise, one draw writing NRD's five inputs through `rtvs`
    // (kRtDenoiseInputFormats, written): the same rays as EncodeGi's and Encode's (the GI's and the specular lobe's), over every pixel;
    // the depth, GB0 to GB3 and the velocity target (constants.velocity) readable by pixel shaders. Then, after NRD, EncodeGiCompose
    // adds GB2 × (1 − F0) × the denoised diffuse radiance (constants.denoised) into `rtv` as EncodeGi does, and EncodeSpecCompose
    // writes the pixel's specular term × the denoised specular radiance (constants.denoised) into `rtv` as Encode does (its stencil
    // test, `dsv` bound). All throw on a device failure.
    void EncodeNrdSignal(ID3D12Device* device,ID3D12GraphicsCommandList* list,BindlessHeap& heap,const std::function<UploadArena::Range(uint64_t)>& upload,
        D3D12_GPU_VIRTUAL_ADDRESS scene,D3D12_GPU_VIRTUAL_ADDRESS table,D3D12_GPU_VIRTUAL_ADDRESS rows,D3D12_GPU_VIRTUAL_ADDRESS lights,
        D3D12_GPU_VIRTUAL_ADDRESS regions,D3D12_GPU_VIRTUAL_ADDRESS sets,D3D12_GPU_VIRTUAL_ADDRESS fogs,RtReflectConstants constants,const D3D12_CPU_DESCRIPTOR_HANDLE rtvs[kRtDenoiseInputs]);
    void EncodeSpecCompose(ID3D12Device* device,ID3D12GraphicsCommandList* list,BindlessHeap& heap,const std::function<UploadArena::Range(uint64_t)>& upload,
        D3D12_GPU_VIRTUAL_ADDRESS scene,D3D12_GPU_VIRTUAL_ADDRESS table,D3D12_GPU_VIRTUAL_ADDRESS rows,D3D12_GPU_VIRTUAL_ADDRESS lights,
        D3D12_GPU_VIRTUAL_ADDRESS regions,D3D12_GPU_VIRTUAL_ADDRESS sets,D3D12_GPU_VIRTUAL_ADDRESS fogs,RtReflectConstants constants,D3D12_CPU_DESCRIPTOR_HANDLE rtv,D3D12_CPU_DESCRIPTOR_HANDLE dsv);
    void EncodeGiCompose(ID3D12Device* device,ID3D12GraphicsCommandList* list,BindlessHeap& heap,const std::function<UploadArena::Range(uint64_t)>& upload,
        D3D12_GPU_VIRTUAL_ADDRESS scene,D3D12_GPU_VIRTUAL_ADDRESS table,D3D12_GPU_VIRTUAL_ADDRESS rows,D3D12_GPU_VIRTUAL_ADDRESS lights,
        D3D12_GPU_VIRTUAL_ADDRESS regions,D3D12_GPU_VIRTUAL_ADDRESS sets,D3D12_GPU_VIRTUAL_ADDRESS fogs,RtReflectConstants constants,D3D12_CPU_DESCRIPTOR_HANDLE rtv);
    // I-0126 R7b (R7-PLAN §3): the radiance cache's update and resolve for this frame, before the frame's passes that query it (the
    // reflections, the water, the GI: their constants then carry kRtReflectCache): its buffers made on the first call (zeroed by a
    // first resolve with kRtReflectCacheClear), one path a kRtCacheBlock block of pixels (a draw over the reduced grid, no target: the
    // viewport and scissor set here, the caller's set again after it), a UAV barrier, the resolve (a dispatch of every entry), a UAV
    // barrier. The depth, GB0 and GB3 readable by pixel shaders; the bindings Encode's (constants.gb0, gb3 and the camera rows).
    // The buffers stay in UNORDERED_ACCESS (promoted from COMMON, a buffer's), out of the frame graph (only UAV barriers between
    // their passes). Throws on a device failure.
    void EncodeCache(ID3D12Device* device,ID3D12GraphicsCommandList* list,BindlessHeap& heap,const std::function<UploadArena::Range(uint64_t)>& upload,
        D3D12_GPU_VIRTUAL_ADDRESS scene,D3D12_GPU_VIRTUAL_ADDRESS table,D3D12_GPU_VIRTUAL_ADDRESS rows,D3D12_GPU_VIRTUAL_ADDRESS lights,
        D3D12_GPU_VIRTUAL_ADDRESS regions,D3D12_GPU_VIRTUAL_ADDRESS sets,D3D12_GPU_VIRTUAL_ADDRESS fogs,RtReflectConstants constants);
    // I-0126 R7d (R7-PLAN §8): ReSTIR GI on a PT GI frame (rt_reflect.hlsl's gi_temporal, gi_spatial, gi_shade), in place of EncodeGi's
    // one draw: its reservoirs (two arrays, RTXDI's block-linear layout) and surfaces (two halves) at constants' extent, made by
    // MakeRestir (true when made: no history yet), then the three draws with UAV barriers between them, the last adding into `rtv` as
    // EncodeGi. The caller holds the buffers (HoldRestir) on every frame that uses them. Throws on a device failure.
    bool MakeRestir(ID3D12Device* device,uint32_t width,uint32_t height,bool initial=false);   // I-0128 R8d: `initial`, the surfaces' third part (a decorrelated share)
    void HoldRestir(std::vector<std::shared_ptr<const void>>& kept) const;
    void ReleaseRestir() noexcept {for(auto& buffer:restir_)buffer.Reset();restir_width_=restir_height_=0;restir_initial_=false;}   // the frames that used them hold them
    void EncodeGiRestir(ID3D12Device* device,ID3D12GraphicsCommandList* list,BindlessHeap& heap,const std::function<UploadArena::Range(uint64_t)>& upload,
        D3D12_GPU_VIRTUAL_ADDRESS scene,D3D12_GPU_VIRTUAL_ADDRESS table,D3D12_GPU_VIRTUAL_ADDRESS rows,D3D12_GPU_VIRTUAL_ADDRESS lights,
        D3D12_GPU_VIRTUAL_ADDRESS regions,D3D12_GPU_VIRTUAL_ADDRESS sets,D3D12_GPU_VIRTUAL_ADDRESS fogs,RtReflectConstants constants,D3D12_CPU_DESCRIPTOR_HANDLE rtv);
    // I-0129 R9a (R9-PLAN §2): the traced probes (rt_reflect.hlsl's `probe`), one group a request: `requests` (at most kRtProbesMost)
    // written into this frame's upload and read through the frame slot `slot`'s view, their results into that slot's buffer (UAV,
    // promoted from COMMON) and copied into its readback, returned: the caller reads it once the slot's work completed (its
    // completions), kRtProbeResult's 64 bytes a request in their order. `completed` the queue's completed value (the heap's slots,
    // made once a slot). The bindings Encode's (the camera rows and the frame's mode in `constants`). Throws on a device failure.
    ID3D12Resource* EncodeProbes(ID3D12Device* device,ID3D12GraphicsCommandList* list,BindlessHeap& heap,const std::function<UploadArena::Range(uint64_t)>& upload,
        D3D12_GPU_VIRTUAL_ADDRESS scene,D3D12_GPU_VIRTUAL_ADDRESS table,D3D12_GPU_VIRTUAL_ADDRESS rows,D3D12_GPU_VIRTUAL_ADDRESS lights,
        D3D12_GPU_VIRTUAL_ADDRESS regions,D3D12_GPU_VIRTUAL_ADDRESS sets,D3D12_GPU_VIRTUAL_ADDRESS fogs,RtReflectConstants constants,
        std::span<const RtProbeRequest> requests,uint32_t slot,uint64_t completed);
    // The step review's M3: the probes' heap slots back to `heap` after `ticket` (the renderer's close, before Close), unset.
    void ReleaseProbeViews(BindlessHeap& heap,uint64_t ticket) noexcept {
        for(auto& p:probe_slots_)for(auto* view:{&p.requests_view,&p.results_view}){if(*view!=UINT32_MAX){try{heap.Release(*view,ticket);}catch(...){}}*view=UINT32_MAX;}}
    // I-0132 R11b (R11-PLAN §13): shader execution reordering on the PT level's paths. MakeSer makes, once, the ray generation state
    // object (rt_reflect.hlsl's trace_hit_ser and gi_temporal_ser, lib_6_9, the reflections' root as its global root) and its shader
    // table, and at this extent the two scratch targets (R11G11B10 colour, R16F hit distance, in UNORDERED_ACCESS) with their four heap
    // slots (UAV, UAV, SRV, SRV; a new extent's in new slots, the former ones released after `submitted`, their targets held by the
    // frames that used them); throws when the device cannot (the caller turns SER off: the pixel shaders then, the same picture).
    // With constants.ser set (SerSlots), Encode's and EncodeGiRestir's path programmes dispatch the reordered builds in their place:
    // Encode's into the scratch targets, then a draw of trace_hit's pipeline state (ser_apply) copies them where trace_hit writes.
    void MakeSer(ID3D12Device* device,BindlessHeap& heap,uint32_t width,uint32_t height,uint64_t completed,uint64_t submitted);
    std::array<uint32_t,4> SerSlots() const noexcept {return ser_slots_;}
    uint64_t SerDispatches() const noexcept {return ser_dispatches_.load(std::memory_order_relaxed);}
    bool SerMade() const noexcept {return ser_state_!=nullptr;}   // the render thread's
    // The render thread's: an Encode or EncodeGiRestir that could not prepare its SER dispatch (it drew with the pixel shader), once.
    std::string TakeSerFailure() {std::string taken;taken.swap(ser_failure_);return taken;}
    void HoldSer(std::vector<std::shared_ptr<const void>>& kept) const;
    void ReleaseSerViews(BindlessHeap& heap,uint64_t ticket) noexcept {for(auto& slot:ser_slots_){if(slot!=UINT32_MAX){try{heap.Release(slot,ticket);}catch(...){}}slot=UINT32_MAX;}}
    void Close() noexcept {ser_state_.Reset();ser_table_.Reset();ser_apply_.Reset();for(auto& target:ser_targets_)target.Reset();ser_width_=ser_height_=0;ser_slots_={UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX};root_.Reset();pipeline_.Reset();hit_.Reset();water_.Reset();gi_.Reset();signal_.Reset();compose_.Reset();update_.Reset();resolve_.Reset();probe_.Reset();for(auto& p:probe_slots_){p.results.Reset();p.readback.Reset();p.requests_view=p.results_view=UINT32_MAX;}
        temporal_.Reset();spatial_.Reset();shade_.Reset();hit_path_.Reset();gi_path_.Reset();spec_compose_.Reset();for(auto& buffer:restir_)buffer.Reset();restir_width_=restir_height_=0;restir_initial_=false;
        for(auto& buffer:cache_)buffer.Reset();cache_none_.Reset();cache_cleared_=false;}
private:
    void Bind(ID3D12GraphicsCommandList* list,BindlessHeap& heap,const std::function<UploadArena::Range(uint64_t)>& upload,D3D12_GPU_VIRTUAL_ADDRESS scene,
        D3D12_GPU_VIRTUAL_ADDRESS table,D3D12_GPU_VIRTUAL_ADDRESS rows,D3D12_GPU_VIRTUAL_ADDRESS lights,D3D12_GPU_VIRTUAL_ADDRESS regions,D3D12_GPU_VIRTUAL_ADDRESS sets,D3D12_GPU_VIRTUAL_ADDRESS fogs,
        RtReflectConstants constants,bool compute=false);
    ID3D12PipelineState* Make(ID3D12Device* device,int kind);
    static D3D12_GRAPHICS_PIPELINE_STATE_DESC Describe(int kind,ID3D12RootSignature* root) noexcept;   // I-0132 R12d: Make's and Warm's
    static Microsoft::WRL::ComPtr<ID3D12StateObject> CreateSerState(ID3D12Device* device,ID3D12RootSignature* root);   // MakeSer's and Warm's
    void MakeRoot(ID3D12Device* device);void MakeCache(ID3D12Device* device);
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_;Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_,hit_,water_,gi_,signal_,compose_,update_,resolve_;   // R7c: signal_, compose_
    Microsoft::WRL::ComPtr<ID3D12PipelineState> temporal_,spatial_,shade_;   // I-0126 R7d: ReSTIR GI's draws
    Microsoft::WRL::ComPtr<ID3D12PipelineState> hit_path_,gi_path_;   // I-0128 R8d: trace_hit and gi with PathFrom (RT_PATHS 1)
    Microsoft::WRL::ComPtr<ID3D12PipelineState> spec_compose_;   // I-0132 R11: NRD's specular written into SpecularAcc
    // I-0132 R11b: SER's state object, its shader table (two raygen records, 64 bytes apart: trace_hit_ser, gi_temporal_ser), the
    // apply draw's pipeline (kind 12: trace_hit's states, ser_apply), the scratch targets (colour, distance) at their extent, their slots.
    Microsoft::WRL::ComPtr<ID3D12StateObject> ser_state_;Microsoft::WRL::ComPtr<ID3D12Resource> ser_table_;Microsoft::WRL::ComPtr<ID3D12PipelineState> ser_apply_;
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,2> ser_targets_;uint32_t ser_width_{},ser_height_{};std::array<uint32_t,4> ser_slots_{UINT32_MAX,UINT32_MAX,UINT32_MAX,UINT32_MAX};
    void DispatchSer(ID3D12GraphicsCommandList4* list4,uint32_t record,uint32_t width,uint32_t height);
    std::atomic<uint64_t> ser_dispatches_{};std::string ser_failure_;   // the step review's L2 (the dispatches, read by the game thread's row) and M1
    void SerBarrier(ID3D12GraphicsCommandList* list,bool to_read);
    // I-0129 R9a: the probes' programme and, a frame slot, its results (kRtProbesMost x 64 bytes, UAV-capable, COMMON) with their
    // readback and the two heap slots (the requests' raw view, written each frame; the results' UAV, written once).
    Microsoft::WRL::ComPtr<ID3D12PipelineState> probe_;
    struct ProbeSlot {Microsoft::WRL::ComPtr<ID3D12Resource> results,readback;uint32_t requests_view{UINT32_MAX},results_view{UINT32_MAX};};
    std::array<ProbeSlot,max_frame_slots> probe_slots_;
    // R7d: ReSTIR GI's reservoirs (RTXDI_PackedGIReservoir, 32 bytes, two arrays) and surfaces (16 bytes, two halves) at their extent,
    // UAV-capable, in COMMON (promoted by their first use in a list, as the cache's); bound at u3 and u4 (the stand-by buffer before).
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,2> restir_;uint32_t restir_width_{},restir_height_{},restir_row_pitch_{},restir_array_pitch_{};
    bool restir_initial_{};   // I-0128 R8d: the surfaces buffer holds the initial estimates' third part
    // R7b: the cache's hash entries (8 bytes an entry), its frame's accumulation and its resolved cells (16 bytes each); bound to
    // every pass of the root (u0 to u2), the queries reading only where the constants carry kRtReflectCache.
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,3> cache_;bool cache_cleared_{};std::array<float,4> previous_{};bool previous_set_{};
    // R7b's step review L2: until the cache is made (its first GI frame), u0 to u2 bound to this small buffer (never read: the
    // queries read only with kRtReflectCache), so the reflections and the water never depend on the cache's allocation.
    Microsoft::WRL::ComPtr<ID3D12Resource> cache_none_;
};

// I-0123 RR-a (RR-PLAN §1): DLSS Ray Reconstruction's guides from the G-buffer (rr_guides.hlsl). Its constants (rr_guides.hlsl's
// Guides): the camera as RtViewConstants' first rows, the extent, the bindless slots of the depth (R32 of the D32S8) and GB0 to GB3.
struct RrGuidesConstants {
    std::array<float,4> right{},up{},forward{},position{},jitter{};
    uint32_t width{},height{},depth{},gb0{},gb1{},gb2{},gb3{},pad{};
};
static_assert(sizeof(RrGuidesConstants)==112);
// The guides' formats, in the order of the pass's targets: the diffuse albedo, the specular albedo, the normal with the roughness.
inline constexpr std::array<DXGI_FORMAT,3> kRrGuideFormats{DXGI_FORMAT_R11G11B10_FLOAT,DXGI_FORMAT_R11G11B10_FLOAT,DXGI_FORMAT_R16G16B16A16_FLOAT};
class RrGuides {
public:
    // One draw on `list` (the full-screen triangle) writing the three guides through `rtvs` (render targets in kRrGuideFormats); the
    // depth and GB0 to GB3 readable by pixel shaders through the bindless heap, which is set on the list (the caller sets its own
    // heap, root signature and bindings again after it); the viewport and scissor the caller's. Throws on a device failure. Close only
    // after the queue's drain.
    void Encode(ID3D12Device* device,ID3D12GraphicsCommandList* list,BindlessHeap& heap,const std::function<UploadArena::Range(uint64_t)>& upload,
        const RrGuidesConstants& constants,const D3D12_CPU_DESCRIPTOR_HANDLE rtvs[3]);
    void Close() noexcept {root_.Reset();pipeline_.Reset();}
private:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_;Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_;
};

}
