#include "model_pose.h"
#include "../world/producers.h"
#include <limits>
#include <utility>

namespace ds3content {
namespace {
// I-0113 tranche 10d (gap 19, Iudex Gundyr's death): a non-finite row is kept, as 16000 keeps it (no finiteness test): the GPU
// drops the triangles it places and draws the rest, so a dying model fades out instead of vanishing (I-0101 marked the
// dependent palettes InvalidInput and skipped every mesh they skin).
ModelPoseStatus InputStatus(std::span<const ModelPoseRow> rows,uint32_t count) noexcept {
    return rows.size()!=count?ModelPoseStatus::MissingInput:ModelPoseStatus::Ready;
}
ModelPoseStatus Dependencies(ModelPoseStatus a,ModelPoseStatus b) noexcept {
    if(a!=ModelPoseStatus::Ready || b!=ModelPoseStatus::Ready)
        return ModelPoseStatus::MissingInput;
    return ModelPoseStatus::Ready;
}
const float (*Rows(std::span<const ModelPoseRow> rows) noexcept)[12] {
    static_assert(sizeof(ModelPoseRow)==12*sizeof(float));
    return reinterpret_cast<const float(*)[12]>(rows.data());
}
float (*Rows(std::vector<ModelPoseRow>& rows) noexcept)[12] {
    return reinterpret_cast<float(*)[12]>(rows.data());
}
void Compose(std::span<const ModelPoseRow> a,std::span<const ModelPoseRow> g,
             uint32_t count,ModelPosePalette& output) {
    output.status=Dependencies(InputStatus(a,count),InputStatus(g,count));
    if(!output.Available())return;
    output.rows.resize(count);
    ds3prod::ProduceClothCancel(Rows(a),Rows(g),count,Rows(output.rows));
}
void Copy(std::span<const ModelPoseRow> rows,uint32_t count,ModelPosePalette& output) {
    output.status=InputStatus(rows,count);
    if(output.Available())output.rows.assign(rows.begin(),rows.end());
}
void Identity(uint32_t count,ModelPosePalette& output) {
    output.rows.resize(count);ds3prod::ProduceIdentityRows(count,Rows(output.rows));
    output.status=ModelPoseStatus::Ready;
}
void SetAll(ModelPoseVersion& version,ModelPoseStatus status) noexcept {
    version.current.status=status;version.auxiliary.status=status;version.cancel.status=status;
}
}
bool ProduceModelPose(const ModelPoseInputs& inputs,ModelPoseVersion& output) noexcept {
    if(inputs.row_count>4096 || inputs.current.size()>4096 ||
       inputs.reference_rows.size()>4096 || inputs.gate_rows.size()>4096)return false;
    switch(inputs.reference) {
    case ModelPoseReference::Unknown:case ModelPoseReference::Null:
        if(!inputs.reference_rows.empty())return false;
        break;
    case ModelPoseReference::Present:break;
    default:return false;
    }
    if(!inputs.gate_available && !inputs.gate_rows.empty())return false;
    try {
        ModelPoseVersion next;
        next.row_count=inputs.row_count;next.gated=inputs.gated;next.compose=inputs.compose;
        next.auxiliary_present=inputs.auxiliary_present;next.fast=inputs.fast;
        next.gate_available=inputs.gate_available;next.reference=inputs.reference;
        next.current_source.assign(inputs.current.begin(),inputs.current.end());
        next.reference_source.assign(inputs.reference_rows.begin(),inputs.reference_rows.end());
        next.gate_source.assign(inputs.gate_rows.begin(),inputs.gate_rows.end());
        if(!inputs.gated)SetAll(next,ModelPoseStatus::UnsupportedNongated);
        else if(inputs.fast)SetAll(next,ModelPoseStatus::UnsupportedFast);
        else if(!inputs.row_count)SetAll(next,ModelPoseStatus::Empty);
        else {
            const uint32_t count=inputs.row_count;
            if(!inputs.compose)Copy(inputs.current,count,next.current);
            else if(!inputs.gate_available)
                next.current.status=Dependencies(InputStatus(inputs.current,count),ModelPoseStatus::MissingInput);
            else Compose(inputs.current,inputs.gate_rows,count,next.current);
            if(!inputs.auxiliary_present) {
                next.auxiliary.status=ModelPoseStatus::Absent;
                next.cancel.status=ModelPoseStatus::Absent;
            }else if(!inputs.compose) {
                next.auxiliary=next.current;Identity(count,next.cancel);
            }else if(inputs.reference==ModelPoseReference::Null) {
                next.auxiliary=next.current;Identity(count,next.cancel);
            }else if(inputs.reference==ModelPoseReference::Present) {
                next.cloth=true;
                next.auxiliary.status=Dependencies(InputStatus(inputs.current,count),
                                                   InputStatus(inputs.reference_rows,count));
                if(next.auxiliary.Available()) {
                    next.auxiliary.rows.resize(count);
                    ds3prod::ProduceClothPalette(Rows(inputs.current),Rows(inputs.reference_rows),
                                                count,Rows(next.auxiliary.rows));
                    //179C0's exact inverse divides by its float determinant; a singular reference's non-finite rows are kept,
                    // as the original keeps them (I-0113 tranche 10d).
                }
                if(inputs.gate_available)
                    Compose(inputs.reference_rows,inputs.gate_rows,count,next.cancel);
                else next.cancel.status=Dependencies(InputStatus(inputs.reference_rows,count),
                                                     ModelPoseStatus::MissingInput);
            }
        }
        output=std::move(next);return true;
    }catch(...){return false;}
}
bool ProduceEffectModelPalettes(bool full_rows,std::span<const ModelPoseRow> a,std::span<const ModelPoseRow> g,
    std::span<const EffectModelMesh> meshes,std::vector<EffectMeshPalette>& out,std::vector<ModelPoseRow>& composed,const char*& reason) {
    try {
        out.assign(meshes.size(),{});composed.clear();
        const auto compose=[&](size_t k,ModelPoseRow& row){
            if(k>=g.size()){reason="effect_model_bind_rows_unreadable";return false;}
            ds3prod::ProduceClothCancel(reinterpret_cast<const float(*)[12]>(a[k].data()),reinterpret_cast<const float(*)[12]>(g[k].data()),1,
                reinterpret_cast<float(*)[12]>(row.data()));return true;};
        for(size_t i=0;i<meshes.size();++i){const auto& mesh=meshes[i];auto& palette=out[i];
            if(full_rows){if(!mesh.dynamic)continue;palette.kind=EffectMeshPaletteKind::Composed;
                if(composed.empty() && !a.empty()){composed.resize(a.size());for(size_t k=0;k<a.size();++k)if(!compose(k,composed[k]))return false;}
                continue;}
            palette.kind=EffectMeshPaletteKind::Own;
            if(!mesh.dynamic && mesh.bones.empty()){
                if(mesh.default_bone>=a.size()){reason="effect_model_bone_out_of_range";return false;}
                palette.rows.push_back(a[mesh.default_bone]);continue;}
            for(const auto k:mesh.bones){if(k<0)break;
                if(size_t(k)>=a.size()){reason="effect_model_bone_out_of_range";return false;}
                ModelPoseRow row=a[size_t(k)];if(mesh.dynamic && !compose(size_t(k),row))return false;palette.rows.push_back(row);}}
        return true;
    }catch(...){reason="effect_model_palette_allocation";return false;}
}
std::shared_ptr<const ModelPoseVersion> ModelPoseOwner::Acquire() const {
    const std::lock_guard hold(lock_);return withdrawn_?nullptr:current_;
}
std::shared_ptr<const ModelPoseVersion> ModelPoseOwner::Update(const ModelPoseInputs& inputs) noexcept {
    try {
        const std::lock_guard hold(lock_);
        if(withdrawn_ || inputs.row_count!=row_count_ || row_count_>4096 ||
           (current_ && current_->generation==std::numeric_limits<uint64_t>::max()))return {};
        auto next=std::make_shared<ModelPoseVersion>();
        if(!ProduceModelPose(inputs,*next))return {};
        next->generation=current_?current_->generation+1:1;
        current_=next;return next;
    }catch(...){return {};}
}
void ModelPoseOwner::Withdraw() {
    const std::lock_guard hold(lock_);withdrawn_=true;current_.reset();
}
}
