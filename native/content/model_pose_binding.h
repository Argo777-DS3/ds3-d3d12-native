#pragma once
#include "model_pose.h"

namespace ds3content {
enum class ModelPoseBindingKind { Unavailable, Current, Auxiliary };
struct ModelPoseBinding {
    ModelPoseBindingKind kind{ModelPoseBindingKind::Unavailable};
    const ModelPosePalette* object{};
    const ModelPosePalette* cancel{};
};
//817:64340 selects auxiliary+cancel for material+24 bit14, or for the
//ordinary deferred flag100 with material+22 and actual pose+76. The normal
//route selects current M. Caller provides explicit current material inputs;
//instanced overrides and nongated/fast families remain outside this contract.
//A normal route may supply the independently produced neutral cancel only
//when the full pose update actually established it. Unknown Q, absent data
//and a non-neutral cloth cancel never become an invented identity palette.
inline ModelPoseBinding SelectOrdinaryModelPoseBinding(const ModelPoseVersion& pose,
    bool material_cloth_enabled,bool force_auxiliary) noexcept {
    ModelPoseBinding result;
    if(!pose.gated || pose.fast || !pose.row_count || pose.row_count>4096)return result;
    if(!force_auxiliary && material_cloth_enabled && pose.compose &&
       pose.auxiliary_present && pose.reference==ModelPoseReference::Unknown)return result;
    const bool auxiliary=force_auxiliary || (material_cloth_enabled && pose.cloth);
    const auto& object=auxiliary?pose.auxiliary:pose.current;
    if(!object.Available() || object.rows.size()!=pose.row_count)return result;
    result.kind=auxiliary?ModelPoseBindingKind::Auxiliary:ModelPoseBindingKind::Current;
    result.object=&object;
    if((auxiliary || !pose.cloth) && pose.cancel.Available() && pose.cancel.rows.size()==pose.row_count)
        result.cancel=&pose.cancel;
    return result;
}
}
