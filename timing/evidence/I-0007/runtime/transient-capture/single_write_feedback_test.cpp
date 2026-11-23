#include "single_write_feedback.hpp"
#include <cstdio>

int main(int argc, char** argv) {
    const float dt = 1.0f/165.0f;
    unsigned int rejected = 0;
    for (unsigned int kind=0; kind<10; ++kind) {
        SingleWriteFeedback s; if (!s.Reset(1,1,1,1)) return 1;
        float output=-77;
        uintptr_t owner=1; uint64_t epoch=1,index=1;
        float before=1,raw=.4f,ddt=dt,native=.8f;
        double rdt=dt; bool active=true; float* target=&output;
        switch(kind) {
            case 0: owner=2; break;
            case 1: epoch=2; break;
            case 2: index=2; break;
            case 3: before=.5f; break;
            case 4: active=false; break;
            case 5: raw=-1; break;
            case 6: ddt=0; break;
            case 7: rdt=0; break;
            case 8: native=1; break;
            case 9: target=nullptr; break;
        }
        if (s.Advance(owner,epoch,index,before,active,true,raw,ddt,rdt,native,target) ||
            s.Value()!=1 || output!=-77) return 2;
        ++rejected;
        // The valid transaction remains reproducible after rejected input.
        if (!s.Advance(1,1,1,1,true,true,.4f,dt,dt,.8f,&output) || output!=1) return 3;
        output=-77;
        if (s.Advance(1,1,1,1,true,true,.4f,dt,dt,.8f,&output) || output!=-77) return 4;
        ++rejected;
    }
    if(argc!=3) return 5;
    FILE *input=nullptr,*output=nullptr;
    if(fopen_s(&input,argv[1],"r") || !input) return 6;
    if(fopen_s(&output,argv[2],"w") || !output) return 7;
    unsigned int active=0,compared=0; unsigned long long index=0;
    float before=0,raw=0,ddt=0,native=0; double rdt=0;
    SingleWriteFeedback s; bool first=true; unsigned int count=0;
    while(fscanf_s(input,"%llu,%f,%u,%u,%f,%f,%lf,%f",&index,&before,&active,
        &compared,&raw,&ddt,&rdt,&native)==8) {
        if(first) { if(!s.Reset(1,1,index,before)) return 8; first=false; }
        float value=0;
        if(!s.Advance(1,1,index,before,active!=0,compared!=0,raw,ddt,rdt,native,&value)) return 9;
        std::fprintf(output,"%.9g\n",static_cast<double>(value)); ++count;
    }
    const bool done=std::feof(input)!=0;
    std::fclose(input);std::fclose(output);
    if(!done || !count) return 10;
    std::printf("PASS: %u replay rows, %u rejected transactions; no game writes.\n",count,rejected);
    return 0;
}
