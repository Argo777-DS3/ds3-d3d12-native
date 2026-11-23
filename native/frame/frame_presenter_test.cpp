// I-0137 (I-0120 E3): a ResizeBuffers failure that is not a device loss no longer throws from FramePresenter::Resize. On a real
// device and a hidden window: a reference kept on a back buffer makes DXGI refuse the resize (DXGI_ERROR_INVALID_CALL), as a
// focus loss or Streamline's proxy made it answer E_ACCESSDENIED in the runs i101-fs-alttab-1 and i111-fs-quit-1.
#include "../renderer/backend.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>

using namespace ds3native;
using Microsoft::WRL::ComPtr;
namespace {
void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
bool Sized(const FramePresenter& presenter,uint32_t width,uint32_t height){
    if(presenter.width!=width || presenter.height!=height || presenter.buffers.size()!=presenter.count)return false;
    for(const auto& buffer:presenter.buffers){if(!buffer)return false;const auto made=buffer->GetDesc();if(made.Width!=width || made.Height!=height)return false;}
    return true;}
}
int main(int argc,char** argv){
    HWND window{};
    try{
        Require(argc==2,"warp or hardware required");const bool warp=!std::strcmp(argv[1],"warp");Require(warp || !std::strcmp(argv[1],"hardware"),"unknown profile");
        window=CreateWindowExW(0,L"STATIC",L"Native presenter control",WS_POPUP,0,0,83,61,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
        Require(window!=nullptr,"control window");
        DeviceQueues gpu;gpu.Open(warp,false);   // no debug layer: the refused resizes below are DXGI's own errors, wanted here
        {FramePresenter presenter;presenter.Open(gpu,window,83,61);
            Require(!presenter.stand_in && Sized(presenter,83,61),"T0 the chain's buffers at the opening size");
            // T1: a resize DXGI accepts takes the chain's buffers at the new size.
            presenter.Resize(gpu,97,71);Require(!presenter.stand_in && Sized(presenter,97,71),"T1 an accepted resize");
            // T2: a resize to another size that DXGI refuses: no throw, stand-ins of the asked size; the follow's resize is refused
            // too while the reference is held and must not take the chain's former buffers (97x71) for 131x89 frames.
            {ComPtr<ID3D12Resource> held;Require(SUCCEEDED(presenter.swapchain->GetBuffer(0,IID_PPV_ARGS(&held))),"a back buffer held");
                presenter.Resize(gpu,131,89);
                Require(presenter.stand_in && Sized(presenter,131,89),"T2 a refused resize leaves stand-ins of the asked size");
                const auto status=presenter.TryResizeBuffers(gpu);
                Require(FAILED(status) && !IsDeviceLoss(status),"T2 the follow's resize refused while the buffer is held");
                Require(presenter.stand_in && Sized(presenter,131,89),"T2 the stand-ins kept, not the chain's former buffers");}
            // T3: the reference gone, the follow's resize succeeds and the chain's own buffers come back at the asked size.
            Require(SUCCEEDED(presenter.TryResizeBuffers(gpu)) && !presenter.stand_in && Sized(presenter,131,89),"T3 the chain's buffers back");
            // T4: a refused resize at the same size (a full-screen toggle's): the chain gives its buffers again, no stand-in.
            {ComPtr<ID3D12Resource> held;Require(SUCCEEDED(presenter.swapchain->GetBuffer(0,IID_PPV_ARGS(&held))),"a back buffer held again");
                const auto refused=presenter.ResizeChain(131,89);   // DXGI's own answer first (the integrity review's L3): the case is really a refusal
                Require(FAILED(refused) && !IsDeviceLoss(refused),"T4 DXGI refuses the same-size resize while the buffer is held");
                presenter.Resize(gpu,131,89);
                Require(!presenter.stand_in && Sized(presenter,131,89),"T4 a refused resize at the same size keeps the chain's buffers");}
            presenter.Resize(gpu,131,89);Require(!presenter.stand_in && Sized(presenter,131,89),"T4 the same resize accepted afterwards");
            (void)gpu.Drain(WaitOther);}
        // T5 (I-0138 tranche 5): an HDR10 chain asked. Where DXGI takes the colour space: 10-bit buffers, kept by a resize, the
        // stand-ins of a refused resize 10-bit too, an 8-bit chain again once it is no longer asked, and back. Where it refuses
        // it: an 8-bit chain and the reason. Which of the two ran is in the PASS line.
        bool hdr_taken=false;
        {const auto formatted=[](const FramePresenter& presenter,DXGI_FORMAT format){if(presenter.format!=format)return false;
                for(const auto& buffer:presenter.buffers)if(!buffer || buffer->GetDesc().Format!=format)return false;return true;};
            constexpr auto ten=DXGI_FORMAT_R10G10B10A2_UNORM,eight=DXGI_FORMAT_R8G8B8A8_UNORM;
            FramePresenter presenter;presenter.hdr_wanted=true;presenter.Open(gpu,window,83,61);hdr_taken=presenter.hdr10;
            if(!hdr_taken)Require(formatted(presenter,eight) && !presenter.hdr_failure.empty() && Sized(presenter,83,61),"T5 a refused HDR10 colour space leaves an 8-bit chain and its reason");
            else{Require(formatted(presenter,ten) && presenter.Hdr10Format() && presenter.hdr_failure.empty() && Sized(presenter,83,61),"T5 the HDR10 chain's buffers are 10-bit");
                presenter.Resize(gpu,97,71);Require(presenter.hdr10 && formatted(presenter,ten) && Sized(presenter,97,71),"T5 a resize keeps the HDR10 chain");
                {ComPtr<ID3D12Resource> held;Require(SUCCEEDED(presenter.swapchain->GetBuffer(0,IID_PPV_ARGS(&held))),"a back buffer held");
                    presenter.Resize(gpu,131,89);
                    Require(presenter.stand_in && formatted(presenter,ten) && Sized(presenter,131,89),"T5 a refused resize leaves 10-bit stand-ins");}
                Require(SUCCEEDED(presenter.TryResizeBuffers(gpu)) && !presenter.stand_in && presenter.hdr10 && formatted(presenter,ten),"T5 the HDR10 chain's buffers back");
                presenter.hdr_wanted=false;presenter.Resize(gpu,131,89);
                Require(!presenter.hdr10 && !presenter.stand_in && formatted(presenter,eight) && Sized(presenter,131,89),"T5 an 8-bit chain once HDR10 is no longer asked");
                presenter.hdr_wanted=true;presenter.Resize(gpu,131,89);
                Require(presenter.hdr10 && !presenter.stand_in && formatted(presenter,ten),"T5 the HDR10 chain again");}
            (void)gpu.Drain(WaitOther);}
        // T6: the probe of the control window's screen (printed, not required: a session without a screen has none; the step review's L6).
        const auto display=ProbeHdrDisplay(window);
        gpu.Close();DestroyWindow(window);
        std::printf("PASS frame presenter %s: T1 an accepted resize takes the chain's buffers; T2 a refused resize to another size throws nothing and leaves stand-ins of the asked size, the follow's refused resize keeps them; T3 the chain's buffers come back once the resize succeeds; T4 a refused resize at the same size keeps the chain's buffers; T5 an HDR10 chain asked: %s; T6 the window's screen: %s, HDR %s, colour space %u, %u bits, %.0f nits at most\n",argv[1],
            hdr_taken?"the colour space taken, 10-bit buffers and stand-ins, 8-bit and back":"the colour space refused, an 8-bit chain with its reason",
            display.found?"found":"none",display.hdr?"on":"off",display.color_space,display.bits,double(display.max_nits));
        return 0;
    }catch(const std::exception& error){std::fprintf(stderr,"FAIL frame presenter: %s\n",error.what());if(window)DestroyWindow(window);return 1;}
}
