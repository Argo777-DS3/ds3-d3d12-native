// Model poses (initialise, update, retire) and the selected scene's types: meshes, models, the scene, engine byte checks,
// simulation time, instances and built-in parameters. Part of game_bootstrap.cpp's single translation unit (the order is
// there): it relies on every earlier part (04c_scene_witnesses.inc before it).
#pragma once
#include "04c_scene_witnesses.inc"
namespace {
void __fastcall InitializeNativeModelPose(uintptr_t receiver,uintptr_t model,BYTE previous) noexcept {
    census::Note(census::PoseInit);   // R13 cR0
    try {
        NativeModelPosePrefix prefix{};int32_t count{};BYTE gated{},single{};uint32_t flags{};
        if(!Read(receiver,&prefix,sizeof(prefix)) || !model ||
           !Read(model+0x1C,&count,4) || count<0 || count>4096 ||
           !Read(model+0x1E0,&gated,1) || !Read(model+0x224,&flags,4) ||
           !Read(image+0x4961ABB,&single,1))Refuse("native_model_pose_initialization_input");
        for(const auto value:prefix.gpu)if(value)Refuse("native_model_pose_original_gpu_present");
        for(const auto value:prefix.matrices)if(value)Refuse("native_model_pose_existing_cpu_storage");
        NativeModelPoseOwner owner;owner.model=model;owner.count=count;owner.ring=previous?2:1;
        owner.init_caller=reinterpret_cast<uintptr_t>(_ReturnAddress())-image;
        owner.auxiliary_present=(flags&0x4000)!=0;
        owner.palettes=std::make_shared<ds3content::ModelPoseOwner>(static_cast<uint32_t>(count));
        prefix.index=0;prefix.ring=owner.ring;prefix.count=count;prefix.model=model;
        prefix.single=single && count==1 && owner.ring==1;
        prefix.fast=prefix.single && gated && !(flags&0x4000);prefix.cloth=0;
        for(int32_t i=0;i<owner.ring;++i){owner.matrices[i].resize(count);
            prefix.matrices[i]=count?reinterpret_cast<uintptr_t>(owner.matrices[i].data()):0;}
        {const std::lock_guard hold(model_pose_lock);
            if(!model_poses.emplace(receiver,std::move(owner)).second)Refuse("native_model_pose_duplicate_initialization");
            std::memcpy(reinterpret_cast<void*>(receiver),&prefix,sizeof(prefix));}
        static LONG64 samples{};
        if(InterlockedIncrement64(&samples)<=8){char row[288]{};sprintf_s(row,
            "{\"event\":\"native_model_pose_initialized\",\"count\":%d,\"ring\":%d,\"gated\":%s,\"cpu_ready\":false,\"legacy_gpu_buffers\":false}\n",
            count,prefix.ring,gated?"true":"false");Log(row);}
    }catch(...){Refuse("native_model_pose_initialization_failed");}
}
// 16050's version of a pose from its current rows `input` (the model's +0x1C of them): M = A, or A*G with the model's cloth gate rows
// (+0x1D0, kept in `gates`) when its +0xB0 is set, and its palettes published through `palettes`. Null, with `skip` named, when the
// gate rows are unreadable (I-0101: the caller keeps what it had) or the palettes were not published (an allocation failure: the
// engine's pose update refuses it, a model particle is skipped). The engine's poses (UpdateNativeModelPose) and the model
// particles' (E03, I-0105).
constexpr const char* native_pose_palette_unpublished="native_model_pose_palette_publication_failed";
std::shared_ptr<NativeModelPoseSnapshot> ComposeNativeModelPose(uintptr_t model,const std::vector<PoseRow>& input,bool compose,bool gated,
    bool auxiliary_present,bool fast,uintptr_t reference,ds3content::ModelPoseOwner& palettes,std::vector<PoseRow>& gates,const char*& skip) {
    auto snapshot=std::make_shared<NativeModelPoseSnapshot>();snapshot->generation=++model_pose_generation;
    const size_t count=input.size(),bytes=count*sizeof(PoseRow);
    gates.clear();
    if(compose && bytes){gates.resize(count);snapshot->matrices.resize(count);uintptr_t gate{};
        if(!Read(model+0x1D0,&gate,8) || !Read(gate,gates.data(),bytes)){skip="pose_gate_rows_unreadable";return nullptr;}   // I-0101
        ds3prod::ProduceClothCancel(reinterpret_cast<const float(*)[12]>(input.data()),
            reinterpret_cast<const float(*)[12]>(gates.data()),static_cast<uint32_t>(count),
            reinterpret_cast<float(*)[12]>(snapshot->matrices.data()));
    }else snapshot->matrices=input;
    ds3content::ModelPoseInputs palette_inputs;
    palette_inputs.row_count=static_cast<uint32_t>(count);palette_inputs.gated=gated;
    palette_inputs.compose=compose;palette_inputs.auxiliary_present=auxiliary_present;
    {const std::lock_guard policy_hold(model_policy_lock);const auto policy=model_policy_flags.find(model);
        if(policy!=model_policy_flags.end() && (policy->second&0x4000))palette_inputs.auxiliary_present=true;}
    palette_inputs.fast=fast;palette_inputs.current=input;palette_inputs.gate_rows=gates;
    palette_inputs.gate_available=!compose || !bytes || gates.size()==input.size();
    palette_inputs.reference=reference?ds3content::ModelPoseReference::Unknown:ds3content::ModelPoseReference::Null;
    std::vector<PoseRow> references;
    if(reference && gated && compose && auxiliary_present && !fast){
        references.resize(count);
        if(!bytes || Read(reference,references.data(),bytes)){
            palette_inputs.reference=ds3content::ModelPoseReference::Present;
            palette_inputs.reference_rows=references;
        }
    }
    snapshot->palettes=palettes.Update(palette_inputs);
    if(!snapshot->palettes){skip=native_pose_palette_unpublished;return nullptr;}
    return snapshot;
}
void UpdateNativeModelPose(uintptr_t receiver,uintptr_t source,uintptr_t reference,bool advance,bool fast=false) {
    const std::lock_guard hold(model_pose_lock);
    const auto found=model_poses.find(receiver);
    if(found==model_poses.end())Refuse("native_model_pose_update_foreign_owner");
    auto& owner=found->second;NativeModelPosePrefix prefix{};BYTE compose{},gated{};
    if(!Read(receiver,&prefix,sizeof(prefix)) || prefix.model!=owner.model ||
       prefix.ring!=owner.ring || prefix.count!=owner.count || prefix.index<0 || prefix.index>=owner.ring ||
       !Read(owner.model+0xB0,&compose,1) || !Read(owner.model+0x1E0,&gated,1))Refuse("native_model_pose_update_contract");
    if(advance)prefix.index=(prefix.index+1)%owner.ring;
    const size_t bytes=size_t(owner.count)*sizeof(PoseRow);
    std::vector<PoseRow> input(owner.count);
    if(bytes && !Read(source,input.data(),bytes)){NoteNativeSkip("pose_current_rows_unreadable",owner.model);return;}   // I-0101: the previous version kept
    const char* skip{};std::vector<PoseRow> gates;
    const auto snapshot=ComposeNativeModelPose(owner.model,input,compose!=0,gated!=0,owner.auxiliary_present,fast,reference,*owner.palettes,gates,skip);
    if(!snapshot){if(skip==native_pose_palette_unpublished)Refuse(skip);NoteNativeSkip(skip,owner.model);return;}
    prefix.cloth=snapshot->palettes->cloth;
    // I-0101 (STUDY §1, Iudex Gundyr's death on 2026-10-27): the original's 16000 has no finiteness test; it copies such rows
    // and uploads them, and the rasterizer drops the triangles they place. Natively too since I-0113 tranche 10d (gap 19: with
    // the dependent palettes skipped, Gundyr's body vanished at once instead of fading out); the admission still skips a model
    // whose object rows (row 0) are not finite; the CPU ring keeps the rows as 16000 does. One row the first time a model has them,
    // then one every 600 such updates: the model, the first row, its 12 values, and whether the source (A) or gate (G) rows hold it.
    {size_t bad=snapshot->matrices.size();
        for(size_t r=0;r<snapshot->matrices.size() && bad==snapshot->matrices.size();++r)
            for(const auto value:snapshot->matrices[r])if(!std::isfinite(value)){bad=r;break;}
        if(bad<snapshot->matrices.size() && owner.nonfinite++%600==0){
            const auto finite=[](const PoseRow& row){for(const auto value:row)if(!std::isfinite(value))return false;return true;};
            std::string values;for(const auto value:snapshot->matrices[bad]){char cell[24]{};
                if(std::isfinite(value))sprintf_s(cell,"%s%.6g",values.empty()?"":",",value);else sprintf_s(cell,"%s\"%s\"",values.empty()?"":",",std::isnan(value)?"nan":value>0?"inf":"-inf");values+=cell;}
            std::string name;for(const auto ch:NativeModelDataName(owner.model))name.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');
            char row[640]{};sprintf_s(row,"{\"event\":\"native_model_pose_nonfinite\",\"model\":\"%.60s\",\"row\":%zu,\"count\":%d,\"times\":%llu,\"source_finite\":%s,\"gate_finite\":%s,\"values\":[%s]}\n",
                name.c_str(),bad,owner.count,static_cast<unsigned long long>(owner.nonfinite),finite(input[bad])?"true":"false",
                gates.size()>bad?(finite(gates[bad])?"true":"false"):"null",values.c_str());Log(row);}}
    // 1186: 16050 advances the ring before writing its new slot, so slot index-1 keeps the replaced version (ring 2).
    // An in-place update (16A60 fast path, no advance) rewrites the current slot and leaves the previous one.
    if(advance && owner.ring==2 && owner.current)owner.previous=owner.current;
    owner.matrices[prefix.index]=snapshot->matrices;
    prefix.matrices[prefix.index]=bytes?reinterpret_cast<uintptr_t>(owner.matrices[prefix.index].data()):0;
    std::memcpy(reinterpret_cast<void*>(receiver),&prefix,sizeof(prefix));owner.current=snapshot;++owner.updates;
    if(snapshot->generation<=8){char row[320]{};sprintf_s(row,
        "{\"event\":\"native_model_pose_updated\",\"generation\":%llu,\"count\":%d,\"index\":%d,\"composed\":%s,\"current_cpu_rows\":true,\"legacy_gpu_buffers\":false,\"auxiliary_available\":%s,\"cancel_available\":%s}\n",
        snapshot->generation,owner.count,prefix.index,compose?"true":"false",
        snapshot->palettes->auxiliary.Available()?"true":"false",snapshot->palettes->cancel.Available()?"true":"false");Log(row);}
}
void __fastcall UpdateNativeModelPoseFull(uintptr_t receiver,uintptr_t source,uintptr_t reference,uint32_t worker) noexcept {
    census::Note(census::PoseFull);   // R13 cR0
    try{(void)worker;UpdateNativeModelPose(receiver,source,reference,true);}
    catch(...){Refuse("native_model_pose_update_failed");}
}
void __fastcall UpdateNativeModelPoseSingle(uintptr_t receiver,uintptr_t source,uint32_t worker) noexcept {
    census::Note(census::PoseSingle);   // R13 cR0
    try{(void)worker;BYTE fast{};if(!Read(receiver+0x74,&fast,1))Refuse("native_model_pose_fast_policy");
        UpdateNativeModelPose(receiver,source,0,!fast,fast!=0);}
    catch(...){Refuse("native_model_pose_single_update_failed");}
}
// 1066: 16000 is the public pose update (D07710 alternate path, D2D040 for
// SprjAsmModelDrawEntity parts via D01610): +74 fast -> in-place 16A60; else
// if [144961B40] and 14473D0DA -> DF570 queue for the old upload drain (DF150
// -> 16050); else 16050. The native owner needs no deferral: apply directly.
void __fastcall UpdateNativeModelPosePublic(uintptr_t receiver,uintptr_t source,uintptr_t reference,uint32_t worker) noexcept {
    census::Note(census::PosePublic);   // R13 cR0
    PopinPoseRate(receiver,source);   // I-0100 tranche 3 (popin-observe and pose-observe)
    try{(void)worker;BYTE fast{};if(!Read(receiver+0x74,&fast,1))Refuse("native_model_pose_public_policy");
        static LONG64 calls{};const auto n=InterlockedIncrement64(&calls);
        if(n<=4){char row[160]{};sprintf_s(row,"{\"event\":\"native_model_pose_public_update\",\"call\":%lld,\"fast\":%s,\"reference\":%s}\n",
            n,fast?"true":"false",reference?"true":"false");Log(row);}
        static const bool pose_witness=ModuleFlag(L"pose-observe.flag");   // review: the per-receiver map only when observed
        if(pose_witness){   // 1105 witness: public pose updates per receiver (first 2, then every 100th).
            static std::mutex count_lock;static std::map<uintptr_t,uint64_t> per_receiver;uint64_t k{};
            {const std::lock_guard hold(count_lock);k=++per_receiver[receiver];}
            if(k<=2 || k%100==0){char row[200]{};sprintf_s(row,"{\"event\":\"native_pose_public_receiver\",\"receiver\":%llu,\"count\":%llu,\"source\":%s}\n",
                static_cast<unsigned long long>(receiver),static_cast<unsigned long long>(k),source?"true":"false");Log(row);}}
        // A receiver never given native storage (15420 empty prefix) has nothing to update.
        {const std::lock_guard hold(model_pose_lock);if(!model_poses.contains(receiver)){static LONG64 foreign{};
            if(InterlockedIncrement64(&foreign)<=4)Log("{\"event\":\"native_model_pose_public_unowned\"}\n");return;}}
        if(fast)UpdateNativeModelPose(receiver,source,0,false,true);
        else UpdateNativeModelPose(receiver,source,reference,true);}
    catch(...){Refuse("native_model_pose_public_update_failed");}
}
void __fastcall RetireNativeModelPose(uintptr_t receiver) noexcept {
    census::NoteRetirement(census::PoseRetire,_ReturnAddress());   // R13 cR0
    try{
        const std::lock_guard hold(model_pose_lock);NativeModelPosePrefix prefix{};
        if(!Read(receiver,&prefix,sizeof(prefix)))Refuse("native_model_pose_retire_unreadable");
        for(const auto value:prefix.gpu)if(value)Refuse("native_model_pose_retire_original_gpu_present");
        const auto found=model_poses.find(receiver);
        if(found==model_poses.end()){
            for(const auto value:prefix.matrices)if(value)Refuse("native_model_pose_retire_foreign_cpu_storage");
            return; // genuine15420 empty prefix, no allocation to withdraw
        }
        prefix.matrices[0]=prefix.matrices[1]=0;prefix.model=0;prefix.count=prefix.index=prefix.ring=0;
        std::memcpy(reinterpret_cast<void*>(receiver),&prefix,sizeof(prefix));found->second.palettes->Withdraw();model_poses.erase(found);
    }catch(...){Refuse("native_model_pose_retirement_failed");}
}
// Direct current-view CPU selection. The spatial algorithms and frustum are
// genuine engine components; no loaded-model scan or old draw-context exists.
struct NativeSelectedMesh {
    ingress::CurrentModelMesh current;
    bool texture_gave_up{};   // I-0120 C1: a binding's registry lookup given up (a mutator ran): the model is not recorded, admitted afresh next
    bool no_cast{};   // D-0180 C2 (SUN-SHADOWS-STATIC-PREP 2): material +0xD1, which refuses the depth rows (140680 with state bit 0x40000)
    // Lightmap floors: cbTexHdrScaleParam from the draw's own lightmap (5B110: its texture's authored constant), when one is bound.
    std::optional<std::array<float,4>> lightmap_constant;
    MeshSlice slice;
    std::shared_ptr<const NativeModelMaterial> material;
    std::shared_ptr<const NativeModelLayout> layout;
    ModelGeometryBinding geometry;
    std::shared_ptr<const NativeModelLayout::Programs> programs;
    std::vector<ds3content::ModelParameterWrite> numeric_writes;
    // Current draw-owned b2 words. Immutable authored/effect and FLVER leases
    // remain in programs/material; no legacy dynamic-parameter buffer is read.
    std::vector<uint8_t> dynamic_bytes;
    std::vector<uint8_t> dynamic_written;
    bool parameter_animations_available{};
    size_t parameter_animation_rows{};
    std::optional<bool> wet_activation;
    std::optional<uint32_t> material_class;
    std::optional<bool> current_special;
    std::optional<uint32_t> base_deformation;
    struct MaterialMasks {uint64_t first{},second{};uint32_t secondary{};};
    std::optional<MaterialMasks> material_masks;
    struct PosePolicy {bool cloth_enabled{};uint32_t flags{};};
    bool cloth_stream{};   // 1184: stream 0 is the current simulated cloth copy (MENUS-MAP 65): VC_isEnableCloth = 1
    // I-0134: with cloth_stream, the copy the current one replaced (NativeClothStream::previous_resource; the geometry's stream
    // owner, that stream, keeps its buffer), the Velo rows' third stream; null before a second upload.
    Microsoft::WRL::ComPtr<ID3D12Resource> cloth_previous;
    std::optional<PosePolicy> pose_policy;
    std::optional<int32_t> material_index;
    std::optional<ds3content::ModelWetMaterialParameters> wet_parameters;
    struct TextureBinding {
        ds3content::ModelTextureBinding input;
        std::shared_ptr<const bootstrap::NativeTextureRecord> texture;
        // I-0071 (ER B, D-0195): a material binding keeps the engine's stable holder of each lookup candidate (the path, its
        // "_l" package, a DOL lightmap's atlas; SprjGxTexture, created at the model's request and seeded with SYSTEX_DUMMY, its
        // inner swapped in place when the texture arrives) and the inner each one held when bound; a kept mesh rebinds when
        // an inner changed, without being admitted again (ER binds through CSGxTexture the same way).
        std::array<uintptr_t,4> holders{},inners{};   // a lightmap set object first (set_first), then the lookup candidates (review M2)
        bool set_first{},looked_up{};   // looked_up: holders from NativeTextureHolders (looked up again while the first is missing, review M3)
        bool dol_constant{};   // the DOL t26 lightmap: its texture's authored constant feeds cbTexHdrScaleParam when the set object is bound
    };
    // Fresh current native generations. A missing lease is unavailable;
    // selected submission will retain the same leases through completion.
    std::vector<TextureBinding> texture_bindings;
    struct SceneTextureBinding {
        uint32_t slot{};
        std::shared_ptr<const bootstrap::NativeTextureRecord> texture;
    };
    // Current selected register associations and native leases remain paired.
    // Submission uses these same versions instead of consulting global slots.
    std::vector<SceneTextureBinding> scene_texture_bindings;
    struct ShaderTextures {
        ds3content::ModelTextureStatus status{ds3content::ModelTextureStatus::Missing};
        std::vector<ds3content::ModelShaderTexture> registers;
    };
    std::array<std::array<ShaderTextures,2>,native_recipe_count> shader_textures;
    // Workstream K P2/P3: immutable blocks, shared between the meshes whose inputs are equal (scene and view blocks per
    // programme for the scene, instance blocks per programme and fade for the model, numeric blocks per programme and
    // exact numeric writes); a null pointer is the default block (Missing, not required), as an uncomputed value was.
    using NumericBlock=std::shared_ptr<const ds3content::ShaderRegisterBlock>;
    // Per available recipe, VS/PS register3 only. Complete draw constants,
    // current recipe eligibility, textures and commands remain separate.
    std::array<std::array<NumericBlock,2>,native_recipe_count> numeric_blocks;
    // C7-1 (PERSISTENT-RECORDS-DESIGN 1): the scene, view and instance blocks are not held by the mesh; prepare looks them up
    // in the scene's tables and the model's instance table by programme key.
};
// C7-2 (PERSISTENT-RECORDS-DESIGN 1): a model's admitted meshes, immutable once built and shared by the scenes that select
// the model while its record holds (a reused model costs one reference, not a rebuilt and destroyed mesh vector).
struct NativeMeshSet {
    std::shared_ptr<const std::vector<NativeSelectedMesh>> items;
    size_t size() const noexcept {return items?items->size():0;}
    bool empty() const noexcept {return !size();}
    const NativeSelectedMesh& operator[](size_t index) const {return (*items)[index];}
    const NativeSelectedMesh& at(size_t index) const {if(index>=size())throw std::out_of_range("native mesh set");return (*items)[index];}
    const NativeSelectedMesh* begin() const noexcept {return items?items->data():nullptr;}
    const NativeSelectedMesh* end() const noexcept {return items?items->data()+items->size():nullptr;}
};
using SharedRegisterBlock=std::shared_ptr<const ds3content::ShaderRegisterBlock>;
// A model's instance blocks by {programme key, fade}. I-0078 (i74-native-71: the pass rows' two lookups per row in a tree were a
// fifth of their time on the workers): a sorted vector, the entries of one model a few tens.
struct NativeInstanceBlockTable {
    using Key=std::pair<uintptr_t,uint32_t>;using Item=std::pair<Key,SharedRegisterBlock>;
    std::vector<Item> items;
    using const_iterator=std::vector<Item>::const_iterator;
    const_iterator end() const noexcept {return items.end();}
    const_iterator find(const Key& key) const noexcept {
        const auto at=std::lower_bound(items.begin(),items.end(),key,[](const Item& item,const Key& k){return item.first<k;});
        return at!=items.end() && at->first==key?at:items.end();}
    SharedRegisterBlock& operator[](const Key& key) {
        auto at=std::lower_bound(items.begin(),items.end(),key,[](const Item& item,const Key& k){return item.first<k;});
        if(at==items.end() || at->first!=key)at=items.insert(at,Item{key,{}});
        return at->second;}
};
struct NativeEffectModel;   // E03 (I-0105), below
// I-0113 tranche 14 (a world object's region IBL): what 5C0E0 gives an object's rows through 4E2A0 (its copy of the group): the two
// regions' keys (+0xB0/+0xB8), the blend +0x13C and the scales +0x140/+0x144 (cbInstanceData rows 13 to 15's .w), and the cubes
// 75B30 binds at t30/t31 for its forward rows from those keys (null: the neutral cube, as the absent REM's +0x170 black).
struct NativeRegionIbl {uint32_t keys[4]{};float blend{},scales[2]{};std::shared_ptr<const bootstrap::NativeTextureRecord> bound[2];};
struct NativeRegionSnapshot;   // 07_scene_lighting
struct NativeSelectedModel {
    uintptr_t entity{},model{},group{},animation{};
    uintptr_t owner{};   // the drawing entity (the map piece of an item, the character of a part; the caster census)
    uint32_t lod_mask{1}; // Current model+B4 loaded LOD mask (63CB0).
    ingress::ModelFadeState fade;
    ingress::ModelFadeSelection geometry_fades;
    std::optional<ingress::CurrentModelPreparationFlags> preparation_flags;
    std::optional<std::array<uint64_t,2>> shader_capabilities;
    std::optional<uint32_t> material_mask,ordinary_category;
    bool phantom{};   // P4: a ChrAsm part drawn with a PhantomParam row (CFDE70 stack state +0x87)
    uint8_t phantom_kind{};float phantom_fade{1.f};   // its +0x84 kind (after the kind-0 rule) and row fade
    // 1103: model +224 (union of material policy flags; 57A20 adds the engine7 row for bit 15).
    std::optional<uint32_t> material_flags;
    std::optional<std::array<uint8_t,64>> texture_scroll;
    std::optional<ingress::ModelWetDrawInputs> wet_draw_inputs;
    std::optional<ingress::ModelDecalChoice> decal_choice;
    // 1060 characters: the per-character decal provider (blood/damage) has no
    // native owner yet; explicit open policy: its t41/t42 use the neutral common decal.
    bool neutral_instance_decal{};   // also marks character parts (world ASM parts, menu parts since 1169)
    bool character{};   // DECALS-WAVE1 D1: a character model (c####: an ASM part or a disp item), never a decal receiver
    // 1169, I-0140: the per-item colour (a disp item's item+780, in every view) the D3DA20 draw path copies into group +40..+4F before each draw
    // (MENUS-MAP 26: skin colour 0, hair slot 6 colour 1, slots 0x11..0x16 colours 2..7), read as FC_SkinMatchColor.
    std::optional<std::array<uint8_t,16>> draw_colour;
    int32_t asm_part{-1}; // 1068 witness: SprjAsmModelDrawEntity part index
    std::optional<std::array<float,24>> cloth_world;   // 1185: VC_World (current, previous) of a cloth draw: the cloth instance frame I+0x50 / +0x90
    std::array<float,12> placement{};
    NativeMeshSet meshes;   // C7-2: shared with the model's record
    std::shared_ptr<const NativeModelStorage> geometry;
    std::shared_ptr<const NativeModelPoseSnapshot> pose;
    std::shared_ptr<const NativeModelPoseSnapshot> previous_pose;   // 1186: b6 of a Velo draw (ring slot index-1; = pose for ring 1)
    bool fade_prepass{};   // I-0113 tranche 10d: the emit descriptor's byte +0x10 (NativeSelectionCandidate::fade_prepass)
    bool velocity_rows{};   // 1186: the draw velocity byte (descriptor byte 1) of this model's emit: 57A20 adds its 0x34 row only then
    std::shared_ptr<const NativeModelStreamVersion> streams;
    uintptr_t patch{};
    bool instance_streams{};
    struct InstanceConstants {
        std::array<uint8_t,ds3prod::instance_record_bytes> bytes{};
        std::array<uint8_t,ds3prod::instance_record_bytes/4> written{};
    };
    std::array<InstanceConstants,2> instances;
    const NativeEffectModel* effect{};   // E03 (I-0105): a model particle's draw (the scene's effect_models hold it)
    // C7-1: this model's instance blocks per programme key and fade (workstream K P2), immutable once the model is admitted.
    std::shared_ptr<const NativeInstanceBlockTable> instance_blocks;
    // cP6 (PREPARE-REDUCTION, R8b S1 to S3): the engine bytes prepare's model blocks read (the group's first 0x190 bytes for
    // VS cbvModelParam +0x60/+0x4B0 and PS cbModelParam's draw-state copy; the animation flags +620, +630 and block +210),
    // copied at collect on the engine thread, so prepare reads no engine memory and may run on the render worker.
    struct EngineBytes {bool group_read{},block_flag_read{},matrix_flag_read{},block_read{};uint8_t block_flag{},matrix_flag{};
        std::array<uint8_t,0x190> group{};std::array<uint8_t,0x410> animation_block{};};
    std::shared_ptr<const EngineBytes> engine_bytes;
    // cP7 (R8b S4): the forward cbLight/cbFog entries of this model (CollectNativeForwardLights), null without a forward row.
    struct ForwardLight {std::vector<uint8_t> light,fog;};
    std::shared_ptr<const ForwardLight> forward_light;
    std::shared_ptr<const NativeRegionIbl> region_ibl;   // I-0113 tranche 14: null when 5C0E0 would leave the group's own
    // I-0113 tranche 15: what 57A20's order key is made from (the candidate's box, rule and extra float, read at admission); the
    // key itself takes the frame's camera (PrepareNativeModelFrame). Empty: no key (an E03 particle has its command's).
    struct SortBox {std::array<float,8> box{};float extra{};uint8_t rule{};};
    std::optional<SortBox> sort;
};
// C7-2 (PERSISTENT-RECORDS-DESIGN 1): the global inputs a model record's meshes were admitted under, read once per main
// collect before the walk: the registries (models, programmes, the engine's named texture registry, its root and the
// native texture ingress), the scene's common images and scene-colour copy (held, so an address cannot come back as
// another object), the wet shader catalogue (by its own epoch) and two engine constants the dynamic block reads (the
// default FC_DynamicDiffuseColor at 4929940 and the phantom unit colour at 3D7FEF0). I-0070: a record compares the stable fields
// (SameStable) and checks the tracked ones itself (CheckNativeModelRecord).
struct NativeRecordEpochs {
    uint64_t models{},programs{},catalog{};
    uint64_t serial{};   // I-0078: the main collect's scene uid (each material's record bytes compared once for it), not compared
    uint64_t texture_withdrawals{};   // I-0070: TextureIngress::Withdrawals (tracked: each record checks its bound records)
    uintptr_t named_registry{};
    std::shared_ptr<const void> common_images,scene_copy,water_depth,reflection;   // water_depth: the t56 record (WT-2); reflection: t54 (WT-7)
    std::shared_ptr<const void> light_grid;   // I-0107: the light grid's records (t59..t62), NativeLightGridRecords
    std::shared_ptr<const void> snow;         // I-0105 tranche 10: the snow textures t47..t51 (snow records only)
    std::array<uint8_t,32> constants{};
    // I-0137 (I-0120 C2): the Reflection option byte [[GXRS+0x5360]+0x6C], which a water record's FC_WaterFresnel words follow
    // (09_model_admission): a change of the option makes the records stale at the next collect, not 512 collects later.
    uint8_t reflection_option{1};
    // I-0070 (scene data persistence): what every record compares as it is. models, programs and texture_withdrawals move at
    // every arrival while the world streams; a record checks those itself (its registry entries, programmes and bound texture
    // records) only when they moved since its last check, and its bindings through their holders at each reuse (I-0071).
    bool SameStable(const NativeRecordEpochs& o) const noexcept {
        return catalog==o.catalog && named_registry==o.named_registry &&
            common_images==o.common_images && scene_copy==o.scene_copy && water_depth==o.water_depth && light_grid==o.light_grid && constants==o.constants &&
            reflection_option==o.reflection_option;}   // reflection: water records only
};
// I-0078 (i74-native-71: the main and reflection collects copied their carried scene, view and planned tables in and out, about
// 0.25 ms of the engine thread at m45): a table carried from the previous collect without a copy. The carried entries are
// immutable and shared with the scenes that carried them; a scene's own additions are kept apart and merged into a new carried
// table only when there are some (a new programme in view).
template<class Value> struct NativeCarriedTable {
    using Map=std::unordered_map<const void*,Value>;
    std::shared_ptr<const Map> carried;Map added;
    const Value* find(const void* key) const {
        if(const auto found=added.find(key);found!=added.end())return &found->second;
        if(carried)if(const auto found=carried->find(key);found!=carried->end())return &found->second;
        return nullptr;}
    // The entry to fill and true when the key is new; a carried key gives null and false.
    std::pair<Value*,bool> try_emplace(const void* key) {
        if(carried && carried->contains(key))return {nullptr,false};
        const auto [entry,fresh]=added.try_emplace(key);return {&entry->second,fresh};}
    std::shared_ptr<const Map> Carry() const {
        if(added.empty())return carried;
        auto merged=std::make_shared<Map>(carried?*carried:Map{});for(const auto& [key,value]:added)merged->insert_or_assign(key,value);
        return merged;}
};
std::atomic<uint64_t> native_scene_uids{1};   // I-0071 review L1: a process-unique id per scene
// E03 (I-0105): one model particle's FLVER draw of the main view's frame (effect_draw::ModelSubmission), admitted by the main collect
// after its walk as a candidate of its own kind (NativeDeferredCandidate kind 3): the submission (its draw state is the candidate's
// group) and the pose made from its bone rows (ComposeNativeModelPose). I-0113 gap 14: 64B30 makes each mesh's palette from those
// rows (ComposeNativeEffectModelPoses): `pose` the rows as given (a gated model's rigid meshes, and the object rows); `mesh_poses`
// by mesh ordinal, the palette of a mesh that differs from them (a Dynamic mesh's rows composed with the bind rows, a non-gated
// model's gathered rows), null where `pose` serves.
struct NativeEffectModel {effect_draw::ModelSubmission submission;std::shared_ptr<const NativeModelPoseSnapshot> pose;
    std::vector<std::shared_ptr<const NativeModelPoseSnapshot>> mesh_poses;
    const NativeModelPoseSnapshot* MeshPose(uint32_t ordinal) const noexcept {
        return ordinal<mesh_poses.size() && mesh_poses[ordinal]?mesh_poses[ordinal].get():pose.get();}};
struct NativeSelectedScene {
    uint64_t uid{native_scene_uids.fetch_add(1,std::memory_order_relaxed)};
    uint64_t serial{};
    bool dlss{};   // I-0111 tranche 4 step 2 (final review M1): this frame's DLSS, decided at the collect on the engine thread
    bool taa{};    // I-0132 R11j: this frame's TAA, decided there too
    std::optional<NativeRecordEpochs> record_epochs;   // C7-2: main collect, absent when model records are off
    std::vector<std::array<uint32_t,10>> light_keys;   // D-0180 C4: the admitted models' light/fog key blocks, index = envData
    // cP6/cP7: the .data default row of PS cbModelParam (4929B20) and the deferred lighting input, read and built at collect.
    std::optional<std::array<uint8_t,16>> default_param_row;
    std::shared_ptr<const NativeLightingInput> lighting;
    std::shared_ptr<const NativeRegionSnapshot> regions;   // I-0113 tranche 14: the main collect's, before its walk (null: no region IBL)
    // Workstream K P2 (K-COLLECT-IMPLEMENTATION-SHEET 2.2): the scene (b8) and view (b0) blocks of this scene's camera, one
    // per programme bytes object (equal bytes give equal interface, reads and plan). Each entry keeps the programme object
    // owning those bytes, so the key cannot be reused by another programme while the scene lives. Written only by the
    // thread building the scene, before publication.
    struct SharedBlock {std::shared_ptr<const NativeModelLayout::Programs> owner;std::shared_ptr<const ds3content::ShaderRegisterBlock> block;};
    NativeCarriedTable<SharedBlock> scene_block_table,view_block_table;   // I-0078: carried without a copy
    // C7-1b: the programme objects whose scene and view entries are already in the tables (each programme once per scene).
    // C7-1c review: each planned programme is owned here, so its address cannot name another programme within the scene or
    // in a carried scene.
    NativeCarriedTable<std::shared_ptr<const NativeModelLayout::Programs>> planned_programs;
    // Workstream K P4a: the common system textures the admission looks up by name, once per scene (common_images is fixed
    // for the scene): the first SYSTEX_WHITE with a texture (FaceGen and default-bound fallbacks) and the first entries
    // named SYSTEX_WHITE and SYSTEX_ZERO whatever their texture (forward slots 33 and 44).
    struct SystemTextures {bool resolved{};std::shared_ptr<const bootstrap::NativeTextureRecord> white_bound,white_named,zero_named;
        // Optimisation review: the common system textures by lower-case name, the first non-null of each name (the former
        // per-binding scan of the list for every absent material texture, every collect).
        std::unordered_map<std::wstring,std::shared_ptr<const bootstrap::NativeTextureRecord>> by_name;
        // I-0097 tranche 2: the scene-colour copy (slots 45 and 55), read once per scene with them, not under its lock for every
        // admitted mesh (the admission workers queued on native_scene_copy_lock at the area crossing).
        std::shared_ptr<const bootstrap::NativeTextureRecord> scene_copy;};
    SystemTextures system_textures;
    SceneViewInput camera;
    ingress::SceneCameraConstants camera_constants;
    ingress::SceneViewConstants view_constants;
    std::shared_ptr<const NativeSimulationTime> simulation_time;
    //Initial ordinary native phase0 consumes the actual generated bank. Its
    //temporal choice is explicit policy; original Gbuf phase parity is open.
    std::shared_ptr<const NativeCommonImages> common_images;
    std::shared_ptr<const ds3content::EngineSamplerBankVersion> sampler_bank;
    std::shared_ptr<const NativeOptionSnapshot> options;   // O2: the frame's graphics options (sampler_bank is its bank)
    // DECALS-WAVE1 D1: the decal managers' kind-0 decals at the main collect (engine thread, under the decal registry lock): each
    // object's first 0x420 bytes, its material object's first 0x190 bytes (+0x270: 1A8580 copies them to the draw state +0x240;
    // +0xD0..+0xDC scale FC_DiffuseScale and are FC_DifColMul), the cbCubeEnvSH1 rows 59470 blends from the material's light
    // banks (keys +0xF8/+0x100, t +0x108; empty when a bank is absent), the native records of its textures
    // +0x190..+0x200 (an SprjGxTexture forwards to its GXTexture2D at +0x10; null when absent or not resident), its manager's
    // settings (+0x400..+0x42F) and programme tables (+0x98 pixel, +0x210 vertex); the decal option ([[GXRS+0x5360]+0x68]);
    // the renderer's copies the decals sample, with their views (t13 GB0, t14 GB1, t4 depth).
    struct DecalSnapshot {std::array<uint8_t,0x420> object{};std::array<uint8_t,0x190> material{};std::optional<std::array<float,12>> sh;
        std::array<std::shared_ptr<const bootstrap::NativeTextureRecord>,15> textures;std::array<uint8_t,0x30> settings{};
        std::array<uintptr_t,47> pixel{};std::array<uintptr_t,14> vertex{};};
    std::vector<DecalSnapshot> decals;bool decal_option{true};
    std::array<std::shared_ptr<const bootstrap::NativeTextureRecord>,3> decal_sources;   // GB0, GB1, depth copies
    // DECALS-PAINT-READY rung 2: the paint commands this collect drained from the managers' queues (+0x20..+0x28, 522B0's budget
    // +0x408), each with what 5FCD0 / 1AAA70 / 1AB370 / 1AC390 read: the receiver's patch (the models it paints: selected.patch),
    // its target, VS b0 (F), mulCol, g_PaintMaterialParamScale, t0..t4 and whether the decal has a mask (s3); `clear` alone for a
    // handle-0 command or a type-2 clear of every bit (a new target's first listings carry no flag: their job clears the target
    // once, EncodeNativePaint's `cleared`); `erase` for a type-1 command with its erase
    // byte (I-0105 BLOOD: 1AAF90's pass 10 over the receiver's meshes), with PS b10 +176 FC_PaintEraseColor and +192
    // FC_PaintFadeParam in `erase_block`.
    struct PaintEvent {uintptr_t patch{};std::shared_ptr<const NativePaintTarget> target;bool clear{},draw{},masked{},erase{};
        std::array<uint8_t,64> view{};std::array<float,4> mul_col{},param_scale{};std::array<float,8> erase_block{};
        std::array<std::shared_ptr<const bootstrap::NativeTextureRecord>,5> textures;
        std::shared_ptr<const bootstrap::NativeTextureRecord> occlusion;};   // D-0193 rung 2b: t32, the target's projector depth map
    std::vector<PaintEvent> paints;std::shared_ptr<std::atomic<bool>> paints_taken=std::make_shared<std::atomic<bool>>(false);
    std::optional<bool> global_variant_enabled,category_override,split_engine5;
    std::optional<float> category_scalar;
    // 23790 cbPerFrame environment rows (old host ProducePerFrame, I-0063):
    // 134A20([root]+4680, context+4110/+4118)+8, eleven floats each; blend context+3FD0.
    struct PerFrameRows { std::array<float,11> a{},b{};float blend{}; };
    std::optional<PerFrameRows> per_frame_rows;
    // I-0113 tranche 34 (tranche 33's final review M3): the pass context's +0x110..+0x118, (f8 f7, f9 f7, f10 f7) of the blended
    // Dominant Light rows, which 140204C30 (and 140142A50) write once into the world render task's pass context and every consumer
    // reads from it: 5B3D0 (the meshes' instance rows), 59470 (the decals and the model particles with their own light) and 264230
    // (the effects' SH rows) multiply row k's .w by +0x110 + 4k. One value a scene (ProducePassDominantScale, at the collect), the
    // base pass context's (1, 1, 1) without the rows (the menu views' collects: their pass context's value not read, INFERRED 1).
    std::array<float,3> pass_row_scale{1.f,1.f,1.f};
    // I-0105 (the surface snow): 23790's cbPerFrame snow rows +0x60..+0xAF from 131780's blend of the `Snow` records of the view
    // context's pair +3FC0/+3FC8 (blend +3FD0, ds3prod::ProduceSnow), and the scene snow rate it returns, which 1A86C0 multiplies
    // into each draw's cbModelParam FC_SnowRate with the `Snow Model` rates of the draw state's pair (+2B0/+2B8, blend +2C0);
    // `model_builtin` the engine bank's built-in rate for a key no gparam layer holds.
    struct SnowRows { std::array<float,20> rows{};float scene_rate{};float model_builtin{}; };   // rows: cbPerFrame +0x60..+0xAF
    std::optional<SnowRows> snow_rows;
    // I-0105 tranche 10: the textures 103510 binds as t47..t51 for every draw (CollectNativeSnowTextures), one value for as long
    // as the objects and their records stand (a model record binding them compares it, NativeRecordEpochs::snow).
    struct SnowTextures {std::array<std::shared_ptr<const bootstrap::NativeTextureRecord>,5> records;std::array<uintptr_t,5> objects{},inners{};std::array<uint8_t,5> sources{};};
    std::shared_ptr<const SnowTextures> snow_textures;
    // 1049 cbLight FC_DirLight[2] / cbFog FC_FogParamArray[2], by the engine's
    // own builders over the 137910 entries (see CollectNativeLightBlocks).
    // O0 (options sheet M-1): shade_sss = the ShadeQuality gate of the skin SSS stage ([[GXRS+0x5360]+0x75] non-zero, from
    // MEDIUM in the original), taken at collect from the scene's option snapshot (O2) so prepare reads a value (phase F
    // rule); true without a snapshot object.
    struct LightBlocks { std::vector<uint8_t> light,fog;bool shade_sss{true};std::vector<uint8_t> authored; };   // authored: I-0141, a byte a set (a cutscene's own)
    std::optional<LightBlocks> light_blocks;
    // Forward pass (1114): the banks and view the per-model forward cbLight/cbFog are built from.
    // 1169: fixed_key (menu views): the gparam key {area 1, id} of the view row replaces the models' group keys.
    // view: the 0x80-byte view record 1496D0/14E900 read with aligned SSE loads (MULPS [RSI+0x30]): 16-byte aligned (Native1290
    // faulted when a new scene member moved it to an 8-byte boundary).
    struct ForwardLightContext { uintptr_t light_bank{},fog_bank{};alignas(16) std::array<uint8_t,0x80> view{};std::optional<std::array<uint32_t,2>> fixed_key; };
    std::optional<ForwardLightContext> forward_light;
    // 1054 point lights: the items EFF50/212810 build for the current view (lighting map).
    struct PointLight { std::array<float,16> param{};std::array<float,4> sphere{};float near_z{};bool legacy{};uintptr_t light{};std::array<float,32> shadow{};bool shadowed{};
        std::array<float,3> world{}; };   // I-0122: world, the light's position +0x150 (param's is the view's)
    std::vector<PointLight> point_lights;
    // I-0106: the spot items F1010/2136B0 build for the current view (types 0x67, 0x6E legacy): 2144C0's eight
    // cbLightAccLightParam rows, the item's view-depth range +0x20/+0x24 (1A6690's depth bounds and cbLightAccVtxParam) and
    // the hull 214310 hands the tile binning (points [+0xF0], count +0xF8, matrix +0x100).
    // I-0106: a light the local shadows took (local_shadows) carries the shadowed rows after its own (2144C0 / 212DB0): a point
    // light item+0x50, item+0x60 and its six face rectangles in the atlas (192 bytes, DPointLightShadowedN), a spot {+0xE0, 0, 0, 0}
    // and its rectangle (160 bytes, DSpotLightShadowedN); `light` is the engine's light object.
    struct SpotLight { std::array<float,32> param{};float near_z{},far_z{};uint32_t type{};ds3tiles::Hull hull;uintptr_t light{};std::array<float,8> shadow{};bool shadowed{}; };
    std::vector<SpotLight> spot_lights;
    // W5a SW-7, I-0106 (on by default, no-local-shadows.flag opts out): the lights the genuine slot 3 shadowed this frame, in the slot order: the light, its
    // programme variant (+0x1C: 0x66/0x6D point, 0x68..0x71 spot), the sort key +0x10, the metric (+0x9C / +0x260), the slot's
    // map object (+0xB0 / +0x1C0) and the whole frame item after the slot (its rectangles: point six faces at +0xBC reduced by 3
    // texels, spot one at +0x268); the container's item count after the slots and the atlas edge.
    struct LocalShadow { alignas(16) std::array<uint8_t,0x280> item{};   // first: no padding before it
        uintptr_t light{},container_item{};uint32_t variant{},order_key{};float metric{};bool spot{}; };   // (the size a multiple of 16)
    std::vector<LocalShadow> local_shadows;
    std::shared_ptr<const NativeRtSceneInput> rt_scene;   // I-0117 (R2 T1, the rays' scene): the loaded world's instances (CollectNativeRtInstances)
    // I-0125 R6d: the rays' placed instances with a forward set (their index in rt_scene's instances) and their draw groups: their
    // forward blocks (mulCol and fog) made once the frame's light blocks are (CollectNativeRtForwardBlocks), into the walk's input while it is still
    // this collect's own (`rt_scene_input`, the same object, released there; the step review's L6: rt_scene stays const for its readers).
    std::vector<std::pair<uint32_t,uintptr_t>> rt_forward_groups;
    std::shared_ptr<NativeRtSceneInput> rt_scene_input;
    // I-0118 (D-0237): the sun's shadow traced this collect (the renderer's rt_shadow on, the rays not failed, the main view): the
    // cascades' caster walk is skipped and the prepare draws no cascade (no casters, no atlas, no blur taps).
    bool sun_traced{};
    // I-0132 R11e step 3: the arrival held (its world shown black, the UI over it) until the rays' scene is ready, and the collect
    // that released it (its frame restarts the denoisers' and DLSS's history through the velocity's reset).
    bool rt_hold{},rt_release{};
    // I-0122 R4b (Thomas's yes, 2026-11-03): the shadowed local lights traced by the native pass (the rays' walk made instances and
    // the pass is on): their faces and records kept (the lights stay shadowed), but no local caster walked and no local atlas drawn.
    bool local_traced{};
    // I-0125 R6c: this collect chose the traced water (CollectNativeWaterPlane): the water's plane for its frame's pass (the
    // step review's L5: the frame's own choice, not the latest collect's).
    std::optional<std::array<float,4>> rt_water_plane;
    std::shared_ptr<const std::vector<std::wstring>> wet_shader_catalog;   // I-0120 B8: read once (NativeWetShaderCatalog)
    ds3content::NativeModelPixelPolicy pixel_policy{ds3content::NativeModelPixelPolicy::Unknown};
    // 1186 velocity family (I-0068 VELOCITY-IMPLEMENTATION-CHECKLIST steps 1-6): the main world view's velocity inputs, present
    // when the quality option allows the 57A20 0x34 rows ([[GXRS+0x5360]+0x21], MotionBlur ENABLE; Reference1186 reads +0x20
    // and +0x21 both 1, factor [[GXRS+0x5368]+0xE0] = 1). Absent: no velocity target, rows or resolve (MotionBlur DISABLE).
    struct Velocity {
        std::array<uint8_t,192> matrices{};   // Velo VS b0 (current, previous view-projection)
        std::array<float,4> scale{};          // Velo PS b7 FC_DifColMul
        Matrix4 current_to_previous{};        // native motion-vector resolve (camera motion from depth)
        bool reset{};                         // no previous view: previous := current (zero camera motion)
        bool rows{};                          // quality byte +0x21: 57A20 records the object Velo rows (LOW: target and resolve only)
        Matrix4 previous_pose{};              // W10 MB-4: the previous camera pose E69A0 hands the library (the current one on a cut)
        Matrix4 clip_to_previous_clip{};      // I-0111: the unjittered current clip -> previous clip (Streamline's clipToPrevClip)
    };
    std::optional<Velocity> velocity;
    // I-0140 tranche 40 (D-0305): a menu view's TAA this frame (its camera jittered in its own pixels): the view's camera motion from its
    // own previous camera (upscaler::MenuViewMotion; no object rows, so no `velocity` and no Velo draws), `reset` on its first frame or a cut.
    struct MenuMotion {Matrix4 current_to_previous{},clip_to_previous_clip{},still_to_previous{};bool reset{},dlss{};float kernel{};};   // kernel: the TAA's reconstruction (I-0140 tranche 41); dlss: DLAA on the view (the option's DLSS mode), the TAA otherwise; still: the camera as its own previous (a repeated presentation)
    std::optional<MenuMotion> menu_motion;
    std::optional<NativeGlowInput> glow;   // W5d GL-3 (CollectNativeGlowInputs; main view)
    std::shared_ptr<const NativeEffectFrame> effects;   // FFX-DRAW N6 (RecordNativeEffects with the draw side; main view)
    std::vector<std::shared_ptr<const NativeEffectModel>> effect_models;   // E03 (I-0105, RecordNativeEffects; main view): admitted after the walk
    // W10 YB-3 (CollectNativeYebisInputs; main view, on by default, no-yebis.flag opts out): the blended Setting Info, Tone Map and Bloom blocks, whether the
    // three converters ran, and the digest of the library's globals block after them (the native passes read the globals).
    struct Yebis {std::array<uint32_t,21> setting{};std::array<uint32_t,35> tone{};std::array<uint32_t,42> bloom{};std::array<uint32_t,28> shaft{};std::array<uint32_t,34> dof{};std::array<uint32_t,21> motion{};bool complete{},converted{},bloom_converted{},shaft_ready{},dof_ready{},motion_ready{},auto_exposure{};uint64_t globals_digest{};
        std::array<uint32_t,12> shaft_post{};bool shaft_post_ready{};   // LS-2b: CPostEffect +0x21E0..+0x220F as E4700 left them (the shafts' two colours)
        std::array<uint32_t,5> grading{};bool grading_ready{};uint32_t grading_area{};std::string grading_source;   // W10 GR: the blended ColorGrading integers and the area and the bank source (Banks::GroupEntries: "area" for the area's own map file) of the record whose LUT id is word 1
        // Fable review: the collect's own snapshot of the library globals block (144961F70, 0x370 bytes); the prepare (render
        // worker) plans the glare, DoF and motion blur from it instead of re-reading the live block the next collect rewrites.
        std::array<uint8_t,0x370> globals{};bool globals_read{};};
    std::optional<Yebis> yebis;
    // W5a SW-4 (SHADOWS-WAVE1-READY 2.2, 2.3): the main view's shadow state of the frame, produced at collect from the world record,
    // the quality row and the LightSet sun (present by default since D-0180 C4; I-0132 R11e: no flag opts out). I-0131 R10a: only what
    // the traced sun and the shadowed sun pass read: the sun, the cbSceneParam shadow words +0x1C0..+0x36F (SceneWords; +0x1D0
    // FC_ShadowColor); the quality row no longer gates it (R11e). The cascade fits, matrices, viewports, filter plan and caster volumes are removed.
    struct Shadow {
        std::array<float,4> sun{};                  // F90F0's direction from the view's ShadowParam record (I-0140 tranche 7)
        std::array<uint8_t,0x1B0> scene_words{};
        bool on{};
        uint32_t blur_passes{};float blur_scale{};   // I-0140 tranche 35: a Player menu view's blur (CollectNativeMenuShadowState)
    };
    std::optional<Shadow> shadow_frame;
    struct PreparedMesh {
        size_t model{},mesh{};
        uint32_t fade{},engine_pass{},recipe{};
        ds3content::ModelTextureTiling tiling;
        std::shared_ptr<const ds3content::TextureScrollVersion> scroll;
        // I-0074: the row's VS and PS instance blocks (NativeMeshInstanceBlock), into the scene's models (stable with the scene);
        // every row is made by PrepareNativeSelectedPasses, which sets them.
        std::array<const SharedRegisterBlock*,2> instances{};
    };
    frame_store::Array<PreparedMesh> prepared_meshes;   // I-0091: on the collect's kept blocks
    uint32_t eligible_meshes{},unavailable_preparations{},no_cast_meshes{};
    // 1026 witness: first refusing gate per model, mesh and preparation.
    std::array<uint32_t,32> refusals{};
    //236290's fresh ordinary preparation rate. Later rate-override families
    // require explicit inputs; this does not claim universal setter coverage.
    float uv_rate{1.0f};
    // m45 collect cost: the camera block's written ranges, the same for every model of the collect (built once, at the first admission).
    std::optional<std::vector<ds3content::ShaderConstantRange>> scene_word_ranges;
    std::atomic<uint32_t> objects{},unsupported{};   // the walk's entity filter (I-0077: the main walk's layers run on the collect's workers)
    // I-0071 point 3: counted by the admission, which runs on the collect's workers (atomic; `tables_lock` guards the scene and
    // view block tables and the planned programmes they fill).
    std::atomic<uint32_t> missing_geometry{},nonzero_variant{},missing_bindings{},instance_streams{},missing_program_family{},
        program_recipes{},zero_fade{},no_meshes{},nonfinite{};   // nonfinite: I-0101, a model skipped for transient non-finite data
    std::mutex tables_lock;
    // 1068 witness: asm part admission refusals {part,code} and pass gates {part,gate}.
    std::vector<std::pair<int32_t,uint8_t>> asm_rejected,asm_gates;
    // 1118 witness: pass gates of character-part meshes by material family (shader|transparent|alpha ref).
    std::map<std::string,std::map<int,uint32_t>> asm_family_gates;
    frame_store::Array<NativeSelectedModel> models;   // I-0091: on the collect's kept blocks
    // I-0100 T0 (popin-observe.flag): the main walk's candidates the admission refused, with its outcome code (1 zero_fade,
    // 2 missing inputs, 3 instance streams, 4 missing geometry, 5 no meshes, 6 another failure, 7 non-finite data: I-0101), for the prepare's rows.
    struct PopinRefusal {uintptr_t entity{},model{},owner{};uint8_t code{};};
    std::vector<PopinRefusal> popin_refused;
    // I-0100 tranche 2 (T2a): the main walk's entities by the gate that ended them, filled by the walk's workers, for the rows
    // of PopinNativeGates: the kind in the high nibble (0 map piece, 1 draw entity, 2 assembled character), the gate in the low
    // one (0 passed, 1 frustum, 2 the enable flag, 3 the visibility words, 4 the category).
    std::mutex popin_gates_lock;std::vector<std::pair<uintptr_t,uint8_t>> popin_gates;
};
std::shared_ptr<const NativeSelectedScene> native_selected_scene;
// m45 frame cost (Native1842 sample profile: the previous scene's destruction on the engine thread, 0.4 ms a frame): an object
// whose last reference the engine thread would drop goes to a releasing thread of lower priority. The render worker already
// drops scenes whose frame it held last, so their destruction is not bound to the engine thread.
void NativeReleaseLater(std::shared_ptr<const void> held) noexcept {
    struct Releaser {std::mutex lock;std::condition_variable wake;std::vector<std::shared_ptr<const void>> pending;};
    static Releaser* releaser=[]{auto* r=new Releaser;   // process lifetime by design, as the worker
        std::thread([r]{SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_BELOW_NORMAL);std::vector<std::shared_ptr<const void>> taken;
            for(;;){{std::unique_lock hold(r->lock);r->wake.wait(hold,[&]{return !r->pending.empty();});taken.swap(r->pending);}taken.clear();}}).detach();
        return r;}();
    if(!held)return;
    try{const std::lock_guard hold(releaser->lock);releaser->pending.push_back(std::move(held));}catch(...){return;}   // on failure: released here
    releaser->wake.notify_one();
}
bool NativeSceneHasVelocity(const std::shared_ptr<const NativeSelectedScene>& scene) noexcept {return scene && scene->velocity.has_value();}
std::vector<std::array<uint32_t,10>> NativeSceneLightKeys(const std::shared_ptr<const NativeSelectedScene>& scene) {
    return scene?scene->light_keys:std::vector<std::array<uint32_t,10>>{};}
bool NativeSceneGlow(const std::shared_ptr<const NativeSelectedScene>& scene,NativeGlowInput& glow,uint32_t& width,uint32_t& height) noexcept {
    if(!scene || !scene->glow)return false;glow=*scene->glow;const auto post=NativePostExtent(scene->camera);width=post[0];height=post[1];return true;}   // I-0111: the glow's extent
void InitializeNativeSimulationTime() {
    std::lock_guard<std::mutex> hold(simulation_time_lock);
    if(native_simulation_time)Refuse("native_simulation_time_duplicate_owner");
    // Native service lifetime replaces213A0's zero-initialized worker clock.
    // No original worker, wall clock or frame-count surrogate is published.
    native_simulation_time=std::make_shared<NativeSimulationTime>();
}
void CloseNativeSimulationTime() {
    std::lock_guard<std::mutex> hold(simulation_time_lock);native_simulation_time.reset();
}
void __fastcall UpdateNativeSimulationTime(uintptr_t receiver,uintptr_t tick) noexcept {
    if(control->stop || control->closed || !cpu_registered){simulation_time_original(receiver,tick);return;}
    try {
        uintptr_t actual{};float delta{};BYTE suppressed{};
        if(!Read(image+0x4793348,&actual,8) || actual!=receiver ||
           !Read(tick+8,&delta,4) || !Read(receiver+0xD6,&suppressed,1))
            Refuse("native_simulation_time_current_input_missing");
        std::lock_guard<std::mutex> hold(simulation_time_lock);
        if(!native_simulation_time)Refuse("native_simulation_time_owner_missing");
        auto next=std::make_shared<NativeSimulationTime>(*native_simulation_time);
        if(!frame::AdvanceSimulationTime(next->now,delta,suppressed!=0,next->now))
            Refuse("native_simulation_time_invalid_delta");
        ++next->generation;native_simulation_time=std::move(next);
    }catch(...){Refuse("native_simulation_time_update_failed");}
    // The unchanged CPU body consumes/clears D6 and updates other genuine
    // services. Its missing original-worker branch cannot duplicate this clock.
    simulation_time_original(receiver,tick);
}
// 1186: the previous view-projection of the main view (CollectNativeVelocityInputs), guarded by scene_view_cpu_lock.
struct NativeVelocityHistory { uintptr_t view{};VelocityMatrix view_projection{};Matrix4 pose{};bool valid{}; };
NativeVelocityHistory native_velocity_history;
// C7-1c (PERSISTENT-RECORDS-DESIGN 1, MeshBlocks): blocks carried from one main-view collect to the next when their inputs
// are equal byte for byte: the scene and view tables (a pure function of the programme and the camera and view constants;
// each entry keeps its programme alive, so a key cannot be reused) and each entity's instance table (a pure function of the
// programme and the model's instance records; the entry keeps the programmes its keys came from). Owner thread (the main
// collect); cleared with the selected scene (area change, Close). Menu views never use them.
// I-0077 (ER A: per-entity scene data kept by the entity, its own Draw job the only writer in a frame): per-entity stores of
// the main collect split into 64 shards by entity address, each with its own lock, so the admission workers (one entity each
// in a collect) no longer queue on one lock (i74-native-53: a third of each worker's admission time was the global locks).
template<class Value> struct NativeEntityShards {
    static constexpr size_t count=64;
    struct alignas(64) Shard {std::mutex lock;std::unordered_map<uintptr_t,Value> map;};
    std::array<Shard,count> shards;
    Shard& Of(uintptr_t entity) noexcept {return shards[(entity*0x9E3779B97F4A7C15ull)>>58];}
    void Clear() {for(auto& shard:shards){const std::lock_guard hold(shard.lock);shard.map.clear();}}
    template<class Predicate> void EraseIf(Predicate predicate) {for(auto& shard:shards){const std::lock_guard hold(shard.lock);std::erase_if(shard.map,predicate);}}
    size_t Size() {size_t n{};for(auto& shard:shards){const std::lock_guard hold(shard.lock);n+=shard.map.size();}return n;}
};
struct NativeBlockCarry {
    std::mutex lock;   // the scene and view tables and the planned programmes (owner thread); the instances have their own shards
    bool scene_valid{};std::array<uint8_t,960> camera{};std::array<uint8_t,240> written{};std::array<uint8_t,128> view{};
    // I-0078: shared with the scenes that carried them (NativeCarriedTable), replaced only when a scene added entries.
    std::shared_ptr<const NativeCarriedTable<NativeSelectedScene::SharedBlock>::Map> scene_table,view_table;
    // The programmes planned since the tables were last cleared (their entries are in the carried tables), owned, so a carried
    // address can never name another programme.
    std::shared_ptr<const NativeCarriedTable<std::shared_ptr<const NativeModelLayout::Programs>>::Map> planned;
    // An instance table's keys are the programmes in planned (owned: every programme it was filled for while carried).
    struct Instances {uintptr_t model{};uint32_t count{};std::array<NativeSelectedModel::InstanceConstants,2> records{};
        std::shared_ptr<const NativeInstanceBlockTable> table;uint64_t serial{};
        NativePlannedPrograms planned;};
    NativeEntityShards<Instances> instances;uint64_t last_sweep{};
};
NativeBlockCarry native_block_carry;
void ResetNativeModelRecords();   // C7-2, with the model records below
void ResetNativeLightKeys();      // I-0071, with the light key registry below
void ResetNativeSelectedScene() {
    const std::lock_guard<std::recursive_mutex> hold(scene_view_cpu_lock);native_selected_scene.reset();native_velocity_history.valid=false;
    {const std::lock_guard carry(native_block_carry.lock);native_block_carry.scene_valid=false;native_block_carry.scene_table.reset();
        native_block_carry.view_table.reset();native_block_carry.planned.reset();native_block_carry.instances.Clear();}
    ResetNativeModelRecords();ResetNativeLightKeys();ResetNativeRtKept();   // I-0117 T3 step review M3
    native_rt_probe_states->Clear();   // I-0129 R9a's step review L1: a load's addresses and cells start without the last map's light
    NoteNativeKeptDrawReset();   // C4-2: the next main prepare drops its kept draws (no wait here for a prepare in progress)
}
std::shared_ptr<const NativeSelectedScene> AcquireNativeSelectedScene() {
    const std::lock_guard<std::recursive_mutex> hold(scene_view_cpu_lock);return native_selected_scene;
}
thread_local bool native_offscreen_preparing{};   // 1169: offscreen menu view selection/preparation (separate witness samples)
struct alignas(16) NativeSelectionVisitor {
    std::array<BYTE,0x80> base{};
    NativeSelectedScene* result{};
    uint32_t world_mask{};
    uint32_t objects{};   // I-0077: the entities this traverser visited (the main walk's per-layer witness)
    // I-0117 T5b: the rays' walk's traversers (the main view's diagnostics leave them alone; I-0125 R6f's final review L1: in place
    // of a walk's own frustum, which only the removed mirror view drew with).
    bool rays_walk{};
    // I-0071 point 3 (ER A): the main walk only gathers its candidates here; they are admitted on workers after the walk.
    std::vector<struct NativeDeferredCandidate>* deferred{};
    // I-0140 tranche 31 (D-0303): a Player menu view's collect: each candidate also gathered here for its traced scene, while the
    // admission goes on as without it (CollectNativeMenuRtScene).
    std::vector<struct NativeDeferredCandidate>* rays_also{};
    // I-0077 (ER 141B4CF40, the entity visit with the collect byte +0x13): the main walk's traversers only test each entity's
    // visibility against the walk's bounds stack and gather it here; its eligibility and candidates are made afterwards on the
    // pool (ER's GXEntityDrawParallelJob over the gathered list), by a traverser marked `expanding` (counted once, by the walk).
    std::vector<uintptr_t>* gather{};bool expanding{};
    bool uncounted{};   // I-0117 T5b: the rays' walk's traversers: their visits counted, not against the scene's object bound
    uint8_t tail_padding[14]{};   // I-0125 R6f: the size a multiple of 16 without the compiler's padding warning (the frustum pointer gone; I-0140 t31: rays_also)
};
static_assert(offsetof(NativeSelectionVisitor,result)==0x80);
static_assert(sizeof(NativeSelectionVisitor)%16==0 && offsetof(NativeSelectionVisitor,tail_padding)+14==sizeof(NativeSelectionVisitor));
// I-0100 tranche 4 (T2b, D-0220 « a margin against flicker »; Thomas, 2026-10-26 17:56-18:03: a silver knight, villagers, the
// woods' crab and the man-serpents vanishing and coming back near him): DS3's frustum tests (the tree's nodes, then a draw entity's
// bones' box E+0xB80, D08190) end a character whose box has just left the view while its mesh, wider than its bones, may still be
// on screen, and flip it at the view's edge as its animation moves the box (c2131 at Anor Londo, i100-t4a-al-n1; the Settlement's
// c1240, i100-t4b/t4c). The main walk runs on a copy of the view's culler whose six planes and world box are pushed out by
// kNativeCullMargin metres (CollectNativeSelectedScene), so DS3's own tests keep what lies within about that margin of the view
// (less near the far corners, where 6F310's first test, the grown world box, cuts before the planes; step review L6);
// no-cull-margin.flag walks the view's culler itself. `real` is the view's culler, for the gate rows (popin gate 6, « margin »:
// a draw entity the view's culler alone would have ended; not counted under a node fully inside the grown culler, nor for map
// pieces: the rows undercount what the margin adds, step review L3).
constexpr float kNativeCullMargin=1.5f;
struct NativeCullMargin {uintptr_t real{};bool active{};float sign{};float forward[3]{};};   // sign: the planes' inside side (+1 or -1)
NativeCullMargin native_cull_margin;
bool PopinObserve() noexcept;double PixelKeyClockMs() noexcept;uint64_t PixelKeyWindow(double ms) noexcept;
// I-0141 tranche 22 (popin-observe.flag; RESULT « Suggested next step » 3): in the main view's walk (gathered, or light-observe.flag's
// whole filter), a map piece admitted (past its gates 1 to 4: the widened culler, by its own test or a node fully inside it, its flag,
// its visibility words and its category) whose box E+0x40 the view's own culler (6F310) ends: which of 6F310's two
// stages ends it, its world box (14001FFF0 on +0x220, tested first) or a plane (+0x160); per plane, the signed distance in metres of
// the box's corner farthest on the plane's inside side (negative: the whole box on its outer side) and the dot of the plane's inside
// normal with the camera's forward axis (near about +1, far about -1); the gap per axis between the box and the view's world box
// (positive: apart); the box; the piece's first models. Once per 60-second window and (entity, world box apart, planes out), at most
// 20,000 rows for the session.
void NoteNativeMarginPiece(uintptr_t entity) noexcept {
    static std::atomic<bool> full{};
    if(!PopinObserve())return;   // the optimisation pass: the flag first, before the margin's copy
    const auto margin=native_cull_margin;
    if(!margin.active || !margin.real || native_offscreen_preparing || full.load(std::memory_order_relaxed))return;
    try{
        const auto classify=reinterpret_cast<int(__fastcall*)(uintptr_t,uintptr_t)>(image+0x06F310);
        if(classify(margin.real,entity+0x40))return;
        alignas(16) float box[8]{},world[8]{};float planes[6][4]{};
        if(!Read(entity+0x40,box,sizeof box) || !Read(margin.real+0x220,world,sizeof world) || !Read(margin.real+0x160,planes,sizeof planes))return;
        const bool world_overlap=reinterpret_cast<char(__fastcall*)(uintptr_t,uintptr_t)>(image+0x01FFF0)(margin.real+0x220,entity+0x40)!=0;
        float distance[6]{},facing[6]{};uint32_t out_mask{};
        for(int p=0;p<6;++p){const auto& q=planes[p];const float n=std::sqrt(q[0]*q[0]+q[1]*q[1]+q[2]*q[2]);if(!(n>0.f) || !std::isfinite(n))return;
            float best=q[3];for(int k=0;k<3;++k)best+=q[k]*(margin.sign*q[k]>=0.f?box[4+k]:box[k]);
            distance[p]=margin.sign*best/n;facing[p]=margin.sign*(q[0]*margin.forward[0]+q[1]*margin.forward[1]+q[2]*margin.forward[2])/n;
            if(distance[p]<0.f)out_mask|=1u<<p;}
        float gap[3]{};for(int k=0;k<3;++k)gap[k]=std::max(world[k]-box[4+k],box[k]-world[4+k]);
        const double ms=PixelKeyClockMs();
        static std::mutex lock;static std::set<std::tuple<uintptr_t,bool,uint32_t>> seen;static uint64_t window{};static size_t written{};bool last{};
        {const std::lock_guard held(lock);const auto now=PixelKeyWindow(ms);if(now>window){window=now;seen.clear();}
            if(full.load(std::memory_order_relaxed) || now<window || !seen.insert({entity,world_overlap,out_mask}).second)return;
            if(++written>=20000){full=true;last=true;}}
        std::string names;uintptr_t head{},node{};uint32_t walked{};if(Read(entity+0x90,&head,8) && head)Read(head,&node,8);
        for(;node && node!=head && walked<4;++walked){uintptr_t item{},model{};if(Read(node+0x10,&item,8) && item && Read(item+0x58,&model,8) && model){
            for(const auto ch:NativeModelDataName(model))names.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');names+=' ';}if(!Read(node,&node,8))break;}
        char row[2048]{};sprintf_s(row,"{\"event\":\"native_margin_piece\",\"qpc_ms\":%.1f,\"entity\":%llu,\"stage\":\"%s\",\"planes_out\":%u,"
            "\"distance\":[%.2f,%.2f,%.2f,%.2f,%.2f,%.2f],\"facing\":[%.2f,%.2f,%.2f,%.2f,%.2f,%.2f],\"world_gap\":[%.2f,%.2f,%.2f],"
            "\"box\":[%.1f,%.1f,%.1f,%.1f,%.1f,%.1f],\"models\":\"%.200s\"}\n",ms,static_cast<unsigned long long>(entity),
            world_overlap?"plane":"world_box",out_mask,distance[0],distance[1],distance[2],distance[3],distance[4],distance[5],
            facing[0],facing[1],facing[2],facing[3],facing[4],facing[5],gap[0],gap[1],gap[2],box[0],box[1],box[2],box[4],box[5],box[6],names.c_str());Log(row);
        if(last)Log("{\"event\":\"native_margin_piece_full\",\"rows\":20000}\n");
    }catch(...){}
}
// I-0117 (R2 T1): during the rays' world walk on this thread, the entities each gate ended or passed (the pop-in gate codes:
// kind << 4 | gate; 0 admitted), so the walk's row says which gate keeps a loaded entity out of the rays.
thread_local std::array<uint32_t,64>* native_rt_gates{};
// Set when the disp service is missing during that walk (every map piece would fail): the walk is skipped, never the game closed;
// one entity's failed visit skips that entity alone (AdmitNativeSelectedObject, I-0117 T2).
thread_local bool native_rt_walk_failed{};
// I-0077: the view a collect runs for (this thread flag), taken by the collecting thread and set on each pool worker for the
// time of its job, so a walk's or an admission's work on the pool sees the same view as on the collecting thread (I-0125 R6f,
// D-0251: the water's mirror view and its flags removed, the water traced).
struct NativeViewFlags {
    bool offscreen{native_offscreen_preparing};
    struct Apply {bool offscreen;
        explicit Apply(const NativeViewFlags& v):offscreen(native_offscreen_preparing){native_offscreen_preparing=v.offscreen;}
        ~Apply(){native_offscreen_preparing=offscreen;}};
};
std::vector<ds3content::ShaderConstantRange> NativeShaderWordRanges(
    std::span<const uint8_t> bytes,std::span<const uint8_t> written) {
    if(bytes.size()%4 || written.size()!=bytes.size()/4)
        Refuse("native_constant_word_coverage_extent");
    std::vector<ds3content::ShaderConstantRange> ranges;
    for(size_t word=0;word<written.size();){
        if(written[word]>1)Refuse("native_constant_word_coverage_value");
        if(!written[word]){++word;continue;}
        const auto begin=word++;
        while(word<written.size() && written[word]==1)++word;
        ranges.push_back({static_cast<uint32_t>(begin*4),bytes.subspan(begin*4,(word-begin)*4)});
    }
    return ranges;
}
// D-0180 C4 (image regression): the light and fog sets of the main view. The original's 137EF0 deduplicates the drawn
// objects' key blocks {light a, light b, fog a, fog b, light t, fog t} (group +F8, +110, +108, +120) each frame into at most
// 225 entries and 5B3D0 writes each object's entry index as instance envData; 137910 builds cbLight/cbFog for every entry.
// Natively each admitted model's key block gets an index, its envData, and CollectNativeLightBlocks builds a set for every
// index (the scene's `light_keys`, NativeLightKeys after the walks). I-0071 point 3: a key keeps its index across collects (it no
// longer depends on which worker admits a model first, and a model keeps its envData from one collect to the next). The
// per-entity blend weights (+0x108, +0x120) make new keys while an entity crosses a gparam blend, so a slot unseen for 8 main
// collects is freed and taken again by a new key; slots are freed, and a scene reset applied, only between collects
// (BeginNativeLightKeys at the main collect's start, pre-merge review H1), and a new key with all 225 slots taken gets 0 (logged).
using NativeLightKey=std::array<uint32_t,10>;
struct NativeLightKeyRegistry {
    static constexpr size_t limit=225;
    std::mutex lock;std::vector<NativeLightKey> keys;std::vector<uint8_t> used;
    std::array<std::atomic<uint64_t>,limit> last_seen{};
    std::atomic<uint64_t> generation{1},collect{1};std::atomic<bool> reset{};
};
NativeLightKeyRegistry native_light_key_registry;
// I-0125 R6b (its step review M3): a key's set when the raster registered it, else 0; registers nothing (the rays' instances, many
// never drawn, would take the registry's 225 sets from the drawn models).
uint32_t NativeLightSetFound(const NativeLightKey& key) {
    auto& registry=native_light_key_registry;
    struct Cache {uint64_t generation{UINT64_MAX};std::vector<NativeLightKey> keys;std::vector<uint8_t> used;};thread_local Cache cache;
    const auto generation=registry.generation.load(std::memory_order_acquire);
    if(cache.generation!=generation){const std::lock_guard hold(registry.lock);cache.keys=registry.keys;cache.used=registry.used;cache.generation=registry.generation.load(std::memory_order_relaxed);}
    for(size_t i=0;i<cache.keys.size();++i)if(cache.used[i] && cache.keys[i]==key)return static_cast<uint32_t>(i);
    return 0;
}
uint32_t NativeLightSetIndex(const NativeLightKey& key) {
    auto& registry=native_light_key_registry;const auto collect=registry.collect.load(std::memory_order_relaxed);
    struct Cache {uint64_t generation{};std::vector<NativeLightKey> keys;std::vector<uint8_t> used;};thread_local Cache cache;   // the registry as this thread last read it
    if(cache.generation==registry.generation.load(std::memory_order_acquire))
        for(size_t i=0;i<cache.keys.size();++i)if(cache.used[i] && cache.keys[i]==key){registry.last_seen[i].store(collect,std::memory_order_relaxed);return static_cast<uint32_t>(i);}
    const std::lock_guard hold(registry.lock);auto& keys=registry.keys;
    size_t at=0;while(at<keys.size() && !(registry.used[at] && keys[at]==key))++at;
    if(at==keys.size()){
        at=0;while(at<keys.size() && registry.used[at])++at;   // a freed slot first
        if(at==registry.limit){static LONG64 full{};if(InterlockedIncrement64(&full)<=4)Log("{\"event\":\"native_light_sets_full\",\"limit\":225}\n");return 0;}
        if(at==keys.size()){keys.push_back(key);registry.used.push_back(1);}else{keys[at]=key;registry.used[at]=1;}
        registry.generation.fetch_add(1,std::memory_order_acq_rel);}
    registry.last_seen[at].store(collect,std::memory_order_relaxed);
    cache.keys=keys;cache.used=registry.used;cache.generation=registry.generation.load(std::memory_order_acquire);
    return static_cast<uint32_t>(at);
}
// The main collect's start: a requested reset, else the slots unseen for 8 collects freed (a freed slot keeps its key, so the
// snapshot still builds a valid set for it, until a new key takes it).
void BeginNativeLightKeys() {
    auto& registry=native_light_key_registry;const auto collect=registry.collect.fetch_add(1,std::memory_order_relaxed)+1;
    const std::lock_guard hold(registry.lock);bool freed=false;
    if(registry.reset.exchange(false)){registry.keys.clear();registry.used.clear();freed=true;}
    else for(size_t i=0;i<registry.keys.size();++i)if(registry.used[i] && registry.last_seen[i].load(std::memory_order_relaxed)+8<collect){registry.used[i]=0;freed=true;}
    if(freed)registry.generation.fetch_add(1,std::memory_order_acq_rel);   // the threads' caches read the registry again
}
std::vector<NativeLightKey> NativeLightKeys() {const std::lock_guard hold(native_light_key_registry.lock);return native_light_key_registry.keys;}
// I-0141 tranche 17's witness: whether slot `set` is in use and holds `key` (false for NativeLightSetIndex's full fallback to 0).
bool NativeLightSetHolds(uint32_t set,const NativeLightKey& key) {auto& registry=native_light_key_registry;const std::lock_guard hold(registry.lock);
    return set<registry.keys.size() && registry.used[set] && registry.keys[set]==key;}
void ResetNativeLightKeys() {native_light_key_registry.reset.store(true);}   // applied at the next main collect's start
// Complementary check (the m33 darkening, Thomas 2026-10-18; OPTIMISATION-REVIEW 1): the instance ambient rows of a light key
// from the native banks (defined after NativeGparamRecords); false when the banks hold no record for the key.
bool NativeInstanceLightRows(const uint32_t* key,std::array<float,12>& rows);
// I-0125 R6a: 5B3D0's rows from the draw's animation object (the descriptor: +0x30 atlasUvScaleOffset from its +0xB8, +0xC0
// vDominantRotateParam from its +0xD0 or the default light), false when its flags cannot be read; and its lighting inputs (the SH rows
// +0x80..+0xBF) from the group's two banks or the owning animation. The raster's instance record and the rays' ambient read the same.
bool NativeInstanceAnimationRows(uintptr_t animation,ds3prod::InstanceRecordInputs& input) {
    BYTE first{},second{};
    if(!Read(animation+0xE0,&first,1) || !Read(animation+0xE1,&second,1))return false;
    std::array<float,4> colour{},auxiliary{};
    if(first || second)input.animation_colour=std::array<float,4>{1,1,0,0};
    else if(Read(animation+0xB8,colour.data(),16))input.animation_colour=colour;
    if(Read(second?image+0x4929B20:animation+0xD0,auxiliary.data(),16))input.animation_auxiliary=auxiliary;
    return true;
}
std::optional<ds3prod::InstanceLightingInputs> NativeInstanceLighting(uintptr_t group,uintptr_t animation,const std::array<float,3>& row_scale) {
    const auto copy=[&](uintptr_t at,auto& value){return Read(at,value.data(),sizeof(value));};
    const auto scalar=[&](uintptr_t at,auto& value){return Read(at,&value,sizeof(value));};
    uintptr_t root{},table{};ds3prod::InstanceLightingInputs light;
    if(Read(image+0x4961B50,&root,8) && root && Read(root+0x41E0,&table,8) && table && scalar(group+0x108,light.blend)){
        const auto lookup=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uintptr_t)>(image+0x32A80);
        const uintptr_t bank_first=lookup(table,group+0xF8),bank_second=lookup(table,group+0x100);
        std::array<float,16> one{},two{};
        light.banks_present=bank_first && bank_second && copy(bank_first+0x290,one) && copy(bank_second+0x290,two);
        if(light.banks_present)for(size_t k=0;k<12;++k){light.bank_rows[0][k]=one[k];light.bank_rows[1][k]=two[k];}
        if(light.banks_present){light.bank_strength[0]=one[12];light.bank_strength[1]=two[12];}
        // The m33 darkening (OPTIMISATION-REVIEW 1.4, Native2211 / Reference2212): natively the engine's banks hold no file, so
        // 32A80 answers its default record whose rows +0x290 are zero, while the original's rows are 1495F0 of the key's set
        // (three rows [0, -0.097, 0, 0.153] at m33). The rows come from the native banks' record of each key instead; the
        // engine's strength (+0x2C0, the set's LightMask Influence: 0.96 for most of m32's sets, 1 at m33; natively the engine record's 1, a gap
        // where an animation owns the model, I-0141 RESULT) stays.
        if(light.banks_present){std::array<float,12> rows{};uint32_t key_a[2]{},key_b[2]{};
            if(Read(group+0xF8,key_a,8) && NativeInstanceLightRows(key_a,rows))light.bank_rows[0]=rows;
            if(Read(group+0x100,key_b,8) && NativeInstanceLightRows(key_b,rows))light.bank_rows[1]=rows;}
        BYTE owned{};
        if(scalar(animation+0x170,owned) && owned){std::array<float,12> rows{};float strength{};
            if(copy(animation+0x130,rows) && scalar(animation+0x16C,strength)){
                light.animation_owned=true;light.animation_rows=rows;light.animation_strength=strength;}}
        light.row_scale=row_scale;   // 5B3D0: every branch's rows (banks, owning animation, override) scaled by the pass context's +0x110
        if(light.banks_present || light.animation_owned)return light;
    }
    return std::nullopt;
}
// I-0129 R9a (R9-PLAN §1.1, §2): a forward model's traced probe into its instance record, on the main view: on a frame whose probes traced the
// ambient (a GI frame), the three aSh rows (+0x80..+0xAF) the probe's (the forward programmes' max(dp4((n, 1), row), 0) a channel);
// where they traced the sun, ShLightMask.x (+0xB0, the first directional light's mask, the sun's) times the probe's visible share
// (its other words 1 where the record left them unwritten). The values quantized (RtProbeQuantize: the same bytes while the light is
// still). A model without a state keeps DS3's rows and mask.
// A forward model's probe key: its draw group with its model (a character's parts share the group; only the models that draw forward
// rows are asked for, so a deferred part keeps its record: its GB1.z, the sun's mask, stays DS3's under the traced shadow); bit 63
// clear (an effect's cell has it).
uint64_t NativeRtProbeKey(uintptr_t group,uintptr_t model) noexcept {return (uint64_t(group)*0x9E3779B97F4A7C15ull^uint64_t(model))&~(1ull<<63);}
// rt-probe-observe.flag (a diagnostic): a model's baked and traced rows' band 0 (rgb, the ambient a surface facing nowhere in
// particular takes) and its mask before and after, its key's first 8 records and then every 600th (at most 4,000 rows).
void ApplyNativeRtProbe(uint64_t key,NativeSelectedModel::InstanceConstants& constants,uintptr_t model) {
    const auto state=native_rt_probe_states->Find(key,GetTickCount64());if(!state)return;
    static const bool observe=ModuleFlag(L"rt-probe-observe.flag");
    float before[13]{};if(observe){std::memcpy(before,constants.bytes.data()+0x80,52);if(!constants.written[0x80/4])std::fill(std::begin(before),std::end(before),-1.f);}
    const auto put=[&](uint32_t at,float v){v=RtProbeQuantize(v);std::memcpy(constants.bytes.data()+at,&v,4);constants.written[at/4]=1;};
    if(state->ambient)for(uint32_t k=0;k<12;++k)put(0x80+4*k,state->rows[k]);
    if(state->sun_valid){float mask=1.f;if(constants.written[0xB0/4])std::memcpy(&mask,constants.bytes.data()+0xB0,4);else for(const uint32_t at:{0xB4u,0xB8u,0xBCu})put(at,1.f);
        put(0xB0,(std::isfinite(mask)?mask:1.f)*state->sun);}
    if(observe){static std::mutex lock;static std::unordered_map<uint64_t,uint32_t> seen;static uint32_t rows{};bool show{};
        {const std::lock_guard hold(lock);const auto n=seen[key]++;show=rows<4000 && (n<8 || n%600==0);if(show)++rows;}
        if(show){float after[13]{};std::memcpy(after,constants.bytes.data()+0x80,52);std::string name;for(const auto ch:NativeModelDataName(model))name.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');
            char row[512]{};sprintf_s(row,"{\"event\":\"native_rt_probe_record\",\"key\":%llu,\"model\":\"%.60s\",\"ambient\":%s,\"sun\":%s,\"baked_l0\":[%.4g,%.4g,%.4g],\"traced_l0\":[%.4g,%.4g,%.4g],\"baked_up\":[%.4g,%.4g,%.4g],\"traced_up\":[%.4g,%.4g,%.4g],\"mask\":[%.4g,%.4g]}\n",
                static_cast<unsigned long long>(key),name.c_str(),state->ambient?"true":"false",state->sun_valid?"true":"false",before[3],before[7],before[11],after[3],after[7],after[11],
                before[3]+before[1],before[7]+before[5],before[11]+before[9],after[3]+after[1],after[7]+after[5],after[11]+after[9],before[12],after[12]);Log(row);}}
}
// row_scale: the scene's pass_row_scale (the pass context's +0x110..+0x118 that 5B3D0 and 59470 read).
// I-0137 (I-0120 B6): false when the group's or the model's fields cannot be read (an entity freed under the walk): the caller
// skips the model for this collect instead of closing the game.
bool ProduceNativeSelectedInstances(NativeSelectedModel& selected,bool light_sets,const std::array<float,3>& row_scale) {   // light_sets: the model takes a light and fog set (envData)
    ds3prod::InstanceRecordInputs input;const auto group=selected.group;
    const auto copy=[&](uintptr_t at,auto& value){return Read(at,value.data(),sizeof(value));};
    const auto scalar=[&](uintptr_t at,auto& value){return Read(at,&value,sizeof(value));};
    if(!copy(group+0xD0,input.group_colour) || !copy(group+0x148,input.origin) ||
       !copy(group+0x154,input.bounds_min) || !copy(group+0x160,input.bounds_max) ||
       !scalar(group+0x13C,input.origin_w) || !scalar(group+0x140,input.bounds_min_w) ||
       !scalar(group+0x144,input.bounds_max_w))return false;
    BYTE instance_flag{},matrix_enabled{};
    if(!scalar(group+0x87,instance_flag) || !scalar(selected.model+0x1E0,matrix_enabled))return false;
    input.instance_flag=instance_flag!=0;
    // P4: a phantom ChrAsm part's CFDE70 stack copy has +0x87 = 1 and mulCol.w times the row fade.
    if(selected.phantom){input.instance_flag=true;input.group_colour[3]*=selected.phantom_fade;}
    // 1177 witness: the instance colour row (+0x70 = group +0xD0; the hair PS alpha is its .w) of a menu part.
    if(native_offscreen_preparing){static LONG64 colour_rows{};if(InterlockedIncrement64(&colour_rows)<=24){std::array<float,4> tint{};Read(group+0x40,tint.data(),16);
        char row[256]{};sprintf_s(row,"{\"event\":\"native_menu_instance_colour\",\"group_colour\":[%g,%g,%g,%g],\"group_tint\":[%g,%g,%g,%g]}\n",
            input.group_colour[0],input.group_colour[1],input.group_colour[2],input.group_colour[3],tint[0],tint[1],tint[2],tint[3]);Log(row);}}
    // A fresh ordinary 5C0E0 record zeros its +110/+118 bounds gates. Other
    // instance families are already outside this selected owner contract.
    input.bounds_suppressed=false;input.palette_base=0;
    if(matrix_enabled && selected.pose && !selected.pose->matrices.empty())
        input.object_rows=selected.pose->matrices.front();
    else if(!matrix_enabled){
        std::array<float,16> authored{};std::array<float,12> rows{};
        if(!copy(image+0x469C9E0,authored))Refuse("native_instance_authored_matrix_unreadable");
        for(size_t r=0;r<3;++r)for(size_t c=0;c<4;++c)rows[4*r+c]=authored[4*c+r];
        input.object_rows=rows;
    }
    // I-0113 tranche 14: 5C0E0's copy of the group carries 4E2A0's blend and scales where the group's own are read.
    if(selected.region_ibl){input.origin_w=selected.region_ibl->blend;input.bounds_min_w=selected.region_ibl->scales[0];input.bounds_max_w=selected.region_ibl->scales[1];}
    if(!(selected.fade.group_flags&2u))input.clip=ds3prod::InstanceClipInputs{};
    // D079C0 descriptor+90 = entity+3D0, carried to instance+68. Only
    // the actual CPU colour inputs are read; no old draw record is created.
    const auto animation=selected.animation;
    // E03 (I-0105, Lightning Spear's s03000 refused for GXFlver_Col's +0x30): 59470 leaves +0x30 and +0xC0 unwritten for a draw
    // without an animation object (29ADF0's); the original's block holds its stack there, zero in nearly every draw read
    // (i105-bolt-ref-3, reference_e03_instance; the other words stray bits of earlier frames), so both are zero.
    if(!NativeInstanceAnimationRows(animation,input) && selected.effect && !animation){input.animation_colour=std::array<float,4>{};input.animation_auxiliary=std::array<float,4>{};}
    // 1043 instance lighting (5B3D0, old host ProduceInstanceRecord, I-0063):
    // the two light parameter sets are the genuine CPU lookups 32A80 of the
    // group keys +F8/+100 in [root 4961B50]+41E0 (native root layer D-0160),
    // viewed from +290; blend group+108; an owning animation (+170) supplies
    // its rows +130 and strength +16C. The device-absent root has no render
    // override (+4DD0). The pass-context row scales (context+110..118) are
    // the scene's pass_row_scale (I-0113 tranche 34; 1 before).
    std::optional<ds3prod::InstanceLightingInputs> lighting;
    // E03 (I-0105): a model particle with its own light (the submission's args+0x424) takes 59470's branch for it: its rows, scale
    // and strength, before any bank or render override.
    if(selected.effect && selected.effect->submission.light[0x44]){const auto& block=selected.effect->submission.light;ds3prod::InstanceLightingInputs light;
        light.state_owned=true;std::memcpy(light.animation_rows.data(),block.data(),48);std::memcpy(light.state_scale.data(),block.data()+0x30,16);
        std::memcpy(&light.state_strength,block.data()+0x40,4);light.row_scale=row_scale;lighting=light;}   // 59470 scales this branch's rows too (its param_1+0x110)
    else lighting=NativeInstanceLighting(group,selected.animation,row_scale);
    // The set index of the group's key block (main view; a menu view's fixed key is the single set 0).
    std::optional<uint32_t> light_set;
    if(light_sets && !native_offscreen_preparing){NativeLightKey key{};
        const bool key_read=Read(group+0xF8,&key[0],16) && Read(group+0x110,&key[4],16) && Read(group+0x108,&key[8],4) && Read(group+0x120,&key[9],4);
        if(key_read)light_set=NativeLightSetIndex(key);
        // Complementary check (Thomas, 2026-10-18, the m33 darkening): with light-observe.flag, a model's light/fog key block
        // (group +F8..+120) each time it changes (at most 6,000 rows), with the set index it took, to compare with
        // reference_model_light_key (the same block of the draw state 57A20 reads in the original).
        static const bool observe=ModuleFlag(L"light-observe.flag");
        // Keyed by the draw group (one per entity: several entities share a model with different keys), at most 6,000 rows.
        if(observe && key_read){static std::mutex lock;static std::unordered_map<uintptr_t,NativeLightKey> last;static int rows{};bool fresh{};
            {const std::lock_guard hold(lock);if(rows<6000){auto& k=last[group];if(k!=key){k=key;++rows;fresh=true;}}}
            if(fresh){std::string name;for(const auto ch:NativeModelDataName(selected.model))name.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');
                float t[2]{};std::memcpy(&t[0],&key[8],4);std::memcpy(&t[1],&key[9],4);char row[640]{};
                // The instance lighting rows the engine's lookup (32A80, root +41E0, the record's +0x290) gave for key a, and its strength.
                std::string sh="null";if(lighting && lighting->banks_present){sh="[";for(size_t k=0;k<12;++k){char cell[24]{};sprintf_s(cell,"%s%.5g",k?",":"",lighting->bank_rows[0][k]);sh+=cell;}
                    char tail[32]{};sprintf_s(tail,"],\"strength\":%.5g",lighting->bank_strength[0]);sh+=tail;}else sh="null,\"strength\":null";
                sprintf_s(row,"{\"event\":\"native_model_light_key\",\"model\":\"%.60s\",\"group\":%llu,\"asm_part\":%d,\"light\":[%u,%u,%u,%u],\"light_t\":%g,\"fog\":[%u,%u,%u,%u],\"fog_t\":%g,\"set\":%d,\"sh\":%s}\n",
                    name.c_str(),static_cast<unsigned long long>(group),selected.asm_part,key[0],key[1],key[2],key[3],t[0],key[4],key[5],key[6],key[7],t[1],light_set?static_cast<int>(*light_set):-1,sh.c_str());Log(row);}}
        // I-0141 tranche 17 (pixel-keys.flag, the castle's keys at Archdragon Peak): once per 60-second window (PixelKeyWindow, as
        // reference_pixel_key's) and (model, group, key words 0 to 7, whether the registry was full): neither the blend weights nor
        // the set (NativeLightSetIndex keys on all ten words, so a blend crossing moves the set) are keyed, the first sample's
        // written (the second step review's H1), so a blend's weights are not followed within a state. set_full: set 0 that does not hold this block (the registry's 225 slots taken, its fallback; the third
        // step review's M2). At most 40,000 rows, each with its QPC time (a row whose time falls before the current window is
        // dropped), then one native_pixel_key_full row: the set this model's envData and so its GBuffer3.w take (the registry's slot
        // of its own full block), against the original's entry at its own envData. native_pixel_key_list beside each world picture
        // maps a picture's GB3.w to its key.
        static const bool pixel_keys=ModuleFlag(L"pixel-keys.flag");static std::atomic<bool> pixel_rows_full{};
        if(pixel_keys && key_read && light_set && !pixel_rows_full.load(std::memory_order_relaxed)){const auto name=NativeModelDataName(selected.model);const double ms=PixelKeyClockMs();
            const bool set_full=*light_set==0 && !NativeLightSetHolds(0,key);
            std::array<uint32_t,11> seen_key{};std::memcpy(seen_key.data(),key.data(),32);
            seen_key[8]=static_cast<uint32_t>(group);seen_key[9]=static_cast<uint32_t>(group>>32);seen_key[10]=set_full?1u:0u;
            static std::mutex lock;static std::set<std::pair<std::string,std::array<uint32_t,11>>> seen;static uint64_t window{};static size_t written{};bool fresh{},last{};
            {const std::lock_guard held(lock);const auto now=PixelKeyWindow(ms);if(now>window){window=now;seen.clear();}   // forward only (L1)
                fresh=now>=window && written<40000 && seen.insert({name,seen_key}).second;if(fresh && ++written>=40000){pixel_rows_full=true;last=true;}}
            if(fresh){std::string clean;for(const auto ch:name)clean.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');float t[2]{};std::memcpy(t,&key[8],8);char row[520]{};
                sprintf_s(row,"{\"event\":\"native_pixel_key\",\"qpc_ms\":%.1f,\"model\":\"%.60s\",\"group\":%llu,\"set\":%u,\"set_full\":%s,\"own\":[%u,%u,%u,%u,%u,%u,%u,%u],\"own_t\":[%s,%s]}\n",ms,clean.c_str(),
                    static_cast<unsigned long long>(group),*light_set,set_full?"true":"false",key[0],key[1],key[2],key[3],key[4],key[5],key[6],key[7],PixelKeyNumber(t[0]).c_str(),PixelKeyNumber(t[1]).c_str());Log(row);
                if(last){sprintf_s(row,"{\"event\":\"native_pixel_key_full\",\"qpc_ms\":%.1f,\"rows\":40000}\n",ms);Log(row);}}}}
    for(uint32_t i=0;i<selected.geometry_fades.count;++i){
        input.opacity=selected.geometry_fades.intents[i].opacity;
        // 5B3D0 writes the lighting rows for row types 0, 3 and 4 (*param_6: the recorder row's +0x30 byte, 59220), the colour
        // rows; its other types are the depth (2) and velocity (5) rows, which read no ambient. That type is not the LOD variant
        // (I-0100 tranche 2: at the m33 lake the original's map pieces are rows of type 3 and its objects 0 or 3, lit, where the
        // native's variants are 0 to 2): the native's main-view record, shared by its passes, takes the rows at every LOD; gated
        // by the variant, every model drawn at LOD 1 or 2 lost its ambient (dark from far; the A/B sh-by-lod.flag removed after
        // Thomas's check at Anor Londo, the Settlement and the Crucifixion Woods, I-0100 tranche 4).
        const auto variant=selected.geometry_fades.intents[i].variant;   // the witnesses' (light-observe rows, the 1142 SH samples)
        input.lighting=lighting;
        auto& constants=selected.instances[i];
        ds3prod::ProduceInstanceRecord(input,constants.bytes.data(),constants.written.data());
        if(!native_offscreen_preparing && !selected.effect)ApplyNativeRtProbe(NativeRtProbeKey(group,selected.model),constants,selected.model);   // I-0129 R9a
        // I-0113 tranche 12 (gap 12, Flame Fan's fan is the model particle s08280): with ffx-record.flag, a model particle's instance
        // words +0x30, +0x70 (its colour), +0xC0 and +0x100, as reference_e03_instance (24 rows a model).
        if(static const bool e03_witness=ModuleFlag(L"ffx-record.flag");e03_witness && selected.effect && !native_offscreen_preparing){
            static std::mutex lock;static std::map<std::string,int> shown;const auto name=NativeModelDataName(selected.model);bool show{};
            {const std::lock_guard held(lock);show=shown.size()<64 && shown[name]++<24;}
            if(show){float w[0x44]{};std::memcpy(w,constants.bytes.data(),sizeof w);std::string clean;for(const auto ch:name)clean.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');char row[400]{};
                sprintf_s(row,"{\"event\":\"native_e03_instance\",\"model\":\"%.60s\",\"w30\":[%g,%g,%g,%g],\"w70\":[%g,%g,%g,%g],\"wC0\":[%g,%g,%g,%g],\"w100\":[%g]",clean.c_str(),
                    w[12],w[13],w[14],w[15],w[28],w[29],w[30],w[31],w[48],w[49],w[50],w[51],w[64]);
                // I-0113 tranche 16 (the crystals' hue): the whole block, as reference_e03_instance's « all ».
                std::string out(row);out+=",\"all\":[";for(int k=0;k<0x44;++k){char cell[24]{};   // a non-finite word as null, as the reference's
                    if(std::isfinite(w[k]))sprintf_s(cell,"%s%g",k?",":"",w[k]);else sprintf_s(cell,"%snull",k?",":"");out+=cell;}out+="]";
                // The light and fog keys of its group (+0xF8/+0x100 with +0x108, +0x110/+0x118 with +0x120: ProduceNativeForwardLight's), as reference_e03_instance's.
                uint32_t keys[8]{};float light_t{},fog_t{};Read(selected.group+0xF8,keys,16);Read(selected.group+0x108,&light_t,4);Read(selected.group+0x110,keys+4,16);Read(selected.group+0x120,&fog_t,4);
                char cell[200]{};sprintf_s(cell,",\"light\":[%u,%u,%u,%u],\"light_t\":%g,\"fog\":[%u,%u,%u,%u],\"fog_t\":%g}\n",keys[0],keys[1],keys[2],keys[3],light_t,keys[4],keys[5],keys[6],keys[7],fog_t);
                out+=cell;Log(out.c_str());}}
        // Optimisation review (the m33 lake trunk): the rows written (+0x80..+0xAF), as reference_instance_rows, light-observe.flag.
        if(static const bool rows_witness=ModuleFlag(L"light-observe.flag");rows_witness && !native_offscreen_preparing){float rows[16]{};std::memcpy(rows,constants.bytes.data()+0x80,64);
            static std::mutex lock;static std::set<std::tuple<std::string,int,int,int,int>> seen;const auto name=NativeModelDataName(selected.model);bool fresh{};
            const int owned=input.lighting && input.lighting->animation_owned?1:0;
            {const std::lock_guard held(lock);fresh=seen.size()<800 && seen.insert({name,int(variant),owned,int(std::lround(rows[3]*1000.f)),int(std::lround(rows[1]*1000.f))}).second;}
            if(fresh){std::string clean;for(const auto ch:name)clean.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');char row[600]{};
                sprintf_s(row,"{\"event\":\"native_instance_rows\",\"model\":\"%.60s\",\"variant\":%d,\"owned\":%d,\"lit\":%s,\"rows\":[%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g],\"blend\":[%g,%g,%g,%g]}\n",clean.c_str(),int(variant),owned,input.lighting?"true":"false",
                    rows[0],rows[1],rows[2],rows[3],rows[4],rows[5],rows[6],rows[7],rows[8],rows[9],rows[10],rows[11],rows[12],rows[13],rows[14],rows[15]);Log(row);}}
        // D-0180 C4 (image regression): envData (+0x60), the light and fog set index the G-buffer writes to GBuffer3.w and
        // the deferred sun and composite index cbLight/cbFog with; 5B3D0 writes it for pass state types 6/7 (the G-buffer
        // rows) from 137EF0's entry of the group's key block (Reference1736: m40's map draws use sets 0 to 3). The record is
        // shared by the model's passes: no Fwd, Dpt or Velo programme reads envData (scan of the 7,779 cached programmes).
        if(light_set){std::memcpy(constants.bytes.data()+0x60,&*light_set,4);constants.written[0x60/4]=1;}
        // Words 5B3D0 never writes (+54..+5C, +60 when no set index was given,
        // +68/+6C, +104..+10C) hold the instance array's creation zero, the
        // old host's rule for unrewritten fields.
        for(const uint32_t offset:{0x54u,0x58u,0x5Cu,0x60u,0x68u,0x6Cu,0x104u,0x108u,0x10Cu})
            if(!constants.written[offset/4]){std::memset(constants.bytes.data()+offset,0,4);constants.written[offset/4]=1;}
        // 1064: matricesData.x (+50) is the palette base the skinned VS adds to its
        // BLENDINDICES; the native skin palette is bound from row 0 (palette_base 0).
        if(!constants.written[0x50/4]){const uint32_t base=input.palette_base;std::memcpy(constants.bytes.data()+0x50,&base,4);constants.written[0x50/4]=1;}
        // 1065: 5B3D0 writes aSh/ShLightMask (+80..+BF) only for draw types 0, 3 and 4
        // (and its other rows by type); words it leaves keep the creation zero.
        for(uint32_t offset=0x80;offset<0x110;offset+=4)
            if(!constants.written[offset/4]){std::memset(constants.bytes.data()+offset,0,4);constants.written[offset/4]=1;}
        // 1142 witness: the instance SH rows (aSh +80..+AF) and their inputs, for the hand/skin lighting gap.
        {static const bool sh_witness=ModuleFlag(L"light-observe.flag");static LONG64 sh_samples{};const auto n=sh_witness?InterlockedIncrement64(&sh_samples):0;   // I-0120 B3: only with light-observe.flag
            if(sh_witness && (n<=40 || n%3000==0)){float sh[12]{};std::memcpy(sh,constants.bytes.data()+0x80,48);char row[512]{};
                sprintf_s(row,"{\"event\":\"native_instance_sh\",\"sample\":%lld,\"model\":%llu,\"variant\":%u,\"lighting\":%d,\"banks\":%d,\"owned\":%d,\"blend\":%g,\"strength\":[%g,%g],\"ash\":[%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g]}\n",
                    n,static_cast<unsigned long long>(selected.model),static_cast<unsigned>(variant),lighting?1:0,lighting?int(lighting->banks_present):-1,lighting?int(lighting->animation_owned):-1,
                    lighting?lighting->blend:0.f,lighting?lighting->bank_strength[0]:0.f,lighting?lighting->bank_strength[1]:0.f,
                    sh[0],sh[1],sh[2],sh[3],sh[4],sh[5],sh[6],sh[7],sh[8],sh[9],sh[10],sh[11]);Log(row);
                // 1AC840's inputs on p = animation+F0: +70/+71 flags, +74/+78 clocks, +88 probe source, +90 thirteen
                // quantized shorts, +B0 bank key; and the bank rows +290 of both group keys.
                if(n<=40){const auto p=selected.animation+0xF0;BYTE flags[2]{};uintptr_t probe{},key[2]{};int16_t packed[13]{};float clocks[2]{};
                    uintptr_t root{},table{};float bank_rows[2][4]{};
                    Read(p+0x70,flags,2);Read(p+0x74,clocks,8);Read(p+0x88,&probe,8);Read(p+0x90,packed,26);Read(p+0xB0,key,16);
                    if(Read(image+0x4961B50,&root,8) && root && Read(root+0x41E0,&table,8) && table){
                        const auto lookup=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uintptr_t)>(image+0x32A80);
                        for(unsigned b=0;b<2;++b){const auto bank=lookup(table,group+(b?0x100:0xF8));if(bank)Read(bank+0x290,bank_rows[b],16);}}
                    char probe_row[640]{};sprintf_s(probe_row,"{\"event\":\"native_instance_probe_inputs\",\"sample\":%lld,\"animation\":%llu,\"flags70_71\":[%u,%u],\"clocks\":[%g,%g],\"probe88\":%llu,\"packed90\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d],\"keyB0\":[%llu,%llu],\"bank0_290\":[%g,%g,%g,%g],\"bank1_290\":[%g,%g,%g,%g]}\n",
                        n,static_cast<unsigned long long>(selected.animation),flags[0],flags[1],clocks[0],clocks[1],static_cast<unsigned long long>(probe),
                        packed[0],packed[1],packed[2],packed[3],packed[4],packed[5],packed[6],packed[7],packed[8],packed[9],packed[10],packed[11],packed[12],
                        static_cast<unsigned long long>(key[0]),static_cast<unsigned long long>(key[1]),
                        bank_rows[0][0],bank_rows[0][1],bank_rows[0][2],bank_rows[0][3],bank_rows[1][0],bank_rows[1][1],bank_rows[1][2],bank_rows[1][3]);Log(probe_row);}}}
    }
    return true;
}
std::wstring ReadFlverWide(uintptr_t address);
const std::vector<ds3content::ModelBuiltinParameterDeclaration>& NativeBuiltinModelParameters() {
    // Pinned CPU65-row definitions, owned once. No old parameter factory,
    // runtime hash-cache initialization, renderer object or GPU default.
    static const auto definitions=[](){
        std::vector<ds3content::ModelBuiltinParameterDeclaration> result;
        for(uint32_t i=0;i<65;++i){const auto at=image+0x3D20580+i*0x40;
            uintptr_t name{};std::array<uint32_t,4> fields{};BYTE transform{};
            if(!Read(at,&name,8) || !name || !Read(at+8,fields.data(),sizeof(fields)) || !Read(at+0x3C,&transform,1))
                Refuse("native_builtin_material_definition_unreadable");
            result.push_back({ReadFlverWide(name),fields[0],fields[1],fields[2],fields[3],transform!=0});
        }
        return result;
    }();
    return definitions;
}
const std::vector<ds3content::ModelTextureDeclaration>& NativeBuiltinModelTextures() {
    //83340's two exact-name CPU tables, independently pinned in source762.
    static const auto definitions=[](){
        std::vector<ds3content::ModelTextureDeclaration> result;
        for(const auto [rva,count]:{std::pair<uintptr_t,uint32_t>{0x473CBB0,35},{0x473CB78,3}})
            for(uint32_t i=0;i<count;++i){const auto at=image+rva+i*16;
                uintptr_t name{};std::array<uint32_t,2> fields{};
                if(!Read(at,&name,8) || !name || !Read(at+8,fields.data(),sizeof(fields)) || fields[0]>=128)
                    Refuse("native_builtin_texture_definition_unreadable");
                result.push_back({ReadFlverWide(name),fields[0],fields[1],
                    static_cast<uint8_t>(fields[1]==11?4:16),-1});}
        return result;
    }();return definitions;
}
}  // namespace
