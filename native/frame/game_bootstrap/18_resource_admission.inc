// Resource admission: shaders, effects, materials, model layouts and materials, the model resolver, gparam files and their
// bindings, the area controller, zone retirement and the effect request door. Part of game_bootstrap.cpp's single
// translation unit (the order is there): it relies on every earlier part (17_render_step.inc before it).
#pragma once
#include "17_render_step.inc"
namespace {
void __fastcall RetireShader(void* wrapper) noexcept {
    const auto receiver=reinterpret_cast<uintptr_t>(wrapper);
    std::lock_guard<std::mutex> hold(shader_lock);
    uintptr_t handle{};
    if(!Read(receiver+0x10,&handle,8) || handle) Refuse("unexpected_legacy_shader_handle");
    shaders.erase(receiver);shader_registry_changes.fetch_add(1,std::memory_order_acq_rel);BumpProgramRegistry();
    // Native bytecode resides solely in the owned registry. The original GPU
    // handle slot remains null; no synthetic D3D11 object is published.
    *reinterpret_cast<uint32_t*>(receiver+0xC)=0;
}
bool __fastcall AdmitShader(void* wrapper,const void* bytes,size_t size,uintptr_t) noexcept {
    try {
        if(!wrapper || !bytes || size<32 || size>16u*1024u*1024u) Refuse("shader_input_extent");
        NativeShader program; program.bytes.resize(size);
        if(!Read(reinterpret_cast<uintptr_t>(bytes),program.bytes.data(),size)) Refuse("shader_input_unreadable");
        if(std::memcmp(program.bytes.data(),"DXBC",4)) Refuse("shader_input_not_dxbc");
        uint32_t declared{}; std::memcpy(&declared,program.bytes.data()+24,4);
        if(declared!=size) Refuse("shader_container_extent");
        Microsoft::WRL::ComPtr<ID3D11ShaderReflection> reflection;
        // D3DReflect is a CPU bytecode inspector in d3dcompiler, not a D3D11
        // device or GPU object. Exact owned bytes become native PSO inputs.
        if(FAILED(D3DReflect(program.bytes.data(),size,IID_PPV_ARGS(&reflection)))) Refuse("shader_reflection_failed");
        D3D11_SHADER_DESC desc{};
        if(FAILED(reflection->GetDesc(&desc)) || D3D11_SHVER_GET_MAJOR(desc.Version)!=5 ||
           D3D11_SHVER_GET_MINOR(desc.Version)!=0 || D3D11_SHVER_GET_TYPE(desc.Version)>5)
            Refuse("shader_model_unsupported");
        program.version=desc.Version;
        const auto receiver=reinterpret_cast<uintptr_t>(wrapper);
        uintptr_t handle{};
        if(!Read(receiver+0x10,&handle,8) || handle) Refuse("shader_wrapper_has_legacy_object");
        std::lock_guard<std::mutex> hold(shader_lock);
        program.generation=++shader_generation;
        shaders.insert_or_assign(receiver,std::make_shared<const NativeShader>(std::move(program)));shader_registry_changes.fetch_add(1,std::memory_order_acq_rel);BumpProgramRegistry();
        // The loader's +8 virtual query returns the actual admitted byte count
        // from +C. GPU handle +10 and vertex GPU metadata remain unpopulated.
        *reinterpret_cast<uint32_t*>(receiver+0xC)=static_cast<uint32_t>(size);
        char row[256]{}; sprintf_s(row,"{\"event\":\"native_shader_admitted\",\"generation\":%llu,\"stage\":%u,\"bytes\":%llu,\"legacy_handle\":false}\n",
            shader_generation,D3D11_SHVER_GET_TYPE(desc.Version),static_cast<unsigned long long>(size)); Log(row);
        return true;
    } catch(const std::exception&) { Refuse("native_shader_admission_failed"); }
}
void __fastcall RetireEffect(void* wrapper) noexcept {
    const auto receiver=reinterpret_cast<uintptr_t>(wrapper);uintptr_t legacy{};
    if(!Read(receiver+0x28,&legacy,8) || legacy)Refuse("unexpected_original_effect_pipeline");
    // Keep archive bytes alive through the genuine CPU destructor. Program
    // leases may outlive withdrawal, independently of engine wrapper addresses.
    std::shared_ptr<const NativeEffect> keep;
    {
        std::lock_guard<std::mutex> hold(effect_lock);
        const auto found=effects.find(receiver);
        if(found!=effects.end()) {keep=found->second;effects.erase(found);++effects_changes;BumpProgramRegistry();}
    }
    effect_retire_original(wrapper);
}
bool __fastcall AdmitEffect(void* wrapper,const void* bytes,size_t size) noexcept {
    try {
        const auto receiver=reinterpret_cast<uintptr_t>(wrapper);uintptr_t header{},legacy{},container{},heap{},temporary{},content{};
        if(!wrapper || !bytes || size<64 || size>128u*1024u*1024u ||
            !Read(receiver,&header,8) || header!=image+0x3D488E8 ||
            !Read(receiver+0x28,&legacy,8) || legacy || !Read(receiver+0x30,&container,8) || container ||
            !Read(receiver+0x10,&heap,8) || !heap || !Read(image+0x4961B78,&temporary,8) || !temporary ||
            !Read(image+0x4961B80,&content,8) || !content)Refuse("effect_cpu_ingress_contract");
        auto owned=std::make_shared<NativeEffect>();owned->archive.resize(size);
        if(!Read(reinterpret_cast<uintptr_t>(bytes),owned->archive.data(),size) ||
            std::memcmp(owned->archive.data(),"BND4",4))Refuse("effect_cpu_archive_unavailable");
        using Allocate=uintptr_t(__fastcall*)(size_t,size_t,uintptr_t);
        const auto allocate=reinterpret_cast<Allocate>(image+0x1777A00);
        container=allocate(0x30,8,heap);const auto parser=allocate(0x48,8,temporary);
        if(!container || !parser)Refuse("effect_cpu_allocation_failed");
        *reinterpret_cast<uintptr_t*>(container)=image+0x3D48E88;
        reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0xADFE0)(container+8);
        *reinterpret_cast<uint32_t*>(container+0x28)=0;
        *reinterpret_cast<uintptr_t*>(receiver+0x30)=container;
        reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t)>(image+0x1ACED0)(parser,content);
        if(!reinterpret_cast<bool(__fastcall*)(uintptr_t,const void*,size_t)>(image+0x1ACF10)(parser,owned->archive.data(),size))
            Refuse("effect_cpu_archive_decode_failed");
        // ACC20 is the real CPU archive enumeration/name/container writer. All
        // six shader factories/retirements at its boundary are natively owned.
        reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t)>(image+0xACC20)(container,parser);
        uint32_t byte_count{};if(!Read(container+0x28,&byte_count,4))Refuse("effect_cpu_byte_count_unreadable");
        *reinterpret_cast<uint32_t*>(receiver+0x18)=byte_count;
        uintptr_t archive_reader{};if(!Read(parser+0x10,&archive_reader,8) || !archive_reader)Refuse("effect_cpu_reader_unavailable");
        const auto entries=reinterpret_cast<uint32_t(__fastcall*)(uintptr_t)>(image+0x179F4C0)(archive_reader);
        if(entries>20000)Refuse("effect_cpu_archive_entry_bound");
        for(uint32_t i=0;i<entries;++i) {
            reinterpret_cast<int(__fastcall*)(uintptr_t,uint32_t,uintptr_t)>(image+0x179F6C0)(archive_reader,i,parser+0x18);
            if(!reinterpret_cast<bool(__fastcall*)(uintptr_t)>(image+0x17A2CD0)(parser+0x18))continue;
            const auto name=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0x17A2F10)(parser+0x18);
            if(name && reinterpret_cast<int(__fastcall*)(uintptr_t,uintptr_t)>(image+0xA98C0)(0,name)==6)
                reinterpret_cast<uint64_t(__fastcall*)(uintptr_t,uintptr_t)>(image+0x1AD070)(parser,receiver+0x40);
        }
        uintptr_t declarations{};uint64_t declaration_size{};
        if(!Read(receiver+0x50,&declarations,8) || !Read(receiver+0x58,&declaration_size,8) || declaration_size>128u*1024u*1024u)
            Refuse("effect_cpu_declaration_extent");
        if(declaration_size) {
            owned->declarations.resize(static_cast<size_t>(declaration_size));
            if(!Read(declarations,owned->declarations.data(),owned->declarations.size()))Refuse("effect_cpu_declarations_unreadable");
            owned->parameter_declarations=ds3content::ReadModelParameterDeclarations(owned->declarations);
            owned->texture_declarations=ds3content::ReadModelTextureDeclarations(owned->declarations);
            // A97F0 builds only CPU declaration tables. The original GPU
            // variant object A9BA0/AA4D0 and its GX global are never invoked.
            reinterpret_cast<bool(__fastcall*)(uintptr_t,uintptr_t,size_t)>(image+0xA97F0)(receiver,declarations,static_cast<size_t>(declaration_size));
        }
        uintptr_t range[3]{};
        if(!Read(container+8,range,sizeof(range)) || range[1]<range[0] || range[2]<range[1] ||
            (range[1]-range[0])%0x40 || (range[1]-range[0])/0x40>20000)Refuse("effect_cpu_program_table");
        for(auto at=range[0];at<range[1];at+=0x40) {
            uintptr_t shader{};uint32_t kind{};std::array<BYTE,32> name{};uint64_t length{},capacity{};
            if(!Read(at,&shader,8) || !Read(at+8,&kind,4) || kind>5 || !Read(at+0x10,name.data(),name.size()))Refuse("effect_cpu_program_record");
            std::memcpy(&length,name.data()+0x10,8);std::memcpy(&capacity,name.data()+0x18,8);
            if(!length || length>2048 || capacity<length)Refuse("effect_cpu_program_name");
            auto chars=at+0x10;if(capacity>7)std::memcpy(&chars,name.data(),8);
            std::wstring program_name(static_cast<size_t>(length)+1,L'\0');
            if(!Read(chars,program_name.data(),program_name.size()*2) || program_name.back())Refuse("effect_cpu_program_name_unreadable");
            program_name.pop_back();std::shared_ptr<const NativeShader> program;
            {
                std::lock_guard<std::mutex> hold(shader_lock);const auto found=shaders.find(shader);
                constexpr UINT engine_to_dxbc[]{5,4,3,2,0,1};
                if(found==shaders.end() || D3D11_SHVER_GET_TYPE(found->second->version)!=engine_to_dxbc[kind])Refuse("effect_native_program_missing");
                program=found->second;
            }
            owned->programs.push_back({std::move(program_name),kind,std::move(program)});
            constexpr ds3content::ShaderStage engine_stage[]{ds3content::ShaderStage::Compute,ds3content::ShaderStage::Domain,
                ds3content::ShaderStage::Hull,ds3content::ShaderStage::Geometry,ds3content::ShaderStage::Pixel,ds3content::ShaderStage::Vertex};
            owned->program_names.push_back({owned->programs.back().name,engine_stage[kind]});
        }
        reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0x1ACFC0)(parser);
        reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0x17A2BF0)(parser+0x18);
        uintptr_t heap_vtable{},release{};
        if(!Read(temporary,&heap_vtable,8) || !Read(heap_vtable+0x68,&release,8) || !release)Refuse("effect_cpu_temporary_heap_unavailable");
        reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t)>(release)(temporary,parser);
        std::lock_guard<std::mutex> hold(effect_lock);owned->generation=++effect_generation;
        char row[320]{};sprintf_s(row,"{\"event\":\"native_effect_cpu_admitted\",\"generation\":%llu,\"archive_bytes\":%zu,\"programs\":%zu,\"declaration_bytes\":%zu,\"native_pipeline_ready\":false,\"legacy_pipeline\":false}\n",
            owned->generation,owned->archive.size(),owned->programs.size(),owned->declarations.size());
        effects.insert_or_assign(receiver,std::move(owned));++effects_changes;BumpProgramRegistry();Log(row);return true;
    } catch(const std::exception& e) {
        char row[320]{};sprintf_s(row,"{\"event\":\"effect_cpu_failure\",\"message\":\"%s\"}\n",e.what());Log(row);
        Refuse("effect_cpu_admission_failed");
    }
}
std::wstring ReadCpuString(uintptr_t string) {
    uint64_t length{},capacity{};uintptr_t chars=string;
    if(!Read(string+0x10,&length,8) || !Read(string+0x18,&capacity,8) ||
       length>2048 || capacity<length)Refuse("material_cpu_string_extent");
    if(capacity>7 && !Read(string,&chars,8))Refuse("material_cpu_string_pointer");
    std::wstring text(static_cast<size_t>(length)+1,L'\0');
    if(!Read(chars,text.data(),text.size()*2) || text.back())Refuse("material_cpu_string_unreadable");
    text.pop_back();return text;
}
void __fastcall RetireMaterial(void* record) noexcept {
    const auto receiver=reinterpret_cast<uintptr_t>(record);uintptr_t legacy{},association{};
    if(!Read(receiver+0x60,&legacy,8) || legacy || !Read(receiver+0x2F8,&association,8) || association)
        Refuse("material_original_gpu_association");
    std::shared_ptr<const NativeMaterial> keep;
    {
        std::lock_guard<std::mutex> hold(material_lock);const auto found=materials.find(receiver);
        if(found!=materials.end()){keep=std::move(found->second);materials.erase(found);BumpProgramRegistry();}
    }
    // D439A0/D40540 withdraw the genuine CPU subscription and destroy the
    // engine strings. Their GPU release branches require +60, kept null.
    material_retire_original(record);
}
void ReadNativeMaterialParameters(NativeMaterial& material) {
    // Genuine CPU MTD parser only. Its temporary nodes die after the numeric
    // values and names have been copied into the native authored owner.
    struct Reader {
        std::array<uintptr_t,2> state{};
        Reader(){reinterpret_cast<void(__fastcall*)(void*)>(image+0x19343C0)(state.data());}
        ~Reader(){reinterpret_cast<void(__fastcall*)(void*)>(image+0x19343D0)(state.data());}
    } reader;
    using Parse=bool(__fastcall*)(void*,const void*,uint32_t);
    if(!reinterpret_cast<Parse>(image+0x1934C80)(reader.state.data(),material.bytes.data(),static_cast<uint32_t>(material.bytes.size())))
        Refuse("native_material_parameter_parse_failed");
    const auto count=reinterpret_cast<uint32_t(__fastcall*)(void*)>(image+0x19349A0)(reader.state.data());
    if(count>4096)Refuse("native_material_parameter_count");
    std::set<std::string> names;
    for(uint32_t i=0;i<count;++i){
        std::array<uintptr_t,3> parameter{};
        reinterpret_cast<void*(__fastcall*)(void*,void*,uint32_t)>(image+0x19348E0)(reader.state.data(),parameter.data(),i);
        const auto node=parameter[1];if(!node)Refuse("native_material_parameter_node_missing");
        uintptr_t name_decoder{},name_version{},chars{};
        if(!Read(node+0x88,&name_decoder,8) || !Read(node+0x90,&name_version,8))Refuse("native_material_parameter_name_extent");
        // Exact 19353F0 lazy CPU name preparation, without its engine-string
        // allocation: native storage immediately owns the resulting text.
        if(name_decoder){
            reinterpret_cast<bool(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uint32_t)>(image+0x193DA10)(name_decoder,node+0x60,name_version,0);
            reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t)>(image+0x193DB40)(name_decoder,node+0x60,name_version);
        }
        if(!Read(node+0x70,&chars,8) || !chars)Refuse("native_material_parameter_name_missing");
        std::string name;
        for(size_t j=0;j<=2048;++j){char ch{};if(!Read(chars+j,&ch,1))Refuse("native_material_parameter_name_unreadable");
            if(!ch)break;if(j==2048)Refuse("native_material_parameter_name_bound");name.push_back(ch);}
        if(name.empty() || !names.insert(name).second)Refuse("native_material_parameter_name_collision");
        const auto type=reinterpret_cast<uint32_t(__fastcall*)(void*)>(image+0x1935550)(parameter.data());
        if(type>=12){material.unsupported_parameters.push_back(std::move(name));continue;}
        uint32_t children{},value_count{};uintptr_t table{},value_node{};
        if(!Read(node+0xEC,&children,4) || !children || !Read(node+0xE0,&table,8) || !table ||
           !Read(table,&value_node,8) || !value_node || !Read(value_node+0x80,&value_count,4) || value_count!=type%4+1)
            Refuse("native_material_parameter_value_extent");
        std::array<uint32_t,4> words{};
        using Values=bool(__fastcall*)(uintptr_t,uint32_t,uint32_t*,uint32_t*,uint32_t*,uint32_t*);
        const auto rva=type<4?0x1935010u:(type<8?0x1934E50u:0x1934D60u);
        if(!reinterpret_cast<Values>(image+rva)(node,type,&words[0],&words[1],&words[2],&words[3]))
            Refuse("native_material_parameter_values_missing");
        ds3prod::MtdParam owned;
        material.numeric_parameters.push_back({name,type,words});
        if(!ds3prod::DecodeMtdNumericParam(std::move(name),type,std::span<const uint32_t>(words.data(),value_count),owned))
            Refuse("native_material_parameter_values_invalid");
        material.parameters.push_back(std::move(owned));
    }
}
void __fastcall AdmitMaterial(void* record,uintptr_t data,uint32_t size) noexcept {
    if(control->stop)return;
    try {
        const auto receiver=reinterpret_cast<uintptr_t>(record);uintptr_t vtable{},legacy{},association{};
        if(!Read(receiver,&vtable,8) || vtable!=image+0x2953278 ||
           !Read(receiver+0x60,&legacy,8) || legacy || !Read(receiver+0x2F8,&association,8) || association ||
           size<16 || size>128u*1024u*1024u)Refuse("material_cpu_input_contract");
        for(const auto& pin:loaded_material_pins) {
            BYTE prefix[16]{};
            if(!Read(image+pin.rva,prefix,sizeof(prefix)) || std::memcmp(prefix,pin.bytes,sizeof(prefix)))
                Refuse("loaded_material_cpu_identity_mismatch");
        }
        auto owned=std::make_shared<NativeMaterial>();owned->bytes.resize(size);
        if(!Read(data,owned->bytes.data(),owned->bytes.size()))Refuse("material_cpu_bytes_unreadable");
        owned->name=ReadCpuString(receiver+0x10);
        // Retain the real CPU MTD parser and metadata producer, supplying no
        // original GPU material. D42EB0 explicitly guards that association;
        // its CPU name notification and subscription lifetime stay genuine.
        using Metadata=void(__fastcall*)(uintptr_t,const void*,uint32_t,const wchar_t*,uintptr_t);
        reinterpret_cast<Metadata>(image+0xD42D50)(receiver+0x68,owned->bytes.data(),size,owned->name.c_str(),0);
        if(!Read(receiver+0x60,&legacy,8) || legacy || !Read(receiver+0x2F8,&association,8) || association ||
           !Read(receiver+0xA0,owned->values.data(),owned->values.size()))Refuse("material_cpu_metadata_contract");
        owned->shader=ReadCpuString(receiver+0x70);
        ReadNativeMaterialParameters(*owned);
        std::lock_guard<std::mutex> hold(material_lock);owned->generation=++material_generation;
        const auto previous=materials.find(receiver);
        owned->incarnation=previous==materials.end()?owned->generation:previous->second->incarnation;
        char row[256]{};sprintf_s(row,"{\"event\":\"native_material_cpu_admitted\",\"generation\":%llu,\"bytes\":%zu,\"name_chars\":%zu,\"shader_chars\":%zu,\"native_pipeline_ready\":false,\"legacy_material\":false}\n",
            owned->generation,owned->bytes.size(),owned->name.size(),owned->shader.size());
        materials.insert_or_assign(receiver,std::move(owned));BumpProgramRegistry();Log(row);
    } catch(const std::exception& e) {
        char row[320]{};sprintf_s(row,"{\"event\":\"material_cpu_failure\",\"message\":\"%s\"}\n",e.what());Log(row);
        Refuse("material_cpu_admission_failed");
    }
}
std::wstring ReadFlverWide(uintptr_t address) {
    std::wstring value;
    for(size_t i=0;i<2048;++i){wchar_t ch{};
        if(!Read(address+i*2,&ch,2))throw std::runtime_error("FLVER authored name unreadable");
        if(!ch)return value;value.push_back(ch);}
    throw std::runtime_error("FLVER authored name bound");
}
uintptr_t __fastcall DestroyModelMaterial(uintptr_t receiver,uint32_t flags) noexcept {
    if(flags)Refuse("native_model_material_external_delete");
    std::shared_ptr<NativeModelMaterial> material;
    {std::lock_guard<std::shared_mutex> hold(model_material_lock);
        const auto found=model_materials.find(receiver);
        if(found==model_materials.end())Refuse("native_model_material_foreign_retirement");
        if(found->second->scroll)found->second->scroll->Withdraw();
        material=found->second;model_materials.erase(found);BumpModelRegistry();}
    // I-0085: its layouts leave with it (the meshes that borrowed them belong to the same FLVER resource); the scene records
    // still holding one keep it alive until they are rebuilt, and its prefix serves the next build.
    // The retired layouts are released after the lock (their programmes' caches and bytecode), so the admission workers' lookups
    // are not held behind about a thousand destructions at an unload; a spare that cannot be recorded is simply not reused.
    std::vector<std::shared_ptr<NativeModelLayout>> released;
    {std::lock_guard<std::shared_mutex> hold(model_layout_lock);
        for(const auto storage:material->layouts){const auto found=model_layouts.find(storage);
            if(found==model_layouts.end() || found->second->material!=material)continue;
            for(auto [at,end]=model_layout_keys.equal_range(found->second->key);at!=end;++at)if(at->second==storage){model_layout_keys.erase(at);break;}
            try{model_layout_spare_prefixes.emplace_back(storage,found->second->heap);released.push_back(found->second);}catch(...){}
            model_layouts.erase(found);++model_layouts_retired;}
        material->layouts.clear();BumpModelRegistry();}
    released.clear();
    // Genuine CPU containers/name retirement; no GPU associations were set.
    reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0x140560)(receiver);
    return receiver;
}
uintptr_t __fastcall ModelPrimaryShader(uintptr_t receiver) noexcept {
    // Actual 1403F0 accessor: the original GPU shader provider remains nil.
    // The CPU association at 14451C immediately supplies this material's
    // real vertex descriptions to our resolver. Carry its native CPU lease
    // separately instead of publishing an invented old shader provider.
    if(reinterpret_cast<uintptr_t>(_ReturnAddress())!=image+0x14451C)
        Refuse("native_model_material_shader_scope_unowned");
    uintptr_t original{};
    if(!Read(receiver+0x70,&original,8) || original)
        Refuse("native_model_material_original_shader_present");
    std::lock_guard<std::shared_mutex> hold(model_material_lock);
    const auto found=model_materials.find(receiver);
    if(found==model_materials.end())Refuse("native_model_layout_material_missing");
    model_layout_material=found->second;
    return 0;
}
uintptr_t __fastcall DestroyModelLayout(uintptr_t receiver,uint32_t flags) noexcept {
    // The resolver cache owns this prefix; mesh associations only borrow it.
    // Actual 1AFAC0's external heap retirement uses virtual0(0).
    if(flags)Refuse("native_model_layout_external_delete_unowned");
    std::array<uintptr_t,160> old_variants{};
    std::array<uintptr_t,3> old_vector{};
    if(!Read(receiver+8,old_variants.data(),sizeof(old_variants)) ||
       !Read(receiver+0x518,old_vector.data(),sizeof(old_vector)))
        Refuse("native_model_layout_retirement_unreadable");
    for(const auto value:old_variants)if(value)Refuse("native_model_layout_original_variant_present");
    for(const auto value:old_vector)if(value)Refuse("native_model_layout_original_vector_present");
    std::lock_guard<std::shared_mutex> hold(model_layout_lock);
    const auto found=model_layouts.find(receiver);
    // I-0085: a prefix already retired with its material (a spare) leaves the spares; the engine frees it after this call.
    if(found==model_layouts.end()){const auto spare=std::find_if(model_layout_spare_prefixes.begin(),model_layout_spare_prefixes.end(),[&](const auto& p){return p.first==receiver;});
        if(spare==model_layout_spare_prefixes.end())Refuse("native_model_layout_foreign_retirement");
        model_layout_spare_prefixes.erase(spare);return receiver;}
    for(auto [at,end]=model_layout_keys.equal_range(found->second->key);at!=end;++at)if(at->second==receiver){model_layout_keys.erase(at);break;}
    model_layouts.erase(found);
    BumpModelRegistry();
    return receiver;
}
// I-0081: the key of a layout's identity (resolver, type, flags, instancing, material object, authored elements), FNV-1a.
uint64_t NativeModelLayoutKey(const NativeModelLayout& layout) noexcept {
    uint64_t hash=14695981039346656037ull;
    const auto mix=[&](const void* data,size_t size){const auto* bytes=static_cast<const uint8_t*>(data);for(size_t i=0;i<size;++i){hash^=bytes[i];hash*=1099511628211ull;}};
    const auto* material=layout.material.get();
    mix(&layout.resolver,sizeof layout.resolver);mix(&layout.type,sizeof layout.type);mix(&layout.flags,sizeof layout.flags);
    mix(&layout.instanced,sizeof layout.instanced);mix(&material,sizeof material);
    if(!layout.elements.empty())mix(layout.elements.data(),layout.elements.size()*sizeof layout.elements[0]);
    return hash;
}
uintptr_t __fastcall BuildModelLayout(uintptr_t resolver,uintptr_t elements,uint32_t count,
                                     uint32_t type,BYTE instanced,uintptr_t shader,uintptr_t flags) noexcept {
    uintptr_t storage{},heap{};
    try {
        auto material=std::move(model_layout_material);model_layout_material.reset();
        const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress())-image;
        if((caller!=0x144552 && caller!=0x1FBB70) || !material || !material->authored || shader ||
           !count || count>256 || instanced>1 || !Read(resolver+8,&heap,8) || !heap)
            throw std::runtime_error("native model CPU layout ingress contract");
        auto owned=std::make_shared<NativeModelLayout>();
        owned->resolver=resolver;owned->type=type;owned->instanced=instanced;owned->material=std::move(material);
        if(!Read(flags,&owned->flags,4))throw std::runtime_error("native model CPU layout flags");
        owned->elements.resize(count);
        if(!Read(elements,owned->elements.data(),size_t(count)*0x1C))
            throw std::runtime_error("native model CPU layout descriptions");
        owned->vertex_elements_available=ds3content::DecodeModelVertexLayout(owned->elements,owned->vertex_elements);
        // Preserve the CPU cache semantics without calling mixed 1B0040 or
        // 1AFB50. Native PSO construction will read these authored elements
        // and shader identity; every old input-layout variant stays nil.
        // I-0081: the equal layout is found by its key (the scan of every layout ran under this lock, which the collect takes
        // for each record, about 64 builds a second while walking); the journal row is written after the lock.
        owned->key=NativeModelLayoutKey(*owned);
        {std::lock_guard<std::shared_mutex> hold(model_layout_lock);
            for(auto [at,end]=model_layout_keys.equal_range(owned->key);at!=end;++at){const auto& value=model_layouts.at(at->second);
                if(value->resolver==resolver && value->type==type && value->flags==owned->flags &&
                   value->instanced==instanced && value->material==owned->material && value->elements==owned->elements)
                    return at->second;}
            static const auto table=[] {std::array<uintptr_t,2> result{};
                result[0]=reinterpret_cast<uintptr_t>(&DestroyModelLayout);
                result[1]=reinterpret_cast<uintptr_t>(&UnsupportedModelStorage);return result;}();
            // I-0085: a retired layout's prefix of the same heap first (constructed once), the last retired first, taken only when
            // it still reads as ours (our table, every variant and the vector nil, as DestroyModelLayout requires); else a new one.
            for(auto spare=model_layout_spare_prefixes.size();!storage && spare-->0;){
                if(model_layout_spare_prefixes[spare].second!=heap)continue;   // final review L4: another heap's spares are passed over
                const auto candidate=model_layout_spare_prefixes[spare].first;model_layout_spare_prefixes.erase(model_layout_spare_prefixes.begin()+spare);
                uintptr_t vtable{};std::array<uintptr_t,160> variants{};std::array<uintptr_t,3> vector{};
                if(Read(candidate,&vtable,8) && vtable==reinterpret_cast<uintptr_t>(table.data()) && Read(candidate+8,variants.data(),sizeof variants) &&
                   Read(candidate+0x518,vector.data(),sizeof vector) && std::all_of(variants.begin(),variants.end(),[](uintptr_t v){return !v;}) &&
                   std::all_of(vector.begin(),vector.end(),[](uintptr_t v){return !v;}))storage=candidate;}
            if(!storage){
                storage=reinterpret_cast<uintptr_t(__fastcall*)(size_t,size_t,uintptr_t)>(image+0x1777A00)(0x538,8,heap);
                if(!storage)throw std::bad_alloc();
                std::memset(reinterpret_cast<void*>(storage),0,0x538);
                reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uintptr_t)>(image+0x1AFA20)(storage,heap);
                *reinterpret_cast<uintptr_t*>(storage)=reinterpret_cast<uintptr_t>(table.data());}
            owned->heap=heap;owned->material->layouts.push_back(storage);
            model_layouts.emplace(storage,owned);model_layout_keys.emplace(owned->key,storage);BumpModelRegistry();}
        char row[256]{};sprintf_s(row,"{\"event\":\"native_model_layout_cpu\",\"elements\":%u,\"type\":%u,\"flags\":%u,\"instanced\":%u,\"mtd_generation\":%llu,\"original_variants_nil\":true,\"native_pipeline_ready\":false}\n",
            count,type,owned->flags,static_cast<unsigned>(instanced),owned->material->authored->generation);Log(row);
        return storage;
    }catch(const std::exception& error){
        if(storage){try{FreeModelPrefix(storage,heap);}catch(...){Refuse("native_model_layout_heap_failed");}}
        char row[320]{};sprintf_s(row,"{\"event\":\"native_model_layout_error\",\"reason\":\"%s\"}\n",error.what());Log(row);
        Refuse("native_model_layout_cpu_admission_failed");
    }
}
// Slot 7 (+0x38), the original's 0x140140410 `mov rax,[rcx+0x80]`: DS3's burn update FUN_140667430 asks every material of a
// burning map object for it and compares the name at +0x48 with its Burn MTDs (Archdragon Peak, 2026-10-27). Only an
// instance this registry made carries this table, so a receiver it does not own is its invariant broken (final review M1).
uintptr_t __fastcall ModelMaterialMtd(uintptr_t receiver) noexcept {
    std::shared_ptr<bootstrap::GxMtdImage> image_of;
    {const std::shared_lock hold(model_material_lock);if(const auto found=model_materials.find(receiver);found!=model_materials.end())image_of=found->second->gx_mtd;}
    if(!image_of)Refuse("native_model_material_mtd_unowned");
    static LONG64 calls{};const auto n=InterlockedIncrement64(&calls);
    if(n<=16){std::string name;for(const auto ch:image_of->name)name.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?char(ch):'?');
        char row[256]{};sprintf_s(row,"{\"event\":\"native_model_material_mtd\",\"call\":%lld,\"stem\":\"%.80s\",\"caller_rva\":%llu}\n",n,name.c_str(),
            static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(_ReturnAddress())-image));Log(row);}
    return reinterpret_cast<uintptr_t>(image_of->bytes.data());
}
const std::array<uintptr_t,64>& ModelMaterialTable() {
    static const auto table=[] {std::array<uintptr_t,64> result{};
        result.fill(reinterpret_cast<uintptr_t>(&UnsupportedModelStorage));
        result[0]=reinterpret_cast<uintptr_t>(&DestroyModelMaterial);
        result[0x20/8]=image+0x140320;   // I-0101: the original's slot 4, `xor eax,eax; ret`
        result[0x28/8]=reinterpret_cast<uintptr_t>(&ModelPrimaryShader);
        result[0x38/8]=reinterpret_cast<uintptr_t>(&ModelMaterialMtd);   // I-0101: slot 7, the GX MTD object
        result[0x60/8]=image+0x140460;   // I-0101: the original's slot 12, `ret 0`
        // Original CPU release delegates virtual0, then the actual stored heap.
        result[0x50/8]=image+0x140420;return result;}();
    return table;
}
std::vector<uint8_t> CopyModelGxRecords(uintptr_t header,uint32_t offset) {
    std::vector<uint8_t> records;if(!offset)return records;
    if(header>UINTPTR_MAX-offset)throw std::runtime_error("FLVER GX address overflow");
    uintptr_t at=header+offset;
    for(size_t count=0;count<65536;++count) {
        std::array<uint8_t,12> prefix{};uint32_t key{};int32_t length{};
        if(!Read(at,prefix.data(),prefix.size()))throw std::runtime_error("FLVER GX header unreadable");
        std::memcpy(&key,prefix.data(),4);std::memcpy(&length,prefix.data()+8,4);
        const auto bytes=key==0x7FFFFFFF?size_t(12):length>=12?static_cast<size_t>(length):0;
        if(!bytes || bytes>1024u*1024u-records.size() || at>UINTPTR_MAX-bytes)
            throw std::runtime_error("FLVER GX native record envelope");
        const auto begin=records.size();records.resize(begin+bytes);
        if(!Read(at,records.data()+begin,bytes) || std::memcmp(prefix.data(),records.data()+begin,12))
            throw std::runtime_error("FLVER GX current record changed or unreadable");
        if(key==0x7FFFFFFF)return records;
        at+=bytes;
    }
    throw std::runtime_error("FLVER GX native record count");
}
uintptr_t __fastcall BuildModelMaterial(uintptr_t resolver,uintptr_t file,uint32_t ordinal,uintptr_t) noexcept {
    try {
        uintptr_t header{},descriptions{};uint32_t count{},version{},texture_count{};
        if(!Read(file,&header,8) || !Read(header+8,&version,4) || version<0x20003 ||
           !Read(header+0x18,&count,4) || count>65536 || ordinal>=count ||
           !Read(file+0x10,&descriptions,8) || !Read(header+0x58,&texture_count,4) || texture_count>65536)
            throw std::runtime_error("FLVER native material family");
        auto owned=std::make_shared<NativeModelMaterial>();
        if(!Read(descriptions+uint64_t(ordinal)*0x20,owned->description.data(),0x20))
            throw std::runtime_error("FLVER native material description");
        //82220/141070 supply current serialized records under this synchronous
        // loading lifetime. Own them before publication. Native count/length
        // bounds do not claim784 established the full FLVER allocation extent.
        owned->gx_records=CopyModelGxRecords(header,owned->description[5]);
        ds3content::TextureScrollHistory scroll;
        if(!ds3content::ReadTextureScrollRecords(owned->gx_records,scroll))
            throw std::runtime_error("FLVER GX native UV records invalid");
        owned->scroll=std::make_shared<ds3content::TextureScrollOwner>(scroll);
        owned->name=ReadFlverWide(header+owned->description[0]);owned->mtd=ReadFlverWide(header+owned->description[1]);
        // Same CPU name contract as 82220: D1760 selects the final path
        // component, D16E0 finds its extension. Registry keys exclude .mtd.
        const auto name=reinterpret_cast<const wchar_t*(__fastcall*)(const wchar_t*)>(image+0xD1760)(owned->mtd.c_str());
        const auto extension=reinterpret_cast<const wchar_t*(__fastcall*)(const wchar_t*)>(image+0xD16E0)(name);
        const std::wstring key=extension?std::wstring(name,extension):std::wstring(name);
        {std::lock_guard<std::mutex> hold(material_lock);
            const auto find=[&](const std::wstring& selected){
                for(const auto& [id,value]:materials){
                    if(_wcsicmp(std::filesystem::path(value->name).filename().c_str(),selected.c_str())==0){
                        owned->authored_identity=id;return value;}}
                return std::shared_ptr<const NativeMaterial>{};};
            owned->authored=find(key);
            if(!owned->authored){
                // Actual DS3 resolver294E6B8+40 -> D24390 retries the authored
                // engine default at294E748 when the requested MTD is absent.
                // Reuse that real loaded CPU lease; retain the FLVER request
                // separately and never invent a shader or old GPU material.
                owned->authored=find(ReadFlverWide(image+0x294E748));
                owned->engine_default_mtd=bool(owned->authored);
            }}
        if(!owned->authored){
            const auto log_key=[](const std::wstring& value,const char* kind){char hex[513]{};
                for(size_t i=0;i<value.size() && i<128;++i)sprintf_s(hex+i*4,sizeof(hex)-i*4,"%04x",static_cast<unsigned>(value[i]));
                char row[768]{};sprintf_s(row,"{\"event\":\"native_model_mtd_key\",\"kind\":\"%s\",\"chars\":%zu,\"utf16_hex\":\"%s\"}\n",kind,value.size(),hex);Log(row);};
            log_key(key,"requested");size_t sampled{};std::lock_guard<std::mutex> hold(material_lock);
            for(const auto& [id,value]:materials){(void)id;log_key(value->name,"loaded");if(++sampled==8)break;}
            throw std::runtime_error("FLVER authored MTD not loaded");
        }
        const auto first=owned->description[3],number=owned->description[2];uintptr_t texture_table{};
        if(first>texture_count || number>texture_count-first || !Read(file+0x40,&texture_table,8))
            throw std::runtime_error("FLVER material texture range");
        owned->texture_inputs.reserve(number);
        for(uint32_t i=0;i<number;++i){std::array<BYTE,0x20> row{};
            if(!Read(texture_table+uint64_t(first+i)*0x20,row.data(),row.size()))
                throw std::runtime_error("FLVER material texture descriptions");
            std::array<uint32_t,2> offsets{};std::memcpy(offsets.data(),row.data(),8);
            ds3content::ModelTextureInput texture;
            texture.path=ReadFlverWide(header+offsets[0]);texture.type=ReadFlverWide(header+offsets[1]);
            std::memcpy(texture.scale.data(),row.data()+8,8);std::memcpy(&texture.metadata,row.data()+0x10,2);
            texture.authored_ordinal=first+i;owned->texture_inputs.push_back(std::move(texture));}
        // Genuine CPU instance construction only. Authored MTD/FLVER inputs
        // are retained here for native composition; no old shader template,
        // default texture, constant pool, PSO or draw-ready result is invented.
        const auto instance=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0x82CC0)(resolver);
        if(!instance)throw std::bad_alloc();
        *reinterpret_cast<uintptr_t*>(instance)=reinterpret_cast<uintptr_t>(ModelMaterialTable().data());
        reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t)>(image+0x1B0AB0)(instance,header+owned->description[0]);
        *reinterpret_cast<uint32_t*>(instance+0x64)=ordinal;
        // 1185 (MENUS-MAP 66): 140F90 writes e+0x22 = bit 13 (0x2000) of the GX MTD flag word gxMtd+0x88, set by the MTD
        // parameter handler 0A7E40 (decrypted at runtime). Its rule, exact on the 512 MTDs of AllMaterialBnd (INFERRED
        // from data): g_GX_MaterialType == 2 or GXFT_Cloth true. 23B7960 admits a cloth mesh on this byte, and 140850
        // gates VC_isEnableCloth with it. The other 140F90 words (+0x18, +0x1C, +0x21, +0x80) stay open.
        // MENUS-MAP 71: the same handler sets bit 11 (0x800, the SSS skin stencil class read by 140850) exactly when
        // g_GX_MaterialType == 3 (the 38 SSS MTDs; INFERRED from data). e+0x24 carries gxMtd+0x88: four data-derived masks are
        // produced here (0x2000, 0x800, 0x2 and 0x100000); every other bit of the word stays clear (MTD-FLAG-BITS-INVENTORY-SHEET 2).
        // I-0113 tranche 13: bit 20 (0x100000, GXFT_InvalidLighting, builtin kind 38: the unlit and particle MTDs s[a]*, m[a]*),
        // A7E40's low bit of the first value; its only reader is 61A20, whose caller 27E5E0 (the E03 model set's factory) stores it
        // at set+0x129, and 27EA70 then scales the model particle's colour by the draw context's +0xAA8
        // (MTD-FLAG-BITS-INVENTORY-SHEET 2.1; no model+0x224 reader).
        {bool cloth{},sss{},wind{},unlit{};
            // A7E40's bit is the low bit of the parameter's first raw word.
            for(const auto& value:owned->authored->numeric_parameters)if(value.name=="GXFT_InvalidLighting")unlit=(value.words[0]&1u)!=0;
            for(const auto& parameter:owned->authored->parameters){if(parameter.values.empty())continue;
                if(parameter.name=="g_GX_MaterialType" && parameter.values[0]==2.0)cloth=true;
                if(parameter.name=="g_GX_MaterialType" && parameter.values[0]==3.0)sss=true;
                if((parameter.name=="GXFT_Cloth" || parameter.name=="g_GXFT_Cloth") && parameter.values[0]!=0.0)cloth=true;
                if((parameter.name=="GXFT_WindEffect" || parameter.name=="g_GXFT_WindEffect") && parameter.values[0]!=0.0)wind=true;}
            // W4 FW-6 (WIND-BLOCK W1, default since FW-9): bit 1 (g_GXFT_WindEffect) for every material whose MTD enables it, once
            // the effect system is published (FFX-WAVE1-READY 2.6: the node update must be the transcribed one); 0619B0 then
            // registers the models' wind nodes and D3E950/D07710 evaluate them against the effect system's wind entries.
            if(wind && !native_effect_published.load(std::memory_order_acquire))wind=false;
            *reinterpret_cast<BYTE*>(instance+0x22)=cloth?1:0;
            *reinterpret_cast<uint32_t*>(instance+0x24)|=(cloth?0x2000u:0u)|(sss?0x800u:0u)|(wind?0x2u:0u)|(unlit?0x100000u:0u);}
        // C2 (I-0068 NPC-FAMILIES-STATIC §2.2, MENUS-MAP 67): 141070, the GX item parser, does not run natively. Its items
        // GX01, GX02, GX03, GX05, GX06, GX09, GX12 and GX13 feed the cbMatDynParam blocks 22E930 reads at material +0xD0 (the producers below): GX03 flag
        // +0xDE, value +0x104 (FC_EmissiveColor); GX05 flag +0xE0, value +0x108 (FC_TranslucencyScale); GX09 flag +0xE3,
        // values +0x200..+0x20C (FC_ParallaxParam). Items {key "GXnn", unk, length, data}; the values are the data words.
        {const auto& gx=owned->gx_records;
            for(size_t at=0,item=0;item<65536 && at+12<=gx.size();++item){
                uint32_t tag{};int32_t length{};std::memcpy(&tag,gx.data()+at,4);std::memcpy(&length,gx.data()+at+8,4);
                if(tag==0x7FFFFFFF || length<12 || static_cast<size_t>(length)>gx.size()-at)break;
                const auto words=(static_cast<size_t>(length)-12)/4;const auto* data=gx.data()+at+12;
                // I-0100 tranche 6: GX00, the material's « Ex Param » block at +0xD0 (14022B7C0; its names from the serializer
                // 1402306C0): item version 0x65: data word 0 bit 0 « Decal » to +0xD0, bit 1 « [Shadow] DisableCasting » to
                // +0xD1, word 1 « [PaintDecal] SlotNo » to +0xD4, word 2 « [Shadow] CullingMode » to +0xD8; another version:
                // Decal and SlotNo only. 142130 classes a Decal material 0x10 (forward) or 8, which the cascade row's mask 0x107
                // refuses (the eyebrows FG_A_2012, i100-t6i-firelink-o1; the class itself is kept until the decal pass exists, see the
                // admission), and the admission's no-cast reads +0xD1 (D-0180 C2).
                if(tag==0x30305847 && words>=1){int32_t item_version{};std::memcpy(&item_version,gx.data()+at+4,4);uint32_t flags{};std::memcpy(&flags,data,4);
                    *reinterpret_cast<BYTE*>(instance+0xD0)=flags&1;if(words>=2)std::memcpy(reinterpret_cast<void*>(instance+0xD4),data+4,4);
                    if(item_version==0x65 && words>=3){*reinterpret_cast<BYTE*>(instance+0xD1)=(flags>>1)&1;std::memcpy(reinterpret_cast<void*>(instance+0xD8),data+8,4);}
                    // Diagnostic (caster-place.flag, the step review's M1): each Decal or DisableCasting material once by its MTD name.
                    static const bool listed=ModuleFlag(L"caster-place.flag");
                    if(listed && (flags&3)){std::string mtd;for(const auto ch:owned->mtd)if(mtd.size()<100)mtd.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');
                        static std::mutex lock;static std::set<std::string> seen;bool fresh{};{const std::lock_guard hold(lock);fresh=seen.size()<400 && seen.insert(mtd+std::to_string(flags&3)).second;}
                        if(fresh){char row[300]{};sprintf_s(row,"{\"event\":\"native_material_ex_param\",\"mtd\":\"%s\",\"version\":%d,\"decal\":%u,\"no_cast\":%u}\n",mtd.c_str(),item_version,flags&1,(flags>>1)&1);Log(row);}}}
                else if(tag==0x33305847 && words>=1){*reinterpret_cast<BYTE*>(instance+0xDE)=1;std::memcpy(reinterpret_cast<void*>(instance+0x104),data,4);}
                else if(tag==0x35305847 && words>=1){*reinterpret_cast<BYTE*>(instance+0xE0)=1;std::memcpy(reinterpret_cast<void*>(instance+0x108),data,4);}
                // I-0105 tranche 9 (the surface snow): GX06 (141070, 140141837): +0xE1 = 1, data word 0 to +0x10C; 22E930 makes cbMatDynParam
                // +0xA0 FC_SnowMaskParam of them. (141070 also raises +0x4E8, the highest block, to 10: the native buffer grows per block.)
                else if(tag==0x36305847 && words>=1){*reinterpret_cast<BYTE*>(instance+0xE1)=1;std::memcpy(reinterpret_cast<void*>(instance+0x10C),data,4);}
                else if(tag==0x39305847 && words>=4){*reinterpret_cast<BYTE*>(instance+0xE3)=1;std::memcpy(reinterpret_cast<void*>(instance+0x200),data,16);}
                // I-0105 (the burn look, BURN.md): GX13 (141070): data words 0..2 to +0xF8, +0xFC, +0x100 and +0xE9 = 1; 22E930 turns them
                // into cbMatDynParam +0x30 (with +0xDC) and +0xC0 FC_BlendRate (the Burn shaders' burn amount, the admission's producer).
                else if(tag==0x33315847 && words>=3){std::memcpy(reinterpret_cast<void*>(instance+0xF8),data,12);*reinterpret_cast<BYTE*>(instance+0xE9)=1;}
                // GX01 (141070): data word 0 to +0xF8 and +0xDC = 1 (the +0x30 block's other gate, FC_BlendMaskParam).
                else if(tag==0x31305847 && words>=1){std::memcpy(reinterpret_cast<void*>(instance+0xF8),data,4);*reinterpret_cast<BYTE*>(instance+0xDC)=1;}
                // I-0113 tranche 7 (ZERO-CONSTANTS-CENSUS): GX36 and GX37 (141070): data words 0..3 to +0x3D0 and +0x3E0, +0xEC and
                // +0xED = 1; 22E930 copies them to cbMatDynParam +0xD0 FC_FlowMapUvTiling and +0xE0 FC_FlowMapIntencity (the
                // c5110[FlowDisplacement] materials of c1320, c1321, c3140 and c3141). GX39 and GX40: data word 0 four times to +0x3F0
                // and +0x400, +0xE6 and +0xE7 = 1; 22E930's +0xF0 FC_SoftVolume and +0x100 FC_PointCloudRate (the m50 effects s09540
                // and s09541 carry 0.3; the other 51 _Soft materials 0).
                else if((tag==0x36335847 || tag==0x37335847) && words>=4){const bool flow=tag==0x37335847;
                    std::memcpy(reinterpret_cast<void*>(instance+(flow?0x3E0:0x3D0)),data,16);*reinterpret_cast<BYTE*>(instance+(flow?0xED:0xEC))=1;}
                else if((tag==0x39335847 || tag==0x30345847) && words>=1){const bool cloud=tag==0x30345847;uint32_t w{};std::memcpy(&w,data,4);
                    const std::array<uint32_t,4> v{w,w,w,w};std::memcpy(reinterpret_cast<void*>(instance+(cloud?0x400:0x3F0)),v.data(),16);
                    *reinterpret_cast<BYTE*>(instance+(cloud?0xE7:0xE6))=1;}
                // I-0113 tranche 6 (Anor Londo's flat wall, the POM m37_00_bldgA_wall_00): GX02 (141070 lines 47-59): data words 0 and 1
                // to +0x110 and +0x114, word 2 to +0x120, words 3 and 4 (integers) as floats to +0x118 and +0x11C, +0xDD = 1; 22E930
                // copies +0x110..+0x12F to cbMatDynParam +0x40..+0x5F (FC_PomHeightScale, the sample counts). GX12 (lines 191-203):
                // the second layer's, words 0 and 1 to +0x124 and +0x128, word 2 to +0x134, words 3 and 4 as floats to +0x12C and
                // +0x130, +0xE8 = 1; 22E930 copies +0x124..+0x133 to +0x70 (FC_Pom2HeightScale).
                else if((tag==0x32305847 || tag==0x32315847) && words>=5){const bool second=tag==0x32315847;int32_t w[5]{};std::memcpy(w,data,20);
                    const uintptr_t at_value=instance+(second?0x124:0x110);std::memcpy(reinterpret_cast<void*>(at_value),w,8);
                    std::memcpy(reinterpret_cast<void*>(instance+(second?0x134:0x120)),&w[2],4);
                    const float samples[2]{static_cast<float>(w[3]),static_cast<float>(w[4])};std::memcpy(reinterpret_cast<void*>(at_value+8),samples,8);
                    *reinterpret_cast<BYTE*>(instance+(second?0xE8:0xDD))=1;}
                // WATER-TESS-WAVE1-READY (the water materials' GX07, 14022C180 on the sub-object +0xD0): data words 0..3 to sub+0xD0,
                // 4..11 to +0xE8, 12..15 to +0x110, 16..17 to +0x120, with version > 0x66 18..19 to +0x128, with > 0x67 the next two to
                // +0xE0; sub+0x12 = 1. 22E930 turns them into cbMatDynParam +0x110..+0x17F (FC_Water*: the producer below).
                else if(tag==0x37305847 && words>=18){int32_t item_version{};std::memcpy(&item_version,gx.data()+at+4,4);const auto sub=instance+0xD0;
                    std::memcpy(reinterpret_cast<void*>(sub+0xD0),data,16);std::memcpy(reinterpret_cast<void*>(sub+0xE8),data+16,32);
                    std::memcpy(reinterpret_cast<void*>(sub+0x110),data+48,16);std::memcpy(reinterpret_cast<void*>(sub+0x120),data+64,8);
                    size_t next=18;if(item_version>0x66 && words>=20){std::memcpy(reinterpret_cast<void*>(sub+0x128),data+72,8);next=20;}
                    if(item_version>0x67 && words>=next+2)std::memcpy(reinterpret_cast<void*>(sub+0xE0),data+4*next,8);
                    *reinterpret_cast<BYTE*>(sub+0x12)=1;}
                // m50 sky: GX14 (22BD90): +0xF0 = 1, +0xEF = 1; item version (+4) 0x65: +0xEE = 1 and +0x420..+0x42C = the first
                // four data words (colour, scale), else colour (1, 1, 1, 0) (143D7F6D0) and scale the first word.
                else if(tag==0x34315847 && words>=1){int32_t item_version{};std::memcpy(&item_version,gx.data()+at+4,4);
                    std::array<uint32_t,4> v{0x3F800000u,0x3F800000u,0x3F800000u,0};
                    if(item_version==0x65 && words>=4){std::memcpy(v.data(),data,16);*reinterpret_cast<BYTE*>(instance+0xEE)=1;}else std::memcpy(&v[3],data,4);
                    *reinterpret_cast<BYTE*>(instance+0xF0)=1;*reinterpret_cast<BYTE*>(instance+0xEF)=1;std::memcpy(reinterpret_cast<void*>(instance+0x420),v.data(),16);}
                // W5e TS-1 (MENUS-MAP 67): GX04 and GX90 write the tessellation fields 1402302C0 and the deformation choice read
                // (+0x250 mode, +0x254 displacement, +0x258/+0x25C factors, +0x260, +0x26C, +0x270, +0x274/+0x278 fade, +0xDF,
                // GX90 also +0xE4). The draw stays flat (DisableDeformation) unless native-tess.flag releases it.
                else if((tag==0x34305847 || tag==0x30395847) && words>=7){ds3native::tess::Material t;
                    if(tag==0x34305847?ds3native::tess::FromGx04(data,words,t):ds3native::tess::FromGx90(data,words,t)){
                        *reinterpret_cast<uint32_t*>(instance+0x250)=t.mode;*reinterpret_cast<BYTE*>(instance+0x254)=t.displacement;
                        *reinterpret_cast<int32_t*>(instance+0x258)=t.edge;*reinterpret_cast<int32_t*>(instance+0x25C)=t.inside;
                        *reinterpret_cast<float*>(instance+0x260)=t.d7;*reinterpret_cast<float*>(instance+0x26C)=t.d11;*reinterpret_cast<float*>(instance+0x270)=t.d10;
                        *reinterpret_cast<float*>(instance+0x274)=t.fade_start;*reinterpret_cast<float*>(instance+0x278)=t.fade_end;
                        *reinterpret_cast<BYTE*>(instance+0xDF)=1;if(t.gx90)*reinterpret_cast<BYTE*>(instance+0xE4)=1;
                        if(t.mode){static LONG64 rows{};if(InterlockedIncrement64(&rows)<=64){char row[512]{};std::string tess_name,tess_mtd;   // W7: the material and MTD names (ASCII)
                            for(const auto ch:owned->name)if(tess_name.size()<60)tess_name.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');
                            for(const auto ch:owned->mtd)if(tess_mtd.size()<80)tess_mtd.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');
                            sprintf_s(row,"{\"event\":\"native_tess_material\",\"ordinal\":%u,\"item\":\"%s\",\"mode\":%u,\"displacement\":%u,\"factors\":[%d,%d],\"d7\":%g,\"fade\":[%g,%g],\"d10\":%g,\"d11\":%g,\"name\":\"%s\",\"mtd\":\"%s\"}\n",
                                ordinal,t.gx90?"GX90":"GX04",t.mode,t.displacement,t.edge,t.inside,t.d7,t.fade_start,t.fade_end,t.d10,t.d11,tess_name.c_str(),tess_mtd.c_str());Log(row);}}}}
                at+=static_cast<size_t>(length);}}
        // m50 sky (Fable review 2026-10-17; 141070 lines 451-466, 22BEC0): after the items, an MTD with the PoweredColor parameter
        // g_DiffuseColor (its provider, kept in diffuse_provider) whose vt+0x38 holds (INFERRED: the
        // MTD carries g_DiffuseColorPower_UseCAParam; vt+0x38 not read), on a
        // material without GX14 (+0xEF clear): +0xEF = 0, +0xF0 = 1, +0xEE = 0, +0x420..+0x42C = (1, 1, 1, 1) (143D7FEF0).
        // 22E930 then writes FC_DynamicDiffuseColor from the provider (the producer below). Data (AllMaterialBnd): every
        // GXFlver_Col MTD carries it; Fujir_Nolight_OpacityBlend and MultiBlend2_et1_nolight do not (their +0x180 stays 0).
        {const ds3prod::MtdParam* colour{};const ds3prod::MtdParam* power{};const ds3prod::MtdParam* use_colour{};const ds3prod::MtdParam* use_power{};
            for(const auto& parameter:owned->authored->parameters){if(parameter.name=="g_DiffuseColor")colour=&parameter;
                else if(parameter.name=="g_DiffuseColorPower")power=&parameter;else if(parameter.name=="g_DiffuseColor_UseCAParam")use_colour=&parameter;
                else if(parameter.name=="g_DiffuseColorPower_UseCAParam")use_power=&parameter;}
            if(colour && colour->values.size()>=3){NativeModelMaterial::DiffuseProvider provider;
                for(size_t i=0;i<3;++i)provider.colour[i]=static_cast<float>(colour->values[i]);
                if(power && !power->values.empty())provider.power=static_cast<float>(power->values[0]);
                provider.use_colour=use_colour && !use_colour->values.empty() && use_colour->values[0]!=0.0;
                provider.use_power=use_power && !use_power->values.empty() && use_power->values[0]!=0.0;
                owned->diffuse_provider=provider;
                if(use_power && !*reinterpret_cast<const BYTE*>(instance+0xEF)){const std::array<uint32_t,4> unit{0x3F800000u,0x3F800000u,0x3F800000u,0x3F800000u};
                    *reinterpret_cast<BYTE*>(instance+0xEF)=0;*reinterpret_cast<BYTE*>(instance+0xF0)=1;*reinterpret_cast<BYTE*>(instance+0xEE)=0;
                    std::memcpy(reinterpret_cast<void*>(instance+0x420),unit.data(),16);}}}
        // I-0101: slot 7's GX MTD image, its name as FUN_14002DD60 makes it (D1760 the last path component, D16E0 the extension cut).
        {const auto leaf=reinterpret_cast<const wchar_t*(__fastcall*)(const wchar_t*)>(image+0xD1760)(owned->authored->shader.c_str());
            const auto cut=reinterpret_cast<const wchar_t*(__fastcall*)(const wchar_t*)>(image+0xD16E0)(leaf);
            owned->gx_mtd=bootstrap::MakeGxMtdImage(cut?std::wstring(leaf,cut):std::wstring(leaf));}
        {std::lock_guard<std::shared_mutex> hold(model_material_lock);model_materials.emplace(instance,owned);}BumpModelRegistry();
        char row[320]{};sprintf_s(row,"{\"event\":\"native_model_material_cpu\",\"ordinal\":%u,\"textures\":%u,\"mtd_generation\":%llu,\"engine_default_mtd\":%s,\"native_pipeline_ready\":false,\"original_gpu_material\":false}\n",ordinal,number,owned->authored->generation,owned->engine_default_mtd?"true":"false");Log(row);
        return instance;
    }catch(const std::exception& error){char row[320]{};
        sprintf_s(row,"{\"event\":\"native_model_material_error\",\"reason\":\"%s\"}\n",error.what());Log(row);
        Refuse("native_model_material_cpu_admission_failed");}
}
void ConstructNativeModelResolver(uintptr_t root,uintptr_t heap) {
    uintptr_t prior{},temporary{};
    if(!Read(root+0x48,&prior,8) || prior || !Read(image+0x479BF30,&temporary,8))
        Refuse("native_model_resolver_not_pristine");
    const auto storage=reinterpret_cast<uintptr_t(__fastcall*)(size_t,size_t,uintptr_t)>(image+0x1777A00)(0xB0,8,heap);
    if(!storage)throw std::bad_alloc();
    reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t)>(image+0x81B40)(storage,0,heap,temporary);
    static const auto table=[] {std::array<uintptr_t,64> result{};
        result.fill(reinterpret_cast<uintptr_t>(&UnsupportedModelStorage));
        result[0x20/8]=reinterpret_cast<uintptr_t>(&BuildModelLayout);
        result[0x28/8]=reinterpret_cast<uintptr_t>(&BuildModelMaterial);
        result[0x30/8]=image+0xD24490;return result;}();
    *reinterpret_cast<uintptr_t*>(storage)=reinterpret_cast<uintptr_t>(table.data());
    *reinterpret_cast<uintptr_t*>(root+0x48)=storage;
    Log("{\"event\":\"native_model_resolver_cpu_created\",\"original_device_nil\":true,\"world_pipeline_ready\":false}\n");
}
BYTE __fastcall NativeModelShaderTagPredicate(uintptr_t model) noexcept {
    try {
        uint32_t flags{};uintptr_t begin{},end{};
        if(!Read(model+0x224,&flags,4))throw std::runtime_error("native model CPU flags");
        if(flags&0xCu)return 1;
        if(!Read(model+0x1F8,&begin,8) || !Read(model+0x200,&end,8) || end<begin ||
           (end-begin)%8 || (end-begin)/8>65536)throw std::runtime_error("native model CPU material range");
        // 0600A0 only classifies the authored shader's name tags. Read that
        // real CPU identity directly; never create an old GPU material just
        // to satisfy its virtual38 metadata accessor.
        for(auto at=begin;at<end;at+=8){uintptr_t instance{};
            if(!Read(at,&instance,8) || !instance)throw std::runtime_error("native model CPU material entry");
            std::shared_ptr<NativeModelMaterial> material;
            {std::shared_lock hold(model_material_lock);const auto found=model_materials.find(instance);
                if(found==model_materials.end())throw std::runtime_error("native model CPU material owner");material=found->second;}
            const std::wstring& shader=material->gx_mtd->name;   // I-0101 review L7: the shader's stem, made once with the material
            const wchar_t* remaining=shader.c_str();
            using Find=const wchar_t*(__fastcall*)(const wchar_t*,wchar_t);
            using Compare=int(__fastcall*)(const wchar_t*,const wchar_t*,size_t);
            while(const auto underscore=reinterpret_cast<Find>(image+0x1FD3014)(remaining,L'_')){
                remaining=underscore+1;
                if(!reinterpret_cast<Compare>(image+0x1FD3280)(remaining,reinterpret_cast<const wchar_t*>(image+0x3D41E90),6) ||
                   !reinterpret_cast<Compare>(image+0x1FD3280)(remaining,reinterpret_cast<const wchar_t*>(image+0x3D41E78),8))return 1;
                if(!*remaining)break;
            }
        }
        return 0;
    }catch(...){Refuse("native_model_cpu_shader_tag_contract");}
}
void EndRemoOverridesOfArea(uint32_t area) noexcept;
void __fastcall RetireGparam(void* record) noexcept {
    const auto receiver=reinterpret_cast<uintptr_t>(record);int64_t legacy{};
    if(!Read(receiver+0x70,&legacy,8) || legacy!=-1)Refuse("gparam_original_gpu_handle");
    std::shared_ptr<const NativeGparam> keep;
    {uint32_t area{};bool own{};{std::lock_guard<std::mutex> hold(gparam_lock);const auto found=gparams.find(receiver);
            own=found!=gparams.end() && found->second->kind==7 && found->second->area>0;if(own)area=static_cast<uint32_t>(found->second->area);}
        if(own)EndRemoOverridesOfArea(area);}   // I-0141 tranche 3: the backstop above NativeRemoMapHolderDestroy's
    {
        std::lock_guard<std::mutex> hold(gparam_lock);const auto found=gparams.find(receiver);
        if(found!=gparams.end()){
            keep=std::move(found->second);gparams.erase(found);
            for(auto it=gparam_scene_bindings.begin();it!=gparam_scene_bindings.end();)
                it=std::get<2>(it->first)==receiver?gparam_scene_bindings.erase(it):std::next(it);   // by receiver: a re-admitted cap leaves none behind
            PublishNativeGparamBanks();
        }
    }
    // The genuine destructor frees its retained CPU fallback bytes via heap
    // lookup EAA9D0 and its engine string. Its entire GX branch needs +70!=-1.
    gparam_retire_original(record);
}
void __fastcall AdmitGparam(void* record,uintptr_t data,uint64_t size) noexcept {
    if(control->stop)return;
    std::shared_ptr<NativeGparam> owned;
    try {
        const auto receiver=reinterpret_cast<uintptr_t>(record);uintptr_t vtable{},fallback{};int64_t legacy{};
        if(!Read(receiver,&vtable,8) || vtable!=image+0x294EF00 ||
           !Read(receiver+0x70,&legacy,8) || legacy!=-1 || !Read(receiver+0x140,&fallback,8) || fallback ||
           size<0x50 || size>(64u<<20))Refuse("gparam_cpu_input_contract");
        owned=std::make_shared<NativeGparam>();owned->name=ReadCpuString(receiver+0x10);
        owned->bytes.resize(static_cast<size_t>(size));
        if(!Read(data,owned->bytes.data(),owned->bytes.size()))Refuse("gparam_cpu_bytes_unreadable");
        // Preserve D272C0's source-qualified CPU filename classification and
        // numeric bank keys, never its GX registry +4788 or fake legacy handle.
        owned->kind=reinterpret_cast<uint32_t(__fastcall*)(const wchar_t*)>(image+0xD27970)(owned->name.c_str());
        owned->area=reinterpret_cast<int32_t(__fastcall*)(const wchar_t*)>(image+0xD27790)(owned->name.c_str());
        owned->bank=reinterpret_cast<int32_t(__fastcall*)(uint32_t,uint32_t)>(image+0xD27BA0)(owned->kind,0);
        if(owned->kind>=14)Refuse("gparam_cpu_kind_extent");
        *reinterpret_cast<uint32_t*>(receiver+0x60)=owned->kind;
        *reinterpret_cast<int32_t*>(receiver+0x64)=owned->area;
        *reinterpret_cast<int32_t*>(receiver+0x7C)=owned->bank;
        if(owned->kind>=2 && owned->kind<=5) {
            uintptr_t heap{},heap_vtable{},allocate{};
            if(!Read(image+0x479C2D8,&heap,8) || !heap || !Read(heap,&heap_vtable,8) ||
               !Read(heap_vtable+0x48,&allocate,8) || !allocate)Refuse("gparam_cpu_fallback_heap");
            fallback=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,size_t)>(allocate)(heap,owned->bytes.size());
            if(!fallback)Refuse("gparam_cpu_fallback_allocation");
            std::memcpy(reinterpret_cast<void*>(fallback),owned->bytes.data(),owned->bytes.size());
            *reinterpret_cast<uintptr_t*>(receiver+0x140)=fallback;
            *reinterpret_cast<uint64_t*>(receiver+0x148)=size;
        }
        const auto kind=owned->kind;const auto bank=owned->bank;
        {std::lock_guard<std::mutex> hold(gparam_lock);owned->generation=++gparam_generation;
            char row[320]{};sprintf_s(row,"{\"event\":\"native_gparam_cpu_admitted\",\"generation\":%llu,\"bytes\":%zu,\"kind\":%u,\"area\":%d,\"bank\":%d,\"legacy_handle\":false}\n",
                owned->generation,owned->bytes.size(),owned->kind,owned->area,owned->bank);
            gparams.insert_or_assign(receiver,std::move(owned));PublishNativeGparamBanks();Log(row);}
        // D272C0's tail: every kind but 0 outside 6..13, and kind 10, is bound on admission through D275F0 with the weight
        // [143D7EE10] (0 for kind 10).
        if(kind!=0 && (((kind-6u)&0xFFFFFFF8u)!=0 || kind==10)){float weight{};
            if(kind!=10 && !Read(image+0x3D7EE10,&weight,4))Refuse("gparam_cpu_admission_weight");
            BindNativeGparamFileFrom(image+0xD27472,receiver,bank,weight);}   // the return address of D272C0's call (D2746D)
    } catch(const std::exception& e) {
        if(owned){char name[513]{},header[169]{};
            for(size_t n=0;n<owned->name.size() && n<128;++n)
                sprintf_s(name+n*4,sizeof(name)-n*4,"%04x",static_cast<unsigned>(owned->name[n]));
            for(size_t n=0;n<owned->bytes.size() && n<84;++n)
                sprintf_s(header+n*2,sizeof(header)-n*2,"%02x",static_cast<unsigned>(owned->bytes[n]));
            char input[896]{};sprintf_s(input,"{\"event\":\"gparam_cpu_failed_input\",\"bytes\":%zu,\"name_utf16_hex\":\"%s\",\"header_hex\":\"%s\"}\n",owned->bytes.size(),name,header);Log(input);}
        char row[320]{};sprintf_s(row,"{\"event\":\"gparam_cpu_failure\",\"message\":\"%s\"}\n",e.what());Log(row);
        Refuse("gparam_cpu_admission_failed");
    }
}
bool NativeHostRdata(uint32_t rva,void* output,size_t bytes) {
    return rva<0x4800000 && bytes<=0x4800000-rva && Read(image+rva,output,bytes);
}
// Banks::Load's key of a resource name: the lower-case file name up to ".gparam", after a "drawparam_" prefix.
std::string NativeGparamBankKey(const std::wstring& resource_name) {
    std::string name;for(const auto ch:std::filesystem::path(resource_name).filename().wstring())
        name.push_back(ch<128?static_cast<char>(towlower(ch)):'?');
    const size_t extension=name.find(".gparam"),prefix=name.find("drawparam_");
    const size_t from=prefix==std::string::npos?0:prefix+10;
    return name.substr(from,extension==std::string::npos?std::string::npos:extension-from);
}
// Sun source (NATIVE-LIGHTING-MAP "Sun source"): the area's own file (kind 7, the D27BA0 bank 5 the area controller CE6350
// binds through D26A70 -> D275F0; a cutscene's kind 11, bound by NativeRemoGparamStep) is the file the engine's light bank reads
// for that area, rotated by the transform of its binding. Banks gets, per area, the latest such binding. Called with gparam_lock held.
// I-0141 tranche 7: the areas' layers at the last application, their fades, and the transition time a kind-7 binding left for the
// next application (all under gparam_lock).
std::map<uint32_t,std::vector<std::pair<std::string,std::array<float,12>>>> gparam_area_layers_last;
std::map<uint32_t,ds3light::Banks::AreaFade> gparam_area_fades;
std::map<uint32_t,float> gparam_pending_fades;
std::map<uint32_t,uint64_t> gparam_area_fade_steps;   // the controller step that started each fade (0: none)
// NativeAreaControllerStep's span (the genuine step and its unbinding): the thread stepping and the step's serial, atomics read by
// ApplyNativeGparamAreaBindings on that thread only (the second review's L2 and L3: no lock taken a step, another thread not in it).
std::atomic<DWORD> gparam_controller_step_thread{};std::atomic<uint64_t> gparam_controller_step_serial{};
void ApplyNativeGparamAreaBindings() {
    if(!gparam_banks)return;
    // D-0180 C4 (Native1752 m45): every live layer is kept, latest first; a lookup takes the latest layer holding its value ID
    // (m45's overlay m45_00_0200 lacks the sets 1, 8, 10, 100, 101 that the original reads from m45_00_0000).
    std::map<uint32_t,std::vector<const NativeGparamSceneBinding*>> live;
    // I-0140 tranche 7: and a cutscene's own file (kind 11, area 2AABBVVVV), bound by the Remo step (NativeRemoGparamStep) with the
    // map's transform: the light bank rotates its records by that binding as by a map's (i140-e16: the original's shadow sun of
    // 31000010's set 22 the map's direction).
    // (the step review's L3: under its own area only, never as a map area's layer through an extra-area list).
    for(const auto& [key,binding]:gparam_scene_bindings){(void)key;
        if(binding.resource->kind==7 || (binding.resource->kind==11 && binding.area==static_cast<uint32_t>(binding.resource->area)))live[binding.area].push_back(&binding);}
    std::map<uint32_t,std::vector<std::pair<std::string,std::array<float,12>>>> layers;
    for(auto& [area,list]:live){std::sort(list.begin(),list.end(),[](const auto* a,const auto* b){return a->sequence>b->sequence;});
        for(const auto* binding:list){std::array<float,12> transform{};std::memcpy(transform.data(),binding->metadata.data(),48);
            layers[area].emplace_back(NativeGparamBankKey(binding->resource->name),transform);}
        if(list.size()>1){static LONG64 rows{};if(InterlockedIncrement64(&rows)<=64){char row[160]{};
            sprintf_s(row,"{\"event\":\"native_gparam_area_layers\",\"area\":%u,\"live_layers\":%zu}\n",area,list.size());Log(row);}}}
    // I-0141 tranche 7 (STUDY-LAYER-FADE): an area whose layers change with a transition time fades from its earlier layers' values (value by value: Shadow and the group values since tranche 23, Sun and Fog since tranche 24)
    // to the new ones' (Banks::SetAreaFades). The time is the one the area controller CE6350 gives that step's unbinding and binding
    // alike (its +0xC0, or +0x10C when not negative; 0 when its caps were found in that step, when its +0x15 is set, or when its +0xC4
    // changed while +0xC5 was set: READ, GhidraMCP 2026-11-15), which reaches the native as the weight of the step's kind-7 binding
    // (RecordNativeGparamBinding, gparam_pending_fades). Natively one step changes the layers twice: the binding (through D275F0's
    // hook), then the unbinding of the cap it replaces (NativeAreaControllerStep); a fade started in the same controller step
    // (gparam_controller_step_thread and _serial, the step review's L3) keeps its earlier layers, so the pair is one fade from the step's first layers.
    // A change without a time ends the area's fade; an unbinding alone takes the step's time (NativeAreaControllerStep).
    // Kept across a bank's republication. The fades are given to the bank before the layers (the step review's L1: in between, the
    // earlier and current layers are the same, no jump).
    const double now=ds3light::FadeClock();
    std::set<uint32_t> areas;for(const auto& [area,list]:layers)areas.insert(area);for(const auto& [area,list]:gparam_area_layers_last)areas.insert(area);
    for(const auto area:areas){
        const auto next=layers.find(area);const auto last=gparam_area_layers_last.find(area);
        const bool changed=(next==layers.end())!=(last==gparam_area_layers_last.end()) || (next!=layers.end() && next->second!=last->second);
        const auto pending=gparam_pending_fades.find(area);const float time=pending!=gparam_pending_fades.end()?pending->second:0.0f;
        if(!changed)continue;
        const auto running=gparam_area_fades.find(area);
        const bool stepping=gparam_controller_step_thread.load(std::memory_order_acquire)==GetCurrentThreadId();
        const uint64_t step=stepping?gparam_controller_step_serial.load(std::memory_order_acquire):0;
        const bool same_step=running!=gparam_area_fades.end() && step && gparam_area_fade_steps[area]==step;
        if(same_step){running->second.start=now;if(0.0f<time)running->second.duration=time;}
        else if(0.0f<time && last!=gparam_area_layers_last.end() && !last->second.empty()){gparam_area_fades[area]=ds3light::Banks::AreaFade{last->second,now,time};
            gparam_area_fade_steps[area]=step;}
        else if(running!=gparam_area_fades.end())gparam_area_fades.erase(running);
        if(const auto made=gparam_area_fades.find(area);made!=gparam_area_fades.end() && made->second.start==now){static LONG64 rows{};if(InterlockedIncrement64(&rows)<=64){char row[200]{};
            sprintf_s(row,"{\"event\":\"native_gparam_area_fade\",\"area\":%u,\"seconds\":%.3g,\"from_layers\":%zu,\"to_layers\":%zu}\n",area,made->second.duration,
                made->second.from.size(),next==layers.end()?size_t{0}:next->second.size());Log(row);}}}
    for(auto it=gparam_area_fades.begin();it!=gparam_area_fades.end();)it=now-it->second.start>=it->second.duration?gparam_area_fades.erase(it):std::next(it);
    gparam_area_layers_last=layers;
    gparam_pending_fades.clear();   // a time only for the application its binding makes
    gparam_banks->SetAreaFades(gparam_area_fades);
    gparam_banks->SetAreaLayers(layers);
    gparam_banks->SetRecordOverrides(remo_record_overrides);   // I-0141 tranche 3: kept across a bank's republication
}
// One binding of a resident file into the engine's scene bank (49D00's bookkeeping; the transform is the receiver's +0x80
// at the moment of the binding, as 49D00 passes it to 37340). Called with gparam_lock held.
void RecordNativeGparamBinding(uintptr_t caller,const std::shared_ptr<const NativeGparam>& resource,uintptr_t receiver,
    uint32_t area,uint32_t bank,uint32_t weight_bits) {
    NativeGparamSceneBinding binding{resource,area,bank,weight_bits,{},++gparam_binding_sequence};
    if(!Read(receiver+0x80,binding.metadata.data(),binding.metadata.size()))Refuse("native_gparam_scene_metadata");
    std::array<float,12> transform{};std::memcpy(transform.data(),binding.metadata.data(),48);std::string name;
    for(const auto ch:resource->name)name.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');
    gparam_scene_bindings.insert_or_assign({area,bank,receiver},std::move(binding));
    char extra[640]{};sprintf_s(extra,"{\"event\":\"native_gparam_scene_binding_source\",\"caller\":%llu,\"area\":%u,\"bank\":%u,\"kind\":%u,\"weight_bits\":%u,\"name\":\"%s\",\"transform\":[%.7g,%.7g,%.7g,%.7g,%.7g,%.7g,%.7g,%.7g,%.7g,%.7g,%.7g,%.7g]}\n",
        static_cast<unsigned long long>(caller-image),area,bank,resource->kind,weight_bits,name.substr(0,96).c_str(),transform[0],transform[1],transform[2],transform[3],transform[4],transform[5],
        transform[6],transform[7],transform[8],transform[9],transform[10],transform[11]);Log(extra);
    // I-0141 tranche 7: a map file's binding weight is its area controller's transition time (D26A70 passes it to D275F0).
    if(float time{};resource->kind==7 && (std::memcpy(&time,&weight_bits,4),std::isfinite(time)) && 0.0f<time)gparam_pending_fades[area]=time;
    if(resource->kind==7 || resource->kind==11)ApplyNativeGparamAreaBindings();
}
// Called with gparam_lock held. Immutable bank snapshots and the existing
// publication serials retain each real CPU input independently of its engine
// allocation. No captured bank, writer output or GPU handle is introduced.
void PublishNativeGparamBanks() {
    LARGE_INTEGER start{};QueryPerformanceCounter(&start);
    struct Timed {LARGE_INTEGER start;~Timed(){LARGE_INTEGER end{};QueryPerformanceCounter(&end);const uint64_t t=uint64_t(end.QuadPart-start.QuadPart);
        native_gparam_publish_ticks.fetch_add(t,std::memory_order_relaxed);native_gparam_publishes.fetch_add(1,std::memory_order_relaxed);
        auto known=native_gparam_publish_max.load();while(t>known && !native_gparam_publish_max.compare_exchange_weak(known,t)){}
        if(GetCurrentThreadId()==owner_thread)native_gparam_publishes_owner.fetch_add(1,std::memory_order_relaxed);}} timed{start};
    std::call_once(native_host_inputs_once,[]{
        ds3hostio::MemoryOnly(native_host_root);
        ds3light::UseReader(&ds3hostio::ReadForProducers);
        ds3light::UseSharedReader(&ds3hostio::SharedForProducers);
        ds3light::UseListingReader(&ds3hostio::List);
        ds3light::UseRdataReader(&NativeHostRdata);
        ds3light::UseRecordBlends(&NativeLightRecordBlend,&NativeFogRecordBlend);   // I-0141 tranche 3
    });
    const auto folder=native_host_root/L"banks";
    std::map<std::filesystem::path,std::shared_ptr<const NativeGparam>> current;
    for(const auto& [receiver,resource]:gparams){
        // D272C0 classifies the short CPU resource name (default, MapBase,
        // mAA_BB_CCCC). D27D30's actual file format is gparam:/%s.gparam.
        // Recreate only this virtual file key required by existing Banks::Load.
        (void)receiver;auto name=std::filesystem::path(resource->name).filename().wstring();
        for(auto& c:name)c=static_cast<wchar_t>(towlower(c));
        if(name.find(L".gparam")==std::wstring::npos)name+=L".gparam";
        const auto file=folder/name;
        auto& selected=current[file];
        if(!selected || resource->generation>selected->generation)selected=resource;
    }
    // I-0097 tranche 5: a retired file also leaves the bank folder's listing; kept, DS3's reload of the same map parts listed
    // each GPARAM once more at every reload and every publication walked them all (0.19 to 0.75 ms over twelve reloads).
    std::map<std::filesystem::path,std::shared_ptr<const std::vector<uint8_t>>> files;
    for(const auto& [path,resource]:current)files.emplace(path,std::shared_ptr<const std::vector<uint8_t>>(resource,&resource->bytes));
    ds3hostio::PublishFolder(folder,gparam_host_paths,files);
    auto banks=std::make_shared<ds3light::Banks>();banks->Load(folder,gparam_banks.get());   // I-0079: unchanged files keep their parsed form
    if(banks->Files()!=current.size())Refuse("native_gparam_host_file_publication");
    gparam_banks=banks;ApplyNativeGparamAreaBindings();
    // Reuse the host's typed memory entry and its lifetime, not a new renderer.
    ds3hostio::PutLazy<ds3light::Banks>(native_host_root/L"current-banks",gparam_banks,[] {return std::vector<std::string>{};});
}
void __fastcall BindNativeGparamScene(uintptr_t excluded_manager,int64_t excluded_handle,
    uint32_t area,uint32_t bank,uintptr_t metadata,uint32_t weight_bits) noexcept {
    try {
        const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());
        // Both genuine CPU area lists call the same binding: D26AB0(E0/108)
        // and D26C00(110/138). Each suppresses duplicates only in its own list.
        const bool secondary=caller==image+0xD26D3A;
        uintptr_t root{};
        if((caller!=image+0xD26BEA && !secondary) ||
           !Read(image+0x4961B50,&root,8) || !root || root!=native_scene_root || excluded_manager!=root+0x41C0 || excluded_handle!=-1 || metadata<0x80)
            Refuse("native_gparam_scene_binding_scope");
        const auto receiver=metadata-0x80;const std::lock_guard hold(gparam_lock);
        const auto found=gparams.find(receiver);
        if(found==gparams.end() || static_cast<uint32_t>(found->second->bank)!=bank || !gparam_banks)
            Refuse("native_gparam_scene_binding_owner");
        uint64_t count{};std::array<uint32_t,8> areas{};
        if(!Read(receiver+(secondary?0x138:0x108),&count,8) || !count || count>areas.size() ||
           !Read(receiver+(secondary?0x110:0xE0),areas.data(),static_cast<size_t>(count)*4) ||
           std::find(areas.begin(),areas.begin()+static_cast<size_t>(count),area)==areas.begin()+static_cast<size_t>(count))
            Refuse("native_gparam_scene_cpu_area");
        RecordNativeGparamBinding(caller,found->second,receiver,area,bank,weight_bits);
        char row[448]{};sprintf_s(row,"{\"event\":\"native_gparam_scene_binding\",\"generation\":%llu,\"area\":%u,\"bank\":%u,\"weight_bits\":%u,\"host_bank_files\":%zu,\"cpu_area_list_offset\":%u,\"cpu_area_count\":%llu,\"existing_host_memory_source\":true,\"actual_cpu_area_bookkeeping\":true,\"legacy_gpu_handle\":false,\"native_scene_draw_ready\":false}\n",
            found->second->generation,area,bank,weight_bits,gparam_banks->Files(),secondary?0x110u:0xE0u,count);Log(row);
    }catch(...){Refuse("native_gparam_scene_binding_failed");}
}
// FUN_140D275F0 (cap, bank, weight), the binding of one resident file: D272C0's admission tail, D26A60 (CDEE50, the Remo
// cutscene's gparam step, its call gated on a handle natively absent: NativeRemoGparamStep calls this instead) and D26A70 (CE6350,
// the area controller: kind 7 on bank D27BA0(7)) end in it. The original returns unless the file
// has its GX handle (+0x70 != -1); a native file never has one, so the native rule is "admitted natively" instead (a file
// not admitted yet is skipped, as the original skips one not loaded yet). The rest is the original: bound once (+0x78),
// bank stored at +0x7C, area from D27670, then 49D00's bookkeeping with the receiver's +0x80 transform and the weight.
void BindNativeGparamFileFrom(uintptr_t caller,uintptr_t cap,int32_t bank,float weight) {
    try {
        const std::lock_guard hold(gparam_lock);
        const auto found=gparams.find(cap);BYTE bound{};
        if(found==gparams.end() || !Read(cap+0x78,&bound,1) || bound || bank==-1)return;
        *reinterpret_cast<int32_t*>(cap+0x7C)=bank;*reinterpret_cast<BYTE*>(cap+0x78)=1;
        alignas(16) std::array<uint32_t,4> area{};
        reinterpret_cast<void(__fastcall*)(uintptr_t,uint32_t*)>(image+0xD27670)(cap,area.data());
        uint32_t weight_bits{};std::memcpy(&weight_bits,&weight,4);
        RecordNativeGparamBinding(caller,found->second,cap,area[0],static_cast<uint32_t>(bank),weight_bits);
    }catch(...){Refuse("native_gparam_file_binding_failed");}
}
void __fastcall BindNativeGparamFile(uintptr_t cap,int32_t bank,float weight) noexcept {
    BindNativeGparamFileFrom(reinterpret_cast<uintptr_t>(_ReturnAddress()),cap,bank,weight);
}
// FUN_140CE6350, the area controller (ten variant caps at +0x48, five layer indices at +0x98, their targets at +0xAC): its
// bind block unbinds the caps its layers leave through FUN_140D26DD0 (lines 117-126) and binds the new ones through D26A70.
// The unbinding is gated inline on the handle (+0x70 != -1), so natively it never happens and a cap left bound refuses its
// next binding (a variant A -> B -> A would keep B). After each step the bound set of an active controller is its layers'
// selection; the native step restores that: a natively bound cap of its slots that no layer selects is unbound as D26DD0
// does (bank +0x7C back to -1, +0x78 cleared, the binding removed). The step's own binds already ran through the D275F0 hook.
using AreaControllerStep=void(__fastcall*)(uintptr_t);
AreaControllerStep area_controller_step_original{};
void __fastcall NativeAreaControllerStep(uintptr_t controller) noexcept {
    // I-0141 tranche 7: the step's span on this thread, so that its binding and its unbinding make one fade (the step review's L3).
    gparam_controller_step_serial.fetch_add(1,std::memory_order_acq_rel);gparam_controller_step_thread.store(GetCurrentThreadId(),std::memory_order_release);
    struct StepEnd {~StepEnd(){gparam_controller_step_thread.store(0,std::memory_order_release);}} step_end;
    // The final review's M1: CE6350 gives its unbinding (D26DD0) the step's time too, computed before its bind block: +0xC0, or +0x10C
    // when not negative; 0 when the step fetches its caps (+0x13, INFERRED: the time is used only when they were all found, bVar3),
    // when +0x15 is set, or when +0xC4 differs from +0xC5 while +0xC5 is set (READ, GhidraMCP 2026-11-15). Read before the genuine step
    // changes them; the unbinding below then fades over it as the original's layer beneath does (STUDY-LAYER-FADE §3).
    // The step sets +0x13 itself when +0x11 is set and its area word +0x18 differs from +0x2C (asm 140CE63B1..140CE63CD: the step
    // review's M1), so that case counts as a fetch too.
    float unbind_time{};{BYTE b11{},b13{},b15{},c4{},c5{};float c0{},t10c{-1.0f};uint32_t a18{},a2c{};
        if(Read(controller+0x11,&b11,1) && Read(controller+0x13,&b13,1) && Read(controller+0x15,&b15,1) && Read(controller+0xC4,&c4,1) && Read(controller+0xC5,&c5,1) &&
           Read(controller+0x18,&a18,4) && Read(controller+0x2C,&a2c,4) && Read(controller+0xC0,&c0,4) && Read(controller+0x10C,&t10c,4)){
            unbind_time=0.0f<=t10c?t10c:c0;if(b13 || (b11 && a18!=a2c) || b15 || (c4!=c5 && c5))unbind_time=0.0f;}}
    area_controller_step_original(controller);
    try {
        BYTE active{};std::array<uintptr_t,10> slots{};std::array<uint32_t,5> layers{};
        if(!Read(controller+0x10,&active,1) || !active || !Read(controller+0x48,slots.data(),sizeof slots) || !Read(controller+0x98,layers.data(),sizeof layers))return;
        std::set<uintptr_t> selected;for(const auto index:layers)if(index<slots.size() && slots[index])selected.insert(slots[index]);
        if(selected.empty())return;   // the bind block has not run (the caps are not all found yet)
        if(std::none_of(slots.begin(),slots.end(),[&](uintptr_t cap){BYTE bound{};return cap && !selected.contains(cap) && Read(cap+0x78,&bound,1) && bound;}))return;   // per step: no lock unless a cap is left bound
        const std::lock_guard hold(gparam_lock);bool changed{};
        for(const auto cap:slots){BYTE bound{};int32_t bank{};
            if(!cap || selected.contains(cap) || !gparams.contains(cap) || !Read(cap+0x78,&bound,1) || !bound || !Read(cap+0x7C,&bank,4))continue;
            alignas(16) std::array<uint32_t,4> area{};reinterpret_cast<void(__fastcall*)(uintptr_t,uint32_t*)>(image+0xD27670)(cap,area.data());
            if(std::isfinite(unbind_time) && 0.0f<unbind_time)gparam_pending_fades[area[0]]=unbind_time;   // the final review's M1
            gparam_scene_bindings.erase({area[0],static_cast<uint32_t>(bank),cap});
            *reinterpret_cast<int32_t*>(cap+0x7C)=-1;*reinterpret_cast<BYTE*>(cap+0x78)=0;changed=true;
            char row[160]{};sprintf_s(row,"{\"event\":\"native_gparam_file_unbinding\",\"area\":%u,\"bank\":%d,\"selected\":%zu}\n",area[0],bank,selected.size());Log(row);}
        if(changed)ApplyNativeGparamAreaBindings();
    }catch(...){Refuse("native_area_controller_step_failed");}
}
// I-0140 tranche 7: FUN_140CDEE50, the Remo cutscene's gparam step (SprjRemoRes). Once its resources are ready it makes the
// MapOverride_forRemo cap (CSGparamResCap_Create into +0x100) and, gated inline on that cap's handle (+0x70 != -1, never set
// natively), binds the cutscene's own file: the cap [[+0xF0]+0x80], its transform +0x80..+0xAF taken from the map's
// [[+0xE8]+0x80] +0x5C0..+0x5EF when CE1DB0 lists the cutscene's id ([+0xC0]+0xC, through CE1930), then the binding with the
// cap's bank and area at weight 0 (the call at CDF12C; the original's row i140-e16-c31000010-orig: s31_00_0010, kind 11, bank 13,
// the map's transform); then 49D00 for MapOverride_forRemo (a copy of `empty`, weight 0: no record, R's reading). Natively the
// gate skips both, so a cutscene's light sets kept no rotation (31000010's sun 156 degrees off, i140-e16). The native step binds
// the cutscene's file as the original does, once the genuine step has taken its ready branch (CDF970, called there alone, adds 1
// to the step counter +0x4C); MapOverride_forRemo's binding is left (no record). The copy and the binding come after the genuine
// step's CDF970 and its vt+0x30 (the original's: before them); no bank lookup runs between.
using RemoGparamStep=void(__fastcall*)(uintptr_t);
RemoGparamStep remo_gparam_step_original{};
void __fastcall NativeRemoGparamStep(uintptr_t remo) noexcept {
    uint32_t before{};uintptr_t made_before{};Read(remo+0x4C,&before,4);Read(remo+0x100,&made_before,8);
    remo_gparam_step_original(remo);
    try {
        // The ready branch writes both the step counter (CDF970) and +0x100 (the new cap); vt+0x30 after it is unread, so either
        // counts (the final review's M1); the cap's +0x78 keeps a second pass from binding twice.
        uint32_t after{};uintptr_t made{},holder{},cap{};
        if(!Read(remo+0x4C,&after,4) || !Read(remo+0x100,&made,8) || !made || (after==before && made==made_before))return;
        if(!Read(remo+0xF0,&holder,8) || !holder || !Read(holder+0x80,&cap,8) || !cap)return;
        uintptr_t map_holder{},map{},scene{};
        if(Read(remo+0xE8,&map_holder,8) && map_holder && Read(map_holder+0x80,&map,8) && map && Read(remo+0xC0,&scene,8) && scene){
            uint64_t slot{};const auto id=reinterpret_cast<const int32_t*(__fastcall*)(uintptr_t,uint64_t*)>(image+0xCE1930)(scene,&slot);
            if(id && reinterpret_cast<bool(__fastcall*)(const int32_t*)>(image+0xCE1DB0)(id)){std::array<uint8_t,0x30> rows{};
                if(!Read(map+0x5C0,rows.data(),rows.size()))return;
                std::memcpy(reinterpret_cast<void*>(cap+0x80),rows.data(),rows.size());}}
        int32_t bank{};BYTE bound{};const bool banked=Read(cap+0x7C,&bank,4) && bank!=-1;
        if(banked)BindNativeGparamFileFrom(image+0xCDF131,cap,bank,0.0f);   // the original's binding of the cap at weight 0 (the return address of CDF12C)
        if(!banked || !Read(cap+0x78,&bound,1) || !bound){static LONG64 rows{};if(InterlockedIncrement64(&rows)<=64){char row[160]{};   // the final review's L7
            sprintf_s(row,"{\"event\":\"native_remo_gparam_unbound\",\"bank\":%d,\"admitted\":%s}\n",bank,[&]{const std::lock_guard hold(gparam_lock);return gparams.contains(cap);}()?"true":"false");Log(row);}}
    }catch(...){Refuse("native_remo_gparam_step_failed");}
}
// I-0141 tranche 3 (ROADMAP workstream 7 step 7): the cutscene act IngameDrawParam (FUN_140CC8180, a virtual call each update
// of a Remo scene that names one) overrides the map's light and fog records by the cutscene file's: FUN_140031A20 (the light
// sets' bank [GXRS+0x41E0]) and FUN_1400380A0 (the fog's, +0x41E8; the act is the one caller of both and of the two removers,
// their other references being unwind data: GhidraMCP's references, the final review's M3) take (bank, key {area, id}, layer, key A, key B, t) and make
// the record of `key` A's, or A and B blended at t when t > 0 (140147ED0 on the light sets' +0x40 records, 14014E5D0 on the fog's
// +0x10), registering the key on `layer` (the kind-9 layer MapOverride_forRemo, bank 11) in the layer table (READ, GhidraMCP,
// 2026-11-15). In the original the ceremony scene 32000000 writes 18 such (bank, key, layer, A, B), and natively the same 18
// (OBSERVED, i141-t2b-c32000010-orig, i141-t2c-c32000010-nat), into engine banks the native renderer does not read. Here the
// genuine functions still run (pass-through: the engine's own state as in the original) and the override is kept in
// remo_record_overrides, which every published Banks answers (Banks::SetRecordOverrides): the deferred and forward light sets,
// the instance rows, the traced sun's lookups, the effects' fog. The act's update itself is hooked too (NativeRemoDrawParamAct):
// the table goes to the banks once after each update that changed it (the act writes several keys an update, their t moving at
// a blend; one generation step an update, the step review's L9). An override ends: FUN_1400320B0 and FUN_1400387D0 (bank, id
// from, id to, area, layer) remove the ids from..to of an area's layer when the act's track names none (the count to - from + 1
// when not negative, else none: 320B0's loop; never observed in a run); the cutscene's map holder's destructor FUN_140CDCCD0
// withdraws the kind-9 layer of its area (+0xD4) through GparamFile_Withdraw_14004A060 when it made its MapOverride_forRemo cap
// (+0x100), the cap's GX handle gate (there in the original, never natively) left out. INFERRED: that +0xD4 is the area of the
// act's key (FUN_140CDDE40 reads +0xD4 of the object the act reaches through CC1350 and CCDB30). How 4A060 withdraws a bank's
// layer, and the act's engine nodes in its five banks (tranche 4: NativeRemoMapHolderDestroy, below). A backstop (the step
// review's M2): the retirement of an area's own file (kind 7) ends that area's overrides, with a row. The act is the one caller of
// its five banks' writers and removers (GhidraMCP's references: CC8180 alone, the others unwind data; tranche 4's step review L1). With light-observe.flag, the observers' rows of 04c_scene_witnesses.inc as
// before. no-remo-overrides.flag leaves the banks without the overrides (the genuine functions still run).
using RemoMapHolderDestroy=void(__fastcall*)(uintptr_t);
RemoMapHolderDestroy remo_map_holder_destroy_original{};
bool RemoOverridesOff(){static const bool off=ModuleFlag(L"no-remo-overrides.flag");return off;}
bool remo_overrides_dirty{};   // a change not yet given to the banks (the act's update flushes it); guarded by gparam_lock
// I-0141 tranche 4: the record nodes the act's genuine writers made in the engine's banks, (group, area, id, layer), apart from
// the overrides (the step review's L1 and L2: the backstop and no-remo-overrides.flag erase or skip overrides, never these); taken out
// by the genuine removers at the holder's withdrawal, and dropped when the act's own removal took them; guarded by gparam_lock.
std::set<std::array<uint32_t,4>> remo_engine_nodes;
// The act's five banks (CC8180: the same override written into each, the same removal from each; the final review's M2): group 0 the
// light sets [GXRS+0x41E0] (31A20, its remover 320B0), 1 the fog +0x41E8 (380A0, 387D0), 2 LightScattering +0x41F0 (3C900, 3D080),
// 3 Env Map +0x42A8 (40DD0, 40690), 4 Snow Model +0x4600 (45CA0, 45560). Groups 0 and 1 are also given to Banks (the overrides);
// the other three are followed for their engine nodes only (the native reads them by key from Banks' own files).
constexpr uint32_t remo_bank_offsets[5]{0x41E0,0x41E8,0x41F0,0x42A8,0x4600};
BankOverride remo_bank_write_original[5]{};BankOverrideRemove remo_bank_remove_original[5]{};   // 2 to 4 (0 and 1: light_ and fog_override_*_original)
void RecordRemoEngineNode(uint32_t group,const uint32_t* key,uint32_t layer) noexcept {
    uint32_t w[2]{};if(!key || !Read(reinterpret_cast<uintptr_t>(key),w,8))return;
    const std::lock_guard hold(gparam_lock);remo_engine_nodes.insert({group,w[0],w[1],layer});}
// Called with gparam_lock held.
void PublishRemoOverrides(){remo_overrides_dirty=false;if(gparam_banks)gparam_banks->SetRecordOverrides(remo_record_overrides);}
void RecordRemoOverride(uint32_t group,const uint32_t* key,uint32_t layer,const uint32_t* a,const uint32_t* b,float t) noexcept {
    uint32_t w[6]{};
    if(!key || !a || !b || !Read(reinterpret_cast<uintptr_t>(key),w,8) || !Read(reinterpret_cast<uintptr_t>(a),w+2,8) || !Read(reinterpret_cast<uintptr_t>(b),w+4,8))return;
    if(!std::isfinite(t))t=0.f;
    ds3light::Banks::RecordOverride o{};std::memcpy(o.a,w+2,8);std::memcpy(o.b,w+4,8);o.t=t;o.layer=layer;
    bool fresh{};size_t count{};
    {const std::lock_guard hold(gparam_lock);remo_engine_nodes.insert({group,w[0],w[1],layer});   // the genuine function made or kept it
        if(RemoOverridesOff())return;   // one lock a write (the step review's L4)
        const auto [slot,inserted]=remo_record_overrides.try_emplace({group,w[0],w[1]},o);fresh=inserted;
        auto& kept=slot->second;
        if(inserted || std::memcmp(kept.a,o.a,8) || std::memcmp(kept.b,o.b,8) || kept.t!=o.t || kept.layer!=o.layer){kept=o;remo_overrides_dirty=true;}
        count=remo_record_overrides.size();}
    static LONG64 rows{};if(fresh && InterlockedIncrement64(&rows)<=64){char row[256]{};
        sprintf_s(row,"{\"event\":\"native_remo_override\",\"bank\":\"%s\",\"key\":[%u,%d],\"layer\":%u,\"a\":[%u,%d],\"b\":[%u,%d],\"t\":%.4g,\"overrides\":%zu}\n",
            group?"fog":"light",w[0],static_cast<int32_t>(w[1]),layer,w[2],static_cast<int32_t>(w[3]),w[4],static_cast<int32_t>(w[5]),static_cast<double>(t),count);Log(row);}
}
// Erases the overrides `match` names and returns their count; a row for a call that erased any. Called without gparam_lock.
template<class Match> size_t EraseRemoOverrides(const char* reason,uint32_t area,uint32_t layer,Match match) noexcept {
    size_t erased{},left{};
    {const std::lock_guard hold(gparam_lock);
        for(auto it=remo_record_overrides.begin();it!=remo_record_overrides.end();)
            if(match(it->first,it->second)){it=remo_record_overrides.erase(it);++erased;}else ++it;
        left=remo_record_overrides.size();if(erased)PublishRemoOverrides();}
    static LONG64 rows{};if(erased && InterlockedIncrement64(&rows)<=64){char row[200]{};
        sprintf_s(row,"{\"event\":\"native_remo_override_end\",\"reason\":\"%s\",\"area\":%u,\"layer\":%u,\"erased\":%zu,\"left\":%zu}\n",reason,area,layer,erased,left);Log(row);}
    return erased;
}
void __fastcall NativeRemoLightOverride(uintptr_t bank,const uint32_t* key,uint32_t layer,const uint32_t* a,const uint32_t* b,float t) noexcept {
    static const bool observe=ModuleFlag(L"light-observe.flag");if(observe)LogBankOverride(0,key,layer,a,b,t);
    light_override_original(bank,key,layer,a,b,t);RecordRemoOverride(0,key,layer,a,b,t);}
void __fastcall NativeRemoFogOverride(uintptr_t bank,const uint32_t* key,uint32_t layer,const uint32_t* a,const uint32_t* b,float t) noexcept {
    static const bool observe=ModuleFlag(L"light-observe.flag");if(observe)LogBankOverride(1,key,layer,a,b,t);
    fog_override_original(bank,key,layer,a,b,t);RecordRemoOverride(1,key,layer,a,b,t);}
void EraseRemoOverrideRange(uint32_t group,uint32_t from,uint32_t to,uint32_t area,uint32_t layer) noexcept {
    if(static_cast<int32_t>(to-from)<0)return;   // 320B0: no id when to - from is negative (Banks::RemovedId)
    {const std::lock_guard hold(gparam_lock);   // the engine took those nodes itself
        for(auto it=remo_engine_nodes.begin();it!=remo_engine_nodes.end();)
            if((*it)[0]==group && (*it)[1]==area && (*it)[3]==layer && ds3light::Banks::RemovedId((*it)[2],from,to))it=remo_engine_nodes.erase(it);else ++it;}
    EraseRemoOverrides(group?"fog_removed":"light_removed",area,layer,[&](const auto& k,const ds3light::Banks::RecordOverride& o){
        return std::get<0>(k)==group && std::get<1>(k)==area && o.layer==layer && ds3light::Banks::RemovedId(std::get<2>(k),from,to);});}
void __fastcall NativeRemoLightOverrideRemove(uintptr_t bank,uint32_t from,uint32_t to,uint32_t area,uint32_t layer,uint64_t last) noexcept {
    static const bool observe=ModuleFlag(L"light-observe.flag");if(observe)LogBankOverrideRemove(0,from,to,area,layer);
    light_override_remove_original(bank,from,to,area,layer,last);EraseRemoOverrideRange(0,from,to,area,layer);}
void __fastcall NativeRemoFogOverrideRemove(uintptr_t bank,uint32_t from,uint32_t to,uint32_t area,uint32_t layer,uint64_t last) noexcept {
    static const bool observe=ModuleFlag(L"light-observe.flag");if(observe)LogBankOverrideRemove(1,from,to,area,layer);
    fog_override_remove_original(bank,from,to,area,layer,last);EraseRemoOverrideRange(1,from,to,area,layer);}
// The act's three other banks: their writes and removals, pass-through, followed in the ledger only.
template<uint32_t Group> void __fastcall NativeRemoBankWrite(uintptr_t bank,const uint32_t* key,uint32_t layer,const uint32_t* a,const uint32_t* b,float t) noexcept {
    remo_bank_write_original[Group](bank,key,layer,a,b,t);RecordRemoEngineNode(Group,key,layer);}
template<uint32_t Group> void __fastcall NativeRemoBankRemove(uintptr_t bank,uint32_t from,uint32_t to,uint32_t area,uint32_t layer,uint64_t last) noexcept {
    remo_bank_remove_original[Group](bank,from,to,area,layer,last);EraseRemoOverrideRange(Group,from,to,area,layer);}
void __fastcall NativeRemoMapHolderDestroy(uintptr_t holder) noexcept {
    uint32_t area{};uintptr_t cap{};const bool known=holder && Read(holder+0xD4,&area,4) && Read(holder+0x100,&cap,8) && cap;   // CDCCD0 withdraws only with its cap
    const auto layer=static_cast<uint32_t>(reinterpret_cast<int32_t(__fastcall*)(uint32_t,uint32_t)>(image+0xD27BA0)(9,0));   // GetGparamKindRegisteredId(9)
    remo_map_holder_destroy_original(holder);
    if(!known)return;
    EraseRemoOverrides("map_holder_withdrawn",area,layer,[&](const auto& k,const ds3light::Banks::RecordOverride& o){return std::get<1>(k)==area && o.layer==layer;});
    // I-0141 tranche 4 (tranche 3's final review, M2): the act's genuine writers also made its record nodes in the engine's own banks
    // (its five, remo_bank_offsets). The original's withdrawal takes them away: 4A060 calls each bank's layer withdrawal (37B50 for the
    // fog's, READ, GhidraMCP 2026-11-15: the layer unbound in the layer table by 96B70, then each node of the area destroyed where no
    // live layer still holds it, 96DA0, else marked to be rebuilt from the layers left; the light sets' bank goes through 314D0, not
    // read). The native's never did (4A060 behind the cap's GX handle): a key the native renderer leaves to the engine's lookup
    // ({area, -1}, NativeGparamRecords' I-0113 rule) found the act's record after the scene. Natively those banks hold no file and
    // their layer table no map layer (49D00 is BindNativeGparamScene's), so the act's nodes are theirs alone and no live layer holds
    // them: the original's rule then destroys each one, as the genuine removers here do, a node at a time, as the act's own removal
    // does (from = to = the id, the area, the layer, a sixth word 0 as CC8180 passes it), from the ledger remo_engine_nodes. Each
    // remover finds the area's node and the id's node and does nothing where either is missing (READ: 320B0 and 387D0, their
    // « node != end » tests; the other three INFERRED alike). The withdrawal is by area and layer, as 4A060's: a second act of the
    // same area that wrote keys before this holder goes loses them until its next write. On the destructor's thread, the act's (the
    // game's); the row names it. The banks are read before the ledger is taken (the final review's L2).
    uintptr_t root{},banks[5]{};bool read=Read(image+0x4961B50,&root,8) && root;
    for(uint32_t g=0;g<5 && read;++g)read=Read(root+remo_bank_offsets[g],&banks[g],8) && banks[g];
    if(!read){static LONG64 failures{};if(InterlockedIncrement64(&failures)<=8){char row[160]{};
            sprintf_s(row,"{\"event\":\"native_remo_engine_nodes_kept\",\"area\":%u,\"layer\":%u,\"reason\":\"banks_unreadable\"}\n",area,layer);Log(row);}
        return;}
    std::vector<std::array<uint32_t,4>> nodes;nodes.reserve(64);
    {const std::lock_guard hold(gparam_lock);
        for(auto it=remo_engine_nodes.begin();it!=remo_engine_nodes.end();)
            if((*it)[1]==area && (*it)[3]==layer && (*it)[0]<5){nodes.push_back(*it);it=remo_engine_nodes.erase(it);}else ++it;}
    if(nodes.empty())return;
    static LONG64 rows{};const bool logged=InterlockedIncrement64(&rows)<=64;   // the lookups below only for a row (the step review's L3)
    // The engines' own bodies behind the effects' detours (I-0140 tranche 8, I-0141 tranche 6), which pass a node over: the probe sees
    // the bank's nodes (the step review's L4 of tranche 4's five banks).
    const auto light_lookup=engine_light_lookup_original?engine_light_lookup_original:reinterpret_cast<GparamLookup>(image+0x32770);
    const auto fog_lookup=engine_fog_lookup_original?engine_fog_lookup_original:reinterpret_cast<GparamLookup>(image+0x38E70);
    const uint32_t probe[2]{area,0xFFFFFFFFu};
    const uintptr_t light_before=logged?light_lookup(banks[0],probe):0,fog_before=logged?fog_lookup(banks[1],probe):0;
    size_t calls[5]{};
    for(const auto& e:nodes){const auto remove=e[0]==0?light_override_remove_original:e[0]==1?fog_override_remove_original:remo_bank_remove_original[e[0]];
        if(remove)remove(banks[e[0]],e[2],e[2],e[1],e[3],0);++calls[e[0]];}
    const uintptr_t light_after=logged?light_lookup(banks[0],probe):0,fog_after=logged?fog_lookup(banks[1],probe):0;
    // 32770 and 38E70 answer a key they lack with {1, 0}'s node or, without one, their built-in record (light bank +0x700, fog bank
    // +0x180: the Reference's rows): `*_after_builtin` tells a node taken from one never there only with the second. Both probes
    // take the engines' own bodies (above), not the effects' detours.
    if(logged){char row[480]{};
        sprintf_s(row,"{\"event\":\"native_remo_engine_nodes_removed\",\"area\":%u,\"layer\":%u,\"nodes\":%zu,\"remover_calls\":[%zu,%zu,%zu,%zu,%zu],\"thread\":%lu,"
            "\"minus_one\":{\"light_before_builtin\":%s,\"light_after_builtin\":%s,\"fog_before_builtin\":%s,\"fog_after_builtin\":%s,\"light\":[\"%llX\",\"%llX\"],\"fog\":[\"%llX\",\"%llX\"]}}\n",
            area,layer,nodes.size(),calls[0],calls[1],calls[2],calls[3],calls[4],GetCurrentThreadId(),
            light_before==banks[0]+0x700?"true":"false",light_after==banks[0]+0x700?"true":"false",fog_before==banks[1]+0x180?"true":"false",fog_after==banks[1]+0x180?"true":"false",
            static_cast<unsigned long long>(light_before),static_cast<unsigned long long>(light_after),static_cast<unsigned long long>(fog_before),static_cast<unsigned long long>(fog_after));Log(row);}
}
// The act's update (FUN_140CC8180), natively: its observer's row under light-observe.flag (ObserveRemoDrawParamAct's), then the
// genuine update, then the overrides it changed given to the banks once.
void __fastcall NativeRemoDrawParamAct(uintptr_t act) noexcept {
    static const bool observe=ModuleFlag(L"light-observe.flag");if(observe)LogRemoDrawParamAct(act,reinterpret_cast<uintptr_t>(_ReturnAddress()));
    remo_draw_param_act_original(act);
    const std::lock_guard hold(gparam_lock);if(remo_overrides_dirty)PublishRemoOverrides();
}
// A backstop: an area's own file (kind 7) retired ends that area's overrides (RetireGparam, before its lock).
void EndRemoOverridesOfArea(uint32_t area) noexcept {
    EraseRemoOverrides("area_file_retired",area,0,[&](const auto& k,const ds3light::Banks::RecordOverride&){return std::get<1>(k)==area;});}
// ds3light::UseRecordBlends: the engine's own blends on Banks' records (31A20: 140148F50 on the four lights of its output, then
// 140147ED0; 380A0: 14014E5D0, its operands aligned).
void NativeLightRecordBlend(uint8_t* out,const uint8_t* a,const uint8_t* b,float t){
    const auto light=reinterpret_cast<void*(__fastcall*)(void*)>(image+0x148F50);
    std::memset(out,0,0x2A0);for(uint32_t k=0;k<4;++k)light(out+0x70*k);
    reinterpret_cast<void(__fastcall*)(void*,const void*,const void*,float)>(image+0x147ED0)(out,a,b,t);}
void NativeFogRecordBlend(uint8_t* out,const uint8_t* a,const uint8_t* b,float t){
    reinterpret_cast<void(__fastcall*)(void*,const void*,const void*,float)>(image+0x14E5D0)(out,a,b,t);}
void __fastcall RetireNativeZoneCpu(uintptr_t receiver) noexcept {
    try {
        // This owns the complete source-qualified 8C68F0 CPU operation. It is
        // also called directly by other real zone instances, not only through
        // 8C6B70's virtual50. Preserve the operation for every genuine entry.
        const auto pointer=[](uintptr_t at){uintptr_t value{};if(!Read(at,&value,8))Refuse("native_zone_cpu_pointer");return value;};
        const auto method=[&](uintptr_t object,uint32_t slot){const auto table=pointer(object);const auto call=pointer(table+slot);
            if(!call)Refuse("native_zone_cpu_method");return call;};
        if(reinterpret_cast<BYTE(__fastcall*)(uintptr_t)>(image+0x88CFF0)(receiver)){
            const auto world=pointer(image+0x4796298);if(!world)Refuse("native_zone_cpu_world_owner");
            reinterpret_cast<void(__fastcall*)(uintptr_t,uint32_t)>(image+0xE00670)(pointer(world+0x10),0);
            // Original104 loaded code disproves Source655's empty export.
            // Preserve its real receiver ABI and reset on EVERY active-zone
            // entry, including repeated entries. The entire leaf is pinned;
            // it touches no allocator, registry or GPU state.
            std::array<BYTE,sizeof(loaded_sfx_scene_reset_code)> body{};
            if(!Read(image+0xE6E620,body.data(),body.size()) ||
               memcmp(body.data(),loaded_sfx_scene_reset_code,body.size()))
                Refuse("native_sfx_scene_reset_cpu_code");
            const auto manager=pointer(image+0x4799C98);
            if(!manager || manager!=native_sfx_manager)Refuse("native_sfx_scene_reset_manager");
            const std::lock_guard hold(sfx_asset_lock);
            reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0xE6E620)(manager);
            const auto generation=++sfx_scene_cpu_reset_generation;
            char row[352]{};sprintf_s(row,"{\"event\":\"native_sfx_scene_cpu_reset\",\"generation\":%llu,\"first\":%llu,\"second\":%llu,\"actual_loaded_cpu_leaf\":true,\"native_sfx_manager\":true,\"effect_system\":%s,\"sfx_draw_ready\":false}\n",
                generation,pointer(manager+0xC0),pointer(manager+0xC8),native_effect_published.load()?"true":"false");Log(row);
        }
        // Preserve 8C68F0's actual CPU lifecycle calls and caller-owned fields.
        // All object destructors and engine heap releases remain genuine.
        reinterpret_cast<void(__fastcall*)(uintptr_t)>(method(receiver,0x98))(receiver);
        const auto release=[&](uint32_t offset,int32_t virtual_slot){
            const auto object=pointer(receiver+offset);if(!object)return;
            const auto heap=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0xEAA9D0)(object);
            if(!heap)Refuse("native_zone_cpu_heap");
            if(virtual_slot>=0)reinterpret_cast<void(__fastcall*)(uintptr_t,uint32_t)>(method(object,static_cast<uint32_t>(virtual_slot)))(object,0);
            reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t)>(method(heap,0x68))(heap,object);
            *reinterpret_cast<uintptr_t*>(receiver+offset)=0;
        };
        release(0x48,8);
        if(const auto object=pointer(receiver+0x2028))*reinterpret_cast<uintptr_t*>(receiver+0x2028)=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0x82C160)(object);
        release(0x2088,0);release(0x2080,0);release(0x20C0,0);release(0x20B8,-1);release(0x20B0,8);
        *reinterpret_cast<BYTE*>(receiver+0x2099)=0;
        reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0x88E300)(receiver);
        if(const auto object=pointer(receiver+0x1FA8))
            reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t)>(method(object,0x20))(object,receiver);
        const auto root=pointer(receiver+0x1F90);if(!root)Refuse("native_zone_cpu_final_root");
        if(const auto child=pointer(root+0xE0)){
            if(const auto object=pointer(child+0x10)){
                const auto heap=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0xEAA9D0)(object);
                reinterpret_cast<void(__fastcall*)(uintptr_t,uint32_t)>(method(object,0))(object,0);
                reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t)>(method(heap,0x68))(heap,object);
                *reinterpret_cast<uintptr_t*>(child+0x10)=0;
            }
        }
        char row[272]{};sprintf_s(row,"{\"event\":\"native_zone_cpu_retired\",\"receiver\":%llu,\"actual_engine_cpu_lifetimes\":true,\"effect_system\":%s,\"native_zone_draw_ready\":false}\n",static_cast<unsigned long long>(receiver),native_effect_published.load()?"true":"false");Log(row);
    }catch(...){Refuse("native_zone_cpu_retirement_failed");}
}
// W4 FW-4: the door. Once the effect system is published, 7CB820 runs as is (its own E6DCD0 creation); before, the deferral
// below answers with the genuine empty result. Requests deferred before the publish are not replayed (FFX-ATTACHED risk 3).
using SfxWorldRequest=uintptr_t(__fastcall*)(uintptr_t,uint32_t,uintptr_t,uint32_t);
SfxWorldRequest sfx_world_request_original{};
// FW-4: the door is open whenever the system is published (FW-9; Native1486 faulted on the then absent shader set,
// Native1487-1489 clean with it). Without the system (no-native-effects.flag) the deferral below answers as before.
uintptr_t __fastcall DeferNativeSfxSceneRequest(uintptr_t output,uint32_t identifier,
    uintptr_t transform,uint32_t flags) noexcept {
    ffx_observe::Note(ffx_observe::WorldRequest,reinterpret_cast<uintptr_t>(_ReturnAddress()),output,identifier,transform,flags);   // W4 FW-1
    if(native_effect_published.load(std::memory_order_acquire))return sfx_world_request_original(output,identifier,transform,flags);
    try {
        uintptr_t legacy{};
        if(!Read(image+0x4799C98,&legacy,8) || !legacy || legacy!=native_sfx_manager || !sfx_cpu_types_initialized ||
           !output || (output&15) || !transform)
            Refuse("native_sfx_deferred_request_scope");
        auto request=std::make_shared<NativeSfxDeferredRequest>();
        request->identifier=identifier;request->flags=flags;
        if(!Read(transform,request->transform.data(),request->transform.size()))
            Refuse("native_sfx_deferred_request_input");
        {const std::lock_guard hold(sfx_asset_lock);
            request->generation=++sfx_deferred_request_generation;
            if(const auto found=sfx_assets.find(identifier);found!=sfx_assets.end())request->asset=found->second;}
        // E6DCD0's result without effect system (+0x58 null), using its actual CPU constructors.
        // 7CB630 copies that result into the caller's fresh storage. It never
        // receives an active instance or fake original renderer handle.
        alignas(16) std::array<BYTE,0x460> empty{};
        const auto source=reinterpret_cast<uintptr_t>(empty.data());
        reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0x1A879A0)(source);
        *reinterpret_cast<uint64_t*>(source+0x20)=0;
        reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0x249950)(source+0x30);
        const auto result=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uintptr_t)>(image+0x7CB630)(output,source);
        reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0x1A879F0)(source);
        uintptr_t instance{};
        if(result!=output || !Read(output+0x18,&instance,8) || instance)
            Refuse("native_sfx_deferred_result_contract");
        ds3hostio::MemoryOnly(native_host_root);
        ds3hostio::PutLazy<NativeSfxDeferredRequest>(native_host_root/L"sfx/latest-deferred-request",request,[] {return std::vector<std::string>{};});
        if(request->generation<=16){char row[400]{};sprintf_s(row,
            "{\"event\":\"native_sfx_request_deferred\",\"generation\":%llu,\"identifier\":%u,\"flags\":%u,\"asset_generation\":%llu,\"genuine_empty_cpu_result\":true,\"retained_current_input\":true,\"effect_created\":false,\"sfx_draw_ready\":false}\n",
            request->generation,identifier,flags,request->asset?request->asset->generation:0);Log(row);}
        return result;
    }catch(...){Refuse("native_sfx_deferred_request_failed");}
}
}  // namespace
