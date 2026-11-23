// Engine programme lookup and the programme sets of the native passes: composed shader groups, deferred lighting, glow, AA,
// AO, YEBIS, tone map, grading and glare. Part of game_bootstrap.cpp's single translation unit (the order is there): it
// relies on every earlier part (05_selected_scene.inc before it).
#pragma once
#include "05_selected_scene.inc"
namespace {
// shader1021-1024: D467E0/AA630 compose a GXFlverShaderGroup by name. For pass
// Gbuf the pixel programmes are "<group>_Gbuf[_[A]|_[AA]]" from callback 0 =
// D1E640 (repository 4792BF8) and the vertex programmes "<group>_Gbuf<skin><ins>
// <channel>" from callback 1 = D1E710 (repository 4792D58). Each repository's
// +60 is a hash container (2356180: bucket count +1C, buckets +20; node name
// wstring +10, next +50); the programme wrapper is node+68, the receiver our
// AdmitShader owns. Natively the matching loaded programmes become one owned
// effect named like an SPX package, so SelectModelPrograms resolves them. No
// old device, group object (A9BA0) or callback is invoked.
std::mutex composed_effect_lock;
std::map<std::wstring,std::shared_ptr<const NativeEffect>> composed_effects;
bool ReadEngineWide(uintptr_t string,std::wstring& output) noexcept {
    uint64_t length{},capacity{};uintptr_t chars=string;
    if(!Read(string+0x10,&length,8) || !Read(string+0x18,&capacity,8) || !length || length>512 || capacity<length)return false;
    if(capacity>7 && !Read(string,&chars,8))return false;
    output.assign(static_cast<size_t>(length),L'\0');
    return Read(chars,output.data(),output.size()*2);
}
template<class Programs> std::string NativeFamilyKey(const Programs& programs);
// 1048 witness for the native lighting family: which deferred-lighting and
// post programmes the engine's per-stage repositories hold natively, with the
// native residency of their wrappers. Read only; once per run, after zone load.
void WitnessLightingProgrammes() noexcept {
    static LONG64 done{};if(InterlockedIncrement64(&done)!=1)return;
    constexpr const wchar_t* patterns[]{L"GXLight",L"DepthScale",L"SSAO",L"EnvMap",L"Composite",L"ToneMap",L"Shadow",L"SSS",L"Fog",L"Blur",L"Down",L"Copy",L"Post",L"Lum",L"Bloom",L"FXAA",L"AA"};
    constexpr std::pair<uintptr_t,const char*> repositories[]{{0x4792BF8,"ps"},{0x4792D58,"vs"},{0x4792B08,"hs"},{0x4792838,"ds"},
        {0x4792A18,"gs"},{0x4792748,"cs"}};
    for(const auto& [rva,stage]:repositories){
        uintptr_t repository{},buckets{};uint32_t count{};
        if(!Read(image+rva,&repository,8) || !repository || !Read(repository+0x60+0x1C,&count,4) || !count || count>(1u<<20) ||
           !Read(repository+0x60+0x20,&buckets,8) || !buckets)continue;
        std::string names;size_t matched{},resident{},visited{};
        for(uint32_t bucket=0;bucket<count && visited<200000;++bucket){
            uintptr_t node{};if(!Read(buckets+bucket*8ull,&node,8))break;
            for(;node && visited<200000;++visited){
                std::wstring name;
                if(ReadEngineWide(node+0x10,name) && name.rfind(L"GXFlver",0)!=0 && std::any_of(std::begin(patterns),std::end(patterns),[&](const wchar_t* p){return name.find(p)!=std::wstring::npos;})){
                    ++matched;bool owned=false;{const std::lock_guard hold(shader_lock);owned=shaders.count(node+0x68)!=0;}
                    if(owned)++resident;
                    if(names.size()<3000){names+=names.empty()?"":",";for(const auto ch:name)names.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');if(!owned)names+="(x)";}
                }
                if(!Read(node+0x50,&node,8))break;
            }
        }
        std::string row="{\"event\":\"native_lighting_programmes\",\"stage\":\""+std::string(stage)+"\",\"records\":"+std::to_string(visited)+
            ",\"matched\":"+std::to_string(matched)+",\"resident\":"+std::to_string(resident)+",\"names\":\""+names+"\"}\n";Log(row.c_str());
    }
}
// 1049: one loaded programme by bare name (keys may carry the extension) from a
// per-stage repository, with the expected DXBC programme type. Read only.
// Optimisation review (Native2201): each call walked the whole repository table (0.27 s of the zone load's first frames,
// mostly the absent `<family>_Phantom_Fwd` names asked again for every new layout). A found programme is kept per
// repository, type and case-folded name while no shader is admitted or retired (shader_registry_changes); an absent one
// for 250 ms only, since the engine may link a repository node after its shader's admission (Native2202: an absence kept
// until the next admission stalled the start at its 64th frame).
std::shared_ptr<const NativeShader> ScanLoadedProgramme(uintptr_t repository_rva,const wchar_t* wanted,UINT type);
std::shared_ptr<const NativeShader> FindLoadedProgramme(uintptr_t repository_rva,const wchar_t* wanted,UINT type) {
    struct Known {std::shared_ptr<const NativeShader> shader;int64_t absent_until{};};
    static std::mutex lock;static std::map<std::tuple<uintptr_t,UINT,std::wstring>,Known> known;static uint64_t known_changes{};
    static const int64_t frequency=[]{LARGE_INTEGER f{};QueryPerformanceFrequency(&f);return int64_t(f.QuadPart);}();
    LARGE_INTEGER now{};QueryPerformanceCounter(&now);
    std::wstring folded(wanted);for(auto& ch:folded)ch=static_cast<wchar_t>(towlower(ch));
    auto key=std::make_tuple(repository_rva,type,std::move(folded));
    const auto changes=shader_registry_changes.load(std::memory_order_acquire);
    {const std::lock_guard hold(lock);if(known_changes!=changes){known.clear();known_changes=changes;}
        if(const auto found=known.find(key);found!=known.end() && (found->second.shader || now.QuadPart<found->second.absent_until))return found->second.shader;}
    auto result=ScanLoadedProgramme(repository_rva,wanted,type);
    {const std::lock_guard hold(lock);if(known_changes==changes && known.size()<65536)known.insert_or_assign(std::move(key),Known{result,result?0:now.QuadPart+frequency/4});}
    return result;
}
std::shared_ptr<const NativeShader> ScanLoadedProgramme(uintptr_t repository_rva,const wchar_t* wanted,UINT type) {
    uintptr_t repository{},buckets{};uint32_t count{};
    if(!Read(image+repository_rva,&repository,8) || !repository || !Read(repository+0x60+0x1C,&count,4) || !count || count>(1u<<20) ||
       !Read(repository+0x60+0x20,&buckets,8) || !buckets)return {};
    const size_t length=std::wcslen(wanted);size_t visited{};
    for(uint32_t bucket=0;bucket<count && visited<200000;++bucket){
        uintptr_t node{};if(!Read(buckets+bucket*8ull,&node,8))break;
        for(;node && visited<200000;++visited){
            std::wstring name;
            if(ReadEngineWide(node+0x10,name) && (name.size()==length || (name.size()==length+4 && name[length]==L'.')) &&
               _wcsnicmp(name.c_str(),wanted,length)==0){
                const std::lock_guard hold(shader_lock);const auto found=shaders.find(node+0x68);
                if(found!=shaders.end() && D3D11_SHVER_GET_TYPE(found->second->version)==type)return found->second;
            }
            if(!Read(node+0x50,&node,8))break;
        }
    }
    return {};
}
// The deferred-lighting programmes of K-0407 the native lighting runs, from the
// engine's loaded repositories (VS 4792D58, PS 4792BF8). Unresolved: no lighting.
// I-0137 (I-0120 C4): the set is still published once and kept for the session (the renderer makes its lighting pipelines for
// one set and refuses a change inside a frame's list: a set replaced between two collects of one frame would close the game).
// What changed is when: a set whose five required programmes are there but which lacks an optional member (the light grid's
// GXLightOrganize, the SSS three, a point or spot size, the mask composite, the shadowed sun, since I-0140 tranche 28 the env regions'
// three, D-0301) is no longer published at once.
// The engine may link a repository node after its shader's admission (Native2202), so the answer stays « unresolved » (no
// lighting yet, as while a required programme is missing) for up to two seconds from the first asking that found the five,
// asking again at each call (FindLoadedProgramme keeps an absence 250 ms); then what is there is published, `whole` false in
// its row. Never met in a run: the 17 journals read for I-0137 show the point, spot, grid, SSS and shadowed-sun members at the
// first asking (the mask composite has no row of its own there; three witness journals list it resident; the env regions' three are
// not counted in those journals: the native_lighting_programmes_resolved row's env_bytes and `whole` say it).
std::shared_ptr<const NativeLightingPrograms> AcquireNativeLightingPrograms() {
    static std::mutex lock;static std::shared_ptr<const NativeLightingPrograms> resolved;
    static ULONGLONG wait_from{};   // the first asking that found the five required programmes without every optional member
    constexpr ULONGLONG wait_ms=2000;
    {const std::lock_guard hold(lock);if(resolved)return resolved;}   // cF.6: the lookups below run without this lock (shader_lock is a leaf)
    constexpr const wchar_t* names[]{L"GXLightAcc",L"GXLightDeferredAO",L"GXLightAcc_DDirLight1",L"GXLightCmbAlbedo",L"GXLightComposite"};
    // 1054: GXLightAcc_DPointLight1..12 and LegacyDPointLight1..12 (each optional).
    std::array<std::shared_ptr<const NativeShader>,24> points;size_t point_count{};
    for(uint32_t i=0;i<24;++i){wchar_t name[64]{};swprintf_s(name,i<12?L"GXLightAcc_DPointLight%u":L"GXLightAcc_LegacyDPointLight%u",i%12+1);
        points[i]=FindLoadedProgramme(0x4792BF8,name,0u);if(points[i])++point_count;}
    // I-0106: GXLightAcc_DSpotLight1..4 (1A6690 by group size; each optional).
    std::array<std::shared_ptr<const NativeShader>,4> spots;size_t spot_count{};
    for(uint32_t i=0;i<4;++i){wchar_t name[64]{};swprintf_s(name,L"GXLightAcc_DSpotLight%u",i+1);
        spots[i]=FindLoadedProgramme(0x4792BF8,name,0u);if(spots[i])++spot_count;}
    std::array<std::shared_ptr<const NativeShader>,5> found;std::string missing;
    for(size_t i=0;i<5;++i){found[i]=FindLoadedProgramme(i?0x4792BF8:0x4792D58,names[i],i?0u:1u);
        if(!found[i]){missing+=missing.empty()?"":",";for(const wchar_t* c=names[i];*c;++c)missing.push_back(static_cast<char>(*c));}}
    static LONG64 reported{};
    if(!missing.empty()){if(InterlockedIncrement64(&reported)%600==1){std::string row="{\"event\":\"native_lighting_programmes_unresolved\",\"missing\":\""+missing+"\"}\n";Log(row.c_str());}
        return {};}
    auto programs=std::make_shared<NativeLightingPrograms>();
    programs->vertex=found[0]->bytes;programs->ambient=found[1]->bytes;programs->sun=found[2]->bytes;
    programs->albedo=found[3]->bytes;programs->composite=found[4]->bytes;
    for(size_t i=0;i<24;++i)if(points[i])programs->point[i]=points[i]->bytes;
    for(size_t i=0;i<4;++i)if(spots[i])programs->spot[i]=spots[i]->bytes;
    // I-0107: GXLightOrganize (the compute repository; 1A36B0's programme at the tile size 64), optional.
    if(const auto organize=FindLoadedProgramme(0x4792748,L"GXLightOrganize",5u))programs->organize=organize->bytes;
    // 1137 SSS stage (114040: VS GXSss by the vertex getter, GXSssBlurX/Y by the pixel getter); all three or none.
    {const auto vertex=FindLoadedProgramme(0x4792D58,L"GXSss",1u),blur_x=FindLoadedProgramme(0x4792BF8,L"GXSssBlurX",0u),
        blur_y=FindLoadedProgramme(0x4792BF8,L"GXSssBlurY",0u);
        if(vertex && blur_x && blur_y){programs->sss_vertex=vertex->bytes;programs->sss_blur_x=blur_x->bytes;programs->sss_blur_y=blur_y->bytes;}}
    // 1147 env-map regions (manager +230 VS, +1F0/+1F8 PS) and the region box (1400697F0's .rdata data).
    {const auto vertex=FindLoadedProgramme(0x4792D58,L"GXEnvMapRegion",1u),pixel=FindLoadedProgramme(0x4792BF8,L"GXEnvMapRegion",0u),
        pcc=FindLoadedProgramme(0x4792BF8,L"GXEnvMapRegion_Pcc",0u);std::vector<uint8_t> box(124);
        if(vertex && pixel && pcc && Read(image+0x3D20520,box.data(),96) && Read(image+0x3D20500,box.data()+96,28)){
            programs->env_vertex=vertex->bytes;programs->env_pixel=pixel->bytes;programs->env_pixel_pcc=pcc->bytes;programs->env_box=std::move(box);}}
    // D-0180: the shadow and AO mask composite (set +0x30 of [[GXRS+0x4DE8]]); optional (without it t32/t33 stay white).
    if(const auto mask=FindLoadedProgramme(0x4792BF8,L"GXRend_CompositeShadowMask",0u))programs->mask_composite=mask->bytes;
    // D-0180 C4: the shadowed sun (19E090 +0x1B0). I-0131 R10a: the mask layer's programmes (the cascades' Soft and blur, removed) are
    // no longer looked up.
    {const auto sun=FindLoadedProgramme(0x4792BF8,L"GXLightAcc_DDirLightShadowed1",0u);
        if(sun)programs->sun_shadowed=sun->bytes;}
    // The optional members a set holds (its point and spot sizes, the light grid, the SSS three, the mask, the shadowed sun, the env
    // regions' three and box).
    const auto members=[](const NativeLightingPrograms& set){size_t n{};
        for(const auto& point:set.point)n+=!point.empty();for(const auto& spot:set.spot)n+=!spot.empty();
        return n+!set.organize.empty()+!set.sss_vertex.empty()+!set.mask_composite.empty()+!set.sun_shadowed.empty()+!set.env_vertex.empty();};
    const size_t every_member=programs->point.size()+programs->spot.size()+5;   // the sizes, then the grid, the SSS three, the mask, the shadowed sun, the env regions (D-0301)
    const auto held=members(*programs);
    const std::lock_guard hold(lock);
    if(resolved)return resolved;   // another thread published meanwhile
    const auto now=GetTickCount64();   // under the lock (the final review's L1): never earlier than a wait_from another thread wrote
    if(held!=every_member){
        if(!wait_from)wait_from=now;
        if(now-wait_from<wait_ms){if(InterlockedIncrement64(&reported)%600==1){char row[160]{};
            sprintf_s(row,"{\"event\":\"native_lighting_programmes_waiting\",\"members\":%zu,\"of\":%zu}\n",held,every_member);Log(row);}
            return {};}}
    char row[640]{};sprintf_s(row,"{\"event\":\"native_lighting_programmes_resolved\",\"bytes\":[%zu,%zu,%zu,%zu,%zu],\"point_programmes\":%zu,\"spot_programmes\":%zu,\"organize_bytes\":%zu,\"sss_bytes\":[%zu,%zu,%zu],\"env_bytes\":[%zu,%zu,%zu,%zu],\"sun_shadowed\":%d,\"mask\":%d,\"whole\":%s,\"waited_ms\":%llu}\n",
        programs->vertex.size(),programs->ambient.size(),programs->sun.size(),programs->albedo.size(),programs->composite.size(),point_count,spot_count,programs->organize.size(),
        programs->sss_vertex.size(),programs->sss_blur_x.size(),programs->sss_blur_y.size(),programs->env_vertex.size(),programs->env_pixel.size(),programs->env_pixel_pcc.size(),programs->env_box.size(),int(!programs->sun_shadowed.empty()),int(!programs->mask_composite.empty()),held==every_member?"true":"false",
        static_cast<unsigned long long>(wait_from?now-wait_from:0));Log(row);
    resolved=std::move(programs);return resolved;
}
// W6 GL-4 (GLOW-WAVE1-READY 5.2): the glow chain's four programmes from the engine's loaded repositories (the vertex shader of
// the post member +0xC18, ScreenSpaceQuadShader, and the three pixel shaders), all four or none, each checked against its pin
// (glow_pins). A missing one is retried at the next frame (the gxposteffect bundle may not be resident yet, row GL8); a pin
// mismatch disables the glow for the session. Neither refuses the game.
std::shared_ptr<const NativeGlowPrograms> AcquireNativeGlowPrograms() {
    static std::mutex lock;static std::shared_ptr<const NativeGlowPrograms> resolved;static bool refused{};
    {const std::lock_guard hold(lock);if(resolved || refused)return resolved;}
    constexpr std::array<std::pair<const wchar_t*,UINT>,4> names{{{L"ScreenSpaceQuadShader",1u},{L"EmissiveToGlowPS",0u},{L"Sample13Weight",0u},{L"GlowCompositeSinglePS",0u}}};
    std::array<std::shared_ptr<const NativeShader>,4> found;std::string missing;
    for(size_t i=0;i<found.size();++i){found[i]=FindLoadedProgramme(names[i].second?0x4792D58:0x4792BF8,names[i].first,names[i].second);
        if(!found[i]){missing+=missing.empty()?"":",";for(const wchar_t* c=names[i].first;*c;++c)missing.push_back(static_cast<char>(*c));}}
    if(!missing.empty()){static LONG64 reported{};if(InterlockedIncrement64(&reported)%600==1){
        const std::string row="{\"event\":\"native_glow_programmes_unresolved\",\"missing\":\""+missing+"\"}\n";Log(row.c_str());}return {};}
    auto programs=std::make_shared<NativeGlowPrograms>();
    programs->blobs={found[0]->bytes,found[1]->bytes,found[2]->bytes,found[3]->bytes};
    const std::array<const std::vector<uint8_t>*,4> blobs{&programs->blobs.vertex,&programs->blobs.extract,&programs->blobs.blur,&programs->blobs.composite};
    std::string mismatched;
    for(size_t i=0;i<blobs.size();++i){char hash[65]{};
        if(blobs[i]->size()!=glow_pins[i].bytes || !ds3content::Sha256(blobs[i]->data(),blobs[i]->size(),hash) || std::strcmp(hash,glow_pins[i].sha256)){
            mismatched+=mismatched.empty()?"":",";for(const wchar_t* c=names[i].first;*c;++c)mismatched.push_back(static_cast<char>(*c));}}
    const std::lock_guard hold(lock);
    if(!mismatched.empty()){refused=true;const std::string row="{\"event\":\"native_glow_programmes_refused\",\"pin_mismatch\":\""+mismatched+"\"}\n";Log(row.c_str());return {};}
    if(!resolved){resolved=std::move(programs);char row[192]{};sprintf_s(row,"{\"event\":\"native_glow_programmes_resolved\",\"bytes\":[%zu,%zu,%zu,%zu],\"pinned\":true}\n",
        resolved->blobs.vertex.size(),resolved->blobs.extract.size(),resolved->blobs.blur.size(),resolved->blobs.composite.size());Log(row);}
    return resolved;
}
// The executable's data the tone-map constants read (yebis_tonemap.h), from the image once.
struct NativeTonemapData {ds3native::yebis::TonemapData data;std::array<uint8_t,64> dither{};ds3native::yebis::VignetteData vignette;bool ok{};};
const NativeTonemapData& TonemapImageData() {
    static const NativeTonemapData d=[]{NativeTonemapData n;
        std::array<float,16> rows{};float scale{},bias{},to_byte{};
        n.ok=image && Read(image+0x2B60C40,n.data.curve.data(),84) && Read(image+0x2B60CD0,&n.data.tx_scale,4) && Read(image+0x3D7EDA4,&n.data.third,4) &&
            Read(image+0x3D7EE10,&n.data.one,4) && Read(image+0x3D7EF70,&n.data.steps,4) && Read(image+0x2B60D60,rows.data(),16) && Read(image+0x2B60D40,rows.data()+4,16) &&
            Read(image+0x2B60D50,rows.data()+8,16) && Read(image+0x2B60D30,rows.data()+12,16) && Read(image+0x3D7ED5C,&scale,4) && Read(image+0x2A5F3CC,&bias,4) &&
            Read(image+0x2B60CE4,&to_byte,4);
        float eighth_pi{};
        uint32_t eighth_pi_bits{};   // TM-3: [142A66FD0] must be pi/8 (3EC90FDB); its tangent as FUN_141FD59E0
        if(n.ok && Read(image+0x2A66FD0,&eighth_pi,4) && (std::memcpy(&eighth_pi_bits,&eighth_pi,4),eighth_pi_bits==0x3EC90FDBu)){   // gives it, 3ED413CD (the captured m31 vertices hold it bit for bit), as constant bits, not the CPU's tan
            const uint32_t tan_bits=0x3ED413CDu;std::memcpy(&n.vignette.tan_eighth_pi,&tan_bits,4);
            const uint32_t k_bits=0x3FAAAAA8u,m_bits=0x3F2AAAE3u;std::memcpy(&n.vignette.k,&k_bits,4);std::memcpy(&n.vignette.m,&m_bits,4);}   // CPostEffect::Initialize's +0x32C, +0x388 (flag bits 29, 30)
        if(n.ok)n.dither=ds3native::yebis::TonemapDither(rows,scale,bias,to_byte);
        if(n.ok && !ds3native::yebis::LoadColorData(n.data.color,[](uint32_t rva,void* out,size_t bytes){return Read(image+rva,out,bytes);}))   // GL-4, I-0113 tranche 20: the matrix and the guards
            Log("{\"event\":\"native_yebis_color_data\",\"ok\":false}\n");   // the review's L4: the saturation-only fallback then, said once
        return n;}();
    return d;
}
// W10 GL-3: the executable's words of yebis::GlareData, read once through the image (the glare aspect 1.5 is CPostEffect's
// init-flag value).
struct NativeGlareData {ds3native::yebis::GlareData data;bool ok{};};
const NativeGlareData& GlareImageData() {
    static const NativeGlareData d=[]{NativeGlareData n;auto& g=n.data;
        n.ok=image && Read(image+0x2899374,&g.luminance_scale,4) && Read(image+0x3D7EE90,&g.radius_c,4) && Read(image+0x3D7EDE8,&g.level_radius,4) &&
            Read(image+0x2B63170,&g.wavelength[0],4) && Read(image+0x2B6316C,&g.wavelength[1],4) && Read(image+0x2B63168,&g.wavelength[2],4) &&
            Read(image+0x3D7EEFC,&g.two_pi,4) && Read(image+0x27D5950,&g.sigma_floor,4) && Read(image+0x2B4CF88,&g.sigma_replacement,4) &&
            Read(image+0x3D7EE00,&g.base_ceiling,4) && Read(image+0x2B55250,&g.height_rounding,4) && Read(image+0x2B60CA0,&g.up_offset,4);
        for(uint32_t q=0;q<8 && n.ok;++q)n.ok=Read(image+0x469B4D0+8*q,&g.quality_scale[q],4);
        // I-0138 tranche 8f: GlareShape 3's entry of the shape table and its preset's row (glare_capture.h's reading, the same words).
        n.ok=n.ok && Read(image+0x469BD70+3*0x38+8,g.shape3.data(),4*g.shape3.size()) && g.shape3[5]<7 &&
            Read(image+0x469BC90+g.shape3[5]*0x20+8,g.preset3.data(),4*g.preset3.size());
        g.glare_aspect=1.5f;return n;}();
    return d;
}
// W10 DF-4: the executable's words of the depth of field (yebis::DofData, DofSetupData, DofConstants), read once through the image.
struct NativeDofData {ds3native::yebis::DofData draw;ds3native::yebis::DofSetupData setup;ds3native::yebis::DofConstants k;bool ok{};};
const NativeDofData& DofImageData() {
    static const NativeDofData d=[]{NativeDofData n;auto& k=n.k;
        const auto read=[](uint32_t rva,void* out,size_t bytes){return Read(image+rva,out,bytes);};
        n.ok=image && ds3native::yebis::LoadDofDrawData(n.draw,read) &&
            ds3native::yebis::LoadDofSetupData(n.setup,read) && read(0x3D7EE10,&k.one,4) && read(0x3D7EDB4,&k.half,4) && read(0x3D7EE7C,&k.two,4) &&
            read(0x3D7EEC0,&k.pi,4) && read(0x3D7EFE8,&k.degrees,4) && read(0x3D7ED10,&k.milli,4) && read(0x3D7F088,&k.full_hd,4) && read(0x3D7EF48,&k.quality_steps,4);
        return n;}();
    return d;
}
// W10 LS-3: the film grain (the mask's t1): the image's block at RVA 0x2B68A40 checked against its SHA-256, with its CPU mips
// (yebis::FilmGrainLevels), built once; null when the bytes differ.
std::shared_ptr<const std::vector<std::vector<uint8_t>>> NativeFilmGrain() {
    static const auto levels=[]()->std::shared_ptr<const std::vector<std::vector<uint8_t>>>{
        std::vector<uint8_t> bytes(size_t(ds3native::yebis::film_grain_size)*ds3native::yebis::film_grain_size*4);char hash[65]{};
        if(!image || !Read(image+ds3native::yebis::film_grain_rva,bytes.data(),bytes.size()) || !ds3content::Sha256(bytes.data(),bytes.size(),hash) ||
           std::strcmp(hash,ds3native::yebis::film_grain_sha256))return nullptr;
        auto all=ds3native::yebis::FilmGrainLevels(bytes);if(all.size()!=9)return nullptr;
        return std::make_shared<const std::vector<std::vector<uint8_t>>>(std::move(all));}();
    return levels;
}
// I-0138 tranche 8d: the depth-of-field lens's inputs from E69A0's camera record and the globals' DoF words (the extent the output's,
// I-0111), shared by the depth of field and Alpha_R's depth spaces.
ds3native::yebis::DofLensInputs NativeDofLensInputs(const ds3native::SceneViewInput& c,const std::array<uint8_t,0x370>& block) {
    const auto word=[&](size_t at){float f;std::memcpy(&f,block.data()+at,4);return f;};const auto integer=[&](size_t at){int32_t i;std::memcpy(&i,block.data()+at,4);return i;};
    const auto post=NativePostExtent(c);ds3native::yebis::DofLensInputs in;
    in.fov=c.vertical_fov;in.near_plane=c.near_plane;in.far_plane=c.far_plane;in.aspect=c.explicit_aspect;in.extent_width=post[0];in.extent_height=post[1];
    in.quality=integer(0x4C);in.focus=word(0x1B0);in.f_number=word(0x1B4);in.p6d0=word(0x1B8);in.angle=word(0x1BC);in.diagonal=word(0x1C0);in.enable=integer(0x1AC)!=0;
    return in;
}
// Alpha_R's inputs (the scene depth's mask into the lit HDR's alpha), shared by the shafts, the motion blur's own Alpha_R
// (AppendNativeAlphaR) and the blur's depth decode. I-0138 tranche 8d: the depth spaces follow the library's S+0xC33 as FUN_1416E9050
// does (READ, GhidraMCP 2026-11-21): in the depth-of-field branch (the lens's `branch`) the focus and the DoF holder's words ((1, 0, 1, 0)
// INFERRED from Reference1660-1669), outside it NearDepthSpaces (c48 the near plane, the blur's and a shaft's case); near and far the
// library's S+0x564/+0x568 (FUN_1416C9450's, the lens's). Before, the native read the focus spaces whenever the Dof record's Enable was
// set and refused Alpha_R otherwise. False without the executable's DoF words.
bool NativeAlphaRInputs(const ds3native::SceneViewInput& c,const std::array<uint8_t,0x370>& block,ds3native::yebis::AlphaRInputs& a,bool* branch=nullptr) {
    const auto& data=DofImageData();if(!data.ok)return false;
    const auto in=NativeDofLensInputs(c,block);const auto lens=ds3native::yebis::DofLensState(data.k,in);
    a=ds3native::yebis::AlphaRInputs{};
    a.spaces=lens.branch?ds3native::yebis::DofDepthSpaces(lens.near_plane,lens.far_plane,in.focus,{1.0f,0.0f,1.0f,0.0f}):ds3native::yebis::NearDepthSpaces(lens.near_plane,lens.far_plane);
    a.source_object=true;a.source_scale=1.0f;a.source_bias=-0.0f;a.colour=1.0f;if(branch)*branch=lens.branch;return true;
}
// W10 LS-1..LS-3 (W10-NATIVE-PASSES 8): Alpha_R and one light shaft computed natively and appended to the glare plan: E69A0's
// shaft section (DriverSlot) on the camera record rebuilt from the scene camera ([engine+0x210]'s layout: pose, fov, near, far,
// size, aspect, flags, tiles 1, tile 0, offset 0 over the size), the globals block and E4700's colours; the record's depth and
// Alpha_R in NativeAlphaRInputs' depth spaces (the depth-of-field branch's focus 144962120, or the near plane outside it, tranche 8d);
// the shaft plan on the glare's base extent (factor 64 and the shape record's bloom +8, as the glare plan: 1 for shapes 1 and 3). Returns the refusal (empty when
// served): « post colours » without E4700's colours, « disabled » (the slot's Enable), « glare luminance » (a guard), « depth of
// field off » (no witness without it), « camera lens », « executable data » (without the executable's DoF words, NativeAlphaRInputs); with a slot colour of 0
// (the library keeps no record: Reference1670, m45) the shafts are drawn with a zero last weight: the same image, a stable layout.
// I-0138 tranche 8i: `presentation` the main view's presentation serial (PrepareNativeModelFrame's), so the record's depth can tell the
// previous frame's conversion words from older ones.
std::string AppendNativeShafts(ds3native::yebis::GlarePlan& plan,const std::array<uint8_t,0x370>& block,const NativeSelectedScene& scene,
    std::shared_ptr<const std::vector<std::vector<uint8_t>>>& film_grain,float shape_bloom,uint64_t presentation) {
    namespace y=ds3native::yebis;
    const auto word=[&](size_t at){float f;std::memcpy(&f,block.data()+at,4);return f;};
    if(!scene.yebis || !scene.yebis->shaft_post_ready)return "post colours";
    // I-0141 (45000030's cyan veil, i141-t8a: without the shafts the original's level): the library keeps a shaft record only when
    // its slot's Enable byte is set, from the globals' +0x130 (E4700 writes the blended LightShaft Enable there, 0 when the settings'
    // scale is not positive; E69A0 passes it to 1416C9790), and only while the glare luminance +0xDC is positive (1416EA000's test
    // of S+0x6A4, which 1416C9670 sets from it); READ, GhidraMCP 2026-11-15 (a ghidra-research reading). Natively both gates were
    // absent. Not the veil's cause (i141-t9a: Enable 1 through most of 45000030's cut; the cause was the focus, 12's camera request).
    // INFERRED left: 1416C9670 keeps S+0x6A4 at its last value while the globals' +0x244 is not positive; here the frame's +0xDC.
    // I-0138 tranche 8c: the caller now skips the whole glare family at that luminance (the library's gate 1416E758B), so the
    // luminance test (« glare luminance », after the Enable test) is a guard, not reached from it.
    {uint32_t enable{};std::memcpy(&enable,block.data()+0x130,4);if(!enable)return "disabled";}
    if(!(word(0xDC)>0.0f))return "glare luminance";
    if(!scene.yebis->dof_ready || !scene.yebis->dof[0])return "depth of field off";
    // The engine context's record holds tiles 1, tile 0, offset 0 over the size at the three shaft zones (Reference1667-1670), whatever
    // the scene output's own tile count: INFERRED as the record's rule; an orthographic lens is refused.
    const auto& c=scene.camera;if(c.projection_flags || !c.width || !c.height)return "camera lens";
    std::array<uint8_t,0x84> record{};const auto put=[&](size_t at,const void* v){std::memcpy(record.data()+at,v,4);};
    std::memcpy(record.data(),c.camera_pose.data(),64);put(0x50,&c.vertical_fov);put(0x54,&c.near_plane);put(0x58,&c.far_plane);
    put(0x5C,&c.width);put(0x60,&c.height);put(0x64,&c.explicit_aspect);const uint32_t tiles=1;put(0x70,&tiles);put(0x7C,&c.width);put(0x80,&c.height);
    y::ShaftDriverInputs d;std::memcpy(d.pose.data(),c.camera_pose.data(),64);
    if(!ds3light::CameraProjection4B070(record.data(),record.size(),d.projection.data()))return "projection";
    std::memcpy(d.globals.data(),block.data(),block.size());
    std::memcpy(d.colour.data(),scene.yebis->shaft_post.data(),16);std::memcpy(d.colour2.data(),scene.yebis->shaft_post.data()+8,16);
    const auto slot=y::DriverSlot(d);
    // I-0141: the native slot's colour, Enable and depth beside the original's reference_shaft_keeper rows (shaft-observe.flag).
    {static const bool observe=ModuleFlag(L"shaft-observe.flag");
        static std::atomic<int> slot_rows{};
        if(observe && slot_rows.load(std::memory_order_relaxed)<400){
        char row[300]{};uint32_t on{};std::memcpy(&on,&slot[20],4);
        sprintf_s(row,"{\"event\":\"native_shaft_slot\",\"colour\":[%.2g,%.2g,%.2g,%.2g],\"enable\":%u,\"luminance\":%.3g,\"depth\":%.3g}\n",slot[0],slot[1],slot[2],slot[3],on&0xFFu,word(0xDC),slot[19]);
        static std::mutex lock;static std::string last;bool fresh{};
        {const std::lock_guard hold(lock);fresh=last!=row;if(fresh){last=row;slot_rows.fetch_add(1,std::memory_order_relaxed);}}
        if(fresh)Log(row);}}
    ds3native::yebis::AlphaRInputs a;if(!NativeAlphaRInputs(c,block,a))return "executable data";const auto& spaces=a.spaces;
    const auto alpha_draw=y::AlphaRDraw(y::AlphaR(a));   // pushed once the shafts are appended (tranche 8g's step review L2: a refusal leaves plan.pre empty)
    // A zero colour (the sun behind the camera, m45): the library keeps no record and draws no shaft (Reference1670); natively the
    // shaft draws stay, their last pass weighted 0, so MadT2 (or shape 3's layers' composite) adds 0 and the layout does not churn as the sun crosses the edge (review).
    // I-0138 tranche 8i (c7e0ac515's step review, M1): the library converts the record's depth (word 19) in FUN_1416EA000, before
    // FUN_1416E9050 stores this frame's conversion words, so with the previous frame's (READ in tranche 8d, yebis_shafts.h); in
    // i138-t8i-oracle-shafts-1250 the captured word excludes this frame's focus and the bright pass's and admits the previous frame's
    // (yebis_shafts_test's Driver). Natively RecordDepthSpaces: this frame's near, far and source with the conversion words of the
    // previous presentation serial's shafts (one caller, the main view's prepare under cache_lock, so the two statics need no lock of
    // their own); a frame whose previous serial did not reach this point takes its own words (a frame refused after it, at the
    // bright pass, the film grain or AppendShafts, still counts as the previous) (INFERRED: whether E9050 runs on the library's
    // skipped frames is not read). Alpha_R keeps the current frame's, as the library's.
    static y::DepthSpaces previous_spaces;static uint64_t previous_serial=~0ull;
    const y::DepthSpaces record_spaces=y::RecordDepthSpaces(spaces,y::PreviousSpacesApply(previous_serial,presentation)?&previous_spaces:nullptr);
    previous_spaces=spaces;previous_serial=presentation;
    y::ShaftInputs in;in.record=y::ShaftRecordFromSlot(slot,record_spaces);in.base_width=plan.base_width;in.factor=64.0f;in.luminance=word(0xDC);
    in.tan_half=y::VignetteFovTerm(TonemapImageData().vignette,c.vertical_fov);in.fov=std::atan(in.tan_half)*2.0f;in.exposure=word(0x98);in.shape_bloom=shape_bloom;
    // I-0141: the depth words and the record beside the original's reference_shaft_depth rows (shaft-observe.flag); since tranche 8i
    // (its step review of 0cbb83ac9, L3) the record's words, the previous frame's conversion as the original's rows read them at EA000.
    {static const bool observe=ModuleFlag(L"shaft-observe.flag");
        static std::atomic<int> depth_rows{};
        if(observe && depth_rows.load(std::memory_order_relaxed)<600){const auto alpha=y::AlphaR(a);
        std::string words;for(size_t k=0;k<in.record.size();++k){char w[24]{};sprintf_s(w,"%s%.4g",k?",":"",in.record[k]);words+=w;}
        char row[900]{};sprintf_s(row,"{\"event\":\"native_shaft_depth\",\"near\":%.4g,\"far\":%.6g,\"space\":%d,\"conv\":[%.5g,%.5g,%.5g,%.5g,%.5g],\"focus\":%.4g,"
            "\"alpha\":[%.5g,%.5g],\"record\":[%s]}\n",record_spaces.near_plane,record_spaces.far_plane,record_spaces.source,record_spaces.c38,record_spaces.c3c,record_spaces.c40,record_spaces.c44,record_spaces.c48,word(0x1B0),
            alpha.scale,alpha.offset,words.c_str());
        const std::string head(row,strstr(row,"\"record\"")?strstr(row,"\"record\""):row);
        static std::mutex lock;static std::string last;static int calls{};bool fresh{};
        {const std::lock_guard hold(lock);fresh=last!=head || ++calls%60==0;if(fresh){last=head;depth_rows.fetch_add(1,std::memory_order_relaxed);}}
        if(fresh)Log(row);}}
    int bright=-1;for(const auto& g:plan.draws)if(g.kind==y::GlareKind::BrightPass){bright=g.target;break;}
    if(bright<0)return "bright pass";
    in.mask_source={plan.surfaces[size_t(bright)].width,plan.surfaces[size_t(bright)].height};
    in.shaft=in.output=in.pingpong[0]=in.pingpong[1]={plan.base_width,plan.base_height};
    film_grain=NativeFilmGrain();if(!film_grain)return "film grain";
    std::string refused;if(!y::AppendShafts(plan,in,y::BuildShaft(in),refused))return refused;
    plan.pre.push_back(alpha_draw);return "";
}
// I-0138 tranche 8c (the final review's M1): Alpha_R alone, for the motion blur, when the shafts did not draw it (their setting off,
// their refusals before their Alpha_R, or a frame whose glare family is skipped, whose shape or bright pass mode is refused, or whose plan the builder refuses: since tranche 8f
// that plan is served empty, 13b). The library draws Alpha_R by its depth-factor stage
// (1416E0040, called at 1416E6218 before the depth of field and the motion blur) whenever a stage that reads the scene alpha is on:
// its flag +0xC7A ORs the motion blur's (+0xC64: the blur on at a mode above 1, or the falloff test), the light shafts' (+0x9A0) and others (1416E42B0, the store at
// 1416E4CE2), never the glare shape; and the motion blur reads its depth from the scene's alpha (1416E2642, channel 3); READ,
// GhidraMCP 2026-11-21, a ghidra-research reading (evidence/I-0138/t8c-readings/alpha-r-gate-reading.md). Returns the refusal.
// I-0138 tranche 8d: no longer refused without the Dof record's Enable (the library's Alpha_R never reads it; its depth spaces then
// NearDepthSpaces, NativeAlphaRInputs).
std::string AppendNativeAlphaR(ds3native::yebis::GlarePlan& plan,const std::array<uint8_t,0x370>& block,const NativeSelectedScene& scene) {
    const auto& c=scene.camera;if(c.projection_flags || !c.width || !c.height)return "camera lens";
    ds3native::yebis::AlphaRInputs a;if(!NativeAlphaRInputs(c,block,a))return "executable data";
    plan.pre.push_back(ds3native::yebis::AlphaRDraw(ds3native::yebis::AlphaR(a)));
    return "";
}
// I-0138 tranches 2 and 4 (ROADMAP workstream 11): the programmes of a post plan (the glare family's, the depth of field's, the
// motion blur's): our own (post_glare_programs.h); a plan naming a technique that is not one of them is refused, the reason naming
// it (a guard: none of the plans the builders make is). Tranche 8: the library's programmes read from the image and the two test
// flags that drew them (post-glare-old.flag, post-glare-verify.flag) are gone; the offline tests keep them as their oracle.
struct NativePostProgrammes {std::shared_ptr<const GlarePrograms> programs;std::string refused;};
NativePostProgrammes ChooseNativePostProgrammes(const ds3native::yebis::GlarePlan& plan) {
    NativePostProgrammes out;out.programs=ds3native::PostGlarePrograms();
    for(const auto* draws:{&plan.draws,&plan.pre})for(const auto& g:*draws)if(!out.programs->contains(g.technique)){
        out.refused="no programme of ours: "+g.technique;out.programs.reset();return out;}
    return out;
}
}  // namespace
