#include "scene_ingress.h"
#include "../content/acquire.h"
#include "../world/producers.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace ds3native::ingress {
namespace {
// The camera writer's caller return and the receiver's vtable (DS3 1.15.2; the I-0036 camera study's pins).
constexpr uintptr_t camera_return_rva=0xE0081C,camera_vtable_rva=0x27D9F08;
template<class T> T Field(const uint8_t* bytes,size_t offset) noexcept {
    T result{};std::memcpy(&result,bytes+offset,sizeof(result));return result;
}
}
bool ProduceCameraConstants(const SceneViewInput& input,SceneCameraConstants& output) noexcept {
    SceneView view{};if(!BuildSceneView(input,view))return false;
    ds3prod::Camera camera{};
    for(size_t n=0;n<3;++n){camera.right[n]=input.camera_pose[n];camera.up[n]=input.camera_pose[4+n];
        camera.forward[n]=input.camera_pose[8+n];camera.position[n]=input.camera_pose[12+n];}
    camera.fovy=input.vertical_fov;camera.near_z=input.near_plane;camera.far_z=input.far_plane;
    camera.width=input.width;camera.height=input.height;
    camera.aspect=input.explicit_aspect?input.explicit_aspect:float(input.width)/float(input.height);
    camera.jitter[0]=input.jitter_x;camera.jitter[1]=input.jitter_y;   // I-0111: the projection words carry the upscaler's jitter
    SceneCameraConstants candidate{};candidate.serial=input.serial;
    ds3prod::UseRenderSize(input.width,input.height);
    ds3prod::ProduceSceneCamera(camera,candidate.bytes.data());
    for(const auto& field:ds3prod::scene_camera_fields)for(uint32_t offset=field.offset;offset<field.offset+field.bytes;offset+=4){
        const auto value=Field<float>(candidate.bytes.data(),offset);if(!std::isfinite(value))return false;
        candidate.written[offset/4]=1;
    }
    output=std::move(candidate);return true;
}
bool ProduceCameraViewConstants(const SceneViewInput& input,const SceneCameraConstants& camera,
    SceneViewConstants& output) noexcept {
    SceneView checked{};if(camera.serial!=input.serial || !BuildSceneView(input,checked))return false;
    float projection[16]{};
    for(size_t row=0;row<4;++row)for(size_t column=0;column<4;++column){
        const auto offset=256+4*(4*column+row);
        if(camera.written[offset/4]!=1)return false;
        projection[4*row+column]=Field<float>(camera.bytes.data(),offset);
        if(!std::isfinite(projection[4*row+column]))return false;
    }
    SceneViewConstants candidate{};candidate.serial=input.serial;
    ds3prod::ProduceViewMatrices(input.camera_pose.data(),projection,candidate.bytes.data());
    for(size_t offset=0;offset<candidate.bytes.size();offset+=4)
        if(!std::isfinite(Field<float>(candidate.bytes.data(),offset)))return false;
    output=candidate;return true;
}
bool CopyRigidPlacement(uintptr_t inputs,uintptr_t expected_model,RigidPlacement& output) noexcept {
    std::array<uint64_t,5> values{};
    if(!expected_model || !ds3content::Read(inputs,values.data(),sizeof(values)) ||
       values[3]!=expected_model || !values[4]) return false;
    std::array<uint8_t,0x88> before{},after{};
    const auto record=static_cast<uintptr_t>(values[4]);
    if(!ds3content::Read(record,before.data(),before.size())) return false;
    const auto active=Field<uint32_t>(before.data(),0x68);
    const auto count=Field<uint32_t>(before.data(),0x70);
    const auto owner=Field<uint64_t>(before.data(),0x80);
    // One acquired rigid matrix, whose CPU owner is the selected engine model.
    if(active>1 || count!=1 || owner!=expected_model) return false;
    uint8_t enabled{};uint64_t source{},again{};
    if(!ds3content::Read(owner+0x1E0,&enabled,1) || !enabled ||
       !ds3content::Read(record+0x20+8*active,&source,8) || !source) return false;
    RigidPlacement result{};result.model=expected_model;result.matrix_record=record;
    result.matrix_source=source;result.active=active;
    if(!ds3content::Read(source,result.shader_rows.data(),sizeof(result.shader_rows)) ||
       !ds3content::Read(record,after.data(),after.size()) || before!=after ||
       !ds3content::Read(record+0x20+8*active,&again,8) || again!=source ||
       !std::all_of(result.shader_rows.begin(),result.shader_rows.end(),[](float v){return std::isfinite(v);})) return false;
    // Stable metadata brackets a bounded copy, not universal writer exclusion.
    output=result;return true;
}
bool CopySceneView(uintptr_t receiver,uintptr_t camera_output,uintptr_t caller,
    uintptr_t image,uint64_t serial,SceneViewInput& output) noexcept {
    if(caller!=image+camera_return_rva)return false;
    return CopyProducedSceneView(receiver,camera_output,image,serial,output);
}
bool CopyProducedSceneView(uintptr_t receiver,uintptr_t camera_output,
    uintptr_t image,uint64_t serial,SceneViewInput& output) noexcept {
    uintptr_t vptr{};
    if(!image || !serial ||
       !ds3content::Read(receiver,&vptr,sizeof(vptr)) || vptr!=image+camera_vtable_rva) return false;
    SceneViewInput result{};result.serial=serial;std::array<uint8_t,0x24> lens{};
    if(!ds3content::Read(camera_output,result.camera_pose.data(),64) ||
       !ds3content::Read(camera_output+0x50,lens.data(),lens.size())) return false;
    result.vertical_fov=Field<float>(lens.data(),0);result.near_plane=Field<float>(lens.data(),4);
    result.far_plane=Field<float>(lens.data(),8);result.width=Field<uint32_t>(lens.data(),12);
    result.height=Field<uint32_t>(lens.data(),16);result.explicit_aspect=Field<float>(lens.data(),20);
    // 1179: +0x68 is a byte (14004B5C0 writes only that byte; +0x69..+0x6B are never written, stack padding of the
    // D1DE70 local: I-0067 static reading). Read as a word it refused the High Wall load (dirty stack after TitleStep::~).
    result.projection_flags=Field<uint8_t>(lens.data(),24);result.tile_count=Field<uint32_t>(lens.data(),32);
    SceneView view{};if(!BuildSceneView(result,view)) return false;
    output=result;return true;
}
}
