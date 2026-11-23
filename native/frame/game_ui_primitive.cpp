#include "game_ui_primitive.h"
#include <cmath>

namespace ds3native::bootstrap {
bool DecodeUiPrimitiveState(uint32_t depth,uint32_t ref,uint32_t blend,uint8_t premultiplied,
    uint8_t target_alpha,const frame::PrimitiveViewport& viewport,UiPrimitiveState& output){
    if(premultiplied>1 || target_alpha>1 || !std::isfinite(viewport.x) || !std::isfinite(viewport.y) ||
       !std::isfinite(viewport.width) || !std::isfinite(viewport.height) || viewport.width<=0 || viewport.height<=0 ||
       viewport.min_depth!=0 || viewport.max_depth!=0)return false;
    UiPrimitiveState state;state.viewport=viewport;
    if(depth==1){if(ref>255)return false;state.stencil=frame::PrimitiveStencil::Disabled;}
    else if(depth==2 && !ref)state.stencil=frame::PrimitiveStencil::ReplaceZero;
    else if(depth==3 && ref<=255)state.stencil=frame::PrimitiveStencil::ClampToReference;
    else if(depth==4 && ref<255)state.stencil=frame::PrimitiveStencil::WriteMaskLevel;
    else if(depth==5 && ref && ref<=255)state.stencil=ref==1?frame::PrimitiveStencil::ReadOne:frame::PrimitiveStencil::ReadMaskLevel;
    else return false;
    state.stencil_reference=ref;
    // The depth row's color-enable field makes1F79550 choose the disabled
    // zero-write blend for modes2/3, independently of the current blend mode.
    if(depth==2 || depth==3 || depth==4)state.blend=frame::PrimitiveBlend::NoColor;
    else if(blend==0 || blend==1 || blend==18)state.blend=premultiplied?
        frame::PrimitiveBlend::PremultipliedAlpha:frame::PrimitiveBlend::StraightAlpha;
    else if(blend==8)state.blend=premultiplied?frame::PrimitiveBlend::PremultipliedAdditive:frame::PrimitiveBlend::AdditiveAlpha;   // table rows 8 and 28 (MENUS-MAP 33)
    else if(blend==3)state.blend=frame::PrimitiveBlend::Multiply;
    else if(blend==4)state.blend=frame::PrimitiveBlend::Screen;
    else return false;
    output=state;return true;
}
static frame::PrimitiveWorkResult ProduceUiPrimitive(const UiCpuPreparedShader& shader,const PrimitiveGeometry& geometry,
    frame::PrimitiveProgramVersion current,uint64_t serial,uint32_t width,uint32_t height,
    const UiPrimitiveState& state,const UiDrawImageBinding* image,frame::PrimitiveWorkPacket& output,const UiResidentImage& resident={}){
    using Status=frame::PrimitiveWorkStatus;
    if(shader.programs.version!=current || shader.serial!=serial || !serial || !shader.draw_identity)
        return {Status::Invalid,shader.draw_identity};
    const auto& layout=shader.programs.vertex_layout;
    if(!layout.installed || !layout.supported || layout.elements.empty() || layout.stride!=geometry.stride)
        return {Status::Unsupported,shader.draw_identity};
    if(!geometry.incarnation || !geometry.vertex_count || !geometry.index_count ||
       geometry.vertices.bytes.size()!=uint64_t(geometry.stride)*geometry.vertex_count ||
       geometry.indices.bytes.size()!=uint64_t(geometry.index_count)*2)return {Status::Invalid,shader.draw_identity};
    frame::PrimitiveWorkSource source;source.content_incarnation=current.incarnation;
    source.generation=source.texture_generation=current.generation;source.serial=serial;source.width=width;source.height=height;
    source.programs={shader.programs.vertex,shader.programs.pixel};source.buffers={geometry.vertices,geometry.indices};
    frame::PrimitiveDraw draw;draw.identity=shader.draw_identity;draw.vertex_program=shader.programs.vertex.identity;
    draw.pixel_program=shader.programs.pixel.identity;draw.vertices=geometry.vertices.identity;draw.indices=geometry.indices.identity;
    draw.elements=layout.elements;draw.constants=shader.constants;draw.stride=geometry.stride;draw.index_width=2;
    draw.count=geometry.index_count;draw.indexed=true;draw.viewport=state.viewport;draw.blend=state.blend;draw.stencil=state.stencil;
    draw.stencil_reference=state.stencil_reference;
    if(image){
        if(!image->images || !ApplyUiDrawImageBinding(*image,image->images->version,serial,draw,source.textures,resident))
            return {Status::Invalid,shader.draw_identity};
        source.texture_generation=image->images->version.generation;
    }
    source.draws.push_back(std::move(draw));return frame::ProducePrimitiveWork(std::move(source),output);
}
frame::PrimitiveWorkResult ProduceNativeUiPrimitive(const UiCpuPreparedShader& shader,const PrimitiveGeometry& geometry,
    frame::PrimitiveProgramVersion current,uint64_t serial,uint32_t width,uint32_t height,
    const UiPrimitiveState& state,frame::PrimitiveWorkPacket& output){
    return ProduceUiPrimitive(shader,geometry,current,serial,width,height,state,nullptr,output);
}
frame::PrimitiveWorkResult ProduceNativeUiPrimitive(const UiCpuPreparedShader& shader,const PrimitiveGeometry& geometry,
    frame::PrimitiveProgramVersion current,uint64_t serial,uint32_t width,uint32_t height,
    const UiPrimitiveState& state,const UiDrawImageBinding& image,frame::PrimitiveWorkPacket& output,const UiResidentImage& resident){
    return ProduceUiPrimitive(shader,geometry,current,serial,width,height,state,&image,output,resident);
}
}
