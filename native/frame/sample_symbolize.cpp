// m45 frame cost: names the frames of a sample-profile.txt (sample_profile.cpp) and writes the hot spots of each thread.
// Usage: sample_symbolize <sample-profile.txt> <report.txt> [own module name, default game_bootstrap.dll] [focus... | within:NAME | frames:FILE]
// I-0074: frames:FILE keeps only the samples whose frame mark (the @N of a marked file) is listed in FILE, one value a line.
// The own module's frames are named from its PDB (function, inline frames, file:line); other modules by their exports, and
// DarkSoulsIII.exe by the start of the function its .pdata gives (FUN_14xxxxxxx, the Ghidra names).
#include <windows.h>
#include <dbghelp.h>
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
struct Module {uint64_t base{},size{};std::string path,name;};
struct Frame {std::string function,line;bool own{},project{};};
struct Thread {std::string name;uint64_t samples{},failures{};std::vector<std::vector<uint64_t>> stacks;};
const HANDLE process=reinterpret_cast<HANDLE>(static_cast<uintptr_t>(0x5A3));
std::vector<Module> modules;
std::string own_name="game_bootstrap.dll";
const Module* ModuleOf(uint64_t address){for(const auto& m:modules)if(address>=m.base && address<m.base+m.size)return &m;return nullptr;}
std::string Hex(uint64_t v){char text[32]{};sprintf_s(text,"%llX",static_cast<unsigned long long>(v));return text;}
std::string FileLine(const IMAGEHLP_LINE64& line){std::string file=line.FileName?line.FileName:"";const auto slash=file.find_last_of("\\/");
    if(slash!=std::string::npos)file=file.substr(slash+1);return file+":"+std::to_string(line.LineNumber);}
// A project frame: a source file of the repository's native/ tree (not the CRT, the STL or a third party), other than the
// thin reading and checking wrappers, whose time belongs to their caller.
bool ProjectFrame(const IMAGEHLP_LINE64& line,const std::string& function){
    std::string file=line.FileName?line.FileName:"";for(auto& ch:file)ch=static_cast<char>(tolower(static_cast<unsigned char>(ch)));
    if(file.find("\\native\\")==std::string::npos || file.find("third_party")!=std::string::npos)return false;
    const auto ends=[&](const char* tail){const std::string t(tail);return function.size()>=t.size() && function.compare(function.size()-t.size(),t.size(),t)==0;};
    return !ends("::Read") && !ends("GuardedCopy") && !ends("GuardedSame") && !ends("::Load") && !ends("::Require") && function!="Read" && function!="Require";
}
// The logical frames of one address, innermost first (inline frames, then the function).
std::vector<Frame> Name(uint64_t address){
    std::vector<Frame> frames;const auto* m=ModuleOf(address);
    if(!m){frames.push_back({"?!"+Hex(address),"",false});return frames;}
    const bool own=_stricmp(m->name.c_str(),own_name.c_str())==0;
    alignas(SYMBOL_INFO) char buffer[sizeof(SYMBOL_INFO)+512]{};auto* symbol=reinterpret_cast<SYMBOL_INFO*>(buffer);
    const auto reset=[&]{std::memset(buffer,0,sizeof buffer);symbol->SizeOfStruct=sizeof(SYMBOL_INFO);symbol->MaxNameLen=511;};
    if(own){
        const DWORD inlines=SymAddrIncludeInlineTrace(process,address);DWORD context{},index{};
        if(inlines && SymQueryInlineTrace(process,address,0,address,address,&context,&index)){
            for(DWORD i=0;i<inlines;++i){reset();DWORD64 displacement{};DWORD line_displacement{};IMAGEHLP_LINE64 line{};line.SizeOfStruct=sizeof line;
                std::string name=SymFromInlineContext(process,address,context+i,&displacement,symbol)?symbol->Name:"?";
                const bool lined=SymGetLineFromInlineContext(process,address,context+i,0,&line_displacement,&line);
                frames.push_back({name,lined?FileLine(line):"",true,lined && ProjectFrame(line,name)});}}
        reset();DWORD64 displacement{};DWORD line_displacement{};IMAGEHLP_LINE64 line{};line.SizeOfStruct=sizeof line;
        std::string name=SymFromAddr(process,address,&displacement,symbol)?symbol->Name:m->name+"+"+Hex(address-m->base);
        std::string where;bool project=false;
        if(inlines){// the outer function's own line: the call site of the outermost inline frame
            if(context && SymGetLineFromInlineContext(process,address,context+inlines,0,&line_displacement,&line)){where=FileLine(line);project=ProjectFrame(line,name);}}
        else if(SymGetLineFromAddr64(process,address,&line_displacement,&line)){where=FileLine(line);project=ProjectFrame(line,name);}
        frames.push_back({name,where,true,project});
        return frames;}
    if(_stricmp(m->name.c_str(),"DarkSoulsIII.exe")==0){
        const auto* entry=static_cast<const IMAGE_RUNTIME_FUNCTION_ENTRY*>(SymFunctionTableAccess64(process,address));
        frames.push_back({entry?"DS3!FUN_"+Hex(0x140000000ull+entry->BeginAddress):"DS3!+"+Hex(address-m->base),"",false});return frames;}
    reset();DWORD64 displacement{};
    if(SymFromAddr(process,address,&displacement,symbol))frames.push_back({m->name+"!"+symbol->Name,"",false});
    else frames.push_back({m->name+"+"+Hex(address-m->base),"",false});
    return frames;
}
template<class Map> void Top(std::ostream& out,const char* title,const Map& counts,uint64_t total,size_t rows){
    std::vector<std::pair<std::string,uint64_t>> sorted(counts.begin(),counts.end());
    std::sort(sorted.begin(),sorted.end(),[](const auto& a,const auto& b){return a.second>b.second;});
    out<<"\n  "<<title<<"\n";
    for(size_t i=0;i<sorted.size() && i<rows;++i){char row[64]{};sprintf_s(row,"  %6.2f%% %7llu  ",100.0*double(sorted[i].second)/double(total?total:1),
        static_cast<unsigned long long>(sorted[i].second));out<<row<<sorted[i].first<<"\n";}
}
}
int main(int argc,char** argv){
    if(argc<3){std::fprintf(stderr,"usage: sample_symbolize <sample-profile.txt> <report.txt> [own module [focus... | within:NAME]]\n");return 2;}
    if(argc>3)own_name=argv[3];
    // Functions whose project callers are listed; "within:NAME" counts only the samples whose stack holds NAME (the percentages
    // stay of all the thread's samples): the inclusive split of one function's subtree (the optimisation review's reflection walk).
    std::vector<std::string> focus;std::string within;std::set<uint64_t> kept_marks;bool by_mark=false;
    for(int i=4;i<argc;++i){const std::string a=argv[i];if(a.rfind("within:",0)==0)within=a.substr(7);
        else if(a.rfind("frames:",0)==0){by_mark=true;std::ifstream marks(a.substr(7));uint64_t value{};while(marks>>value)kept_marks.insert(value);}
        else focus.push_back(a);}
    std::ifstream in(argv[1]);if(!in){std::fprintf(stderr,"cannot read %s\n",argv[1]);return 2;}
    std::vector<Thread> threads;std::string text;
    while(std::getline(in,text)){std::istringstream line(text);std::string kind;line>>kind;
        if(kind=="module"){Module m;line>>std::hex>>m.base>>m.size>>std::dec;std::getline(line,m.path);m.path.erase(0,m.path.find_first_not_of(' '));
            const auto slash=m.path.find_last_of("\\/");m.name=slash==std::string::npos?m.path:m.path.substr(slash+1);modules.push_back(m);}
        else if(kind=="thread"){size_t index{};Thread t;line>>index>>t.name>>t.samples>>t.failures;if(threads.size()<=index)threads.resize(index+1);threads[index]=t;}
        else if(kind=="s"){size_t index{};line>>index>>std::ws;bool keep=!by_mark;
            if(line.peek()=='@'){line.get();uint64_t mark{};line>>mark;keep=!by_mark || kept_marks.count(mark);}
            std::vector<uint64_t> stack;uint64_t pc{};while(line>>std::hex>>pc)stack.push_back(pc);
            if(keep && index<threads.size())threads[index].stacks.push_back(std::move(stack));}}
    SymSetOptions(SYMOPT_UNDNAME|SYMOPT_LOAD_LINES|SYMOPT_DEFERRED_LOADS|SYMOPT_FAIL_CRITICAL_ERRORS|SYMOPT_NO_PROMPTS);
    if(!SymInitialize(process,nullptr,FALSE)){std::fprintf(stderr,"SymInitialize failed\n");return 1;}
    for(const auto& m:modules){std::wstring path(m.path.begin(),m.path.end());
        SymLoadModuleExW(process,nullptr,path.c_str(),nullptr,m.base,static_cast<DWORD>(m.size),nullptr,0);}
    std::unordered_map<uint64_t,std::vector<Frame>> names;
    const auto named=[&](uint64_t address)->const std::vector<Frame>&{auto found=names.find(address);if(found==names.end())found=names.emplace(address,Name(address)).first;return found->second;};
    std::ofstream out(argv[2]);
    for(const auto& t:threads){
        const uint64_t total=t.stacks.size();
        out<<"\n== thread "<<t.name<<": "<<total<<" samples"<<(by_mark?" in the listed frames (of "+std::to_string(t.samples)+")":std::string())<<", "<<t.failures<<" failures\n";
        std::vector<std::map<std::string,uint64_t>> focus_callers(focus.size());
        std::map<std::string,uint64_t> leaf,leaf_module,own_innermost,own_innermost_function,project_innermost,project_function,project_caller,inclusive;
        for(const auto& stack:t.stacks){if(stack.empty())continue;
            if(!within.empty()){bool holds=false;
                for(size_t at=0;at<stack.size() && !holds;++at)for(const auto& f:named(at?stack[at]-1:stack[at]))if(f.function.find(within)!=std::string::npos){holds=true;break;}
                if(!holds)continue;}
            const auto& first=named(stack[0]);leaf[first.front().function]++;
            const auto* m=ModuleOf(stack[0]);leaf_module[m?m->name:"?"]++;
            bool found_own=false;std::vector<std::string> seen;std::string project_first;bool project_done=false;
            std::vector<int> focus_state(focus.size());   // 0: not met, 1: met (the next project frame is its caller), 2: counted
            for(size_t at=0;at<stack.size();++at){const auto& frames=named(at?stack[at]-1:stack[at]);   // a caller: its call instruction
                for(const auto& f:frames){
                    if(f.own && !found_own){found_own=true;own_innermost[f.function+"  "+f.line]++;own_innermost_function[f.function]++;}
                    if(f.project && !project_done){
                        if(project_first.empty()){project_first=f.function;project_innermost[f.function+"  "+f.line]++;project_function[f.function]++;}
                        else if(f.function!=project_first){project_caller[project_first+"  <-  "+f.function+"  "+f.line]++;project_done=true;}}
                    for(size_t k=0;k<focus.size();++k){const bool match=f.function.find(focus[k])!=std::string::npos;
                        if(match && !focus_state[k])focus_state[k]=1;
                        else if(focus_state[k]==1 && !match && f.project){focus_callers[k][f.function+"  "+f.line]++;focus_state[k]=2;}}
                    if(std::find(seen.begin(),seen.end(),f.function)==seen.end()){seen.push_back(f.function);inclusive[f.function]++;}}}}
        Top(out,"leaf module",leaf_module,total,15);
        Top(out,"leaf function",leaf,total,40);
        Top(out,"innermost own frame (function, line): its self time with the callees outside the module",own_innermost,total,80);
        Top(out,"innermost own function",own_innermost_function,total,60);
        Top(out,"innermost project frame (function, line): its self time with the CRT, STL, system and driver below it",project_innermost,total,100);
        Top(out,"innermost project function",project_function,total,80);
        Top(out,"innermost project function <- its project caller (call line)",project_caller,total,100);
        Top(out,"inclusive (logical frames, each once a sample)",inclusive,total,100);
        for(size_t k=0;k<focus.size();++k)Top(out,("project callers of "+focus[k]).c_str(),focus_callers[k],total,40);
    }
    {IMAGEHLP_MODULEW64 info{};info.SizeOfStruct=sizeof info;for(const auto& m:modules)if(_stricmp(m.name.c_str(),own_name.c_str())==0){
        SymGetModuleInfoW64(process,m.base,&info);out<<"\nown module "<<m.name<<" symbols: "<<(info.SymType==SymPdb?"pdb":"none")<<(info.LineNumbers?", lines":"")<<"\n";}}
    SymCleanup(process);
    return 0;
}
