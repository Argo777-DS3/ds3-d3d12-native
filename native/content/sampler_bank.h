#pragma once
#include "sampler_inputs.h"
#include <memory>
#include <mutex>

namespace ds3content {
struct EngineSamplerBankVersion {
    uint64_t generation{};
    uint32_t external_selector{};
    std::array<EngineSamplerValues,16> engine_values;
    std::array<D3D12_SAMPLER_DESC,16> descriptors;
};

// Current external intent only: no implicit selector or original worker.
// D21F0 supplies raw authored CPU values;219B0 supplies five settings pairs.
// 797's owner+A8 is an external request, not a universal effective selector:
// original23660 respects Ignore External Settings. Local editor overrides are
// outside this contract. Caller must publish a current validated request; it may keep using the last lease (Acquire) when a read of the request fails.
// Selection retains the exact returned immutable version through encoding and
// completion. CPU generation is not a fence or proof of GPU lifetime.
class EngineSamplerBankOwner {
public:
    using Lease=std::shared_ptr<const EngineSamplerBankVersion>;
    Lease PublishExternalSelector(uint32_t selector) {
        if(selector>=profiles_.size())return {};
        const std::lock_guard<std::mutex> hold(mutex_);
        if(current_ && current_->external_selector==selector)return current_;
        if(last_generation_==UINT64_MAX)return {};
        auto next=std::make_shared<EngineSamplerBankVersion>();
        next->engine_values=initial_values_;
        const auto& profile=profiles_[selector];
        if(!ApplyEngineSamplerSettings(next->engine_values,profile[0],profile[1]))return {};
        for(size_t row=0;row<next->engine_values.size();++row)
            if(!DecodeEngineSampler(next->engine_values[row],next->descriptors[row]))return {};
        next->generation=last_generation_+1;next->external_selector=selector;
        current_=std::move(next);last_generation_=current_->generation;
        return current_;
    }
    Lease Acquire() const {
        const std::lock_guard<std::mutex> hold(mutex_);return current_;
    }
    // Withdraw availability, not held leases. A later explicit valid publish
    // creates a fresh generation, including when its selector was seen before.
    void Withdraw() {
        const std::lock_guard<std::mutex> hold(mutex_);current_.reset();
    }
private:
    // Exact D21F0 inputs in bank-index order, before legacy GetDesc. Trailing
    // original+34 padding is excluded. No native version exposes this table
    // until an explicit external selector has updated rows12/13 and decoded it.
    inline static constexpr std::array<EngineSamplerValues,16> initial_values_{
        EngineSamplerValues{{0,0,0,0,0,1,7,0,0,0,0,0,0x7F7FFFFF}}, //0
        EngineSamplerValues{{7,0,0,0,0,1,7,0,0,0,0,0,0x7F7FFFFF}}, //1
        EngineSamplerValues{{6,0,0,0,0,1,7,0,0,0,0,0,0}},          //2
        EngineSamplerValues{{0,2,2,2,0,1,7,0,0,0,0,0,0x7F7FFFFF}}, //3
        EngineSamplerValues{{7,2,2,2,0,1,7,0,0,0,0,0,0x7F7FFFFF}}, //4
        EngineSamplerValues{{6,2,2,2,0,1,7,0,0,0,0,0,0}},          //5
        EngineSamplerValues{{0,3,3,3,0,1,7,0x3F800000,0x3F800000,0x3F800000,0x3F800000,0,0x7F7FFFFF}}, //6
        EngineSamplerValues{{7,3,3,3,0,1,7,0x3F800000,0x3F800000,0x3F800000,0x3F800000,0,0x7F7FFFFF}}, //7
        EngineSamplerValues{{8,0,0,0,0,4,7,0,0,0,0,0,0x7F7FFFFF}}, //8
        EngineSamplerValues{{8,2,2,2,0,4,7,0,0,0,0,0,0x7F7FFFFF}}, //9
        EngineSamplerValues{{8,3,3,3,0,4,7,0,0,0,0,0,0x7F7FFFFF}}, //10
        EngineSamplerValues{{8,3,3,3,0,4,7,0x3F000000,0x3F000000,0x3F000000,0,0,0x7F7FFFFF}}, //11
        EngineSamplerValues{{8,0,0,0,0,4,7,0x3F000000,0x3F000000,0x3F000000,0,0,0x7F7FFFFF}}, //12
        EngineSamplerValues{{8,2,2,2,0,4,7,0x3F000000,0x3F000000,0x3F000000,0,0,0x7F7FFFFF}}, //13
        EngineSamplerValues{{7,0,0,0,0,0,7,0x3F000000,0x3F000000,0x3F000000,0,0,0x7F7FFFFF}}, //14
        EngineSamplerValues{{15,3,3,3,0,0,1,0,0,0,0,0,0}}};        //15
    inline static constexpr std::array<std::array<uint32_t,2>,5> profiles_{
        std::array<uint32_t,2>{1,1},{2,1},{3,4},{3,8},{3,16}};
    mutable std::mutex mutex_;
    Lease current_;
    uint64_t last_generation_{};
};
}
