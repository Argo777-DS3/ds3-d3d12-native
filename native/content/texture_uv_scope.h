#pragma once
#include <cmath>
#include <cstddef>
#include "texture_tiling.h"

namespace ds3content {
// One ordered material-preparation invocation owns this scope. The caller
// supplies the current F8/F9 flags; an unknown flag has no implicit default.
// A clear mask does not claim that the physical zero pairs are known values.
struct TextureUvScope {
    bool skip_kind8,special_kind5;
    TextureUvOffsets offsets{};
    explicit TextureUvScope(bool current_skip_kind8,bool current_special_kind5) noexcept
        :skip_kind8(current_skip_kind8),special_kind5(current_special_kind5) {}
};

// Merge only explicit pairs from an updated native TextureScrollOwner version.
// Advance/update belongs to the caller after eligibility, in material order.
// Existing unavailable pairs may be retained, but a new masked contribution
// must supply every pair: it cannot silently inherit an older known value.
// Failure leaves the complete scope unchanged; unrelated pairs keep their
// value and availability across preparations in this same invocation.
inline bool MergeTextureScrollContribution(TextureUvScope& scope,
                                           const TextureUvOffsets& contribution) noexcept {
    constexpr uint32_t valid_mask=0xFFFFFu;
    const auto valid=[](const TextureUvOffsets& offsets) noexcept {
        if(((offsets.mask|offsets.available)&~valid_mask) ||
           (offsets.available&~offsets.mask))return false;
        for(size_t slot=0;slot<offsets.pairs.size();++slot)
            if(offsets.available&(1u<<slot))
                for(const auto value:offsets.pairs[slot])
                    if(!std::isfinite(value))return false;
        return true;
    };
    if(contribution.mask!=contribution.available ||
       !valid(scope.offsets) || !valid(contribution))return false;
    auto candidate=scope.offsets;
    for(size_t slot=0;slot<contribution.pairs.size();++slot)
        if(contribution.available&(1u<<slot))candidate.pairs[slot]=contribution.pairs[slot];
    candidate.mask|=contribution.mask;
    candidate.available|=contribution.available;
    scope.offsets=candidate;
    return true;
}
}
