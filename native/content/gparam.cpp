#include "gparam.h"
#include "../third_party/zlib/zlib.h"
#include <cstring>
#include <stdexcept>

namespace ds3content {
namespace {
void Require(bool value,const char* label) { if(!value) throw std::runtime_error(label); }
struct Reader {
    const std::vector<uint8_t>& d;
    uint32_t U32(size_t at) const { Require(at+4<=d.size(),"GPARAM field extent"); uint32_t v; std::memcpy(&v,d.data()+at,4); return v; }
    uint16_t U16(size_t at) const { Require(at+2<=d.size(),"GPARAM field extent"); uint16_t v; std::memcpy(&v,d.data()+at,2); return v; }
    uint8_t U8(size_t at) const { Require(at<d.size(),"GPARAM field extent"); return d[at]; }
    // UTF-16 null-terminated string; returns the end offset (after the terminator).
    size_t Utf16(size_t at,std::string& out) const {
        out.clear(); for(;;at+=2) { const uint16_t c=U16(at); if(!c) return at+2;
            if(c<0x80) out.push_back(static_cast<char>(c));
            else if(c<0x800) { out.push_back(static_cast<char>(0xC0|(c>>6))); out.push_back(static_cast<char>(0x80|(c&0x3F))); }
            else { out.push_back(static_cast<char>(0xE0|(c>>12))); out.push_back(static_cast<char>(0x80|((c>>6)&0x3F))); out.push_back(static_cast<char>(0x80|(c&0x3F))); } }
    }
};
// The qualified DS3 DCX/DFLT envelope, same checks as texture_container.cpp's Expand (kept separate so that the
// historical texture builds do not change).
std::vector<uint8_t> ExpandDcx(const std::vector<uint8_t>& in) {
    auto be=[&](size_t at){ Require(at+4<=in.size(),"GPARAM DCX field"); return (uint32_t(in[at])<<24)|(uint32_t(in[at+1])<<16)|(uint32_t(in[at+2])<<8)|uint32_t(in[at+3]); };
    auto magic=[&](size_t at,const char* m){ return at+4<=in.size() && !std::memcmp(in.data()+at,m,4); };
    Require(in.size()>=76 && magic(0,"DCX\0") && be(4)==0x10000 && be(8)==0x18 && be(12)==0x24 && be(16)==0x44 && be(20)==0x4C &&
        magic(24,"DCS\0") && magic(36,"DCP\0") && magic(40,"DFLT") && be(44)==0x20 && in[48]==9 && !in[49] && !in[50] && !in[51] &&
        !be(52) && !be(56) && !be(60) && be(64)==0x10100 && magic(68,"DCA\0") && be(72)==8,"unsupported GPARAM DCX envelope");
    const uint32_t expanded=be(28), compressed=be(32);
    Require(expanded>0 && expanded<=(64u<<20) && compressed>0 && compressed==in.size()-76,"GPARAM DCX declared extent");
    std::vector<uint8_t> out(expanded); z_size_t out_size=expanded, in_size=compressed;
    Require(uncompress2_z(out.data(),&out_size,in.data()+76,&in_size)==Z_OK && out_size==expanded && in_size==compressed,"GPARAM DCX stream");
    return out;
}
unsigned Words(uint8_t type) { return type==0xC?2u:type==0xD?3u:type==0xE?4u:1u; }
size_t Stride(uint8_t type) {
    switch(type) { case 1: case 5: case 0xB: return 1; case 2: return 2; case 3: case 7: case 9: case 0xF: return 4; case 0xC: case 0xD: case 0xE: return 16; }
    throw std::runtime_error("GPARAM value type");
}
}
unsigned GparamParam::Components() const noexcept { return Words(type); }
int GparamParam::IndexOf(int32_t id) const noexcept { for(size_t i=0;i<ids.size();++i) if(ids[i]==id) return static_cast<int>(i); return -1; }
float GparamParam::Float(size_t value,unsigned component) const noexcept {
    float f=0.0f; if(value<values.size() && component<4) std::memcpy(&f,&values[value][component],4); return f;
}
const GparamGroup* Gparam::Group(const std::string& name2) const noexcept { for(const auto& g:groups) if(g.name2==name2) return &g; return nullptr; }
Gparam ReadGparam(const std::vector<uint8_t>& input) {
    const std::vector<uint8_t> d=(input.size()>=4 && !std::memcmp(input.data(),"DCX\0",4))?ExpandDcx(input):input;
    const Reader r{d};
    // Magic "filt" or its UTF-16 form "f\0i\0l\0t\0" (the DS3 files); the latter shifts every header field by 4.
    const bool wide=d.size()>=8 && !std::memcmp(d.data(),"f\0i\0l\0t\0",8);
    Require(d.size()>=0x50 && (wide || !std::memcmp(d.data(),"filt",4)),"GPARAM magic");
    const size_t s=wide?4:0;
    Gparam g; g.game=r.U32(s+4); Require(g.game==3,"GPARAM game (DS3 only)");
    const uint32_t group_count=r.U32(s+12), header=r.U32(s+20);
    const uint32_t group_headers=r.U32(s+24), param_header_offsets=r.U32(s+28), param_headers=r.U32(s+32), values=r.U32(s+36), value_ids=r.U32(s+40);
    // An empty group table may start exactly at EOF. A nonempty table must
    // have every declared offset in the payload before any row is visited.
    if(group_count>=4096 || header<s+0x48 || header>d.size() ||
       group_count>(d.size()-header)/4)
        throw std::runtime_error("GPARAM header groups="+std::to_string(group_count)+
            " offset="+std::to_string(header)+" bytes="+std::to_string(d.size()));
    const size_t group_table=header;   // group header offsets start at the header size
    for(uint32_t gi=0;gi<group_count;++gi) {
        GparamGroup grp; const size_t gh=group_headers+r.U32(group_table+4*gi);
        const uint32_t param_count=r.U32(gh), pho=r.U32(gh+4); Require(param_count<4096,"GPARAM param count");
        r.Utf16(r.Utf16(gh+8,grp.name1),grp.name2);
        for(uint32_t pi=0;pi<param_count;++pi) {
            GparamParam p; const size_t ph=param_headers+r.U32(param_header_offsets+pho+4*pi);
            const uint32_t vo=r.U32(ph), io=r.U32(ph+4); p.type=r.U8(ph+8); const uint8_t count=r.U8(ph+9);
            r.Utf16(r.Utf16(ph+12,p.name1),p.name2);
            const size_t stride=Stride(p.type);
            for(uint8_t k=0;k<count;++k) { std::array<uint32_t,4> w{}; const size_t at=values+vo+k*stride;
                switch(p.type) {
                case 1: case 5: case 0xB: w[0]=r.U8(at); break;
                case 2: w[0]=static_cast<uint32_t>(static_cast<int32_t>(static_cast<int16_t>(r.U16(at)))); break;
                default: for(unsigned c=0;c<Words(p.type);++c) w[c]=r.U32(at+4*c); }
                p.values.push_back(w); p.ids.push_back(static_cast<int32_t>(r.U32(value_ids+io+4*k))); }
            grp.params.push_back(std::move(p)); }
        g.groups.push_back(std::move(grp)); }
    return g;
}
}
