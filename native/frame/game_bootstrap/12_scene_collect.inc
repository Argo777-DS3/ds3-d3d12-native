// The selected scene's collect: YEBIS, velocity and lighting inputs, forward lights, shadow casters, decal and paint
// snapshots, the water's plane and CollectNativeSelectedScene. Part of game_bootstrap.cpp's single translation unit (the
// order is there): it relies on every earlier part (10_options_and_passes.inc before it).
#pragma once
#include "10_options_and_passes.inc"
namespace {
alignas(16) std::array<uint8_t,0x2400> native_yebis_post{};   // the converters' post object, zeroed at each use (YB-4)
// W10 YB-3/YB-4 (YEBIS-WAVE1-READY 4.3, 4.4; on by default, no-yebis.flag opts out): the main view's YEBIS feed of FB890 for
// the three first groups (i = 0 Setting Info, 1 Tone Map, 3 Bloom: keys A at context+0x3FD8+8i, B at +0x4040+8i, weight
// +0x40A8+4i, epsilon [143D7ECE8]), the blocks built from the native banks and blended (YB-1), then the three first converters
// on a zero-filled scratch object (E4070 Setting Info, E42A0 Tone Map, E4460 Bloom only when [GXRS+0x5368] exists), which
// write the library's globals block 144961F70 as the original does each frame; the native passes read them (yebis.globals);
// row native_yebis_globals digests the 0x370-byte block and names the V1 observables.
void CollectNativeYebisInputs(NativeSelectedScene& scene,uintptr_t view) {
    static const bool enabled=NativeYebisPass(L"");
    if(!enabled)return;
    native_effect_exposure.store(0.f,std::memory_order_relaxed);   // review: no carried exposure on an early return
    // W10 auto exposure (YP8): the map-override gparam id per area. FUN_140619BD0 loads gparam:/MapOverride_%04u.gparam (FUN_140D27E20)
    // with the id FUN_140CE70A0([1447914C8], area) returns: the area's record (0x110 bytes, [table+0x20] + area x 0x110, [table+0x10]
    // records) word +0x1C (+0x18 the id FUN_140A36420 takes). One row per session with the records whose override id is not 0
    // (MapOverride_0001 and _0003 set AutoExposure): which areas turn auto exposure on.
    {static bool logged{};if(!logged){uintptr_t table{},records{};int32_t count{};
        if(Read(image+0x47914C8,&table,8) && table && Read(table+0x10,&count,4) && Read(table+0x20,&records,8) && records && count>0 && count<4096){
            logged=true;std::string cells;int listed{};
            for(int32_t i=0;i<count;++i){uint32_t words[2]{};uint8_t head[24]{};
                if(!Read(records+uint64_t(i)*0x110+0x18,words,8) || !Read(records+uint64_t(i)*0x110,head,24))continue;
                if(!words[1])continue;if(listed++>=64)break;
                char cell[160]{};sprintf_s(cell,"%s[%d,%u,%u,\"%s\"]",cells.empty()?"":",",i,words[0],words[1],NativeWordsList(reinterpret_cast<const uint32_t*>(head),6).c_str());cells+=cell;}
            const std::string row="{\"event\":\"native_map_override_table\",\"records\":"+std::to_string(count)+",\"with_override\":["+cells+"]}\n";Log(row.c_str());}}}
    uintptr_t context{},root{},settings{};
    {const std::lock_guard<std::recursive_mutex> hold(scene_view_cpu_lock);
        const auto found=scene_view_cpu_owners.find(view);if(found!=scene_view_cpu_owners.end())context=found->second.context;}
    std::shared_ptr<const ds3light::Banks> banks;{const std::lock_guard hold(gparam_lock);banks=gparam_banks;}
    float epsilon{};
    if(!context || !banks || !Read(image+0x3D7ECE8,&epsilon,4))return;
    // FB890's gate (lines 108 and 294): the context's +0x50 bit 10 (the world view sets it, the menu views do not) and the
    // library's [144961F70]; post+0x2270 ("ignore external settings") has no native object (YB-7).
    uint32_t context_flags{},library_gate{};const bool gate_read=Read(context+0x50,&context_flags,4) && Read(image+0x4961F70,&library_gate,4);
    const bool gate_open=gate_read && (context_flags&0x400u) && library_gate;
    LogPostKeys(2,context,0);   // I-0138 tranche 8h: the keys this collect reads (07 LogPostKeys, shaft-observe.flag)
    const bool have_settings=Read(image+0x4961B50,&root,8) && root && Read(root+0x5368,&settings,8) && settings;
    struct Key {uint32_t a[2]{},b[2]{};float weight{};};
    const auto key_of=[&](uint32_t i){Key k;Read(context+0x3FD8+8*i,k.a,8);Read(context+0x4040+8*i,k.b,8);Read(context+0x40A8+4*i,&k.weight,4);return k;};
    const Key setting_key=key_of(0),tone_key=key_of(1),bloom_key=key_of(3),shaft_key=key_of(9);   // YB-4b: i = 9 light shafts
    const Key feedback_key=key_of(5),aa_key=key_of(7);   // YB-4b: i = 5 feedback blur, 7 anti-alias
    const Key dof_key=key_of(2),motion_key=key_of(6);    // YB-4b.1/.2: i = 2 depth of field, 6 motion blur
    const uint64_t external_focus=native_external_focus.load(std::memory_order_acquire);   // I-0141: the camera's focus request (07 NativeExternalFocus), read once a collect
    const Key grading_key=key_of(4);                      // YB-4c: i = 4 colour grading
    std::string refused;
    // Fable review (m33): the bank layer each group's record comes from (Banks::GroupEntries' source), logged at each change.
    std::string record_sources;
    const auto entries=[&](const uint32_t key[2],const char* group,ds3native::yebis::Entries& out){std::string source;
        if(!banks->GroupEntries(key[0],key[1],group,out,&source)){refused+=refused.empty()?"":",";refused+=group;return false;}
        record_sources+=std::string(group)+"["+std::to_string(key[0])+","+std::to_string(key[1])+"]="+source+";";return true;};
    struct SourcesRow{std::string& text;~SourcesRow(){static std::mutex lock;static std::string last;bool fresh{};{const std::lock_guard held(lock);fresh=text!=last;if(fresh)last=text;}
        if(fresh && !text.empty()){std::string ascii;for(const auto ch:text)if(ascii.size()<900)ascii.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');
            const std::string row="{\"event\":\"native_yebis_sources\",\"records\":\""+ascii+"\"}\n";Log(row.c_str());}}} sources_row{record_sources};
    ds3native::yebis::Entries sa,sb,ta,tb,ba,bb;
    std::array<uint32_t,21> setting_a{},setting_b{};std::array<uint32_t,35> tone_a{},tone_b{};std::array<uint32_t,42> bloom_a{},bloom_b{};
    const bool setting_ok=entries(setting_key.a,"Setting Info",sa) && entries(setting_key.b,"Setting Info",sb) &&
        ds3native::yebis::SettingBlock(sa,setting_a) && ds3native::yebis::SettingBlock(sb,setting_b);
    const bool tone_ok=entries(tone_key.a,"Tone Map",ta) && entries(tone_key.b,"Tone Map",tb) &&
        ds3native::yebis::ToneBlock(ta,tone_a) && ds3native::yebis::ToneBlock(tb,tone_b);
    const bool bloom_ok=entries(bloom_key.a,"Bloom",ba) && entries(bloom_key.b,"Bloom",bb) &&
        ds3native::yebis::BloomBlock(ba,bloom_a) && ds3native::yebis::BloomBlock(bb,bloom_b);
    NativeSelectedScene::Yebis yebis;
    if(setting_ok)yebis.setting=ds3native::yebis::BlendSetting(setting_a,setting_b,setting_key.weight,epsilon);
    if(tone_ok)yebis.tone=ds3native::yebis::BlendTone(tone_a,tone_b,tone_key.weight,epsilon);
    if(bloom_ok)yebis.bloom=ds3native::yebis::BlendBloom(bloom_a,bloom_b,bloom_key.weight,epsilon);
    // YB-4b (FFX-WAVE1-READY 4.1): the LightShaft block (bank root+0x4628, blend 14021D690); fed to E4700 after the Bloom, as FB890.
    {ds3native::yebis::Entries la,lb;std::array<uint32_t,28> shaft_a{},shaft_b{};
        yebis.shaft_ready=entries(shaft_key.a,"LightShaft",la) && entries(shaft_key.b,"LightShaft",lb) &&
            ds3native::yebis::LightShaftBlock(la,shaft_a) && ds3native::yebis::LightShaftBlock(lb,shaft_b);
        if(yebis.shaft_ready)yebis.shaft=ds3native::yebis::BlendLightShaft(shaft_a,shaft_b,shaft_key.weight,epsilon);}
    // YB-4b.1 (YEBIS-SECOND-TRANCHE-READY 2.2): the Dof record (bank root+0x4630, blend 14021E720), then FB890's override when
    // context+0x413D is set (word 12 and 24 zero, word 2 = context+0x4140). YB-4b.2 (2.3): the MotionBlur record (root+0x4640,
    // blend 1401D9910).
    {ds3native::yebis::Entries xa,xb;std::array<uint32_t,34> a{},b{};
        yebis.dof_ready=entries(dof_key.a,"Dof",xa) && entries(dof_key.b,"Dof",xb) && ds3native::yebis::DofBlock(xa,a) && ds3native::yebis::DofBlock(xb,b);
        if(yebis.dof_ready){yebis.dof=ds3native::yebis::BlendDof(a,b,dof_key.weight,epsilon);BYTE scripted{};
            if(Read(context+0x413D,&scripted,1) && scripted){yebis.dof[12]=0;yebis.dof[24]=0;Read(context+0x4140,&yebis.dof[2],4);}}
        // I-0141 (45000030's focus): the DoF keys, their records' focus distance (word 2) and the blend's as the native reads them (after
        // the context's scripted override, which has no writer in retail: the original's reference_dof_blend rows are before it), and the
        // camera's request (07 NativeExternalFocus); shaft-observe.flag. One row at each change of those (the weight left out: it moves
        // every frame of a fade), at most 400; the focus applied is native_shaft_depth's.
        static const bool observe=ModuleFlag(L"shaft-observe.flag");
        static int rows{};   // the collect's one serial call site
        if(observe && rows<400){static std::string last;float wa{},wb{},out{},asked{};std::memcpy(&wa,&a[2],4);std::memcpy(&wb,&b[2],4);if(yebis.dof_ready)std::memcpy(&out,&yebis.dof[2],4);
            const uint32_t bits=uint32_t(external_focus);std::memcpy(&asked,&bits,4);
            char row[320]{};sprintf_s(row,"{\"event\":\"native_dof_focus\",\"ready\":%s,\"keys\":[%u,%u,%u,%u],\"a2\":%.4g,\"b2\":%.4g,\"word2\":%.4g,\"enable\":[%u,%u],\"auto\":%u,\"camera\":[%u,%.4g]",
                yebis.dof_ready?"true":"false",dof_key.a[0],dof_key.a[1],dof_key.b[0],dof_key.b[1],wa,wb,out,a[12],b[12],yebis.dof_ready?yebis.dof[24]:0u,unsigned(external_focus>>32),asked);
            if(last!=row){last=row;++rows;char weight[48]{};sprintf_s(weight,",\"weight\":%.4g}\n",dof_key.weight);Log((std::string(row)+weight).c_str());}}}
    {ds3native::yebis::Entries xa,xb;std::array<uint32_t,21> a{},b{};
        yebis.motion_ready=entries(motion_key.a,"MotionBlur",xa) && entries(motion_key.b,"MotionBlur",xb) && ds3native::yebis::MotionBlurBlock(xa,a) && ds3native::yebis::MotionBlurBlock(xb,b);
        if(yebis.motion_ready){yebis.motion=ds3native::yebis::BlendMotionBlur(a,b,motion_key.weight,epsilon);
            // 1401D9910's in-between branch leaves word 12, the top bytes of word 14 and byte 3 of word 19 as the caller's buffer
            // holds them: in FB890 the same buffer just held the DoF record (after the override), so those are its words.
            if(yebis.dof_ready && std::fabs(motion_key.weight)>epsilon && std::fabs(motion_key.weight-1.0f)>epsilon){yebis.motion[12]=yebis.dof[12];
                yebis.motion[14]=(yebis.dof[14]&~0xFFu)|(yebis.motion[14]&0xFFu);yebis.motion[19]=(yebis.dof[19]&0xFF000000u)|(yebis.motion[19]&0xFFFFFFu);}}}
    // YB-4c (FFX-WAVE1-READY 4.2): the colour-grading integers (bank root+0x4650, the integer table of 14021C150; names and
    // LUT texture fields are not built: the LUT itself waits for the library host).
    std::array<uint32_t,5> grading{};bool grading_ready=false;
    {ds3native::yebis::Entries xa,xb;std::array<uint32_t,5> a{},b{};
        grading_ready=entries(grading_key.a,"ColorGrading",xa) && entries(grading_key.b,"ColorGrading",xb) && ds3native::yebis::GradingBlock(xa,a) && ds3native::yebis::GradingBlock(xb,b);
        if(grading_ready)grading=ds3native::yebis::BlendGrading(a,b,grading_key.weight);}
    // W10 GR: word 1 is A's LutSourceId below weight 1, B's from 1 (14021C150); its record names the LUT with its file's prefix.
    yebis.grading=grading;yebis.grading_ready=grading_ready;
    {const uint32_t* chosen=grading_key.weight>=1.0f?grading_key.b:grading_key.a;yebis.grading_area=chosen[0];
        ds3native::yebis::Entries unused;if(grading_ready)banks->GroupEntries(chosen[0],chosen[1],"ColorGrading",unused,&yebis.grading_source);}
    // YB-4b: the Anti-Alias record (bank root+0x4638, blend 140220B50) and the FeedBackBlur record (root+0x4648, 140221640).
    std::array<uint32_t,9> aa{};std::array<uint32_t,8> feedback{};bool aa_ready=false,feedback_ready=false;
    {ds3native::yebis::Entries xa,xb;std::array<uint32_t,9> a{},b{};
        aa_ready=entries(aa_key.a,"Anti-Alias",xa) && entries(aa_key.b,"Anti-Alias",xb) && ds3native::yebis::AntiAliasBlock(xa,a) && ds3native::yebis::AntiAliasBlock(xb,b);
        if(aa_ready)aa=ds3native::yebis::BlendAntiAlias(a,b,aa_key.weight,epsilon);}
    {ds3native::yebis::Entries xa,xb;std::array<uint32_t,8> a{},b{};
        feedback_ready=entries(feedback_key.a,"FeedBackBlur",xa) && entries(feedback_key.b,"FeedBackBlur",xb) && ds3native::yebis::FeedbackBlock(xa,a) && ds3native::yebis::FeedbackBlock(xb,b);
        if(feedback_ready)feedback=ds3native::yebis::BlendFeedback(a,b,feedback_key.weight);}
    yebis.complete=setting_ok && tone_ok && bloom_ok;
    // YB-4b.4 (YEBIS-SECOND-TRANCHE-READY 2.5, 4.3): auto exposure (word 9 or `Enable AutoExposure` positive in either Tone Map
    // record: the stored-exposure blend reads both) and auto focus (the blended DoF word 24, after the scripted-focus override)
    // need the library's read-back (YB-8): refused with a row; the feed goes on, the exposure global stays the tone-map value
    // and the focus distance the record's word 2. BlendToneStored is the transcription the host will use.
    ds3native::yebis::AutoFlags auto_flags;
    if(tone_ok){const auto fa=ds3native::yebis::ToneAutoFlags(ta),fb=ds3native::yebis::ToneAutoFlags(tb);
        auto_flags.exposure=fa.exposure || fb.exposure;auto_flags.exposure_gx=fa.exposure_gx || fb.exposure_gx;}
    if(yebis.dof_ready)auto_flags.focus=ds3native::yebis::DofAutoFocus(yebis.dof);
    yebis.auto_exposure=auto_flags.exposure || auto_flags.exposure_gx;   // W10 TM-2: our tone-map pass serves the manual case only
    {static int last_auto=-1;const int now=(auto_flags.exposure?1:0)|(auto_flags.exposure_gx?2:0)|(auto_flags.focus?4:0);
        if(now!=last_auto){last_auto=now;char row[320]{};sprintf_s(row,"{\"event\":\"native_yebis_auto\",\"approximated\":%s,\"auto_exposure\":%s,\"auto_exposure_gx\":%s,"
            "\"auto_focus\":%s,\"tone_keys\":[%u,%u,%u,%u,%g],\"dof_keys\":[%u,%u,%u,%u,%g],\"readback_latency\":%u}\n",auto_flags.Any()?"true":"false",
            auto_flags.exposure?"true":"false",auto_flags.exposure_gx?"true":"false",auto_flags.focus?"true":"false",tone_key.a[0],tone_key.a[1],tone_key.b[0],
            tone_key.b[1],tone_key.weight,dof_key.a[0],dof_key.a[1],dof_key.b[0],dof_key.b[1],dof_key.weight,ds3native::yebis::kReadbacks[0].latency);Log(row);}}
    // YB-4: the converters on the scratch object, in FB890's order (Setting Info, Tone Map, Bloom), each with its blended block
    // in a zero-padded buffer (the records are 0x80 bytes, 55 and 50 words; the converters read the words above).
    if(!yebis.complete && refused.empty())refused=!setting_ok?"Setting Info block":!tone_ok?"Tone Map block (layout)":"Bloom block";
    // One serial call site (the main view's collect on the engine thread): the static scratch object is not shared.
    if(yebis.complete && gate_open){
        auto& scratch=native_yebis_post;std::memset(scratch.data(),0,scratch.size());   // zeroed each frame (the V1-validated path)
        alignas(16) std::array<uint32_t,64> buffer{};
        std::copy(yebis.setting.begin(),yebis.setting.end(),buffer.begin());
        reinterpret_cast<void(__fastcall*)(void*,void*)>(image+0xE4070)(scratch.data(),buffer.data());
        buffer.fill(0);std::copy(yebis.tone.begin(),yebis.tone.end(),buffer.begin());
        reinterpret_cast<void(__fastcall*)(void*)>(image+0xE42A0)(buffer.data());
        if(have_settings){buffer.fill(0);std::copy(yebis.bloom.begin(),yebis.bloom.end(),buffer.begin());
            reinterpret_cast<void(__fastcall*)(void*,void*)>(image+0xE4460)(scratch.data(),buffer.data());yebis.bloom_converted=true;}
        // E4700 reads only [GXRS+0x5360]/[GXRS+0x5368] (guarded inside) and writes post+0x21E0..0x221F: the scratch object holds.
        if(yebis.shaft_ready){buffer.fill(0);std::copy(yebis.shaft.begin(),yebis.shaft.end(),buffer.begin());
            reinterpret_cast<void(__fastcall*)(void*,void*)>(image+0xE4700)(scratch.data(),buffer.data());
            std::memcpy(yebis.shaft_post.data(),scratch.data()+0x21E0,sizeof(yebis.shaft_post));yebis.shaft_post_ready=true;}   // LS-2b
        // E4890 reads post+0x2271/+0x2272 (the constructor's 1 and 1; +0x2270 0), the quality object and, unguarded whenever
        // [GXRS+0x5360] exists, [GXRS+0x5368] (1400E48F6): called only with both, or without the quality object, as E4B50.
        uintptr_t quality_object{};const bool has_quality=root && Read(root+0x5360,&quality_object,8) && quality_object;
        if(yebis.dof_ready && (!has_quality || have_settings)){buffer.fill(0);std::copy(yebis.dof.begin(),yebis.dof.end(),buffer.begin());
            // I-0141: +0x2271 as the game's camera asked it (07 NativeExternalFocus, FUN_140017ea0): 0 whenever the camera gives its own
            // focus distance (in play and in cutscenes), which the converter then leaves in place and which is written below as
            // FUN_140017ea0 writes it.
            const bool use_record=(external_focus>>32)!=0;
            scratch[0x2270]=0;scratch[0x2271]=use_record?1:0;scratch[0x2272]=1;
            reinterpret_cast<void(__fastcall*)(void*,void*)>(image+0xE4890)(scratch.data(),buffer.data());}
        // E4B50 dereferences [GXRS+0x5368] whenever [GXRS+0x5360] exists: called only with both, or without the quality object.
        {if(yebis.motion_ready && (!has_quality || have_settings)){buffer.fill(0);std::copy(yebis.motion.begin(),yebis.motion.end(),buffer.begin());
                reinterpret_cast<void(__fastcall*)(void*)>(image+0xE4B50)(buffer.data());}}
        yebis.converted=true;}
    // I-0141 tranche 9: the camera's focus distance (07 NativeExternalFocus) where FUN_140017ea0 writes it, whatever the DoF record:
    // after the converter (which leaves +0x1B0 while +0x2271 is 0), before the snapshot the prepare reads.
    if(!(external_focus>>32)){const uint32_t bits=uint32_t(external_focus);
        std::memcpy(reinterpret_cast<void*>(image+0x4962120),&bits,4);std::memcpy(reinterpret_cast<void*>(image+0x4961DB0),&bits,4);}
    // FB890 lines 389-449 (post+0x2270 being 0), whatever the other blocks: 144962190 = Enable && AntiAliasType == 0, forced 0
    // when the quality object's +0x08 is 0 or the factor [[GXRS+0x5368]+0x98] <= 0 (XMM7 holds 1.0 from 1400FB953 when the factor
    // object is absent: review); 14496218C = the previous 144962178, then 144962178..188 = the feedback words 0..4.
    if(gate_open){
        if(aa_ready){uint32_t flag=aa[0]!=0 && aa[1]==0?1u:0u;uintptr_t quality_object{};
            if(root && Read(root+0x5360,&quality_object,8) && quality_object){float factor=1.0f;BYTE aa_quality{};
                if(settings)Read(settings+0x98,&factor,4);
                if(!Read(quality_object+8,&aa_quality,1) || !aa_quality || factor<=0.0f)flag=0;}
            *reinterpret_cast<uint32_t*>(image+0x4962190)=flag;}
        if(feedback_ready){auto* g=reinterpret_cast<uint32_t*>(image+0x4962178);g[5]=g[0];for(size_t i=0;i<5;++i)g[i]=feedback[i];}
        // YB-7 precondition (FFX-WAVE1-READY 4.2, row FF14): these are the original's values (144962228 the LUT-copy trigger), but the
        // names and texture fields 1449622C8/D0 are not built: a library host must force 144962228 and 144962224 to 0 (and log it)
        // until the LUT source is fed, or the LUT copy reads absent textures. Nothing native reads them before the host.
        if(grading_ready){auto* g=reinterpret_cast<uint32_t*>(image+0x4962228);for(size_t i=0;i<5;++i)g[i]=grading[i];}}
    // I-0138 tranche 5c: the GX anti-aliasing filter (FB890's 1B9790 after the tone map) is not run natively (Thomas, 2026-11-14).
    std::array<uint8_t,0x370> globals{};const bool globals_read=Read(image+0x4961F70,globals.data(),globals.size());
    uint64_t digest=1469598103934665603ull;for(const auto b:globals){digest^=b;digest*=1099511628211ull;}
    yebis.globals_digest=globals_read?digest:0;
    yebis.globals=globals;yebis.globals_read=globals_read;   // Fable review: carried to the prepare
    scene.yebis=yebis;
    {float e{};static const bool tonemap=NativeYebisPass(L"tonemap");   // review: only when our tone map multiplies the scene by it
        if(gate_open && tone_ok && tonemap && !yebis.auto_exposure)std::memcpy(&e,yebis.tone.data(),4);native_effect_exposure.store(std::isfinite(e) && e>0.f?e:0.f,std::memory_order_relaxed);}
    static LONG64 calls{};static uint64_t last{};const auto n=InterlockedIncrement64(&calls);
    if(n<=3 || n%600==0 || yebis.globals_digest!=last){last=yebis.globals_digest;   // (light shafts included in the digest)
        const auto word=[&](uintptr_t address){uint32_t v{};std::memcpy(&v,globals.data()+(address-0x4961F70),4);return v;};
        const auto real=[&](uintptr_t address){float v{};std::memcpy(&v,globals.data()+(address-0x4961F70),4);return v;};
        std::string setting_words;for(uintptr_t a=0x4961F78;a<=0x4961FBC;a+=4)setting_words+=(setting_words.empty()?"":",")+std::to_string(word(a));
        char row[2048]{};sprintf_s(row,"{\"event\":\"native_yebis_globals\",\"call\":%lld,\"keys\":[[%u,%u,%u,%u,%g],[%u,%u,%u,%u,%g],[%u,%u,%u,%u,%g]],"
            "\"gate\":[%u,%u],\"complete\":%s,\"refused\":\"%.80s\",\"auto_flags\":%d,\"converted\":%s,\"bloom\":%s,\"gate_f70\":%u,\"digest\":\"%016llX\",\"exposure\":%g,\"gamma\":%g,\"tone_type\":%u,"
            "\"filmic_contrast\":%g,\"scene_luminance\":%g,\"grading\":%u,\"setting\":[%s],\"block_digests\":[\"%016llX\",\"%016llX\",\"%016llX\",\"%016llX\"],\"shaft_key\":[%u,%u,%u,%u,%g],\"shaft_globals\":[\"%08X\",\"%08X\",\"%08X\",\"%08X\"],\"feedback_aa\":[\"%08X\",\"%08X\",\"%08X\",\"%08X\",\"%08X\",\"%08X\",\"%08X\"],\"dof_motion\":[\"%016llX\",\"%016llX\"],\"dof_globals\":[\"%08X\",\"%08X\",\"%08X\",\"%08X\"],\"motion_globals\":[\"%08X\",\"%08X\",\"%08X\",\"%08X\"],\"grading_globals\":[\"%08X\",\"%08X\",\"%08X\",\"%08X\",\"%08X\"],\"tone\":[%s]}\n",
            static_cast<long long>(n),setting_key.a[0],setting_key.a[1],setting_key.b[0],setting_key.b[1],setting_key.weight,tone_key.a[0],tone_key.a[1],tone_key.b[0],
            tone_key.b[1],tone_key.weight,bloom_key.a[0],bloom_key.a[1],bloom_key.b[0],bloom_key.b[1],bloom_key.weight,(context_flags>>10)&1u,library_gate,yebis.complete?"true":"false",refused.c_str(),(auto_flags.exposure?1:0)|(auto_flags.exposure_gx?2:0)|(auto_flags.focus?4:0),
            yebis.converted?"true":"false",yebis.bloom_converted?"true":"false",word(0x4961F70),static_cast<unsigned long long>(yebis.globals_digest),
            real(0x4962008),real(0x496200C),word(0x4962010),real(0x4962018),real(0x4962004),word(0x4962224),setting_words.c_str(),
            static_cast<unsigned long long>(NativeWordsDigest(yebis.setting.data(),21)),static_cast<unsigned long long>(NativeWordsDigest(yebis.tone.data(),35)),
            static_cast<unsigned long long>(NativeWordsDigest(yebis.bloom.data(),42)),static_cast<unsigned long long>(NativeWordsDigest(yebis.shaft.data(),28)),
            shaft_key.a[0],shaft_key.a[1],shaft_key.b[0],shaft_key.b[1],shaft_key.weight,word(0x49620A0),word(0x49620B0),word(0x49620B4),word(0x49621BC),
            word(0x4962178),word(0x496217C),word(0x4962180),word(0x4962184),word(0x4962188),word(0x496218C),word(0x4962190),
            static_cast<unsigned long long>(NativeWordsDigest(yebis.dof.data(),34)),static_cast<unsigned long long>(NativeWordsDigest(yebis.motion.data(),21)),
            word(0x496211C),word(0x4962124),word(0x4962140),word(0x4962144),word(0x496214C),word(0x4962154),word(0x4962158),word(0x4962160),
            word(0x4962228),word(0x496222C),word(0x4962230),word(0x4962234),word(0x4962238),NativeWordsList(yebis.tone.data(),35).c_str());Log(row);}
}
void CollectNativeVelocityInputs(NativeSelectedScene& scene,uintptr_t view) {
    uintptr_t root{},quality{};BYTE view_enabled{};float factor{},view_scale{},epsilon{};std::array<uint8_t,8> row{};uint32_t level{};
    const bool option_read=scene.options && scene.options->object_valid;
    if(option_read){std::memcpy(row.data(),scene.options->object.data()+0x20,row.size());level=uint32_t(scene.options->levels[4]);}
    // I-0111: while the upscaler runs, the target, the resolve and the object rows are its motion vectors, whatever the motion
    // blur option says (the motion blur passes keep their own gate). I-0126 R7c (the final review's H1): the traced GI too, whose
    // frames without RR (DLSS inactive, the RR option off) NRD denoises from the G-buffer surface's motion, object rows included,
    // until NRD fails for the session (the fixes' step review L-d). An unread MaxBlurLength then takes the fallback below, as on a
    // DLSS frame (the fixes' step review L-e: the rows' scale and the resolve's agree; a blur that frame reads its own scale).
    const bool upscaler=scene.dlss || scene.taa || (renderer.rt_gi.load(std::memory_order_relaxed) && !renderer.rt_nrd_off.load(std::memory_order_relaxed));
    const BYTE option[2]{BYTE(upscaler?1:row[0]),BYTE(upscaler?1:row[1])};
    const bool factor_read=Read(image+0x4961B50,&root,8) && root && Read(root+0x5368,&quality,8) && quality && Read(quality+0xE0,&factor,4);
    const bool view_read=Read(view+0x595,&view_enabled,1) && Read(view+0xCA4,&view_scale,4);
    // MaxBlurLength of the blended MotionBlur gparam (87FB0/FB890 -> E4B50 -> [144962158] = max(value, 0.001)).
    float motion_blur_length{};std::string length_sources;bool length_read=false;
    {uintptr_t context{};{const std::lock_guard<std::recursive_mutex> hold(scene_view_cpu_lock);
            const auto found=scene_view_cpu_owners.find(view);if(found!=scene_view_cpu_owners.end())context=found->second.context;}
        std::shared_ptr<const ds3light::Banks> banks;{const std::lock_guard hold(gparam_lock);banks=gparam_banks;}
        uint32_t keys[4]{};float weight{},same{};
        if(context && banks && Read(context+0x4008,&keys[0],8) && Read(context+0x4070,&keys[2],8) && Read(context+0x40C0,&weight,4) &&
           Read(image+0x3D7ECE8,&same,4)){
            float a{},b{};std::string source_a,source_b;
            if(banks->GroupFloat(keys[0],keys[1],"MotionBlur","MaxBlurLength",a,&source_a) &&
               banks->GroupFloat(keys[2],keys[3],"MotionBlur","MaxBlurLength",b,&source_b)){
                const float blended=std::fabs(weight)<=same?a:std::fabs(weight-1.0f)<=same?b:(b-a)*weight+a;
                motion_blur_length=std::max(blended,0.001f);length_read=true;length_sources=source_a+"/"+source_b;}}
        static LONG64 key_samples{};if(InterlockedIncrement64(&key_samples)<=3 || !length_read){static LONG64 failures{};
            if(length_read || InterlockedIncrement64(&failures)<=6){char length_row[320]{};sprintf_s(length_row,
                "{\"event\":\"native_velocity_blur_length\",\"read\":%s,\"keys\":[%u,%u,%u,%u],\"weight\":%g,\"length\":%g,\"sources\":\"%.60s\"}\n",
                length_read?"true":"false",keys[0],keys[1],keys[2],keys[3],weight,motion_blur_length,length_sources.c_str());Log(length_row);}}}
    bool read=option_read && factor_read && view_read && length_read && Read(image+0x3D7ECF4,&epsilon,4);
    // I-0111 (step review M1): while the upscaler runs it needs its vectors every frame; the motion-blur-only words (the factor, the
    // view's bytes, MaxBlurLength, the epsilon) are then not required: an unread length takes Reference1188's 0.1 and an unread
    // epsilon the .rdata's 1.0E-5 (the scale is the rows' encoding and the resolve's decoding alike, so any positive one holds).
    if(upscaler && !read){if(!length_read)motion_blur_length=0.1f;if(!(epsilon>0))epsilon=1.0e-5f;read=true;}
    VelocityMatrix current{};const bool projected=VelocityViewProjection(scene.camera,current);
    const std::array<float,2> jitter{scene.camera.jitter_x,scene.camera.jitter_y};const bool jittered=jitter[0]!=0 || jitter[1]!=0;
    const auto ndc=upscaler::JitterNdc(jitter,scene.camera.width,scene.camera.height);
    std::optional<NativeSelectedScene::Velocity> velocity;bool reset=false;
    // The pass byte (+0x20, 102380's gate) makes the target and the resolve; the row byte (+0x21) adds the object rows.
    bool reprojected=true;
    if(read && option[0] && projected){
        NativeSelectedScene::Velocity inputs;VelocityMatrix previous=current;inputs.rows=option[1]!=0;
        {const std::lock_guard<std::recursive_mutex> hold(scene_view_cpu_lock);auto& history=native_velocity_history;
            bool cut=!history.valid || history.view!=view;
            if(!cut)cut=ds3native::yebis::CameraCut(history.pose.data(),scene.camera.camera_pose.data());   // YB-4b.5: one rule
            if(!cut)previous=history.view_projection;reset=cut;
            inputs.previous_pose=cut?scene.camera.camera_pose:history.pose;   // W10 MB-4: the same cut rule for the motion blur's previous camera
            // A view-projection that cannot be inverted gives no velocity this frame and restarts the history.
            // I-0111 (UPSCALER-DLSS-READY §3.3): with the upscaler's jitter the depth was rasterised through the jittered current
            // matrix: the resolve inverts that one, reprojects with the unjittered previous one and adds the jitter back to its
            // output, so its vectors stay unjittered; the history keeps the unjittered matrix.
            reprojected=VelocityCurrentToPrevious(jittered?upscaler::ShiftClip(current,ndc):current,previous,inputs.current_to_previous);
            if(reprojected && jittered)upscaler::FoldJitter(inputs.current_to_previous,jitter,scene.camera.width,scene.camera.height);
            VelocityMatrix inverse{};
            if(upscaler && reprojected && VelocityInverse(current,inverse)){const auto m=VelocityMultiply(inverse,previous);
                for(unsigned i=0;i<16;++i)inputs.clip_to_previous_clip[i]=static_cast<float>(m[i]);}
            if(reprojected)history={view,current,scene.camera.camera_pose,true};else history.valid=false;}
        // I-0111: the object rows rasterise against the jittered depth: both matrices take the current frame's NDC shift, which
        // cancels in their difference (the decoded vector stays unjittered, T-U13).
        inputs.matrices=jittered?VelocityMatrixBlock(upscaler::ShiftClip(current,ndc),upscaler::ShiftClip(previous,ndc)):VelocityMatrixBlock(current,previous);
        inputs.reset=reset;
        const float aspect=scene.camera.explicit_aspect?scene.camera.explicit_aspect:float(scene.camera.width)/float(scene.camera.height);
        // 5A120's param_1 is the GX view 140024740 builds on its stack (ctor 139CE0); its +0xCA4 comes from the store at
        // fStack_744 (I-0068 VELOCITY-CA4-WRITER): for the YEBIS (world) view, [144962158] = max(field [3] MaxBlurLength of the
        // blended MotionBlur gparam, 0.001) (E4B50). The selection's view is another object (+0xCA4 0). Produced here from the
        // pipeline context's MotionBlur keys (+0x4008, +0x4070, weight +0x40C0, blended as 1D9910) over the admitted GPARAM
        // files. Reference1188: 0.1 on every context, FC_DifColMul (8.889, -5, 0, 1) at 16/9.
        inputs.scale=VelocityScale(aspect,motion_blur_length,epsilon);if(reprojected)velocity=inputs;
    }else{const std::lock_guard<std::recursive_mutex> hold(scene_view_cpu_lock);native_velocity_history.valid=false;}
    scene.velocity=velocity;
    static LONG64 samples{};const auto n=InterlockedIncrement64(&samples);
    if(n<=3 || n%600==0 || ((reset || !reprojected) && n<=6000)){uint32_t cap{};std::memcpy(&cap,row.data()+4,4);char text[480]{};sprintf_s(text,
        "{\"event\":\"native_velocity_inputs\",\"sample\":%lld,\"read\":[%u,%u,%u],\"level\":%u,\"option_20\":%u,\"option_21\":%u,\"sample_cap\":%u,\"object\":%s,\"factor\":%g,\"view_595\":%u,\"view_ca4\":%g,\"epsilon\":%g,\"enabled\":%s,\"reset\":%s,\"reprojected\":%s,\"scale\":[%g,%g]}\n",
        n,option_read?1u:0u,factor_read?1u:0u,view_read?1u:0u,level,option[0],option[1],cap,option_read?"true":"false",factor,view_enabled,view_scale,epsilon,
        velocity?"true":"false",reset?"true":"false",reprojected?"true":"false",velocity?velocity->scale[0]:0.f,velocity?velocity->scale[1]:0.f);Log(text);}
}
// cP7 (PREPARE-REDUCTION, R8b S4): the deferred lighting input of a scene (programmes, cbLightAcc, the SSS records, the
// environment regions of the main view, the point-light batches), built at collect on the engine thread: prepare calls
// no engine function and only attaches it to a frame that has deferred draws.
// I-0129 R9a (R9-PLAN §2): the main view's probe requests: one a model that draws forward rows this collect (CollectNativeForwardLights'
// entries) and has its candidate's world box (the order key's: a character's owner box, a draw entity's +0xB80, an item's instance
// box), in the scene's order, at most kRtProbesMost; its light set the record's envData (+0x60), else 0. A box not finite, empty or
// wider than kRtProbeBoxMost a side (kRtProbeMixedBoxMost with G-buffer rows) is passed over. The step review's H1: a model that also draws G-buffer rows (engine passes 5 to 7: they
// write ShLightMask.x into GB1.z, the deferred sun's mask under the traced shadow) asks no sun rays (its record keeps DS3's mask); a
// water model (rows 0x24, 0x27: its lighting R11's) asks nothing.
std::shared_ptr<const RtProbeRequests> NativeRtProbeRequests(const NativeSelectedScene& scene) {
    auto out=std::make_shared<RtProbeRequests>();out->states=native_rt_probe_states;out->epoch=native_rt_probe_states->Epoch();
    std::unordered_set<uint64_t> seen;
    std::vector<uint8_t> rows(scene.models.size());   // bit 0 a G-buffer row, bit 1 a water row
    for(const auto& p:scene.prepared_meshes)if(p.model<rows.size())rows[p.model]|=uint8_t((p.engine_pass>=5 && p.engine_pass<=7)?1:(p.engine_pass==0x24 || p.engine_pass==0x27)?2:0);
    for(size_t index=0;index<scene.models.size();++index){const auto& m=scene.models[index];
        if(out->requests.size()>=kRtProbesMost)break;
        if(!m.forward_light || !m.sort || m.effect || (rows[index]&2))continue;
        const auto& b=m.sort->box;RtProbeRequest r;bool good=true;
        // The final review's M3: a box at most kRtProbeBoxMost a side (kRtProbeMixedBoxMost for a model with G-buffer rows: a room-
        // sized map piece's box sees outside its room, and its rays start under its floor), else no probe (DS3's rows).
        const float most=(rows[index]&1)?kRtProbeMixedBoxMost:kRtProbeBoxMost;
        for(int k=0;k<3;++k){r.lo[k]=std::min(b[k],b[4+k]);r.hi[k]=std::max(b[k],b[4+k]);
            good&=std::isfinite(r.lo[k]) && std::isfinite(r.hi[k]) && r.hi[k]-r.lo[k]<=most;}
        if(!good || !(r.hi[0]-r.lo[0]+r.hi[1]-r.lo[1]+r.hi[2]-r.lo[2]>0.f))continue;
        const uint64_t key=NativeRtProbeKey(m.group,m.model);if(!seen.insert(key).second)continue;
        if(m.instances[0].written[0x60/4])std::memcpy(&r.set,m.instances[0].bytes.data()+0x60,4);
        if(rows[index]&1)r.flags|=kRtProbeRequestNoSun;
        out->requests.push_back(r);out->keys.push_back(key);}
    // I-0129 R9b: then the effects' cells noted by 1DB60's detour since the last requests (each its metre cell as its box, set 0).
    for(const uint64_t key:effect_probe::Take()){if(out->requests.size()>=kRtProbesMost)break;if(!seen.insert(key).second)continue;
        out->requests.push_back(RtProbeCellRequest(key,0));out->keys.push_back(key);}
    return out;}
// D-0301 (Thomas, 2026-11-21): a menu view (an untraced offscreen view) draws its environment regions as the original does, in
// place of a cleared SpecularAcc (STUDY-MENU-ENV-REGIONS). The original's selection 67070 (STATIC, re-read 2026-11-21), per region of
// the manager's list +0x38..+0x40 while it is on (+0x10):
// 1. fade = region +0xC8, or the manager's +0x2C0 when +0x2BD;
// 2. the masks of the handle 69940 finds for it (the last of the handles +0x18..+0x20 resolving to it: index +8 not above the table
//    count +0x60, generation +0xA that of the table +0x58's 0x18-byte entry) against the view's pair: (e+8 & pair.lo) && (e+0xC &
//    pair.hi). A menu view's pair is {1, 2} (D64C30 gives its composite +0xE0 {0, 2}; the base scene's slot 4 makes {(lo & ~0xFF) | 1,
//    hi}: STATIC); D0C2F0 adds the menu views' own region (GILM0010, the 60-unit cube at the origin) with masks {0x11, 2} and every
//    world region has {0x11, 1}, so only it passes in a menu view (natively D0C2F0 adds it with its key gate answered from the
//    native banks and its manager calls admitted by the native region registry: NativeMenuRegionAdd, 16_offscreen_and_movies.inc);
// 3. the frustum test (+0x130 vt+0x58, an OBB against the camera frustum): not applied (open policy: a box off screen rasterizes
//    nothing, so the drawn pixels are the same);
// 4. refused when fade > 0 and fade <= the eye's distance to the box (the eye's offset to the centre +0x50 projected on the axes
//    +0x20/+0x30/+0x40, clamped to the half extents +0x10);
// 5. refused when the manager's +0x2DA and not the entry's bucket +0x12.
// The draw's inputs are those of the executor 675B0 as the legacy producer transcribed them (K-0424, ProduceNativeEnvRegions before
// I-0132 R11e): CB_ENVMAP_REGION (W = diag(half) rows +20/+30/+40 and the translation +50 with the .data row 144929B20, its inverse,
// the extents, the colour scale, the fade distance +CC, the fade rate 1 - clamp01((d - (C8 - D8)) / D8)), the parallax-corrected
// variant when the record's +D4 and not the manager's +2BC, the inside variant when the camera's near sphere reaches the box. The cube:
// an area-1 key {1, id} names the TPF cube GILM<id> (3FEB0's empty prefix for a gparam that is not a map's), resolved as the menu
// forward rows' (NativeMenuEnvCube), its colour scale the gparam Env Map row's (NativeEnvMapScale: default.gparam's row 10 has 1.0);
// another key takes the world regions' lookup (NativeEnvMapCube); with the manager's +0x2B8 the record's own cube +0xE8 and scale
// +0xDC (the legacy producer's branch). A witness row per view extent (its first 4 calls, then each change of its outcome, at most 40,
// and every 600th call over all, its first 64) gives the counts and each listed region's key, masks, box, fade and verdict but the world regions
// refused by their masks (the step review's M1, M2, L1, L2).
void ProduceNativeMenuEnvRegions(const SceneViewInput& camera,const ingress::SceneCameraConstants& constants,NativeLightingInput& lighting) {
    lighting.env_regions.clear();lighting.env_scene.clear();
    static LONG64 calls{};const auto n=InterlockedIncrement64(&calls);
    // Whether this call's outcome is logged: per view extent, its first 4 calls and each change of its outcome (40 such rows at most),
    // and every 600th call over all views, its first 64 (not counted in an extent's 40: the step review of its answers, L1; capped:
    // the final review's L5).
    const auto witness=[&](uint64_t outcome){static std::mutex lock;static std::map<uint64_t,std::pair<uint32_t,uint64_t>> extents;
        const std::lock_guard hold(lock);auto& [rows,last]=extents[uint64_t(camera.width)<<32|camera.height];
        const bool counted=rows<4 || (outcome!=last && rows<40);last=outcome;rows+=counted;return counted || (n%600==0 && n<=600*64);};
    const auto refuse=[&](uint64_t code,const char* reason){if(witness(code)){char row[200]{};_snprintf_s(row,sizeof row,_TRUNCATE,
        "{\"event\":\"native_menu_env_regions\",\"call\":%lld,\"extent\":[%u,%u],\"refused\":\"%s\"}\n",n,camera.width,camera.height,reason);Log(row);}};
    for(uint32_t offset=16;offset<400;offset+=4)if(constants.written[offset/4]!=1)return refuse(1,"camera_constants");
    uintptr_t root{},table{};std::array<uint8_t,0x340> manager{};float row_c[4]{};
    if(!Read(image+0x4961B50,&root,8) || !root || !Read(root+0x42B0,manager.data(),manager.size()))return refuse(2,"manager_unreadable");
    if(!manager[0x10])return refuse(3,"manager_off");
    if(!Read(root+0x42A8,&table,8) || !table || !Read(image+0x4929B20,row_c,16))return refuse(4,"table_unreadable");
    const auto P=[&](uint32_t at){uintptr_t v{};std::memcpy(&v,manager.data()+at,8);return v;};
    const auto M=[&](uint32_t at){float v{};std::memcpy(&v,manager.data()+at,4);return v;};
    const uintptr_t handles=P(0x18),handles_end=P(0x20),list=P(0x38),list_end=P(0x40),entries=P(0x58);
    uint32_t count{};std::memcpy(&count,manager.data()+0x60,4);
    if(handles_end<handles || (handles_end-handles)/8>4096 || list_end<list || (list_end-list)/8>4096)return refuse(5,"lists");
    // 69940: each handle's region and table index (a handle that does not resolve names the manager's +0x80, never a listed region).
    std::vector<std::pair<uintptr_t,uint16_t>> resolved;
    for(uintptr_t at=handles;at<handles_end;at+=8){uintptr_t handle{},region{};int32_t id{};uint16_t generation{},current{};
        if(!Read(at,&handle,8) || !handle || !Read(handle+8,&id,4) || id==-1)continue;
        const uint16_t index=static_cast<uint16_t>(id);
        if(count<index || !Read(handle+0xA,&generation,2) || !Read(entries+0x10+0x18ull*index,&current,2) || generation!=current ||
           !Read(entries+0x18ull*index,&region,8) || !region)continue;
        resolved.push_back({region,index});}
    static constexpr uint32_t pair[2]{1,2};   // the menu views' (STATIC, D64C30)
    const float eye[3]{camera.camera_pose[12],camera.camera_pose[13],camera.camera_pose[14]};
    const float tangent=std::tan(camera.vertical_fov*0.5f),aspect=camera.explicit_aspect?camera.explicit_aspect:float(camera.width)/float(camera.height);
    const float near_radius=std::sqrt((tangent*camera.near_plane)*(tangent*camera.near_plane)+(tangent*aspect*camera.near_plane)*(tangent*aspect*camera.near_plane)+camera.near_plane*camera.near_plane);
    // The listed regions' fields, formatted only when the row is logged (L2); a value that is not finite is written null (L5).
    struct Item {uint32_t key[2],masks[2];bool handled;float values[10];const char* verdict;std::string cube,state;float scale;};
    std::vector<Item> items;uint32_t regions{},masked{},world_masked{},far_away{},bucket{},no_cube{};
    for(uintptr_t at=list;at<list_end && lighting.env_regions.size()<64;at+=8){
        uintptr_t record{};std::array<uint8_t,0x100> r{};if(!Read(at,&record,8) || !record || !Read(record,r.data(),r.size()))continue;
        ++regions;const auto R=[&](uint32_t o){float v{};std::memcpy(&v,r.data()+o,4);return v;};
        uint32_t key[2]{},masks[2]{};std::memcpy(key,r.data(),8);uint8_t region_bucket{};bool handled{};
        for(auto j=resolved.rbegin();j!=resolved.rend();++j)if(j->first==record){
            handled=Read(entries+0x18ull*j->second+8,masks,8) && Read(entries+0x18ull*j->second+0x12,&region_bucket,1);break;}
        const float fade=manager[0x2BD]?M(0x2C0):R(0xC8);
        float local[3]{},q[3]{};
        for(int k=0;k<3;++k){const float d0=eye[0]-R(0x50),d1=eye[1]-R(0x54),d2=eye[2]-R(0x58);
            local[k]=(d0*R(0x20+16*k)+d1*R(0x24+16*k))+d2*R(0x28+16*k);
            const float e=R(0x10+4*k);float v=-e;v=v>local[k]?v:local[k];v=v<e?v:e;q[k]=(v-local[k])*(v-local[k]);}
        const float distance=std::sqrt((q[0]+q[1])+q[2]);
        const char* verdict="drawn";
        if(!handled || !(masks[0]&pair[0]) || !(masks[1]&pair[1])){verdict="masks";++masked;}
        else if(fade>0.f && fade<=distance){verdict="far";++far_away;}
        else if(manager[0x2DA] && !region_bucket){verdict="bucket";++bucket;}
        std::shared_ptr<const bootstrap::NativeTextureRecord> resident;float scale{1.f};std::string cube_name,cube_state;
        if(!std::strcmp(verdict,"drawn")){uintptr_t cube{};
            if(manager[0x2B8]){std::memcpy(&cube,r.data()+0xE8,8);scale=R(0xDC);cube_state="record";}
            else if(key[0]==1){resident=NativeMenuEnvCube(key,cube_name,cube_state);scale=NativeEnvMapScale(key);}
            else if((cube=NativeEnvMapCube(table,record,scale)))cube_state="world";
            if(cube)if(auto t=textures.Acquire(cube);t && t->resident && t->resource && t->view)resident=std::move(t);
            if(!resident || !resident->description.cube){verdict="no_cube";++no_cube;resident.reset();}}
        float base=R(0xC8),span=R(0xD8);if(manager[0x2BD]){base=M(0x2C0);span=M(0x2C4);}
        const float t=(distance-(base-span))/span;const float clamped=0.f>t?0.f:(t<=1.f?t:1.f),rate=1.f-clamped;
        // The row lists every region but the world's ones a menu view refuses by their masks {0x11, 1} (counted in world_masked).
        const bool world=masks[0]==0x11 && masks[1]==1 && !std::strcmp(verdict,"masks");world_masked+=world;
        if(!world && items.size()<8)items.push_back({{key[0],key[1]},{masks[0],masks[1]},handled,{R(0x10),R(0x14),R(0x18),R(0x50),R(0x54),R(0x58),fade,span,distance,rate},
            verdict,cube_name.substr(0,24),cube_state.substr(0,16),scale});
        if(!resident)continue;
        NativeEnvRegionDraw draw;
        const float sx=R(0x10),sy=R(0x14),sz=R(0x18);
        const float a[4][4]={{sx,0,0,0},{0,sy,0,0},{0,0,sz,0},{row_c[0],row_c[1],row_c[2],row_c[3]}};
        float w[16]{},inv[16]{},f[100]{};
        for(int i=0;i<4;++i)for(int j=0;j<4;++j)w[4*i+j]=((a[i][3]*R(0x50+4*j)+a[i][1]*R(0x30+4*j))+a[i][2]*R(0x40+4*j))+a[i][0]*R(0x20+4*j);
        ds3prod::InverseMatrix(w,inv);
        for(int col=0;col<4;++col)for(int row=0;row<4;++row){f[4*col+row]=w[4*row+col];f[16+4*col+row]=inv[4*row+col];}
        for(int k=0;k<4;++k)f[32+k]=R(0x10+4*k);
        f[36]=scale;f[37]=R(0xCC);f[80]=rate;
        std::memcpy(draw.constants.data(),f,400);
        draw.parallax_corrected=r[0xD4]!=0 && !manager[0x2BC];
        draw.inside=distance<=near_radius;
        draw.cube.stage=ds3content::ShaderStage::Pixel;draw.cube.shader_register=30;
        draw.cube.descriptor=resident->view.handle;draw.cube.resource=resident->resource;draw.cube.owner=resident;
        lighting.env_regions.push_back(std::move(draw));
    }
    if(!lighting.env_regions.empty())lighting.env_scene.assign(constants.bytes.begin(),constants.bytes.end());
    // The outcome: a marker bit (never a refusal's code) and each count in its own field (the step review of its answers, M1).
    const auto field=[](size_t v){return uint64_t(v<255?v:255);};
    const uint64_t outcome=1ull<<63|field(lighting.env_regions.size())|field(no_cube)<<8|field(far_away)<<16|field(masked)<<24|field(bucket)<<32|
        field(world_masked)<<40|uint64_t(regions<65535?regions:65535)<<48;
    if(witness(outcome)){
        const auto J=[](float v){char text[32]{};if(!std::isfinite(v))return std::string("null");_snprintf_s(text,sizeof text,_TRUNCATE,"%g",v);return std::string(text);};
        std::string listed;for(const auto& i:items){listed+=std::string(listed.empty()?"":",")+"{\"key\":["+std::to_string(i.key[0])+","+std::to_string(static_cast<int32_t>(i.key[1]))+
            "],\"masks\":["+std::to_string(i.masks[0])+","+std::to_string(i.masks[1])+"],\"handle\":"+(i.handled?"1":"0")+",\"half\":["+J(i.values[0])+","+J(i.values[1])+","+J(i.values[2])+
            "],\"centre\":["+J(i.values[3])+","+J(i.values[4])+","+J(i.values[5])+"],\"fade\":"+J(i.values[6])+",\"span\":"+J(i.values[7])+",\"distance\":"+J(i.values[8])+
            ",\"rate\":"+J(i.values[9])+",\"verdict\":\""+i.verdict+"\",\"cube\":\""+i.cube+"\",\"state\":\""+i.state+"\",\"scale\":"+J(i.scale)+"}";}
        std::string row="{\"event\":\"native_menu_env_regions\",\"call\":"+std::to_string(n)+",\"extent\":["+std::to_string(camera.width)+","+std::to_string(camera.height)+"],\"regions\":"+std::to_string(regions)+
            ",\"handles\":"+std::to_string(resolved.size())+",\"drawn\":"+std::to_string(lighting.env_regions.size())+",\"masked\":"+std::to_string(masked)+",\"world_masked\":"+std::to_string(world_masked)+
            ",\"far\":"+std::to_string(far_away)+",\"bucket\":"+std::to_string(bucket)+",\"no_cube\":"+std::to_string(no_cube)+
            ",\"eye\":["+std::to_string(eye[0])+","+std::to_string(eye[1])+","+std::to_string(eye[2])+"],\"near_radius\":"+std::to_string(near_radius)+
            ",\"listed\":["+listed+"]}\n";Log(row.c_str());}
}
void ProduceNativeSceneLighting(NativeSelectedScene& scene_out,bool offscreen) {
    if(scene_out.prepared_meshes.empty())return;   // review: no draw, no lighting (as when it was built by prepare)
    if(!scene_out.light_blocks)return;
    const auto* scene=&scene_out;
    // 1049 native deferred lighting: programmes and blocks all current, else colour only.
    auto lighting=std::make_shared<NativeLightingInput>();lighting->programs=AcquireNativeLightingPrograms();
    if(lighting->programs && ProduceNativeLightAccParam(scene->camera,scene->camera_constants,lighting->light_acc)){
        lighting->light=scene->light_blocks->light;lighting->fog=scene->light_blocks->fog;lighting->authored_sets=scene->light_blocks->authored;   // I-0141
        // I-0125 R6c: FC_ShadowColor, from the shadow frame's cbSceneParam words (+0x1D0, what the sun pass's b8 binds: 13b); since
        // I-0132 R11d (D-0262) neither the hits nor the sun pass read DS3's value (its floor removed): a witness row only.
        if(!offscreen && scene->shadow_frame){std::array<float,4> colour{};std::memcpy(colour.data(),scene->shadow_frame->scene_words.data()+(0x1D0-0x1C0),16);
            // Its first values and each change (at most 8 rows; the main view's collect, one thread).
            static std::array<float,4> last{-1,-1,-1,-1};static LONG64 rows{};if(colour!=last && InterlockedIncrement64(&rows)<=8){last=colour;char row[160]{};
                sprintf_s(row,"{\"event\":\"native_rt_shadow_colour\",\"colour\":[%g,%g,%g,%g]}\n",colour[0],colour[1],colour[2],colour[3]);Log(row);}}
        {static const bool no_sun=ModuleFlag(L"no-sun.flag");lighting->skip_sun=no_sun;}   // image regression: development A/B of the sun's share
        // O0 (M-1): below Shade MEDIUM the original records no SSS command; the native stage is skipped the same way.
        if(!lighting->programs->sss_vertex.empty() && !scene->light_blocks->shade_sss){
            static LONG64 gated{};if(InterlockedIncrement64(&gated)%600==1)Log("{\"event\":\"native_sss_skipped\",\"reason\":\"shade_quality\"}\n");}
        else if(!lighting->programs->sss_vertex.empty() && !ProduceNativeSssInputs(scene->camera,scene->camera_constants,lighting->sss_profiles,lighting->sss_scene)){
            lighting->sss_profiles.clear();lighting->sss_scene.clear();
            static LONG64 skipped{};if(InterlockedIncrement64(&skipped)%600==1)Log("{\"event\":\"native_sss_skipped\",\"reason\":\"native_sss_inputs_unavailable\"}\n");}
        // D-0301: a menu view's environment regions (the main view's specular ambient is traced; I-0132 R11e removed its regions).
        if(offscreen && !lighting->programs->env_vertex.empty())ProduceNativeMenuEnvRegions(scene->camera,scene->camera_constants,*lighting);
        // 1054: 1A1DA0's schedule: the 234E40 binning over 128x128 tiles (I-0047
        // transcription), per tile and class (DPointLight / LegacyDPointLight)
        // batches of at most 12 in list order; cbLightAccLightParam rows item
        // [+20,+50) and {+90,+94,0,0}; cbLightAccVtxParam {r,0,0,0}, r = min(z'/w',1)
        // of (0,0,min near,1) by the projection (0 unless z',w' > 0).
        // I-0106: the spot items join the binning with their hull (214310's points by the item's matrix, 140244520), and each
        // tile draws its spot groups of at most 4 (1A6690, DSpotLight1..4) after its point batches, in the tile list's order;
        // cbLightAccVtxParam.x of a group is its min near (+0x20) projected as the point batches' depth.
        // I-0132 R11e (its step review's H1): a main view with the rays but no local light at all gets an empty list (its tiles
        // empty), so the local lights' pass, the reflections and the GI still serve its frames (without a list they were unserved: black,
        // then the fatal close). A view with lights whose list cannot be made keeps none (a real failure: judged unserved).
        if(scene->point_lights.empty() && scene->spot_lights.empty() && !offscreen && native_rt_scene.load(std::memory_order_relaxed) &&
           renderer.rt_lights.load(std::memory_order_relaxed)){auto empty=std::make_shared<RtLightList>();
            empty->columns=(scene->camera.width+127)/128;empty->rows=(scene->camera.height+127)/128;empty->tiles.assign(2ull*empty->columns*empty->rows,0);
            lighting->rt_lights=std::move(empty);}
        if(!scene->point_lights.empty() || !scene->spot_lights.empty()){
            // I-0132 R11b (R11-PLAN §13 item 4; its final review's M1): DS3's point batches only for a view that draws them (the menu views,
            // or the rays not asked); the main view with the rays shades its lights from the rays' list alone. `local_lights` is set where a
            // batch would have been made (a tile on screen with a light: the step review's M1), so the frame's judgement (cause 8) is unchanged.
            const bool batches=offscreen || !renderer.rt_asked.load(std::memory_order_relaxed);
            ds3tiles::View view{};view.fov=scene->camera.vertical_fov;view.aspect=scene->camera.explicit_aspect;
            view.aspect_w=scene->camera.width;view.aspect_h=scene->camera.height;view.views=1;view.width=scene->camera.width;view.height=scene->camera.height;
            const size_t point_count=scene->point_lights.size();
            std::vector<ds3tiles::Sphere> spheres;for(const auto& light:scene->point_lights)spheres.push_back({light.sphere[0],light.sphere[1],light.sphere[2],light.sphere[3]});
            std::vector<ds3tiles::Hull> hulls(point_count);
            for(const auto& spot:scene->spot_lights){spheres.push_back({});hulls.push_back(spot.hull);}
            std::vector<std::vector<uint32_t>> tiles;uint32_t columns{};
            float projection[16]{};bool projected=true;
            for(size_t row=0;row<4;++row)for(size_t column=0;column<4;++column){const auto offset=256+4*(4*column+row);
                if(scene->camera_constants.written[offset/4]!=1)projected=false;else std::memcpy(&projection[4*row+column],scene->camera_constants.bytes.data()+offset,4);}
            const auto depth_of=[&](float nearest){const float clip_z=nearest*projection[10]+projection[14],clip_w=nearest*projection[11]+projection[15];
                return clip_z>0.f && clip_w>0.f?std::min(clip_z/clip_w,1.f):0.f;};
            size_t spot_batches{};
            // I-0122 R4a (on with the rays, the main view; R4-PLAN §4): every light a batch would draw, as one world-space list with
            // each tile's indices in the batches' order (points, legacy points, spots), which the renderer shades natively
            // (RtLocalLights). R4b: a shadowed light (one its class's `Shadowed` programmes draw) with its shadow words
            // (RtShadowedOf: its strength and a point's faces' near plane), traced by the pass; counted in the batches' row, a point
            // whose ShadowCenterPos is not its position (the programmes' cube is centred on it, the rays on the position) and one whose
            // near plane cannot be read (the pass leaves it unshadowed). The batches
            // are made only for a view that draws them (I-0132 R11b): the menu views, all unshadowed (I-0131 R10a), or the rays not asked; I-0132 R11e: the main view draws none, a frame its pass cannot run is unserved (cause 8, from `local_lights`).
            // I-0123 step review M1 (« skip the object and
            // go on »): a light that cannot give its record (a value not finite) gets an inert one (no colour, out of every reach) and is
            // counted (`rt_inert`), instead of leaving the frame without a list; a list holds every light, in the scene's order (index = record). The row again when that count changes.
            std::shared_ptr<RtLightList> rt_list;size_t rt_centre_off{},rt_near_unknown{},rt_inert{},rt_unshadowed{},rt_physical{},rt_physical_refused{};
            const float local_physical=renderer.rt_local_physical.load(std::memory_order_relaxed);   // I-0132 R11h: 0 off
            if(!offscreen && native_rt_scene.load(std::memory_order_relaxed) && renderer.rt_lights.load(std::memory_order_relaxed)){
                rt_list=std::make_shared<RtLightList>();rt_list->lights.reserve(point_count+scene->spot_lights.size());
                float pose[16]{},view_matrix[16]{};std::memcpy(pose,scene->camera.camera_pose.data(),64);ds3prod::InverseMatrix(pose,view_matrix);
                const float local_scale=renderer.rt_local_scale.load(std::memory_order_relaxed);   // I-0132 R11d: 1 (DS3's own) by default since D-0293, 2 and 4 test tools
                for(size_t i=0;i<point_count+scene->spot_lights.size() && rt_list;++i){RtLight record;bool made{};
                    if(i<point_count){const auto& light=scene->point_lights[i];made=RtPointLightOf(light.param,light.world,light.legacy,record);
                        // I-0131 R10a (its step review's M2): shadow words a record cannot take (a strength not finite) leave the light
                        // unshadowed (no shadowed batch draws it any more), counted in rt_inert's row as rt_unshadowed; no raster programme gates it.
                        if(made && light.shadowed){if(!RtShadowedOf(light.shadow,record))++rt_unshadowed;
                            rt_centre_off+=std::abs(light.shadow[4]-light.param[0])+std::abs(light.shadow[5]-light.param[1])+std::abs(light.shadow[6]-light.param[2])>1e-3f;
                            rt_near_unknown+=(record.kind&kRtLightShadowed) && !(record.trans[3]>0.f);}}
                    else{const auto& light=scene->spot_lights[i-point_count];made=RtSpotLightOf(light.param,pose,view_matrix,record);
                        if(made && light.shadowed && !RtShadowedOf(light.shadow,record))++rt_unshadowed;}
                    // I-0132 R11d (R11-PLAN §5, a test tool for Thomas's sheets: rt-local-x2.flag, rt-local-x4.flag): the local lights' colours
                    // scaled on the rays' list (the native lights pass and the hits), their reach unchanged.
                    // I-0132 R11h (R11-PLAN section 9 step 2, a test tool: rt-local-physical-10/-50/-200.flag): the record in physical terms
                    // (RtPhysicalLight: the inverse square windowed at a reach from its brightness, its intensity times the ratio to the
                    // sun) in place of rt_local_scale (1 since D-0293); a record it cannot convert keeps DS3's falloff with that scale (counted).
                    if(made && local_physical>0.f){if(RtPhysicalLight(record,local_physical,kRtLocalPhysicalMinimum))++rt_physical;else ++rt_physical_refused;}
                    if(made && !(record.kind&kRtLightPhysical) && local_scale!=1.f)for(int c=0;c<3;++c){record.diffuse[c]*=local_scale;record.specular[c]*=local_scale;}
                    if(!made){record=RtLight{};record.position={0,0,0,1e30f};record.trans={0,1,0,0};++rt_inert;}   // 1 / R² huge: no reach; TL.y 1: no 0^0
                    rt_list->lights.push_back(record);}}
            // I-0132 R11h: a physical light's screen tiles from its window's sphere (r), a spot's too (its hull dropped: the window,
            // not DS3's range, bounds it), as the hits' grid takes it (RtLightReach).
            if(rt_list)for(size_t i=0;i<rt_list->lights.size() && i<spheres.size();++i){const auto& record=rt_list->lights[i];
                if(!(record.kind&kRtLightPhysical) || !(record.position[3]>0.f))continue;const float r=1.f/std::sqrt(record.position[3]);
                if(i<point_count)spheres[i].r=r;
                else{const auto& spot=scene->spot_lights[i-point_count];spheres[i]={spot.param[0],spot.param[1],spot.param[2],r};hulls[i].valid=false;}}
            // The second step review's L1, L2: a row at the first list and when the refused count changes, at most 32 (atomics: no mutable static).
            {static std::atomic<int64_t> physical_seen{-1};static std::atomic<int> physical_rows{};
             if(rt_list && local_physical>0.f && int64_t(rt_physical_refused)!=physical_seen.load(std::memory_order_relaxed) && physical_rows.fetch_add(1,std::memory_order_relaxed)<32){
                physical_seen.store(int64_t(rt_physical_refused),std::memory_order_relaxed);char row[200]{};
                sprintf_s(row,"{\"event\":\"native_rt_local_physical\",\"ratio\":%g,\"lights\":%zu,\"refused\":%zu}\n",local_physical,rt_physical,rt_physical_refused);Log(row);}}
            if(projected && ds3tiles::Bin(view,spheres,hulls,128,128,0,0,0,0,tiles,columns) && columns){
                if(rt_list){rt_list->columns=columns;rt_list->rows=(view.height+127)/128;
                    if(columns!=(view.width+127)/128 || tiles.size()>size_t(columns)*rt_list->rows)rt_list.reset();}
                if(rt_list){const size_t count=size_t(rt_list->columns)*rt_list->rows;rt_list->tiles.assign(2*count,0);
                    for(uint32_t tile=0;tile<tiles.size();++tile){const auto first=static_cast<uint32_t>(rt_list->tiles.size());
                        for(const int kind:{0,1,2})for(const auto index:tiles[tile])
                            if(kind==(index>=point_count?2:scene->point_lights[index].legacy?1:0))rt_list->tiles.push_back(index);
                        rt_list->tiles[2*tile]=first;rt_list->tiles[2*tile+1]=static_cast<uint32_t>(rt_list->tiles.size())-first;}}
                for(uint32_t tile=0;tile<tiles.size();++tile){
                    const int32_t left=static_cast<int32_t>((tile%columns)*128),top=static_cast<int32_t>((tile/columns)*128);
                    const int32_t right=std::min<int32_t>(left+128,static_cast<int32_t>(view.width)),bottom=std::min<int32_t>(top+128,static_cast<int32_t>(view.height));
                    if(right<=left || bottom<=top)continue;
                    if(!tiles[tile].empty())lighting->local_lights=true;
                    if(!batches)continue;
                    // 1A1DA0's lists by item type, drawn in this order: 0x65 points (1A5B50, 12 a group), 0x6C legacy points, 0x67/0x6E spots
                    // (1A6690, 4). I-0131 R10a: the shadowed lists (0x66, 0x6D, 0x68/0x6F) are not made any more: with no shadow atlas every
                    // light, a shadowed one too, goes into the unshadowed programmes (I-0132 R11e: the frame the rays cannot serve is judged unserved and not shown).
                    const auto group=[&](const std::vector<uint32_t>& members,bool spot,bool legacy){
                        const size_t limit=spot?4:12;
                        for(size_t start=0;start<members.size();start+=limit){
                            const size_t count=std::min<size_t>(limit,members.size()-start);
                            NativePointLightBatch batch;batch.spot=spot;
                            batch.programme=(legacy?12u:0u)+static_cast<uint32_t>(count)-1;
                            batch.left=left;batch.top=top;batch.right=right;batch.bottom=bottom;
                            float nearest=FLT_MAX;
                            for(size_t k=0;k<count;++k){
                                if(spot){const auto& light=scene->spot_lights[members[start+k]];
                                    const auto* bytes=reinterpret_cast<const uint8_t*>(light.param.data());batch.lights.insert(batch.lights.end(),bytes,bytes+128);
                                    nearest=std::min(nearest,light.near_z);}
                                else{const auto& light=scene->point_lights[members[start+k]];
                                    const auto* bytes=reinterpret_cast<const uint8_t*>(light.param.data());batch.lights.insert(batch.lights.end(),bytes,bytes+64);
                                    nearest=std::min(nearest,light.near_z);}}
                            batch.depth=depth_of(nearest);
                            if(spot)++spot_batches;
                            lighting->point_batches.push_back(std::move(batch));
                        }};
                    for(const bool legacy:{false,true}){
                        std::vector<uint32_t> members;for(const auto index:tiles[tile])if(index<point_count && scene->point_lights[index].legacy==legacy)members.push_back(index);
                        group(members,false,legacy);}
                    {std::vector<uint32_t> members;for(const auto index:tiles[tile])if(index>=point_count)members.push_back(index-static_cast<uint32_t>(point_count));
                        group(members,true,false);}
                }
            }
            else rt_list.reset();
            // I-0128 R8c (R8-PLAN §6): the list binned in a world grid about the camera (the pose's translation), for the rays' hits
            // (rt_light_grid.h); no-rt-light-grid.flag leaves it out (the hits loop over the whole list).
            RtLightGridStats rt_grid{};
            if(rt_list && !rt_list->lights.empty() && renderer.rt_light_grid.load(std::memory_order_relaxed)){
                const std::array<float,3> eye{scene->camera.camera_pose[12],scene->camera.camera_pose[13],scene->camera.camera_pose[14]};
                BuildRtLightGrid(rt_list->lights,eye,rt_list->grid,rt_grid);}
            // The fixes' step review L1: the frames whose grid went past its budget, counted (the rows are sampled).
            static uint64_t rt_grid_dropped_frames{};if(rt_grid.dropped)++rt_grid_dropped_frames;
            if(rt_list && !rt_list->lights.empty())lighting->rt_lights=std::move(rt_list);
            static LONG64 batch_witness{};const auto n=InterlockedIncrement64(&batch_witness);
            const bool main_list=!offscreen;   // the fixes' step review L1: the main view's collect only
            static size_t last_inert{};const bool inert_changed=main_list && rt_inert!=last_inert;if(main_list)last_inert=rt_inert;
            // I-0128 R8c: the grid's figures (the cells holding a light, the longest cell's list, the entries, the lights put into every
            // cell and the ones out of the grid; `rt_grid` false without one, `rt_grid_dropped` past its entry budget, the frames so since the start).
            if(n<=2 || (n%600==0 && n<=6000) || inert_changed){char row[768]{};sprintf_s(row,"{\"event\":\"native_point_light_batches\",\"lights\":%zu,\"spots\":%zu,\"tiles\":%zu,\"columns\":%u,\"batches\":%zu,\"spot_batches\":%zu,\"batches_made\":%s,\"rt_lights\":%zu,\"rt_indices\":%zu,\"rt_centre_off\":%zu,\"rt_near_unknown\":%zu,\"rt_inert\":%zu,\"rt_unshadowed\":%zu,\"rt_grid\":%s,\"rt_grid_cells\":%u,\"rt_grid_longest\":%u,\"rt_grid_entries\":%llu,\"rt_grid_everywhere\":%u,\"rt_grid_outside\":%u,\"rt_grid_dropped\":%s,\"rt_grid_dropped_frames\":%llu}\n",
                point_count,scene->spot_lights.size(),tiles.size(),columns,lighting->point_batches.size(),spot_batches,batches?"true":"false",
                lighting->rt_lights?lighting->rt_lights->lights.size():0,lighting->rt_lights?lighting->rt_lights->tiles.size()-2*size_t(lighting->rt_lights->columns)*lighting->rt_lights->rows:0,rt_centre_off,rt_near_unknown,rt_inert,rt_unshadowed,
                lighting->rt_lights && !lighting->rt_lights->grid.empty()?"true":"false",rt_grid.filled,rt_grid.longest,static_cast<unsigned long long>(rt_grid.entries),rt_grid.everywhere,rt_grid.outside,rt_grid.dropped?"true":"false",static_cast<unsigned long long>(rt_grid_dropped_frames));Log(row);}
        }
        // I-0107 (1A0080): GXLightOrganize's inputs for the main view: every point and spot light of the frame (shadowed ones too),
        // at most 1,024 of each as its buffers hold; the point records the batch rows with word 15 the radius (212810's +0x98 =
        // light+0x15C), the spot records the batch rows with word 14 = 1 (214710); cbLightAccParam the deferred assembly then
        // the counts, near and far, the slice scale 0.8 and bias -2.1 and the tile size 64 (14019D5E0's constants), 64 >> 5.
        if(!offscreen && NativeForwardLightsOn() && !renderer.LightGridFailed() && !lighting->programs->organize.empty() && lighting->light_acc.size()==320 &&
           scene->camera.width && scene->camera.height){
            NativeLightingInput::LightOrganize organize;
            const size_t points=std::min<size_t>(scene->point_lights.size(),1024),spots=std::min<size_t>(scene->spot_lights.size(),1024);
            organize.points.resize(points*64);organize.spots.resize(spots*128);
            for(size_t i=0;i<points;++i){auto row=scene->point_lights[i].param;row[14]=0.f;row[15]=scene->point_lights[i].sphere[3];
                std::memcpy(organize.points.data()+64*i,row.data(),64);}
            for(size_t i=0;i<spots;++i){auto row=scene->spot_lights[i].param;row[14]=1.f;std::memcpy(organize.spots.data()+128*i,row.data(),128);}
            std::memcpy(organize.constants.data(),lighting->light_acc.data(),320);
            const uint32_t words[]{static_cast<uint32_t>(points),0,static_cast<uint32_t>(spots),0};std::memcpy(organize.constants.data()+4*68,words,16);
            const float range[4]{scene->camera.near_plane,scene->camera.far_plane,0.8f,-2.1f};std::memcpy(organize.constants.data()+4*72,range,16);
            const uint32_t tile[4]{64,64>>5,0,0};std::memcpy(organize.constants.data()+4*76,tile,16);
            organize.groups_x=(scene->camera.width+63)/64;organize.groups_y=(scene->camera.height+63)/64;
            lighting->organize=std::move(organize);
            // D-0151: lights past the buffers' 1,024 are dropped as the original's buffers would overflow; counted, never silent.
            const bool overflow=scene->point_lights.size()>1024 || scene->spot_lights.size()>1024;
            static LONG64 witness{},overflows{};if(overflow && InterlockedIncrement64(&overflows)<=20){char row[160]{};
                sprintf_s(row,"{\"event\":\"native_light_organize_overflow\",\"points\":%zu,\"spots\":%zu}\n",scene->point_lights.size(),scene->spot_lights.size());Log(row);}
            if(const auto n=InterlockedIncrement64(&witness);n<=2 || n%600==0){char row[200]{};
                sprintf_s(row,"{\"event\":\"native_light_organize\",\"call\":%lld,\"points\":%zu,\"spots\":%zu,\"groups\":[%u,%u],\"near_far\":[%g,%g]}\n",
                    static_cast<long long>(n),points,spots,lighting->organize->groups_x,lighting->organize->groups_y,range[0],range[1]);Log(row);}}
        // I-0129 R9a: the traced probes' requests (the forward rows' models), with the rays on the main view.
        if(!offscreen && native_rt_scene.load(std::memory_order_relaxed) && renderer.rt_probes.load(std::memory_order_relaxed))
            if(auto probes=NativeRtProbeRequests(*scene);!probes->requests.empty())lighting->rt_probes=std::move(probes);
        scene_out.lighting=std::move(lighting);}
}
// Phantoms (Native1876: the skin's Phantom_Fwd and Fwd programmes refused for cbPerFrame +0x30): 23790 builds the PS cbPerFrame
// from its stack image: +0x00 FC_DebugParam, +0x30 FC_GlobalSpecularMask.. and +0x40 the 16 bytes at 1439F24B0, +0x20
// FC_DebugParam2 and +0x50 the 16 bytes at 1439F2490 (before the snow rows +0x60.., the time +0xB0, the Dominant rows +0xC0).
void ProduceNativePerFrameGlobals(uint8_t* bytes,uint32_t size,uint8_t* written) {
    static const auto constants=[]{std::array<uint32_t,8> words{};Read(image+0x39F24B0,words.data(),16);Read(image+0x39F2490,words.data()+4,16);return words;}();
    for(const auto& [offset,from]:{std::pair<uint32_t,uint32_t>{0x00,0},{0x30,0},{0x40,0},{0x20,4},{0x50,4}})
        ds3prod::ProduceMatDynScalars(offset,constants.data()+from,4,bytes,size,written);
}
// I-0105 (the surface snow): 23790's snow rows (NativeSelectedScene::SnowRows) into a cbPerFrame image, and 1A86C0's cbModelParam
// FC_SnowRate +0x40 from the draw state's group (state +0x240): the `Snow Model` rates of its pair +0x70/+0x78 (state +0x2B0/+0x2B8;
// a key no gparam layer holds takes the engine bank's built-in rate), blended by +0x80, times the scene snow rate.
void ProduceNativeSnowRows(const NativeSelectedScene& scene,uint8_t* bytes,uint32_t size,uint8_t* written) {
    if(!scene.snow_rows || size<0xB0)return;
    std::memcpy(bytes+0x60,scene.snow_rows->rows.data(),0x50);std::fill(written+0x60/4,written+0xB0/4,uint8_t(1));}
// I-0120 B8: the wet shader catalogue (43 UTF-16 names 512 bytes apart at 144731BE0) is initialised data of the image's .data
// (its first name, GXFlver_SSSEmiIblGlow_Bloody, is in the file); the only code found to use it is 2DD60's compare at an MTD
// load, which computes the address (GhidraMCP: no reference to its base). Read once at the first success and kept for the
// session instead of a 22 KB guarded copy and 43 strings every collect. INFERRED: no code writes it after the start.
std::shared_ptr<const std::vector<std::wstring>> NativeWetShaderCatalog() {
    static std::mutex lock;static std::shared_ptr<const std::vector<std::wstring>> kept;
    const std::lock_guard hold(lock);if(kept)return kept;
    std::vector<std::wstring> rows;if(ingress::ReadModelWetShaderCatalog(image,rows))kept=std::make_shared<const std::vector<std::wstring>>(std::move(rows));
    return kept;
}
void ProduceNativeSnowRate(const NativeSelectedScene& scene,const uint8_t* group,uint8_t* bytes,uint32_t size,uint8_t* written) {
    if(!scene.snow_rows)return;
    uint32_t keys[4]{};float blend{};std::memcpy(keys,group+0x70,16);std::memcpy(&blend,group+0x80,4);
    std::shared_ptr<const ds3light::Banks> banks;{const std::lock_guard hold(gparam_lock);banks=gparam_banks;}
    // The rates by key, kept per bank and record generation (admissions run on several workers).
    static std::mutex lock;static std::shared_ptr<const ds3light::Banks> cached_banks;static uint64_t cached_generation{};
    static std::unordered_map<uint64_t,std::optional<float>> rates;
    const uint64_t generation=banks?banks->RecordGeneration():0;   // I-0141 tranche 23: a fade's steps renew the rates
    const auto rate=[&](uint32_t area,uint32_t id){const uint64_t key=(uint64_t(area)<<32)|id;std::optional<float> found;
        {const std::lock_guard hold(lock);if(cached_banks!=banks || cached_generation!=generation){rates.clear();cached_banks=banks;cached_generation=generation;}
            if(const auto at=rates.find(key);at!=rates.end())found=at->second;else{float r{};if(banks && banks->SnowModelAt(area,id,r))found=r;rates.emplace(key,found);}}
        return found?*found:scene.snow_rows->model_builtin;};
    const float a=rate(keys[0],keys[1]),b=rate(keys[2],keys[3]);
    ds3prod::ProduceSnowRate(a,b,blend,scene.snow_rows->scene_rate,bytes,size,written);
    static std::atomic<uint64_t> produced{};const auto n=++produced;   // review L1: the native side of reference_snow_rate
    if(n<=48 || (n%5000==0 && n<=500000)){char row[240]{};sprintf_s(row,"{\"event\":\"native_snow_rate\",\"n\":%llu,\"keys\":[%u,%u,%u,%u],\"blend\":%.4g,\"rate\":%.4g}\n",
        static_cast<unsigned long long>(n),keys[0],keys[1],keys[2],keys[3],blend,((b-a)*blend+a)*scene.snow_rows->scene_rate);Log(row);}}
// I-0105 tranche 10 (final review M1): a `Snow` record's five texture words are filled by 131EA0 (each frame, from 12F350 through
// the manager update 499C0, a null word retried) with the repository's [root+0x47C8] lookups (vtable +8: D49B10, D4ED20's
// entry+0x60 holder) of "%sDeposit_{a,n,r,s,m}_%04d" (143D517E8, 143D51798, 143D51810, 143D517C0, 143D51870: words +0x00, +0x08,
// +0x10, +0x18, +0x20) from the record's string +0x28 and its Texture:ID; the built-in record's string is "m22_00" (12DCC0), for
// which no texture exists. Natively the engine banks receive no file and [root+0x47C8] is not published (D573B0 does not run), so
// the names are made here: a file record's string is its area's stem, INFERRED "mAA_BB_" from the key's area (the High Wall's
// record m30_00_ names map/m30/m30_9999.tpf's m30_00_Deposit_*_0000, I-0110), the built-in record's its own string and ID (read
// from the engine's record, ReadEngineWideString). In t47..t51's order: a, r, s, n, m.
// An MSVC std::wstring in engine memory at `at` (its inline buffer or pointer, size +0x10, capacity +0x18), bounded; empty if unreadable.
std::wstring ReadEngineWideString(uintptr_t at) {
    uint64_t size{},capacity{};uintptr_t text=at;
    if(!Read(at+0x10,&size,8) || !Read(at+0x18,&capacity,8) || size>64 || (capacity>7 && !Read(at,&text,8)))return {};
    std::wstring out(size,L'\0');if(size && !Read(text,out.data(),size*2))return {};return out;
}
std::wstring NativeSnowAreaPrefix(uint32_t area) {
    wchar_t prefix[16]{};swprintf_s(prefix,L"m%02u_%02u_",(area/1000000u)%100u,(area/10000u)%100u);return prefix;
}
std::array<std::wstring,5> NativeSnowRecordNames(const std::wstring& prefix,int32_t texture_id) {
    std::array<std::wstring,5> names;constexpr wchar_t letters[5]{L'a',L'r',L's',L'n',L'm'};
    for(size_t i=0;i<5;++i){wchar_t name[96]{};swprintf_s(name,L"%sDeposit_%c_%04d",prefix.c_str(),letters[i],texture_id);names[i]=name;}
    return names;
}
// The holders of those names now, through the repository D49B10 answers from (the resource singleton [144793348]'s +0x30, which
// D573B0 publishes as [root+0x47C8]); looked up at each collect as 131EA0 retries, so no holder outlives its entry here.
std::array<uintptr_t,5> NativeSnowRecordHolders(const std::array<std::wstring,5>& names) {
    std::array<uintptr_t,5> holders{};uintptr_t singleton{},repository{},table{},lookup{},manager{};
    // D49B10 reads the texture manager [144792F58] unguarded (step review M2); a broken link is reported once (L1).
    if(!Read(image+0x4793348,&singleton,8) || !singleton || !Read(singleton+0x30,&repository,8) || !repository ||
       !Read(repository,&table,8) || !Read(table+8,&lookup,8) || lookup!=image+0xD49E70 || !Read(image+0x4792F58,&manager,8) || !manager){   // D49E70 jumps to D49B10
        static LONG64 broken{};if(InterlockedIncrement64(&broken)<=4){char row[200]{};sprintf_s(row,"{\"event\":\"native_snow_repository_missing\",\"singleton\":%s,\"repository\":%s,\"lookup_rva\":\"%llX\",\"manager\":%s}\n",
            singleton?"true":"false",repository?"true":"false",static_cast<unsigned long long>(lookup>image?lookup-image:0),manager?"true":"false");Log(row);}
        return holders;}
    for(size_t i=0;i<5;++i)holders[i]=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,const wchar_t*)>(lookup)(repository,names[i].c_str());
    // Witness: the names and whether each was found, on each change (bounded).
    static std::array<std::wstring,5> logged_names;static std::array<bool,5> logged_found{};static int rows{};
    std::array<bool,5> found{};for(size_t i=0;i<5;++i)found[i]=holders[i]!=0;
    if(rows<32 && (names!=logged_names || found!=logged_found)){logged_names=names;logged_found=found;++rows;std::string row="{\"event\":\"native_snow_record_names\",\"names\":[";
        for(size_t i=0;i<5;++i){row+=i?",\"":"\"";for(const auto ch:names[i])row.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');row+="\"";}
        row+="],\"found\":[";for(size_t i=0;i<5;++i)row+=std::string(i?",":"")+(found[i]?"true":"false");
        row+="]}\n";Log(row.c_str());}
    return holders;
}
// I-0105 tranche 10 (the snow pieces' textures): 103510 binds t47..t51 for every draw from the blended `Snow` record's first five
// words (`record`, given in t47..t51's order: words +0x00, +0x10, +0x18, +0x08, +0x20), a null one replaced by the root's [root+0x4800], +0x4810,
// +0x4818, +0x4808, +0x4820 (StoreNativeSurfaceTextures: D59E90's global_snow_*, then D1E8A0's snow_* or D1EA10's dust_*; no
// writer of +0x4820 is known), then by GX's default table [[144961AB0]+0x270] +0, +0x20, +0x20, +0x30, +0x10 (06C00's entries 0, 4, 4, 6, 2:
// SYSTEX_WHITE, _BLACKSPEC, _BLACKSPEC, _NORMAL, _GRAY; that table is absent natively, the native bank `system` holds 06C00's
// entries 0..13 at the same indices). An object is resolved as the material bindings' are (a SprjGxTexture holder through its
// inner); one without a resident native record stays null and its slot unbound. The same value is returned while the objects,
// their inners and records stand.
std::shared_ptr<const NativeSelectedScene::SnowTextures> CollectNativeSnowTextures(const NativeSelectedScene& scene,uintptr_t root,
    const std::array<uintptr_t,5>& record) {
    static constexpr uint32_t root_at[5]{0x4800,0x4810,0x4818,0x4808,0x4820};
    static constexpr size_t default_entry[5]{0,4,4,6,2};   // the table's offsets / 8
    const auto default_record=[&](size_t i)->std::shared_ptr<const bootstrap::NativeTextureRecord>{
        return scene.common_images?scene.common_images->system[default_entry[i]].second:nullptr;};
    NativeSelectedScene::SnowTextures now;
    for(size_t i=0;i<5;++i){uintptr_t object=record[i];now.sources[i]=0;
        if(!object){now.sources[i]=1;if(!Read(root+root_at[i],&object,8))object=0;}
        if(!object){now.sources[i]=2;now.records[i]=default_record(i);continue;}
        now.objects[i]=object;uintptr_t table{};if(!Read(object,&table,8))continue;
        now.inners[i]=table==image+0x29559E8?NativeHolderInner(object):object;
        if(!now.inners[i])continue;
        if(auto texture=textures.Acquire(now.inners[i]);texture && texture->resident && texture->resource && texture->view)now.records[i]=std::move(texture);}
    std::shared_ptr<const NativeSelectedScene::SnowTextures> last;
    {const std::lock_guard<std::recursive_mutex> lock(scene_view_cpu_lock);   // review L3: one module value, released at Close
        last=std::static_pointer_cast<const NativeSelectedScene::SnowTextures>(native_snow_textures_last);
        if(last && last->objects==now.objects && last->inners==now.inners && last->sources==now.sources && last->records==now.records)return last;
        last=std::make_shared<const NativeSelectedScene::SnowTextures>(std::move(now));native_snow_textures_last=last;}
    // Witness: each new value (bounded): the objects (their addresses, to name them against native_surface_store), tables, inners and records.
    static LONG64 logged{};if(InterlockedIncrement64(&logged)<=32){std::string row="{\"event\":\"native_snow_textures\",\"slots\":[";
        for(size_t i=0;i<5;++i){const auto& texture=last->records[i];uintptr_t table{};if(last->objects[i])Read(last->objects[i],&table,8);
            char item[320]{};sprintf_s(item,"%s{\"slot\":%zu,\"source\":%u,\"object\":\"%llX\",\"table_rva\":\"%llX\",\"inner\":%s,\"record\":%s,\"size\":[%u,%u],\"format\":%u,\"name\":\"%.60s\"}",
                i?",":"",47+i,last->sources[i],static_cast<unsigned long long>(last->objects[i]),static_cast<unsigned long long>(table>image?table-image:0),last->inners[i]?"true":"false",
                texture?"true":"false",texture?texture->description.width:0,texture?texture->description.height:0,texture?unsigned(texture->description.format):0,
                texture && texture->content?texture->content->name.c_str():"");row+=item;}
        row+="]}\n";Log(row.c_str());}
    return last;
}
// E03 (I-0105): the main view's model particles as candidates of the main walk (kind 3), after the walk's own: those of the effect
// passes 0x29 and 0x21 (I-0113: the Estus Flask's s07002 is submitted in 0x21, the Ashen Estus Flask's in 0x29) with the model's own
// material table (636F0's args+0x1A8 null). The passes 0x2D, 0x20 and 0x31 have no native model rows: such a draw is counted
// (NoteNativeEffectCandidates), not drawn. Each candidate's identity (the key of its record, instance
// carry and engine bytes) is its bone array with its rank among this frame's draws of that array (29B2C0 rewrites one renderer's
// array for each particle it draws, so the particles of one appearance share it): the top bit, the array's address / 16 and the
// rank in the low 16 bits, never a user-mode address (Read fails at it at once), only a key; its placement the identity (236290's
// object rows, args+0x120: the bone rows carry the particle's world); no fade owner (no parameter collection: the generated values
// stand, as 5A120 with a null descriptor+0x20); no dynamic stream source (a null field).
void AppendNativeEffectCandidates(const NativeSelectedScene& scene,std::vector<NativeDeferredCandidate>& deferred) {
    static const uintptr_t no_source{};
    constexpr std::array<float,12> identity{1,0,0,0, 0,1,0,0, 0,0,1,0};
    std::unordered_map<uintptr_t,uintptr_t> ranks;
    for(const auto& effect:scene.effect_models){const auto& m=effect->submission;
        if((m.pass!=0x29 && m.pass!=0x21) || m.materials || !m.bones)continue;
        NativeDeferredCandidate item;item.kind=3;auto& c=item.candidate;
        c.entity=c.pose=(uintptr_t(1)<<63)|((m.bones>>4)<<16)|(ranks[m.bones]++&0xFFFF);c.model=m.model;c.group=reinterpret_cast<uintptr_t>(m.state.data());
        c.source_field=reinterpret_cast<uintptr_t>(&no_source);c.placement=identity;c.effect=effect.get();
        deferred.push_back(std::move(item));}
}
// E03 witness (I-0105): the model particles' outcomes, at the first three collects that have some and every 600th: the submissions,
// those of another pass, with their own material table or no bones, the candidates admitted and refused by code (09_model_admission's
// codes), of the admitted models those with forward rows and those with their forward light (a model without it draws nothing), with
// their own light (args+0x424), and the first models' data names. After the forward lights.
void NoteNativeEffectCandidates(const NativeSelectedScene& scene,const std::vector<NativeDeferredCandidate>& deferred,size_t first) {
    if(scene.effect_models.empty())return;
    const auto clean=[](uintptr_t model){std::string name;for(const auto ch:NativeModelDataName(model))name.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');return name.substr(0,40);};
    static const bool witness=ModuleFlag(L"ffx-record.flag");
    if(witness){   // ffx-record.flag: each model's first submission too (Lightning Spear: the original draws s03000 about ten frames a
        // cast, which the sampled rows miss): its pass, own material table, bones, admission code (-1: not a candidate), forward
        // rows (0x29 or 0x21) and forward light (-1: no admitted model); the first 4 submissions of the first 64 data names and passes
        // of the session. I-0113 gap 5: with the blend and depth bank offsets the effect context held and the command that bound each
        // (its pass, key and renderer class; pass -1: none bound in this execution), against the Reference's reference_e03_states.
        static std::mutex seen_lock;static std::map<std::string,int> seen;std::string fresh;
        {const std::lock_guard hold(seen_lock);
            for(const auto& effect:scene.effect_models){const auto& m=effect->submission;const auto name=clean(m.model);const auto key=name+"/"+std::to_string(m.pass);
                if(!seen.contains(key) && seen.size()>=64)continue;if(seen[key]++>=4)continue;
                int code=-1,rows=-1,lit=-1;for(size_t i=first;i<deferred.size();++i)if(deferred[i].candidate.model==m.model){code=deferred[i].code;break;}
                for(size_t i=0;i<scene.models.size();++i)if(scene.models[i].effect==effect.get()){rows=0;lit=scene.models[i].forward_light!=nullptr;
                    for(const auto& p:scene.prepared_meshes)if((p.engine_pass==0x29 || p.engine_pass==0x21) && p.model==i)++rows;break;}
                const auto& b=m.blend_origin;const auto& d=m.depth_origin;char cell[460]{};
                sprintf_s(cell,"%s{\"model\":\"%s\",\"pass\":%d,\"key\":%u,\"renderer\":\"%llX\",\"materials\":%d,\"bones\":%d,\"code\":%d,\"rows\":%d,\"lit\":%d,"
                    "\"blend\":\"%X\",\"blend_from\":[%d,%u,\"%llX\"],\"depth\":\"%X\",\"depth_from\":[%d,%u,\"%llX\"],\"cull\":[\"%X\",%d]}",fresh.empty()?"":",",
                    name.c_str(),m.pass,m.key,static_cast<unsigned long long>(m.renderer),int(m.materials!=0),int(m.bones!=0),code,rows,lit,
                    m.blend,b.pass,b.key,static_cast<unsigned long long>(b.renderer),m.depth,d.pass,d.key,static_cast<unsigned long long>(d.renderer),m.arg_cull,m.arg_cull_mode);fresh+=cell;
                fresh.pop_back();fresh+=",\"sequence\":[";   // the commands run before it in its execution: [pass, key, class, blends set]
                for(size_t i=0;i<m.sequence.size();++i){const auto& s=m.sequence[i];char step[80]{};
                    sprintf_s(step,"%s[%d,%u,\"%llX\",%u]",i?",":"",s.command.pass,s.command.key,static_cast<unsigned long long>(s.command.renderer),s.blends);fresh+=step;}
                fresh+="]}";}}
        if(!fresh.empty())Log(("{\"event\":\"native_effect_model_first\",\"models\":["+fresh+"]}\n").c_str());
        // The submissions and admitted candidates by data name over the session, every 3000th submission (first at 30), as the
        // Reference's reference_ffx_model_draws counts 636F0's calls: the two compare draw for draw.
        static std::map<std::string,std::array<uint64_t,2>> tally;static uint64_t submissions{};std::string row;
        {const std::lock_guard hold(seen_lock);bool due=false;
            for(const auto& effect:scene.effect_models){auto& t=tally[clean(effect->submission.model)];++t[0];due|=++submissions%3000==30;}
            for(size_t i=first;i<deferred.size();++i)if(!deferred[i].code)++tally[clean(deferred[i].candidate.model)][1];
            if(due){row="{\"event\":\"native_effect_model_tally\",\"submissions\":"+std::to_string(submissions)+",\"models\":[";bool lead=true;
                for(const auto& [name,t]:tally){row+=(lead?"[\"":",[\"")+name+"\","+std::to_string(t[0])+","+std::to_string(t[1])+"]";lead=false;}row+="]}\n";}}
        if(!row.empty())Log(row.c_str());}
    static LONG64 collects{};const auto n=InterlockedIncrement64(&collects);if(n>3 && n%600)return;
    uint32_t other_pass{},own_materials{},no_bones{},candidates{},admitted{};std::array<uint32_t,8> refused{};std::string names;
    for(const auto& effect:scene.effect_models){const auto& m=effect->submission;
        const bool drawn_pass=m.pass==0x29 || m.pass==0x21;other_pass+=!drawn_pass;own_materials+=drawn_pass && m.materials;no_bones+=drawn_pass && !m.materials && !m.bones;}
    for(size_t i=first;i<deferred.size();++i){++candidates;const auto code=deferred[i].code;if(!code)++admitted;else ++refused[code<refused.size()?code:refused.size()-1];
        if(names.size()<240)names+=(names.empty()?"\"":",\"")+clean(deferred[i].candidate.model)+":"+std::to_string(code)+"\"";}
    uint32_t with_rows{},lit{},own_light{},full_rows{};std::vector<bool> rows(scene.models.size());
    for(const auto& p:scene.prepared_meshes)if((p.engine_pass==0x29 || p.engine_pass==0x21) && p.model<rows.size())rows[p.model]=true;
    for(size_t i=0;i<scene.models.size();++i){const auto& m=scene.models[i];if(!m.effect)continue;
        with_rows+=rows[i];lit+=rows[i] && m.forward_light;own_light+=m.effect->submission.light[0x44]!=0;
        BYTE gated{};full_rows+=Read(m.model+0x1E0,&gated,1) && gated;}   // the model's +0x1E0 (INFERRED: a particle's rows are full rows)
    char row[480]{};sprintf_s(row,"{\"event\":\"native_effect_models\",\"collect\":%lld,\"submissions\":%zu,\"other_pass\":%u,\"own_materials\":%u,\"no_bones\":%u,\"candidates\":%u,\"admitted\":%u,\"refused\":[%u,%u,%u,%u,%u,%u,%u,%u],\"with_rows\":%u,\"lit\":%u,\"own_light\":%u,\"full_rows\":%u,\"models\":[",
        static_cast<long long>(n),scene.effect_models.size(),other_pass,own_materials,no_bones,candidates,admitted,refused[0],refused[1],refused[2],refused[3],refused[4],refused[5],refused[6],refused[7],with_rows,lit,own_light,full_rows);
    Log((std::string(row)+names+"]}\n").c_str());
}
// cP7 (R8b S4): each model's forward cbLight/cbFog entries (the engine's sun and fog builders over its group keys), built
// once at collect for the models with a forward row, so prepare calls no engine function; a model whose entries cannot be
// built keeps none and its forward rows are refused as before.
void CollectNativeForwardLights(NativeSelectedScene& scene,bool main_view) {
    std::vector<bool> forward(scene.models.size());
    for(const auto& p:scene.prepared_meshes)if(p.engine_pass==0x29 || p.engine_pass==0x1F || p.engine_pass==0x21 || p.engine_pass==0x22 || p.engine_pass==0x24 || p.engine_pass==0x27)forward.at(p.model)=true;   // and the water rows (WT-3, WT-4)
    // I-0120 D4: the entries depend only on the group's keys and factors and on the collect's context (banks, view): built once per
    // key in a collect and shared by the models that have it (every forward model had its sun and fog builders run, ~80 reads each).
    std::map<std::array<uint32_t,12>,std::shared_ptr<const NativeSelectedModel::ForwardLight>> built;
    for(size_t index=0;index<scene.models.size();++index)if(forward[index]){std::array<uint32_t,12> key;
        if(!NativeForwardLightKey(scene,scene.models[index].group,key))continue;
        if(const auto found=built.find(key);found!=built.end()){scene.models[index].forward_light=found->second;continue;}   // a failed key: none
        auto entries=std::make_shared<NativeSelectedModel::ForwardLight>();
        bool authored{};
        if(ProduceNativeForwardLight(scene,key,&entries->light,entries->fog,&authored)){
            // I-0132 R11d's final review M6 (D-0262): the forward rows' own cbLight sets lose their fill lights (lights 1 to 3: Col and Spc
            // rows 5-7 and 9-11 of each 288-byte set) as the deferred block does, the session's GI asked (its frames without the GI's
            // inputs, magenta in the test runs, are the only ones where the two differ).
            // I-0141: but for a cutscene's own set (its lights 1 to 3 the cut's authored lights), a map key the act overrides by one
            // included (tranche 5, D-0282).
            if(main_view && renderer.rt_gi.load(std::memory_order_relaxed) && !authored)   // the main view's (its fixes' step review M2: not the menu views')
                for(size_t set=0;set+288<=entries->light.size();set+=288)for(const size_t row:{5,6,7,9,10,11})std::memset(entries->light.data()+set+16*row,0,16);
            // m50 sky (S4): the first 16 forward models' fog entries by name (FogParam, FogCol, ExtFogParam, HeightFogParam, HeightFogCol, FogType)
            // and instance colour (group +0xD0).
            static const bool fog_diagnostics=ModuleFlag(L"light-observe.flag");   // review: diagnostics only with light-observe.flag
            if(fog_diagnostics){static std::mutex fog_witness_lock;static std::set<std::string> fog_witness_seen;const auto name=NativeModelDataName(scene.models[index].model);bool fresh{};
                {const std::lock_guard hold(fog_witness_lock);fresh=fog_witness_seen.size()<16 && fog_witness_seen.insert(name).second;}
                if(fresh){std::array<float,24> f{};std::memcpy(f.data(),entries->fog.data(),(std::min)(entries->fog.size(),sizeof(f)));std::array<float,4> colour{};Read(scene.models[index].group+0xD0,colour.data(),16);
                    std::string row="{\"event\":\"native_forward_fog\",\"model\":\"";for(const auto ch:name)row.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');row+="\",\"fog\":[";
                    for(size_t i=0;i<f.size();++i){char v[24]{};sprintf_s(v,"%s%g",i?",":"",f[i]);row+=v;}
                    std::array<float,8> scroll{};if(scene.models[index].texture_scroll)std::memcpy(scroll.data(),scene.models[index].texture_scroll->data(),32);
                    char tail[320]{};sprintf_s(tail,"],\"colour\":[%g,%g,%g,%g],\"near_far\":[%g,%g],\"scroll_present\":%s,\"scroll\":[%g,%g,%g,%g,%g,%g,%g,%g]}\n",colour[0],colour[1],colour[2],colour[3],scene.camera.near_plane,scene.camera.far_plane,scene.models[index].texture_scroll?"true":"false",
                        scroll[0],scroll[1],scroll[2],scroll[3],scroll[4],scroll[5],scroll[6],scroll[7]);row+=tail;Log(row.c_str());}}
            built.emplace(key,entries);scene.models[index].forward_light=std::move(entries);}
        else built.emplace(key,nullptr);}   // the final review's L5: the builders not run again for the key
}
// I-0125 R6d (R6-PLAN §7): the rays' forward instances' blocks (RtForwardRow), once the frame's light blocks are made (the context
// ProduceNativeForwardLight reads), one a draw group, shared by its listed instances (R6d's final review: not read and made again for
// each): its fog, the model's own entry when the raster drew the group this collect (CollectNativeForwardLights'), else that entry built
// the same way (the fog entry alone); its mulCol the group's +0xD0 as ProduceInstanceRecord writes it (+0x70). A group whose entry
// cannot be built or whose mulCol cannot be read gives its instances no block, and the rays pass their forward slices over (the raster
// refuses its forward rows, or the instance, then: the step review's L1; R6d's final review, an unread mulCol was taken white). The
// main view's collect, after CollectNativeForwardLights, on the walk's own unpublished input (rt_scene_input), released here.
void CollectNativeRtForwardBlocks(NativeSelectedScene& scene) {
    const auto input=std::move(scene.rt_scene_input);
    if(!input || scene.rt_forward_groups.empty())return;
    std::unordered_map<uintptr_t,std::shared_ptr<RtForwardRow>> made;made.reserve(scene.rt_forward_groups.size());
    for(const auto& [index,group]:scene.rt_forward_groups)made.try_emplace(group);
    std::unordered_set<uintptr_t> built;   // the groups the raster drew: their model's own entry (the step review's L6: only the listed groups)
    for(const auto& m:scene.models)if(m.forward_light && m.forward_light->fog.size()>=96)if(const auto found=made.find(m.group);found!=made.end() && built.insert(m.group).second){
        found->second=std::make_shared<RtForwardRow>();std::memcpy(found->second->fog.data(),m.forward_light->fog.data(),96);}
    for(auto& [group,block]:made){
        if(!built.count(group)){std::vector<uint8_t> entry;
            if(ProduceNativeForwardLight(scene,group,nullptr,entry) && entry.size()>=96){block=std::make_shared<RtForwardRow>();std::memcpy(block->fog.data(),entry.data(),96);}}
        if(block && !Read(group+0xD0,block->colour.data(),16))block.reset();}
    for(const auto& [index,group]:scene.rt_forward_groups)if(index<input->instances.size())input->instances[index].ambient.forward=made[group];
}
// I-0077: what a walk keeps from one collect to the next (per layer position: visits, the order of its tickets, and gathered
// counts, the reserves); one for each walk (the main walk, the rays' walk).
struct NativeWalkHistory {std::vector<uint32_t> objects;std::vector<size_t> sizes;};
// I-0132 R11g: the module's own calls on the traversers' engine heap (the view's, 234620's argument) made by one thread at a time:
// their construction with the bounds stack's reserve (19C1D0), their destruction (234720) and the guard's serial walk (its stack grows
// from that heap). The main walk on the engine thread and the rays' walk on its own thread run at once. Not covered: a worker's bounds
// stack grown past its reserve (never seen, I-0132 RESULT « R11g ») and engine code on the engine thread allocating from that heap
// meanwhile; that heap's own thread safety is not read (INFERRED safe: DS3's own layer tasks, C1D10, run and free their traverser
// clones on the GX workers, COLLECT-ORIGINAL-STATIC §2).
std::mutex native_traverser_heap_lock;
// I-0077 (ER A, GXLayerTraverseJob, ER-STUDY section 1; DS3's own C1FB0 makes the same split when its traverser allows
// deferral: one task per layer {layer, traverser clone}, COLLECT-ORIGINAL-STATIC section 2): the main and rays' walks run
// C1FB0's inline branch themselves, layer by layer (world+0x10..+0x18, each layer's vt+8 with the traverser), each layer with
// its own traverser (234620, as C1FB0's clone), the layers taken by ticket on the admission pool, largest first by the previous
// collect's visits (a fresh traverser equals the shared one at a layer's start: every layer leaves the bounds stack balanced,
// and the bytes +0x10..+0x13 stay zero). As ER's entity visit (141B4CF40) the traversers only test the entities that need the
// bounds stack and gather them; the gathered entities, joined in layer order (C1FB0's serial order), are then expanded into
// candidates on the pool in chunks (ER's GXEntityDrawParallelJob), joined in the same order. The view flags of the collecting
// thread are set on the workers. light-observe.flag walks on this thread and keeps the
// whole filter in the walk (its refusal diagnostics). Main walk: a witness row every 600 collects (per layer its class, visits,
// gathered entities and time) and, under record-revalidation.flag, the serial walk compared every 64th collect.
void WalkNativeLayers(uintptr_t world,uintptr_t heap,uint64_t mask,uintptr_t culler,uint32_t world_mask,const uintptr_t* table,
    NativeSelectedScene& result,std::vector<NativeDeferredCandidate>& deferred,NativeWalkHistory& history,
    bool main_walk,struct NativeRtWalkShare* rays=nullptr);
// I-0117 T5b: the rays' walk on the pool (WalkNativeLayers with `rays`): each job of the walk sets this thread's native_rt_gates
// and native_rt_walk_failed for its time (an entity whose visit fails is skipped alone on a worker too, never the game closed),
// then adds its gates and failure here; its traversers are `uncounted` (the entities not counted against the scene's bound).
struct NativeRtWalkShare {std::array<std::atomic<uint32_t>,64> gates{};std::atomic<bool> failed{};
    struct Scope {NativeRtWalkShare* share;std::array<uint32_t,64> gates{};std::array<uint32_t,64>* saved{};bool saved_failed{};
        explicit Scope(NativeRtWalkShare* s) noexcept:share(s){if(!share)return;saved=native_rt_gates;saved_failed=native_rt_walk_failed;native_rt_gates=&gates;native_rt_walk_failed=false;}
        ~Scope(){if(!share)return;for(size_t g=0;g<gates.size();++g)if(gates[g])share->gates[g].fetch_add(gates[g],std::memory_order_relaxed);
            if(native_rt_walk_failed)share->failed.store(true,std::memory_order_relaxed);native_rt_gates=saved;native_rt_walk_failed=saved_failed;}
        Scope(const Scope&)=delete;Scope& operator=(const Scope&)=delete;};};
// I-0117 (R2 T1, R2-PLAN §3; Thomas 2026-10-30 15:41: the rays see everything the engine has loaded): the loaded world's
// instances. The world walk again, as the shadow walks, with a culler copied from the view's whose six planes and world box hold
// the whole world (every loaded entity passes the frustum tests), on the main view's categories with the sun's 0x02, the local
// lights' 0x04 and the reflection's 0x10; the entities' eligibility gates are the walks' own (AdmitNativeSelectedObject), and
// their candidates are gathered (`deferred`, never admitted: nothing is prepared for drawing). Each candidate becomes an instance
// of its model storage's BLAS at its placement; the skinned ones (an assembled character's parts, a pose of more than one row)
// a skinned instance of their own (T2, NativeRtSkinnedOf). Counts in `counts` (instances, map pieces, objects, skinned, no storage,
// no BLAS, not finite). Null, or why not.
constexpr uint32_t kNativeRtProgramsAWalk=16;   // T5: the programme sets the rays build a walk for models the raster has not admitted
// and the time they may take a walk: 1 ms (a build started under it ends past it).
uint64_t NativeRtProgramsTicks() noexcept {static const uint64_t ticks=[]{LARGE_INTEGER f{};QueryPerformanceFrequency(&f);return uint64_t(f.QuadPart/1000);}();return ticks;}
struct NativeRtWalkCounts {uint64_t candidates{},instances{},map{},objects{},skinned{},no_storage{},no_blas{},nonfinite{},suppressed{};
    std::array<uint64_t,3> skinned_kinds{};std::array<uint32_t,64> gates{};   // skinned by kind (map item, assembled part, draw entity)
    std::array<uint64_t,4> menu_no_storage{};   // I-0140 t31, the menu walk's no_storage: no receiver, not stored, a skinned one not readable, pending
    // T2: the skinned instances made, their slices skinned and excluded (a mesh not drawn: its flags, LOD or capabilities) or not
    // supported (its layout, its pose binding, its stream), the candidates left out (no pose, no slice, their inputs); the slices
    // read from a physics stream (T2b) and from a cloth mesh's simulated copy (T2c).
    // Duplicates: a model object met twice in one walk (its one BLAS taken once, its masks joined; step review M2), of which with
    // another placement (final review L1); poses: a model object met again with another pose, its own instance (the T3 run: every
    // duplicate of the T2 rule had another pose or placement); held: instances whose pose has a row not finite.
    uint64_t skinned_instances{},skinned_slices{},excluded_slices{},unsupported_slices{},no_pose{},no_slice{},no_inputs{},streamed{},cloth{},cloth_framed{},cloth_frame_missing{},cloth_left_out{},duplicates{},
        duplicates_differing{},poses{},held{};
    // T3: the instances with an alpha-tested slice and those slices; of the alpha-tested slices, those whose diffuse texture is not
    // resident yet (opaque until it is) and those whose UV element the rays do not read (opaque); the sets made this walk (I-0119:
    // every material set, alpha-tested or not).
    uint64_t alpha_instances{},alpha_slices{},alpha_unresolved{},alpha_unsupported{},alpha_made{},alpha_untextured{},alpha_unknown{};
    // I-0119 R3a: the LOD-0 slices given a material (their G-buffer programme read and its g_DiffuseTexture resolved; since build-8,
    // before it every slice with its programme read), their textures by role (diffuse, specular,
    // bump, emissive) named by the programme and of them resolved, those not resident yet (tried again), the slices without a
    // G-buffer programme read (no material: the triangle and the alpha test alone), without a UV0 or a vertex frame the rays read.
    uint64_t material_slices{},material_pending{},material_no_gbuf{},material_no_uv{},material_no_frame{};
    uint64_t material_blends{},material_no_weights{};   // R3c: the map blends' slices, of them without vertex weights read
    uint64_t material_forward{},material_forward_made{};   // I-0125 R6d: the forward slices met, of them with a record (NativeRtMaterialTally)
    uint64_t material_emissive{};   // I-0126 R7b: the slices with an emission (FC_EmissiveColor read, above 0)
    std::array<uint64_t,4> material_named{},material_resolved{};
    // T5: the instances whose view bit became RtMaskForward (no surface slice), whose sun or local bits went (no slice that casts),
    // those whose slices were not all classed (the engine's masks kept), and those whose slices answer different rays (their
    // records; `overlaid`: of them, those with a slice that is not a surface).
    uint64_t forward_only{},uncast{},unclassed{},mixed{},overlaid{};
    uint64_t translucent{};   // I-0118 R5c: the casters with a transparent slice that casts
    // T6: the instances moved by the wind (their slices skinned while it blows: placed or posed), those whose set moves by the wind
    // with no wind blowing (their animation block unset, no wind counted, or no model size: the rest pose, as before T6), the slices
    // moved by the wind in the walk's sets and those whose wind is not read; a placed one met again at its placement (one instance)
    // and at another (that one static, in its rest pose).
    uint64_t wind_instances{},wind_calm{},wind_slices{},wind_unsupported{},wind_duplicates{},wind_differing{};
    // T5: the programme sets the walk built (AcquireNativeModelPrograms returned a set) and their time (ticks), from its budget.
    uint32_t programs_built{};uint64_t programs_ticks{};
    uint64_t traverse_ticks{};   // T5: the engine traversal's time (QueryPerformanceCounter ticks; T5b: WalkNativeLayers' whole, its gates included)
    // T5b: the static instances' time (the pool's chunks and their join) and the skinned candidates'; T5c: the threads that took a
    // static chunk this walk.
    uint64_t static_ticks{},skinned_ticks{};uint32_t static_workers{};
    struct NativeRtProgramsBudget* budget{};   // T5b: the walk's, shared by its chunks (set for the walk only)
    // T5b: a chunk's counts added to the walk's (every count but the gates and the traversal's, the walk's own).
    void Add(const NativeRtWalkCounts& o) noexcept {
        candidates+=o.candidates;instances+=o.instances;map+=o.map;objects+=o.objects;skinned+=o.skinned;no_storage+=o.no_storage;no_blas+=o.no_blas;
        nonfinite+=o.nonfinite;suppressed+=o.suppressed;for(size_t k=0;k<skinned_kinds.size();++k)skinned_kinds[k]+=o.skinned_kinds[k];
        skinned_instances+=o.skinned_instances;skinned_slices+=o.skinned_slices;excluded_slices+=o.excluded_slices;unsupported_slices+=o.unsupported_slices;
        no_pose+=o.no_pose;no_slice+=o.no_slice;no_inputs+=o.no_inputs;streamed+=o.streamed;cloth+=o.cloth;cloth_framed+=o.cloth_framed;cloth_frame_missing+=o.cloth_frame_missing;cloth_left_out+=o.cloth_left_out;duplicates+=o.duplicates;
        duplicates_differing+=o.duplicates_differing;poses+=o.poses;held+=o.held;
        alpha_instances+=o.alpha_instances;alpha_slices+=o.alpha_slices;alpha_unresolved+=o.alpha_unresolved;alpha_unsupported+=o.alpha_unsupported;
        alpha_made+=o.alpha_made;alpha_untextured+=o.alpha_untextured;alpha_unknown+=o.alpha_unknown;
        material_slices+=o.material_slices;material_pending+=o.material_pending;material_no_gbuf+=o.material_no_gbuf;material_no_uv+=o.material_no_uv;
        material_no_frame+=o.material_no_frame;material_blends+=o.material_blends;material_no_weights+=o.material_no_weights;
        material_forward+=o.material_forward;material_forward_made+=o.material_forward_made;material_emissive+=o.material_emissive;for(size_t k=0;k<4;++k){material_named[k]+=o.material_named[k];material_resolved[k]+=o.material_resolved[k];}
        forward_only+=o.forward_only;uncast+=o.uncast;unclassed+=o.unclassed;mixed+=o.mixed;overlaid+=o.overlaid;translucent+=o.translucent;
        wind_instances+=o.wind_instances;wind_calm+=o.wind_calm;wind_slices+=o.wind_slices;wind_unsupported+=o.wind_unsupported;
        wind_duplicates+=o.wind_duplicates;wind_differing+=o.wind_differing;}
};
// T5: the programme sets a walk may still build (kNativeRtProgramsAWalk, and while their time is under kNativeRtProgramsTicks);
// T5b: shared by the walk's chunks on the pool (their builds' time summed, so the bound holds across threads).
struct NativeRtProgramsBudget {std::atomic<uint32_t> left{kNativeRtProgramsAWalk},built{};std::atomic<uint64_t> ticks{};
    bool Open() const noexcept {return left.load(std::memory_order_relaxed) && ticks.load(std::memory_order_relaxed)<NativeRtProgramsTicks();}
    bool Take() noexcept {for(uint32_t n=left.load(std::memory_order_relaxed);n;)if(left.compare_exchange_weak(n,n-1,std::memory_order_relaxed))return true;return false;}};
// I-0117 (R2 T2): the skinned instances kept from one walk to the next by model instance and pose (the engine model object and
// the pose that places it: one model object posed twice is two instances, T3): its GPU state (RtSkinnedBlas, the recording thread's) and the slices it was made for; new slices (another storage or
// layout) make a new one. One the walk did not reach for two walks is dropped: its positions and BLAS go once the frames holding
// them completed. The walk reads an entry and writes it back under the lock (T5c: from the pool, one job a model object, so one
// entry is never read and written by two threads in a walk); the shutdown's close clears the map and
// marks it closed, after which nothing is written back (T3 step review L1); a scene reset clears it too (ResetNativeRtKept).
// The palette rows its slices index (RtSkinRowsOf, from the storage's bytes once an entry), and whether one of those rows is not
// finite (`hold`), read once a pose snapshot (`hold_generation`; T2 step review, carried: neither every row bound nor every walk).
struct NativeRtSkinnedEntry {std::shared_ptr<RtSkinnedBlas> blas;std::shared_ptr<const std::vector<RtSkinSlice>> slices;ID3D12Resource* source{};uint64_t seen{};
    std::shared_ptr<const RtSkinRows> rows;uint64_t hold_generation{UINT64_MAX};bool hold{};};
struct NativeRtSkinKey {uintptr_t model{},pose{};bool operator==(const NativeRtSkinKey&) const=default;};
// The key's hash mixed (T5c step review L1, its final review L2 for every map of these keys: a model object's address keeps its low
// bits constant, a bucket would read only them).
struct NativeRtSkinKeyHash {size_t operator()(const NativeRtSkinKey& k) const noexcept {uint64_t h=(k.model^(k.pose*0x9E3779B97F4A7C15ull))*0xBF58476D1CE4E5B9ull;
    return static_cast<size_t>(h^(h>>31));}};
std::mutex native_rt_skinned_lock;std::unordered_map<NativeRtSkinKey,NativeRtSkinnedEntry,NativeRtSkinKeyHash> native_rt_skinned;uint64_t native_rt_skinned_walk{};std::atomic<bool> native_rt_skinned_closed{};   // atomic since I-0140 t31 (read under the menu entries' lock too)
// I-0140 tranche 31 (D-0303): the Player menu views' own skinned entries (their characters'), apart from the main walk's, and their
// walk (one a menu collect that traces a view, CollectNativeMenuRtScene): the main walk's eviction and revalidation keep their own
// cadence, and character creation, which has no main walk, still drops what it no longer draws. Taken by NativeRtSkinnedOf on the
// thread that sets native_rt_menu_caches (the menu collect, under native_offscreen_collect_lock); the same close.
std::mutex native_rt_menu_skinned_lock;std::unordered_map<NativeRtSkinKey,NativeRtSkinnedEntry,NativeRtSkinKeyHash> native_rt_menu_skinned;uint64_t native_rt_menu_walk{};
// Reviewed 2026-11-21 (I-0140 tranche 31, the step review of the fixes, L4; outside the thread-affinity checker's bodies): set and
// cleared only by CollectNativeMenuRtScene, under native_offscreen_collect_lock; read by the cache selectors on that thread.
thread_local bool native_rt_menu_caches{};
std::mutex& NativeRtSkinnedLock() noexcept {return native_rt_menu_caches?native_rt_menu_skinned_lock:native_rt_skinned_lock;}
std::unordered_map<NativeRtSkinKey,NativeRtSkinnedEntry,NativeRtSkinKeyHash>& NativeRtSkinnedMap() noexcept {return native_rt_menu_caches?native_rt_menu_skinned:native_rt_skinned;}
void CloseNativeRtSkinned() noexcept {{const std::lock_guard hold(native_rt_skinned_lock);native_rt_skinned.clear();native_rt_skinned_closed=true;}
    const std::lock_guard hold(native_rt_menu_skinned_lock);native_rt_menu_skinned.clear();}
// I-0117 T6: a candidate's wind block as the raster binds it (13b_prepare_model_frame.inc's cbvModelParam): VC_ForceParam the
// animation block [anim+210] when [anim+620] is set (else zero: no wind), VC_ModelSizeParam the draw group's +0x60. Null when no
// wind blows (the flag clear or unread, neither count of WindForceLoopParam above 0) or the size's y is not a positive finite number
// (the programme's bend is then not finite, a triangle the rasterizer drops): the rays keep the rest pose.
std::shared_ptr<const RtWindBlock> NativeRtWindBlockOf(const NativeSelectionCandidate& c) {
    BYTE flag{};if(!c.animation || !Read(c.animation+0x620,&flag,1) || !flag)return nullptr;
    auto block=std::make_shared<RtWindBlock>();
    if(!Read(c.animation+0x210,block->data(),kRtWindForceBytes) || !Read(c.group+0x60,block->data()+kRtWindForceBytes,16))return nullptr;
    int32_t loops[2]{};std::memcpy(loops,block->data(),sizeof loops);float size[4]{};std::memcpy(size,block->data()+kRtWindForceBytes,sizeof size);
    if((loops[0]<=0 && loops[1]<=0) || !std::isfinite(size[1]) || !(size[1]>0))return nullptr;
    return block;
}
uint64_t NativeRtWindHash(const RtWindBlock& block) noexcept {uint64_t h=1469598103934665603ull;for(const uint8_t b:block)h=(h^b)*1099511628211ull;return h;}
// I-0117 T6: a placed candidate (a map piece or an object of one placement) whose material set moves by the wind, while its wind
// blows: its own positions and BLAS (RtSkinner) in place of the storage's, every slice that BLAS takes placed by the world row (mode
// 3, the placement the static instance's transform) and the set's wind slices moved; kept by model object and pose with the skinned
// ones (dropped two walks after the last walk that met it, cleared at a scene reset); skinned again when the wind block or the
// placement changed. False when no slice the BLAS takes moves, or after the close.
bool NativeRtWindedOf(const NativeSelectionCandidate& c,const NativeModelStorage& storage,const RtMaterialSet& set,std::shared_ptr<const RtWindBlock> wind,uint64_t walk,RtSkinnedInstance& out) {
    auto slices=std::make_shared<std::vector<RtSkinSlice>>(storage.meshes.size());bool moved=false;
    for(size_t i=0;i<slices->size();++i){auto& s=(*slices)[i];s.slice=storage.meshes[i];if(!RtSliceEligible(s.slice))continue;s.mode=3;
        if(i<set.slices.size() && set.slices[i].wind){s.wind=set.slices[i].wind;s.wind_offset=set.slices[i].wind_offset;moved=true;}}
    if(!moved)return false;
    uint64_t generation=NativeRtWindHash(*wind);for(const float v:c.placement){uint32_t bits{};std::memcpy(&bits,&v,4);generation=(generation^bits)*1099511628211ull;}
    out.source=storage.resource;out.world=c.placement;out.generation={generation,0};out.wind=std::move(wind);out.framed=true;   // its frame the placement's (the step review's M1)
    const std::lock_guard hold(native_rt_skinned_lock);if(native_rt_skinned_closed)return false;
    auto& kept=native_rt_skinned[{c.model,c.pose}];
    if(!kept.blas || kept.source!=storage.resource.Get() || !kept.slices || *kept.slices!=*slices){kept=NativeRtSkinnedEntry{};kept.blas=std::make_shared<RtSkinnedBlas>();
        kept.slices=std::move(slices);kept.source=storage.resource.Get();}
    kept.seen=walk;out.blas=kept.blas;out.slices=kept.slices;return true;
}
// The G-buffer vertex programme's shape (recipes 0-2, the deferred pass's) from the layout's kept programme set (the admission's,
// built on its pool; none is built here: T2 final review L4): whether it indexes the palette (VS b5) by NORMAL.w (rigid) or by
// the four BLENDINDICES with BLENDWEIGHT (blended), and whether it reads VS b12. Not known before the raster admitted a model of
// that layout. Read once a programme set (its `rt_shape`; T2 step review, carried: not again each walk).
struct NativeRtProgrammeShape {bool known{},rigid{},blended{},cancel{};};
NativeRtProgrammeShape NativeRtShapeOf(NativeModelLayout& layout) {
    std::shared_ptr<const NativeModelLayout::Programs> programs;
    {const std::lock_guard hold(layout.programs_lock);for(const auto& choice:layout.choices)if(choice.programs){programs=choice.programs;break;}}
    NativeRtProgrammeShape shape;if(!programs)return shape;
    if(const auto kept=programs->rt_shape.load(std::memory_order_relaxed))return {(kept&1)!=0,(kept&2)!=0,(kept&4)!=0,(kept&8)!=0};
    for(size_t recipe=0;recipe<3;++recipe){const auto& face=programs->interfaces[recipe][0];
        if(programs->recipes[recipe].status!=ds3content::ModelProgramStatus::Resolved || !face)continue;
        const auto used=[&](const char* semantic,uint8_t bits){for(const auto& input:face->inputs)
            if(!input.semantic_index && _stricmp(input.semantic.c_str(),semantic)==0)return (input.usage_mask&bits)==bits;return false;};
        const auto& reads=programs->ReadsOf(recipe,0);const bool palette=reads.dynamic.contains(5);
        shape.known=true;shape.cancel=reads.dynamic.contains(12);
        shape.blended=palette && used("BLENDINDICES",0xF) && used("BLENDWEIGHT",0xF);
        shape.rigid=palette && !shape.blended && used("NORMAL",0x8);
        break;}
    programs->rt_shape.store(static_cast<uint8_t>(0x80|(shape.known?1:0)|(shape.rigid?2:0)|(shape.blended?4:0)|(shape.cancel?8:0)),std::memory_order_relaxed);
    return shape;
}
// A skinned candidate as the raster would draw it (AdmitNativeModelCandidate, the ordinary pass rows, PrepareNativeModelFrame's
// pose binding): its storage's slices by the engine mesh's ordinal; a mesh taken when its flags hold bit 0, its LOD mask LOD 0
// (the face set the storage keeps) and its material's capability words meet the model's (63CB0); placed as its G-buffer vertex
// programme does, when that is one of the shapes rt_skin.hlsl reproduces: rigid (layout type 0, the row NORMAL.w, no b12) or
// blended (BLENDINDICES, BLENDWEIGHT), on the pose's current or auxiliary rows as SelectOrdinaryModelPoseBinding chooses them,
// with the cancel rows when the programme reads VS b12 and the binding has them or a cloth pose retains them (the 1120 rule), an
// auxiliary binding without them refused as the raster refuses it; a cloth mesh's simulated copy as its source, placed by the
// world row when its material enables the cloth (VC_isEnableCloth = material +0x22 with the copy bound, the admission's 1184 rule).
// As the raster (AdmitNativeModelCandidate's mesh loop, before any mesh's own test: T2 step review, carried), the model stays out
// when its capability words cannot be read, or a mesh of the face set the rays take (LOD 0) has no material or layout in their
// registries, its layout's material retired, or no physics stream while the model has some. A pose with a row not finite among
// the rows its slices index holds the instance's last positions (`hold`). T6: the slices `wind_set` moves by the wind are marked,
// and with a wind block blowing (`wind`) moved after their pose (the _GBuf_Skin Grass programmes), its hash in the generation. Null,
// or why it stays out.
const char* NativeRtSkinnedOf(const NativeSelectionCandidate& c,const std::shared_ptr<NativeModelStorage>& storage,uint64_t walk,RtSkinnedInstance& out,NativeRtWalkCounts& counts,
    const RtMaterialSet* wind_set,const std::shared_ptr<const RtWindBlock>& wind) {
    ingress::CurrentModelInputs inputs;
    if(!ingress::ReadCurrentModelInputsAt(image,c.model,c.source_field,c.instance_field,c.pose,inputs) || inputs.instance_streams ||
       inputs.meshes.size()!=storage->meshes.size()){++counts.no_inputs;return "inputs";}
    // The snapshot's generation is the engine-wide one (each pose update a new one; T2 final review L3: a version's own restarts
    // with its owner).
    std::shared_ptr<const NativeModelPoseSnapshot> snapshot;
    {const std::shared_lock hold(model_pose_lock);const auto found=model_poses.find(c.pose);
        if(found!=model_poses.end() && found->second.model==c.model && found->second.current)snapshot=found->second.current;}
    const auto pose=snapshot?snapshot->palettes:nullptr;
    if(!pose || !pose->current.Available() || pose->current.rows.empty()){++counts.no_pose;return "pose";}
    // T2b: the physics streams (UpdateNativePhysicalModelStreams), the source of the meshes they rewrite, by the mesh's patch index
    // (D3FA10, as the admission binds them); held by the version this frame reads.
    std::shared_ptr<const NativeModelStreamVersion> streams;
    if(inputs.patch){const std::lock_guard hold(model_stream_lock);const auto found=model_streams.find(inputs.patch);
        if(found!=model_streams.end() && found->second.model==c.model && found->second.storage.get()==storage.get())streams=found->second.current;}
    std::array<uint64_t,2> capabilities{};bool capable=false;
    if(c.capability_source){capable=Read(c.capability_source,capabilities.data(),sizeof capabilities);capabilities[1]|=uint64_t(1)<<63;}
    else capable=Read(c.group+0xC0,capabilities.data(),sizeof capabilities);
    if(!capable){++counts.no_inputs;return "capabilities";}   // the raster's gate 2: the model not drawn
    // The raster's whole-model gates, for every mesh of LOD 0 first (T2 final review M2; its step review, carried: the material
    // gate the registry's, as the raster's).
    std::vector<std::shared_ptr<NativeModelLayout>> layouts(inputs.meshes.size());
    for(size_t i=0;i<inputs.meshes.size();++i){const auto& mesh=inputs.meshes[i];if(!(mesh.lod_mask&1))continue;
        std::shared_ptr<NativeModelMaterial> material;
        {const std::shared_lock hold(model_material_lock);const auto found=model_materials.find(mesh.material);if(found!=model_materials.end())material=found->second;}
        if(!material){++counts.no_inputs;return "material";}
        {const std::shared_lock hold(model_layout_lock);const auto found=model_layouts.find(mesh.layout);if(found!=model_layouts.end())layouts[i]=found->second;}
        if(!layouts[i]){++counts.no_inputs;return "layout";}
        // A material retired between the lookups, or a layout of another material or type (the raster's miss, or its refusal).
        if(layouts[i]->material!=material || layouts[i]->type!=mesh.layout_type){++counts.no_inputs;return "retired material";}
        if(streams && std::none_of(streams->streams.begin(),streams->streams.end(),[&](const NativeModelStream& x){return x.mesh==mesh.patch_index;})){
            ++counts.no_inputs;return "missing stream";}}   // the admission's missing_stream: the model not drawn
    auto slices=std::make_shared<std::vector<RtSkinSlice>>(storage->meshes.size());bool any=false,auxiliary=false,cancel=false,world_rows=false,moved=false;
    uint64_t stream_generation=streams?streams->generation:0;
    for(size_t i=0;i<inputs.meshes.size();++i){const auto& mesh=inputs.meshes[i];if(mesh.ordinal>=slices->size())continue;
        auto& s=(*slices)[mesh.ordinal];s.slice=storage->meshes[mesh.ordinal];
        uint64_t first{},second{};
        if(!(mesh.flags&1) || !(mesh.lod_mask&1) || (Read(mesh.material+8,&first,8) && Read(mesh.material+0x10,&second,8) &&
           !(first&capabilities[0]) && !(second&capabilities[1]))){++counts.excluded_slices;continue;}
        const auto& layout=layouts[i];
        // T5 (rt-view.flag): the first 32 slices left out named once (the model, the mesh, why, the programme family): the i117-t5b
        // runs' patch on the characters' heads (the raster nearer than the rays).
        static const bool name_unsupported=ModuleFlag(L"rt-view.flag");
        const auto unsupported=[&](const char* why){s=RtSkinSlice{};s.slice=storage->meshes[mesh.ordinal];++counts.unsupported_slices;
            if(!name_unsupported)return;static std::mutex unsupported_lock;static std::set<std::pair<uintptr_t,uint32_t>> unsupported_named;bool fresh{};
            {const std::lock_guard hold(unsupported_lock);fresh=unsupported_named.size()<32 && unsupported_named.emplace(c.model,mesh.ordinal).second;}
            if(!fresh)return;std::string name;for(const auto ch:NativeModelDataName(c.model))name.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');
            std::string family;if(const auto& l=layouts[i];l){std::shared_ptr<const NativeModelLayout::Programs> programs;
                {const std::lock_guard hold(l->programs_lock);programs=l->choices[0].programs?l->choices[0].programs:l->choices[1].programs;}if(programs)family=NativeFamilyKey(*programs);}
            // The layout's elements (semantic index slot format byte offset), for the shapes the rays do not read yet.
            std::string elements;if(const auto& l=layouts[i];l && l->vertex_elements_available)for(const auto& e:l->vertex_elements){char cell[64]{};
                sprintf_s(cell,"%s%.12s %u %u %u %u",elements.empty()?"":",",e.semantic.c_str(),e.semantic_index,e.input_slot,static_cast<unsigned>(e.format),e.byte_offset);elements+=cell;}
            char row[1024]{};sprintf_s(row,"{\"event\":\"native_rt_skin_unsupported\",\"model\":\"%.60s\",\"mesh\":%u,\"layout_type\":%u,\"why\":\"%s\",\"family\":\"%.200s\",\"elements\":\"%.600s\"}\n",
                name.c_str(),mesh.ordinal,mesh.layout_type,why,family.c_str(),elements.c_str());Log(row);};
        // The pose policy's words (the raster's pose_policy): unreadable, no palette is bound and the mesh is not drawn.
        BYTE cloth_enabled{};uint32_t flags{};
        if(!Read(mesh.material+0x22,&cloth_enabled,1) || !Read(mesh.material+0x24,&flags,4)){unsupported("pose policy");continue;}
        const auto element=[&](const char* semantic,uint32_t slot=0)->const ds3content::ModelVertexElement*{if(!layout->vertex_elements_available)return nullptr;
            for(const auto& e:layout->vertex_elements)if(!e.semantic_index && e.input_slot==slot && _stricmp(e.semantic.c_str(),semantic)==0)return &e;
            return nullptr;};
        const auto* position=element("POSITION");
        if(!position || position->byte_offset || position->format!=DXGI_FORMAT_R32G32B32_FLOAT){unsupported("position");continue;}
        const NativeModelStream* stream{};
        if(streams){for(const auto& candidate:streams->streams)if(candidate.mesh==mesh.patch_index){stream=&candidate;break;}
            if(!stream || !stream->resource || stream->stride!=s.slice.stride || stream->vertices!=s.slice.vertex_count || stream->position_format!=6 || stream->position_offset){unsupported("stream");continue;}}
        // The programme's shape (T2 final review M3: only the shapes reproduced; L4: only the programme sets the admission built).
        const auto shape=NativeRtShapeOf(*layout);
        if(!shape.known || (mesh.layout_type?!shape.blended:(!shape.rigid || shape.cancel))){unsupported("shape");continue;}
        const auto index_format=[](const ds3content::ModelVertexElement* e){return !e?-1:e->format==DXGI_FORMAT_R8G8B8A8_UINT?0:e->format==DXGI_FORMAT_R16G16B16A16_UINT?1:-1;};
        if(!mesh.layout_type){const auto* normal=element("NORMAL");const int f=index_format(normal);if(f<0){unsupported("rigid index");continue;}
            s.mode=1;s.index_offset=normal->byte_offset;s.index_format=static_cast<uint32_t>(f);}
        // T5: the blend elements in slot 0, or both in slot 1, the mesh's second stream (the face's FG_A models: i117-t5d's
        // native_rt_skin_unsupported rows, BLENDINDICES R8G8B8A8_UINT at 12 and BLENDWEIGHT R8G8B8A8_UNORM at 16 of slot 1).
        else{const auto* indices=element("BLENDINDICES");const auto* weights=element("BLENDWEIGHT");
            if(!indices && !weights && s.slice.stride2){indices=element("BLENDINDICES",1);weights=element("BLENDWEIGHT",1);s.blend_stream=indices && weights;}
            const int f=index_format(indices);
            const int w=!weights?-1:weights->format==DXGI_FORMAT_R8G8B8A8_UNORM?0:weights->format==DXGI_FORMAT_R16G16B16A16_UNORM?1:
                weights->format==DXGI_FORMAT_R16G16B16A16_SNORM?2:weights->format==DXGI_FORMAT_R32G32B32A32_FLOAT?3:-1;
            if(f<0 || w<0){unsupported("blend elements");continue;}
            s.mode=2;s.index_offset=indices->byte_offset;s.index_format=static_cast<uint32_t>(f);s.weight_offset=weights->byte_offset;s.weight_format=static_cast<uint32_t>(w);}
        auto binding=ds3content::SelectOrdinaryModelPoseBinding(*pose,cloth_enabled!=0,(flags&0x4000u)!=0);
        if(binding.kind==ds3content::ModelPoseBindingKind::Unavailable && cloth_enabled)binding=ds3content::SelectOrdinaryModelPoseBinding(*pose,false,(flags&0x4000u)!=0);
        if(binding.kind==ds3content::ModelPoseBindingKind::Unavailable){unsupported("no palette");continue;}   // the raster's palette unavailable: not drawn
        const bool auxiliary_route=binding.kind==ds3content::ModelPoseBindingKind::Auxiliary;
        if(shape.cancel && auxiliary_route && !binding.cancel){unsupported("cancel");continue;}   // the raster refuses it (b12 not bound; final review L2)
        s.palette=auxiliary_route;
        s.cancel=shape.cancel && (binding.cancel || (binding.kind==ds3content::ModelPoseBindingKind::Current && pose->cloth &&
            pose->cancel.Available() && pose->cancel.rows.size()==pose->row_count));
        if(stream){out.sources.push_back(stream->resource);s.source=static_cast<uint32_t>(out.sources.size());s.source_offset=0;++counts.streamed;
            if(out.owners.empty() || out.owners.front()!=streams)out.owners.insert(out.owners.begin(),streams);}
        // T2c (the admission's 1184 rule): a cloth mesh with a published simulated copy, not rewritten by a physics stream, draws that
        // copy as its slot 0; with its material's cloth enabled (+0x22, VC_isEnableCloth) its _cloth programme places it by VC_World
        // (the cloth instance frame, I-0113 tranche 37), else by the palette as any slot 0 (T2 final review M1).
        else if(c.cloth_set){uintptr_t objects{},objects_end{},object{};
            if(Read(c.cloth_set+0x18,&objects,8) && Read(c.cloth_set+0x20,&objects_end,8) && objects && objects_end>=objects &&
               mesh.ordinal<(objects_end-objects)/8 && Read(objects+8ull*mesh.ordinal,&object,8) && object){
                std::shared_ptr<const NativeClothStream> copy;
                {const std::lock_guard hold(cloth_stream_lock);const auto found=cloth_streams.find(object);if(found!=cloth_streams.end())copy=found->second;}
                // I-0113 tranche 37 (gap 25, the veins on a cape's traced shadows): rt-no-cloth.flag leaves a simulated cloth
                // mesh out of the rays (mode 0), the A/B between the traced copy and the denoiser's history; a diagnostic only.
                static const bool no_cloth=ModuleFlag(L"rt-no-cloth.flag");
                if(no_cloth && copy && copy->simulated){s.mode=0;++counts.cloth_left_out;}
                else if(copy && copy->simulated && copy->resource && ((uint64_t(s.slice.vertex_count)*s.slice.stride+3)&~uint64_t(3))==copy->bytes){
                    out.sources.push_back(copy->resource);out.owners.push_back(copy);s.source=static_cast<uint32_t>(out.sources.size());s.source_offset=0;
                    stream_generation=stream_generation*1099511628211ull^copy->generation;++counts.cloth;
                    if(cloth_enabled){s.mode=3;s.palette=0;s.cancel=0;world_rows=true;}}}}
        // T6: the set's wind slices marked whether the wind blows or not (the step review's L2: a slice set that changes with the
        // wind would make a new BLAS at each change); calm, no block (RtSkinner reads no wind).
        if(wind_set && mesh.ordinal<wind_set->slices.size() && wind_set->slices[mesh.ordinal].wind){
            s.wind=wind_set->slices[mesh.ordinal].wind;s.wind_offset=wind_set->slices[mesh.ordinal].wind_offset;moved=true;}
        auxiliary|=s.palette!=0;cancel|=s.cancel!=0;any=true;++counts.skinned_slices;}
    if(!any){++counts.no_slice;return "no slice";}
    if(moved && wind){out.wind=wind;stream_generation=(stream_generation^NativeRtWindHash(*wind))*1099511628211ull;}
    const auto rows=[&](const ds3content::ModelPosePalette& palette){return std::shared_ptr<const RtPoseRows>(pose,&palette.rows);};
    // A mode-3 slice reads the cloth instance frame (`out.world`): its change makes a new generation as a pose's does.
    // I-0113 tranche 37 (gap 25): the frame is the cloth instance's (I+0x50, VC_World of the raster's cloth draws), not the part's
    // placement: placed by the placement, the traced cape lay turned about the body's origin (a shadow on the abdomen after a half
    // turn, i113-t37-cape4-pt against -nocloth and -original). An unreadable or not finite frame leaves the mode-3 slices out of
    // this walk (mode 0: the rest of the character traced; the final review's L1), never placed by the part's placement
    // (`cloth_frame_missing`: unreadable or not finite).
    if(world_rows){std::array<float,12> frame{};bool finite=false;const bool read=NativeClothWorldRows(c.cloth_instance,0x50,frame,&finite);
        if(read && finite){++counts.cloth_framed;out.world=frame;
            for(const float v:out.world){uint32_t bits{};std::memcpy(&bits,&v,4);stream_generation=stream_generation*1099511628211ull^bits;}}
        else{++counts.cloth_frame_missing;for(auto& s:*slices)if(s.mode==3)s.mode=0;}}
    out.source=storage->resource;out.generation={snapshot->generation,stream_generation};
    out.palettes={rows(pose->current),auxiliary?rows(pose->auxiliary):nullptr,cancel?rows(pose->cancel):nullptr};
    // The entry (the lock held for the lookups only: never two threads at once on one entry, T5c's job of its model object and then
    // a failed first's retry on the engine's thread; the close only clears the map).
    NativeRtSkinnedEntry kept;
    {const std::lock_guard hold(NativeRtSkinnedLock());auto& map=NativeRtSkinnedMap();if(const auto found=map.find({c.model,c.pose});found!=map.end())kept=found->second;}   // I-0140 t31: the menu's own when it collects
    if(!kept.blas || !kept.rows || kept.source!=storage->resource.Get() || *kept.slices!=*slices){   // T6 final review L4: a placed wind entry has no rows
        kept.blas=std::make_shared<RtSkinnedBlas>();kept.rows=std::make_shared<const RtSkinRows>(RtSkinRowsOf(storage->bytes,*slices));
        kept.slices=std::move(slices);kept.source=storage->resource.Get();kept.hold_generation=UINT64_MAX;}
    // T2 final review M2: a row not finite (the original copies such rows, the rasterizer drops what they place) would turn
    // triangles inactive, which a refit may not do: the instance keeps its last positions until the rows it reads are finite again.
    if(kept.hold_generation!=snapshot->generation){const auto& used=*kept.rows;
        const auto finite=[&](const RtPoseRows& palette,size_t role){const auto row=[&](size_t r){return std::all_of(palette[r].begin(),palette[r].end(),[](float v){return std::isfinite(v);});};
            if(used.all[role]){for(size_t r=0;r<palette.size();++r)if(!row(r))return false;return true;}
            for(const uint32_t r:used.rows[role])if(r<palette.size() && !row(r))return false;   // a row past the palette reads zeros
            return true;};
        kept.hold=!finite(pose->current.rows,0) || (auxiliary && !finite(pose->auxiliary.rows,1)) || (cancel && !finite(pose->cancel.rows,2));
        kept.hold_generation=snapshot->generation;}
    kept.seen=walk;out.hold=kept.hold;out.blas=kept.blas;out.slices=kept.slices;
    {const std::lock_guard hold(NativeRtSkinnedLock());if(native_rt_skinned_closed)return "closed";NativeRtSkinnedMap()[{c.model,c.pose}]=std::move(kept);}
    return nullptr;
}
// I-0119 (R3a): the programme families met by the rays' material records with rt-view.flag, by name (NativeFamilyKey), each an
// index for the control view's rows (0 the families past kNativeRtFamilies, named "other", as the control view counts them). Any thread of the walk.
constexpr uint32_t kNativeRtFamilies=kRtViewFamilies;
std::mutex native_rt_family_lock;std::unordered_map<std::string,uint32_t> native_rt_family_index;std::vector<std::string> native_rt_family_names{"other"};
uint32_t NativeRtFamilyOf(const std::string& family) {
    const std::lock_guard hold(native_rt_family_lock);const auto found=native_rt_family_index.find(family);if(found!=native_rt_family_index.end())return found->second;
    if(native_rt_family_names.size()>=kNativeRtFamilies)return 0;
    const auto index=static_cast<uint32_t>(native_rt_family_names.size());native_rt_family_names.push_back(family);native_rt_family_index.emplace(family,index);return index;
}
std::string NativeRtFamilyName(uint32_t index) {const std::lock_guard hold(native_rt_family_lock);return index<native_rt_family_names.size()?native_rt_family_names[index]:std::string{};}
// What a material entry tallies at each walk that meets it (NativeRtWalkCounts' material_*).
// I-0117 T6: wind, its slices moved by the wind; wind_unsupported, those whose wind the rays do not read (their rest pose).
// I-0125 R6d (its final review): forward, the forward slices met; forward_made, those with a record (their parts, textures and blend read).
struct NativeRtMaterialTally {uint64_t slices{},pending{},no_gbuf{},no_uv{},no_frame{},blends{},no_weights{},wind{},wind_unsupported{},forward{},forward_made{},emissive{};std::array<uint64_t,4> named{},resolved{};};
// I-0117 T6: whether the slice's G-buffer vertex programme moves it by the wind as the Grass programmes do (GXFlver_Grass*_GBuf and
// _GBuf_Skin, Eclipse_grass_Gbuf: one body, rt_skin.hlsl's Wind; the census of 2026-11-03: every G-buffer vertex programme reading
// VC_ForceParam is one of them): cbvModelParam's VC_ForceParam (RDEF used) read; with VC_ModelSizeParam and TEXCOORD1's xyz read and
// that element in slot 0 as float3, half4 or 8-bit UNORM (RtWind*), its format and byte in `out`; otherwise counted unsupported (its
// rest pose).
void NativeRtSliceWind(const NativeModelLayout::Programs& programs,const NativeModelLayout& layout,RtMaterialSlice& out,NativeRtMaterialTally& tally) {
    const ds3content::ShaderInterface* vertex{};
    for(uint32_t pixel=0;pixel<3 && !vertex;++pixel){const auto r=NativeRecipe(6,pixel);
        if(programs.recipes[r].status==ds3content::ModelProgramStatus::Resolved && programs.interfaces[r][0])vertex=programs.interfaces[r][0].get();}
    if(!vertex)return;
    const auto used=[&](const char* name){for(const auto& block:vertex->constants)if(block.name=="cbvModelParam")
        for(const auto& v:block.variables)if(v.name==name)return (v.flags&2u)!=0;return false;};
    if(!used("VC_ForceParam"))return;
    const bool reads=std::any_of(vertex->inputs.begin(),vertex->inputs.end(),[](const auto& input){
        return input.semantic_index==1 && _stricmp(input.semantic.c_str(),"TEXCOORD")==0 && (input.usage_mask&7)==7;});
    const ds3content::ModelVertexElement* e{};
    if(layout.vertex_elements_available)for(const auto& x:layout.vertex_elements)
        if(x.semantic_index==1 && x.input_slot==0 && _stricmp(x.semantic.c_str(),"TEXCOORD")==0){e=&x;break;}
    const uint32_t format=!e?RtWindNone:(e->format==DXGI_FORMAT_R32G32B32_FLOAT || e->format==DXGI_FORMAT_R32G32B32A32_FLOAT)?RtWindFloat3:
        e->format==DXGI_FORMAT_R16G16B16A16_FLOAT?RtWindHalf4:e->format==DXGI_FORMAT_R8G8B8A8_UNORM?RtWindUnorm8:
        e->format==DXGI_FORMAT_B8G8R8A8_UNORM?RtWindBgra8:RtWindNone;
    if(!used("VC_ModelSizeParam") || !reads || format==RtWindNone){++tally.wind_unsupported;
        // The first 8 named with rt-view.flag (the i117-t6a runs: every grass slice unsupported, its UV1 a Byte4C the first list did
        // not take), with the programme family.
        static const bool name_unsupported=ModuleFlag(L"rt-view.flag");
        static std::atomic<int> named{};if(name_unsupported && named.fetch_add(1,std::memory_order_relaxed)<8){char row[512]{};
            sprintf_s(row,"{\"event\":\"native_rt_wind_unsupported\",\"family\":\"%.200s\",\"size\":%s,\"reads\":%s,\"element\":%s,\"format\":%d,\"slot\":%d,\"offset\":%d}\n",
                NativeFamilyKey(programs).c_str(),used("VC_ModelSizeParam")?"true":"false",reads?"true":"false",e?"true":"false",e?static_cast<int>(e->format):-1,
                e?static_cast<int>(e->input_slot):-1,e?static_cast<int>(e->byte_offset):-1);Log(row);}
        return;}
    out.wind=format;out.wind_offset=e->byte_offset;++tally.wind;
}
// I-0119 (R3a, R3-PLAN §1): one LOD-0 slice's material as its G-buffer programme writes it: the first resolved programme of the
// Gbuf recipes (0 to 2, the pixel variants), its textures by name (g_DiffuseTexture, g_SpecularTexture, g_BumpmapTexture,
// g_EmissiveTexture) at their registers, each resolved through its holders as the raster resolves them (a texture not resident,
// or its "_l" stand-in: `pending`, the set made again later) with its FC_TextureTilingScale row (register 1's rows, without the UV
// animation offsets); the MTD's g_DiffuseMapColor (FC_DifMapMulCol) and g_SpecularPower (the gloss scale (p / 256)², FC_SpcParam.x);
// UV0 (UV16, float2 or half2), NORMAL (R8G8B8A8_UINT or R16G16B16A16_FLOAT) and TANGENT (R8G8B8A8_UINT) from the layout's elements of slot 0 or 1. The triangle's data
// (indices, positions) are written for every slice in the storage's bounds, a material or not.
// The texture the raster binds for a material binding none of whose holders holds a resident texture (NativeFallbackTexture,
// 08_model_programmes.inc): the system texture of the path's stem (SYSTEX_*), else SYSTEX_WHITE for a texture bound with no data;
// null otherwise (the texture is waited for). `waits`: the white one, whose real texture may still arrive (the raster rebinds it as
// soon as a holder's inner changes; the step review's M1): the caller keeps its set pending, tried again every 30 walks.
std::shared_ptr<const bootstrap::NativeTextureRecord> NativeRtFallbackTexture(const std::wstring& folded,bool& waits) {
    waits=false;const auto common=AcquireNativeCommonImages();if(!common)return {};
    const auto usable=[](const auto& t){return t && t->resident && t->resource && t->view;};
    for(const auto& [name,texture]:common->system)if(usable(texture) && _wcsicmp(name.c_str(),folded.c_str())==0)return texture;
    if(NativeDefaultBoundTextureFolded(folded))for(const auto& [name,texture]:common->system)if(usable(texture) && _wcsicmp(name.c_str(),L"SYSTEX_WHITE")==0){waits=true;return texture;}
    return {};
}
// I-0125 R6d: a forward slice's FC_DynamicDiffuseColor as the admission makes it (NativeDynamicDiffuseColour on the engine material's
// dynamic object; its block unwritten, zero), a colour not finite zero (the admission's block writes nothing then: the step review's M2).
std::array<float,4> NativeRtForwardColour(uintptr_t material) {
    std::shared_ptr<const NativeModelMaterial> owned;
    {const std::shared_lock hold(model_material_lock);const auto found=model_materials.find(material);if(found!=model_materials.end())owned=found->second;}
    std::array<BYTE,0x28> flags{};std::array<float,4> colour{};
    if(material && Read(material+0xD0,flags.data(),flags.size()))(void)NativeDynamicDiffuseColour(material+0xD0,flags,owned.get(),colour);
    if(!std::all_of(colour.begin(),colour.end(),[](float v){return std::isfinite(v);}))colour={};
    return colour;
}
// I-0125 R6d (R6-PLAN §7): the forward pixel programme of an unlit GXFlver_Col slice (the sky's dome and clouds: GXFlver_Col_Fwd, read
// 2026-11-06): the group's body GXFlver_Col (without regard to case, as the raster finds its groups) and a resolved Fwd recipe (pixel
// variants 0 to 2) whose programme samples g_DiffuseTexture, uses cbMatDynParam's FC_DynamicDiffuseColor and binds no cbLight; null
// otherwise (the ColSh, ColZFeather, Emi and NoLight bodies are other programmes, not read). R6d step 2: also the scrolling zenith's
// MultiBlend2_et1_nolight_MuitiBlendMask_Fwd (read 2026-11-06; `kind` RtForwardBlendMask): its four textures snp_0_Texture2D_0..3 at
// t4..t1, cbPerFrame's FC_GlobalTime and cbMatDynParam's generated words used, no cbLight.
const ds3content::ShaderInterface* NativeRtForwardInterface(const NativeModelLayout::Programs& programs,const std::string& body,uint32_t& kind) {
    kind=RtForwardCol;
    const bool col=_stricmp(body.c_str(),"GXFlver_Col")==0,mask=_stricmp(body.c_str(),"MultiBlend2_et1_nolight_MuitiBlendMask")==0,
        opacity=_stricmp(body.c_str(),"Fujir_Nolight_OpacityBlend")==0;
    if(!col && !mask && !opacity)return nullptr;
    for(uint32_t pixel=0;pixel<3;++pixel){const auto r=NativeRecipe(0,pixel);
        if(programs.recipes[r].status!=ds3content::ModelProgramStatus::Resolved || !programs.interfaces[r][1])continue;
        const auto& f=*programs.interfaces[r][1];
        const auto uses=[&](const char* block,const char* name){const auto c=std::find_if(f.constants.begin(),f.constants.end(),[&](const auto& x){return x.name==block;});
            return c!=f.constants.end() && std::any_of(c->variables.begin(),c->variables.end(),[&](const auto& v){return (v.flags&2) && v.name==name;});};
        const bool lit=std::any_of(f.constants.begin(),f.constants.end(),[](const auto& c){return c.name=="cbLight";});
        if(lit)continue;
        if(col){const bool diffuse=std::any_of(f.resources.begin(),f.resources.end(),[](const auto& x){return x.input_type==2 && x.name=="g_DiffuseTexture";});
            if(diffuse && uses("cbMatDynParam","FC_DynamicDiffuseColor"))return &f;continue;}
        // The cloud sheets' Fujir_Nolight_OpacityBlend_Fwd (read 2026-11-06): snp_0_Texture2D_1, _4 and _5 at t3, t2 and t1.
        if(opacity){uint32_t found{};static constexpr std::pair<uint32_t,const char*> sheets[3]{{3,"Texture2D_1"},{2,"Texture2D_4"},{1,"Texture2D_5"}};
            for(const auto& x:f.resources)for(uint32_t k=0;k<3;++k)if(x.input_type==2 && x.shader_register==sheets[k].first && x.name.size()>11 &&
                x.name.compare(x.name.size()-11,11,sheets[k].second)==0)found|=1u<<k;
            if(found==7 && uses("cbPerFrame","FC_GlobalTime")){kind=RtForwardOpacity;return &f;}continue;}
        uint32_t found{};
        for(const auto& x:f.resources)for(uint32_t k=0;k<4;++k)if(x.input_type==2 && x.shader_register==4-k && x.name.size()>11 &&
            x.name.compare(x.name.size()-11,11,"Texture2D_"+std::to_string(k))==0)found|=1u<<k;
        if(found==15 && uses("cbPerFrame","FC_GlobalTime") && uses("cbMtdParam","FC_DifMapMulCol")){kind=RtForwardBlendMask;return &f;}}
    return nullptr;
}
void NativeRtSliceMaterial(const NativeModelLayout::Programs& programs,const NativeModelLayout& layout,const NativeModelStorage& storage,uint32_t ordinal,
    uintptr_t material,uintptr_t named_registry,uintptr_t face_owner,RtMaterialSlice& out,NativeRtMaterialTally& tally,bool& pending,std::vector<std::shared_ptr<const bootstrap::NativeTextureRecord>>& kept) {
    const auto& slice=storage.meshes[ordinal];
    if(uint64_t(slice.index_offset)+uint64_t(slice.index_count)*slice.index_width>storage.bytes.size() ||
       uint64_t(slice.vertex_offset)+uint64_t(slice.vertex_count?slice.vertex_count-1:0)*slice.stride+12>storage.bytes.size()){++tally.no_gbuf;return;}
    out.index_offset=slice.index_offset;out.index_width=slice.index_width;out.position_offset=slice.vertex_offset;out.position_stride=slice.stride;
    const ds3content::ShaderInterface* gbuf{};
    for(uint32_t pixel=0;pixel<3 && !gbuf;++pixel){const auto r=NativeRecipe(6,pixel);
        if(programs.recipes[r].status==ds3content::ModelProgramStatus::Resolved && programs.interfaces[r][1])gbuf=programs.interfaces[r][1].get();}
    // The programme's group (the file name of the MTD's shader path, without its extension): the bodies decoded by name below.
    std::string body;for(const auto ch:programs.program_path){if(ch==L'\\' || ch==L'/')body.clear();else if(ch==L'.')break;else body.push_back(ch>=32 && ch<127?static_cast<char>(ch):'?');}
    // I-0125 R6d: a slice the raster draws forward only (its class has no G-buffer bit: 142130's 0x80 forward, 2 transparent) is drawn
    // by its Fwd programme, whatever G-buffer programme its group also resolves (the m40 dome has one: run i125-r6d1, whose witness
    // showed it taken for a G-buffer slice): an unlit GXFlver_Col slice's record is a forward one (below).
    const bool forward_class=programs.policy && !(ds3content::ClassifyModelMaterial(*programs.policy,false)&kRtGbufferClasses);
    uint32_t forward_kind{};const ds3content::ShaderInterface* forward=forward_class?NativeRtForwardInterface(programs,body,forward_kind):nullptr;
    if(forward)gbuf=forward;
    if(!gbuf || !programs.texture_bindings_available){++(forward?tally.forward:tally.no_gbuf);return;}   // R6d: a forward slice met, no record
    // An element of the vertex layout (slot 0, or slot 1 the mesh's second stream): its first byte and stride, within the storage.
    const auto element=[&](const char* semantic,uint32_t index)->const ds3content::ModelVertexElement*{if(!layout.vertex_elements_available)return nullptr;
        for(const auto& e:layout.vertex_elements)if(e.semantic_index==index && e.input_slot<2 && _stricmp(e.semantic.c_str(),semantic)==0)return &e;return nullptr;};
    const auto place=[&](const ds3content::ModelVertexElement& e,uint32_t bytes,uint32_t& first,uint32_t& stride){
        const uint64_t base=e.input_slot?slice.vertex_offset2:slice.vertex_offset;stride=e.input_slot?slice.stride2:slice.stride;
        const uint64_t at=base+e.byte_offset,last=at+uint64_t(slice.vertex_count?slice.vertex_count-1:0)*stride+bytes;
        first=static_cast<uint32_t>(at);const bool fits=stride && slice.vertex_count && at%4==0 && stride%4==0 && last<=storage.bytes.size();if(!fits)stride=0;return fits;};
    if(const auto* uv=element("TEXCOORD",0)){
        out.uv_format=(uv->format==DXGI_FORMAT_R16G16_SINT || uv->format==DXGI_FORMAT_R16G16B16A16_SINT)?RtUvSint16:
            (uv->format==DXGI_FORMAT_R32G32_FLOAT || uv->format==DXGI_FORMAT_R32G32B32A32_FLOAT)?RtUvFloat2:
            (uv->format==DXGI_FORMAT_R16G16_FLOAT || uv->format==DXGI_FORMAT_R16G16B16A16_FLOAT)?RtUvHalf2:UINT32_MAX;
        if(out.uv_format==UINT32_MAX || !place(*uv,out.uv_format==RtUvFloat2?8:4,out.uv_offset,out.uv_stride)){out.uv_format=0;out.uv_stride=0;}}
    if(!out.uv_stride)++tally.no_uv;
    // NORMAL in R8G8B8A8_UINT ((v - 127) / 127, the reference's vertex programmes) or R16G16B16A16_FLOAT (its xyz as is: the map
    // blends' vertex programmes, the final review's M1); TANGENT in R8G8B8A8_UINT only (every vertex programme of the census decodes
    // it so, .w its sign; the fixes' step review M1), and only beside a NORMAL read (L1: no bump over a faceted triangle normal).
    const auto frame=[&](const char* semantic,uint32_t& first,uint32_t& stride,bool& half,bool half_allowed){const auto* e=element(semantic,0);half=false;
        if(e && e->format==DXGI_FORMAT_R8G8B8A8_UINT && place(*e,4,first,stride))return true;
        if(half_allowed && e && e->format==DXGI_FORMAT_R16G16B16A16_FLOAT && place(*e,8,first,stride)){half=true;return true;}
        first=stride=0;return false;};
    const bool normal=frame("NORMAL",out.normal_offset,out.normal_stride,out.normal_half,true);
    const bool tangent=normal && frame("TANGENT",out.tangent_offset,out.tangent_stride,out.tangent_half,false);
    if(!tangent){out.tangent_offset=out.tangent_stride=0;out.tangent_half=false;}
    if(!normal || !tangent)++tally.no_frame;
    // The MTD's values (the material's runtime numeric writes not read: R3-PLAN §4's risk, the control view shows a family off).
    if(programs.authored)for(const auto& p:programs.authored->parameters){
        if(p.name=="g_DiffuseMapColor")for(size_t k=0;k<3 && k<p.values.size();++k)out.albedo[k]=static_cast<float>(p.values[k]);
        else if(p.name=="g_SpecularPower" && !p.values.empty()){const float power=static_cast<float>(p.values[0])/256.0f;out.albedo[3]=power*power;}}
    std::array<float,4*25> rows{};std::array<bool,25> rows_written{};   // FC_TextureTilingScale's 25 rows
    {ds3content::ModelTextureTiling tiling;
        if(ds3content::ProduceModelTextureTiling(programs.texture_bindings,ds3content::ModelTextureTilingInputs{1,false,false,{}},tiling))
            for(size_t row=0;row<25 && 16*row+16<=tiling.values.bytes.size();++row)
                if(std::all_of(tiling.values.written.begin()+4*row,tiling.values.written.begin()+4*row+4,[](uint8_t w){return w!=0;})){
                    std::memcpy(&rows[4*row],tiling.values.bytes.data()+16*row,16);rows_written[row]=true;}}
    // The texture bound at register `reg` resolved as the raster resolves it into `role`, with its tiling row (the binding's remap,
    // else its register: ProduceModelTextureTiling's destination). Step review M2: false for no bound path there (none, an empty
    // path, a FaceGen binding without a face owner), left out for good; one bound but not resident (or its stand-in) makes the set again later.
    // I-0120 S6: what this slice adds to the set's pending state and kept textures, taken back below when it ends with no material
    // and its albedo was not the one waiting (a set is not made again for textures it never reads).
    // R6d's final review L7: `slice_waits`, a texture of this slice not arrived yet (whatever the set's earlier slices left pending).
    // I-0120 S5: no named texture registry readable at this walk: no role could be resolved, so the set waits (made again later, as
    // the alpha path's), instead of being kept for good with every role left out.
    if(!named_registry)pending=true;
    const bool pending_before=pending;const size_t kept_before=kept.size();bool albedo_pending{},slice_waits{};
    const auto resolve=[&](uint32_t reg,RtMaterialSlice& into,uint32_t role){
        for(size_t index=0;index<programs.texture_bindings.size() && named_registry;++index){const auto& input=programs.texture_bindings[index];
            // R3c: a FaceGen binding (FG_face_gen) the candidate's composed face image, as the raster binds it (NativeComposedFace);
            // not composed yet: pending, the set tried again every 30 walks (the final review's M1); a face composed again later
            // withdraws the record the entry holds, which makes the set again (L1: no retry once composed).
            const bool facegen=programs.texture_lookups[index].facegen;
            if(input.declaration.slot!=reg || input.path.empty() || (facegen && !face_owner))continue;
            const int32_t row=input.declaration.remap>=0?input.declaration.remap:static_cast<int32_t>(reg);
            if(row<25 && rows_written[row])std::memcpy(into.tiling[role].data(),&rows[4*row],16);
            size_t chosen{};std::array<uintptr_t,4> inners{};
            auto texture=facegen?NativeComposedFace(face_owner):ResolveNativeTextureHolders(NativeTextureHolders(programs,index,layout.resolver,named_registry),inners,&chosen);
            // The raster's fallback when no holder holds a resident texture (NativeFallbackTexture): a path whose stem is a system
            // texture never arrives otherwise (the High Wall MulMaskDOL piece's mask and layer-0 roles waited for good, run
            // i119-r3c-roles-m30-on): that texture, not pending (the white one of a texture bound with no data: drawn, as the raster draws it, and pending, its texture tried again).
            if(!facegen && (!texture || !texture->view)){bool waits{};if(auto fallback=NativeRtFallbackTexture(programs.texture_lookups[index].leaf_folded,waits)){texture=std::move(fallback);chosen=waits?1:0;}}
            if(!texture || !texture->view || chosen){pending=true;slice_waits=true;albedo_pending|=&into==&out && role==RtTextureDiffuse;}
            if(!texture || !texture->view){++tally.pending;return true;}
            if(texture->description.cube || texture->description.slices!=1)return true;   // the shader reads a Texture2D
            into.views[role]=texture->view.handle;into.textures[role]=texture;kept.push_back(texture);return true;}
        return false;};
    // I-0125 R6d (R6-PLAN §7): a forward GXFlver_Col slice: g_DiffuseTexture (its register, its tiling row), COLOR1 (the pixel
    // programme's alpha factor), FC_DynamicDiffuseColor as the admission makes it (NativeDynamicDiffuseColour: its block unwritten,
    // zero, as the raster's); no material, no ambient (it is unlit). Its blend is set by the caller (the draw group's row). Its
    // diffuse waiting: the set pending, as a G-buffer slice's.
    // R6d step 2 (MultiBlend2_et1_nolight_MuitiBlendMask_Fwd.ppo and .vpo, read 2026-11-06): the scrolling zenith. Its textures
    // snp_0_Texture2D_k at t(4 - k) in the roles k (each tiled by FC_TextureTilingScale[k], the row the programme reads), UV0 =
    // TEXCOORD0.xy (VC_TexScroll_0) and UV1 = TEXCOORD0.zw (VC_TexScroll_1) in R16G16B16A16_SINT, the weight TEXCOORD1.x, COLOR0.w;
    // the generated words snp_0_color_0 (rgb), float_0, float_1 and the four scrolls float2_1..4 (cbMatDynParam from 0x270, as R3c's
    // blends read them; their animation not applied) with FC_DifMapMulCol (the MTD's g_DiffuseMapColor): rgb in `albedo` (colour_0 ×
    // FC_DifMapMulCol), float_0 in its w, float_1 in `emissive`.x; its layer record (`layers`) UV1 in its UV element, TEXCOORD1 in its
    // lightmap element (RtWeight*), the scrolls float2_1, float2_2 in its albedo and float2_3, float2_4 in its emissive. Every part
    // read, else no record (counted in `forward` without one).
    // R6d step 2 (Fujir_Nolight_OpacityBlend_Fwd.ppo and .vpo, read 2026-11-06): the cloud sheets. rgb = t3(uv0 + T f2_2 tiled by row
    // 1).rgb × colour_0, a = t1(uv2 + T f2_0, row 5).x × t2(uv1 + T f2_1, row 4).x × TEXCOORD3.x, the whole × mulCol; uv0, uv1, uv2 =
    // TEXCOORD0, 1, 2 / 2048 (no VC_TexScroll). t3 in the diffuse role (row 1), t2 in role 1 (row 4), t1 in role 2 (row 5); UV0 in its
    // UV element (the scroll not applied), TEXCOORD3 in its lightmap element (RtWeight*), colour_0 in `albedo`; its layer record UV1 in
    // its UV element, UV2 in its lightmap element (RtUv*), f2_0, f2_1 in its albedo and f2_2 in its emissive.xy.
    // Both: the generated words by their names' suffixes in the programme's cbMatDynParam (the words from 0x270, R3c's route), false
    // when one is missing; a vertex element of four values as a weight (RtWeight*: its first value read), false otherwise (the step
    // review's L4: one lookup each for the two records).
    // R6d's final review: a forward slice refused for a part it cannot read (not for a texture still loading) keeps none of its
    // textures and leaves the set's pending state as it was (no retry for it), as S6 does for the G-buffer slices below; every forward
    // slice met past the storage check counted in `forward` (its record counted by the caller, after its blend: NativeRtMaterialsOf), not
    // in R3's roles.
    const auto refuse=[&]{kept.resize(kept_before);pending=pending_before;out.textures={};out.views={};out.layers.clear();};
    const auto generated_words=[&](std::span<const char* const> suffixes,float* words){
        const auto dynamic=std::find_if(gbuf->constants.begin(),gbuf->constants.end(),[](const auto& c){return c.name=="cbMatDynParam";});
        const auto& generated=programs.generated_parameters;const uint32_t from=generated?generated->byte_offset:0;
        if(dynamic==gbuf->constants.end() || !generated)return false;
        for(size_t k=0;k<suffixes.size();++k){int32_t at=-1;const size_t n=std::strlen(suffixes[k]);
            for(const auto& v:dynamic->variables)if(v.name.size()>n && v.name.compare(v.name.size()-n,n,suffixes[k])==0){at=static_cast<int32_t>(v.byte_offset);break;}
            if(at<0 || uint32_t(at)<from || (uint32_t(at)-from)%4 || (uint32_t(at)-from)/4>=generated->words.size())return false;
            std::memcpy(&words[k],&generated->words[(uint32_t(at)-from)/4],4);}
        return true;};
    const auto weight_of=[&](uint32_t index,uint32_t& first,uint32_t& stride,uint32_t& format){const auto* e=element("TEXCOORD",index);if(!e)return false;
        format=e->format==DXGI_FORMAT_R32G32B32A32_FLOAT?RtWeightFloat4:e->format==DXGI_FORMAT_R16G16B16A16_FLOAT?RtWeightHalf4:
            e->format==DXGI_FORMAT_R8G8B8A8_UNORM?RtWeightUnorm8:e->format==DXGI_FORMAT_R16G16B16A16_UNORM?RtWeightUnorm16:0u;
        return format && place(*e,format==RtWeightFloat4?16:format==RtWeightUnorm8?4:8,first,stride);};
    if(forward && forward_kind==RtForwardOpacity){out.ambient=RtAmbientNone;out.forward_kind=RtForwardOpacity;out.albedo={0,0,0,0};out.unscrolled=true;
        static constexpr std::array<std::pair<uint32_t,uint32_t>,3> roles{{{3,1},{2,4},{1,5}}};   // (register, tiling row) by role
        for(uint32_t k=0;k<3;++k){(void)resolve(roles[k].first,out,k);const uint32_t r=roles[k].second;
            if(rows_written[r])out.tiling[k]={rows[4*r],rows[4*r+1],rows[4*r+2],rows[4*r+3]};}
        const auto uv_of=[&](uint32_t index,uint32_t& first,uint32_t& stride,uint32_t& format){const auto* e=element("TEXCOORD",index);if(!e)return false;
            format=e->format==DXGI_FORMAT_R32G32_FLOAT || e->format==DXGI_FORMAT_R32G32B32A32_FLOAT?RtUvFloat2Scaled:
                e->format==DXGI_FORMAT_R16G16_FLOAT || e->format==DXGI_FORMAT_R16G16B16A16_FLOAT?RtUvHalf2Scaled:
                e->format==DXGI_FORMAT_R16G16_SINT || e->format==DXGI_FORMAT_R16G16B16A16_SINT?RtUvSint16:UINT32_MAX;
            return format!=UINT32_MAX && place(*e,format==RtUvFloat2Scaled?8:4,first,stride);};
        RtMaterialSlice layer;
        bool parts=uv_of(0,out.uv_offset,out.uv_stride,out.uv_format) && uv_of(1,layer.uv_offset,layer.uv_stride,layer.uv_format) &&
            uv_of(2,layer.lightmap_offset,layer.lightmap_stride,layer.lightmap_format);
        for(const auto& role:roles)parts=parts && rows_written[role.second];   // the fixes' step review L2: every row the programme reads written
        parts=parts && weight_of(3,out.lightmap_offset,out.lightmap_stride,out.lightmap_format);
        std::array<float,9> words{};
        static constexpr const char* suffixes[9]{"_snp_0_color_0_0","_snp_0_color_0_1","_snp_0_color_0_2","_snp_0_float2_0_0","_snp_0_float2_0_1",
            "_snp_0_float2_1_0","_snp_0_float2_1_1","_snp_0_float2_2_0","_snp_0_float2_2_1"};
        parts=parts && generated_words(suffixes,words.data());
        out.albedo={words[0],words[1],words[2],0};layer.albedo={words[3],words[4],words[5],words[6]};layer.emissive={words[7],words[8],0,0};
        out.layers.clear();out.layers.push_back(std::move(layer));
        const bool read=parts && std::all_of(words.begin(),words.end(),[](float v){return std::isfinite(v);});
        out.forward=read && out.textures[0] && out.textures[1] && out.textures[2];
        if(!out.forward)out.layers.clear();
        if(!read || (!out.forward && !slice_waits))refuse();   // the step review's L-1: a texture that cannot arrive too
        ++tally.forward;return;}
    if(forward && forward_kind==RtForwardBlendMask){out.ambient=RtAmbientNone;out.forward_kind=RtForwardBlendMask;
        const std::array<float,4> mtd_colour=out.albedo;out.albedo={0,0,0,0};
        for(uint32_t k=0;k<4;++k){const auto reg=4-k;(void)resolve(reg,out,k);if(rows_written[k])out.tiling[k]={rows[4*k],rows[4*k+1],rows[4*k+2],rows[4*k+3]};}
        RtMaterialSlice layer;bool parts=out.uv_format==RtUvSint16 && out.uv_stride;
        for(uint32_t k=0;k<4;++k)parts=parts && rows_written[k];   // the fixes' step review L2: every row the programme reads written
        if(const auto* e=element("TEXCOORD",0);parts && e && e->format==DXGI_FORMAT_R16G16B16A16_SINT && place(*e,8,layer.uv_offset,layer.uv_stride)){
            layer.uv_offset+=4;layer.uv_format=RtUvSint16;}else parts=false;
        parts=parts && weight_of(1,layer.lightmap_offset,layer.lightmap_stride,layer.lightmap_format);
        if(const auto* e=element("COLOR",0)){
            const uint32_t format=(e->format==DXGI_FORMAT_R8G8B8A8_UNORM || e->format==DXGI_FORMAT_B8G8R8A8_UNORM)?RtAlphaColourUnorm8:e->format==DXGI_FORMAT_R32G32B32A32_FLOAT?RtAlphaColourFloat4:0u;
            if(format && place(*e,format==RtAlphaColourFloat4?16:4,out.forward_colour_offset,out.forward_colour_stride))out.forward_colour_format=format;}
        if(!out.forward_colour_format)parts=false;
        std::array<float,13> words{};
        static constexpr const char* suffixes[13]{"_snp_0_color_0_0","_snp_0_color_0_1","_snp_0_color_0_2","_snp_0_float_0_0","_snp_0_float_1_0",
            "_snp_0_float2_1_0","_snp_0_float2_1_1","_snp_0_float2_2_0","_snp_0_float2_2_1","_snp_0_float2_3_0","_snp_0_float2_3_1","_snp_0_float2_4_0","_snp_0_float2_4_1"};
        parts=parts && generated_words(suffixes,words.data());
        out.albedo={words[0]*mtd_colour[0],words[1]*mtd_colour[1],words[2]*mtd_colour[2],words[3]};out.emissive={words[4],0,0,0};
        layer.albedo={words[5],words[6],words[7],words[8]};layer.emissive={words[9],words[10],words[11],words[12]};
        out.layers.clear();out.layers.push_back(std::move(layer));
        const bool read=parts && std::all_of(words.begin(),words.end(),[](float v){return std::isfinite(v);});
        out.forward=read && std::all_of(out.textures.begin(),out.textures.end(),[](const auto& t){return t!=nullptr;});
        if(!out.forward)out.layers.clear();
        if(!read || (!out.forward && !slice_waits))refuse();   // the step review's L-1: a texture that cannot arrive too
        ++tally.forward;return;}
    if(forward){out.ambient=RtAmbientNone;out.albedo={0,0,0,0};
        const auto declared=std::find_if(gbuf->resources.begin(),gbuf->resources.end(),[](const auto& r){return r.input_type==2 && r.name=="g_DiffuseTexture";});
        if(declared!=gbuf->resources.end())(void)resolve(declared->shader_register,out,RtTextureDiffuse);
        if(const auto* e=element("COLOR",1)){
            const uint32_t format=(e->format==DXGI_FORMAT_R8G8B8A8_UNORM || e->format==DXGI_FORMAT_B8G8R8A8_UNORM)?RtAlphaColourUnorm8:   // .w the same byte
                e->format==DXGI_FORMAT_R32G32B32A32_FLOAT?RtAlphaColourFloat4:0u;
            if(format && place(*e,format==RtAlphaColourFloat4?16:4,out.forward_colour_offset,out.forward_colour_stride))out.forward_colour_format=format;}
        // The step review's L4: a record only with its UV0 and COLOR1 read (the vertex programme reads both; another format: out of the
        // rays, counted in `forward` without a record).
        const bool read=out.uv_stride && out.forward_colour_format;
        out.albedo=read?NativeRtForwardColour(material):std::array<float,4>{};out.forward=read && out.textures[RtTextureDiffuse]!=nullptr;   // L-2: no engine call for a refused slice
        if(!read || (!out.forward && !slice_waits))refuse();   // the step review's L-1: a texture that cannot arrive too
        ++tally.forward;return;}
    // I-0125 R6a (R6-PLAN §1): the ambient its programme writes into GB4: a lightmapped body's (it samples t26 g_DOLTexture and t27
    // g_DOLTexture2), the others' the instance's SH rows (the census's Ibl bodies and the map blends without a lightmap; the control
    // view's rows check every family against GB4). The lightmap UV's element by the body (the step review's H2, from the census: the
    // vertex programmes make it from TEXCOORD0.zw, the two-layer Mul, MulMask and EmiMul bodies from TEXCOORD1.xy, in shorts; the
    // lightmapped map blends (MultiBlend*_lightmap, FourBlend*_lightmap) from TEXCOORD1.xy in floats, both / 2048: the final review's H1);
    // the Burn, DirtMask and SandBlend bodies sample other interpolants, and the other blends' vertex programmes (BloodBlend, RockBlend)
    // are not read: their ambient counted missing (a form, no element; its step review's M-1).
    {const auto declares=[&](const char* name,uint32_t reg){return std::any_of(gbuf->resources.begin(),gbuf->resources.end(),[&](const auto& r){
            return r.input_type==2 && r.shader_register==reg && r.name==name;});};
        std::string folded=body;for(auto& ch:folded)ch=static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        const auto has=[&](const char* part){return folded.find(part)!=std::string::npos;};
        if(!(declares("g_DOLTexture",26) && declares("g_DOLTexture2",27)))out.ambient=RtAmbientSh;
        else{out.ambient=RtAmbientDol;
            const bool blend=has("multiblend") || has("fourblend");   // the census's lightmapped blends' .vpo (MultiBlend2, MultiBlend4, FourBlend)
            if(!has("burn") && !has("dirtmask") && !has("sand") && (blend || !has("blend"))){const bool second=has("mul") || blend;const auto* e=element("TEXCOORD",second?1:0);
                const auto f=e?e->format:DXGI_FORMAT_UNKNOWN;
                // The blends' TEXCOORD1 two floats only (the step review's L-3: four are the unlit blends' weights).
                const uint32_t format=!blend && (f==DXGI_FORMAT_R16G16B16A16_SINT || (second && f==DXGI_FORMAT_R16G16_SINT))?RtUvSint16:
                    blend && f==DXGI_FORMAT_R32G32_FLOAT?RtUvFloat2Scaled:blend && f==DXGI_FORMAT_R16G16_FLOAT?RtUvHalf2Scaled:UINT32_MAX;
                if(format!=UINT32_MAX && place(*e,second?(format==RtUvFloat2Scaled?8:4):8,out.lightmap_offset,out.lightmap_stride)){
                    if(!second)out.lightmap_offset+=4;out.lightmap_format=format;}
                else out.lightmap_offset=out.lightmap_stride=0;}}}
    static constexpr const char* skins[]{"GXFlver_SkinIbl","GXFlver_SkinIbl_Bloody","GXFlver_SkinSSSIbl_Bloody"};
    static constexpr const char* muls[]{"GXFlver_ColDifSpcBumpMulDOL","GXFlver_ColDifSpcBumpMulEnvDOL","GXFlver_ColDifSpcBumpMulIbl",
        "GXFlver_ColDifSpcBumpMulMaskDOL","GXFlver_ColDifSpcBumpMulMaskEnvDOL","GXFlver_ColDifSpcBumpMulMaskIbl"};
    const auto among=[&](const auto& list){return std::any_of(std::begin(list),std::end(list),[&](const char* name){return _stricmp(body.c_str(),name)==0;});};
    // R3c: the hair (GBUF-FAMILIES §17; the pixel programmes read 2026-11-03): the reference shape with saturate(sqrt(t0) ×
    // FC_SkinMatchColor)² as the albedo before FC_DifMapMulCol (the _Bloody and HairIbl2 bodies' wet and blood blocks not applied, as
    // the skin's); SAT_HairIbl2 by its rows (below).
    static constexpr const char* hairs[]{"GXFlver_HairIbl","GXFlver_HairIbl2","GXFlver_HairIbl_Bloody","GXFlver_HairIbl_Bloody_Cloth","GXFlver_HairSSSIbl_Bloody",
        "GXFlver_HairSSSIbl_Bloody_Cloth","GXFlver_HairSSSIbl_Bloody_Cm","GXFlver_HairSSSIbl_Bloody_Cm_Cloth"};
    // R3c: the face (GBUF-FAMILIES §17; the pixel and vertex programmes read 2026-11-03): FgFaceIbl, FgFaceIbl_Bloody, FgFaceSSSIbl_Bloody
    // as a Mul piece (below), the _Bloody ones curving the face image too.
    static constexpr const char* faces[]{"GXFlver_FgFaceIbl","GXFlver_FgFaceIbl_Bloody","GXFlver_FgFaceSSSIbl_Bloody"};
    const bool skin_body=among(skins),mul_body=among(muls),hair_body=among(hairs),face_body=among(faces);
    // R3c (the final review's H1 and its fixes' step review M1): the reference albedo (g_DiffuseTexture × FC_DifMapMulCol), here or
    // as the skin's or a Mul piece's layer 0, only where the G-buffer programme writes it: FC_DifMapMulCol used (cbMtdParam's
    // D3D_SVF_USED; ColDif, ColSh, Col: FC_DynamicDiffuseColor, GBUF-FAMILIES §13; EmiGlow: FC_EmissiveColor × FC_GlowScale;
    // Veill_Curtain: cbModelParam's FC_DifColMul), FC_SkinMatchColor unused but by the skin and hair bodies (the eyes and face tint
    // with it otherwise). The others name the diffuse and keep no material until decoded (ColDif's t0,
    // resolved by the fallback, took the reference shape: 254 steps of albedo at the High Wall, run i119-r3c-fallback-m30-on);
    // their roles are not resolved. A second diffuse alone does not refuse (172 programmes: the _Df back-face sets, exact on their
    // front faces; the undecoded Mul variants, EmiMul, Pom, Burn, DirtMask, water, Eclipse: their layer 0 as the reference's,
    // a known approximation, as the Mul bodies before R3c's second tranche).
    const auto used=[&](const char* buffer,const char* name){const auto c=std::find_if(gbuf->constants.begin(),gbuf->constants.end(),[&](const auto& x){return x.name==buffer;});
        return c!=gbuf->constants.end() && std::any_of(c->variables.begin(),c->variables.end(),[&](const auto& v){return (v.flags&2) && v.name==name;});};
    const bool reference=used("cbMtdParam","FC_DifMapMulCol") && (skin_body || hair_body || face_body || !used("cbModelParam","FC_SkinMatchColor"));
    static constexpr const char* names[RtTextureRoles]{"g_DiffuseTexture","g_SpecularTexture","g_BumpmapTexture","g_EmissiveTexture"};
    bool named_diffuse{};
    for(uint32_t role=0;role<RtTextureRoles;++role){
        const auto declared=std::find_if(gbuf->resources.begin(),gbuf->resources.end(),[&](const auto& r){return r.input_type==2 && r.name==names[role];});
        if(declared==gbuf->resources.end())continue;
        ++tally.named[role];named_diffuse|=role==RtTextureDiffuse;
        if(reference && resolve(declared->shader_register,out,role) && out.textures[role])++tally.resolved[role];}
    // I-0126 R7b: the emission (the Emi* programmes' GB5, o5 = g_EmissiveTexture × FC_EmissiveColor: STUDY-R7-DS3-AMBIENT §2 row F)
    // with FC_EmissiveColor as the admission writes cbMatDynParam +0x20 (22E930's +0E block: {f, f, f, 1}, f the material's dynamic
    // object (+0xD0) +0x34 when its flag +0x0E is set, else the buffer's creation zero), for a programme that uses it; read when the
    // set is made (a later write to the material not followed: R3-PLAN §4's risk).
    if(out.textures[RtTextureEmissive] && used("cbMatDynParam","FC_EmissiveColor")){BYTE flag{};float f{};
        if(material && Read(material+0xD0+0x0E,&flag,1) && flag && Read(material+0xD0+0x34,&f,4) && std::isfinite(f) && f>0.f)out.emissive={f,f,f,1.f};}
    // I-0119 R3c: a map blend (MultiBlend, FourBlend, GBUF-FAMILIES §16): no g_DiffuseTexture named; its layers' textures by their
    // tiling rows (the diffuse 1 to 4, the specular 5 to 8, the bump 13 to 16, the mask 0: the same in the 2-, 3- and 4-layer
    // programmes read), each the material's binding at a register the programme samples; two layers at least. Only the decoded shape
    // (the blend review's H1): the rows sampled exactly those (besides the lightmaps, t26 and t27), cbMatDynParam's three blend
    // parameters by their names (`*_snp_0_float_0_0`, `_1_0`, `_2_0`, consecutive) and the last vertex TEXCOORD besides UV0 for the weights;
    // each layer its specular and bump rows, and the programme using no other blend parameter (the final review's H1); the other
    // blends (`_mt1`, `_p`, `_em`, BloodBlend, Sand, the equipment's blends and the unlit ones) keep no material (`_e`'s body is the
    // plain one's: caught). Layer 0
    // in the slice's roles (no FC_DifMapMulCol, the raw gloss: scales 1; UV0 not scrolled, their vertex programmes read no
    // VC_TexScroll), layers 1 to 3 in `layers`; the first of them also the mask, the weights, COLOR0 and P0, P1, P2 (the material's
    // generated words, as the raster writes them into cbMatDynParam; their animation not read).
    // I-0119 R3c: the equipment (SAT_Equip, SAT_Equip_cloth, SAT_Equip_SSS, SAT_Equip_SSS_cloth and their Phantom and [A] twins; the
    // pixel programmes read 2026-11-03: one body, the SSS ones a register later for the same rows): no g_DiffuseTexture by name, the
    // textures by their tiling rows, the diffuse row 5 (the albedo its texel, no FC_DifMapMulCol), the specular row 3 (F0), the bump
    // row 4 (the normal from its rg as the reference's, the gloss its raw blue). Not applied: the damage detail normal (row 2 at UV0 x
    // FC_DamagedNormalUvTiling, added by FC_DamagedNormalParam.x: cbMatDynParam, the hits'), the wet and blood blocks (as the skin's),
    // the rim glow in GB5. The three rows bound, sampled and viewable, else no material (a role waiting keeps the set pending).
    // R3c fourth tranche: SAT_HairIbl2 and SAT_HairIbl2_cloth (the character hair of the witness saves; the pixel programmes read
    // 2026-11-03) the same way by their rows: the diffuse row 0 (t3) with the hair's curve, no FC_DifMapMulCol, the specular row 2
    // (t1), the bump row 1 (t2) whose raw blue is the gloss; their wet and blood decal blocks (t41 by FC_BloodAmount) not applied.
    struct Rowed {const char* body;int32_t diffuse,specular,bump;bool hair;};
    static constexpr Rowed rowed[]{{"SAT_Equip",5,3,4,false},{"SAT_Equip_cloth",5,3,4,false},{"SAT_Equip_SSS",5,3,4,false},{"SAT_Equip_SSS_cloth",5,3,4,false},
        {"SAT_HairIbl2",0,2,1,true},{"SAT_HairIbl2_cloth",0,2,1,true}};
    const Rowed* rows_of{};
    if(!named_diffuse)for(const auto& r:rowed)if(_stricmp(body.c_str(),r.body)==0){rows_of=&r;break;}
    const bool equipment=rows_of!=nullptr;
    if(equipment){
        const auto at_row=[&](int32_t row)->int32_t{for(const auto& input:programs.texture_bindings){
            const int32_t r=input.declaration.remap>=0?input.declaration.remap:static_cast<int32_t>(input.declaration.slot);
            if(r==row && !input.path.empty() && std::any_of(gbuf->resources.begin(),gbuf->resources.end(),[&](const auto& x){return x.input_type==2 && x.shader_register==input.declaration.slot;}))
                return static_cast<int32_t>(input.declaration.slot);}return -1;};
        const int32_t diffuse=at_row(rows_of->diffuse),specular=at_row(rows_of->specular),bump=at_row(rows_of->bump);const bool waiting=pending;out.hair=rows_of->hair;
        if(diffuse>=0 && specular>=0 && bump>=0){(void)resolve(static_cast<uint32_t>(diffuse),out,RtTextureDiffuse);
            (void)resolve(static_cast<uint32_t>(specular),out,RtTextureSpecular);(void)resolve(static_cast<uint32_t>(bump),out,RtTextureBump);out.albedo={1,1,1,1};}
        if(!out.textures[RtTextureDiffuse] || !out.textures[RtTextureSpecular] || !out.textures[RtTextureBump]){
            albedo_pending|=pending && !waiting;out.textures[RtTextureDiffuse].reset();out.views[RtTextureDiffuse]={};}
        // R3c (the final review's M2): a witness (rt-view.flag) by body and outcome: the rows' registers (-1 not bound and sampled)
        // and whether the slice took its material.
        static const bool witness=ModuleFlag(L"rt-view.flag");
        if(witness){static std::mutex lock;static std::set<std::string> seen;const bool took=out.textures[RtTextureDiffuse]!=nullptr;bool fresh{};
            const bool own=pending && !waiting;   // the slice's own waiting (its step review's 4), not the set's
            {const std::lock_guard hold(lock);fresh=seen.size()<32 && seen.insert(body+(took?"|1":"|0")+(own?"|p":"")).second;}
            if(fresh){char row[300]{};sprintf_s(row,"{\"event\":\"native_rt_rows_shape\",\"family\":\"%.120s\",\"diffuse\":%d,\"specular\":%d,\"bump\":%d,\"material\":%s,\"pending\":%s}\n",
                body.c_str(),diffuse,specular,bump,took?"true":"false",own?"true":"false");Log(row);}}
        ++tally.named[RtTextureDiffuse];if(out.textures[RtTextureDiffuse])++tally.resolved[RtTextureDiffuse];}
    if(!named_diffuse && !equipment){
        const auto sampled=[&](uint32_t reg){return std::any_of(gbuf->resources.begin(),gbuf->resources.end(),[&](const auto& r){return r.input_type==2 && r.shader_register==reg;});};
        std::array<int32_t,17> at_row;at_row.fill(-1);
        for(const auto& input:programs.texture_bindings){const int32_t row=input.declaration.remap>=0?input.declaration.remap:static_cast<int32_t>(input.declaration.slot);
            if(row>=0 && row<=16 && !input.path.empty() && sampled(input.declaration.slot))at_row[row]=static_cast<int32_t>(input.declaration.slot);}
        uint32_t layers=0;while(layers<4 && at_row[1+layers]>=0)++layers;
        // The shape: the rows sampled exactly 0, 1..L, 5..4+L, 13..12+L (the lightmaps, at rows 26 and 27, apart).
        bool shaped=layers>=2 && at_row[0]>=0;uint32_t refused=(layers<2?1u:0u)|(at_row[0]<0?2u:0u);int32_t odd_row=-1;
        // The final review's H1: every layer has its specular and bump rows (the unlit and `_em` bodies have none).
        for(uint32_t l=0;l<layers;++l)if(at_row[5+l]<0 || at_row[13+l]<0){shaped=false;refused|=32u;}
        for(const auto& input:programs.texture_bindings){const int32_t row=input.declaration.remap>=0?input.declaration.remap:static_cast<int32_t>(input.declaration.slot);
            if(input.path.empty() || !sampled(input.declaration.slot) || row==26 || row==27)continue;
            const bool expected=row==0 || (row>=1 && row<=int32_t(layers)) || (row>=5 && row<=int32_t(4+layers)) || (row>=13 && row<=int32_t(12+layers));
            if(!expected){shaped=false;refused|=4u;odd_row=row;}}
        std::array<float,3> parameters{0,0,1};int32_t parameter_base=-1;
        const auto dynamic=std::find_if(gbuf->constants.begin(),gbuf->constants.end(),[](const auto& c){return c.name=="cbMatDynParam";});
        if(shaped && dynamic!=gbuf->constants.end()){
            const auto offset_of=[&](const char* suffix)->int32_t{for(const auto& v:dynamic->variables)if(v.name.size()>std::strlen(suffix) &&
                v.name.compare(v.name.size()-std::strlen(suffix),std::strlen(suffix),suffix)==0)return static_cast<int32_t>(v.byte_offset);return -1;};
            const int32_t a=offset_of("_snp_0_float_0_0"),b=offset_of("_snp_0_float_1_0"),c=offset_of("_snp_0_float_2_0");
            // ... and the programme uses no other blend parameter (D3D_SVF_USED: the parallax `_p`, `_em`, the unlit and the
            // equipment's bodies use more; the final review's H1).
            const auto used_snp=std::count_if(dynamic->variables.begin(),dynamic->variables.end(),[](const auto& v){return (v.flags&2) && v.name.find("_snp_")!=std::string::npos;});
            if(a>=0 && b==a+4 && c==a+8 && used_snp==3)parameter_base=a;}
        if(parameter_base<0){if(shaped)refused|=8u;shaped=false;}   // bit 8 only where the parameters were looked at
        // P0, P1, P2 as the raster writes them into cbMatDynParam: the material's generated words (22E930's emission from the SPX
        // and the GX records, 09_model_admission.inc's `generated_parameters`, from byte 0x270), not the MTD's numeric parameters
        // (cbMtdParam's: an earlier lookup by name found none of them, every blend family -1 at build-16). Their animation (the
        // entity's rows, 22E5C0) not applied (INFERRED: none on a map piece), nor the admission's later generator write at the dynamic
        // object's register +3B8 (the final review's L3, kept: it starts at the same byte, 0x270, and the rays' blends match the
        // G-buffer at the reference families' level with these words, RESULT « the blends' parameters »). Bit 64: the generated
        // words do not cover the three.
        if(shaped){const auto& generated=programs.generated_parameters;
            const uint32_t from=generated?generated->byte_offset:0;
            if(generated && uint32_t(parameter_base)>=from && (uint32_t(parameter_base)-from)%4==0 && (uint32_t(parameter_base)-from)/4+3<=generated->words.size())
                for(uint32_t k=0;k<3;++k)std::memcpy(&parameters[k],&generated->words[(uint32_t(parameter_base)-from)/4+k],4);
            else{refused|=64u;shaped=false;}}
        // The weights: the last vertex TEXCOORD the vertex programme reads besides UV0 (TEXCOORD1; TEXCOORD2 in the lightmapped
        // variants, which read TEXCOORD1 too: the run i119-r3c-why-m40-on; their vertex programme passes TEXCOORD2 on, GBUF-FAMILIES §16).
        int32_t weight_index=-1;
        if(shaped){const ds3content::ShaderInterface* vertex{};
            for(uint32_t pixel=0;pixel<3 && !vertex;++pixel){const auto r=NativeRecipe(6,pixel);
                if(programs.recipes[r].status==ds3content::ModelProgramStatus::Resolved && programs.interfaces[r][0])vertex=programs.interfaces[r][0].get();}
            if(vertex)for(const auto& input:vertex->inputs)if(_stricmp(input.semantic.c_str(),"TEXCOORD")==0 && input.semantic_index>0){
                weight_index=(std::max)(weight_index,static_cast<int32_t>(input.semantic_index));}
            if(weight_index<0){shaped=false;refused|=16u;}}
        // A diagnostic (rt-view.flag): the first 32 families with two layers' rows, caught (`why` 0) or refused and why (bits: 1 fewer
        // than two layers, 2 no mask row, 4 another row sampled (the last), 8 not exactly the three blend parameters by name and use,
        // 16 no weight TEXCOORD, 32 a layer without its specular or bump row, 64 P0 to P2 not known); its parameters' byte, the
        // generated words' first byte and count, P0 to P2.
        static const bool witness=ModuleFlag(L"rt-view.flag");
        if(witness && layers>=2){static std::mutex lock;static std::set<std::string> named;auto family=NativeFamilyKey(programs);bool fresh{};
            {const std::lock_guard hold(lock);fresh=named.size()<32 && named.insert(family).second;}
            if(fresh){char row[500]{};sprintf_s(row,"{\"event\":\"native_rt_blend_shape\",\"family\":\"%.200s\",\"layers\":%u,\"caught\":%s,\"why\":%u,\"row\":%d,\"weight\":%d,\"base\":%d,\"generated_offset\":%d,\"generated_words\":%zu,\"p\":[%g,%g,%g]}\n",
                family.c_str(),layers,shaped?"true":"false",shaped?0u:refused,odd_row,weight_index,parameter_base,
                programs.generated_parameters?int32_t(programs.generated_parameters->byte_offset):-1,programs.generated_parameters?programs.generated_parameters->words.size():size_t(0),
                parameters[0],parameters[1],parameters[2]);Log(row);}}
        if(shaped){++tally.blends;out.layers.resize(layers-1);
            for(uint32_t l=0;l<layers;++l){RtMaterialSlice& into=l?out.layers[l-1]:out;
                (void)resolve(static_cast<uint32_t>(at_row[1+l]),into,RtTextureDiffuse);
                if(at_row[5+l]>=0)(void)resolve(static_cast<uint32_t>(at_row[5+l]),into,RtTextureSpecular);
                if(at_row[13+l]>=0)(void)resolve(static_cast<uint32_t>(at_row[13+l]),into,RtTextureBump);}
            auto& first=out.layers[0];
            if(at_row[0]>=0)(void)resolve(static_cast<uint32_t>(at_row[0]),first,RtTextureEmissive);
            if(const auto* e=element("TEXCOORD",static_cast<uint32_t>(weight_index))){
                const uint32_t format=e->format==DXGI_FORMAT_R8G8B8A8_UNORM?RtWeightUnorm8:e->format==DXGI_FORMAT_R32G32B32A32_FLOAT?RtWeightFloat4:
                    e->format==DXGI_FORMAT_R16G16B16A16_FLOAT?RtWeightHalf4:e->format==DXGI_FORMAT_R16G16B16A16_UNORM?RtWeightUnorm16:0u;
                if(format && place(*e,format==RtWeightFloat4?16:format==RtWeightUnorm8?4:8,first.uv_offset,first.uv_stride))first.uv_format=format;else first.uv_stride=0;}
            if(!first.uv_stride)++tally.no_weights;
            if(const auto* e=element("COLOR",0)){
                const uint32_t format=(e->format==DXGI_FORMAT_R8G8B8A8_UNORM || e->format==DXGI_FORMAT_B8G8R8A8_UNORM)?RtAlphaColourUnorm8:   // .w the same byte (L4)
                    e->format==DXGI_FORMAT_R32G32B32A32_FLOAT?RtAlphaColourFloat4:0u;
                if(format && place(*e,format==RtAlphaColourFloat4?16:4,first.colour_offset,first.colour_stride))first.colour_format=format;}
            first.albedo={parameters[0],parameters[1],parameters[2],static_cast<float>(layers)};
            out.albedo={1,1,1,1};out.unscrolled=true;
            if(out.uv_format==RtUvFloat2)out.uv_format=RtUvFloat2Scaled;   // the vertex programme's TEXCOORD0 x 1/2048 (L3), whatever its format
            else if(out.uv_format==RtUvHalf2)out.uv_format=RtUvHalf2Scaled;
            ++tally.named[RtTextureDiffuse];if(out.textures[RtTextureDiffuse])++tally.resolved[RtTextureDiffuse];}}
    // I-0119 R3c: a two-layer piece (GBUF-FAMILIES §3 and §4; their pixel and vertex programmes read 2026-11-03: MulDOL and MulEnvDOL
    // one body, MulMaskDOL and MulMaskEnvDOL one body, MulIbl, MulMaskIbl): a second layer, g_DiffuseTexture2, g_SpecularTexture2 and
    // g_BumpmapTexture2 at UV1 = TEXCOORD0.zw (R16G16B16A16_SINT: VC_TexScroll_1, / 2048) in TANGENT1's frame, lerped by COLOR1.w;
    // MulMask: g_BlendMaskTexture at TEXCOORD1 (.zw in the lightmapped DOL bodies, whose .xy is the lightmap's; .xy in MulMaskIbl;
    // VC_TexScroll_3, / 2048) and FC_BlendMaskParam's .x and .z as the raster's 22E930 block makes them ({a, b, -0.5 a, 0.5 - 0.5 b}
    // from the material's +0xD0+0x28 when its +0xD0+0x0C or +0x19 flag is set, else 0: 09_model_admission.inc). Only those six bodies
    // by name (the _Bloody, _SnowLt, _Cloth, Ghost, Emi and Pom variants carry other layers or programmes not read); a piece missing
    // a part keeps the single-layer reference shape. A witness (rt-view.flag): 48 rows by body and outcome, caught or why not (bits:
    // 1 a second texture not named, 2 TEXCOORD0 not four SINT16 in bounds, 4 no TANGENT1, 8 no COLOR1, 16 the mask's UV, 32
    // FC_BlendMaskParam not known (not finite, or its block unwritten while another writer may cover +0x30: the generator's register,
    // the generated words), 64 a texture of either layer or the mask with no bound path or not yet viewable). The bodies matched
    // without regard to case, as the raster finds its groups (the final review's L4). FC_BlendMaskParam from NativeBlendMaskParam, the
    // raster's own producer (09_model_admission.inc); its block unwritten, the buffer's creation zero.
    if(named_diffuse){
        // The face (R3c fourth tranche): the same two layers, its second diffuse the composed face image (the binding FG_face_gen),
        // the weight COLOR0.w (the vertex programmes pass COLOR0.w as the pixel programme's COLOR1.w) and COLOR0.rgb a factor of the
        // lerped albedo (B8G8R8A8 read as such); a face refused keeps no material (never layer 0 alone).
        if((mul_body || face_body) && reference){
            const bool masked=_stricmp(body.c_str()+(std::min)(body.size(),size_t(21)),"MulMaskDOL")==0 || _stricmp(body.c_str()+(std::min)(body.size(),size_t(21)),"MulMaskEnvDOL")==0 ||
                _stricmp(body.c_str()+(std::min)(body.size(),size_t(21)),"MulMaskIbl")==0;
            const bool lightmapped=body.size()>3 && _stricmp(body.c_str()+body.size()-3,"DOL")==0;
            uint32_t why{};RtMaterialSlice second;
            const auto named=[&](const char* name)->int32_t{const auto r=std::find_if(gbuf->resources.begin(),gbuf->resources.end(),[&](const auto& x){return x.input_type==2 && x.name==name;});
                return r==gbuf->resources.end()?-1:static_cast<int32_t>(r->shader_register);};
            const int32_t d2=named("g_DiffuseTexture2"),s2=named("g_SpecularTexture2"),b2=named("g_BumpmapTexture2"),m2=masked?named("g_BlendMaskTexture"):-1;
            if(d2<0 || s2<0 || b2<0 || (masked && m2<0))why|=1;
            const auto* uv=element("TEXCOORD",0);
            uint32_t uv_first{},uv_stride{};   // the step review's L2: the whole four components in bounds
            if(!uv || uv->format!=DXGI_FORMAT_R16G16B16A16_SINT || out.uv_format!=RtUvSint16 || !out.uv_stride || !place(*uv,8,uv_first,uv_stride))why|=2;
            else{second.uv_offset=uv_first+4;second.uv_stride=uv_stride;second.uv_format=RtUvSint16;}
            if(const auto* e=element("TANGENT",1);!e || e->format!=DXGI_FORMAT_R8G8B8A8_UINT || !place(*e,4,second.tangent_offset,second.tangent_stride))why|=4;
            if(const auto* e=element("COLOR",face_body?0:1)){
                const uint32_t format=e->format==DXGI_FORMAT_R8G8B8A8_UNORM?RtAlphaColourUnorm8:e->format==DXGI_FORMAT_B8G8R8A8_UNORM?(face_body?RtAlphaColourBgra8:RtAlphaColourUnorm8):
                    e->format==DXGI_FORMAT_R32G32B32A32_FLOAT?RtAlphaColourFloat4:0u;
                if(format && place(*e,format==RtAlphaColourFloat4?16:4,second.colour_offset,second.colour_stride))second.colour_format=format;}
            if(!second.colour_format)why|=8;
            std::array<float,2> mask_param{};
            if(masked){const auto* e=element("TEXCOORD",1);uint32_t first{},stride{};
                const bool four=e && e->format==DXGI_FORMAT_R16G16B16A16_SINT,two=e && e->format==DXGI_FORMAT_R16G16_SINT;
                if((lightmapped?four:(four || two)) && place(*e,four?8:4,first,stride)){second.normal_offset=first+(lightmapped?4u:0u);second.normal_stride=stride;}
                else why|=16;
                // The step review's M1: the raster's producer; its block unwritten, +0x30 holds the creation zero unless the generator's
                // register (the dynamic object's +1A, +3B8) or the generated words reach it (then refused, not guessed).
                if(const auto param=NativeBlendMaskParam(material)){
                    // The final review's M1: all four finite, as the block's writer requires (else the raster leaves +0x30 unwritten).
                    if(std::all_of(param->begin(),param->end(),[](float x){return std::isfinite(x);}))mask_param={(*param)[0],(*param)[2]};else why|=32;}
                else{BYTE generator{};uint32_t reg{};const auto& generated=programs.generated_parameters;
                    if(!material || (Read(material+0xD0+0x1A,&generator,1) && generator && Read(material+0xD0+0x3B8,&reg,4) && reg<=3) ||
                       (generated && generated->byte_offset<0x40 && generated->byte_offset+4*generated->words.size()>0x30))why|=32;}}
            // The textures resolved before the witness (the step review's L1, L4): a role with no bound path refuses the layer.
            // The final review's L1 and L2: every role of both layers (and the mask) bound and viewable, else the layer refused and what
            // its resolutions added to the set's pending state and kept textures taken back (a layer-0 role missing would lerp from white).
            // Layer 0's specular and bump first (its fixes' step review, 1): without them the second layer is not resolved at all (a
            // pending one has already made the set pending).
            uint32_t roles{};   // the witness's: 1, 2 layer 0's specular, bump missing; 4, 8, 16, 32 the second diffuse, specular, bump, mask unbound; x 16 not viewable
            if(!out.textures[RtTextureSpecular])roles|=1;if(!out.textures[RtTextureBump])roles|=2;
            if(!why && roles)why|=64;
            if(!why){const bool pending_mul=pending;const size_t kept_mul=kept.size();
                const bool bound_d=resolve(static_cast<uint32_t>(d2),second,RtTextureDiffuse),bound_s=resolve(static_cast<uint32_t>(s2),second,RtTextureSpecular),
                    bound_b=resolve(static_cast<uint32_t>(b2),second,RtTextureBump),bound_m=!masked || resolve(static_cast<uint32_t>(m2),second,RtTextureEmissive);
                const bool bound=bound_d && bound_s && bound_b && bound_m;
                roles|=(bound_d?0u:4u)|(bound_s?0u:8u)|(bound_b?0u:16u)|(bound_m?0u:32u)|(second.textures[RtTextureDiffuse]?0u:64u)|(second.textures[RtTextureSpecular]?0u:128u)|
                    (second.textures[RtTextureBump]?0u:256u)|(!masked || second.textures[RtTextureEmissive]?0u:512u);
                const bool viewable=second.textures[RtTextureDiffuse] && second.textures[RtTextureSpecular] && second.textures[RtTextureBump] &&
                    (!masked || second.textures[RtTextureEmissive]);
                // Not viewable for good (a role without a path, a cube or an array: no pending set): taken back too.
                if(!bound || !viewable){why|=64;if(!bound || pending==pending_mul){pending=pending_mul;kept.resize(kept_mul);}}}
            static const bool witness=ModuleFlag(L"rt-view.flag");
            if(witness){static std::mutex lock;static std::set<std::string> seen;bool fresh{};
                {const std::lock_guard hold(lock);fresh=seen.size()<48 && seen.insert(body+"|"+std::to_string(why)+"|"+std::to_string(roles)).second;}   // by body and outcome (L4)
                if(fresh){char row[300]{};sprintf_s(row,"{\"event\":\"native_rt_mul_shape\",\"family\":\"%.120s\",\"mask\":%s,\"caught\":%s,\"why\":%u,\"roles\":%u,\"pending\":%s,\"mask_param\":[%g,%g]}\n",
                    body.c_str(),masked?"true":"false",why?"false":"true",why,roles,pending && !pending_before?"true":"false",mask_param[0],mask_param[1]);Log(row);}}
            if(!why){second.albedo={mask_param[0],mask_param[1],0,0};
                out.layers.assign(1,std::move(second));out.mul=true;out.mul_mask=masked;
                out.face=face_body;out.face_both=face_body && _stricmp(body.c_str(),"GXFlver_FgFaceIbl")!=0;}
            // A face refused keeps no material; its image not composed yet keeps the set pending (the final review's M1: as an
            // albedo waiting, not taken back below).
            // Its step review's 3: a role waiting only (why 64), not a shape refused.
            else if(face_body){albedo_pending|=pending && !pending_before && (why&~64u)==0;out.textures[RtTextureDiffuse].reset();out.views[RtTextureDiffuse]={};}}
        // I-0119 R3c: the skin (GBUF-FAMILIES §17; the pixel programmes read 2026-11-03: SkinIbl, SkinIbl_Bloody, SkinSSSIbl_Bloody
        // and their Phantom and [A] twins): albedo = FC_DifMapMulCol × saturate(sqrt(t0) + FC_SkinMatchColor)², the colour PS
        // cbModelParam +0x60, the instance's (RtInstance::skin). The _Bloody bodies' wet and blood blocks (FC_WetParam*, FC_BloodAmount)
        // not applied: neutral where the control view met them (F0 and gloss within a step). The hair: its curve (above). The face
        // (FgFace*): the Mul path above. The eyes (the colour by t0.a at a parallax UV) left: no material.
        out.skin=skin_body || face_body;out.hair=hair_body;}
    // Final review L1: a material only where the albedo is there (the reference's g_DiffuseTexture, a decoded blend's layer 0, the
    // equipment's row 5 or the SAT hair's row 0, resolved; a face with its image); the other families keep their triangle and alpha test, no material. The family's index only for the control view
    // (L2: rt-view.flag; 0 otherwise).
    static const bool families=ModuleFlag(L"rt-view.flag");
    if(families)out.family=NativeRtFamilyOf(NativeFamilyKey(programs));
    out.material=out.textures[RtTextureDiffuse]!=nullptr;if(out.material){++tally.slices;tally.emissive+=out.emissive[0]>0.f;}   // R7b: the records with an emission
    // Its other textures then unread: no view made for them (the fixes' step review L-d).
    else{for(uint32_t role=RtTextureSpecular;role<RtTextureRoles;++role){out.textures[role].reset();out.views[role]={};}
        out.layers.clear();out.mul=out.mul_mask=out.skin=out.hair=out.face=out.face_both=false;   // the blend review's L3: no view for an unread layer (R3c's step review L3: nor a Mul flag)
        if(!albedo_pending)pending=pending_before;kept.resize(kept_before);}
}
// I-0119 R3c: a candidate's FC_SkinMatchColor, PS cbModelParam +0x60 (the draw state's +0x280, ProduceModelParamPs): the part's
// appearance colour CFDE70 copies into its stack copy of the group (the candidate's draw_colour), else the draw group's +0x40;
// unread or not finite, rgb 0 (the skin curve's identity) with w -1 (the hair's curve then its identity, 1), else w 1.
void NativeRtSkinColour(const NativeSelectionCandidate& c,std::array<float,4>& skin) noexcept {skin={};
    const uintptr_t from=c.draw_colour?c.draw_colour:c.group?c.group+0x40:0;
    if(!from || !Read(from,skin.data(),16) || !std::all_of(skin.begin(),skin.end(),[](float x){return std::isfinite(x);}))skin={0,0,0,-1};
    else skin[3]=1;}   // the step review's L2: w (no curve reads it) tells the hair's curve the colour unread (its identity 1, not black)
// The material sets' and the lightmaps' revalidation and retry, in walks (NativeRtMaterialsOf, NativeRtLightmapsOf).
constexpr uint64_t kNativeRtMaterialRevalidate=600,kNativeRtMaterialRetry=30;
// I-0125 R6a (the step review's M1: the raster binds them per placement): an instance's lightmaps, the t26 and t27 objects of its
// lightmap set (ReadNativeLightmapSet on its descriptor) as ApplyNativeLightmapSet binds them, a holder's inner or an admitted
// texture, resident with a view; cbTexHdrScaleParam the t26 object's constant (native_texture_constants), else its texture's float
// metadata, else 1 (13b's rule). Kept by descriptor between walks: made again after kNativeRtMaterialRevalidate walks, or after
// kNativeRtMaterialRetry while one is not resident (its ambient missing meanwhile), or when a texture is withdrawn; dropped two walks
// after the walk last met it. No set: none (the hits' ambient missing). Not the raster's fallback to the material's own t26 path
// (the step review's L1: counted as missing).
struct NativeRtLightmapEntry {std::array<std::shared_ptr<const bootstrap::NativeTextureRecord>,2> maps;std::array<float,4> hdr{1,1,1,1};uint64_t made{},seen{};bool pending{};};
std::mutex native_rt_lightmaps_lock;std::unordered_map<uintptr_t,NativeRtLightmapEntry> native_rt_lightmaps;
void NativeRtLightmapsOf(uintptr_t descriptor,uint64_t walk,RtInstanceAmbient& out) {
    const auto apply=[&](const NativeRtLightmapEntry& e){out.hdr=e.hdr;for(uint32_t k=0;k<2;++k)if(e.maps[k]){out.lightmaps[k]=e.maps[k];out.views[k]=e.maps[k]->view.handle;}};
    {const std::lock_guard hold(native_rt_lightmaps_lock);
        if(const auto found=native_rt_lightmaps.find(descriptor);found!=native_rt_lightmaps.end()){auto& e=found->second;e.seen=walk;
            const bool withdrawn=std::any_of(e.maps.begin(),e.maps.end(),[](const auto& t){return t && t->withdrawn.load(std::memory_order_acquire);});
            if(!withdrawn && walk<e.made+(e.pending?kNativeRtMaterialRetry:kNativeRtMaterialRevalidate)){apply(e);return;}}}
    NativeRtLightmapEntry e;e.made=e.seen=walk;
    // Both of ApplyNativeLightmapSet's forms (the final review's H1): a DOL set's t26 and t27 its objects 3 and 4, another set's its
    // objects 0 and 1 (t26..t28 from 0..2).
    if(const auto lightmaps=ReadNativeLightmapSet(descriptor)){const uint32_t first=lightmaps->objects[3]?3:0;
        for(uint32_t k=0;k<2;++k){const uintptr_t object=lightmaps->objects[first+k];if(!object)continue;
            std::shared_ptr<const bootstrap::NativeTextureRecord> record;uintptr_t table{};
            if(Read(object,&table,8) && table==image+0x29559E8){const std::array<uintptr_t,4> holders{object,0,0,0};std::array<uintptr_t,4> inners{};
                record=ResolveNativeTextureHolders(holders,inners);}
            else if(auto acquired=textures.Acquire(object);acquired && acquired->resident && acquired->resource && acquired->view)record=std::move(acquired);
            if(!record || !record->view){e.pending=true;continue;}
            if(!record->description.cube && record->description.slices==1)e.maps[k]=std::move(record);}   // the shader reads a Texture2D
        bool constant{};
        // The constant only from a DOL set's t26 object (5B110), as ApplyNativeLightmapSet; else the texture's metadata.
        if(first==3){const auto receiver=NativeLightmapReceiver(lightmaps->objects[3]);const std::lock_guard hold(native_texture_constant_lock);
            if(const auto found=native_texture_constants.find(receiver);found!=native_texture_constants.end()){e.hdr=found->second;constant=true;}}
        if(!constant && e.maps[0] && e.maps[0]->content && e.maps[0]->content->float_metadata){
            const auto& words=e.maps[0]->content->float_metadata->words;for(size_t i=0;i<4 && i<words.size();++i)std::memcpy(&e.hdr[i],&words[i],4);}}
    apply(e);
    const std::lock_guard hold(native_rt_lightmaps_lock);native_rt_lightmaps[descriptor]=std::move(e);
}
// I-0125 R6a: an instance's ambient rows (RtInstanceAmbient) as ProduceNativeSelectedInstances makes the raster's record: its animation
// object's rows and its group's lighting inputs through ProduceInstanceRecord; only what its set's slices read (a lightmapped slice the
// atlas and the rotation, an Ibl one the SH rows: zero when the inputs are not read, as the raster's record then).
void NativeRtInstanceAmbient(const NativeSelectionCandidate& c,const RtMaterialSet& set,uint64_t walk,const std::array<float,3>& row_scale,RtInstanceAmbient& out) {
    // I-0125 R6b: its light and fog set as ProduceNativeSelectedInstances indexes it (the group's key block), the hits' directional
    // lights; a key the raster has not registered (a model not drawn) set 0.
    {NativeLightKey key{};if(Read(c.group+0xF8,&key[0],16) && Read(c.group+0x110,&key[4],16) && Read(c.group+0x108,&key[8],4) && Read(c.group+0x120,&key[9],4))
        out.light_set=NativeLightSetFound(key);}   // the step review's M3: found, never registered by the rays (0 otherwise)
    if(!set.dol && !set.sh)return;
    if(set.dol)NativeRtLightmapsOf(c.animation,walk,out);
    ds3prod::InstanceRecordInputs input;(void)NativeInstanceAnimationRows(c.animation,input);
    if(set.sh)input.lighting=NativeInstanceLighting(c.group,c.animation,row_scale);   // I-0113 tranche 34: the raster's pass scale
    std::array<uint8_t,ds3prod::instance_record_bytes> bytes{};std::array<uint8_t,ds3prod::instance_record_bytes/4> written{};
    ds3prod::ProduceInstanceRecord(input,bytes.data(),written.data());
    if(input.animation_colour)std::memcpy(out.atlas.data(),bytes.data()+0x30,16);
    if(input.animation_auxiliary)std::memcpy(out.rotate.data(),bytes.data()+0xC0,16);
    if(input.lighting){std::memcpy(out.sh.data(),bytes.data()+0x80,64);out.sh_read=true;}
}
// I-0119 (R3a): a candidate's material set, its LOD-0 slices' materials (NativeRtSliceMaterial) with T3's alpha test.
// I-0117 (R2 T3, R2-PLAN §3): a candidate's alpha-tested slices as its programmes test them (rt_material.hlsli; RESULT « T3 »): a mesh
// of LOD 0 whose layout's programme set (the choice of the candidate's prefer_channel, group+0x98 bit 1 clear, as the admission's)
// has an alpha reference (the policy's template value: the DptA and Gbuf_[A] programmes) and a nonzero g_AlphaRef (FC_AlphaRef.x
// = (ref - 0.0001) / 255, as the MTD producer writes it; 0 is -100, no test); its diffuse texture the binding of t0
// (g_DiffuseTexture; R3c: or of the one register its DptA samples when that is the material's diffuse, the equipment's) resolved as the raster resolves it (NativeTextureHolders, the first resident candidate; a stand-in "_l"
// candidate tried again); its UV the layout's TEXCOORD0 as UV16 (slot 0 or the second stream); the vertex's COLOR1.w when both DptA
// programmes read it (the Grass, Col and Emi families: T3's step reviews); its tiling row FC_TextureTilingScale[0] as the ordinary
// register 1 makes it (ProduceModelTextureTiling, without the UV animation offsets). Kept by model object between walks: made
// again when the storage changes, every kNativeRtMaterialRevalidate walks, or after kNativeRtMaterialRetry walks while a texture is not
// resident, a stand-in or withdrawn; dropped two walks after the walk last met it, and at a scene reset (T3 step review M3). Its
// tallies counted at every walk that meets it (M4). Null: no slice with a material, an alpha test or mixed rays (every slice
// opaque, no record). The walk's thread or the pool's (T5b, T5c: the entries in shards under their locks).
struct NativeRtMaterialEntry {std::shared_ptr<const RtMaterialSet> set;uint64_t storage_generation{},made{},seen{},scene{};bool pending{};
    bool deferred{};   // T5 final review M2: a programme set left unbuilt by the walk's budget alone: made again at the next walk
    uint64_t slices{},unresolved{},unsupported{},untextured{},unknown{};   // its alpha-tested slices, of them not resident, not read,
    // untextured; its meshes whose programme set or DptA programme is not there yet
    std::vector<std::shared_ptr<const bootstrap::NativeTextureRecord>> textures;
    RtSurfaceClasses surface;   // T5: its LOD-0 slices' material classes (RtInstanceMask)
    NativeRtMaterialTally material;   // I-0119 R3a: its slices' materials
    // I-0125 R6d (the fixes' step review M-a): its forward slices (their index, their engine material), whose FC_DynamicDiffuseColor is
    // checked at every walk: the set made again only when one changed.
    std::vector<std::pair<uint32_t,uintptr_t>> forward_materials;};
// By model object and programme channel (two poses of one model may draw through groups of another channel: the third T3 step
// review's L6). T5c: in 16 shards by the key's hash, each with its lock (T5b's walk takes an entry for every static instance from
// the pool's chunks: one lock for all serialised them, INFERRED from m45's 0.70 ms of static chunks).
// The shard from the mixed hash's top bits, the buckets from the whole (NativeRtSkinKeyHash). Not aligned to a cache line (T5c final
// review L1, kept: a shard spans more than two lines, its neighbours share one at most; alignas pads and warns, C4324).
struct NativeRtMaterialShard {std::mutex lock;std::unordered_map<NativeRtSkinKey,NativeRtMaterialEntry,NativeRtSkinKeyHash> map;};
std::array<NativeRtMaterialShard,16> native_rt_materials;std::atomic<bool> native_rt_materials_closed{};
std::array<NativeRtMaterialShard,16> native_rt_menu_materials;   // I-0140 tranche 31: the menu views' sets (native_rt_menu_caches)
NativeRtMaterialShard& NativeRtMaterialShardOf(const NativeRtSkinKey& key) noexcept {auto& shards=native_rt_menu_caches?native_rt_menu_materials:native_rt_materials;
    return shards[(NativeRtSkinKeyHash{}(key)>>60)%shards.size()];}
// T5 final review M2: the programme sets the rays' builds did not get, by layout and channel: the registries' generation then and the
// walk until which they are not tried again.
// The layout held weakly (step review L1: a layout freed in a zone whose address a new one reuses never matches, as the choices'
// materials, 08_model_programmes.inc).
struct NativeRtProgramsFailed {uint64_t generation{},until{};std::weak_ptr<const NativeModelLayout> layout;};
std::mutex native_rt_programs_failed_lock;std::unordered_map<NativeRtSkinKey,NativeRtProgramsFailed,NativeRtSkinKeyHash> native_rt_programs_failed;
// I-0140 tranche 31 (its step review M1): the menu walk's own records, its `until` in the menu's walk numbers (one lock for both maps).
std::unordered_map<NativeRtSkinKey,NativeRtProgramsFailed,NativeRtSkinKeyHash> native_rt_menu_programs_failed;
std::unordered_map<NativeRtSkinKey,NativeRtProgramsFailed,NativeRtSkinKeyHash>& NativeRtProgramsFailedMap() noexcept {
    return native_rt_menu_caches?native_rt_menu_programs_failed:native_rt_programs_failed;}
void CloseNativeRtMaterials() noexcept {native_rt_materials_closed.store(true);for(auto* shards:{&native_rt_materials,&native_rt_menu_materials})for(auto& shard:*shards){const std::lock_guard hold(shard.lock);shard.map.clear();}
    {const std::lock_guard hold(native_rt_programs_failed_lock);native_rt_menu_programs_failed.clear();}
    {const std::lock_guard hold(native_rt_lightmaps_lock);native_rt_lightmaps.clear();}}   // I-0125 R6a
// I-0140 tranche 31 (its step review M2): the menu walk's caches emptied (a scene reset, or 2.5 s after the last menu collect that
// traced a view: nothing of them is drawn again; the frames that hold their entries keep them to completion).
void ClearNativeMenuRtCaches() noexcept {
    {const std::lock_guard hold(native_rt_menu_skinned_lock);native_rt_menu_skinned.clear();}
    for(auto& shard:native_rt_menu_materials){const std::lock_guard hold(shard.lock);shard.map.clear();}
    {const std::lock_guard hold(native_rt_programs_failed_lock);native_rt_menu_programs_failed.clear();}
}
// I-0125 R6d: a forward slice's blend as its row's (13b_prepare_model_frame.inc): NativeOrdinaryForwardBlend with the draw group's
// kind (+0x84) and row (+0x85), else the MTD's mode (0 unblended, 2 SRC_ALPHA / INV_SRC_ALPHA; another one the raster does not draw);
// as two factors for the rays (RtForwardBlend), the colour's operation ADD with a source ONE, SRC_ALPHA or ZERO and a destination
// ZERO, ONE or INV_SRC_ALPHA. False: another blend (the slice left out of the rays, counted in `forward` without `forward_made`). The set is made by model
// object: its first candidate's group decides for every placement (a kind differing between placements not modelled).
bool NativeRtForwardBlendOf(uintptr_t group,uint32_t blend_mode,uint32_t& out) {
    std::array<uint8_t,2> kind{};D3D12_BLEND_DESC desc{};
    if(Read(group+0x84,kind.data(),2) && NativeOrdinaryForwardBlend(kind[0],kind[1],blend_mode,desc)){const auto& t=desc.RenderTarget[0];
        if(!t.BlendEnable){out=RtForwardBlend(RtFactorOne,RtFactorZero);return true;}
        if(t.BlendOp!=D3D12_BLEND_OP_ADD)return false;
        const uint32_t source=t.SrcBlend==D3D12_BLEND_ONE?RtFactorOne:t.SrcBlend==D3D12_BLEND_SRC_ALPHA?RtFactorSrcAlpha:t.SrcBlend==D3D12_BLEND_ZERO?RtFactorZero:UINT32_MAX;
        const uint32_t destination=t.DestBlend==D3D12_BLEND_ZERO?RtFactorZero:t.DestBlend==D3D12_BLEND_ONE?RtFactorOne:t.DestBlend==D3D12_BLEND_INV_SRC_ALPHA?RtFactorInvSrcAlpha:UINT32_MAX;
        if(source==UINT32_MAX || destination==UINT32_MAX)return false;
        out=RtForwardBlend(source,destination);return true;}
    if(blend_mode==0){out=RtForwardBlend(RtFactorOne,RtFactorZero);return true;}
    if(blend_mode==2){out=RtForwardBlend(RtFactorSrcAlpha,RtFactorInvSrcAlpha);return true;}
    return false;
}
// T5: `surface` the slices' material classes, read in the same pass (the entry's, kept with it).
std::shared_ptr<const RtMaterialSet> NativeRtMaterialsOf(const NativeSelectionCandidate& c,const NativeModelStorage& storage,uint64_t walk,NativeRtWalkCounts& counts,RtSurfaceClasses& surface) {
    const auto scene=NativeKeptDrawResets();
    const auto tally=[&](const NativeRtMaterialEntry& e){counts.alpha_slices+=e.slices;counts.alpha_unresolved+=e.unresolved;counts.alpha_unsupported+=e.unsupported;
        counts.alpha_untextured+=e.untextured;counts.alpha_unknown+=e.unknown;const auto& m=e.material;
        counts.material_slices+=m.slices;counts.material_pending+=m.pending;counts.material_no_gbuf+=m.no_gbuf;counts.material_no_uv+=m.no_uv;counts.material_no_frame+=m.no_frame;counts.material_blends+=m.blends;counts.material_no_weights+=m.no_weights;
        counts.wind_slices+=m.wind;counts.wind_unsupported+=m.wind_unsupported;   // T6
        counts.material_forward+=m.forward;counts.material_forward_made+=m.forward_made;counts.material_emissive+=m.emissive;   // I-0125 R6d; I-0126 R7b
        for(size_t k=0;k<4;++k){counts.material_named[k]+=m.named[k];counts.material_resolved[k]+=m.resolved[k];}};
    surface={};uint32_t group_flags{};if(!Read(c.group+0x98,&group_flags,4))return nullptr;
    const size_t channel=(group_flags&2u)?0:1;   // the admission's prefer_channel (09_model_admission.inc), the programmes' slot
    const NativeRtSkinKey key{c.model,channel};auto& shard=NativeRtMaterialShardOf(key);
    // I-0125 R6d (step review M2, the fixes' review M-a): a forward slice's FC_DynamicDiffuseColor as the admission's of this collect:
    // the set made again when one changed (its other inputs on the usual cadence). The colours are read outside the shard's lock (R6d's
    // final review: NativeDynamicDiffuseColour may call the engine's provider), then the entry taken again if it is still that set.
    std::shared_ptr<const RtMaterialSet> cached;std::vector<std::pair<uint32_t,uintptr_t>> coloured;
    {const std::lock_guard hold(shard.lock);
        if(const auto found=shard.map.find(key);found!=shard.map.end() && found->second.storage_generation==storage.generation && found->second.scene==scene){
            auto& e=found->second;e.seen=walk;
            const bool withdrawn=std::any_of(e.textures.begin(),e.textures.end(),[](const auto& t){return t && t->withdrawn.load(std::memory_order_acquire);});
            if(!withdrawn && walk<e.made+(e.deferred?1:e.pending?kNativeRtMaterialRetry:kNativeRtMaterialRevalidate)){
                if(!e.set || e.forward_materials.empty()){tally(e);surface=e.surface;return e.set;}
                cached=e.set;coloured=e.forward_materials;}}}
    if(cached && std::all_of(coloured.begin(),coloured.end(),[&](const auto& f){
        return f.first<cached->slices.size() && NativeRtForwardColour(f.second)==cached->slices[f.first].albedo;})){
        const std::lock_guard hold(shard.lock);
        if(const auto found=shard.map.find(key);found!=shard.map.end() && found->second.set==cached){tally(found->second);surface=found->second.surface;return cached;}}
    ++counts.alpha_made;
    NativeRtMaterialEntry entry;entry.storage_generation=storage.generation;entry.made=entry.seen=walk;entry.scene=scene;
    ingress::CurrentModelInputs inputs;
    if(ingress::ReadCurrentModelInputsAt(image,c.model,c.source_field,c.instance_field,c.pose,inputs) && inputs.meshes.size()==storage.meshes.size()){
        uintptr_t named_registry{};Read(image+0x4792F58,&named_registry,8);
        auto set=std::make_shared<RtMaterialSet>();set->slices.resize(storage.meshes.size());bool any=false,any_material=false,any_wind=false,any_forward=false;entry.surface.known=true;uint8_t first_keep{};
        for(const auto& mesh:inputs.meshes){if(mesh.ordinal>=storage.meshes.size() || !(mesh.lod_mask&1))continue;
            std::shared_ptr<NativeModelLayout> layout;
            {const std::shared_lock hold(model_layout_lock);const auto found=model_layouts.find(mesh.layout);if(found!=model_layouts.end())layout=found->second;}
            // A mesh whose programme set is not built yet (the raster has not admitted the model: the rays see everything loaded)
            // is counted as unknown and its set tried again after kNativeRtMaterialRetry walks (final review M1). T5: the rays build
            // it as the admission does (AcquireNativeModelPrograms with the mesh's material, the same channel), at most
            // kNativeRtProgramsAWalk a walk (the i117-t5a runs: 787 slices at the High Wall, 279 at m45 had none).
            std::shared_ptr<const NativeModelLayout::Programs> programs;
            if(layout){const std::lock_guard hold(layout->programs_lock);programs=layout->choices[channel].programs;}
            // The admission's gate first (a material retired between the lookups, or a layout of another material or type: step
            // review L3); the builds bounded by count and by time (M1: the engine's thread; a miss of the analysis cache is long).
            // A layout whose build returned none is not tried again for kNativeRtMaterialRevalidate walks while the registries are the
            // same (final review M2: a failing build would take the budget every cycle); one the budget alone left is made again at the
            // next walk (`deferred`).
            if(!programs && layout){const NativeRtSkinKey failed_key{reinterpret_cast<uintptr_t>(layout.get()),channel};
                const auto generation=program_registry_generation.load(std::memory_order_acquire);bool refused{};
                {const std::lock_guard hold(native_rt_programs_failed_lock);auto& records=NativeRtProgramsFailedMap();const auto found=records.find(failed_key);
                    refused=found!=records.end() && found->second.generation==generation && walk<found->second.until &&
                        found->second.layout.lock()==layout;}
                auto& budget=*counts.budget;
                if(!refused && !budget.Open())entry.deferred=true;
                else if(!refused){std::shared_ptr<const NativeModelMaterial> material;
                    {const std::shared_lock hold(model_material_lock);const auto found=model_materials.find(mesh.material);if(found!=model_materials.end())material=found->second;}
                    if(material && layout->material==material && layout->type==mesh.layout_type){
                        if(!budget.Take())entry.deferred=true;   // T5b: another chunk took the last build
                        else{LARGE_INTEGER a{},b{};QueryPerformanceCounter(&a);
                            {const kprofile::Scope k(kprofile::MeshPrograms);programs=AcquireNativeModelPrograms(material,layout,channel==1);}
                            QueryPerformanceCounter(&b);budget.ticks.fetch_add(uint64_t(b.QuadPart-a.QuadPart),std::memory_order_relaxed);
                            if(programs)budget.built.fetch_add(1,std::memory_order_relaxed);
                            else{const std::lock_guard hold(native_rt_programs_failed_lock);NativeRtProgramsFailedMap()[failed_key]={generation,walk+kNativeRtMaterialRevalidate,layout};}}}}}
            if(!programs){++entry.unknown;entry.pending=true;entry.surface.known=false;continue;}
            // T5: its class with the CPU special flag false: a Decal keeps its non-Decal class here (explicit policy since I-0140
            // tranche 6, where the admission gives it class 8 or 0x10: the rays keep their surfaces and casters as before; it never casts):
            // a surface when of a G-buffer class and writing depth (the raster's rows take the MTD's g_GX_bDisableDepthWrite, and since I-0140 tranche 6 a G-buffer row its +0x111, 13b: the
            // High Wall's sky and m45's grime cards are G-buffer slices without it, INFERRED from the i117-t5a pictures and 13b, the
            // materials not read), casting by the cascade row's rule (RtSliceCasts: a surface or, I-0118 R5c, a transparent slice,
            // unless a Decal, its no-cast byte or a water or refraction material); its ray bits in its record (RtSliceKeep).
            if(!programs->policy){entry.surface.known=false;continue;}
            {const uint32_t cls=ds3content::ClassifyModelMaterial(*programs->policy,false);entry.surface.classes|=cls;
                BYTE decal{},no_cast{};Read(mesh.material+0xD0,&decal,1);Read(mesh.material+0xD1,&no_cast,1);
                // I-0125 R6c: the water's own rays find a g_GXFT_PlanarReflection slice (0x10, the materials that sample t54; RtMaskWater).
                const bool water=(programs->policy->flags&0x10)!=0;
                const bool is_surface=(cls&kRtGbufferClasses) && !programs->policy->disable_depth_write,casts=RtSliceCasts(cls,is_surface,decal,no_cast,(programs->policy->flags&0x30)!=0);
                entry.surface.surfaces|=is_surface;entry.surface.casts|=casts;entry.surface.overlays|=!is_surface;entry.surface.translucent|=casts && !is_surface;
                entry.surface.water|=water;
                const uint8_t keep=RtSliceKeep(is_surface,casts,water);set->slices[mesh.ordinal].keep=keep;
                if(first_keep==0)first_keep=keep;else if(keep!=first_keep)entry.surface.mixed=true;}
            // I-0119 R3a: its material (and its triangle's data), whatever its alpha test.
            if(layout){const size_t kept_at=entry.textures.size();const bool pending_at=entry.pending;   // R6d: a refused blend's textures given back
                NativeRtSliceMaterial(*programs,*layout,storage,mesh.ordinal,mesh.material,named_registry,c.face_owner,set->slices[mesh.ordinal],entry.material,entry.pending,entry.textures);
                any_material|=set->slices[mesh.ordinal].material;
                // I-0125 R6d: a forward slice's blend (another blend: out of the rays); a witness once by body and outcome.
                if(auto& slice=set->slices[mesh.ordinal];slice.forward || (!(slice.keep&RtMaskView) && !slice.material)){   // a forward-only slice
                    const bool blended=slice.forward && NativeRtForwardBlendOf(c.group,programs->policy->blend_mode,slice.forward_blend);
                    // L1: no useless layer record; R6d's final review: nor its textures kept or the set left pending for them.
                    if(slice.forward && !blended){slice.forward=false;slice.layers.clear();slice.textures={};slice.views={};entry.textures.resize(kept_at);entry.pending=pending_at;}
                    any_forward|=slice.forward;entry.material.forward_made+=slice.forward?1:0;
                    if(slice.forward && slice.forward_kind==RtForwardCol)entry.forward_materials.emplace_back(mesh.ordinal,mesh.material);
                    static const bool forward_witness=ModuleFlag(L"rt-view.flag");   // the step review's L2: with the other witnesses' flag
                    static std::mutex witness_lock;static std::set<std::string> witnessed;bool fresh{};
                    if(forward_witness){const auto family=NativeFamilyKey(*programs);
                        {const std::lock_guard hold(witness_lock);fresh=witnessed.size()<32 && witnessed.insert(family+(slice.forward?"|1":"|0")).second;}
                    if(fresh){std::string name;for(const auto ch:NativeModelDataName(c.model))name.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');char row[512]{};
                        sprintf_s(row,"{\"event\":\"native_rt_forward_slice\",\"model\":\"%.60s\",\"family\":\"%.200s\",\"forward\":%s,\"blend\":\"%X\",\"mode\":%u,\"colour\":[%g,%g,%g,%g]}\n",
                            name.c_str(),family.c_str(),slice.forward?"true":"false",slice.forward_blend,programs->policy->blend_mode,slice.albedo[0],slice.albedo[1],slice.albedo[2],slice.albedo[3]);Log(row);}}}
                NativeRtSliceWind(*programs,*layout,set->slices[mesh.ordinal],entry.material);any_wind|=set->slices[mesh.ordinal].wind!=0;}   // T6
            if(!programs->policy->alpha_ref || !programs->authored || !programs->texture_bindings_available)continue;
            double reference{};
            for(const auto& p:programs->authored->parameters)if(p.name=="g_AlphaRef" && !p.values.empty()){reference=p.values[0];break;}
            if(reference==0)continue;   // FC_AlphaRef.x -100: no pixel is cut
            // The shape the rays reproduce (the census of the 660 alpha-tested programmes, RESULT « T3 »): the alpha-tested depth
            // programme (DptA, recipe 21) samples t0 g_DiffuseTexture alone, besides the dither (t38) and the depth (t56), or no
            // texture: its alpha is t0's .w (or 1), times the vertex's COLOR1.w where both its programmes read it (the vertex
            // programme's input and the pixel programme's varying: the Grass, Col and Emi families; T3 step review M1, its second
            // review's M1 and L2). With neither a texture nor the colour the census's alpha is the constant 1 (62 programmes: nothing
            // is cut), save 6 (the water DptA's 0, a vertex TEXCOORD2.x) not told apart here: all left opaque, counted as untextured
            // (the third review's M2). Another texture, a second one or a blend: not reproduced (opaque, counted; R3's material table).
            const bool resolved=programs->recipes[21].status==ds3content::ModelProgramStatus::Resolved;
            const auto& face=programs->interfaces[21][1];const auto& vertex=programs->interfaces[21][0];int sampled{};bool diffuse{};uint32_t alpha_reg{};
            if(resolved && face)for(const auto& r:face->resources)
                if(r.input_type==2 && r.shader_register!=38 && r.shader_register!=56){++sampled;diffuse|=r.shader_register==0;alpha_reg=r.shader_register;}
            const auto reads=[](const std::shared_ptr<const ds3content::ShaderInterface>& f){if(f)for(const auto& input:f->inputs)
                if(input.semantic_index==1 && _stricmp(input.semantic.c_str(),"COLOR")==0 && (input.usage_mask&8))return true;return false;};
            const bool colour=resolved && reads(vertex) && reads(face);
            if(resolved && face && !sampled && !colour){++entry.slices;++entry.untextured;continue;}
            if(resolved && face)++entry.slices;
            // A DptA programme missing: unknown, tried again (it comes only to a group composed again from loose programmes; a
            // family without one stays unknown: T4 counts it apart); ambiguous, unsupported, invalid or not analysed: not reproduced.
            if(programs->recipes[21].status==ds3content::ModelProgramStatus::Missing){++entry.unknown;entry.pending=true;
                // T5 (rt-view.flag): the first 32 such families named once, with their Gbuf recipes' and Dpt's statuses.
                static const bool name_unknown=ModuleFlag(L"rt-view.flag");
                if(name_unknown){static std::mutex unknown_lock;static std::set<std::string> unknown_named;auto family=NativeFamilyKey(*programs);bool fresh{};
                    {const std::lock_guard hold(unknown_lock);fresh=unknown_named.size()<32 && unknown_named.insert(family).second;}
                    if(fresh){std::string name;for(const auto ch:NativeModelDataName(c.model))name.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');
                        const auto st=[&](uint32_t pass,uint32_t pixel){return static_cast<unsigned>(programs->recipes[NativeRecipe(pass,pixel)].status);};
                        char row[512]{};sprintf_s(row,"{\"event\":\"native_rt_alpha_unknown\",\"model\":\"%.60s\",\"family\":\"%.200s\",\"ref\":%g,\"gbuf\":[%u,%u,%u],\"dpt\":[%u,%u]}\n",
                            name.c_str(),family.c_str(),reference,st(6,0),st(6,1),st(6,2),st(2,0),st(2,1));Log(row);}}
                continue;}
            if(!resolved || !face){++entry.slices;++entry.unsupported;continue;}
            // R3c (the final review's L9): one texture at another register is the shape only as the material's diffuse there (the
            // equipment's DptA samples t1, `*_snp_0_Texture2D_5`, at UV0 tiled by row 5, the material's row-5 diffuse: SAT_Equip's
            // DptA programmes read 2026-11-03): a binding with a path there (the step review's L1), the texture the material's (below),
            // its tiling the material's row (the SAT families' `*_Texture2D_N` read with row N; the DptA's own row not compared).
            const bool bound_there=std::any_of(programs->texture_bindings.begin(),programs->texture_bindings.end(),[&](const auto& input){
                return input.declaration.slot==alpha_reg && !input.path.empty();});   // the step review's L1
            if(sampled>1 || (sampled && !diffuse && (!set->slices[mesh.ordinal].textures[RtTextureDiffuse] || !bound_there))){++entry.unsupported;continue;}
            const auto& slice=storage.meshes[mesh.ordinal];auto& out=set->slices[mesh.ordinal];
            // An element of the vertex layout (slot 0, or slot 1 the mesh's second stream): its first byte and stride.
            const auto element=[&](const char* semantic,uint32_t index)->const ds3content::ModelVertexElement*{if(!layout->vertex_elements_available)return nullptr;
                for(const auto& e:layout->vertex_elements)if(e.semantic_index==index && e.input_slot<2 && _stricmp(e.semantic.c_str(),semantic)==0)return &e;return nullptr;};
            const auto place=[&](const ds3content::ModelVertexElement& e,uint32_t bytes,uint32_t& first,uint32_t& stride){
                const uint64_t base=e.input_slot?slice.vertex_offset2:slice.vertex_offset;stride=e.input_slot?slice.stride2:slice.stride;
                const uint64_t at=base+e.byte_offset,last=at+uint64_t(slice.vertex_count?slice.vertex_count-1:0)*stride+bytes;
                first=static_cast<uint32_t>(at);return stride && slice.vertex_count && at%4==0 && stride%4==0 && last<=storage.bytes.size();};
            const auto* uv=element("TEXCOORD",0);
            // Every t0-alone DptA vertex programme of the census reads TEXCOORD0 as an int (UV16 at 1/2048): another format is not
            // that shape (final review L2: the float-UV family scales without the scroll row).
            const uint32_t format=!uv?UINT32_MAX:(uv->format==DXGI_FORMAT_R16G16_SINT || uv->format==DXGI_FORMAT_R16G16B16A16_SINT)?RtUvSint16:UINT32_MAX;
            uint32_t uv_first{},uv_stride{};
            // A colour-only slice reads no UV (the third review's L4).
            if((sampled && (format==UINT32_MAX || !place(*uv,4,uv_first,uv_stride))) ||
               uint64_t(slice.index_offset)+uint64_t(slice.index_count)*slice.index_width>storage.bytes.size()){++entry.unsupported;continue;}
            // COLOR1: an 8-bit UNORM or a float colour; its .w.
            uint32_t colour_format=RtAlphaColourNone,colour_first{},colour_stride{};
            if(colour){const auto* e=element("COLOR",1);
                colour_format=!e?UINT32_MAX:(e->format==DXGI_FORMAT_R8G8B8A8_UNORM || e->format==DXGI_FORMAT_B8G8R8A8_UNORM)?RtAlphaColourUnorm8:
                    e->format==DXGI_FORMAT_R32G32B32A32_FLOAT?RtAlphaColourFloat4:UINT32_MAX;
                if(colour_format==UINT32_MAX || !place(*e,colour_format==RtAlphaColourFloat4?16:4,colour_first,colour_stride)){++entry.unsupported;continue;}}
            // The diffuse texture: t0's binding (R3c: the register sampled), resolved through its holders as the raster's (not resident
            // yet, or its "_l" stand-in: tried again).
            std::shared_ptr<const bootstrap::NativeTextureRecord> texture;size_t chosen{};
            if(sampled)for(size_t index=0;index<programs->texture_bindings.size();++index){const auto& input=programs->texture_bindings[index];
                if(input.declaration.slot!=alpha_reg || input.path.empty() || !named_registry || programs->texture_lookups[index].facegen)continue;
                std::array<uintptr_t,4> inners{};texture=ResolveNativeTextureHolders(NativeTextureHolders(*programs,index,layout->resolver,named_registry),inners,&chosen);
                // R3c (the step review's L1): the raster's fallback as the material's (NativeRtFallbackTexture), its white one tried again.
                if(!texture || !texture->view){bool waits{};if(auto fallback=NativeRtFallbackTexture(programs->texture_lookups[index].leaf_folded,waits)){texture=std::move(fallback);chosen=waits?1:0;}}
                break;}
            if(sampled && (!texture || !texture->view)){++entry.unresolved;entry.pending=true;continue;}
            if(chosen)entry.pending=true;
            if(texture && (texture->description.cube || texture->description.slices!=1)){++entry.unsupported;continue;}   // the shader reads a Texture2D
            // The test samples t0 at UV0 tiled by FC_TextureTilingScale[0] (the ordinary register 1's row, no UV animation offset):
            // the material's diffuse when that is t0 too (the G-buffer's g_DiffuseTexture; R3a), else this texture with that row; a
            // material diffuse of another texture is not the shape (opaque, counted). R3c: another register only as the material's
            // diffuse, with the material's row.
            if(texture && out.textures[RtTextureDiffuse] && out.textures[RtTextureDiffuse]!=texture){++entry.unsupported;continue;}
            if(texture && !out.textures[RtTextureDiffuse]){std::array<float,4> tiling{1,1,0,0};ds3content::ModelTextureTiling rows;
                if(ds3content::ProduceModelTextureTiling(programs->texture_bindings,ds3content::ModelTextureTilingInputs{1,false,false,{}},rows) &&
                   std::all_of(rows.values.written.begin(),rows.values.written.begin()+4,[](uint8_t w){return w!=0;}))std::memcpy(tiling.data(),rows.values.bytes.data(),16);
                out.views[RtTextureDiffuse]=texture->view.handle;out.textures[RtTextureDiffuse]=texture;out.tiling[RtTextureDiffuse]=tiling;entry.textures.push_back(texture);}
            out.index_offset=slice.index_offset;out.index_width=slice.index_width;   // the triangle's data (none without a material read)
            out.position_offset=slice.vertex_offset;out.position_stride=slice.stride;
            if(sampled){out.uv_offset=uv_first;out.uv_stride=uv_stride;out.uv_format=format;}   // step review M1: a colour-only test keeps the material's UV0
            out.colour_offset=colour_first;out.colour_stride=colour_stride;out.colour_format=colour_format;
            out.ref=static_cast<float>((reference-0.0001)/255.0);out.alpha=true;out.alpha_texture=texture!=nullptr;any=true;}
        // T5: a mixed set's records pass slices over; R3a: a set as soon as one slice has a material; T6: or moves by the wind.
        if(any || any_material || any_wind || any_forward || entry.surface.mixed){set->buffer=storage.resource;set->non_opaque=any || entry.surface.mixed;set->wind=any_wind;
            set->forward=any_forward;   // I-0125 R6d
            set->skin=std::any_of(set->slices.begin(),set->slices.end(),[](const RtMaterialSlice& x){return x.skin || x.hair;});
            // I-0125 R6a: a lightmapped slice that can sample (its element read; the step review's L-2: no lightmaps read for the others).
            set->dol=std::any_of(set->slices.begin(),set->slices.end(),[](const RtMaterialSlice& x){return x.ambient==RtAmbientDol && x.lightmap_stride;});
            set->sh=std::any_of(set->slices.begin(),set->slices.end(),[](const RtMaterialSlice& x){return x.ambient==RtAmbientSh;});
            entry.set=std::move(set);}}
    if(!entry.surface.classes)entry.surface.known=false;   // no LOD-0 slice classed: the engine's masks
    surface=entry.surface;tally(entry);
    const std::lock_guard hold(shard.lock);if(native_rt_materials_closed.load())return nullptr;   // the close's (as the skinned entries')
    // Final review L4 (T5b: the walk's chunks on the pool): the first set made this walk for a key is kept, a second chunk's
    // equal work dropped, so every placement of the model object reads one set in a walk; T5c (the fixes' step review L1): only a
    // complete one (not deferred, no texture withdrawn since), else the new one replaces it.
    auto& kept=shard.map[key];
    const bool complete=!kept.deferred && std::none_of(kept.textures.begin(),kept.textures.end(),[](const auto& t){return t && t->withdrawn.load(std::memory_order_acquire);});
    if(kept.made==walk && kept.scene==scene && kept.storage_generation==storage.generation && complete){surface=kept.surface;return kept.set;}
    // I-0119 R3a: a set made again equal to the one it replaces (a stand-in still in place, the revalidation) keeps the former,
    // so its records and views stay in the material table.
    if(kept.set && entry.set && RtSameMaterials(*kept.set,*entry.set))entry.set=kept.set;
    kept=std::move(entry);return kept.set;
}
// T3 step review M3: at a scene reset (a load) the rays' kept entries go, so the former zone's textures, buffers and skinned
// structures they hold are released with the loads' (the frames holding them keep them to completion).
// I-0132 R11e: the last walk's scene of this load (a skipped walk's frame traces it; step 1) and the loads so far (an arrival each:
// NativeRtArrivalStep holds its world until the scene is ready; step 3), both moved here.
std::mutex native_rt_last_scene_lock;std::shared_ptr<const NativeRtSceneInput> native_rt_last_scene;std::atomic<uint64_t> native_rt_arrivals{};
struct NativeRtWalkOutcome {const char* skipped{};bool reused{};uint64_t candidates{},instances{},skinned{},no_storage{},map{};double ms{};};   // I-0132 R11c: `map`, this walk's map pieces (0 on a reused or skipped walk)
void ResetNativeRtKept() noexcept {
    {const std::lock_guard hold(native_rt_last_scene_lock);native_rt_last_scene.reset();native_rt_arrivals.fetch_add(1,std::memory_order_relaxed);}   // one lock (the second step review's L-A)
    for(auto& shard:native_rt_materials){const std::lock_guard hold(shard.lock);shard.map.clear();}
    {const std::lock_guard hold(native_rt_skinned_lock);native_rt_skinned.clear();}
    {const std::lock_guard hold(native_rt_programs_failed_lock);native_rt_programs_failed.clear();}   // T5: the scene's records
    ClearNativeMenuRtCaches();   // I-0140 tranche 31 (its step review M2)
}
// `view_low`: the main view's own categories (its walk's low byte), for the view bit (T1 step review M2: not the walk's union).
const char* CollectNativeRtInstances(NativeSelectedScene& scene,uintptr_t heap,uint64_t mask,uint8_t view_low,uintptr_t culler,uintptr_t world,uintptr_t manager,const uintptr_t* table,NativeRtWalkCounts& counts) {
    alignas(16) std::array<uint8_t,0x300> forged{};if(!Read(culler,forged.data(),forged.size()))return "culler";
    const auto& pose=scene.camera.camera_pose;const float ahead[3]{pose[12]+5*pose[8],pose[13]+5*pose[9],pose[14]+5*pose[10]};
    int positive{},negative{};
    for(int i=0;i<6;++i){float q[4];std::memcpy(q,forged.data()+0x160+16*i,16);const float d=q[0]*ahead[0]+q[1]*ahead[1]+q[2]*ahead[2]+q[3];(d>=0?positive:negative)++;}
    if(positive!=6 && negative!=6)return "culler sign";
    const float s=positive==6?1.f:-1.f,r=1.0e6f;   // a box a thousand kilometres a side around the origin: every map holds in it
    const float planes[6][4]{{s,0,0,s*r},{-s,0,0,s*r},{0,s,0,s*r},{0,-s,0,s*r},{0,0,s,s*r},{0,0,-s,s*r}};
    std::memcpy(forged.data()+0x160,planes,sizeof planes);
    const float lo[4]{-r,-r,-r,1},hi[4]{r,r,r,1};std::memcpy(forged.data()+0x220,lo,12);std::memcpy(forged.data()+0x230,hi,12);
    // Kept between walks (T1 final review L3: no megabyte allocated and freed each frame); this thread's only (since I-0132 R11g the
    // collect's rays worker, or the engine thread when the walk runs inline: one walk at a time).
    // A thread_local named inside the pool's jobs below would be each worker's own (empty) one: the jobs reach them through these
    // references (i117-t5g: the workers' chunks saw no candidate, the instances fell to the engine thread's chunks alone).
    thread_local std::vector<NativeDeferredCandidate> kept_candidates;thread_local NativeWalkHistory history;
    auto& candidates=kept_candidates;candidates.clear();
    // The main view's diagnostics (the pop-in gates and margins) leave this walk alone (its traversers' rays_walk).
    // T5b (the walk's cost on the engine's thread: 2.8 ms at m45 in i117-t5c, 1.7 of them the serial traversal with every entity's
    // gates): the walk on the pool as the main walk's (WalkNativeLayers: each layer's traverser gathers, the entities' gates and
    // candidates made in chunks, joined in the walk's order: the same list as the serial walk's), its gates and a missing disp
    // service through the share; its failures (the layers unreadable, unbalanced bounds) thrown and skipping the walk.
    NativeRtWalkShare share;
    LARGE_INTEGER t0{},t1{};QueryPerformanceCounter(&t0);
    WalkNativeLayers(world,heap,mask,reinterpret_cast<uintptr_t>(forged.data()),reinterpret_cast<uint32_t(__fastcall*)(uintptr_t)>(image+0xDFD240)(manager),
        table,scene,candidates,history,false,&share);
    QueryPerformanceCounter(&t1);counts.traverse_ticks=uint64_t(t1.QuadPart-t0.QuadPart);   // T5: the traversal's share of the walk
    for(size_t g=0;g<counts.gates.size();++g)counts.gates[g]=share.gates[g].load(std::memory_order_relaxed);
    if(share.failed.load(std::memory_order_relaxed))return "disp service";   // the disp service missing (AdmitNativeSelectedObject): the walk skipped
    counts.candidates=candidates.size();
    auto input=std::make_shared<NativeRtSceneInput>();
    // T5b: the programme builds' budget, shared by the chunks below and the skinned candidates (the walk's only).
    NativeRtProgramsBudget budget;counts.budget=&budget;
    struct BudgetOff {NativeRtWalkCounts& counts;NativeRtProgramsBudget& budget;
        ~BudgetOff(){counts.programs_built=budget.built.load(std::memory_order_relaxed);counts.programs_ticks=budget.ticks.load(std::memory_order_relaxed);counts.budget=nullptr;}} const budget_off{counts,budget};
    // One structure read a model storage (the lock taken for each lookup, not across the loop: T1 step review L5).
    // T3: with its storage (its alpha set's buffer, slices and bytes).
    struct Stored {Microsoft::WRL::ComPtr<ID3D12Resource> structure;std::shared_ptr<NativeModelStorage> storage;};
    // The bits by the original's views (T1 final review H1; the main walk's rule, AdmitNativeModelCandidate): an entity's category
    // byte (+0x20) puts it in the main view through the 0x45 view (0x01, no suppress test) or the pass 5..8 view (0x02/0x04), the
    // sun's cascades (passes 5..8, 0x02), the local lights' faces (0x04) and the reflection (0x10); the suppress byte the probe
    // pick writes (item +0x71A, a draw entity's or a character's +0xA5D) counts for the passes 5..8 only. The same rule for an
    // object and a character (the main walk reads their +0x20 alike; T1 step review M-b, L-e).
    // T5: then by its slices' material classes (RtInstanceMask): the sky's pieces (a dome drawn without a depth write, transparent
    // clouds) leave the view's bit (the forward-only ones seen so far have the category byte 1: no caster's bit); counted, the first
    // 32 such models named once (rt-view.flag). I-0118 R5c: the casters (the sun's or the local lights') with a transparent slice
    // that casts (`translucent`; the candidates made, as the other tallies: a skinned duplicate that only joins its mask is not
    // counted, a placed one the wind moves is, before its join),
    // the first 32 such models named once (rt-view.flag).
    static const bool name_forward=ModuleFlag(L"rt-view.flag");
    // `made`: the candidate makes an instance (its counts); a duplicate only joins its mask (T5 step review L1). `tally`: the
    // counts of the chunk or of the walk that makes it (T5b).
    const auto mask_of=[&](const NativeDeferredCandidate& d,const RtSurfaceClasses& surface,bool made,NativeRtWalkCounts& tally){const auto& c=d.candidate;
        BYTE entity_suppress{};const bool suppressed=c.geometry_suppressed || (d.kind!=0 && Read(c.fade_owner+0xA5D,&entity_suppress,1) && entity_suppress);
        tally.suppressed+=suppressed;
        uint8_t category{};Read(c.fade_owner+0x20,&category,1);const uint8_t own=category&view_low;
        const uint8_t bits=static_cast<uint8_t>((((own&0x01) || ((own&0x06) && !suppressed))?RtMaskView:0)|(((category&0x02) && !suppressed)?RtMaskSun:0)|
            ((category&0x04)?RtMaskLocal:0)|((category&0x10)?RtMaskReflection:0));
        const uint8_t out=RtInstanceMask(bits,surface);
        if(!made)return out;
        if(!surface.known)++tally.unclassed;
        tally.mixed+=surface.mixed;tally.overlaid+=surface.mixed && surface.overlays;
        if((bits&RtMaskView) && !(out&RtMaskView)){++tally.forward_only;
            if(name_forward){static std::mutex named_lock;static std::set<uintptr_t> named;bool fresh{};
                {const std::lock_guard hold(named_lock);fresh=named.size()<32 && named.insert(c.model).second;}
                if(fresh){std::string name;for(const auto ch:NativeModelDataName(c.model))name.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');
                    char row[256]{};sprintf_s(row,"{\"event\":\"native_rt_forward_only\",\"model\":\"%.60s\",\"classes\":\"%X\",\"category\":\"%X\",\"bits\":\"%X\",\"mask\":\"%X\"}\n",
                        name.c_str(),surface.classes,category,bits,out);Log(row);}}}
        if((bits&(RtMaskSun|RtMaskLocal)) && !(out&(RtMaskSun|RtMaskLocal)))++tally.uncast;
        if(surface.translucent && (out&(RtMaskSun|RtMaskLocal))){++tally.translucent;
            if(name_forward){static std::mutex translucent_lock;static std::set<uintptr_t> translucent_named;bool fresh{};
                {const std::lock_guard hold(translucent_lock);fresh=translucent_named.size()<32 && translucent_named.insert(c.model).second;}
                if(fresh){std::string name;for(const auto ch:NativeModelDataName(c.model))name.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');
                    char row[256]{};sprintf_s(row,"{\"event\":\"native_rt_translucent_caster\",\"model\":\"%.60s\",\"classes\":\"%X\",\"category\":\"%X\",\"surfaces\":%s,\"mask\":\"%X\"}\n",
                        name.c_str(),surface.classes,category,surface.surfaces?"true":"false",out);Log(row);}}}
        return out;};
    const uint64_t walk=++native_rt_skinned_walk;
    // T5b: the static candidates (a map piece or an object of one placement: their storage's BLAS) made into instances on the pool
    // in chunks taken by ticket, each chunk with its own instances, counts and structures read, joined in the walk's order (their
    // ids then); the skinned ones after them (T5c: their firsts on the pool, the walk's order replayed on this thread).
    // T6: `winds`, the chunk's placed instances the wind moves (by model object and pose), each with its static instance (the
    // storage's BLAS at its placement: a second placement of the same model object and pose, which one entry cannot hold).
    struct Wind {NativeRtSkinKey key;RtSkinnedInstance item;RtInstance placed;};
    // I-0125 R6d: `forward`, the chunk's instances with a forward set (their index in `instances`) and their draw groups.
    struct Chunk {std::vector<RtInstance> instances;std::vector<std::pair<size_t,uintptr_t>> skinned;std::vector<Wind> winds;NativeRtWalkCounts counts;
        std::vector<std::pair<size_t,uintptr_t>> forward;};
    constexpr size_t per_chunk=64;const size_t chunks=(candidates.size()+per_chunk-1)/per_chunk;
    // Kept between walks as `candidates` (step review L3: not a megabyte allocated and freed each walk), cleared before use.
    // Final review L5: every kept chunk cleared (not only this walk's: an earlier walk that threw kept its instances' structures).
    thread_local std::vector<Chunk> kept_lists;auto& lists=kept_lists;if(lists.size()<chunks)lists.resize(chunks);
    for(auto& chunk:lists){chunk.instances.clear();chunk.skinned.clear();chunk.winds.clear();chunk.forward.clear();chunk.counts=NativeRtWalkCounts{};}
    // T5c (the fixes' step review L3): cleared on any exit too, so a walk that throws holds none of its instances' structures.
    struct ClearChunks {std::vector<Chunk>& lists;~ClearChunks(){for(auto& chunk:lists){chunk.instances.clear();chunk.skinned.clear();chunk.winds.clear();}}} const clear_chunks{lists};
    std::atomic<size_t> next{};std::atomic<uint32_t> workers{};const NativeViewFlags view;
    const std::function<void(size_t)> place=[&](size_t){const NativeViewFlags::Apply view_flags(view);std::unordered_map<uintptr_t,Stored> structures;bool took{};
        struct Took {bool& took;std::atomic<uint32_t>& workers;~Took(){if(took)workers.fetch_add(1,std::memory_order_relaxed);}} const took_one{took,workers};
        for(size_t k;(k=next.fetch_add(1,std::memory_order_relaxed))<chunks;){took=true;auto& chunk=lists[k];auto& tally=chunk.counts;tally.budget=&budget;
            chunk.instances.reserve(per_chunk);
            for(size_t i=k*per_chunk;i<candidates.size() && i<(k+1)*per_chunk;++i){const auto& d=candidates[i];const auto& c=d.candidate;
                uint32_t pose_rows{};const bool skinned=d.kind==1 || (Read(c.pose+0x70,&pose_rows,4) && pose_rows>1);
                if(!std::all_of(c.placement.begin(),c.placement.end(),[](float v){return std::isfinite(v);})){++tally.nonfinite;continue;}   // the raster's gate (final review M2)
                uintptr_t receiver{};if(!Read(c.model+0xE8,&receiver,8) || !receiver){++tally.no_storage;continue;}
                if(skinned){chunk.skinned.emplace_back(i,receiver);continue;}   // T2: its own positions and BLAS, below
                auto known=structures.find(receiver);
                if(known==structures.end()){Stored stored;
                    {const std::shared_lock hold(model_storage_lock);const auto found=model_storage.find(receiver);
                        if(found!=model_storage.end()){stored.storage=found->second;const auto& blas=found->second->blas;
                            if(blas && blas->state.load(std::memory_order_acquire)!=RtModelBlas::State::failed)stored.structure=blas->Structure();}}
                    if(!stored.storage){++tally.no_storage;continue;}
                    known=structures.emplace(receiver,std::move(stored)).first;}
                if(!known->second.structure){++tally.no_blas;continue;}
                RtInstance instance;instance.transform=c.placement;instance.blas=known->second.structure;
                RtSurfaceClasses surface;
                if((instance.materials=NativeRtMaterialsOf(c,*known->second.storage,walk,tally,surface))){tally.alpha_instances+=instance.materials->non_opaque;   // T3, R3a
                    if(!Read(c.group,instance.scroll.data(),64))instance.scroll=kRtScrollIdentity;
                    if(instance.materials->skin)NativeRtSkinColour(c,instance.skin);
                    NativeRtInstanceAmbient(c,*instance.materials,walk,scene.pass_row_scale,instance.ambient);}   // I-0125 R6a
                ++(d.kind==0?tally.map:tally.objects);
                // T6: a set the wind moves, while it blows: its own positions and BLAS, joined after the static instances.
                if(instance.materials && instance.materials->wind){
                    if(auto wind=NativeRtWindBlockOf(c)){RtSkinnedInstance item;
                        if(NativeRtWindedOf(c,*known->second.storage,*instance.materials,std::move(wind),walk,item)){
                            item.materials=instance.materials;item.scroll=instance.scroll;item.skin=instance.skin;item.ambient=instance.ambient;item.rest=instance.blas;++tally.wind_instances;
                            item.mask=instance.mask=mask_of(d,surface,true,tally);
                            chunk.winds.push_back({NativeRtSkinKey{c.model,c.pose},std::move(item),std::move(instance)});continue;}}
                    else ++tally.wind_calm;}
                instance.mask=mask_of(d,surface,true,tally);
                if(instance.materials && instance.materials->forward)chunk.forward.emplace_back(chunk.instances.size(),c.group);   // I-0125 R6d
                chunk.instances.push_back(std::move(instance));}
            tally.budget=nullptr;}};
    LARGE_INTEGER s0{},s1{};QueryPerformanceCounter(&s0);
    if(chunks<2)place(0);else AdmissionPool().Run(place);
    counts.static_workers=workers.load(std::memory_order_relaxed);   // T5c: how many threads made the static chunks (1: the pool was busy)
    {size_t total{};for(size_t k=0;k<chunks;++k)total+=lists[k].instances.size();input->instances.reserve(total);}
    scene.rt_forward_groups.clear();
    for(size_t k=0;k<chunks;++k){auto& chunk=lists[k];counts.Add(chunk.counts);const size_t base=input->instances.size();
        for(const auto& [at,group]:chunk.forward)scene.rt_forward_groups.emplace_back(static_cast<uint32_t>(base+at),group);   // I-0125 R6d
        for(auto& instance:chunk.instances){instance.id=static_cast<uint32_t>(input->instances.size());input->instances.push_back(std::move(instance));}}
    // T6: the placed instances the wind moves, in the walk's order, bit 23 in their ids as the skinned instances'. A model object met
    // again with the same pose and placement: one instance, its masks joined (as the skinned ones); with another placement (the step
    // review's L3) its static instance at that placement, in its rest pose (`wind_differing`).
    {std::unordered_map<NativeRtSkinKey,size_t,NativeRtSkinKeyHash> winds;
        for(size_t k=0;k<chunks;++k)for(auto& wind:lists[k].winds){const auto [at,fresh]=winds.try_emplace(wind.key,input->skinned.size());
            --counts.wind_instances;   // counted below once made
            if(!fresh && input->skinned[at->second].world==wind.item.world){input->skinned[at->second].mask|=wind.item.mask;++counts.wind_duplicates;continue;}
            if(!fresh){++counts.wind_differing;wind.placed.id=static_cast<uint32_t>(input->instances.size());input->instances.push_back(std::move(wind.placed));continue;}
            ++counts.wind_instances;wind.item.id=(1u<<23)|static_cast<uint32_t>(input->skinned.size());input->skinned.push_back(std::move(wind.item));}}
    // What a model object's candidate read (NativeRtSkinnedOf's inputs besides the model); the first candidate's with its placement,
    // its instance once made, and the inputs already found to stay out (T3 step review L3: such a duplicate joins no mask).
    struct Inputs {uintptr_t pose{},source_field{},instance_field{},capability_source{},group{},cloth_set{};
        bool Same(const NativeSelectionCandidate& c) const {return pose==c.pose && source_field==c.source_field && instance_field==c.instance_field &&
            capability_source==c.capability_source && group==c.group && cloth_set==c.cloth_set;}};
    struct Met {std::array<float,12> placement{};size_t index{SIZE_MAX};std::vector<Inputs> failed;RtSurfaceClasses surface;};
    QueryPerformanceCounter(&s1);counts.static_ticks=uint64_t(s1.QuadPart-s0.QuadPart);
    std::unordered_map<NativeRtSkinKey,Met,NativeRtSkinKeyHash> skinned_met;skinned_met.reserve(64);std::unordered_set<uintptr_t> models_met;models_met.reserve(64);
    // T5c (the skinned candidates, read one after another on the engine's thread: 0.35 ms at m45, 0.61 at the High Wall): the
    // first candidate of each model object and pose made on the pool, one job a model object taking its poses' firsts in the walk's
    // order (its poses share its alpha entry and its programme builds: step review M1), then the walk's order replayed on this
    // thread as before (the duplicates' masks joined, a failed first's later candidates with other inputs made here, the ids). A
    // candidate's made instance (or why not) and its counts.
    struct Made {size_t candidate{};std::shared_ptr<NativeModelStorage> storage;RtSkinnedInstance item;RtSurfaceClasses surface;bool failed{};NativeRtWalkCounts counts;};
    const auto make=[&](const NativeDeferredCandidate& d,const std::shared_ptr<NativeModelStorage>& storage,RtSkinnedInstance& item,RtSurfaceClasses& surface,NativeRtWalkCounts& tally){
        // T6: the set first, for the slices the wind moves (the wind block read when the set has some).
        const auto& c=d.candidate;item.materials=NativeRtMaterialsOf(c,*storage,walk,tally,surface);
        // (Its tallies count a candidate the gates below leave out: the final review's L1, recorded.)
        std::shared_ptr<const RtWindBlock> wind;if(item.materials && item.materials->wind)wind=NativeRtWindBlockOf(c);
        if(NativeRtSkinnedOf(c,storage,walk,item,tally,item.materials.get(),wind))return false;
        tally.held+=item.hold;tally.wind_instances+=item.wind!=nullptr;tally.wind_calm+=item.materials && item.materials->wind && !wind;
        if(item.materials){tally.alpha_instances+=item.materials->non_opaque;   // T3, R3a; an unreadable scroll row the identity (step review L3)
            if(!Read(c.group,item.scroll.data(),64))item.scroll=kRtScrollIdentity;
            if(item.materials->skin)NativeRtSkinColour(c,item.skin);
            NativeRtInstanceAmbient(c,*item.materials,walk,scene.pass_row_scale,item.ambient);}   // I-0125 R6a
        item.mask=mask_of(d,surface,true,tally);return true;};
    std::vector<Made> firsts;std::vector<std::shared_ptr<NativeModelStorage>> storages;std::vector<size_t> order;   // order: the skinned candidates in the walk's order
    {std::unordered_set<NativeRtSkinKey,NativeRtSkinKeyHash> keys;keys.reserve(64);
        for(size_t k=0;k<chunks;++k)for(const auto& [i,receiver]:lists[k].skinned){const auto& c=candidates[i].candidate;
            std::shared_ptr<NativeModelStorage> storage;
            {const std::shared_lock hold(model_storage_lock);const auto found=model_storage.find(receiver);
                if(found!=model_storage.end() && found->second->resource && found->second->rt_readable && !found->second->geometry_pending.load(std::memory_order_acquire))storage=found->second;}
            order.push_back(i);storages.push_back(storage);
            if(storage && keys.insert(NativeRtSkinKey{c.model,c.pose}).second){Made m;m.candidate=i;m.storage=storage;m.counts.budget=&budget;firsts.push_back(std::move(m));}}}
    {std::vector<std::vector<size_t>> groups;{std::unordered_map<uintptr_t,size_t> group_of;group_of.reserve(firsts.size());
            for(size_t f=0;f<firsts.size();++f){const auto [at,fresh]=group_of.try_emplace(candidates[firsts[f].candidate].candidate.model,groups.size());
                if(fresh)groups.emplace_back();groups[at->second].push_back(f);}}
        std::atomic<size_t> ticket{};
        const std::function<void(size_t)> skin=[&](size_t){const NativeViewFlags::Apply view_flags(view);
            for(size_t g;(g=ticket.fetch_add(1,std::memory_order_relaxed))<groups.size();)for(const size_t f:groups[g]){auto& m=firsts[f];
                m.failed=!make(candidates[m.candidate],m.storage,m.item,m.surface,m.counts);m.counts.budget=nullptr;}};
        if(groups.size()<8)skin(0);else AdmissionPool().Run(skin);}
    size_t next_first{};
    for(size_t o=0;o<order.size();++o){const size_t i=order[o];const auto& d=candidates[i];const auto& c=d.candidate;
        ++counts.skinned;++counts.skinned_kinds[d.kind<3?d.kind:2];   // T2: its own positions and BLAS
        const auto& storage=storages[o];
        if(!storage){++counts.no_storage;continue;}
        // A model object met again in this walk with the same pose: one geometry, kept once, its masks joined (step review M2,
        // final review L1); with another pose its own instance (T3). One whose first candidate stayed out is tried again when it
        // reads other inputs (T2 step review, carried).
        auto [met,first]=skinned_met.try_emplace(NativeRtSkinKey{c.model,c.pose});
        if(first && !models_met.insert(c.model).second)++counts.poses;
        const Inputs read{c.pose,c.source_field,c.instance_field,c.capability_source,c.group,c.cloth_set};
        RtSkinnedInstance item;bool made{};
        if(first){met->second.placement=c.placement;auto& m=firsts[next_first++];   // made on the pool, in this order
            counts.Add(m.counts);met->second.surface=m.surface;made=!m.failed;if(made)item=std::move(m.item);}
        else{++counts.duplicates;
            if(met->second.placement!=c.placement)++counts.duplicates_differing;
            if(std::any_of(met->second.failed.begin(),met->second.failed.end(),[&](const Inputs& f){return f.Same(c);}))continue;
            if(met->second.index!=SIZE_MAX){input->skinned[met->second.index].mask|=mask_of(d,met->second.surface,false,counts);continue;}
            made=make(d,storage,item,met->second.surface,counts);}   // the walk's counts carry the budget
        if(!made){met->second.failed.push_back(read);continue;}
        item.id=(1u<<23)|static_cast<uint32_t>(input->skinned.size());   // bit 23: a skinned instance (T4's colours)
        met->second.index=input->skinned.size();input->skinned.push_back(std::move(item));}
    {LARGE_INTEGER s2{};QueryPerformanceCounter(&s2);counts.skinned_ticks=uint64_t(s2.QuadPart-s1.QuadPart);}
    counts.instances=input->instances.size();counts.skinned_instances=input->skinned.size();
    // The skinned instances not reached by this walk nor the one before: dropped (the frames holding them keep them to completion).
    {const std::lock_guard hold(native_rt_skinned_lock);std::erase_if(native_rt_skinned,[&](const auto& entry){return entry.second.seen+1<walk;});}
    for(auto& shard:native_rt_materials){const std::lock_guard hold(shard.lock);std::erase_if(shard.map,[&](const auto& entry){return entry.second.seen+1<walk;});}   // T3
    {const std::lock_guard hold(native_rt_lightmaps_lock);std::erase_if(native_rt_lightmaps,[&](const auto& entry){return entry.second.seen+1<walk;});}   // I-0125 R6a
    {const std::lock_guard hold(native_rt_programs_failed_lock);   // T5 step review L2: the expired records and those of freed layouts
        std::erase_if(native_rt_programs_failed,[&](const auto& entry){return entry.second.until<=walk || entry.second.layout.expired();});}
    // The step review's M1: the raster's material samplers' anisotropy and mip bias (the bank's settings rows), the rays' level.
    if(scene.sampler_bank){const auto& d=scene.sampler_bank->descriptors[12];
        input->anisotropy=D3D12_DECODE_IS_ANISOTROPIC_FILTER(d.Filter)?(std::max)(d.MaxAnisotropy,1u):1u;input->texture_bias=d.MipLODBias;}
    // I-0125 R6a: the frame's dominant rows as ProducePerFrame blends them (cbPerFrame +0xC0: +0xC4 FC_DominantSharpnesOffset, +0xCC
    // FC_DominantColorScale, +0xD0 FC_DominantRgbScale), for the lightmapped ambient.
    if(scene.per_frame_rows){std::array<uint8_t,0x100> bytes{};std::array<uint8_t,0x40> written{};float blended[7]{};
        ds3prod::ProducePerFrame(0,scene.per_frame_rows->a.data(),scene.per_frame_rows->b.data(),scene.per_frame_rows->blend,bytes.data(),0x100,written.data());
        std::memcpy(blended,bytes.data()+0xC0,28);input->dominant={blended[4]*blended[3],blended[5]*blended[3],blended[6]*blended[3],blended[1]};}
    if(scene.simulation_time)input->time=scene.simulation_time->now;   // I-0125 R6d: FC_GlobalTime, as ProducePerFrame's (13b)
    if(!scene.rt_forward_groups.empty())scene.rt_scene_input=input;   // I-0125 R6d: its forward blocks made later in this collect
    scene.rt_scene=std::move(input);
    return nullptr;
}
// I-0140 tranche 31 (D-0303): a Player menu view's traced scene, what its self-shadow's ray reads: the view's own candidates (its
// character's parts, which the admission gathers beside the raster's: NativeSelectionVisitor::rays_also) made into the rays'
// instances as the main walk makes them (CollectNativeRtInstances: the storage's structure for a placed one, its own positions
// and BLAS for a skinned one, the material sets for the alpha test), in the menu's own caches (native_rt_menu_caches) at the menu's
// walk `walk`. Every instance casts (the original's casters are the view layer's items: MENUS-MAP 48) but a suppressed one (the
// main walk's test: the passes 5 to 8 skip it), and is in the view (its surfaces); no ambient, lightmap, wind nor skin colour (the
// shadow ray reads none), and a skinned candidate whose first try fails is not tried again for a later duplicate (the main walk's
// `failed` list: a view's parts are met once each; explicit policy). Null when no instance is made. The caller holds
// native_offscreen_collect_lock.
std::shared_ptr<const NativeRtSceneInput> CollectNativeMenuRtScene(const std::vector<NativeDeferredCandidate>& candidates,uint64_t walk,NativeRtWalkCounts& counts) {
    native_rt_menu_caches=true;struct Off {~Off(){native_rt_menu_caches=false;}} const off;
    NativeRtProgramsBudget budget;counts.budget=&budget;struct BudgetOff {NativeRtWalkCounts& counts;~BudgetOff(){counts.budget=nullptr;}} const budget_off{counts};
    auto input=std::make_shared<NativeRtSceneInput>();
    std::unordered_set<NativeRtSkinKey,NativeRtSkinKeyHash> skinned_met;
    for(const auto& d:candidates){const auto& c=d.candidate;++counts.candidates;
        if(!std::all_of(c.placement.begin(),c.placement.end(),[](float v){return std::isfinite(v);})){++counts.nonfinite;continue;}
        uintptr_t receiver{};if(!Read(c.model+0xE8,&receiver,8) || !receiver){++counts.no_storage;++counts.menu_no_storage[0];continue;}
        uint32_t pose_rows{};const bool skinned=d.kind==1 || (Read(c.pose+0x70,&pose_rows,4) && pose_rows>1);
        // The suppress test as the main walk's (a geometry's, a drawn entity's or character's +0xA5D: the passes 5 to 8 skip it).
        BYTE entity_suppress{};const bool suppressed=c.geometry_suppressed || (d.kind!=0 && Read(c.fade_owner+0xA5D,&entity_suppress,1) && entity_suppress);
        counts.suppressed+=suppressed;
        const uint8_t engine=static_cast<uint8_t>(RtMaskView|(suppressed?0:RtMaskSun));
        std::shared_ptr<NativeModelStorage> storage;Microsoft::WRL::ComPtr<ID3D12Resource> structure;
        {const std::shared_lock hold(model_storage_lock);const auto found=model_storage.find(receiver);
            if(found!=model_storage.end()){storage=found->second;
                if(!skinned && found->second->blas && found->second->blas->state.load(std::memory_order_acquire)!=RtModelBlas::State::failed)structure=found->second->blas->Structure();
                if(skinned && (!found->second->resource || !found->second->rt_readable || found->second->geometry_pending.load(std::memory_order_acquire))){
                    ++counts.menu_no_storage[found->second->geometry_pending.load(std::memory_order_acquire)?3:2];storage.reset();}}
            else ++counts.menu_no_storage[1];}
        if(!storage){++counts.no_storage;continue;}
        RtSurfaceClasses surface;
        if(!skinned){if(!structure){++counts.no_blas;continue;}
            RtInstance instance;instance.transform=c.placement;instance.blas=std::move(structure);
            if((instance.materials=NativeRtMaterialsOf(c,*storage,walk,counts,surface))){counts.alpha_instances+=instance.materials->non_opaque;
                if(!Read(c.group,instance.scroll.data(),64))instance.scroll=kRtScrollIdentity;}
            instance.mask=RtInstanceMask(engine,surface);instance.id=static_cast<uint32_t>(input->instances.size());++counts.objects;
            input->instances.push_back(std::move(instance));continue;}
        ++counts.skinned;
        // A model object and pose met again: one geometry (the main walk joins the masks; here every one casts alike).
        if(!skinned_met.insert(NativeRtSkinKey{c.model,c.pose}).second){++counts.duplicates;continue;}
        RtSkinnedInstance item;item.materials=NativeRtMaterialsOf(c,*storage,walk,counts,surface);
        if(NativeRtSkinnedOf(c,storage,walk,item,counts,item.materials.get(),nullptr))continue;
        counts.held+=item.hold;
        if(item.materials){counts.alpha_instances+=item.materials->non_opaque;if(!Read(c.group,item.scroll.data(),64))item.scroll=kRtScrollIdentity;}
        item.mask=RtInstanceMask(engine,surface);item.id=(1u<<23)|static_cast<uint32_t>(input->skinned.size());
        input->skinned.push_back(std::move(item));}
    counts.instances=input->instances.size();counts.skinned_instances=input->skinned.size();
    if(input->instances.empty() && input->skinned.empty())return nullptr;
    return input;
}
// I-0140 tranche 31: after a menu collect's views, the menu's entries no view of this walk nor the one before named: dropped (the
// frames holding them keep them to completion), as the main walk drops its own.
void EvictNativeMenuRtCaches(uint64_t walk) {
    {const std::lock_guard hold(native_rt_menu_skinned_lock);std::erase_if(native_rt_menu_skinned,[&](const auto& entry){return entry.second.seen+1<walk;});}
    for(auto& shard:native_rt_menu_materials){const std::lock_guard hold(shard.lock);std::erase_if(shard.map,[&](const auto& entry){return entry.second.seen+1<walk;});}
    {const std::lock_guard hold(native_rt_programs_failed_lock);   // the step review's M1: its records as the main walk's (T5 step review L2)
        std::erase_if(native_rt_menu_programs_failed,[&](const auto& entry){return entry.second.until<=walk || entry.second.layout.expired();});}
}
// I-0117 (R2 T1): the main view's collect (one walk at a time: since I-0132 R11g on the collect's rays worker, the engine thread when
// inline; never an off-screen view) puts
// the loaded world's instances in the scene for the rays (the rays' scene); every 600th collect their counts and the walk's cost
// (the mean and the largest since the last row, CPU ms on that thread). An exception skips this walk (T1 final review L2; I-0132
// R11e: its frame traces the last scene), its text bounded and escaped in the row (T1's last step review); once the top level failed, no walk (L4).
// I-0132 R11e step 1 (D-0262, no fallback): a walk that skips or throws hands its frame the last walk's scene of this load (kept in
// native_rt_last_scene, dropped at a scene reset by ResetNativeRtKept), counted (`reused`); only a failed top level leaves a frame
// without a scene, and a run of reuses stops after 2 s (native_rt_reuse_stopped: the frames unserved, the fatal rule then). Returns what
// the arrival's rule reads (NativeRtArrivalStep).
NativeRtWalkOutcome CollectNativeRtScene(NativeSelectedScene& scene,uintptr_t heap,uint64_t mask,uintptr_t culler,uintptr_t world,uintptr_t manager,const uintptr_t* table) {
    LARGE_INTEGER t0{},t1{},f{};QueryPerformanceCounter(&t0);QueryPerformanceFrequency(&f);
    NativeRtWalkCounts counts;const char* skipped="top level failed";
    const uint64_t load=native_rt_arrivals.load(std::memory_order_relaxed);   // R11e's step review L4: a reset during the walk
    thread_local std::string why;   // an exception's text, kept for the row below
    if(!renderer.RtSceneFailed())try{skipped=CollectNativeRtInstances(scene,heap,mask|0x02u|0x04u|0x10u,static_cast<uint8_t>(mask&0xFF),culler,world,manager,table,counts);}
        catch(const std::exception& error){why="exception: ";for(const char* c=error.what();c && *c && why.size()<160;++c)why.push_back(*c=='"' || *c=='\\' || *c<32 || *c>126?'?':*c);
            skipped=why.c_str();scene.rt_scene.reset();}
        catch(...){skipped="exception";scene.rt_scene.reset();}
    // The final review's L3: a reuse lasting 2 s (every walk skipping) stops: the frames then have no scene (unserved, cause 2), so the
    // fatal rule applies instead of a stale scene traced for ever; one row when it stops.
    bool reused=false;static uint64_t reused_total{},reuse_since{};
    {const std::lock_guard hold(native_rt_last_scene_lock);
        if(native_rt_arrivals.load(std::memory_order_relaxed)!=load){}   // a scene reset during the walk: its scene neither kept nor replaced
        else if(!skipped && scene.rt_scene){native_rt_last_scene=scene.rt_scene;reuse_since=0;}
        else if(!renderer.RtSceneFailed() && native_rt_last_scene){const uint64_t tick=GetTickCount64();if(!reuse_since)reuse_since=tick;
            if(tick-reuse_since<2000){scene.rt_scene=native_rt_last_scene;scene.rt_scene_input.reset();scene.rt_forward_groups.clear();reused=true;++reused_total;}
            else{native_rt_last_scene.reset();reuse_since=0;Log("{\"event\":\"native_rt_reuse_stopped\",\"after_ms\":2000}\n");}}}
    QueryPerformanceCounter(&t1);const double ms=1000.0*double(t1.QuadPart-t0.QuadPart)/double(f.QuadPart);
    static double ms_sum{},ms_max{},traverse_sum{},programs_sum{},programs_max{},static_sum{},skinned_sum{};static LONG64 walks{},since{},skips{},built_sum{};ms_sum+=ms;traverse_sum+=1000.0*double(counts.traverse_ticks)/double(f.QuadPart);
    static_sum+=1000.0*double(counts.static_ticks)/double(f.QuadPart);skinned_sum+=1000.0*double(counts.skinned_ticks)/double(f.QuadPart);   // T5b
    // T5 final review L3: the programme builds since the last row (their number, their time summed and the largest a walk).
    {const double programs_ms=1000.0*double(counts.programs_ticks)/double(f.QuadPart);programs_sum+=programs_ms;programs_max=(std::max)(programs_max,programs_ms);built_sum+=counts.programs_built;}ms_max=(std::max)(ms_max,ms);++since;const auto n=++walks;
    const bool skip_row=skipped && (++skips<=8 || skips%600==0);   // T1 step review L4: a persistent skip rate-limited
    // I-0118 (the cascades removal's last step review M1): the skipped walks since the last row and the longest run reaching into
    // that interval (counted from its start); a failed top level counts too. I-0132 R11e: a skipped walk's frame traces the last
    // scene (`reused`, since the start); the former rule (the 30th skip in a row stopping the traced sun and lights) is removed.
    static uint32_t skips_since{},run{},run_max{};if(skipped){++skips_since;run_max=(std::max)(run_max,++run);}else run=0;
    // I-0132 R11d's final review M2 (its fixes' step review H1: on every walk, outside the sampled rows): the main view's frames the
    // rays did not serve outside an arrival's hold (R11e: magenta under rt-fail-magenta.flag, their world black otherwise) as soon as
    // their count changes, at most 32 rows (the walks' sampled rows miss a one-minute start check): their count and the OR of their
    // causes (1 sun, 2 top level, 4 GI, 8 lights), with the frames held black at the arrivals so far (R11e step 3).
    // I-0132 R11h (on by default, D-0264): the automatic exposure's reading (eight frames late) every 120 of its frames, and its failure once.
    {static uint64_t exposure_rows{};static bool exposure_failed{};const uint64_t frames=renderer.AutoExposureFrames();
        if(frames/120!=exposure_rows){exposure_rows=frames/120;const auto r=renderer.AutoExposure();char exposure_row[320]{};
            sprintf_s(exposure_row,"{\"event\":\"native_auto_exposure\",\"walk\":%lld,\"frames\":%llu,\"valid\":%s,\"log2_factor\":%.4f,\"goal_log2\":%.4f,\"mean\":%.4f,\"target\":%.4f,\"area_exposure\":%.4g,\"pixels\":%u}\n",
                static_cast<long long>(n),static_cast<unsigned long long>(frames),r.valid?"true":"false",r.log2_factor,r.goal_log2,r.mean,r.target,r.area_exposure,r.pixels);Log(exposure_row);}
        if(!exposure_failed){std::string failure=renderer.AutoExposureFailure();if(!failure.empty()){exposure_failed=true;for(auto& ch:failure)if(ch=='"' || ch=='\\' || ch<32 || ch>126)ch='?';
            char failure_row[320]{};sprintf_s(failure_row,"{\"event\":\"native_auto_exposure_failed\",\"walk\":%lld,\"why\":\"%.200s\"}\n",static_cast<long long>(n),failure.c_str());Log(failure_row);}}}
    {static uint64_t magenta_seen{};static int magenta_rows{};const uint64_t magenta=renderer.RtFailMagentaFrames(),held=renderer.RtHeldFrames();
        if(magenta!=magenta_seen && magenta_rows<32){magenta_seen=magenta;++magenta_rows;char magenta_row[200]{};
            sprintf_s(magenta_row,"{\"event\":\"native_rt_fail_magenta\",\"walk\":%lld,\"frames\":%llu,\"causes\":%u,\"held\":%llu}\n",static_cast<long long>(n),static_cast<unsigned long long>(magenta),renderer.RtFailMagentaCauses(),static_cast<unsigned long long>(held));Log(magenta_row);}}
    if(n<=3 || n%600==0 || skip_row){std::string gates;for(size_t g=0;g<counts.gates.size();++g)if(counts.gates[g]){char cell[24]{};sprintf_s(cell,"%s\"%zu\":%u",gates.empty()?"":",",g,counts.gates[g]);gates+=cell;}
        char row[3072]{};sprintf_s(row,"{\"event\":\"native_rt_walk\",\"call\":%lld,\"skipped\":\"%.200s\",\"candidates\":%llu,\"instances\":%llu,\"map\":%llu,\"objects\":%llu,\"skinned\":%llu,\"skinned_kinds\":[%llu,%llu,%llu],\"suppressed\":%llu,\"no_storage\":%llu,\"no_blas\":%llu,\"nonfinite\":%llu,\"forward_only\":%llu,\"uncast\":%llu,\"unclassed\":%llu,\"mixed\":%llu,\"overlaid\":%llu,\"translucent\":%llu,\"programs_built\":%lld,\"programs_ms\":%.3f,\"programs_ms_max\":%.3f,\"gates\":{%.1100s},\"ms\":%.3f,\"ms_mean\":%.3f,\"ms_max\":%.3f,\"traverse_ms_mean\":%.3f,\"static_ms_mean\":%.3f,\"skinned_ms_mean\":%.3f,\"static_workers\":%u,\"skips\":%u,\"skip_run_max\":%u,\"reused\":%llu}\n",
            static_cast<long long>(n),skipped?skipped:"",static_cast<unsigned long long>(counts.candidates),static_cast<unsigned long long>(counts.instances),static_cast<unsigned long long>(counts.map),
            static_cast<unsigned long long>(counts.objects),static_cast<unsigned long long>(counts.skinned),static_cast<unsigned long long>(counts.skinned_kinds[0]),static_cast<unsigned long long>(counts.skinned_kinds[1]),
            static_cast<unsigned long long>(counts.skinned_kinds[2]),static_cast<unsigned long long>(counts.suppressed),static_cast<unsigned long long>(counts.no_storage),static_cast<unsigned long long>(counts.no_blas),
            static_cast<unsigned long long>(counts.nonfinite),static_cast<unsigned long long>(counts.forward_only),static_cast<unsigned long long>(counts.uncast),
            static_cast<unsigned long long>(counts.unclassed),static_cast<unsigned long long>(counts.mixed),static_cast<unsigned long long>(counts.overlaid),static_cast<unsigned long long>(counts.translucent),static_cast<long long>(built_sum),
            programs_sum,programs_max,gates.c_str(),ms,ms_sum/double(since),ms_max,traverse_sum/double(since),static_sum/double(since),skinned_sum/double(since),counts.static_workers,skips_since,run_max,static_cast<unsigned long long>(reused_total));Log(row);if(n>3){skips_since=0;run_max=0;ms_sum=0;ms_max=0;traverse_sum=0;static_sum=0;skinned_sum=0;programs_sum=0;programs_max=0;built_sum=0;since=0;}
        // T2: the walk's skinned instances and slices; the renderer's skinning (its last frame's instances and left out, since the
        // start its builds, refits and skinned vertices, the positions, structures and scratch it holds).
        // T3: the walk's alpha instances, alpha-tested slices, not resident, not read, sets made.
        // T6: the walk's instances moved by the wind, its calm ones, the wind slices, those not read, the placed ones met again at
        // their placement and at another (`wind`); the renderer's instances skinned with their wind and their vertices since the start.
        const auto t=renderer.RtSkinTotals();char skin[1536]{};
        sprintf_s(skin,"{\"event\":\"native_rt_skin\",\"call\":%lld,\"instances\":%llu,\"slices\":%llu,\"excluded\":%llu,\"unsupported\":%llu,\"no_pose\":%llu,\"no_slice\":%llu,\"no_inputs\":%llu,\"streamed\":%llu,\"cloth\":%llu,\"cloth_framed\":%llu,\"cloth_frame_missing\":%llu,\"cloth_left_out\":%llu,\"duplicates\":%llu,\"duplicates_differing\":%llu,\"poses\":%llu,\"held\":%llu,\"alpha\":[%llu,%llu,%llu,%llu,%llu,%llu,%llu],\"kept\":%zu,\"traced\":%llu,\"left_out\":%llu,\"builds\":%llu,\"refits\":%llu,\"vertices\":%llu,\"mb\":%.1f,\"wind\":[%llu,%llu,%llu,%llu,%llu,%llu],\"winds\":%llu,\"wind_vertices\":%llu}\n",
            static_cast<long long>(n),static_cast<unsigned long long>(counts.skinned_instances),static_cast<unsigned long long>(counts.skinned_slices),static_cast<unsigned long long>(counts.excluded_slices),
            static_cast<unsigned long long>(counts.unsupported_slices),static_cast<unsigned long long>(counts.no_pose),static_cast<unsigned long long>(counts.no_slice),static_cast<unsigned long long>(counts.no_inputs),
            static_cast<unsigned long long>(counts.streamed),static_cast<unsigned long long>(counts.cloth),static_cast<unsigned long long>(counts.cloth_framed),static_cast<unsigned long long>(counts.cloth_frame_missing),static_cast<unsigned long long>(counts.cloth_left_out),static_cast<unsigned long long>(counts.duplicates),static_cast<unsigned long long>(counts.duplicates_differing),static_cast<unsigned long long>(counts.poses),static_cast<unsigned long long>(counts.held),
            static_cast<unsigned long long>(counts.alpha_instances),static_cast<unsigned long long>(counts.alpha_slices),static_cast<unsigned long long>(counts.alpha_unresolved),
            static_cast<unsigned long long>(counts.alpha_unsupported),static_cast<unsigned long long>(counts.alpha_made),static_cast<unsigned long long>(counts.alpha_untextured),static_cast<unsigned long long>(counts.alpha_unknown),
            []{const std::lock_guard hold(native_rt_skinned_lock);return native_rt_skinned.size();}(),static_cast<unsigned long long>(t.instances),
            static_cast<unsigned long long>(t.failed),static_cast<unsigned long long>(t.builds),static_cast<unsigned long long>(t.refits),static_cast<unsigned long long>(t.vertices),double(t.bytes)/1048576.0,
            static_cast<unsigned long long>(counts.wind_instances),static_cast<unsigned long long>(counts.wind_calm),static_cast<unsigned long long>(counts.wind_slices),
            static_cast<unsigned long long>(counts.wind_unsupported),static_cast<unsigned long long>(counts.wind_duplicates),static_cast<unsigned long long>(counts.wind_differing),static_cast<unsigned long long>(t.winds),static_cast<unsigned long long>(t.wind_vertices));Log(skin);
        // I-0119 R3a: the walk's slices given a material, their textures by role (diffuse, specular, bump, emissive) named by the
        // G-buffer programme and resolved, those not resident, the slices without a G-buffer programme, a UV0 or a vertex frame read;
        // the renderer's material table (sets, records and views alive, the records written and the views refused since the start,
        // the table's records), the families named so far (with rt-view.flag) and the table's time on the recording thread (the last
        // call's and the mean, ms). I-0125 R6d: the forward slices met and of them with a record (after the blends, the map blends').
        {const auto m=renderer.RtMaterialTotals();size_t families{};{const std::lock_guard hold(native_rt_family_lock);families=native_rt_family_names.size();}
            const auto four=[](const std::array<uint64_t,4>& a){char out[96]{};sprintf_s(out,"[%llu,%llu,%llu,%llu]",static_cast<unsigned long long>(a[0]),static_cast<unsigned long long>(a[1]),
                static_cast<unsigned long long>(a[2]),static_cast<unsigned long long>(a[3]));return std::string(out);};
            char material_row[1024]{};sprintf_s(material_row,"{\"event\":\"native_rt_materials\",\"call\":%lld,\"slices\":%llu,\"named\":%s,\"resolved\":%s,\"pending\":%llu,\"no_gbuf\":%llu,\"no_uv\":%llu,\"no_frame\":%llu,\"blends\":%llu,\"no_weights\":%llu,\"forward\":%llu,\"forward_made\":%llu,\"emissive\":%llu,\"sets\":%llu,\"records\":%llu,\"buffers\":%llu,\"textures\":%llu,\"written\":%llu,\"full\":%llu,\"capacity\":%llu,\"families\":%zu,\"encode_ms\":%.3f,\"encode_ms_mean\":%.3f}\n",
                static_cast<long long>(n),static_cast<unsigned long long>(counts.material_slices),four(counts.material_named).c_str(),four(counts.material_resolved).c_str(),
                static_cast<unsigned long long>(counts.material_pending),static_cast<unsigned long long>(counts.material_no_gbuf),static_cast<unsigned long long>(counts.material_no_uv),
                static_cast<unsigned long long>(counts.material_no_frame),static_cast<unsigned long long>(counts.material_blends),
                static_cast<unsigned long long>(counts.material_no_weights),static_cast<unsigned long long>(counts.material_forward),
                static_cast<unsigned long long>(counts.material_forward_made),static_cast<unsigned long long>(counts.material_emissive),static_cast<unsigned long long>(m.sets),static_cast<unsigned long long>(m.records),static_cast<unsigned long long>(m.buffers),
                static_cast<unsigned long long>(m.textures),static_cast<unsigned long long>(m.written),static_cast<unsigned long long>(m.full),static_cast<unsigned long long>(m.capacity),families,m.encode_ms_last,m.encode_ms_mean);Log(material_row);}
        // T4 (rt-view.flag): the control view's samples: how many, the last one's frame and pixels by class (agree, close, nearer,
        // farther, missed, sky, extra; T5: then the nearer on a clockwise triangle, the disagreeing met by an alpha test), the same summed over the samples, and the agreeing share of the raster's surfaces; once it
        // failed, off and why.
        // I-0118 R5a (on with the rays; step review L1): whether the traced sun shadow still runs, and why not once it failed.
        static const bool rt_shadow_asked=renderer.rt_shadow.load(std::memory_order_relaxed);
        if(rt_shadow_asked){std::string failure=renderer.RtSunFailure();for(auto& ch:failure)if(ch=='"' || ch=='\\' || ch<32 || ch>126)ch='?';
            char shadow_row[256]{};sprintf_s(shadow_row,"{\"event\":\"native_rt_shadow\",\"call\":%lld,\"on\":%s,\"why\":\"%.160s\"}\n",static_cast<long long>(n),failure.empty()?"true":"false",failure.c_str());Log(shadow_row);}
        // I-0122 R4a (on with the rays): whether the native local lights still run, why not once they failed, and the frames whose
        // list was not drawn (their batches drawn instead); R4c: the frames shaded at the PT level.
        static const bool rt_lights_asked=renderer.rt_lights.load(std::memory_order_relaxed);
        if(rt_lights_asked){std::string failure=renderer.RtLightsFailure();for(auto& ch:failure)if(ch=='"' || ch=='\\' || ch<32 || ch>126)ch='?';
            char lights_row[352]{};sprintf_s(lights_row,"{\"event\":\"native_rt_lights\",\"call\":%lld,\"on\":%s,\"why\":\"%.160s\",\"missed\":%llu,\"pt_frames\":%llu,\"pt_refused\":%s}\n",static_cast<long long>(n),
                failure.empty()?"true":"false",failure.c_str(),static_cast<unsigned long long>(renderer.RtLightsMissed()),static_cast<unsigned long long>(renderer.RtPtFrames()),renderer.RtPtRefused()?"true":"false");Log(lights_row);}
        // I-0129 R9a (on with the rays): whether the traced probes still run, why not once they failed, their frames, the states held
        // and the deliveries so far.
        static const bool rt_probes_asked=renderer.rt_probes.load(std::memory_order_relaxed);
        if(rt_probes_asked && !renderer.rt_probes.load(std::memory_order_relaxed))effect_probe::on=false;   // the step review's L2: failed, the effects' detour idle
        if(rt_probes_asked){std::string failure=renderer.RtProbeFailure();for(auto& ch:failure)if(ch=='"' || ch=='\\' || ch<32 || ch>126)ch='?';
            char probes_row[320]{};sprintf_s(probes_row,"{\"event\":\"native_rt_probes\",\"call\":%lld,\"on\":%s,\"why\":\"%.160s\",\"frames\":%llu,\"states\":%zu,\"deliveries\":%llu}\n",static_cast<long long>(n),
                failure.empty()?"true":"false",failure.c_str(),static_cast<unsigned long long>(renderer.RtProbeFrames()),native_rt_probe_states->Size(),static_cast<unsigned long long>(native_rt_probe_states->Deliveries()));Log(probes_row);}
        // I-0125 R6b (on with the rays): whether the traced reflections still run, why not once they failed, the frames they drew.
        static const bool rt_reflect_asked=renderer.rt_reflect.load(std::memory_order_relaxed);
        if(rt_reflect_asked){std::string failure=renderer.RtReflectFailure();for(auto& ch:failure)if(ch=='"' || ch=='\\' || ch<32 || ch>126)ch='?';
            char reflect_row[300]{};sprintf_s(reflect_row,"{\"event\":\"native_rt_reflect\",\"call\":%lld,\"on\":%s,\"why\":\"%.160s\",\"frames\":%llu,\"missed\":%llu}\n",static_cast<long long>(n),
                failure.empty()?"true":"false",failure.c_str(),static_cast<unsigned long long>(renderer.RtReflectFrames()),static_cast<unsigned long long>(renderer.RtReflectMissed()));Log(reflect_row);}
        // I-0132 R11b (the PT level asked): whether SER still runs, why not once it could not be made, its dispatches (up to two a frame).
        const bool rt_ser_asked=renderer.rt_ser.load(std::memory_order_relaxed) && renderer.rt_pt.load(std::memory_order_relaxed);   // I-0132 R12f: the level live (the step review's L6)
        if(rt_ser_asked){std::string failure=renderer.RtSerFailure();for(auto& ch:failure)if(ch=='"' || ch=='\\' || ch<32 || ch>126)ch='?';
            char ser_row[300]{};sprintf_s(ser_row,"{\"event\":\"native_rt_ser\",\"call\":%lld,\"on\":%s,\"why\":\"%.160s\",\"dispatches\":%llu,\"make_ms\":%.1f}\n",static_cast<long long>(n),
                failure.empty()?"true":"false",failure.c_str(),static_cast<unsigned long long>(renderer.RtSerDispatches()),
                renderer.RtSerMakeMicroseconds()<0?-1.0:double(renderer.RtSerMakeMicroseconds())/1000.0);Log(ser_row);}   // -1: not made (the step review's M1)
        // I-0125 R6c (on with the rays and the water's reflection): whether the traced water still runs, why not once it failed, the
        // frames it drew, the collects that published a plane for it.
        static const bool rt_water_asked=renderer.rt_water.load(std::memory_order_relaxed);
        if(rt_water_asked){std::string failure=renderer.RtWaterFailure();for(auto& ch:failure)if(ch=='"' || ch=='\\' || ch<32 || ch>126)ch='?';
            char water_row[300]{};sprintf_s(water_row,"{\"event\":\"native_rt_water\",\"call\":%lld,\"on\":%s,\"why\":\"%.160s\",\"frames\":%llu,\"planes\":%llu}\n",static_cast<long long>(n),
                failure.empty()?"true":"false",failure.c_str(),static_cast<unsigned long long>(renderer.RtWaterFrames()),static_cast<unsigned long long>(native_rt_water_planes.load(std::memory_order_relaxed)));Log(water_row);}
        // I-0126 R7a (on with the rays): whether the traced GI still runs, why not once it failed, its frames and those without a top level.
        static const bool rt_gi_asked=renderer.rt_gi.load(std::memory_order_relaxed);
        if(rt_gi_asked){std::string failure=renderer.RtGiFailure();for(auto& ch:failure)if(ch=='"' || ch=='\\' || ch<32 || ch>126)ch='?';
            // I-0126 R7b: the cache's frames and, once it failed, why.
            std::string cache_failure=renderer.RtCacheFailure();for(auto& ch:cache_failure)if(ch=='"' || ch=='\\' || ch<32 || ch>126)ch='?';
            // I-0126 R7c: the GI frames NRD denoised (RR off) and, once NRD failed, why.
            std::string nrd_failure=renderer.RtNrdFailure();for(auto& ch:nrd_failure)if(ch=='"' || ch=='\\' || ch<32 || ch>126)ch='?';
            // I-0126 R7d: ReSTIR GI's frames (the PT level) and, once it failed, why.
            std::string restir_failure=renderer.RtRestirFailure();for(auto& ch:restir_failure)if(ch=='"' || ch=='\\' || ch<32 || ch>126)ch='?';
            char gi_row[1280]{};sprintf_s(gi_row,"{\"event\":\"native_rt_gi\",\"call\":%lld,\"on\":%s,\"why\":\"%.160s\",\"frames\":%llu,\"missed\":%llu,\"cache_frames\":%llu,\"cache_why\":\"%.160s\",\"nrd_frames\":%llu,\"nrd_missed\":%llu,\"nrd_why\":\"%.160s\",\"restir_frames\":%llu,\"restir_why\":\"%.160s\"}\n",static_cast<long long>(n),
                failure.empty()?"true":"false",failure.c_str(),static_cast<unsigned long long>(renderer.RtGiFrames()),static_cast<unsigned long long>(renderer.RtGiMissed()),
                static_cast<unsigned long long>(renderer.RtCacheFrames()),cache_failure.c_str(),static_cast<unsigned long long>(renderer.RtNrdFrames()),static_cast<unsigned long long>(renderer.RtNrdMissed()),nrd_failure.c_str(),
                static_cast<unsigned long long>(renderer.RtRestirFrames()),restir_failure.c_str());Log(gi_row);}
        static const bool rt_view_asked=renderer.rt_view.load(std::memory_order_relaxed);   // set at the start, before the first walk
        if(rt_view_asked){std::string failure=renderer.RtViewFailure();const auto v=renderer.RtViewSamples();
            for(auto& ch:failure)if(ch=='"' || ch=='\\' || ch<32 || ch>126)ch='?';const auto share=[](const RtViewCounts& c){return c.Surfaces()?double(c.agree)/double(c.Surfaces()):0.0;};
            const auto cells=[](const RtViewCounts& c){char out[256]{};sprintf_s(out,"[%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu]",static_cast<unsigned long long>(c.agree),static_cast<unsigned long long>(c.close),
                static_cast<unsigned long long>(c.nearer),static_cast<unsigned long long>(c.farther),static_cast<unsigned long long>(c.missed),static_cast<unsigned long long>(c.sky),static_cast<unsigned long long>(c.extra),static_cast<unsigned long long>(c.nearer_cw),static_cast<unsigned long long>(c.alpha_met));return std::string(out);};
            char view[1024]{};sprintf_s(view,"{\"event\":\"native_rt_view\",\"call\":%lld,\"on\":%s,\"why\":\"%.160s\",\"samples\":%llu,\"serial\":%llu,\"last\":%s,\"sum\":%s,\"agree_last\":%.4f,\"agree_sum\":%.4f}\n",
                static_cast<long long>(n),failure.empty()?"true":"false",failure.c_str(),static_cast<unsigned long long>(v.samples),static_cast<unsigned long long>(v.serial),cells(v.last).c_str(),cells(v.sum).c_str(),share(v.last),share(v.sum));Log(view);
            // I-0119 R3b: each programme family's comparison with the G-buffer summed over the samples (the agreeing pixels with a
            // material): its pixels, the shares within tolerance (albedo, F0, gloss, normal) and the mean errors (albedo and gloss in
            // 8-bit steps, the normal in degrees); the families with pixels only.
            for(uint32_t k=0;k<kRtViewFamilies;++k){const auto& check_of=v.families[k];if(!check_of.pixels)continue;
                std::string family=NativeRtFamilyName(k);for(auto& ch:family)if(ch=='"' || ch=='\\' || ch<32 || ch>126)ch='?';const double p=double(check_of.pixels);
                // R3c: the mean signed errors (traced less raster: albedo by channel, gloss; a bias apart from the spread) and the
                // share of pixels whose second-layer weight is at least one half with their mean albedo error.
                const double layered=double(check_of.layered);
                char check[1100]{};sprintf_s(check,"{\"event\":\"native_rt_material_check\",\"call\":%lld,\"family\":\"%.200s\",\"index\":%u,\"pixels\":%llu,\"albedo\":%.4f,\"f0\":%.4f,\"gloss\":%.4f,\"normal\":%.4f,\"albedo_error\":%.2f,\"gloss_error\":%.2f,\"normal_degrees\":%.2f,\"albedo_bias\":[%.2f,%.2f,%.2f],\"gloss_bias\":%.2f,\"layered\":%.4f,\"layered_albedo_error\":%.2f,\"ambient\":%llu,\"ambient_close\":%.4f,\"ambient_log2_bias\":%.4f,\"ambient_log2_error\":%.4f,\"ambient_missing\":%llu}\n",
                    static_cast<long long>(n),family.c_str(),k,static_cast<unsigned long long>(check_of.pixels),double(check_of.albedo)/p,double(check_of.f0)/p,double(check_of.gloss)/p,
                    double(check_of.normal)/p,double(check_of.albedo_error)/p,double(check_of.gloss_error)/p,double(check_of.normal_error)/(10*p),
                    double(check_of.albedo_bias[0])/p,double(check_of.albedo_bias[1])/p,double(check_of.albedo_bias[2])/p,double(check_of.gloss_bias)/p,layered/p,
                    layered?double(check_of.layered_error)/layered:0.0,
                    // I-0125 R6a: the ambient against GB4 (its pixels; the share within 5 %; the mean signed and absolute log2 ratios).
                    static_cast<unsigned long long>(check_of.ambient),check_of.ambient?double(check_of.ambient_close)/double(check_of.ambient):0.0,
                    check_of.ambient?double(check_of.ambient_bias)/(256.0*double(check_of.ambient)):0.0,check_of.ambient?double(check_of.ambient_error)/(256.0*double(check_of.ambient)):0.0,
                    static_cast<unsigned long long>(check_of.ambient_missing));Log(check);}}}
    NativeRtWalkOutcome outcome;outcome.skipped=skipped;outcome.reused=reused;outcome.candidates=counts.candidates;outcome.no_storage=counts.no_storage;
    outcome.map=skipped || reused?0:counts.map;
    if(scene.rt_scene){outcome.instances=scene.rt_scene->instances.size();outcome.skinned=scene.rt_scene->skinned.size();}
    outcome.ms=ms;return outcome;
}
// I-0132 R11e step 4 (R11-PLAN §8; Thomas, 2026-11-09 04:45, D-0262: « Ok pour le message d'erreur, en espérant qu'il n'arrive
// jamais »): the rays cannot serve the frames any more (a traced pass's latched failure, RtFatalFailure) or could not start (no DXR
// 1.1, the self-test failed): once a process, a row `native_rt_fatal`, then the error box on its own thread (no owner: never the game's
// window thread blocked; topmost) and, once it is closed, SC_CLOSE posted to the game's window, DS3's own quit (the start check's close:
// its saves and threads stopped). In the test runs (rt-fail-magenta.flag) no box: the row, then the close. Any thread.
std::atomic<HWND> native_game_window{};
void NativeRtFatal(const std::string& reason) noexcept {
    static std::atomic<bool> once{};if(once.exchange(true))return;
    try{std::string shown=reason.substr(0,240);for(auto& c:shown)if(c=='"' || c=='\\' || c<32 || c>126)c='?';
        char row[400]{};sprintf_s(row,"{\"event\":\"native_rt_fatal\",\"reason\":\"%s\",\"box\":%s}\n",shown.c_str(),renderer.rt_fail_magenta.load()?"false":"true");Log(row);
        const bool box=!renderer.rt_fail_magenta.load();
        std::thread([shown,box]{
            if(box){const std::wstring text=L"Dark Souls III: Rekindled needs ray tracing to draw the world, and it stopped working:\n\n"+
                    std::wstring(shown.begin(),shown.end())+L"\n\nThe game will now close.";
                MessageBoxW(nullptr,text.c_str(),L"Dark Souls III: Rekindled",MB_OK|MB_ICONERROR|MB_TOPMOST|MB_SETFOREGROUND);}
            // R11e's step review M4: SC_CLOSE again every 10 s while the window lives (one posted during the boot or lost is not the end),
            // three times, then every 30 s for as long as the window lives (the second step review's L-D), a row at the fourth.
            for(int attempt=0;;++attempt){const HWND window=native_game_window.load();if(!window || !IsWindow(window))return;
                {DWORD owner{};GetWindowThreadProcessId(window,&owner);if(owner!=GetCurrentProcessId())return;}   // the last step review's L2: our window only
                if(attempt==3)Log("{\"event\":\"native_rt_fatal_close_unanswered\"}\n");
                PostMessageW(window,WM_SYSCOMMAND,SC_CLOSE,0);Sleep(attempt<3?10000:30000);}}).detach();}
    catch(...){if(const HWND window=native_game_window.load())PostMessageW(window,WM_SYSCOMMAND,SC_CLOSE,0);}
}
// I-0132 R11e step 3 (R11-PLAN §8; Thomas, 2026-11-09 04:43, « Le cas 1 peut être corrigé? »): an arrival (the first walk after a
// scene reset, native_rt_arrivals) holds its world black (the UI drawn over it: NativeSelectedScene::rt_hold) until the rays' scene
// is ready: the main-view frames the renderer judged served (RtJudged, past the arrival's fourth, on three walks in a row), instances
// in the scene and their count steady (within 2 %, or 2, over max(250 ms, the last 10 walks' span)); at most 5 s from the arrival's
// fourth judged frame (`timeout`: its frames then shown, black where unserved; the step review's M1, M2, L3, L-B); held frames never
// judged for 60 s and 600 frames close the game (M-A, the final review's M1). The released frame restarts the denoisers' and DLSS's history (rt_release: the
// velocity's reset). One row an arrival (at most 64), the first 240 walks of the first three arrivals journaled (the rule's measure).
// The main view's collect only (one thread).
void NativeRtArrivalStep(NativeSelectedScene& result,const NativeRtWalkOutcome& walk) {
    constexpr uint64_t kTimeoutMs=5000,kSteadyMs=250;constexpr uint32_t kSteadyWalks=10;
    struct State {uint64_t seen{UINT64_MAX},number{},start{},clock{},judged_start{},judged_seen{},first_instances{},first_served{},unjudged_start{},unjudged_since{};
        bool holding{};uint32_t walks{},served_walks{};
        std::deque<std::pair<uint64_t,uint64_t>> totals;};   // (tick ms, the scene's instances), the last 512 walks
    static State a;static uint64_t numbers{};static int rows{};
    const uint64_t now=GetTickCount64(),arrivals=native_rt_arrivals.load(std::memory_order_relaxed);
    if(arrivals!=a.seen){a=State{};a.seen=arrivals;a.holding=true;a.start=now;a.number=++numbers;a.judged_start=renderer.RtJudged()>>8;a.unjudged_start=renderer.RtHeldUnjudged();}
    if(!a.holding)return;
    ++a.walks;const uint64_t total=walk.instances+walk.skinned;if(total && !a.first_instances)a.first_instances=a.walks;
    const uint64_t judged=renderer.RtJudged();const uint32_t causes=uint32_t(judged&0xFF);
    // Its step review's M2: the timeout's clock starts at the arrival's first judged (lit) frame, so a long load does not use it up.
    if(!a.clock && (judged>>8)>a.judged_start+3)a.clock=now;   // the second step review's L-B: past the frames in flight, as below
    // L3: a frame judged past the arrival's fourth (the frames lag the collect by up to three), served on three walks in a row.
    // The second step review's L-C: a walk counts only when a newer frame was judged (three judged frames, not one read thrice).
    if((judged>>8)!=a.judged_seen){a.judged_seen=judged>>8;a.served_walks=(judged>>8)>a.judged_start+3 && !causes?a.served_walks+1:0;}
    const bool served=a.served_walks>=3;
    if(served && !a.first_served)a.first_served=a.walks;
    a.totals.emplace_back(now,total);if(a.totals.size()>512)a.totals.pop_front();   // 512: the 250 ms past 2,000 walks a second
    // M1: steady over max(250 ms, the span of the last 10 walks), so a slow frame rate still meets the rule.
    bool steady=false;
    if(total && a.totals.size()>=kSteadyWalks){const uint64_t window=(std::max)(kSteadyMs,now-a.totals[a.totals.size()-kSteadyWalks].first);
        if(a.totals.front().first+window<=now || a.totals.size()==kSteadyWalks){uint64_t low=UINT64_MAX,high=0;
            for(const auto& [t,v]:a.totals)if(t+window>=now){low=(std::min)(low,v);high=(std::max)(high,v);}
            steady=high-low<=(std::max)(uint64_t{2},high/50) && now-a.totals.front().first>=kSteadyMs;}}
    const uint64_t held_ms=now-a.start;
    if(a.number<=3 && a.walks<=240){char row[400]{};
        sprintf_s(row,"{\"event\":\"native_rt_arrival_walk\",\"arrival\":%llu,\"walk\":%u,\"ms\":%llu,\"skipped\":%s,\"reused\":%s,\"candidates\":%llu,\"instances\":%llu,\"skinned\":%llu,\"no_storage\":%llu,\"walk_ms\":%.3f,\"judged\":%llu,\"causes\":%u,\"steady\":%s}\n",
            static_cast<unsigned long long>(a.number),a.walks,static_cast<unsigned long long>(held_ms),walk.skipped?"true":"false",walk.reused?"true":"false",static_cast<unsigned long long>(walk.candidates),
            static_cast<unsigned long long>(walk.instances),static_cast<unsigned long long>(walk.skinned),static_cast<unsigned long long>(walk.no_storage),walk.ms,
            static_cast<unsigned long long>((judged>>8)-(std::min)(judged>>8,a.judged_start)),causes,steady?"true":"false");Log(row);}
    // I-0132 R11c (the zones' tour, `i132-r11c1-ariandel-travel`): ready only once this walk met the map's own pieces (`map`, not the
    // static objects or the skinned instances alone: Ariandel's arrival was released on 7 skinned instances, its map not yet walked; the
    // Ringed City's map came after its 5 s; the final review's M2): until then the timeout's rule.
    const bool ready=served && steady && walk.map>0,timeout=a.clock && now-a.clock>=kTimeoutMs;
    // The second step review's M-A, its final review's M1: main-view frames held black and never judged (no lighting input) for 60 s
    // and 600 frames: the fatal close, not a black screen for ever; counted from the first such frame (a loading screen, which encodes no
    // main-view frame, never counts).
    if(!a.clock){const uint64_t unjudged=renderer.RtHeldUnjudged()-a.unjudged_start;if(unjudged && !a.unjudged_since)a.unjudged_since=now;
        if(a.unjudged_since && now-a.unjudged_since>=60000 && unjudged>=600){result.rt_hold=true;NativeRtFatal("the world was held 60 s without a lit frame after an arrival");return;}}
    if(ready || timeout){a.holding=false;result.rt_release=true;
        if(rows<64){++rows;char row[320]{};
            sprintf_s(row,"{\"event\":\"native_rt_arrival\",\"arrival\":%llu,\"reason\":\"%s\",\"walks\":%u,\"ms\":%llu,\"held_total\":%llu,\"instances\":%llu,\"map\":%llu,\"skinned\":%llu,\"first_instances_walk\":%llu,\"first_served_walk\":%llu,\"causes\":%u}\n",
                static_cast<unsigned long long>(a.number),ready?"ready":"timeout",a.walks,static_cast<unsigned long long>(held_ms),static_cast<unsigned long long>(renderer.RtHeldFrames()),
                static_cast<unsigned long long>(walk.instances),static_cast<unsigned long long>(walk.map),static_cast<unsigned long long>(walk.skinned),static_cast<unsigned long long>(a.first_instances),static_cast<unsigned long long>(a.first_served),causes);Log(row);}
        return;}
    result.rt_hold=true;
}
// DECALS-WAVE1 D1: the kind-0 decals of every decal manager (51AA0 draws its list +0x68..+0x70 in order) with what their native
// draw needs (NativeSelectedScene::DecalSnapshot), and the renderer's copies as texture records (made once per resource).
std::mutex native_decal_sources_lock;std::array<std::shared_ptr<const bootstrap::NativeTextureRecord>,3> native_decal_sources;
std::shared_ptr<const bootstrap::NativeTextureRecord> NativeDecalSourceRecord(const Microsoft::WRL::ComPtr<ID3D12Resource>& resource,DXGI_FORMAT view_format,const char* what) {
    auto record=std::make_shared<bootstrap::NativeTextureRecord>();record->resource=resource;record->resident=true;record->render_target=true;
    {const auto desc=resource->GetDesc();record->description={static_cast<uint32_t>(desc.Width),desc.Height,1,view_format,1,false};}
    try{record->view=gpu.AllocateCpuDescriptor();}catch(...){Refuse(what);}
    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=view_format;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;
    gpu.device->CreateShaderResourceView(resource.Get(),&srv,record->view.handle);return record;}
void SnapshotNativeDecals(NativeSelectedScene& scene) {
    {uintptr_t settings{};uint8_t option{1};if(Read(image+0x4961B50,&settings,8) && settings && Read(settings+0x5360,&settings,8) && settings && Read(settings+0x68,&option,1))scene.decal_option=option!=0;}
    if(!scene.decal_option)return;
    {const std::lock_guard hold(decal_cpu_lock);
        for(const auto& [manager,owned]:decal_cpu_registries){std::array<uintptr_t,2> list{};
            if(!Read(manager+0x68,list.data(),16) || list[1]<list[0] || (list[1]-list[0])%8 || (list[1]-list[0])/8>owned.count)continue;
            for(auto at=list[0];at!=list[1];at+=8){uintptr_t object{},table{},material{};
                if(!Read(at,&object,8) || !object || !Read(object,&table,8) || table!=image+0x3D5B8D8 || !Read(object+0x270,&material,8) || !material)continue;   // kind 0, with its material (198910's first test)
                auto& d=scene.decals.emplace_back();if(!Read(object,d.object.data(),d.object.size())){scene.decals.pop_back();continue;}
                if(!Read(material,d.material.data(),d.material.size())){scene.decals.pop_back();continue;}
                Read(manager+0x400,d.settings.data(),d.settings.size());
                // 59470 L175-248 (the entity-less decal call, no render override natively): the two banks 32A80 finds in
                // [root+0x41E0] for the material's keys, 12 floats at +0x290 each, A when t <= FLT_EPSILON, B when t >= 1, else
                // (b-a)*t+a; the row scales [1439F24B0] 1, row k's .w times the pass context's +0x110 + 4k (59470's param_1: the
                // scene's pass_row_scale, I-0113 tranche 34; 1 before).
                {uintptr_t root{},banks{};float t{};std::memcpy(&t,d.material.data()+0x108,4);
                    if(Read(image+0x4961B50,&root,8) && root && Read(root+0x41E0,&banks,8) && banks){
                        const auto lookup=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uintptr_t)>(image+0x32A80);
                        const uintptr_t first=lookup(banks,material+0xF8),second=lookup(banks,material+0x100);std::array<float,12> a{},b{};
                        if(first && second && Read(first+0x290,a.data(),48) && Read(second+0x290,b.data(),48)){std::array<float,12> rows{};
                            for(size_t k=0;k<12;++k)rows[k]=t<=1.1920929e-7f?a[k]:t>=1.0f?b[k]:(b[k]-a[k])*t+a[k];
                            // The engine's rows, zero natively (no file in its banks) as the original's b11 at its decal draws (Reference2134:
                            // all 27 decal_oracle rows; tranche 34's final review H1 reverted the native banks' rows); scaled as 59470 scales them.
                            for(size_t r=0;r<3;++r)rows[4*r+3]*=scene.pass_row_scale[r];d.sh=rows;}}}
                d.pixel=owned.programmes;d.vertex=owned.constant_descriptions;
                for(size_t i=0;i<d.textures.size();++i){uintptr_t entry{},texture{};std::memcpy(&entry,d.object.data()+0x190+8*i,8);if(!entry)continue;
                    auto record=textures.Acquire(entry);if(!record && Read(entry+0x10,&texture,8) && texture)record=textures.Acquire(texture);   // SprjGxTexture -> GXTexture2D
                    if(record && record->resident && record->resource && record->view)d.textures[i]=std::move(record);}}}}
    if(scene.decals.empty())return;
    const auto sources=renderer.DecalSources();if(!sources.gb0 || !sources.gb1 || !sources.depth){scene.decals.clear();return;}
    const std::lock_guard hold(native_decal_sources_lock);
    const std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,3> resources{sources.gb0,sources.gb1,sources.depth};
    const std::array<DXGI_FORMAT,3> views{DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS};
    for(size_t i=0;i<3;++i)if(!native_decal_sources[i] || native_decal_sources[i]->resource.Get()!=resources[i].Get())
        native_decal_sources[i]=NativeDecalSourceRecord(resources[i],views[i],"native_decal_source_view");
    scene.decal_sources=native_decal_sources;
}
// I-0100 tranche 2: an engine byte ORed with bits, guarded as the paint writes (an entity retired under the walk).
bool OrNativeEngineByte(uintptr_t at,BYTE bits) noexcept {
    const bool outer=guarded_copy_active;guarded_copy_active=true;bool written=true;
    __try{_InterlockedOr8(reinterpret_cast<volatile char*>(at),static_cast<char>(bits));}__except(EXCEPTION_EXECUTE_HANDLER){written=false;}
    guarded_copy_active=outer;return written;}
// DECALS-PAINT-READY rung 2: the original's CPU side effects of a drained paint command, guarded (the engine's memory).
bool WriteNativePaintBytes(uintptr_t at,const void* bytes,size_t count) noexcept {
    const bool outer=guarded_copy_active;guarded_copy_active=true;bool written=true;   // I-0082: a handled fault, not a crash
    __try{std::memcpy(reinterpret_cast<void*>(at),bytes,count);}__except(EXCEPTION_EXECUTE_HANDLER){written=false;}
    guarded_copy_active=outer;return written;}
// DECALS-PAINT-READY rung 2 (main collect, engine thread, under the decal registry lock, which also holds the requests whose
// synchronous world query appends to the queue, and the queues read under the engine's CPU lock below): what 522B0 does natively for the type-0 paint commands, which nothing else
// drains (the original runs it as a render-pass slot): each manager's queue +0x20..+0x28 (0x30-byte entries {receiver, handle,
// +0x18 type, +0x1C floats}), at most its budget +0x408 a collect, the taken entries erased as 522B0 does. A handle-0 command
// clears the receiver's target (5F430); another takes the kind-1 object of its bucket (5FCD0's generation test), paints only
// when the receiver's gate is set (+0x78 for a material paint, +0x79 for a source one, descriptor +0x78), and then decrements
// the object's pending count +0x13C and sets its painted byte +0x10 (5FCD0). Rung 2 draws the box material paint (projection 0,
// no source); the cube and height variants are consumed without a draw (counted). I-0105 (BLOOD.md, 5F430): a type-2 command
// clears the receiver's target with the mask p0 (0xFFFF from both producers: the handle-0 clear), a type-1 command with its
// erase byte +0x2C draws 1AAF90's erase pass over the receiver's meshes; both only on a target the native already made (a target
// never painted holds the neutral clear). A masked clear and type 1 without the byte (the bloodmask blit, no producer in the
// shipped code) are counted, not drained. Neither writes the target's +0x54 flags (as the type-0 path).
void SnapshotNativePaint(NativeSelectedScene& scene) {
    static const bool observe=ModuleFlag(L"decal-observe.flag");
    if(!scene.common_images)return;
    static std::array<LONG64,12> counts{};   // drained, painted, clears, foreign, stale, gate, variant, texture, target, other types, erases, no target
    const auto note=[&](size_t what){InterlockedIncrement64(&counts[what]);};
    const auto system=[&](const wchar_t* name)->std::shared_ptr<const bootstrap::NativeTextureRecord>{
        for(const auto& [system_name,texture]:scene.common_images->system)if(texture && _wcsicmp(system_name.c_str(),name)==0)return texture;return {};};
    const auto resolve=[&](uintptr_t entry)->std::shared_ptr<const bootstrap::NativeTextureRecord>{if(!entry)return {};uintptr_t texture{};
        auto record=textures.Acquire(entry);if(!record && Read(entry+0x10,&texture,8) && texture)record=textures.Acquire(texture);   // SprjGxTexture -> GXTexture2D
        return record && record->resident && record->resource && record->view?record:nullptr;};
    // D-0193 review: +0x50 holds a pointer to the receiver's GXBloodyMaterialPatch (5E990 allocates it), the model's selected.patch.
    const auto patch_of=[](uintptr_t receiver)->uintptr_t{uintptr_t patch{};return Read(receiver+0x50,&patch,8)?patch:0;};
    bool drained{};
    {const std::lock_guard hold(native_paint_lock);   // retirement: the receiver gone or its slot-0 target replaced
        for(auto it=native_paints.begin();it!=native_paints.end();){uintptr_t table{},slot{};
            if(!Read(it->first,&table,8) || table!=image+0x3D41DD8 || !Read(it->first+0x20,&slot,8) || slot!=it->second.slot){it=native_paints.erase(it);}
            else{if(it->second.announce){--it->second.announce;auto& e=scene.paints.emplace_back();e.patch=patch_of(it->first);e.target=it->second.target;}   // its first clear
                ++it;}}}
    // I-0105 tranche 4 M2: each queue read and compacted under the engine's CPU lock, which its producers (D071B0, D00E60, the
    // enemy reset and the SpEffect ends) hold while they push (a push between the read and the end pointer's write was lost, a
    // reallocating one left the compaction a freed buffer); the taken entries are then drained under the native locks only. Tried,
    // never waited for (final review L1): when another thread holds it the queues wait for the next collect, counted.
    std::optional<EngineCpuLock> engine;engine.emplace(true);
    if(!engine->held){static LONG64 busy{};if(const auto n=InterlockedIncrement64(&busy);n<=4 || n%600==0){char row[96]{};
        sprintf_s(row,"{\"event\":\"native_paint_queue_busy\",\"n\":%lld}\n",static_cast<long long>(n));Log(row);}return;}
    const std::lock_guard hold(decal_cpu_lock);
    std::vector<std::pair<NativeDecalCpuRegistry*,std::vector<uint8_t>>> taken;taken.reserve(decal_cpu_registries.size());   // no throw after an erase
    for(auto& [manager,owned]:decal_cpu_registries){
        std::array<uintptr_t,2> queue{};uint32_t budget{};
        if(!Read(manager+0x20,queue.data(),16) || queue[1]<queue[0] || (queue[1]-queue[0])%0x30 || (queue[1]-queue[0])/0x30>4096 || !Read(manager+0x408,&budget,4))continue;
        const size_t count=(queue[1]-queue[0])/0x30,take=std::min<size_t>(count,budget);if(!take)continue;
        std::vector<uint8_t> entries(count*0x30);if(!Read(queue[0],entries.data(),entries.size()))continue;
        // Review (not yet witnessed live): the queue's layout is the static reading; nothing is written unless every entry names a
        // GXDecalReceiver with a type of 0..2 or 10, so a wrong layout drains nothing.
        // Review: an entry whose receiver is gone is skipped and erased with the others; the layout holds when every type is known and
        // at least one entry names a GXDecalReceiver.
        {bool layout=true,receiver_seen=false;for(size_t i=0;i<count && layout;++i){const auto* e=entries.data()+0x30*i;uintptr_t receiver{},table{};uint32_t type{};
                std::memcpy(&receiver,e,8);std::memcpy(&type,e+0x18,4);layout=type<=2 || type==10;
                receiver_seen|=receiver && Read(receiver,&table,8) && table==image+0x3D41DD8;}
            layout&=receiver_seen;
            if(!layout){static LONG64 logged{};if(InterlockedIncrement64(&logged)<=4)Log("{\"event\":\"native_paint_queue_layout_refused\"}\n");continue;}}
        // 522B0's erase: the entries past the taken ones moved to the front, the end pointer after them.
        const size_t remain=count-take;
        if(remain && !WriteNativePaintBytes(queue[0],entries.data()+take*0x30,remain*0x30))continue;
        const uintptr_t end=queue[0]+remain*0x30;if(!WriteNativePaintBytes(manager+0x28,&end,8))continue;   // a fault on memory just read: not drained
        entries.resize(take*0x30);taken.emplace_back(&owned,std::move(entries));}
    engine.reset();
    for(auto& [owned_at,entries]:taken){auto& owned=*owned_at;const size_t take=entries.size()/0x30;
        for(size_t i=0;i<take;++i){const auto* e=entries.data()+0x30*i;uintptr_t receiver{};uint32_t handle{},type{};
            std::memcpy(&receiver,e,8);std::memcpy(&handle,e+8,4);std::memcpy(&type,e+0x18,4);note(0);drained=true;
            if(type==1 || type==2){uint32_t mask{};std::memcpy(&mask,e+0x1C,4);
                // The masked clear, the bloodmask blit, an erase of another surface than 0 (p0, 0 from both producers: step review)
                if((type==2 && mask!=0xFFFFu) || (type==1 && (!e[0x2C] || mask))){note(9);continue;}
                uintptr_t table{},slot{};
                if(!receiver || !Read(receiver,&table,8) || table!=image+0x3D41DD8 || !Read(receiver+0x20,&slot,8) || !slot){note(3);continue;}
                std::lock_guard paint_hold(native_paint_lock);const auto found=native_paints.find(receiver);
                if(found==native_paints.end()){note(11);continue;}
                auto& c=scene.paints.emplace_back();c.patch=patch_of(receiver);c.target=found->second.target;
                if(type==2){c.clear=true;note(2);continue;}
                // 1AAF90 (5F430 hands it {p2, p3, p1}): FC_PaintEraseColor (0, 0, 0, 1 - p1), FC_PaintFadeParam (p2 + p3, p2, p3 ? 1 / p3 : 0, 0).
                float q[4]{};std::memcpy(q,e+0x1C,16);
                c.erase=true;c.erase_block={0,0,0,1.0f-q[1],q[2]+q[3],q[2],q[3]!=0.0f?1.0f/q[3]:0.0f,0};note(10);continue;}
            if(type!=0){note(9);continue;}   // type 10 (the targets' slot +0x10): not drained
            uintptr_t table{},slot{};
            if(!receiver || !Read(receiver,&table,8) || table!=image+0x3D41DD8 || !Read(receiver+0x20,&slot,8) || !slot){note(3);continue;}
            if(!handle){note(2);std::lock_guard paint_hold(native_paint_lock);const auto found=native_paints.find(receiver);
                if(found!=native_paints.end()){auto& c=scene.paints.emplace_back();c.patch=patch_of(receiver);c.target=found->second.target;c.clear=true;}continue;}
            // 5FCD0: the bucket's object of this generation, and its native description.
            const auto index=handle&0xFFFFu;uintptr_t object{},object_table{};uint16_t generation{};
            const auto request=owned.requests.find(handle);
            if(index>=owned.count || !Read(owned.buckets+uint64_t(index)*8,&object,8) || !object || !Read(owned.generations+uint64_t(index)*2,&generation,2) ||
               generation!=(handle>>16) || !Read(object,&object_table,8) || object_table!=image+0x3D5B978 || request==owned.requests.end() || request->second.kind!=1){note(4);continue;}
            const auto& d=request->second.descriptor;uintptr_t source{};uint32_t projection{};std::memcpy(&source,d.data()+0x78,8);std::memcpy(&projection,d.data()+0x3C,4);
            uint8_t gates[2]{};if(!Read(receiver+0x78,gates,2)){note(5);continue;}
            // D-0193: 51F80 copied the gates from the model's +224 while it was still 0 natively; the native union of the model's
            // material policy flags (bits 2 and 3, native_patch_flags by the receiver's patch [+0x50]) stands for that word.
            if(!gates[0] && !gates[1]){const std::lock_guard policy_hold(model_policy_lock);
                if(const auto found=native_patch_flags.find(patch_of(receiver));found!=native_patch_flags.end()){
                    gates[0]=uint8_t((found->second>>2)&1u);gates[1]=uint8_t((found->second>>3)&1u);}}
            if(!gates[source?1:0]){note(5);continue;}
            const auto consumed=[&]{int32_t pending{};if(Read(object+0x13C,&pending,4)){--pending;WriteNativePaintBytes(object+0x13C,&pending,4);}
                const uint8_t painted=1;WriteNativePaintBytes(object+0x10,&painted,1);};
            if(source || projection){note(6);consumed();continue;}
            NativeSelectedScene::PaintEvent event;event.patch=patch_of(receiver);event.draw=true;
            ds3content::PaintProjector projector;if(!ds3content::ComputePaintProjector(d.data(),projector)){note(6);consumed();continue;}
            std::memcpy(event.view.data(),projector.wvp.data(),64);
            // 1AB370: mulCol = d+0xC8 rgb times the paint context's +0x310 scale (INFERRED 1), w the context's +0x31C (INFERRED 1);
            // g_PaintMaterialParamScale = (d+0xD4, d+0xE0). 1AC390: t0 d+0x48, t1 +0x50, t2 +0x58, t3 +0x60 when the byte +0xE8 is
            // set (else the default image [[GX+0x270]+0x10], INFERRED the flat normal), t4 +0x70 (else [[GX+0x270]+0], INFERRED
            // white); s3 the bank row 4 instead of 10 without a mask.
            std::memcpy(event.mul_col.data(),d.data()+0xC8,12);event.mul_col[3]=1.0f;
            std::memcpy(&event.param_scale[0],d.data()+0xD4,4);std::memcpy(&event.param_scale[1],d.data()+0xE0,4);
            // D-0193 (Native2353 decal_observe_paint_request): the mask t4 is d+0x70 (1AC390), not d+0x68 (zero in every request seen), so
            // the mask was never bound and each paint covered its projector square wherever t0 (BC1, opaque) has alpha.
            uintptr_t entries_at[5]{};for(size_t t=0;t<4;++t)std::memcpy(&entries_at[t],d.data()+0x48+8*t,8);std::memcpy(&entries_at[4],d.data()+0x70,8);
            event.masked=entries_at[4]!=0;
            // An absent t0..t2 is D3D11's null view (zero): SYSTEX_ZERO; a present one must be resident.
            const auto texture_or_zero=[&](uintptr_t entry){return entry?resolve(entry):system(L"SYSTEX_ZERO");};
            event.textures={texture_or_zero(entries_at[0]),texture_or_zero(entries_at[1]),texture_or_zero(entries_at[2]),
                d[0xE8] && entries_at[3]?resolve(entries_at[3]):system(L"SYSTEX_NORMAL"),entries_at[4]?resolve(entries_at[4]):system(L"SYSTEX_WHITE")};
            if(std::any_of(event.textures.begin(),event.textures.end(),[](const auto& t){return !t;})){note(7);consumed();continue;}
            // The receiver's target (4CFB0's size: the slot-0 target's creation descriptor +0x60, INFERRED the edge; 1024 otherwise).
            {std::lock_guard paint_hold(native_paint_lock);auto found=native_paints.find(receiver);
                if(found==native_paints.end()){uint32_t size{};Read(slot+0x60,&size,4);const bool read=size>=64 && size<=4096 && !(size&(size-1));
                    if(!read)size=1024;
                    std::shared_ptr<NativePaintTarget> made;if(native_paints.size()<64)try{made=renderer.MakePaintTarget(size);}catch(...){}
                    if(!made){note(8);consumed();continue;}
                    NativePaintRecord record;record.slot=slot;record.target=made;record.announce=8;
                    record.diffuse=NativeDecalSourceRecord(made->diffuse,SceneRenderer::paint_formats[0],"native_paint_view");
                    record.specular=NativeDecalSourceRecord(made->specular,SceneRenderer::paint_formats[1],"native_paint_view");
                    if(made->occlusion)record.occlusion=NativeDecalSourceRecord(made->occlusion,DXGI_FORMAT_R16_UNORM,"native_paint_occlusion_view");   // D-0193 rung 2b
                    found=native_paints.emplace(receiver,std::move(record)).first;
                    char row[200]{};sprintf_s(row,"{\"event\":\"native_paint_target\",\"size\":%u,\"size_read\":%s,\"targets\":%zu}\n",size,read?"true":"false",native_paints.size());Log(row);}
                event.target=found->second.target;event.occlusion=found->second.occlusion;}
            consumed();note(1);
            if(observe){static LONG64 logged{};if(InterlockedIncrement64(&logged)<=24){const auto& w=projector.wvp;char row[1024]{};
                // D-0193: t0's size and name, s (d+0x24), z (d+0x2C) and range (d+0x30); the name kept to JSON-safe characters.
                const auto& t0=event.textures[0]->description;float s{},z{},range{};std::memcpy(&s,d.data()+0x24,4);std::memcpy(&z,d.data()+0x2C,4);std::memcpy(&range,d.data()+0x30,4);
                std::string t0_name=event.textures[0]->content?event.textures[0]->content->name.substr(0,100):std::string();for(auto& c:t0_name)if(c=='"' || c==0x5C || (c>=0 && c<32))c='?';
                sprintf_s(row,"{\"event\":\"native_paint_event\",\"handle\":%u,\"diverging\":%s,\"wvp\":[%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g],\"mul_col\":[%g,%g,%g],\"masked\":%s,\"t0\":[%u,%u,%u,%u],\"t0_entry\":%s,\"s_z_range\":[%g,%g,%g],\"param_scale\":[%g,%g],\"t0_name\":\"%s\"}\n",
                    handle,projector.diverging?"true":"false",w[0],w[1],w[2],w[3],w[4],w[5],w[6],w[7],w[8],w[9],w[10],w[11],w[12],w[13],w[14],w[15],
                    event.mul_col[0],event.mul_col[1],event.mul_col[2],event.masked?"true":"false",t0.width,t0.height,t0.mips,unsigned(t0.format),
                    entries_at[0]?"true":"false",s,z,range,event.param_scale[0],event.param_scale[1],t0_name.c_str());Log(row);}}
            scene.paints.push_back(std::move(event));}}
    if(drained){static LONG64 rows{};if(const auto n=InterlockedIncrement64(&rows);n<=16 || n%100==0){char row[512]{};
        sprintf_s(row,"{\"event\":\"native_paint_drain\",\"n\":%lld,\"drained\":%lld,\"painted\":%lld,\"clears\":%lld,\"foreign\":%lld,\"stale\":%lld,\"gate\":%lld,\"variant\":%lld,\"texture\":%lld,\"target\":%lld,\"other_types\":%lld,\"erases\":%lld,\"no_target\":%lld}\n",
            n,counts[0],counts[1],counts[2],counts[3],counts[4],counts[5],counts[6],counts[7],counts[8],counts[9],counts[10],counts[11]);Log(row);}}
}
// WT-6, I-0125 R6c (R6-PLAN §6): the water's reflection traced from its own surface by the main frame (I-0125 R6f, D-0251: the
// mirror view, its walk and its draws removed). This collect publishes the plane of the nearest water mesh and the t54 target
// for the frame's pass; nothing when the option is off, no water mesh is in the main view, the traced water is off or failed, or a
// step fails (logged): the water then samples the black default, as the original without a reflection object. A collect without
// the rays' scene keeps the previous state (below).
void CollectNativeWaterPlane(NativeSelectedScene& main) noexcept {
    const auto publish_none=[]{const std::lock_guard hold(native_reflection.lock);native_reflection.traced=false;};
    // The target (RGBA16F, half the output extent), made once per extent on this thread; its record is what t54 binds. Under the lock.
    const auto make_target=[](uint32_t width,uint32_t height){
        if(native_reflection.target && native_reflection.width==width && native_reflection.height==height)return;
        D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=width;desc.Height=height;
        desc.DepthOrArraySize=1;desc.MipLevels=1;desc.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;desc.SampleDesc.Count=1;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        D3D12_HEAP_PROPERTIES properties{};properties.Type=D3D12_HEAP_TYPE_DEFAULT;properties.CreationNodeMask=properties.VisibleNodeMask=1;
        D3D12_CLEAR_VALUE clear{};clear.Format=desc.Format;
        Microsoft::WRL::ComPtr<ID3D12Resource> target;
        if(FAILED(CountedResource(gpu.device)->CreateCommittedResource(&properties,D3D12_HEAP_FLAG_NONE,&desc,
            static_cast<D3D12_RESOURCE_STATES>(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),&clear,IID_PPV_ARGS(&target))))
            throw std::runtime_error("reflection target allocation");
        native_reflection.target=target;native_reflection.width=width;native_reflection.height=height;native_reflection.written=false;
        native_reflection.record=NativeDecalSourceRecord(target,DXGI_FORMAT_R16G16B16A16_FLOAT,"native_reflection_view");
        char row[160]{};sprintf_s(row,"{\"event\":\"native_reflection_target\",\"size\":[%u,%u]}\n",width,height);Log(row);};
    static LONG64 failures{};
    try {
        // The ReflectionQuality row [[GXRS+0x5360]+0x6C] (MENUS-MAP 77.3: DISABLE 0, every other level 1), from the collect's option
        // snapshot: 0, the original binds the black default (1.3). I-0129 R9c: kept (the water's FC_WaterFresnel words follow the same
        // option in the admission's record; the player's « no reflection » until R12's menu replaces DS3's options).
        static_assert(bootstrap::QualityObjectBytes>0x6C);uint8_t option=1;
        if(main.options && main.options->object_valid)option=main.options->object[0x6C];
        if(!option){publish_none();return;}
        // The traced water off or failed for the session: no reflection (D-0251: no raster mirror view to fall back on).
        if(!renderer.rt_water.load(std::memory_order_relaxed) || renderer.RtSceneFailed()){publish_none();return;}
        // A collect without the rays' scene (a skipped rays' walk, R6f's step review M1): `traced` and the target left as they are, so
        // the next collect's epochs (which read NativeReflectionRecord before its walk) keep the record and the water its last picture
        // for this frame, which has no traced pass; no black frame and no fresh water admissions after an isolated skip. From the 30th
        // skipped collect in a row (the traced shadows' rule, CollectNativeSelectedScene) the black default: no old picture kept
        // through a long run of skips (the final review's L3; across a load, up to 29 skipped collects may still bind the last
        // zone's picture: a known limit, the scene reset not reaching this state).
        static uint32_t skipped{};   // the main view's collects, one at a time
        if(!main.rt_scene){if(++skipped>=30){skipped=30;publish_none();}return;}
        skipped=0;
        // WT-6: the plane of the nearest water mesh (policy bit 4, g_GXFT_PlanarReflection): the owner's box centre, n = +Y.
        const auto& pose=main.camera.camera_pose;
        std::array<float,4> plane{0,1,0,0};float nearest=FLT_MAX;std::string source;uint32_t candidates{};std::array<float,2> span{};   // R6c: its box's y
        for(const auto& m:main.models){bool water=false;
            for(const auto& mesh:m.meshes)if(mesh.programs && mesh.programs->policy && (mesh.programs->policy->flags&0x10)){water=true;break;}
            if(!water)continue;
            const uintptr_t owner=m.owner?m.owner:m.entity;uintptr_t table{};alignas(16) float box[8]{};
            if(!owner || !Read(owner,&table,8))continue;
            if(table==image+0x29489A0){if(!Read(owner+0x40,box,sizeof box))continue;}
            else if(table==image+0x2948EF8){if(!Read(owner+0xB80,box,sizeof box))continue;}
            else continue;
            bool finite=true;for(const auto v:box)if(!std::isfinite(v))finite=false;if(!finite)continue;
            const float cx=(box[0]+box[4])*.5f,cy=(box[1]+box[5])*.5f,cz=(box[2]+box[6])*.5f;
            const float dx=cx-pose[12],dy=cy-pose[13],dz=cz-pose[14],t=std::sqrt(dx*dx+dy*dy+dz*dz);++candidates;
            if(t<nearest){nearest=t;plane={0,1,0,-cy};source=NativeModelDataName(m.model);span={box[1],box[5]};}}
        if(!candidates){publish_none();return;}
        {static std::array<float,4> last{};static LONG64 rows{};if(plane!=last && InterlockedIncrement64(&rows)<=48){last=plane;char row[256]{};
            sprintf_s(row,"{\"event\":\"native_water_plane\",\"plane\":[%g,%g,%g,%g],\"distance\":%g,\"candidates\":%u,\"source\":\"%.40s\",\"box_y\":[%g,%g],\"eye_y\":%g}\n",
                plane[0],plane[1],plane[2],plane[3],nearest,candidates,source.c_str(),span[0],span[1],pose[13]);Log(row);}}
        const uint32_t width=(std::max)(1u,output_width/2),height=(std::max)(1u,output_height/2);
        if(width>4096 || height>4096){publish_none();   // the target's bound (the former offscreen path's, pre-merge review)
            static LONG64 too_wide{};if(InterlockedIncrement64(&too_wide)<=1)Log("{\"event\":\"native_reflection_skipped\",\"reason\":\"extent over 4096\"}\n");return;}
        {const std::lock_guard hold(native_reflection.lock);make_target(width,height);native_reflection.traced=true;}
        main.rt_water_plane=plane;
        native_rt_water_planes.fetch_add(1,std::memory_order_relaxed);
    }catch(const std::exception& e){publish_none();
        if(InterlockedIncrement64(&failures)<=12){char row[400]{};sprintf_s(row,"{\"event\":\"native_reflection_failure\",\"message\":\"%.300s\"}\n",e.what());Log(row);}}
    catch(...){publish_none();if(InterlockedIncrement64(&failures)<=12)Log("{\"event\":\"native_reflection_failure\",\"message\":\"unknown exception\"}\n");}
}
void WalkNativeLayers(uintptr_t world,uintptr_t heap,uint64_t mask,uintptr_t culler,uint32_t world_mask,const uintptr_t* table,
    NativeSelectedScene& result,std::vector<NativeDeferredCandidate>& deferred,NativeWalkHistory& history,
    bool main_walk,NativeRtWalkShare* rays) {
    // Review L5: the rays' walk's failures skip that walk (its caller catches), the main walk's refuse, as before.
    const auto fail=[main_walk](const char* reason){if(main_walk)Refuse(reason);throw std::runtime_error(reason);};
    uintptr_t begin{},end{};
    if(!Read(world+0x10,&begin,8) || !Read(world+0x18,&end,8) || end<begin || (end-begin)%8 || (end-begin)/8>256)fail("native_scene_selection_world_layers");
    const size_t count=(end-begin)/8;std::vector<uintptr_t> layers(count),methods(count);
    if(count && !Read(begin,layers.data(),count*8))fail("native_scene_selection_world_layers");
    for(size_t i=0;i<count;++i){uintptr_t vtable{};if(!layers[i] || !Read(layers[i],&vtable,8) || !Read(vtable+8,&methods[i],8) || !methods[i])fail("native_scene_selection_world_layers");}
    struct Layer {NativeSelectionVisitor visitor;std::vector<uintptr_t> gathered;int64_t ticks{};uintptr_t stack{};bool alive{};};
    std::vector<Layer> walks(count);
    // T5b step review M1: a failure (thrown) destroys the traversers still alive, as the join does (each holds its bounds stack
    // from the engine's heap): the rays' walk skips a failed walk and goes on, never leaking a frame's traversers.
    struct Destroy {std::vector<Layer>& walks;~Destroy(){const std::lock_guard heap_hold(native_traverser_heap_lock);for(auto& walk:walks)if(walk.alive){walk.alive=false;
        reinterpret_cast<uintptr_t(__fastcall*)(void*,uint32_t)>(image+0x234720)(&walk.visitor,0);}}} const destroy{walks};
    auto& last_objects=history.objects;auto& last_sizes=history.sizes;last_objects.resize(count);last_sizes.resize(count);
    const NativeViewFlags view;   // set on the workers for their jobs
    // light-observe.flag's walk diagnostics (the refused map pieces by gate) need the whole filter inside the walk: no gathering.
    static const bool whole_filter=ModuleFlag(L"light-observe.flag");
    {const std::lock_guard heap_hold(native_traverser_heap_lock);   // R11g: released before a failure's Destroy takes it
    for(size_t i=0;i<count;++i){auto& visitor=walks[i].visitor;
        reinterpret_cast<uintptr_t(__fastcall*)(void*,uintptr_t,uint64_t,uintptr_t)>(image+0x234620)(&visitor,heap,mask,culler);walks[i].alive=true;
        *reinterpret_cast<const uintptr_t**>(visitor.base.data())=table;
        // Both scheduling getters read these zero bytes. The entire bounds stack and paired push/pop remain the actual CPU
        // algorithms. Occlusion is open.
        visitor.base[0x10]=visitor.base[0x11]=visitor.base[0x12]=0;
        // Review M1: the bounds stack (+0x50 begin, +0x58 end, +0x60 capacity, 0x20-byte entries, grown by 19C130 through the
        // traverser's heap, which only the engine thread used before I-0077) reserved here for 64 levels with 234620's own
        // reserve 19C1D0, so the workers never allocate from that heap (the node walk keeps one push per tree level).
        reinterpret_cast<void(__fastcall*)(void*,uint64_t)>(image+0x19C1D0)(visitor.base.data()+0x50,64);
        {uintptr_t stack[3]{};std::memcpy(stack,visitor.base.data()+0x50,24);
            if(stack[1]!=stack[0] || stack[2]-stack[0]<64*0x20)fail("native_scene_selection_bounds_stack_reserve");
            walks[i].stack=stack[0];}
        visitor.result=&result;visitor.world_mask=world_mask;visitor.rays_walk=rays!=nullptr;visitor.uncounted=rays!=nullptr;   // T5b: the rays' walk not against the scene's bound
        visitor.deferred=&deferred;   // with whole_filter (one thread), the candidates go straight to the list
        if(!whole_filter){walks[i].gathered.reserve(last_sizes[i]+last_sizes[i]/8);visitor.gather=&walks[i].gathered;}}}
    std::vector<size_t> order(count);for(size_t i=0;i<count;++i)order[i]=i;
    if(!whole_filter)std::stable_sort(order.begin(),order.end(),[&](size_t a,size_t b){return last_objects[a]>last_objects[b];});   // one thread: the layer order (the list is then filled in walk order)
    std::atomic<size_t> ticket{};
    const std::function<void(size_t)> job=[&](size_t){const NativeViewFlags::Apply view_flags(view);const NativeRtWalkShare::Scope rays_scope(rays);
        for(size_t n;(n=ticket.fetch_add(1,std::memory_order_relaxed))<count;){const size_t i=order[n];auto& walk=walks[i];
            LARGE_INTEGER t0{},t1{};QueryPerformanceCounter(&t0);
            {const kprofile::Scope k(kprofile::Traversal);
                reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t)>(methods[i])(layers[i],reinterpret_cast<uintptr_t>(&walk.visitor));}
            QueryPerformanceCounter(&t1);walk.ticks=t1.QuadPart-t0.QuadPart;}};
    if(whole_filter || count<2)job(0);else AdmissionPool().Run(job);
    std::vector<uintptr_t> gathered;{size_t total{};for(const auto& walk:walks)total+=walk.gathered.size();gathered.reserve(total);}
    for(size_t i=0;i<count;++i){auto& walk=walks[i];
        int32_t depth{};std::memcpy(&depth,walk.visitor.base.data()+0x20,4);
        uintptr_t stack{};std::memcpy(&stack,walk.visitor.base.data()+0x50,8);
        if(stack!=walk.stack){static LONG64 grown{};if(InterlockedIncrement64(&grown)<=8)Log("{\"event\":\"native_walk_bounds_stack_grown\"}\n");}   // review M1: a worker allocated
        {const std::lock_guard heap_hold(native_traverser_heap_lock);walk.alive=false;reinterpret_cast<uintptr_t(__fastcall*)(void*,uint32_t)>(image+0x234720)(&walk.visitor,0);}
        if(depth)fail("native_scene_selection_unbalanced_bounds");
        last_objects[i]=walk.visitor.objects;last_sizes[i]=walk.gathered.size();
        gathered.insert(gathered.end(),walk.gathered.begin(),walk.gathered.end());}
    // ER's entity Draw jobs (141B4CD50 -> GXEntityDrawParallelJob): the gathered entities' eligibility and candidates on the pool,
    // in chunks taken by ticket, each chunk with its own candidate list, joined in the gathered order (the walk's order).
    if(!gathered.empty()){constexpr size_t per_chunk=32;const size_t chunks=(gathered.size()+per_chunk-1)/per_chunk;
        std::vector<std::vector<NativeDeferredCandidate>> lists(chunks);std::atomic<size_t> next{};
        const std::function<void(size_t)> expand=[&](size_t){const NativeViewFlags::Apply view_flags(view);const NativeRtWalkShare::Scope rays_scope(rays);
            NativeSelectionVisitor visitor;visitor.result=&result;visitor.world_mask=world_mask;visitor.expanding=true;visitor.rays_walk=rays!=nullptr;
            const int32_t inside=1;std::memcpy(visitor.base.data()+0x24,&inside,4);std::memcpy(visitor.base.data()+0x18,&culler,8);
            for(size_t c;(c=next.fetch_add(1,std::memory_order_relaxed))<chunks;){visitor.deferred=&lists[c];lists[c].reserve(per_chunk*2);
                for(size_t i=c*per_chunk;i<gathered.size() && i<(c+1)*per_chunk;++i)AdmitNativeSelectedObject(&visitor,gathered[i]);}};
        if(whole_filter)expand(0);else AdmissionPool().Run(expand);
        size_t total=deferred.size();for(const auto& list:lists)total+=list.size();deferred.reserve(total);
        for(auto& list:lists)std::move(list.begin(),list.end(),std::back_inserter(deferred));}
    // Guard (record-revalidation.flag): every 64th main walk made again on this thread as before I-0077 (C1FB0 with one traverser,
    // the whole filter in the walk) and its candidate list compared in order (entity, model, kind, part, placement).
    // T5b step review L2: the rays' walk too (its own count; a share of its own, so the serial walk's gates are not added again).
    static LONG64 walks_made[2]{};static std::atomic<uint64_t> checks[2]{},mismatches[2]{};const size_t guard=rays?1:0;
    if((main_walk || rays) && NativeRecordRevalidationPeriod()<512 && !whole_filter && decal_world_query_original && InterlockedIncrement64(&walks_made[guard])%64==0){
        NativeRtWalkShare serial_share;const NativeRtWalkShare::Scope serial_scope(rays?&serial_share:nullptr);
        std::unique_lock heap_hold(native_traverser_heap_lock);   // R11g: its traverser's stack grows from the heap during the walk
        std::vector<NativeDeferredCandidate> serial_list;NativeSelectionVisitor visitor;visitor.rays_walk=rays!=nullptr;
        reinterpret_cast<uintptr_t(__fastcall*)(void*,uintptr_t,uint64_t,uintptr_t)>(image+0x234620)(&visitor,heap,mask,culler);
        *reinterpret_cast<const uintptr_t**>(visitor.base.data())=table;visitor.base[0x10]=visitor.base[0x11]=visitor.base[0x12]=0;
        visitor.result=&result;visitor.world_mask=world_mask;visitor.deferred=&serial_list;visitor.expanding=true;   // expanding: not counted again
        // Review L4: the filter's diagnostic counts as before the guard's walk.
        // R11g (its step review's L1): the main walk's only; the rays' guard adds none of them (native_rt_gates) and runs while the main
        // walk counts on the other thread, whose counts a restore would drop.
        const uint32_t unsupported=result.unsupported.load();const uint64_t counts[4]{native_disp_children.load(),native_disp_items.load(),native_disp_unsupported_children.load(),native_asm_parts.load()};
        decal_world_query_original(world,reinterpret_cast<uintptr_t>(&visitor));
        if(!rays){result.unsupported=unsupported;native_disp_children=counts[0];native_disp_items=counts[1];native_disp_unsupported_children=counts[2];native_asm_parts=counts[3];}
        int32_t depth{};std::memcpy(&depth,visitor.base.data()+0x20,4);
        reinterpret_cast<uintptr_t(__fastcall*)(void*,uint32_t)>(image+0x234720)(&visitor,0);heap_hold.unlock();
        if(depth)fail("native_scene_selection_unbalanced_bounds");
        const char* field=serial_list.size()!=deferred.size()?"count":nullptr;
        for(size_t i=0;!field && i<deferred.size();++i){const auto& a=serial_list[i];const auto& b=deferred[i];
            if(a.candidate.entity!=b.candidate.entity || a.candidate.model!=b.candidate.model)field="entity";
            else if(a.kind!=b.kind || a.part!=b.part)field="kind";
            else if(a.candidate.placement!=b.candidate.placement || a.candidate.pose!=b.candidate.pose || a.candidate.cloth_set!=b.candidate.cloth_set)field="fields";}
        const auto n=checks[guard].fetch_add(1)+1;const auto bad=mismatches[guard].fetch_add(field?1:0)+(field?1:0);
        if(field || n<=4 || n%64==0){char row[256]{};sprintf_s(row,"{\"event\":\"native_main_walk_check\",\"walk\":\"%s\",\"checks\":%llu,\"mismatches\":%llu,\"candidates\":%zu,\"serial\":%zu,\"field\":\"%s\"}\n",
            rays?"rays":"main",static_cast<unsigned long long>(n),static_cast<unsigned long long>(bad),deferred.size(),serial_list.size(),field?field:"");Log(row);}}
    if(main_walk){static LONG64 rows{};if(const auto n=InterlockedIncrement64(&rows);n<=3 || n%600==0){LARGE_INTEGER f{};QueryPerformanceFrequency(&f);
        std::string row="{\"event\":\"native_main_walk_layers\",\"call\":"+std::to_string(n)+",\"parallel\":"+(whole_filter || count<2?"false":"true")+",\"layers\":[";
        for(size_t i=0;i<count;++i){uintptr_t vtable{};Read(layers[i],&vtable,8);char cell[120]{};
            sprintf_s(cell,"%s[%llu,%u,%zu,%.1f]",i?",":"",static_cast<unsigned long long>(vtable>=image?vtable-image:0),walks[i].visitor.objects,walks[i].gathered.size(),
                1e6*double(walks[i].ticks)/double(f.QuadPart));row+=cell;}
        row+="]}\n";Log(row.c_str());}}
}
// I-0100 tranche 2 (T2a, popin-observe.flag): one popin_native_gate row each time an entity's gate in the main walk changes
// (passed, or the gate that ended it; absent: not visited, its tree node culled or masked), with its kind, its models (a map
// piece's first children, a draw entity's binding, an assembled character's first parts, read once while it is listed) and its
// centre (a map piece's bounds E+0x40, minimum +0x40 and maximum +0x50, INFERRED layout; an entity's draw matrix, as
// popin_reference), so an object the original draws and the native never yields, or a character drawn on some frames only, is
// found with the gate that stops it.
void PopinNativeGates(NativeSelectedScene& scene,uintptr_t culler) noexcept {
    try{struct Seen {uint8_t code{};std::string names;float centre[3]{};};
        static std::unordered_map<uintptr_t,Seen> previous;
        static const char* const kinds[]{"piece","draw","character"};static const char* const gates[]{"passed","frustum","enable","visibility","category","absent","margin"};
        std::unordered_map<uintptr_t,uint8_t> now;now.reserve(scene.popin_gates.size());
        // A repeated visit: its best gate; a draw kept by the margin (gate 6) and then passed is shown as « margin » (tranche 4).
        const auto better=[](uint8_t a,uint8_t b){const bool am=(a&0xF)==6 && (b&0xF)==0,bm=(b&0xF)==6 && (a&0xF)==0;return am || (!bm && a<b);};
        for(const auto& [entity,code]:scene.popin_gates)if(const auto [at,fresh]=now.try_emplace(entity,code);!fresh && better(code,at->second))at->second=code;
        const auto names_of=[](uintptr_t entity,uint8_t kind){std::string names;const auto add=[&](uintptr_t model){if(model){if(!names.empty())names+=' ';names+=PopinName(model);}};
            if(kind==0){uintptr_t head{},node{};if(Read(entity+0x90,&head,8) && head)Read(head,&node,8);
                for(uint32_t walked=0;node && node!=head && walked<3;++walked){uintptr_t item{},model{};if(Read(node+0x10,&item,8) && item && Read(item+0x58,&model,8))add(model);if(!Read(node,&node,8))break;}}
            else if(kind==1){uintptr_t binding{},model{};if(Read(entity+0xE8,&binding,8) && binding && Read(binding+8,&model,8))add(model);}
            else{uintptr_t descriptors{},binding{},model{};uint32_t parts{};
                if(Read(entity+0xAF8,&parts,4) && Read(entity+0xB00,&descriptors,8) && descriptors)
                    for(uint32_t i=0;i<parts && i<3;++i)if(Read(descriptors+0x50ull*i+0x10,&binding,8) && binding && Read(binding+8,&model,8))add(model);}
            return names;};
        const auto centre_of=[](uintptr_t entity,uint8_t kind,float* centre){
            if(kind==0){float bounds[8]{};Read(entity+0x40,bounds,sizeof bounds);for(int k=0;k<3;++k)centre[k]=(bounds[k]+bounds[k+4])/2;}
            else{const auto place=kind==1?PopinReferencePlace(1,entity,0):PopinReferencePlace(2,0,entity);std::memcpy(centre,place.centre,12);}};
        // A draw entity's bounds E+0xB80 (minimum, maximum) beside its draw matrix's centre (Archdragon Peak, Thomas 2026-10-26: a
        // man-serpent 10 m away culled by the frustum test while drawn on screen the frames before and after).
        // The culler's six planes (+0x160) with their inside sign (a point 5 m ahead of the camera, as the shadow walk's copy), so a
        // draw centre within 1 m of the view is told apart from a centre outside it.
        float planes[6][4]{};int sign=0;
        if(Read(culler+0x160,planes,sizeof planes)){const auto& pose=scene.camera.camera_pose;const float ahead[3]{pose[12]+5*pose[8],pose[13]+5*pose[9],pose[14]+5*pose[10]};int positive{};
            for(const auto& q:planes)positive+=q[0]*ahead[0]+q[1]*ahead[1]+q[2]*ahead[2]+q[3]>=0;sign=positive==6?1:positive==0?-1:0;}
        const auto in_view=[&](const float* c){if(!sign)return -1;for(const auto& q:planes)if(sign*(q[0]*c[0]+q[1]*c[1]+q[2]*c[2]+q[3])<-1.f)return 0;return 1;};
        // I-0100 tranche 4 (Thomas, 2026-10-26 17:56: a silver knight 9 m away vanishing and coming back, culled by the frustum gate
        // with its bounds at its draw position): the same tests against the view the native draws this collect (the planes of
        // scene.camera's view-projection), so a culler whose planes are not the drawn view's
        // (late, or another camera) is told apart: drawn_box 1 when the bounds E+0xB80 cross that view, drawn_centre for the
        // draw centre (within 1 m), -1 when the view cannot be built; and the camera's position beside the culler's sign.
        float drawn[6][4]{};int drawn_sign=0;
        {SceneView v{};if(BuildSceneView(scene.camera,v)){const auto& vp=v.view_projection;const auto column=[&](size_t j){return std::array<float,4>{vp[j],vp[4+j],vp[8+j],vp[12+j]};};
            const auto c0=column(0),c1=column(1),c2=column(2),c3=column(3);
            for(size_t i=0;i<4;++i){drawn[0][i]=c3[i]+c0[i];drawn[1][i]=c3[i]-c0[i];drawn[2][i]=c3[i]+c1[i];drawn[3][i]=c3[i]-c1[i];drawn[4][i]=c2[i];drawn[5][i]=c3[i]-c2[i];}
            for(auto& p:drawn){const float n=std::sqrt(p[0]*p[0]+p[1]*p[1]+p[2]*p[2]);if(n>0)for(auto& x:p)x/=n;}
            const auto& pose=scene.camera.camera_pose;const float ahead[3]{pose[12]+5*pose[8],pose[13]+5*pose[9],pose[14]+5*pose[10]};int positive{};
            for(const auto& q:drawn)positive+=q[0]*ahead[0]+q[1]*ahead[1]+q[2]*ahead[2]+q[3]>=0;drawn_sign=positive==6?1:positive==0?-1:0;}}
        const auto drawn_centre=[&](const float* c){if(!drawn_sign)return -1;for(const auto& q:drawn)if(drawn_sign*(q[0]*c[0]+q[1]*c[1]+q[2]*c[2]+q[3])<-1.f)return 0;return 1;};
        const auto drawn_box=[&](const float* b){if(!drawn_sign)return -1;   // b: minimum b[0..2], maximum b[4..6]; outside when its farthest corner along a plane is outside
            for(const auto& q:drawn){const float s=float(drawn_sign);const float x=s*q[0]>=0?b[4]:b[0],y=s*q[1]>=0?b[5]:b[1],z=s*q[2]>=0?b[6]:b[2];
                if(s*(q[0]*x+q[1]*y+q[2]*z+q[3])<0.f)return 0;}return 1;};
        const auto row=[&](uintptr_t entity,const Seen& s,uint8_t gate){const uint8_t kind=s.code>>4;char text[760]{};float box[8]{};if(kind==1)Read(entity+0xB80,box,sizeof box);
            const auto& pose=scene.camera.camera_pose;
            sprintf_s(text,"{\"event\":\"popin_native_gate\",\"serial\":%llu,\"entity\":\"%llx\",\"kind\":\"%s\",\"gate\":\"%s\",\"models\":\"%.200s\",\"centre\":[%.2f,%.2f,%.2f],\"d\":%.2f,\"in_view\":%d,\"box\":[%.2f,%.2f,%.2f,%.2f,%.2f,%.2f],"
                "\"drawn_centre\":%d,\"drawn_box\":%d,\"camera\":[%.2f,%.2f,%.2f]}\n",
                static_cast<unsigned long long>(scene.serial),static_cast<unsigned long long>(entity),kinds[kind<3?kind:0],gates[gate<7?gate:5],s.names.c_str(),s.centre[0],s.centre[1],s.centre[2],PopinDistance(s.centre),
                in_view(s.centre),box[0],box[1],box[2],box[4],box[5],box[6],drawn_centre(s.centre),kind==1?drawn_box(box):-1,pose[12],pose[13],pose[14]);Log(text);};
        for(const auto& [entity,code]:now){auto found=previous.find(entity);
            if(found!=previous.end() && found->second.code==code)continue;
            if(found==previous.end() || (found->second.code>>4)!=(code>>4)){Seen s;s.names=names_of(entity,code>>4);found=previous.insert_or_assign(entity,std::move(s)).first;}
            found->second.code=code;centre_of(entity,code>>4,found->second.centre);row(entity,found->second,code&0xF);}
        for(auto it=previous.begin();it!=previous.end();){if(now.contains(it->first)){++it;continue;}row(it->first,it->second,5);it=previous.erase(it);}
        scene.popin_gates.clear();scene.popin_gates.shrink_to_fit();}
    catch(...){}
}
void CollectNativeSelectedScene(uintptr_t view,uintptr_t culler,uintptr_t heap,
    const SceneViewInput& camera,const ingress::SceneCameraConstants& camera_constants) {
    uintptr_t world{},manager{},table{};uint64_t policy{};
    if(!Read(view+0x40,&world,8) || !world || !Read(world,&table,8) || table!=image+0x3D20398 ||
       !Read(view+0xE0,&policy,8) || !Read(culler,&table,8) || table!=image+0x3D42D58 ||
       !Read(image+0x4796298,&manager,8) || !manager)
        Refuse("native_scene_selection_view_contract");
    // I-0141 tranche 7: the zone files' fades move here, once a world collect, before any model's lighting is asked (the second
    // review's L7: the instance rows and cbLight of one frame at one weight; Banks::AdvanceFades on FadeClock's seconds).
    {std::shared_ptr<const ds3light::Banks> banks;{const std::lock_guard hold(gparam_lock);banks=gparam_banks;}if(banks)banks->AdvanceFades();}
    // I-0141 tranche 16: a row for each key a lookup answered from an unbound area's file (Banks::UnboundAnswers; I-0138 part 4b's
    // MapOffset rule, which only such an answer reaches). At most two atomic loads a collect; the rows only when a new key came.
    {static std::atomic<uint64_t> logged{};static std::mutex logging;static size_t rows{};   // rows: under `logging`
        if(ds3light::Banks::UnboundAnswers(nullptr)!=logged.load(std::memory_order_acquire))if(const std::unique_lock hold(logging,std::try_to_lock);hold.owns_lock()){
            std::vector<ds3light::Banks::UnboundAnswer> seen;logged.store(ds3light::Banks::UnboundAnswers(&seen),std::memory_order_release);
            for(;rows<seen.size();++rows){const auto& u=seen[rows];char row[220]{};
                sprintf_s(row,"{\"event\":\"native_bank_unbound_answer\",\"area\":%u,\"id\":%d,\"group\":\"%s\",\"offset\":%s,\"fade_view\":%s}\n",
                    u.area,static_cast<int32_t>(u.id),u.group?"fog":"light_set",u.offset?"true":"false",u.fade_view?"true":"false");Log(row);}}}
    // 1183 (MENUS-MAP 62): C2EC0 sets the low category byte per engine pass (5..8 G-buffer 0x02, 9..0x38 forward 0x04;
    // 0x01 for passes outside its table). The native selection serves the G-buffer and forward rows in one walk, so it
    // visits the union; with 0x01 alone only entities carrying bit 0 in E+0x20 were visited (no objects, 2 of 6 c1100).
    // Fable review (m33: placed characters and objects never visited): all-groups.flag ignores the view's display-group words
    // (a diagnostic of the group test; never a default), and the policy is logged at each change.
    static const bool all_groups=ModuleFlag(L"all-groups.flag");
    const uint64_t mask=all_groups?(~uint64_t(0xFF))|0x07u:(policy&~uint64_t(0xFF))|0x07u;
    {static uint64_t last_policy{~0ull};static LONG64 policy_rows{};if(policy!=last_policy && InterlockedIncrement64(&policy_rows)<=24){last_policy=policy;char row[160]{};
        sprintf_s(row,"{\"event\":\"native_view_policy\",\"policy\":\"%016llX\",\"all_groups\":%s}\n",static_cast<unsigned long long>(policy),all_groups?"true":"false");Log(row);}}
    static const auto visitor_table=[] {
        std::array<uintptr_t,16> methods{};
        if(!Read(image+0x3D638D8,methods.data(),sizeof(methods)))
            Refuse("native_scene_selection_base_table_unreadable");
        methods[0x50/8]=reinterpret_cast<uintptr_t>(&AdmitNativeSelectedObject);return methods;}();
    BeginNativeLightKeys();   // I-0071 review H1: the light-set slots freed (and a reset applied) only here, between collects
    auto result=std::make_shared<NativeSelectedScene>();result->serial=camera.serial;
    result->common_images=AcquireNativeCommonImages();
    {std::lock_guard<std::mutex> hold(simulation_time_lock);result->simulation_time=native_simulation_time;}
    //O2: the options of the last render step (one read a frame, UpdateNativeOptions); the bank is the snapshot's (O1: a
    //failed read kept the last valid one; with none yet, no scene). The menu views' collect acquires its own: a render step
    //between the two can give them the newer snapshot, one frame early (accepted).
    result->options=AcquireNativeOptions();if(result->options)result->sampler_bank=NativeMaterialBank(result->options->sampler_bank);   // I-0111: the mip bias
    SnapshotNativePaint(*result);    // DECALS-PAINT-READY rung 2: before the record epochs (a new target re-admits its receiver)
    {   uintptr_t context{},root{},bank_table{};
        {const std::lock_guard<std::recursive_mutex> hold(scene_view_cpu_lock);
            const auto found=scene_view_cpu_owners.find(view);if(found!=scene_view_cpu_owners.end())context=found->second.context;}
        NativeSelectedScene::PerFrameRows rows;
        if(context && Read(image+0x4961B50,&root,8) && root && Read(root+0x4680,&bank_table,8) && bank_table && Read(context+0x3FD0,&rows.blend,4)){
            const auto lookup=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uintptr_t)>(image+0x134A20);
            const uintptr_t first=lookup(bank_table,context+0x4110),second=lookup(bank_table,context+0x4118);
            if(first && second && Read(first+8,rows.a.data(),44) && Read(second+8,rows.b.data(),44)){
                // Lightmap floors (Reference1844 against Native1845 at m37: the same keys, the original's rows the area file's
                // `Dominant Light` group, the native bank's its built-in record): natively the engine's bank [root+0x4680] never
                // receives a file (49D00 is bookkeeping), so each key's record is read from the game's gparam files; a key whose
                // layers have no such group keeps the built-in record, as the original's lookup does.
                std::shared_ptr<const ds3light::Banks> banks;{const std::lock_guard hold(gparam_lock);banks=gparam_banks;}
                uint32_t keys[4]{};std::string sources[2];
                const bool keyed=banks && Read(context+0x4110,keys,8) && Read(context+0x4118,keys+2,8);
                // Review (cost): the rows of the last keys and bank, kept by the collecting thread; the bank lookups only on a change.
                struct DominantCache {std::shared_ptr<const ds3light::Banks> banks;uint64_t generation{};std::array<uint32_t,4> keys{};std::array<float,11> a{},b{};bool from_a{},from_b{};std::string sources[2];};
                thread_local DominantCache cache;
                const uint64_t generation=keyed?banks->RecordGeneration():0;   // pre-merge review: a layer or area binding change with the same keys renews the rows; I-0141 tranche 23: a fade's steps too
                if(keyed && (cache.banks!=banks || cache.generation!=generation || !std::equal(cache.keys.begin(),cache.keys.end(),keys))){cache.banks=banks;cache.generation=generation;   // review: held, so a reloaded bank at the same address is seen
                    std::copy(keys,keys+4,cache.keys.begin());   // Fable review: this statement had been swallowed by the comment above (the cache never hit)
                    cache.a=rows.a;cache.b=rows.b;cache.from_a=banks->DominantAt(keys[0],keys[1],cache.a,&cache.sources[0]);cache.from_b=banks->DominantAt(keys[2],keys[3],cache.b,&cache.sources[1]);}
                const bool from_a=keyed && cache.from_a,from_b=keyed && cache.from_b;
                if(from_a){rows.a=cache.a;sources[0]=cache.sources[0];}if(from_b){rows.b=cache.b;sources[1]=cache.sources[1];}
                result->per_frame_rows=rows;
                // I-0113 tranche 34: the pass context's +0x110..+0x118 (140204C30) for every consumer: the effects, the meshes, the
                // decals, the model particles and the rays' instance ambient.
                {float scale[4]{};ds3prod::ProducePassDominantScale(rows.a.data(),rows.b.data(),rows.blend,scale);result->pass_row_scale={scale[0],scale[1],scale[2]};}
                static LONG64 dominant_source{};static std::array<uint32_t,6> last{};const auto n=InterlockedIncrement64(&dominant_source);
                const std::array<uint32_t,6> now{keys[0],keys[1],keys[2],keys[3],from_a?1u:0u,from_b?1u:0u};
                if(n<=2 || now!=last){last=now;char row[400]{};sprintf_s(row,"{\"event\":\"native_dominant_source\",\"keys\":[%u,%u,%u,%u],\"a\":\"%.32s\",\"b\":\"%.32s\",\"color_scale\":[%g,%g],\"pass_row_scale\":[%g,%g,%g]}\n",
                    keys[0],keys[1],keys[2],keys[3],from_a?sources[0].c_str():"builtin",from_b?sources[1].c_str():"builtin",rows.a[3],rows.b[3],result->pass_row_scale[0],result->pass_row_scale[1],result->pass_row_scale[2]);Log(row);}
            }
            // Image regression lead L2 (IMAGE-REGRESSION-STATIC-LEADS): the Dominant Light rows the engine's bank returns (natively the
            // bank never receives a file), with their keys, against the gparam file's group for the key.
            {static LONG64 dominant_witness{};const auto n=InterlockedIncrement64(&dominant_witness);
                if(first && second && (n<=2 || n%1800==0)){uint32_t keys[4]{};Read(context+0x4110,keys,8);Read(context+0x4118,keys+2,8);
                    std::array<uint32_t,0x60> block{};Read(context+0x3FC0,block.data(),sizeof(block));   // the same key block as reference_dominant_rows
                    char row[600]{};sprintf_s(row,"{\"event\":\"native_dominant_rows\",\"keys\":[%u,%u,%u,%u],\"blend\":%.4g,\"a\":[%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g],"
                        "\"b\":[%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g],\"block\":[",keys[0],keys[1],keys[2],keys[3],rows.blend,
                        rows.a[0],rows.a[1],rows.a[2],rows.a[3],rows.a[4],rows.a[5],rows.a[6],rows.a[7],rows.a[8],rows.a[9],rows.a[10],
                        rows.b[0],rows.b[1],rows.b[2],rows.b[3],rows.b[4],rows.b[5],rows.b[6],rows.b[7],rows.b[8],rows.b[9],rows.b[10]);
                    const std::string text=row+NativeWordsList(block.data(),block.size())+"]}\n";Log(text.c_str());}}
        }
        // I-0105 (the surface snow): the `Snow` records of the context's pair +3FC0/+3FC8 (blend +3FD0) as 23790 takes them through
        // 12F400 ([root+0x45F8]); natively that bank never receives a file (as the Dominant bank's above), so each key's record comes
        // from the gparam files and, for a key no layer holds, from the engine bank's built-in record (12F400's +0x300: the record
        // 131780 reads at +0x10). The `Snow Model` bank's built-in rate (46300's +0x118, +8) for the draw states' keys.
        {uintptr_t snow_root{},snow_bank{},model_bank{};float blend{};uint32_t keys[4]{};
            if(context && Read(image+0x4961B50,&snow_root,8) && snow_root && Read(snow_root+0x45F8,&snow_bank,8) && snow_bank &&
               Read(snow_root+0x4600,&model_bank,8) && model_bank && Read(context+0x3FD0,&blend,4) && Read(context+0x3FC0,keys,16)){
                std::shared_ptr<const ds3light::Banks> banks;{const std::lock_guard hold(gparam_lock);banks=gparam_banks;}
                // The rows of the last keys, blend and bank, kept by the collecting thread (as the Dominant rows' cache).
                struct SnowCache {std::shared_ptr<const ds3light::Banks> banks;uint64_t generation{},binding{};std::array<uint32_t,4> keys{};float blend{};bool valid{};NativeSelectedScene::SnowRows snow;
                    std::array<std::wstring,5> names_a,names_b;};   // tranche 10: each record's five texture names (NativeSnowRecordNames), in t47..t51's order
                thread_local SnowCache cache;
                const uint64_t generation=banks?banks->RecordGeneration():0;   // I-0141 tranche 23: a fade's steps renew the snow rows
                // The names' inputs (step review L4): the bindings, not a fade's steps (Texture:ID is declared without the fade; the final
                // review's L4 of I-0141 tranche 23).
                const uint64_t binding=banks?banks->BindingGeneration():0;
                const bool names_stale=!cache.valid || cache.banks!=banks || cache.binding!=binding || !std::equal(cache.keys.begin(),cache.keys.end(),keys);
                if(!cache.valid || cache.banks!=banks || cache.generation!=generation || cache.blend!=blend || !std::equal(cache.keys.begin(),cache.keys.end(),keys)){
                    cache.valid=false;cache.banks=banks;cache.generation=generation;cache.binding=binding;cache.blend=blend;std::copy(keys,keys+4,cache.keys.begin());
                    std::array<uintptr_t,2> records{};
                    const auto builtin=[&](uintptr_t key,std::array<float,16>& out,uintptr_t& record){
                        const uintptr_t at=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uintptr_t)>(image+0x12F400)(snow_bank,key);
                        std::array<float,0x38> f{};if(!at || !Read(at+0x10,f.data(),sizeof(f)))return false;record=at+0x10;
                        constexpr uint8_t from[16]{0x60,0x64,0x90,0x94,0x80,0xD0,0xD4,0xB4,0xB8,0xBC,0xA0,0xA4,0xA8,0xAC,0xB0,0xC0};
                        for(size_t i=0;i<16;++i)out[i]=f[from[i]/4];return true;};
                    const uint32_t no_key[2]{0xFFFFFFFFu,0xFFFFFFFFu};
                    const uintptr_t model_at=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uintptr_t)>(image+0x46300)(model_bank,reinterpret_cast<uintptr_t>(no_key));
                    std::array<float,16> a{},b{};std::string sources[2]{"builtin","builtin"};
                    if(builtin(context+0x3FC0,a,records[0]) && builtin(context+0x3FC8,b,records[1]) && model_at && Read(model_at+8,&cache.snow.model_builtin,4)){
                        int32_t ids[2]{};bool filed[2]{};
                        if(banks){filed[0]=banks->SnowAt(keys[0],keys[1],a,&sources[0],&ids[0]);filed[1]=banks->SnowAt(keys[2],keys[3],b,&sources[1],&ids[1]);}
                        // A file record (an area key, not `default`'s) is named by its area's stem, otherwise the engine's built-in record by its own.
                        if(names_stale)for(size_t s=0;s<2;++s){const uint32_t area=keys[2*s];auto& names=s?cache.names_b:cache.names_a;
                            if(filed[s] && area>1 && sources[s]!="default")names=NativeSnowRecordNames(NativeSnowAreaPrefix(area),ids[s]);
                            else{int32_t id{};Read(records[s]+0x58,&id,4);names=NativeSnowRecordNames(ReadEngineWideString(records[s]+0x28),id);}}
                        std::array<uint8_t,0xB0> block{};std::array<uint8_t,0xB0/4> marks{};
                        cache.snow.scene_rate=ds3prod::ProduceSnow(a.data(),b.data(),blend,block.data(),static_cast<uint32_t>(block.size()),marks.data());
                        std::memcpy(cache.snow.rows.data(),block.data()+0x60,0x50);cache.valid=true;
                        // Review L2: a row when the keys change or the blend reaches an end (a transition moves it every frame).
                        static std::array<uint32_t,4> logged_keys{};static LONG64 snow_rows_logged{};const bool settled=blend==0 || blend==1;
                        if((settled || !std::equal(logged_keys.begin(),logged_keys.end(),keys)) && InterlockedIncrement64(&snow_rows_logged)<=64){std::copy(keys,keys+4,logged_keys.begin());
                            const auto& w=cache.snow.rows;char row[700]{};
                            sprintf_s(row,"{\"event\":\"native_snow_rows\",\"keys\":[%u,%u,%u,%u],\"a\":\"%.32s\",\"b\":\"%.32s\",\"blend\":%.4g,\"scene_rate\":%.4g,\"model_builtin\":%.4g,\"rows\":[%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g]}\n",
                                keys[0],keys[1],keys[2],keys[3],sources[0].c_str(),sources[1].c_str(),blend,cache.snow.scene_rate,cache.snow.model_builtin,
                                w[0],w[1],w[2],w[3],w[4],w[5],w[6],w[7],w[8],w[9],w[10],w[11],w[12],w[13],w[14],w[15],w[16],w[17],w[18],w[19]);
                            Log(row);}}}
                if(cache.valid){result->snow_rows=cache.snow;
                    // 131780: a's texture objects at a blend of 0 (within its epsilon), b's otherwise (its other end and between).
                    result->snow_textures=CollectNativeSnowTextures(*result,snow_root,NativeSnowRecordHolders(std::fabs(blend)<=1.1920929e-07f?cache.names_a:cache.names_b));}}}
        // 1051 witness for the tone map family (lighting map): ToneMap A/B keys
        // +3FE0/+4048, ratio +40AC, bank [root+4618], the pass block +3080.. (FB890).
        {static LONG64 tone_witness{};const auto n=InterlockedIncrement64(&tone_witness);
            if(context && root && (n==2 || (n%120==0 && n<=1800))){std::array<uint32_t,4> keys{};float ratio{};uintptr_t bank{};std::array<float,8> block{};
                Read(context+0x3FE0,&keys[0],8);Read(context+0x4048,&keys[2],8);Read(context+0x40AC,&ratio,4);Read(root+0x4618,&bank,8);Read(context+0x3080,block.data(),32);
                char row[400]{};sprintf_s(row,"{\"event\":\"native_tonemap_inputs\",\"keys\":[%u,%u,%u,%u],\"ratio\":%.4g,\"bank\":%s,\"block\":[%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g,%.4g]}\n",
                    keys[0],keys[1],keys[2],keys[3],ratio,bank?"true":"false",block[0],block[1],block[2],block[3],block[4],block[5],block[6],block[7]);Log(row);}}
    }
    SnapshotNativeDecals(*result);   // DECALS-WAVE1 D1; I-0113 tranche 34: after the Dominant rows (their pass_row_scale)
    result->wet_shader_catalog=NativeWetShaderCatalog();
    {std::array<uint8_t,16> row{};if(Read(image+0x4929B20,row.data(),row.size()))result->default_param_row=row;}   // cP6
    BYTE variant_enabled{},category_override{};bool split_engine5{};float category_scalar{};
    if(Read(image+0x473D39E,&variant_enabled,1))result->global_variant_enabled=variant_enabled!=0;
    if(Read(image+0x4962314,&category_override,1))result->category_override=category_override!=0;
    if(ingress::ReadModelSplitPassSetting(image,split_engine5))result->split_engine5=split_engine5;
    if(Read(image+0x3D7EE10,&category_scalar,4))result->category_scalar=category_scalar;
    //Explicit initial native policy: ordinary deferred, no alpha-coverage or
    //forced-plain mode. This new engine contract is not an acquired value of
    //the removed worker's714 or a claim of AA-setting parity.
    result->pixel_policy=ds3content::NativeModelPixelPolicy::Default;
    result->camera=camera;result->camera_constants=camera_constants;
    result->taa=NativeTaaActive();   // I-0132 R11j
    result->dlss=NativeDlssActive();   // final review M1: the frame's decision the velocity, the prepare and the renderer read (the BeginFrame value)
    if(!ingress::ProduceCameraViewConstants(camera,camera_constants,result->view_constants))
        Refuse("native_selected_view_constant_producer_failed");
    CollectNativeVelocityInputs(*result,view);
    CollectNativeGlowInputs(*result,view);   // W5d GL-3 (row native_glow_inputs)
    CollectNativeYebisInputs(*result,view);  // W10 YB-3/YB-4 (on by default, no-yebis.flag; row native_yebis_globals)
    // FFX draw N5a (on by default, no-ffx-draw.flag; ffx-record.flag adds the witness rows). I-0088 P2: on the side worker beside
    // the walk, joined before the main walk's admission (I-0105: its model particles are candidates of it). Of the scene it writes
    // the effect frame and the model particles only, which nothing reads before that join; its engine calls are the effect system's, which no other collect part calls during the DrawStep row;
    // one process-wide write remains, E07's settings shadow at [[GXRS]+0x4DD0] during Execute, which the module's readers take as
    // absent (NativeDeviceAbsent); an engine reader of that member on the pool during the walk is not known (final review M1).
    const bool effects_aside=collect_side_worker && collect_side_worker->Running() && !control->stop;
    if(effects_aside)collect_side_worker->Post([result,view]{
        if(const auto hook=WorkPool::worker_hook.load(std::memory_order_relaxed))hook("side");   // a sample-profile run sees the worker
        RecordNativeEffects(*result,view);});
    else RecordNativeEffects(*result,view);
    // Diagnostic (D-0180 C4 cost at m30): wall-clock of the collect's sections, logged every 600th collect (native_collect_sections).
    static double section_ms[8]{};static LONG64 section_collects{};LARGE_INTEGER section_mark{},section_freq{};QueryPerformanceFrequency(&section_freq);
    static double rays_wait_ms{},rays_span_ms{};static LONG64 rays_waits{};   // I-0132 R11g: the join's wait and the walk's span from its post
    const auto section=[&](int i){LARGE_INTEGER now{};QueryPerformanceCounter(&now);if(i>=0)section_ms[i]+=1000.0*double(now.QuadPart-section_mark.QuadPart)/double(section_freq.QuadPart);section_mark=now;};
    section(-1);
    CollectNativeShadowState(*result,view);   // W5a SW-1/SW-2: the shadow frame (the traced sun, the shadow words) and its row
    section(0);
    {const std::lock_guard carry(native_block_carry.lock);auto& c=native_block_carry;
        if(c.scene_valid && c.camera==camera_constants.bytes && c.written==camera_constants.written && c.view==result->view_constants.bytes){
            result->scene_block_table.carried=c.scene_table;result->view_block_table.carried=c.view_table;result->planned_programs.carried=c.planned;kprofile::Add(kprofile::SceneTablesCarried);}}
    CaptureNativeRecordEpochs(*result);   // C7-2: before the walk
    result->regions=CollectNativeRegionSnapshot();   // I-0113 tranche 14: the region IBL's manager copy, before the walk
    {static uint64_t last_sweep{};if(result->serial>=last_sweep+600){last_sweep=result->serial;   // entities unseen for 600 collects
        native_region_memos.EraseIf([&](const auto& entry){return entry.second.serial+600<result->serial;});}}
    // m45 collect cost: the models vector sized from the previous collect's two walks (no regrowth moves of 1,667 models).
    static std::atomic<size_t> models_hint{};result->models.reserve(models_hint.load(std::memory_order_relaxed)*9/8);
    // I-0071 point 3 (ER A): the walk gathers the candidates; they are admitted on the pool afterwards, merged in walk order.
    // I-0077: the walk itself by layers on the pool (WalkNativeLayers); this visitor only carries the scene and the world
    // mask to the admission.
    NativeSelectionVisitor visitor;visitor.result=result.get();
    visitor.world_mask=reinterpret_cast<uint32_t(__fastcall*)(uintptr_t)>(image+0xDFD240)(manager);
    // The exact C1FB0 CPU trampoline retained by decal ownership (the shadow walk and the guard call it).
    if(!decal_world_query_original)Refuse("native_scene_selection_world_entry_missing");
    std::vector<NativeDeferredCandidate> deferred;
    static NativeWalkHistory history;
    // I-0100 tranche 4 (T2b, NativeCullMargin): the main walk's culler, a copy of the view's whose planes (+0x160, normalised, their
    // inside sign read at a point five metres ahead of the camera, as the shadow walks' copies) and world box
    // (+0x220/+0x230) are pushed out by kNativeCullMargin; the view's own when the sign is mixed or with no-cull-margin.flag.
    alignas(16) std::array<uint8_t,0x300> widened{};uintptr_t walk_culler=culler;
    {static const bool off=ModuleFlag(L"no-cull-margin.flag");NativeCullMargin margin;margin.real=culler;
        if(!off && Read(culler,widened.data(),widened.size())){const auto& pose=camera.camera_pose;const float ahead[3]{pose[12]+5*pose[8],pose[13]+5*pose[9],pose[14]+5*pose[10]};
            float planes[6][4]{};std::memcpy(planes,widened.data()+0x160,sizeof planes);int positive{},negative{};bool finite=true;
            for(const auto& q:planes){(q[0]*ahead[0]+q[1]*ahead[1]+q[2]*ahead[2]+q[3]>=0?positive:negative)++;const float n=std::sqrt(q[0]*q[0]+q[1]*q[1]+q[2]*q[2]);if(!(n>0.f) || !std::isfinite(n))finite=false;}
            if(finite && (positive==6 || negative==6)){const float s=positive==6?1.f:-1.f;
                for(auto& q:planes){const float n=std::sqrt(q[0]*q[0]+q[1]*q[1]+q[2]*q[2]);for(auto& v:q)v/=n;q[3]+=s*kNativeCullMargin;}
                std::memcpy(widened.data()+0x160,planes,sizeof planes);
                float lo[4]{},hi[4]{};std::memcpy(lo,widened.data()+0x220,16);std::memcpy(hi,widened.data()+0x230,16);
                for(int k=0;k<3;++k){lo[k]-=kNativeCullMargin;hi[k]+=kNativeCullMargin;}
                std::memcpy(widened.data()+0x220,lo,16);std::memcpy(widened.data()+0x230,hi,16);
                walk_culler=reinterpret_cast<uintptr_t>(widened.data());margin.active=true;margin.sign=s;   // I-0141 t22: the margin pieces' row
                const float f=std::sqrt(pose[8]*pose[8]+pose[9]*pose[9]+pose[10]*pose[10]);if(f>0.f)for(int k=0;k<3;++k)margin.forward[k]=pose[8+k]/f;}}
        native_cull_margin=margin;}
    // I-0118 (D-0237): the sun's shadow traced: the main view with the rays on, the traced pass on and the rays not failed (I-0122 R4b:
    // the local lights alike). I-0132 R11g: set before the rays' walk (renderer state only; the walk changes neither), as the local
    // shadows' selection below reads `local_traced`.
    const bool rays_walk=native_rt_scene.load(std::memory_order_relaxed) && !native_offscreen_preparing;
    if(rays_walk){result->sun_traced=renderer.rt_shadow.load(std::memory_order_relaxed) && !renderer.RtSceneFailed();
        // I-0129 R9c (Thomas, 2026-11-08 08:55: no DS3 option may cut an input of the rays): the shadow frame `on` under the traced sun,
        // whatever Shadow Quality's on byte (+0x28: off, the frame's sun, scene words and FC_ShadowColor were dropped from the rays and
        // the sun went unshadowed, no cascade being drawn under the traced sun); a row each time the force changes it (at most 20).
        if(result->sun_traced && result->shadow_frame && !result->shadow_frame->on){result->shadow_frame->on=true;
            static LONG64 forced{};if(InterlockedIncrement64(&forced)<=20)Log("{\"event\":\"native_rt_sun_option_forced\",\"shadow_quality_on\":0,\"on\":1}\n");}
        result->local_traced=renderer.rt_lights.load(std::memory_order_relaxed) && !renderer.RtSceneFailed();}
    // I-0132 R11g (R11-PLAN §8: the walk asynchronous within the frame): the rays' walk (CollectNativeRtScene) on its own thread
    // (collect_rays_worker) beside the main walk, its admission, the passes' preparation and the lights, joined before the rays'
    // forward blocks, its scene's first reader on this thread; never across the engine's next update (it reads the entities and
    // poses the update writes). Its inputs on the scene (the camera, the sampler bank, the per-frame rows, the time, the pass row
    // scale) are all set above; of the scene it writes only rt_scene, rt_scene_input and rt_forward_groups, which nothing here reads
    // before the join (its module registries, the materials, skinned sets, lightmaps, programme failures and last scene, are locked;
    // effect_probe::on is atomic). It now overlaps the effect record on the side worker too: the side worker's open point (E07's
    // settings shadow written during Execute, an engine reader of it on another thread not known) extends to the rays' engine calls
    // (the forward colour's provider, the programme builds), as it did to the main walk's (its step review's L5). Both walks' pool jobs share the admission pool's workers (WorkPool::Run, R11g); their traversers' engine heap is
    // taken by the module's own calls one thread at a time (native_traverser_heap_lock). Two differences from the walk after the admission,
    // one frame each: a light key the admission registers for the first time in this collect may be found by the rays only from the next
    // one (NativeLightSetFound: set 0 for that instance); a programme set the admission builds in this collect may be built again by the
    // rays (against their budget) or deferred, its instance without a material for that walk (the final review's L6). no-rays-aside.flag (A/B) walks on this thread after the admission as before.
    const bool rays_aside=rays_walk && collect_rays_worker && collect_rays_worker->Running() && !collect_rays_worker->Faulted() && !control->stop;
    // The outcome held by the job too (its step review's M1: never a write into this frame after it is gone).
    const auto rays_outcome=std::make_shared<NativeRtWalkOutcome>();LARGE_INTEGER rays_posted{};
    // The join on every way out of this collect (an exception thrown below unwinds through it): the walk never outlives the collect. An
    // exception leaving the collect ends in its caller's refusal, which closes the process (14: native_scene_cpu_update_failed). The
    // worker's own wait throws at its deadline (30 s) with the walk still running (its step review's M1): that failure goes on at once
    // to the refusal's row and crash dump (the final review's L5: waiting longer would hang the game without either); the walk holds
    // its scene and outcome itself (shared), never this frame's.
    struct RaysJoin {RenderWorker* worker;bool posted{};
        void Wait(){if(!posted)return;posted=false;worker->Wait();}
        ~RaysJoin(){if(posted)try{worker->Wait();}catch(...){}}} rays_join{collect_rays_worker};
    if(rays_aside){QueryPerformanceCounter(&rays_posted);
        collect_rays_worker->Post([result,heap,mask,culler,world,manager,rays_outcome]{
            if(const auto hook=WorkPool::worker_hook.load(std::memory_order_relaxed))hook("rays");   // a sample-profile run sees the worker
            *rays_outcome=CollectNativeRtScene(*result,heap,mask,culler,world,manager,visitor_table.data());});   // I-0117 (R2 T1)
        rays_join.posted=true;}
    WalkNativeLayers(world,heap,mask,walk_culler,visitor.world_mask,visitor_table.data(),*result,deferred,history,true);
    if(effects_aside)collect_side_worker->Wait();   // I-0088 P2, I-0105: the effect frame and its model particles complete
    const size_t walk_candidates=deferred.size();
    AppendNativeEffectCandidates(*result,deferred);
    AdmitNativeDeferredCandidates(visitor,deferred);
    // I-0100 tranche 2-3 (T2a; Thomas 2026-10-26 07:01: distant enemies animated at about 2 frames/s natively, normally in the
    // original): the original's main-view draw (pass 0x45) marks the object drawn in an 8-slot ring, the byte's bit of the slot
    // byte+1 names: D3DA20 for a map piece item (item+0x794, slot +0x795), D079C0 for a draw entity (E+0xC50, slot +0xC51; the asm
    // 140D07DC3, Ghidra's longlong* indexing had read +0x18A) and D02A30 for an assembled character's parts (E+0xCB0, slot +0xCB1).
    // DS3 moves the slot each frame and clears the slot it enters (popin_drawn_bytes, RESULT tranche 3 step 2): a mark expires.
    // The native draws without those calls, so it marks every object its main walk admitted. Measured (Anor Londo,
    // i100-t3c/t3d-pose-*): without the marks DS3 never updated the distant characters' poses natively; with them, in view at every
    // frame and out of view at about 2 Hz, as the original. The readers (tranche 5 step 1): the character manager's update-rate step
    // 1408DDFA0 through the characters' slot-114/115 pair, and for the item ring the map object's update 14066BBB0, which hands an
    // animated object's pose to its model only while its item is marked (DS3 moves the item's slot only for those objects, 1406667C0;
    // a map piece's bit is never read). D3DA20's early exit when 812DF0 answers is not reproduced (false in the measured runs), nor
    // DS3's marks of what the native cannot draw yet. On by default; no-drawn-bits.flag opts out.
    {static const bool enabled=!ModuleFlag(L"no-drawn-bits.flag");
        if(enabled){uintptr_t previous_entity{},previous_owner{};std::string cells;const bool witness=PopinObserve() && result->serial%60==0;
            const auto mark=[&](uintptr_t object,uint32_t ring,unsigned kind){BYTE slot{},before{};if(!Read(object+ring+1,&slot,1))return;
                if(witness && std::count(cells.begin(),cells.end(),'[')<12 && Read(object+ring,&before,1)){char cell[80]{};
                    sprintf_s(cell,"%s[%u,%u,%u]",cells.empty()?"":",",kind,before,slot);cells+=cell;}   // review: the ring before the write
                OrNativeEngineByte(object+ring,static_cast<BYTE>(1u<<(slot&31)));};
            for(const auto& m:result->models){uintptr_t vtable{};if(m.effect)continue;   // consecutive models share their entity and owner: each marked once (I-0105: a model particle has no entity)
                if(m.entity && m.entity!=previous_entity){previous_entity=m.entity;if(Read(m.entity,&vtable,8) && vtable==image+0x2951FF8)mark(m.entity,0x794,0);}
                if(m.owner && m.owner!=previous_owner){previous_owner=m.owner;
                    if(Read(m.owner,&vtable,8)){if(vtable==image+0x2948EF8)mark(m.owner,0xC50,1);else if(vtable==image+0x2947DB8)mark(m.owner,0xCB0,2);}}}
            if(!cells.empty())Log(("{\"event\":\"popin_drawn_bytes\",\"serial\":"+std::to_string(result->serial)+",\"marks\":["+cells+"]}\n").c_str());}}
    // I-0100 T0 (popin-observe.flag): the main walk's refusals, kept for the prepare's per-entity rows (PopinNativeFrame).
    if(PopinObserve()){std::unordered_set<uint64_t> admitted;admitted.reserve(result->models.size());
        for(const auto& m:result->models)admitted.insert(uint64_t(m.entity)^(uint64_t(m.model)<<1));
        for(const auto& d:deferred)if(!admitted.contains(uint64_t(d.candidate.entity)^(uint64_t(d.candidate.model)<<1)))
            result->popin_refused.push_back({d.candidate.entity,d.candidate.model,d.candidate.fade_owner,d.code?d.code:uint8_t(6)});
        PopinNativeGates(*result,culler);}
    // m45 collect cost: the main walk's outcomes (entities visited, models admitted, candidates refused by cause), sampled.
    {static LONG64 rows{};if(const auto n=InterlockedIncrement64(&rows);n<=3 || n%600==0){char row[400]{};
        sprintf_s(row,"{\"event\":\"native_main_walk_outcomes\",\"call\":%lld,\"objects\":%u,\"models\":%zu,\"zero_fade\":%u,\"missing_bindings\":%u,"
            "\"instance_streams\":%u,\"missing_geometry\":%u,\"no_meshes\":%u,\"nonzero_variant\":%u,\"unsupported\":%u,\"nonfinite\":%u}\n",static_cast<long long>(n),
            result->objects.load(),result->models.size(),result->zero_fade.load(),result->missing_bindings.load(),result->instance_streams.load(),
            result->missing_geometry.load(),result->no_meshes.load(),result->nonzero_variant.load(),result->unsupported.load(),result->nonfinite.load());Log(row);}
        static const bool marks=NativeAdmissionMarksOn();static LONG64 collects{};
        if(marks){FlushNativeAdmissionMarks();++collects;if(collects>=600){LARGE_INTEGER f{};QueryPerformanceFrequency(&f);std::string text="{\"event\":\"native_admission_sections\",\"collects\":"+std::to_string(collects)+",\"ms_per_collect\":[";
            for(size_t i=0;i<12;++i){char cell[48]{};sprintf_s(cell,"%s%.3f",i?",":"",1000.0*double(InterlockedExchange64(&native_admission_ticks[i],0))/double(f.QuadPart)/double(collects));text+=cell;}
            text+="],\"marks_per_collect\":[";for(size_t i=0;i<12;++i){char cell[32]{};sprintf_s(cell,"%s%.1f",i?",":"",double(InterlockedExchange64(&native_admission_counts[i],0))/double(collects));text+=cell;}
            text+="]}\n";Log(text.c_str());collects=0;}}}
    // I-0132 R11g: no-rays-aside.flag (or no rays worker) walks here, on this thread, as before R11g.
    if(rays_walk && !rays_aside)*rays_outcome=CollectNativeRtScene(*result,heap,mask,culler,world,manager,visitor_table.data());   // I-0117 (R2 T1)
    // I-0106: the local-light shadows' selection (slot 3 on the container) before the walks. On by default (no-local-shadows.flag opts
    // out). I-0131 R10a: the selection only: its lights keep their shadowed records for the rays (RtShadowedOf); no caster is walked
    // and no atlas drawn.
    {uintptr_t light_context{};{const std::lock_guard<std::recursive_mutex> hold(scene_view_cpu_lock);
            const auto found=scene_view_cpu_owners.find(view);if(found!=scene_view_cpu_owners.end())light_context=found->second.context;}
        CollectNativeLocalShadows(*result,light_context);}
    models_hint.store(result->models.size(),std::memory_order_relaxed);   // both walks
    result->light_keys=NativeLightKeys();   // I-0071: every set index the walks gave (NativeLightSetIndex)
    section(1);   // the carry, record epochs and the main walk
    // I-0091: the rows' places sized once, by the parallel preparation (no hint, no regrowth).
    {const kprofile::Scope k(kprofile::Passes);PrepareNativeSelectedPasses(*result,true);}   // I-0071 point 3b: on the pool
    section(2);
    // I-0141 tranche 17 (pixel-keys.flag; the final review's M3): every 300th main collect, the models with a G-buffer row in it
    // (engine passes 5 to 7, the rows' bit 0 of the census of model rows earlier in this file; the step review's M2),
    // native_pixel_key_gbuffer, so native_pixel_key's models (every model that takes a set, forward ones included) can be restricted
    // to those whose pixels carry GBuffer3.w (a model drawn only between two samples is missed).
    {static const bool pixel_keys=ModuleFlag(L"pixel-keys.flag");static LONG64 calls{};if(pixel_keys && InterlockedIncrement64(&calls)%300==1){
        std::vector<uint8_t> drawn(result->models.size());for(const auto& p:result->prepared_meshes)if(p.engine_pass>=5 && p.engine_pass<=7 && p.model<drawn.size())drawn[p.model]=1;
        std::set<std::string> names;for(size_t i=0;i<drawn.size();++i)if(drawn[i]){std::string clean;
            for(const auto ch:NativeModelDataName(result->models[i].model))clean.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');names.insert(clean.substr(0,60));}
        char head[96]{};sprintf_s(head,"{\"event\":\"native_pixel_key_gbuffer\",\"qpc_ms\":%.1f,\"models\":[",PixelKeyClockMs());std::string row=head;bool first=true;
        for(const auto& n:names){row+=first?"\"":",\"";row+=n;row+="\"";first=false;}
        row+="]}\n";Log(row.c_str());}}
    // I-0141 tranche 24 (pixel-keys.flag, « Suggested next step » 4): every 30th main collect, once per 60-second window (PixelKeyWindow)
    // and (model, engine pass) of the prepared meshes, native_model_pass_window: the passes the native draws each model in, by window,
    // against the original's reference_model_pass_window (a model drawn only between two samples is missed). Keyed by the model's
    // address within a window; at most 20,000 rows a window (the step review's M1), then one native_model_pass_window_full row naming it.
    {static const bool pixel_keys=ModuleFlag(L"pixel-keys.flag");static LONG64 calls{};
        if(pixel_keys && InterlockedIncrement64(&calls)%30==1){const double ms=PixelKeyClockMs();
            static std::mutex lock;static std::set<std::pair<uintptr_t,uint32_t>> seen;static uint64_t window{};static size_t written{};
            const std::lock_guard held(lock);const auto now=PixelKeyWindow(ms);if(now>window){window=now;seen.clear();written=0;}
            for(const auto& p:result->prepared_meshes){if(p.model>=result->models.size() || written>=20000)continue;const uintptr_t model=result->models[p.model].model;
                if(!seen.emplace(model,p.engine_pass).second)continue;std::string clean;
                for(const auto ch:NativeModelDataName(model))clean.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');char row[200]{};
                sprintf_s(row,"{\"event\":\"native_model_pass_window\",\"qpc_ms\":%.1f,\"model\":\"%.60s\",\"pass\":%u}\n",ms,clean.c_str(),unsigned(p.engine_pass));Log(row);
                if(++written>=20000){sprintf_s(row,"{\"event\":\"native_model_pass_window_full\",\"qpc_ms\":%.1f,\"window\":%llu,\"rows\":20000}\n",ms,static_cast<unsigned long long>(window));Log(row);}}}}
    // Diagnostic (m30 map pieces missing from the G-buffer, Native1711/1754): every 600th collect, the main-view map pieces
    // (data name m...) without a G-buffer row (engine pass 5), with their fade variants and loaded LOD mask.
    {static const bool census=ModuleFlag(L"light-observe.flag");static LONG64 calls{};if(census && InterlockedIncrement64(&calls)%600==300){std::vector<uint32_t> rows(result->models.size());
        for(const auto& p:result->prepared_meshes)if(p.engine_pass==5)++rows[p.model];
        size_t pieces{},without{};std::string names;
        for(size_t i=0;i<result->models.size();++i){const auto& m=result->models[i];const auto name=NativeModelDataName(m.model);
            if(name.empty() || name[0]!='m')continue;++pieces;if(rows[i])continue;++without;
            if(names.size()<1500){char cell[200]{};sprintf_s(cell,"%s{\"name\":\"%.40s\",\"fades\":%u,\"v0\":%d,\"lod_mask\":%u,\"meshes\":%zu,\"cat\":%d}",names.empty()?"":",",name.c_str(),
                m.geometry_fades.count,m.geometry_fades.count?m.geometry_fades.intents[0].variant:-9,m.lod_mask,m.meshes.size(),m.ordinary_category?int(*m.ordinary_category):-1);names+=cell;}}
        std::string row="{\"event\":\"native_map_piece_rows\",\"pieces\":"+std::to_string(pieces)+",\"without_gbuffer\":"+std::to_string(without)+",\"refusals\":{";
        bool first=true;for(size_t code=0;code<std::size(result->refusals);++code)if(result->refusals[code]){row+=std::string(first?"\"":",\"")+std::to_string(code)+"\":"+std::to_string(result->refusals[code]);first=false;}
        row+="},\"examples\":["+names+"]}\n";Log(row.c_str());}}
    const kprofile::Scope lights(kprofile::Lights);
    CollectNativeLightBlocks(*result);
    {uintptr_t light_context{};{const std::lock_guard<std::recursive_mutex> hold(scene_view_cpu_lock);
        const auto found=scene_view_cpu_owners.find(view);if(found!=scene_view_cpu_owners.end())light_context=found->second.context;}
    CollectNativePointLights(*result,light_context);}
    section(3);
    CollectNativeForwardLights(*result,true);ProduceNativeSceneLighting(*result,false);   // cP7: prepare calls no engine function
    // I-0132 R11g: the rays' walk joined (the time this thread waits for it, `rays_wait_ms` in native_collect_sections), then its
    // outcome judged here as before. I-0132 R11e: a walk that skips or throws traces the last scene of this load (CollectNativeRtScene);
    // the arrival's frames are held (NativeRtArrivalStep); a frame still unserved shows its world black (magenta in the test runs), and
    // a latched failure closes the game with a message (step 4).
    if(rays_walk){
        {LARGE_INTEGER before{},after{};QueryPerformanceCounter(&before);rays_join.Wait();QueryPerformanceCounter(&after);
            if(rays_aside){rays_wait_ms+=1000.0*double(after.QuadPart-before.QuadPart)/double(section_freq.QuadPart);
                rays_span_ms+=1000.0*double(after.QuadPart-rays_posted.QuadPart)/double(section_freq.QuadPart);++rays_waits;}}
        NativeRtArrivalStep(*result,*rays_outcome);   // I-0132 R11e step 3
        // I-0132 R11e step 4: a traced pass's latched failure read every 15th walk (its strings under the renderer's lock), then closes.
        // And main-view frames unserved outside a hold for 2 s and 30 frames in a row (what no latch names; its step reviews' L5, M-B).
        {static uint32_t polls{};if(++polls%15==0){if(const auto why=renderer.RtFatalFailure();!why.empty())NativeRtFatal(why);
            else if(const auto last=renderer.RtUnservedLast(),since=renderer.RtUnservedSince();since && last>=since && last-since>=2000 && renderer.RtUnservedRun()>=30)   // last read first (the step review's L1)   // M-B: 30 frames; the final review's L1: to the run's last frame
                NativeRtFatal("the rays did not serve "+std::to_string(renderer.RtUnservedRun())+" frames in a row over 2 s (causes "+std::to_string(renderer.RtUnservedRunCauses())+
                ": 1 sun shadow, 2 scene, 4 global illumination, 8 local lights)");}}}
    CollectNativeRtForwardBlocks(*result);   // I-0125 R6d: the rays' forward instances' blocks (the light blocks' context made above)
    NoteNativeEffectCandidates(*result,deferred,walk_candidates);   // I-0105 witness
    section(4);
    // WT-6, I-0125 R6c and R6f (on by default, D-0189; no-water-reflection.flag opts out): the water's plane and t54 target for this
    // frame's traced pass.
    if(NativeReflectionEnabled())CollectNativeWaterPlane(*result);
    {const std::lock_guard carry(native_block_carry.lock);auto& c=native_block_carry;
        c.scene_valid=true;c.camera=camera_constants.bytes;c.written=camera_constants.written;c.view=result->view_constants.bytes;
        c.scene_table=result->scene_block_table.Carry();c.view_table=result->view_block_table.Carry();c.planned=result->planned_programs.Carry();   // I-0078: no copy unless a programme was added
        if(result->serial>=c.last_sweep+120){c.last_sweep=result->serial;c.instances.EraseIf([&](const auto& item){return item.second.serial+120<result->serial;});}}
    section(5);
    FinishNativeModelRecords(*result);
    section(6);
    {const kprofile::Scope k(kprofile::Teardown);std::shared_ptr<const NativeSelectedScene> previous;
        {const std::lock_guard<std::recursive_mutex> hold(scene_view_cpu_lock);previous=std::move(native_selected_scene);native_selected_scene=result;}
        NativeReleaseLater(std::move(previous));}
    section(7);
    if(++section_collects%600==0){char row[400]{};sprintf_s(row,"{\"event\":\"native_collect_sections\",\"ms\":[%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f],\"rays_aside\":%lld,\"rays_wait_ms\":%.3f,\"rays_span_ms\":%.3f}\n",
        section_ms[0]/600,section_ms[1]/600,section_ms[2]/600,section_ms[3]/600,section_ms[4]/600,section_ms[5]/600,section_ms[6]/600,section_ms[7]/600,
        static_cast<long long>(rays_waits),rays_waits?rays_wait_ms/double(rays_waits):0.0,rays_waits?rays_span_ms/double(rays_waits):0.0);Log(row);
        for(auto& v:section_ms)v=0;rays_wait_ms=rays_span_ms=0;rays_waits=0;
        // I-0091: the collect's kept blocks since the start (taken again, asked of the system, kept back, given back; held now).
        const auto store=frame_store::Shared().Read();sprintf_s(row,"{\"event\":\"native_frame_store\",\"collects\":%lld,\"reused\":%llu,\"fresh\":%llu,\"kept\":%llu,\"freed\":%llu,\"held_mb\":%.1f,\"held_blocks\":%llu}\n",
            static_cast<long long>(section_collects),static_cast<unsigned long long>(store.reused),static_cast<unsigned long long>(store.fresh),static_cast<unsigned long long>(store.kept),
            static_cast<unsigned long long>(store.freed),double(store.held_bytes)/1048576.0,static_cast<unsigned long long>(store.held_blocks));Log(row);}
    static LONG64 empty{},populated{};
    const auto sample=result->objects?InterlockedIncrement64(&populated):InterlockedIncrement64(&empty);
    if(sample<=4 || sample%240==0){char row[640]{};sprintf_s(row,
        "{\"event\":\"native_current_view_selected\",\"sample\":%lld,\"objects\":%u,\"model_intents\":%zu,\"unsupported\":%u,\"missing_geometry\":%u,\"nonzero_variant\":%u,\"missing_bindings\":%u,\"instance_streams_unavailable\":%u,\"missing_program_family\":%u,\"program_recipe_pairs\":%u,\"actual_current_culler\":true,\"synchronous\":true,\"native_geometry_leases\":%zu,\"occlusion_available\":false,\"materials_ready\":false,\"drawn\":false}\n",
        static_cast<long long>(sample),result->objects.load(),result->models.size(),result->unsupported.load(),result->missing_geometry.load(),result->nonzero_variant.load(),
        result->missing_bindings.load(),result->instance_streams.load(),result->missing_program_family.load(),result->program_recipes.load(),result->models.size());Log(row);}
    if(sample<=4 || sample%240==0){char row[384]{};sprintf_s(row,"{\"event\":\"native_disp_selection_totals\",\"children\":%llu,\"fd4_items\":%llu,\"other_children\":%llu,\"asm_parts\":%llu,\"eligible_meshes\":%u,\"unavailable_preparations\":%u,\"prepared_meshes\":%zu}\n",
        native_disp_children.load(),native_disp_items.load(),native_disp_unsupported_children.load(),native_asm_parts.load(),result->eligible_meshes,result->unavailable_preparations,result->prepared_meshes.size());Log(row);}
    // Gates: 0 scene inputs; 1-4 model preparation flags/capabilities/material
    // mask/category; 5-9 mesh class/masks/deformation/programmes/policy; 10+reason
    // EvaluateOrdinaryModelPass; 24-25 scroll; 26-29 intent/deformation/instanced/
    // texture bindings; 30 recipe; 31 tiling.
    if(sample<=4 || sample%240==0){std::string gates;
        for(size_t i=0;i<result->refusals.size();++i)if(result->refusals[i]){char item[32]{};
            sprintf_s(item,"%s\"%zu\":%u",gates.empty()?"":",",i,result->refusals[i]);gates+=item;}
        std::string row="{\"event\":\"native_pass_refusals\",\"gates\":{"+gates+"}}\n";Log(row.c_str());}
}
}  // namespace
