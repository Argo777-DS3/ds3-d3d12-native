#include "gpu_texture_upload.h"
#include <array>
#include <atomic>
#include <cstring>
#include <stdexcept>
namespace ds3content {
namespace {
constexpr uint64_t Limit=512ull*1024ull*1024ull;
void Require(bool value,const char* message){if(!value)throw std::invalid_argument(message);}
void Check(HRESULT value,const char* operation){if(FAILED(value))throw std::runtime_error(operation);}
D3D12_RESOURCE_DESC Image(const CpuTextureResource& texture) {
    D3D12_RESOURCE_DESC result{};result.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    result.Width=texture.width;result.Height=texture.height;result.DepthOrArraySize=static_cast<UINT16>(texture.array_slices);
    result.MipLevels=static_cast<UINT16>(texture.mip_levels);result.Format=texture.format;result.SampleDesc.Count=1;return result;
}
constexpr auto ResidentState=static_cast<D3D12_RESOURCE_STATES>(
    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}
D3D12_SHADER_RESOURCE_VIEW_DESC NativeTextureView(const CpuTextureResource& texture) {
    D3D12_SHADER_RESOURCE_VIEW_DESC result{};result.Format=texture.format;
    result.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    if(texture.cube && texture.array_slices==6) {
        result.ViewDimension=D3D12_SRV_DIMENSION_TEXTURECUBE;result.TextureCube.MipLevels=texture.mip_levels;
    } else if(texture.cube) {
        result.ViewDimension=D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        result.TextureCubeArray.MipLevels=texture.mip_levels;result.TextureCubeArray.NumCubes=texture.array_slices/6;
    } else if(texture.array_slices==1) {
        result.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;result.Texture2D.MipLevels=texture.mip_levels;
    } else {
        result.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        result.Texture2DArray.MipLevels=texture.mip_levels;result.Texture2DArray.ArraySize=texture.array_slices;
    }
    return result;
}

NativeTextureUpload RecordNativeTextureUpload(ds3native::DeviceQueues& gpu,ID3D12GraphicsCommandList* list,const CpuTextureResource& source) {
    Require(list && gpu.device && ValidCpuTextureResource(source),"native CPU texture resource");
    auto device=gpu.device.Get();const auto desc=Image(source);
    // I-0079: a format's support asked once (each texture asked the driver before).
    static std::array<std::atomic<uint32_t>,256> support_known{};   // Support1 | 1u<<31 once asked, by DXGI format
    uint32_t support1{};
    if(const auto index=static_cast<size_t>(source.format);index<support_known.size() && (support_known[index].load(std::memory_order_relaxed)>>31))
        support1=support_known[index].load(std::memory_order_relaxed)&~(1u<<31);
    else{D3D12_FEATURE_DATA_FORMAT_SUPPORT support{};support.Format=source.format;
        Check(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT,&support,sizeof(support)),"native texture format support");
        support1=static_cast<uint32_t>(support.Support1)&~(1u<<31);if(index<support_known.size())support_known[index].store(support1|1u<<31,std::memory_order_relaxed);}
    const auto needed=static_cast<uint32_t>(D3D12_FORMAT_SUPPORT1_TEXTURE2D|D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE|(source.cube?D3D12_FORMAT_SUPPORT1_TEXTURECUBE:0));
    Require((support1&needed)==needed,"native texture sampling format unsupported");
    NativeTextureUpload result;
    // I-0079: placed in a pooled texture heap, its bytes staged in a pooled upload chunk (backend.h, CreateStreamed).
    result.resource=gpu.CreateStreamed(desc,D3D12_RESOURCE_STATE_COPY_DEST,ds3native::PlacedKind::Texture);
    const auto count=static_cast<UINT>(source.subresources.size());
    for(const auto& sub:source.subresources)result.content_bytes+=sub.bytes.size();
    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(count);std::vector<UINT> rows(count);
    std::vector<UINT64> row_bytes(count);UINT64 total{};
    device->GetCopyableFootprints(&desc,0,count,0,footprints.data(),rows.data(),row_bytes.data(),&total);
    Require(total && total<=Limit,"native package upload byte bound");result.upload_bytes=total;
    auto staging=gpu.AllocateStaging(total);
    // Retained before recording; released locally if recording fails before any possible queue submission.
    result.upload=staging.lease;
    // Only the rows are written: the copy reads no byte between them (the former clear of the whole range wrote it twice).
    for(UINT sub=0;sub<count;++sub) {
        const auto& input=source.subresources[sub];auto footprint=footprints[sub];
        Require(row_bytes[sub]==input.row_bytes && rows[sub]==input.rows && footprint.Footprint.Depth==1 &&
            footprint.Footprint.RowPitch>=input.row_bytes,"native package CPU/D3D12 footprint agreement");
        if(footprint.Footprint.RowPitch==input.row_bytes){
            const auto bytes=static_cast<UINT64>(rows[sub])*input.row_bytes;
            Require(footprint.Offset<=total && bytes<=total-footprint.Offset,"native package upload row extent");
            std::memcpy(staging.cpu+footprint.Offset,input.bytes.data(),static_cast<size_t>(bytes));}
        else for(UINT row=0;row<rows[sub];++row) {
            const auto destination=footprint.Offset+static_cast<UINT64>(row)*footprint.Footprint.RowPitch;
            Require(destination<=total && input.row_bytes<=total-destination,"native package upload row extent");
            std::memcpy(staging.cpu+destination,input.bytes.data()+static_cast<size_t>(row)*input.row_bytes,input.row_bytes);
        }
        footprint.Offset+=staging.offset;
        D3D12_TEXTURE_COPY_LOCATION from{};from.pResource=staging.resource;from.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;from.PlacedFootprint=footprint;
        D3D12_TEXTURE_COPY_LOCATION to{};to.pResource=result.resource.Get();
        to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;to.SubresourceIndex=sub;
        list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
    }
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition={result.resource.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        D3D12_RESOURCE_STATE_COPY_DEST,ResidentState};list->ResourceBarrier(1,&barrier);

    return result;
}
}
