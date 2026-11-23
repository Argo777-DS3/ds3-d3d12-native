#pragma once
#include "primitive_constants.h"
#include <memory>
#include <mutex>
#include <variant>

namespace ds3native::frame {
constexpr size_t PrimitiveTextureLimit=1024;   // the images one UI texture set holds (ValidImages, Commit)
struct PrimitiveTextureVersion {
    uint64_t incarnation{},generation{};
    bool operator==(const PrimitiveTextureVersion&) const=default;
};
struct PrimitiveTextureSnapshot {
    PrimitiveTextureVersion version;
    std::vector<PrimitiveTexture> textures;
};
struct PrimitiveTextureReplace {PrimitiveTexture texture;};
struct PrimitiveTextureRemove {uint64_t identity{};};
struct PrimitiveTextureWrite {
    uint64_t identity{};
    uint32_t mip{},x{},y{},width{},height{},row_pitch{};
    std::vector<uint8_t> bytes;
};
using PrimitiveTextureEdit=std::variant<PrimitiveTextureReplace,PrimitiveTextureRemove,PrimitiveTextureWrite>;
enum class PrimitiveTextureStatus : uint8_t {Ready,Missing,Stale,Unsupported,Invalid};

// Own native CPU identities/versions and complete images. Caller transactions
// must be stable while copied. No legacy object address, setter stream, factory,
// view or GPU handle is admitted; installed engine delivery remains separate.
class PrimitiveTextureStore {
public:
    using Lease=std::shared_ptr<const PrimitiveTextureSnapshot>;
    PrimitiveTextureStatus Define(PrimitiveTextureVersion version,const std::vector<PrimitiveTexture>& images);
    PrimitiveTextureStatus Commit(PrimitiveTextureVersion expected,uint64_t next_generation,std::span<const PrimitiveTextureEdit> edits);
    Lease Acquire(PrimitiveTextureVersion expected) const;
    bool Withdraw(PrimitiveTextureVersion expected);
private:
    mutable std::mutex mutex_;
    Lease current_;
    uint64_t incarnation_{},last_generation_{};
};
}
