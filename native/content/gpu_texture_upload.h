#pragma once
#include "dds_resource.h"
#include "../renderer/backend.h"

namespace ds3content {
// Shared upload recording for the package bank and installed CPU asset ingress.
// Caller owns recording/submission synchronization and retains BOTH resources
// through completion. This function never submits or changes engine readiness.
struct NativeTextureUpload {
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    std::shared_ptr<void> upload;   // I-0079: the staging range's lease (DeviceQueues::AllocateStaging)
    uint64_t upload_bytes{},content_bytes{};
};
NativeTextureUpload RecordNativeTextureUpload(ds3native::DeviceQueues& gpu,
    ID3D12GraphicsCommandList* list,const CpuTextureResource& source);
D3D12_SHADER_RESOURCE_VIEW_DESC NativeTextureView(const CpuTextureResource& texture);
}
