#include "game_ui_shaders.h"
#include "../content/acquire.h"   // C7-0 rule: in-process reads through the installed guarded copy
#include <cstring>
#include <stdexcept>

namespace ds3native::bootstrap {
namespace {
struct Descriptor {uintptr_t unused0{},unused8{};const uint8_t* uniforms{};const int8_t* semantics{};};
struct CpuWriter {
    std::array<uint8_t,0xFF8> unused_prefix{};
    uintptr_t no_vertex_gpu{};
    Descriptor* vertex{};
    uintptr_t no_pixel_gpu{};
    Descriptor* pixel{};
    Descriptor vertex_metadata,pixel_metadata;
    std::array<uintptr_t,3> table{};
    const frame::PrimitiveProgramBank* bank{};
    frame::PrimitiveProgramVersion version;
    uint64_t serial{},draw_identity{};
    uint32_t mode{},selections{},writes{};
    frame::PrimitiveProgramPair programs;
    frame::PrimitiveUniformSource values;
    UiCpuFailure failure{};
    frame::PrimitiveConstantStatus status{frame::PrimitiveConstantStatus::Missing};
    const char* data_case{};   // tranche 5: set by the selection when the answer is a case of the game's data
    [[noreturn]] void Fail(const char* reason) const noexcept {failure(reason);std::terminate();}
};
static_assert(offsetof(Descriptor,uniforms)==0x10 && offsetof(Descriptor,semantics)==0x18 &&
    offsetof(CpuWriter,no_vertex_gpu)==0xFF8 && offsetof(CpuWriter,vertex)==0x1000 && offsetof(CpuWriter,pixel)==0x1010);
uintptr_t Metadata(CpuWriter* writer){return reinterpret_cast<uintptr_t>(&writer->no_vertex_gpu);}
void DestroyCpuCallback(CpuWriter* writer){writer->Fail("native_ui_unexpected_cpu_writer_destructor");}
// ROADMAP workstream 4 step 1 (I-0137 tranche 5): a pair whose uniform layout the native values do not take (77 of the 1,316
// selectors) no longer ends the game: the pair's own descriptors are published (the CPU algorithm writes and binds through them as
// for any pair; the writes are dropped by WriteCpuCallback) and the caller skips the draw, counted (« layout_unsupported »). A
// selector without a programme pair (486) still fails here: the algorithm then binds its image at the pixel programme's row 20
// (1F784C0, 1F81EB0, unchecked), which no published descriptor can serve (the final review's H1: the game's empty row is register
// -1, refused by the image binding). The writer's own contract (a second selection, a stale or invalid bank or layout, an
// exception) fails as before.
uint8_t SelectCpuCallback(CpuWriter* writer,uint32_t selector,uintptr_t) noexcept {
    try{
        if(writer->selections++)writer->Fail("native_ui_repeated_cpu_selector");
        const auto result=frame::SelectPrimitivePrograms(*writer->bank,writer->version,writer->mode,selector,writer->programs);
        // The actual CPU algorithm dereferences installed metadata after selection
        // even if its legacy GPU-ready result is false. Publish only valid owned
        // descriptors; failed selection is raised before that dereference.
        if(result!=frame::PrimitiveProgramStatus::Ready)writer->Fail("native_ui_unavailable_cpu_selector");
        writer->status=writer->values.Begin(writer->programs,writer->serial,writer->draw_identity);
        // Only Unsupported is the data's (the last step review's L2: Missing is a layout the native's own bank did not install).
        if(writer->status!=frame::PrimitiveConstantStatus::Ready && writer->status!=frame::PrimitiveConstantStatus::Unsupported)
            writer->Fail("native_ui_unsupported_cpu_layout");
        if(writer->status==frame::PrimitiveConstantStatus::Unsupported)writer->data_case="layout_unsupported";
        writer->vertex_metadata={0,0,writer->programs.vertex_uniforms.uniforms.data(),writer->programs.vertex_uniforms.semantics.data()};
        writer->pixel_metadata={0,0,writer->programs.pixel_uniforms.uniforms.data(),writer->programs.pixel_uniforms.semantics.data()};
        writer->vertex=&writer->vertex_metadata;writer->pixel=&writer->pixel_metadata;
        return 0; // No legacy GPU wrappers/readiness, including the vertex layout.
    }catch(...){writer->Fail("native_ui_cpu_selector_exception");}
}
void WriteCpuCallback(CpuWriter* writer,uintptr_t pair,uint32_t uniform,uintptr_t source,uint32_t count,int32_t offset,uint32_t) noexcept {
    ++writer->writes;
    if(writer->status!=frame::PrimitiveConstantStatus::Ready)return;
    std::array<float,1000> current{};
    if(pair!=Metadata(writer) || !source || !count || count>current.size() || offset<0 ||
       !ds3content::Read(source,current.data(),size_t(count)*4)){
        writer->status=frame::PrimitiveConstantStatus::Invalid;return;
    }
    writer->status=writer->values.WriteUniform(uniform,{current.data(),count},static_cast<uint32_t>(offset));
}
}
void GameUiCpuShaderPolicy::Unsupported(uintptr_t profile) noexcept {
    reinterpret_cast<Profile*>(profile)->failure("unqualified_native_cpu_shader_policy");std::terminate();
}
GameUiCpuShaderPolicy::GameUiCpuShaderPolicy(uintptr_t module,UiCpuFailure failure){
    if(!module || !failure)throw std::invalid_argument("native CPU shader policy");
    table_.fill(reinterpret_cast<uintptr_t>(&Unsupported));
    table_[0x48/8]=module+0x1BDB130;table_[0x50/8]=module+0x1BDAF90;
    table_[0x60/8]=module+0x1BDB080;table_[0x68/8]=module+0x1BDDCC0;
    profile_={reinterpret_cast<uintptr_t>(table_.data()),failure};manager_[0x28/8]=reinterpret_cast<uintptr_t>(&profile_);
}
uintptr_t GameUiCpuShaderPolicy::Manager() const {
    if(thread_!=std::this_thread::get_id()){profile_.failure("native_cpu_shader_policy_thread");std::terminate();}
    return reinterpret_cast<uintptr_t>(manager_.data());
}
frame::PrimitiveConstantStatus PrepareNativeUiCpu(UiCpuPrepare producer,const frame::PrimitiveProgramBank& bank,
    frame::PrimitiveProgramVersion current,uint32_t mode,uint64_t serial,uint64_t draw_identity,
    const UiCpuPrepareInputs& inputs,UiCpuPreparedShader& output,UiCpuFailure failure,const char** data_case){
    using Status=frame::PrimitiveConstantStatus;
    if(bank.Version()!=current)return Status::Stale;
    if(mode!=3 || inputs.draw_type || inputs.meshes!=1)return Status::Unsupported;
    if(!producer || !failure || !serial || !draw_identity || !inputs.shader_manager || !inputs.fill || !inputs.vertex_format ||
       !inputs.view || !inputs.mesh_nodes)return Status::Invalid;
    uintptr_t mesh_header{},node{};uint8_t flags{};
    auto load=[&](uintptr_t address,void* target,size_t bytes){return address &&
        ds3content::Read(address,target,bytes);};
    if(!load(inputs.mesh_nodes,&mesh_header,8) || !load(mesh_header,&node,8) || !load(node+0xF,&flags,1))return Status::Invalid;
    if(flags&0x10)return Status::Unsupported;
    CpuWriter writer;writer.bank=&bank;writer.version=current;writer.mode=mode;writer.serial=serial;writer.draw_identity=draw_identity;writer.failure=failure;
    writer.table={reinterpret_cast<uintptr_t>(&DestroyCpuCallback),reinterpret_cast<uintptr_t>(&WriteCpuCallback),reinterpret_cast<uintptr_t>(&SelectCpuCallback)};
    const auto table=reinterpret_cast<uintptr_t>(writer.table.data());std::memcpy(writer.unused_prefix.data(),&table,8);
    auto current_flags=inputs.flags;
    const auto metadata=producer(inputs.shader_manager,inputs.fill,&current_flags,inputs.draw_type,inputs.vertex_format,
        inputs.meshes,inputs.view,inputs.mesh_nodes,reinterpret_cast<uintptr_t>(&writer));
    // The contract checks first (the last step review's L1): a breach is never counted as a data case.
    if(writer.selections!=1 || metadata!=Metadata(&writer) || writer.no_vertex_gpu || writer.no_pixel_gpu)return Status::Invalid;
    if(writer.status!=Status::Ready){if(data_case)*data_case=writer.data_case;return writer.status;}
    // Complex fills publish their actual mesh-local UV transforms at meshF8
    // after CPU generation. Replace the normal node-derived UV values with
    // these current authored transforms (original complex draw uniform21).
    // Matrix selection/copy is synchronous; no engine view is retained.
    if(inputs.texture_matrix_count){
        if(!inputs.texture_matrices || inputs.texture_matrix_count>2)return Status::Invalid;
        for(uint32_t texture=0;texture<inputs.texture_matrix_count;++texture){std::array<float,8> matrix{};
            if(!load(inputs.texture_matrices+texture*sizeof(matrix),matrix.data(),sizeof(matrix)))return Status::Invalid;
            const auto status=writer.values.WriteUniform(21,matrix,texture*2);if(status!=Status::Ready)return status;
        }
    }
    UiCpuPreparedShader candidate;candidate.serial=serial;candidate.draw_identity=draw_identity;
    candidate.flags=current_flags;candidate.uniform_writes=writer.writes;candidate.programs=std::move(writer.programs);
    auto status=writer.values.Resolve(current,serial,candidate.values);if(status!=Status::Ready)return status;
    status=frame::PackPrimitiveConstants(candidate.programs,candidate.values,candidate.constants);if(status!=Status::Ready)return status;
    output=std::move(candidate);return Status::Ready;
}
frame::PrimitiveConstantStatus PrepareNativeUiFilterCpu(const frame::PrimitiveProgramBank& bank,frame::PrimitiveProgramVersion current,
    uint64_t serial,uint64_t draw_identity,uint32_t selector,const std::function<void(uintptr_t,uintptr_t)>& writes,
    UiCpuPreparedShader& output,UiCpuFailure failure){
    using Status=frame::PrimitiveConstantStatus;
    if(bank.Version()!=current)return Status::Stale;
    if(!failure || !serial || !draw_identity || !writes)return Status::Invalid;
    CpuWriter writer;writer.bank=&bank;writer.version=current;writer.mode=3;writer.serial=serial;writer.draw_identity=draw_identity;writer.failure=failure;
    writer.table={reinterpret_cast<uintptr_t>(&DestroyCpuCallback),reinterpret_cast<uintptr_t>(&WriteCpuCallback),reinterpret_cast<uintptr_t>(&SelectCpuCallback)};
    const auto table=reinterpret_cast<uintptr_t>(writer.table.data());std::memcpy(writer.unused_prefix.data(),&table,8);
    // 1F778D0 selects through the writer (+10) and stops when that reports no GPU programme. Native selection never
    // reports GPU readiness; a refused CPU selection fails inside the callback instead.
    (void)SelectCpuCallback(&writer,selector,0);
    if(writer.status!=Status::Ready)return writer.status;
    writes(reinterpret_cast<uintptr_t>(&writer),Metadata(&writer));
    if(writer.status!=Status::Ready)return writer.status;
    if(writer.selections!=1 || writer.no_vertex_gpu || writer.no_pixel_gpu)return Status::Invalid;
    writer.values.CompleteWrittenRows();   // float2 writes into float4 rows; instance 0 of the batched VS 507
    UiCpuPreparedShader candidate;candidate.serial=serial;candidate.draw_identity=draw_identity;
    candidate.uniform_writes=writer.writes;candidate.programs=std::move(writer.programs);
    auto status=writer.values.Resolve(current,serial,candidate.values);if(status!=Status::Ready)return status;
    status=frame::PackPrimitiveConstants(candidate.programs,candidate.values,candidate.constants);if(status!=Status::Ready)return status;
    output=std::move(candidate);return Status::Ready;
}
}
