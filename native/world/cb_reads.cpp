#include "cb_reads.h"
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <sstream>

namespace ds3reads {
namespace {
int Component(char c) { return c=='x'?0:c=='y'?1:c=='z'?2:c=='w'?3:-1; }
// Splits "op dst, src1, src2" into operands (commas inside [] or () are kept).
std::vector<std::string> Operands(const std::string& rest) {
    std::vector<std::string> out; std::string cur; int depth=0;
    for(char c:rest) {
        if(c=='[' || c=='(') ++depth; else if(c==']' || c==')') --depth;
        if(c==',' && !depth) { out.push_back(cur); cur.clear(); continue; }
        cur+=c;
    }
    if(!cur.empty()) out.push_back(cur);
    for(auto& s:out) { size_t a=s.find_first_not_of(' '),b=s.find_last_not_of(' '); s=a==std::string::npos?std::string():s.substr(a,b-a+1); }
    return out;
}
void Mark(std::vector<uint8_t>& v,unsigned index,uint8_t mask) { if(v.size()<=index) v.resize(index+1); v[index]|=mask; }
}
namespace {
bool ConservativeStage(const std::string& line) {
    return line=="vs_5_0" || line=="ps_5_0" || line=="gs_5_0" ||
        line=="hs_5_0" || line=="ds_5_0" || line=="cs_5_0";
}
bool ConservativeOperandStart(const std::string& line,size_t& space) {
    // SDK opcode decorations can contain spaces, e.g.
    // ld_structured_indexable(structured_buffer, stride=12)(mixed,...).
    unsigned depth=0;space=std::string::npos;
    for(size_t at=0;at<line.size();++at) {
        const char c=line[at];
        if(c=='(') { if(++depth>64) return false; }
        else if(c==')') { if(!depth) return false;--depth; }
        else if(!depth && (c==' ' || c=='\t')) { space=at;return true; }
    }
    return depth==0;
}
bool BalancedOperands(const std::string& rest) {
    std::vector<char> closes;
    for(char c:rest) {
        if(c=='[' || c=='(') {
            if(closes.size()>=64) return false;
            closes.push_back(c=='['?']':')');
        } else if(c==']' || c==')') {
            if(closes.empty() || closes.back()!=c) return false;
            closes.pop_back();
        }
    }
    return closes.empty();
}
bool ConservativeRelativeIndex(const std::string& index) {
    if(index.size()<4 || (index[0]!='r' && index[0]!='v')) return false;
    size_t at=1,begin=at;
    while(at<index.size() && std::isdigit(static_cast<unsigned char>(index[at]))) ++at;
    if(at==begin || at+1>=index.size() || index[at]!='.' || Component(index[at+1])<0) return false;
    at+=2;
    while(at<index.size() && std::isspace(static_cast<unsigned char>(index[at]))) ++at;
    if(at==index.size()) return true;
    if(index[at++]!='+') return false;
    while(at<index.size() && std::isspace(static_cast<unsigned char>(index[at]))) ++at;
    begin=at;unsigned base=0;
    while(at<index.size() && std::isdigit(static_cast<unsigned char>(index[at]))) {
        base=base*10+static_cast<unsigned>(index[at++]-'0');
        if(base>4095) return false;
    }
    return at>begin && at==index.size();
}
bool ConservativeOperands(const std::string& rest,Reads& reads) {
    if(!BalancedOperands(rest)) return false;
    const auto operands=Operands(rest);
    if(operands.size()>64) return false;
    for(const auto& operand:operands) {
        for(size_t p=0;p+1<operand.size();++p) {
            if(std::tolower(static_cast<unsigned char>(operand[p]))!='c' ||
               std::tolower(static_cast<unsigned char>(operand[p+1]))!='b') continue;
            if(p && (std::isalnum(static_cast<unsigned char>(operand[p-1])) || operand[p-1]=='_')) continue;
            if(operand[p]!='c' || operand[p+1]!='b') return false;
            size_t q=p+2;unsigned reg=0;bool digits=false;
            while(q<operand.size() && std::isdigit(static_cast<unsigned char>(operand[q]))) {
                reg=reg*10+static_cast<unsigned>(operand[q]-'0');
                if(reg>13) return false;
                ++q;digits=true;
            }
            if(!digits || q>=operand.size() || operand[q]!='[') return false;
            const size_t close=operand.find(']',q+1);
            if(close==std::string::npos) return false;
            std::string index=operand.substr(q+1,close-q-1);
            const size_t first=index.find_first_not_of(" \t"),last=index.find_last_not_of(" \t");
            if(first==std::string::npos || index.find_first_of("[]")!=std::string::npos) return false;
            index=index.substr(first,last-first+1);
            uint8_t mask=0xF;q=close+1;
            if(q<operand.size() && operand[q]=='.') {
                ++q;mask=0;size_t components=0;
                while(q<operand.size() && Component(operand[q])>=0) {
                    if(++components>4) return false;
                    mask|=static_cast<uint8_t>(1u<<Component(operand[q++]));
                }
                if(!components) return false;
            }
            for(size_t tail=q;tail<operand.size();++tail)
                if(operand[tail]!=')' && !std::isspace(static_cast<unsigned char>(operand[tail]))) return false;
            bool immediate=true;unsigned at=0;
            for(char c:index) {
                if(!std::isdigit(static_cast<unsigned char>(c))) { immediate=false;break; }
                at=at*10+static_cast<unsigned>(c-'0');
                if(at>4095) return false;
            }
            if(immediate) Mark(reads.fixed[reg],at,mask);
            else {
                // An unsupported numeric spelling is not a relative operand.
                if(!ConservativeRelativeIndex(index)) return false;
                Mark(reads.dynamic[reg],0,0xF);
            }
            p=q? q-1:q;
        }
    }
    return true;
}
Reads AnalyzeConservativeListing(const std::string& listing) {
    Reads reads;bool stage=false,instruction=false;
    if(listing.size()>16u*1024*1024) return {};
    std::istringstream lines(listing);std::string line;
    while(std::getline(lines,line)) {
        if(line.size()>65536 || line.find('\0')!=std::string::npos) return {};
        const size_t first=line.find_first_not_of(" \t\r"),last=line.find_last_not_of(" \t\r");
        if(first==std::string::npos) continue;
        line=line.substr(first,last-first+1);
        if(line[0]=='/') continue;
        if(ConservativeStage(line)) { if(stage || instruction) return {};stage=true;continue; }
        if(!stage) return {};
        if(line.rfind("dcl_",0)==0) continue;
        instruction=true;
        size_t space{};
        if(!ConservativeOperandStart(line,space)) return {};
        if(space==std::string::npos) {
            if(line.find("cb")!=std::string::npos || line.find("CB")!=std::string::npos) return {};
            continue;
        }
        if(!ConservativeOperands(line.substr(space+1),reads)) return {};
    }
    if(lines.bad() || !stage || !instruction) return {};
    reads.ok=true;return reads;
}
}
Reads AnalyzeConservative(const std::vector<uint8_t>& dxbc) {
    try {
        if(dxbc.size()<32 || dxbc.size()>1024u*1024 || std::memcmp(dxbc.data(),"DXBC",4)!=0) return {};
        Microsoft::WRL::ComPtr<ID3DBlob> text;
        if(FAILED(D3DDisassemble(dxbc.data(),dxbc.size(),D3D_DISASM_INSTRUCTION_ONLY,nullptr,&text)) || !text) return {};
        const auto* data=static_cast<const char*>(text->GetBufferPointer());
        size_t bytes=text->GetBufferSize();
        if(!data || !bytes || bytes>16u*1024*1024) return {};
        if(data[bytes-1]=='\0') --bytes;
        return AnalyzeConservativeListing(std::string(data,bytes));
    } catch(...) { return {}; }
}
std::vector<uint8_t> FloatMask(const Reads& r,unsigned reg,uint32_t bytes,bool* dynamic) {
    std::vector<uint8_t> m(bytes/4,0);
    if(auto it=r.fixed.find(reg); it!=r.fixed.end())
        for(unsigned i=0;i<it->second.size();++i) for(unsigned c=0;c<4;++c) if((it->second[i]>>c)&1 && 4*i+c<m.size()) m[4*i+c]=1;
    if(auto it=r.dynamic.find(reg); it!=r.dynamic.end()) {
        if(dynamic) *dynamic=true;
        const auto st=r.stride.find(reg); const unsigned stride=st==r.stride.end()?0:st->second;
        for(unsigned i=0;i<it->second.size();++i) if(it->second[i]) {
            if(stride) { for(size_t e=i;4*e<m.size();e+=stride) for(unsigned c=0;c<4;++c) if((it->second[i]>>c)&1 && 4*e+c<m.size()) m[4*e+c]=1; }   // same field of every record
            else for(size_t j=4*i;j<m.size();++j) if((it->second[i]>>(j%4))&1) m[j]=1; }
    }
    return m;
}
}
