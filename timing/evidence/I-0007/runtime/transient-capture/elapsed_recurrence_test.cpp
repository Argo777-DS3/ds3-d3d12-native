#include "elapsed_recurrence.hpp"
#include <cstdio>
#include <limits>
int main(int argc, char** argv) {
    for (int code=0; code<6561; ++code) {
        ElapsedRecurrence e; if (!e.Seed(.5f)) return 1;
        int n=code; float native=.5f;
        for(int i=0;i<8;++i) {
            const int b=n%3;n/=3;
            native=ElapsedRecurrence::Native(native,b!=0,b==1);
            if(!e.Advance(b!=0,b==1,1.0/60.0)||e.value!=native)return 2;
        }
    }
    ElapsedRecurrence full,split;full.Seed(.5f);split.Seed(.5f);
    for(int i=0;i<10;++i){
        if(!full.Advance(true,false,1.0/60.0))return 3;
        for(int j=0;j<11;++j)if(!split.Advance(true,false,1.0/660.0))return 4;
        if(std::fabs(full.value-split.value)>1e-6f)return 5;
    }
    ElapsedRecurrence gap;gap.Seed(.5f);gap.Advance(true,false,1.0/120.0);
    const float held=gap.value;
    for(int i=0;i<100;++i)if(!gap.Advance(false,false,1.0/60.0)||gap.value!=held)return 6;
    if(!gap.Advance(true,false,1.0/120.0)||gap.value!=.6f)return 7;
    if(gap.Advance(true,false,0)||gap.Advance(true,false,std::numeric_limits<double>::quiet_NaN())||gap.Advance(true,false,1))return 8;
    if(argc==3) {
        FILE *input=nullptr,*output=nullptr;
        if(fopen_s(&input,argv[1],"r")||!input)return 9;
        if(fopen_s(&output,argv[2],"w")||!output){std::fclose(input);return 10;}
        unsigned int active=0,reduce=0;double dt=0;
        ElapsedRecurrence replay;replay.Seed(1);
        while(std::fscanf(input,"%u,%u,%lf",&active,&reduce,&dt)==3){
            if(!replay.Advance(active!=0,reduce!=0,dt))return 11;
            std::fprintf(output,"%.9g\n",static_cast<double>(replay.value));
        }
        std::fclose(input);std::fclose(output);
    }
    std::puts("PASS: 6561 native60 branch/skip sequences, fractional partition, inactive hold/resume and invalid delta rejection. Continuous prototype only; no game proof.");
}
