#pragma once

#include <array>
#include <cstdint>

enum class SpatialSite : std::uint8_t {
    forward_entry, forward_write, reverse_entry, reverse_write
};

enum class PairResult : std::uint8_t {
    entry_opened, pair_accepted, invalid_common, non_monotonic,
    duplicate_entry, orphan_write, invalid_payload, capacity_exceeded
};

struct SpatialEvent {
    std::uint64_t sequence{};
    std::uint32_t tid{};
    std::uint64_t update_index{};
    SpatialSite site{};
    std::uintptr_t owner{};
    std::uintptr_t descriptor{};
    std::uintptr_t selected{};
    bool pointer_chain_valid{};
    bool selected48_valid{};
    bool payload48_valid{};
    std::array<std::uint32_t,4> elapsed{};
};

class SpatialPairing {
public:
    PairResult Process(const SpatialEvent& event) {
        if (!event.sequence || !event.tid || !event.owner || !event.selected ||
            !event.pointer_chain_valid || (IsEntry(event.site)&&!event.descriptor))
            return PairResult::invalid_common;
        if (event.sequence <= last_sequence_) return PairResult::non_monotonic;
        last_sequence_ = event.sequence;
        if (IsEntry(event.site)) {
            if (!event.selected48_valid) return PairResult::invalid_payload;
            if (Find(event) != entries_.size()) return PairResult::duplicate_entry;
            for (auto& slot : entries_) if (!slot.used) {
                slot={true,event}; return PairResult::entry_opened;
            }
            return PairResult::capacity_exceeded;
        }
        const std::size_t index=Find(event);
        if (index==entries_.size()) return PairResult::orphan_write;
        const SpatialEvent entry=entries_[index].event;
        entries_[index].used=false;
        if (!event.payload48_valid ||
            (Direction(event.site)==0 && entry.elapsed!=event.elapsed)) {
            return PairResult::invalid_payload;
        }
        return PairResult::pair_accepted;
    }

    std::size_t OpenCount() const {
        std::size_t count=0; for (const auto& slot:entries_) if (slot.used) ++count;
        return count;
    }

private:
    struct Slot { bool used{}; SpatialEvent event{}; };
    std::array<Slot,8> entries_{};
    std::uint64_t last_sequence_{};

    static bool IsEntry(SpatialSite site) {
        return site==SpatialSite::forward_entry || site==SpatialSite::reverse_entry;
    }
    static unsigned Direction(SpatialSite site) {
        return site==SpatialSite::forward_entry || site==SpatialSite::forward_write ? 0u : 1u;
    }
    static bool SameKey(const SpatialEvent& left,const SpatialEvent& right) {
        return Direction(left.site)==Direction(right.site) && left.tid==right.tid &&
            left.update_index==right.update_index && left.owner==right.owner &&
            left.selected==right.selected;
    }
    std::size_t Find(const SpatialEvent& event) const {
        for (std::size_t i=0;i<entries_.size();++i)
            if (entries_[i].used && SameKey(entries_[i].event,event)) return i;
        return entries_.size();
    }
};
