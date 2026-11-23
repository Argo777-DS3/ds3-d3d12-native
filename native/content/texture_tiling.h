#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>

namespace ds3content {
// CPU intent, independent of original resource objects and GPU wrappers.
// animation_offset is current CPU state supplied by the caller, not sampled
// here. The source slot selects that state; array_index may remap its output.
struct TextureTilingRow {
    uint32_t source_slot{};
    uint32_t array_index{UINT32_MAX};
    std::array<float,2> scale{};
    bool animated{};
    std::array<float,2> animation_offset{};
    bool operator==(const TextureTilingRow&) const = default;
};
using TextureTilingBlock = std::array<uint8_t,400>;

struct TextureTilingWrite {
    TextureTilingRow row;
    bool available{true};
};
struct TextureTilingValues {
    TextureTilingBlock bytes{};
    std::array<uint8_t,100> written{};
    bool operator==(const TextureTilingValues&) const = default;
};
// Current ordered writes from1AD990/1ADEB0. An unavailable later write clears
// that destination's coverage. Physical zeroes never imply semantic coverage.
// Repeated sources/destinations are permitted; invalid input leaves output intact.
bool ProduceTextureTiling(std::span<const TextureTilingWrite> rows,
                          TextureTilingValues& output) noexcept;

struct TextureUvOffsets {
    uint32_t mask{},available{};
    std::array<std::array<float,2>,20> pairs{};
    bool operator==(const TextureUvOffsets&) const = default;
};
struct TextureScrollRow {
    int32_t source_slot{-1};
    std::array<float,2> coefficient{};
    float phase{},previous_time{};
    bool operator==(const TextureScrollRow&) const = default;
};
struct TextureScrollHistory {
    uint32_t active_mask{};
    std::array<TextureScrollRow,16> rows{};
    bool operator==(const TextureScrollHistory&) const = default;
};
//141070's ordered FLVER/GX scroll records. Empty means the caller has no GX
// input. Recognized rows copy raw coefficient words and preserve phase/time.
// Other GX responsibilities are outside this decoder. A nonempty input must
// contain a terminal record; malformed input leaves owned history unchanged.
bool ReadTextureScrollRecords(std::span<const uint8_t> bytes,
                              TextureScrollHistory& history) noexcept;
//230610's current CPU update. The caller owns history/time/rate and the selection
// scope's existing offsets. No reset, clock choice or legacy pointer is invented.
// Duplicate slots overwrite in row order; slots>=20 advance history only.
// Failure leaves both owned inputs intact.
bool AdvanceTextureScroll(TextureScrollHistory& history,float now,float rate,
                          TextureUvOffsets& offsets) noexcept;

struct TextureScrollVersion {
    uint64_t generation{},clock_generation{};
    float now{},rate{};
    bool updated{};
    TextureScrollHistory history;
    TextureUvOffsets offsets;
};
// An engine material owns history; selection keeps an immutable successful
// version. Withdrawal blocks future update without invalidating
// versions retained by preparation or a later recording/completion owner.
class TextureScrollOwner {
public:
    explicit TextureScrollOwner(const TextureScrollHistory& authored);
    std::shared_ptr<const TextureScrollVersion> Update(uint64_t clock_generation,
                                                     float now,float rate);
    void Withdraw();
private:
    mutable std::mutex lock_;
    std::shared_ptr<const TextureScrollVersion> current_;
    bool withdrawn_{};
};
}
