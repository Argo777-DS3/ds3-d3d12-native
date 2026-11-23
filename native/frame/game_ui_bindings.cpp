#include "game_ui_bindings.h"
#include <algorithm>
#include <cfloat>

namespace ds3native::bootstrap {
bool DecodeUiSampler(uint8_t index,frame::PrimitiveSampler& output) noexcept {
    if(index>3)return false;
    constexpr frame::PrimitiveSampler modes[]{frame::PrimitiveSampler::PointWrap,frame::PrimitiveSampler::PointClamp,
        frame::PrimitiveSampler::LinearWrap,frame::PrimitiveSampler::LinearClamp};
    output=modes[index];return true;
}
bool UiDrawImageSource::Begin(UiImageIngress::Lease current,uint64_t serial,uint64_t draw_identity){
    binding_={};bound_mask_=0;valid_=current && current->textures && current->version.incarnation && current->version.generation &&
        current->textures->version==current->version && serial && draw_identity;
    if(valid_){binding_.images=std::move(current);binding_.serial=serial;binding_.draw_identity=draw_identity;}
    return valid_;
}
bool UiDrawImageSource::Bind(UiImageIngress::Lease current,uintptr_t engine_image,uint32_t slot,uint8_t index){
    frame::PrimitiveSampler sampler;
    if(!valid_ || slot>1 || (bound_mask_&(1u<<slot)) || !current || current!=binding_.images || !DecodeUiSampler(index,sampler))return valid_=false;
    const auto image=current->images.find(engine_image);
    // Imported assets retain their shared native resource directly. A token is
    // needed to retire an upload owned by UiImageIngress, not to bind that asset.
    if(image==current->images.end() || !image->second.identity || !image->second.gpu ||
       !image->second.gpu->resident || !image->second.gpu->upload_fence || !image->second.gpu->resource || !image->second.gpu->view)return valid_=false;
    const auto texture=std::find_if(current->textures->textures.begin(),current->textures->textures.end(),
        [&](const auto& t){return t.identity==image->second.identity;});
    if(texture==current->textures->textures.end() || !frame::ValidPrimitiveTexture(*texture))return valid_=false;
    const auto& actual=image->second.gpu->description;const auto& cpu=texture->texture;
    if(actual.width!=cpu.width || actual.height!=cpu.height || actual.mips!=cpu.mips.size() || actual.format!=cpu.format ||
       actual.slices!=1 || actual.cube)return valid_=false;
    if(!slot){binding_.texture=image->second.identity;binding_.sampler_index=index;binding_.sampler=sampler;
        binding_.mip_bias=-.75f;binding_.min_lod=0;binding_.max_lod=FLT_MAX;}
    else {binding_.secondary={image->second.identity,sampler,-.75f,0,FLT_MAX};binding_.secondary_sampler_index=index;}
    bound_mask_|=1u<<slot;return true;
}
bool UiDrawImageSource::Resolve(frame::PrimitiveTextureVersion current,uint64_t serial,UiDrawImageBinding& output) const {
    if(!valid_ || (binding_.secondary.texture && !binding_.texture) || !binding_.images || binding_.images->version!=current || binding_.serial!=serial)return false;
    output=binding_;return true;
}
bool ApplyUiDrawImageBinding(const UiDrawImageBinding& binding,frame::PrimitiveTextureVersion current,
    uint64_t serial,frame::PrimitiveDraw& draw,std::vector<frame::PrimitiveTexture>& textures,const UiResidentImage& resident){
    if(!binding.images || !binding.images->textures || binding.images->version!=current || binding.serial!=serial ||
       !serial || binding.draw_identity!=draw.identity || draw.texture || draw.secondary.texture || draw.sampler!=frame::PrimitiveSampler::None)return false;
    if(!binding.texture)return binding.sampler==frame::PrimitiveSampler::None && !binding.secondary.texture;
    frame::PrimitiveSampler sampler;
    if(!DecodeUiSampler(binding.sampler_index,sampler) || sampler!=binding.sampler || binding.mip_bias!=-.75f ||
       binding.min_lod!=0 || binding.max_lod!=FLT_MAX)return false;
    const auto found=std::find_if(binding.images->textures->textures.begin(),binding.images->textures->textures.end(),
        [&](const auto& t){return t.identity==binding.texture;});
    if(found==binding.images->textures->textures.end() || !frame::ValidPrimitiveTexture(*found))return false;
    const auto old=std::find_if(textures.begin(),textures.end(),[&](const auto& t){return t.identity==binding.texture;});
    if(old!=textures.end() && (old->texture.format!=found->texture.format || old->texture.width!=found->texture.width ||
            old->texture.height!=found->texture.height || old->texture.mips!=found->texture.mips || old->texture.shader_slot!=0))return false;
    auto prepared=draw;prepared.texture=binding.texture;prepared.sampler=sampler;prepared.mip_bias=binding.mip_bias;
    prepared.min_lod=binding.min_lod;prepared.max_lod=binding.max_lod;
    const frame::PrimitiveTexture* second_image{};
    if(binding.secondary.texture){frame::PrimitiveSampler secondary_sampler;
        if(!DecodeUiSampler(binding.secondary_sampler_index,secondary_sampler) || secondary_sampler!=binding.secondary.sampler ||
           binding.secondary.mip_bias!=-.75f || binding.secondary.min_lod!=0 || binding.secondary.max_lod!=FLT_MAX)return false;
        const auto second=std::find_if(binding.images->textures->textures.begin(),binding.images->textures->textures.end(),
            [&](const auto& t){return t.identity==binding.secondary.texture;});
        if(second==binding.images->textures->textures.end() || !frame::ValidPrimitiveTexture(*second))return false;
        second_image=&*second;prepared.secondary=binding.secondary;
    }
    // Every check passed: the images are added only now (a resident payload, once taken, always lands in `textures`).
    // m45 frame cost: room for two more, grown geometrically (an exact reserve per binding moved the whole list every time).
    const bool primary_new=old==textures.end();if(textures.capacity()<textures.size()+2)textures.reserve(std::max(textures.size()+2,textures.capacity()*2));   // invalidates `old`
    const auto take=[&](const frame::PrimitiveTexture& origin){frame::PrimitiveTexture taken;
        if(resident && resident(origin,taken)){taken.identity=origin.identity;textures.push_back(std::move(taken));}else textures.push_back(origin);};
    if(primary_new)take(*found);
    if(second_image && std::none_of(textures.begin(),textures.end(),[&](const auto& t){return t.identity==second_image->identity;}))take(*second_image);
    draw=std::move(prepared);return true;
}
}
