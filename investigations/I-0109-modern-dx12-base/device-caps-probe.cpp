#include <initguid.h>
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstdio>
#include <initializer_list>
using Microsoft::WRL::ComPtr;
int main(){
    ComPtr<IDXGIFactory6> f; if(FAILED(CreateDXGIFactory2(0,IID_PPV_ARGS(&f)))){puts("factory fail");return 1;}
    ComPtr<IDXGIAdapter1> a; f->EnumAdapterByGpuPreference(0,DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,IID_PPV_ARGS(&a));
    DXGI_ADAPTER_DESC1 d{}; a->GetDesc1(&d); wprintf(L"adapter %s vram_mb %llu\n",d.Description,(unsigned long long)(d.DedicatedVideoMemory>>20));
    LARGE_INTEGER umd{}; if(SUCCEEDED(a->CheckInterfaceSupport(__uuidof(IDXGIDevice),&umd))) printf("umd %u.%u.%u.%u\n",HIWORD(umd.HighPart),LOWORD(umd.HighPart),HIWORD(umd.LowPart),LOWORD(umd.LowPart));
    ComPtr<ID3D12Device> dev; HRESULT hr=D3D12CreateDevice(a.Get(),D3D_FEATURE_LEVEL_12_2,IID_PPV_ARGS(&dev)); printf("create_12_2 0x%08lX\n",hr); if(FAILED(hr)) hr=D3D12CreateDevice(a.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&dev)); if(FAILED(hr))return 2;
    D3D_FEATURE_LEVEL lv[]={D3D_FEATURE_LEVEL_11_0,D3D_FEATURE_LEVEL_12_0,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_2}; D3D12_FEATURE_DATA_FEATURE_LEVELS fl{4,lv};
    dev->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS,&fl,sizeof fl); printf("max_feature_level 0x%X\n",fl.MaxSupportedFeatureLevel);
    D3D12_FEATURE_DATA_SHADER_MODEL sm{}; for(int m: {0x6A,0x69,0x68,0x67,0x66,0x65}){sm.HighestShaderModel=(D3D_SHADER_MODEL)m; if(SUCCEEDED(dev->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL,&sm,sizeof sm))){printf("shader_model 0x%X\n",sm.HighestShaderModel);break;}}
    D3D12_FEATURE_DATA_D3D12_OPTIONS o{}; dev->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS,&o,sizeof o); printf("binding_tier %d heap_tier %d\n",o.ResourceBindingTier,o.ResourceHeapTier);
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 o5{}; dev->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5,&o5,sizeof o5); printf("raytracing_tier 0x%X render_passes %d\n",o5.RaytracingTier,o5.RenderPassesTier);
    D3D12_FEATURE_DATA_D3D12_OPTIONS7 o7{}; dev->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7,&o7,sizeof o7); printf("mesh_shader_tier %d sampler_feedback %d\n",o7.MeshShaderTier,o7.SamplerFeedbackTier);
    D3D12_FEATURE_DATA_D3D12_OPTIONS12 o12{}; printf("options12 0x%08lX enhanced_barriers %d\n",dev->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12,&o12,sizeof o12),o12.EnhancedBarriersSupported);
    ComPtr<ID3D12Device5> d5; printf("device5 %d\n",SUCCEEDED(dev.As(&d5)));
    ComPtr<ID3D12Device10> d10; printf("device10 %d\n",SUCCEEDED(dev.As(&d10)));
    ComPtr<ID3D12SDKConfiguration1> cfg; hr=D3D12GetInterface(CLSID_D3D12SDKConfiguration,IID_PPV_ARGS(&cfg)); printf("sdk_configuration1 0x%08lX\n",hr);
    return 0;}
