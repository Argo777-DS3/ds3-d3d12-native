#include "primitive_textures.h"
#include <algorithm>
#include <cstring>
#include <set>
#include <type_traits>

namespace ds3native::frame {
namespace {
constexpr uint64_t MaxBytes=256u*1024u*1024u;
uint64_t Bytes(const SceneTexture& image){uint64_t n{};for(const auto& mip:image.mips)n+=mip.size();return n;}
bool ValidImages(const std::vector<PrimitiveTexture>& images,uint64_t& bytes){
    if(images.size()>PrimitiveTextureLimit)return false;
    std::set<uint64_t> ids;bytes=0;
    for(const auto& entry:images){
        if(!entry.identity || !ids.insert(entry.identity).second || !ValidPrimitiveTexture(entry))return false;
        const auto n=Bytes(entry.texture);if(n>MaxBytes-bytes)return false;bytes+=n;
    }
    return true;
}
PrimitiveTextureStatus Write(SceneTexture& image,const PrimitiveTextureWrite& patch){
    if(patch.mip>=image.mips.size())return PrimitiveTextureStatus::Invalid;
    const auto width=std::max(1u,image.width>>patch.mip),height=std::max(1u,image.height>>patch.mip);
    if(!patch.width || !patch.height || patch.x>width || patch.width>width-patch.x ||
       patch.y>height || patch.height>height-patch.y)return PrimitiveTextureStatus::Invalid;
    PrimitiveTextureLayout full{},part{};
    if(!GetPrimitiveTextureLayout(width,height,image.format,full) ||
       !GetPrimitiveTextureLayout(patch.width,patch.height,image.format,part))return PrimitiveTextureStatus::Unsupported;
    if(patch.x%full.block_width || patch.y%full.block_height ||
       (patch.width%full.block_width && patch.x+patch.width!=width) ||
       (patch.height%full.block_height && patch.y+patch.height!=height))return PrimitiveTextureStatus::Unsupported;
    if(patch.row_pitch<part.row_bytes || uint64_t(patch.row_pitch)*part.rows!=patch.bytes.size() ||
       patch.bytes.size()>MaxBytes)return PrimitiveTextureStatus::Invalid;
    const auto origin=uint64_t(patch.y/full.block_height)*full.row_bytes+uint64_t(patch.x/full.block_width)*full.block_bytes;
    auto& bytes=image.mips[patch.mip];
    for(uint32_t row=0;row<part.rows;++row){
        const auto destination=origin+uint64_t(row)*full.row_bytes;
        if(destination>bytes.size() || part.row_bytes>bytes.size()-destination)return PrimitiveTextureStatus::Invalid;
    }
    for(uint32_t row=0;row<part.rows;++row)std::memcpy(bytes.data()+origin+uint64_t(row)*full.row_bytes,
        patch.bytes.data()+uint64_t(row)*patch.row_pitch,part.row_bytes);
    return PrimitiveTextureStatus::Ready;
}
}
PrimitiveTextureStatus PrimitiveTextureStore::Define(PrimitiveTextureVersion version,const std::vector<PrimitiveTexture>& images){
    uint64_t bytes{};
    if(!version.incarnation || !version.generation || !ValidImages(images,bytes))return PrimitiveTextureStatus::Invalid;
    auto candidate=std::make_shared<const PrimitiveTextureSnapshot>(PrimitiveTextureSnapshot{version,images});
    const std::lock_guard<std::mutex> lock(mutex_);
    if((incarnation_ && incarnation_!=version.incarnation) || version.generation<=last_generation_)return PrimitiveTextureStatus::Stale;
    current_=std::move(candidate);incarnation_=version.incarnation;last_generation_=version.generation;return PrimitiveTextureStatus::Ready;
}
PrimitiveTextureStatus PrimitiveTextureStore::Commit(PrimitiveTextureVersion expected,uint64_t next_generation,std::span<const PrimitiveTextureEdit> edits){
    if(!expected.incarnation || !expected.generation || next_generation<=expected.generation || edits.empty() || edits.size()>4096)return PrimitiveTextureStatus::Invalid;
    const std::lock_guard<std::mutex> lock(mutex_);
    if(!current_ || current_->version!=expected || next_generation<=last_generation_)return PrimitiveTextureStatus::Stale;
    auto candidate=std::make_shared<PrimitiveTextureSnapshot>(*current_);uint64_t bytes{};
    if(!ValidImages(candidate->textures,bytes))return PrimitiveTextureStatus::Invalid;
    uint64_t input_bytes{};
    for(const auto& edit:edits){
        const uint64_t id=std::visit([](const auto& value)->uint64_t {
            if constexpr(std::is_same_v<std::decay_t<decltype(value)>,PrimitiveTextureReplace>)return value.texture.identity;
            else return value.identity;
        },edit);
        if(!id)return PrimitiveTextureStatus::Invalid;
        auto found=std::find_if(candidate->textures.begin(),candidate->textures.end(),[&](const auto& t){return t.identity==id;});
        if(const auto* replacement=std::get_if<PrimitiveTextureReplace>(&edit)){
            const auto& image=replacement->texture;
            if(!ValidPrimitiveTexture(image))return PrimitiveTextureStatus::Invalid;
            const auto incoming=Bytes(image.texture),prior=found==candidate->textures.end()?0:Bytes(found->texture);
            if(incoming>MaxBytes-input_bytes || incoming>MaxBytes-(bytes-prior))return PrimitiveTextureStatus::Invalid;
            input_bytes+=incoming;bytes=bytes-prior+incoming;
            if(found==candidate->textures.end()){
                if(candidate->textures.size()>=PrimitiveTextureLimit)return PrimitiveTextureStatus::Invalid;
                candidate->textures.push_back(image);
            }else *found=image;
        }else if(std::holds_alternative<PrimitiveTextureRemove>(edit)){
            if(found==candidate->textures.end())return PrimitiveTextureStatus::Missing;
            bytes-=Bytes(found->texture);candidate->textures.erase(found);
        }else{
            if(found==candidate->textures.end())return PrimitiveTextureStatus::Missing;
            const auto& patch=std::get<PrimitiveTextureWrite>(edit);
            if(patch.bytes.size()>MaxBytes-input_bytes)return PrimitiveTextureStatus::Invalid;
            input_bytes+=patch.bytes.size();const auto result=Write(found->texture,patch);
            if(result!=PrimitiveTextureStatus::Ready)return result;
        }
    }
    candidate->version.generation=next_generation;current_=std::move(candidate);last_generation_=next_generation;
    return PrimitiveTextureStatus::Ready;
}
PrimitiveTextureStore::Lease PrimitiveTextureStore::Acquire(PrimitiveTextureVersion expected) const {
    const std::lock_guard<std::mutex> lock(mutex_);
    return current_ && current_->version==expected?current_:Lease{};
}
bool PrimitiveTextureStore::Withdraw(PrimitiveTextureVersion expected){
    const std::lock_guard<std::mutex> lock(mutex_);
    if(!current_ || current_->version!=expected)return false;
    current_.reset();return true;
}
}
