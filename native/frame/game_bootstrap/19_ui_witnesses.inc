// UI reference witnesses: the UI factory, processed primitives, CPU UI selection and uniforms, mesh writes, image decode
// and delivery, glyph updates and their ABI probes. Part of game_bootstrap.cpp's single translation unit (the order is
// there): it relies on every earlier part (18_resource_admission.inc before it).
#pragma once
#include "18_resource_admission.inc"
namespace {
void __fastcall ObserveUiFactory(uintptr_t manager) noexcept {
    const auto call=InterlockedIncrement64(&ui_factory_calls);
    const auto return_address=reinterpret_cast<uintptr_t>(_ReturnAddress());
    auto snapshot=[&](const char* edge){
        uintptr_t manager_table{},file_opener{},wrapper{},hal{},loader{},wrapper_hal{},hal_table{};
        const bool read=Read(manager,&manager_table,8) && Read(manager+0x968,&file_opener,8) &&
            Read(manager+0x978,&wrapper,8) && Read(manager+0x980,&hal,8) && Read(manager+0x988,&loader,8);
        if(wrapper)Read(wrapper+8,&wrapper_hal,8);if(hal)Read(hal,&hal_table,8);
        auto rva=[](uintptr_t p){return static_cast<unsigned long long>(p>=image && p-image<0x5000000?p-image:0);};
        char row[768]{};sprintf_s(row,"{\"event\":\"reference_game_ui_factory\",\"call\":%lld,\"edge\":\"%s\",\"thread\":%lu,\"tick\":%llu,\"receiver\":%llu,\"read\":%s,\"return_rva\":%llu,\"manager_vtable_rva\":%llu,\"file_opener\":%llu,\"wrapper\":%llu,\"hal\":%llu,\"loader\":%llu,\"wrapper_hal_matches\":%s,\"hal_vtable_rva\":%llu,\"phase_calls\":%lld,\"original_gpu_payload_read\":false,\"native_game_frame\":false}\n",
            call,edge,GetCurrentThreadId(),GetTickCount64(),static_cast<unsigned long long>(manager),read?"true":"false",rva(return_address),rva(manager_table),
            static_cast<unsigned long long>(file_opener),static_cast<unsigned long long>(wrapper),static_cast<unsigned long long>(hal),
            static_cast<unsigned long long>(loader),hal && wrapper_hal==hal?"true":"false",rva(hal_table),control->phase_calls);Log(row);
    };
    if(call<=4){
        snapshot("enter");
        MEMORY_BASIC_INFORMATION memory{};
        if(return_address>=image+160 && return_address<image+0x5000000 &&
           VirtualQuery(reinterpret_cast<void*>(return_address),&memory,sizeof(memory)) &&
           (memory.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY))){
            std::array<uint8_t,320> code{};
            if(Read(return_address-160,code.data(),code.size())){
                char hex[641]{},row[928]{};for(size_t n=0;n<code.size();++n)sprintf_s(hex+n*2,3,"%02x",code[n]);
                sprintf_s(row,"{\"event\":\"reference_game_ui_factory_return_code\",\"call\":%lld,\"rva\":%llu,\"bytes\":320,\"cpu_code_only\":true,\"hex\":\"%s\"}\n",
                    call,static_cast<unsigned long long>(return_address-160-image),hex);Log(row);
            }
        }
    }
    // Separate Original run only. This mixed factory body is never installed
    // as a native implementation; preserve real observation behavior unchanged.
    ui_factory_original(manager);
    if(call<=4)snapshot("leave");
}
void __fastcall ObserveProcessedPrimitive(void* hal,void* primitive,uintptr_t begin,uintptr_t end) noexcept {
    const auto count=InterlockedIncrement64(&processed_primitive_calls);
    if(count<=8 || (count<=4096 && count%128==0)) {
        const auto h=reinterpret_cast<uintptr_t>(hal),p=reinterpret_cast<uintptr_t>(primitive);
        uintptr_t vtable{},prepare{},emit{},emit_instanced{},head{};uint32_t mode{},records{},display_flags{};
        // Separate original observer: read only prepared CPU transaction shape
        // and dispatch CODE addresses. No original GPU resource or bytes are
        // sampled, and this entry is never a native producer/forwarding path.
        const bool ok=Read(h,&vtable,8) && Read(vtable+0x318,&prepare,8) &&
            Read(vtable+0x348,&emit,8) && Read(vtable+0x350,&emit_instanced,8) &&
            // Source1F741F0/1F7BFF0 assembly: flags are H58, not adjacent H5C.
            Read(h+0x39F18,&mode,4) && Read(p+0x40,&records,4) && Read(p+0x28,&head,8) && Read(h+0x58,&display_flags,4);
        char row[512]{};sprintf_s(row,"{\"event\":\"reference_processed_primitive\",\"call\":%lld,\"thread\":%lu,\"tick\":%llu,\"read\":%s,\"hal_vtable_rva\":%llu,\"prepare_rva\":%llu,\"emit_rva\":%llu,\"instanced_rva\":%llu,\"mode\":%u,\"records\":%u,\"display_flags\":%u,\"explicit_begin\":%s,\"default_head\":%s,\"end\":%s}\n",
            count,GetCurrentThreadId(),GetTickCount64(),ok?"true":"false",
            static_cast<unsigned long long>(vtable>=image?vtable-image:0),
            static_cast<unsigned long long>(prepare>=image?prepare-image:0),
            static_cast<unsigned long long>(emit>=image?emit-image:0),
            static_cast<unsigned long long>(emit_instanced>=image?emit_instanced-image:0),
            mode,records,display_flags,begin?"true":"false",head?"true":"false",end?"true":"false");Log(row);
    }
    processed_primitive_original(hal,primitive,begin,end);
}
bool ObserveCpuVertexFormat(uintptr_t format,LONG64 call,const char* role,bootstrap::CpuPrimitiveVertexFormat& owned) {
    struct Format {uint32_t stride,padding;uintptr_t elements;} layout{};
    if(!Read(format,&layout,sizeof(layout)) || !layout.stride || layout.stride>4096 || !layout.elements)return false;
    owned.stride=layout.stride;
    char row[256]{};
    sprintf_s(row,"{\"event\":\"reference_cpu_vertex_format\",\"call\":%lld,\"role\":\"%s\",\"stride\":%u}\n",call,role,layout.stride);Log(row);
    for(uint32_t index=0;index<16;++index) {
        uint32_t element[2]{};
        if(!Read(layout.elements+index*8,element,sizeof(element)))return false;
        if(!element[1])return true;
        owned.elements.push_back({element[0],element[1]});
        sprintf_s(row,"{\"event\":\"reference_cpu_vertex_element\",\"call\":%lld,\"role\":\"%s\",\"element\":%u,\"offset\":%u,\"attributes\":%u}\n",call,role,index,element[0],element[1]);Log(row);
    }
    return false;
}
void ObserveLockedCpuMeshInputs(void* cache,void* item,LONG64 call) {
    // At the writer's +68 call,1CF8750 has locked/generated CPU staging.
    // Observe that upstream family, before any original Map. No destination,
    // original buffer contents or graphics objects are read here.
    const auto c=reinterpret_cast<uintptr_t>(cache),g=reinterpret_cast<uintptr_t>(item);
    const auto context=cpu_mesh_context;
    struct MeshSet {uintptr_t data;uint32_t count,padding;uint64_t stride;} set{};
    uintptr_t primitive{},fill{},source_format{},destination_format{},staging{};
    uint32_t vertices{},indices{},mesh_count{};
    bootstrap::CpuPrimitiveVertexFormat source_layout,destination_layout;
    struct OwnedMesh {uint32_t vertices{},indices{};std::vector<uint8_t> vertex_bytes,index_bytes;};
    std::vector<OwnedMesh> owned_meshes;
    bool ok=context.cache==c && context.batch && context.meshes &&
        Read(context.meshes,&set,sizeof(set)) && set.count && set.count<=256 && set.stride>=8 && set.stride<=4096 &&
        Read(context.batch+0x40,&primitive,8) && Read(primitive+0x18,&fill,8) &&
        Read(fill+0x30,&source_format,8) && Read(context.batch+0x30,&destination_format,8) &&
        Read(c+0x68,&staging,8) && staging && Read(g+0x58,&mesh_count,4) &&
        Read(g+0x5C,&vertices,4) && Read(g+0x60,&indices,4);
    uint64_t total_vertices{},total_indices{};char row[384]{};
    if(ok)ok=ObserveCpuVertexFormat(source_format,call,"source",source_layout) && ObserveCpuVertexFormat(destination_format,call,"destination",destination_layout);
    for(uint32_t ordinal=0;ok && ordinal<set.count;++ordinal) {
        uintptr_t mesh{};uint64_t allocated{},vertex_offset{},index_offset{};uint32_t locks{},v{},i{};
        ok=Read(set.data+ordinal*set.stride,&mesh,8) && Read(mesh+0x28,&allocated,8) && allocated &&
            Read(mesh+0x30,&vertex_offset,8) && Read(mesh+0x38,&index_offset,8) &&
            Read(mesh+0x40,&locks,4) && locks && Read(mesh+0x44,&v,4) && Read(mesh+0x48,&i,4);
        if(!ok)break;
        const auto source_bytes=uint64_t(v)*source_layout.stride,index_bytes=uint64_t(i)*2;
        // M+28 is the locked staging allocation's byte charge, not a GPU
        // address. Bound both source ranges within that mesh's CPU block.
        if(allocated>(16u<<20) || vertex_offset>(16u<<20) || index_offset<vertex_offset ||
            source_bytes>allocated || index_offset-vertex_offset>allocated ||
            index_bytes>allocated-(index_offset-vertex_offset) || !v || v>65536 ||
            vertex_offset>UINTPTR_MAX-staging || index_offset>UINTPTR_MAX-staging) {ok=false;break;}
        OwnedMesh owned{v,i,std::vector<uint8_t>(static_cast<size_t>(source_bytes)),std::vector<uint8_t>(static_cast<size_t>(index_bytes))};
        if(!Read(staging+vertex_offset,owned.vertex_bytes.data(),owned.vertex_bytes.size()) ||
            (index_bytes && !Read(staging+index_offset,owned.index_bytes.data(),owned.index_bytes.size()))) {ok=false;break;}
        owned_meshes.push_back(std::move(owned));
        total_vertices+=v;total_indices+=i;
        sprintf_s(row,"{\"event\":\"reference_locked_cpu_mesh\",\"call\":%lld,\"ordinal\":%u,\"staging_allocation\":%llu,\"vertex_offset\":%llu,\"index_offset\":%llu,\"locks\":%u,\"vertices\":%u,\"indices\":%u}\n",call,ordinal,allocated,vertex_offset,index_offset,locks,v,i);Log(row);
    }
    if(ok)ok=mesh_count==set.count && vertices==total_vertices && indices==total_indices;
    sprintf_s(row,"{\"event\":\"reference_cpu_mesh_source\",\"call\":%lld,\"thread\":%lu,\"read\":%s,\"meshes\":%u,\"vertices\":%u,\"indices\":%u,\"source_vertex_total\":%llu,\"source_index_total\":%llu,\"gpu_bytes_read\":false}\n",call,GetCurrentThreadId(),ok?"true":"false",set.count,vertices,indices,total_vertices,total_indices);Log(row);
    if(!ok)return;
    std::vector<bootstrap::CpuStagedPrimitiveMesh> meshes;
    for(const auto& mesh:owned_meshes)meshes.push_back({mesh.vertices,mesh.indices,mesh.vertex_bytes,mesh.index_bytes});
    bootstrap::PrimitiveGeometryStorage storage;
    const auto geometry=bootstrap::ProduceStagedPrimitiveGeometry(storage,g,source_layout,destination_layout,meshes);
    // Separate reference: independently run only the qualified original CPU
    // conversion/rebase algorithms into OTHER owned CPU vectors. These never
    // become native inputs and are never original GPU mapping destinations.
    auto source_elements=source_layout.elements,destination_elements=destination_layout.elements;
    source_elements.push_back({});destination_elements.push_back({});
    struct CpuFormat {uint32_t stride,padding;const bootstrap::CpuPrimitiveVertexElement* elements;};
    const CpuFormat source{source_layout.stride,0,source_elements.data()},destination{destination_layout.stride,0,destination_elements.data()};
    std::vector<uint8_t> reference_vertices(geometry->vertices.bytes.size()),reference_indices(geometry->indices.bytes.size());
    using Convert=void(__fastcall*)(const CpuFormat*,const void*,const CpuFormat*,void*,uint32_t,const void*);
    using Rebase=void(__fastcall*)(void*,const void*,uint32_t,uint16_t);
    size_t vertex_base{},index_base{};
    for(size_t ordinal=0;ordinal<meshes.size();++ordinal) {
        const auto mesh_ordinal=static_cast<uint8_t>(ordinal);const auto* ordinal_address=&mesh_ordinal;const auto& mesh=meshes[ordinal];
        reinterpret_cast<Convert>(image+0x1CF2170)(&source,mesh.vertex_bytes.data(),&destination,
            reference_vertices.data()+vertex_base*destination.stride,mesh.vertices,&ordinal_address);
        reinterpret_cast<Rebase>(image+0x1CF1FC0)(reference_indices.data()+index_base*2,mesh.index_bytes.data(),mesh.indices,static_cast<uint16_t>(vertex_base));
        vertex_base+=mesh.vertices;index_base+=mesh.indices;
    }
    const bool equal=geometry->vertices.bytes==reference_vertices && geometry->indices.bytes==reference_indices;
    sprintf_s(row,"{\"event\":\"reference_native_cpu_geometry\",\"call\":%lld,\"vertices\":%u,\"indices\":%u,\"vertex_bytes\":%zu,\"index_bytes\":%zu,\"cpu_reference_exact\":%s,\"gpu_bytes_read\":false,\"source_retained\":false}\n",call,geometry->vertex_count,geometry->index_count,geometry->vertices.bytes.size(),geometry->indices.bytes.size(),equal?"true":"false");Log(row);
}
void __fastcall ObserveCpuMeshWrite(void* cache,uintptr_t context,uintptr_t batch,uintptr_t meshes,BYTE wait) noexcept {
    static volatile LONG64 provider_calls{};
    const auto call=InterlockedIncrement64(&provider_calls);
    if(call<=8){
        // Reference-only upstream identity observation, before CPU staging or
        // original Map. This does not invoke a provider or read GPU data.
        struct Set {uintptr_t data;uint32_t count,pad;uint64_t stride;} set{};
        const bool set_ok=Read(meshes,&set,sizeof(set)) && set.count && set.count<=24 && set.stride>=8 && set.stride<=4096;
        if(set_ok)for(uint32_t n=0;n<set.count;++n){
            uintptr_t mesh{},mesh_table{},provider{},table{},generate{},invalidate{};uint64_t stage{};
            const bool ok=Read(set.data+size_t(n)*set.stride,&mesh,8) && Read(mesh,&mesh_table,8) &&
                Read(mesh+0x28,&stage,8) && Read(mesh+0x50,&provider,8) && Read(provider,&table,8) &&
                Read(table+0x18,&generate,8) && Read(table+0x20,&invalidate,8);
            uintptr_t delegate{},delegate_table{},leaf{};
            // Only the qualified1C68640 CPU delegation shape is followed.
            const bool nested=ok && table==image+0x3AE0C90 && generate==image+0x1C68640 &&
                Read(provider+0x20,&delegate,8) && Read(delegate+0x10,&delegate_table,8) && Read(delegate_table+0x18,&leaf,8);
            auto rva=[](uintptr_t p){return static_cast<unsigned long long>(p>=image && p-image<0x5000000?p-image:0);};
            char row[640]{};sprintf_s(row,"{\"event\":\"reference_cpu_mesh_provider_identity\",\"call\":%lld,\"ordinal\":%u,\"read\":%s,\"mesh_vtable_rva\":%llu,\"provider_vtable_rva\":%llu,\"generate_rva\":%llu,\"invalidate_rva\":%llu,\"nested_read\":%s,\"nested_cpu_vtable_rva\":%llu,\"nested_generate_rva\":%llu,\"staging_present\":%s,\"gpu_input\":false}\n",
                call,n,ok?"true":"false",rva(mesh_table),rva(table),rva(generate),rva(invalidate),nested?"true":"false",rva(delegate_table),rva(leaf),stage?"true":"false");Log(row);
        }
    }
    const auto previous=cpu_mesh_context;
    cpu_mesh_context={reinterpret_cast<uintptr_t>(cache),batch,meshes};
    cpu_mesh_write_original(cache,context,batch,meshes,wait);
    cpu_mesh_context=previous;
}
uintptr_t __fastcall ObserveCpuRawImage(uint32_t format,int mips,uintptr_t size,uint32_t use,uintptr_t heap,uintptr_t palette) noexcept {
    const auto result=cpu_raw_image_original(format,mips,size,use,heap,palette);
    //1BCE9C0 allocates CPU image data, never maps an original GPU texture.
    // Retain the genuine CPU owner across this synchronous decode scope so a
    // temporary's early retirement cannot turn its address into a stale range.
    if(result && cpu_decode_scope)try {
        CpuImageData data{};uintptr_t vtable{},association{};
        if(Read(result,&vtable,8) && vtable==image+0x3AD5E80 && Read(result+0x10,&association,8) && !association &&
            Read(result+0x28,&data,sizeof(data)) && data.planes && data.planes<=56 && data.table) {
            using Retain=int(__fastcall*)(uintptr_t);
            reinterpret_cast<Retain>(image+0x1BB6C10)(result);
            std::shared_ptr<void> owner(reinterpret_cast<void*>(result),[](void* p){if(!native_process_terminating.load(std::memory_order_acquire))reinterpret_cast<Phase>(image+0x1BB6D00)(p);});
            cpu_decode_scope->owners.push_back(std::move(owner));
            for(uint16_t plane=0;plane<data.planes;++plane) {
                CpuImagePlane allocation{};
                if(!Read(data.table+size_t(plane)*32,&allocation,32) || !allocation.data || !allocation.bytes ||
                    allocation.bytes>(256u<<20) || allocation.bytes>UINTPTR_MAX-allocation.data)break;
                cpu_decode_scope->allocations.push_back(allocation);
            }
        }
    }catch(...){Log("{\"event\":\"reference_cpu_image_allocation_failure\"}\n");}
    return result;
}
void __fastcall ObserveCpuImageDecode(void* texture) noexcept {
    const auto count=InterlockedIncrement64(&cpu_decode_calls);
    CpuDecodeScope local{count,{},{}};auto* previous=cpu_decode_scope;
    cpu_decode_scope=count<=32?&local:nullptr;
    if(count<=32) {
        const auto p=reinterpret_cast<uintptr_t>(texture);uintptr_t source{},vtable{},decoder{},type_query{};
        uint32_t format{};uint8_t mips{};uint16_t use{};
        const bool ok=Read(p+0x30,&source,8) && Read(source,&vtable,8) && Read(vtable+0x40,&decoder,8) &&
            Read(vtable+0x18,&type_query,8) && Read(p+0x44,&mips,1) && Read(p+0x46,&use,2);
        if(ok) {
            uintptr_t format_wrapper{},format_table{};
            if(Read(p+0x58,&format_wrapper,8) && Read(format_wrapper+8,&format_table,8))Read(format_table,&format,4);
        }
        char row[384]{};sprintf_s(row,"{\"event\":\"reference_cpu_image_decode\",\"call\":%lld,\"read\":%s,\"format\":%u,\"mips\":%u,\"use\":%u,\"decoder_rva\":%llu,\"type_query_rva\":%llu}\n",
            count,ok?"true":"false",format,mips,use,static_cast<unsigned long long>(decoder>=image?decoder-image:0),static_cast<unsigned long long>(type_query>=image?type_query-image:0));Log(row);
    }
    cpu_image_decode_original(texture);cpu_decode_scope=previous;
}
void ObserveCpuImageDelivery(uintptr_t input) {
    CpuImageData data{};
    if(!cpu_decode_scope || cpu_decode_scope->allocations.empty() || !Read(input,&data,sizeof(data)) ||
        !data.mips || data.mips>14 || !data.planes || data.planes>14 || !data.table)return;
    // Restrict the descriptor table to readable memory before the qualified
    // pure CPU mip helper. Every payload must belong to an actual current
    //1BCE9C0 CPU allocation. Original GPU Map destinations are never admitted.
    std::array<CpuImagePlane,14> descriptors{};
    if(!Read(data.table,descriptors.data(),size_t(data.planes)*32))return;
    using GetPlane=void(__fastcall*)(const CpuImageData*,uint32_t,CpuImagePlane*);
    std::vector<std::vector<uint8_t>> owned;std::vector<CpuImagePlane> planes;
    uint64_t total{};
    for(uint32_t mip=0;mip<data.mips;++mip) {
        CpuImagePlane plane{};reinterpret_cast<GetPlane>(image+0x1BD0140)(&data,mip,&plane);
        if(!plane.data || !plane.bytes || plane.bytes>(256u<<20)-total || plane.bytes>UINTPTR_MAX-plane.data)return;
        bool cpu_owned{};
        for(const auto& allocation:cpu_decode_scope->allocations)
            if(plane.data>=allocation.data && plane.data-allocation.data<=allocation.bytes &&
                plane.bytes<=allocation.bytes-(plane.data-allocation.data)) {cpu_owned=true;break;}
        if(!cpu_owned)return;
        total+=plane.bytes;std::vector<uint8_t> bytes(static_cast<size_t>(plane.bytes));
        if(!Read(plane.data,bytes.data(),bytes.size()))return;
        planes.push_back(plane);owned.push_back(std::move(bytes));
    }
    std::vector<bootstrap::CpuPrimitiveImageMip> mips;
    for(size_t mip=0;mip<planes.size();++mip) {
        const auto& plane=planes[mip];
        // The helper's DataSize may include following contiguous mips. Pass
        // only this mip's physical extent to the native producer.
        const auto format=data.format&0xFFF;const auto rows=(format>=50 && format<=53)?std::max(1u,(plane.height+3)/4):plane.height;
        if(!rows || plane.pitch>(256u<<20) || plane.pitch*rows>owned[mip].size())return;
        mips.push_back({plane.width,plane.height,plane.pitch,std::span(owned[mip]).first(static_cast<size_t>(plane.pitch*rows))});
    }
    const auto delivery=InterlockedIncrement64(&cpu_image_deliveries);
    frame::PrimitiveTexture native;
    const bool produced=bootstrap::ProduceCpuPrimitiveImage(static_cast<uint64_t>(delivery),data.format,mips,native);
    char row[384]{};sprintf_s(row,"{\"event\":\"reference_native_cpu_image\",\"call\":%lld,\"delivery\":%lld,\"format\":%u,\"mips\":%u,\"width\":%u,\"height\":%u,\"produced\":%s,\"current_cpu_allocation\":true,\"gpu_bytes_read\":false,\"source_retained\":false}\n",
        cpu_decode_scope->call,delivery,data.format,data.mips,planes[0].width,planes[0].height,produced?"true":"false");Log(row);
}
void __fastcall ObserveCpuImageUpload(void* texture,uintptr_t input) noexcept {
    try{ObserveCpuImageDelivery(input);}catch(const std::exception& error) {
        char row[384]{};sprintf_s(row,"{\"event\":\"reference_cpu_image_delivery_failure\",\"message\":\"%s\"}\n",error.what());Log(row);
    }
    cpu_image_delivery_original(texture,input);
}
uint64_t __fastcall ObserveCpuImageUpdate(void* texture,uintptr_t input,uint32_t count,uint32_t mip) noexcept {
    const auto call=InterlockedIncrement64(&cpu_image_update_calls);
    if(call<=8) {
        const auto p=reinterpret_cast<uintptr_t>(texture);uintptr_t wrapper{},format_table{};uint32_t format{},width{},height{};
        const bool metadata=Read(p+0x58,&wrapper,8) && Read(wrapper+8,&format_table,8) && Read(format_table,&format,4) &&
            Read(p+0x38,&width,4) && Read(p+0x3C,&height,4);
        char row[512]{};sprintf_s(row,"{\"event\":\"reference_cpu_image_update\",\"call\":%lld,\"read\":%s,\"format\":%u,\"width\":%u,\"height\":%u,\"records\":%u,\"mip\":%u,\"payload_read\":false}\n",call,metadata?"true":"false",format,width,height,count,mip);Log(row);
        if(count<=4096)for(uint32_t ordinal=0;ordinal<std::min(count,4u);++ordinal) {
            CpuImageUpdateRecord record{};const bool read=Read(input+size_t(ordinal)*56,&record,sizeof(record));
            sprintf_s(row,"{\"event\":\"reference_cpu_image_update_record\",\"call\":%lld,\"ordinal\":%u,\"read\":%s,\"width\":%u,\"height\":%u,\"pitch\":%llu,\"bytes\":%llu,\"source_present\":%s,\"x1\":%d,\"y1\":%d,\"x2\":%d,\"y2\":%d,\"plane\":%u}\n",call,ordinal,read?"true":"false",record.source.width,record.source.height,record.source.pitch,record.source.bytes,record.source.data?"true":"false",record.x1,record.y1,record.x2,record.y2,record.plane);Log(row);
        }
        void* callers[12]{};const auto frames=CaptureStackBackTrace(1,12,callers,nullptr);
        for(USHORT frame=0;frame<frames;++frame) {
            const auto address=reinterpret_cast<uintptr_t>(callers[frame]);
            sprintf_s(row,"{\"event\":\"reference_cpu_image_update_caller\",\"call\":%lld,\"frame\":%u,\"rva\":%llu}\n",call,frame,static_cast<unsigned long long>(address>=image && address-image<0x5000000?address-image:0));Log(row);
        }
    }
    return cpu_image_update_original(texture,input,count,mip);
}
void __fastcall ObserveMeshWriteDestinations(void* cache,void* item,uintptr_t* vertices,uintptr_t* indices) noexcept {
    const auto count=InterlockedIncrement64(&mesh_write_calls);
    void* callers[16]{};USHORT frames{};
    if(count<=8) {
        frames=CaptureStackBackTrace(1,16,callers,nullptr);
        try{ObserveLockedCpuMeshInputs(cache,item,count);}
        catch(const std::exception& error) {
            char row[384]{};sprintf_s(row,"{\"event\":\"reference_cpu_geometry_failure\",\"call\":%lld,\"message\":\"%s\"}\n",count,error.what());Log(row);
        }
    }
    mesh_write_original(cache,item,vertices,indices);
    if(count<=8) {
        const auto g=reinterpret_cast<uintptr_t>(item);
        uint64_t vertex_offset{},vertex_bytes{},index_offset{},index_bytes{};uintptr_t v{},i{};
        // Reference-only CPU allocation contract: destinations are observed for
        // presence, never dereferenced as original GPU inputs or copied for replay.
        const bool ok=Read(g+0x78,&vertex_offset,8) && Read(g+0x80,&vertex_bytes,8) &&
            Read(g+0x88,&index_offset,8) && Read(g+0x90,&index_bytes,8) &&
            Read(reinterpret_cast<uintptr_t>(vertices),&v,8) && Read(reinterpret_cast<uintptr_t>(indices),&i,8);
        char row[384]{};sprintf_s(row,"{\"event\":\"reference_mesh_write\",\"call\":%lld,\"thread\":%lu,\"read\":%s,\"vertex_offset\":%llu,\"vertex_bytes\":%llu,\"index_offset\":%llu,\"index_bytes\":%llu,\"vertex_destination\":%s,\"index_destination\":%s,\"stack_frames\":%u}\n",
            count,GetCurrentThreadId(),ok?"true":"false",vertex_offset,vertex_bytes,index_offset,index_bytes,v?"true":"false",i?"true":"false",frames);Log(row);
        for(USHORT frame=0;frame<frames;++frame) {
            const auto address=reinterpret_cast<uintptr_t>(callers[frame]);
            const auto rva=address>=image && address-image<0x5000000?address-image:0;
            sprintf_s(row,"{\"event\":\"reference_mesh_write_caller\",\"call\":%lld,\"frame\":%u,\"rva\":%llu}\n",count,frame,static_cast<unsigned long long>(rva));Log(row);
        }
    }
}
}  // namespace
