#include "game_ui_images.h"
#include <algorithm>
#include <stdexcept>

namespace ds3native::bootstrap {
namespace {
ds3content::CpuTextureResource PhysicalImage(const SceneTexture& image){
    ds3content::CpuTextureResource output;
    output.name="engine-ui-cpu-image";output.width=image.width;output.height=image.height;
    output.mip_levels=static_cast<uint32_t>(image.mips.size());output.array_slices=1;output.format=image.format;
    for(uint32_t mip=0;mip<output.mip_levels;++mip){
        const auto width=std::max(1u,image.width>>mip),height=std::max(1u,image.height>>mip);
        frame::PrimitiveTextureLayout layout;
        if(!frame::GetPrimitiveTextureLayout(width,height,image.format,layout))throw std::runtime_error("UI image physical layout");
        output.subresources.push_back({width,height,layout.row_bytes,layout.rows,image.mips[mip]});
    }
    return output;
}
auto FindImage(const NativeUiImages& state,uint64_t identity){
    return std::find_if(state.textures->textures.begin(),state.textures->textures.end(),
        [identity](const auto& image){return image.identity==identity;});
}
}
UiImageIngress::UiImageIngress(TextureIngress& uploads,uint64_t incarnation):uploads_(uploads){
    frame::PrimitiveTextureStore store;
    if(store.Define({incarnation,1},{})!=frame::PrimitiveTextureStatus::Ready)throw std::runtime_error("UI image incarnation");
    current_=std::make_shared<const NativeUiImages>(NativeUiImages{{incarnation,1},store.Acquire({incarnation,1}),{}});
}
bool UiImageIngress::Create(uintptr_t engine_image,frame::PrimitiveTextureVersion expected,uint32_t format,
    std::span<const CpuPrimitiveImageMip> mips){
    const std::lock_guard<std::mutex> hold(mutex_);
    if(!engine_image || !current_ || current_->version!=expected || last_identity_==UINT64_MAX)return false;
    const auto found=current_->images.find(engine_image);
    const auto identity=found==current_->images.end()?last_identity_+1:found->second.identity;
    frame::PrimitiveTexture image;
    if(!ProduceCpuPrimitiveImage(identity,format,mips,image))return false;
    const std::array<frame::PrimitiveTextureEdit,1> edits{frame::PrimitiveTextureReplace{image}};
    if(!Publish(engine_image,expected,image,edits))return false;
    last_identity_=std::max(last_identity_,identity);return true;
}
bool UiImageIngress::UpdateGlyphs(uintptr_t engine_image,frame::PrimitiveTextureVersion expected,
    std::span<const CpuGlyphImageUpdate> updates){
    const std::lock_guard<std::mutex> hold(mutex_);
    if(!current_ || current_->version!=expected)return false;
    const auto found=current_->images.find(engine_image);if(found==current_->images.end())return false;
    const auto image=FindImage(*current_,found->second.identity);
    if(image==current_->textures->textures.end())return false;
    std::vector<frame::PrimitiveTextureEdit> edits;
    if(!ProduceCpuGlyphImageUpdates(image->identity,image->texture,updates,edits))return false;
    return Publish(engine_image,expected,*image,edits);
}
bool UiImageIngress::Import(uintptr_t engine_image,frame::PrimitiveTextureVersion expected,
    std::shared_ptr<const NativeTextureRecord> texture){
    const std::lock_guard<std::mutex> hold(mutex_);
    if(!engine_image || !current_ || current_->version!=expected || expected.generation==UINT64_MAX ||
       last_identity_==UINT64_MAX || current_->images.contains(engine_image) || !texture ||
       !texture->resident || !texture->resource || !texture->view || !texture->content ||
       texture->description.cube || texture->description.slices!=1)return false;
    const auto& content=*texture->content;
    if(content.subresources.size()!=content.mip_levels)return false;
    frame::PrimitiveTexture image;image.identity=last_identity_+1;
    // 1250: an imported asset is a resident native resource (shader-resource state since its upload or its target
    // admission) and is bound as is when its description is the content's; its bytes are then never read and the UI
    // texture set keeps one empty entry per mip. The movie screen imports its image every update and each import or
    // retirement copies the whole set (Native1250: the movie at 10 frames per second with 8 MB per copy).
    const auto resource=texture->resource->GetDesc();
    const bool bound_as_is=texture->render_target || (resource.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D && resource.DepthOrArraySize==1 &&
        resource.Width==content.width && resource.Height==content.height && resource.MipLevels==content.mip_levels && resource.Format==content.format);
    // I-0096: an admitted texture keeps its description only (game_texture_ingress.cpp): its bytes are read only when it cannot
    // be bound as is, which a record whose resource was made from that description never meets; refused, not drawn neutral.
    if(!bound_as_is && !ds3content::ValidCpuTextureResource(content))return false;
    if(bound_as_is)image.external=texture->resource;
    image.texture.width=content.width;image.texture.height=content.height;image.texture.format=content.format;
    for(const auto& mip:content.subresources)image.texture.mips.push_back(bound_as_is?std::vector<uint8_t>{}:mip.bytes);
    const frame::PrimitiveTextureVersion next{expected.incarnation,expected.generation+1};
    frame::PrimitiveTextureStore prepared;
    const std::array<frame::PrimitiveTextureEdit,1> edits{frame::PrimitiveTextureReplace{image}};
    if(prepared.Define(expected,current_->textures->textures)!=frame::PrimitiveTextureStatus::Ready ||
       prepared.Commit(expected,next.generation,edits)!=frame::PrimitiveTextureStatus::Ready)return false;
    auto candidate=std::make_shared<NativeUiImages>(*current_);
    candidate->version=next;candidate->textures=prepared.Acquire(next);
    // The asset registry retains its own ownership. This UI lease shares the
    // already fenced native allocation; no upload token or legacy GPU input.
    candidate->images.emplace(engine_image,NativeUiImage{image.identity,{},std::move(texture)});
    current_=std::move(candidate);last_identity_=image.identity;return true;
}
bool UiImageIngress::Publish(uintptr_t engine_image,frame::PrimitiveTextureVersion expected,
    const frame::PrimitiveTexture& image,std::span<const frame::PrimitiveTextureEdit> edits){
    if(expected.generation==UINT64_MAX)return false;
    const frame::PrimitiveTextureVersion next{expected.incarnation,expected.generation+1};
    // Prepare all CPU state/allocations before GPU submission. A failed upload
    // cannot replace the published image or invalidate retained consumers.
    frame::PrimitiveTextureStore prepared;
    if(prepared.Define(expected,current_->textures->textures)!=frame::PrimitiveTextureStatus::Ready ||
       prepared.Commit(expected,next.generation,edits)!=frame::PrimitiveTextureStatus::Ready)return false;
    auto candidate=std::make_shared<NativeUiImages>(*current_);
    candidate->version=next;candidate->textures=prepared.Acquire(next);
    const auto incoming=FindImage(*candidate,image.identity);
    if(incoming==candidate->textures->textures.end())return false;
    const auto old=current_->images.find(engine_image);
    const auto old_token=old==current_->images.end()?std::shared_ptr<uint8_t>{}:old->second.upload_token;
    const auto token=std::make_shared<uint8_t>(uint8_t{0});
    candidate->images.insert_or_assign(engine_image,NativeUiImage{image.identity,token,{}});
    const auto resource=PhysicalImage(incoming->texture);
    const auto upload_key=reinterpret_cast<uintptr_t>(token.get());
    try {
        auto gpu=uploads_.AdmitOwned(upload_key,resource);
        if(!gpu || !gpu->resident || !gpu->upload_fence){uploads_.Retire(upload_key);return false;}
        candidate->images.at(engine_image).gpu=std::move(gpu);
    }catch(...){uploads_.Retire(upload_key);throw;}
    current_=std::move(candidate);
    if(old_token)uploads_.Retire(reinterpret_cast<uintptr_t>(old_token.get()));
    return true;
}
bool UiImageIngress::Retire(uintptr_t engine_image,frame::PrimitiveTextureVersion expected){
    const std::lock_guard<std::mutex> hold(mutex_);
    if(!current_ || current_->version!=expected || expected.generation==UINT64_MAX)return false;
    const auto found=current_->images.find(engine_image);if(found==current_->images.end())return false;
    const auto token=found->second.upload_token;
    frame::PrimitiveTextureStore prepared;
    const std::array<frame::PrimitiveTextureEdit,1> edits{frame::PrimitiveTextureRemove{found->second.identity}};
    if(prepared.Define(expected,current_->textures->textures)!=frame::PrimitiveTextureStatus::Ready ||
       prepared.Commit(expected,expected.generation+1,edits)!=frame::PrimitiveTextureStatus::Ready)return false;
    auto candidate=std::make_shared<NativeUiImages>(*current_);
    candidate->version.generation++;candidate->textures=prepared.Acquire(candidate->version);candidate->images.erase(engine_image);
    current_=std::move(candidate);if(token)uploads_.Retire(reinterpret_cast<uintptr_t>(token.get()));return true;
}
UiImageIngress::Lease UiImageIngress::Acquire() const {
    const std::lock_guard<std::mutex> hold(mutex_);return current_;
}
void UiImageIngress::Close(){
    const std::lock_guard<std::mutex> hold(mutex_);
    if(!current_)return;
    for(const auto& [key,image]:current_->images){(void)key;if(image.upload_token)uploads_.Retire(reinterpret_cast<uintptr_t>(image.upload_token.get()));}
    current_.reset();
}
}
