#include "texture_tiling.h"
#include <bit>
#include <cmath>
#include <cstring>
#include <limits>

namespace ds3content {
namespace {
bool TilingValues(const TextureTilingRow& row,std::array<float,4>& values) noexcept {
    values={row.scale[0],row.scale[1],0.0f,0.0f};
    for(size_t axis=0;axis<2;++axis) {
        if(!std::isfinite(row.scale[axis]) ||
           (row.animated && !std::isfinite(row.animation_offset[axis])))return false;
        const float difference=1.0f-row.scale[axis];
        const float centered=difference*0.5f;
        values[axis+2]=row.animated?centered+row.animation_offset[axis]:centered;
        if(!std::isfinite(values[axis+2]))return false;
    }
    return true;
}
}
bool ProduceTextureTiling(std::span<const TextureTilingWrite> rows,
                          TextureTilingValues& output) noexcept {
    if(rows.size()>128)return false;
    TextureTilingValues candidate;
    for(const auto& write:rows) {
        const auto& row=write.row;
        const auto index=row.array_index==UINT32_MAX?row.source_slot:row.array_index;
        if(row.source_slot>=128 || index>=25)return false;
        const size_t byte=static_cast<size_t>(index)*16,word=byte/4;
        // Clear before evaluating this write; unavailable replacement must not
        // retain an earlier material row's values or written mask.
        std::memset(candidate.bytes.data()+byte,0,16);
        std::fill_n(candidate.written.data()+word,4,uint8_t(0));
        if(!write.available)continue;
        std::array<float,4> values;
        if(!TilingValues(row,values))return false;
        std::memcpy(candidate.bytes.data()+byte,values.data(),16);
        std::fill_n(candidate.written.data()+word,4,uint8_t(1));
    }
    output=candidate;return true;
}
bool ReadTextureScrollRecords(std::span<const uint8_t> bytes,
                              TextureScrollHistory& history) noexcept {
    if(bytes.empty())return true;
    if(bytes.size()>1024u*1024u)return false;
    constexpr uint32_t keys[]{0x30325847,0x31325847,0x32325847,0x33325847,
        0x34325847,0x35325847,0x36325847,0x37325847,0x38335847,0x38325847,
        0x39325847,0x30335847,0x31335847,0x32335847,0x33335847,0x34335847};
    constexpr int32_t slots[]{0,7,1,8,2,9,3,10,16,4,11,5,12,6,13,14};
    auto candidate=history;size_t at{};
    for(size_t count=0;count<65536;++count) {
        if(at>bytes.size() || bytes.size()-at<12)return false;
        uint32_t key{};int32_t length{};
        std::memcpy(&key,bytes.data()+at,4);
        std::memcpy(&length,bytes.data()+at+8,4);
        if(key==0x7FFFFFFF){history=candidate;return true;}
        if(length<12 || static_cast<size_t>(length)>bytes.size()-at)return false;
        for(size_t i=0;i<std::size(keys);++i)if(key==keys[i]) {
            if(length<20)return false;
            candidate.active_mask|=1u<<i;candidate.rows[i].source_slot=slots[i];
            std::memcpy(candidate.rows[i].coefficient.data(),bytes.data()+at+12,8);
            break;
        }
        at+=static_cast<size_t>(length);
    }
    return false;
}
bool AdvanceTextureScroll(TextureScrollHistory& history,float now,float rate,
                          TextureUvOffsets& offsets) noexcept {
    if(!std::isfinite(now) || !std::isfinite(rate) || (offsets.available&~0xFFFFFu))return false;
    auto next=history;auto selected=offsets;
    for(size_t i=0;i<next.rows.size();++i) {
        auto& row=next.rows[i];
        if(!(next.active_mask&(1u<<i)) || row.source_slot<0)continue;
        if(!std::isfinite(row.previous_time) || !std::isfinite(row.phase))return false;
        // Round at the original SUBSS/MULSS boundaries; do not fuse to FMA.
        const volatile float delta=now-row.previous_time;
        const volatile float scaled=delta*rate;
        const float phase=scaled+row.phase;
        if(!std::isfinite(phase))return false;
        row.previous_time=now;row.phase=phase;
        if(row.source_slot>=20)continue;
        if(!std::isfinite(row.coefficient[0]) || !std::isfinite(row.coefficient[1]))return false;
        const auto slot=static_cast<uint32_t>(row.source_slot);
        const std::array<float,2> pair{phase*row.coefficient[0],phase*row.coefficient[1]};
        if(!std::isfinite(pair[0]) || !std::isfinite(pair[1]))return false;
        selected.pairs[slot]=pair;selected.mask|=1u<<slot;selected.available|=1u<<slot;
    }
    history=next;offsets=selected;return true;
}
TextureScrollOwner::TextureScrollOwner(const TextureScrollHistory& authored) {
    auto initial=std::make_shared<TextureScrollVersion>();
    initial->generation=1;initial->history=authored;current_=std::move(initial);
}
std::shared_ptr<const TextureScrollVersion> TextureScrollOwner::Update(
    uint64_t clock_generation,float now,float rate) {
    std::lock_guard<std::mutex> hold(lock_);
    if(withdrawn_ || !clock_generation || !std::isfinite(now) || !std::isfinite(rate))return {};
    if(current_->updated && current_->clock_generation==clock_generation &&
       std::bit_cast<uint32_t>(current_->now)==std::bit_cast<uint32_t>(now) &&
       std::bit_cast<uint32_t>(current_->rate)==std::bit_cast<uint32_t>(rate))return current_;
    auto next=std::make_shared<TextureScrollVersion>(*current_);
    next->offsets={};
    if(!AdvanceTextureScroll(next->history,now,rate,next->offsets))return {};
    next->generation=current_->generation+1;next->clock_generation=clock_generation;
    next->now=now;next->rate=rate;next->updated=true;current_=next;return next;
}
void TextureScrollOwner::Withdraw() {
    std::lock_guard<std::mutex> hold(lock_);withdrawn_=true;current_.reset();
}
}
