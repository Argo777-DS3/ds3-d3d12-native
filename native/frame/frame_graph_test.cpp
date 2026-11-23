// Phase E E1 control (D3D12-PHASE-E-FRAME-GRAPH-SHEET 8.1): T-E6, the retired I-0038 frame plan control ported unchanged in
// its checks (frame_plan_test.cpp of 7340c5f2^); T-E1, the state tracker and barrier planner; T-E3, the placement planner.
#include "../renderer/frame_graph.h"
#include "../renderer/frame_graph_shadow.h"

#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <vector>

using namespace ds3native;

namespace {
unsigned failures{};
void Check(bool ok,const char* message) {
    if(ok)return;
    ++failures;std::fprintf(stderr,"FAIL %s\n",message);
}

// The retired I-0038 material-colour plan builder, kept with its only caller (production builds its plans in frame_graph_shadow.cpp).
FramePlan MaterialColorFramePlan(uint64_t serial,uint64_t content_generation,uint64_t target_generation,uint32_t texture_count=9,bool scene_resolve=false,bool native_lighting=false,bool final_display=false,bool native_mask=false,uint32_t environment_regions=0) {
    FramePlan plan;plan.serial=serial;
    if(texture_count>64 || environment_regions>64 || ((native_lighting || final_display) && !scene_resolve) || ((native_mask || environment_regions) && !native_lighting))return plan;
    const ResourceKey geometry{1,content_generation};
    plan.resources.push_back({geometry,ResourceKind::Buffer,true});
    const ResourceKey constants{11,serial};
    plan.resources.push_back({constants,ResourceKind::Buffer,true});
    for(uint64_t t=0;t<texture_count;++t)plan.resources.push_back({{100+t,content_generation},ResourceKind::Texture,true});
    for(uint64_t c=0;c<6;++c)plan.resources.push_back({{20+c,target_generation},ResourceKind::Color,false});
    const ResourceKey depth{26,target_generation},output{27,target_generation},readback{28,target_generation};
    plan.resources.push_back({depth,ResourceKind::Depth,false});
    plan.resources.push_back({output,ResourceKind::Presentation,false});
    plan.resources.push_back({readback,ResourceKind::Buffer,false});
    FramePass clear{1,PassKind::Clear,{},{}},material{2,PassKind::Material,{1},{}};
    for(uint64_t c=0;c<6;++c) {
        clear.resources.push_back({{20+c,target_generation},ResourceAccess::Write});
        material.resources.push_back({{20+c,target_generation},ResourceAccess::ReadWrite});
    }
    clear.resources.push_back({depth,ResourceAccess::Write});
    material.resources.push_back({depth,ResourceAccess::ReadWrite});
    material.resources.push_back({geometry,ResourceAccess::Read});
    material.resources.push_back({constants,ResourceAccess::Read});
    for(uint64_t t=0;t<texture_count;++t)material.resources.push_back({{100+t,content_generation},ResourceAccess::Read});
    plan.passes.push_back(std::move(clear));plan.passes.push_back(std::move(material));
    uint32_t display_pass=3;
    ResourceKey display_source{22,target_generation};
    if(scene_resolve) {
        const ResourceKey quad{29,content_generation},parameters{30,serial},resolved{32,target_generation};
        plan.resources.push_back({quad,ResourceKind::Buffer,true});
        plan.resources.push_back({parameters,ResourceKind::Buffer,true});
        const ResourceKey lighting_irradiance{31,native_lighting?target_generation:content_generation};
        plan.resources.push_back({lighting_irradiance,native_lighting?ResourceKind::Color:ResourceKind::Texture,!native_lighting});
        plan.resources.push_back({resolved,ResourceKind::Color,false});
        uint32_t resolve_pass=3;
        if(native_lighting) {
            const ResourceKey specular_output{33,target_generation},specular_input{34,environment_regions?target_generation:content_generation},shadow{35,native_mask?target_generation:content_generation};
            plan.resources.push_back({specular_output,ResourceKind::Color,false});
            plan.resources.push_back({specular_input,environment_regions?ResourceKind::Color:ResourceKind::Texture,!environment_regions});
            plan.resources.push_back({shadow,native_mask?ResourceKind::Color:ResourceKind::Texture,!native_mask});
            uint32_t lighting_pass=3;
            if(native_mask) {
                const ResourceKey ssao{38,content_generation},mask_parameters{39,serial};
                plan.resources.push_back({ssao,ResourceKind::Texture,true});
                plan.resources.push_back({mask_parameters,ResourceKind::Buffer,true});
                plan.passes.push_back({3,PassKind::Shadow,{2},{{{21,target_generation},ResourceAccess::Read},
                    {depth,ResourceAccess::Read},{quad,ResourceAccess::Read},{parameters,ResourceAccess::Read},
                    {ssao,ResourceAccess::Read},{mask_parameters,ResourceAccess::Read},{shadow,ResourceAccess::Write}}});
                lighting_pass=4;
            }
            if(environment_regions) {
                const ResourceKey volume{42,content_generation},scene{43,serial},regions{44,serial};
                plan.resources.push_back({volume,ResourceKind::Buffer,true});
                plan.resources.push_back({scene,ResourceKind::Buffer,true});
                plan.resources.push_back({regions,ResourceKind::Buffer,true});
                FramePass environment{lighting_pass,PassKind::Environment,{lighting_pass-1},{{{20,target_generation},ResourceAccess::Read},
                    {{21,target_generation},ResourceAccess::Read},{{23,target_generation},ResourceAccess::Read},
                    {depth,ResourceAccess::Read},{volume,ResourceAccess::Read},{scene,ResourceAccess::Read},
                    {regions,ResourceAccess::Read},{specular_input,ResourceAccess::Write}}};
                for(uint64_t region=0;region<environment_regions;++region) {
                    const ResourceKey cube{200+region,content_generation};plan.resources.push_back({cube,ResourceKind::Texture,true});
                    environment.resources.push_back({cube,ResourceAccess::Read});
                }
                plan.passes.push_back(std::move(environment));++lighting_pass;
            }
            plan.passes.push_back({lighting_pass,PassKind::Lighting,{lighting_pass-1},{{{24,target_generation},ResourceAccess::Read},
                {quad,ResourceAccess::Read},{parameters,ResourceAccess::Read},{specular_input,ResourceAccess::Read},
                {shadow,ResourceAccess::Read},{lighting_irradiance,ResourceAccess::Write},{specular_output,ResourceAccess::Write}}});
            resolve_pass=lighting_pass+1;
        }
        plan.passes.push_back({resolve_pass,PassKind::SceneResolve,{resolve_pass-1},{{display_source,ResourceAccess::Read},
            {quad,ResourceAccess::Read},{parameters,ResourceAccess::Read},{lighting_irradiance,ResourceAccess::Read},{resolved,ResourceAccess::Write}}});
        display_pass=resolve_pass+1;display_source=resolved;
    }
    const ResourceKey encoded{36,target_generation},lut{37,content_generation};
    if(final_display) {
        plan.resources.push_back({encoded,ResourceKind::Color,false});
        plan.resources.push_back({lut,ResourceKind::Texture,true});
    }
    plan.passes.push_back({display_pass,PassKind::Composition,{display_pass-1},{{display_source,ResourceAccess::Read},{final_display?encoded:output,ResourceAccess::Write}}});
    if(final_display) {
        ++display_pass;
        plan.passes.push_back({display_pass,PassKind::FinalDisplay,{display_pass-1},{{encoded,ResourceAccess::Read},{lut,ResourceAccess::Read},{output,ResourceAccess::Write}}});
    }
    FramePass readback_pass{display_pass+1,PassKind::Readback,{display_pass},{{output,ResourceAccess::Read},{readback,ResourceAccess::Write}}};
    if(native_mask) {
        const ResourceKey gbuffer_copy{40,target_generation},mask_copy{41,target_generation};
        plan.resources.push_back({gbuffer_copy,ResourceKind::Buffer,false});
        plan.resources.push_back({mask_copy,ResourceKind::Buffer,false});
        readback_pass.resources.push_back({{21,target_generation},ResourceAccess::Read});
        readback_pass.resources.push_back({{35,target_generation},ResourceAccess::Read});
        readback_pass.resources.push_back({gbuffer_copy,ResourceAccess::Write});
        readback_pass.resources.push_back({mask_copy,ResourceAccess::Write});
    }
    if(environment_regions) {
        for(uint64_t i=0;i<2;++i) {
            const ResourceKey copy{45+i,target_generation};plan.resources.push_back({copy,ResourceKind::Buffer,false});
            readback_pass.resources.push_back({{i?33u:34u,target_generation},ResourceAccess::Read});
            readback_pass.resources.push_back({copy,ResourceAccess::Write});
        }
    }
    plan.passes.push_back(std::move(readback_pass));
    plan.passes.push_back({display_pass+2,PassKind::Present,{display_pass+1},{{output,ResourceAccess::Read}}});
    return plan;
}

FramePlan RawPlan(bool ordered) {
    FramePlan p{};p.serial=12;p.resources={{{1,4},ResourceKind::Buffer,false},{{2,8},ResourceKind::Presentation,false}};
    p.passes={{1,PassKind::Upload,{},{{{1,4},ResourceAccess::Write}}},
        {2,PassKind::Material,ordered?std::vector<uint32_t>{1}:std::vector<uint32_t>{},{{{1,4},ResourceAccess::Read}}},
        {3,PassKind::Composition,{1,2},{{{2,8},ResourceAccess::Write}}},
        {4,PassKind::Present,{3},{{{2,8},ResourceAccess::Read}}}};
    return p;
}
FramePlan WawPlan(bool ordered) {
    FramePlan p{};p.serial=13;p.resources={{{1,4},ResourceKind::Buffer,false},{{2,8},ResourceKind::Presentation,false}};
    p.passes={{1,PassKind::Upload,{},{{{1,4},ResourceAccess::Write}}},
        {2,PassKind::Effect,ordered?std::vector<uint32_t>{1}:std::vector<uint32_t>{},{{{1,4},ResourceAccess::Write}}},
        {3,PassKind::Composition,{1,2},{{{2,8},ResourceAccess::Write}}},
        {4,PassKind::Present,{3},{{{2,8},ResourceAccess::Read}}}};
    return p;
}
FramePlan WarPlan(bool ordered) {
    FramePlan p{};p.serial=14;p.resources={{{1,4},ResourceKind::Buffer,true},{{2,8},ResourceKind::Presentation,false}};
    p.passes={{1,PassKind::Material,{},{{{1,4},ResourceAccess::Read}}},
        {2,PassKind::Effect,{},{{{1,4},ResourceAccess::Read}}},
        {3,PassKind::Upload,ordered?std::vector<uint32_t>{1,2}:std::vector<uint32_t>{1},{{{1,4},ResourceAccess::Write}}},
        {4,PassKind::Composition,{1,2,3},{{{2,8},ResourceAccess::Write}}},
        {5,PassKind::Present,{4},{{{2,8},ResourceAccess::Read}}}};
    return p;
}

void TestDefaultAndMaterialPlan() {
    CompiledFramePlan empty{};
    Check(!static_cast<bool>(empty) && empty.Status().error==PlanError::Extent,
        "default compiled plan is invalid until compilation succeeds");
    auto input=MaterialColorFramePlan(41,7,9);
    auto compiled=CompileFramePlan(input);
    Check(static_cast<bool>(compiled) && compiled.Status().error==PlanError::None,
        "normal material-color plan compiles");
    Check(compiled.Plan().serial==41 && compiled.Plan().passes.size()==5 &&
        compiled.Plan().passes[0].kind==PassKind::Clear && compiled.Plan().passes[1].kind==PassKind::Material &&
        compiled.Plan().passes[2].kind==PassKind::Composition && compiled.Plan().passes[3].kind==PassKind::Readback &&
        compiled.Plan().passes[4].kind==PassKind::Present,"material-color plan retains its five ordered semantic passes");
    input.serial=99;input.passes[0].identity=99;input.resources[0].key.generation=999;
    Check(compiled.Plan().serial==41 && compiled.Plan().passes[0].identity==1 &&
        compiled.Plan().resources[0].key.generation==7,"compiled plan owns an immutable copy of source plan");
    auto stale=MaterialColorFramePlan(2,7,9);
    stale.passes[0].resources[0].key.generation=10;
    Check(CompileFramePlan(stale).Status().error==PlanError::ResourceVersion,
        "stale target/resource generation is rejected");
    Check(CompileFramePlan(MaterialColorFramePlan(0,7,9)).Status().error==PlanError::Extent,
        "serial zero is rejected");
    for(const auto textures:{0u,6u,9u,64u}) {
        const auto variable=CompileFramePlan(MaterialColorFramePlan(7,8,9,textures));
        Check(static_cast<bool>(variable) && variable.Plan().resources.size()==static_cast<size_t>(textures)+11,
            "variable texture set retains disjoint geometry/constants/targets and pass dependencies");
    }
    Check(!CompileFramePlan(MaterialColorFramePlan(7,8,9,65)),"texture extent beyond native capacity refused");
    auto resolve=MaterialColorFramePlan(8,8,9,6,true);
    const auto resolved=CompileFramePlan(resolve);
    Check(resolved && resolved.Plan().passes.size()==6 && resolved.Plan().passes[2].kind==PassKind::SceneResolve &&
        resolved.Plan().passes[3].kind==PassKind::Composition && resolved.Plan().resources.size()==21,
        "genuine scene resolve consumes native material output before display and presentation");
    resolve.passes[2].after.clear();
    Check(CompileFramePlan(resolve).Status().error==PlanError::MissingDependency,"scene resolve cannot read native GBuffer before producer");
    resolve=MaterialColorFramePlan(8,8,9,6,true);resolve.passes[3].after={2};
    Check(CompileFramePlan(resolve).Status().error==PlanError::MissingDependency,"display cannot bypass scene resolve dependency");
    resolve=MaterialColorFramePlan(8,8,9,6,true);
    for(auto& resource:resolve.resources)if(resource.key.identity==31)resource.initialized=false;
    Check(CompileFramePlan(resolve).Status().error==PlanError::UninitializedRead,"unproduced irradiance cannot be consumed by scene resolve");
    auto lighting=MaterialColorFramePlan(8,8,9,6,true,true);
    const auto lit=CompileFramePlan(lighting);
    Check(lit && lit.Plan().passes.size()==7 && lit.Plan().passes[2].kind==PassKind::Lighting &&
        lit.Plan().passes[3].kind==PassKind::SceneResolve && lit.Plan().resources.size()==24,
        "native ambient lighting production precedes scene color resolve");
    Check(!CompileFramePlan(MaterialColorFramePlan(8,8,9,6,false,true)),"lighting without a scene consumer contract is refused");
    lighting.passes[3].after={2};
    Check(CompileFramePlan(lighting).Status().error==PlanError::MissingDependency,"scene resolve cannot bypass native lighting producer");
    lighting=MaterialColorFramePlan(8,8,9,6,true,true);
    for(auto& resource:lighting.resources)if(resource.key.identity==35)resource.initialized=false;
    Check(CompileFramePlan(lighting).Status().error==PlanError::UninitializedRead,"missing shadow input cannot become a lighting success");
    auto display=MaterialColorFramePlan(8,8,9,6,true,true,true);
    const auto displayed=CompileFramePlan(display);
    Check(displayed && displayed.Plan().passes.size()==8 && displayed.Plan().passes[5].kind==PassKind::FinalDisplay,
        "genuine RGB table pass reads owned encoded source and precedes readback/presentation");
    display.passes[5].after={4};
    Check(CompileFramePlan(display).Status().error==PlanError::MissingDependency,"final display cannot bypass encoded source producer");
    display=MaterialColorFramePlan(8,8,9,6,true,true,true);
    for(auto& resource:display.resources)if(resource.key.identity==37)resource.initialized=false;
    Check(CompileFramePlan(display).Status().error==PlanError::UninitializedRead,"uninitialized RGB table is refused");
    Check(!CompileFramePlan(MaterialColorFramePlan(8,8,9,6,false,false,true)),"final display without owned scene composition is refused");
    auto mask=MaterialColorFramePlan(8,8,9,6,true,true,true,true);
    const auto masked=CompileFramePlan(mask);
    Check(masked && masked.Plan().passes.size()==9 && masked.Plan().passes[2].kind==PassKind::Shadow && masked.Plan().passes[3].kind==PassKind::Lighting,
        "native GBuffer/stencil mask producer precedes lighting and final display");
    mask.passes[3].after={2};
    Check(CompileFramePlan(mask).Status().error==PlanError::MissingDependency,"lighting cannot bypass native shadow producer");
    mask=MaterialColorFramePlan(8,8,9,6,true,true,true,true);
    for(auto& resource:mask.resources)if(resource.key.identity==38)resource.initialized=false;
    Check(CompileFramePlan(mask).Status().error==PlanError::UninitializedRead,"unproduced SSAO prevents shadow execution");
    Check(!CompileFramePlan(MaterialColorFramePlan(8,8,9,6,true,false,true,true)),"mask without its lighting consumer is refused");
    auto environment=MaterialColorFramePlan(8,8,9,6,true,true,true,true,1);
    Check(static_cast<bool>(CompileFramePlan(environment)),"native environment owns specular producer before lighting");
    for(auto& resource:environment.resources)if(resource.key.identity==200)resource.initialized=false;
    Check(CompileFramePlan(environment).Status().error==PlanError::UninitializedRead,"missing CPU cube blocks environment execution");
    environment=MaterialColorFramePlan(8,8,9,6,true,true,true,true,1);environment.passes[4].after={3};
    Check(CompileFramePlan(environment).Status().error==PlanError::MissingDependency,"lighting cannot bypass owned environment output");
    Check(!CompileFramePlan(MaterialColorFramePlan(8,8,9,6,true,false,true,false,1)),"environment without native lighting consumer refused");

    const auto& life=compiled.Lifetimes();
    auto find=[&](uint64_t identity){return std::find_if(life.begin(),life.end(),[&](const auto& item){return item.key.identity==identity;});};
    const auto output=find(27),readback=find(28),depth=find(26);
    Check(output!=life.end() && output->first==2 && output->last==4,
        "presentation output lifetime spans composition through final present");
    Check(readback!=life.end() && readback->first==3 && readback->last==3,
        "readback scratch lifetime is its single readback pass");
    Check(depth!=life.end() && depth->first==0 && depth->last==1,
        "depth lifetime begins at clear and ends at material pass");
}

void TestDependencyHazards() {
    const auto raw=CompileFramePlan(RawPlan(false));
    const auto waw=CompileFramePlan(WawPlan(false));
    const auto war=CompileFramePlan(WarPlan(false));
    Check(raw.Status().error==PlanError::MissingDependency && raw.Status().pass==1 &&
        static_cast<bool>(CompileFramePlan(RawPlan(true))),
        "read-after-write without producer dependency is rejected");
    Check(waw.Status().error==PlanError::MissingDependency && waw.Status().pass==1 &&
        static_cast<bool>(CompileFramePlan(WawPlan(true))),
        "write-after-write without prior writer dependency is rejected");
    Check(war.Status().error==PlanError::MissingDependency && war.Status().pass==2 &&
        static_cast<bool>(CompileFramePlan(WarPlan(true))),
        "write-after-read without reader dependency is rejected");

    FramePlan read_read{};read_read.serial=8;
    read_read.resources={{{1,2},ResourceKind::Texture,true},{{2,3},ResourceKind::Presentation,false}};
    read_read.passes={{1,PassKind::Material,{},{{{1,2},ResourceAccess::Read}}},
        {2,PassKind::Effect,{},{{{1,2},ResourceAccess::Read}}},
        {3,PassKind::Composition,{1,2},{{{2,3},ResourceAccess::Write}}},
        {4,PassKind::Present,{3},{{{2,3},ResourceAccess::Read}}}};
    Check(static_cast<bool>(CompileFramePlan(read_read)),"unordered read-read use is legal");

    FramePlan rewrite{};rewrite.serial=9;
    rewrite.resources={{{1,1},ResourceKind::Buffer,false},{{2,1},ResourceKind::Presentation,false}};
    rewrite.passes={{1,PassKind::Upload,{},{{{1,1},ResourceAccess::Write}}},
        {2,PassKind::Effect,{1},{{{1,1},ResourceAccess::Write}}},
        {3,PassKind::Composition,{2},{{{2,1},ResourceAccess::Write}}},
        {4,PassKind::Present,{3},{{{2,1},ResourceAccess::Read}}}};
    Check(static_cast<bool>(CompileFramePlan(rewrite)),"explicitly ordered resource rewrite is legal");

    FramePlan uninitialized{};uninitialized.serial=10;
    uninitialized.resources={{{1,1},ResourceKind::Texture,false},{{2,1},ResourceKind::Presentation,false}};
    uninitialized.passes={{1,PassKind::Material,{},{{{1,1},ResourceAccess::Read}}},
        {2,PassKind::Composition,{1},{{{2,1},ResourceAccess::Write}}},
        {3,PassKind::Present,{2},{{{2,1},ResourceAccess::Read}}}};
    Check(CompileFramePlan(uninitialized).Status().error==PlanError::UninitializedRead,
        "first read of uninitialized resource is rejected");
}

FramePlan WiderSyntheticPlan() {
    FramePlan p{};p.serial=50;
    p.resources={{{1,1},ResourceKind::Buffer,true},{{2,1},ResourceKind::Depth,false},
        {{3,1},ResourceKind::Color,false},{{4,1},ResourceKind::Color,false},
        {{5,1},ResourceKind::Color,false},{{6,1},ResourceKind::Color,false},
        {{7,1},ResourceKind::Presentation,false}};
    p.passes={
        {1,PassKind::Shadow,{},{{{1,1},ResourceAccess::Read},{{2,1},ResourceAccess::Write}}},
        {2,PassKind::Material,{1},{{{1,1},ResourceAccess::Read},{{2,1},ResourceAccess::Read},{{3,1},ResourceAccess::Write}}},
        {3,PassKind::Effect,{2},{{{3,1},ResourceAccess::Read},{{4,1},ResourceAccess::Write}}},
        {4,PassKind::Ui,{2},{{{5,1},ResourceAccess::Write}}},
        {5,PassKind::Composition,{3,4},{{{4,1},ResourceAccess::Read},{{5,1},ResourceAccess::Read},
            {{6,1},ResourceAccess::Write},{{7,1},ResourceAccess::Write}}},
        {6,PassKind::Present,{5},{{{7,1},ResourceAccess::Read}}}
    };
    return p;
}

void TestSyntheticFamiliesAndTopologyRefusals() {
    auto wider=WiderSyntheticPlan();
    const auto wide=CompileFramePlan(wider);
    Check(static_cast<bool>(wide),"synthetic shadow/material/effect/UI/composition/present plan shares dependency and lifetime rules");
    Check(wide.Plan().passes.size()==6 && wide.Lifetimes().size()==7,
        "synthetic family plan owns its pass list and records every used resource lifetime");

    FramePlan missing{};missing.serial=1;
    missing.resources={{{1,1},ResourceKind::Buffer,true}};
    missing.passes={{1,PassKind::Upload,{},{{{1,1},ResourceAccess::Write}}}};
    Check(CompileFramePlan(missing).Status().error==PlanError::Presentation,"missing final presentation is refused");
    auto duplicate=MaterialColorFramePlan(3,1,1);duplicate.passes.push_back({6,PassKind::Present,{5},{{{27,1},ResourceAccess::Read}}});
    Check(CompileFramePlan(duplicate).Status().error==PlanError::Presentation,
        "presentation must be unique and final");
    FramePlan disconnected{};disconnected.serial=4;
    disconnected.resources={{{1,1},ResourceKind::Texture,true},{{2,1},ResourceKind::Presentation,false}};
    disconnected.passes={{1,PassKind::Material,{},{{{1,1},ResourceAccess::Read}}},
        {2,PassKind::Composition,{},{{{2,1},ResourceAccess::Write}}},
        {3,PassKind::Present,{2},{{{2,1},ResourceAccess::Read}}}};
    Check(CompileFramePlan(disconnected).Status().error==PlanError::Presentation,
        "final presentation must depend on disconnected work too");

    auto future=MaterialColorFramePlan(5,1,1);future.passes[1].after={99};
    Check(CompileFramePlan(future).Status().error==PlanError::Dependency,
        "missing or future dependency identity is refused");
    auto self=MaterialColorFramePlan(6,1,1);self.passes[1].after={2};
    Check(CompileFramePlan(self).Status().error==PlanError::Dependency,
        "self-cycle dependency is refused before a cyclic plan can compile");
}

// ---- T-E1: subresource uses, state tracker and barrier planner ----
constexpr auto RT=D3D12_RESOURCE_STATE_RENDER_TARGET,SRV=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
    ANY_SRV=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
    DEPTH_WRITE=D3D12_RESOURCE_STATE_DEPTH_WRITE,DEPTH_SRV=D3D12_RESOURCE_STATE_DEPTH_READ|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
    PRESENT=D3D12_RESOURCE_STATE_PRESENT,UAV=D3D12_RESOURCE_STATE_UNORDERED_ACCESS,COPY_DEST=D3D12_RESOURCE_STATE_COPY_DEST;
PassResource Use(ResourceKey key,ResourceAccess access,D3D12_RESOURCE_STATES state,SubresourceRange range={}){
    return {key,access,range,static_cast<uint32_t>(state),true};}
// G-buffer (color + depth with two planes), a lighting pass reading the color and the depth plane 0 while the stencil plane
// stays writable (two uses of the depth in disjoint ranges), composition into the back buffer, present.
FramePlan DepthPlanesPlan(){
    FramePlan p{};p.serial=70;
    const ResourceKey color{1,1},depth{2,1},lit{3,1},back{4,1};
    p.resources={{color,ResourceKind::Color,false},{depth,ResourceKind::Depth,false},{lit,ResourceKind::Color,false},{back,ResourceKind::Presentation,false}};
    p.passes={{1,PassKind::Clear,{},{Use(color,ResourceAccess::Write,RT),Use(depth,ResourceAccess::Write,DEPTH_WRITE)}},
        {2,PassKind::Material,{1},{Use(color,ResourceAccess::ReadWrite,RT),Use(depth,ResourceAccess::ReadWrite,DEPTH_WRITE)}},
        {3,PassKind::Lighting,{2},{Use(color,ResourceAccess::Read,SRV),Use(depth,ResourceAccess::Read,DEPTH_SRV,{0,1}),
            Use(depth,ResourceAccess::ReadWrite,DEPTH_WRITE,{1,1}),Use(lit,ResourceAccess::Write,RT)}},
        {4,PassKind::Composition,{3},{Use(lit,ResourceAccess::Read,ANY_SRV),Use(back,ResourceAccess::Write,RT)}},
        {5,PassKind::Present,{4},{Use(back,ResourceAccess::Read,PRESENT)}}};
    return p;
}
std::vector<FrameResourceDesc> DepthPlanesDescs(){
    return {{{1,1},FrameLifetime::Transient,FrameHeapClass::Target,1,RT,3<<20,65536},{{2,1},FrameLifetime::Transient,FrameHeapClass::Target,2,DEPTH_WRITE,8<<20,65536},
        {{3,1},FrameLifetime::Transient,FrameHeapClass::Target,1,RT,4<<20,65536},{{4,1},FrameLifetime::External,FrameHeapClass::Target,1,PRESENT,0,65536}};
}
bool Has(const BarrierBatch& batch,uint64_t identity,uint32_t subresource,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after){
    return std::any_of(batch.transitions.begin(),batch.transitions.end(),[&](const GraphTransition& t){
        return t.key.identity==identity && t.subresource==subresource && t.before==before && t.after==after;});}
void TestBarrierPlanner(){
    const auto compiled=CompileFramePlan(DepthPlanesPlan());
    Check(static_cast<bool>(compiled),"T-E1 two uses of one key in disjoint plane ranges compile");
    {auto overlapping=DepthPlanesPlan();overlapping.passes[2].resources[2].range={0,2};
        Check(CompileFramePlan(overlapping).Status().error==PlanError::DuplicateUse,"T-E1 overlapping ranges of one key are a DuplicateUse");
     auto whole=DepthPlanesPlan();whole.passes[2].resources[1].range={};
        Check(CompileFramePlan(whole).Status().error==PlanError::DuplicateUse,"T-E1 a whole-resource use with another use of the key is a DuplicateUse");
     auto empty=DepthPlanesPlan();empty.passes[2].resources[1].range={0,0};
        Check(CompileFramePlan(empty).Status().error==PlanError::Extent,"T-E1 an empty range is refused");}
    // Review: a read is initialised by earlier writes of its whole range only (the stencil plane written alone does not
    // initialise the depth plane), and writes of the same pass do not count.
    {FramePlan partial{};partial.serial=73;const ResourceKey depth{1,1},back{2,1};
        partial.resources={{depth,ResourceKind::Depth,false},{back,ResourceKind::Presentation,false}};
        partial.passes={{1,PassKind::Clear,{},{Use(depth,ResourceAccess::Write,DEPTH_WRITE,{1,1})}},
            {2,PassKind::Lighting,{1},{Use(depth,ResourceAccess::Read,DEPTH_SRV,{0,1}),Use(back,ResourceAccess::Write,RT)}},
            {3,PassKind::Present,{2},{Use(back,ResourceAccess::Read,PRESENT)}}};
        Check(CompileFramePlan(partial).Status().error==PlanError::UninitializedRead,"T-E1 a read of a plane no earlier pass wrote is refused");
        partial.passes[0].resources[0].range={};
        Check(static_cast<bool>(CompileFramePlan(partial)),"T-E1 a read of a plane an earlier whole write covers compiles");
        partial.passes[0].resources[0].range={0,1};partial.passes[0].resources.push_back(Use(depth,ResourceAccess::Write,DEPTH_WRITE,{1,1}));
        partial.passes[1].resources[0].range={0,2};   // an open-ended read would need a write that reaches the last subresource
        Check(static_cast<bool>(CompileFramePlan(partial)),"T-E1 two plane writes together cover a read of both planes");
        FramePlan same_pass{};same_pass.serial=74;same_pass.resources=partial.resources;
        same_pass.passes={{1,PassKind::Lighting,{},{Use(depth,ResourceAccess::Write,DEPTH_WRITE,{1,1}),Use(depth,ResourceAccess::Read,DEPTH_SRV,{0,1}),Use(back,ResourceAccess::Write,RT)}},
            {2,PassKind::Present,{1},{Use(back,ResourceAccess::Read,PRESENT)}}};
        Check(CompileFramePlan(same_pass).Status().error==PlanError::UninitializedRead,"T-E1 a write of the same pass does not initialise a read, whatever their order");}
    const auto plan=PlanFrameBarriers(compiled,DepthPlanesDescs());
    Check(static_cast<bool>(plan.status),"T-E1 depth-plane plan gives barriers");
    Check(plan.before_pass.size()==5 && plan.before_pass[0].transitions.empty() && plan.before_pass[1].transitions.empty(),
        "T-E1 no redundant transition: the clear and the material find their home states");
    Check(plan.before_pass.size()==5 && plan.before_pass[2].transitions.size()==2 && Has(plan.before_pass[2],1,NoPlanIndex,RT,SRV) && Has(plan.before_pass[2],2,0,DEPTH_WRITE,DEPTH_SRV),
        "T-E1 lighting: the colour to SRV and only plane 0 of the depth to DEPTH_READ|SRV, the stencil plane left writable");
    Check(plan.before_pass.size()==5 && plan.before_pass[3].transitions.size()==2 && Has(plan.before_pass[3],3,NoPlanIndex,RT,ANY_SRV) && Has(plan.before_pass[3],4,NoPlanIndex,PRESENT,RT),
        "T-E1 composition: the combined PIXEL|NON_PIXEL state and the back buffer from its boundary state");
    Check(plan.before_pass.size()==5 && plan.before_pass[4].transitions.size()==1 && Has(plan.before_pass[4],4,NoPlanIndex,RT,PRESENT),"T-E1 present: the back buffer to PRESENT");
    Check(plan.after_pass.size()==5 && plan.after_pass[2].transitions.size()==2 && Has(plan.after_pass[2],1,NoPlanIndex,SRV,RT) &&
        Has(plan.after_pass[2],2,0,DEPTH_SRV,DEPTH_WRITE) && plan.after_pass[3].transitions.size()==1 && Has(plan.after_pass[3],3,NoPlanIndex,ANY_SRV,RT) &&
        plan.after_pass[0].transitions.empty() && plan.after_pass[1].transitions.empty() && plan.after_pass[4].transitions.empty(),
        "T-E1 G2: each transient back home after its last pass (the depth plane 0 alone)");
    Check(plan.end.transitions.empty(),"T-E1 end: the back buffer is already in its boundary state");
    Check(plan.TransitionCount()==8,"T-E1 eight transitions in all");
    const auto depth_first=std::find_if(plan.first_uses.begin(),plan.first_uses.end(),[](const auto& f){return f.key.identity==2;});
    Check(plan.first_uses.size()==3 && depth_first!=plan.first_uses.end() && depth_first->action==FirstUse::Clear && depth_first->pass==0,
        "T-E1 first uses: the three transients, cleared by the clear pass; the external back buffer has none");
    const auto stencil_state=std::find_if(plan.use_states.begin(),plan.use_states.end(),[](const auto& s){return s.pass==2 && s.use==2;});
    Check(stencil_state!=plan.use_states.end() && stencil_state->subresource==1 && stencil_state->state==DEPTH_WRITE,"T-E1 use states: the stencil plane's state at its use");
    // A read requirement contained in a read-only state needs no transition; a write requirement always does.
    Check(GraphStateSatisfies(ANY_SRV,SRV) && !GraphStateSatisfies(SRV,ANY_SRV) && !GraphStateSatisfies(RT,SRV) && GraphStateSatisfies(RT,RT) &&
        !GraphStateSatisfies(DEPTH_SRV,DEPTH_WRITE) && GraphStateSatisfies(PRESENT,PRESENT),"T-E1 state satisfaction rules");
    {auto descs=DepthPlanesDescs();descs.pop_back();
        Check(PlanFrameBarriers(compiled,descs).status.error==GraphError::Description,"T-E1 a resource without a description is refused");
     auto planes=DepthPlanesDescs();planes[1].subresources=1;
        Check(PlanFrameBarriers(compiled,planes).status.error==GraphError::Description,"T-E1 a range past the resource's subresources is refused");
     auto past=DepthPlanesPlan();past.passes[2].resources[2].range={1,2};
        Check(PlanFrameBarriers(CompileFramePlan(past),DepthPlanesDescs()).status.error==GraphError::Description,"T-E1 a range running past the last subresource is refused");
     // G8: invalid required states are refused at plan time.
     const auto refused=[&](D3D12_RESOURCE_STATES state,ResourceAccess access){auto bad=DepthPlanesPlan();bad.passes[3].resources[0]=Use({3,1},access,state);
        return PlanFrameBarriers(CompileFramePlan(bad),DepthPlanesDescs()).status.error==GraphError::UnknownState;};
     Check(refused(RT|SRV,ResourceAccess::Read) && refused(DEPTH_WRITE|SRV,ResourceAccess::Read) && refused(D3D12_RESOURCE_STATE_DEPTH_READ|DEPTH_WRITE,ResourceAccess::Read) &&
        refused(static_cast<D3D12_RESOURCE_STATES>(0x80000000u),ResourceAccess::Read) && refused(SRV,ResourceAccess::ReadWrite) && !refused(ANY_SRV,ResourceAccess::Read),
        "T-E1 G8: write states combined, unknown bits and a write use in a read-only state are refused");}
    Check(PlanFrameBarriers(CompiledFramePlan{},DepthPlanesDescs()).status.error==GraphError::Plan,"T-E1 an uncompiled plan is refused");
    // Copy initialisation and UAV barriers.
    FramePlan copy{};copy.serial=71;const ResourceKey a{1,1},b{2,1},out{3,1};
    copy.resources={{a,ResourceKind::Texture,false},{b,ResourceKind::Texture,false},{out,ResourceKind::Presentation,false}};
    copy.passes={{1,PassKind::Upload,{},{Use(a,ResourceAccess::Write,COPY_DEST)}},
        {2,PassKind::PostProcess,{1},{Use(a,ResourceAccess::Read,ANY_SRV),Use(b,ResourceAccess::Write,UAV)}},
        {3,PassKind::PostProcess,{2},{Use(b,ResourceAccess::ReadWrite,UAV)}},
        {4,PassKind::Composition,{3},{Use(b,ResourceAccess::Read,ANY_SRV),Use(out,ResourceAccess::Write,RT)}},
        {5,PassKind::Present,{4},{Use(out,ResourceAccess::Read,PRESENT)}}};
    const auto copied=PlanFrameBarriers(CompileFramePlan(copy),{{a,FrameLifetime::Transient,FrameHeapClass::Texture,1,ANY_SRV,1<<20,65536},
        {b,FrameLifetime::Transient,FrameHeapClass::Texture,1,UAV,1<<20,65536},{out,FrameLifetime::External,FrameHeapClass::Target,1,PRESENT,0,65536}});
    Check(static_cast<bool>(copied.status) && copied.first_uses.size()==2 && copied.first_uses[0].action==FirstUse::CopyInit && copied.first_uses[1].action==FirstUse::Discard,
        "T-E1 first uses: a copy destination is a copy initialisation, another write a discard");
    Check(static_cast<bool>(copied.status) && copied.before_pass[0].transitions.size()==1 && copied.before_pass[1].uav.empty() && copied.before_pass[2].uav.size()==1 &&
        copied.before_pass[2].transitions.empty(),"T-E1 a UAV use after an earlier UAV write: a UAV barrier, no transition");
    // E5: a persistent resource whose first frame starts in its creation state and ends in its boundary state.
    {FramePlan first{};first.serial=75;const ResourceKey neutral{1,1},back{2,1};
        first.resources={{neutral,ResourceKind::Color,true},{back,ResourceKind::Presentation,false}};
        first.passes={{1,PassKind::Clear,{},{Use(neutral,ResourceAccess::Write,RT)}},{2,PassKind::Lighting,{1},{Use(neutral,ResourceAccess::Read,SRV),Use(back,ResourceAccess::Write,RT)}},
            {3,PassKind::Present,{2},{Use(back,ResourceAccess::Read,PRESENT)}}};
        FrameResourceDesc neutral_desc{neutral,FrameLifetime::Persistent,FrameHeapClass::Target,1,SRV,1,65536};neutral_desc.start_declared=true;neutral_desc.start=RT;
        const auto started=PlanFrameBarriers(CompileFramePlan(first),{neutral_desc,{back,FrameLifetime::External,FrameHeapClass::Target,1,PRESENT,0,65536}});
        Check(static_cast<bool>(started.status) && started.before_pass[0].transitions.empty() && Has(started.before_pass[1],1,NoPlanIndex,RT,SRV) && started.end.transitions.empty(),
            "T-E1 E5: a declared start state is the first state, the boundary state the last (no transition back)");
        FramePlan idle=first;idle.passes.erase(idle.passes.begin());idle.passes[0].after.clear();idle.passes[0].resources.erase(idle.passes[0].resources.begin());idle.serial=76;
        const auto unused=PlanFrameBarriers(CompileFramePlan(idle),{neutral_desc,{back,FrameLifetime::External,FrameHeapClass::Target,1,PRESENT,0,65536}});
        Check(static_cast<bool>(unused.status) && Has(unused.end,1,NoPlanIndex,RT,SRV),"T-E1 E5: an unused resource goes from its start state to its boundary state at the end");}
    // Caps: 256 passes compile, 257 do not; the peak counter follows. 254 effects, a composition after all of them, present.
    FramePlan many{};many.serial=72;many.resources={{{1,1},ResourceKind::Buffer,true},{{2,1},ResourceKind::Presentation,false}};
    std::vector<uint32_t> effects;
    for(uint32_t i=1;i<=254;++i){many.passes.push_back({i,PassKind::Effect,{},{{{1,1},ResourceAccess::Read}}});effects.push_back(i);}
    many.passes.push_back({300,PassKind::Composition,effects,{{{2,1},ResourceAccess::Write}}});
    many.passes.push_back({301,PassKind::Present,{300},{{{2,1},ResourceAccess::Read}}});
    TakePlanPassesPeak();
    Check(many.passes.size()==256 && static_cast<bool>(CompileFramePlan(many)) && TakePlanPassesPeak()==256,"T-E1 256 passes compile and the peak counter reports them");
    auto too_many=many;too_many.passes.insert(too_many.passes.begin(),{400,PassKind::Effect,{},{{{1,1},ResourceAccess::Read}}});
    too_many.passes[255].after.push_back(400);
    Check(CompileFramePlan(too_many).Status().error==PlanError::Extent,"T-E1 257 passes are refused");
    auto dependencies=many;dependencies.passes[254].after.resize(257,1);
    Check(CompileFramePlan(dependencies).Status().error==PlanError::Extent,"T-E1 257 dependencies of one pass are refused");
}

// ---- T-E3: placement ----
struct Rng {uint64_t s;uint64_t Next(){s^=s<<13;s^=s>>7;s^=s<<17;return s;}};
void TestPlacement(){
    for(uint64_t seed=1;seed<=200;++seed){
        Rng rng{seed*0x9E3779B97F4A7C15ull};
        FramePlan p{};p.serial=seed;const uint32_t resources=2+static_cast<uint32_t>(rng.Next()%30),passes=2+static_cast<uint32_t>(rng.Next()%40);
        std::vector<FrameResourceDesc> descs;
        for(uint32_t r=1;r<=resources;++r){p.resources.push_back({{r,1},ResourceKind::Color,false});
            const auto heap=static_cast<FrameHeapClass>(rng.Next()%3);const auto persistent=rng.Next()%7==0;
            descs.push_back({{r,1},persistent?FrameLifetime::Persistent:FrameLifetime::Transient,heap,1,RT,(1+rng.Next()%64)<<16,rng.Next()%5==0?(4ull<<20):65536ull});}
        const ResourceKey back{9999,1};p.resources.push_back({back,ResourceKind::Presentation,false});
        descs.push_back({back,FrameLifetime::External,FrameHeapClass::Target,1,PRESENT,0,65536});
        // Each pass writes one resource and may read one written earlier, in a chain (every pass after the previous one).
        std::vector<bool> written(resources+1);
        for(uint32_t i=1;i<=passes;++i){FramePass pass{i,PassKind::Effect,i>1?std::vector<uint32_t>{i-1}:std::vector<uint32_t>{},{}};
            const uint32_t w=1+static_cast<uint32_t>(rng.Next()%resources);pass.resources.push_back({{w,1},ResourceAccess::Write});
            const uint32_t r=1+static_cast<uint32_t>(rng.Next()%resources);if(r!=w && written[r])pass.resources.push_back({{r,1},ResourceAccess::Read});
            written[w]=true;p.passes.push_back(std::move(pass));}
        p.passes.push_back({passes+1,PassKind::Composition,{passes},{{back,ResourceAccess::Write}}});
        p.passes.push_back({passes+2,PassKind::Present,{passes+1},{{back,ResourceAccess::Read}}});
        const auto compiled=CompileFramePlan(p);
        if(!compiled){Check(false,"T-E3 random plan compiles");continue;}
        for(const bool tier2:{false,true}){
            const auto placement=PlanFramePlacement(compiled,descs,tier2);
            if(!placement.status){Check(false,"T-E3 random placement succeeds");continue;}
            bool aligned=true,disjoint=true,only_transients=true,bounded=true;
            const auto& life=compiled.Lifetimes();
            const auto lifetime=[&](ResourceKey key){return *std::find_if(life.begin(),life.end(),[&](const auto& l){return l.key==key;});};
            const auto desc=[&](ResourceKey key)->const FrameResourceDesc&{return *std::find_if(descs.begin(),descs.end(),[&](const auto& d){return d.key==key;});};
            for(size_t i=0;i<placement.placements.size();++i){
                const auto& a=placement.placements[i];const auto& da=desc(a.key);
                if(da.lifetime!=FrameLifetime::Transient)only_transients=false;
                if(a.offset%da.alignment)aligned=false;
                const auto heap=tier2?size_t{0}:static_cast<size_t>(da.heap_class);
                if(a.offset+da.size>placement.heap_size[heap])bounded=false;
                for(size_t j=i+1;j<placement.placements.size();++j){
                    const auto& b=placement.placements[j];const auto& db=desc(b.key);
                    if((tier2?size_t{0}:static_cast<size_t>(db.heap_class))!=heap)continue;
                    const auto la=lifetime(a.key),lb=lifetime(b.key);
                    const bool time=la.first<=lb.last && lb.first<=la.last,memory=a.offset<b.offset+db.size && b.offset<a.offset+da.size;
                    if(time && memory)disjoint=false;}}
            Check(aligned && disjoint && only_transients && bounded,"T-E3 placements aligned, disjoint in memory when their lifetimes overlap, transients only, within the heap");
            // Bound: alignment padding aside (one largest alignment per heap), a heap is never larger than its resources apart.
            for(size_t h=0;h<3;++h)Check(placement.heap_size[h]<=placement.separate_size[h]+(4ull<<20),"T-E3 a heap is within one alignment of its resources apart");
            const auto again=PlanFramePlacement(compiled,descs,tier2);
            bool same=again.placements.size()==placement.placements.size();
            for(size_t i=0;same && i<again.placements.size();++i)same=again.placements[i].key==placement.placements[i].key && again.placements[i].offset==placement.placements[i].offset;
            Check(same,"T-E3 placement is deterministic");
            if(tier2)Check(!placement.heap_size[1] && !placement.heap_size[2],"T-E3 Tier 2 uses one heap class");
        }
    }
    // A ping-pong chain: the first and third transients (disjoint lifetimes) share storage, the second does not.
    FramePlan p{};p.serial=80;const ResourceKey a{1,1},b{2,1},c{3,1},back{4,1};
    p.resources={{a,ResourceKind::Color,false},{b,ResourceKind::Color,false},{c,ResourceKind::Color,false},{back,ResourceKind::Presentation,false}};
    p.passes={{1,PassKind::Effect,{},{{a,ResourceAccess::Write}}},{2,PassKind::Effect,{1},{{a,ResourceAccess::Read},{b,ResourceAccess::Write}}},
        {3,PassKind::Effect,{2},{{b,ResourceAccess::Read},{c,ResourceAccess::Write}}},{4,PassKind::Composition,{3},{{c,ResourceAccess::Read},{back,ResourceAccess::Write}}},
        {5,PassKind::Present,{4},{{back,ResourceAccess::Read}}}};
    const std::vector<FrameResourceDesc> descs{{a,FrameLifetime::Transient,FrameHeapClass::Target,1,RT,1<<20,65536},{b,FrameLifetime::Transient,FrameHeapClass::Target,1,RT,1<<20,65536},
        {c,FrameLifetime::Transient,FrameHeapClass::Target,1,RT,1<<20,65536},{back,FrameLifetime::External,FrameHeapClass::Target,1,PRESENT,0,65536}};
    const auto placed=PlanFramePlacement(CompileFramePlan(p),descs,false);
    const auto offset=[&](ResourceKey key){const auto f=std::find_if(placed.placements.begin(),placed.placements.end(),[&](const auto& x){return x.key==key;});
        return f==placed.placements.end()?UINT64_MAX:f->offset;};
    Check(static_cast<bool>(placed.status) && placed.placements.size()==3 && offset(a)!=offset(b) && offset(b)!=offset(c) && offset(a)==offset(c) &&
        placed.heap_size[0]==(2ull<<20) && placed.separate_size[0]==(3ull<<20),"T-E3 a ping-pong chain needs two slots: the first and third transients share storage");
    auto bad=descs;bad[0].alignment=3;
    Check(PlanFramePlacement(CompileFramePlan(p),bad,false).status.error==GraphError::Description,"T-E3 a non-power-of-two alignment is refused");
}
// ---- T-E2: the shadow recorder on a synthetic encoder (the hand-written barriers of a G-buffer, a lighting pass reading
// the depth plane 0 with the stencil writable, a composition and a present) ----
ID3D12Resource* Fake(uintptr_t value){return reinterpret_cast<ID3D12Resource*>(value);}
std::vector<std::string> ShadowFrame(int fault){
    GraphShadow shadow;shadow.Begin(90);
    auto *color=Fake(0x1000),*depth=Fake(0x2000),*lit=Fake(0x3000),*back=Fake(0x4000);
    shadow.Declare(color,1,RT,FrameLifetime::Transient,"color");shadow.Declare(depth,2,DEPTH_WRITE,FrameLifetime::Transient,"depth");
    shadow.Declare(lit,1,RT,FrameLifetime::Transient,"lit");shadow.Declare(back,1,PRESENT,FrameLifetime::External,"back");
    shadow.Pass("gbuffer",PassKind::Material);shadow.Use(color,ResourceAccess::Write,RT);shadow.Use(depth,ResourceAccess::Write,DEPTH_WRITE);
    shadow.Transition(color,NoPlanIndex,fault==1?SRV:RT,SRV,1);shadow.Transition(depth,0,DEPTH_WRITE,DEPTH_SRV,2);
    shadow.Pass("lighting",PassKind::Lighting);shadow.Use(color,ResourceAccess::Read,fault==2?ANY_SRV:SRV);shadow.Use(depth,ResourceAccess::Read,DEPTH_SRV,{0,1});
    shadow.Use(depth,ResourceAccess::ReadWrite,DEPTH_WRITE,{1,1});shadow.Use(lit,ResourceAccess::Write,RT);
    shadow.Transition(lit,NoPlanIndex,RT,SRV,3);
    shadow.Transition(color,NoPlanIndex,SRV,RT,4);if(fault!=3)shadow.Transition(depth,0,DEPTH_SRV,DEPTH_WRITE,5);
    shadow.Transition(back,NoPlanIndex,PRESENT,RT,6);
    shadow.Pass("composition",PassKind::Composition);shadow.Use(lit,ResourceAccess::Read,SRV);shadow.Use(back,ResourceAccess::Write,RT);
    shadow.Transition(lit,NoPlanIndex,SRV,RT,7);shadow.Transition(back,NoPlanIndex,RT,PRESENT,8);
    shadow.Pass("present",PassKind::Present);shadow.Use(back,ResourceAccess::Read,PRESENT);
    return shadow.End();
}
bool HasKind(const std::vector<std::string>& rows,const char* kind){
    return std::any_of(rows.begin(),rows.end(),[&](const std::string& row){return row.find(std::string("\"kind\":\"")+kind+"\"")!=std::string::npos;});}
void TestShadow(){
    Check(ShadowFrame(0).empty(),"T-E2 a hand-written frame equal to its plan gives no mismatch");
    Check(HasKind(ShadowFrame(1),"hand_before"),"T-E2 a hand-written transition from a wrong state is a mismatch");
    Check(HasKind(ShadowFrame(2),"use_state"),"T-E2 a use whose hand-written state differs from the graph's is a mismatch");
    Check(HasKind(ShadowFrame(3),"end_state"),"T-E2 a resource not back in its state at the end of the frame is a mismatch");
    GraphShadow idle;idle.Transition(Fake(0x1000),NoPlanIndex,RT,SRV,1);idle.Use(Fake(0x1000),ResourceAccess::Read,SRV);
    Check(idle.End().empty() && !idle.Active(),"T-E2 an inactive shadow records nothing");
}
// ---- T-E3b (optimisation review, axis 3): the executor on a WARP device. The first frame of a key is compared and caches
// its plan; the executed frames are compared in full until eight of them matched, then one in sixty-four (ComparedFrames,
// SampledOutFrames); every frame records the plan's barriers and ends with every resource in its declared state. ----
void TestExecutorSampling(){
    using Microsoft::WRL::ComPtr;
    ComPtr<IDXGIFactory4> factory;if(FAILED(CreateDXGIFactory2(0,IID_PPV_ARGS(&factory)))){std::puts("T-E3b skipped: no DXGI factory");return;}
    ComPtr<IDXGIAdapter> adapter;ComPtr<ID3D12Device> device;
    if(FAILED(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter))) || FAILED(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)))){std::puts("T-E3b skipped: no WARP device");return;}
    ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> list;
    Check(SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator))) &&
          SUCCEEDED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list))),"T-E3b command list");
    const auto texture=[&](D3D12_RESOURCE_STATES state){D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=desc.Height=16;desc.DepthOrArraySize=desc.MipLevels=1;
        desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        ComPtr<ID3D12Resource> r;Check(SUCCEEDED(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&r))),"T-E3b texture");return r;};
    const auto color=texture(RT),lit=texture(RT),back=texture(PRESENT);
    GraphShadow shadow;shadow.SetExecutor(true);
    // The frame as an encoder records it: hand-written transitions only where the executor does not suppress them.
    const auto hand=[&](ID3D12Resource* r,D3D12_RESOURCE_STATES from,D3D12_RESOURCE_STATES to,uint32_t line){if(!shadow.Suppress(r))shadow.Transition(r,NoPlanIndex,from,to,line);};
    size_t mismatch_rows{};
    for(uint64_t serial=1;serial<=100;++serial){
        shadow.Begin(serial,list.Get());
        shadow.Declare(color.Get(),1,RT,FrameLifetime::Transient,"color");shadow.Declare(lit.Get(),1,RT,FrameLifetime::Transient,"lit");
        shadow.Declare(back.Get(),1,PRESENT,FrameLifetime::External,"back",true);
        shadow.Execute(7);
        shadow.Pass("gbuffer",PassKind::Material);shadow.Use(color.Get(),ResourceAccess::Write,RT);
        hand(color.Get(),RT,SRV,1);
        shadow.Pass("lighting",PassKind::Lighting);shadow.Use(color.Get(),ResourceAccess::Read,SRV);shadow.Use(lit.Get(),ResourceAccess::Write,RT);
        hand(lit.Get(),RT,SRV,2);hand(color.Get(),SRV,RT,3);hand(back.Get(),PRESENT,RT,4);
        shadow.Pass("composition",PassKind::Composition);shadow.Use(lit.Get(),ResourceAccess::Read,SRV);shadow.Use(back.Get(),ResourceAccess::Write,RT);
        hand(lit.Get(),SRV,RT,5);hand(back.Get(),RT,PRESENT,6);
        shadow.Pass("present",PassKind::Present);shadow.Use(back.Get(),ResourceAccess::Read,PRESENT);
        const auto rows=shadow.End();mismatch_rows+=rows.size();
        if(serial==1)Check(!shadow.Executing() && shadow.ExecutedFrames()==0,"T-E3b the first frame of a key is compared, not executed");
    }
    Check(mismatch_rows==0,"T-E3b no frame mismatches its plan");
    Check(shadow.ExecutedFrames()==99,"T-E3b every frame after the first is executed by the cached plan");
    Check(shadow.ComparedFrames()==10 && shadow.SampledOutFrames()==90,"T-E3b the first frame, eight executed frames and the 64th are compared; the other executed frames are sampled out");
    if(shadow.GraphBarriers()!=99*6 || shadow.BarrierCalls()!=99*3)std::fprintf(stderr,"T-E3b barriers %llu calls %llu\n",static_cast<unsigned long long>(shadow.GraphBarriers()),static_cast<unsigned long long>(shadow.BarrierCalls()));
    Check(shadow.GraphBarriers()==99*6 && shadow.BarrierCalls()==99*3,"T-E3b each executed frame records the plan's six transitions in three calls (one per pass boundary with barriers)");
    Check(shadow.EvictedPlans()==0,"T-E3b no plan evicted");
    Check(SUCCEEDED(list->Close()),"T-E3b the list closes with the recorded barriers");
}
} // namespace

int main() {
    TestShadow();
    TestDefaultAndMaterialPlan();
    TestDependencyHazards();
    TestSyntheticFamiliesAndTopologyRefusals();
    TestBarrierPlanner();
    TestPlacement();
    TestExecutorSampling();
    if(failures){std::fprintf(stderr,"FAIL frame graph controls: %u assertion(s)\n",failures);return 1;}
    std::puts("PASS frame graph CPU controls: T-E6 the I-0038 frame plan checks (owned copy, material plans, refusals, RAW/WAW/WAR, presentation topology); "
        "T-E1 disjoint plane ranges accepted and overlapping refused, minimal transitions, plane-0 depth read with the stencil writable, combined read states, "
        "reads initialised by earlier writes of their whole range, home state after the last pass (G2), invalid states refused (G8), boundary states, first-use actions, "
        "UAV barriers, caps 256 (passes and dependencies) and the peak; T-E2 shadow recorder: equal frame, wrong hand-written state, wrong use state, wrong end state; T-E3 random placements aligned and disjoint, deterministic, Tier 2, ping-pong sharing; "
        "T-E3b executor on WARP: the first frame compared and cached, eight executed frames compared then one in sixty-four, every executed frame recording the plan's barriers");
    return 0;
}
