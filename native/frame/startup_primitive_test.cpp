#include "game_primitive_geometry.h"
#include "../renderer/render_worker.h"
#include <d3dcompiler.h>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <cfloat>

using namespace ds3native;
void Require(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
template<class F> void Refuses(F action) {bool refused{};try{action();}catch(const std::invalid_argument&){refused=true;}Require(refused,"invalid startup request accepted");}
frame::PrimitiveProgram Program(uint64_t identity,ds3content::ShaderStage stage,const char* source) {
    Microsoft::WRL::ComPtr<ID3DBlob> code,errors;
    const auto status=D3DCompile(source,std::strlen(source),nullptr,nullptr,nullptr,"main",
        stage==ds3content::ShaderStage::Vertex?"vs_5_0":"ps_5_0",D3DCOMPILE_ENABLE_STRICTNESS,0,&code,&errors);
    Require(SUCCEEDED(status),"control shader compilation");const auto* bytes=static_cast<const uint8_t*>(code->GetBufferPointer());
    ds3content::CpuShaderProgram program{0,"startup-control",stage,5,0,{bytes,bytes+code->GetBufferSize()}};
    return {identity,std::move(program)};
}
int main(int argc,char** argv) {
    HWND window{};
    try {
        Require(argc==2,"warp or hardware required");const bool warp=!std::strcmp(argv[1],"warp");
        Require(warp || !std::strcmp(argv[1],"hardware"),"unknown profile");
        constexpr uint32_t width=83,height=61;
        window=CreateWindowExW(0,L"STATIC",L"Native startup primitive control",WS_POPUP,0,0,width,height,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
        Require(window!=nullptr,"control window");SceneRenderer renderer;renderer.Start(window,warp,true,width,height);
        const auto loading=renderer.RenderStartup(1);Require(!loading.primitive_draw_count && !loading.debug_errors,"loading clear control");
        bootstrap::PrimitiveGeometryStorage storage;auto write=storage.Allocate(7,8,3,3);
        // Synthetic control only. These shaders/vertices are never installed
        // in the game, admitted as its content, or used to draw substitute UI.
        const float vertices[]={-.8f,-.8f,.8f,-.8f,0,.8f};const uint16_t indices[]={0,1,2};
        std::memcpy(write->Vertices(),vertices,sizeof(vertices));std::memcpy(write->Indices(),indices,sizeof(indices));
        const auto geometry=storage.Seal(write);
        frame::PrimitiveWorkSource source;source.content_incarnation=7;source.generation=source.texture_generation=1;
        source.serial=2;source.width=width;source.height=height;
        source.programs={Program(101,ds3content::ShaderStage::Vertex,"float4 main(float2 p:POSITION):SV_Position{return float4(p,0,1);}"),
            Program(102,ds3content::ShaderStage::Pixel,"float4 main():SV_Target{return float4(1,0,0,1);}")};
        source.buffers={geometry->vertices,geometry->indices};frame::PrimitiveDraw draw;
        // Exercise actual PrimitivePass allocation/upload/retirement for every
        // admitted UI image format, including non-block-aligned dimensions.
        for(const auto format:std::array<DXGI_FORMAT,7>{DXGI_FORMAT_R8_UNORM,DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_FORMAT_B8G8R8A8_UNORM,DXGI_FORMAT_BC1_UNORM,DXGI_FORMAT_BC2_UNORM,DXGI_FORMAT_BC3_UNORM,DXGI_FORMAT_BC7_UNORM}) {
            frame::PrimitiveTextureLayout layout;Require(frame::GetPrimitiveTextureLayout(7,5,format,layout),"UI format layout");
            SceneTexture texture;texture.width=7;texture.height=5;texture.format=format;
            texture.mips.push_back(std::vector<uint8_t>(size_t(layout.row_bytes)*layout.rows,0));
            source.textures.push_back({uint64_t(200+source.textures.size()),std::move(texture)});
        }
        draw.identity=1;draw.vertex_program=101;draw.pixel_program=102;draw.vertices=geometry->vertices.identity;draw.indices=geometry->indices.identity;
        draw.elements={{"POSITION",0,0,DXGI_FORMAT_R32G32_FLOAT}};draw.stride=8;draw.index_width=2;draw.count=3;draw.indexed=true;
        draw.viewport={0,0,float(width),float(height),0,1};draw.blend=frame::PrimitiveBlend::StraightAlpha;
        source.draws={draw};frame::PrimitiveWorkPacket packet;
        Require(frame::ProducePrimitiveWork(source,packet).status==frame::PrimitiveWorkStatus::Ready,"owned control transaction");
        {   // The moving admission yields the same packet; a refused source keeps its payloads and the prior output.
            auto moved=source;frame::PrimitiveWorkPacket by_move;
            Require(frame::ProducePrimitiveWork(std::move(moved),by_move).status==frame::PrimitiveWorkStatus::Ready &&
                by_move.programs.size()==packet.programs.size() && by_move.buffers.size()==packet.buffers.size() && by_move.draws.size()==packet.draws.size() &&
                by_move.buffers.front().bytes==packet.buffers.front().bytes && by_move.programs.front().program.bytecode==packet.programs.front().program.bytecode,"moving admission differs");
            auto refused=source;refused.width=0;const auto kept=by_move.draws.size();
            Require(frame::ProducePrimitiveWork(std::move(refused),by_move).status==frame::PrimitiveWorkStatus::Invalid &&
                refused.draws.size()==source.draws.size() && refused.buffers.front().bytes==source.buffers.front().bytes && by_move.draws.size()==kept && by_move.width==packet.width,"refused moving admission consumed its source");
        }
        renderer.AdmitPrimitiveWork(packet);storage.Withdraw(7);
        Refuses([&]{renderer.RenderStartupPrimitives(2,2,true);});
        Refuses([&]{renderer.RenderStartupPrimitives(3,1,true);});
        Refuses([&]{renderer.RenderStartupPrimitives(2,1,false);});Require(!renderer.Faulted(),"invalid request poisoned owner");
        const auto ui=renderer.RenderStartupPrimitives(2,1,true);
        Require(ui.content_incarnation==7 && ui.primitive_draw_count==1 && ui.fence && ui.presentation_fence && !ui.debug_errors,"startup primitive execution receipt");
        Refuses([&]{renderer.AdmitPrimitiveWork(packet);});renderer.WithdrawPrimitiveWork(1);
        Refuses([&]{renderer.RenderStartupPrimitives(3,1,true);});
        const auto empty=renderer.RenderStartup(3);Require(!empty.primitive_draw_count && !empty.debug_errors,"withdrawn primitive still drawn");
        source.programs[1]=Program(102,ds3content::ShaderStage::Pixel,
            "Texture2D<float4> t:register(t0);SamplerState s:register(s0);float4 main(float4 p:SV_Position):SV_Target{return float4(t.SampleLevel(s,p.xy/11,0).rrr,1);}");
        source.serial=3;
        for(const auto sampler:std::array{frame::PrimitiveSampler::PointWrap,frame::PrimitiveSampler::PointClamp,
            frame::PrimitiveSampler::LinearWrap,frame::PrimitiveSampler::LinearClamp}){
            source.generation++;source.serial++;source.draws[0].texture=source.textures[0].identity;
            source.draws[0].sampler=sampler;source.draws[0].mip_bias=-.75f;source.draws[0].max_lod=FLT_MAX;
            Require(frame::ProducePrimitiveWork(source,packet).status==frame::PrimitiveWorkStatus::Ready,"textured native control preflight");
            renderer.AdmitPrimitiveWork(packet);const auto textured=renderer.RenderStartupPrimitives(source.serial,source.generation,true);
            Require(textured.primitive_draw_count==1 && !textured.debug_errors && textured.fence,"native authored sampler execution");
            renderer.WithdrawPrimitiveWork(source.generation);
        }
        // F1 (I-0067 SCALEFORM-FILTERS-NATIVE): a draw into a packet target, then a frame draw sampling that target; a draw
        // sampling the target it writes, a viewport past its target and an unknown target are refused.
        {source.programs.push_back(Program(103,ds3content::ShaderStage::Pixel,"float4 main():SV_Target{return float4(1,0,0,1);}"));
            source.targets={{900,16,12}};SceneTexture shape;shape.width=16;shape.height=12;shape.format=DXGI_FORMAT_R8G8B8A8_UNORM;shape.mips.emplace_back();
            frame::PrimitiveTexture sampled{950,shape};sampled.target=900;source.textures.push_back(sampled);
            auto into=source.draws[0];into.identity=2;into.target=900;into.pixel_program=103;into.texture=0;into.sampler=frame::PrimitiveSampler::None;
            into.viewport={0,0,16,12,0,1};
            auto from=source.draws[0];from.identity=3;from.texture=950;from.sampler=frame::PrimitiveSampler::LinearClamp;
            source.generation++;source.serial++;source.draws={into,from};
            Require(frame::ProducePrimitiveWork(source,packet).status==frame::PrimitiveWorkStatus::Ready,"target transaction preflight");
            renderer.AdmitPrimitiveWork(packet);const auto targeted=renderer.RenderStartupPrimitives(source.serial,source.generation,true);
            Require(targeted.primitive_draw_count==2 && !targeted.debug_errors && targeted.fence,"target draw then sample execution");
            renderer.WithdrawPrimitiveWork(source.generation);
            auto self=source;self.draws[0].texture=950;self.draws[0].sampler=frame::PrimitiveSampler::LinearClamp;
            Require(frame::ProducePrimitiveWork(self,packet).status==frame::PrimitiveWorkStatus::Invalid,"draw sampling its own target accepted");
            auto wide=source;wide.draws[0].viewport={0,0,17,12,0,1};
            Require(frame::ProducePrimitiveWork(wide,packet).status==frame::PrimitiveWorkStatus::Invalid,"viewport past its target accepted");
            auto unknown=source;unknown.draws[0].target=901;
            Require(frame::ProducePrimitiveWork(unknown,packet).status==frame::PrimitiveWorkStatus::Invalid,"unknown target accepted");
            // F6 shape: a subtree target, two ping-pong passes (one target per use, lives that end and start at one draw
            // never share a resource), a target first referenced by a sample (cleared, never drawn), then a composite
            // sampling the last pass at t0 and the retained subtree at t1.
            source.programs.push_back(Program(104,ds3content::ShaderStage::Pixel,
                "Texture2D<float4> a:register(t0);Texture2D<float4> b:register(t1);SamplerState s:register(s0);SamplerState u:register(s1);"
                "float4 main(float4 p:SV_Position):SV_Target{return a.SampleLevel(s,p.xy/16,0)*.5+b.SampleLevel(u,p.xy/16,0)*.5;}"));
            source.targets={{900,16,12},{901,16,12},{902,16,12},{903,16,12},{904,16,12}};
            for(uint64_t t=0;t<4;++t){frame::PrimitiveTexture view{951+t,shape};view.target=901+t;source.textures.push_back(view);}
            auto subtree=into;subtree.identity=4;
            auto pass0=source.draws[1];pass0.identity=5;pass0.target=901;pass0.texture=950;pass0.viewport={0,0,16,12,0,1};
            auto pass1=pass0;pass1.identity=6;pass1.target=902;pass1.texture=951;
            auto blank=pass0;blank.identity=7;blank.target=904;blank.texture=953;   // 903 starts here, read only
            auto composite=pass0;composite.identity=8;composite.target=0;composite.pixel_program=104;composite.texture=952;
            composite.secondary={950,frame::PrimitiveSampler::LinearClamp,-.75f,0,FLT_MAX};composite.viewport={0,0,float(width),float(height),0,1};
            source.generation++;source.serial++;source.draws={subtree,pass0,pass1,blank,composite};
            Require(frame::ProducePrimitiveWork(source,packet).status==frame::PrimitiveWorkStatus::Ready,"filter chain preflight");
            renderer.AdmitPrimitiveWork(packet);const auto chain=renderer.RenderStartupPrimitives(source.serial,source.generation,true);
            Require(chain.primitive_draw_count==5 && !chain.debug_errors && chain.fence,"filter chain execution");
            renderer.WithdrawPrimitiveWork(source.generation);}
        // R13 cR2 (T-R5, inline part, X1): an admitted build is a snapshot the next admission leaves untouched; a frame job
        // draws the build it holds; a build of another serial, a wrong generation and a new build whose stencil the frame
        // does not initialize are refused without poisoning the renderer.
        {const auto base=source.generation;source.serial++;source.generation++;
            Require(frame::ProducePrimitiveWork(source,packet).status==frame::PrimitiveWorkStatus::Ready,"snapshot A preflight");
            const auto a=renderer.AdmitPrimitiveWork(packet);const auto a_draws=a->Packet()->draws.size();
            source.serial++;source.generation++;
            Require(frame::ProducePrimitiveWork(source,packet).status==frame::PrimitiveWorkStatus::Ready,"snapshot B preflight");
            const auto b=renderer.AdmitPrimitiveWork(packet);
            Require(a->Packet() && a->Packet()->generation==base+1 && a->Packet()->draws.size()==a_draws && b->Packet()->generation==base+2,"admission modified the held build");
            NativeFrameJob job;job.serial=source.serial;job.ui=a;job.ui_generation=base+1;job.initialize_ui_stencil=true;
            Refuses([&]{renderer.ExecuteFrame(job);});   // A was built for the previous serial
            job.ui=b;Refuses([&]{renderer.ExecuteFrame(job);});   // the job names A's generation
            job.ui_generation=base+2;job.initialize_ui_stencil=false;Refuses([&]{renderer.ExecuteFrame(job);});   // B's stencil is not initialized
            job.initialize_ui_stencil=true;const auto held=renderer.ExecuteFrame(job);
            Require(held.primitive_draw_count==b->Packet()->draws.size() && !held.debug_errors && held.fence && !renderer.Faulted(),"frame job execution");
            renderer.WithdrawPrimitiveWork(source.generation);}
        // R13 cR3 (T-R5, affinity): with a render thread set, a frame job built on the owner thread runs on the render worker
        // and is refused on the owner. I-0078: the UI admission then belongs to the render thread: refused on the owner, and a
        // job carrying its packet is built by the worker before its frame; neither refusal faults the renderer.
        {RenderWorker worker({30000,false});worker.Start();renderer.SetRenderThread(worker.ThreadId());
            const auto refused_logic=[](auto action){try{action();}catch(const std::invalid_argument&){return false;}catch(const std::logic_error&){return true;}return false;};
            source.serial++;source.generation++;
            Require(frame::ProducePrimitiveWork(source,packet).status==frame::PrimitiveWorkStatus::Ready,"render thread preflight");
            Require(refused_logic([&]{renderer.AdmitPrimitiveWork(packet);}),"UI admission accepted on the owner with a render thread");
            NativeFrameJob job;job.serial=source.serial;job.ui_packet=std::make_shared<frame::PrimitiveWorkPacket>(packet);
            job.ui_generation=source.generation;job.initialize_ui_stencil=true;
            Require(refused_logic([&]{renderer.ExecuteFrame(job);}),"frame job accepted off the render thread");
            FrameReceipt receipt;
            worker.Post([&]{renderer.WaitFrameSlot();receipt=renderer.ExecuteFrame(job);});
            worker.Quiesce();worker.RethrowFault();worker.Resume();
            Require(receipt.primitive_draw_count==packet.draws.size() && receipt.fence && !receipt.debug_errors && renderer.PrimitiveBuildCurrent() && !renderer.Faulted(),
                "frame job on the render thread");
            Require(refused_logic([&]{renderer.WaitFrameSlot();}),"slot wait accepted off the render thread");
            worker.Stop();renderer.SetRenderThread(0);renderer.WithdrawPrimitiveWork(source.generation);}
        // D3D12 review R2 (phase B2c): arena ranges are 256-byte aligned, zero-filled and distinct within a submission;
        // their chunk is kept until the fence passes the retirement ticket, then reused; a range above 1 MB takes a chunk of
        // the next size class (cD3), pooled in that class; an unsubmitted ticket is refused.
        {auto& queues=renderer.CpuIngressDevice();auto* device=queues.device.Get();UploadArena arena;
            const auto a=arena.Allocate(device,100),b=arena.Allocate(device,300);
            Require(a.cpu && !(a.gpu%256) && b.gpu==a.gpu+256 && b.cpu==a.cpu+256 && !a.cpu[99] && !b.cpu[299],"arena range layout");
            std::memset(a.cpu,0xAB,100);
            bool refused{};try{arena.Retire(queues,queues.LastSubmitted()+1);}catch(const std::invalid_argument&){refused=true;}
            Require(refused,"arena accepted an unsubmitted ticket");
            const auto before=queues.PendingRetirements();arena.Retire(queues,queues.Drain());
            Require(queues.PendingRetirements()==before+1 && !arena.FreeChunks(),"arena chunk not retired by ticket");
            queues.CollectCompleted();Require(arena.FreeChunks()==1,"completed arena chunk not returned");
            const auto c=arena.Allocate(device,64);Require(c.cpu==a.cpu && !c.cpu[0] && !c.cpu[63],"reused arena range");
            const auto big=arena.Allocate(device,UploadArena::chunk_bytes+1);Require(big.cpu && !(big.gpu%256),"oversized arena range");
            arena.Retire(queues,queues.Drain());queues.CollectCompleted();
            Require(arena.FreeChunks()==2 && arena.FreeChunks(1)==1 && UploadArenaChunksCreated()>=2,"4 MB class arena chunk not pooled");
            arena.Reset();Require(!arena.FreeChunks(),"arena reset kept chunks");}
        // D3D12 review R3 (phase B1): an owner retired after a submitted ticket stays held until the fence passes it,
        // is released by the next collection, an unsubmitted ticket is refused, and Close releases what is still held.
        {auto& queues=renderer.CpuIngressDevice();
            queues.Drain();queues.CollectCompleted();   // the UI chunks and targets of the frames above, retired by ticket (B2)
            Require(!queues.PendingRetirements(),"retirement list not empty after collection");
            auto held=std::make_shared<int>(1);std::weak_ptr<int> watch=held;
            const auto ticket=queues.LastSubmitted();Require(ticket,"no submitted ticket");
            queues.RetireAfter(ticket,std::move(held));
            bool refused{};try{queues.RetireAfter(ticket+1000,std::make_shared<int>(2));}catch(const std::invalid_argument&){refused=true;}
            Require(refused && queues.PendingRetirements()==1,"unsubmitted retirement ticket accepted");
            queues.Drain();Require(queues.CollectCompleted()==1 && watch.expired() && !queues.PendingRetirements(),"completed retirement not released");
            auto late=std::make_shared<int>(3);std::weak_ptr<int> late_watch=late;
            queues.RetireAfter(queues.LastSubmitted(),std::move(late));Require(queues.PendingRetirements()==1,"retirement not recorded");
            renderer.Close();Require(late_watch.expired() && !queues.PendingRetirements(),"close kept a retired owner");}
        renderer.Close();
        // D3D12 phase D cD6 (T-D3 at the renderer level): two frame slots. Frames submit without a completion wait, their
        // tickets increase, their release-time results arrive through the channel when their slot is reused (frame N's at
        // frame N+2), the slot count is fixed once started, and close settles the frames still in flight.
        // Frame 3 is recorded and submitted behind a queue wait on a fence the test signals later, so frame 4 provably
        // records while frame 3 has not executed (forced overlap); the startup site takes no completion wait or drain.
        {SceneRenderer two;two.SetFrameSlots(2);two.Start(window,warp,true,width,height);Require(two.FrameSlots()==2,"two frame slots");
            auto& queues=two.CpuIngressDevice();Microsoft::WRL::ComPtr<ID3D12Fence> gate;
            Require(SUCCEEDED(queues.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&gate))),"two-slot gate");
            const uint64_t drains_before=gpu_audit.drains[WaitStartup],completes_before=gpu_audit.completes[WaitStartup];
            uint64_t last{},gated{};std::vector<uint64_t> completed;
            for(uint64_t serial=1;serial<=6;++serial){
                if(serial==3)Require(SUCCEEDED(queues.queue->Wait(gate.Get(),1)),"two-slot queue wait");
                two.WaitFrameSlot();const auto receipt=two.RenderStartup(serial);
                Require(receipt.serial==serial && receipt.fence && receipt.presentation_fence>receipt.fence && receipt.presentation_fence>last,"two-slot receipt");
                last=receipt.presentation_fence;if(serial==3)gated=receipt.presentation_fence;
                if(serial==4){Require(queues.CompletedValue()<gated,"two-slot frame 4 recorded without overlap");
                    Require(SUCCEEDED(gate->Signal(1)),"two-slot gate release");}
                for(const auto& frame:two.TakeCompleted()){Require(!frame.debug_errors,"two-slot completed frame error");completed.push_back(frame.serial);}}
            Require(completed==std::vector<uint64_t>{1,2,3,4},"two-slot completions not two frames late");
            Require(gpu_audit.drains[WaitStartup]==drains_before && gpu_audit.completes[WaitStartup]==completes_before,"two-slot frames waited");
            bool refused{};try{two.SetFrameSlots(3);}catch(const std::invalid_argument&){refused=true;}
            Require(refused,"frame slots changed after start");two.Close();
            // Close released the two frames still in flight: their results are in the channel.
            std::vector<uint64_t> closed;for(const auto& frame:two.TakeCompleted()){Require(!frame.debug_errors,"two-slot close error");closed.push_back(frame.serial);}
            Require(closed==std::vector<uint64_t>{5,6},"two-slot close did not release the frames in flight");}
        // cD8: three back buffers and the frame-latency waitable object at two slots: frames wait on it before they record,
        // the chain reports its three buffers, the presentation is fixed once started.
        {SceneRenderer three;three.SetFrameSlots(2);three.SetPresentation(3,true);three.Start(window,warp,true,width,height);
            for(uint64_t serial=1;serial<=8;++serial){three.WaitFrameSlot();const auto receipt=three.RenderStartup(serial);
                Require(receipt.presentation_fence && !receipt.debug_errors,"waitable receipt");(void)three.TakeCompleted();}
            uint32_t view_width{},view_height{};Require(three.ReadStartupView(view_width,view_height) && view_width==width,"waitable chain view");
            Require(!three.WaitableTimeouts(),"waitable wait timed out");
            bool refused{};try{three.SetPresentation(2,false);}catch(const std::invalid_argument&){refused=true;}
            Require(refused,"presentation changed after start");three.Close();}
        DestroyWindow(window);window=nullptr;
        std::printf("PASS %s startup primitives: shared renderer before scene readiness, moving admission equal to the copy and refusal leaving its source, retirement tokens (R3), upload arena ranges retired by ticket (R2 B2c), owned geometry, seven UI image formats/odd extents, all four UI samplers executed, packet target drawn then sampled (F1), filter pass chain with per-use targets and a sampled-only target (F6), explicit stencil, current serial/generation, frame job holding an admitted build the next admission leaves untouched (T-R5 inline), the frame job on the render worker and the affinity refusals (T-R5), withdrawal, native submission and D3D12 validation\n",argv[1]);return 0;
    }catch(const std::exception& error){if(window)DestroyWindow(window);std::fprintf(stderr,"FAIL: %s\n",error.what());return 1;}
}
