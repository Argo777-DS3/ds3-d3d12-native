// Scene lighting inputs: point and spot lights, local shadows, SSS profiles, env regions, light accumulation, the gparam
// records, light blocks and the forward light. Part of game_bootstrap.cpp's single translation unit (the order is there):
// it relies on every earlier part (06_pass_programmes.inc before it).
#pragma once
#include "06_pass_programmes.inc"
namespace {
// 1054 point lights (lighting map "Point/spot lights"). FCE30 keeps the lights of
// [[root]+41D0] (vector +10..+18) with +B0 set and +A0 & category (the main view: 1);
// for the point class (vtable 3D4E808) 10A220 calls its +30 = EFF50: a distance fade
// 1F1C60, then 212810 fills the frame item from the light and the view. Native
// transcription of both, without the old recorder: the view is the inverse camera
// pose; 1F1C60's plane (recorder+130) is taken as the view depth (inferred). The
// fade settings are 10A220's SettingInfo blend (15E920 on [root+4610] by the context
// keys +3FD8/+4040, ratio +40A8: +68..+70, flag +74) with [[root+5360]+40] (else 1).
// Spot lights (vtable 3D4E8E8): F1010's fade, then the genuine 2136B0 item (I-0106); the local shadows are CollectNativeLocalShadows'.
// 1F1C60 (the distance fade of one light for a view): the centre's depth on the view plane (the view's z row, inferred above),
// less the extent (a point light: 2 x radius, EFF50; a spot: light+0x1E4, F1010), against the light's own distance (+0xB8) or
// the settings' distance for that extent (+0x68 + extent x +0x6C, at most +0x70), scaled by the quality fade.
struct NativeLightFadeInputs {std::array<float,4> settings{};bool flag{};float one{},two{},fade_scale{},fade_limit{};};
float NativeLightFade(const NativeLightFadeInputs& in,float own_distance,float centre_depth,float extent) {
    float fade=in.one;
    if(in.flag || own_distance!=0.f){
        const float depth=centre_depth-extent;float distance=own_distance;
        if(distance==0.f){distance=extent*in.settings[1]+in.settings[0];if(in.settings[2]<=distance)distance=in.settings[2];}
        distance*=in.settings[3];
        const float width=in.fade_limit<=distance*in.fade_scale?10.f:distance*in.fade_scale,end=width+distance;
        if(distance<depth)fade=depth<end?(end-depth)/width:0.f;
    }
    return fade;
}
// The light container [[GXRS]+0x41D0] (vector +0x10..+0x18) and 10A220's fade inputs for the main view, shared by the point and
// spot lights and the local shadows' selection (I-0106); the pass record 212810 and 2136B0 read of 10A220's recorder ({+0x10
// the view, +0xD0 the camera world, +0x280 the metric scale 1.0 of the main view}, row SL9). False when a word is unreadable.
struct NativeLightWalk {alignas(16) std::array<uint8_t,0x290> pass{};   // first: no padding before it (aligned SSE reads)
    uintptr_t root{},first{},last{};NativeLightFadeInputs fade;std::array<float,4> weights{};float view[16]{};};
bool NativeLightWalkSetup(const NativeSelectedScene& scene,uintptr_t context,NativeLightWalk& w) {
    uintptr_t container{};
    if(!Read(image+0x4961B50,&w.root,8) || !w.root || !Read(w.root+0x41D0,&container,8) || !container ||
       !Read(container+0x10,&w.first,8) || !Read(container+0x18,&w.last,8) || w.last<w.first || w.last-w.first>8*4096)return false;
    float one{},two{},fade_scale{},fade_limit{};
    if(!Read(image+0x3D7EE10,&one,4) || !Read(image+0x3D7EE7C,&two,4) || !Read(image+0x3D7ED64,&fade_scale,4) ||
       !Read(image+0x3D7EF40,&fade_limit,4) || !Read(image+0x3D7F5F0,w.weights.data(),16))return false;
    std::array<float,4> settings{0,0,0,one};bool settings_flag{};
    {uintptr_t bank{};float ratio{};
        if(context && Read(w.root+0x4610,&bank,8) && bank && Read(context+0x40A8,&ratio,4)){
            const auto lookup=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uintptr_t)>(image+0x15E920);
            const uintptr_t a=lookup(bank,context+0x3FD8),b=lookup(bank,context+0x4040);
            std::array<float,3> fa{},fb{};BYTE flag_a{},flag_b{};
            if(a && b && Read(a+0x68,fa.data(),12) && Read(b+0x68,fb.data(),12) && Read(a+0x74,&flag_a,1) && Read(b+0x74,&flag_b,1)){
                for(size_t k=0;k<3;++k)settings[k]=(fb[k]-fa[k])*ratio+fa[k];
                settings_flag=(0.f<ratio?flag_b:flag_a)!=0;}
        }
        // The Lighting fade of the quality object (+0x40), from the scene's option snapshot (O2).
        if(scene.options && scene.options->object_valid)std::memcpy(&settings[3],scene.options->object.data()+0x40,4);
        // I-0129 R9c (the final review's M2): with the rays' local lights, at least HIGH's scale (MENUS-MAP 7787: 0.5 at DISABLE and
        // LOW, 0.75 MEDIUM, 1.0 HIGH, 1.2 MAX), so a low Lighting Quality no longer drops the lights the rays light and shadow.
        if(native_rt_scene.load(std::memory_order_relaxed) && renderer.rt_lights.load(std::memory_order_relaxed) && !renderer.RtSceneFailed() && !(settings[3]>=1.f))settings[3]=1.f;}
    ds3prod::InverseMatrix(scene.camera.camera_pose.data(),w.view);
    w.fade={settings,settings_flag,one,two,fade_scale,fade_limit};
    std::memcpy(w.pass.data()+0x10,w.view,64);std::memcpy(w.pass.data()+0xD0,scene.camera.camera_pose.data(),64);
    const float metric_scale=1.f;std::memcpy(w.pass.data()+0x280,&metric_scale,4);
    return true;
}
// F1010's fade of a spot light (1F1C60 on the centre, the class's +0x28 = light+0x180, over the extent light+0x1E4); 0 when unreadable.
float NativeSpotFade(const NativeLightWalk& w,uintptr_t light) {
    std::array<float,4> centre{};float own{},extent{};
    if(!Read(light+0x180,centre.data(),12) || !Read(light+0xB8,&own,4) || !Read(light+0x1E4,&extent,4))return 0.f;
    const float depth=centre[0]*w.view[2]+centre[1]*w.view[6]+centre[2]*w.view[10]+w.view[14];
    return NativeLightFade(w.fade,own,depth,extent);
}
void CollectNativePointLights(NativeSelectedScene& scene,uintptr_t context) {
    ffx_observe::Note(ffx_observe::LightCollect,0,context,0,0,0);   // W4 FW-1: the order against the effect update (FW-5)
    NativeLightWalk walk;if(!NativeLightWalkSetup(scene,context,walk))return;
    const uintptr_t first=walk.first,last=walk.last;const float* view=walk.view;const auto& fade_inputs=walk.fade;const auto& weights=walk.weights;
    const float one=fade_inputs.one,two=fade_inputs.two;const auto& settings=fade_inputs.settings;const bool settings_flag=fade_inputs.flag;
    const auto& pass=walk.pass;
    static const bool spots_enabled=!ModuleFlag(L"no-spot-lights.flag");
    size_t points{},spots{},faded{},spot_items{},spot_refused{};
    for(uintptr_t at=first;at<last;at+=8){
        uintptr_t light{},vtable{};BYTE enabled{};uint32_t mask{};
        if(!Read(at,&light,8) || !light || !Read(light,&vtable,8) || !Read(light+0xB0,&enabled,1) || !Read(light+0xA0,&mask,4) || !enabled || !(mask&1u))continue;
        if(vtable==image+0x3D4E8E8){++spots;
            if(!spots_enabled)continue;
            // F1010: the fade 1F1C60 of the centre over the extent (NativeSpotFade), then 2136B0's item.
            const float fade=NativeSpotFade(walk,light);
            if(!(0.f<fade)){++faded;continue;}
            alignas(16) std::array<uint8_t,0x280> item{};
            reinterpret_cast<void*(__fastcall*)(void*,uintptr_t,const void*,float)>(image+0x2136B0)(item.data(),light,pass.data(),fade);
            const auto U=[&](size_t offset){uint32_t value{};std::memcpy(&value,item.data()+offset,4);return value;};
            NativeSelectedScene::SpotLight spot;spot.type=U(0x1C);spot.light=light;
            // 1A1DA0 draws 0x67 and 0x6E through 1A6690 (the projected-mask types 0x69/0x70 have no tile path); a shadowed type
            // (0x68/0x6F, local shadows) is not produced here (no slot 3 ran on this item).
            if(spot.type!=0x67 && spot.type!=0x6E){++spot_refused;continue;}
            std::memcpy(&spot.near_z,item.data()+0x20,4);std::memcpy(&spot.far_z,item.data()+0x24,4);
            // 2144C0: item [+0x30, +0x60), {+0xE4, +0xE8, 0, +0x1C9 ? 0 : 1}, item [+0x60, +0xA0).
            std::memcpy(spot.param.data(),item.data()+0x30,0x30);std::memcpy(spot.param.data()+12,item.data()+0xE4,8);
            spot.param[14]=0.f;spot.param[15]=item[0x1C9]?0.f:one;std::memcpy(spot.param.data()+16,item.data()+0x60,0x40);
            uintptr_t hull_points{};std::memcpy(&hull_points,item.data()+0xF0,8);spot.hull.count=U(0xF8);
            std::memcpy(spot.hull.m,item.data()+0x100,64);
            if(!hull_points || spot.hull.count>8 || !Read(hull_points,spot.hull.points,16*spot.hull.count) ||
               !std::all_of(spot.param.begin(),spot.param.end(),[](float v){return std::isfinite(v);})){++spot_refused;continue;}
            spot.hull.valid=true;scene.spot_lights.push_back(spot);++spot_items;continue;}
        if(vtable!=image+0x3D4E808)continue;
        std::array<uint8_t,0x1A0> l{};if(!Read(light,l.data(),l.size()))continue;
        const auto F=[&](size_t offset){float value{};std::memcpy(&value,l.data()+offset,4);return value;};
        const float x=F(0x150),y=F(0x154),z=F(0x158),r=F(0x15C);
        float v[4]{};for(size_t c=0;c<4;++c)v[c]=x*view[c]+y*view[4+c]+z*view[8+c]+view[12+c];
        const float fade=NativeLightFade(fade_inputs,F(0xB8),v[2],r*two);
        if(!(0.f<fade)){++faded;continue;}
        NativeSelectedScene::PointLight item;item.light=light;
        const float scale=F(0x19C),k=F(0x7C),s=F(0x8C);
        item.param={v[0],v[1],v[2],1.f/(r*r),
            scale*k*F(0x70)*F(0x70)*fade,scale*k*F(0x74)*F(0x74)*fade,scale*k*F(0x78)*F(0x78)*fade,F(0x90),
            scale*s*F(0x80)*F(0x80)*fade,scale*s*F(0x84)*F(0x84)*fade,scale*s*F(0x88)*F(0x88)*fade,0.f,
            F(0x98),F(0x9C),0.f,0.f};
        item.param[11]=weights[1]*item.param[9]+weights[3]*(scale*s*s*fade)+weights[0]*item.param[8]+weights[2]*item.param[10];
        item.sphere={v[0],v[1],v[2],r};item.near_z=v[2]-r;item.world={x,y,z};
        uint32_t legacy{};std::memcpy(&legacy,l.data()+0xAC,4);item.legacy=legacy==1;
        scene.point_lights.push_back(item);++points;
    }
    static LONG64 witness{};const auto n=InterlockedIncrement64(&witness);
    // 1152 witness: the lights nearest the camera (kind, world distance, radius, diffuse colour/strength, scale),
    // for the bonfire/fire-orb lighting gap on the player.
    if(n==240 || n==1200){struct Near{float distance;int spot;float x,y,z,r,c[3],k,scale;BYTE enabled;uint32_t mask;};std::vector<Near> nearest;
        const float eye[3]{scene.camera.camera_pose[12],scene.camera.camera_pose[13],scene.camera.camera_pose[14]};
        for(uintptr_t at=first;at<last;at+=8){uintptr_t light{},vtable{};if(!Read(at,&light,8) || !light || !Read(light,&vtable,8))continue;
            std::array<uint8_t,0x1A0> l{};if(!Read(light,l.data(),l.size()))continue;
            const auto F=[&](size_t offset){float value{};std::memcpy(&value,l.data()+offset,4);return value;};
            Near item{};item.spot=vtable==image+0x3D4E8E8?1:vtable==image+0x3D4E808?0:-1;item.x=F(0x150);item.y=F(0x154);item.z=F(0x158);item.r=F(0x15C);
            item.c[0]=F(0x70);item.c[1]=F(0x74);item.c[2]=F(0x78);item.k=F(0x7C);item.scale=F(0x19C);item.enabled=l[0xB0];std::memcpy(&item.mask,l.data()+0xA0,4);
            item.distance=std::sqrt((item.x-eye[0])*(item.x-eye[0])+(item.y-eye[1])*(item.y-eye[1])+(item.z-eye[2])*(item.z-eye[2]));nearest.push_back(item);}
        std::sort(nearest.begin(),nearest.end(),[](const Near& a,const Near& b){return a.distance<b.distance;});
        for(size_t i=0;i<nearest.size() && i<8;++i){const auto& it=nearest[i];char row[360]{};
            sprintf_s(row,"{\"event\":\"native_light_near\",\"call\":%lld,\"rank\":%zu,\"kind\":%d,\"distance\":%g,\"radius\":%g,\"colour\":[%g,%g,%g],\"strength\":%g,\"scale\":%g,\"enabled\":%u,\"mask\":%u}\n",
                n,i,it.spot,it.distance,it.r,it.c[0],it.c[1],it.c[2],it.k,it.scale,it.enabled,it.mask);Log(row);}}
    if(n<=2 || (n%120==0 && n<=1800) || n%600==0){char row[320]{};sprintf_s(row,"{\"event\":\"native_point_lights\",\"lights\":%llu,\"points\":%zu,\"faded\":%zu,\"spots\":%zu,\"spot_items\":%zu,\"spots_dropped\":%zu,\"settings\":[%.4g,%.4g,%.4g,%.4g],\"flag\":%d}\n",
        static_cast<unsigned long long>((last-first)/8),points,faded,spots,spot_items,spot_refused,settings[0],settings[1],settings[2],settings[3],settings_flag?1:0);Log(row);}
    // I-0106 witness: the first spot items (type, depth range, the four first words, hull count), against the original's
    // 2136B0 output (the item rows of reference_spot_items when observed).
    if((n<=2 || n%600==0) && !scene.spot_lights.empty()){std::string row="{\"event\":\"native_spot_items\",\"call\":"+std::to_string(n)+",\"items\":[";
        for(size_t i=0;i<scene.spot_lights.size() && i<6;++i){const auto& s=scene.spot_lights[i];char cell[480]{};
            sprintf_s(cell,"%s{\"type\":\"%X\",\"near\":%.6g,\"far\":%.6g,\"hull\":%u,\"row0\":[%.6g,%.6g,%.6g,%.6g],\"row3\":[%.6g,%.6g,%.6g,%.6g]}",i?",":"",s.type,s.near_z,s.far_z,s.hull.count,
                s.param[0],s.param[1],s.param[2],s.param[3],s.param[12],s.param[13],s.param[14],s.param[15]);row+=cell;}
        row+="]}\n";Log(row.c_str());}
    // I-0106: the lights the local shadows took (CollectNativeLocalShadows, before the walks) are marked shadowed: their shadow words
    // from the frame item after slot 3 (212DB0 / 2144C0), which the rays read (RtShadowedOf: the strength, a point's near plane).
    // I-0131 R10a: no atlas rectangles any more, and a light is shadowed on its hit alone (it needed a face in the local atlas before);
    // a slot that chose the other texture of 2144C0 / 212DB0 (a spot's +0x264, a point's +0xB8: the container's frame counter, 0
    // natively) is refused.
    if(!scene.local_shadows.empty()){size_t applied{},refused{};
        for(const auto& s:scene.local_shadows){const auto* item=s.item.data();
            {uint32_t counter{};std::memcpy(&counter,item+(s.spot?0x264:0xB8),4);if(counter){++refused;continue;}}
            if(s.spot){const auto found=std::find_if(scene.spot_lights.begin(),scene.spot_lights.end(),[&](const auto& l){return l.light==s.light;});
                if(found==scene.spot_lights.end())continue;
                std::memcpy(found->shadow.data(),item+0xE0,4);
                std::memcpy(&found->type,item+0x1C,4);found->shadowed=true;++applied;}
            else{const auto found=std::find_if(scene.point_lights.begin(),scene.point_lights.end(),[&](const auto& l){return l.light==s.light;});
                if(found==scene.point_lights.end())continue;
                // 212DB0: item [+0x20, +0x50) and {+0x90, +0x94, 0, 0}, then +0x50 and +0x60 (the shadow words).
                std::memcpy(found->param.data(),item+0x20,0x30);std::memcpy(found->param.data()+12,item+0x90,8);found->param[14]=found->param[15]=0.f;
                std::memcpy(found->shadow.data(),item+0x50,32);
                found->shadowed=true;++applied;}}
        if(n<=2 || n%600==0 || refused){static LONG64 refusals{};if(!refused || InterlockedIncrement64(&refusals)<=20){char row[200]{};
            sprintf_s(row,"{\"event\":\"native_local_shadow_lights\",\"call\":%lld,\"shadowed\":%zu,\"applied\":%zu,\"refused\":%zu}\n",n,scene.local_shadows.size(),applied,refused);Log(row);}}}
}
// W5a SW-7 (SHADOWS-WAVE1-READY 3, rows SL1, SL8-SL11; I-0106: on by default since seen at Anor Londo and Central Irithyll,
// i106-look1, no-local-shadows.flag opts out): the local-light shadow selection
// of the main view on the engine's own container [GXRS+0x41D8] (built natively by 1400B24E0 in PublishNativeSceneRoot).
// SW-7a: 1400B27A0's CPU half once per container, on 140048B80's configuration block: spot tile D573B0[+0x20] (0x800), the
// spot level counts 2/4/6/10, point tile D573B0[+0x1C] (0x200), the point level counts 2/2/2/2, atlas edge D573B0[+0x24]
// (0x1000; the two tiles come from getters in protected code that returned their default arguments at the reference, SL11);
// the quad-tree allocator by the genuine 140242340 on {edge, edge >> 5}; ready +0x258 when it succeeded. 1400DCB80 (the GX
// atlas) is not run: the native local atlas is the renderer's (SW-7c). Each frame, 140023790's two statements: the genuine
// 1400B3B20 (empties the item vector, re-initialises the allocator levels) and the root mode byte +0x4E1A into +0x288.
// SW-7b: 10A220's selection on native items. Each enabled light of the category (as CollectNativePointLights) with a positive
// 1F1C60 fade gets its frame item from the genuine builder on a native pass record {+0x10 the view, +0xD0 the camera world
// matrix, +0x280 the metric scale 1.0 of the main view (row SL9)}: 140212810 (point, 0x120 bytes, extent 2r, centre +0x150)
// or 1402136B0 (spot, 0x280 bytes, extent light+0x1E4, centre light+0x180 = the class's +0x28); the items are stable-sorted on
// the unsigned +0x10 descending (140111820 with 1400FF3B0) and, under the gate context+0x78 == 1 && context+0x3EBF &&
// [[GXRS+0x5360]+0x44] (the context the native collect reads is 10A220's third argument, row SL1), each item's genuine slot 3
// (vtable +0x18: 140212C40 or 140214350) runs on the native pass record and the container. The result per shadowed light (its
// programme variant +0x1C, its tile rectangles, its frame item) is kept in the scene: the faces, the casters and the receivers.
std::mutex native_local_shadow_lock;
uintptr_t native_local_shadow_container{};   // the container configured (SW-7a); native_local_shadow_lock held
constexpr std::array<uint32_t,11> native_local_shadow_config{0x800,2,4,6,10,0x200,2,2,2,2,0x1000};
// I-0131 R10a (its step review's M1): whether the container items a slot-3 hit appended hold one face DS3's atlas could draw, the
// test I-0106 made before building the faces (the atlas is gone, the rule kept so that a traced frame shadows the same lights): the
// items after `before` (the vector's end before the hit, moved with its base), at most 48 across the frame (`seen`, the container's
// capacity), each with six face cameras (+0x378 == 6) or one; a face camera's world -> clip matrix (its pose inverted by 14004BA40's
// projection of its lens) finite and invertible with a finite corner volume, its tile (a point's six +0xBC rectangles, reduced by 3
// texels, widened back; a spot's +0x268) inside the atlas edge.
constexpr size_t native_local_shadow_items_max=48;
bool NativeLocalHitHasFace(uintptr_t container,uintptr_t before,uintptr_t base_before,bool spot,const uint8_t* item,size_t& seen) {
    uintptr_t base_now{},after{};Read(container+0x260,&base_now,8);Read(container+0x268,&after,8);
    const uintptr_t first_new=before>=base_before?base_now+(before-base_before):after;
    std::array<int32_t,24> tiles{};
    if(spot)std::memcpy(tiles.data(),item+0x268,16);
    else for(int k=0;k<6;++k){int32_t r[4]{};std::memcpy(r,item+0xBC+16*k,16);tiles[4*k]=r[0]-3;tiles[4*k+1]=r[1]-3;tiles[4*k+2]=r[2]+3;tiles[4*k+3]=r[3]+3;}
    const int32_t edge=int32_t(native_local_shadow_config[10]);bool found=false;
    for(uintptr_t at=first_new;after>at && at+0x1850<=after;at+=0x1850){std::array<uint8_t,0x380> head{};if(!Read(at,head.data(),head.size()))break;
        if(seen++>=native_local_shadow_items_max)continue;
        int32_t index{};std::memcpy(&index,head.data()+0x378,4);const int faces=index==6?6:1;
        for(int k=0;k<faces && !found;++k){const uint8_t* camera=head.data()+0x90*k;
            alignas(16) float pose[16]{},view_matrix[16]{},projection[16]{};std::memcpy(pose,camera,64);ds3prod::InverseMatrix(pose,view_matrix);
            float lens[6]{};std::memcpy(lens,camera+0x50,24);const float aspect=lens[5]!=0.f?lens[5]:1.f;
            reinterpret_cast<void(__fastcall*)(float*,float,float,float,float)>(image+0x4BA40)(projection,lens[0],aspect,lens[1],lens[2]);
            VelocityMatrix dm{},inverse{};bool finite=true;
            for(int r=0;r<4;++r)for(int c=0;c<4;++c){float v{};for(int m=0;m<4;++m)v+=view_matrix[4*r+m]*projection[4*m+c];dm[4*r+c]=v;finite=finite && std::isfinite(v);}
            const int tile=index==6?k:(index>=0 && index<6?index:0);const int32_t* rect=tiles.data()+4*tile;
            finite=finite && VelocityInverse(dm,inverse) && rect[2]>rect[0] && rect[3]>rect[1] && rect[0]>=0 && rect[1]>=0 && rect[2]<=edge && rect[3]<=edge;
            for(int corner=0;corner<8 && finite;++corner){const double p[4]{corner&1?1.0:-1.0,corner&2?1.0:-1.0,corner&4?1.0:0.0,1.0};double w[4]{};
                for(int j=0;j<4;++j)for(int m=0;m<4;++m)w[j]+=p[m]*inverse[4*m+j];
                if(!(std::abs(w[3])>1e-9)){finite=false;break;}
                for(int c=0;c<3;++c)if(!std::isfinite(w[c]/w[3]))finite=false;}
            found=finite;}}
    return found;
}
void CollectNativeLocalShadows(NativeSelectedScene& scene,uintptr_t context) {
    static const bool enabled=!ModuleFlag(L"no-local-shadows.flag");
    // Called only from the main view's collect, before its shadow walks (I-0106: the local walk takes the items' boxes): a menu
    // view must never reset the engine container. FCE30 walks the lights only when context+0x3EBE is set (843C0's flag,
    // transcribed).
    uint8_t walk_gate{};
    if(!enabled || !context || !Read(context+0x3EBE,&walk_gate,1) || !walk_gate)return;
    NativeLightWalk walk;if(!NativeLightWalkSetup(scene,context,walk))return;
    const uintptr_t root=walk.root,first=walk.first,last=walk.last;const auto& fade=walk.fade;const float* view=walk.view;const auto& pass=walk.pass;
    int32_t pipeline{};uint8_t variant{},mode{};uintptr_t container{};
    if(!Read(context+0x78,&pipeline,4) || !Read(context+0x3EBF,&variant,1) || pipeline!=1)return;   // the world pipeline only
    if(!Read(root+0x41D8,&container,8) || !container || !Read(root+0x4E1A,&mode,1))return;
    const uint8_t lighting=scene.options && scene.options->object_valid?scene.options->object[0x44]:0;
    struct Item {alignas(16) std::array<uint8_t,0x280> bytes{};uintptr_t light{};bool spot{};};
    std::vector<std::unique_ptr<Item>> items;
    // FCE30's light visibility: its visitor (vtable 143D622B0, slot 140222FF0) keeps a light whose own test (vtable +0x18:
    // point 1400EFF10 = the frustum's +0x50 on the sphere +0x150, spot 1400F0FD0 = the frustum's +0x38 on the shape +0x220) passes
    // on the view's frustum record, built by the genuine 14006E780 and 14006E9E0 from the CED670 camera output (the same bytes as
    // the scene's camera, checked). I-0106: the spot items are the genuine 2136B0's on the native pass record (the cone state is
    // the loader's, RESULT step 1; Native1509's « wrong keys » compared different lights before this visibility rule existed).
    // Open (review of W5): 222FF0 also requires light+0x60 & visitor+0x18 and visitor+0x1C & light+0x64 and stops at the
    // visitor's capacity +0x40, and FCE30 adds its 0x39 / quality+0x6D condition; none is reproduced here (the masks are logged),
    // so the native side can keep lights the engine drops. A kept/items difference in SL9b is that gap before a builder fault.
    alignas(16) std::array<uint8_t,0x300> frustum{};bool culled{};
    std::array<uint8_t,0x88> camera_bytes{};
    {std::optional<std::array<uint8_t,0x88>> camera;{const std::lock_guard<std::recursive_mutex> hold(scene_view_cpu_lock);camera=native_scene_camera_bytes;}
        if(camera)camera_bytes=*camera;
        if(camera && !std::memcmp(camera->data(),scene.camera.camera_pose.data(),64)){
            reinterpret_cast<void*(__fastcall*)(void*)>(image+0x6E780)(frustum.data());
            reinterpret_cast<void(__fastcall*)(void*,const void*)>(image+0x6E9E0)(frustum.data(),camera->data());culled=true;}}
    size_t spots{},invisible{};
    static volatile LONG64 visibility_calls{};const auto visibility_n=InterlockedIncrement64(&visibility_calls);
    const bool sample_visibility=visibility_n<=3 || visibility_n%600==0;bool record_logged{};
    for(uintptr_t at=first;at<last && culled;at+=8){
        uintptr_t light{},vtable{};BYTE on{};uint32_t mask{};
        if(!Read(at,&light,8) || !light || !Read(light,&vtable,8) || !Read(light+0xB0,&on,1) || !Read(light+0xA0,&mask,4) || !on || !(mask&1u))continue;
        const bool spot=vtable==image+0x3D4E8E8;
        if(!spot && vtable!=image+0x3D4E808)continue;
        {static const bool no_spots=ModuleFlag(L"no-spot-lights.flag");if(spot && no_spots)continue;}   // the spots' opt-out holds their shadows too
        spots+=spot;
        uintptr_t test{};
        if(!Read(vtable+0x18,&test,8) || test!=image+(spot?0xF0FD0:0xEFF10))continue;
        const auto visible=reinterpret_cast<int32_t(__fastcall*)(uintptr_t,void*)>(test)(light,frustum.data());
        if(sample_visibility){   // row SL9b's native side: every light of the first three collects and of every 600th
            std::array<float,4> sphere{};uint32_t masks[2]{};Read(light+(spot?0x180:0x150),sphere.data(),16);Read(light+0x60,masks,8);
            char row[320]{};sprintf_s(row,"{\"event\":\"native_light_visibility\",\"light\":%llu,\"spot\":%d,\"sphere\":[%.7g,%.7g,%.7g,%.7g],\"masks\":[%u,%u],\"kept\":%d",
                static_cast<unsigned long long>(light),spot?1:0,sphere[0],sphere[1],sphere[2],sphere[3],masks[0],masks[1],visible?1:0);
            std::string text=row;
            if(!record_logged){record_logged=true;text+=",\"camera\":\""+HexBytes(camera_bytes.data(),camera_bytes.size())+"\",\"record\":\""+HexBytes(frustum.data(),frustum.size())+"\"";}
            text+="}\n";Log(text.c_str());}
        if(!visible){++invisible;continue;}
        float f{};
        if(spot)f=NativeSpotFade(walk,light);
        else{std::array<float,4> centre{};float own{},extent{};
            if(!Read(light+0x150,centre.data(),12) || !Read(light+0xB8,&own,4) || !Read(light+0x15C,&extent,4))continue;
            extent*=fade.two;
            const float depth=centre[0]*view[2]+centre[1]*view[6]+centre[2]*view[10]+view[14];
            f=NativeLightFade(fade,own,depth,extent);}
        if(!(0.f<f))continue;
        auto item=std::make_unique<Item>();item->light=light;item->spot=spot;
        reinterpret_cast<void*(__fastcall*)(void*,uintptr_t,const void*,float)>(image+(spot?0x2136B0:0x212810))(item->bytes.data(),light,pass.data(),f);
        items.push_back(std::move(item));
    }
    const auto key=[](const Item& i){uint32_t value{};std::memcpy(&value,i.bytes.data()+0x10,4);return value;};
    std::stable_sort(items.begin(),items.end(),[&](const auto& a,const auto& b){return key(*a)>key(*b);});
    // I-0129 R9c: with the rays' local lights this collect (`local_traced`: they trace the shadowed lights' shadows)
    // the lights a low Lighting Quality leaves unshadowed in the original are marked shadowed all the same.
    const bool gate=variant && (lighting || scene.local_traced);
    size_t hits{},refused{},bad_faces{},items_seen{};uint8_t ready{};
    std::vector<NativeSelectedScene::LocalShadow> shadowed;
    {const std::lock_guard hold(native_local_shadow_lock);
        if(native_local_shadow_container!=container){
            uintptr_t nodes{};Read(container+0x18,&nodes,8);bool allocated=nodes!=0;
            if(!allocated){const uint32_t extent[2]{native_local_shadow_config[10],native_local_shadow_config[10]>>5};
                allocated=(reinterpret_cast<uint64_t(__fastcall*)(uintptr_t,const uint32_t*)>(image+0x242340)(container+0x10,extent)&0xFF)!=0;}
            *reinterpret_cast<uint8_t*>(container+0x258)=allocated?1:0;
            std::memcpy(reinterpret_cast<void*>(container+0x28C),native_local_shadow_config.data(),sizeof native_local_shadow_config);
            native_local_shadow_container=container;
            uint32_t levels{},capacity{};Read(container+0x24,&levels,4);Read(container+0x280,&capacity,4);
            char row[224]{};sprintf_s(row,"{\"event\":\"native_local_shadow_container\",\"allocated\":%s,\"levels\":%u,\"capacity\":%u,\"edge\":%u,\"spot_tile\":%u,\"point_tile\":%u}\n",
                allocated?"true":"false",levels,capacity,native_local_shadow_config[10],native_local_shadow_config[0],native_local_shadow_config[5]);Log(row);}
        reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0xB3B20)(container);
        *reinterpret_cast<uint8_t*>(container+0x288)=mode;Read(container+0x258,&ready,1);
        if(gate && ready)for(const auto& item:items){
            uintptr_t vtable{},slot{};std::memcpy(&vtable,item->bytes.data(),8);
            if(!Read(vtable+0x18,&slot,8) || slot!=image+(item->spot?0x214350:0x212C40)){++refused;continue;}
            uintptr_t before{},base_before{};Read(container+0x268,&before,8);Read(container+0x260,&base_before,8);   // a hit appends its items at the end
            // 140214350 returns nothing: a shadowed light is one whose map object (+0xB0 / +0x1C0) the slot set.
            reinterpret_cast<void(__fastcall*)(void*,const void*,uintptr_t)>(slot)(item->bytes.data(),pass.data(),container);
            NativeSelectedScene::LocalShadow s{};std::memcpy(&s.container_item,item->bytes.data()+(item->spot?0x1C0:0xB0),8);
            if(!s.container_item)continue;
            // I-0131 R10a (its step review's M1): a hit is a shadowed light only with a face DS3's atlas could draw, as before the
            // atlas's removal (I-0106's faces: the container items this hit appended, capacity 48, each face camera's world -> clip
            // matrix finite and invertible, its tile inside the atlas edge); without one the light stays unshadowed, counted.
            if(!NativeLocalHitHasFace(container,before,base_before,item->spot,item->bytes.data(),items_seen)){++bad_faces;continue;}
            ++hits;s.spot=item->spot;s.light=item->light;s.item=item->bytes;
            std::memcpy(&s.variant,item->bytes.data()+0x1C,4);std::memcpy(&s.order_key,item->bytes.data()+0x10,4);
            std::memcpy(&s.metric,item->bytes.data()+(item->spot?0x260:0x9C),4);
            shadowed.push_back(s);}}
    // I-0131 R10a: the selection only (its lights' shadow words for the rays); the faces, the items' boxes and the local atlas are removed.
    scene.local_shadows=std::move(shadowed);
    static LONG64 calls{};static size_t last_hits{~size_t{}};const auto n=InterlockedIncrement64(&calls);
    if(n<=2 || n%600==0 || hits!=last_hits){last_hits=hits;
        char text[640]{};sprintf_s(text,"{\"event\":\"native_local_shadow\",\"call\":%lld,\"items\":%zu,\"spots\":%zu,\"invisible\":%zu,\"culled\":%d,\"gate\":[%d,%u,%u],\"mode\":%u,\"hits\":%zu,\"refused\":%zu,\"bad_faces\":%zu,\"shadowed\":[",
            static_cast<long long>(n),items.size(),spots,invisible,culled?1:0,pipeline,variant,lighting,mode,hits,refused,bad_faces);
        std::string row=text;
        for(size_t i=0;i<scene.local_shadows.size() && i<8;++i){const auto& s=scene.local_shadows[i];std::array<float,4> position{},words{};
            Read(s.light+(s.spot?0x180:0x150),position.data(),16);std::memcpy(words.data(),s.item.data()+0x50,16);
            char cell[800]{};sprintf_s(cell,"%s{\"spot\":%d,\"position\":[%g,%g,%g,%g],\"order\":%u,\"metric\":%.8g,\"variant\":\"%X\",\"words\":[%.8g,%.8g,%.8g,%.8g]}",
                i?",":"",s.spot?1:0,position[0],position[1],position[2],position[3],s.order_key,s.metric,s.variant,words[0],words[1],words[2],words[3]);row+=cell;}
        row+="]}\n";Log(row.c_str());}
}
// 1049 cbLightAccParam (19F640/10ED10 assembly, I-0044 LightAccParamCommon) from the
// current camera: floats 0 transpose(inverse(projection)), 16 view columns x/y/z,
// 28 {W, H, 1/W, 1/H}, 32 transpose(camera pose), 52 transpose(projection); the
// spec-power word (cam+9CC) and the light-count visualization stay 0 (unread by
// the programmes run here).
// 1141 native SSS profiles (I-0067 TONEMAP-SKIN sections 3 and 7). Natively the engine's GX light manager and its
// profile manager [1449622F8] are never created (Native1138: SSS object and vector null; Native1140: no material
// has the 22BF70 link, source+370 null), so the native backend owns the profile registry, transcribing 113F80,
// 113D80, 22BF70, 112C60 and 1141B0:
// - entry 0 is 113D80's default profile (strength and width immediates at 140113DD5/DDC/DE3 and 140113DC4);
// - a material whose MTD carries the SSS parameter block (g_SSSWidth) and whose FLVER GX list has a GX15 record gets
//   FC_SSS_ID: with g_SSSStrength_UseCAParam the linked instance {strength = GX15 words +0C/+10/+14, radius = 112C60
//   along the link = the MTD base width}, otherwise the MTD base {g_SSSStrength, g_SSSWidth} (I-0067 section 8);
// - equal profiles share one entry (112C90: equal radius and strength), entries keep their creation order (at most
//   0xE3, the record array of 114A50); the entry index is the material's FC_SSS_ID word and its cbSSS slot.
// Open: g_SSSStrength_UseCAParam (strength from the character's extra parameters) is not read.
struct NativeSssProfile {std::array<float,3> strength{};float radius{};std::array<float,68> kernel{};};
std::mutex native_sss_lock;
std::vector<NativeSssProfile> native_sss_profiles;
// Entry 0, 113D80s default profile; native_sss_lock held.
bool NativeSssDefault() {
    if(native_sss_profiles.empty()){
        NativeSssProfile fallback{};uint32_t words[4]{};
        if(!Read(image+0x113DD5,&words[0],4) || !Read(image+0x113DDC,&words[1],4) || !Read(image+0x113DE3,&words[2],4) || !Read(image+0x113DC4,&words[3],4))return false;
        std::memcpy(fallback.strength.data(),words,12);std::memcpy(&fallback.radius,&words[3],4);
        float kernel[17][4]{};if(!ds3light::SssProfileKernel(fallback.strength.data(),kernel))return false;
        std::memcpy(fallback.kernel.data(),kernel,sizeof(kernel));native_sss_profiles.push_back(fallback);}
    return true;
}
bool NativeSssEntry(const std::array<float,3>& strength,float radius,uint32_t& index) {
    const std::lock_guard hold(native_sss_lock);
    if(!NativeSssDefault())return false;
    for(uint32_t i=0;i<native_sss_profiles.size();++i)
        if(native_sss_profiles[i].radius==radius && native_sss_profiles[i].strength==strength){index=i;return true;}
    if(native_sss_profiles.size()>=0xE3)return false;
    NativeSssProfile entry{};entry.strength=strength;entry.radius=radius;
    float kernel[17][4]{};if(!ds3light::SssProfileKernel(entry.strength.data(),kernel))return false;
    std::memcpy(entry.kernel.data(),kernel,sizeof(kernel));
    index=static_cast<uint32_t>(native_sss_profiles.size());native_sss_profiles.push_back(entry);
    char row[256]{};sprintf_s(row,"{\"event\":\"native_sss_profile_created\",\"index\":%u,\"strength\":[%g,%g,%g],\"radius\":%g,\"kernel0\":[%g,%g,%g]}\n",
        index,strength[0],strength[1],strength[2],radius,kernel[0][0],kernel[0][1],kernel[0][2]);Log(row);
    return true;
}
// The FC_SSS_ID profile of a material: nullopt when 22BF70 would not link it (no SSS MTD block or no GX15).
std::optional<uint32_t> NativeSssProfileOf(const NativeModelMaterial& material) {
    if(!material.authored)return std::nullopt;
    const auto& parameters=material.authored->parameters;
    const auto find=[&](const char* name){return std::find_if(parameters.begin(),parameters.end(),[&](const ds3prod::MtdParam& p){return p.name==name;});};
    const auto width=find("g_SSSWidth"),base_strength=find("g_SSSStrength"),use_ca=find("g_SSSStrength_UseCAParam");
    if(width==parameters.end() || width->values.empty())return std::nullopt;
    const auto& gx=material.gx_records;std::optional<std::array<float,3>> strength;
    for(size_t at=0,count=0;count<65536 && at+12<=gx.size();++count){
        uint32_t key{};int32_t length{};std::memcpy(&key,gx.data()+at,4);std::memcpy(&length,gx.data()+at+8,4);
        if(key==0x7FFFFFFF || length<12 || static_cast<size_t>(length)>gx.size()-at)break;
        if(key==0x35315847 && length>=24){std::array<float,3> s{};std::memcpy(s.data(),gx.data()+at+12,12);strength=s;break;}
        at+=static_cast<size_t>(length);}
    if(!strength)return std::nullopt;   // no GX15: 22BF70 is not reached, FC_SSS_ID stays unwritten
    // FC_SSS_ID's profile: the instance when the block's query (+18, g_SSSStrength_UseCAParam) holds, else the base.
    const bool instance=use_ca!=parameters.end() && !use_ca->values.empty() && use_ca->values[0]!=0.0;
    if(!instance){if(base_strength==parameters.end() || base_strength->values.size()<3)return std::nullopt;
        strength=std::array<float,3>{static_cast<float>(base_strength->values[0]),static_cast<float>(base_strength->values[1]),static_cast<float>(base_strength->values[2])};}
    uint32_t index{};if(!NativeSssEntry(*strength,static_cast<float>(width->values[0]),index))return std::nullopt;
    return index;
}
// cbSSS (1141B0): per entry {radius * 1/tanf(fov * 0.5), 0, 0, 0, kernel[17]} in 0x120 bytes; cbSceneParam words [0,48)
// with FC_DepthComputeParam (32). The default entry always exists once the registry is used.
bool ProduceNativeSssInputs(const SceneViewInput& camera,const ingress::SceneCameraConstants& constants,std::vector<uint8_t>& profiles,std::vector<uint8_t>& scene_words) {
    for(uint32_t offset=32;offset<48;offset+=4)if(constants.written[offset/4]!=1)return false;
    if(!(camera.vertical_fov>0.f) || !(camera.vertical_fov<3.1f))return false;
    const float scale=1.f/std::tan(camera.vertical_fov*0.5f);
    {const std::lock_guard hold(native_sss_lock);if(!NativeSssDefault())return false;
        profiles.assign(native_sss_profiles.size()*0x120,0);
        for(size_t i=0;i<native_sss_profiles.size();++i){const auto& p=native_sss_profiles[i];const float width=p.radius*scale;
            std::memcpy(profiles.data()+i*0x120,&width,4);std::memcpy(profiles.data()+i*0x120+0x10,p.kernel.data(),0x110);}}
    scene_words.assign(constants.bytes.begin(),constants.bytes.begin()+48);
    static LONG64 witness{};const auto n=InterlockedIncrement64(&witness);
    if(n<=2 || n%600==0){float first[8]{};std::memcpy(first,profiles.data(),32);float depth[4]{};std::memcpy(depth,scene_words.data()+32,16);char row[320]{};
        sprintf_s(row,"{\"event\":\"native_sss_inputs\",\"entries\":%zu,\"fov\":%g,\"record0\":[%g,%g,%g,%g,%g,%g,%g,%g],\"depth_compute\":[%g,%g,%g,%g]}\n",
            profiles.size()/0x120,camera.vertical_fov,first[0],first[1],first[2],first[3],first[4],first[5],first[6],first[7],depth[0],depth[1],depth[2],depth[3]);Log(row);}
    return true;
}
// The IBL cube 416A0 gives the env-map key at `key` ({area, id}: a region record's first words) and its entry's colour scale +0xC:
// the entry's +0x40, else (1148: 14006CED0's binding, absent natively) the admitted TPF cube named "%s%04d" (prefix +10, id +8).
uintptr_t NativeEnvMapCube(uintptr_t table,uintptr_t key,float& scale) {
    const auto lookup=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uintptr_t)>(image+0x416A0);
    uint32_t words[2]{};if(!Read(key,words,8))return 0;
    uintptr_t cube{};const auto entry=lookup(table,key);if(!entry || !Read(entry+0xC,&scale,4) || !Read(entry+0x40,&cube,8))cube=0;
    if(entry && !cube){int32_t id{};uint64_t length{},capacity{};
        if(Read(entry+8,&id,4) && Read(entry+0x20,&length,8) && Read(entry+0x28,&capacity,8) && length<64){
            std::wstring prefix(static_cast<size_t>(length),L'\0');uintptr_t chars=entry+0x10;
            if(capacity>7 && !Read(entry+0x10,&chars,8))chars=0;
            if(chars && (!length || Read(chars,prefix.data(),static_cast<size_t>(length)*2))){
                char name[96]{};sprintf_s(name,"%ls%04d",prefix.c_str(),id);
                // Natively 416A0 only holds its default entry (id 0, "EnvMap_", Native1150) while the Envmap TPF cubes are named
                // m<aa>_<bb>_GILM<id>, one per region id of that area (INFERRED from the admitted names, Native1150): the key's own
                // {area, id} names its cube.
                if(id==0 && prefix==L"EnvMap_")sprintf_s(name,"m%02u_%02u_GILM%04d",(words[0]/1000000u)%100u,(words[0]/10000u)%100u,static_cast<int32_t>(words[1]));
                const std::lock_guard hold(native_cube_name_lock);const auto found=native_cube_names.find(name);
                if(found!=native_cube_names.end())cube=found->second;
                static LONG64 names_logged{};if(InterlockedIncrement64(&names_logged)<=40){char row[256]{};
                    sprintf_s(row,"{\"event\":\"native_env_cube_name\",\"area\":%u,\"id\":%d,\"name\":\"%.80s\",\"found\":%s,\"admitted_cubes\":%zu}\n",
                        words[0],static_cast<int32_t>(words[1]),name,found!=native_cube_names.end()?"true":"false",native_cube_names.size());Log(row);}}}}
    return cube;
}
// 4E2A0's scale for an env-map key: its 416A0 entry's +0xC, the gparam « Env Map » ColorScale of the key (IMAGE-REGRESSION-STATIC-
// LEADS L2). Natively 416A0 holds only its default entry (1.0), so the value comes from the admitted gparam files with 416A0's
// rule: {-1, -1} reads {1, 0}, and a key whose area or id the table lacks reads {1, 0}, default.gparam's id 0 (1.5): the original's
// scales of the groups' own keys {0, -1} were 1.5 on every row (OBSERVED, i113-t14-val-original). 1 when no file answers.
float NativeEnvMapScale(const uint32_t* key) {
    std::shared_ptr<const ds3light::Banks> banks;{const std::lock_guard hold(gparam_lock);banks=gparam_banks;}
    float scale=1.f;if(!banks)return scale;
    const bool unset=key[0]==0xFFFFFFFFu && key[1]==0xFFFFFFFFu;
    if(unset || !banks->EnvColorScale(key[0],key[1],scale))banks->EnvColorScale(1,0,scale);
    return scale;
}
// I-0113 tranche 14 (a world object's region IBL, RESULT « Tranche 14 »): the region manager root+0x42B0 as 5C0E0 reads it, copied
// once per main collect while the manager is enabled (+0x10; 85540 sets the scene context's +0x3ECF from it and the view policy's
// bit 27, which the main view carries: OBSERVED, i113-t14-al-original): the regions of its list +0x38..+0x40 in order, each with
// the masks of the handle 69940 finds for it (the last of the handles +0x18..+0x20 that resolves to it: its index +8 not above the
// table count +0x60 and its generation +0xA that of the table +0x58's 0x18-byte entry {region, masks +8 and +0xC, generation
// +0x10}) and the cube 75B30 binds for its key (416A0's entry +0x40, NativeEnvMapCube); the default handle +0x70's region (the
// record +0x80 when it does not resolve); the scale of the key {0, -1}, the groups' own keys whose entries 4E2A0 reads the scales
// from (OBSERVED: every sampled group; NativeEnvMapScale). uid changes with any of it (the models' choices are kept while it stays).
struct NativeRegionSnapshot {
    // NativeEnvMapCube's answer for a record: the engine cube, the deferred regions' colour scale (its native 416A0 entry's +0xC, 1
    // without an entry), and its native cube record, resident or not yet (a forward row waits for it, D-0197; the deferred regions
    // pass takes it once resident).
    struct Cube {uintptr_t engine{};float scale{1.f};std::shared_ptr<const bootstrap::NativeTextureRecord> record;
        bool Ready() const noexcept {return record && record->resident && record->resource && record->view && record->description.cube;}};
    std::vector<ds3native::env_region::Region> listed;std::vector<uintptr_t> records;std::vector<Cube> cubes;
    ds3native::env_region::Region fallback;Cube fallback_cube;uintptr_t fallback_record{};
    float default_scale{1.f};uint64_t uid{};   // default_scale: NativeEnvMapScale({0, -1}), every group's own keys so far
    uint64_t textures_version{},names_generation{},binding_generation{};std::shared_ptr<const ds3light::Banks> banks;   // what the lookups depended on (kept while they stay)
    uint64_t record_generation{};   // banks' RecordGeneration at this collect: the own keys' scales of the models' memos (09)
};
// 1147 env-map regions: ProduceNativeEnvRegions (the region records into the lighting input) is removed by I-0132 R11e (its
// step review M3): the region cubes were read only by the rays' fallback on a frame without a top level, itself removed.
std::shared_ptr<const NativeRegionSnapshot> CollectNativeRegionSnapshot() {
    uintptr_t root{},table{};std::array<uint8_t,0x88> m{};
    if(!Read(image+0x4961B50,&root,8) || !root || !Read(root+0x42B0,m.data(),m.size()) || !m[0x10] || !Read(root+0x42A8,&table,8) || !table){const std::lock_guard<std::recursive_mutex> hold(scene_view_cpu_lock);native_region_snapshot_last.reset();return nullptr;}
    const auto P=[&](uint32_t at){uintptr_t v{};std::memcpy(&v,m.data()+at,8);return v;};
    const uintptr_t manager=root+0x42B0,handles=P(0x18),handles_end=P(0x20),list=P(0x38),list_end=P(0x40),entries=P(0x58);
    uint32_t count{};std::memcpy(&count,m.data()+0x60,4);
    if(handles_end<handles || (handles_end-handles)/8>4096 || list_end<list || (list_end-list)/8>4096){const std::lock_guard<std::recursive_mutex> hold(scene_view_cpu_lock);native_region_snapshot_last.reset();return nullptr;}
    const auto resolve=[&](uintptr_t handle)->uintptr_t{int32_t id{};uint16_t generation{},current{};uintptr_t region{};
        if(!Read(handle+8,&id,4) || id==-1)return manager+0x80;
        const uint16_t index=static_cast<uint16_t>(id);
        if(count<index || !Read(handle+0xA,&generation,2) || !Read(entries+0x10+0x18ull*index,&current,2) || generation!=current ||
           !Read(entries+0x18ull*index,&region,8))return manager+0x80;
        return region;};
    std::vector<std::pair<uintptr_t,uint16_t>> resolved;   // each handle's region and table index
    for(uintptr_t at=handles;at<handles_end;at+=8){uintptr_t handle{};int32_t id{};
        if(Read(at,&handle,8) && handle && Read(handle+8,&id,4))resolved.push_back({resolve(handle),static_cast<uint16_t>(id)});}
    auto snapshot=std::make_shared<NativeRegionSnapshot>();uint64_t h=1469598103934665603ull;
    const auto mix=[&](const void* data,size_t bytes){const auto* p=static_cast<const uint8_t*>(data);for(size_t i=0;i<bytes;++i){h^=p[i];h*=1099511628211ull;}};
    const auto mix_region=[&](const ds3native::env_region::Region& r){mix(r.key,8);mix(r.box,80);mix(&r.fade,4);mix(&r.priority,4);mix(r.masks,8);mix(&r.handle,1);};
    const auto fill=[&](uintptr_t record,ds3native::env_region::Region& region){std::array<uint8_t,0xD4> r{};
        if(!Read(record,r.data(),r.size()))return false;
        std::memcpy(region.key,r.data(),8);std::memcpy(region.box,r.data()+0x10,80);std::memcpy(&region.fade,r.data()+0xCC,4);std::memcpy(&region.priority,r.data()+0xD0,4);
        return true;};
    // A record whose key is the previous collect's keeps its cube when that cube was ready (and not withdrawn since), or, not found
    // or not resident yet, while the texture ingress has not moved: the steady state makes no lookup (main collect only, on the
    // engine thread). A key whose id is -1 binds the REM (75B30), absent natively: no lookup.
    std::shared_ptr<const NativeRegionSnapshot> previous;
    {const std::lock_guard<std::recursive_mutex> hold(scene_view_cpu_lock);previous=std::static_pointer_cast<const NativeRegionSnapshot>(native_region_snapshot_last);}
    const uint64_t textures_version=textures.Version(),names_generation=native_cube_names_generation.load(std::memory_order_acquire);
    const auto cube_of=[&](uintptr_t record,const ds3native::env_region::Region& region){
        if(region.key[1]==0xFFFFFFFFu)return NativeRegionSnapshot::Cube{};
        if(previous){const auto& p=*previous;
            const auto kept=[&](const ds3native::env_region::Region& was,const NativeRegionSnapshot::Cube& cube){
                if(was.key[0]!=region.key[0] || was.key[1]!=region.key[1])return false;
                return cube.Ready()?!cube.record->withdrawn.load(std::memory_order_acquire):p.textures_version==textures_version && p.names_generation==names_generation;};
            if(record==p.fallback_record && kept(p.fallback,p.fallback_cube))return p.fallback_cube;
            for(size_t i=0;i<p.records.size();++i)if(p.records[i]==record){if(kept(p.listed[i],p.cubes[i]))return p.cubes[i];break;}}
        NativeRegionSnapshot::Cube cube;cube.engine=NativeEnvMapCube(table,record,cube.scale);
        if(cube.engine)if(auto native=textures.Acquire(cube.engine);native && native->description.cube)cube.record=std::move(native);
        return cube;};
    const auto mix_cube=[&](const NativeRegionSnapshot::Cube& cube){const void* c=cube.record.get();mix(&c,sizeof c);const bool ready=cube.Ready();mix(&ready,1);};
    for(uintptr_t at=list;at<list_end;at+=8){uintptr_t record{};ds3native::env_region::Region region;
        if(!Read(at,&record,8) || !record || !fill(record,region))continue;
        for(auto j=resolved.rbegin();j!=resolved.rend();++j)if(j->first==record){
            region.handle=Read(entries+0x18ull*j->second+8,region.masks,8);break;}
        auto cube=cube_of(record,region);
        mix_region(region);mix_cube(cube);
        snapshot->listed.push_back(region);snapshot->records.push_back(record);snapshot->cubes.push_back(std::move(cube));}
    const uintptr_t fallback=resolve(manager+0x70);
    if(!fill(fallback,snapshot->fallback)){const std::lock_guard<std::recursive_mutex> hold(scene_view_cpu_lock);native_region_snapshot_last.reset();return nullptr;}
    snapshot->fallback_record=fallback;snapshot->fallback_cube=cube_of(fallback,snapshot->fallback);
    {std::shared_ptr<const ds3light::Banks> banks;{const std::lock_guard hold(gparam_lock);banks=gparam_banks;}
        snapshot->banks=banks;snapshot->binding_generation=banks?banks->BindingGeneration():0;snapshot->record_generation=banks?banks->RecordGeneration():0;snapshot->textures_version=textures_version;snapshot->names_generation=names_generation;
        if(previous && previous->banks==snapshot->banks && previous->binding_generation==snapshot->binding_generation)snapshot->default_scale=previous->default_scale;
        else{const uint32_t own[2]{0,0xFFFFFFFFu};snapshot->default_scale=NativeEnvMapScale(own);}}
    mix_region(snapshot->fallback);mix(&snapshot->default_scale,4);mix_cube(snapshot->fallback_cube);
    snapshot->uid=h;
    static const bool observe=ModuleFlag(L"light-observe.flag");
    static LONG64 rows{};if(observe)if(const auto n=InterlockedIncrement64(&rows);n<=3 || n%600==0){size_t ready{},handled{};
        for(size_t i=0;i<snapshot->listed.size();++i){ready+=snapshot->cubes[i].Ready();handled+=snapshot->listed[i].handle;}
        const auto& f=snapshot->fallback.box;char row[480]{};
        sprintf_s(row,"{\"event\":\"native_region_snapshot\",\"regions\":%zu,\"handles\":%zu,\"with_handle\":%zu,\"cubes_ready\":%zu,\"fallback\":[%u,%d],\"fallback_box\":[%g,%g,%g,%g,%g,%g,%g,%g,%g,%g],\"fallback_fade\":%g,\"default_scale\":%g}\n",
            snapshot->listed.size(),resolved.size(),handled,ready,snapshot->fallback.key[0],static_cast<int32_t>(snapshot->fallback.key[1]),
            f[0],f[1],f[2],f[3],f[4],f[9],f[14],f[16],f[17],f[18],snapshot->fallback.fade,snapshot->default_scale);Log(row);}
    {const std::lock_guard<std::recursive_mutex> hold(scene_view_cpu_lock);native_region_snapshot_last=snapshot;}
    return snapshot;
}
bool ProduceNativeLightAccParam(const SceneViewInput& camera,const ingress::SceneCameraConstants& constants,std::vector<uint8_t>& output) {
    float projection[16]{};
    for(size_t row=0;row<4;++row)for(size_t column=0;column<4;++column){
        const auto offset=256+4*(4*column+row);if(constants.written[offset/4]!=1)return false;
        std::memcpy(&projection[4*row+column],constants.bytes.data()+offset,4);}
    float inverse[16]{},view[16]{};
    ds3prod::InverseMatrix(projection,inverse);ds3prod::InverseMatrix(camera.camera_pose.data(),view);
    std::array<float,80> f{};
    for(unsigned c=0;c<4;++c)for(unsigned k=0;k<4;++k){f[4*c+k]=inverse[4*k+c];f[32+4*c+k]=camera.camera_pose[4*k+c];f[52+4*c+k]=projection[4*k+c];}
    for(unsigned c=0;c<3;++c)for(unsigned k=0;k<4;++k)f[16+4*c+k]=view[4*k+c];
    const float width=static_cast<float>(camera.width),height=static_cast<float>(camera.height);
    if(!camera.width || !camera.height)return false;
    f[28]=width;f[29]=height;f[30]=1.f/width;f[31]=1.f/height;
    if(!std::all_of(f.begin(),f.end(),[](float v){return std::isfinite(v);}))return false;
    output.assign(320,0);std::memcpy(output.data(),f.data(),320);return true;
}
// 1049 cbLight/cbFog: 137910's loop without its upload, calling the engine's own CPU
// builders. 4A3D0 hands 137910 [root]+41C0+38: entries [+88] (stride 30: sun keys
// +8/+10, fog keys +18/+20, factors +28/+2C), count +90, banks [+98] (+20 light set
// lookup 32770, +28 fog lookup 38E70). Equal keys: 1496D0(out, set+40, view) and
// 14E900(out, set+10, view); else the blends 14A2D0(out, view, a, b, t) and
// 14E5D0(tmp, a+10, b+10, t) when t > 0. The view is the camera output record
// (pose +0, lens +50: fov, near, far, W +5C, H +60), rebuilt from the current copy.
// Native1049: the entries stay empty natively. The old renderer's 24740 fills
// them with 137EF0, deduplicating per-object source lists (+8.., counts +48,
// reset each frame by 23790); objects keep {list, index} at state+F0 (5C0E0)
// and 5B3D0 reads it back as instance envData. The native equivalent of 137EF0
// runs over the selected models in draw order with their group key block
// {+F8 light a, +100 light b, +108 light t, +110 fog a, +118 fog b, +120 fog t}
// (the 0x30 entry and 5A120's +338.. layout); the G-buffer's envData is 0, so
// FC_DirLight[0]/FC_FogParamArray[0] are the first entry.
// 1169: fixed_key = a menu view's gparam key {1, id} (MENUS-MAP 27: FUN_140088AF0 sets it on every bank slot of the
// view pipeline, weight 0), used for both light and fog entries instead of the scene entries.
// 1173: light and fog set records of the resident gparam files. Natively the engine's GX parameter banks never receive
// a file: D272C0 is AdmitGparam, D275F0 is BindNativeGparamFile and 49D00 is BindNativeGparamScene (bookkeeping only), so
// 32770/38E70 return their built-in default for every key (native_light_blocks sun0 (0.7071, 0.7071, 0, 50) for the
// menu key {1, 11}, {1, 0} and the m40 keys alike). The records are built from the same admitted files with the engine's
// rule ({-1, -1} -> {1, 0}; absent (area, id) -> {1, 0}; MENUS-MAP 39, 43): Banks::Sun/Fog, qualified against the engine's
// records in I-0045/I-0063. 1178: map areas too. Sun source (2026-10-13; corrects MENUS-MAP 43, which saw only the
// constructor's identity): an area's records come from the file its controller binds (kind 7) and take their rotation
// from that binding's transform, already in the file's +0x80 when CE6350 binds it (Native1544/1545/1547 equal the original at m37,
// m31, m40); Banks gets both through BindArea. An area without an admitted file keeps the engine lookup (logged).
using GparamLookup=uintptr_t(__fastcall*)(uintptr_t,const void*);
GparamLookup engine_fog_lookup_original{};   // I-0140 tranche 8: 38E70's own body, behind the detour NativeEngineFogLookup below
GparamLookup engine_light_lookup_original{};   // I-0141 tranche 6: 32770's own body, behind the detour NativeEngineLightLookup below
// The keys the native banks answer and the area and id they read for one (NativeGparamRecords and NativeEngineFogLookup, one rule;
// the Handmaid chair's note below): {-1, -1} as {1, 0}, an area of 1 or more with an id other than -1.
bool NativeResidentGparamKey(const uint32_t* key) {return (key[0]==0xFFFFFFFFu && key[1]==0xFFFFFFFFu) || (key[0]>=1 && key[1]!=0xFFFFFFFFu);}
void NativeGparamKeyArea(const uint32_t* key,uint32_t& area,uint32_t& id) {area=key[0]==0xFFFFFFFFu?1:key[0];id=key[0]==0xFFFFFFFFu?0:key[1];}
class NativeGparamRecords {
public:
    NativeGparamRecords(){const std::lock_guard hold(gparam_lock);banks_=gparam_banks;}
    // Lookup-layout records: light set at +0x40 (0x260 bytes, 32770), fog set at +0x10 (38E70).
    uintptr_t Light(uintptr_t bank,const uint32_t* key){return Build(bank,key,false);}
    uintptr_t Fog(uintptr_t bank,const uint32_t* key){return Build(bank,key,true);}
    // I-0141 tranche 5 (D-0279, D-0282): both light keys of an entry (words 2-3 and 4-5) a cutscene's own set, by its key or by the act's
    // override of a map key (Banks::AuthoredLightKey), asked of the same banks the records are built from (the step review's L1);
    // a key of area -1 asked as {1, 0}, the key its record comes from (L5; Build's own mapping).
    bool Authored(const std::array<uint32_t,12>& entry) const {
        // I-0141 tranche 25: a test tool for D-0279's and D-0282's share in a cutscene's picture on one build: no set keeps its lights
        // 1 to 3 on a traced frame (D-0262's rule everywhere).
        static const bool no_cutscene_lights=ModuleFlag(L"no-cutscene-fill-lights.flag");
        if(no_cutscene_lights || (witness_share_variant.load(std::memory_order_relaxed)&kShareNoFill))return false;   // tranche 26: live
        if(ds3light::CutsceneLightSet(entry[2],entry[4]))return true;
        // A test tool for D-0282's A/B on one build and one screen (I-0141 RESULT « Suggested next step »): without the overridden map
        // keys' part, the rule before tranche 5.
        static const bool no_override_lights=ModuleFlag(L"no-override-fill-lights.flag");
        if(!banks_ || no_override_lights)return false;
        const auto key=[&](size_t at,uint32_t& area,uint32_t& id){area=entry[at];id=entry[at+1];if(area==0xFFFFFFFFu){area=1;id=0;}};   // Build's mapping (the final review's L3)
        uint32_t a{},ai{},b{},bi{};key(2,a,ai);key(4,b,bi);
        return banks_->AuthoredLightKey(a,ai) && banks_->AuthoredLightKey(b,bi);}
private:
    struct alignas(16) Record { std::array<uint8_t,0x2C0> bytes; };
    std::shared_ptr<const ds3light::Banks> banks_;
    std::deque<Record> records_;
    // I-0113 (the Handmaid chair o404854, key {m40, -1}): in play the original's bank holds a node for {m40, -1} (32770 and 38E70
    // return a record, not their built-in default: i113-area2-original) whose values are the defaults (sun (0.7071, 0.7071, 0),
    // colours (1, 1, 1, 1): i113-chair2-original's reference_light_set), so the key never reaches {1, 0} (default.gparam,
    // colours (1, 1, 1, 2.5), resident in play: i113-area1-original). Natively such a key takes the engine lookup, whose built-in
    // record carries those defaults (its words +0x130..+0x13F, the third light's colour, differ: read by 1496D0 only with +0x24D
    // set). Who registers the {area, -1} nodes is not read; {-1, -1} keeps its {1, 0}.
    uintptr_t Build(uintptr_t bank,const uint32_t* key,bool fog){
        // I-0141 tranche 3: a key the cutscene act overrides is the override's, {area, -1} included (the original's node of the key
        // holds the override's record).
        if(banks_ && (NativeResidentGparamKey(key) || banks_->Overridden(fog?1:0,key[0],key[1]))){auto& record=records_.emplace_back();record.bytes.fill(0);
            uint32_t area{},id{};NativeGparamKeyArea(key,area,id);
            if(fog?banks_->Fog(area,id,record.bytes.data()+0x10,0x80):banks_->Sun(area,id,record.bytes.data()+0x40,0x260))
                return reinterpret_cast<uintptr_t>(record.bytes.data());
            records_.pop_back();
            static LONG64 missing{};if(InterlockedIncrement64(&missing)<=4){char row[160]{};sprintf_s(row,"{\"event\":\"native_gparam_record_unavailable\",\"fog\":%u,\"key\":[%u,%u]}\n",fog?1u:0u,key[0],key[1]);Log(row);}}
        const auto own=fog?engine_fog_lookup_original:engine_light_lookup_original;   // the engine's own body behind each detour
        const auto lookup=own?own:reinterpret_cast<GparamLookup>(image+(fog?0x38E70:0x32770));
        return lookup(bank,key);}
};
// I-0140 tranche 8 (gap 16 and the tour's 32000010 smoke): the engine's own effect draws ask the engine's fog bank for their cbFog
// (264230 at 1402661F4: 38E70([GXRS+0x41E8], the draw's fog key), then 14014E900 fills the draw context's +0x880 block that PS b6
// reads; the GPU particles' 263A60, the model effects' 5A120 (skipped natively) and the cutscene act's 380A0 the same; READ,
// GhidraMCP). Natively that bank never receives a file (above), so every effect drew with the built-in record: no fog at all
// (`ffx_flame_witness` fog words (1, 0, 0, 0, 0, ...) natively in i113-t26/t30/i140-e21, the map's fog in the original's own rows
// at Archdragon Peak: colour (0.596, 0.709, 0.907), density -0.0132 for 132000000, and Firelink's for 140000000). The soft
// billboards multiply their alpha by (1 - fog) and lerp their colour toward the fog colour (PS 2AFA, `fxc /dumpbin`).
// Here, for the fog bank only: a key the native rule resolves (NativeResidentGparamKey, NativeGparamRecords' rule) or one the
// cutscene act overrides (Banks::Overridden, I-0141 tranche 3, {area, -1} included) takes the record built from the native banks
// as NativeGparamRecords builds it (an overridden key: the act's own engine node, below); any other key, or one the banks lack,
// the engine's own answer, but never a node of the engine's
// bank: natively that bank holds only the nodes the act's 380A0 writes (I-0141's second observation), which the banks already
// answer while the act holds them; I-0141 tranche 4 removes them at the act's withdrawal (before it, R's note 2026-11-15, the
// ceremony's {132000000, -1} stayed after the scene), so the pass-over is now a guard: such a node is answered with the bank's
// built-in record (+0x180, 38E70's last return), as every key was before. Any other bank: the engine's own answer. The records are shared by content (equal
// fog words, one record; never freed, so a returned record outlives any caller's use, and no caller is known to write through it;
// their number bounded by the files' distinct fog records, overridden keys excepted, above), each thread indexing them per banks publication (native_gparam_publishes,
// read without gparam_lock; the lock only when it changed) and binding generation; one row when the store reaches its bound, rows
// for the first keys answered and for the first engine nodes passed over. Installed only without
// no-effect-fog-banks.flag (22_setup).
uintptr_t NativeEngineFogAnswer(uintptr_t bank,const void* key_words) noexcept {
    try{
        uintptr_t root{},fog_bank{};const auto* key=static_cast<const uint32_t*>(key_words);
        if(!key || !Read(image+0x4961B50,&root,8) || !root || !Read(root+0x41E8,&fog_bank,8) || bank!=fog_bank)
            return engine_fog_lookup_original(bank,key_words);
        const auto engine_answer=[&]{const uintptr_t engine=engine_fog_lookup_original(bank,key_words);if(engine==bank+0x180)return engine;
            static std::atomic<int> nodes{};if(const int n=nodes.fetch_add(1)+1;n<=16){char row[200]{};
                sprintf_s(row,"{\"event\":\"native_effect_fog_engine_node\",\"n\":%d,\"key\":[%u,%u]}\n",n,key[0],key[1]);Log(row);}
            return bank+0x180;};
        struct alignas(16) Fresh {std::array<uint8_t,0x2C0> bytes{};};
        thread_local struct {uint64_t serial{~0ull},generation{~0ull};std::weak_ptr<const ds3light::Banks> banks;bool none{};std::unordered_map<uint64_t,uintptr_t> records;std::deque<Fresh> step,older;} local;
        std::shared_ptr<const ds3light::Banks> banks;const uint64_t published=native_gparam_publishes.load(std::memory_order_acquire);
        if(published==local.serial){if(local.none)return engine_answer();banks=local.banks.lock();}
        if(!banks){const std::lock_guard hold(gparam_lock);banks=gparam_banks;local.banks=banks;local.none=!banks;local.generation=~0ull;local.records.clear();   // the stores rotate once, at the generation check below (the step review's H1)
           
            local.serial=native_gparam_publishes.load(std::memory_order_acquire);}
        if(!banks)return engine_answer();
        // I-0141 tranche 7's step review M1: the generation before any answer is built (a fade's step renews it); the step store kept
        // one generation more (the second review's M2: a caller's two lookups across a step, 380A0's A and B, both stay readable).
        if(const uint64_t generation=banks->RecordGeneration();generation!=local.generation){local.records.clear();local.older.swap(local.step);local.step.clear();local.generation=generation;}
        // The final review's M1: a key the act overrides is blended at each act update, so its records are not stored (they would
        // fill the store); while the act holds it, the engine's own node of the key, which 380A0 writes at each update from the
        // banks' sources, is the override (I-0141 tranche 3, R's note); a fresh record in a small per-thread ring only in the
        // window before 380A0 wrote the node (the final review's L5), used by its caller at once (14E900 copies it).
        if(banks->Overridden(1,key[0],key[1])){const uintptr_t engine=engine_fog_lookup_original(bank,key_words);if(engine!=bank+0x180)return engine;
            thread_local std::array<Fresh,8> ring{};thread_local uint32_t next{};
            uint32_t area{},id{};NativeGparamKeyArea(key,area,id);auto& slot=ring[next++%ring.size()];slot.bytes.fill(0);
            return banks->Fog(area,id,slot.bytes.data()+0x10,0x80)?reinterpret_cast<uintptr_t>(slot.bytes.data()):engine;}
        // I-0141 tranche 6's witness (i141-t6a): a key of area 0 is no bank's (27790's table registers none there), so the engine's
        // lookup falls to {1, 0}, default's record (the original's effect_light_key rows for {0, 32000}, {0, -1}, {0, -2}: default's
        // {1, 0}, not the built-in record); here the same, where the native rule left those keys to the built-in record.
        const uint32_t fallback[2]{1,0};if(key[0]==0)key=fallback;
        if(!NativeResidentGparamKey(key))return engine_answer();
        uint32_t area{},id{};NativeGparamKeyArea(key,area,id);
        const uint64_t index=(uint64_t(area)<<32)|id;
        uintptr_t answer{};
        if(const auto found=local.records.find(index);found!=local.records.end())answer=found->second;
        // I-0141 tranche 7: a key of a fading area, whose record changes at each fade step: built once a step on this thread, kept in
        // this thread's step store until its first lookup after the next step (the content store would keep every step's record).
        else if(banks->Fading(area)){if(local.step.size()>=4096)return engine_answer();   // bounded (the second review's L4)
            auto& made=local.step.emplace_back();
            answer=banks->Fog(area,id,made.bytes.data()+0x10,0x80)?reinterpret_cast<uintptr_t>(made.bytes.data()):0;if(!answer)local.step.pop_back();
            if(local.records.size()<1024)local.records.emplace(index,answer);}
        else{
            struct alignas(16) Record {std::array<uint8_t,0x2C0> bytes{};};
            static std::mutex store_lock;static std::deque<Record> store;static std::map<std::array<uint8_t,0x80>,uintptr_t> by_content;static bool full_told{};
            if(Record record{};banks->Fog(area,id,record.bytes.data()+0x10,0x80)){std::array<uint8_t,0x80> words{};std::memcpy(words.data(),record.bytes.data()+0x10,words.size());
                bool told{};{const std::lock_guard hold(store_lock);
                    if(const auto known=by_content.find(words);known!=by_content.end())answer=known->second;
                    else if(store.size()<4096){store.push_back(record);answer=reinterpret_cast<uintptr_t>(store.back().bytes.data());by_content.emplace(words,answer);}
                    else if(!full_told)full_told=told=true;}
                if(told)Log("{\"event\":\"native_effect_fog_store_full\",\"records\":4096}\n");}
            static std::atomic<int> rows{};if(const int n=rows.fetch_add(1)+1;n<=16){char row[200]{};
                sprintf_s(row,"{\"event\":\"native_effect_fog_record\",\"n\":%d,\"key\":[%u,%u],\"from_banks\":%s}\n",n,key[0],key[1],answer?"true":"false");Log(row);}
            if(local.records.size()<1024)local.records.emplace(index,answer);}
        return answer?answer:engine_answer();
    }catch(...){return engine_fog_lookup_original(bank,key_words);}
}
// I-0140 tranche 8's witness (effect-fog-observe.flag, both modes; the haze shot of 32000000 the fix loses, i140-e25): each
// (caller, key, answer's first 24 fog words) asked of the engine's fog bank the first time it is met (at most 600 rows; i140-e26's
// first form, a row each 600th ask under 160 rows, spent them at Firelink before the cutscene): the caller's address, the key,
// whether the answer is the bank's built-in record (+0x180), and those words (+0x10), the block 14E900 turns into cbFog. In the
// original through a pass-through observer of 38E70 (22_setup); natively in the detour, after its answer.
void LogEffectFogKey(const char* mode,uintptr_t caller,uintptr_t bank,const void* key_words,uintptr_t record) noexcept {
    static const bool on=ModuleFlag(L"effect-fog-observe.flag");if(!on || !key_words)return;
    uintptr_t root{},fog_bank{};if(!Read(image+0x4961B50,&root,8) || !root || !Read(root+0x41E8,&fog_bank,8) || bank!=fog_bank)return;
    uint32_t key[2]{};if(!Read(reinterpret_cast<uintptr_t>(key_words),key,8))return;
    float words[24]{};if(record)Read(record+0x10,words,sizeof words);
    uint64_t hash=1469598103934665603ull;for(const auto byte:std::span(reinterpret_cast<const uint8_t*>(words),sizeof words))hash=(hash^byte)*1099511628211ull;
    static std::mutex lock;static std::map<std::tuple<uintptr_t,uint32_t,uint32_t,uint64_t>,uint64_t> asked;static int rows{};uint64_t n{};
    {const std::lock_guard hold(lock);if(rows>=600)return;n=++asked[{caller,key[0],key[1],hash}];if(n!=1)return;++rows;}   // the final review's L5
    std::string row;char cell[200]{};
    sprintf_s(cell,"{\"event\":\"effect_fog_key\",\"mode\":\"%s\",\"caller\":\"%llX\",\"key\":[%u,%u],\"n\":%llu,\"builtin\":%s,\"f\":[",mode,
        static_cast<unsigned long long>(caller>image?caller-image:caller),key[0],key[1],static_cast<unsigned long long>(n),record==bank+0x180?"true":"false");row=cell;
    for(int k=0;k<24;++k){if(std::isfinite(words[k]))sprintf_s(cell,"%s%.6g",k?",":"",words[k]);else sprintf_s(cell,"%snull",k?",":"");row+=cell;}
    row+="]}\n";Log(row.c_str());}
uintptr_t __fastcall NativeEngineFogLookup(uintptr_t bank,const void* key_words) noexcept {
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());const uintptr_t answer=NativeEngineFogAnswer(bank,key_words);
    LogEffectFogKey("native",caller,bank,key_words,answer);return answer;}
uintptr_t __fastcall ObserveEngineFogLookup(uintptr_t bank,const void* key_words) noexcept {
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());const uintptr_t answer=engine_fog_lookup_original(bank,key_words);
    LogEffectFogKey("reference",caller,bank,key_words,answer);return answer;}
// I-0141 tranche 6 (the ceremony scene 32000000's smoke at 28 to 35 s, natively up to 2.7 times the original's level on i141-t5a's
// shots paired by the second; I-0140 RESULT « Next » item 6): the engine's effect draws ask the engine's light bank too. 264230 calls
// 32770([GXRS+0x41E0], the draw's light key +0xA20) four times (READ, GhidraMCP, 2026-11-15): at 140265919 for the record's +0x2C0
// (a strength the draw's colours +0x590..+0x59C are scaled toward a constant by), at 140265AB3 for {-1, -1} when the key's id is -1
// (the record's +0x40 and +0x48 copied), at 140265B4E for 1496D0 on the record's +0x40 (cbLight's light rows) and its ambient words
// +0x220..+0x23F, and at 140265CDB for 1495F0's hemisphere rows; the GPU particles (2A45A0, 2A4EB0, 2ADC80, 2AE5A0) and the act's
// writer 31A20 call it as well. Natively that bank never receives a file (above), so every lit effect drew with the bank's built-in
// record (+0x700, 32770's last return) whatever its zone, scene or the act's override. Here, for the light bank only, the same rule as
// NativeEngineFogAnswer's: a key the native rule resolves or one the act overrides (Banks::Overridden group 0, its engine node while
// 31A20 holds one) takes the record built from the native banks; any other key the engine's answer, a node of the engine's bank passed
// over for the built-in record. The record: a whole engine record (0x350 bytes, MENUS-MAP « A LightSet record »), the built-in
// record's bytes (its header and the words SetDefaults 14B3A0 writes beyond Banks' range, +0x330..+0x340; READ, GhidraMCP
// 2026-11-15), the strength +0x2C0 from the set's « LightMask Influence » (Banks::OwnSun's +0x280; the witness i141-t6a),
// Banks::Sun's words over +0x40..+0x290 and its rotation at +0x2D0, the key's id at +0x2E0 and area at +0x2E4 (30D80's order; the
// key as NativeGparamKeyArea maps it, {-1, -1} as {1, 0}; an id the area lacks keeps the asked id where the engine's {1, 0} node
// holds 0 and 1), then the rows +0x290 from the engine's 1495F0 as 14BA90 ends an engine record (NativeInstanceLightRows' call).
// Stored once a key and content (the key is in the record: no two keys share one) and never freed, as the fog's; no caller is known
// to write through it (the callers above read it). Installed only without no-effect-light-banks.flag (22_setup).
constexpr size_t kNativeLightRecordBytes=0x350;
bool NativeEngineLightRecord(const ds3light::Banks& banks,uintptr_t bank,uint32_t area,uint32_t id,uint8_t* record) noexcept {
    alignas(16) std::array<uint8_t,0x2A0> sun{};
    if(!banks.Sun(area,id,sun.data(),static_cast<uint32_t>(sun.size())) || !Read(bank+0x700,record,kNativeLightRecordBytes))return false;
    std::memcpy(record+0x40,sun.data(),0x250);std::memcpy(record+0x2D0,sun.data()+0x290,16);std::memcpy(record+0x2C0,sun.data()+0x280,4);   // the strength: LightMask Influence (Banks::OwnSun)
    std::memcpy(record+0x2E0,&id,4);std::memcpy(record+0x2E4,&area,4);   // id then area (30D80's order), {-1, -1} as {1, 0} (the reviews' M2, L1)
    const auto rows=reinterpret_cast<void(__fastcall*)(const void*,float*)>(image+0x1495F0);
    alignas(16) float out[16]{};rows(record,out);std::memcpy(record+0x290,out,48);
    return true;}
uintptr_t NativeEngineLightAnswer(uintptr_t bank,const void* key_words) noexcept {
    try{
        uintptr_t root{},light_bank{};const auto* key=static_cast<const uint32_t*>(key_words);
        if(!key || !Read(image+0x4961B50,&root,8) || !root || !Read(root+0x41E0,&light_bank,8) || bank!=light_bank)
            return engine_light_lookup_original(bank,key_words);
        const auto engine_answer=[&]{const uintptr_t engine=engine_light_lookup_original(bank,key_words);if(engine==bank+0x700)return engine;
            static std::atomic<int> nodes{};if(const int n=nodes.fetch_add(1)+1;n<=16){char row[200]{};
                sprintf_s(row,"{\"event\":\"native_effect_light_engine_node\",\"n\":%d,\"key\":[%u,%u]}\n",n,key[0],key[1]);Log(row);}
            return bank+0x700;};
        struct alignas(16) Record {std::array<uint8_t,kNativeLightRecordBytes> bytes{};};
        thread_local struct {uint64_t serial{~0ull},generation{~0ull};std::weak_ptr<const ds3light::Banks> banks;bool none{};std::unordered_map<uint64_t,uintptr_t> records;std::deque<Record> step,older;} local;
        std::shared_ptr<const ds3light::Banks> banks;const uint64_t published=native_gparam_publishes.load(std::memory_order_acquire);
        if(published==local.serial){if(local.none)return engine_answer();banks=local.banks.lock();}
        if(!banks){const std::lock_guard hold(gparam_lock);banks=gparam_banks;local.banks=banks;local.none=!banks;local.generation=~0ull;local.records.clear();   // the stores rotate once, at the generation check below (the step review's H1)
           
            local.serial=native_gparam_publishes.load(std::memory_order_acquire);}
        if(!banks)return engine_answer();
        // I-0141 tranche 7's step review M1: the generation before any answer is built (a fade's step renews it); the step store kept
        // one generation more (as the fog's: 31A20's A and B across a step).
        if(const uint64_t generation=banks->RecordGeneration();generation!=local.generation){local.records.clear();local.older.swap(local.step);local.step.clear();local.generation=generation;}
        // A key the act overrides: its engine node, which 31A20 writes at each update from its sources' records (this detour answers its
        // two 32770 calls, so Banks' records where the sources are keys the native rule resolves, as the act's are in the scenes read:
        // {232000000, id}; a source {area, -1} would take the built-in record there and Banks' rule in Banks::Sun), else a fresh record in a
        // small per-thread ring, used by its caller at once (as the fog's; its rows 1495F0's of the blended record, where 31A20 blends
        // A's and B's rows).
        if(banks->Overridden(0,key[0],key[1])){const uintptr_t engine=engine_light_lookup_original(bank,key_words);if(engine!=bank+0x700)return engine;
            thread_local std::array<Record,8> ring{};thread_local uint32_t next{};
            uint32_t area{},id{};NativeGparamKeyArea(key,area,id);auto& slot=ring[next++%ring.size()];
            return NativeEngineLightRecord(*banks,bank,area,id,slot.bytes.data())?reinterpret_cast<uintptr_t>(slot.bytes.data()):engine;}
        // I-0141 tranche 6's witness (i141-t6a): a key of area 0 is no bank's (27790's table registers none there), so the engine's
        // lookup falls to {1, 0}, default's record (the original's effect_light_key rows for {0, 32000}, {0, -1}, {0, -2}: default's
        // {1, 0}, not the built-in record); here the same, where the native rule left those keys to the built-in record.
        const uint32_t fallback[2]{1,0};if(key[0]==0)key=fallback;
        if(!NativeResidentGparamKey(key))return engine_answer();
        uint32_t area{},id{};NativeGparamKeyArea(key,area,id);
        const uint64_t index=(uint64_t(area)<<32)|id;
        uintptr_t answer{};
        if(const auto found=local.records.find(index);found!=local.records.end())answer=found->second;
        // I-0141 tranche 7: a key of a fading area (as the fog's): built once a fade step, in this thread's step store.
        else if(banks->Fading(area)){if(local.step.size()>=4096)return engine_answer();   // bounded (the second review's L4)
            auto& made=local.step.emplace_back();
            answer=NativeEngineLightRecord(*banks,bank,area,id,made.bytes.data())?reinterpret_cast<uintptr_t>(made.bytes.data()):0;if(!answer)local.step.pop_back();
            if(local.records.size()<1024)local.records.emplace(index,answer);}
        else{
            static std::mutex store_lock;static std::deque<Record> store;static std::map<std::array<uint8_t,kNativeLightRecordBytes>,uintptr_t> by_content;static bool full_told{};
            if(Record record{};NativeEngineLightRecord(*banks,bank,area,id,record.bytes.data())){bool told{};{const std::lock_guard hold(store_lock);
                    if(const auto known=by_content.find(record.bytes);known!=by_content.end())answer=known->second;
                    else if(store.size()<4096){store.push_back(record);answer=reinterpret_cast<uintptr_t>(store.back().bytes.data());by_content.emplace(record.bytes,answer);}
                    else if(!full_told)full_told=told=true;}
                if(told)Log("{\"event\":\"native_effect_light_store_full\",\"records\":4096}\n");}
            static std::atomic<int> rows{};if(const int n=rows.fetch_add(1)+1;n<=16){char row[200]{};
                sprintf_s(row,"{\"event\":\"native_effect_light_record\",\"n\":%d,\"key\":[%u,%u],\"from_banks\":%s}\n",n,key[0],key[1],answer?"true":"false");Log(row);}
            if(local.records.size()<1024)local.records.emplace(index,answer);}
        return answer?answer:engine_answer();
    }catch(...){return engine_light_lookup_original(bank,key_words);}
}
// The witness (effect-light-observe.flag, both modes): each (caller, key, answer's words) asked of the engine's light bank the first
// time it is met, at most 600 rows: light 0 (+0x40: direction, colour, specular colour), the ambient words +0x220..+0x23F and the
// strength +0x2C0, and whether the answer is the built-in record. In the original through a pass-through observer of 32770 (22_setup).
void LogEffectLightKey(const char* mode,uintptr_t caller,uintptr_t bank,const void* key_words,uintptr_t record) noexcept {
    static const bool on=ModuleFlag(L"effect-light-observe.flag");if(!on || !key_words)return;
    uintptr_t root{},light_bank{};if(!Read(image+0x4961B50,&root,8) || !root || !Read(root+0x41E0,&light_bank,8) || bank!=light_bank)return;
    uint32_t key[2]{};if(!Read(reinterpret_cast<uintptr_t>(key_words),key,8))return;
    float words[21]{};if(record){Read(record+0x40,words,48);Read(record+0x220,words+12,32);Read(record+0x2C0,words+20,4);}
    uint64_t hash=1469598103934665603ull;for(const auto byte:std::span(reinterpret_cast<const uint8_t*>(words),sizeof words))hash=(hash^byte)*1099511628211ull;
    static std::mutex lock;static std::map<std::tuple<uintptr_t,uint32_t,uint32_t,uint64_t>,uint64_t> asked;static int rows{};
    {const std::lock_guard hold(lock);if(rows>=600 || ++asked[{caller,key[0],key[1],hash}]!=1)return;++rows;}   // the bound first (the step review's L3)
    std::string row;char cell[200]{};
    sprintf_s(cell,"{\"event\":\"effect_light_key\",\"mode\":\"%s\",\"caller\":\"%llX\",\"key\":[%u,%u],\"builtin\":%s,\"f\":[",mode,
        static_cast<unsigned long long>(caller>image?caller-image:caller),key[0],key[1],record==bank+0x700?"true":"false");row=cell;
    for(int k=0;k<21;++k){if(std::isfinite(words[k]))sprintf_s(cell,"%s%.6g",k?",":"",words[k]);else sprintf_s(cell,"%snull",k?",":"");row+=cell;}
    row+="]}\n";Log(row.c_str());}
uintptr_t __fastcall NativeEngineLightLookup(uintptr_t bank,const void* key_words) noexcept {
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());const uintptr_t answer=NativeEngineLightAnswer(bank,key_words);
    LogEffectLightKey("native",caller,bank,key_words,answer);return answer;}
uintptr_t __fastcall ObserveEngineLightLookup(uintptr_t bank,const void* key_words) noexcept {
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());const uintptr_t answer=engine_light_lookup_original(bank,key_words);
    LogEffectLightKey("reference",caller,bank,key_words,answer);return answer;}
// I-0141 (the haze in play at Archdragon Peak after 32000010, natively denser than the original's on i141-t5a's shots): the cbFog
// blocks the engine's builder 14E900 makes (the composite's world fog, GXLightComposite's cbFog, and the forward rows' and effects'),
// in both modes with fog-build-observe.flag: each distinct block (its words but Param.z and HeightParam.w, which follow the camera every
// frame) once per caller, with the camera's position (the view record's +0x30..+0x38, 14E900's +0x34), at most 200 rows a caller and 1200 in all; natively our
// collect calls 14E900 through its address, so the hook sees those calls too.
using FogBlockBuild=void(__fastcall*)(void*,const void*,const void*);
FogBlockBuild fog_block_build_original{};
// I-0141 tranche 14 (the haze in play stays without the effects, i141-t14k; each caller's latest block shows only an array's last
// entry): every 5 s one whole deferred array, its keys, fog blocks and light blocks (fog_array, at most 300 rows a mode). The
// original's: 137910 (READ, GhidraMCP) builds the light entries (1496D0 at 137A7D, the blend 14A2D0 at 137ABF; entry i at
// base+0x120*i), uploads them, then the fog entries (14E900 at 137B92 returning to 137B97, the blend's at 137C39 returning to
// 137C3E; entry i at out+0x60*i), into the same allocation when [[..]+0xA8] is set, else the light at [object+0x68] and the fog at
// [object+0x70]; object = [root]+0x41F8, its entries +0x88 (0x30 bytes each: light a, light b, fog a, fog b, light t, fog t from
// word 2) and count +0x90. So at a run's first fog call (out not 0x60 past the last), before the builder writes, a run due (5 s
// after the last due run's start) snapshots the keys and the light entries whole; its fog blocks follow; it is emitted when the next run starts.
// The row says whether its counts agree (keys, fog blocks, light entries: « consistent ») and where the light was read
// (« separate »: [object+0x68] when [object+0x70] is the run's first fog address; « fog_output »: else the run's first fog address,
// the one allocation when 137910's selector [[..]+0xA8] is set, a selector not read here; « unread »: that read failed; « none »:
// no entries read). Its time is the row's emission (the next run's start), its camera the view at the run's start (null unread).
// Each block is the whole 0x60 bytes: 14E900 writes words 0 to 20 (READ), word 20 the int 1; 21 to 23 it never writes. Words 0 to
// 19 print as floats (%.4g here, %.6g in fog_block_latest and fog_block_build; a denormal among them prints 0 on a thread whose
// MXCSR treats denormal inputs as zero, DAZ), 20 to 23 as their bits (« tail »: the float conversion may read the int 1 as 0 there, DAZ, else 1.4e-45),
// split as in fog_block_latest and fog_block_build; a block whose read failed prints « block » and « tail » null.
// The native's: written by CollectNativeLightBlocks from its own keys and blocks (at least two: one key is doubled), with the sets
// whose light record failed and those whose fog block is set 0's (a light failure replaces both).
std::string FogCamera(const float* c) {std::string out="[";char cell[24]{};   // the view's position, null when not finite
    for(int j=0;j<3;++j){if(std::isfinite(c[j]))sprintf_s(cell,"%s%.4g",j?",":"",c[j]);else sprintf_s(cell,"%snull",j?",":"");out+=cell;}return out+"]";}
std::string FogArrayRow(const char* mode,double ms,const float* camera,size_t entries,const uint32_t* keys,size_t key_count,const uint8_t* blocks,
                        const uint8_t* light,size_t light_count,const std::string& extra,const uint8_t* read=nullptr) {   // read: per block, 0 when its read failed
    std::string row;char cell[200]{};
    const auto number=[&](const char* lead,float v,const char* format){if(std::isfinite(v))sprintf_s(cell,format,lead,v);else sprintf_s(cell,"%snull",lead);row+=cell;};
    sprintf_s(cell,"{\"event\":\"fog_array\",\"mode\":\"%s\",\"qpc_ms\":%.1f,\"entries\":%zu,\"keys\":%zu,\"lights\":%zu",mode,ms,entries,key_count,light?light_count:0);row=cell;
    row+=extra+",\"camera\":[";for(int j=0;j<3;++j)number(j?",":"",camera[j],"%s%.4g");row+="],\"rows\":[";
    const auto floats=[&](const char* name,const uint8_t* at,int n){row+=name;
        for(int j=0;j<n;++j){float v{};std::memcpy(&v,at+4*j,4);number(j?",":"",v,"%s%.4g");}
        row+="]";};
    for(size_t i=0;i<entries;++i){row+=i?",{":"{";
        if(i<key_count){const uint32_t* k=keys+12*i;float t[2]{};std::memcpy(t,k+10,8);
            sprintf_s(cell,"\"key\":[%u,%u,%u,%u,%u,%u,%u,%u],\"t\":[",k[2],k[3],k[4],k[5],k[6],k[7],k[8],k[9]);row+=cell;
            number("",t[0],"%s%.3g");number(",",t[1],"%s%.3g");row+="],";}
        if(read && !read[i])row+="\"block\":null,\"tail\":null";
        else{floats("\"block\":[",blocks+0x60*i,20);uint32_t w[4]{};std::memcpy(w,blocks+0x60*i+0x50,16);
            sprintf_s(cell,",\"tail\":[\"%08X\",\"%08X\",\"%08X\",\"%08X\"]",w[0],w[1],w[2],w[3]);row+=cell;}
        if(light && i<light_count)floats(",\"light\":[",light+0x120*i,72);
        row+="}";}
    return row+"]}\n";}
double FogArrayClockMs() noexcept {LARGE_INTEGER now{},rate{};QueryPerformanceCounter(&now);QueryPerformanceFrequency(&rate);return double(now.QuadPart)*1000.0/double(rate.QuadPart);}
struct OriginalFogArrayRun {std::vector<uint8_t> blocks,light,read;std::vector<uint32_t> keys;uintptr_t last{};float camera[3]{};bool due{};const char* light_from{"none"};};
std::mutex original_fog_array_lock;OriginalFogArrayRun original_fog_array;double original_fog_array_next_ms{};int original_fog_array_rows{};
// Before the builder: a new run's start emits the run before it (when it was due; formatted outside the lock) and, when this run
// is due, snapshots the keys and the light entries. Not noexcept: ObserveFogBlockBuild catches an allocation failure.
void OriginalFogArrayBefore(uintptr_t caller,uintptr_t out,const void* view) {
    if(caller!=image+0x137B97 && caller!=image+0x137C3E)return;
    OriginalFogArrayRun done;
    {const std::lock_guard hold(original_fog_array_lock);auto& run=original_fog_array;if(out==run.last+0x60 || (!run.due && original_fog_array_rows>=300))return;
        const uintptr_t last=run.last;if(run.due && !run.blocks.empty())std::swap(done,run);
        run.blocks.clear();run.read.clear();run.light.clear();run.keys.clear();run.light_from="none";run.due=false;run.last=last;
        const double ms=FogArrayClockMs();if(original_fog_array_rows<300 && ms>=original_fog_array_next_ms){
        original_fog_array_next_ms=ms+5000.0;++original_fog_array_rows;run.due=true;run.read.reserve(225);run.blocks.reserve(0x60*225);
        for(auto& c:run.camera)c=std::numeric_limits<float>::quiet_NaN();Read(reinterpret_cast<uintptr_t>(view)+0x30,run.camera,sizeof run.camera);
        uintptr_t root{},entries{},fog_buffer{},light_buffer{};int32_t count{};const uintptr_t object=[&]{Read(image+0x4961B50,&root,8);return root?root+0x41F8:0;}();
        if(object && Read(object+0x90,&count,4) && Read(object+0x88,&entries,8) && entries && count>0 && count<=225){
            run.keys.resize(12ull*count);if(!Read(entries,run.keys.data(),run.keys.size()*4))run.keys.clear();
            const bool separate=Read(object+0x70,&fog_buffer,8) && fog_buffer==out && Read(object+0x68,&light_buffer,8) && light_buffer;
            const uintptr_t base=separate?light_buffer:out;run.light_from=separate?"separate":"fog_output";
            run.light.resize(0x120ull*count);if(!Read(base,run.light.data(),run.light.size())){run.light.clear();run.light_from="unread";}}}}
    if(!done.due)return;
    const size_t blocks=done.blocks.size()/0x60,keys=done.keys.size()/12,lights=done.light.size()/0x120;
    Log(FogArrayRow("reference",FogArrayClockMs(),done.camera,blocks,done.keys.data(),keys,done.blocks.data(),done.light.data(),lights,
                    std::string(",\"consistent\":")+(blocks==keys && keys==lights?"1":"0")+",\"light_from\":\""+done.light_from+"\"",done.read.data()).c_str());}
// After the builder: the run's next fog block (a due run's), and the run's last block address past the cap too.
void OriginalFogArrayAfter(uintptr_t caller,uintptr_t out) {
    if(caller!=image+0x137B97 && caller!=image+0x137C3E)return;
    const std::lock_guard hold(original_fog_array_lock);auto& run=original_fog_array;run.last=out;if(!run.due || run.blocks.size()>=0x60*225)return;
    if(run.read.size()>=run.read.capacity())return;   // room for 225 flags reserved when the run became due: the push below cannot throw
    const size_t at=run.blocks.size();run.blocks.resize(at+0x60);
    run.read.push_back(Read(out,run.blocks.data()+at,0x60)?1:0);}
void __fastcall ObserveFogBlockBuild(void* out,const void* set,const void* view) noexcept {
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());
    try{OriginalFogArrayBefore(caller,reinterpret_cast<uintptr_t>(out),view);}catch(...){}
    fog_block_build_original(out,set,view);
    try{OriginalFogArrayAfter(caller,reinterpret_cast<uintptr_t>(out));float block[24]{},camera[3]{};if(!Read(reinterpret_cast<uintptr_t>(out),block,sizeof block) || !Read(reinterpret_cast<uintptr_t>(view)+0x30,camera,sizeof camera))return;
        uint64_t hash=1469598103934665603ull;for(int k=0;k<24;++k){if(k==2 || k==15)continue;uint32_t bits{};std::memcpy(&bits,&block[k],4);for(int b=0;b<4;++b)hash=(hash^((bits>>(8*b))&0xFF))*1099511628211ull;}
        static std::mutex lock;static std::set<std::pair<uintptr_t,uint64_t>> seen;static std::map<uintptr_t,int> per_caller;static int rows{};
        // I-0141 tranche 14 (the haze in play after 32000010: the rows above were spent before the play): every 5 s, each caller's latest
        // block with the QPC time (fog_block_latest, at most 3000 rows), so the last rows of a run are the blocks in play.
        {static std::map<uintptr_t,std::array<float,27>> latest;static double next_ms{};static int latest_rows{};std::vector<std::string> due;
            LARGE_INTEGER now{},rate{};QueryPerformanceCounter(&now);QueryPerformanceFrequency(&rate);const double ms=double(now.QuadPart)*1000.0/double(rate.QuadPart);
            {const std::lock_guard hold(lock);auto& keep=latest[caller];std::memcpy(keep.data(),block,sizeof block);std::memcpy(keep.data()+24,camera,sizeof camera);
                if(ms>=next_ms && latest_rows<3000){next_ms=ms+5000.0;
                    for(const auto& [who,b]:latest){if(++latest_rows>3000)break;char head[160]{};
                        sprintf_s(head,"{\"event\":\"fog_block_latest\",\"mode\":\"%s\",\"qpc_ms\":%.1f,\"caller\":\"%llX\",\"camera\":%s,\"block\":[",
                            control && control->mode==bootstrap::Mode::NativeBootstrap?"native":"reference",ms,static_cast<unsigned long long>(who>image?who-image:who),FogCamera(b.data()+24).c_str());
                        std::string text=head;for(int k=0;k<20;++k){char c[24]{};if(std::isfinite(b[k]))sprintf_s(c,"%s%.6g",k?",":"",b[k]);else sprintf_s(c,"%snull",k?",":"");text+=c;}
                        uint32_t w[4]{};std::memcpy(w,b.data()+20,16);char tail[64]{};sprintf_s(tail,"],\"tail\":[\"%08X\",\"%08X\",\"%08X\",\"%08X\"",w[0],w[1],w[2],w[3]);   // words 20 to 23's bits
                        due.push_back(text+tail+"]}\n");}}}
            for(const auto& text:due)Log(text.c_str());}
        {const std::lock_guard hold(lock);if(rows>=1200 || per_caller[caller]>=200 || !seen.emplace(caller,hash).second)return;++rows;++per_caller[caller];}
        std::string row;char cell[160]{};
        sprintf_s(cell,"{\"event\":\"fog_block_build\",\"mode\":\"%s\",\"caller\":\"%llX\",\"camera\":%s,\"block\":[",control && control->mode==bootstrap::Mode::NativeBootstrap?"native":"reference",
            static_cast<unsigned long long>(caller>image?caller-image:caller),FogCamera(camera).c_str());row=cell;
        for(int k=0;k<20;++k){if(std::isfinite(block[k]))sprintf_s(cell,"%s%.6g",k?",":"",block[k]);else sprintf_s(cell,"%snull",k?",":"");row+=cell;}   // words 0 to 19; 20 to 23 as bits
        {uint32_t w[4]{};std::memcpy(w,block+20,16);sprintf_s(cell,"],\"tail\":[\"%08X\",\"%08X\",\"%08X\",\"%08X\"",w[0],w[1],w[2],w[3]);row+=cell;}   // words 20 to 23's bits
        row+="]}\n";Log(row.c_str());
    }catch(...){}
}
// I-0141 (45000030's cyan veil: our light shafts drawn where the original draws none): the library's keeper FUN_1416EA000(S), run each
// frame from EndPostEffectScene_Process (READ, GhidraMCP 2026-11-15: one argument, S in RCX; returns AL = kept > 0, also stored at
// S+0x9A0). Its loop (READ): the slots are an array at the pointer [S+0x6B0], 0x58 bytes each, [S+0x378] of them; a slot is kept
// when its byte +0x50 is set, the glare luminance S+0x6A4 is positive and one of its colour words +0x00..+0x08 is positive; the
// kept count goes to [[S+0x108]+0x1C0]. Observed in the original with shaft-observe.flag: S, the count, the luminance and the first
// two slots' colour, Enable, depth +0x4C and depth space +0x54; one row at each change, at most 400.
using ShaftKeeper=char(__fastcall*)(uintptr_t);
ShaftKeeper shaft_keeper_original{};
char __fastcall ObserveShaftKeeper(uintptr_t library) noexcept {
    const char kept=shaft_keeper_original(library);
    // I-0138 tranche 8e: the stored glare shape (S+0xBC4: 1416C9690 stores the globals' +0xD8 there each frame; 0 skips the glare
    // family, 3 is the « Standard » glare) and the glare luminance S+0x6A4, one reference_glare_shape row at each change of either
    // the shape or the luminance's sign, at most 400, with the keeper's call count and S (per S: one context in every journal so far, up
    // to the keeper rows' 400-row cap; its default states between scenes; a second context would get its own last state). A failed read gets a key no read can
    // produce and a row with "read":false; a non-finite luminance is written null.
    try{static std::mutex shape_lock;static std::map<uintptr_t,int64_t> shape_last;static int shape_rows{};static uint64_t shape_calls{};
        char row[320]{};
        {const std::lock_guard hold(shape_lock);const uint64_t call=shape_calls++;
            if(shape_rows<400){int32_t shape{-1};float luminance{};
                const bool read=Read(library+0xBC4,&shape,4) && Read(library+0x6A4,&luminance,4);
                const int64_t key=read?int64_t(shape)*2+(luminance>0.0f?1:0):INT64_MIN;
                const auto [at,fresh]=shape_last.try_emplace(library,key);
                if(fresh || at->second!=key){at->second=key;++shape_rows;char value[32]{"null"};if(std::isfinite(luminance))sprintf_s(value,"%.4g",luminance);
                    sprintf_s(row,"{\"event\":\"reference_glare_shape\",\"call\":%llu,\"library\":\"%llX\",\"read\":%s,\"shape\":%d,\"luminance\":%s%s}\n",
                        static_cast<unsigned long long>(call),static_cast<unsigned long long>(library),read?"true":"false",shape,value,CutsceneClockCell().c_str());}}}
        if(row[0])Log(row);}catch(...){}
    // Each section's cap checked before its reads (nothing more done past it).
    static std::atomic<int> keeper_rows{};
    static std::atomic<int> depth_rows{};
    const bool keeper_open=keeper_rows.load(std::memory_order_relaxed)<400,depth_open=depth_rows.load(std::memory_order_relaxed)<600;
    if(!keeper_open && !depth_open)return kept;
    try{uintptr_t glare{},slots{};int32_t count{-1},n{};float luminance{};BYTE skip{};
        Read(library+0x108,&glare,8);if(glare)Read(glare+0x1C0,&count,4);Read(library+0x378,&n,4);Read(library+0x6B0,&slots,8);
        Read(library+0x6A4,&luminance,4);Read(library+0x6DE,&skip,1);
        static std::mutex lock;
        if(keeper_open){std::string cells;
        for(int32_t i=0;slots && i<n && i<2;++i){float colour[4]{},depth{};BYTE enable{};int32_t space{};
            Read(slots+uint64_t(i)*0x58,colour,16);Read(slots+uint64_t(i)*0x58+0x4C,&depth,4);Read(slots+uint64_t(i)*0x58+0x50,&enable,1);
            Read(slots+uint64_t(i)*0x58+0x54,&space,4);
            char cell[200]{};sprintf_s(cell,"%s{\"colour\":[%.3g,%.3g,%.3g,%.3g],\"enable\":%u,\"depth\":%.4g,\"space\":%d}",i?",":"",colour[0],colour[1],colour[2],colour[3],enable,depth,space);cells+=cell;}
        char row[700]{};sprintf_s(row,"{\"event\":\"reference_shaft_keeper\",\"S\":\"%llX\",\"kept\":%d,\"slots\":%d,\"luminance\":%.4g,\"skip\":%u,\"ret\":%d,\"slot\":[%s]}\n",
            static_cast<unsigned long long>(library),count,n,luminance,skip,int(kept),cells.c_str());
        static std::map<uintptr_t,std::string> last;
        bool fresh{};
        {const std::lock_guard hold(lock);fresh=keeper_rows.load(std::memory_order_relaxed)<400 && last[library]!=row;if(fresh){last[library]=row;keeper_rows.fetch_add(1,std::memory_order_relaxed);}}
        if(fresh)Log(row);}
        if(!depth_open)return kept;
        // The depth words Alpha_R and the record's depth read (yebis_shafts.h DepthSpaces, AlphaRInputs): near and far +0x564/+0x568,
        // the source space +0x43C, the DoF branch byte +0xC33 and the conversion words +0xC38..+0xC48, the focus +0x6C0, the depth
        // source's (scale, bias) +0x448/+0x44C and its objects +0x430/+0x50, the colour words +0x454/+0x6E8/+0x69C/+0x698; then the
        // kept record 0 at [[S+0x108]+0x1C8] (0x50 bytes, FUN_141752DB0/E60/EF0), when one was kept.
        float near_far[2]{},conv[5]{},focus{},source[2]{},colour_words[3]{};int32_t space{};BYTE dof{},tone{};uintptr_t objects[2]{};
        Read(library+0x564,near_far,8);Read(library+0x43C,&space,4);Read(library+0xC33,&dof,1);Read(library+0xC38,conv,20);Read(library+0x6C0,&focus,4);
        Read(library+0x448,source,8);Read(library+0x430,&objects[0],8);Read(library+0x50,&objects[1],8);Read(library+0x454,&tone,1);
        Read(library+0x6E8,&colour_words[0],4);Read(library+0x69C,&colour_words[1],4);Read(library+0x698,&colour_words[2],4);
        float record[20]{};uintptr_t records{};if(glare && count>0 && Read(glare+0x1C8,&records,8) && records)Read(records,record,sizeof(record));
        std::string words;for(int k=0;k<20;++k){char w[24]{};sprintf_s(w,"%s%.4g",k?",":"",record[k]);words+=w;}
        char depth_row[900]{};sprintf_s(depth_row,"{\"event\":\"reference_shaft_depth\",\"S\":\"%llX\",\"near\":%.4g,\"far\":%.6g,\"space\":%d,\"dof\":%u,\"conv\":[%.5g,%.5g,%.5g,%.5g,%.5g],\"focus\":%.4g,"
            "\"source\":[%.4g,%.4g],\"objects\":[%d,%d],\"tone\":%u,\"colour_words\":[%.4g,%.4g,%.4g],\"record\":[%s]}\n",
            static_cast<unsigned long long>(library),near_far[0],near_far[1],space,dof,conv[0],conv[1],conv[2],conv[3],conv[4],focus,source[0],source[1],
            objects[0]?1:0,objects[1]?1:0,tone,colour_words[0],colour_words[1],colour_words[2],words.c_str());
        // One row when a word before the record changes, else one every 60 calls (the record follows the camera every frame).
        const std::string head(depth_row,strstr(depth_row,"\"record\"")?strstr(depth_row,"\"record\""):depth_row);
        static std::string last_depth;
        static int depth_calls{};
        bool fresh_depth{};
        {const std::lock_guard hold(lock);fresh_depth=depth_rows.load(std::memory_order_relaxed)<600 && (last_depth!=head || ++depth_calls%60==0);
            if(fresh_depth){last_depth=head;depth_rows.fetch_add(1,std::memory_order_relaxed);}}
        if(fresh_depth)Log(depth_row);
    }catch(...){}
    return kept;
}
// I-0141 (45000030's veil, its cause): the game's camera gives the library a focus distance of its own. READ (GhidraMCP 2026-11-15):
// the camera update FUN_14062ddf0, with its byte +0xC1 set, calls FUN_140017ea0(use, distance) with the camera's own distance (+0xC4,
// or +0xD0 when its +0xCC is set, `use` then 0), which writes the library's post object's byte +0x2271
// ([[144961B50+0x4DD8]+0xCB8]+0x2271: the record's distance used or not) and, when `use` is 0, the focus distance 144962120 (and
// 144961DB0); the DoF converter E4890 writes the record's word 2 there only when +0x2271 is set. OBSERVED (i141-t14a, both modes): the
// camera in play asks it too (about 3.08 m, the original's S+0x6C0 3.07 to 3.08 in play) and a cutscene's camera its own (45000030:
// 3.8 to 3.64 m, then 4.9). Natively the library's post object is not made, so FUN_140017ea0 wrote nothing and the converters' scratch
// object held +0x2271 at 1: the record's distance (1 m in play, 0 in 45000030's first cut). The detour keeps what the game asks
// (CollectNativeYebisInputs gives it to the scratch object and the focus global) and calls the original.
using ExternalFocusFn=void(__fastcall*)(char,float);   // READ: the distance in XMM1 (140017EC4)
ExternalFocusFn external_focus_original{};
std::atomic<uint64_t> native_external_focus{uint64_t(1)<<32};   // (use the record's distance << 32) | the distance's bits; the post object's 1 by default
void __fastcall NativeExternalFocus(char use_record,float distance) noexcept {
    uint32_t bits{};std::memcpy(&bits,&distance,4);native_external_focus.store((uint64_t(use_record?1u:0u)<<32)|bits,std::memory_order_release);
    external_focus_original(use_record,distance);
}
// I-0141 (45000030's focus): the DoF record's blend DepthOfFieldParam_Blend_14021e720(out, A, B, w) (READ: words {0, 8..13, 18, 24,
// 25} from B, the others lerped; word 2 DofFocusDistance), observed in the original with shaft-observe.flag: A's, B's and the blend's
// word 2, the weight, the Enable words 12; one row at each change, at most 400.
using DofBlendFn=void*(__fastcall*)(void*,const void*,const void*,float);
DofBlendFn dof_blend_original{};
void* __fastcall ObserveDofBlend(void* out,const void* a,const void* b,float w) noexcept {
    void* const r=dof_blend_original(out,a,b,w);
    static std::atomic<int> rows{};if(rows.load(std::memory_order_relaxed)>=400)return r;   // the cap first: nothing more done past it
    try{float a2{},b2{},o2{};uint32_t ea{},eb{},auto_focus{};
        Read(reinterpret_cast<uintptr_t>(a)+8,&a2,4);Read(reinterpret_cast<uintptr_t>(b)+8,&b2,4);Read(reinterpret_cast<uintptr_t>(out)+8,&o2,4);
        Read(reinterpret_cast<uintptr_t>(a)+48,&ea,4);Read(reinterpret_cast<uintptr_t>(b)+48,&eb,4);Read(reinterpret_cast<uintptr_t>(out)+96,&auto_focus,4);
        // One row at each change of the words (the weight left out: it moves every frame of a fade).
        char row[240]{};sprintf_s(row,"{\"event\":\"reference_dof_blend\",\"a2\":%.4g,\"b2\":%.4g,\"word2\":%.4g,\"enable\":[%u,%u],\"auto\":%u",a2,b2,o2,ea,eb,auto_focus);
        static std::mutex lock;static std::string last;bool fresh{};
        {const std::lock_guard hold(lock);fresh=last!=row;if(fresh){last=row;rows.fetch_add(1,std::memory_order_relaxed);}}
        if(fresh){char weight[48]{};sprintf_s(weight,",\"weight\":%.4g}\n",w);Log((std::string(row)+weight).c_str());}
    }catch(...){}
    return r;
}
// I-0138 tranche 8h (the cut starts' default record: natively the view context's post keys are all 0xFFFFFFFF on a cut's first
// frames, three or four collects a cut in i138-t8h2, where the original's FB890 never meets the default): the keys the post feed
// reads, in both modes, at three places, test tooling behind shaft-observe.flag: « apply » after
// AreaControllerSet_PerFrameApply_140d52b70(set, context) (READ: the writer of the whole set, called by E006A0 in the original and by
// UpdateNativeSceneCpu natively; FUN_1400884f0, called after it for group 2 when stage+0x510 is set and also from 140D25295, writes one
// group more and is not seen by this row; the list of writers not exhaustive), with the set's tokens +0x190/+0x194, the tokens it compares them to ([1446402E0]/[1446402E4]) and its single
// key +0x188; « fb890 » at the entry of the original's
// FUN_1400FB890 (param_5 the context; FCE30 calls it for the pass 0x45); « collect » at CollectNativeYebisInputs's read. Each row:
// the context, FB890's gate (its +0x50 bit 10, [144961F70]), the ten groups' keys A +0x3FD8, B +0x4040 and weights +0x40A8, the
// calling thread, GetTickCount64 and the Remo clock's tag. A row at each change of a site's (context, gate, keys) (the weights left
// out: they move every frame of a fade, a row when one has moved by 0.05 since the last), on every call of the eight after the Remo
// tag's cut changes (the cut starts' window), and every 300 calls; at most 2,000 a site.
void LogPostKeys(int site,uintptr_t context,uintptr_t set) noexcept {
    static const bool observe=ModuleFlag(L"shaft-observe.flag");
    if(!observe || !context || site<0 || site>2)return;
    static const char* const names[3]{"apply","fb890","collect"};
    struct Site {std::string last;float weights[10]{};uint64_t calls{};int rows{};int window{};int64_t cut{-2};};
    static std::mutex lock;static Site sites[3];
    try{uint32_t keys[10][5]{};
        for(uint32_t i=0;i<10;++i){Read(context+0x3FD8+8ull*i,keys[i],8);Read(context+0x4040+8ull*i,keys[i]+2,8);Read(context+0x40A8+4ull*i,keys[i]+4,4);}
        uint32_t flags{},library{};const bool gate_read=Read(context+0x50,&flags,4) && Read(image+0x4961F70,&library,4);
        std::string signature;char cell[256]{};   // the set's cell is about 160 characters at most (sprintf_s ends the process on an overflow)
        sprintf_s(cell,"\"context\":\"%llX\",\"gate\":[%d,%d]",static_cast<unsigned long long>(context),gate_read?int((flags>>10)&1u):-1,gate_read?int(library!=0):-1);signature=cell;
        // The original's draw gate on this call (14 ReadNativeDrawGate): the root's +0xCA, the timing object's +0x27B and +0x278 (-1 unread);
        // I-0138 tranche 8i: the limiter's catch-up window (the draw-gate reading, Q5: its mode +8 and +0xC, the in-focus window +0x270,
        // the out-of-focus one +0x274) and whether DS3's window is the foreground one (its choice between the two at 140EED88E).
        {uintptr_t root{},timing{},window{};HWND hwnd{};BYTE bytes[3]{};int draw_gate[3]{-1,-1,-1};int32_t limiter[4]{-1,-1,-1,-1};
            if(Read(image+0x4793348,&root,8) && root && Read(root+0xCA,&bytes[0],1))draw_gate[0]=bytes[0];
            if(Read(image+0x489DD10,&timing,8) && timing){if(Read(timing+0x27B,&bytes[1],1))draw_gate[1]=bytes[1];if(Read(timing+0x278,&bytes[2],1))draw_gate[2]=bytes[2];
                Read(timing+8,&limiter[0],4);Read(timing+0xC,&limiter[1],4);Read(timing+0x270,&limiter[2],4);Read(timing+0x274,&limiter[3],4);}
            const int focus=Read(image+0x489F118,&window,8) && window && Read(window+8,&hwnd,8) && hwnd?int(GetForegroundWindow()==hwnd):-1;
            sprintf_s(cell,",\"draw_gate\":[%d,%d,%d],\"limiter\":[%d,%d,%d,%d],\"focus\":%d",draw_gate[0],draw_gate[1],draw_gate[2],
                limiter[0],limiter[1],limiter[2],limiter[3],focus);signature+=cell;}
        if(set){uint32_t tokens[2]{},expected[2]{},single[2]{};const bool read=Read(set+0x190,tokens,8) && Read(image+0x46402E0,expected,8) && Read(set+0x188,single,8);
            sprintf_s(cell,",\"set\":\"%llX\",\"tokens\":[%u,%u],\"expected\":[%u,%u],\"single\":[%u,%u],\"set_read\":%s",static_cast<unsigned long long>(set),tokens[0],tokens[1],
                expected[0],expected[1],single[0],single[1],read?"true":"false");signature+=cell;}
        signature+=",\"keys\":[";
        for(uint32_t i=0;i<10;++i){sprintf_s(cell,"%s[%u,%u,%u,%u]",i?",":"",keys[i][0],keys[i][1],keys[i][2],keys[i][3]);signature+=cell;}
        signature+="]";
        const uint64_t tag=cutscene_clock_tag.load(std::memory_order_relaxed);const int64_t cut=(tag>>63)?int64_t((tag>>32)&0xFFFF):-1;
        uint64_t call{};bool emit{};
        {const std::lock_guard hold(lock);Site& s=sites[site];call=++s.calls;
            if(cut!=s.cut){s.cut=cut;s.window=8;}
            const bool in_window=s.window>0;if(in_window)--s.window;
            bool moved{};for(uint32_t i=0;i<10;++i){float w{};std::memcpy(&w,&keys[i][4],4);if(std::isfinite(w) && std::fabs(w-s.weights[i])>=0.05f)moved=true;}
            emit=s.rows<2000 && (signature!=s.last || in_window || moved || call%300==0);
            if(emit){s.last=signature;++s.rows;for(uint32_t i=0;i<10;++i){float w{};std::memcpy(&w,&keys[i][4],4);s.weights[i]=std::isfinite(w)?w:0.f;}}}
        if(!emit)return;
        std::string weights;for(uint32_t i=0;i<10;++i){float w{};std::memcpy(&w,&keys[i][4],4);sprintf_s(cell,"%s%.4g",i?",":"",std::isfinite(w)?w:-1.0f);weights+=cell;}
        char head[160]{};sprintf_s(head,"{\"event\":\"post_keys\",\"site\":\"%s\",\"call\":%llu,\"thread\":%lu,\"tick\":%llu,",names[site],static_cast<unsigned long long>(call),
            GetCurrentThreadId(),static_cast<unsigned long long>(GetTickCount64()));
        Log((std::string(head)+signature+",\"weights\":["+weights+"]"+CutsceneClockCell()+"}\n").c_str());
    }catch(...){}
}
using AreaSetApplyFn=void(__fastcall*)(uintptr_t,uintptr_t);
AreaSetApplyFn area_set_apply_original{};
void __fastcall ObserveAreaSetApply(uintptr_t set,uintptr_t context) noexcept {
    area_set_apply_original(set,context);LogPostKeys(0,context,set);
}
using PostFeedFn=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
PostFeedFn post_feed_original{};
void __fastcall ObservePostFeed(uintptr_t p1,uintptr_t p2,uintptr_t p3,uintptr_t p4,uintptr_t context,uintptr_t p6,uintptr_t p7,uintptr_t root) noexcept {
    LogPostKeys(1,context,0);post_feed_original(p1,p2,p3,p4,context,p6,p7,root);
}
// The instance ambient rows of a light key: the engine's own 1495F0 (a pure function of the set's ambient words +0x1E0..+0x22F,
// the three rows of (colour difference, colour sum) the lookup record keeps at +0x290, written by 14BA90 when the engine builds a
// record from a file) on the record built from the native banks, as NativeGparamRecords builds it for cbLight. Keyed by (area,
// id) and the banks' binding generation (the bank object and its layers change at a zone change), since the collect asks it for
// every admitted model.
// I-0113 (step review L1): a key {area, -1} keeps the banks here, unlike NativeGparamRecords: the Handmaid chair's instance rows
// equal the original's to the digit with it (i113-chair2: native_instance_rows = reference_instance_rows).
bool NativeInstanceLightRows(const uint32_t* key,std::array<float,12>& rows) {
    std::shared_ptr<const ds3light::Banks> banks;{const std::lock_guard hold(gparam_lock);banks=gparam_banks;}
    if(!banks)return false;
    const bool fallback=key[0]==0xFFFFFFFFu && key[1]==0xFFFFFFFFu;
    if(!fallback && key[0]<1)return false;   // the engine's own records (natively the default): left to the engine's lookup
    const uint32_t area=fallback?1:key[0],id=fallback?0:key[1];
    // The bank object held (review: a reloaded bank at the same address must not reuse the rows) and its binding generation,
    // which SelectVariant bumps too. Optimisation review: the collect asks twice per admitted model (Native2276 at m45: 3,500
    // calls a collect); each thread keeps its own answers, without a lock (the former shared cache took one per call).
    const uint64_t generation=banks->RecordGeneration(),cache_key=(uint64_t(area)<<32)|id;   // I-0141 tranche 7: the fades' steps too
    // A weak hold: an unexpired one is the same bank object (no thread keeps a released bank alive).
    thread_local struct {std::weak_ptr<const ds3light::Banks> banks;const void* object{};uint64_t generation{};std::unordered_map<uint64_t,std::array<float,12>> rows;} local;
    if(local.object!=banks.get() || local.banks.expired() || local.generation!=generation){local.rows.clear();local.banks=banks;local.object=banks.get();local.generation=generation;}
    // I-0141 tranche 7: a fading area's rows change at each fade step, which renews the generation (Banks::AdvanceFades).
    if(const auto found=local.rows.find(cache_key);found!=local.rows.end()){rows=found->second;return true;}
    alignas(16) std::array<uint8_t,0x2C0> record{};
    if(!banks->Sun(area,id,record.data()+0x40,0x260))return false;
    const auto adjust=reinterpret_cast<void(__fastcall*)(const void*,float*)>(image+0x1495F0);
    alignas(16) float out[16]{};adjust(record.data(),out);std::copy_n(out,12,rows.begin());   // review: an aligned output, as the engine's record +0x290
    if(!std::all_of(rows.begin(),rows.end(),[](float v){return std::isfinite(v);}))return false;
    if(local.rows.size()<4096)local.rows[cache_key]=rows;return true;
}
void CollectNativeLightBlocks(NativeSelectedScene& scene,const std::optional<std::array<uint32_t,2>>& fixed_key) {
    uintptr_t root{},entries{},banks{},light_bank{},fog_bank{};int32_t count{};
    if(!Read(image+0x4961B50,&root,8) || !root)return;
    // 1051 witness: the light container [[root]+41D0] (vector +10..+18) that
    // FCE30 walks for the point/spot light chain (lighting map). Read only.
    {static LONG64 container_witness{};const auto n=InterlockedIncrement64(&container_witness);
        if(n==1 || (n%120==0 && n<=1800)){uintptr_t container{},first{},last{};
            if(Read(root+0x41D0,&container,8) && container && Read(container+0x10,&first,8) && Read(container+0x18,&last,8)){
                const size_t lights=last>=first?static_cast<size_t>((last-first)/8):0;std::string rows;
                for(size_t i=0;i<lights && i<8;++i){uintptr_t light{},vtable{};uint32_t mask{},type{};BYTE enabled{};float position[4]{};
                    if(!Read(first+8*i,&light,8) || !light || !Read(light,&vtable,8))continue;
                    std::array<uintptr_t,7> methods{};Read(vtable,methods.data(),sizeof(methods));
                    Read(light+0xA0,&mask,4);Read(light+0xB0,&enabled,1);Read(light+0x1C,&type,4);Read(light+0x20,position,16);
                    char item[320]{};sprintf_s(item,"%s{\"vtable_rva\":%llu,\"mask\":%u,\"enabled\":%u,\"word1c\":%u,\"f20\":[%.4g,%.4g,%.4g,%.4g],\"m10\":%llu,\"m18\":%llu,\"m30\":%llu}",
                        rows.empty()?"":",",static_cast<unsigned long long>(vtable-image),mask,enabled,type,position[0],position[1],position[2],position[3],
                        static_cast<unsigned long long>(methods[2]-image),static_cast<unsigned long long>(methods[3]-image),static_cast<unsigned long long>(methods[6]-image));rows+=item;}
                std::string row="{\"event\":\"native_light_container\",\"count\":"+std::to_string(lights)+",\"lights\":["+rows+"]}\n";Log(row.c_str());}
            else Log("{\"event\":\"native_light_container\",\"available\":false}\n");}}
    const uintptr_t object=root+0x41F8;
    if(!Read(object+0x90,&count,4) || !Read(object+0x88,&entries,8) || !Read(object+0x98,&banks,8) || !banks ||
       !Read(banks+0x20,&light_bank,8) || !Read(banks+0x28,&fog_bank,8) || !light_bank || !fog_bank){
        static LONG64 unreadable{};if(InterlockedIncrement64(&unreadable)<=2)Log("{\"event\":\"native_light_entries_unavailable\",\"reason\":\"banks\"}\n");return;}
    static LONG64 samples{};const bool sample=InterlockedIncrement64(&samples)%600==1;
    std::vector<std::array<uint32_t,12>> keys;bool engine_entries=false;
    if(fixed_key){
        std::array<uint32_t,12> entry{};
        for(const size_t at:{2u,4u,6u,8u}){entry[at]=(*fixed_key)[0];entry[at+1]=(*fixed_key)[1];}
        keys.push_back(entry);
    }else if(scene.light_keys.empty() && count>0 && count<=225 && entries){   // the engine's own entries (natively empty)
        engine_entries=true;keys.resize(static_cast<size_t>(count));
        for(int32_t k=0;k<count;++k)if(!Read(entries+0x30ull*k,keys[static_cast<size_t>(k)].data(),0x30))return;
    }else{
        // D-0180 C4: every registered set (NativeLightSetIndex), in index order: the drawn models' instance envData index them.
        std::array<uint32_t,16> witness{};bool witnessed=false;
        for(const auto& prepared:scene.prepared_meshes){const auto group=scene.models.at(prepared.model).group;
            if(group && Read(group+0xF0,witness.data(),sizeof(witness))){witnessed=true;break;}}
        for(const auto& key:scene.light_keys){std::array<uint32_t,12> entry{};entry[1]=static_cast<uint32_t>(keys.size());
            std::copy(key.begin(),key.end(),entry.begin()+2);keys.push_back(entry);}
        static LONG64 group_witness{};
        if(witnessed && [&]{const auto n=InterlockedIncrement64(&group_witness);return n<=2 || n%600==0;}()){char row[320]{};sprintf_s(row,"{\"event\":\"native_light_group_keys\",\"unique\":%zu,\"group_f0\":[%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u]}\n",keys.size(),
            witness[0],witness[1],witness[2],witness[3],witness[4],witness[5],witness[6],witness[7],witness[8],witness[9],witness[10],witness[11],witness[12],witness[13],witness[14],witness[15]);Log(row);}
    }
    if(keys.empty()){
        if(sample)Log("{\"event\":\"native_light_entries_unavailable\",\"reason\":\"no_selected_keys\"}\n");
        return;}
    while(keys.size()<2)keys.push_back(keys.front());
    alignas(16) std::array<uint8_t,0x80> view{};
    std::memcpy(view.data(),scene.camera.camera_pose.data(),64);
    const float lens[3]{scene.camera.vertical_fov,scene.camera.near_plane,scene.camera.far_plane};
    std::memcpy(view.data()+0x50,lens,12);std::memcpy(view.data()+0x5C,&scene.camera.width,4);std::memcpy(view.data()+0x60,&scene.camera.height,4);
    std::memcpy(view.data()+0x64,&scene.camera.explicit_aspect,4);std::memcpy(view.data()+0x68,&scene.camera.projection_flags,4);
    std::memcpy(view.data()+0x70,&scene.camera.tile_count,4);
    const auto sun=reinterpret_cast<void(__fastcall*)(void*,uintptr_t,const void*)>(image+0x1496D0);
    const auto sun_blend=reinterpret_cast<void(__fastcall*)(void*,const void*,uintptr_t,uintptr_t,float)>(image+0x14A2D0);
    const auto fog=reinterpret_cast<void(__fastcall*)(void*,const void*,const void*)>(image+0x14E900);
    const auto fog_blend=reinterpret_cast<void(__fastcall*)(void*,uintptr_t,uintptr_t,float)>(image+0x14E5D0);
    NativeSelectedScene::LightBlocks blocks;if(keys.size()>225)keys.resize(225);blocks.light.assign(0x120*keys.size(),0);blocks.fog.assign(0x60*keys.size(),0);NativeGparamRecords records;std::vector<size_t> failed_light,failed_fog;
    for(size_t k=0;k<keys.size();++k){
        const auto& entry=keys[k];
        float light_t{},fog_t{};std::memcpy(&light_t,&entry[10],4);std::memcpy(&fog_t,&entry[11],4);
        if(entry[2]==entry[4] && entry[3]==entry[5]){
            const auto set=records.Light(light_bank,&entry[2]);if(!set){if(!k)return;failed_light.push_back(k);failed_fog.push_back(k);continue;}
            if(sample && k<12){{float f[64]{};std::memcpy(f,reinterpret_cast<const void*>(set+0x40),sizeof f);
                std::string record="{\"event\":\"native_light_set\",\"entry\":"+std::to_string(k)+",\"key\":["+std::to_string(entry[2])+","+std::to_string(entry[3])+"],\"f\":[";for(int i=0;i<64;++i){char v[24]{};sprintf_s(v,"%s%.6g",i?",":"",f[i]);record+=v;}
                record+="]}\n";Log(record.c_str());}}
            sun(blocks.light.data()+0x120*k,set+0x40,view.data());
        }else{
            const auto a=records.Light(light_bank,&entry[2]),b=records.Light(light_bank,&entry[4]);if(!a || !b){if(!k)return;failed_light.push_back(k);failed_fog.push_back(k);continue;}
            sun_blend(blocks.light.data()+0x120*k,view.data(),a,b,light_t);
        }
        alignas(16) std::array<uint8_t,0x60> set{};
        if(entry[6]==entry[8] && entry[7]==entry[9]){
            const auto record=records.Fog(fog_bank,&entry[6]);if(!record){if(!k)return;failed_fog.push_back(k);continue;}
            if(sample && k<12){float f[24]{};std::memcpy(f,reinterpret_cast<const void*>(record+0x10),sizeof f);
                std::string row="{\"event\":\"native_fog_set\",\"entry\":"+std::to_string(k)+",\"key\":["+std::to_string(entry[6])+","+std::to_string(entry[7])+"],\"f\":[";
                for(int i=0;i<24;++i){char v[24]{};sprintf_s(v,"%s%.6g",i?",":"",f[i]);row+=v;}row+="]}\n";Log(row.c_str());}
            fog(blocks.fog.data()+0x60*k,reinterpret_cast<const void*>(record+0x10),view.data());
        }else{
            const auto a=records.Fog(fog_bank,&entry[6]),b=records.Fog(fog_bank,&entry[8]);if(!a || !b){if(!k)return;failed_fog.push_back(k);continue;}
            if(0.f<fog_t)fog_blend(set.data(),a+0x10,b+0x10,fog_t);else if(!Read(a+0x10,set.data(),set.size())){if(!k)return;failed_fog.push_back(k);continue;}
            fog(blocks.fog.data()+0x60*k,set.data(),view.data());
        }
    }
    // I-0141: the sets of a cutscene's own file (their lights 1 to 3 kept on a traced frame; light_producers.h CutsceneLightSet), and
    // since tranche 5 (D-0282) a map key the cutscene act overrides by a cutscene file's set (NativeGparamRecords::Authored).
    blocks.authored.assign(keys.size(),0);
    for(size_t k=0;k<keys.size();++k)blocks.authored[k]=records.Authored(keys[k])?1:0;
    // Review: a set whose record fails takes set 0's block (never a zero set: its models would be unlit and unfogged), logged.
    for(const auto k:failed_light){std::memcpy(blocks.light.data()+0x120*k,blocks.light.data(),0x120);blocks.authored[k]=blocks.authored[0];}
    for(const auto k:failed_fog)std::memcpy(blocks.fog.data()+0x60*k,blocks.fog.data(),0x60);
    // I-0141 tranche 14: the whole native array every 5 s beside the original's (OriginalFogArrayBefore, fog-build-observe.flag); a
    // menu view's fixed key is not the world's array and is left out.
    {static const bool observe=ModuleFlag(L"fog-build-observe.flag");static double next_ms{};static int rows{};
        if(observe && !fixed_key && rows<300){const double ms=FogArrayClockMs();if(ms>=next_ms){next_ms=ms+5000.0;++rows;float camera[3]{};std::memcpy(camera,view.data()+0x30,sizeof camera);
            std::vector<uint32_t> flat;for(const auto& entry:keys)flat.insert(flat.end(),entry.begin(),entry.end());
            const auto list=[](const auto& at){std::string out="[";for(size_t i=0;i<at.size();++i)out+=(i?",":"")+std::to_string(at[i]);return out+"]";};
            Log(FogArrayRow("native",ms,camera,keys.size(),flat.data(),keys.size(),blocks.fog.data(),blocks.light.data(),blocks.light.size()/0x120,
                            ",\"light_failed\":"+list(failed_light)+",\"fog_replaced\":"+list(failed_fog)).c_str());}}}
    if(!failed_light.empty() || !failed_fog.empty()){static LONG64 failed_rows{};if(InterlockedIncrement64(&failed_rows)<=8){char row[160]{};
        sprintf_s(row,"{\"event\":\"native_light_sets_failed\",\"light\":%zu,\"fog\":%zu,\"sets\":%zu}\n",failed_light.size(),failed_fog.size(),keys.size());Log(row);}}
    static LONG64 block_witness{};
    if([&]{const auto n=InterlockedIncrement64(&block_witness);return n<=2 || n%600==0;}()){
        float sun_head[8]{},fog_head[4]{};std::memcpy(sun_head,blocks.light.data(),32);std::memcpy(fog_head,blocks.fog.data(),16);
        char row[700]{};sprintf_s(row,"{\"event\":\"native_light_blocks\",\"engine_entries\":%s,\"unique\":%zu,\"keys\":[[%u,%u,%u,%u,%u,%u,%u,%u],[%u,%u,%u,%u,%u,%u,%u,%u]],"
            "\"sun0\":[%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g],\"fog0\":[%.4g,%.4g,%.4g,%.4g]}\n",
            engine_entries?"true":"false",keys.size(),keys[0][2],keys[0][3],keys[0][4],keys[0][5],keys[0][6],keys[0][7],keys[0][8],keys[0][9],
            keys[1][2],keys[1][3],keys[1][4],keys[1][5],keys[1][6],keys[1][7],keys[1][8],keys[1][9],
            sun_head[0],sun_head[1],sun_head[2],sun_head[3],sun_head[4],sun_head[5],sun_head[6],sun_head[7],fog_head[0],fog_head[1],fog_head[2],fog_head[3]);Log(row);
        std::string all;for(size_t k=0;k<keys.size() && k<12;++k){char cell[160]{};sprintf_s(cell,"%s[%u,%u,%u,%u,%u,%u,%u,%u]",k?",":"",keys[k][2],keys[k][3],keys[k][4],keys[k][5],
            keys[k][6],keys[k][7],keys[k][8],keys[k][9]);all+=cell;}
        std::string block;for(size_t i=0;i<72;++i){float v{};std::memcpy(&v,blocks.light.data()+4*i,4);char cell[24]{};sprintf_s(cell,"%s%.6g",i?",":"",v);block+=cell;}
        const std::string keys_row="{\"event\":\"native_light_entries\",\"count\":"+std::to_string(keys.size())+",\"keys\":["+all+"],\"f\":["+block+"]}\n";Log(keys_row.c_str());
        // I-0129 R9d: each set's fill lights (the first 12 sets): rows 1 to 3 (Vec), 5 to 7 (Col), 16 (bLocalLight) and 17 (FC_SsaoWeight).
        // Written whatever the flag, beside native_light_entries and at its rate (the step review's L1: about 4 KB a row, kept for the fill's study).
        std::string fill;for(size_t k=0;k<keys.size() && k<12 && 288*(k+1)<=blocks.light.size();++k){fill+=k?",[":"[";
            size_t cells=0;for(const size_t fill_row_at:{1,2,3,5,6,7,16,17})for(size_t c=0;c<4;++c){float v{};std::memcpy(&v,blocks.light.data()+288*k+16*fill_row_at+4*c,4);
                char cell[24]{};uint32_t bits{};std::memcpy(&bits,&v,4);   // the step review's L2: row 16 (bLocalLight) as the integers the shader reads
                if(fill_row_at==16)sprintf_s(cell,"%s%u",cells++?",":"",bits);else sprintf_s(cell,"%s%.4g",cells++?",":"",v);fill+=cell;}
            fill+="]";}
        const std::string fill_row="{\"event\":\"native_light_fill\",\"rows\":[1,2,3,5,6,7,16,17],\"sets\":["+fill+"]}\n";Log(fill_row.c_str());}
    {if(scene.options && scene.options->object_valid)blocks.shade_sss=scene.options->object[0x75]!=0;
        static std::atomic<int> last_gate{-1};if(last_gate.exchange(int(blocks.shade_sss))!=int(blocks.shade_sss)){
            Log(blocks.shade_sss?"{\"event\":\"native_shade_sss_gate\",\"sss\":true}\n":"{\"event\":\"native_shade_sss_gate\",\"sss\":false}\n");}}
    scene.light_blocks=std::move(blocks);
    scene.forward_light=NativeSelectedScene::ForwardLightContext{light_bank,fog_bank,{},fixed_key};std::memcpy(scene.forward_light->view.data(),view.data(),view.size());
}
// Forward pass (1114): the Fwd programmes declare cbLight (b5) as one 0x120 entry
// (FC_DirLightVec/Col/Spc_0..3, FC_AmbientSH[3], FC_AmbientReflection, FC_bLocalLight,
// FC_SsaoWeight) and cbFog (b6) beginning with one 0x60 entry (FogParam, FogCol,
// ExtFogParam, HeightFogParam, HeightFogCol, FogType): the same entries 1496D0/14A2D0
// and 14E900/14E5D0 build for the deferred arrays. Built here from the model's own
// group key (+F8/+110/+108/+120), as the deferred fallback does.
// I-0125 R6d (its final review): `light` null makes the fog entry alone (the rays' forward instances: their light records still
// looked up, so a group refused for its light is refused for its fog too, as the raster's forward rows).
// The group's keys and blend factors the forward entries are built from (words 2 to 11; 0 and 1 stay zero): the light keys at
// +0xF8, the fog keys at +0x110, the light and fog factors at +0x108 and +0x120, or the scene's fixed key.
bool NativeForwardLightKey(const NativeSelectedScene& scene,uintptr_t group,std::array<uint32_t,12>& entry) {
    entry={};if(!scene.forward_light || !group)return false;
    if(scene.forward_light->fixed_key){for(const size_t at:{2u,4u,6u,8u}){entry[at]=(*scene.forward_light->fixed_key)[0];entry[at+1]=(*scene.forward_light->fixed_key)[1];}return true;}
    return Read(group+0xF8,&entry[2],16) && Read(group+0x110,&entry[6],16) && Read(group+0x108,&entry[10],4) && Read(group+0x120,&entry[11],4);
}
bool ProduceNativeForwardLight(const NativeSelectedScene& scene,const std::array<uint32_t,12>& entry,std::vector<uint8_t>* light,std::vector<uint8_t>& fog,bool* authored=nullptr) {
    if(!scene.forward_light)return false;
    const auto& context=*scene.forward_light;const void* view=context.view.data();
    const auto sun=reinterpret_cast<void(__fastcall*)(void*,uintptr_t,const void*)>(image+0x1496D0);
    const auto sun_blend=reinterpret_cast<void(__fastcall*)(void*,const void*,uintptr_t,uintptr_t,float)>(image+0x14A2D0);
    const auto fog_build=reinterpret_cast<void(__fastcall*)(void*,const void*,const void*)>(image+0x14E900);
    const auto fog_blend=reinterpret_cast<void(__fastcall*)(void*,uintptr_t,uintptr_t,float)>(image+0x14E5D0);
    float light_t{},fog_t{};std::memcpy(&light_t,&entry[10],4);std::memcpy(&fog_t,&entry[11],4);NativeGparamRecords records;
    alignas(16) std::array<uint8_t,0x120> light_entry{};alignas(16) std::array<uint8_t,0x60> fog_entry{},set{};
    if(entry[2]==entry[4] && entry[3]==entry[5]){
        const auto found=records.Light(context.light_bank,&entry[2]);if(!found)return false;
        if(light)sun(light_entry.data(),found+0x40,view);
    }else{
        const auto a=records.Light(context.light_bank,&entry[2]),b=records.Light(context.light_bank,&entry[4]);if(!a || !b)return false;
        if(light)sun_blend(light_entry.data(),view,a,b,light_t);
    }
    if(entry[6]==entry[8] && entry[7]==entry[9]){
        const auto record=records.Fog(context.fog_bank,&entry[6]);if(!record)return false;
        fog_build(fog_entry.data(),reinterpret_cast<const void*>(record+0x10),view);
    }else{
        const auto a=records.Fog(context.fog_bank,&entry[6]),b=records.Fog(context.fog_bank,&entry[8]);if(!a || !b)return false;
        if(0.f<fog_t)fog_blend(set.data(),a+0x10,b+0x10,fog_t);else if(!Read(a+0x10,set.data(),set.size()))return false;
        fog_build(fog_entry.data(),set.data(),view);
    }
    if(light)light->assign(light_entry.begin(),light_entry.end());
    if(authored)*authored=records.Authored(entry);   // I-0141 tranche 5: from the records' own banks (the step review's L1)
    fog.assign(fog_entry.begin(),fog_entry.end());return true;
}
bool ProduceNativeForwardLight(const NativeSelectedScene& scene,uintptr_t group,std::vector<uint8_t>* light,std::vector<uint8_t>& fog) {
    std::array<uint32_t,12> entry;return NativeForwardLightKey(scene,group,entry) && ProduceNativeForwardLight(scene,entry,light,fog);
}
}  // namespace
