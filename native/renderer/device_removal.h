#pragma once
// I-0082 (DESIGN 4): what the device was doing when it was lost. DRED (Device Removed Extended Data) is part of the D3D12
// runtime (no Graphics Tools needed); DeviceQueues::Open enables it before the device is created (device_removal_data). ER
// never asks why its device was removed (I-0080 ER-STUDY 7).
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <string>
#include <vector>

namespace ds3native {
struct DeviceRemoval {
    HRESULT reason{S_OK};          // GetDeviceRemovedReason
    std::vector<std::string> rows; // JSON rows (no newline): the breadcrumb nodes not completed, then the page fault
};
// The reason, and when the device is removed the DRED breadcrumbs (at most `max_nodes` unfinished lists, each with the
// operations around the last one the GPU completed, by name) and the page fault with the allocations near it.
DeviceRemoval DescribeRemoval(ID3D12Device* device,size_t max_nodes=16);
const char* BreadcrumbOpName(D3D12_AUTO_BREADCRUMB_OP op) noexcept;
}
