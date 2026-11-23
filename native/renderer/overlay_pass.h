#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "overlay_frame.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace ds3native {
// I-0132 R12a (R12-PLAN « R12a's design »): the native overlay, drawn after the game's UI into the same target (the display
// stage's intermediate, or the back buffer without it), before the gamma pass: our pages' quads (overlay_frame.h) alpha-blended in
// order. The engine thread hands the latest frame (Set); the recording thread draws it (Record). Each image of bytes is a texture
// made again whenever its version changes (COPY_DEST, copied, then PIXEL_SHADER_RESOURCE for good), an external one (a texture the
// native holds, R12a's second tranche) is bound as it is; the images of a frame sit in one
// block of a shader-visible heap, and a block (with its textures) returns to the free list only when every recorded frame that used
// it has completed: Record returns a hold for the frame slot's kept list.
class OverlayPass {
public:
    using Allocate=std::function<std::pair<uint8_t*,D3D12_GPU_VIRTUAL_ADDRESS>(uint64_t bytes)>;   // the frame's upload arena
    OverlayPass();
    ~OverlayPass();
    OverlayPass(const OverlayPass&)=delete;
    OverlayPass& operator=(const OverlayPass&)=delete;
    // `Blend::Screen` (I-0132 R12e): a screen blend instead of the alpha blend (target + colour x alpha x (1 - target)): light that shows on
    // the dark parts of what is under it and leaves its white parts as they are (the title's embers over DS3's opaque title movie).
    // `Blend::Multiply` (R12e, Thomas 2026-11-12 02:14: DS3's white logo tinted orange as his mock-up): target x (1 - alpha + colour x
    // alpha), black kept black, white toward the colour.
    enum class Blend : uint8_t {Alpha,Screen,Multiply};
    bool Open(ID3D12Device* device,DXGI_FORMAT target,std::string& reason,Blend blend=Blend::Alpha);
    bool IsOpen() const noexcept {return pipeline_!=nullptr;}
    bool FloatFailed() const noexcept {return float_failed_;}
    const std::string& FloatFailure() const noexcept {return float_failure_;}   // read once FloatFailed, on the recording thread
    // The engine thread's latest frame (null: nothing drawn); an invalid frame is refused and counted.
    void Set(std::shared_ptr<const overlay::Frame> frame);
    // The recording thread's snapshot of the latest frame, taken once a recorded frame (the step review's H1: the frame graph's plan
    // key, the pass's declaration and the draw all see this one): null when none, an empty one, or one laid out for another size.
    std::shared_ptr<const overlay::Frame> Take(uint32_t width,uint32_t height);
    // Records `frame` (from Take) into `target` (RENDER_TARGET, `width` x `height`): its uploads (the staging buffers and the new
    // textures into `kept`), then the quads. Returns the quads drawn.
    // `float_target` (I-0138 tranche 5): the target is an R16G16B16A16_FLOAT picture (an HDR10 chain's), drawn with the pass's
    // second pipeline (the same programme and blend), made at the first such Record; were it to fail, nothing is drawn on float
    // targets from then on (FloatFailed) and the pass goes on as before on the others.
    uint32_t Record(ID3D12GraphicsCommandList* list,D3D12_CPU_DESCRIPTOR_HANDLE target,uint32_t width,uint32_t height,
        const std::shared_ptr<const overlay::Frame>& frame,const Allocate& allocate,std::vector<std::shared_ptr<const void>>& kept,bool float_target=false);
    struct Counts {uint64_t frames{},quads{},uploads{},upload_bytes{},refused{},skipped_size{},stale_blocks{};};
    Counts TakeCounts() noexcept;
    void Close() noexcept;
    void QuarantineToProcessExit() noexcept;
private:
    struct Block;
    std::shared_ptr<Block> TakeBlock();
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_,pipeline_float_;
    D3D12_SHADER_BYTECODE vs_{},ps_{};Blend mode_{Blend::Alpha};bool float_failed_{};std::string float_failure_;   // what a pipeline is made from
    void MakePipeline(DXGI_FORMAT target,Microsoft::WRL::ComPtr<ID3D12PipelineState>& out,const char* what);
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap_;
    uint32_t stride_{};
    mutable std::mutex lock_;std::shared_ptr<const overlay::Frame> pending_;   // Set's, taken by Record
    // The recording thread's: the textures by identity (their version), the current block and its images.
    struct Texture {uint64_t version{};uint32_t width{},height{};overlay::Format format{};Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        DXGI_FORMAT view_format{DXGI_FORMAT_UNKNOWN};};   // an external texture's own format (R12a's second tranche)
    std::unordered_map<uint64_t,Texture> textures_;
    std::shared_ptr<Block> block_;std::vector<std::pair<uint64_t,uint64_t>> block_images_;   // (identity, version) in the frame's order
    std::shared_ptr<std::mutex> free_lock_;std::shared_ptr<std::vector<uint32_t>> free_;   // shared with the blocks' deleters
    std::shared_ptr<std::atomic<uint64_t>> generation_shared_;std::atomic<uint64_t>& generation_;   // each Close a new generation
    std::atomic<uint64_t> frames_{},quads_{},uploads_{},upload_bytes_{},refused_{},skipped_size_{},stale_blocks_{};
    static constexpr uint32_t kBlocks=32;
};
}
