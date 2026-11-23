#include "model_pass.h"
#include <stdexcept>

namespace ds3content {
NativeModelPixelSelection SelectNativeModelPixelVariant(uint32_t shader_pass,const NativeModelPixelInputs& input) noexcept {
    NativeModelPixelSelection result;
    if(shader_pass!=0 && shader_pass!=1 && shader_pass!=2 && shader_pass!=6 && shader_pass!=7 && shader_pass!=12){result.status=NativeModelPixelStatus::Unsupported;return result;}
    if(input.policy==NativeModelPixelPolicy::Unknown)return result;
    // WATER-TESS-WAVE1-READY WT-1: WatDpt has one pixel programme.
    if(shader_pass==12){result.variant=0;result.status=NativeModelPixelStatus::Selected;return result;}
    if(input.policy!=NativeModelPixelPolicy::Default && input.policy!=NativeModelPixelPolicy::ForcePlain &&
       input.policy!=NativeModelPixelPolicy::AlphaCoverage){result.status=NativeModelPixelStatus::Unsupported;return result;}
    result.variant=(input.template_value || !input.global_variant_enabled || input.fade_dither)?1u:0u;
    // D-0180 C2 (SUN-SHADOWS-STATIC-PREP 2): 140680 turns Dpt into DptA for an alpha-tested template (+0x7C) or a fading draw.
    if(shader_pass==2){result.variant=(input.template_value || input.fade_dither)?1u:0u;result.status=NativeModelPixelStatus::Selected;return result;}
    if(input.policy==NativeModelPixelPolicy::ForcePlain)result.variant=0;
    if(shader_pass==6 && input.policy==NativeModelPixelPolicy::AlphaCoverage)result.variant=2;
    result.status=NativeModelPixelStatus::Selected;return result;
}
uint32_t EffectiveOrdinaryModelDeformation(uint32_t base,bool disabled,uint32_t requested_lod) noexcept {
    return disabled || requested_lod?0u:base;
}
uint32_t SelectOrdinaryModelLod(uint32_t requested,uint32_t loaded_mask) noexcept {
    if(requested>31)return 0;
    if(!requested || (loaded_mask>>requested&1))return requested;
    uint32_t lod=requested-1;
    while(lod && !(loaded_mask>>lod&1))--lod;
    return lod;
}
OrdinaryModelPassDecision EvaluateOrdinaryModelPass(const OrdinaryModelPassInput& input) noexcept {
    OrdinaryModelPassDecision result;
    result.effective_deformation=EffectiveOrdinaryModelDeformation(input.base_deformation,
        (input.flags&static_cast<uint32_t>(NativeModelPassFlag::DisableDeformation))!=0,input.requested_lod);
    const auto unsupported=[&](NativeModelPassReason reason){result.status=NativeModelPassStatus::Unsupported;result.reason=reason;return result;};
    const auto filtered=[&](NativeModelPassReason reason){result.status=NativeModelPassStatus::Filtered;result.reason=reason;return result;};
    const bool forward=input.family==NativeModelPassFamily::OrdinaryForward;
    const bool velocity=input.family==NativeModelPassFamily::OrdinaryVelocity;
    // D-0180 C2 (SUN-SHADOWS-WAVE1 2): the cascade passes' row of 57A20 (passes 1..0x38 but 0x27: preparation 2, class mask 0x107,
    // secondary 0xFF; INFERRED from the decompile, the static preparation confirms it): shader pass 2 (Dpt), no tessellation.
    const bool shadow=input.family==NativeModelPassFamily::OrdinaryShadow;
    if(shadow)result.effective_deformation=0;
    // W7 TS-2b (TESS-SHADOW-FORWARD-NATIVE 1.1): the velocity pass kinds take deformation 0 (Velo has no tessellated variant).
    if(velocity)result.effective_deformation=0;
    if(input.family!=NativeModelPassFamily::OrdinaryDeferred && !forward && !velocity && !shadow)return unsupported(NativeModelPassReason::Family);
    // P8 (I-0067 PHANTOMS-SIGNS-NATIVE §5): a model of category 0x21 (phantom, faded in) gets the forward rows 0x21 (class
    // mask 0xFEFF) and 0x22 (0x100, the caller adds it only with +224 bit 15); a model of category 0x29 (fading, kind 1/3) the
    // generic forward row 0x29 with class mask 0xFFFF. Category 2 keeps the ordinary rows. Shader pass 0 (Fwd) for all three
    // (0x22 would be FwdSdw: explicit policy while the native has no shadows).
    const bool phantom_category=input.category==0x21 || input.category==0x29;
    if(input.category!=2 && !phantom_category)return unsupported(NativeModelPassReason::Category);
    if(phantom_category && shadow)return unsupported(NativeModelPassReason::Category);
    if(phantom_category && (!forward || (input.category==0x21?(input.engine_pass!=0x21 && input.engine_pass!=0x22):input.engine_pass!=0x29)))
        return unsupported(NativeModelPassReason::Category);
    // 1181: 57A20 adds a second forward row (engine pass 0x1F, preparation 0, class mask 0x80: forward materials that are not
    // transparent, e.g. alpha-tested hair P[ChrCustom_Hair]) for a model with +224 bit 12; the caller adds that row only then.
    if(shadow?(input.engine_pass<1 || input.engine_pass>4 || input.preparation!=2 || input.shader_pass!=2):
       velocity?(input.engine_pass!=0x34 || input.preparation!=5 || input.shader_pass!=1):
       // WATER-TESS-WAVE1-READY WT-1 (57A20:626-685): a kind-0x27 (water) draw's rows 0x24 (preparation 6, WatDpt) and 0x27
       // (preparation 0, Fwd), class mask 0xFFFF; the caller gives water meshes these rows and no other.
       forward && (input.engine_pass==0x24 || input.engine_pass==0x27)?(phantom_category || input.preparation!=(input.engine_pass==0x24?6u:0u) ||
           input.shader_pass!=(input.engine_pass==0x24?12u:0u)):
       // I-0113 tranche 10d: a draw's depth pre-pass in 0x29 (preparation 1, shader pass 2, Dpt; class mask 2 in category 2, 0xFFFF
       // in 0x29; 57A20:550-569, 657-720).
       forward?((phantom_category?false:(input.engine_pass!=0x29 && input.engine_pass!=0x1F)) || !((input.preparation==0 && input.shader_pass==0) ||
           ((input.category==2 || input.category==0x29) && input.engine_pass==0x29 && input.preparation==1 && input.shader_pass==2))):
       ((input.engine_pass!=5 && input.engine_pass!=6 && input.engine_pass!=7) || input.preparation!=3 || input.shader_pass!=6))
        return unsupported(NativeModelPassReason::Selectors);
    if(input.required_mesh_mask!=1)return unsupported(NativeModelPassReason::RequiredMeshMask);
    if(input.requested_lod>31)return unsupported(NativeModelPassReason::RequestedLod);
    result.effective_lod=SelectOrdinaryModelLod(input.requested_lod,input.model_lod_mask);
    const bool split=(input.flags&static_cast<uint32_t>(NativeModelPassFlag::SplitEngine5))!=0;
    if(input.flags&~(static_cast<uint32_t>(NativeModelPassFlag::DisableDeformation)|static_cast<uint32_t>(NativeModelPassFlag::SplitEngine5)))
        return unsupported(NativeModelPassReason::Flags);
    if(split?(input.engine_pass!=5 || (input.row_class_mask!=1 && input.row_class_mask!=4)):input.row_class_mask!=0)
        return unsupported(NativeModelPassReason::Flags);
    if(input.base_deformation>4){result.reason=NativeModelPassReason::Deformation;return result;}
    // 57A20 rows: engine5 class 1|4 (or split 1, 4), engine6 class 8, engine7 class 0x100 (model +224 bit 15,
    // preparation 3; the caller adds that row only for such models).
    result.material_class_mask=shadow?0x107u:velocity?0xFFFFu:phantom_category?(input.engine_pass==0x21?0xFEFFu:input.engine_pass==0x22?0x100u:0xFFFFu):
        forward?(input.engine_pass==0x24 || input.engine_pass==0x27?0xFFFFu:input.engine_pass==0x1F?0x80u:input.preparation==1?2u:0x12u):split?input.row_class_mask:input.engine_pass==5?5u:input.engine_pass==6?8u:0x100u;result.secondary_mask=0xFF;
    if(input.model_material_mask && !(*input.model_material_mask&result.material_class_mask))return filtered(NativeModelPassReason::ModelClassMask);
    if((input.mesh_flags&input.required_mesh_mask)!=input.required_mesh_mask)return filtered(NativeModelPassReason::MeshMask);
    if(!(input.mesh_lod_mask>>result.effective_lod&1))return filtered(NativeModelPassReason::MeshLod);
    if(!(input.material_class&result.material_class_mask))return filtered(NativeModelPassReason::MaterialClass);
    if(!(input.material_secondary&result.secondary_mask))return filtered(NativeModelPassReason::MaterialSecondary);
    if(!(input.material_capabilities_first&input.group_capabilities_first) &&
       !(input.material_capabilities_second&input.group_capabilities_second))return filtered(NativeModelPassReason::Capabilities);
    result.status=NativeModelPassStatus::Eligible;
    result.pixel=SelectNativeModelPixelVariant(input.shader_pass,input.pixel);
    if(result.pixel.status==NativeModelPixelStatus::Selected)
        result.intent=ModelProgramIntent{input.shader_pass,input.layout_type,result.effective_deformation,result.pixel.variant,false,input.prefer_channel};
    return result;
}
bool ModelPreparationPairState::Begin(ModelPreparationPair candidate){
    if(!candidate.material)throw std::invalid_argument("native material preparation pair identity");
    if(current_ && *current_==candidate)return false;
    current_.reset();pending_=candidate;return true;
}
bool ModelPreparationPairState::RememberSuccess(ModelPreparationPair candidate) noexcept {
    if(!candidate.material || !pending_ || *pending_!=candidate)return false;
    current_=candidate;pending_.reset();return true;
}
}
