#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <array>
#include <optional>
#include <span>

// I-0042: engine-facing native producers of the constant blocks DS3's original
// GBuffer/lighting programmes read. Each producer is a pure function from engine
// CPU state (taken at the I-0039 seam) to bytes at the RDEF offsets of the
// original constant buffer, and reports which byte ranges it owns so that a
// checker or a renderer can combine it with other producers.
namespace ds3prod {
struct Field { const char* name; uint32_t offset, bytes; };

// ---- Camera family of cbSceneParam (960 bytes, VS/PS b8) --------------------
// Input: the CED670 camera output (K-0395): row-major 4x4 pose (rows right, up,
// forward, position; row-vector), vertical FOV/near/far at [16..18], viewport
// width/height as integers at [19]/[20], aspect at [21].
// Matrices are stored row by row for column vectors (clip = M * v), the layout
// observed in the oracle bytes; VC_MatrixViewProj is camera-relative (the
// programmes project position - SC_CameraPos).
inline constexpr Field scene_camera_fields[]={
    {"FC_ScreenSize",16,16},{"FC_DepthComputeParam",32,16},{"SC_CameraPos",48,16},{"VC_ClipPlane",64,16},
    {"FC_MatrixView",80,48},{"FC_MatrixInvViewProj",128,64},{"FC_MatrixInvProj",192,64},{"FC_MatrixProj",256,64},
    {"FC_MatrixInvView",320,64},{"VC_MatrixViewProj",384,64},{"FC_FrustumNear",880,4},
};
// jitter: I-0111 (R1), the temporal upscaler's sub-pixel offset in render pixels (+x right, +y down); zero keeps the engine's
// -0.0 centre words.
struct Camera { float right[3],up[3],forward[3],position[3],fovy,near_z,far_z,aspect; uint32_t width,height; float jitter[2]{}; };
inline Camera ReadCamera(const float c[24]) {
    Camera k{};
    for(int i=0;i<3;++i) { k.right[i]=c[i]; k.up[i]=c[4+i]; k.forward[i]=c[8+i]; k.position[i]=c[12+i]; }
    k.fovy=c[16]; k.near_z=c[17]; k.far_z=c[18]; std::memcpy(&k.width,&c[19],4); std::memcpy(&k.height,&c[20],4); k.aspect=c[21];
    return k;
}
void ProduceSceneCamera(const Camera& k,uint8_t out[960]);
// I-0063 (D-0151): FC_ScreenSize is the size the frame renders at (its GBuffer), not the camera's viewport: at 1680 x 945
// the camera says 945 and the game's block 948, its render height. Set per frame from the frame's resources; 0 = the camera's.
void UseRenderSize(uint32_t width,uint32_t height);
// 140004BCA0: the engine 4x4 inverse (row-major in, row-major out), same operation order.
void InverseMatrix(const float* m,float* r);

// ---- cbInstanceData record (0x110 bytes per instance, VS/PS b4) -------------
// The function of 5B3D0 (decompiled/DS3-1152-F-0005B3D0.c) from the engine state it
// reads. Views point at engine bytes (offsets relative to the named object); absent
// objects are null. The two light parameter sets are the engine lookup results for
// the group's keys (+0xF8, +0x100), viewed from their +0x290.
constexpr uint32_t instance_record_bytes=0x110;
// Direct native producer contract. Values are owned and express the inputs
// 5B3D0 uses; no fabricated 0x130-byte legacy instance or pointer graph.
// Missing optional inputs leave their output words unproduced.
struct InstanceLightingInputs {
    std::array<std::array<float,12>,2> bank_rows{};
    std::array<float,2> bank_strength{};
    std::array<float,12> animation_rows{},override_rows{};
    std::array<float,3> row_scale{};
    float blend{},animation_strength{};
    bool banks_present{},animation_owned{},render_override{};
    // I-0105 (E03): 59470's branch for a draw state that carries its own light (args+0x424, set by 29ADF0 from the FXR): the rows
    // animation_rows, the blend vector (scale - 1) * strength + 1. It precedes the render override and the banks.
    bool state_owned{};std::array<float,4> state_scale{};float state_strength{};
};
struct InstanceClipInputs {
    bool enabled{};
    std::array<float,16> context{};
    std::array<float,4> vector{};
};
struct InstanceRecordInputs {
    std::optional<std::array<float,12>> object_rows;
    uint32_t palette_base{};
    std::optional<std::array<float,4>> animation_colour,animation_auxiliary;
    std::optional<InstanceLightingInputs> lighting;
    std::optional<InstanceClipInputs> clip;
    std::array<float,4> group_colour{};
    std::array<float,3> origin{},bounds_min{},bounds_max{};
    float origin_w{},bounds_min_w{},bounds_max_w{},opacity{};
    std::optional<uint32_t> type_value;
    bool bounds_suppressed{},instance_flag{};
};
void ProduceInstanceRecord(const InstanceRecordInputs& input,uint8_t out[instance_record_bytes],
                          uint8_t written[instance_record_bytes/4]);
struct InstanceState {
    const uint8_t* instance;       // instance record, >= 0x130 bytes
    const uint8_t* group;          // [instance+0x100], >= 0x170 bytes
    const uint8_t* anim;           // [instance+0x68] viewed from +0x80 (>= 0x100 bytes), or null
    const uint8_t* light[2];       // parameter sets viewed from +0x290 (>= 0x40 bytes), or null
    const uint8_t* ctx90;          // draw context +0x90 .. +0xD0
    const uint8_t* ctx110;         // draw context +0x110 .. +0x11C
    const uint8_t* ctx1290;        // draw context +0x1290 .. +0x12A0 (cbModelParam)
    const uint8_t* anim_tail;      // [instance+0x68] + 0x620 .. +0x640, or null (cbModelParam)
    const uint8_t* render190;      // [[0x144961B50]+0x4DD0] + 0x190 .. +0x1C0
    const uint8_t* default_matrix; // .data 0x14469C9E0 (0x40 bytes)
    const uint8_t* default_light;  // .data 0x144929B20 (0x10 bytes)
    uint32_t index;                // instance index in the draw call
    int32_t params_type;           // *drawParams
    int32_t state_type;            // draw state +0x1C
    uint32_t type_value;           // engine lookup result for state types 6/7
    bool render_override;          // [[0x144961B50]+0x4DD0] + 0x13E8
    bool palette_destination;      // the caller also requested the matrix palette
    bool object_matrix;            // [instance+8] gate set and its table entry non-null
    const uint8_t* object_matrix_bytes; // that 3x4 matrix (48 bytes) when object_matrix
    uint32_t object_bones;         // [[instance+8]+0x70]
};
// Retained diagnostic entry for genuine historical byte views. Production
// calls the owned value contract above. Sets written[i] for each owned word.
void ProduceInstanceRecord(const InstanceState& s,uint8_t out[instance_record_bytes],uint8_t written[instance_record_bytes/4]);
}
namespace ds3prod {
// ---- Cloth palette (VS b5 of the pass-7 cloth programmes) ---------------------
// 16050 uploads, for a gated object given a second bone array Q, rows A_i * inverse(Q_i)
// (affine 3x4 rows, implicit last row 0 0 0 1), with the engine's 3x4 inverse (179C0).
void ProduceClothPalette(const float (*bones)[12],const float (*reference)[12],uint32_t count,float (*out)[12]);
}
namespace ds3prod {
// ---- cbMtdParam (PS b3) from the original material definition (MTD) ------------
// Parameters of the material's .mtd (from allmaterialbnd.mtdbnd), by name. The block
// layout is the programme's own RDEF; the producer writes each variable it knows by name.
struct MtdParam { std::string name; std::vector<double> values; };
// 193AED0's bool[1..4], int[1..4], float[1..4] type table. Owned scalar
// words from the genuine CPU getters; invalid input does not publish output.
bool DecodeMtdNumericParam(std::string name,uint32_t type,std::span<const uint32_t> words,MtdParam& output);
struct ConstantVariable { std::string name; uint32_t offset, bytes; };
// Writes the known variables into `out` (block bytes) and marks their words in `written`.
// Returns the names of RDEF variables it does not produce.
std::vector<std::string> ProduceMtdParams(const std::vector<MtdParam>& mtd,const std::vector<ConstantVariable>& layout,uint8_t* out,uint32_t bytes,uint8_t* written);
}
namespace ds3prod {
// ---- cbModelParam (VS b7): the second block 5B3D0 uploads (0x4F0 bytes); its content is built in the engine's model frame inputs.
constexpr uint32_t model_param_bytes=0x4F0;
}
namespace ds3prod {
// ---- cbTexHdrScaleParam (PS b0): the optional float metadata of the TPF entry of the material's lightmap
// (FLVER texture g_DOLTexture1, e.g. 40000000_m000063_gi_0000_00_dol_00), written at FC_TexHdrScaleParam.
void ProduceTexHdrScale(const std::vector<float>& floats,uint32_t offset,uint8_t* out,uint32_t bytes,uint8_t* written);
// ---- cbClothCancelObjMatrix (VS b12): 16050 uploads, for a gated object given a second bone array Q, rows
// Q_i x G_i (17CE0, G = the [gate+0x1D0] rows, SSE order), else identity rows (.data 0x14469C9B0, 0x1684A).
void ProduceClothCancel(const float (*reference)[12],const float (*gate_rows)[12],uint32_t count,float (*out)[12]);
void ProduceIdentityRows(uint32_t count,float (*out)[12]);
}
namespace ds3prod {
// ---- I-0043: families of the draw state (the 0x4E0-byte object 1A86C0 uploads from) ---------------------
constexpr uint32_t draw_state_bytes=0x4E0;
// cbTexScroll (VS b11): the 64 bytes at draw state +0x240 (the first bytes of the draw call's 400-byte third argument).
void ProduceTexScroll(const uint8_t* state,uint8_t* out,uint32_t bytes,uint8_t* written);
// PS cbModelParam (b7), the block 1A86C0 assembles: +0x00 state +0x310 (16 B), +0x10 state +0x37C, +0x20 1.0 when state
// +0x200 bit 20 (5A120: [params+0x1A9]) else 0 (FC_DitherEnable), +0x30 (1, 0.5, 2, 0.5), +0x44 0, +0x48 1, +0x50
// (8192, 0.2, 2048, 32), +0x60 state +0x280 (16 B, FC_SkinMatchColor). The lookups (+0x14, +0x18, +0x40) are not produced.
// I-0064: with `default_row` (.data 0x144929B20, 16 bytes) also the parallax-corrected cubemap boxes (+0x80..+0x1A7).
void ProduceModelParamPs(const uint8_t* state,uint8_t* out,uint32_t bytes,uint8_t* written,const uint8_t* default_row=nullptr);
// ---- cbPerFrame (PS b9), the part 23790 builds from the clock and two environment bank rows -------------------
// +0xB0 FC_GlobalTime = [G+0x4E34]; +0xC0..+0xD8 the first seven of the eleven floats 1368D0 blends from the rows
// (row_a when |t| <= FLT_EPSILON, row_b when |t - 1| <= FLT_EPSILON, else (b - a) * t + a); +0xDC 1.0.
void ProducePerFrame(float time,const float row_a[11],const float row_b[11],float t,uint8_t* out,uint32_t bytes,uint8_t* written);
// I-0113 tranche 33 (gap 16): the effect pass context's float4 +0x110, which 264230 multiplies into the three SH rows' .w words
// (+0x110, +0x114, +0x118 for rows 0 to 2). The world render task's constructor 140204C30 blends the same two Dominant Light
// records as 1368D0 (ProducePerFrame's rule) and stores (f8 f7, f9 f7, f10 f7, 1) (READ in its disassembly); without an
// environment the base pass context's constructor 140203D00 leaves (1, 1, 1, 1).
void ProducePassDominantScale(const float row_a[11],const float row_b[11],float t,float out[4]);
// I-0105 (the surface snow): cbPerFrame +0x60..+0xAF, the rows 23790 builds from 131780's blend of the two `Snow` records
// (ds3light::Banks::SnowAt's order): with |t| <= FLT_EPSILON record a, with |t - 1| <= FLT_EPSILON record b (131590's copy), else
// each field (b - a) * t + a and the two angles through 1316D0 (elevation in [-pi/2, pi/2], azimuth in [-pi, pi]); the direction
// always from the angles 131780 holds (a's when it copies). FC_SnowDir +0x60 (cos e sin z, -sin e, cos e cos z, 0), FC_SnowTiling
// Scale +0x70 (s, (1 - s) / 2), FC_SnowAlbedo +0x80, +0x90 FC_SnowReflectance, FC_GroundPlaneHeight, FC_RcpSnowHeightRange
// 1 / (plane - ground), FC_SnowNormalScale, FC_SnowSpakleTinlingScale +0xA0 (s, (1 - s) / 2). Returns the blended Scene Snow Rate
// (23790 stores it at [root+0x4E70]; 1A86C0 scales the model's Snow Model rate by it).
float ProduceSnow(const float a[16],const float b[16],float t,uint8_t* out,uint32_t bytes,uint8_t* written);
// cbModelParam +0x40 FC_SnowRate (1A86C0): ((b - a) * t + a) * scene, a and b the Snow Model rates of the draw state's pair.
void ProduceSnowRate(float a,float b,float t,float scene,uint8_t* out,uint32_t bytes,uint8_t* written);
// The independently known clock range; no environment-bank defaults.
void ProduceFrameTime(float time,uint8_t* out,uint32_t bytes,uint8_t* written);
void ProduceDitherAlpha(uint8_t* out,uint32_t bytes,uint8_t* written);
// Known 22E930 scalar words. Provider/colour resolution belongs to the caller.
void ProduceMatDynScalars(uint32_t byte_offset,const uint32_t* words,uint32_t count,
    uint8_t* out,uint32_t bytes,uint8_t* written);
//22E930's wet calculation, reusable with current typed engine inputs. This
//entry writes only230..26F; activation and current input availability belong
//to the caller. No legacy object, creation zero-fill or generated values.
struct MatDynWet {
    std::array<float,4> diffuse{},reflectance{};
    float a{},b{},c{};
    uint32_t type{};
    float scale{};
};
void ProduceMatDynWet(const MatDynWet& input,uint8_t* out,uint32_t bytes,uint8_t* written);
}
namespace ds3prod {
// ---- I-0064: cbMatrixWVP (VS b0) of a view: VC_View_MatrixCamera = the view's pose (row-major, row-vector: rows right, up,
// forward, position), VC_View_MatrixViewProj = its camera-relative view-projection (the inverse of the pose's rotation times the
// projection `proj`, both row-major, row-vector), stored transposed as cbSceneParam's VC_MatrixViewProj (K-0414). Writes 128 bytes.
void ProduceViewMatrices(const float pose[16],const float proj[16],uint8_t out[128]);
}
