// I-0111 (R1) T-U2, T-U3, T-U5, T-U7, T-U13 (UPSCALER-DLSS-READY §5): the Halton jitter and its phase counts; the scene camera
// producer's projection words at zero jitter (the engine's -0.0, bit for bit) and with a jitter (m20 = 2 jx / W, m21 = -2 jy / H,
// every other camera word unchanged); the resolve's matrix with the jitter folded giving the unjittered motion; the object rows'
// shifted matrices giving the unjittered NDC delta.
#include "../renderer/upscaler.h"
#include "../world/producers.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <random>
#include <string>
#include <vector>

using namespace ds3native;
namespace {
int failures{},checks{};
void Expect(bool ok,const std::string& what) {++checks;if(!ok){++failures;std::printf("FAIL %s\n",what.c_str());}}
uint32_t Bits(const uint8_t* p,uint32_t offset) {uint32_t u;std::memcpy(&u,p+offset,4);return u;}
float Word(const uint8_t* p,uint32_t offset) {float f;std::memcpy(&f,p+offset,4);return f;}
std::array<double,4> Row(const std::array<double,4>& v,const VelocityMatrix& m) {
    std::array<double,4> r{};for(unsigned c=0;c<4;++c)for(unsigned k=0;k<4;++k)r[c]+=v[k]*m[k*4+c];return r;}
std::array<double,4> RowF(const std::array<double,4>& v,const Matrix4& m) {
    std::array<double,4> r{};for(unsigned c=0;c<4;++c)for(unsigned k=0;k<4;++k)r[c]+=v[k]*double(m[k*4+c]);return r;}
VelocityMatrix ViewProjection(double yaw,double x,double y,double z,double fov,double aspect) {
    const double c=std::cos(yaw),s=std::sin(yaw);
    const VelocityMatrix pose{c,0,-s,0, 0,1,0,0, s,0,c,0, x,y,z,1};VelocityMatrix view{};(void)VelocityInverse(pose,view);
    const double sy=1/std::tan(fov/2),n=0.1,f=1000,a=f/(f-n);
    return VelocityMultiply(view,{sy/aspect,0,0,0, 0,sy,0,0, 0,0,a,1, 0,0,-n*a,0});
}
}
int main() {
    // T-U7: the sequence (Halton (2, 3) from index 1, minus 0.5) and the phase counts.
    const double expected[8][2]{{0,-0.1666667},{-0.25,0.1666667},{0.25,-0.3888889},{-0.375,-0.0555556},{0.125,0.2777778},{-0.125,-0.2777778},{0.375,0.0555556},{-0.4375,0.3888889}};
    for(uint32_t i=0;i<8;++i){const auto j=upscaler::Jitter(i,8);
        Expect(std::fabs(j[0]-expected[i][0])<1e-6 && std::fabs(j[1]-expected[i][1])<1e-6,"Halton sample "+std::to_string(i));}
    Expect(upscaler::Jitter(8,8)==upscaler::Jitter(0,8),"the sequence wraps at its phase count");
    Expect(upscaler::JitterPhases(1920,1920)==8 && upscaler::JitterPhases(1280,1920)==18 && upscaler::JitterPhases(960,1920)==32 &&
        upscaler::JitterPhases(640,1920)==72,"phase counts (DLAA, Quality, Performance, Ultra Performance)");
    // T-U2 and T-U3: the camera block at zero jitter and with a jitter.
    ds3prod::Camera camera{};camera.right[0]=1;camera.up[1]=1;camera.forward[2]=1;camera.position[0]=12.5f;camera.position[1]=-3.25f;camera.position[2]=40;
    camera.fovy=0.9f;camera.near_z=0.1f;camera.far_z=2000;camera.aspect=16.0f/9.0f;camera.width=1280;camera.height=720;
    ds3prod::UseRenderSize(1280,720);
    uint8_t plain[960]{},jittered[960]{};ds3prod::ProduceSceneCamera(camera,plain);
    // FC_MatrixProj at 256, transposed: m[2][0] at row 0 column 2 (byte 264), m[2][1] at row 1 column 2 (byte 280).
    Expect(Bits(plain,264)==0x80000000u && Bits(plain,280)==0x80000000u,"zero jitter keeps the -0.0 centre words");
    camera.jitter[0]=0.25f;camera.jitter[1]=-0.375f;ds3prod::ProduceSceneCamera(camera,jittered);
    Expect(Word(jittered,264)==2.0f*0.25f/1280.0f && Word(jittered,280)==-2.0f*-0.375f/720.0f,"jittered centre words");
    const auto ndc=upscaler::JitterNdc({0.25f,-0.375f},1280,720);
    Expect(ndc[0]==Word(jittered,264) && ndc[1]==Word(jittered,280),"JitterNdc equals the producer's words");
    Expect(upscaler::JitterNdc({0,0},1280,720)[0]==-0.0f && std::signbit(upscaler::JitterNdc({0,0},1280,720)[0]),"JitterNdc at zero is -0.0");
    // I-0116: an engine block's projection shifted as the camera's: FC_MatrixProj (transposed) gives the producer's words, the same
    // matrix stored row by row gives its transpose, an orthographic one is refused.
    {float transposed[16]{},rows[16]{},ortho[16]{};std::memcpy(transposed,plain+256,64);
        for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)rows[r*4+c]=transposed[c*4+r];
        Expect(upscaler::ShiftProjectionWords(transposed,ndc)==1,"the camera's projection read as transposed");
        bool same=true;for(unsigned i=0;i<16;++i)same=same && transposed[i]==Word(jittered,256+4*i);
        Expect(same,"the shifted block equals the producer's jittered FC_MatrixProj");
        Expect(upscaler::ShiftProjectionWords(rows,ndc)==2,"the row-by-row projection read as rows");
        bool transpose=true;for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)transpose=transpose && rows[r*4+c]==transposed[c*4+r];
        Expect(transpose,"the row-by-row shift is the transposed one's transpose");
        ortho[0]=ortho[5]=ortho[10]=ortho[15]=1;Expect(upscaler::ShiftProjectionWords(ortho,ndc)==0 && ortho[0]==1 && ortho[3]==0,"an orthographic block refused and untouched");
        // A world-to-clip block (the step review's H2): V·P with a turned, moved view (row vectors), stored transposed and row by
        // row: not told by the projection's pattern, and shifted by its declared layout it equals V·(the jittered P).
        float p[16]{};for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)p[r*4+c]=Word(plain,256+4*(c*4+r));   // the logical P
        float pj[16]{};std::memcpy(pj,p,64);upscaler::ShiftProjectionWords(pj,ndc,2);
        const float cs=std::cos(0.3f),sn=std::sin(0.3f);const float v[16]{cs,0,-sn,0, 0,1,0,0, sn,0,cs,0, 4,-2,7,1};
        float vp[16]{},vpj[16]{};
        for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)for(unsigned k=0;k<4;++k){vp[r*4+c]+=v[r*4+k]*p[k*4+c];vpj[r*4+c]+=v[r*4+k]*pj[k*4+c];}
        float vp_rows[16]{},vp_transposed[16]{};std::memcpy(vp_rows,vp,64);for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)vp_transposed[c*4+r]=vp[r*4+c];
        Expect(upscaler::ShiftProjectionWords(vp_rows,ndc)==0 && vp_rows[0]==vp[0],"a view-projection is not told by the projection's pattern");
        Expect(upscaler::ShiftProjectionWords(vp_rows,ndc,2)==2 && upscaler::ShiftProjectionWords(vp_transposed,ndc,1)==1,"a view-projection shifted by its declared layout");
        double worst=0;for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c){const double want=vpj[r*4+c];
            worst=std::max({worst,std::fabs(vp_rows[r*4+c]-want)/std::max(1.0,std::fabs(want)),std::fabs(vp_transposed[c*4+r]-want)/std::max(1.0,std::fabs(want))});}
        Expect(worst<1e-5,"the shifted view-projection equals V times the jittered projection");}
    // I-0116 (step reviews 2 and 3): a world-to-clip block told against the scene camera's view-projection, and the arena copy's
    // sites shifted once a block, outside their block refused, without the camera's matrices left.
    {const VelocityMatrix vp=ViewProjection(0.7,30,5,-12,0.9,16.0/9.0);VelocityMatrix inverse{};Expect(VelocityInverse(vp,inverse),"the camera's view-projection inverts");
        const auto store=[](const VelocityMatrix& m,int layout,float (&f)[16]){for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)(layout==1?f[c*4+r]:f[r*4+c])=float(m[r*4+c]);};
        // The same pose through another lens: field of view, near and far planes, a jitter in NDC (m20, m21).
        const auto lens=[](double fov,double n,double f,double jx,double jy){
            const double c=std::cos(0.7),s=std::sin(0.7);const VelocityMatrix pose{c,0,-s,0, 0,1,0,0, s,0,c,0, 30,5,-12,1};VelocityMatrix view{};(void)VelocityInverse(pose,view);
            const double sy=1/std::tan(fov/2),a=f/(f-n);return VelocityMultiply(view,{sy/(16.0/9.0),0,0,0, 0,sy,0,0, jx,jy,a,1, 0,0,-n*a,0});};
        float rows[16]{},transposed[16]{};store(vp,2,rows);store(vp,1,transposed);
        Expect(upscaler::WorldClipLayout(rows,inverse,false)==2 && upscaler::WorldClipLayout(transposed,inverse,false)==1,"a view-projection's layout told by the camera's");
        store(lens(0.9,0.5,20000,0,0),2,rows);Expect(upscaler::WorldClipLayout(rows,inverse,false)==2,"a view-projection with other near and far planes accepted");
        store(lens(0.95,0.1,1000,0,0),2,rows);Expect(upscaler::WorldClipLayout(rows,inverse,false)==0,"a view-projection of another field of view refused");
        const double c=std::cos(1.1),s=std::sin(1.1);const VelocityMatrix world{2*c,0,-2*s,0, 0,2,0,0, 2*s,0,2*c,0, 214.5,-37.25,980,1};   // scaled, turned, far from the origin
        const auto wvp=VelocityMultiply(world,vp);store(wvp,2,rows);store(wvp,1,transposed);
        Expect(upscaler::WorldClipLayout(rows,inverse,true)==2 && upscaler::WorldClipLayout(transposed,inverse,true)==1,"a world-view-projection's layout told by the camera's");
        Expect(upscaler::WorldClipLayout(rows,inverse,false)==0,"a world-view-projection is not the camera's view-projection");
        store(VelocityMultiply(world,lens(0.9,2,500,0,0)),1,transposed);Expect(upscaler::WorldClipLayout(transposed,inverse,true)==1,"a world-view-projection with other near and far planes accepted");
        store(ViewProjection(2.4,-60,40,15,0.6,1.0),2,rows);Expect(upscaler::WorldClipLayout(rows,inverse,false)==0,"another view's projection refused");
        const VelocityMatrix ortho{0.01,0,0,0, 0,0.01,0,0, 0,0,0.001,0, 0.3,-0.2,0.5,1};store(VelocityMultiply(world,ortho),2,rows);
        Expect(upscaler::WorldClipLayout(rows,inverse,true)==0,"an orthographic (light) block refused");
        // A block whose z column is a times its w column in both readings (fourth step review L2) refused; one in neither (w
        // constant: an orthographic reading) refused by its a = 0.
        const float both[16]{1,0,0,0, 0,1,0,0, 0,0,1.2f,1, 0,0,1,0};Expect(upscaler::WorldClipLayout(both,inverse,true)==0,"a block both layouts hold refused");
        const float flat[16]{1,0,0,0, 0,1,0,0, 0,0,0,0, 0,0,0.5f,1};Expect(upscaler::WorldClipLayout(flat,inverse,true)==0,"a block without a perspective w refused");
        // The arena: a transposed pure projection (FC_MatrixProj) at block 64 offset 0, the view-projection stored transposed at
        // block 192 offset 0 while declared row_major, a world-view-projection row by row at block 320.
        std::vector<uint8_t> arena(512);std::memcpy(arena.data()+64,plain+256,64);
        store(vp,1,transposed);std::memcpy(arena.data()+192,transposed,64);store(wvp,2,rows);std::memcpy(arena.data()+320,rows,64);
        const auto before=arena;std::unordered_set<uint64_t> done;upscaler::JitterCounts n;
        for(int draw=0;draw<2;++draw)upscaler::ShiftSite(arena.data(),arena.size(),64,128,{0,0,1},ndc,&inverse,done,n);
        Expect(n.shifted[0]==1 && std::memcmp(arena.data()+64,jittered+256,64)==0,"a block two draws share shifted once, as the producer's");
        for(int draw=0;draw<2;++draw){upscaler::ShiftSite(arena.data(),arena.size(),64,128,{96,0,1},ndc,&inverse,done,n);
            upscaler::ShiftSite(arena.data(),arena.size(),480,64,{0,0,1},ndc,&inverse,done,n);}
        Expect(n.outside==2 && n.shifted[0]==1,"a site beyond its block or the arena refused, once a block");
        {std::vector<uint8_t> twice(128);std::memcpy(twice.data(),jittered+256,64);const auto kept=twice;std::unordered_set<uint64_t> once;upscaler::JitterCounts m;
            upscaler::ShiftSite(twice.data(),twice.size(),0,64,{0,0,1},ndc,&inverse,once,m);
            Expect(m.unmatched==1 && m.shifted[0]==0 && twice==kept,"a projection already off-centre left (final review L2)");
            float rows_j[16]{};for(unsigned r=0;r<4;++r)for(unsigned col=0;col<4;++col)rows_j[r*4+col]=Word(jittered,256+4*(col*4+r));   // the same, row by row
            std::memcpy(twice.data()+64,rows_j,64);const auto kept_rows=twice;upscaler::ShiftSite(twice.data(),twice.size(),64,64,{0,0,2},ndc,&inverse,once,m);
            Expect(m.unmatched==2 && m.shifted[0]==0 && twice==kept_rows,"a projection already off-centre, row by row, left (fourth step review L3)");}
        upscaler::ShiftSite(arena.data(),arena.size(),320,64,{0,2,2},ndc,nullptr,done,n);
        Expect(n.unmatched==1 && std::memcmp(arena.data()+320,before.data()+320,64)==0,"a world-to-clip block without the camera's matrices left");
        std::unordered_set<uint64_t> again;upscaler::ShiftSite(arena.data(),arena.size(),192,64,{0,1,2},ndc,&inverse,again,n);
        upscaler::ShiftSite(arena.data(),arena.size(),320,64,{0,2,2},ndc,&inverse,again,n);
        Expect(n.shifted[1]==1 && n.shifted[2]==1 && n.disagreed==1,"a view-projection shifted by the camera's layout, its declared class counted against it");
        const auto want=upscaler::ShiftClip(vp,ndc),want_world=upscaler::ShiftClip(wvp,ndc);double worst=0;
        for(unsigned r=0;r<4;++r)for(unsigned col=0;col<4;++col){
            worst=std::max(worst,std::fabs(double(Word(arena.data(),192+4*(col*4+r)))-want[r*4+col])/std::max(1.0,std::fabs(want[r*4+col])));
            worst=std::max(worst,std::fabs(double(Word(arena.data(),320+4*(r*4+col)))-want_world[r*4+col])/std::max(1.0,std::fabs(want_world[r*4+col])));}
        Expect(worst<1e-5,"the shifted blocks equal the camera's jittered view-projection and world-view-projection");}
    Expect(std::memcmp(plain+16,jittered+16,64)==0 && std::memcmp(plain+80,jittered+80,48)==0 && std::memcmp(plain+320,jittered+320,64)==0 &&
        std::memcmp(plain+880,jittered+880,4)==0,"screen, depth, position, clip, view, inverse view and near unchanged by the jitter");
    Expect(std::memcmp(plain+128,jittered+128,64)!=0 && std::memcmp(plain+192,jittered+192,64)!=0 && std::memcmp(plain+384,jittered+384,64)!=0,
        "InvViewProj, InvProj and VC_MatrixViewProj carry the jitter");
    ds3prod::UseRenderSize(0,0);
    // T-U5, T-U13 and the resolve: random cameras moving as from one frame to the next, jitters and world points. The resolve is
    // float (as the shader's constants): its error is compared with the unjittered resolve's own at the same point.
    std::mt19937 random(1111);std::uniform_real_distribution<double> unit(-1,1);
    double resolve_error=0,resolve_excess=0,object_error=0;
    for(int trial=0;trial<200;++trial){
        const uint32_t w=1000+uint32_t(trial*7),h=600+uint32_t(trial*3);
        const double yaw=unit(random)*3,x=unit(random)*10,y=unit(random)*2,z=unit(random)*10;
        const auto current=ViewProjection(yaw,x,y,z,1.0,double(w)/h);
        const auto previous=ViewProjection(yaw+unit(random)*0.05,x+unit(random)*0.3,y+unit(random)*0.1,z+unit(random)*0.3,1.0,double(w)/h);
        const std::array<float,2> jitter{float(unit(random)*0.5),float(unit(random)*0.5)};const auto shift=upscaler::JitterNdc(jitter,w,h);
        const auto jittered_current=upscaler::ShiftClip(current,shift);
        Matrix4 resolve{},unjittered{};if(!VelocityCurrentToPrevious(jittered_current,previous,resolve) || !VelocityCurrentToPrevious(current,previous,unjittered)){Expect(false,"resolve matrix");continue;}
        upscaler::FoldJitter(resolve,jitter,w,h);
        for(int point=0;point<20;++point){
            // A world point in front of the current camera.
            const std::array<double,4> world{unit(random)*50,unit(random)*20,unit(random)*50,1};
            const auto clip=Row(world,current);if(clip[3]<1)continue;const auto before=Row(world,previous);if(before[3]<1)continue;
            const auto uv_of=[](const std::array<double,4>& c){return std::array<double,2>{(c[0]/c[3])*0.5+0.5,0.5-(c[1]/c[3])*0.5};};
            const auto cur=uv_of(clip),prev=uv_of(before);
            const auto jclip=Row(world,jittered_current);const auto pixel=uv_of(jclip);   // the pixel the jittered raster puts it at
            const auto h4=RowF({pixel[0],pixel[1],jclip[2]/jclip[3],1},resolve);
            const double mv[2]{h4[0]/h4[3]-pixel[0],h4[1]/h4[3]-pixel[1]};   // the resolve's previous - uv
            // The float resolve's own precision: the same point through the unjittered matrix at its unjittered pixel.
            const auto p4=RowF({cur[0],cur[1],clip[2]/clip[3],1},unjittered);
            const double base=std::max(std::fabs(p4[0]/p4[3]-prev[0])*w,std::fabs(p4[1]/p4[3]-prev[1])*h);
            const double err=std::max(std::fabs(mv[0]-(prev[0]-cur[0]))*w,std::fabs(mv[1]-(prev[1]-cur[1]))*h);
            resolve_error=std::max(resolve_error,err);resolve_excess=std::max(resolve_excess,err-base);
            // The object rows: both matrices shifted, the NDC difference equals the unjittered one.
            const auto a=Row(world,upscaler::ShiftClip(current,shift)),b=Row(world,upscaler::ShiftClip(previous,shift));
            const double d[2]{a[0]/a[3]-b[0]/b[3],a[1]/a[3]-b[1]/b[3]},u[2]{clip[0]/clip[3]-before[0]/before[3],clip[1]/clip[3]-before[1]/before[3]};
            object_error=std::max({object_error,std::fabs(d[0]-u[0])*w,std::fabs(d[1]-u[1])*h});}}
    Expect(resolve_error<0.02 && resolve_excess<0.005,"resolve with the jitter folded gives the unjittered motion (max "+std::to_string(resolve_error)+
        " px, beyond the unjittered resolve's float error "+std::to_string(resolve_excess)+" px)");
    Expect(object_error<1e-6,"object rows' shifted matrices give the unjittered delta (max "+std::to_string(object_error)+" px)");
    // I-0140 tranche 40 (D-0305): a menu view's own camera motion (MenuViewMotion): the first frame and a cut reset; a static camera
    // under two jitters gives zero motion at the jittered raster's pixels; a moved camera the unjittered motion; a lens BuildSceneView
    // refuses gives none and drops the history.
    {SceneViewInput view{};view.camera_pose={1,0,0,0, 0,1,0,0, 0,0,1,0, 0.5f,1.2f,-3.0f,1};view.vertical_fov=0.1745f;view.near_plane=0.08f;
        view.far_plane=10000.0f;view.width=560;view.height=768;view.explicit_aspect=560.0f/768.0f;view.serial=1;
        upscaler::MenuViewMotionHistory history;Matrix4 resolve{};bool reset{};
        const auto motion_error=[&](const SceneViewInput& now,const SceneViewInput& before){double worst=0;
            VelocityMatrix cur_vp{},prev_vp{};if(!VelocityViewProjection(now,cur_vp) || !VelocityViewProjection(before,prev_vp))return 1e9;
            const auto jcur=upscaler::ShiftClip(cur_vp,upscaler::JitterNdc({now.jitter_x,now.jitter_y},now.width,now.height));
            const auto uv_of=[](const std::array<double,4>& c){return std::array<double,2>{(c[0]/c[3])*0.5+0.5,0.5-(c[1]/c[3])*0.5};};
            for(int i=0;i<64;++i){const std::array<double,4> world{0.5+0.3*unit(random),1.2+0.4*unit(random),0.2*unit(random),1};
                const auto j=Row(world,jcur),c=Row(world,cur_vp),p=Row(world,prev_vp);if(j[3]<0.1 || p[3]<0.1)continue;
                const auto pixel=uv_of(j),cur=uv_of(c),prev=uv_of(p);const auto h4=RowF({pixel[0],pixel[1],j[2]/j[3],1},resolve);
                worst=std::max({worst,std::fabs(h4[0]/h4[3]-pixel[0]-(prev[0]-cur[0]))*now.width,std::fabs(h4[1]/h4[3]-pixel[1]-(prev[1]-cur[1]))*now.height});}
            return worst;};
        auto a=view;const auto ja=upscaler::Jitter(3,8);a.jitter_x=ja[0];a.jitter_y=ja[1];
        Expect(upscaler::MenuViewMotion(a,false,history,resolve,reset) && reset && history.valid,"a menu view's first frame resets");
        Expect(motion_error(a,a)<0.01,"a menu view's reset frame: zero motion at its jittered pixels");
        auto b=view;const auto jb=upscaler::Jitter(4,8);b.jitter_x=jb[0];b.jitter_y=jb[1];
        Expect(upscaler::MenuViewMotion(b,false,history,resolve,reset) && !reset,"a menu view's next frame keeps its history");
        Expect(motion_error(b,a)<0.01,"a static menu camera under another jitter: zero motion ("+std::to_string(motion_error(b,a))+" px)");
        auto c=b;c.camera_pose[12]+=0.01f;c.camera_pose[13]-=0.005f;const auto jc=upscaler::Jitter(5,8);c.jitter_x=jc[0];c.jitter_y=jc[1];
        Expect(upscaler::MenuViewMotion(c,false,history,resolve,reset) && !reset,"a moved menu camera keeps its history");
        Expect(motion_error(c,b)<0.02,"a moved menu camera: the unjittered motion ("+std::to_string(motion_error(c,b))+" px)");
        // DLSS's constants for a DLAA view: the unjittered current clip -> previous clip of the same history (a moved camera).
        {auto d=c;d.camera_pose[12]+=0.02f;Matrix4 clip{};
            Expect(upscaler::MenuViewMotion(d,false,history,resolve,reset,&clip) && !reset,"a menu view's clip to previous clip keeps its history");
            VelocityMatrix cur_vp{},prev_vp{};double worst=1e9;
            if(VelocityViewProjection(d,cur_vp) && VelocityViewProjection(c,prev_vp)){worst=0;
                for(int i=0;i<32;++i){const std::array<double,4> world{0.5+0.3*unit(random),1.2+0.4*unit(random),0.2*unit(random),1};
                    const auto now=Row(world,cur_vp),before=Row(world,prev_vp),mapped=RowF(now,clip);if(now[3]<0.1 || before[3]<0.1)continue;
                    for(unsigned k=0;k<3;++k)worst=std::max(worst,std::fabs(mapped[k]/mapped[3]-before[k]/before[3]));}}
            Expect(worst<1e-4,"a menu view's clip to previous clip maps the current clip to the previous one ("+std::to_string(worst)+")");}
        Expect(upscaler::MenuViewMotion(b,true,history,resolve,reset) && reset && motion_error(b,b)<0.01,"a cut resets: zero motion");
        auto bad=view;bad.vertical_fov=0;
        Expect(!upscaler::MenuViewMotion(bad,false,history,resolve,reset) && !history.valid,"a refused lens: no motion, the history dropped");
        Expect(upscaler::MenuViewMotion(b,false,history,resolve,reset) && reset,"the frame after a refused lens resets");}
    std::printf("%s upscaler: %d checks, %d failures (resolve %.2e px, rows %.2e px)\n",failures?"FAIL":"PASS",checks,failures,resolve_error,object_error);
    return failures?1:0;
}
