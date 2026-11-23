#include "model_programs.h"
#include <algorithm>

namespace ds3content {
namespace {
wchar_t Lower(wchar_t c){return c>=L'A' && c<=L'Z'?static_cast<wchar_t>(c-L'A'+L'a'):c;}
bool Same(const std::wstring& a,const std::wstring& b){return a.size()==b.size() &&
    std::equal(a.begin(),a.end(),b.begin(),[](wchar_t x,wchar_t y){return Lower(x)==Lower(y);});}
struct Match {size_t index{};uint32_t count{};bool wrong_stage{};};
// Programme keys may omit the extension (SPX packages of MultiBlend families,
// Native1028); the stage is carried by the entry, so compare without it.
std::wstring Bare(const std::wstring& name){
    if(name.size()>4 && name[name.size()-4]==L'.' && Lower(name[name.size()-2])==L'p' && Lower(name.back())==L'o')
        return name.substr(0,name.size()-4);
    return name;
}
Match Find(std::span<const ModelProgramName> entries,const std::wstring& name,ShaderStage stage){
    Match found;const auto wanted=Bare(name);
    for(size_t i=0;i<entries.size();++i){
        // A keyed entry matches exactly; a bare key (no extension) names one
        // programme per stage, VS and PS sharing it, so only its stage decides.
        const bool bare=Bare(entries[i].name).size()==entries[i].name.size();
        if(!(bare?Same(entries[i].name,wanted):Same(entries[i].name,name)))continue;
        if(entries[i].stage!=stage){if(!bare)found.wrong_stage=true;}
        else {found.index=i;++found.count;}}
    return found;
}
ModelProgramStatus Status(const Match& match){
    if(match.wrong_stage)return ModelProgramStatus::Invalid;
    if(match.count>1)return ModelProgramStatus::Ambiguous;
    return match.count?ModelProgramStatus::Resolved:ModelProgramStatus::Missing;
}
}
ModelProgramSelection SelectModelPrograms(const std::wstring& path,const ModelProgramIntent& intent,
    std::span<const ModelProgramName> programs) {
    ModelProgramSelection result;
    if(path.empty() || path.size()>2048 || path.find(L'\0')!=std::wstring::npos ||
       programs.empty() || programs.size()>20000)return result;
    // Pass table 473CE60 (Reference1114): 0 Fwd, 1 Velo, 6 Gbuf, 7 Gbuf2; Fwd and Velo have no _[AA].
    // D-0180 C2: 2 Dpt (pixel variant 0) and its alpha-tested programmes DptA (variant 1; the pass table's 3), no _[AA].
    // WATER-TESS-WAVE1-READY WT-1: 12 WatDpt (the water depth/tint pass of row 0x24), one pixel variant, no _[A].
    if((intent.shader_pass!=0 && intent.shader_pass!=1 && intent.shader_pass!=2 && intent.shader_pass!=6 && intent.shader_pass!=7 && intent.shader_pass!=12) ||
       ((intent.shader_pass==0 || intent.shader_pass==1 || intent.shader_pass==2) && intent.pixel_variant==2) || (intent.shader_pass==12 && intent.pixel_variant) || intent.layout_type>1 ||
       (intent.deformation && (intent.deformation!=3 || (intent.shader_pass!=0 && intent.shader_pass!=6) || intent.phantom)) ||
       intent.pixel_variant>2){result.status=ModelProgramStatus::Unsupported;return result;}
    // W7 TS-2 (WATER-TESS-WAVE1-READY 1.7): deformation 3 (PN, no displacement) selects `<family>_<pass>[_Skin][_Ins]_PN` for the
    // vertex stage and `<family>_<pass>_PN` for the hull and domain stages (the pass name compared without case: the cache
    // spells GBuf_PN against Gbuf), the ordinary pixel programme; FT (1, 2) and PN_D (4) stay unsupported (TS-6).
    const bool pn=intent.deformation==3;
    const auto separator=path.find_last_of(L"/\\");
    const auto basename=path.substr(separator==std::wstring::npos?0:separator+1);
    // An authored path may name the package without its .spx extension.
    const bool package=basename.size()>4 && Same(basename.substr(basename.size()-4),L".spx");
    if(basename.empty() || (basename.find(L'.')!=std::wstring::npos && !package))return result;
    const auto family=package?basename.substr(0,basename.size()-4):basename;
    const bool depth=intent.shader_pass==2;
    const std::wstring pass=intent.shader_pass==0?L"Fwd":intent.shader_pass==1?L"Velo":depth?(intent.pixel_variant?L"DptA":L"Dpt"):intent.shader_pass==12?L"WatDpt":intent.shader_pass==6?L"Gbuf":L"Gbuf2";
    const auto base=family+L"_"+pass;
    const auto vertex=base+(intent.layout_type?L"_Skin":L"")+(intent.instanced?L"_Ins":L"")+(pn?L"_PN":L"");
    // C5: the alternate set swaps only the Fwd pixel programme for <family>_Phantom_Fwd[_[A]] (A9EF0).
    if(intent.phantom && intent.shader_pass!=0){result.status=ModelProgramStatus::Unsupported;return result;}
    const auto pixel=(intent.phantom?family+L"_Phantom_"+pass:base)+(depth || intent.shader_pass==12?L"":intent.pixel_variant==1?L"_[A]":intent.pixel_variant==2?L"_[AA]":L"")+L".ppo";
    bool channel=intent.prefer_channel;
    auto selected_vertex=Find(programs,vertex+(channel?L"@[cl]":L"")+L".vpo",ShaderStage::Vertex);
    // AA280 falls back to the plain channel only when the preferred VS is
    // absent. Missing PS, malformed stages or collisions never trigger it.
    if(channel && Status(selected_vertex)==ModelProgramStatus::Missing){
        channel=false;selected_vertex=Find(programs,vertex+L".vpo",ShaderStage::Vertex);}
    result.status=Status(selected_vertex);if(result.status!=ModelProgramStatus::Resolved)return result;
    // AA630 (param 1 = Velo): the plain pixel slot is the shared GXFlver_Common_Velo, resolved by the caller from the
    // loose repository; the alpha-tested slot is the family's own <family>_Velo_[A].
    if(intent.shader_pass==1 && intent.pixel_variant==0){
        result.vertex=selected_vertex.index;result.channel=channel;result.shared_pixel=true;return result;}
    const auto selected_pixel=Find(programs,pixel,ShaderStage::Pixel);
    result.status=Status(selected_pixel);if(result.status!=ModelProgramStatus::Resolved)return result;
    if(pn){const auto hull=Find(programs,base+L"_PN.hpo",ShaderStage::Hull),domain=Find(programs,base+L"_PN.dpo",ShaderStage::Domain);
        result.status=Status(hull);if(result.status!=ModelProgramStatus::Resolved)return result;
        result.status=Status(domain);if(result.status!=ModelProgramStatus::Resolved)return result;
        result.hull=hull.index;result.domain=domain.index;result.tessellated=true;}
    result.vertex=selected_vertex.index;result.pixel=selected_pixel.index;result.channel=channel;return result;
}
}
