#pragma once
#include "primitive_work.h"
#include <map>
#include <span>

namespace ds3native::frame {
struct PrimitiveProgramVersion {
    uint64_t incarnation{},generation{};
    bool operator==(const PrimitiveProgramVersion&) const = default;
};
enum class PrimitiveProgramStatus : uint8_t { Ready,Missing,UnknownSelector,UnsupportedMode,Stale,Invalid };
struct PrimitiveUniformLayout {
    // Exact installed CPU records:25 ten-byte uniform rows and25 semantic triples.
    // These contain no GPU wrapper, reflection upload or runtime address.
    bool installed{};
    std::array<uint8_t,250> uniforms{};
    std::array<int8_t,75> semantics{};
};
struct PrimitiveVertexLayout {
    bool installed{},supported{};
    uint32_t stride{};
    std::vector<SceneVertexElement> elements;
};
struct PrimitiveProgramPair {
    PrimitiveProgramVersion version;
    uint32_t selector{},record{},source_mode{};
    PrimitiveProgram vertex,pixel;
    PrimitiveUniformLayout vertex_uniforms,pixel_uniforms;
    PrimitiveVertexLayout vertex_layout;
};
class PrimitiveProgramBank {
public:
    PrimitiveProgramVersion Version() const {return version_;}
    size_t ProgramCount() const {return programs_.size();}
private:
    PrimitiveProgramVersion version_;
    std::map<uint64_t,PrimitiveProgram> programs_;
    std::map<uint64_t,PrimitiveUniformLayout> uniforms_;
    std::map<uint64_t,PrimitiveVertexLayout> vertex_layouts_;
    friend PrimitiveProgramStatus DecodePrimitiveProgramBank(std::span<const uint8_t>,PrimitiveProgramVersion,PrimitiveProgramBank&);
    friend PrimitiveProgramStatus DecodeOriginalPrimitiveProgramBank(std::span<const uint8_t>,PrimitiveProgramVersion,PrimitiveProgramBank&);
    friend PrimitiveProgramStatus SelectPrimitivePrograms(const PrimitiveProgramBank&,PrimitiveProgramVersion,uint32_t,uint32_t,PrimitiveProgramPair&);
};
// PPBK0001 is an owned CPU container. Only the qualified original-build mode3
// catalogue is supported. No original object, factory, GPU pointer or API call
// stream enters this interface. Exact original programme hashes are required.
// A bank may lack programmes; selection refuses instead of substituting them.
// Version is the current owner's token, never inferred from a source pointer.
PrimitiveProgramStatus DecodePrimitiveProgramBank(std::span<const uint8_t> bytes,PrimitiveProgramVersion current,PrimitiveProgramBank& output);
// Genuine installed original-build CPU data, not a captured programme bank.
// Reads only the static descriptor +20 DXBC pointers and file-backed payloads;
// verifies the whole PE and reuses the existing complete programme validator.
// Publication is atomic; this supplies no GPU objects or pipeline readiness.
PrimitiveProgramStatus DecodeOriginalPrimitiveProgramBank(std::span<const uint8_t> pe,PrimitiveProgramVersion current,PrimitiveProgramBank& output);
PrimitiveProgramStatus SelectPrimitivePrograms(const PrimitiveProgramBank& bank,PrimitiveProgramVersion current,uint32_t source_mode,uint32_t selector,PrimitiveProgramPair& output);
}
