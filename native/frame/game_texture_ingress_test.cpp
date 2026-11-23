#include "game_texture_ingress.h"
#include "game_ui_bindings.h"
#include "../content/texture_container.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <cfloat>

using namespace ds3native;
using Microsoft::WRL::ComPtr;
namespace {
void Require(bool ok,const char* label){if(!ok)throw std::runtime_error(label);}
std::vector<uint8_t> CubePackage() {
    // A declared BC1 cube with two mips, every face carrying distinct bytes.
    std::vector<uint8_t> dds(128+6*2*8);
    auto word=[&](size_t at,uint32_t value){std::memcpy(dds.data()+at,&value,4);};
    std::memcpy(dds.data(),"DDS ",4);word(4,124);word(12,4);word(16,4);word(28,2);
    word(76,32);word(80,4);word(84,0x31545844);word(112,0xFE00);
    for(size_t i=128;i<dds.size();++i)dds[i]=static_cast<uint8_t>(i-127);
    std::vector<uint8_t> tpf(64+dds.size());
    auto tpf_word=[&](size_t at,uint32_t value){std::memcpy(tpf.data()+at,&value,4);};
    std::memcpy(tpf.data(),"TPF\0",4);tpf_word(4,static_cast<uint32_t>(dds.size()));tpf_word(8,1);tpf[13]=3;
    tpf_word(16,64);tpf_word(20,static_cast<uint32_t>(dds.size()));tpf[25]=1;tpf[26]=2;tpf_word(28,52);tpf_word(32,1);
    tpf_word(36,0xF1234567);tpf_word(40,8);tpf_word(44,0x3FC00000);tpf_word(48,0x7FC12345);
    std::memcpy(tpf.data()+52,"cube",5);std::memcpy(tpf.data()+64,dds.data(),dds.size());return tpf;
}
std::vector<BYTE> Readback(DeviceQueues& gpu,ID3D12Resource* image,UINT sub) {
    const auto desc=image->GetDesc();D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
    UINT rows{};UINT64 row_bytes{},total{};
    gpu.device->GetCopyableFootprints(&desc,sub,1,0,&footprint,&rows,&row_bytes,&total);
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_READBACK;heap.CreationNodeMask=heap.VisibleNodeMask=1;
    D3D12_RESOURCE_DESC buffer{};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=total;
    buffer.Height=1;buffer.DepthOrArraySize=buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ComPtr<ID3D12Resource> output;
    Require(SUCCEEDED(gpu.device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,IID_PPV_ARGS(&output))),"readback allocation");
    FrameRecorder recorder;recorder.Open(gpu.device.Get());
    Require(SUCCEEDED(recorder.allocator->Reset()) && SUCCEEDED(recorder.list->Reset(recorder.allocator.Get(),nullptr)),"readback reset");
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition={image,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        static_cast<D3D12_RESOURCE_STATES>(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
        D3D12_RESOURCE_STATE_COPY_SOURCE};recorder.list->ResourceBarrier(1,&barrier);
    D3D12_TEXTURE_COPY_LOCATION from{};from.pResource=image;from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;from.SubresourceIndex=sub;
    D3D12_TEXTURE_COPY_LOCATION to{};to.pResource=output.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint=footprint;
    recorder.list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
    std::swap(barrier.Transition.StateBefore,barrier.Transition.StateAfter);recorder.list->ResourceBarrier(1,&barrier);
    Require(SUCCEEDED(recorder.list->Close()),"readback close");gpu.Execute(recorder.list.Get());gpu.Drain();
    void* mapped{};const D3D12_RANGE extent{0,static_cast<SIZE_T>(total)};
    Require(SUCCEEDED(output->Map(0,&extent,&mapped)),"readback map");
    std::vector<BYTE> result(static_cast<size_t>(row_bytes)*rows);
    for(UINT row=0;row<rows;++row)std::memcpy(result.data()+static_cast<size_t>(row)*static_cast<size_t>(row_bytes),
        static_cast<BYTE*>(mapped)+footprint.Offset+static_cast<size_t>(row)*footprint.Footprint.RowPitch,static_cast<size_t>(row_bytes));
    const D3D12_RANGE empty{};output->Unmap(0,&empty);return result;
}
}
int main(int argc,char** argv) {
    try {
        const bool warp=argc>1 && std::strcmp(argv[1],"warp")==0;
        DeviceQueues gpu;gpu.Open(warp,true);std::mutex submissions;
        bootstrap::TextureIngress ingress(gpu,submissions);
        // Non-square mips and padded CPU rows distinguish a real upload from
        // copying a single contiguous slice or retaining the borrowed pointer.
        BYTE mip0[48]{},mip1[16]{};
        for(UINT row=0;row<2;++row)for(UINT col=0;col<16;++col)mip0[row*24+col]=static_cast<BYTE>(row*16+col+1);
        for(UINT col=0;col<8;++col)mip1[col]=static_cast<BYTE>(col+100);
        struct Source {uintptr_t bytes;uint32_t row,slice;};
        const Source source[]{{reinterpret_cast<uintptr_t>(mip0),24,0},{reinterpret_cast<uintptr_t>(mip1),16,0}};
        const auto version_before=ingress.Version(),withdrawals_before=ingress.Withdrawals();
        const auto old=ingress.Admit(0x1000,{4,2,2,DXGI_FORMAT_R8G8B8A8_UNORM},reinterpret_cast<uintptr_t>(source));
        Require(old->resident && old->content_bytes==40 && old->upload_fence && ingress.Acquire(0x1000)==old,"real resident receipt");
        // Workstream K: the version moves at the admission and again when the record becomes resident (2 changes).
        Require(ingress.Version()==version_before+2,"texture ingress version: admission and residency");
        // I-0070: a new receiver's admission and residency withdraw nothing.
        Require(ingress.Withdrawals()==withdrawals_before && !old->withdrawn,"texture ingress withdrawals: a new admission");
        std::memset(mip0,0,sizeof(mip0));std::memset(mip1,0,sizeof(mip1));
        const auto a=Readback(gpu,old->resource.Get(),0),b=Readback(gpu,old->resource.Get(),1);
        for(size_t i=0;i<a.size();++i)Require(a[i]==i+1,"mip0 exact owned bytes");
        for(size_t i=0;i<b.size();++i)Require(b[i]==i+100,"mip1 exact owned bytes");
        // D3D12 review R5 (B3a): an admission records on the ingress context without a CPU wait and without the
        // submission lock (a worker admits while this thread holds it); the next submission carries the copy first.
        {const auto drains=gpu.drains.load(),completes=gpu.completes.load(),lists=gpu.ingress_lists.load();
            std::shared_ptr<const bootstrap::NativeTextureRecord> queued;
            for(UINT row=0;row<2;++row)for(UINT col=0;col<16;++col)mip0[row*24+col]=static_cast<BYTE>(row*16+col+1);
            {std::lock_guard<std::mutex> frame(submissions);
                std::thread loader([&]{queued=ingress.Admit(0x3000,{4,2,2,DXGI_FORMAT_R8G8B8A8_UNORM},reinterpret_cast<uintptr_t>(source));});loader.join();}
            std::memset(mip0,0,sizeof(mip0));   // the admission owns a copy
            Require(queued && queued->resident && queued->upload_fence && gpu.drains==drains && gpu.completes==completes,"ingress admission waited");
            const auto copied=Readback(gpu,queued->resource.Get(),0);
            Require(gpu.ingress_lists==lists+1 && copied==a,"ingress copy not carried ahead of the next submission");
            {auto discarded=gpu.RecordIngress();Require(discarded.List()!=nullptr,"ingress recording list");}   // dropped: never submitted
            Require(!gpu.SubmitIngress(),"discarded ingress recording submitted");
            ingress.Retire(0x3000);}
        {const auto version_retire=ingress.Version(),withdrawals_retire=ingress.Withdrawals();ingress.Retire(0x1000);Require(ingress.Version()==version_retire+1,"texture ingress version: retirement");
            Require(ingress.Withdrawals()==withdrawals_retire+1 && old->withdrawn,"texture ingress withdrawals: retirement");
            ingress.Retire(0x1000);Require(ingress.Version()==version_retire+1 && ingress.Withdrawals()==withdrawals_retire+1,"texture ingress version: no change, no move");}
        Require(!ingress.Acquire(0x1000),"retired logical identity");
        // A retained consumer snapshot remains usable after withdrawal; a new
        // incarnation at the same engine address must not reuse that identity.
        Require(Readback(gpu,old->resource.Get(),0)==a,"retained consumer lifetime");
        std::shared_ptr<const bootstrap::NativeTextureRecord> fresh;
        std::thread worker([&]{fresh=ingress.Admit(0x1000,{4,2,2,DXGI_FORMAT_R8G8B8A8_UNORM},reinterpret_cast<uintptr_t>(source));});worker.join();
        Require(fresh->generation!=old->generation && fresh->resource.Get()!=old->resource.Get(),"worker upload new incarnation");
        // I-0070: an admission over a receiver's resident record withdraws it.
        {const auto before=ingress.Withdrawals();ingress.Admit(0x1000,{4,2,2,DXGI_FORMAT_R8G8B8A8_UNORM},reinterpret_cast<uintptr_t>(source));
            Require(ingress.Withdrawals()==before+1 && fresh->withdrawn,"texture ingress withdrawals: a replacing admission");}
        const auto zero=Readback(gpu,fresh->resource.Get(),0);
        for(const auto byte:zero)Require(!byte,"new source used");
        bool rejected{};const Source malformed[]{{reinterpret_cast<uintptr_t>(mip0),1,0}};
        try {ingress.Admit(0x2000,{4,2,1,DXGI_FORMAT_R8G8B8A8_UNORM},reinterpret_cast<uintptr_t>(malformed));}
        catch(const std::exception&){rejected=true;}
        Require(rejected && !ingress.Acquire(0x2000),"invalid rows never resident");
        const auto encoded=CubePackage();const auto entries=ds3content::DecodeCpuTextureContainer(encoded);
        auto cube=ds3content::DecodeNativeTextureDDS(entries.at(0));
        Require(cube.float_metadata && cube.float_metadata->tag==0xF1234567 &&
            cube.float_metadata->words==std::vector<uint32_t>({0x3FC00000,0x7FC12345}),"container metadata reaches native CPU resource");
        const auto native_cube=ingress.AdmitOwned(0x3000,cube);
        cube.float_metadata.reset();
        Require(native_cube->content->float_metadata && native_cube->content->float_metadata->tag==0xF1234567 &&
            native_cube->content->float_metadata->words==std::vector<uint32_t>({0x3FC00000,0x7FC12345}),"current texture generation owns metadata after source withdrawal");
        Require(native_cube->description.cube && native_cube->description.slices==6 && native_cube->content_bytes==96,"cube ownership");
        for(UINT sub=0;sub<12;++sub)Require(Readback(gpu,native_cube->resource.Get(),sub)==cube.subresources[sub].bytes,"cube face and mip bytes");
        // I-0074: the entry read in place (a view of the container's bytes) decodes to the same image, and the admission that
        // moves its CPU image gives the same GPU bytes; an index past the container gives none.
        {std::vector<uint8_t> expanded;const auto view=ds3content::ViewCpuTextureContainerEntry(encoded,0,expanded);
            Require(view && view->name==entries.at(0).name && view->dds.size()==entries.at(0).dds.size() &&
                std::equal(view->dds.begin(),view->dds.end(),entries.at(0).dds.begin()) &&
                view->dds.data()>=encoded.data() && view->dds.data()<encoded.data()+encoded.size(),"container entry view in place");
            auto viewed=ds3content::DecodeNativeTextureDDS(*view);
            Require(viewed.subresources.size()==cube.subresources.size(),"container entry view decode");
            const auto moved=ingress.AdmitOwned(0x3100,std::move(viewed));
            for(UINT sub=0;sub<12;++sub)Require(Readback(gpu,moved->resource.Get(),sub)==cube.subresources[sub].bytes,"moved admission bytes");
            Require(!ds3content::ViewCpuTextureContainerEntry(encoded,entries.size(),expanded),"container entry view past the end");}
        bootstrap::UiImageIngress ui(ingress,77);
        const auto empty_ui=ui.Acquire();
        std::array<uint8_t,20> atlas{};
        const std::array<bootstrap::CpuPrimitiveImageMip,1> atlas_source{{{5,4,5,atlas}}};
        Require(ui.Create(0x5000,empty_ui->version,9,atlas_source),"native UI atlas creation");
        const auto blank=ui.Acquire();const auto blank_gpu=blank->images.at(0x5000).gpu;
        Require(blank_gpu->resident && blank_gpu->upload_fence && Readback(gpu,blank_gpu->resource.Get(),0)==std::vector<uint8_t>(20),"real R8 atlas upload");
        std::array<uint8_t,7> glyph{11,12,99,99,99,21,22};
        const std::array<bootstrap::CpuGlyphImageUpdate,1> patch{{{{5,4,5,glyph},{3,2,5,4},0}}};
        Require(ui.UpdateGlyphs(0x5000,blank->version,patch),"native UI raster update");
        const auto written=ui.Acquire();const auto written_gpu=written->images.at(0x5000).gpu;glyph.fill(0);
        std::vector<uint8_t> expected(20);expected[13]=11;expected[14]=12;expected[18]=21;expected[19]=22;
        Require(Readback(gpu,written_gpu->resource.Get(),0)==expected &&
            written->textures->textures[0].texture.mips[0]==expected,"exact native GPU cropped raster");
        Require(Readback(gpu,blank_gpu->resource.Get(),0)==std::vector<uint8_t>(20),"old GPU atlas consumer lease");
        bootstrap::UiDrawImageBinding retained_binding;
        for(uint8_t index=0;index<4;++index){
            bootstrap::UiDrawImageSource bindings;
            Require(bindings.Begin(written,31,101) && bindings.Bind(written,0x5000,0,index) &&
                bindings.Resolve(written->version,31,retained_binding),"current native image/sampler binding");
            const std::array expected_modes{frame::PrimitiveSampler::PointWrap,frame::PrimitiveSampler::PointClamp,
                frame::PrimitiveSampler::LinearWrap,frame::PrimitiveSampler::LinearClamp};
            Require(retained_binding.sampler==expected_modes[index] && retained_binding.mip_bias==-.75f &&
                retained_binding.min_lod==0 && retained_binding.max_lod==FLT_MAX,"authored sampler policy changed");
            frame::PrimitiveDraw image_draw;image_draw.identity=101;std::vector<frame::PrimitiveTexture> image_textures;
            Require(bootstrap::ApplyUiDrawImageBinding(retained_binding,written->version,31,image_draw,image_textures) &&
                image_draw.texture==written->images.at(0x5000).identity && image_textures.size()==1 &&
                image_textures[0].texture.mips[0]==expected,"native draw did not own current raster");
            Require(!bootstrap::ApplyUiDrawImageBinding(retained_binding,written->version,31,image_draw,image_textures) &&
                image_textures.size()==1,"duplicate draw binding mutation");
            {   // A resident payload: asked with the snapshot's own image object, moved in under its identity; a refusal never asks.
                const frame::PrimitiveTexture* asked{};auto lend=image_textures[0];lend.identity=999;
                const bootstrap::UiResidentImage resident=[&](const frame::PrimitiveTexture& origin,frame::PrimitiveTexture& taken){
                    asked=&origin;taken=std::move(lend);return true;};
                frame::PrimitiveDraw lent_draw;lent_draw.identity=101;std::vector<frame::PrimitiveTexture> lent_textures;
                Require(bootstrap::ApplyUiDrawImageBinding(retained_binding,written->version,31,lent_draw,lent_textures,resident) &&
                    asked==&*std::find_if(written->textures->textures.begin(),written->textures->textures.end(),[&](const auto& t){return t.identity==retained_binding.texture;}) &&
                    lent_textures.size()==1 && lent_textures[0].identity==retained_binding.texture && lent_textures[0].texture.mips[0]==expected,"resident image payload");
                asked=nullptr;
                Require(!bootstrap::ApplyUiDrawImageBinding(retained_binding,written->version,31,lent_draw,lent_textures,resident) && !asked &&
                    lent_textures.size()==1,"refused binding asked for a resident image");
            }
        }
        bootstrap::UiDrawImageSource invalid_binding;
        Require(invalid_binding.Begin(written,31,101) && !invalid_binding.Bind(written,0x5000,2,0) &&
            !invalid_binding.Resolve(written->version,31,retained_binding),"unsupported slot retained prior binding");
        Require(invalid_binding.Begin(written,31,101) && !invalid_binding.Bind(written,0x5000,0,4) &&
            !invalid_binding.Resolve(written->version,31,retained_binding),"unknown sampler byte admitted");
        Require(invalid_binding.Begin(written,31,101) && !invalid_binding.Bind(blank,0x5000,0,0),"old native snapshot bound");
        Require(invalid_binding.Begin(written,31,101) && !invalid_binding.Bind(written,0x9999,0,0),"foreign engine image bound");
        Require(invalid_binding.Begin(written,31,101) && invalid_binding.Bind(written,0x5000,0,2) &&
            invalid_binding.Resolve(written->version,31,retained_binding) &&
            !invalid_binding.Resolve(blank->version,31,retained_binding) &&
            !invalid_binding.Resolve(written->version,32,retained_binding),"binding current version/serial contract");
        Require(!ui.UpdateGlyphs(0x5000,blank->version,patch) && ui.Acquire()==written,"stale glyph generation");
        auto invalid_patch=patch;invalid_patch[0].destination[2]=6;
        Require(!ui.UpdateGlyphs(0x5000,written->version,invalid_patch) && ui.Acquire()==written,"invalid patch atomic publication");
        Require(ui.Retire(0x5000,written->version) && Readback(gpu,written_gpu->resource.Get(),0)==expected,"retired GPU atlas lease");
        Require(retained_binding.images==written && Readback(gpu,retained_binding.images->images.at(0x5000).gpu->resource.Get(),0)==expected,
            "draw image lease retired before consumer");
        const auto retired=ui.Acquire();
        Require(ui.Create(0x5000,retired->version,9,atlas_source) &&
            ui.Acquire()->images.at(0x5000).identity!=written->images.at(0x5000).identity,"reused engine address fresh native identity");
        ui.Close();Require(!ui.Acquire() && !ui.Create(0x5000,retired->version,9,atlas_source) && ingress.Acquire(0x3000)==native_cube,"UI close isolated from game assets");
        bootstrap::UiImageIngress failed_ui(ingress,78);const auto failed_empty=failed_ui.Acquire();
        Require(failed_ui.Create(0x6000,failed_empty->version,9,atlas_source),"upload failure control setup");
        const auto before_failure=failed_ui.Acquire();ingress.Close();bool upload_rejected{};
        try{failed_ui.UpdateGlyphs(0x6000,before_failure->version,patch);}catch(const std::exception&){upload_rejected=true;}
        Require(upload_rejected && failed_ui.Acquire()==before_failure &&
            Readback(gpu,before_failure->images.at(0x6000).gpu->resource.Get(),0)==std::vector<uint8_t>(20),"failed upload retained published atlas");
        failed_ui.Close();
        if(gpu.diagnostics)for(UINT64 i=0;i<gpu.diagnostics->GetNumStoredMessages();++i) {
            SIZE_T size{};gpu.diagnostics->GetMessage(i,nullptr,&size);std::vector<BYTE> bytes(size);
            auto* message=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());
            Require(SUCCEEDED(gpu.diagnostics->GetMessage(i,message,&size)) && message->Severity>D3D12_MESSAGE_SEVERITY_ERROR,"D3D12 validation error");
        }
        ingress.Close();Require(!ingress.Acquire(0x1000),"closed ownership");
        Require(native_cube->content->float_metadata && native_cube->content->float_metadata->words.at(1)==0x7FC12345,
            "retained native generation metadata after close");fresh.reset();gpu.Close();
        std::printf("PASS %s: padded rows, borrowed bytes overwritten, worker upload, incarnation/retirement, invalid input, TPF/DDS BC1 cube 12 subresources, owned metadata in actual native generation after source withdrawal and close, existing package bank; UI R8 creation/cropped raster GPU bytes/current and retained generations/retirement/reused address/isolated close/failed upload atomicity; current draw image leases/resident payloads asked by snapshot object/all four authored samplers/stale and foreign refusal/atomic packet binding; D3D12 validation\n",warp?"warp":"hardware");
        return 0;
    } catch(const std::exception& e) {std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}
}
