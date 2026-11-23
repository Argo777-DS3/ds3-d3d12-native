#include "model_parameters.h"
#include "shader_interface.h"
#include "../world/producers.h"
#include <set>
#include <stdexcept>
#include <bit>
#include <cmath>
#include <limits>
#include <algorithm>

namespace ds3content {
std::vector<ModelParameterWrite> ProduceOrdinaryInactiveModelParameters(const ShaderInterface& shader,
    std::optional<bool> damage_enabled,std::optional<bool> wet_enabled) {
    std::vector<ModelParameterWrite> writes;
    if(shader.stage!=ShaderStage::Pixel)return writes;
    const auto resource=std::find_if(shader.resources.begin(),shader.resources.end(),[](const auto& row){
        return row.name=="cbMatDynParam" && row.input_type==0 && row.shader_register==2 && row.count==1;});
    if(resource==shader.resources.end())return writes;
    const auto buffer=std::find_if(shader.constants.begin(),shader.constants.end(),[](const auto& row){return row.name=="cbMatDynParam";});
    if(buffer==shader.constants.end())return writes;
    const auto matches=[&](const char* name,uint32_t offset){
        if(offset>buffer->declared_bytes || 16>buffer->declared_bytes-offset)return false;
        unsigned overlapping{};bool exact{};
        for(const auto& row:buffer->variables)if(uint64_t(row.byte_offset)<uint64_t(offset)+16 &&
            uint64_t(row.byte_offset)+row.bytes>offset){
            ++overlapping;exact=row.name==name && row.byte_offset==offset && row.bytes==16 &&
                row.value_class==1 && row.scalar_type==3 && row.rows==1 && row.columns==4 && !row.elements && !row.members;
        }
        return overlapping==1 && exact;
    };
    if(damage_enabled && !*damage_enabled && matches("FC_DamagedNormalParam",0x200) &&
       matches("FC_DamagedNormalUvTiling",0x210) && matches("FC_DamagedNormal2UvTiling",0x220)){
        writes.push_back({0x200,4,{}});writes.push_back({0x210,16,{}});writes.push_back({0x220,16,{}});
    }
    if(wet_enabled && !*wet_enabled && matches("FC_WetDiffuseColor",0x230) &&
       matches("FC_WetParam1",0x240) && matches("FC_WetRefrectanceColor",0x250) && matches("FC_WetParam2",0x260))
        for(uint32_t offset=0x230;offset<0x270;offset+=16)writes.push_back({offset,16,{}});
    return writes;
}
namespace {
uint64_t Number(std::span<const uint8_t> bytes,uint64_t at,size_t count) {
    if(at>bytes.size() || count>bytes.size()-static_cast<size_t>(at))
        throw std::runtime_error("SPX parameter field extent");
    uint64_t value{};for(size_t i=0;i<count;++i)value|=uint64_t(bytes[static_cast<size_t>(at)+i])<<(8*i);
    return value;
}
std::wstring Name(std::span<const uint8_t> bytes,uint64_t at) {
    if(at&1)throw std::runtime_error("SPX parameter name alignment");
    std::wstring result;
    for(size_t i=0;i<=2048;++i){
        const auto ch=static_cast<wchar_t>(Number(bytes,at,2));
        if(!ch){if(result.empty())throw std::runtime_error("SPX parameter empty name");return result;}
        if(i==2048)throw std::runtime_error("SPX parameter name bound");
        result.push_back(ch);at+=2;
    }
    throw std::runtime_error("SPX parameter name extent");
}
// 1128: type11 colour groups (descriptor component 0..3 = R,G,B,A, 4 = intensity). 22C4B0 packs the four raw components into
// one RGBA word, byte = (int64)(value*255) (R bits 16-23, G 8-15, B 0-7, A 24-31; the packed word starts at 0xFFFFFFFF and
// persists across groups) and keeps the intensity. 22E930 then emits, at the intensity descriptor, R*I, G*I, B*I and A from
// byte/255, with R,G,B raised to 2.2 first when that descriptor's +39 flag is 1; colour components emit nothing else. Other
// descriptors emit their raw scalar word unchanged. (22E5C0 packs its animated colour the same way into +0x398.)
bool EmitGeneratedWords(const std::vector<uint32_t>& raw,const std::vector<ModelGeneratedParameters::Descriptor>& descriptors,std::vector<uint32_t>& words) {
    if(raw.size()!=descriptors.size())return false;
    std::vector<uint32_t> emitted;emitted.reserve(raw.size());uint32_t packed=0xFFFFFFFFu;
    for(size_t i=0;i<raw.size();++i){const auto& d=descriptors[i];
        if(d.type!=11){emitted.push_back(raw[i]);continue;}
        const float value=std::bit_cast<float>(raw[i]);if(!std::isfinite(value))return false;
        const auto quantized=static_cast<uint32_t>(static_cast<int64_t>(value*255.0f));
        if(d.component==0)packed=(packed&~0x00FF0000u)|((quantized<<16)&0x00FF0000u);
        else if(d.component==1)packed=(packed&~0x0000FF00u)|((quantized<<8)&0x0000FF00u);
        else if(d.component==2)packed=(packed&~0x000000FFu)|(quantized&0xFFu);
        else if(d.component==3)packed=(quantized<<24)|(packed&0x00FFFFFFu);
        else{float r=static_cast<float>((packed>>16)&0xFF)/255.0f,g=static_cast<float>((packed>>8)&0xFF)/255.0f,b=static_cast<float>(packed&0xFF)/255.0f;
            const float a=static_cast<float>(packed>>24)/255.0f;
            if(d.gamma==1){r=std::pow(r,2.2f);g=std::pow(g,2.2f);b=std::pow(b,2.2f);}
            for(const float out:{r*value,g*value,b*value,a})emitted.push_back(std::bit_cast<uint32_t>(out));}
    }
    words=std::move(emitted);return true;
}
}
bool AnimateModelGeneratedParameters(const ModelGeneratedParameters& parameters,const ModelParameterAnimations& animations,
    const std::wstring& material_name,std::vector<uint32_t>& words) {
    if(!animations.available || parameters.raw.size()!=parameters.descriptors.size())return false;
    auto raw=parameters.raw;bool animated{};
    for(size_t i=0;i<raw.size();++i){const auto& d=parameters.descriptors[i];
        // 22E5C0: a descriptor's first component (+0x30 == 0) with a nonzero key (+0x3C); types 1..4 move that many values,
        // type 11 four (R, G, B, A), from this descriptor on; other types nothing.
        const size_t extent=d.type>=1 && d.type<=4?d.type:d.type==11?4u:0u;
        if(d.component || !d.animation_key || !extent || i+extent>raw.size())continue;
        ModelParameterAnimation row;
        if(!SelectModelParameterAnimation(animations,material_name,d.animation_key,row) || !row.target || !std::isfinite(row.strength))continue;
        std::array<uint32_t,4> moved{};bool finite=true;
        for(size_t j=0;j<extent;++j){const float v=std::bit_cast<float>(raw[i+j]),result=((*row.target)[j]-v)*row.strength+v;
            finite=finite && std::isfinite(result);moved[j]=std::bit_cast<uint32_t>(result);}
        if(!finite)continue;   // as the damage rate (ProduceModelDamageParameters): a non-finite blend leaves the words
        for(size_t j=0;j<extent;++j)raw[i+j]=moved[j];
        animated=true;
    }
    // The packed colours are emitted again from the raw values in group order; 22E5C0 writes its packed slot +0x398[k] by the
    // ordinal k of the ANIMATED colour groups, 22E930 reads by the ordinal of all colour groups: the two agree unless an
    // unanimated colour group precedes an animated one (whether any family has that order is not checked; noted divergence).
    return animated && EmitGeneratedWords(raw,parameters.descriptors,words);
}
std::vector<ModelParameterDeclaration> ReadModelParameterDeclarations(std::span<const uint8_t> bytes) {
    if(bytes.size()<0x98 || Number(bytes,8,4)<=1)throw std::runtime_error("SPX parameter version envelope");
    // A97F0: declaration header = blob+18. 1B0BC0 relocates its +8
    // parameter table and each 0x50-byte row's name against the blob base.
    const auto table=Number(bytes,0x20,8),count=Number(bytes,0x2C,4);
    if(count>4096 || table>bytes.size() || count>(bytes.size()-static_cast<size_t>(table))/0x50)
        throw std::runtime_error("SPX parameter table extent");
    std::vector<ModelParameterDeclaration> result;std::set<std::wstring> names;
    for(uint64_t i=0;i<count;++i){
        const auto at=table+0x50*i;
        ModelParameterDeclaration row{Name(bytes,Number(bytes,at,8)),
            static_cast<uint32_t>(Number(bytes,at+0x28,4)),static_cast<uint32_t>(Number(bytes,at+0x30,4)),
            static_cast<uint8_t>(Number(bytes,at+0x2D,1)),
            Number(bytes,at+0x2E,1)!=0};
        if(!names.insert(row.name).second)throw std::runtime_error("SPX parameter name collision");
        result.push_back(std::move(row));
    }
    return result;
}
ModelGeneratedStatus ReadModelGeneratedParameters(std::span<const uint8_t> spx,
    std::span<const uint8_t> gx,ModelGeneratedParameters& output) {
    try {
        if(spx.size()<0x98 || Number(spx,8,4)<=1 || gx.size()>1024u*1024u)
            return ModelGeneratedStatus::Invalid;
        const auto table=Number(spx,0x18,8),count=Number(spx,0x28,4);
        const auto scalar_bytes=Number(spx,0x4C,4),reg=Number(spx,0x50,4);
        if(count>4096 || table>spx.size() || count>(spx.size()-static_cast<size_t>(table))/0x60 ||
           (scalar_bytes&3) || scalar_bytes/4!=count || reg>4096 ||
           reg*16+scalar_bytes>65536)return ModelGeneratedStatus::Invalid;
        if(!count || gx.empty())return ModelGeneratedStatus::Missing;
        //22E930 subsequently overlays fixed material fields below270;
        //140850 overlays cloth/dither. Until those current typed writers are
        //connected, a generated range there cannot establish final coverage.
        if(reg<0x27)return ModelGeneratedStatus::Unsupported;
        std::span<const uint8_t> record;size_t at{};bool terminated=false;
        for(size_t records=0;records<65536;++records){
            if(at>gx.size() || gx.size()-at<12)return ModelGeneratedStatus::Invalid;
            const auto key=Number(gx,at,4),length=Number(gx,at+8,4);
            if(key==0x7FFFFFFF){terminated=true;break;}
            if(length<12 || length>gx.size()-at)return ModelGeneratedStatus::Invalid;
            if(key==0x444D5847){
                if(!record.empty())return ModelGeneratedStatus::Unsupported;
                record=gx.subspan(at,static_cast<size_t>(length));
            }
            at+=static_cast<size_t>(length);
        }
        if(!terminated)return ModelGeneratedStatus::Invalid;
        if(record.empty() || Number(record,4,4)==100)return ModelGeneratedStatus::Missing;
        const auto row_count=Number(record,12,4);
        if(row_count>4096)return ModelGeneratedStatus::Invalid;
        struct Row {uint32_t key{},components{};std::array<uint32_t,5> words{};};
        std::vector<Row> rows;rows.reserve(static_cast<size_t>(row_count));at=16;
        for(uint64_t i=0;i<row_count;++i){
            Row row;row.key=static_cast<uint32_t>(Number(record,at,4));
            const auto type=Number(record,at+4,4);at+=8;
            if(type==0 || type==1 || type==5)row.components=1;
            else if(type>=2 && type<=4)row.components=static_cast<uint32_t>(type);
            else if(type>=6 && type<=8)row.components=static_cast<uint32_t>(type-4);
            else if(type==11)row.components=5;
            else return ModelGeneratedStatus::Unsupported;
            for(uint32_t j=0;j<row.components;++j){row.words[j]=static_cast<uint32_t>(Number(record,at,4));at+=4;}
            rows.push_back(row);
        }
        ModelGeneratedParameters candidate;candidate.byte_offset=static_cast<uint32_t>(reg*16);
        // Every descriptor is checked even on the ordered branch. Type0
        // substitutes MTD providers (still unavailable); type11 packed colours
        // are composed below (1128: 22C4B0 packing, 22E930 output).
        for(uint64_t i=0;i<count;++i){const auto type=Number(spx,table+i*0x60+0x38,1);
            if(type==0 || type>11)return ModelGeneratedStatus::Unsupported;}
        const bool ordered=Number(record,4,4)==Number(spx,0x48,4) && row_count==Number(spx,0x54,4);
        if(ordered){
            for(const auto& row:rows)for(uint32_t j=0;j<row.components;++j)candidate.words.push_back(row.words[j]);
            if(candidate.words.size()!=count)return ModelGeneratedStatus::Invalid;
        }else{
            candidate.words.reserve(static_cast<size_t>(count));
            for(uint64_t i=0;i<count;++i){const auto descriptor=table+i*0x60;
                const auto key=Number(spx,descriptor+0x2C,4),component=Number(spx,descriptor+0x30,4);
                if(component>4)return ModelGeneratedStatus::Unsupported;
                auto word=static_cast<uint32_t>(Number(spx,descriptor+0x34,4));
                const auto found=std::find_if(rows.begin(),rows.end(),[&](const auto& row){return row.key==key;});
                if(found!=rows.end()){
                    // The original parser supplies global defaults beyond a
                    // row's declared extent; this route has no such input.
                    if(component>=found->components)return ModelGeneratedStatus::Unsupported;
                    word=found->words[static_cast<size_t>(component)];
                }
                candidate.words.push_back(word);
            }
        }
        candidate.descriptors.reserve(static_cast<size_t>(count));
        for(uint64_t i=0;i<count;++i){const auto descriptor=table+i*0x60;ModelGeneratedParameters::Descriptor d;
            d.type=static_cast<uint8_t>(Number(spx,descriptor+0x38,1));d.gamma=static_cast<uint8_t>(Number(spx,descriptor+0x39,1));
            d.component=static_cast<uint32_t>(Number(spx,descriptor+0x30,4));d.animation_key=static_cast<uint32_t>(Number(spx,descriptor+0x3C,4));
            candidate.descriptors.push_back(d);}
        candidate.raw=candidate.words;
        if(!EmitGeneratedWords(candidate.raw,candidate.descriptors,candidate.words))return ModelGeneratedStatus::Invalid;
        output=std::move(candidate);return ModelGeneratedStatus::Ready;
    }catch(const std::runtime_error&){return ModelGeneratedStatus::Invalid;}
}
bool PrepareModelNumericParameter(const ModelParameterDeclaration& declaration,
    const ModelNumericParameter& value,PreparedModelParameter& output) {
    if(value.type>=12 || value.name.empty() || declaration.producer_kind>1 ||
       (value.type%4==0 && declaration.scalar_offset>3) ||
       declaration.vector_index>std::numeric_limits<uint32_t>::max()/16)return false;
    // The current bounded name family is ASCII; no invented UTF conversion.
    std::wstring name;for(const unsigned char ch:value.name){if(ch>=128)return false;name.push_back(ch);}
    if(name!=declaration.name)return false;
    PreparedModelParameter prepared;prepared.declaration=declaration;prepared.components=value.type%4+1;
    for(uint32_t i=0;i<prepared.components;++i){
        const auto word=value.words[i];
        if(value.type<4){if(word>255)return false;prepared.base_words[i]=word?1u:0u;}
        else if(value.type<8)prepared.base_words[i]=std::bit_cast<uint32_t>(static_cast<float>(std::bit_cast<int32_t>(word)));
        else {if(!std::isfinite(std::bit_cast<float>(word)))return false;prepared.base_words[i]=word;}
    }
    // 1FF130 zeroes components outside the declared numeric extent.
    output=std::move(prepared);return true;
}
ModelGeneratedStatus ReadModelDamageParameters(std::span<const uint8_t> gx,
    std::span<const ModelParameterDeclaration> declarations,
    std::span<const ModelBuiltinParameterDeclaration> builtins,
    std::span<const ModelNumericParameter> values,bool provider_values_available,
    ModelDamageParameters& output) {
    try {
        if(gx.size()>1024u*1024u)return ModelGeneratedStatus::Invalid;
        ModelDamageParameters candidate;
        std::array<std::span<const uint8_t>,3> records;
        bool terminated=gx.empty();size_t at{};
        for(size_t count=0;!terminated && count<65536;++count){
            if(at>gx.size() || gx.size()-at<12)return ModelGeneratedStatus::Invalid;
            const auto key=Number(gx,at,4),length=Number(gx,at+8,4);
            if(key==0x7FFFFFFF){terminated=true;break;}
            if(length<12 || length>gx.size()-at)return ModelGeneratedStatus::Invalid;
            for(size_t index=0;index<3;++index)if(key==0x30385847u+index*0x01000000u){
                if(!records[index].empty())return ModelGeneratedStatus::Unsupported;
                if(length<(index?32u:16u))return ModelGeneratedStatus::Invalid;
                records[index]=gx.subspan(at,static_cast<size_t>(length));
            }
            at+=static_cast<size_t>(length);
        }
        if(!terminated)return ModelGeneratedStatus::Invalid;
        if(records[0].empty()){output=std::move(candidate);return ModelGeneratedStatus::Ready;}
        candidate.enabled=true;candidate.rate=static_cast<uint32_t>(Number(records[0],12,4));
        const bool forced=!records[1].empty() && (Number(records[1],12,4)&1)!=0;
        constexpr std::array<std::wstring_view,2> names{L"g_DamagedNormal1UvTiling",L"g_DamagedNormal2UvTiling"};
        for(size_t index=0;index<2;++index){
            if(records[index+1].empty())continue;
            std::array<uint32_t,4> words;
            for(size_t lane=0;lane<4;++lane)words[lane]=static_cast<uint32_t>(Number(records[index+1],16+4*lane,4));
            if(forced){candidate.uv[index]=words;continue;}
            if(!provider_values_available)continue;
            const auto value=std::find_if(values.begin(),values.end(),[&](const auto& row){
                return std::wstring(row.name.begin(),row.name.end())==names[index];});
            if(value==values.end()){candidate.uv[index]=words;continue;}
            //A same-name SPX factory can replace the kind4 provider. Its
            //specialized accessor has not been established for this route.
            if(std::any_of(declarations.begin(),declarations.end(),[&](const auto& row){return row.name==names[index];}))continue;
            const auto builtin=std::find_if(builtins.begin(),builtins.end(),[&](const auto& row){return row.name==names[index];});
            if(builtin==builtins.end() || builtin->kind!=4)continue;
            //Loaded1FF700 copies converted1FF130 words to+30..3F.
            //1FF660 returns that base vector directly;1FF910's later b3
            //animation/RGB writes do not mutate this provider base.
            PreparedModelParameter base;
            if(PrepareModelNumericParameter({std::wstring(names[index]),0,0,0,false},*value,base))
                candidate.uv[index]=base.base_words;
        }
        output=std::move(candidate);return ModelGeneratedStatus::Ready;
    }catch(const std::runtime_error&){return ModelGeneratedStatus::Invalid;}
}
std::vector<ModelParameterWrite> ProduceModelDamageParameters(const ModelDamageParameters& parameters,
    const ModelParameterAnimations& animations,const std::wstring& material_name,
    std::optional<int32_t> material_index) {
    std::vector<ModelParameterWrite> writes;if(!parameters.enabled)return writes;
    if(animations.available && (animations.rows.empty() || material_index)){
        const bool gate=std::any_of(animations.rows.begin(),animations.rows.end(),[&](const auto& row){
            return row.universal || row.material_index==*material_index;});
        ModelParameterAnimation animation{true,std::nullopt,0.f};
        //The outer gate can be opened by a different key/name row; only
        //then does22D620 perform the whole-table damage-rate CRC lookup.
        bool known=true;
        if(gate){
            constexpr std::wstring_view name=L"ExParam_DamagedNormal_DamageRate";uint32_t key=0xFFFFFFFFu;
            for(const wchar_t ch:name)for(unsigned byte=0;byte<2;++byte){
                key^=(static_cast<uint32_t>(ch)>>(8*byte))&255u;
                for(unsigned bit=0;bit<8;++bit)key=(key>>1)^((key&1u)?0xEDB88320u:0u);
            }
            known=SelectModelParameterAnimation(animations,material_name,~key,animation);
        }
        uint32_t rate=parameters.rate;
        if(known && animation.target){
            const float base=std::bit_cast<float>(rate),target=(*animation.target)[0];
            const float result=(target-base)*animation.strength+base;
            known=std::isfinite(base) && std::isfinite(target) && std::isfinite(animation.strength) && std::isfinite(result);
            if(known)rate=std::bit_cast<uint32_t>(result);
        }
        if(known)writes.push_back({0x200,4,{rate,0,0,0}});
    }
    for(size_t index=0;index<2;++index)if(parameters.uv[index])
        writes.push_back({static_cast<uint32_t>(0x210+16*index),16,*parameters.uv[index]});
    return writes;
}
bool SelectModelWetActivation(std::wstring_view shader_family,std::span<const uint8_t> spx,
    const std::vector<std::wstring>* current_catalog,bool& output) {
    if(shader_family.empty() || shader_family.size()>2048 || shader_family.find(L'\0')!=std::wstring_view::npos)return false;
    if(shader_family==L"GXFlver_SSSEmiIblGlow_Bloody_Cloth" ||
       (current_catalog && std::find(current_catalog->begin(),current_catalog->end(),shader_family)!=current_catalog->end())){
        output=true;return true;}
    try {
        if(!spx.empty()){
            //A97F0 ASM9877/987B publishes blob+18 as the header address.
            //The word at blob+18 is its scalar-table offset, not that address.
            if(spx.size()<0x98 || Number(spx,8,4)<=1)return false;
            if(Number(spx,0x58,8)&2){output=true;return true;}
        }
    }catch(const std::runtime_error&){return false;}
    if(!current_catalog)return false;
    output=false;return true;
}
std::vector<ModelParameterWrite> ProduceModelWetParameters(bool enabled,
    const ModelWetMaterialParameters& parameters,const std::optional<std::array<float,3>>& draw_abc,
    const ModelParameterAnimations& animations,const std::wstring& material_name,
    std::optional<int32_t> material_index) {
    if(!enabled || !animations.available || (!animations.rows.empty() && !material_index) ||
       (!parameters.override_abc && !draw_abc))return {};
    ds3prod::MatDynWet input;input.diffuse=parameters.diffuse;input.reflectance=parameters.reflectance;
    const auto& abc=parameters.override_abc?*parameters.override_abc:*draw_abc;
    input.a=abc[0];input.b=abc[1];input.c=abc[2];input.type=parameters.type;input.scale=parameters.scale;
    const bool gate=std::any_of(animations.rows.begin(),animations.rows.end(),[&](const auto& row){
        return row.universal || row.material_index==*material_index;});
    const auto animation=[&](std::wstring_view name,uint32_t category,ModelParameterAnimation& result){
        if(!gate){result={true,std::nullopt,0.f};return true;}
        if(!SelectModelParameterAnimation(animations,material_name,0,result,category))return false;
        if(result.target)return true;
        uint32_t key=0xFFFFFFFFu;
        for(const wchar_t ch:name)for(unsigned byte=0;byte<2;++byte){
            key^=(static_cast<uint32_t>(ch)>>(8*byte))&255u;
            for(unsigned bit=0;bit<8;++bit)key=(key>>1)^((key&1u)?0xEDB88320u:0u);
        }
        return SelectModelParameterAnimation(animations,material_name,~key,result);
    };
    const auto blend=[](float base,float target,float strength){return (target-base)*strength+base;};
    for(size_t index=0;index<2;++index){
        ModelParameterAnimation update;
        if(!animation(index?L"ExParam_WetReflectanceColor":L"ExParam_WetDiffuseColor",static_cast<uint32_t>(15+index),update))return {};
        if(update.target){
            if(!std::isfinite(update.strength))return {};
            auto& color=index?input.reflectance:input.diffuse;
            for(size_t lane=0;lane<4;++lane)color[lane]=blend(color[lane],(*update.target)[lane],update.strength);
        }
    }
    if(parameters.override_abc){
        ModelParameterAnimation update;if(!animation(L"ExParam_WetParam",17,update))return {};
        if(update.target){if(!std::isfinite(update.strength))return {};
            input.a=blend(input.a,(*update.target)[0],update.strength);
            input.c=blend(input.c,(*update.target)[2],update.strength);}
    }
    for(const auto& color:{input.diffuse,input.reflectance})for(const auto value:color)if(!std::isfinite(value))return {};
    for(const auto value:{input.a,input.b,input.c,input.scale})if(!std::isfinite(value))return {};
    std::array<uint8_t,0x270> bytes{};std::array<uint8_t,0x270/4> written{};
    ds3prod::ProduceMatDynWet(input,bytes.data(),static_cast<uint32_t>(bytes.size()),written.data());
    std::vector<ModelParameterWrite> writes;
    for(uint32_t at=0x230;at<0x270;at+=16){
        ModelParameterWrite write{at,16,{}};std::memcpy(write.words.data(),bytes.data()+at,16);
        for(size_t lane=0;lane<4;++lane)if((at!=0x260 || lane!=0) && !std::isfinite(std::bit_cast<float>(write.words[lane])))return {};
        writes.push_back(write);
    }
    return writes;
}
std::vector<ModelParameterWrite> ProduceModelPhantomParameters(const ModelPhantomParameters& parameters,
    const std::array<uint32_t,7>& fallback_keys,const ModelParameterAnimations& animations,const std::wstring& material_name,
    std::optional<int32_t> material_index) {
    if(!parameters.enabled)return {};
    auto subtract=parameters.subtract;auto colours=parameters.colours;
    // 22D620: the overrides apply only when a row is universal or names this material index (an unavailable snapshot or a
    // nonempty table without the index produces nothing: the words stay unknown, not defaulted).
    if(!animations.available || (!animations.rows.empty() && !material_index))return {};
    const bool gate=std::any_of(animations.rows.begin(),animations.rows.end(),[&](const auto& row){
        return row.universal || row.material_index==*material_index;});
    if(gate){
        const auto find=[&](uint32_t category,ModelParameterAnimation& result){
            if(!SelectModelParameterAnimation(animations,material_name,0,result,category))return false;
            if(!result.target && !SelectModelParameterAnimation(animations,material_name,fallback_keys[category-8],result))return false;
            return true;};
        for(uint32_t index=0;index<2;++index){ModelParameterAnimation update;if(!find(8+index,update))return {};
            if(update.target){if(!std::isfinite(update.strength))return {};
                // byte = (value != 0) * w + (1 - w) * byte, truncated toward zero
                const float value=(std::bit_cast<uint32_t>((*update.target)[0])!=0u?1.f:0.f)*update.strength+(1.f-update.strength)*subtract[index];
                if(!std::isfinite(value))return {};subtract[index]=static_cast<uint8_t>(static_cast<int64_t>(value));}}
        for(uint32_t index=0;index<5;++index){ModelParameterAnimation update;if(!find(10+index,update))return {};
            if(update.target){if(!std::isfinite(update.strength))return {};
                for(size_t lane=0;lane<4;++lane)colours[index][lane]+=((*update.target)[lane]-colours[index][lane])*update.strength;}}
    }
    std::vector<ModelParameterWrite> writes;
    for(uint32_t index=0;index<5;++index){
        const auto& c=colours[index];const float sign=index<2 && subtract[index]?-1.f:1.f;
        const std::array<float,4> row{c[0]*c[0]*c[3]*sign,c[1]*c[1]*c[3]*sign,c[2]*c[2]*c[3]*sign,c[3]};
        if(!std::all_of(row.begin(),row.end(),[](float v){return std::isfinite(v);}))return {};
        ModelParameterWrite write{0x1A0+16*index,16,{}};std::memcpy(write.words.data(),row.data(),16);writes.push_back(write);
    }
    return writes;
}
bool PrepareModelBuiltinParameter(const ModelBuiltinParameterDeclaration& declaration,
    const ModelNumericParameter& value,PreparedModelParameter& output) {
    if(declaration.kind!=0 && declaration.kind!=1 && declaration.kind!=3)return false;
    // Loaded A7E40 hashes UTF16 little-endian bytes, starting FFFFFFFF and
    // complementing the result. The pinned256-word table was checked755 to
    // match IEEE CRC32 polynomial EDB88320; no original factory/global init.
    uint32_t key=0xFFFFFFFFu;
    for(const wchar_t ch:declaration.name)for(unsigned byte=0;byte<2;++byte){
        key^=(static_cast<uint32_t>(ch)>>(8*byte))&255u;
        for(unsigned bit=0;bit<8;++bit)key=(key>>1)^((key&1u)?0xEDB88320u:0u);
    }
    ModelParameterDeclaration binding{declaration.name,declaration.vector_index,~key,
        static_cast<uint8_t>(declaration.kind==3?1:0),declaration.transform_rgb,
        declaration.scalar_offset,declaration.animation_selector};
    return PrepareModelNumericParameter(binding,value,output);
}
bool ProduceModelParameter(const PreparedModelParameter& parameter,
    const ModelParameterAnimation& animation,ModelParameterWrite& output) {
    if(!animation.lookup_complete || parameter.components<1 || parameter.components>4 ||
       parameter.declaration.producer_kind>1 ||
       parameter.declaration.vector_index>std::numeric_limits<uint32_t>::max()/16)return false;
    const auto at=uint64_t(parameter.declaration.vector_index)*16+
        (parameter.components==1?uint64_t(parameter.declaration.scalar_offset)*4:0u);
    if(at>UINT32_MAX || (parameter.components==1 && parameter.declaration.scalar_offset>3))return false;
    ModelParameterWrite produced;produced.byte_offset=static_cast<uint32_t>(at);
    produced.bytes=parameter.components==1?4u:16u;produced.words=parameter.base_words;
    if(animation.target){
        if(!std::isfinite(animation.strength))return false;
        // Vector branch writes all four lanes, also for float2/float3.
        const uint32_t lanes=produced.bytes/4;
        for(uint32_t i=0;i<lanes;++i){
            const auto base=std::bit_cast<float>(parameter.base_words[i]),target=(*animation.target)[i];
            if(!std::isfinite(base) || !std::isfinite(target))return false;
            const float result=(target-base)*animation.strength+base;
            if(!std::isfinite(result))return false;
            produced.words[i]=std::bit_cast<uint32_t>(result);
        }
    }else if(parameter.components>1 && parameter.declaration.transform_rgb){
        // Loaded 1FF910's scalar exponent at143D7EE90 =2.2, already used by
        // the existing native colour producers. Alpha is unchanged.
        for(uint32_t i=0;i<3;++i){const auto result=std::pow(std::bit_cast<float>(produced.words[i]),2.2f);
            if(!std::isfinite(result))return false;produced.words[i]=std::bit_cast<uint32_t>(result);}
    }
    output=produced;return true;
}
bool SelectModelParameterAnimation(const ModelParameterAnimations& inputs,
    const std::wstring& material_name,uint32_t lookup_key,ModelParameterAnimation& output,uint32_t selector) {
    if(!inputs.available)return false;
    const ModelParameterAnimationRow* universal=nullptr;
    for(const auto& row:inputs.rows){
        if((selector?row.kind:row.lookup_key)!=(selector?selector:lookup_key))continue;
        if(!row.universal && row.material_name==material_name){
            output={true,row.target,row.strength};return true;}
        if(row.universal && !universal)universal=&row;
    }
    output={true,universal?std::optional(universal->target):std::nullopt,universal?universal->strength:0.f};
    return true;
}
bool PrepareModelMaterialPolicy(std::wstring_view shader_family,
    std::span<const ModelParameterDeclaration> declarations,
    std::span<const ModelBuiltinParameterDeclaration> builtins,
    std::span<const ModelNumericParameter> values,ModelMaterialPolicy& output) {
    if(shader_family.empty() || shader_family.size()>2048 ||
       shader_family.find(L'\0')!=std::wstring_view::npos || values.size()>4096)return false;
    ModelMaterialPolicy policy;
    // A7E40 cases21..40: actual bit assignments, not D430A0 presence bits.
    constexpr std::array<uint8_t,20> bits{12,17,6,18,4,1,5,9,2,3,7,8,13,14,15,10,19,20,21,22};
    for(const auto& value:values){
        if(value.type>=12 || value.name.empty())return false;
        std::wstring name;for(const unsigned char ch:value.name){if(ch>=128)return false;name.push_back(ch);}
        // Source declaration wins, even for unsupported numeric stage kinds.
        if(std::any_of(declarations.begin(),declarations.end(),[&](const auto& row){return row.name==name;}))continue;
        // A7E40's14-unit comparison includes the terminating zero.
        if(name==L"g_BloodAmount")policy.flags|=4;
        const auto found=std::find_if(builtins.begin(),builtins.end(),[&](const auto& row){return row.name==name;});
        if(found==builtins.end())continue;
        const auto raw=value.words[0],kind=found->kind;
        if(kind==6){policy.transparent|=raw!=0;policy.blend_mode=std::max(policy.blend_mode,raw);} // 2007C0/200AC0: any nonzero
        else if(kind==7)policy.alpha_ref=std::max(policy.alpha_ref,raw); // 200D00/200F10: unsigned maximum
        else if(kind==16){policy.material_type=raw;if(raw==3)policy.flags|=0x800;else if(raw==2)policy.flags|=0x2000;}
        else if(kind==19)policy.disable_decal=(raw&255u)!=0;
        else if(kind==20)policy.disable_depth_write=(raw&255u)!=0;
        else if(kind>=21 && kind<=40){const auto mask=1u<<bits[kind-21];policy.flags=(policy.flags&~mask)|((raw&1u)?mask:0u);}
        // Other admitted kinds have zero +50/+58 and no classification flag
        // callback (policy vtables759). Tess/water alter separate fields.
        else if(kind>41 || kind==5 || kind==8 || kind==13)return false;
    }
    const auto contains=[&](std::wstring_view text){return shader_family.find(text)!=std::wstring_view::npos;};
    const bool rnm=contains(L"RNM"),dol=!rnm && contains(L"DOL");
    if(dol)policy.flags|=0x400;
    policy.forward=(policy.flags&0x1000)!=0;
    if(rnm || dol)policy.flags|=1;
    else if(shader_family.size()<12 && contains(L"GXFlver_Col")){
        policy.forward=true;policy.flags|=0x1001;
    }else if(const auto gi=shader_family.find(L"GI");gi!=std::wstring_view::npos && gi+2==shader_family.size())policy.flags|=1;
    if(contains(L"Emi"))policy.flags|=0x8000;
    if(contains(L"Glow"))policy.flags|=0x10000;
    if(contains(L"MultiBlend4"))policy.flags|=0x800000;
    if(contains(L"MultiBlend3"))policy.flags|=0x1000000;
    if(contains(L"MultiBlend2"))policy.flags|=0x2000000;
    output=policy;return true;
}
uint32_t ClassifyModelMaterial(const ModelMaterialPolicy& policy,bool special) {
    if(special)return policy.forward?0x10u:8u;
    if(policy.transparent || (policy.flags&0x200000))return 2;
    if(policy.forward)return 0x80;
    if(policy.flags&0x8000)return 0x100;
    return policy.alpha_ref?4u:1u;
}
}
