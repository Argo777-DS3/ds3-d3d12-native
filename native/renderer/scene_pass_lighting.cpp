// The scene renderer's deferred lighting: its resources, the point and effect lights, the lighting pass, the shadow mask,
// the env-map regions, SSS and the depth atlases (scene_pass_internal.h has the shared helpers).
#include "scene_pass_internal.h"

namespace ds3native {
void SceneRenderer::NativeLightingResources(const NativeLightingInput& input) {
    auto* device=gpu_.device.Get();
    if(native_light_programs_!=input.programs){gpu_audit.lighting_programme_changes.fetch_add(1,std::memory_order_relaxed);
        // cD3 review: the former pipelines and env box are released at once; a list still recording may have used them
        // (complement 4.2: the programmes resolve once per process, from an empty state).
        if(native_light_programs_ && frames_.Recording())throw std::logic_error("native lighting programme change inside an open frame list");
        if(native_light_programs_)frames_.WaitIdle(gpu_,WaitOther);   // cD6 review: frames in flight may still use the former objects
        const auto& programs=*input.programs;
        if(programs.vertex.empty() || programs.ambient.empty() || programs.sun.empty() || programs.albedo.empty() || programs.composite.empty())
            throw std::invalid_argument("native lighting programmes");
        if(!native_light_root_){
            // b0 cbLightAccParam, b4 cbLightAccVtxParam (VS), b5 cbLight, b6 cbFog;
            // s7 SS_ClampPoint, s8 SS_WrapLinear as the programmes declare them; 1054: b1
            // cbLightAccLightParam and t33 g_SSAOTexture of the point-light programmes.
            D3D12_ROOT_PARAMETER parameters[10]{};
            const UINT registers[4]{0,4,5,6};
            for(unsigned i=0;i<4;++i){parameters[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;parameters[i].Descriptor.ShaderRegister=registers[i];
                parameters[i].ShaderVisibility=i==1?D3D12_SHADER_VISIBILITY_VERTEX:D3D12_SHADER_VISIBILITY_PIXEL;}
            const D3D12_DESCRIPTOR_RANGE ranges[3]{{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,9,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,17,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,32,0,0}};
            for(unsigned i=0;i<3;++i){parameters[4+i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                parameters[4+i].DescriptorTable={1,&ranges[i]};parameters[4+i].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;}
            parameters[7].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;parameters[7].Descriptor.ShaderRegister=1;parameters[7].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
            const D3D12_DESCRIPTOR_RANGE ssao{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,33,0,0};
            parameters[8].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[8].DescriptorTable={1,&ssao};parameters[8].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
            D3D12_STATIC_SAMPLER_DESC samplers[2]{};
            for(unsigned i=0;i<2;++i){auto& sampler=samplers[i];sampler.Filter=i?D3D12_FILTER_MIN_MAG_MIP_LINEAR:D3D12_FILTER_MIN_MAG_MIP_POINT;
                sampler.AddressU=sampler.AddressV=sampler.AddressW=i?D3D12_TEXTURE_ADDRESS_MODE_WRAP:D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
                sampler.MaxAnisotropy=1;sampler.ComparisonFunc=D3D12_COMPARISON_FUNC_NEVER;sampler.MaxLOD=FLT_MAX;
                sampler.ShaderRegister=7+i;sampler.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;}
            // D-0180 C4: b8 cbSceneParam for GXLightAcc_DDirLightShadowed1 (FC_ShadowColor).
            parameters[9].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;parameters[9].Descriptor.ShaderRegister=8;parameters[9].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
            // I-0131 R10a: the shadowed lights' b2 (root 10) and t9, t10 (root 11) are removed with their programmes' use.
            D3D12_ROOT_SIGNATURE_DESC desc{10,parameters,2,samplers,D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT};
            ComPtr<ID3DBlob> blob,error;
            Check(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"native lighting root serialization");
            Check(ds3native::Pipelines().Root(device,blob->GetBufferPointer(),blob->GetBufferSize(),native_light_root_.ReleaseAndGetAddressOf()),"native lighting root");   // I-0072: the point lights' pipelines are cached
            // GXLightAcc input: POSITION float4, TEXCOORD float2; one full-screen strip.
            const float quad[24]{-1,1,0,1,0,0, 1,1,0,1,1,0, -1,-1,0,1,0,1, 1,-1,0,1,1,1};
            native_light_quad_=Upload(device,quad,sizeof(quad));native_light_quad_view_={native_light_quad_->GetGPUVirtualAddress(),sizeof(quad),24};
            // gLitTextureFlat is read only for the light-count visualization (off).
            const uint8_t empty[8]{};native_light_list_=Upload(device,empty,sizeof(empty),256);
            // b1 (point-light parameters outside the batches) is a static block of zeros; b0/b2/b3 are written per
            // submission into frame_uploads_ (B2c, EncodeNativeLighting).
            const uint8_t zeros[256]{};native_light_zeros_=Upload(device,zeros,sizeof(zeros));
            for(unsigned i=0;i<2;++i){
                const auto image=Image(1,1,1,DXGI_FORMAT_R8G8B8A8_UNORM,D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
                D3D12_CLEAR_VALUE clear{};clear.Format=image.Format;for(unsigned c=0;c<4;++c)clear.Color[c]=i?0.f:1.f;
                const auto heap=Heap(D3D12_HEAP_TYPE_DEFAULT);
                Check(CountedResource(device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&image,D3D12_RESOURCE_STATE_RENDER_TARGET,&clear,IID_PPV_ARGS(&native_light_neutral_[i])),"native lighting neutral image");
            }
            native_neutral_ready_=false;   // E5: cleared by the next lighting recording, then left shader-readable
            // 1137 GXSssBlurX/Y: b1 cbSSS, b8 cbSceneParam, t1 g_DepthTexture .. t3 g_SSSStrengthTexture,
            // t36 g_LightAccTexture; s7 SS_ClampPoint, s9 SS_ClampLinear.
            D3D12_ROOT_PARAMETER sss[4]{};
            for(unsigned i=0;i<2;++i){sss[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;sss[i].Descriptor.ShaderRegister=i?8:1;sss[i].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;}
            const D3D12_DESCRIPTOR_RANGE sss_ranges[2]{{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,3,1,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,36,0,0}};
            for(unsigned i=0;i<2;++i){sss[2+i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                sss[2+i].DescriptorTable={1,&sss_ranges[i]};sss[2+i].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;}
            D3D12_STATIC_SAMPLER_DESC sss_samplers[2]{};
            for(unsigned i=0;i<2;++i){auto& sampler=sss_samplers[i];sampler.Filter=i?D3D12_FILTER_MIN_MAG_MIP_LINEAR:D3D12_FILTER_MIN_MAG_MIP_POINT;
                sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
                sampler.MaxAnisotropy=1;sampler.ComparisonFunc=D3D12_COMPARISON_FUNC_NEVER;sampler.MaxLOD=FLT_MAX;
                sampler.ShaderRegister=i?9:7;sampler.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;}
            D3D12_ROOT_SIGNATURE_DESC sss_desc{4,sss,2,sss_samplers,D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT};
            ComPtr<ID3DBlob> sss_blob,sss_error;
            Check(D3D12SerializeRootSignature(&sss_desc,D3D_ROOT_SIGNATURE_VERSION_1,&sss_blob,&sss_error),"native SSS root serialization");
            Check(ds3native::Pipelines().Root(device,sss_blob->GetBufferPointer(),sss_blob->GetBufferSize(),native_sss_root_.ReleaseAndGetAddressOf()),"native SSS root");
            // cbSSS: the engine's CPU record array is 0xFF60 bytes (114A50); the shader indexes it by GBuffer1.w*255.
            // Written per submission into frame_uploads_ (B2c, EncodeNativeSss).
            // 1147 GXEnvMapRegion: b3 CB_ENVMAP_REGION and b8 cbSceneParam (VS and PS), t0/t1/t3/t5,
            // t30 g_IBLTexture; s7 SS_ClampPoint, s8 SS_WrapLinear (the programme's declarations).
            {D3D12_ROOT_PARAMETER env[4]{};
                for(unsigned i=0;i<2;++i){env[i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;env[i].Descriptor.ShaderRegister=i?8u:3u;env[i].ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;}
                const D3D12_DESCRIPTOR_RANGE gbuffer[3]{{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,2,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,3,0,2},{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,5,0,3}};
                const D3D12_DESCRIPTOR_RANGE cube{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,30,0,0};
                env[2].ParameterType=env[3].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                env[2].DescriptorTable={3,gbuffer};env[3].DescriptorTable={1,&cube};env[2].ShaderVisibility=env[3].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
                D3D12_ROOT_SIGNATURE_DESC env_desc{4,env,2,samplers,D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT};
                ComPtr<ID3DBlob> env_blob,env_error;
                Check(D3D12SerializeRootSignature(&env_desc,D3D_ROOT_SIGNATURE_VERSION_1,&env_blob,&env_error),"native env region root serialization");
                Check(ds3native::Pipelines().Root(device,env_blob->GetBufferPointer(),env_blob->GetBufferSize(),native_env_root_.ReleaseAndGetAddressOf()),"native env region root");
                // Per-region constants (400 bytes in 512-byte slots) and the 960-byte scene block: frame_uploads_ (B2c).
            }
        }
        const D3D12_INPUT_ELEMENT_DESC elements[]{
            {"POSITION",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
            {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,16,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}};
        const std::vector<uint8_t>* pixel[4]{&programs.ambient,&programs.sun,&programs.albedo,&programs.composite};
        for(unsigned pass=0;pass<4;++pass){
            D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};Blend(pso);pso.pRootSignature=native_light_root_.Get();
            pso.VS={programs.vertex.data(),programs.vertex.size()};pso.PS={pixel[pass]->data(),pixel[pass]->size()};
            pso.InputLayout={elements,2};pso.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;pso.RasterizerState.DepthClipEnable=FALSE;
            pso.DepthStencilState.DepthEnable=FALSE;
            if(pass<2){pso.NumRenderTargets=2;pso.RTVFormats[0]=pso.RTVFormats[1]=DXGI_FORMAT_R11G11B10_FLOAT;
                for(auto& target:pso.BlendState.RenderTarget)target.RenderTargetWriteMask=7;
                // Light accumulation adds each light's diffuse/specular pair.
                if(pass==1){pso.BlendState.IndependentBlendEnable=FALSE;auto& target=pso.BlendState.RenderTarget[0];
                    target.BlendEnable=TRUE;target.SrcBlend=target.DestBlend=D3D12_BLEND_ONE;}}
            else{pso.NumRenderTargets=1;pso.RTVFormats[0]=DXGI_FORMAT_R16G16B16A16_FLOAT;}
            Check(ds3native::Pipelines().Graphics(device,pso,native_light_psos_[pass].ReleaseAndGetAddressOf()),"native lighting pipeline");
        }
        // D-0180 (10CC20): GXRend_CompositeShadowMask into the RGBA8 mask, RGB only (the shadow stays in w), on the read-only depth
        // view with the stencil test read mask 0x10 EQUAL (two draws, references 0 and 0x10), depth off.
        native_sun_shadowed_pso_.Reset();menu_shadow_ready_.store(false,std::memory_order_release);
        if(!programs.sun_shadowed.empty()){   // D-0180 C4: DDirLightShadowed1 with the sun pass's state (two outputs, the first added)
            D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};Blend(pso);pso.pRootSignature=native_light_root_.Get();
            pso.VS={programs.vertex.data(),programs.vertex.size()};pso.PS={programs.sun_shadowed.data(),programs.sun_shadowed.size()};
            pso.InputLayout={elements,2};pso.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;pso.RasterizerState.DepthClipEnable=FALSE;
            pso.DepthStencilState.DepthEnable=FALSE;pso.NumRenderTargets=2;pso.RTVFormats[0]=pso.RTVFormats[1]=DXGI_FORMAT_R11G11B10_FLOAT;
            for(auto& target:pso.BlendState.RenderTarget)target.RenderTargetWriteMask=7;
            pso.BlendState.IndependentBlendEnable=FALSE;auto& target=pso.BlendState.RenderTarget[0];target.BlendEnable=TRUE;target.SrcBlend=target.DestBlend=D3D12_BLEND_ONE;
            Check(ds3native::Pipelines().Graphics(device,pso,native_sun_shadowed_pso_.ReleaseAndGetAddressOf()),"native shadowed sun pipeline");
            menu_shadow_ready_.store(true,std::memory_order_release);}
        native_mask_pso_.Reset();
        if(!programs.mask_composite.empty()){
            D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};Blend(pso);pso.pRootSignature=native_light_root_.Get();
            pso.VS={programs.vertex.data(),programs.vertex.size()};pso.PS={programs.mask_composite.data(),programs.mask_composite.size()};
            pso.InputLayout={elements,2};pso.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;pso.RasterizerState.DepthClipEnable=FALSE;
            auto& depth=pso.DepthStencilState;depth.DepthEnable=FALSE;depth.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ZERO;
            depth.StencilEnable=TRUE;depth.StencilReadMask=0x10;depth.StencilWriteMask=0;
            depth.FrontFace={D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_COMPARISON_FUNC_EQUAL};depth.BackFace=depth.FrontFace;
            pso.DSVFormat=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;pso.NumRenderTargets=1;pso.RTVFormats[0]=MaskStage::ColorFormat();
            pso.BlendState.RenderTarget[0].RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_RED|D3D12_COLOR_WRITE_ENABLE_GREEN|D3D12_COLOR_WRITE_ENABLE_BLUE;
            Check(ds3native::Pipelines().Graphics(device,pso,native_mask_pso_.ReleaseAndGetAddressOf()),"native shadow mask composite pipeline");}
        // 1137 (140114640): GXSss VS with GXSssBlurX/Y, depth off, stencil read mask 4 EQUAL (reference 4,
        // never written) on the read-only depth view, one irradiance-format target written whole.
        for(auto& pipeline:native_sss_psos_)pipeline.Reset();
        if(!programs.sss_vertex.empty() && !programs.sss_blur_x.empty() && !programs.sss_blur_y.empty()){
            const std::vector<uint8_t>* blur[2]{&programs.sss_blur_x,&programs.sss_blur_y};
            for(unsigned pass=0;pass<2;++pass){
                D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};Blend(pso);pso.pRootSignature=native_sss_root_.Get();
                pso.VS={programs.sss_vertex.data(),programs.sss_vertex.size()};pso.PS={blur[pass]->data(),blur[pass]->size()};
                pso.InputLayout={elements,2};pso.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;pso.RasterizerState.DepthClipEnable=FALSE;
                auto& depth=pso.DepthStencilState;depth.DepthEnable=FALSE;depth.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ZERO;
                depth.StencilEnable=TRUE;depth.StencilReadMask=4;depth.StencilWriteMask=0;
                depth.FrontFace={D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_COMPARISON_FUNC_EQUAL};depth.BackFace=depth.FrontFace;
                pso.DSVFormat=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;pso.NumRenderTargets=1;pso.RTVFormats[0]=DXGI_FORMAT_R16G16B16A16_FLOAT;
                Check(ds3native::Pipelines().Graphics(device,pso,native_sss_psos_[pass].ReleaseAndGetAddressOf()),"native SSS pipeline");
            }
        }
        // 1147 (675B0): POSITION float3 strip of the region box; outside: cull front, depth LESS_EQUAL
        // unwritten (69B00 index 2); camera inside: cull back, depth off (index 0); both stencil read/write
        // mask 8 EQUAL (reference 0xFF), blend source alpha over inverse source alpha, RGB into SpecularAcc.
        for(auto& pipeline:native_env_psos_)pipeline.Reset();
        if(!programs.env_vertex.empty() && !programs.env_pixel.empty() && !programs.env_pixel_pcc.empty() && programs.env_box.size()==124){
            const std::vector<uint8_t>* pixels[2]{&programs.env_pixel,&programs.env_pixel_pcc};
            const D3D12_INPUT_ELEMENT_DESC position{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0};
            for(unsigned variant=0;variant<2;++variant)for(unsigned inside=0;inside<2;++inside){
                D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};Blend(pso);pso.pRootSignature=native_env_root_.Get();
                pso.VS={programs.env_vertex.data(),programs.env_vertex.size()};pso.PS={pixels[variant]->data(),pixels[variant]->size()};
                pso.InputLayout={&position,1};pso.RasterizerState.FrontCounterClockwise=TRUE;
                pso.RasterizerState.CullMode=inside?D3D12_CULL_MODE_BACK:D3D12_CULL_MODE_FRONT;
                auto& depth=pso.DepthStencilState;depth.DepthEnable=inside?FALSE:TRUE;depth.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ZERO;
                depth.DepthFunc=inside?D3D12_COMPARISON_FUNC_LESS:D3D12_COMPARISON_FUNC_LESS_EQUAL;
                depth.StencilEnable=TRUE;depth.StencilReadMask=depth.StencilWriteMask=8;
                depth.FrontFace.StencilFunc=D3D12_COMPARISON_FUNC_EQUAL;depth.BackFace=depth.FrontFace;
                for(auto& target:pso.BlendState.RenderTarget){target.BlendEnable=TRUE;target.SrcBlend=D3D12_BLEND_SRC_ALPHA;
                    target.DestBlend=D3D12_BLEND_INV_SRC_ALPHA;target.RenderTargetWriteMask=7;}
                pso.NumRenderTargets=1;pso.RTVFormats[0]=DXGI_FORMAT_R11G11B10_FLOAT;pso.DSVFormat=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
                Check(ds3native::Pipelines().Graphics(device,pso,native_env_psos_[variant*2+inside].ReleaseAndGetAddressOf()),"native env region pipeline");
            }
            native_env_box_=Upload(device,programs.env_box.data(),programs.env_box.size(),256);
            native_env_vertices_={native_env_box_->GetGPUVirtualAddress(),96,12};
            native_env_indices_={native_env_box_->GetGPUVirtualAddress()+96,28,DXGI_FORMAT_R16_UINT};
        }
        native_point_psos_.clear();
        native_light_programs_=input.programs;
    }
    // 1168: the view and target heaps belong to the current target set (main view or a menu offscreen view);
    // the neutral 1x1 images are shared, their RTVs live in each set's heap (slots 4, 5).
    if(!view_->light_views){
        D3D12_DESCRIPTOR_HEAP_DESC views{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,env_cube_views+env_region_limit*env_range_count,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
        Check(CountedHeap(device)->CreateDescriptorHeap(&views,IID_PPV_ARGS(&view_->light_views)),"native lighting views");
        // Axis 2: the CPU mirror of the fixed part (the env cube ranges stay per frame slot in the shader-visible heap only).
        D3D12_DESCRIPTOR_HEAP_DESC mirror{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,light_fixed_views,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
        Check(CountedHeap(device)->CreateDescriptorHeap(&mirror,IID_PPV_ARGS(&view_->light_views_cpu)),"native lighting views mirror");
        D3D12_DESCRIPTOR_HEAP_DESC rtvs{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,8,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
        Check(CountedHeap(device)->CreateDescriptorHeap(&rtvs,IID_PPV_ARGS(&view_->light_rtvs)),"native lighting targets");
        for(unsigned i=0;i<2;++i)device->CreateRenderTargetView(native_light_neutral_[i].Get(),nullptr,CPU(view_->light_rtvs.Get(),rtv_stride_,4+i));
        view_->light_generation=UINT64_MAX;
    }
    if(view_->light_generation==view_->target_generation)return;
    // Targets follow the native G-buffer extent and depth incarnation.
    const DXGI_FORMAT formats[4]{DXGI_FORMAT_R11G11B10_FLOAT,DXGI_FORMAT_R11G11B10_FLOAT,DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R16G16B16A16_FLOAT};
    const auto heap=Heap(D3D12_HEAP_TYPE_DEFAULT);
    for(unsigned i=0;i<4;++i){
        const auto image=Image(ViewWidth(),ViewHeight(),1,formats[i],D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        D3D12_CLEAR_VALUE clear{};clear.Format=formats[i];
        Check(CountedResource(device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&image,D3D12_RESOURCE_STATE_RENDER_TARGET,&clear,IID_PPV_ARGS(&view_->light_targets[i])),"native lighting target");
        device->CreateRenderTargetView(view_->light_targets[i].Get(),nullptr,CPU(view_->light_rtvs.Get(),rtv_stride_,i));
    }
    const auto view=[&](uint32_t pass,uint32_t slot,ID3D12Resource* resource,DXGI_FORMAT format){
        D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=format;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
        srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;
        device->CreateShaderResourceView(resource,&srv,CPU(view_->light_views_cpu.Get(),binding_stride_,pass*light_pass_views+slot));};   // axis 2: into the mirror
    for(uint32_t pass=0;pass<light_passes;++pass){
        for(uint32_t slot=0;slot<9;++slot)view(pass,slot,nullptr,DXGI_FORMAT_R8G8B8A8_UNORM);
        D3D12_SHADER_RESOURCE_VIEW_DESC list{};list.Format=DXGI_FORMAT_UNKNOWN;list.ViewDimension=D3D12_SRV_DIMENSION_BUFFER;
        list.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;list.Buffer.NumElements=1;list.Buffer.StructureByteStride=8;
        device->CreateShaderResourceView(native_light_list_.Get(),&list,CPU(view_->light_views_cpu.Get(),binding_stride_,pass*light_pass_views+9));
        view(pass,10,native_light_neutral_[0].Get(),DXGI_FORMAT_R8G8B8A8_UNORM);
    }
    // t33 (point lights' SSAO): neutral white always (I-0126 R7c, D-0251: no SSAO).
    view(light_passes,0,native_light_neutral_[0].Get(),DXGI_FORMAT_R8G8B8A8_UNORM);
    // K-0407 resource roles: t0 GBuffer0 (normal), t1 depth, t3 GBuffer1, t4 GBuffer2
    // (albedo), t5 GBuffer3, t6 irradiance, t7 specular acc, t8 emissive (GBuffer5).
    constexpr DXGI_FORMAT depth=DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
    // 1147: t7 is the SpecularAcc (the specular ambient, R11G11B10, cleared to zero each frame, so black without the traced
    // reflections or a region), RTV slot 7 (D-0301: the region cubes' raster pass draws it on an untraced menu view; the rays read the regions).
    {const auto image=Image(ViewWidth(),ViewHeight(),1,DXGI_FORMAT_R11G11B10_FLOAT,D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        D3D12_CLEAR_VALUE clear{};clear.Format=DXGI_FORMAT_R11G11B10_FLOAT;
        Check(CountedResource(device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&image,D3D12_RESOURCE_STATE_RENDER_TARGET,&clear,IID_PPV_ARGS(&view_->env_target)),"native env region target");
        device->CreateRenderTargetView(view_->env_target.Get(),nullptr,CPU(view_->light_rtvs.Get(),rtv_stride_,7));}
    view(0,env_views,view_->colors[0].Get(),color_formats[0]);view(0,env_views+1,view_->depth.Get(),depth);
    view(0,env_views+2,view_->colors[1].Get(),color_formats[1]);view(0,env_views+3,view_->colors[3].Get(),color_formats[3]);
    view(0,6,view_->colors[4].Get(),color_formats[4]);view(0,7,view_->env_target.Get(),DXGI_FORMAT_R11G11B10_FLOAT);
    view(1,0,view_->colors[0].Get(),color_formats[0]);view(1,1,view_->depth.Get(),depth);view(1,3,view_->colors[1].Get(),color_formats[1]);view(1,5,view_->colors[3].Get(),color_formats[3]);
    view(2,4,view_->colors[2].Get(),color_formats[2]);view(2,6,view_->light_targets[0].Get(),formats[0]);
    view(3,1,view_->depth.Get(),depth);view(3,4,view_->colors[2].Get(),color_formats[2]);view(3,5,view_->colors[3].Get(),color_formats[3]);
    view(3,6,view_->light_targets[2].Get(),formats[2]);view(3,7,view_->light_targets[1].Get(),formats[1]);view(3,8,view_->colors[5].Get(),color_formats[5]);
    // 1137 SSS temporary: the irradiance size, format code 0xA (R16G16B16A16_FLOAT, as GXLightCmbAlbedo's target).
    {const auto image=Image(ViewWidth(),ViewHeight(),1,formats[2],D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        D3D12_CLEAR_VALUE clear{};clear.Format=formats[2];
        Check(CountedResource(device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&image,D3D12_RESOURCE_STATE_RENDER_TARGET,&clear,IID_PPV_ARGS(&view_->sss_temp)),"native SSS temporary");
        device->CreateRenderTargetView(view_->sss_temp.Get(),nullptr,CPU(view_->light_rtvs.Get(),rtv_stride_,6));}
    const auto sss_view=[&](uint32_t slot,ID3D12Resource* resource,DXGI_FORMAT format){view(0,sss_views+slot,resource,format);};
    sss_view(0,view_->depth.Get(),depth);sss_view(1,nullptr,DXGI_FORMAT_R8G8B8A8_UNORM);sss_view(2,view_->colors[1].Get(),color_formats[1]);
    sss_view(3,view_->light_targets[2].Get(),formats[2]);sss_view(4,view_->sss_temp.Get(),formats[2]);
    D3D12_SHADER_RESOURCE_VIEW_DESC lit{};lit.Format=formats[3];lit.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
    lit.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;lit.Texture2D.MipLevels=1;
    device->CreateShaderResourceView(view_->light_targets[3].Get(),&lit,CPU(view_->bindings.Get(),binding_stride_,composition_binding+1));
    // Axis 2: the fixed part of the mirror into the shader-visible heap (the mask views may have been bound into the mirror earlier).
    device->CopyDescriptorsSimple(light_fixed_views,CPU(view_->light_views.Get(),binding_stride_,0),CPU(view_->light_views_cpu.Get(),binding_stride_,0),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    if(mask_views_.heap==view_->light_views.Get())mask_views_={};   // this view's mask views written again for its new targets (another view's key kept: R7a's step review M3)
    view_->light_generation=view_->target_generation;
}
// 1054: per-tile point-light batches (1A1DA0) added into the diffuse/specular pair; I-0106: each tile's spot groups (1A6690)
// after its point batches, with the same quad, targets, blend and bindings (DSpotLightN declares the point programmes' t0,
// t1, t3, t5, t33, b0, b1 and b4); 1A6690's depth bounds are an optimisation the native leaves out (the programmes test the cone).
// after the sun, with the sun pass's G-buffer views and a neutral t33.
void SceneRenderer::EncodeNativePointLights(ID3D12GraphicsCommandList* list,const NativeLightingInput& input,const D3D12_RECT& screen) {
    if(input.point_batches.empty())return;
    auto* device=gpu_.device.Get();const auto& programs=*input.programs;
    // Per batch: cbLightAccLightParam (<= 12 x 64 bytes) then cbLightAccVtxParam. I-0131 R10a: every batch is unshadowed (the
    // shadowed programmes and the local atlas are removed).
    constexpr uint64_t stride=1280,vertex_offset=768;
    for(const auto& batch:input.point_batches){
        const size_t programmes=batch.spot?programs.spot.size():programs.point.size();
        if(batch.lights.empty() || batch.lights.size()>vertex_offset || batch.programme>=programmes || (batch.spot && batch.lights.size()!=128*(batch.programme+1)) ||
           (!batch.spot && batch.lights.size()%64))
            throw std::invalid_argument("native point light batch");}
    const auto range=frame_uploads_.Allocate(device,stride*input.point_batches.size());   // zero-filled (B2c)
    for(size_t k=0;k<input.point_batches.size();++k){const auto& batch=input.point_batches[k];
        std::memcpy(range.cpu+k*stride,batch.lights.data(),batch.lights.size());
        const float vertex[4]{batch.depth,0,0,0};std::memcpy(range.cpu+k*stride+vertex_offset,vertex,sizeof(vertex));}
    const D3D12_INPUT_ELEMENT_DESC elements[]{
        {"POSITION",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
        {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,16,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}};
    const D3D12_CPU_DESCRIPTOR_HANDLE accumulation[]{CPU(view_->light_rtvs.Get(),rtv_stride_,0),CPU(view_->light_rtvs.Get(),rtv_stride_,1)};
    list->OMSetRenderTargets(2,accumulation,FALSE,nullptr);
    const auto base=1*light_pass_views;
    list->SetGraphicsRootDescriptorTable(4,GPU(view_->light_views.Get(),binding_stride_,base));
    list->SetGraphicsRootDescriptorTable(5,GPU(view_->light_views.Get(),binding_stride_,base+9));
    list->SetGraphicsRootDescriptorTable(6,GPU(view_->light_views.Get(),binding_stride_,base+10));
    list->SetGraphicsRootDescriptorTable(8,GPU(view_->light_views.Get(),binding_stride_,light_passes*light_pass_views));
    const auto ring=range.gpu;
    for(size_t i=0;i<input.point_batches.size();++i){const auto& batch=input.point_batches[i];
        const auto& pixel=batch.spot?programs.spot[batch.programme]:programs.point[batch.programme];
        if(pixel.empty())continue;
        auto& pipeline=native_point_psos_[&pixel];
        if(!pipeline){
            D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};Blend(pso);pso.pRootSignature=native_light_root_.Get();
            pso.VS={programs.vertex.data(),programs.vertex.size()};pso.PS={pixel.data(),pixel.size()};
            pso.InputLayout={elements,2};pso.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;pso.RasterizerState.DepthClipEnable=FALSE;
            pso.DepthStencilState.DepthEnable=FALSE;pso.NumRenderTargets=2;pso.RTVFormats[0]=pso.RTVFormats[1]=DXGI_FORMAT_R11G11B10_FLOAT;
            for(auto& target:pso.BlendState.RenderTarget)target.RenderTargetWriteMask=7;
            auto& target=pso.BlendState.RenderTarget[0];target.BlendEnable=TRUE;target.SrcBlend=target.DestBlend=D3D12_BLEND_ONE;
            Check(ds3native::Pipelines().Graphics(device,pso,pipeline.ReleaseAndGetAddressOf()),"native point light pipeline");
        }
        list->SetPipelineState(pipeline.Get());
        list->SetGraphicsRootConstantBufferView(7,ring+i*stride);
        list->SetGraphicsRootConstantBufferView(1,ring+i*stride+vertex_offset);
        const D3D12_RECT rect{batch.left,batch.top,batch.right,batch.bottom};list->RSSetScissorRects(1,&rect);
        list->DrawInstanced(4,1,0,0);
    }
    list->RSSetScissorRects(1,&screen);
    list->SetGraphicsRootConstantBufferView(1,native_light_constants_[1]);
}
void SceneRenderer::EncodeNativeLighting(ID3D12GraphicsCommandList* list,const NativeLightingInput& input,
    const D3D12_VIEWPORT& viewport,const D3D12_RECT& scissor,const NativeShadowMaskInput* mask,const NativeModelFrameInput* sun) {
    // I-0132 R11e (D-0262, no fallback): the main view with the rays never takes DS3's fill lights nor GB4's baked ambient (R11d's final
    // review M3 kept them on a frame without the GI's inputs: the fallback, removed); such a frame is judged unserved and not shown.
    const bool traced=view_==&main_view_ && rt_asked.load(std::memory_order_relaxed);
    const std::vector<uint8_t>* blocks[4]{&input.light_acc,nullptr,&input.light,&input.fog};
    const size_t limits[4]{512,256,80*1024,32*1024};
    for(unsigned i=0;i<4;++i)if(blocks[i] && (blocks[i]->empty() || blocks[i]->size()>limits[i]))throw std::invalid_argument("native lighting constant extent");
    // B2c: each block in a range of its full extent, zero past the block (cbLight/cbFog are indexed by GBuffer3.w*255
    // and root descriptors have no bounds checking).
    for(unsigned i=0;i<4;++i)if(blocks[i]){
        const auto range=frame_uploads_.Allocate(gpu_.device.Get(),limits[i]);
        std::memcpy(range.cpu,blocks[i]->data(),blocks[i]->size());native_light_constants_[i]=range.gpu;
        // D-0262 (Thomas, 2026-11-09): the sets' fill lights (lights 1 to 3: Col and Spc rows 5-7 and 9-11 of each 288-byte set) removed
        // on the GI frames for the pixel's passes and the rays' hits alike; the traced bounce replaces them (R11e: on every traced frame).
        // I-0141: but for a cutscene's own sets (input.authored_sets), whose lights 1 to 3 are the cut's authored lights.
        if(i==2 && traced)
            for(size_t set=0;set+288<=blocks[i]->size();set+=288){const size_t index=set/288;
                if(index<input.authored_sets.size() && input.authored_sets[index])continue;
                for(const size_t row:{5,6,7,9,10,11})std::memset(range.cpu+set+16*row,0,16);}}
    native_light_constants_[1]=native_light_zeros_->GetGPUVirtualAddress();
    // I-0132 R11b (its step reviews' H1; the RCA in the RESULT): on a frame with SER the reordered dispatches (ray generation, a non-pixel
    // stage) read the depth's plane 0 and GB0 to GB3, so the whole lighting phase holds them PIXEL | NON_PIXEL_SHADER_RESOURCE (every pass
    // declares that to the graph: its planner keeps a read state's superset, so a pass-wide excursion would never match the hand barriers),
    // else PIXEL_SHADER_RESOURCE as before. GbReadState gives a resource's state; the stencil plane stays writable.
    lit_gb_read_=rt_ser_now_ && view_==&main_view_?kSerReadState:D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;lit_depth_read_=D3D12_RESOURCE_STATE_DEPTH_READ|lit_gb_read_;
    const auto depth_read=lit_depth_read_;
    for(unsigned i=0;i<6;++i)Transition(list,view_->colors[i].Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,i<4?lit_gb_read_:D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    DepthPlaneTransition(list,view_->depth.Get(),D3D12_RESOURCE_STATE_DEPTH_WRITE,depth_read);
    // E2: the frame graph's passes of the lighting, each with the SRVs its table binds (NativeLightingResources' `view` table:
    // pass 0 t6 GBuffer4, t7 SpecularAcc; pass 1 and the point lights t0 GBuffer0, t1 depth, t3 GBuffer1, t5 GBuffer3; pass 2
    // t4 GBuffer2, t6 light0; pass 3 t1 depth, t4 GBuffer2, t5 GBuffer3, t6 light2, t7 light1, t8 GBuffer5; every pass t32
    // the stage's mask, a menu view's self-shadow mask (SelfMaskBound: traced, held or a refused rewrite away from it) or neutral0,
    // and the point lights' t33 neutral0).
    auto* graph=active_graph_shadow;
    const auto reads=[&](GraphShadow* g,std::initializer_list<ID3D12Resource*> resources,bool depth){
        for(auto* resource:resources)g->Use(resource,ResourceAccess::Read,GbReadState(resource));
        if(depth)g->Use(view_->depth.Get(),ResourceAccess::Read,depth_read,{0,1});};
    const auto& gb=view_->colors;const auto& lt=view_->light_targets;
    // E5 (sheet 6.4): the neutral images (white t32/t33 and the black one) never change: cleared once after their creation,
    // then shader-readable between frames (they were cleared and toggled every frame).
    if(!native_neutral_ready_){
        if(graph)graph->Pass("light_neutral",PassKind::Clear);
        for(unsigned i=0;i<2;++i){const float colour[4]{i?0.f:1.f,i?0.f:1.f,i?0.f:1.f,i?0.f:1.f};
            list->ClearRenderTargetView(CPU(view_->light_rtvs.Get(),rtv_stride_,4+i),colour,0,nullptr);
            if(graph)graph->Use(native_light_neutral_[i].Get(),ResourceAccess::Write,D3D12_RESOURCE_STATE_RENDER_TARGET);
            Transition(list,native_light_neutral_[i].Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);}
        native_neutral_ready_=true;}
    // D-0180: the shadow mask before the env regions and GXLightDeferredAO (NATIVE-PASS-ORDER 6); a view whose lighting views name
    // the stage's target without a mask this frame goes back to the neutral white. t33 (the AO term every light reads) is always the
    // neutral white (I-0126 R7c, D-0251: the SSAO is removed). I-0126 R7a: on a GI frame (the main view's) pass 0 reads the neutral
    // black for GB4 (no baked diffuse ambient: the traced bounce is added after GXLightCmbAlbedo).
    const bool gi=rt_gi_now_ && view_==&main_view_;
    const bool gb4_black=traced;   // I-0132 R11e: GB4 the neutral black on every traced frame (the fallback's GB4 removed)
    // I-0128 R8a (rt-gpu-measure.flag): the zones GpuZoneBegin and GpuZoneEnd time on the main view (nothing otherwise).
    if(mask)EncodeNativeShadowMask(list,*mask,viewport,scissor,sun,gb4_black);
    else{SyncNativeMaskViews(false,gb4_black);
        if(view_!=&main_view_)EncodeNativeSelfShadow(list,sun,viewport,scissor);}   // I-0140 t31: a Player menu view's
    ID3D12DescriptorHeap* heaps[]{view_->light_views.Get()};list->SetDescriptorHeaps(1,heaps);
    rt_lights_uploaded_={};   // a fresh list each frame (I-0125 R6b step review M1: before the reflections' upload, not after it)
    rt_cache_now_=false;if(gi){GpuZoneBegin(list,GpuZone::RtCache);EncodeRtCache(list,input,graph);if(rt_cache_now_)GpuZoneEnd(list,GpuZone::RtCache);else GpuZoneCancel(GpuZone::RtCache);}
    if(view_==&main_view_ && input.rt_probes && rt_probes.load(std::memory_order_relaxed)){GpuZoneBegin(list,GpuZone::RtProbe);EncodeRtProbes(list,input);   // I-0129 R9a: the probes, after the cache
        if(rt_probe_pending_.readback)GpuZoneEnd(list,GpuZone::RtProbe);else GpuZoneCancel(GpuZone::RtProbe);}   // I-0126 R7b: the cache, before the passes that query it
    // SpecularAcc (t7 of GXLightDeferredAO): the traced reflections, else the env-map regions' raster pass of an untraced menu view (D-0301), else cleared.
    list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
    const bool rr_hit=rr_now_ && view_==&main_view_ && rr_targets_[4] && rr_rtvs_;   // I-0125 R6c: RR's specular hit distance this frame
    // I-0126 R7d: its transition before the pass's declaration, as every hand transition (after it, the graph's shadow had the target in
    // RENDER_TARGET already: a graph_barrier_mismatch row a frame since R6c, and the plan never equal on RR's frames).
    if(rr_hit)Transition(list,rr_targets_[4].Get(),kRrGuideState,D3D12_RESOURCE_STATE_RENDER_TARGET);
    // I-0132 R11 (D-0252, Thomas 2026-11-09 09:24): on an NRD frame (a GI frame RR does not denoise) the specular goes through NRD with
    // the GI: one signal pass for both here (EncodeRtNrd: the velocity target read, transitioned before the pass declares it as R7d's
    // M2 asks, back to RENDER_TARGET after it; R7d's bias check written there), NRD's dispatches, then the specular's compose into
    // SpecularAcc; the GI's compose stays after GXLightCmbAlbedo (EncodeRtGi).
    const bool nrd=gi && rt_nrd_now_;rt_nrd_recorded_=false;
    if(nrd)Transition(list,velocity_target_.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    if(graph){graph->Pass("env_regions",PassKind::Environment);graph->Use(view_->env_target.Get(),ResourceAccess::Write,D3D12_RESOURCE_STATE_RENDER_TARGET);
        reads(graph,{gb[0].Get(),gb[1].Get(),gb[3].Get()},true);
        graph->Use(view_->depth.Get(),ResourceAccess::ReadWrite,D3D12_RESOURCE_STATE_DEPTH_WRITE,{1,1});   // the stencil plane writable
        if(rr_hit)graph->Use(rr_targets_[4].Get(),ResourceAccess::Write,D3D12_RESOURCE_STATE_RENDER_TARGET);   // I-0125 R6c
        if(nrd){graph->Use(velocity_target_.Get(),ResourceAccess::Read,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            if(rt_gi_mean_now_)graph->Use(rt_gi_mean_target_.Get(),ResourceAccess::ReadWrite,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);}}   // I-0126 R7d's bias check
    // I-0125 R6c: RR's specular hit distance cleared on each frame RR denoises (0: no specular ambient), then written by the traced pass.
    rr_hit_written_=false;   // the step review's M1: RR reads the distance only when the traced pass wrote it this frame
    if(rr_hit){constexpr float none[4]{};list->ClearRenderTargetView(CPU(rr_rtvs_.Get(),rtv_stride_,3),none,0,nullptr);}
    // I-0128 R8a (the step review's L2): a zone timed only where its traced pass ran (a refused pass's zone dropped, GpuZoneCancel).
    // I-0132 R11: on an NRD frame the zone holds both signals' rays, NRD's dispatches and the specular's compose (RtGi its compose).
    GpuZoneBegin(list,GpuZone::RtReflect);bool reflected=false;
    if(nrd){try{reflected=EncodeRtNrd(list,input);}
        catch(...){Transition(list,velocity_target_.Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);throw;}
        Transition(list,velocity_target_.Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);}
    else reflected=EncodeRtReflections(list,input);
    // Traced, else the region draws of the input (D-0301: an untraced menu view draws its own region; a view with no region keeps
    // SpecularAcc cleared to zero, as the region pass clears it first).
    if(reflected)GpuZoneEnd(list,GpuZone::RtReflect);
    else{GpuZoneCancel(GpuZone::RtReflect);EncodeNativeEnvRegions(list,input);}
    if(rr_hit)Transition(list,rr_targets_[4].Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,kRrGuideState);
    // I-0141 tranche 27 (a bounded run's share variant): the specular ambient's share taken out of the main view, SpecularAcc zero.
    if(view_==&main_view_ && (share_variant_off.load(std::memory_order_relaxed)&kShareRendererNoSpecular)){constexpr float none[4]{};
        list->ClearRenderTargetView(CPU(view_->light_rtvs.Get(),rtv_stride_,7),none,0,nullptr);}
    Transition(list,view_->env_target.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    // I-0125 R6c: the water's t54 traced, before the forward rows sample it (EncodeRtWater's own guard is this condition: nothing to do otherwise).
    if(rt_water_now_ && view_==&main_view_){GpuZoneBegin(list,GpuZone::RtWater);EncodeRtWater(list,input,graph,viewport,scissor);GpuZoneEnd(list,GpuZone::RtWater);}
    list->SetGraphicsRootSignature(native_light_root_.Get());
    for(unsigned i=0;i<4;++i)list->SetGraphicsRootConstantBufferView(i,native_light_constants_[i]);
    // Every root parameter bound: b1 (point lights) zeros, t33 neutral until used.
    list->SetGraphicsRootConstantBufferView(7,native_light_constants_[1]);
    list->SetGraphicsRootDescriptorTable(8,GPU(view_->light_views.Get(),binding_stride_,light_passes*light_pass_views));
    list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
    list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);list->IASetVertexBuffers(0,1,&native_light_quad_view_);list->IASetIndexBuffer(nullptr);
    const auto pass=[&](uint32_t index,std::initializer_list<uint32_t> targets){
        std::array<D3D12_CPU_DESCRIPTOR_HANDLE,2> handles{};uint32_t count{};
        for(const auto target:targets)handles[count++]=CPU(view_->light_rtvs.Get(),rtv_stride_,target);
        // D-0180 C4: the sun pass takes DDirLightShadowed1 on a frame with the screen-space sun shadow (19FB20: pass+0x710 == 3).
        list->OMSetRenderTargets(count,handles.data(),FALSE,nullptr);
        list->SetPipelineState(index==1 && sun && native_sun_shadowed_pso_?native_sun_shadowed_pso_.Get():native_light_psos_[index].Get());
        const auto base=index*light_pass_views;
        list->SetGraphicsRootDescriptorTable(4,GPU(view_->light_views.Get(),binding_stride_,base));
        list->SetGraphicsRootDescriptorTable(5,GPU(view_->light_views.Get(),binding_stride_,base+9));
        list->SetGraphicsRootDescriptorTable(6,GPU(view_->light_views.Get(),binding_stride_,base+10));
        list->DrawInstanced(4,1,0,0);};
    // Review (step 1): the passes read the views' t32 (every block) and t33 (the point lights): the stage's mask on a frame with the
    // mask, which the frame graph must see read (it leaves a persistent resource in its last state until then); t33 the neutral white.
    // I-0140 t31: a menu view's context names its self-shadow mask in every block while its published views name it
    // (SelfMaskBound, after BindSelfShadowViews: a traced frame or SelfMaskHeld, the mask then traced or cleared white, or a
    // refused rewrite away from it, the mask cleared white), the neutral white otherwise.
    auto* t32=mask?mask_.Mask():view_!=&main_view_ && SelfMaskBound()?view_->self_mask.Get():native_light_neutral_[0].Get();auto* t33=native_light_neutral_[0].Get();
    if(graph){graph->Pass("light_ambient_sun",PassKind::Lighting);for(unsigned i=0;i<2;++i)graph->Use(view_->light_targets[i].Get(),ResourceAccess::Write,D3D12_RESOURCE_STATE_RENDER_TARGET);
        reads(graph,{gb4_black?native_light_neutral_[1].Get():gb[4].Get(),view_->env_target.Get(),gb[0].Get(),gb[1].Get(),gb[3].Get(),t32},true);if(t33!=t32)reads(graph,{t33},false);
        }
    if(sun)list->SetGraphicsRootConstantBufferView(9,sun_scene_block_);   // b8 of the shadowed sun (FC_ShadowColor)
    pass(0,{0,1});if(!input.skip_sun)pass(1,{0,1}); // GXLightDeferredAO, then the sun added (no-sun.flag: skipped, development)
    // I-0122 R4a: with the frame's list and the native pass (RtLightsFrame, the main view), the local lights shaded natively; R4b:
    // the shadowed ones traced when the frame has a top level, else shaded unshadowed by the pass itself (I-0131 R10a). A main-view
    // frame whose pass cannot run (a size in transition, a refused camera, a failure) is counted missed and judged unserved (cause 8;
    // I-0132 R11e: DS3's point batches, the fallback, no longer drawn there); the menu views draw their batches.
    const bool lights_pass=input.rt_lights && rt_lights_now_ && view_==&main_view_;if(lights_pass)GpuZoneBegin(list,GpuZone::RtLights);
    const bool native=lights_pass && EncodeRtLights(list,input);if(native)GpuZoneEnd(list,GpuZone::RtLights);else GpuZoneCancel(GpuZone::RtLights);
    if(!native){if(input.rt_lights)rt_lights_missed_.fetch_add(1,std::memory_order_relaxed);
        if(!traced)EncodeNativePointLights(list,input,scissor);}
    for(unsigned i=0;i<2;++i)Transition(list,view_->light_targets[i].Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    if(graph){graph->Pass("light_albedo",PassKind::Lighting);graph->Use(view_->light_targets[2].Get(),ResourceAccess::Write,D3D12_RESOURCE_STATE_RENDER_TARGET);
        reads(graph,{gb[2].Get(),lt[0].Get(),t32},false);}
    pass(2,{2}); // GXLightCmbAlbedo
    bool gi_served=false;   // I-0132 R11d: the GI added its bounce this frame (the magenta's test)
    if(gi){GpuZoneBegin(list,GpuZone::RtGi);const auto gi_missed=rt_gi_missed_.load(std::memory_order_relaxed);EncodeRtGi(list,input,graph);
        gi_served=rt_gi_missed_.load(std::memory_order_relaxed)==gi_missed;if(gi_served)GpuZoneEnd(list,GpuZone::RtGi);else GpuZoneCancel(GpuZone::RtGi);}   // a frame without its inputs untimed   // I-0126 R7a: the traced diffuse ambient added, before the SSS and the composite
    if(native_sss_psos_[0] && !input.sss_profiles.empty() && !input.sss_scene.empty()){
        EncodeNativeSss(list,input);
        // Restore the lighting bindings for the composite.
        list->SetGraphicsRootSignature(native_light_root_.Get());
        for(unsigned i=0;i<4;++i)list->SetGraphicsRootConstantBufferView(i,native_light_constants_[i]);
        list->SetGraphicsRootConstantBufferView(7,native_light_constants_[1]);
        list->SetGraphicsRootDescriptorTable(8,GPU(view_->light_views.Get(),binding_stride_,light_passes*light_pass_views));
    }
    Transition(list,view_->light_targets[2].Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    if(graph){graph->Pass("light_composite",PassKind::Lighting);graph->Use(view_->light_targets[3].Get(),ResourceAccess::Write,D3D12_RESOURCE_STATE_RENDER_TARGET);
        reads(graph,{gb[2].Get(),gb[3].Get(),lt[2].Get(),lt[1].Get(),gb[5].Get(),t32},true);}
    pass(3,{3}); // GXLightComposite: lit HDR
    // I-0132 R11d (Thomas, 2026-11-09 04:39: « Oui en magenta, comme ça on voit mieux »; rt-fail-magenta.flag, the test runs' tool): a
    // main-view frame the rays do not fully serve while they were asked at startup shows its lit HDR magenta instead of DS3's fallback,
    // so a capture cannot miss it; counted with its causes (the final review's M1): 1 no traced sun, 2 no top level or material
    // table, 4 no GI bounce, 8 no native local lights' pass. RR and frame generation may carry it into the next frames.
    // I-0132 R11e (D-0262, no fallback): every main-view frame judged (RtJudged, the arrival's rule); an arrival's held frame is not
    // magenta (its world black at the frame's end, counted there); an unserved frame without the flag shows its world black.
    if(view_==&main_view_ && rt_asked.load(std::memory_order_relaxed)){
        const uint32_t causes=(rt_sun_now_?0u:1u)|(rt_scene_address_ && rt_material_table_ && rt_material_rows_?0u:2u)|(gi_served?0u:4u)|((input.rt_lights || input.local_lights || !input.point_batches.empty()) && !native?8u:0u);   // 8 only where there are lights to shade (its fixes' step review M4; R11e: or batches without the list; I-0132 R11b: `local_lights`, a tile on screen with a light, where no batch is made)
        rt_judged_.store((((rt_judged_.load(std::memory_order_relaxed)>>8)+1)<<8)|causes,std::memory_order_release);rt_judged_now_=true;   // this thread alone writes it
        // R11e: the run of unserved frames outside a hold (the fatal rule, in time: its step review's L5), ended by a served or held frame (M2).
        if(!causes || rt_hold_now_){rt_unserved_run_.store(0,std::memory_order_relaxed);rt_unserved_since_.store(0,std::memory_order_relaxed);rt_unserved_last_.store(0,std::memory_order_relaxed);}
        if(causes && !rt_hold_now_){rt_fail_magenta_frames_.fetch_add(1,std::memory_order_relaxed);rt_fail_magenta_causes_.fetch_or(causes,std::memory_order_relaxed);
            rt_unserved_run_.store(rt_unserved_run_.load(std::memory_order_relaxed)+1,std::memory_order_relaxed);rt_unserved_run_causes_.store(causes,std::memory_order_relaxed);
            {const uint64_t tick=GetTickCount64();if(!rt_unserved_since_.load(std::memory_order_relaxed))rt_unserved_since_.store(tick,std::memory_order_relaxed);
                rt_unserved_last_.store(tick,std::memory_order_relaxed);}   // the final review's L1: the run's span to its last frame
            if(rt_fail_magenta.load(std::memory_order_relaxed)){const float magenta[4]{1,0,1,1};list->ClearRenderTargetView(CPU(view_->light_rtvs.Get(),rtv_stride_,3),magenta,0,nullptr);}
            else rt_world_black_=true;}}
    Transition(list,view_->light_targets[3].Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    if(rr_now_ && view_==&main_view_){GpuZoneBegin(list,GpuZone::RrGuides);EncodeRrGuides(list,graph);GpuZoneEnd(list,GpuZone::RrGuides);}   // I-0123 RR-a
    for(unsigned i=0;i<3;++i)Transition(list,view_->light_targets[i].Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);

    Transition(list,view_->env_target.Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);
    DepthPlaneTransition(list,view_->depth.Get(),depth_read,D3D12_RESOURCE_STATE_DEPTH_WRITE);
    for(unsigned i=0;i<6;++i)Transition(list,view_->colors[i].Get(),i<4?lit_gb_read_:D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);
}
// I-0122 R4a (L3): the frame's list and tiles uploaded once a frame (the check's second pass and, I-0125 R6b, the reflections read the
// same ranges).
// I-0128 R8c: the list's light grid (rt_light_grid.h's words) right after its records in the same range (RtReflections::Bind reads
// it there), unless no-rt-light-grid.flag.
void SceneRenderer::UploadRtLights(const RtLightList& rt) {
    if(rt_lights_uploaded_.list==&rt)return;
    auto* device=gpu_.device.Get();
    const bool grid=!rt.grid.empty() && rt_light_grid.load(std::memory_order_relaxed);
    const size_t records=sizeof(RtLight)*rt.lights.size(),words=grid?4*rt.grid.size():0;
    const auto lights=frame_uploads_.Allocate(device,(std::max)(records+words,size_t{16}),UploadArena::alignment,false);   // R11e: an empty list still has an address
    if(records)std::memcpy(lights.cpu,rt.lights.data(),records);if(grid)std::memcpy(lights.cpu+records,rt.grid.data(),words);
    const auto tiles=frame_uploads_.Allocate(device,4*rt.tiles.size(),UploadArena::alignment,false);
    std::memcpy(tiles.cpu,rt.tiles.data(),4*rt.tiles.size());
    rt_lights_uploaded_={&rt,lights.gpu,tiles.gpu,grid};
}
// I-0125 R6b (R6-PLAN §3): the deferred surfaces' specular ambient traced (RtReflections) in place of the region draws, on the main
// view's frames with the local lights' pass (its camera and the frame's list): the target cleared as the region draws clear it, the
// regions as one zeroed row (UploadRtRegions, count 0, unread since I-0132 R11e: it keeps the root's t4 bound) in this frame's upload, the PT level, every
// surface's lobe sampled (R11d, D-0262: with or without DS3's regions), R6c with RR's
// specular hit distance (rr_targets_[4], cleared and made writable by the caller). False: the caller draws the region pass (a menu view's regions, else a cleared target)
// (D-0301; a failure turns the pass off for the session). The lighting's heap is set again after it.
bool SceneRenderer::EncodeRtReflections(ID3D12GraphicsCommandList* list,const NativeLightingInput& input) {
    auto* const heap=rt_heap_.load(std::memory_order_acquire);
    if(!rt_reflect.load(std::memory_order_relaxed) || view_!=&main_view_)return false;   // I-0132 R11d (D-0262): with or without DS3's regions
    // The final review's M3: a frame without the local lights' pass (its camera and list) gets no specular ambient (cleared by the caller), counted.
    if(!rt_lights_now_ || !input.rt_lights || !heap || !heap->IsOpen()){rt_reflect_missed_.fetch_add(1,std::memory_order_relaxed);return false;}
    bool drawn=false;
    try{if(input.light.size()<288)throw std::invalid_argument("rt reflections input");
        auto* device=gpu_.device.Get();
        const auto regions=UploadRtRegions(input,*heap);
        UploadRtLights(*input.rt_lights);
        RtReflectConstants c=RtMainReflectConstants(input);
        const auto target=CPU(view_->light_rtvs.Get(),rtv_stride_,7);constexpr float empty[4]{};list->ClearRenderTargetView(target,empty,0,nullptr);
        const auto read_depth=CPU(view_->dsv.Get(),gpu_.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV),1);
        list->OMSetStencilRef(0xFF);
        // R6c: on the frames RR denoises, the specular hit distance with it (its target cleared and made writable by the caller).
        const D3D12_CPU_DESCRIPTOR_HANDLE distance=rr_now_ && rr_targets_[4] && rr_rtvs_?CPU(rr_rtvs_.Get(),rtv_stride_,3):D3D12_CPU_DESCRIPTOR_HANDLE{};
        rt_reflect_pass_.Encode(device,list,*heap,[&](uint64_t bytes){return frame_uploads_.Allocate(device,bytes,UploadArena::alignment,false);},
            rt_scene_address_,rt_material_table_,rt_material_rows_,rt_lights_uploaded_.lights,regions,native_light_constants_[2],native_light_constants_[3],c,target,read_depth,distance);
        SerFailed();   // I-0132 R11b: a reordered dispatch that could not be prepared turns SER off
        rt_reflect_frames_.fetch_add(1,std::memory_order_relaxed);drawn=true;rr_hit_written_=distance.ptr!=0;
    }catch(const std::exception& error){rt_reflect.store(false,std::memory_order_relaxed);   // its reason for the game thread's row
        try{const std::lock_guard hold(rt_view_lock_);if(rt_reflect_failure_.empty())rt_reflect_failure_=*error.what()?error.what():"unknown";}catch(...){}}
    ID3D12DescriptorHeap* heaps[]{view_->light_views.Get()};list->SetDescriptorHeaps(1,heaps);
    return drawn;
}
// I-0132 R11 (D-0252; Thomas, 2026-11-09 09:24: « NRD, qui s'active quand le RR est désactivé puis passe le relais au RR »): on an NRD
// frame, in EncodeRtReflections' place, NRD's signal pass (the GI's ray and the specular lobe's, the guides and the motion: the
// velocity target readable, the caller's), NRD's dispatches (REBLUR_DIFFUSE_SPECULAR), then the specular's compose: SpecularAcc cleared
// and written, term × the denoised L, where Encode writes (the stencil bit 0x08). True when SpecularAcc was written (the caller clears
// it otherwise). rt_nrd_recorded_ tells EncodeRtGi whether the GI's compose has a denoised radiance this frame (else its frame is
// missed, judged unserved). A frame without its inputs records nothing (EncodeRtGi counts the GI missed); the signal's failure turns
// the GI off for the session (as its own pass's did, R7c), NRD's refusal NRD (FailRtNrd), the compose's the reflections (each fatal,
// I-0132 R11e). The lighting's heap is set again after it.
bool SceneRenderer::EncodeRtNrd(ID3D12GraphicsCommandList* list,const NativeLightingInput& input) {
    auto* const heap=rt_heap_.load(std::memory_order_acquire);
    if(!rt_reflect.load(std::memory_order_relaxed) || !rt_lights_now_ || !input.rt_lights || !heap || !heap->IsOpen() || !rt_scene_address_ || !rt_material_table_ || !rt_material_rows_){
        rt_reflect_missed_.fetch_add(1,std::memory_order_relaxed);return false;}
    bool drawn=false;
    auto* device=gpu_.device.Get();
    const auto upload=[&](uint64_t bytes){return frame_uploads_.Allocate(device,bytes,UploadArena::alignment,false);};
    RtReflectConstants c{};D3D12_GPU_VIRTUAL_ADDRESS regions{};
    try{if(input.light.size()<288 || input.fog.empty())throw std::invalid_argument("rt nrd input");
        regions=UploadRtRegions(input,*heap);
        UploadRtLights(*input.rt_lights);
        c=RtMainReflectConstants(input);c.mode|=rt_gi_diagnostic.load(std::memory_order_relaxed);   // the GI's diagnostics reach the specular's rays too (the final review's L1, recorded)
        if(rt_gi_mean_now_){c.mean=rt_gi_mean_slot_;c.mode|=kRtReflectGiMean|(rt_gi_mean_restart_?kRtReflectGiMeanRestart:0u);}   // I-0126 R7d's bias check
        c.velocity=rt_velocity_slot_;c.velocity_scale=rt_nrd_velocity_scale_;c.current_to_previous=rt_nrd_current_to_previous_;
        c.hit_distance={kRtDenoiseHitDistance[0],kRtDenoiseHitDistance[1],kRtDenoiseHitDistance[2],0};
        D3D12_CPU_DESCRIPTOR_HANDLE inputs[kRtDenoiseInputs]{};for(uint32_t i=0;i<kRtDenoiseInputs;++i)inputs[i]=rt_denoise_.InputRtv(i);
        rt_reflect_pass_.EncodeNrdSignal(device,list,*heap,upload,rt_scene_address_,rt_material_table_,rt_material_rows_,rt_lights_uploaded_.lights,regions,
            native_light_constants_[2],native_light_constants_[3],c,inputs);
    }catch(const std::exception& error){rt_gi.store(false,std::memory_order_relaxed);   // the GI's reason for the game thread's row (R7c: the signal was the GI's pass)
        try{const std::lock_guard hold(rt_view_lock_);if(rt_gi_failure_.empty())rt_gi_failure_=*error.what()?error.what():"unknown";}catch(...){}
        rt_reflect_missed_.fetch_add(1,std::memory_order_relaxed);
        ID3D12DescriptorHeap* heaps[]{view_->light_views.Get()};list->SetDescriptorHeaps(1,heaps);return false;}
    // NRD's dispatches: a refusal (checked before anything is recorded) turns NRD off for the session; this frame then adds no GI and no
    // specular (counted missed, judged unserved); without RR the next ones are unserved too, and 2 s of them in a row (30 frames at
    // least) close the game (I-0132 R11e).
    // The step review's L1: an exception of the recording itself (an allocation) is NRD's refusal too, never out of the lighting.
    std::string reason;bool recorded=false;
    try{recorded=rt_denoise_.Record(list,rt_nrd_camera_,upload,reason);}catch(const std::exception& error){reason=*error.what()?error.what():"nrd record";}
    if(recorded){
        rt_nrd_recorded_=true;rt_nrd_serial_=rt_nrd_serial_now_;rt_nrd_frames_.fetch_add(1,std::memory_order_relaxed);
        try{const auto target=CPU(view_->light_rtvs.Get(),rtv_stride_,7);constexpr float empty[4]{};list->ClearRenderTargetView(target,empty,0,nullptr);
            const auto read_depth=CPU(view_->dsv.Get(),gpu_.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV),1);
            list->OMSetStencilRef(0xFF);
            c.denoised=rt_denoised_spec_slot_;
            rt_reflect_pass_.EncodeSpecCompose(device,list,*heap,upload,rt_scene_address_,rt_material_table_,rt_material_rows_,rt_lights_uploaded_.lights,regions,
                native_light_constants_[2],native_light_constants_[3],c,target,read_depth);
            rt_reflect_frames_.fetch_add(1,std::memory_order_relaxed);drawn=true;
        }catch(const std::exception& error){rt_reflect.store(false,std::memory_order_relaxed);
            try{const std::lock_guard hold(rt_view_lock_);if(rt_reflect_failure_.empty())rt_reflect_failure_=*error.what()?error.what():"unknown";}catch(...){}}}
    else{FailRtNrd(reason.c_str());rt_reflect_missed_.fetch_add(1,std::memory_order_relaxed);}
    ID3D12DescriptorHeap* heaps[]{view_->light_views.Get()};list->SetDescriptorHeaps(1,heaps);
    return drawn;
}
// I-0126 R7a (R7-PLAN §1): the traced diffuse ambient added into GXLightCmbAlbedo's target (light_targets[2], a render target from
// pass 2 to the SSS), the reflections' bindings (the frame's regions and list, uploaded once a frame, the top level, the sets and
// fogs) and camera, GB2's slot; a frame without a top level or its material table adds nothing (its ambient lost, counted: pass 0
// already left it out). Its graph pass declared first, whatever the frame (the plan key holds it: the step review's H1).
// A failure turns the GI off for the session (the game then closes with a message, I-0132 R11e, NativeRtFatal). The
// lighting's heap, root and bindings are set again after it.
void SceneRenderer::EncodeRtGi(ID3D12GraphicsCommandList* list,const NativeLightingInput& input,GraphShadow* graph) {
    // I-0126 R7c: on an NRD frame the compose of NRD's denoised radiance into the CmbAlbedo target (I-0132 R11: its signal pass and
    // NRD's dispatches run before GXLightDeferredAO with the specular's, EncodeRtNrd; rt_nrd_recorded_ whether they served this
    // frame); on an RR frame the one additive pass.
    const bool nrd=rt_nrd_now_;
    if(graph){graph->Pass("rt_gi",PassKind::Lighting);graph->Use(view_->light_targets[2].Get(),ResourceAccess::ReadWrite,D3D12_RESOURCE_STATE_RENDER_TARGET);
        for(auto* resource:{view_->colors[0].Get(),view_->colors[1].Get(),view_->colors[2].Get(),view_->colors[3].Get()})graph->Use(resource,ResourceAccess::Read,lit_gb_read_);   // I-0132 R11b: the lighting phase's (GbReadState)
        graph->Use(view_->depth.Get(),ResourceAccess::Read,lit_depth_read_,{0,1});
        if(rt_gi_mean_now_ && !nrd)graph->Use(rt_gi_mean_target_.Get(),ResourceAccess::ReadWrite,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);}   // I-0126 R7d: the bias check (NRD's frames: EncodeRtNrd's)
    auto* const heap=rt_heap_.load(std::memory_order_acquire);
    if(!heap || !heap->IsOpen() || !rt_scene_address_ || !rt_material_table_ || !rt_material_rows_ || !input.rt_lights || (nrd && !rt_nrd_recorded_)){rt_gi_missed_.fetch_add(1,std::memory_order_relaxed);
        rt_restir_serial_=UINT64_MAX;rt_gi_mean_frame_=UINT64_MAX;   // I-0126 R7d: nothing written this frame: no history, the mean restarted (the step review's L1)
        return;}
    // I-0141 tranche 27 (a bounded run's share variant): the traced diffuse ambient's share taken out, the frame counted served
    // (NRD's signal and dispatches already recorded keep its history; ReSTIR's and the bias check's restart as on a missed frame).
    const bool share_off=(share_variant_off.load(std::memory_order_relaxed)&kShareRendererNoGi)!=0;
    if(share_off){rt_gi_frames_.fetch_add(1,std::memory_order_relaxed);rt_restir_serial_=UINT64_MAX;rt_gi_mean_frame_=UINT64_MAX;}
    else try{if(input.light.size()<288 || input.fog.empty())throw std::invalid_argument("rt gi input");
        auto* device=gpu_.device.Get();
        const auto regions=UploadRtRegions(input,*heap);
        UploadRtLights(*input.rt_lights);
        RtReflectConstants c=RtMainReflectConstants(input);c.gb2=rt_main_views_[rt_gb2_view];c.mode|=rt_gi_diagnostic.load(std::memory_order_relaxed);
        if(rt_gi_mean_now_){c.mean=rt_gi_mean_slot_;c.mode|=kRtReflectGiMean|(rt_gi_mean_restart_?kRtReflectGiMeanRestart:0u);}   // I-0126 R7d's bias check
        const auto upload=[&](uint64_t bytes){return frame_uploads_.Allocate(device,bytes,UploadArena::alignment,false);};
        // I-0126 R7d: on a PT GI frame, ReSTIR GI's three draws in place of the one ray's.
        if(!nrd && rt_restir_now_){c.eye_previous=rt_restir_eye_previous_;c.current_to_previous=rt_restir_current_to_previous_;
            c.restir[2]=rt_restir_stages.load(std::memory_order_relaxed)|(rt_restir_history_?kRtRestirHistory:0u)|((rt_restir_serial_&1)?kRtRestirOdd:0u);
            try{rt_reflect_pass_.EncodeGiRestir(device,list,*heap,upload,rt_scene_address_,rt_material_table_,rt_material_rows_,rt_lights_uploaded_.lights,regions,
                    native_light_constants_[2],native_light_constants_[3],c,CPU(view_->light_rtvs.Get(),rtv_stride_,2));
                SerFailed();   // I-0132 R11b
                rt_restir_frames_.fetch_add(1,std::memory_order_relaxed);}
            catch(const std::exception& error){rt_restir.store(false,std::memory_order_relaxed);   // the next frames the one ray; this one adds none (counted missed)
                try{const std::lock_guard hold(rt_view_lock_);if(rt_restir_failure_.empty())rt_restir_failure_=*error.what()?error.what():"unknown";}catch(...){}
                rt_gi_missed_.fetch_add(1,std::memory_order_relaxed);rt_gi_mean_frame_=UINT64_MAX;
                rt_reflect_pass_.ReleaseRestir();}}   // the step review's L7: its buffers go (the frames that used them hold them)
        else if(!nrd)rt_reflect_pass_.EncodeGi(device,list,*heap,upload,rt_scene_address_,rt_material_table_,rt_material_rows_,rt_lights_uploaded_.lights,regions,
            native_light_constants_[2],native_light_constants_[3],c,CPU(view_->light_rtvs.Get(),rtv_stride_,2));
        else{c.denoised=rt_denoised_slot_;   // I-0132 R11: NRD's signal and dispatches already recorded (EncodeRtNrd)
            rt_reflect_pass_.EncodeGiCompose(device,list,*heap,upload,rt_scene_address_,rt_material_table_,rt_material_rows_,rt_lights_uploaded_.lights,regions,
                native_light_constants_[2],native_light_constants_[3],c,CPU(view_->light_rtvs.Get(),rtv_stride_,2));}
        rt_gi_frames_.fetch_add(1,std::memory_order_relaxed);
    }catch(const std::exception& error){rt_gi.store(false,std::memory_order_relaxed);rt_gi_missed_.fetch_add(1,std::memory_order_relaxed);   // its reason for the game thread's row; counted missed (I-0132 R11d, its fixes' step review L1)
        try{const std::lock_guard hold(rt_view_lock_);if(rt_gi_failure_.empty())rt_gi_failure_=*error.what()?error.what():"unknown";}catch(...){}
        rt_restir_serial_=UINT64_MAX;rt_gi_mean_frame_=UINT64_MAX;}   // I-0126 R7d
    ID3D12DescriptorHeap* heaps[]{view_->light_views.Get()};list->SetDescriptorHeaps(1,heaps);
    list->SetGraphicsRootSignature(native_light_root_.Get());
    for(unsigned i=0;i<4;++i)list->SetGraphicsRootConstantBufferView(i,native_light_constants_[i]);
    list->SetGraphicsRootConstantBufferView(7,native_light_constants_[1]);
    list->SetGraphicsRootDescriptorTable(8,GPU(view_->light_views.Get(),binding_stride_,light_passes*light_pass_views));
    list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);list->IASetVertexBuffers(0,1,&native_light_quad_view_);list->IASetIndexBuffer(nullptr);
}
// I-0126 R7b (R7-PLAN §3): the radiance cache's update and resolve on a GI frame (the main view's), before the reflections, the water
// and the GI query it: the reflections' bindings (the frame's regions and list, uploaded once a frame, the top level, the sets and fogs)
// and camera, with R7a's diagnostic bits (the cache built as the GI's hits are lit). Its graph pass declared on every GI frame (the plan
// key holds it, as the GI's): the depth, GB0 and GB3 read; the cache's buffers are the pass's own (UAV barriers, out of the graph). A
// frame without a top level, its table or the list leaves the cache unread (nothing hits it; no baked ambient since I-0132 R11d); a failure turns the cache off
// for the session. The viewport, scissor and heap are the caller's to set again (the region pass sets them).
void SceneRenderer::EncodeRtCache(ID3D12GraphicsCommandList* list,const NativeLightingInput& input,GraphShadow* graph) {
    if(graph){graph->Pass("rt_cache",PassKind::Lighting);
        for(auto* resource:{view_->colors[0].Get(),view_->colors[3].Get()})graph->Use(resource,ResourceAccess::Read,lit_gb_read_);   // I-0132 R11b
        graph->Use(view_->depth.Get(),ResourceAccess::Read,lit_depth_read_,{0,1});}
    auto* const heap=rt_heap_.load(std::memory_order_acquire);
    if(!rt_cache.load(std::memory_order_relaxed) || !heap || !heap->IsOpen() || !rt_scene_address_ || !rt_material_table_ || !rt_material_rows_ || !input.rt_lights)return;
    try{if(input.light.size()<288 || input.fog.empty())throw std::invalid_argument("rt cache input");
        auto* device=gpu_.device.Get();
        const auto regions=UploadRtRegions(input,*heap);
        UploadRtLights(*input.rt_lights);
        RtReflectConstants c=RtMainReflectConstants(input);c.mode|=rt_gi_diagnostic.load(std::memory_order_relaxed);
        rt_reflect_pass_.EncodeCache(device,list,*heap,[&](uint64_t bytes){return frame_uploads_.Allocate(device,bytes,UploadArena::alignment,false);},
            rt_scene_address_,rt_material_table_,rt_material_rows_,rt_lights_uploaded_.lights,regions,native_light_constants_[2],native_light_constants_[3],c);
        rt_cache_now_=true;rt_cache_frames_.fetch_add(1,std::memory_order_relaxed);
    }catch(const std::exception& error){rt_cache.store(false,std::memory_order_relaxed);   // its reason for the game thread's row
        try{const std::lock_guard hold(rt_view_lock_);if(rt_cache_failure_.empty())rt_cache_failure_=*error.what()?error.what():"unknown";}catch(...){}}
    ID3D12DescriptorHeap* heaps[]{view_->light_views.Get()};list->SetDescriptorHeaps(1,heaps);
}
// I-0129 R9a (R9-PLAN §2): the traced probes on the main view's traced frames (the top level, its table and the list): the frame's
// requests (the first kRtProbesMost) traced with the reflections' bindings and constants (the cache's answers at the hits on a GI
// frame, the sun's rays where the frame traces it), their results copied into this frame slot's readback, which the frame's end
// hands to the slot's completions with the requests and the frame's bits. Out of the frame graph (the pass's own buffers, a
// transition each). A failure turns the probes off for the session.
void SceneRenderer::EncodeRtProbes(ID3D12GraphicsCommandList* list,const NativeLightingInput& input) {
    rt_probe_pending_={};
    auto* const heap=rt_heap_.load(std::memory_order_acquire);
    if(!heap || !heap->IsOpen() || !rt_scene_address_ || !rt_material_table_ || !rt_material_rows_ || !input.rt_probes || input.rt_probes->requests.empty() ||
       !input.rt_probes->states || input.rt_probes->keys.size()!=input.rt_probes->requests.size())return;
    try{if(input.light.size()<288 || input.fog.empty())throw std::invalid_argument("rt probes input");
        auto* device=gpu_.device.Get();
        // The final review's L1: what the frame would trace first (FillRtReflectFrame's sun rule), nothing uploaded for nothing.
        {RtReflectConstants probe{};FillRtReflectFrame(probe,input);if(!rt_cache_now_ && !(probe.mode&kRtReflectSun))return;}
        const auto regions=UploadRtRegions(input,*heap);
        if(input.rt_lights && rt_lights_uploaded_.list!=input.rt_lights.get())UploadRtLights(*input.rt_lights);
        RtReflectConstants c=RtMainReflectConstants(input);
        const uint32_t bits=(rt_cache_now_?kRtProbeAmbient:0u)|((c.mode&kRtReflectSun)?kRtProbeSun:0u);   // not zero: checked above
        const auto& requests=input.rt_probes->requests;const auto count=std::min<size_t>(requests.size(),kRtProbesMost);
        auto* const readback=rt_reflect_pass_.EncodeProbes(device,list,*heap,[&](uint64_t bytes){return frame_uploads_.Allocate(device,bytes,UploadArena::alignment,false);},
            rt_scene_address_,rt_material_table_,rt_material_rows_,input.rt_lights?rt_lights_uploaded_.lights:0,regions,native_light_constants_[2],native_light_constants_[3],c,
            {requests.data(),count},frames_.CurrentIndex(),gpu_.CompletedValue());
        rt_probe_pending_={readback,input.rt_probes,uint32_t(count),bits};rt_probe_frames_.fetch_add(1,std::memory_order_relaxed);
    }catch(const std::exception& error){rt_probes.store(false,std::memory_order_relaxed);rt_probe_pending_={};
        try{const std::lock_guard hold(rt_view_lock_);if(rt_probe_failure_.empty())rt_probe_failure_=*error.what()?error.what():"unknown";}catch(...){}}
    ID3D12DescriptorHeap* heaps[]{view_->light_views.Get()};list->SetDescriptorHeaps(1,heaps);
}
// I-0125 R6c: the frame's regions for the traced passes' t4. I-0132 R11e (its step review's M3): since the region cubes' sampling left
// rt_reflect.hlsl (the fallback's), nothing reads them: one zeroed region's room, once a frame, keeps the root's t4 bound (the count
// passed is 0); no cube view copied, nothing that can fail.
D3D12_GPU_VIRTUAL_ADDRESS SceneRenderer::UploadRtRegions(const NativeLightingInput&,BindlessHeap&) {
    if(rt_reflect_frame_==frames_.Frames() && rt_reflect_regions_)return rt_reflect_regions_;
    const auto regions=frame_uploads_.Allocate(gpu_.device.Get(),kRtReflectRegionBytes,UploadArena::alignment,false);
    std::memset(regions.cpu,0,kRtReflectRegionBytes);
    rt_reflect_frame_=frames_.Frames();rt_reflect_regions_=regions.gpu;
    return regions.gpu;
}
// The main view's constants for the reflections, the cache and the GI (the final review's L4: one copy): the local lights' camera
// rows (rt_lights_camera_), the extent, the depth's, GB0's, GB1's and GB3's slots, the frame's (FillRtReflectFrame), the list's count.
// I-0132 R11b: a resource's read state in the lighting phase (lit_gb_read_ for GB0 to GB3, PIXEL_SHADER_RESOURCE for the others).
D3D12_RESOURCE_STATES SceneRenderer::GbReadState(ID3D12Resource* resource) const {
    for(unsigned i=0;i<4;++i)if(view_->colors[i].Get()==resource)return lit_gb_read_;
    return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
}
// The step review's M1: a reordered dispatch that could not be prepared (the frame drawn with the pixel shader): SER off for the session.
void SceneRenderer::SerFailed() {
    std::string failure=rt_reflect_pass_.TakeSerFailure();if(failure.empty())return;
    rt_ser.store(false,std::memory_order_relaxed);
    try{const std::lock_guard hold(rt_view_lock_);if(rt_ser_failure_.empty())rt_ser_failure_=failure;}catch(...){}
}
RtReflectConstants SceneRenderer::RtMainReflectConstants(const NativeLightingInput& input) const {
    const auto& camera=rt_lights_camera_;RtReflectConstants c{};c.right=camera.right;c.up=camera.up;c.forward=camera.forward;c.position=camera.position;c.jitter=camera.jitter;
    c.width=camera.width;c.height=camera.height;c.depth=camera.depth;c.gb0=camera.gb0;c.gb1=camera.gb1;c.gb3=camera.gb3;
    FillRtReflectFrame(c,input);c.lights=input.rt_lights?uint32_t(input.rt_lights->lights.size()):0;
    if(c.lights && rt_lights_uploaded_.list==input.rt_lights.get() && rt_lights_uploaded_.grid)c.mode|=kRtReflectLightGrid;   // I-0128 R8c (uploaded by the caller)
    return c;}
void SceneRenderer::FillRtReflectFrame(RtReflectConstants& c,const NativeLightingInput& input) const {
    c.seed=uint32_t(rt_seed_serial_);c.regions=0;   /* R11e: the regions unread (UploadRtRegions) */c.sets=uint32_t(input.light.size()/288);
    const bool pt=rt_pt_now_ && rr_now_;   // I-0132 R12f: the frame's level
    c.mode=kRtReflectTraced|(pt?kRtReflectPt:0u);   // I-0132 R11d (D-0262): every surface's lobe sampled at both levels (no roughness cut, no mirror-only frames)
    // I-0128 R8b: the PT level's paths (their vertices from startup; the reference without the cache, a diagnostic).
    c.bounces=pt?std::clamp<uint32_t>(rt_bounces_now_,1,kRtPathVerticesMost):1;
    c.gi_paths=pt?std::clamp<uint32_t>(rt_gi_paths_now_,1,kRtGiPathsMost):1;   // I-0132 R12g: the GI's paths a pixel at the PT level
    if(pt && rt_path_reference.load(std::memory_order_relaxed))c.mode|=kRtReflectPathNoCache;
    else if(pt && c.bounces>1 && rt_roulette.load(std::memory_order_relaxed))c.mode|=kRtReflectRoulette;   // I-0132 R11a
    c.mode|=rt_emissive_bits.load(std::memory_order_relaxed);   // I-0132 R11d's test tools (0 by default)
    c.cone=rr_now_?std::cos(kRtSunAngularRadius):0.f;c.anisotropy=rt_reflect_anisotropy_;c.texture_bias=rt_reflect_bias_;c.time=rt_reflect_time_;   // R11d: the dominant rows and FC_ShadowColor no longer read by the rays
    // The step review's M2: the hits' sun shadow along the frame's traced shadow's direction, on its frames (else unmasked, as the
    // original's white mask); w the local lights' sphere share for the PT level's rays (L4).
    // The final review's L3: a direction finite and not zero, normalised (RtSunConstantsOf's rule), else the hits unmasked.
    if(const float len=std::sqrt(rt_reflect_sun_[0]*rt_reflect_sun_[0]+rt_reflect_sun_[1]*rt_reflect_sun_[1]+rt_reflect_sun_[2]*rt_reflect_sun_[2]);rt_sun_now_ && std::isfinite(len) && len>0){
        for(int i=0;i<3;++i)c.sun[i]=rt_reflect_sun_[i]/len;c.mode|=kRtReflectSun;}
    c.sun[3]=kRtLightRadiusShare;
    if(rt_sky_witness.load(std::memory_order_relaxed))c.mode|=kRtReflectSkyWitness;   // I-0125 R6d: a diagnostic
    if(rt_cache_now_)c.mode|=kRtReflectCache;   // I-0126 R7b: the hits' ambient from the cache resolved this frame
    if(rt_ser_now_)c.ser=rt_reflect_pass_.SerSlots();   // I-0132 R11b: the path programmes reordered (Encode's and EncodeGiRestir's)
}
// I-0125 R6c (R6-PLAN §6): the water's g_ReflectionTexture traced into the frame's reflection target (RtReflections::EncodeWater),
// after the specular ambient's pass and before the forward rows that sample it: the regions' buffer and cube slots, the frame's light
// list (when the local lights' pass has one), the top level and the sets shared; the water's mirror one ray along R (no lobe). The
// target goes to RENDER_TARGET and back whatever happens (the plan key holds the pass); a failure turns the pass off for the session
// (the water then samples the black default). The lighting's heap, viewport and scissor are set again after it.
void SceneRenderer::EncodeRtWater(ID3D12GraphicsCommandList* list,const NativeLightingInput& input,GraphShadow* graph,const D3D12_VIEWPORT& viewport,const D3D12_RECT& scissor) {
    if(!rt_water_now_ || view_!=&main_view_)return;
    auto* const target=rt_water_target_.Get();auto* const heap=rt_heap_.load(std::memory_order_acquire);
    Transition(list,target,kRtWaterState,D3D12_RESOURCE_STATE_RENDER_TARGET);
    if(graph){graph->Pass("rt_water",PassKind::Lighting);graph->Use(target,ResourceAccess::Write,D3D12_RESOURCE_STATE_RENDER_TARGET);}
    try{if(!heap || !heap->IsOpen() || input.light.size()<288)throw std::runtime_error("rt water input");
        auto* device=gpu_.device.Get();
        const auto regions=UploadRtRegions(input,*heap);
        D3D12_GPU_VIRTUAL_ADDRESS lights{};uint32_t count{};
        if(rt_lights_now_ && input.rt_lights){UploadRtLights(*input.rt_lights);lights=rt_lights_uploaded_.lights;count=uint32_t(input.rt_lights->lights.size());}
        const auto& camera=rt_water_camera_;RtReflectConstants c{};c.right=camera.right;c.up=camera.up;c.forward=camera.forward;c.position=camera.position;c.jitter=camera.jitter;
        c.width=camera.width;c.height=camera.height;
        FillRtReflectFrame(c,input);c.lights=count;
        if(count && rt_lights_uploaded_.grid)c.mode|=kRtReflectLightGrid;   // I-0128 R8c
        const auto d=target->GetDesc();c.plane=rt_water_plane_;c.water_width=uint32_t(d.Width);c.water_height=d.Height;
        rt_reflect_pass_.EncodeWater(device,list,*heap,[&](uint64_t bytes){return frame_uploads_.Allocate(device,bytes,UploadArena::alignment,false);},
            rt_scene_address_,rt_material_table_,rt_material_rows_,lights,regions,native_light_constants_[2],native_light_constants_[3],c,CPU(rt_water_rtvs_.Get(),rtv_stride_,0));
        rt_water_drawn_.store(target,std::memory_order_release);rt_water_frames_.fetch_add(1,std::memory_order_relaxed);
    }catch(const std::exception& error){rt_water.store(false,std::memory_order_relaxed);rt_water_drawn_.store(nullptr,std::memory_order_release);   // L3: no stale address
        try{const std::lock_guard hold(rt_view_lock_);if(rt_water_failure_.empty())rt_water_failure_=*error.what()?error.what():"unknown";}catch(...){}}
    Transition(list,target,D3D12_RESOURCE_STATE_RENDER_TARGET,kRtWaterState);
    ID3D12DescriptorHeap* heaps[]{view_->light_views.Get()};list->SetDescriptorHeaps(1,heaps);
    list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
}
// I-0122 R4a: the native local lights' pass (RtLocalLights) inside the lighting's ambient and sun pass, which declares its reads (GB0,
// GB1, GB3, the depth) and writes (the pair): the frame's list uploaded, the camera RtLightsFrame took; R4b: the frame's top level (built
// before the lighting, as the traced sun's) with its material table, the shadowed lights traced when there is one. Whether it drew:
// a failure turns it off for the session and the caller draws the batches instead. The lighting's heap, root signature, constants, topology and
// quad set again after it, for the batches and passes that follow.
bool SceneRenderer::EncodeRtLights(ID3D12GraphicsCommandList* list,const NativeLightingInput& input) {
    auto* const heap=rt_heap_.load(std::memory_order_acquire);bool drawn=false;
    try{if(!rt_lights.load(std::memory_order_relaxed))return false;   // off since an earlier failure (nothing set on the list yet)
        if(!heap || !heap->IsOpen() || !input.rt_lights)throw std::runtime_error("rt local lights frame");
        auto* device=gpu_.device.Get();const auto& rt=*input.rt_lights;
        if(rt.columns!=rt_lights_camera_.columns || rt.tiles.size()<2ull*rt.columns*rt.rows)throw std::runtime_error("rt local lights tiles");
        UploadRtLights(rt);
        // I-0132 R11e (its step review's H1): an empty list (a view without any local light) is served: nothing to shade, the list
        // uploaded for the reflections and the GI.
        if(!rt.lights.empty()){auto constants=rt_lights_camera_;   // I-0126 R7c (the final review's M3): no AO term (D-0251), no t33 slot
        // I-0123 RR-b: the traced lights' spheres on an RR frame.
        if(rr_now_){constants.offset[3]=kRtLightRadiusShare;constants.seed=uint32_t(rr_serial_);}
        // R4c: the PT level on a frame RR denoises with a top level.
        const bool pt=rt_pt_now_ && rr_now_ && rt_scene_address_;
        const D3D12_CPU_DESCRIPTOR_HANDLE accumulation[2]{CPU(view_->light_rtvs.Get(),rtv_stride_,0),CPU(view_->light_rtvs.Get(),rtv_stride_,1)};
        rt_lights_pass_.Encode(device,list,*heap,[&](uint64_t bytes){return frame_uploads_.Allocate(device,bytes,UploadArena::alignment,false);},
            rt_lights_uploaded_.lights,rt_lights_uploaded_.tiles,rt_scene_address_,rt_material_table_,rt_material_rows_,constants,accumulation,pt);
        if(pt && !rt_lights_pass_.PtRefused())rt_pt_frames_.fetch_add(1,std::memory_order_relaxed);}
        drawn=true;
    }catch(const std::exception& error){FailRtLights(error.what());}
    ID3D12DescriptorHeap* heaps[]{view_->light_views.Get()};list->SetDescriptorHeaps(1,heaps);
    list->SetGraphicsRootSignature(native_light_root_.Get());
    for(unsigned i=0;i<4;++i)list->SetGraphicsRootConstantBufferView(i,native_light_constants_[i]);
    list->SetGraphicsRootConstantBufferView(7,native_light_constants_[1]);
    list->SetGraphicsRootDescriptorTable(8,GPU(view_->light_views.Get(),binding_stride_,light_passes*light_pass_views));
    list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);list->IASetVertexBuffers(0,1,&native_light_quad_view_);list->IASetIndexBuffer(nullptr);
    return drawn;
}
// I-0123 RR-a (RR-PLAN §1): Ray Reconstruction's guides (RrGuides: GB0 to GB3 and the depth through rt_main_views_, readable here),
// then the lit HDR as the composite left it (before every forward row and effect) copied as the colour before transparency. The
// targets rest readable between frames (kRrGuideState); the heap the guides' pass set stays (the lighting draws nothing after it). A
// failure latches RR off for the session; its targets keep their states, so the frame's barriers and declared uses stand.
void SceneRenderer::EncodeRrGuides(ID3D12GraphicsCommandList* list,GraphShadow* graph) {
    auto* const heap=rt_heap_.load(std::memory_order_acquire);auto* const hdr=view_->light_targets[3].Get();auto* const before=rr_targets_[3].Get();
    const auto depth_read=lit_depth_read_;   // I-0132 R11b: the lighting phase's
    for(uint32_t i=0;i<3;++i)Transition(list,rr_targets_[i].Get(),kRrGuideState,D3D12_RESOURCE_STATE_RENDER_TARGET);
    if(graph){graph->Pass("rr_guides",PassKind::Lighting);for(uint32_t i=0;i<3;++i)graph->Use(rr_targets_[i].Get(),ResourceAccess::Write,D3D12_RESOURCE_STATE_RENDER_TARGET);
        for(size_t i=0;i<4;++i)graph->Use(view_->colors[i].Get(),ResourceAccess::Read,lit_gb_read_);
        graph->Use(view_->depth.Get(),ResourceAccess::Read,depth_read,{0,1});}
    try{if(!heap || !heap->IsOpen())throw std::runtime_error("rr guides heap");
        const D3D12_CPU_DESCRIPTOR_HANDLE rtvs[3]{CPU(rr_rtvs_.Get(),rtv_stride_,0),CPU(rr_rtvs_.Get(),rtv_stride_,1),CPU(rr_rtvs_.Get(),rtv_stride_,2)};
        rr_guides_.Encode(gpu_.device.Get(),list,*heap,[&](uint64_t bytes){return frame_uploads_.Allocate(gpu_.device.Get(),bytes,UploadArena::alignment,false);},rr_camera_,rtvs);}
    catch(const std::exception& error){LatchRrOff(rr_serial_,std::string("guides: ")+error.what());}
    for(uint32_t i=0;i<3;++i)Transition(list,rr_targets_[i].Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,kRrGuideState);
    Transition(list,hdr,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);Transition(list,before,kRrGuideState,D3D12_RESOURCE_STATE_COPY_DEST);
    if(graph){graph->Pass("rr_before_transparency",PassKind::Effect);graph->Use(hdr,ResourceAccess::Read,D3D12_RESOURCE_STATE_COPY_SOURCE);
        graph->Use(before,ResourceAccess::Write,D3D12_RESOURCE_STATE_COPY_DEST);}
    list->CopyResource(before,hdr);
    Transition(list,before,D3D12_RESOURCE_STATE_COPY_DEST,kRrGuideState);Transition(list,hdr,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}
// D-0180: the lighting views' t32 (every pass block's slot 10) names the mask stage's mask (stage) or the neutral white, t33 (after
// the pass blocks) the neutral white always (I-0126 R7c, D-0251). The frames that read the former views keep the former heap (a
// change happens at the first mask frame, after a resize and when the mask stops).
// I-0126 R7a: the mask views this frame wants (`stage`: the mask stage's t32; `gi`: pass 0's GB4 the neutral black), written
// again only when they change or the heap or the stage's targets do; neither: the views back to their defaults once.
void SceneRenderer::SyncNativeMaskViews(bool stage,bool gi) {
    const bool bound=mask_views_.heap==view_->light_views.Get() && mask_views_.generation==view_->light_generation;
    if(stage || gi){if(!bound || mask_views_.stage!=stage || mask_views_.gi!=gi || (stage && mask_views_.targets!=mask_.TargetGeneration()))BindNativeMaskViews(stage,gi);}
    else if(mask_views_.heap==view_->light_views.Get())BindNativeMaskViews(false,false);
}
void SceneRenderer::BindNativeMaskViews(bool stage,bool gi) {
    auto* device=gpu_.device.Get();
    // Axis 2: the mask views into the mirror, then the mirror's fixed part into a fresh shader-visible heap, the former heap
    // retired by ticket (the frames in flight keep reading it).
    const auto view=[&](uint32_t index,ID3D12Resource* resource){
        D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=DXGI_FORMAT_R8G8B8A8_UNORM;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
        srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;
        device->CreateShaderResourceView(resource,&srv,CPU(view_->light_views_cpu.Get(),binding_stride_,index));};
    for(uint32_t pass=0;pass<light_passes;++pass)view(pass*light_pass_views+10,stage?mask_.Mask():native_light_neutral_[0].Get());
    view(light_passes*light_pass_views,native_light_neutral_[0].Get());
    // R7a: pass 0's t6 (GB4, the baked diffuse ambient) the neutral black on the GI frames, GB4 otherwise.
    if(gi)view(6,native_light_neutral_[1].Get());
    else{D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=color_formats[4];srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
        srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;
        device->CreateShaderResourceView(view_->colors[4].Get(),&srv,CPU(view_->light_views_cpu.Get(),binding_stride_,6));}
    RefreshLightViews();
    mask_views_=stage || gi?MaskViews{view_->light_views.Get(),view_->light_generation,stage?mask_.TargetGeneration():UINT64_MAX,stage,gi}:MaskViews{};
}
void SceneRenderer::RefreshLightViews() {
    auto* device=gpu_.device.Get();auto* former=view_->light_views.Get();
    ComPtr<ID3D12DescriptorHeap> fresh;const D3D12_DESCRIPTOR_HEAP_DESC views{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,env_cube_views+env_region_limit*env_range_count,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
    Check(CountedHeap(device)->CreateDescriptorHeap(&views,IID_PPV_ARGS(&fresh)),"native lighting views");
    device->CopyDescriptorsSimple(light_fixed_views,CPU(fresh.Get(),binding_stride_,0),CPU(view_->light_views_cpu.Get(),binding_stride_,0),D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    RetireAfterCurrentRecording(HoldObject(former));view_->light_views=fresh;
    // The mirror holds the mask views: a key that named the former heap names the fresh copy.
    if(mask_views_.heap==former)mask_views_.heap=fresh.Get();
}
// D-0180 (SHADOWS-NATIVE 1.5; I-0126 R7c, D-0251: the SSAO passes that came first are removed): the mask t32: cleared to white
// (the sun's shadow term w = 1 until the cascaded shadow exists) and GXRend_CompositeShadowMask twice, RGB only, by the G-buffer
// stencil bit 0x10 (10CC20: reference 0 with the pair of 10CDD9, then 0x10 with the pair of 10D01E): x the AO (t33, the neutral
// white), y and z the GBuffer1 terms GXLightDeferredAO (x y) and the sun (z) read.
void SceneRenderer::EncodeNativeShadowMask(ID3D12GraphicsCommandList* list,const NativeShadowMaskInput& mask_in,const D3D12_VIEWPORT& viewport,const D3D12_RECT& scissor,
    const NativeModelFrameInput* sun,bool gi) {
    SyncNativeMaskViews(true,gi);
    auto* graph=active_graph_shadow;
    const auto depth_read=lit_depth_read_;   // I-0132 R11b: the lighting phase's
    struct Recorder final:StageRecorder {
        GraphShadow* graph;ID3D12Resource* depth;
        Recorder(GraphShadow* g,ID3D12Resource* d):graph(g),depth(d){}
        void Barrier(ID3D12GraphicsCommandList* l,ID3D12Resource* r,D3D12_RESOURCE_STATES from,D3D12_RESOURCE_STATES to) override {Transition(l,r,from,to);}
        void Pass(const char* name) override {if(graph)graph->Pass(name,PassKind::PostProcess);}
        void Use(ID3D12Resource* r,D3D12_RESOURCE_STATES state,bool write,int plane) override {if(!graph)return;
            const auto access=write?ResourceAccess::Write:ResourceAccess::Read;
            if(r==depth)graph->Use(r,access,state,{plane==1?1u:0u,1});else graph->Use(r,access,state);}
    } recorder(graph,view_->depth.Get());
    // The mask: white, then the two composite draws (the lighting root and quad; t3 GBuffer1 in the sun pass's block, t33 the neutral white).
    auto* mask=mask_.Mask();const auto mask_rtv=mask_.MaskRtv();
    const auto read_depth=CPU(view_->dsv.Get(),gpu_.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV),1);
    // D-0180 C4 (SUN-SHADOWS-C4-STATIC-PREP 1-2): the mask layer first (it leaves the shadow in the mask's w and the packed depth in
    // xyz, which its blur reads), then the composite over its xyz. The atlas is read between the casters and the next frame's.
    // I-0117 R5 (on with the rays): the sun's shadow traced in place of the layer (RtSunFrame chose it with the plan key): its two
    // draws into the mask (OutputColor's value off the G-buffer, the traced term on it), the composite after them as before. A
    // failure turns it off for the session (I-0131 R10a: the next frames' mask cleared white, the sun unshadowed: no raster layer
    // since the cascades' removal); this frame's mask then lit.
    if(sun && rt_sun_now_){auto* const heap=rt_heap_.load(std::memory_order_acquire);
        recorder.Barrier(list,mask,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);
        recorder.Pass("rt_sun_shadow");recorder.Use(view_->depth.Get(),depth_read,false,0);recorder.Use(view_->depth.Get(),D3D12_RESOURCE_STATE_DEPTH_WRITE,false,1);
        recorder.Use(mask,D3D12_RESOURCE_STATE_RENDER_TARGET,true,0);
        // Step review M1, L2: no top level this frame (no instance yet, its build failed), or a camera or sun this frame cannot
        // give: the mask lit everywhere for the frame (w = 1). Final review M1: cleared white, the mask's optimised clear value (the
        // debug layer's mismatch warning closes a run; the composite rewrites RGB on every pixel, as on the frames without the layer).
        RtViewConstants camera{};RtSunConstants constants{};const float lit[4]{1,1,1,1};
        if(!heap || !rt_scene_address_ || !RtViewCameraOf(sun->camera,camera) || !RtSunConstantsOf(camera,sun->rt_sun,constants))list->ClearRenderTargetView(mask_rtv,lit,0,nullptr);
        else try{constants.depth=rt_main_views_[rt_depth_view];if(rt_shadow_witness.load(std::memory_order_relaxed))constants.offset[3]=1;
            // I-0123 RR-b: the penumbra on a frame Ray Reconstruction denoises (rr_now_), sharp otherwise (no noise on screen).
            if(rr_now_){constants.cone=std::cos(kRtSunAngularRadius);constants.seed=uint32_t(sun->serial);}   // step review L2: the frame's serial, not the view's version
            list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
            GpuZoneBegin(list,GpuZone::RtSun);   // I-0128 R8a: the traced sun's draws only
            rt_sun_.Encode(gpu_.device.Get(),list,*heap,[&](uint64_t bytes){return frame_uploads_.Allocate(gpu_.device.Get(),bytes,UploadArena::alignment,false);},
                rt_scene_address_,rt_material_table_,rt_material_rows_,constants,mask_rtv,read_depth,MaskStage::ColorFormat());GpuZoneEnd(list,GpuZone::RtSun);}
        catch(const std::exception& error){FailRtSun(error.what());list->ClearRenderTargetView(mask_rtv,lit,0,nullptr);}   // off for the session; no shadow this frame
        recorder.Barrier(list,mask,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);}
    Transition(list,mask,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);   // review: before the pass
    if(graph){graph->Pass("shadow_mask",PassKind::Lighting);graph->Use(mask,ResourceAccess::Write,D3D12_RESOURCE_STATE_RENDER_TARGET);
        graph->Use(view_->colors[1].Get(),ResourceAccess::Read,lit_gb_read_);
        graph->Use(native_light_neutral_[0].Get(),ResourceAccess::Read,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);   // the white t33
        graph->Use(view_->depth.Get(),ResourceAccess::Read,D3D12_RESOURCE_STATE_DEPTH_WRITE,{1,1});}   // the stencil plane (read by the test)
    const float white[4]{1,1,1,1};if(!sun)list->ClearRenderTargetView(mask_rtv,white,0,nullptr);   // without the layer: no shadow (w = 1)
    ID3D12DescriptorHeap* heaps[]{view_->light_views.Get()};list->SetDescriptorHeaps(1,heaps);
    list->SetGraphicsRootSignature(native_light_root_.Get());
    for(unsigned i=0;i<4;++i)list->SetGraphicsRootConstantBufferView(i,native_light_constants_[i]);
    list->SetGraphicsRootConstantBufferView(7,native_light_constants_[1]);
    list->SetGraphicsRootDescriptorTable(4,GPU(view_->light_views.Get(),binding_stride_,1*light_pass_views));      // t3 GBuffer1
    list->SetGraphicsRootDescriptorTable(5,GPU(view_->light_views.Get(),binding_stride_,1*light_pass_views+9));
    list->SetGraphicsRootDescriptorTable(6,GPU(view_->light_views.Get(),binding_stride_,light_passes*light_pass_views));   // t32: unread by the composite
    list->SetGraphicsRootDescriptorTable(8,GPU(view_->light_views.Get(),binding_stride_,light_passes*light_pass_views));   // t33 the neutral white
    list->OMSetRenderTargets(1,&mask_rtv,FALSE,&read_depth);list->SetPipelineState(native_mask_pso_.Get());
    list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
    list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);list->IASetVertexBuffers(0,1,&native_light_quad_view_);list->IASetIndexBuffer(nullptr);
    for(unsigned draw=0;draw<2;++draw){const auto range=frame_uploads_.Allocate(gpu_.device.Get(),256);std::memcpy(range.cpu,mask_in.composite[draw].data(),32);
        list->SetGraphicsRootConstantBufferView(2,range.gpu);   // b5 cbCompositeShadowMask
        list->OMSetStencilRef(draw?0x10u:0u);list->DrawInstanced(4,1,0,0);}
    Transition(list,mask,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}
// I-0140 tranche 31 (D-0303): a Player menu view's self-shadow mask, at its context's extent, the mask stage's format (the traced
// sun's pipelines are made for that one), shader-readable between frames (its clear value white); made before the view's frame
// graph declares it (RenderNativeOffscreen), its RTV with it. The depth's bindless view (R32 of the D32S8, as the main view's
// MakeRtMainViews) for the context's target generation; the former released against the last submitted ticket (no list submitted
// after it reads it). I-0140 tranche 35: the blur's second target made with the mask (its twin), and the bindless views of both
// made and released with the depth's (three slots, all or none).
void SceneRenderer::MakeSelfShadowMask(BindlessHeap& heap) {
    const auto target=[&](ComPtr<ID3D12Resource>& image,ComPtr<ID3D12DescriptorHeap>& view,const char* what,const char* view_what){
        D3D12_HEAP_PROPERTIES type{};type.Type=D3D12_HEAP_TYPE_DEFAULT;
        const auto desc=Image(view_->width,view_->height,1,MaskStage::ColorFormat(),D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        D3D12_CLEAR_VALUE clear{};clear.Format=desc.Format;for(auto& c:clear.Color)c=1.f;
        ComPtr<ID3D12Resource> made;ComPtr<ID3D12DescriptorHeap> rtv;
        Check(CountedResource(gpu_.device)->CreateCommittedResource(&type,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,&clear,IID_PPV_ARGS(&made)),what);
        const D3D12_DESCRIPTOR_HEAP_DESC rtv_desc{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,1};
        Check(CountedHeap(gpu_.device)->CreateDescriptorHeap(&rtv_desc,IID_PPV_ARGS(&rtv)),view_what);
        gpu_.device->CreateRenderTargetView(made.Get(),nullptr,rtv->GetCPUDescriptorHandleForHeapStart());
        image=std::move(made);view=std::move(rtv);};
    if(!view_->self_mask)target(view_->self_mask,view_->self_mask_rtv,"menu self-shadow mask","menu self-shadow mask view");
    if(!view_->self_blur)target(view_->self_blur,view_->self_blur_rtv,"menu self-shadow blur target","menu self-shadow blur target view");
    if(view_->self_depth_generation==view_->target_generation && view_->self_depth_view!=BindlessIndices::none)return;
    std::array<uint32_t,3> slots{BindlessIndices::none,BindlessIndices::none,BindlessIndices::none};
    for(auto& slot:slots)if((slot=heap.Allocate(gpu_.CompletedValue()))==BindlessIndices::none){
        for(const auto made:slots)if(made!=BindlessIndices::none)try{heap.Release(made,gpu_.LastSubmitted());}catch(...){}
        throw std::runtime_error("menu self-shadow view slots");}
    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;
    gpu_.device->CreateShaderResourceView(view_->depth.Get(),&srv,heap.Cpu(slots[0]));
    srv.Format=MaskStage::ColorFormat();
    gpu_.device->CreateShaderResourceView(view_->self_mask.Get(),&srv,heap.Cpu(slots[1]));gpu_.device->CreateShaderResourceView(view_->self_blur.Get(),&srv,heap.Cpu(slots[2]));
    ReleaseSelfShadowViews(*view_);view_->self_depth_view=slots[0];view_->self_mask_view=slots[1];view_->self_blur_view=slots[2];
    view_->self_depth_generation=view_->target_generation;
}
void SceneRenderer::ReleaseSelfShadowViews(ViewContext& context) noexcept {
    if(context.self_depth_view==BindlessIndices::none)return;
    if(auto* const heap=rt_heap_.load(std::memory_order_acquire))
        for(const auto slot:{context.self_depth_view,context.self_mask_view,context.self_blur_view})try{heap->Release(slot,gpu_.LastSubmitted());}catch(...){}
    context.self_depth_view=context.self_mask_view=context.self_blur_view=BindlessIndices::none;context.self_depth_generation=UINT64_MAX;
}
void SceneRenderer::FailMenuShadow(const char* why) noexcept {
    if(menu_shadow_failed_.load(std::memory_order_acquire))return;
    try{menu_shadow_failure_=why && *why?why:"unknown";}catch(...){}
    menu_shadow_failed_.store(true,std::memory_order_release);
}
// I-0140 tranche 31 (D-0303), before the view's graph is declared (the review of the fixes' fixes, M1: the declaration then says
// whether the lighting reads the mask): the view's t32 (every pass block's slot 10) names the image of the frame, its mask on a
// traced frame and while SelfMaskHeld, the neutral white otherwise (the step review's L3: a context never traced keeps the former
// views), once a key (the mirror, its light generation, the image): written into the mirror and copied into a fresh shader-visible
// heap (RefreshLightViews: the former retired by ticket; no command recorded). A mirror NativeLightingResources made again already
// names the neutral, published. A refused rewrite turns the menu self-shadow off, the frame untraced, and leaves the mirror's slot
// 10 and the bound image as the published views name them (the last step review's L2, its review's L1); after a failure the bound
// image of the same mirror is kept, not tried again every frame (its L3). `traced`: the view's scene built (EncodeMenuRtScene) with
// the mask, its depth view and the open heap; the result, self_traced_now_, is the frame's.
void SceneRenderer::BindSelfShadowViews(bool traced) {
    auto* const heap=rt_heap_.load(std::memory_order_acquire);
    self_traced_now_=traced && view_->self_mask && view_->self_depth_view!=BindlessIndices::none && menu_rt_scene_ && heap && heap->IsOpen();
    // Once the menu self-shadow failed, nothing held (the review of 43c39bc27, L1): the neutral, its rewrite tried once a mirror.
    const bool held=!self_traced_now_ && !menu_shadow_failed_.load(std::memory_order_acquire) && SelfMaskHeld();
    if(self_traced_now_)view_->self_traced_frame=frames_.Numbered();   // also a frame whose rewrite is refused below (the session then failed: nothing held)
    if(!view_->self_mask && !view_->self_bound_mask)return;   // a context never traced: its views are the former ones
    auto* const neutral=native_light_neutral_[0].Get();auto* const named=self_traced_now_ || held?view_->self_mask.Get():neutral;
    const auto bind=[&](ID3D12Resource* image){view_->self_bound_mirror=view_->light_views_cpu.Get();view_->self_bound_generation=view_->light_generation;
        view_->self_bound_mask=image;view_->self_bind_refused=false;};
    const bool fresh=view_->self_bound_mirror!=view_->light_views_cpu.Get() || view_->self_bound_generation!=view_->light_generation;
    if(fresh && named==neutral){bind(neutral);return;}
    // I-0140 tranche 34: a held recording counted once its views name the mask (its step review's L1: not when the rename is refused),
    // a rename to the neutral once it succeeded (`menu_shadow_held_`, `menu_shadow_neutral_`: events, not frames or views).
    if(!fresh && view_->self_bound_mask==named){if(held)menu_shadow_held_.fetch_add(1,std::memory_order_relaxed);return;}
    if(!fresh && view_->self_bind_refused)return;
    const auto name=[&](ID3D12Resource* image){
        D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=image==view_->self_mask.Get()?MaskStage::ColorFormat():DXGI_FORMAT_R8G8B8A8_UNORM;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
        srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;
        for(uint32_t pass=0;pass<light_passes;++pass)gpu_.device->CreateShaderResourceView(image,&srv,CPU(view_->light_views_cpu.Get(),binding_stride_,pass*light_pass_views+10));};
    try{name(named);RefreshLightViews();bind(named);
        if(held)menu_shadow_held_.fetch_add(1,std::memory_order_relaxed);else if(named==neutral)menu_shadow_neutral_.fetch_add(1,std::memory_order_relaxed);}
    catch(const std::exception& error){FailMenuShadow(error.what());self_traced_now_=false;
        if(fresh){name(neutral);bind(neutral);}   // the remade views, which name the neutral
        else{name(SelfMaskBound()?view_->self_mask.Get():neutral);view_->self_bind_refused=true;}}   // the former views, not retried on this mirror
}
// The mask's pass, on every frame whose graph declared the mask (RenderNativeOffscreen: a traced view, `input`, or SelfMaskBound,
// its views naming the mask), so the passes follow the plan's key and every declared mask is the one the lighting reads (t32
// SelfMaskBound) (the review of the Lows' fixes, H1 and M1, and its review's M1): on a traced frame OutputColor's white where the
// G-buffer stencil bit 0x08 is clear and the traced sun term where it is set (RtSunShadow, `neutral`: its xyz white, as the neutral
// image), cleared white otherwise (a trace that cannot run, a held frame, a refused rewrite away from the mask). The traced sun's
// offsets and its sharp ray are the main view's.
// I-0140 tranche 35 (the penumbra, tranche 31's left item): the original softens a menu view's shadow by its screen-space blur
// passes (1D7850, GXScreenSpaceShadow_Blur; the menu ids' ShadowParam: 4 passes of radius 4, id 11 radius 8, plus the quality row's),
// far more than by its PCF taps (a texel of a 1024 to 2048 map over about a metre). So on a traced frame with `self_blur_passes_now_`
// passes the trace writes the target the parity names (the blur target when the count is odd, 1B8F30's rule, so that the last pass
// lands in the mask), xyz the G-buffer's flag (`neutral` 2), then each pass ("self_shadow_blur", its own graph pass: its source read,
// its destination written) the fill and the blur of the other target (RtSunShadow::EncodeBlur), the last with xyz white. A pass that
// cannot run (a failed trace or an earlier failed pass, a refused blur) clears its destination white, so the mask ends white as a
// failed trace leaves it; every declared pass is recorded (the plan's key holds the count).
void SceneRenderer::EncodeNativeSelfShadow(ID3D12GraphicsCommandList* list,const NativeModelFrameInput* input,const D3D12_VIEWPORT& viewport,const D3D12_RECT& scissor) {
    const bool traced=input;   // RenderNativeOffscreen passes it on a frame self_traced_now_
    if(!traced && !SelfMaskBound())return;   // the graph declared no mask
    auto* const heap=rt_heap_.load(std::memory_order_acquire);
    const uint32_t passes=traced?self_blur_passes_now_:0u;
    ID3D12Resource* const images[2]{view_->self_mask.Get(),view_->self_blur.Get()};
    const D3D12_CPU_DESCRIPTOR_HANDLE rtvs[2]{view_->self_mask_rtv->GetCPUDescriptorHandleForHeapStart(),
        passes?view_->self_blur_rtv->GetCPUDescriptorHandleForHeapStart():D3D12_CPU_DESCRIPTOR_HANDLE{}};
    const uint32_t views[2]{view_->self_mask_view,view_->self_blur_view};
    const auto read_depth=CPU(view_->dsv.Get(),gpu_.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV),1);
    const auto upload=[&](uint64_t bytes){return frame_uploads_.Allocate(gpu_.device.Get(),bytes,UploadArena::alignment,false);};
    const float white[4]{1,1,1,1};
    uint32_t at=passes%2;   // the trace's target: the blur target when the count is odd
    Transition(list,images[at],D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);
    if(auto* graph=active_graph_shadow){graph->Pass("self_shadow",PassKind::Lighting);graph->Use(images[at],ResourceAccess::Write,D3D12_RESOURCE_STATE_RENDER_TARGET);
        if(traced){graph->Use(view_->depth.Get(),ResourceAccess::Read,lit_depth_read_,{0,1});graph->Use(view_->depth.Get(),ResourceAccess::Read,D3D12_RESOURCE_STATE_DEPTH_WRITE,{1,1});}}
    bool drawn=false;RtSunConstants constants{};
    if(traced && heap){RtViewConstants camera{};
        if(RtViewCameraOf(input->camera,camera) && RtSunConstantsOf(camera,input->rt_sun,constants))try{
            constants.depth=view_->self_depth_view;constants.neutral=passes?2u:1u;
            list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
            rt_sun_.Encode(gpu_.device.Get(),list,*heap,upload,menu_rt_scene_,menu_rt_table_,menu_rt_rows_,constants,rtvs[at],read_depth,MaskStage::ColorFormat());
            drawn=true;menu_shadow_traced_.fetch_add(1,std::memory_order_relaxed);}
        catch(const std::exception& error){FailMenuShadow(error.what());}}
    if(!drawn){list->ClearRenderTargetView(rtvs[at],white,0,nullptr);if(traced)menu_shadow_refused_.fetch_add(1,std::memory_order_relaxed);}
    Transition(list,images[at],D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    bool blurred=drawn;
    for(uint32_t pass=0;pass<passes;++pass){const uint32_t source=at,target=1-at;
        Transition(list,images[target],D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);
        if(auto* graph=active_graph_shadow){graph->Pass("self_shadow_blur",PassKind::Lighting);graph->Use(images[source],ResourceAccess::Read,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            graph->Use(images[target],ResourceAccess::Write,D3D12_RESOURCE_STATE_RENDER_TARGET);
            graph->Use(view_->depth.Get(),ResourceAccess::Read,lit_depth_read_,{0,1});graph->Use(view_->depth.Get(),ResourceAccess::Read,D3D12_RESOURCE_STATE_DEPTH_WRITE,{1,1});}
        if(blurred)try{constants.source=views[source];constants.blur=input->self_shadow_blur_scale;constants.neutral=pass+1==passes?1u:2u;
            list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&scissor);
            rt_sun_.EncodeBlur(gpu_.device.Get(),list,*heap,upload,menu_rt_scene_,constants,rtvs[target],read_depth);}
        catch(const std::exception& error){FailMenuShadow(error.what());blurred=false;}
        if(!blurred)list->ClearRenderTargetView(rtvs[target],white,0,nullptr);
        Transition(list,images[target],D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        at=target;}
    if(passes && blurred)menu_shadow_blurred_.fetch_add(1,std::memory_order_relaxed);
}
// 1147 env-map regions (pass 4, 67070/675B0): each selected region box draws GXEnvMapRegion[_Pcc] over
// the G-buffer pixels it encloses (stencil bit 8), blending its IBL cube's contribution into SpecularAcc.
// Entered with the target as a render target, the depth plane readable and the stencil plane writable.
void SceneRenderer::EncodeNativeEnvRegions(ID3D12GraphicsCommandList* list,const NativeLightingInput& input) {
    constexpr float empty[4]{};
    list->ClearRenderTargetView(CPU(view_->light_rtvs.Get(),rtv_stride_,7),empty,0,nullptr);
    if(!native_env_psos_[0] || input.env_regions.empty())return;
    if(input.env_regions.size()>env_region_limit || input.env_scene.size()!=960)throw std::invalid_argument("native env region extent");
    auto* device=gpu_.device.Get();
    // B2c: the scene block and the per-region slots in ranges of this submission.
    const auto scene=frame_uploads_.Allocate(device,1024);std::memcpy(scene.cpu,input.env_scene.data(),960);
    const auto regions=frame_uploads_.Allocate(device,512*input.env_regions.size());
    for(size_t i=0;i<input.env_regions.size();++i)std::memcpy(regions.cpu+i*512,input.env_regions[i].constants.data(),400);
    // cD3 (D1): this recording's range: per frame slot, then per recording of this context within the frame.
    if(view_->env_frame!=frames_.Frames()){view_->env_frame=frames_.Frames();view_->env_ranges=0;}
    if(view_->env_ranges>=env_ranges_per_slot)throw std::logic_error("native env region descriptor ranges exhausted in one frame");
    const uint32_t cubes=env_cube_views+(frames_.CurrentIndex()*env_ranges_per_slot+view_->env_ranges++)*env_region_limit;
    for(size_t i=0;i<input.env_regions.size();++i){const auto& cube=input.env_regions[i].cube;
        if(!cube.resource || !cube.descriptor.ptr)throw std::invalid_argument("native env region cube");
        device->CopyDescriptorsSimple(1,CPU(view_->light_views.Get(),binding_stride_,cubes+static_cast<uint32_t>(i)),cube.descriptor,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);}
    const auto read_depth=CPU(view_->dsv.Get(),gpu_.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV),1);
    const auto target=CPU(view_->light_rtvs.Get(),rtv_stride_,7);
    list->OMSetRenderTargets(1,&target,FALSE,&read_depth);list->OMSetStencilRef(0xFF);
    list->SetGraphicsRootSignature(native_env_root_.Get());
    list->SetGraphicsRootConstantBufferView(1,scene.gpu);
    list->SetGraphicsRootDescriptorTable(2,GPU(view_->light_views.Get(),binding_stride_,env_views));
    list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);list->IASetVertexBuffers(0,1,&native_env_vertices_);list->IASetIndexBuffer(&native_env_indices_);
    for(size_t i=0;i<input.env_regions.size();++i){const auto& region=input.env_regions[i];
        list->SetPipelineState(native_env_psos_[(region.parallax_corrected?2:0)+(region.inside?1:0)].Get());
        list->SetGraphicsRootConstantBufferView(0,regions.gpu+i*512);
        list->SetGraphicsRootDescriptorTable(3,GPU(view_->light_views.Get(),binding_stride_,cubes+static_cast<uint32_t>(i)));
        list->DrawIndexedInstanced(14,1,0,0,0);}
}
// 1137 skin SSS stage (140114640, between GXLightCmbAlbedo and GXLightComposite): BlurX from the
// irradiance (CmbAlbedo output) into the cleared temporary, then BlurY from the temporary back into
// the irradiance, both only where the G-buffer stencil carries the SSS class (bit 4). Entered with the
// irradiance as a render target, the depth plane readable and the stencil plane in DEPTH_WRITE; leaves
// the irradiance as a render target.
void SceneRenderer::EncodeNativeSss(ID3D12GraphicsCommandList* list,const NativeLightingInput& input) {
    if(input.sss_profiles.size()>0x10000 || input.sss_profiles.size()%0x120 || input.sss_scene.size()>256)
        throw std::invalid_argument("native SSS constant extent");
    // B2c: the profile array in a range of its full extent (indexed by GBuffer1.w*255), zero past the records.
    auto* device=gpu_.device.Get();
    const auto profiles=frame_uploads_.Allocate(device,0x10000),scene=frame_uploads_.Allocate(device,256);
    std::memcpy(profiles.cpu,input.sss_profiles.data(),input.sss_profiles.size());std::memcpy(scene.cpu,input.sss_scene.data(),input.sss_scene.size());
    const auto read_depth=CPU(view_->dsv.Get(),gpu_.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV),1);
    const auto irradiance=CPU(view_->light_rtvs.Get(),rtv_stride_,2),temporary=CPU(view_->light_rtvs.Get(),rtv_stride_,6);
    list->SetGraphicsRootSignature(native_sss_root_.Get());
    list->SetGraphicsRootConstantBufferView(0,profiles.gpu);
    list->SetGraphicsRootConstantBufferView(1,scene.gpu);
    list->SetGraphicsRootDescriptorTable(2,GPU(view_->light_views.Get(),binding_stride_,sss_views));
    list->OMSetStencilRef(4);
    constexpr float empty[4]{};
    list->ClearRenderTargetView(temporary,empty,0,nullptr);
    Transition(list,view_->light_targets[2].Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    const auto depth_read=lit_depth_read_;   // I-0132 R11b: the lighting phase's
    if(auto* graph=active_graph_shadow){graph->Pass("sss_x",PassKind::Effect);graph->Use(view_->sss_temp.Get(),ResourceAccess::Write,D3D12_RESOURCE_STATE_RENDER_TARGET);
        graph->Use(view_->light_targets[2].Get(),ResourceAccess::Read,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);graph->Use(view_->depth.Get(),ResourceAccess::Read,depth_read,{0,1});
        graph->Use(view_->colors[1].Get(),ResourceAccess::Read,lit_gb_read_);   // t2 GBuffer1 (review)
        graph->Use(view_->depth.Get(),ResourceAccess::ReadWrite,D3D12_RESOURCE_STATE_DEPTH_WRITE,{1,1});}   // the stencil test through the read-only DSV
    list->OMSetRenderTargets(1,&temporary,FALSE,&read_depth);list->SetPipelineState(native_sss_psos_[0].Get());
    list->SetGraphicsRootDescriptorTable(3,GPU(view_->light_views.Get(),binding_stride_,sss_views+3));
    list->DrawInstanced(4,1,0,0);
    Transition(list,view_->sss_temp.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    Transition(list,view_->light_targets[2].Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);
    if(auto* graph=active_graph_shadow){graph->Pass("sss_y",PassKind::Effect);graph->Use(view_->light_targets[2].Get(),ResourceAccess::Write,D3D12_RESOURCE_STATE_RENDER_TARGET);
        graph->Use(view_->sss_temp.Get(),ResourceAccess::Read,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);graph->Use(view_->depth.Get(),ResourceAccess::Read,depth_read,{0,1});
        graph->Use(view_->colors[1].Get(),ResourceAccess::Read,lit_gb_read_);
        graph->Use(view_->depth.Get(),ResourceAccess::ReadWrite,D3D12_RESOURCE_STATE_DEPTH_WRITE,{1,1});}
    list->OMSetRenderTargets(1,&irradiance,FALSE,&read_depth);list->SetPipelineState(native_sss_psos_[1].Get());
    list->SetGraphicsRootDescriptorTable(3,GPU(view_->light_views.Get(),binding_stride_,sss_views+4));
    list->DrawInstanced(4,1,0,0);
    Transition(list,view_->sss_temp.Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);
}
// I-0107: the light grid's buffers (14019EF60's sizes), off the render thread as SceneColourCopy: grown when the presentation
// outgrows them (a former set stays alive in its holders until their draws complete).
std::string SceneRenderer::LightGridFailure() const {const std::lock_guard hold(channel_lock_);return organize_failure_;}
NativeFrameJob::LightGridSet SceneRenderer::LightGridSources() {
    RequireThread(false,"native light grid on the render thread");
    if(!open_ || !gpu_.device || !presenter_.width || !presenter_.height)return {};
    if(LightGridFailed()){light_grid_={};return {};}   // step review L3: a latched grid's buffers released (frames in flight hold theirs)
    const uint32_t tiles16=((presenter_.width+15)/16)*((presenter_.height+15)/16);
    if(light_grid_.indices && light_grid_.tiles16>=tiles16)return light_grid_;
    NativeFrameJob::LightGridSet made;made.tiles16=tiles16;made.serial=++light_grid_serial_;
    const auto buffer=[&](uint64_t bytes,bool unordered,ComPtr<ID3D12Resource>& out,const char* what){
        D3D12_RESOURCE_DESC desc=Buffer(bytes);if(unordered)desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        const auto heap=Heap(D3D12_HEAP_TYPE_DEFAULT);
        Check(CountedResource(gpu_.device)->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&out)),what);};
    // Final review: a refused allocation (the indices are tiles16 x 8 KiB: 265 MB at 3840x2160) latches; the session goes on
    // without the grid (the forward rows keep the empty buffers).
    try{
        buffer(uint64_t(tiles16)*1024*8,true,made.indices,"native light grid indices");
        buffer(uint64_t(tiles16)*12,true,made.lookup,"native light grid lookup");
        buffer(4,true,made.work,"native light grid work");
        buffer(1024*64,false,made.points,"native light grid points");
        buffer(1024*128,false,made.spots,"native light grid spots");
        buffer(1024*80,false,made.lines,"native light grid lines");
    }catch(const std::exception& error){{const std::lock_guard hold(channel_lock_);organize_failure_=error.what();}organize_failed_=true;light_grid_={};return {};}
    light_grid_=made;return made;
}
// I-0107 (a failure latch, as the sun shadow's): GXLightOrganize's root (b0, t1, t19..t21, u0..u2: the
// programme's declarations, evidence/I-0107/organize/rdef-GXLightOrganize.txt) and pipeline, made before the plan key; a refusal
// latches (LightGridFailed: the frame start logs it once and stops publishing the grid) instead of failing the frame.
void SceneRenderer::OpenNativeLightOrganize(const NativeLightingInput& input) {
    if(organize_failed_ || !input.programs || input.programs->organize.empty() || (organize_pso_ && organize_programme_==&input.programs->organize))return;
    auto* device=gpu_.device.Get();
    try{
    if(!organize_root_){
        // b0 cbLightAccParam; t1 g_DepthTexture; t19..t21 the light buffers; u0..u2 g_wLightIndices, g_lightLookup, g_workData
        // (GXLightOrganize's declarations, tools/rdef.py of I-0106).
        D3D12_ROOT_PARAMETER parameters[4]{};
        parameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV;parameters[0].Descriptor.ShaderRegister=0;
        const D3D12_DESCRIPTOR_RANGE ranges[3]{{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,1,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,3,19,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,3,0,0,0}};
        for(unsigned i=0;i<3;++i){parameters[1+i].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameters[1+i].DescriptorTable={1,&ranges[i]};}
        const D3D12_ROOT_SIGNATURE_DESC desc{4,parameters,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
        ComPtr<ID3DBlob> blob,error;
        Check(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error),"native light organize root serialization");
        Check(ds3native::Pipelines().Root(device,blob->GetBufferPointer(),blob->GetBufferSize(),organize_root_.ReleaseAndGetAddressOf()),"native light organize root");}
    {
        D3D12_COMPUTE_PIPELINE_STATE_DESC pso{};pso.pRootSignature=organize_root_.Get();pso.CS={input.programs->organize.data(),input.programs->organize.size()};
        Check(ds3native::Pipelines().Compute(device,pso,organize_pso_.ReleaseAndGetAddressOf()),"native light organize pipeline");
        organize_programme_=&input.programs->organize;}
    }catch(const std::exception& error){organize_pso_.Reset();{const std::lock_guard hold(channel_lock_);organize_failure_=error.what();}organize_failed_=true;}
}
bool SceneRenderer::NativeLightOrganizeReady(const NativeLightingInput& input) const {
    const auto& grid=light_grid_frame_;
    return !organize_failed_ && input.organize && input.programs && !input.programs->organize.empty() && organize_pso_ &&
        organize_programme_==&input.programs->organize && grid.indices && grid.lookup && grid.work &&
        grid.points && grid.spots && grid.lines && uint64_t(((ViewWidth()+15)/16)*((ViewHeight()+15)/16))<=grid.tiles16;   // not during a resize
}
// I-0107 (1A0080's tail, 1A36B0): the main view's GXLightOrganize. The point and spot records copied into their buffers, the work
// counter zeroed (as 1A36B0's UpdateSubresource), then the programme over ceil(W / 64) x ceil(H / 64) groups with b0 the organize
// cbLightAccParam, t1 the depth plane, t19..t21 the light buffers and u0..u2 the indices, the lookup and the work counter. The
// grid's buffers are this stage's own (raw barriers, as the grading stage's volume): the indices, lookup and light buffers rest
// in NON_PIXEL | PIXEL_SHADER_RESOURCE between dispatches (the forward rows read them as t59..t62), the work counter in
// UNORDERED_ACCESS. The depth goes DEPTH_WRITE -> readable -> DEPTH_WRITE through the frame graph (pass light_organize).
bool SceneRenderer::EncodeNativeLightOrganize(ID3D12GraphicsCommandList* list,const NativeLightingInput& input) {
    if(!NativeLightOrganizeReady(input))return false;
    const auto& grid=light_grid_frame_;const auto& organize=*input.organize;
    if(organize.points.size()>1024*64 || organize.points.size()%64 || organize.spots.size()>1024*128 || organize.spots.size()%128 ||
       !organize.groups_x || !organize.groups_y)throw std::invalid_argument("native light organize input");
    auto* device=gpu_.device.Get();
    const auto raw=[&](ID3D12Resource* r,D3D12_RESOURCE_STATES from,D3D12_RESOURCE_STATES to){D3D12_RESOURCE_BARRIER b{};
        b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={r,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,from,to};list->ResourceBarrier(1,&b);};
    constexpr auto readable=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    if(light_grid_ready_!=grid.serial){   // a new set: COMMON to the states it rests in
        for(auto* r:{grid.indices.Get(),grid.lookup.Get(),grid.points.Get(),grid.spots.Get(),grid.lines.Get()})raw(r,D3D12_RESOURCE_STATE_COMMON,readable);
        raw(grid.work.Get(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        light_grid_ready_=grid.serial;}
    auto* depth=view_->depth.Get();
    if(organize_views_depth_!=depth || organize_views_generation_!=view_->target_generation || organize_views_grid_!=grid.serial){
        // The seven views into a fresh heap, the former retired by ticket: t1, t19..t21, u0..u2.
        ComPtr<ID3D12DescriptorHeap> fresh;const D3D12_DESCRIPTOR_HEAP_DESC heap{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,7,D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,0};
        Check(CountedHeap(device)->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&fresh)),"native light organize views");
        D3D12_SHADER_RESOURCE_VIEW_DESC plane{};plane.Format=DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;plane.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
        plane.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;plane.Texture2D.MipLevels=1;
        device->CreateShaderResourceView(depth,&plane,CPU(fresh.Get(),binding_stride_,0));
        const auto structured=[&](ID3D12Resource* r,uint32_t stride,uint32_t elements,uint32_t index){D3D12_SHADER_RESOURCE_VIEW_DESC v{};v.Format=DXGI_FORMAT_UNKNOWN;
            v.ViewDimension=D3D12_SRV_DIMENSION_BUFFER;v.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;v.Buffer.NumElements=elements;
            v.Buffer.StructureByteStride=stride;device->CreateShaderResourceView(r,&v,CPU(fresh.Get(),binding_stride_,index));};
        structured(grid.points.Get(),64,1024,1);structured(grid.spots.Get(),128,1024,2);structured(grid.lines.Get(),80,1024,3);
        const auto unordered=[&](ID3D12Resource* r,uint32_t stride,uint32_t elements,uint32_t index){D3D12_UNORDERED_ACCESS_VIEW_DESC u{};u.Format=DXGI_FORMAT_UNKNOWN;
            u.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;u.Buffer.NumElements=elements;u.Buffer.StructureByteStride=stride;
            device->CreateUnorderedAccessView(r,nullptr,&u,CPU(fresh.Get(),binding_stride_,index));};
        unordered(grid.indices.Get(),8,grid.tiles16*1024,4);unordered(grid.lookup.Get(),12,grid.tiles16,5);unordered(grid.work.Get(),4,1,6);
        if(organize_views_)RetireAfterCurrentRecording(HoldObject(organize_views_.Get()));
        organize_views_=fresh;organize_views_depth_=depth;organize_views_generation_=view_->target_generation;organize_views_grid_=grid.serial;}
    // The records and the zeroed counter, then the constants.
    const uint64_t point_bytes=organize.points.size(),spot_bytes=organize.spots.size();
    const auto range=frame_uploads_.Allocate(device,1024+point_bytes+spot_bytes);   // zero-filled (B2c)
    std::memcpy(range.cpu,organize.constants.data(),organize.constants.size());
    if(point_bytes)std::memcpy(range.cpu+1024,organize.points.data(),point_bytes);
    if(spot_bytes)std::memcpy(range.cpu+1024+point_bytes,organize.spots.data(),spot_bytes);
    auto* upload=range.resource;
    if(point_bytes || spot_bytes){
        if(point_bytes)raw(grid.points.Get(),readable,D3D12_RESOURCE_STATE_COPY_DEST);
        if(spot_bytes)raw(grid.spots.Get(),readable,D3D12_RESOURCE_STATE_COPY_DEST);
        if(point_bytes)list->CopyBufferRegion(grid.points.Get(),0,upload,range.offset+1024,point_bytes);
        if(spot_bytes)list->CopyBufferRegion(grid.spots.Get(),0,upload,range.offset+1024+point_bytes,spot_bytes);
        if(point_bytes)raw(grid.points.Get(),D3D12_RESOURCE_STATE_COPY_DEST,readable);
        if(spot_bytes)raw(grid.spots.Get(),D3D12_RESOURCE_STATE_COPY_DEST,readable);}
    raw(grid.work.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_DEST);
    list->CopyBufferRegion(grid.work.Get(),0,upload,range.offset+512,4);   // zeros (the range is zero-filled past the constants)
    raw(grid.work.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    raw(grid.indices.Get(),readable,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);raw(grid.lookup.Get(),readable,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    // I-0132 R11b: on a frame with SER the graph's planner holds the depth's plane 0 from the lighting phase in DEPTH_READ | PIXEL |
    // NON_PIXEL (a read state's superset satisfies this pass's), so this pass takes that state too (else a use_state mismatch every frame).
    const auto depth_read=D3D12_RESOURCE_STATE_DEPTH_READ|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE|(rt_ser_now_?D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:D3D12_RESOURCE_STATES(0));   // this recording's flag (the final step review's L1)
    DepthPlaneTransition(list,depth,D3D12_RESOURCE_STATE_DEPTH_WRITE,depth_read);
    if(auto* graph=active_graph_shadow){graph->Pass("light_organize",PassKind::Lighting);graph->Use(depth,ResourceAccess::Read,depth_read,{0,1});}
    ID3D12DescriptorHeap* heaps[]{organize_views_.Get()};list->SetDescriptorHeaps(1,heaps);
    list->SetComputeRootSignature(organize_root_.Get());list->SetPipelineState(organize_pso_.Get());
    list->SetComputeRootConstantBufferView(0,range.gpu);
    list->SetComputeRootDescriptorTable(1,GPU(organize_views_.Get(),binding_stride_,0));
    list->SetComputeRootDescriptorTable(2,GPU(organize_views_.Get(),binding_stride_,1));
    list->SetComputeRootDescriptorTable(3,GPU(organize_views_.Get(),binding_stride_,4));
    list->Dispatch(organize.groups_x,organize.groups_y,1);
    raw(grid.indices.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,readable);raw(grid.lookup.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,readable);
    {D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;b.UAV.pResource=grid.work.Get();list->ResourceBarrier(1,&b);}
    DepthPlaneTransition(list,depth,depth_read,D3D12_RESOURCE_STATE_DEPTH_WRITE);
    return true;
}
}
