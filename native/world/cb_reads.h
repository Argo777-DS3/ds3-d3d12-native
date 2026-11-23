#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

// I-0042: which constant-buffer components an original SM5.0 programme reads,
// from its D3DDisassemble listing. A component counts as read when it appears in
// a source swizzle at a position the instruction consumes (destination write
// mask positions, or the first 2/3/4 components of dp2/dp3/dp4). Dynamically
// indexed reads (cbN[rX.y + k]) are recorded separately with their base k.
namespace ds3reads {
struct Reads {
    std::map<unsigned,std::vector<uint8_t>> fixed;   // register -> per-float4 4-bit component mask
    std::map<unsigned,std::vector<uint8_t>> dynamic; // register -> per-base-index mask of dynamic reads
    std::map<unsigned,unsigned> stride;              // register -> element stride of its index (imul ..., l(N)), when known
    bool ok{};
};
// Direct preparation: conservative SM5.0 CB reads, independent of the historical
// host mask policy. Every mentioned swizzle component counts, including the
// first operand of conditionals. Dynamic indexing covers the whole buffer via
// dynamic[reg][0]=0xF with no inferred stride. Registers0..13 and static float4
// indices0..4095 are accepted; unavailable/ambiguous inputs return ok=false.
Reads AnalyzeConservative(const std::vector<uint8_t>& dxbc);
// Byte mask (one entry per 4-byte float) for a buffer of `bytes`; dynamic reads mark the
// whole tail from their base (the programme may index any later element).
std::vector<uint8_t> FloatMask(const Reads& r,unsigned reg,uint32_t bytes,bool* dynamic=nullptr);
}
