#include "game_model_selection.h"
#include <cstring>
#include <cmath>
#include <algorithm>

namespace ds3native::ingress {
namespace {
template<class T> bool Load(uintptr_t at,T& value) noexcept {
    return ds3content::Read(at,&value,sizeof(value));
}
template<class T> T Field(const std::array<uint8_t,0x48>& bytes,size_t offset) noexcept {
    T value{};std::memcpy(&value,bytes.data()+offset,sizeof(value));return value;
}
struct Membership {
    uintptr_t table{},storage{},meshes{},materials_begin{},materials_end{};
    int32_t count{};
    bool operator==(const Membership&)const=default;
};
bool ReadMembership(uintptr_t model,Membership& value) noexcept {
    return Load(model,value.table) && Load(model+0xE8,value.storage) &&
        Load(model+0xA0,value.count) && Load(model+0xA8,value.meshes) &&
        Load(model+0x1F8,value.materials_begin) && Load(model+0x200,value.materials_end);
}
}
bool ReadModelFadeState(uintptr_t entity,ModelFadeState& output) noexcept {
    return ReadModelFadeStateAt(entity,ModelFadeLayout{},output);
}
bool ReadModelFadeStateAt(uintptr_t owner,const ModelFadeLayout& layout,ModelFadeState& output) noexcept {
    if(!owner || owner>UINTPTR_MAX-0x1000)return false;
    ModelFadeState state;uint8_t suppressed{},enabled{},alternate{};
    const auto track=[&](uintptr_t at,ModelFadeTrack& value){
        return Load(at,value.current) && Load(at+4,value.previous) && Load(at+8,value.interpolation) &&
            Load(at+12,value.weight) && Load(at+24,value.current_variant) && Load(at+28,value.previous_variant);};
    const auto group=owner+layout.group;
    if(!track(owner+layout.normal,state.normal) || !track(owner+layout.alternate,state.alternate) ||
       !Load(owner+layout.geometry_weight,state.geometry_weight) ||
       (layout.suppressed>=0 && !Load(owner+static_cast<uint32_t>(layout.suppressed),suppressed)) ||
       !Load(group+0x124,state.group_opacity) || !Load(group+0x98,state.group_flags) ||
       !Load(group+0x88,enabled) || !Load(group+0x89,alternate))return false;
    state.geometry_suppressed=suppressed!=0;state.fade_enabled=enabled!=0;state.alternate_fade_enabled=alternate!=0;
    output=state;return true;
}
bool ReadModelWetDrawInputsAt(uintptr_t group,ModelWetDrawInputs& output) noexcept {
    if(!group || group>UINTPTR_MAX-0x98)return false;
    const auto entity=group-0x240;
    // Source order in the current group is b,a,c. One bounded read owns all
    // three words together; the selected version retains no borrowed address.
    std::array<float,3> words;
    if(!ds3content::Read(entity+0x2CC,words.data(),sizeof(words)))return false;
    for(const auto word:words)if(!std::isfinite(word))return false;
    output={words[1],words[0],words[2]};return true;
}
bool ReadModelWetMaterialParameters(uintptr_t material,ds3content::ModelWetMaterialParameters& output) noexcept {
    if(!material || material>UINTPTR_MAX-0x250)return false;
    std::array<uint8_t,64> bytes;
    if(!ds3content::Read(material+0x210,bytes.data(),bytes.size()))return false;
    ds3content::ModelWetMaterialParameters result;
    std::memcpy(result.diffuse.data(),bytes.data(),16);std::memcpy(result.reflectance.data(),bytes.data()+16,16);
    std::memcpy(&result.type,bytes.data()+0x30,4);std::memcpy(&result.scale,bytes.data()+0x34,4);
    if(bytes[0x38]){
        std::array<float,3> abc;
        std::memcpy(&abc[0],bytes.data()+0x20,4);std::memcpy(&abc[1],bytes.data()+0x3C,4);
        std::memcpy(&abc[2],bytes.data()+0x24,4);result.override_abc=abc;
    }
    output=result;return true;
}
bool ReadModelWetShaderCatalog(uintptr_t image,std::vector<std::wstring>& output) {
    if(!image || image>UINTPTR_MAX-0x47371E0)return false;
    std::array<std::array<wchar_t,256>,43> rows;
    if(!ds3content::Read(image+0x4731BE0,rows.data(),sizeof(rows)))return false;
    std::vector<std::wstring> result;result.reserve(rows.size());
    for(const auto& row:rows){
        const auto end=std::find(row.begin(),row.end(),L'\0');if(end==row.end())return false;
        result.emplace_back(row.begin(),end);
    }
    output=std::move(result);return true;
}
bool ReadModelDecalChoice(uintptr_t image,uintptr_t model,uintptr_t provider,
                         ModelDecalChoice& output) noexcept {
    if(!image || !model || image>UINTPTR_MAX-0x473C7E6 || model>UINTPTR_MAX-0x228)return false;
    if(provider){output=ModelDecalChoice::InstanceProvider;return true;}
    uint32_t flags{};if(!Load(model+0x224,flags))return false;
    auto choice=ModelDecalChoice::None;
    if(flags&4u){
        uint8_t enabled{};if(!Load(image+0x473C7E5,enabled))return false;
        if(enabled)choice=ModelDecalChoice::Common;
    }
    output=choice;return true;
}
bool ReadModelSplitPassSetting(uintptr_t image,bool& output) noexcept {
    if(!image || image>UINTPTR_MAX-0x473CBB0)return false;
    uint32_t setting{};if(!Load(image+0x473CBAC,setting))return false;
    output=setting==2;return true;
}
bool ReadModelParameterAnimations(uintptr_t owner,ds3content::ModelParameterAnimations& output) {
    if(!owner || owner>UINTPTR_MAX-0x10)return false;
    uintptr_t table{},begin{},end{};
    if(!Load(owner+8,table) || !table)return false;
    ds3content::ModelParameterAnimations result;
    {   // I-0120 B11: `table` is not null here (returned above)
        if(table>UINTPTR_MAX-0x10 || !Load(table,begin) || !Load(table+8,end) || end<begin ||
           (end-begin)%0x90 || (end-begin)/0x90>4096 || (end!=begin && !begin))return false;
        for(uintptr_t at=begin;at!=end;at+=0x90){
            ds3content::ModelParameterAnimationRow row;uint8_t universal{};
            if(!ds3content::Read(at,row.target.data(),16) || !Load(at+0x14,row.strength) ||
               !Load(at+0x10,row.kind) ||
               !Load(at+0x78,row.material_index) ||
               !Load(at+0x7C,row.lookup_key) || !Load(at+0x80,universal))return false;
            row.universal=universal!=0;
            uint64_t length{},capacity{};uintptr_t chars=at+0x18;
            if(!Load(at+0x28,length) || !Load(at+0x30,capacity) || length>2048 || capacity<length)return false;
            if(capacity>7 && !Load(at+0x18,chars))return false;
            row.material_name.resize(static_cast<size_t>(length));
            if(length && !ds3content::Read(chars,row.material_name.data(),static_cast<size_t>(length)*2))return false;
            result.rows.push_back(std::move(row));
        }
        uintptr_t current_begin{},current_end{};
        if(!Load(table,current_begin) || !Load(table+8,current_end) || current_begin!=begin || current_end!=end)return false;
    }
    uintptr_t current_table{};if(!Load(owner+8,current_table) || current_table!=table)return false;
    result.available=true;output=std::move(result);return true;
}
bool ReadModelPreparationFlagsAt(uintptr_t animation,CurrentModelPreparationFlags& output) noexcept {
    if(!animation || animation>UINTPTR_MAX-0x640)return false;
    uint8_t kind8_gate{},kind5_gate{};
    if(!Load(animation+0xE1,kind8_gate) || !Load(animation+0x630,kind5_gate))return false;
    output={kind8_gate==0,kind5_gate!=0};return true;
}
bool SelectModelFade(const ModelFadeState& state,uint32_t engine_pass,ModelFadeSelection& output) noexcept {
    const auto& track=engine_pass==0x39?state.alternate:state.normal;
    const float blend=track.current==track.previous?1.f:1.f-track.interpolation;
    const float weight=engine_pass==0x39?track.weight:
        (engine_pass>=1 && engine_pass<=0x38?state.geometry_weight:track.weight);
    const float first=blend*weight;
    if(!std::isfinite(blend) || !std::isfinite(weight) || !std::isfinite(first) ||
       !std::isfinite(state.group_opacity))return false;
    ModelFadeSelection selected;
    if((engine_pass>=5 && engine_pass<=8 && state.geometry_suppressed) || !(first>0.f)){
        output=selected;return true;}
    const bool adjusted=first!=1.f || track.current_variant!=track.previous_variant;
    const float opacity=adjusted?state.group_opacity*first:state.group_opacity;
    if(!std::isfinite(opacity))return false;
    selected.intents[0]={track.current_variant,0,first,opacity,
        adjusted || state.fade_enabled,adjusted || state.alternate_fade_enabled};selected.count=1;
    if(adjusted && track.current_variant!=track.previous_variant){
        // Actual source order: (base opacity * weight) * (1 - blend).
        const float second=weight*(1.f-blend),second_opacity=(state.group_opacity*weight)*(1.f-blend);
        if(!std::isfinite(second) || !std::isfinite(second_opacity))return false;
        selected.intents[1]={track.previous_variant,1,second,second_opacity,true,true};selected.count=2;
    }
    output=selected;return true;
}
bool PreferMainViewFade(uint8_t category,const ModelFadeSelection& geometry,const ModelFadeSelection& main_view,bool view_weight_fix) noexcept {
    if(!(category&6u) || !geometry.count)return main_view.count || !(category&6u);
    return view_weight_fix && main_view.count && main_view.intents[0].opacity>geometry.intents[0].opacity;
}
bool ReadCurrentModelInputs(uintptr_t image,uintptr_t entity,uintptr_t model,
                            CurrentModelInputs& output) noexcept {
    if(!entity || entity>UINTPTR_MAX-0xB08)return false;
    return ReadCurrentModelInputsAt(image,model,entity+0xA60,entity+0xA48,entity+0xA80,output);
}
bool ReadCurrentModelInputsAt(uintptr_t image,uintptr_t model,uintptr_t source_field,uintptr_t instance_field,
                              uintptr_t pose,CurrentModelInputs& output) noexcept {
    try {
        if(!image || !model || !source_field || !pose || model>UINTPTR_MAX-0x208)return false;
        Membership before{},after{};
        if(!ReadMembership(model,before) || before.table!=image+ds3content::model_vtable_rva ||
           !before.storage || !before.meshes || before.count<=0 || before.count>4096 ||
           before.materials_end<before.materials_begin ||
           (before.materials_end-before.materials_begin)%8 ||
           (before.materials_end-before.materials_begin)/8>65536)return false;
        CurrentModelInputs current;current.model=model;current.storage=before.storage;current.pose=pose;
        uintptr_t source{},source_after{},patch_after{},instance_vector{};
        if(!Load(source_field,source) || (source && !Load(source+0x50,current.patch)) ||
           (instance_field && !Load(instance_field,instance_vector)))return false;
        if(instance_vector) {
            uintptr_t begin{},end{};
            if(!Load(instance_vector,begin) || !Load(instance_vector+8,end) || end<begin ||
               (end-begin)%0x90 || (end-begin)/0x90>4096)return false;
            current.instance_streams=begin!=end;
        }
        const auto material_count=(before.materials_end-before.materials_begin)/8;
        current.meshes.reserve(before.count);
        for(int32_t ordinal=0;ordinal<before.count;++ordinal) {
            std::array<uint8_t,0x48> mesh{};
            if(!ds3content::Read(before.meshes+uint64_t(ordinal)*mesh.size(),mesh.data(),mesh.size()))return false;
            CurrentModelMesh selected;selected.ordinal=static_cast<uint32_t>(ordinal);
            selected.material_id=Field<uint16_t>(mesh,0);selected.patch_index=Field<uint16_t>(mesh,2);
            selected.layout_type=Field<uint32_t>(mesh,8);selected.flags=Field<uint32_t>(mesh,0x10);
            selected.lod_mask=Field<uint16_t>(mesh,0x14);selected.layout=Field<uintptr_t>(mesh,0x40);
            if(selected.material_id>=material_count ||
               !Load(before.materials_begin+uint64_t(selected.material_id)*8,selected.material) || !selected.material)return false;
            current.meshes.push_back(selected);
        }
        if(!ReadMembership(model,after) || before!=after || !Load(source_field,source_after) || source_after!=source ||
           (source && (!Load(source+0x50,patch_after) || patch_after!=current.patch)))return false;
        output=std::move(current);return true;
    }catch(...){return false;}
}
}
