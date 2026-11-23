#pragma once
#include "primitive_programs.h"

namespace ds3native::frame {
struct PrimitiveFloat4Values {
    SceneShaderStage stage{};
    std::string variable;
    std::vector<std::array<float,4>> values;
};
struct PrimitiveDrawConstants {
    uint64_t draw_identity{};
    std::vector<PrimitiveFloat4Values> variables;
};
enum class PrimitiveConstantStatus : uint8_t { Ready,Missing,Unsupported,Stale,Invalid };
class PrimitiveUniformSource {
public:
    // Begin clears all prior CPU values/coverage. No previous-frame defaults.
    PrimitiveConstantStatus Begin(const PrimitiveProgramPair& pair,uint64_t serial,uint64_t draw_identity);
    // Actual CPU semantic values from the current mesh/fill algorithms. Implements
    //1F78AD0 semantic remap and1F78BC0 shared CPU float storage; no GPU setter stream.
    PrimitiveConstantStatus Write(uint32_t semantic,std::span<const float> values,uint32_t element_offset=0,uint32_t instance=0);
    // Qualified CPU uniform callback AFTER the original semantic remap.
    PrimitiveConstantStatus WriteUniform(uint32_t uniform,std::span<const float> values,uint64_t element_offset=0);
    // I-0067 SCALEFORM-FILTERS-NATIVE: the filter HAL writes float2 values into float4 rows (texscale, offset,
    // srctexscale) and instance 0 only of the batched VS 507 row; the components and instances it never writes are
    // never read and hold zero. Rows with no written value in their first instance stay missing.
    void CompleteWrittenRows();
    PrimitiveConstantStatus Resolve(PrimitiveProgramVersion current,uint64_t serial,PrimitiveDrawConstants& output) const;
private:
    PrimitiveProgramPair pair_;
    uint64_t serial_{},draw_identity_{};
    std::array<float,1000> shadow_{};
    std::array<bool,1000> written_{};
};
// Exact shader-declared float4 variables/arrays, complete for both stages.
// Native storage and packing by original RDEF offsets, not an old setter-call
// stream. Unknown source-value meaning is not inferred from variable names.
// All required values must be supplied; gaps/non-float4 layouts refuse rather
// than introducing padding/default or retained source state. Previous output
// stays untouched on any refusal. GPU/full-frame readiness remains separate.
PrimitiveConstantStatus PackPrimitiveConstants(const PrimitiveProgramPair& pair,const PrimitiveDrawConstants& values,std::vector<SceneConstantBinding>& output);
}
