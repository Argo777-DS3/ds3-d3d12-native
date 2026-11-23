// I-0132 R12a (R12-PLAN « R12a's design », « The game's files for the overlay »): the native overlay's engine side. DS3's menu
// fonts are read from the game's own memory, where its menus loaded them: the Scaleform repository (GfxRepositoryImp, the global
// [image+0x4796CE8]) keeps each file the game's file requests read under its resolved path (data1:/font/<family>/font.gfx,
// OBSERVED in i132-r12a3-overlay to i132-r12a5-overlay), the node's bytes at +0x60 and size at +0x68. The repository's lookup
// (FUN_140E0E980) takes no lock while the game's file completions insert and remove nodes on another thread than the engine's
// (OBSERVED in i132-r12a6-overlay and i132-r12a7-overlay-debug), so the module never calls it: an observer of the insertions (FUN_140E0EA10, pass-through) keeps the
// node of each family as it is inserted, and the engine thread copies the bytes from that node (guarded reads) once, then parses them
// (font_outline.h); never a copy beside the module. The test screen (rekindled-overlay-test.flag, a test tool; the settings pages are the game's own since D-0271)
// draws a panel and lines of text in DS3's fonts above the game's UI, on the title, the menus and in play, shown 3 s and hidden 1 s in
// turn (the frame graph's plan key with and without the overlay, R12a's final review M3); one row names each font read and the
// overlay's counts. The observer is installed by default since I-0132 R12d (the precompile screen's words; no-rekindled-overlay.flag
// leaves the overlay out).
#pragma once
namespace rekindled_overlay {
namespace ov=ds3native::overlay;namespace ft=ds3native::font;
// The families of the menus (FONTS-LANGUAGES-NATIVE §4; STUDY-R12-SETTINGS-PAGE §3.2): the titles' TsukuOldMinPro, the European
// languages' body TsukuMinPro, English's body MatisseProN; since R12h's sixth tranche the other languages' own (FONTS-LANGUAGES-NATIVE
// §2, folders as the Data1 index names them): Japanese's and Traditional Chinese's TsukuMinProSub, Polish, Hangul, SimplifiedChinese,
// TraditionalChinese, each read only when the game loads it (its language's run). `key` is the family's folder in lower case, as the
// inserted paths are compared (« /font/tsukuminpro/ » never matches TsukuMinProSub's folder); `node` the repository's node the
// observer saw inserted (0 until then).
struct FontSlot {const wchar_t* path;const wchar_t* key;const char* name;std::atomic<uintptr_t> node{};std::shared_ptr<const ft::Font> font;
    std::unique_ptr<ov::GlyphCache> cache;bool requested{},failed{};uintptr_t failed_node{};};
FontSlot fonts[]={{L"data1:/font/TsukuOldMinPro/font.gfx",L"/font/tsukuoldminpro/","TsukuOldMinPro"},{L"data1:/font/TsukuMinPro/font.gfx",L"/font/tsukuminpro/","TsukuMinPro"},
    {L"data1:/font/MatisseProN/font.gfx",L"/font/matissepron/","MatisseProN"},{L"data1:/font/TsukuMinProSub/font.gfx",L"/font/tsukuminprosub/","TsukuMinProSub"},
    {L"data1:/font/Polish/font.gfx",L"/font/polish/","Polish"},{L"data1:/font/Hangul/font.gfx",L"/font/hangul/","Hangul"},
    {L"data1:/font/SimplifiedChinese/font.gfx",L"/font/simplifiedchinese/","SimplifiedChinese"},
    {L"data1:/font/TraditionalChinese/font.gfx",L"/font/traditionalchinese/","TraditionalChinese"}};
bool TestRequested() noexcept {static const bool on=ModuleFlag(L"rekindled-overlay-test.flag");return on;}
// I-0132 R12d: the overlay's screens, off with no-rekindled-overlay.flag. A refused observer (the step review's L3: an optional overlay
// never stops the start) leaves the screens without DS3's fonts (the precompile screen's bar still drawn: the second review's L-1).
std::atomic<bool> observer_refused{};
bool Enabled() noexcept {static const bool off=ModuleFlag(L"no-rekindled-overlay.flag");return !off;}
// The observer (pass-through) of the repository's insertions, FUN_140E0EA10(repository, const wchar_t* path, data, size) -> node (the
// file caps' completion, FUN_1404F1370; 4 register arguments, the node in RAX: as the decompiled caller passes and reads them). A
// family's node is kept once the original insertion has returned (its bytes copied in). The first 8 paths and every font path are
// named in rows (32 at most) with the inserting thread (the engine thread's in its own row, M2's comparison).
using RepositoryInsert=uintptr_t(*)(uintptr_t,const wchar_t*,uintptr_t,uint64_t);
RepositoryInsert repository_insert_original{};std::atomic<uint32_t> repository_inserts{};
std::atomic<bool> precompile_page_shown{};   // R12h: the page up (its fonts read only then; the final review's L2)
std::atomic<bool> precompile_screen_over{};   // below (R12d)
// A path's lower-case copy in `out` (at most its 127 first characters), and whether it holds `key` (lower case); no allocation (R12d's
// second review M-1: the observer stays on for the session, every insertion, so the fonts the menus load after the precompile screen are
// seen for R12e).
bool PathHas(const wchar_t* path,const wchar_t* key,wchar_t (&out)[128]) noexcept {
    size_t n=0;for(;path && path[n] && n<127;++n)out[n]=wchar_t(towlower(path[n]));out[n]=0;return std::wcsstr(out,key)!=nullptr;}
// I-0138 tranche 5b (options_film.h; 02b's Graphics tab): the options film as the game hands it to the repository, given three more
// slots for the Graphics tab when it is the shipped film byte for byte (its SHA-256, and the result's: the bytes checked offline
// on the game's film, parsed again). The repository's node copies the bytes it is given (FUN_140E0F3C0: an allocation of the size,
// a copy), so the patched bytes live only across the call. `out` stays empty when the film is left as it is; the tab is told
// the slots it has either way (options_film_slots). no-options-film-slots.flag, or no-rekindled-graphics.flag, leaves the film.
void OptionsFilmWithSlots(std::vector<uint8_t>& out,uintptr_t data,uint64_t size) noexcept {
    namespace of=ds3native::options_film;const char* result="patched";uint32_t slots=of::kFilmSlots;
    try{
        static const bool off=ModuleFlag(L"no-options-film-slots.flag") || ModuleFlag(L"no-rekindled-graphics.flag");
        std::vector<uint8_t> bytes(size);char hash[65]{};std::optional<std::vector<uint8_t>> added;
        if(off)result="off";
        else if(!Read(data,bytes.data(),bytes.size()))result="unreadable";
        else if(!ds3content::Sha256(bytes.data(),bytes.size(),hash) || std::strcmp(hash,of::kFilmSha256))result="not the shipped film";
        else if(!(added=of::AddSlots(bytes,of::kGraphicsSprite,of::kGraphicsCharacter,of::kAddedSlots)) ||
                !of::RetypeSlot(*added,of::kGraphicsSprite,of::kRetypedSlot,of::kSlotChoice))result="refused";   // tranche 5d: the sixth slot's kind
        else if(!ds3content::Sha256(added->data(),added->size(),hash) || std::strcmp(hash,of::kPatchedSha256))result="another result";
        else {out=std::move(*added);slots=of::kSlotsWanted;}}
    catch(...){out.clear();result="failed";slots=of::kFilmSlots;}
    options_film_slots.store(slots);
    static std::atomic<uint32_t> rows{};if(rows.fetch_add(1)<8){char row[192]{};
        sprintf_s(row,"{\"event\":\"native_options_film\",\"slots\":%u,\"result\":\"%s\",\"bytes\":%llu,\"thread\":%lu}\n",slots,result,static_cast<unsigned long long>(out.empty()?size:out.size()),GetCurrentThreadId());Log(row);}}
uintptr_t RepositoryInsertObserved(uintptr_t repository,const wchar_t* path,uintptr_t data,uint64_t size) noexcept {
    wchar_t lower[128]{};std::vector<uint8_t> film;   // the options film with its added slots, alive across the insertion
    if(path && data && size==ds3native::options_film::kFilmBytes && PathHas(path,L"/02_040_optionsetting.gfx",lower))OptionsFilmWithSlots(film,data,size);
    const uintptr_t node=repository_insert_original(repository,path,film.empty()?data:reinterpret_cast<uintptr_t>(film.data()),film.empty()?size:film.size());
    const uint32_t n=repository_inserts.fetch_add(1);bool font_path=false;
    font_path=path && PathHas(path,L"/font/",lower);
    if(font_path && node)for(auto& slot:fonts)if(std::wcsstr(lower,slot.key))slot.node.store(node,std::memory_order_release);
    static std::atomic<uint32_t> logged{};if(TestRequested() && (n<8 || font_path) && logged.fetch_add(1)<32){   // the test runs' rows only (L1)
        char text[160]{};for(size_t k=0;path && path[k] && k+1<sizeof(text);++k){const wchar_t c=path[k];text[k]=(c<32 || c>126 || c=='"' || c=='\\')?'?':char(c);}
        char row[320]{};sprintf_s(row,"{\"event\":\"native_overlay_repository_insert\",\"n\":%u,\"path\":\"%s\",\"bytes\":%llu,\"node\":%s,\"thread\":%lu}\n",n,text,(unsigned long long)size,node?"true":"false",GetCurrentThreadId());Log(row);}
    return node;}
bool InstallRepositoryObserver() noexcept {
    if(MH_CreateHook(reinterpret_cast<void*>(image+0xE0EA10),reinterpret_cast<void*>(&RepositoryInsertObserved),reinterpret_cast<void**>(&repository_insert_original))==MH_OK)return true;
    observer_refused=true;Log("{\"event\":\"native_overlay_state\",\"observer\":false}\n");return false;}
// The bytes of a node the observer kept (guarded reads; none when they cannot be read).
std::optional<std::vector<uint8_t>> NodeBytes(uintptr_t node) noexcept {
    try{
        uintptr_t data{};int32_t size{};
        if(!node || !Read(node+0x60,&data,8) || !Read(node+0x68,&size,4) || !data || size<=0 || size>(64<<20))return std::nullopt;
        std::vector<uint8_t> bytes(static_cast<size_t>(size));if(!Read(data,bytes.data(),bytes.size()))return std::nullopt;
        return bytes;
    }catch(...){return std::nullopt;}}
// A « CFX » movie (zlib after its 8-byte header) inflated; a « GFX » or « FWS » one as it is.
std::optional<std::vector<uint8_t>> Inflated(std::vector<uint8_t> movie){
    if(movie.size()<8)return std::nullopt;
    if(movie[0]!='C')return movie;
    const uint32_t length=uint32_t(movie[4])|(uint32_t(movie[5])<<8)|(uint32_t(movie[6])<<16)|(uint32_t(movie[7])<<24);
    if(length<8 || length>(128u<<20))return std::nullopt;
    std::vector<uint8_t> out(length);std::memcpy(out.data(),movie.data(),8);out[0]='G';
    uLongf produced=length-8;if(uncompress(out.data()+8,&produced,movie.data()+8,uLong(movie.size()-8))!=Z_OK)return std::nullopt;
    out.resize(8+produced);return out;}
// A family the game did not load (TsukuMinPro in English: MatisseProN lacks ë, î, ï, ù, û, ÿ, œ, æ and their capitals, read offline on
// the game's file) is requested once through the game's own file request, as its menus request theirs (FUN_140E5A2F0 at 140E5A642:
// FUN_140503D30(the file manager [image+0x47559C8], the resolved wide path, 0, 0), the returned cap kept by the caller): the cap is kept
// for the session (never released, so its bytes stay in the repository), asked once a font of the game's own has arrived (its file
// system up).
void RequestFamily(FontSlot& slot) noexcept {
    if(slot.requested)return;slot.requested=true;
    uintptr_t manager{};const bool ok=image && Read(image+0x47559C8,&manager,8) && manager;
    uintptr_t cap{};if(ok)cap=reinterpret_cast<uintptr_t(*)(uintptr_t,const wchar_t*,uintptr_t,uint32_t)>(image+0x503D30)(manager,slot.path,0,0);
    char row[200]{};sprintf_s(row,"{\"event\":\"native_overlay_font_request\",\"family\":\"%s\",\"manager\":%s,\"cap\":%s}\n",slot.name,ok?"true":"false",cap?"true":"false");Log(row);}
// Each frame on the engine thread: a family whose node has been seen is read and parsed (its time named in its row). `title_only`
// (R12e, the step review's M1): the titles' family alone (the title's word; MatisseProN's or TsukuMinPro's 95 to 110 ms never spent at
// the title).
void ReadFonts(bool title_only=false) {
    static uint64_t frame{};++frame;
    // The final review's L1: the request (TsukuMinPro, 8.4 MB kept) only for the overlay test until a screen needs that family (R12e).
    if(TestRequested() && (fonts[0].font || fonts[2].font) && !fonts[1].font && !fonts[1].node.load(std::memory_order_acquire) && frame>120)RequestFamily(fonts[1]);
    for(size_t i=0;i<std::size(fonts);++i){auto& slot=fonts[i];
        if(slot.font || slot.failed || (title_only && i!=0))continue;
        const uintptr_t node=slot.node.load(std::memory_order_acquire);if(!node || node==slot.failed_node)continue;   // a new node tried again
        LARGE_INTEGER t0{},t1{},f{};QueryPerformanceCounter(&t0);QueryPerformanceFrequency(&f);
        auto bytes=NodeBytes(node);
        if(!bytes){slot.failed_node=node;char row[200]{};sprintf_s(row,"{\"event\":\"native_overlay_font\",\"family\":\"%s\",\"read\":false,\"reason\":\"node\"}\n",slot.name);Log(row);continue;}
        const size_t raw=bytes->size();auto movie=Inflated(std::move(*bytes));
        auto read=movie?ft::ReadFonts(*movie):std::vector<ft::Font>{};QueryPerformanceCounter(&t1);
        const double ms=double(t1.QuadPart-t0.QuadPart)*1000.0/double(f.QuadPart);
        char row[360]{};
        if(read.empty()){slot.failed=true;sprintf_s(row,"{\"event\":\"native_overlay_font\",\"family\":\"%s\",\"bytes\":%zu,\"read\":false,\"ms\":%.1f}\n",slot.name,raw,ms);Log(row);continue;}
        auto font=std::make_shared<const ft::Font>(std::move(read.front()));
        std::string name=font->name.substr(0,48);for(auto& c:name)if(c=='"' || c=='\\' || static_cast<unsigned char>(c)<32 || static_cast<unsigned char>(c)>126)c='?';
        sprintf_s(row,"{\"event\":\"native_overlay_font\",\"family\":\"%s\",\"bytes\":%zu,\"read\":true,\"name\":\"%s\",\"glyphs\":%zu,\"kerning\":%zu,\"layout\":%s,\"ms\":%.1f,\"requested\":%s}\n",
            slot.name,raw,name.c_str(),font->glyphs.size(),font->kerning.size(),font->has_layout?"true":"false",ms,slot.requested?"true":"false");Log(row);
        slot.cache=std::make_unique<ov::GlyphCache>(font,0x52454B00ull+2*i,1024);slot.font=std::move(font);}}
// R12a's second tranche: DS3's menu pieces from the textures its menus loaded (01_common.tpf's MENU_* entries, held by the native's
// texture ingress under their entry names), sampled as they are. The rectangles are in texels, measured on the decoded textures
// (R12-PLAN « The pieces »: alpha bounding boxes of MENU_OptionBase and MENU_ItemParts): the Options pages' dark torn-edged panel,
// the orange glow behind a selected row, the thin rule (white in the texture, tinted gold here by eye on the stills: the films' colour
// transforms are not read). The rectangles hold for the measured sizes only: a texture of that name with another size is refused
// (named in its row), as a variant or a mod would draw wrong parts. A record withdrawn by the ingress is dropped and looked up again.
struct PieceTexture {const char* name;uint64_t identity;uint32_t width,height;std::shared_ptr<const bootstrap::NativeTextureRecord> record;uint32_t said{};};
PieceTexture piece_textures[]={{"MENU_OptionBase",0x52454B10ull,2048,1024},{"MENU_ItemParts",0x52454B11ull,1024,512}};
struct Piece {size_t texture;float u0,v0,u1,v1;};
constexpr Piece kPanel{0,4,0,1216,684},kCursorGlow{1,5,77,412,116},kRule{1,10,4,410,6};   // the glow's rows 77-115 (its profile, the final review's M1)
void FindPieces() {
    static uint64_t frame{};if((++frame%30)!=1)return;
    for(auto& piece:piece_textures){
        if(piece.record && !piece.record->withdrawn.load())continue;   // the step review's L1: a withdrawn record looked up again
        piece.record=textures.FindByName(piece.name);if(!piece.record)continue;
        const auto& d=piece.record->description;const bool fits=d.width==piece.width && d.height==piece.height;
        if(piece.said<4){++piece.said;char row[260]{};
            sprintf_s(row,"{\"event\":\"native_overlay_piece\",\"name\":\"%s\",\"width\":%u,\"height\":%u,\"format\":%d,\"generation\":%llu,\"used\":%s}\n",
                piece.name,d.width,d.height,int(d.format),(unsigned long long)piece.record->generation,fits?"true":"false");Log(row);}
        if(!fits)piece.record.reset();}}
// A piece's quad (output pixels) and its texture added to the frame (none while the texture is not there).
bool DrawPiece(ov::Frame& frame,const Piece& piece,float x0,float y0,float x1,float y1,std::array<float,4> colour) {
    const auto& texture=piece_textures[piece.texture];if(!texture.record)return false;
    ov::Image picture;picture.identity=texture.identity;picture.version=texture.record->generation;   // the step review's M1: a new record, a new block
picture.width=texture.record->description.width;picture.height=texture.record->description.height;
    picture.external=texture.record->resource.Get();picture.owner=texture.record;
    const int32_t index=ov::ImageIndex(frame,picture);if(index<0)return false;
    frame.quads.push_back({x0,y0,x1,y1,piece.u0,piece.v0,piece.u1,piece.v1,colour,index});return true;}
// The chains: the titles' TsukuOldMinPro first, the body's the game's own (MatisseProN in English, else TsukuMinPro), each with the
// others behind it for a code it lacks.
std::array<ov::GlyphCache*,3> TitleChain() noexcept {return {fonts[0].cache.get(),fonts[1].cache.get(),fonts[2].cache.get()};}
std::array<ov::GlyphCache*,3> BodyChain() noexcept {return {fonts[2].cache.get(),fonts[1].cache.get(),fonts[0].cache.get()};}
bool AnyFont() noexcept {for(auto& slot:fonts)if(slot.cache)return true;return false;}
uint32_t Missing() noexcept {uint32_t n{};for(auto& slot:fonts)if(slot.cache)n+=slot.cache->Missing();return n;}
// The test screen, laid out in DS3's 1280x720 units scaled by the output height; shown 180 frames, hidden 60, in turn.
void BuildTest(uint32_t width,uint32_t height) {
    static uint64_t frames{};const bool shown=(frames++%240)<180;
    const auto title_chain=TitleChain(),body_chain=BodyChain();const ov::Chain title(title_chain),body(body_chain);
    if(!shown || !AnyFont() || !width || !height){renderer.SetOverlay(nullptr);return;}
    auto frame=std::make_shared<ov::Frame>();frame->width=width;frame->height=height;
    const float s=float(height)/720.f,x0=float(width)*0.5f-420.f*s,y0=150.f*s;
    // The panel and the rule: DS3's own pieces once its menus loaded them (a plain panel and line before).
    const float panel_height=840.f*s*(kPanel.v1-kPanel.v0)/(kPanel.u1-kPanel.u0);   // L7: the panel at its own aspect
    if(!DrawPiece(*frame,kPanel,x0,y0,x0+840.f*s,y0+panel_height,{1,1,1,1}))frame->quads.push_back({x0,y0,x0+840.f*s,y0+400.f*s,0,0,0,0,{0.02f,0.02f,0.02f,0.82f},-1});
    if(!DrawPiece(*frame,kRule,x0+40.f*s,y0+86.f*s,x0+800.f*s,y0+88.f*s,{0.78f,0.62f,0.36f,1}))
        frame->quads.push_back({x0+40.f*s,y0+86.f*s,x0+800.f*s,y0+88.f*s,0,0,0,0,{0.78f,0.56f,0.24f,0.9f},-1});
    (void)DrawPiece(*frame,kCursorGlow,x0+40.f*s,y0+140.f*s+48.f*s-31.f*s,x0+800.f*s,y0+140.f*s+48.f*s+11.f*s,{1,1,1,0.85f});   // the « selected » second line
    ov::TextStyle heading;heading.pixels=std::round(38.f*s);heading.colour={0.92f,0.88f,0.80f,1};heading.scale=s;
    heading.shadow.blur=uint32_t((std::clamp)(std::lround(s),1L,long(ov::GlyphCache::kPad)));   // the shadow's blur with the output
    ov::TextStyle line=heading;line.pixels=std::round(24.f*s);line.colour={0.82f,0.80f,0.76f,1};
    ov::TextStyle dim=line;dim.colour={0.45f,0.44f,0.42f,1};
    ov::DrawText(*frame,title,ov::Utf8("Dark Souls III: Rekindled"),x0+40.f*s,y0+70.f*s,heading);
    const char* lines[]={"Ray tracing \xE2\x80\x94 Path tracing","Qualit\xC3\xA9 des r\xC3\xA9glages : \xC2\xAB \xC3\x89lev\xC3\xA9 \xC2\xBB","Nettet\xC3\xA9, aberration chromatique, vignettage",
        "AVWAY Tokyo Kerning: To Ta Te Yo \xC5\x93 \xC3\xAF \xC3\xBB"};
    float y=y0+140.f*s;for(const char* text:lines){ov::DrawText(*frame,body,ov::Utf8(text),x0+60.f*s,y,line);y+=48.f*s;}
    ov::DrawText(*frame,body,ov::Utf8("G\xC3\xA9n\xC3\xA9ration d'images (indisponible sans DLSS)"),x0+60.f*s,y,dim);
    renderer.SetOverlay(std::move(frame));}
// I-0132 R12e, the crackle (Thomas, 2026-11-12 08:54: the bonfires' crackling at the title): the game's own fire loop played while the
// title's music plays, through the game's own menu sound path (MENU-AUDIO-NATIVE §3, read in the DS3 mirror 2026-11-12):
// FUN_1404AD510(out, category, id, -1) forms the event's name and issues it, writing its 8-byte handle {id, serial} into `out`;
// FUN_141A29EB0(handle) stops it; an empty handle holds the sound system's invalid pair [144932300], [144932304]. The title's music is
// switched by FUN_14076BBA0(sound object, id) (the menu sound object's method, also in its table [1449D74BC]; FUN_14076B920(id), called
// by the TitleTopDialog's constructor FUN_140AD8900 and by FUN_140ADA260 with id 1, m000000001, only jumps to it: the step review's M1)
// and stopped by FUN_14076B9F0 (FUN_140AE8930, the title dialog's step leaving for the game, and FUN_140AE8AB0; also in the table):
// a pass-through observer of FUN_14076BBA0 itself (every caller and the table's) starts the crackle after a switch that leaves the
// object's current music (+0x10) at 1 and stops it after a switch to any other music; R12h's detour of the music's stop (15
// DropTitleBgm, one detour for both) stops it after the stop, so the crackle is installed only with that detour (the final review's
// L1, L2: a crackle that could never stop). While the precompile page is up a switch's start waits for its end (Resume, from Step:
// the final review's M1); the page itself starts the same event under its fire (StartFromPage, R12h's fifth tranche, Thomas
// 2026-11-12 18:17: the game's own sound for the page's fire, not a synthesised one) and the title's switch then finds it playing, so
// the fire's sound goes on from the page into the title; a failed issue is not retried until the music stops or changes (its L3),
// but for the page's own retries. The requests come on the engine thread (r12h-mu-page3: the game's and R12h's release on one
// thread); the lock is a guard. Every visit to the title (a return from play builds a new TitleTopDialog) starts it again; the Load Game list and the Options keep it, as they keep the music. The event's loop and its level (the SE volume) are the game's;
// that it sounds at the title, played with no position as the menu sounds are, is INFERRED until a run and Thomas's ear (the step
// review's L3). no-title-crackle.flag leaves it out; a refused hook only writes a row (an optional sound never stops the start).
namespace title_crackle {
// The event: s444444441 (category 5, the letter table [142786578]'s « s »), in the resident fdp_main.fev (I-0069's list). The
// bonfire's own effect asks for it (the effects' sound request FUN_140D53940 under the effect node's update, the same caller chain
// at the High Wall's and at Firelink's bonfire: r12h-fire-census-m30 and -m40, audio-census.flag); Firelink's adds s444444444. That
// it is the crackle Thomas hears at a bonfire is INFERRED (by its caller and its zones), his ear to confirm.
constexpr uint32_t kCategory=5;constexpr int32_t kId=444444441;
using SwitchBgm=void(__fastcall*)(uintptr_t,int32_t);SwitchBgm switch_bgm_original{};
std::mutex lock;uint32_t handle[2]{};bool playing{},failed{},pending{},from_page{};uint64_t starts{},stops{};
// The sound system's readiness, read without making it: FUN_141A37810 returns the singleton [1449323B8], making it on first use, and
// the issue FUN_141A2CFE0 answers the invalid pair unless its byte +0x698 is set (read in the DS3 mirror 2026-11-12). Read here so
// that the page, earlier than any menu sound, never makes the singleton itself; an unset byte means not yet.
bool SoundReady() noexcept {uintptr_t system{};uint8_t ready{};return Read(image+0x49323B8,&system,8) && system && Read(system+0x698,&ready,1) && ready;}
void Row(const char* what,bool valid) noexcept {
    constexpr char letters[]="acfopsmvxbgyze";char row[256]{};
    sprintf_s(row,"{\"event\":\"native_title_crackle\",\"%s\":true,\"name\":\"%c%09d\",\"handle_valid\":%s,\"starts\":%llu,\"stops\":%llu}\n",what,
        kCategory<sizeof(letters)-1?letters[kCategory]:'?',kId,valid?"true":"false",(unsigned long long)starts,(unsigned long long)stops);Log(row);}
// Under the lock: the handle stopped (the event playing), neither `pending` nor `failed` touched.
void StopLocked() noexcept {
    if(!playing)return;playing=from_page=false;++stops;
    reinterpret_cast<uintptr_t(__fastcall*)(uint32_t*)>(image+0x1A29EB0)(handle);Row("stopped",true);}
void Stop() noexcept {const std::lock_guard hold(lock);failed=pending=false;StopLocked();}
// The game's calls (FUN_1404AD510, FUN_141A29EB0) run under the lock: neither reaches FUN_14076BBA0 or FUN_14076B9F0 in the mirror
// (FUN_1404AD510 calls FUN_1404AD2D0, FUN_1407CE930, FUN_141A298B0; FUN_141A29EB0 posts a message), so no re-entry (the step review's
// L-c). FUN_1407CE930's body FUN_1407CEC40 (read 2026-11-12) goes, unless the sound is muted ([144769280]: a stored handle copied),
// through FUN_141A2CFE0 (the +0x698 gate below, else the sound system's table +0x40) and FUN_141A29D00 (nothing for the invalid pair,
// else a message posted to the sound system's +0xD0 queue): only the sound system itself, made and ready by then (SoundReady).
// Under the lock: one issue, the handle kept when it is valid; `pending` and `failed` left to the callers.
bool IssueLocked() noexcept {
    uint32_t invalid[2]{};if(playing || !SoundReady() || !Read(image+0x4932300,invalid,8))return false;
    uint32_t issued[2]{invalid[0],invalid[1]};
    reinterpret_cast<uintptr_t(__fastcall*)(uint32_t*,uint32_t,int32_t,int32_t)>(image+0x4AD510)(issued,kCategory,kId,-1);
    handle[0]=issued[0];handle[1]=issued[1];++starts;playing=issued[0]!=invalid[0];Row("started",playing);return true;}
// `held`: Resume's start, dropped if a Stop came since (its L-b).
void Start(bool held=false) noexcept {
    const std::lock_guard hold(lock);if(held && !pending)return;pending=false;if(playing || failed)return;
    if(IssueLocked())failed=!playing;}
bool PageUp() noexcept {return precompile_page_shown.load() && !precompile_screen_over.load();}
void __fastcall ObserveSwitchBgm(uintptr_t sound,int32_t id) noexcept {
    switch_bgm_original(sound,id);
    int32_t current{-1};if(!sound || !Read(sound+0x10,&current,4))return;
    if(current!=1){Stop();return;}
    if(PageUp()){const std::lock_guard hold(lock);pending=true;return;}
    Start();}
// From Step on the engine thread: a start held under the page, once the page is over and the title's music still the current one.
void Resume() noexcept {
    {const std::lock_guard hold(lock);if(!pending || PageUp())return;}
    uintptr_t sound{};int32_t current{-1};
    if(!Read(image+0x4763868,&sound,8) || !sound || !Read(sound+0x10,&current,4) || current!=1){const std::lock_guard hold(lock);pending=false;return;}
    Start(true);}
// From the precompile page's drawn frames on the engine thread (16b page_fx::StartSound): the event under the page's fire. Tried every
// 0.5 s: while the sound system is not ready (SoundReady) nothing is issued; once it is, an issue answered with the invalid pair is
// tried again, 40 times at most (20 s). Why one would be is not read (INFERRED, the final review's L1: the name is resolved later on
// the sound thread, so an unknown name likely comes back valid and silent). One row when it first waits, one when it starts or gives up.
constexpr uint32_t kPageTries=40;
void StartFromPage() noexcept {
    static uint64_t first{},next{};static uint32_t tries{};static bool waited{},gave_up{};
    const uint64_t now=GetTickCount64();if(!first)first=now;
    {const std::lock_guard hold(lock);if(playing || gave_up || now<next)return;}
    next=now+500;
    if(!SoundReady()){if(!waited){waited=true;Log("{\"event\":\"native_precompile_fire_sound\",\"waiting\":\"sound_system\"}\n");}return;}
    // The page's own issue (the step review's M1): neither the title's `pending` nor its `failed` touched, so a title start held under
    // the page, or one after it, still tries as before.
    bool now_playing{};{const std::lock_guard hold(lock);if(!IssueLocked())return;now_playing=playing;from_page=playing;}
    ++tries;
    if(now_playing || tries>=kPageTries){gave_up=!now_playing;char row[160]{};
        sprintf_s(row,"{\"event\":\"native_precompile_fire_sound\",\"%s\":true,\"tries\":%u,\"after_ms\":%llu}\n",now_playing?"started":"gave_up",tries,(unsigned long long)(now-first));Log(row);}}
// The page's overlay lost (no drawn page, no sound: R12h's second tranche, its final review's M1): the event stopped only if the page
// started it (a title's start stays the title's), and a title start held under the page kept (the step review's L1).
void StopFromPage() noexcept {const std::lock_guard hold(lock);if(from_page)StopLocked();}
// The page's end (its `over`, Step's catches; the final review's M1): the page's event kept only when the title's music is there to
// take it over (a start held under the page, the menu sound object's current music at 1, or the title's m1 request held for the page's
// music, 15 title_bgm_deferred); otherwise stopped, so it never plays over the start logos, and the title's switch issues it again.
void PageOver() noexcept {
    uintptr_t sound{};int32_t current{-1};const bool title_music=(Read(image+0x4763868,&sound,8) && sound && Read(sound+0x10,&current,4) && current==1) || title_bgm_deferred.load()==1;
    const std::lock_guard hold(lock);if(from_page && !pending && !title_music)StopLocked();}
}
// I-0132 R12h's second tranche (Thomas, 2026-11-12 10:36-10:47: the page's background, « rendre l'image vivante »): the precompile
// page over a picture of Thomas's (a kneeling knight, his sword planted in glowing rocks, a cathedral at the right), brought to life:
// flames and a glow where the sword enters the rocks, embers rising from them and from the cape, ash and dust in the air, fog in
// layers, the tattered cape moving, one wind for all of them (blowing to the left, as the picture's cape and debris do), and the
// fire's sound while the page is up (since the fifth tranche the game's own bonfire event, title_crackle::StartFromPage).
// The picture: rekindled-precompile-bg.jpg beside the module or in %LOCALAPPDATA%/DS3-DX12, then .png the same (Thomas's PC only: made by
// his own image session, not in the repository); decoded once by WIC on a thread of its own from the page's first shown frame (the
// fire's frames baked there too), faded in once ready, scaled to the lines the output will show of it (no mips: no shimmer); without
// it the page stays black and draws none of this.
// Every place is in the picture's own coordinates (0 to 1 across and down) and follows it as it is laid over the output (covering it,
// cropped at the sides or top and bottom, a slow push in of 4 % over a minute). The particles are simulated on the engine thread
// from the frame's time step (at most 0.1 s), deterministic (an LCG). no-precompile-background.flag: the black page of R12h's first
// tranche; no-precompile-sound.flag: no sound under the page (neither the music nor the fire's).
namespace page_fx {
constexpr uint64_t kBackgroundId=0x52454B3042470001ull,kDotId=0x52454B3044540001ull,kFlameId=0x52454B30464C0001ull,kCloudId=0x52454B30434C0001ull;
uint32_t seed=0x6B1D53A9u;
float Random() noexcept {seed=seed*1664525u+1013904223u;return float(seed>>8)/16777216.f;}
float Lattice(int x,int y,uint32_t salt) noexcept {uint32_t h=uint32_t(x)*0x8DA6B343u^uint32_t(y)*0xD8163841u^salt*0xCB1AB31Fu;h^=h>>13;h*=0x5BD1E995u;h^=h>>15;return float(h&0xFFFFu)/65535.f;}
float Noise(float x,float y,uint32_t salt) noexcept {const int ix=int(std::floor(x)),iy=int(std::floor(y));const float fx=x-float(ix),fy=y-float(iy);
    const float sx=fx*fx*(3.f-2.f*fx),sy=fy*fy*(3.f-2.f*fy);
    const float a=Lattice(ix,iy,salt),b=Lattice(ix+1,iy,salt),c=Lattice(ix,iy+1,salt),d=Lattice(ix+1,iy+1,salt);
    return a+(b-a)*sx+(c-a)*sy+(a-b-c+d)*sx*sy;}
float Smooth(float e0,float e1,float x) noexcept {const float t=std::clamp((x-e0)/(e1-e0),0.f,1.f);return t*t*(3.f-2.f*t);}
// The images made once: a soft dot (embers, glows, dust, ash) and a
// cloud (a radial falloff broken by value noise).
ov::Image Coverage(uint64_t id,uint32_t w,uint32_t h,const auto& value){
    auto bytes=std::make_shared<std::vector<uint8_t>>(size_t(w)*h);
    for(uint32_t y=0;y<h;++y)for(uint32_t x=0;x<w;++x)(*bytes)[size_t(y)*w+x]=uint8_t(std::lround(255.f*std::clamp(value((float(x)+0.5f)/float(w),(float(y)+0.5f)/float(h)),0.f,1.f)));
    return ov::Image{id,1,w,h,ov::Format::Coverage8,std::move(bytes)};}
const ov::Image& Dot(){static const ov::Image picture=Coverage(kDotId,32,32,[](float u,float v){const float dx=u*2.f-1.f,dy=v*2.f-1.f;return std::exp(-(dx*dx+dy*dy)*4.f)-0.018f;});return picture;}
const ov::Image& Cloud(){static const ov::Image picture=Coverage(kCloudId,128,128,[](float u,float v){const float dx=u*2.f-1.f,dy=v*2.f-1.f;
        const float falloff=std::clamp(1.f-std::sqrt(dx*dx+dy*dy),0.f,1.f),noise=0.5f*Noise(u*4.f,v*4.f,11)+0.3f*Noise(u*8.f,v*8.f,12)+0.2f*Noise(u*16.f,v*16.f,13);
        return falloff*falloff*(0.35f+0.9f*noise)-0.05f;});return picture;}
// The fire (Thomas, 11:44: the drawn tongues looked cheap): value noise rising through a tongue narrowing upward and leaning left
// (three octaves, faster as they get finer), its heat mapped to a fire's ramp (deep red, orange, yellow, a pale core), its alpha the
// heat. 32 frames of 56x96 texels over a 2 s loop (each frame the field at t and at t-2 s crossfaded by t, so the loop has no seam),
// baked once into one atlas of 8 by 4 on the picture's thread (the step review's M3: no texture made and uploaded every frame);
// Draw crossfades two neighbouring frames.
constexpr uint32_t kFireW=56,kFireH=96,kFireFrames=32,kFireColumns=8;constexpr float kFireLoopS=2.f;
float FireHeat(float x01,float h,float t){
    const float warp=(Noise(x01*3.f,h*2.5f-t*1.9f,54)-0.5f)*0.32f*h;   // the sides broken into licks, more so higher up
    const float u=x01-0.5f+h*h*0.12f+warp;   // sampled to the right higher up: the tongue leans left, as the wind blows
    const float n=0.5f*Noise(u*5.f+7.f,h*3.f-t*2.4f,51)+0.3f*Noise(u*11.f+3.f,h*7.f-t*3.9f,52)+0.2f*Noise(u*23.f,h*15.f-t*6.2f,53);
    const float spread=0.26f*(1.f-0.75f*h),shape=std::exp(-(u*u)/(2.f*spread*spread));
    return std::clamp(shape*(1.75f*n-0.12f)-h*0.72f,0.f,1.f)*Smooth(0.f,0.14f,h);}
ov::Image FireAtlas(){
    constexpr uint32_t aw=kFireW*kFireColumns,ah=kFireH*(kFireFrames/kFireColumns);auto bytes=std::make_shared<std::vector<uint8_t>>(size_t(aw)*ah*4);
    for(uint32_t f=0;f<kFireFrames;++f){const float t=kFireLoopS*float(f)/float(kFireFrames),mix=t/kFireLoopS;
        const uint32_t ox=(f%kFireColumns)*kFireW,oy=(f/kFireColumns)*kFireH;
        for(uint32_t y=0;y<kFireH;++y){const float h=1.f-(float(y)+0.5f)/float(kFireH);   // 0 the foot, 1 the top
            for(uint32_t x=0;x<kFireW;++x){const float x01=(float(x)+0.5f)/float(kFireW);
                const float heat=FireHeat(x01,h,t)*(1.f-mix)+FireHeat(x01,h,t-kFireLoopS)*mix;
                uint8_t* texel=bytes->data()+(size_t(oy+y)*aw+ox+x)*4;
                texel[0]=uint8_t(std::lround(255.f*Smooth(0.f,0.32f,heat)));texel[1]=uint8_t(std::lround(255.f*0.88f*Smooth(0.22f,0.72f,heat)));
                texel[2]=uint8_t(std::lround(255.f*0.55f*Smooth(0.7f,1.f,heat)));texel[3]=uint8_t(std::lround(255.f*0.9f*Smooth(0.f,0.75f,heat)));}}}   // a soft edge
    return ov::Image{kFlameId,1,aw,ah,ov::Format::Rgba8,std::move(bytes)};}
// The picture, decoded on a thread of its own.
std::mutex background_lock;std::shared_ptr<const ov::Image> background,fire;bool released{};std::atomic<int> background_state{};   // 0 not asked, 1 asked
bool Decode(const std::wstring& path,uint32_t screen_w,uint32_t screen_h,std::vector<uint8_t>& rgba,uint32_t& width,uint32_t& height){
    const HRESULT co=CoInitializeEx(nullptr,COINIT_MULTITHREADED);bool ok=false;
    {Microsoft::WRL::ComPtr<IWICImagingFactory> factory;Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
        Microsoft::WRL::ComPtr<IWICBitmapScaler> scaler;Microsoft::WRL::ComPtr<IWICFormatConverter> converter;UINT w{},h{};
        if(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory))) &&
           SUCCEEDED(factory->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnDemand,&decoder)) &&
           SUCCEEDED(decoder->GetFrame(0,&frame)) && SUCCEEDED(frame->GetSize(&w,&h)) && w && h && w<=16384 && h<=16384){
            IWICBitmapSource* source=frame.Get();
            // The lines the page will show of it: covering the output, pushed in by 4 % (Draw), a little margin.
            const UINT lines=UINT(std::ceil(double(screen_h)*1.05*(std::max)(1.0,(double(screen_w)/double(screen_h))/(double(w)/double(h)))));
            if(lines>=16 && h>lines){const UINT sw=(std::max)(1u,UINT(std::lround(double(w)*lines/h)));
                if(SUCCEEDED(factory->CreateBitmapScaler(&scaler)) && SUCCEEDED(scaler->Initialize(frame.Get(),sw,lines,WICBitmapInterpolationModeFant))){source=scaler.Get();w=sw;h=lines;}}
            if(w<=8192 && h<=8192 && SUCCEEDED(factory->CreateFormatConverter(&converter)) &&   // the step review's L1: ov::Valid's limits
               SUCCEEDED(converter->Initialize(source,GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0.,WICBitmapPaletteTypeCustom))){
                rgba.resize(size_t(w)*h*4);ok=SUCCEEDED(converter->CopyPixels(nullptr,w*4,UINT(rgba.size()),rgba.data()));width=w;height=h;}}}
    if(SUCCEEDED(co))CoUninitialize();
    return ok;}
std::wstring BackgroundPath(){
    for(const wchar_t* name:{L"rekindled-precompile-bg.jpg",L"rekindled-precompile-bg.png"}){
        const auto beside=ModuleSibling(name);if(!beside.empty() && GetFileAttributesW(beside.c_str())!=INVALID_FILE_ATTRIBUTES)return beside;
        std::array<wchar_t,MAX_PATH> local{};const DWORD chars=GetEnvironmentVariableW(L"LOCALAPPDATA",local.data(),MAX_PATH);
        if(chars && chars<MAX_PATH){std::wstring path=std::wstring(local.data())+L"\\DS3-DX12\\"+name;if(GetFileAttributesW(path.c_str())!=INVALID_FILE_ATTRIBUTES)return path;}}
    return {};}
void MakeSound();   // below
void StartBackground(uint32_t screen_w,uint32_t screen_h){
    int expected=0;if(!background_state.compare_exchange_strong(expected,1))return;
    std::thread([screen_w,screen_h]{try{   // the step review's L2: nothing thrown out of the thread (std::terminate)
        LARGE_INTEGER t0{},t1{},f{};QueryPerformanceCounter(&t0);QueryPerformanceFrequency(&f);
        const auto path=BackgroundPath();std::vector<uint8_t> rgba;uint32_t w{},h{};const bool ok=!path.empty() && Decode(path,screen_w,screen_h,rgba,w,h);
        auto flames=ok?std::make_shared<const ov::Image>(FireAtlas()):nullptr;
        if(ok){auto picture=std::make_shared<ov::Image>();picture->identity=kBackgroundId;picture->version=1;picture->width=w;picture->height=h;picture->format=ov::Format::Rgba8;
            picture->bytes=std::make_shared<const std::vector<uint8_t>>(std::move(rgba));const std::lock_guard hold(background_lock);if(!released){background=std::move(picture);fire=std::move(flames);}}   // the final review's L1
        QueryPerformanceCounter(&t1);char row[160]{};   // the picture's time alone (the third tranche's step review's L7)
        sprintf_s(row,"{\"event\":\"native_precompile_background\",\"found\":%s,\"decoded\":%s,\"width\":%u,\"height\":%u,\"ms\":%.0f}\n",path.empty()?"false":"true",ok?"true":"false",w,h,
            double(t1.QuadPart-t0.QuadPart)*1000.0/double(f.QuadPart));Log(row);
        if(ok)MakeSound();}   // after the picture is published (the second step review's L4); the music since the third tranche
        catch(...){Log("{\"event\":\"native_precompile_background\",\"failed\":\"exception\"}\n");}}).detach();}
std::shared_ptr<const ov::Image> Background(){const std::lock_guard hold(background_lock);return background;}
std::shared_ptr<const ov::Image> Fire(){const std::lock_guard hold(background_lock);return fire;}
// I-0132 R12h's third tranche (Thomas, 2026-11-12 12:46-13:08: the prologue's music under the page, « Prologue », Yuka Kitamura, his
// own FLAC): the page's sound is one stereo 16-bit waveOut stream mixed on a thread of its own: Thomas's music (rekindled-precompile-
// music.flac, .mp3 or .wav beside the module or in %LOCALAPPDATA%/DS3-DX12, his PC only, not in the repository; decoded once to 16-bit
// PCM by Media Foundation's source reader on the picture's thread, at its own rate) faded in over 2 s and looped; when the page ends
// it fades out over 1.5 s, then the stream closes. The mixer shares what it plays (a shared pointer): the page's release never frees
// it. The fire's sound under it is the game's own bonfire event through FMOD since the fifth tranche (title_crackle::StartFromPage, at
// the game's SE volume), which goes on into the title; the second tranche's synthesised crackle is gone. Without the music no stream
// opens. no-precompile-sound.flag: no sound at all; no-precompile-music.flag: the fire's sound alone; no-title-crackle.flag (22_setup:
// no observer, no stop) leaves the fire's sound out too (the step review's L3: never an event without its stop).
struct Music {std::vector<int16_t> frames;uint32_t rate{};};   // interleaved stereo
std::shared_ptr<const Music> music;   // under background_lock (made on the picture's thread)
bool DecodeMusic(const std::wstring& path,Music& out){
    // The final review's L5: Media Foundation delay-loaded (Build-GameBootstrap's link line); absent (a Windows N without its pack), no music.
    // Security audit L1 (2026-11-23): from System32 only, never a namesake in the game's folder (the delay-load then finds them loaded).
    if(!LoadLibraryExW(L"mfplat.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32) || !LoadLibraryExW(L"mfreadwrite.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32))return false;
    bool ok=false;const HRESULT co=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    if(SUCCEEDED(MFStartup(MF_VERSION,MFSTARTUP_LITE))){
        {Microsoft::WRL::ComPtr<IMFSourceReader> reader;Microsoft::WRL::ComPtr<IMFMediaType> wanted,got;
            if(SUCCEEDED(MFCreateSourceReaderFromURL(path.c_str(),nullptr,&reader)) && SUCCEEDED(MFCreateMediaType(&wanted)) &&
               SUCCEEDED(wanted->SetGUID(MF_MT_MAJOR_TYPE,MFMediaType_Audio)) && SUCCEEDED(wanted->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_PCM)) &&
               SUCCEEDED(wanted->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS,2)) && SUCCEEDED(wanted->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE,16)) &&
               SUCCEEDED(reader->SetStreamSelection(DWORD(MF_SOURCE_READER_ALL_STREAMS),FALSE)) &&
               SUCCEEDED(reader->SetStreamSelection(DWORD(MF_SOURCE_READER_FIRST_AUDIO_STREAM),TRUE)) &&
               SUCCEEDED(reader->SetCurrentMediaType(DWORD(MF_SOURCE_READER_FIRST_AUDIO_STREAM),nullptr,wanted.Get())) &&
               SUCCEEDED(reader->GetCurrentMediaType(DWORD(MF_SOURCE_READER_FIRST_AUDIO_STREAM),&got))){
                UINT32 rate{},channels{},bits{};got->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND,&rate);got->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS,&channels);got->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE,&bits);
                if(rate>=8000 && rate<=192000 && channels==2 && bits==16){
                    out.rate=rate;const size_t kMaxFrames=size_t(rate)*60*12;   // twelve minutes at most (L6: at the stream's rate)
                    for(;;){DWORD flags{};Microsoft::WRL::ComPtr<IMFSample> sample;
                        if(FAILED(reader->ReadSample(DWORD(MF_SOURCE_READER_FIRST_AUDIO_STREAM),0,nullptr,&flags,nullptr,&sample)))break;
                        if(flags&MF_SOURCE_READERF_ENDOFSTREAM){ok=!out.frames.empty();break;}
                        if(flags&MF_SOURCE_READERF_ERROR)break;   // L4: no reader call after an error
                        if(flags&MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED){   // L4, the final review's L6: kept only at the same rate and layout
                            Microsoft::WRL::ComPtr<IMFMediaType> now;UINT32 r2{},c2{},b2{};
                            if(FAILED(reader->GetCurrentMediaType(DWORD(MF_SOURCE_READER_FIRST_AUDIO_STREAM),&now)))break;
                            now->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND,&r2);now->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS,&c2);now->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE,&b2);
                            if(r2!=rate || c2!=2 || b2!=16)break;}
                        if(!sample)continue;
                        Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer;BYTE* data{};DWORD length{};
                        if(FAILED(sample->ConvertToContiguousBuffer(&buffer)) || FAILED(buffer->Lock(&data,nullptr,&length)))break;
                        const size_t samples=length/2;const auto* pcm=reinterpret_cast<const int16_t*>(data);
                        try{out.frames.insert(out.frames.end(),pcm,pcm+samples);}catch(...){buffer->Unlock();out.frames.clear();break;}   // L5: unlocked, MF shut down
                        buffer->Unlock();
                        if(out.frames.size()/2>kMaxFrames){ok=true;break;}}}}}
        MFShutdown();}
    if(SUCCEEDED(co))CoUninitialize();
    if(out.frames.size()%2)out.frames.pop_back();
    return ok;}
std::wstring MusicPath(){
    for(const wchar_t* name:{L"rekindled-precompile-music.flac",L"rekindled-precompile-music.mp3",L"rekindled-precompile-music.wav"}){
        const auto beside=ModuleSibling(name);if(!beside.empty() && GetFileAttributesW(beside.c_str())!=INVALID_FILE_ATTRIBUTES)return beside;
        std::array<wchar_t,MAX_PATH> local{};const DWORD chars=GetEnvironmentVariableW(L"LOCALAPPDATA",local.data(),MAX_PATH);
        if(chars && chars<MAX_PATH){std::wstring path=std::wstring(local.data())+L"\\DS3-DX12\\"+name;if(GetFileAttributesW(path.c_str())!=INVALID_FILE_ATTRIBUTES)return path;}}
    return {};}
// Made on the picture's thread once it is published (the second step review's L4).
void MakeSound(){
    static const bool off=ModuleFlag(L"no-precompile-sound.flag"),no_music=ModuleFlag(L"no-precompile-music.flag");if(off)return;
    LARGE_INTEGER t0{},t1{},f{};QueryPerformanceCounter(&t0);QueryPerformanceFrequency(&f);
    const auto path=no_music?std::wstring{}:MusicPath();if(path.empty())return;   // no music, no stream (the fire's sound is the game's)
    // The step review's L1 (a): the game's title music held from now, before the decode (a title request during it waited too).
    {const std::lock_guard hold(background_lock);if(released)return;title_bgm_held=true;}
    auto tune=std::make_shared<Music>();const bool decoded=DecodeMusic(path,*tune);
    QueryPerformanceCounter(&t1);char row[200]{};
    sprintf_s(row,"{\"event\":\"native_precompile_music\",\"found\":true,\"decoded\":%s,\"rate\":%u,\"seconds\":%.1f,\"ms\":%.0f}\n",decoded?"true":"false",
        decoded?tune->rate:0u,decoded?double(tune->frames.size()/2)/double(tune->rate):0.0,double(t1.QuadPart-t0.QuadPart)*1000.0/double(f.QuadPart));Log(row);
    const std::lock_guard hold(background_lock);if(released || !decoded)title_bgm_held=false;if(released || !decoded)return;
    music=std::move(tune);}
// The mixer: four 100 ms buffers on a waveOut stream, refilled as each one is done; gains ramped per frame.
struct Mixer {std::atomic<bool> closing{};};
std::shared_ptr<Mixer> mixer;bool playing{},music_done{},mixer_launched{};   // mixer_launched: a mixer lets the title's music go at its own end (M1)   // music_done: once stopped, a restart plays no music (L1 c)
constexpr float kMusicGain=0.8f,kMusicFadeInS=2.f,kFadeOutS=1.5f;
void StartMusic(){
    if(playing || music_done)return;   // one mixer at a time; a stream that fails to open logs it (its row) and is not retried
    std::shared_ptr<const Music> tune;{const std::lock_guard hold(background_lock);tune=music;}
    if(!tune || tune->frames.size()<2)return;   // decoded with the picture (none yet, or no music)
    const uint32_t rate=tune->rate;auto state=std::make_shared<Mixer>();
    // The game's title music waits (15 HoldTitleBgm, held since MakeSound) until this mixer's fade is over: one music at a time.
    std::thread worker([state,tune,rate]{try{
        WAVEFORMATEX format{};format.wFormatTag=WAVE_FORMAT_PCM;format.nChannels=2;format.nSamplesPerSec=rate;format.wBitsPerSample=16;
        format.nBlockAlign=4;format.nAvgBytesPerSec=rate*4;
        const uint32_t chunk=rate/10;std::array<std::vector<int16_t>,4> blocks;for(auto& block:blocks)block.assign(size_t(chunk)*2,0);   // the final review's L3: sized first
        HANDLE ready=CreateEventW(nullptr,FALSE,FALSE,nullptr);HWAVEOUT out{};
        const bool stream_open=ready && waveOutOpen(&out,WAVE_MAPPER,&format,DWORD_PTR(ready),0,CALLBACK_EVENT)==MMSYSERR_NOERROR;
        char row[128]{};sprintf_s(row,"{\"event\":\"native_precompile_sound\",\"playing\":%s,\"music\":true,\"rate\":%u}\n",stream_open?"true":"false",rate);Log(row);
        bool failed=!stream_open;
        if(stream_open){
            std::array<WAVEHDR,4> headers{};
            uint64_t frame=0,fade_from=~uint64_t{0};const uint64_t music_frames=tune->frames.size()/2;
            const uint64_t fade_in=uint64_t(kMusicFadeInS*float(rate)),fade_out=uint64_t(kFadeOutS*float(rate));bool done=false;
            const auto fill=[&](std::vector<int16_t>& block){
                for(uint32_t i=0;i<chunk;++i,++frame){
                    if(fade_from==~uint64_t{0} && state->closing.load(std::memory_order_relaxed))fade_from=frame;
                    const float out_gain=fade_from==~uint64_t{0}?1.f:(std::max)(0.f,1.f-float(frame-fade_from)/float(fade_out));
                    if(fade_from!=~uint64_t{0} && frame-fade_from>=fade_out)done=true;
                    const uint64_t m=frame%music_frames;const float in=(std::min)(1.f,float(frame)/float(fade_in))*kMusicGain/32768.f;
                    const float l=float(tune->frames[size_t(m)*2])*in,r=float(tune->frames[size_t(m)*2+1])*in;
                    block[size_t(i)*2]=int16_t(std::lround(std::clamp(l*out_gain,-1.f,1.f)*32767.f));block[size_t(i)*2+1]=int16_t(std::lround(std::clamp(r*out_gain,-1.f,1.f)*32767.f));}};
            for(size_t b=0;b<blocks.size();++b){fill(blocks[b]);headers[b].lpData=reinterpret_cast<LPSTR>(blocks[b].data());headers[b].dwBufferLength=DWORD(blocks[b].size()*2);
                if(waveOutPrepareHeader(out,&headers[b],sizeof(WAVEHDR))!=MMSYSERR_NOERROR || waveOutWrite(out,&headers[b],sizeof(WAVEHDR))!=MMSYSERR_NOERROR)failed=true;}
            while(!done && !failed && !precompile_sound_stop.load(std::memory_order_relaxed)){   // L3: a failed write ends it (no slot idles)
                WaitForSingleObject(ready,200);
                for(size_t b=0;b<blocks.size() && !done;++b)if(headers[b].dwFlags&WHDR_DONE){
                    waveOutUnprepareHeader(out,&headers[b],sizeof(WAVEHDR));fill(blocks[b]);headers[b].dwFlags=0;
                    if(waveOutPrepareHeader(out,&headers[b],sizeof(WAVEHDR))!=MMSYSERR_NOERROR || waveOutWrite(out,&headers[b],sizeof(WAVEHDR))!=MMSYSERR_NOERROR){failed=true;break;}}}
            if(done && !failed && !precompile_sound_stop.load(std::memory_order_relaxed))Sleep(DWORD(blocks.size())*100);   // the queued fade's tail
            waveOutReset(out);for(auto& header:headers)if(header.dwFlags&WHDR_PREPARED)waveOutUnprepareHeader(out,&header,sizeof(WAVEHDR));waveOutClose(out);}
        if(ready)CloseHandle(ready);
        if(failed){char gone[96]{};sprintf_s(gone,"{\"event\":\"native_precompile_sound\",\"failed\":true}\n");Log(gone);}
    }catch(...){}
        title_bgm_held=false;});   // the step review's L1 (b), L2: the title's music let go once this sound is over, faded or failed
    worker.detach();mixer=state;playing=true;mixer_launched=true;}   // the final review's L2: set once the thread exists
void StopMusic(){music_done=true;if(!mixer_launched)title_bgm_held=false;if(!playing)return;   // the final review's M1: a launched mixer lets the title's music go after its fade
    if(mixer)mixer->closing=true;mixer.reset();playing=false;}
// The page's end (the step review's M4): the sound stopped, the picture, the fire's atlas and the sound's bytes freed for the session.
// The bonfire's own textures for the page's fire (Thomas, 13:10; r12h-ff-probe1: resident while the page is up): s11409, the flame's
// noise (f000001020's flame layers), and s02000, its smoke puff; looked up by name every 30 frames until found, kept until the page ends (the overlay pass keeps its own reference until it closes, L3).
std::shared_ptr<const bootstrap::NativeTextureRecord> bonfire_noise,bonfire_smoke;
void Release(){StopMusic();title_crackle::PageOver();bonfire_noise.reset();bonfire_smoke.reset();const std::lock_guard hold(background_lock);released=true;background.reset();fire.reset();music.reset();}
// The page's sound from its drawn frames: the music's stream and the game's fire event (the fifth tranche). Lost: both stopped.
bool SoundOff() noexcept {static const bool off=ModuleFlag(L"no-precompile-sound.flag");return off;}
void StartSound(){if(SoundOff())return;StartMusic();if(title_crackle_stop)title_crackle::StartFromPage();}   // the event only with its stop installed (22_setup: a crackle that could never stop)
void StopSound(){StopMusic();title_crackle::StopFromPage();}
// The particles and the fog.
struct Mote {float u{},v{},du{},dv{},life{},age{},size{},phase{},warm{};};
std::array<Mote,180> embers;std::array<Mote,140> ash;std::array<Mote,150> dust;
struct Bank {float u{},v{},size{},alpha{},speed{},turn{};};std::array<Bank,9> fog;bool seeded{};
// Where the sword enters the rocks (the picture's 0.704, 0.731, read on r12h-fx-page2's still), and the ground's other glowing spots.
constexpr float kFireU=0.704f,kFireV=0.731f;
constexpr std::array<std::array<float,3>,4> kGlows{{{0.704f,0.735f,1.f},{0.206f,0.758f,0.45f},{0.155f,0.792f,0.35f},{0.735f,0.755f,0.5f}}};
bool placed_layers{};   // Draw: the picture is Thomas's (the second step review's L1: the embers' fire and cape sources only then)
void SpawnEmber(Mote& m,bool anywhere){
    const float pick=placed_layers?Random():0.9f;
    if(pick<0.62f){m.u=kFireU+(Random()-0.5f)*0.06f;m.v=kFireV+0.01f-Random()*0.02f;}          // from the fire
    else if(pick<0.82f){m.u=0.12f+Random()*0.3f;m.v=0.3f+Random()*0.4f;}                         // from the cape's glowing seams
    else{m.u=0.1f+Random()*0.7f;m.v=0.72f+Random()*0.1f;}                                        // from the ground
    m.du=-0.004f-0.01f*Random();m.dv=-(0.025f+0.07f*Random());m.life=2.5f+3.5f*Random();m.age=anywhere?Random()*m.life:0.f;
    m.size=1.0f+2.6f*Random();m.phase=6.2832f*Random();m.warm=Random();}
void SpawnAsh(Mote& m,bool anywhere){m.u=anywhere?Random():1.02f+0.05f*Random();m.v=anywhere?Random():Random()*0.9f;m.du=0;m.dv=0.006f+0.018f*Random();
    m.life=0;m.age=0;m.size=1.3f+3.f*Random();m.phase=6.2832f*Random();m.warm=Random();}
void SpawnDust(Mote& m){m.u=Random();m.v=Random();m.du=0;m.dv=0;m.life=0;m.age=0;m.size=0.6f+1.f*Random();m.phase=6.2832f*Random();m.warm=Random();}
void Seed(){
    for(auto& m:embers)SpawnEmber(m,true);for(auto& m:ash)SpawnAsh(m,true);for(auto& m:dust)SpawnDust(m);
    // Fog banks: three low over the rocks, three as a haze at mid height, three far away by the cathedral (smaller, paler).
    for(size_t i=0;i<fog.size();++i){auto& b=fog[i];const int band=int(i/3);
        b.u=Random()*1.4f-0.2f;b.v=band==0?0.84f+0.08f*Random():band==1?0.55f+0.15f*Random():0.42f+0.1f*Random();
        b.size=band==0?0.55f+0.3f*Random():band==1?0.45f+0.3f*Random():0.25f+0.15f*Random();
        b.alpha=band==0?0.16f+0.08f*Random():band==1?0.07f+0.05f*Random():0.08f+0.05f*Random();b.speed=band==2?0.4f:band==1?0.7f:1.f;b.turn=Random()<0.5f?0.f:1.f;
        if(band==2)b.u=0.6f+0.45f*Random();}
    seeded=true;}
// The wind: a steady push to the left with gusts (sums of slow sines), 1 its mean.
float Wind(float t) noexcept {return (std::max)(0.3f,1.f+0.35f*std::sin(0.37f*t)+0.25f*std::sin(0.91f*t+1.3f)+0.15f*std::sin(2.3f*t+0.4f));}
// Draws the living background into `frame` (its first quads), the page's `opacity` over everything; false without the picture.
// Thomas (13:10, « récupérer les effets du bonfire pour le feu de l'épée »): the bonfire's effect f000001020 (WAVE1-LAST-EFFECTS-STATIC-PREP
// §2.1) draws its flame from FFX textures 11409 and 10000, its sparks from 2000 and 2001, its haze from 6000, named s%05d in the effects'
// repositories (21_effects NativeTexture); r12h-ff-probe1 found the five in the texture ingress while the page is up (no _a variant).
// precompile-bonfire-textures-test.flag (a look for the pilot): the five textures drawn as they are in a row at the page's top.
void ShowBonfireTextures(ov::Frame& frame,uint32_t height){
    static const bool wanted=ModuleFlag(L"precompile-bonfire-textures-test.flag");if(!wanted)return;
    static constexpr const char* shown[]{"s11409","s10000","s02000","s02001","s06000"};
    static std::array<std::shared_ptr<const bootstrap::NativeTextureRecord>,std::size(shown)> held{};static uint64_t asked{};
    if((++asked%30)==1)for(size_t i=0;i<std::size(shown);++i)if(!held[i] || held[i]->withdrawn.load())held[i]=textures.FindByName(shown[i]);   // the final review's L3
    const float side=float(height)*0.22f;float x=side*0.1f;
    for(size_t i=0;i<std::size(shown);++i){const auto& record=held[i];if(!record)continue;
        ov::Image texture;texture.identity=0x52454B3054580000ull+i;texture.version=record->generation;texture.width=record->description.width;texture.height=record->description.height;
        texture.external=record->resource.Get();texture.owner=record;const int32_t index=ov::ImageIndex(frame,texture);if(index<0)continue;
        frame.quads.push_back({x,side*0.1f,x+side,side*1.1f,0,0,float(texture.width),float(texture.height),{1,1,1,1},index});x+=side*1.1f;}}
bool Draw(ov::Frame& frame,uint32_t width,uint32_t height,float opacity){
    static const bool off=ModuleFlag(L"no-precompile-background.flag");if(off)return false;
    StartBackground(width,height);
    const auto picture=Background(),flames=Fire();if(!picture || !flames)return false;
    placed_layers=std::abs(float(picture->width)/float(picture->height)-3840.f/2144.f)<0.009f;   // before the first seeding (below)
    static LARGE_INTEGER start{},last{},frequency{};LARGE_INTEGER now{};QueryPerformanceCounter(&now);
    if(!start.QuadPart){QueryPerformanceFrequency(&frequency);start=last=now;}
    const float t=float(double(now.QuadPart-start.QuadPart)/double(frequency.QuadPart)),dt=std::clamp(float(double(now.QuadPart-last.QuadPart)/double(frequency.QuadPart)),0.f,0.1f);last=now;
    if(!seeded)Seed();
    const float wind=Wind(t);
    opacity*=(std::min)(1.f,t/0.4f);   // the step review's L3: the picture fades in over 0.4 s once decoded (after the page's own fade-in)
    frame.quads.reserve(frame.quads.size()+14300);   // the clamped tiles (5,625 and 7,700) and the particles: about 14,000 at most
    // The picture covers the output, pushed in slowly (4 % over a minute, then held), centred.
    // The final review's L8: the fire, the glows, the cape's and the fire's waves are placed for Thomas's picture: drawn only on a
    // picture of its shape (3840x2144, 1.791:1, within 0.5 %); another picture gets the wind's layers alone (fog, ash, dust, embers).
    const bool placed=placed_layers;
    const float iw=float(picture->width),ih=float(picture->height),zoom=1.f+0.04f*(std::min)(1.f,t/60.f);
    const float scale=(std::max)(float(width)/iw,float(height)/ih)*zoom,dw=iw*scale,dh=ih*scale;
    const float x0=(float(width)-dw)*0.5f,y0=(float(height)-dh)*0.5f;
    const auto X=[&](float u){return x0+u*dw;};const auto Y=[&](float v){return y0+v*dh;};
    const float px=dh/1080.f;   // one pixel of a 1080-line picture, in output pixels
    const int32_t bg=int32_t(frame.images.size());frame.images.push_back(*picture);
    const int32_t dot=int32_t(frame.images.size());frame.images.push_back(Dot());
    const int32_t flame=int32_t(frame.images.size());frame.images.push_back(*flames);
    const int32_t cloud=int32_t(frame.images.size());frame.images.push_back(Cloud());
    static uint64_t looks{};if((++looks%30)==1){
        if(!bonfire_noise || bonfire_noise->withdrawn.load())bonfire_noise=textures.FindByName("s11409");
        if(!bonfire_smoke || bonfire_smoke->withdrawn.load())bonfire_smoke=textures.FindByName("s02000");
        // The final review's L4: only as r12h-ff-look2 saw them (a same-named texture of another kind, a mod's, not drawn).
        if(bonfire_smoke && bonfire_smoke->description.format!=DXGI_FORMAT_BC7_UNORM && bonfire_smoke->description.format!=DXGI_FORMAT_BC7_UNORM_SRGB)bonfire_smoke.reset();
        if(bonfire_noise && (bonfire_noise->description.width<16 || bonfire_noise->description.height<16))bonfire_noise.reset();}
    const auto external=[&](const std::shared_ptr<const bootstrap::NativeTextureRecord>& record,uint64_t identity)->int32_t{
        if(!record)return -1;ov::Image texture;texture.identity=identity;texture.version=record->generation;
        texture.width=record->description.width;texture.height=record->description.height;texture.external=record->resource.Get();texture.owner=record;
        const int32_t index=ov::ImageIndex(frame,texture);return index>=0?index:-1;};
    const int32_t noise=external(bonfire_noise,0x52454B30424E0001ull),smoke=external(bonfire_smoke,0x52454B30424E0002ull);
    frame.quads.push_back({X(0),Y(0),X(1),Y(1),0,0,iw,ih,{1,1,1,opacity},bg});
    // The step review's L5: the moving tiles only while the picture is opaque (blended over it during a fade they showed as patches).
    const bool tiles=placed && opacity>=0.999f;
    // The cape: its region cut in tiles of about 14 output pixels, each sampling the picture a little aside (a wave travelling to the
    // left with the wind, strongest at the free tattered ends, none where the cloth meets the shoulders, the body or the rocks), so its
    // edges match the picture where the field is zero.
    {const float u0=0.f,u1=0.44f,v0=0.16f,v1=0.82f;const int nx=tiles?std::clamp(int((X(u1)-X(u0))/14.f),8,75):0,ny=std::clamp(int((Y(v1)-Y(v0))/14.f),8,75);   // at most 5,625 tiles (the fire's wave the rest of the budget, M2)
        const float amplitude=0.010f*ih*(0.5f+0.5f*wind);   // texels (r12h-fx-page1: 0.0045 barely seen)
        for(int j=0;j<ny;++j)for(int i=0;i<nx;++i){
            const float ua=u0+(u1-u0)*float(i)/float(nx),ub=u0+(u1-u0)*float(i+1)/float(nx),va=v0+(v1-v0)*float(j)/float(ny),vb=v0+(v1-v0)*float(j+1)/float(ny);
            const float uc=(ua+ub)*0.5f,vc=(va+vb)*0.5f;
            const float weight=(1.f-Smooth(0.30f,0.42f,uc))*Smooth(0.16f,0.26f,vc)*(1.f-Smooth(0.70f,0.82f,vc))*(0.4f+0.6f*(1.f-uc/0.44f))*Smooth(0.f,0.05f,uc);   // L6: none at the edge (no smear)
            if(weight<=0.001f)continue;
            const float phase=6.2832f*(uc*2.6f+vc*1.2f)+t*2.0f,flutter=std::sin(phase)+0.35f*std::sin(phase*2.3f+1.7f+t*1.3f)+0.5f*std::sin(t*0.8f+uc*3.f);
            const float shift_u=amplitude*weight*flutter,shift_v=0.5f*amplitude*weight*std::sin(phase*0.8f+0.9f);
            frame.quads.push_back({X(ua),Y(va),X(ub),Y(vb),ua*iw+shift_u,va*ih+shift_v,ub*iw+shift_u,vb*ih+shift_v,{1,1,1,opacity},bg});}}
    // The heat haze above the fire (the step review's H1: drawn before the fog, the glows and the fire, which it hid): tiles of the
    // picture sampled a texel or two aside, rising quickly, strongest near the flames.
    // Thomas (12:43, « l'effet que t'as mis sur la cape, mets-le sur le feu de l'épée aussi »): the picture's own fire and sparks at the
    // sword's foot wave as the cape does, faster and rising, strongest on the flames, none at the rocks' line below them.
    {const float hu0=0.64f,hu1=0.77f,hv0=0.48f,hv1=0.775f;const int nx=tiles?std::clamp(int((X(hu1)-X(hu0))/4.f),4,70):0,ny=std::clamp(int((Y(hv1)-Y(hv0))/4.f),4,110);   // M2: 4 px to about 1440 lines   // 4 px (r12h-ff-fire1: 6 px steps showed on the blade's edge)
        const float amplitude=0.0022f*ih;   // the haze's; the flames' up to 2.5 times it (below)
        for(int j=0;j<ny;++j)for(int i=0;i<nx;++i){
            const float ua=hu0+(hu1-hu0)*float(i)/float(nx),ub=hu0+(hu1-hu0)*float(i+1)/float(nx),va=hv0+(hv1-hv0)*float(j)/float(ny),vb=hv0+(hv1-hv0)*float(j+1)/float(ny);
            const float uc=(ua+ub)*0.5f,vc=(va+vb)*0.5f;
            const float weight=Smooth(hu0,hu0+0.025f,uc)*(1.f-Smooth(hu1-0.025f,hu1,uc))*Smooth(hv0,hv0+0.1f,vc)*(1.f-Smooth(hv1-0.03f,hv1,vc))*
                (1.f+1.5f*Smooth(0.64f,0.71f,vc));   // up to 2.5 times on the flames (r12h-ff-fire1: four times broke the blade into steps)
            if(weight<=0.001f)continue;
            const float phase=6.2832f*vc*13.f+uc*38.f+t*2.f,flicker=std::sin(phase)+0.45f*std::sin(phase*2.1f+t*1.3f+1.1f);   // the cape's speed in time (Thomas, 13:12: « la même vitesse que pour la cape »)
            const float shift_u=amplitude*weight*flicker,shift_v=0.7f*amplitude*weight*std::sin(phase*1.3f+2.f);
            frame.quads.push_back({X(ua),Y(va),X(ub),Y(vb),ua*iw+shift_u,va*ih+shift_v,ub*iw+shift_u,vb*ih+shift_v,{1,1,1,opacity},bg});}}
    // The fog: banks drifting left with the wind (the far ones slower), coming back on the right.
    for(auto& b:fog){b.u-=0.02f*b.speed*wind*dt;if(b.u+b.size*0.9f<0.f)b.u=1.f+b.size*0.9f;
        const float half=b.size*0.5f,aspect=0.42f;const float a=1.6f*b.alpha*opacity*(0.85f+0.15f*std::sin(t*0.3f+b.u*5.f));
        const float uu0=b.turn*128.f,uu1=128.f-uu0;
        frame.quads.push_back({X(b.u-half),Y(b.v-half*aspect*dw/dh),X(b.u+half),Y(b.v+half*aspect*dw/dh),uu0,0,uu1,128,{0.62f,0.58f,0.52f,a},cloud});}
    // The glows: the fire's light on the rocks and the cape's foot, flickering; smaller at the ground's other embers.
    if(placed)for(const auto& g:kGlows){const float f=0.75f+0.25f*Noise(t*3.f+g[0]*40.f,g[1]*20.f,21)+0.1f*std::sin(t*11.f+g[0]*30.f);
        const float r=0.085f*g[2]*dh;
        frame.quads.push_back({X(g[0])-r*1.6f,Y(g[1])-r,X(g[0])+r*1.6f,Y(g[1])+r,0,0,32,32,{1.f,0.42f,0.12f,0.30f*g[2]*f*opacity},dot});}
    // The fire at the sword's foot: four quads of the atlas (the middle one tallest, the others smaller, two mirrored), their foot
    // just under the crack so the rocks' line hides their base; two neighbouring frames of the atlas crossfaded.
    if(placed){const float base=Y(kFireV)+0.006f*dh;
        const std::array<std::array<float,4>,4> tongues{{{0.f,1.f,0.f,0.65f},{-0.013f,0.7f,1.f,0.55f},{0.012f,0.62f,0.f,0.5f},{-0.004f,0.5f,1.f,0.45f}}};   // u offset, size, mirrored, alpha (12:43: fainter, the picture's own flames waving under them)
        const float at=std::fmod(t*0.6f,kFireLoopS)/kFireLoopS*float(kFireFrames),   // played at 0.6 (Thomas, 13:10: « un peu trop rapide »)
            mix=at-std::floor(at);const uint32_t f0=uint32_t(at)%kFireFrames,f1=(f0+1)%kFireFrames;
        for(const auto& g:tongues){const float h=0.19f*dh*g[1],w=h*0.55f,cx=X(kFireU+g[0]);
            for(const auto& [f,share]:{std::pair{f0,1.f-mix},std::pair{f1,mix}}){
                const float ox=float((f%kFireColumns)*kFireW),oy=float((f/kFireColumns)*kFireH),ua=g[2]>0.5f?ox+float(kFireW):ox,ub=g[2]>0.5f?ox:ox+float(kFireW);
                if(noise<0){frame.quads.push_back({cx-w*0.5f,base-h,cx+w*0.5f,base,ua,oy,ub,oy+float(kFireH),{1,1,1,g[3]*share*opacity},flame});continue;}
                // The bonfire's way (f000001020's flame: its noise layers scrolling through the flame): the heat broken up by s11409
                // rising through it, 0.7 of the texture across, a different offset per tongue (the overlay's fire mode).
                const float nw=float(bonfire_noise->description.width),nh=float(bonfire_noise->description.height);   // L2: its own size
                const float nu=nw*(0.21f*float(&g-tongues.data())),nv=nh*(0.37f*float(&g-tongues.data())+t*0.22f);   // the final review's M1: rising (v grows downward)
                ov::Quad tongue{cx-w*0.5f,base-h,cx+w*0.5f,base,ua,oy,ub,oy+float(kFireH),{1.15f,1.f,0.95f,g[3]*share*opacity},flame};   // the alpha a linear gain (M1)
                tongue.image2=noise;tongue.u2_0=nu;tongue.v2_0=nv;tongue.u2_1=nu+nw*0.7f;tongue.v2_1=nv+nh*1.1f;tongue.mode=1;frame.quads.push_back(tongue);}}
        // Its smoke: s02000 puffs rising from the flames' tips, growing, drifting left with the wind, fading.
        if(smoke>=0)for(int k=0;k<6;++k){const float life=3.2f,age=std::fmod(t+float(k)*life/6.f,life)/life;
            const float size=(0.05f+0.11f*age)*dh,puff_x=X(kFireU)-age*0.03f*wind*dw+std::sin(float(k)*2.1f+t*0.6f)*0.004f*dw,puff_y=Y(kFireV)-0.12f*dh-age*0.2f*dh;
            const float a=0.32f*opacity*std::sin(3.1416f*age);if(a<=0.004f)continue;
            frame.quads.push_back({puff_x-size,puff_y-size,puff_x+size,puff_y+size,0,0,float(bonfire_smoke->description.width),float(bonfire_smoke->description.height),{0.42f,0.39f,0.36f,a},smoke});}}
    // The embers: rising, swaying, carried left by the wind, flickering, cooling and fading out.
    for(auto& m:embers){m.age+=dt;if(m.age>=m.life)SpawnEmber(m,false);
        m.phase+=dt*(2.f+m.warm*2.f);m.u+=(m.du-0.018f*wind)*dt+0.0006f*std::sin(m.phase);m.v+=m.dv*dt;
        if(m.v<-0.02f || m.u<-0.02f){SpawnEmber(m,false);continue;}
        const float k=m.age/m.life,a=opacity*Smooth(0.f,0.08f,k)*(1.f-Smooth(0.5f,1.f,k))*(0.6f+0.4f*std::sin(t*9.f+m.phase*3.f));
        if(a<=0.004f)continue;
        const float r=m.size*px*(1.f-0.5f*k),x=X(m.u),y=Y(m.v);const float g=0.35f+0.4f*m.warm*(1.f-k);
        const float trail=std::abs(m.dv)*dh*0.06f;   // a short streak below it along its rise (the eye's blur of a fast spark)
        frame.quads.push_back({x-r*4.f,y-r*4.f,x+r*4.f,y+r*4.f+trail,0,0,32,32,{1.f,g*0.8f,0.1f,a*0.35f},dot});
        frame.quads.push_back({x-r,y-r,x+r,y+r+trail,0,0,32,32,{1.f,g+0.25f,0.3f,a},dot});}
    // The ash: dark and pale flakes blown left, falling slowly, tumbling (their size breathing).
    for(auto& m:ash){m.phase+=dt*(1.5f+m.warm*2.f);m.u-=(0.045f+0.035f*m.warm)*wind*dt;m.v+=m.dv*dt+0.0008f*std::sin(m.phase*0.7f);
        if(m.u<-0.02f || m.v>1.02f)SpawnAsh(m,false);
        const float tumble=0.55f+0.45f*std::abs(std::sin(m.phase)),r=m.size*px,x=X(m.u),y=Y(m.v),grey=m.warm<0.6f?0.18f+0.1f*m.warm:0.55f+0.15f*m.warm;
        frame.quads.push_back({x-r,y-r*tumble,x+r,y+r*tumble,0,0,32,32,{grey,grey*0.97f,grey*0.94f,(0.5f+0.35f*m.warm)*opacity},dot});}
    // The dust: faint motes wandering, carried a little by the wind, twinkling where the light catches them.
    for(auto& m:dust){m.phase+=dt*(0.5f+m.warm);
        m.u+=(0.006f*(Noise(m.phase*0.3f,m.v*7.f,41)-0.5f)-0.004f*wind)*dt;m.v+=0.004f*(Noise(m.u*7.f,m.phase*0.3f,42)-0.5f)*dt;
        if(m.u<0.f)m.u+=1.f;if(m.u>1.f)m.u-=1.f;if(m.v<0.f)m.v+=1.f;if(m.v>1.f)m.v-=1.f;
        const float a=opacity*(0.1f+0.22f*m.warm)*(0.5f+0.5f*std::sin(t*(1.f+m.warm*2.f)+m.phase*4.f));if(a<=0.004f)continue;
        const float r=m.size*px,x=X(m.u),y=Y(m.v);
        frame.quads.push_back({x-r,y-r,x+r,y+r,0,0,32,32,{0.95f,0.88f,0.74f,a},dot});}
    ShowBonfireTextures(frame,height);   // a test tool's look only (its flag)
    // The text's ground: the bottom darkened in bands (the bar and the text stay readable over the rocks).
    for(int i=0;i<10;++i){const float a=float(i)/10.f,b=float(i+1)/10.f,y0b=float(height)*(0.70f+0.30f*a),y1b=float(height)*(0.70f+0.30f*b);
        frame.quads.push_back({0,y0b,float(width),y1b,0,0,0,0,{0,0,0,opacity*0.62f*Smooth(0.f,1.f,(a+b)*0.5f)},-1});}
    return true;}
}
// I-0132 R12d, R12h: the precompile page. While the pipeline store's precompile (its list of earlier sessions' pipelines) or the
// native passes' warm-up (01_state.inc StartNativeWarm) runs, a page of its own over the whole picture (R12h, Thomas 2026-11-12 02:34,
// « une nouvelle page même »; R12d drew a line and a bar at the bottom right over the start logos and the title): black, at the
// bottom, centred (Thomas, 2026-11-12 10:06: « en bas de l'écran, toujours centré », the bar longer: 700 units, was 440; 10:45 « raccourcir un peu » the bar and the text from 840 and 34, 22, 17),
// « Compilation des shaders » in the titles' font, a bar DS3's rule colour over a dark track, the percentage, and one line saying when
// the step is long. Laid out in DS3's 1280x720 units scaled by the output height (D-0151). DS3's fonts once its menus loaded them (the
// bar alone before). Shown only while the work still runs 3 s after our first Step and 30 frames (the precompile and the warm-up
// start during Start, before it): the pilot's threshold for D-0273's « only when a compilation really happens », a warm start being
// over within about 1 s on this clock (r12d-pt2 1,028 ms; 0.2 to 1.3 s in R12h's runs; seconds, not frames alone, from the review
// of R12h's first draft, its L3); then held at least kPageMinS (no flash), faded in over kPageFadeInS and out over kPageFadeOutS once
// both are done. A page whose progress has not moved for kPageStallS closes anyway with a row (that draft review's M1: a store whose
// precompile never starts, Precompile without a device, or a compile that hangs would otherwise hold a black page for the session).
// The game boots behind it: R12e's title is built only once the page is over (Step, precompile_screen_over), so its word starts
// its intro after the page, on the title's first uncovered frame. One row when shown and one when over. While the page is up no
// input reaches the game (R12h's L7, Thomas 2026-11-12; 04 input_focus, no-precompile-input-block.flag for the A/B): its end's row
// counts the polls it held back.
struct Progress {uint64_t done{},total{};bool finished{};};
Progress PrecompileProgress() noexcept {
    const auto stats=Pipelines().Stats();Progress p;
    p.total=stats.precompile_total+native_warm_total.load(std::memory_order_relaxed);
    p.done=(std::min)(stats.precompiled+stats.precompile_failed,stats.precompile_total)+(std::min)(native_warm_done.load(std::memory_order_relaxed),native_warm_total.load(std::memory_order_relaxed));
    p.finished=stats.precompile_done && native_warm_finished.load(std::memory_order_relaxed);return p;}
// I-0132 R12h's sixth tranche (Thomas, 2026-11-12 20:12: the page in the game's other languages where the translation is sure, English
// otherwise): the page's words in each language a Steam PC build can run in (FONTS-LANGUAGES-NATIVE §1: thirteen ordinals of
// [14479BA10]; every other Steam language is filtered to engUS's 8), with the language's own two families (§2, MenuFont_01 and _02:
// the fonts[] slots above). The language is decided once, when both its families are read (the game loads them for the language it
// chose, so the ordinal read is the one the fonts follow) and every code of its words is in their chains; otherwise English, also after
// kPageWordsWaitS with its families still unread, and no words at all if English is not covered either. Until then no words are drawn
// (no language flipping on the page). One row names the choice and the first missing code.
struct PageWords {int32_t ordinal;const char* name;uint8_t primary,secondary;bool spaced_percent;const char* title;const char* note;};
constexpr PageWords kPageWords[]={   // the first entry English, the fallback
    {8,"engUS",2,0,false,
        "Compiling shaders",
        "This step is only long on the first launch and after a graphics driver update."},
    {2,"fraFR",1,0,true,
        "Compilation des shaders",
        "Cette \xC3\xA9tape n'est longue qu'au premier lancement et apr\xC3\xA8s une mise \xC3\xA0 jour du pilote graphique."},
    {3,"itaIT",1,0,false,
        "Compilazione degli shader",
        "Questo passaggio richiede molto tempo solo al primo avvio e dopo un aggiornamento del driver grafico."},
    {4,"deuDE",1,0,true,
        "Shader werden kompiliert",
        "Dieser Schritt dauert nur beim ersten Start und nach einem Grafiktreiber-Update l\xC3\xA4nger."},
    {5,"spaES",1,0,false,
        "Compilaci\xC3\xB3n de shaders",
        "Este paso solo tarda bastante en el primer inicio y tras actualizar el controlador gr\xC3\xA1" "fico."},
    {6,"spaAR",1,0,false,
        "Compilaci\xC3\xB3n de shaders",
        "Este paso solo tarda bastante en el primer inicio y tras actualizar el controlador gr\xC3\xA1" "fico."},
    {7,"porBR",1,0,false,
        "Compilando shaders",
        "Esta etapa s\xC3\xB3 demora na primeira execu\xC3\xA7\xC3\xA3o e ap\xC3\xB3s uma atualiza\xC3\xA7\xC3\xA3o do driver de v\xC3\xAD" "deo."},
    {10,"polPL",4,4,false,
        "Kompilowanie shader\xC3\xB3w",
        "Ten etap trwa d\xC5\x82ugo tylko przy pierwszym uruchomieniu i po aktualizacji sterownika karty graficznej."},
    {11,"rusRU",1,1,true,
        "\xD0\x9A\xD0\xBE\xD0\xBC\xD0\xBF\xD0\xB8\xD0\xBB\xD1\x8F\xD1\x86\xD0\xB8\xD1\x8F \xD1\x88\xD0\xB5\xD0\xB9\xD0\xB4\xD0\xB5\xD1\x80\xD0\xBE\xD0\xB2",
        "\xD0\xAD\xD1\x82\xD0\xBE\xD1\x82 \xD1\x8D\xD1\x82\xD0\xB0\xD0\xBF \xD0\xB4\xD0\xBE\xD0\xBB\xD0\xB3\xD0\xB8\xD0\xB9 \xD1\x82\xD0\xBE\xD0\xBB\xD1\x8C\xD0\xBA\xD0\xBE \xD0\xBF\xD1\x80\xD0\xB8 \xD0\xBF\xD0\xB5\xD1\x80\xD0\xB2\xD0\xBE\xD0\xBC \xD0\xB7\xD0\xB0\xD0\xBF\xD1\x83\xD1\x81\xD0\xBA\xD0\xB5 \xD0\xB8 \xD0\xBF\xD0\xBE\xD1\x81\xD0\xBB\xD0\xB5 \xD0\xBE\xD0\xB1\xD0\xBD\xD0\xBE\xD0\xB2\xD0\xBB\xD0\xB5\xD0\xBD\xD0\xB8\xD1\x8F \xD0\xB2\xD0\xB8\xD0\xB4\xD0\xB5\xD0\xBE\xD0\xB4\xD1\x80\xD0\xB0\xD0\xB9\xD0\xB2\xD0\xB5\xD1\x80\xD0\xB0."},
    {0,"jpnJP",2,3,false,
        "\xE3\x82\xB7\xE3\x82\xA7\xE3\x83\xBC\xE3\x83\x80\xE3\x83\xBC\xE3\x82\x92\xE3\x82\xB3\xE3\x83\xB3\xE3\x83\x91\xE3\x82\xA4\xE3\x83\xAB\xE4\xB8\xAD",
        "\xE3\x81\x93\xE3\x81\xAE\xE5\x87\xA6\xE7\x90\x86\xE3\x81\xAB\xE6\x99\x82\xE9\x96\x93\xE3\x81\x8C\xE3\x81\x8B\xE3\x81\x8B\xE3\x82\x8B\xE3\x81\xAE\xE3\x81\xAF\xE3\x80\x81\xE5\x88\x9D\xE5\x9B\x9E\xE8\xB5\xB7\xE5\x8B\x95\xE6\x99\x82\xE3\x81\xA8\xE3\x82\xB0\xE3\x83\xA9\xE3\x83\x95\xE3\x82\xA3\xE3\x83\x83\xE3\x82\xAF\xE3\x83\x89\xE3\x83\xA9\xE3\x82\xA4\xE3\x83\x90\xE3\x83\xBC\xE3\x81\xAE\xE6\x9B\xB4\xE6\x96\xB0\xE5\xBE\x8C\xE3\x81\xAE\xE3\x81\xBF\xE3\x81\xA7\xE3\x81\x99\xE3\x80\x82"},
    {20,"zhoCN",6,6,false,
        "\xE6\xAD\xA3\xE5\x9C\xA8\xE7\xBC\x96\xE8\xAF\x91\xE7\x9D\x80\xE8\x89\xB2\xE5\x99\xA8",
        "\xE6\xAD\xA4\xE6\xAD\xA5\xE9\xAA\xA4\xE4\xBB\x85\xE5\x9C\xA8\xE9\xA6\x96\xE6\xAC\xA1\xE5\x90\xAF\xE5\x8A\xA8\xE5\x92\x8C\xE6\x9B\xB4\xE6\x96\xB0\xE6\x98\xBE\xE5\x8D\xA1\xE9\xA9\xB1\xE5\x8A\xA8\xE7\xA8\x8B\xE5\xBA\x8F\xE5\x90\x8E\xE8\x80\x97\xE6\x97\xB6\xE8\xBE\x83\xE9\x95\xBF\xE3\x80\x82"},
    {18,"zhoTW",7,3,false,
        "\xE6\xAD\xA3\xE5\x9C\xA8\xE7\xB7\xA8\xE8\xAD\xAF\xE8\x91\x97\xE8\x89\xB2\xE5\x99\xA8",
        "\xE6\xAD\xA4\xE6\xAD\xA5\xE9\xA9\x9F\xE5\x83\x85\xE5\x9C\xA8\xE9\xA6\x96\xE6\xAC\xA1\xE5\x95\x9F\xE5\x8B\x95\xE5\x8F\x8A\xE6\x9B\xB4\xE6\x96\xB0\xE9\xA1\xAF\xE7\xA4\xBA\xE5\x8D\xA1\xE9\xA9\x85\xE5\x8B\x95\xE7\xA8\x8B\xE5\xBC\x8F\xE5\xBE\x8C\xE9\x9C\x80\xE8\xA6\x81\xE8\xBC\x83\xE9\x95\xB7\xE6\x99\x82\xE9\x96\x93\xE3\x80\x82"},
    {19,"korKR",5,5,false,
        "\xEC\x85\xB0\xEC\x9D\xB4\xEB\x8D\x94 \xEC\xBB\xB4\xED\x8C\x8C\xEC\x9D\xBC \xEC\xA4\x91",
        "\xEC\x9D\xB4 \xEB\x8B\xA8\xEA\xB3\x84\xEB\x8A\x94 \xEC\xB2\x98\xEC\x9D\x8C \xEC\x8B\xA4\xED\x96\x89\xED\x95\xA0 \xEB\x95\x8C\xEC\x99\x80 \xEA\xB7\xB8\xEB\x9E\x98\xED\x94\xBD \xEB\x93\x9C\xEB\x9D\xBC\xEC\x9D\xB4\xEB\xB2\x84\xEB\xA5\xBC \xEC\x97\x85\xEB\x8D\xB0\xEC\x9D\xB4\xED\x8A\xB8\xED\x95\x9C \xED\x9B\x84\xEC\x97\x90\xEB\xA7\x8C \xEC\x98\xA4\xEB\x9E\x98 \xEA\xB1\xB8\xEB\xA6\xBD\xEB\x8B\x88\xEB\x8B\xA4."},
};
constexpr float kPageWordsWaitS=10.f;
constexpr size_t kFontSlots=std::extent_v<decltype(fonts)>;
// The final review's L7: the slots' glyph and shadow identities (0x52454B00 + 2 i, + 1) stay below the menu pieces' 0x52454B10; every
// language names two slots that exist; one entry per ordinal.
static_assert(kFontSlots*2<=0x10,"font slot identities would reach the menu pieces'");
static_assert([]{for(const auto& w:kPageWords){if(w.primary>=kFontSlots || w.secondary>=kFontSlots)return false;
    for(const auto& v:kPageWords)if(&v!=&w && v.ordinal==w.ordinal)return false;}return true;}(),"kPageWords: a slot out of range or an ordinal twice");
// The first code of `text` no family of `chain` has (0 when every code is there).
uint32_t FirstMissing(ov::Chain chain,std::u32string_view text) noexcept {
    for(const auto cp:text){bool found{};for(auto* cache:chain)if(cache && cache->Font().Find(cp)){found=true;break;}if(!found)return cp;}
    return 0;}
// A chain for `words` drawn in the run's own families (`own`, the language the game chose): the body's family (MenuFont_01) first, then
// the other, then every other family read; the title's starts with the titles' family (MenuFont_02) only when that family alone has
// every code of the title (the step review's L1: Japanese's and Traditional Chinese's TsukuMinProSub is a 126 KB subset; a title half in
// it would mix two typefaces, or Japanese Han forms in Chinese). English as a fallback goes through the run's own families first too
// (the final review's L3).
using PageChainArray=std::array<ov::GlyphCache*,kFontSlots>;
PageChainArray PageChain(const PageWords& own,const PageWords& words,bool title){
    PageChainArray chain{};size_t n{};
    const auto add=[&](size_t i){if(fonts[i].cache && std::find(chain.begin(),chain.begin()+n,fonts[i].cache.get())==chain.begin()+n)chain[n++]=fonts[i].cache.get();};
    bool titles_first=false;
    if(title && fonts[own.secondary].cache){ov::GlyphCache* const one[]{fonts[own.secondary].cache.get()};titles_first=!FirstMissing(ov::Chain(one),ov::Utf8(words.title));}
    add(titles_first?own.secondary:own.primary);add(titles_first?own.primary:own.secondary);for(size_t i=0;i<kFontSlots;++i)add(i);
    return chain;}
// The page's words and their two chains, made once at the decision (the final review's L2: no typeface change after it, nothing rebuilt
// each frame); `drawn` false when not even English is covered (no words rather than a line with holes).
struct PageText {const PageWords* words{};PageChainArray title{},body{};bool drawn{};};
uint32_t FirstMissing(const PageWords& own,const PageWords& words){
    const auto title=PageChain(own,words,true),body=PageChain(own,words,false);
    if(const uint32_t cp=FirstMissing(ov::Chain(title),ov::Utf8(words.title)))return cp;
    if(const uint32_t cp=FirstMissing(ov::Chain(body),ov::Utf8(words.note)))return cp;
    return FirstMissing(ov::Chain(body),U"0123456789%");}
const PageText* PageLanguage(float shown_s){
    static PageText decided;static bool done{};if(done)return decided.drawn?&decided:nullptr;
    int32_t ordinal{-1};Read(image+0x479BA10,&ordinal,4);
    const PageWords* own=&kPageWords[0];for(const auto& words:kPageWords)if(words.ordinal==ordinal)own=&words;
    const bool read=fonts[own->primary].cache && fonts[own->secondary].cache;
    if(!read && shown_s<kPageWordsWaitS)return nullptr;
    const uint32_t missing=read?FirstMissing(*own,*own):0xFFFFFFFFu;const bool covered=read && !missing;
    const PageWords* words=covered?own:&kPageWords[0];
    const uint32_t english_missing=covered?0:FirstMissing(*own,kPageWords[0]);
    done=true;decided.words=words;decided.drawn=covered || !english_missing;
    decided.title=PageChain(*own,*words,true);decided.body=PageChain(*own,*words,false);
    char row[280]{};sprintf_s(row,"{\"event\":\"native_precompile_language\",\"ordinal\":%d,\"own\":\"%s\",\"families_read\":%s,\"covered\":%s,\"missing\":\"U+%04X\",\"words\":\"%s\",\"drawn\":%s,\"english_missing\":\"U+%04X\",\"after_s\":%.1f}\n",
        ordinal,own->name,read?"true":"false",covered?"true":"false",read?missing:0u,words->name,decided.drawn?"true":"false",english_missing,double(shown_s));Log(row);
    return decided.drawn?&decided:nullptr;}
constexpr float kPageAfterS=3.f,kPageMinS=1.5f,kPageFadeInS=0.25f,kPageFadeOutS=0.5f,kPageStallS=60.f;
// The overlay lost for the session (its failure kept, not the screen layer's alone: scene_pass_frame.cpp RecordOverlay; a device
// reopen clears it). One short string copied under the renderer's lock per page frame.
bool OverlayLost() {const auto failure=renderer.OverlayFailure();return !failure.empty() && failure.rfind("screen ",0)!=0 && failure.rfind("tint ",0)!=0;}
// True while the page is up (its frame given to the renderer), its fade-out included.
bool BuildPrecompileScreen(uint32_t width,uint32_t height) {
    if(precompile_screen_over)return false;
    static uint64_t frames{};static bool shown{},closing{};static LARGE_INTEGER started{},shown_at{},closed_at{},moved{};static uint64_t last_done{~uint64_t{}};
    LARGE_INTEGER now{},frequency{};QueryPerformanceCounter(&now);QueryPerformanceFrequency(&frequency);
    const auto since=[&](const LARGE_INTEGER& t){return float(double(now.QuadPart-t.QuadPart)/double(frequency.QuadPart));};
    if(!frames)started=now;++frames;
    auto p=PrecompileProgress();
    // precompile-screen-hold.flag (a test tool: its look shown to Thomas on a PC whose driver cache makes the precompile quick): the page
    // held 4800 frames (past the title's first seconds: 1800 ended before the title in r12d-screen1), its bar filling over them once the
    // precompile is done.
    static const bool hold=ModuleFlag(L"precompile-screen-hold.flag");
    if(hold && frames<4800){p.finished=false;if(PrecompileProgress().finished){p.total=4800;p.done=frames;}}
    const auto over=[&]{precompile_screen_over=true;input_focus::EndPageBlock();page_fx::Release();if(shown)renderer.SetOverlay(nullptr);char row[224]{};
        sprintf_s(row,"{\"event\":\"native_precompile_screen\",\"done\":true,\"shown\":%s,\"frames\":%llu,\"ms\":%.0f,\"shown_ms\":%.0f,\"pipelines\":%llu}\n",shown?"true":"false",
            (unsigned long long)frames,double(since(started))*1000.0,shown?double(since(shown_at))*1000.0:0.0,(unsigned long long)p.total);Log(row);
        // R12h's L7: the polls whose input the page held back (04 input_focus), when it was up.
        if(shown){char held[256]{};sprintf_s(held,"{\"event\":\"native_precompile_input\",\"blocked\":%s,\"keyboard\":%llu,\"mouse\":%llu,\"menu_mouse\":%llu,\"pad_polls\":%llu,\"pad_hooked\":%s}\n",
            input_focus::page_block_set.load()?"true":"false",(unsigned long long)input_focus::page_keyboard_held.load(),(unsigned long long)input_focus::page_mouse_held.load(),
            (unsigned long long)input_focus::page_menu_mouse_held.load(),(unsigned long long)input_focus::page_pad_skipped.load(),input_focus::pad_hooked?"true":"false");Log(held);}
        return false;};
    if(!shown){
        if(p.finished)return over();
        if(frames<30 || since(started)<kPageAfterS || !width || !height)return false;
        // The step review's L1: no title layer left from before the page (BuildTitleScreen runs only once the page is over, Step); the
        // tint's too (the final review's Low).
        shown=true;shown_at=now;precompile_page_shown=true;renderer.SetScreenLayer(nullptr);renderer.SetTintLayer(nullptr);
        char row[192]{};
        sprintf_s(row,"{\"event\":\"native_precompile_screen\",\"shown\":true,\"page\":true,\"done\":%llu,\"of\":%llu}\n",(unsigned long long)p.done,(unsigned long long)p.total);Log(row);}
    // R12h's L7: no input reaches the game while the page is drawn. The step review's L1: an overlay lost for the session (its open or
    // record failed: nothing drawn) shows no page, so it holds no input either (the page goes on to its end for the title's order);
    // the final review's L1: held again if a device reopen brings the overlay back.
    const bool lost=OverlayLost();
    if(lost)page_fx::StopSound();   // R12h's second tranche, its final review's M1: no sound without a drawn page, whatever the block
    if(input_focus::page_block_enabled){
        if(!lost && !input_focus::PageBlocks())input_focus::StartPageBlock();
        else if(lost && input_focus::PageBlocks()){input_focus::EndPageBlock();Log("{\"event\":\"native_precompile_input\",\"released\":\"overlay_lost\"}\n");}}
    if(p.done!=last_done){last_done=p.done;moved=now;}
    if(!p.finished && !closing && since(moved)>=kPageStallS){closing=true;closed_at=now;char row[192]{};
        sprintf_s(row,"{\"event\":\"native_precompile_screen\",\"stalled\":true,\"done\":%llu,\"of\":%llu,\"still_s\":%.0f}\n",(unsigned long long)p.done,(unsigned long long)p.total,double(since(moved)));Log(row);}
    if(p.finished && !closing && since(shown_at)>=kPageMinS){closing=true;closed_at=now;}
    float opacity=(std::min)(1.f,since(shown_at)/kPageFadeInS);
    if(closing){const float out=since(closed_at)/kPageFadeOutS;if(out>=1.f)return over();opacity*=1.f-out;}
    if(!width || !height)return true;   // the last frame kept until an output exists again (the crackle goes on: the page is still up)
    auto frame=std::make_shared<ov::Frame>();frame->width=width;frame->height=height;
    const float s=float(height)/720.f,cx=float(width)*0.5f,thick=(std::max)(2.f,3.f*s);
    const float fraction=p.finished?1.f:p.total?float(double(p.done)/double(p.total)):0.f;
    // The page: black over the whole output (the start logos and the title under it).
    frame->quads.push_back({0,0,float(width),float(height),0,0,0,0,{0,0,0,opacity},-1});
    // R12h's second tranche: the living background over the black (Thomas's picture, page_fx); its sound from its first frame.
    const bool pictured=page_fx::Draw(*frame,width,height,opacity);
    if(pictured && !lost)page_fx::StartSound();   // the step review's M1: never without a drawn page
    // The bar, 700 units wide (Thomas, 10:06: longer; 10:45: a little shorter), centred under the heading, the block at the bottom (its note 40 units above the edge).
    const float half=(std::min)(350.f*s,0.45f*float(width)),bar_y=596.f*s;   // the final review's L10: inside an output narrower than 7:6
    frame->quads.push_back({cx-half,bar_y,cx+half,bar_y+thick,0,0,0,0,{0.16f,0.15f,0.14f,opacity},-1});
    frame->quads.push_back({cx-half,bar_y,cx-half+2.f*half*(std::clamp)(fraction,0.f,1.f),bar_y+thick,0,0,0,0,{0.78f,0.62f,0.36f,opacity},-1});
    if(const PageText* text=AnyFont()?PageLanguage(since(shown_at)):nullptr){
        const PageWords* words=text->words;const ov::Chain title(text->title),body(text->body);
        const uint32_t blur=uint32_t((std::clamp)(std::lround(s),1L,long(ov::GlyphCache::kPad)));
        ov::TextStyle heading;heading.pixels=std::round(29.f*s);heading.colour={0.92f,0.88f,0.80f,opacity};heading.scale=s;heading.shadow.blur=blur;
        ov::TextStyle number=heading;number.pixels=std::round(19.f*s);number.colour={0.82f,0.80f,0.76f,opacity};
        ov::TextStyle note=number;note.pixels=std::round(15.f*s);note.colour=pictured?std::array<float,4>{0.66f,0.63f,0.58f,opacity}:std::array<float,4>{0.50f,0.48f,0.45f,opacity};   // lighter than R12h's 0.50 over the picture only (Thomas 11:09-11:10: the text and the percentage in contrast)
        const auto centred=[&](ov::Chain chain,const std::u32string& text,float baseline,const ov::TextStyle& style){
            ov::DrawText(*frame,chain,text,std::round(cx-ov::Measure(chain,text,style.pixels)*0.5f),baseline,style);};
        centred(title,ov::Utf8(words->title),576.f*s,heading);
        // The step review's L2: rounded down, « 100 » only once the work is done (the page stays up for the last pipelines).
        const int percent=p.finished?100:(std::min)(99,int(std::floor((std::clamp)(fraction,0.f,1.f)*100.f)));char digits[16]{};
        if(words->spaced_percent)sprintf_s(digits,"%d %%",percent);else sprintf_s(digits,"%d%%",percent);
        centred(body,ov::Utf8(digits),630.f*s,number);
        centred(body,ov::Utf8(words->note),680.f*s,note);}
    renderer.SetOverlay(std::move(frame));return true;}
// I-0132 R12e (D-0182, Thomas's mock-up .project/design/title-screen-mockup.png): the « Rekindled » title on DS3's title screen: under
// the game's logo (its movie 05_001_Title_Logo, laid out in DS3's 1280x720 units, centred, scaled by the output height: its letters at
// y 269 to 384), « REKINDLED » letter-spaced in ember colour in the titles' font (TsukuOldMinPro alone: never a stand-in body font, the
// step review's L4), between two rules fading outward. Once per visit to the title (the logo's movie advancing, kTitleGraceMs of grace;
// 15_cpu_service_and_input.inc NoteRekindledTitle) it fades in, then a glow crosses it from outside the word to outside the word, again every 10 s.
// The title stays under its pages and its message boxes (Thomas 2026-11-12 02:46: it replaces the old title): a page (Load Game, New
// Game's, the Options) fades the word out and dims the rest, a message box blurs the word (BuildTitleScreen says each layer's share);
// a loading screen, a film (New Game's opening) or character creation hides it all; the intro is not replayed (the step review's M2). A row when it shows and hides. The halo is the glyphs' blurred atlas (its pad, 3 output
// pixels at any size: narrower in 720-line units as the output grows, the step review's L5, kept). The embers and the smoke (a screen
// layer over the UI, below) are its second tranche.
// I-0132 R12e: the title's embers and smoke, in the screen layer (after DS3's UI, a screen blend: they light its black and leave its
// white logo as it is; an underlay before it showed nothing in r12e-film3, the title movie read as opaque). Their second look (Thomas,
// 2026-11-11 23:24 and 00:08: more embers, brighter and bigger, crossing the white title if they must; the smoke orange at its edges, as
// D-0182's mock-up, the first grey smoke « laide »): the mock-up's warm smoke banks along the bottom and the sides, lit orange from below,
// and its ember field, glowing flakes and a few large blurred ones near the bottom; the ember field as dense as Thomas's animated mock-up
// (2026-11-12 00:07, its sparks over the whole height; its grey smoke not kept: « laide », the orange-edged one preferred). Six images made
// once, at the title's first frame: a soft dot (the embers' halo, the large ones, the glows), a wispy cloud (a radial falloff carved by
// value noise into wisps) and its rim (the cloud's edge band: the orange of its edges), the burnt ring, the ash flakes and the hot core;
// the tint layer adds the logo's burn.
// Laid out in DS3's 1280x720 units around the centre, scaled by the output height, as wide as the output; simulated on the engine
// thread from the frame's time step (at most 0.1 s), deterministic (an LCG). `strength` is the title's fade.
namespace title_fx {
uint32_t seed=0x2D3E5A17u;
float Random() noexcept {seed=seed*1664525u+1013904223u;return float(seed>>8)/16777216.f;}
ov::Image Dot(){
    static const ov::Image dot=[]{constexpr uint32_t n=32;auto bytes=std::make_shared<std::vector<uint8_t>>(n*n);
        for(uint32_t y=0;y<n;++y)for(uint32_t x=0;x<n;++x){const float dx=(float(x)+0.5f)/n*2.f-1.f,dy=(float(y)+0.5f)/n*2.f-1.f;
            const float r=std::sqrt(dx*dx+dy*dy);(*bytes)[y*n+x]=uint8_t(std::lround(255.f*std::clamp(std::exp(-r*r*4.f)-0.018f,0.f,1.f)));}
        return ov::Image{0x52454B20ull,1,n,n,ov::Format::Coverage8,std::move(bytes)};}();
    return dot;}
float Lattice(int x,int y,uint32_t salt) noexcept {uint32_t h=uint32_t(x)*0x8DA6B343u^uint32_t(y)*0xD8163841u^salt*0xCB1AB31Fu;h^=h>>13;h*=0x5BD1E995u;h^=h>>15;return float(h&0xFFFFu)/65535.f;}
float Noise(float x,float y,uint32_t salt) noexcept {const int ix=int(std::floor(x)),iy=int(std::floor(y));const float fx=x-float(ix),fy=y-float(iy);
    const float sx=fx*fx*(3.f-2.f*fx),sy=fy*fy*(3.f-2.f*fy);
    const float a=Lattice(ix,iy,salt),b=Lattice(ix+1,iy,salt),c=Lattice(ix,iy+1,salt),d=Lattice(ix+1,iy+1,salt);
    return a+(b-a)*sx+(c-a)*sy+(a-b-c+d)*sx*sy;}
// The cloud's density at (u, v) in 0..1: a soft disc carved by three octaves of noise, warped once (wisps rather than blobs).
// Thomas (2026-11-21 23:07): the smoke with fewer streaks: the 16-cycle octave (8 texels a cycle, about 40 output pixels once a bank is
// scaled up) and the rim's gradient of it drew its streaks; now 2, 4 and 8 cycles (I-0140 tranche 34).
float Density(float u,float v) noexcept {
    const float wu=u+0.18f*(Noise(u*3.f,v*3.f,7)-0.5f),wv=v+0.18f*(Noise(u*3.f+5.f,v*3.f,8)-0.5f);
    const float dx=wu*2.f-1.f,dy=wv*2.f-1.f,r=std::sqrt(dx*dx+dy*dy),falloff=std::clamp(1.f-r,0.f,1.f);
    const float n=0.22f*Noise(wu*2.f,wv*2.f,4)+0.43f*Noise(wu*4.f,wv*4.f,1)+0.35f*Noise(wu*8.f,wv*8.f,2);
    return std::clamp((falloff*1.6f)*(n*1.9f-0.55f),0.f,1.f);}
// The cloud (its density) and its rim, 128 texels a side. The rim is the band at the cloud's outer edge (where the density, blurred over
// 9 texels, is about 0.14), uneven along it (a 5-cycle noise): the orange of its edges. It was the density's gradient, which also lit
// every wisp's border inside the cloud (Thomas's streaks, I-0140 tranche 34); the two images' mean coverage about as before (0.19 and
// 0.17 against 0.23 and 0.17, computed offline on this code's copy).
ov::Image CloudImage(bool rim){
    constexpr uint32_t n=128;
    static const std::shared_ptr<const std::vector<float>> field=[]{auto d=std::make_shared<std::vector<float>>(n*n);
        for(uint32_t y=0;y<n;++y)for(uint32_t x=0;x<n;++x)(*d)[y*n+x]=Density((float(x)+0.5f)/n,(float(y)+0.5f)/n);return d;}();
    const auto make=[&](bool edge){auto bytes=std::make_shared<std::vector<uint8_t>>(n*n);
        std::vector<float> blurred;
        if(edge){   // a separable 9-texel box blur, clamped at the borders
            const auto pass=[&](const std::vector<float>& from,bool across){std::vector<float> to(n*n);
                for(uint32_t y=0;y<n;++y)for(uint32_t x=0;x<n;++x){float sum=0.f;
                    for(int k=-4;k<=4;++k){const int xx=std::clamp(int(x)+(across?k:0),0,int(n)-1),yy=std::clamp(int(y)+(across?0:k),0,int(n)-1);sum+=from[size_t(yy)*n+size_t(xx)];}
                    to[y*n+x]=sum/9.f;}
                return to;};
            blurred=pass(pass(*field,true),false);}
        for(uint32_t y=0;y<n;++y)for(uint32_t x=0;x<n;++x){const float d=(*field)[y*n+x];float value=d;
            if(edge){const float band=(blurred[y*n+x]-0.14f)/0.24f,u=(float(x)+0.5f)/n,v=(float(y)+0.5f)/n;
                value=std::exp(-band*band)*(0.35f+0.65f*Noise(u*5.f,v*5.f,9))*std::clamp(d*5.f,0.f,1.f);}
            // Both images faded over the 16 texels at their border (a smoothstep; the final review's M1: the density still 0.11 at the
            // border and the band up to 0.49 there, cut by the quad's straight edges): the border texels under 0.002 (offline).
            const float border=std::clamp(((std::min)((std::min)(float(x),float(n-1-x)),(std::min)(float(y),float(n-1-y)))+0.5f)/16.f,0.f,1.f);
            value*=border*border*(3.f-2.f*border);
            (*bytes)[y*n+x]=uint8_t(std::lround(255.f*std::clamp(value,0.f,1.f)));}
        return ov::Image{edge?0x52454B22ull:0x52454B21ull,3,n,n,ov::Format::Coverage8,std::move(bytes)};};
    static const ov::Image cloud=make(false),edge=make(true);
    return rim?edge:cloud;}
// Thomas's mock-up's drawing behind the title (2026-11-12 02:14: « un petit dessin dans le fond »): a burnt ring, a sword through
// it. The ring's image made once, 384 texels a side over the ring's box, burnt as the mock-up's (the coordinator, Thomas asleep,
// 05:58: r12e-film8's ring too clean and even): a stroke of uneven width, charred gaps, ember spots along it, short cracks running
// out of it, a few sparks beside it, a faint glow; its lower part fading out behind the logo. Noise sampled on the circle (no seam).
ov::Image SignImage(){
    static const ov::Image sign=[]{constexpr uint32_t n=384;auto bytes=std::make_shared<std::vector<uint8_t>>(n*n);
        constexpr float kRadius=0.86f,kTau=6.2831853f;constexpr int kCracks=72;
        for(uint32_t y=0;y<n;++y)for(uint32_t x=0;x<n;++x){const float dx=(float(x)+0.5f)/n*2.f-1.f,dy=(float(y)+0.5f)/n*2.f-1.f;
            const float r=std::sqrt(dx*dx+dy*dy),angle=std::atan2(dy,dx),d=r-kRadius;
            const float ca=std::cos(angle),sa=std::sin(angle);
            const float fade=std::clamp((0.25f-dy)*2.f,0.f,1.f);
            if(fade<=0.f || std::fabs(d)>0.14f){(*bytes)[y*n+x]=0;continue;}
            // Charred gaps (the stroke gone over a third of the ring), its width uneven, ember spots along it.
            const float keep=std::clamp((Noise(ca*4.f+20.f,sa*4.f,21)-0.36f)*5.f,0.f,1.f);
            const float width=0.004f+0.011f*Noise(ca*7.f+40.f,sa*7.f,24);
            const float spots=std::pow(Noise(ca*26.f+60.f,sa*26.f+r*20.f,22),2.f)*1.7f+0.15f;
            const float stroke=std::exp(-std::pow(d/width,2.f))*spots*keep;
            // Cracks: at some of seventy-two angles, a thin line running out of the stroke (outward or inward), fading with distance.
            const float slot=angle/kTau*float(kCracks),nearest=std::round(slot),tangent=std::fabs(slot-nearest)*kTau*r/float(kCracks);
            // The final review's L6: the slots wrapped (one slot at the ring's left point).
            const int index=((int(nearest)%kCracks)+kCracks)%kCracks;const float pick=Lattice(index,3,25),reach=0.02f+0.07f*Lattice(index,5,26),side=Lattice(index,7,27)<0.5f?1.f:-1.f;
            const float run=d*side;
            const float crack=pick>0.55f && run>0.f && run<reach?std::exp(-std::pow(tangent/0.0035f,2.f))*(1.f-run/reach)*0.9f*keep:0.f;
            // A few sparks beside the stroke, and the glow.
            const float spark=Lattice(int(x),int(y),28)>0.9985f && std::fabs(d)<0.06f?0.8f:0.f;
            const float glow=std::exp(-std::pow(d/0.045f,2.f))*0.22f*(0.4f+0.6f*keep);
            (*bytes)[y*n+x]=uint8_t(std::lround(255.f*std::clamp((stroke+crack+spark+glow)*fade,0.f,1.f)));}
        return ov::Image{0x52454B23ull,2,n,n,ov::Format::Coverage8,std::move(bytes)};}();
    return sign;}
// An ember (Thomas, 2026-11-12 02:13: the motion « pas très naturel », a steady rise with a regular sine sway): its velocity follows
// a turbulent field (a gust shared by all, eddies drifting through the field) with inertia, its buoyancy falls as it cools, and its
// heat (its brightness, its size, its colour from yellow to red) runs out over its life.
// An ember's look (Thomas, 2026-11-12 02:36: the motion good, the particles to look more like embers): a flake of glowing ash, not a
// soft dot: four irregular flakes (sharp edged, uneven: a stretched ellipse carved by noise) in one 128x32 image, and a sharp hot dot
// for its core (white-yellow), over its orange flake and a faint red halo.
ov::Image FlakeImage(){
    static const ov::Image flakes=[]{constexpr uint32_t w=128,h=32;auto bytes=std::make_shared<std::vector<uint8_t>>(w*h);
        constexpr float ax[4]{1.f,0.62f,0.9f,1.f},ay[4]{0.55f,1.f,0.8f,0.4f};
        for(uint32_t y=0;y<h;++y)for(uint32_t x=0;x<w;++x){const uint32_t cell=x/32;const float u=(float(x%32)+0.5f)/32.f*2.f-1.f,v=(float(y)+0.5f)/32.f*2.f-1.f;
            const float e=(u/ax[cell])*(u/ax[cell])+(v/ay[cell])*(v/ay[cell]),n=Noise(u*3.f+float(cell)*9.f,v*3.f,31);
            const float shape=std::exp(-e*2.2f)*(0.55f+0.8f*n);(*bytes)[y*w+x]=uint8_t(std::lround(255.f*std::clamp((shape-0.22f)*2.6f,0.f,1.f)));}
        return ov::Image{0x52454B24ull,1,w,h,ov::Format::Coverage8,std::move(bytes)};}();
    return flakes;}
ov::Image HotImage(){
    static const ov::Image hot=[]{constexpr uint32_t n=16;auto bytes=std::make_shared<std::vector<uint8_t>>(n*n);
        for(uint32_t y=0;y<n;++y)for(uint32_t x=0;x<n;++x){const float dx=(float(x)+0.5f)/n*2.f-1.f,dy=(float(y)+0.5f)/n*2.f-1.f;
            (*bytes)[y*n+x]=uint8_t(std::lround(255.f*std::clamp(std::exp(-(dx*dx+dy*dy)*9.f)*1.15f-0.05f,0.f,1.f)));}
        return ov::Image{0x52454B25ull,1,n,n,ov::Format::Coverage8,std::move(bytes)};}();
    return hot;}
// The rules beside the word (Thomas, 2026-11-21 23:07: smooth, not pixelated; I-0140 tranche 34): one quad a side sampling this image,
// its u from the word outward, its v across. Along: a fade-in over its first 2 %, 0 at its first texel (no hard inner end), then a
// smooth fall to 0 at the outer end, (1 - u)^0.85, its mean 0.53 (it was five plain quads at 1, 0.8, 0.6, 0.4 and 0.2: steps, their mean 0.6). Across: a
// Gaussian 1.5 units wide at half height, its integral 1.56 units (the old bar 1.6 units tall), in a quad 8 units tall (no hard edge).
ov::Image RuleImage(){
    static const ov::Image rule=[]{constexpr uint32_t w=256,h=32;auto bytes=std::make_shared<std::vector<uint8_t>>(w*h);
        for(uint32_t y=0;y<h;++y)for(uint32_t x=0;x<w;++x){const float u=(float(x)+0.5f)/w,v=((float(y)+0.5f)/h-0.5f)/0.11f;
            const float along=std::clamp(float(x)/(0.02f*w),0.f,1.f)*std::pow(1.f-u,0.85f);   // the review of 49aa2aa00, L2: texel 0 at 0
            (*bytes)[y*w+x]=uint8_t(std::lround(255.f*std::clamp(along*std::exp(-v*v),0.f,1.f)));}
        return ov::Image{0x52454B27ull,1,w,h,ov::Format::Coverage8,std::move(bytes)};}();
    return rule;}
// The logo's burn (Thomas, 2026-11-12 08:01: « un effet cramoisi, brûlé, à Dark Souls III et Rekindled »): charred patches and specks,
// 256x64 texels over the logo's letters, faded at the image's border (no straight edge), multiplied crimson in the tint layer.
ov::Image BurnImage(){
    static const ov::Image burn=[]{constexpr uint32_t w=256,h=64;auto bytes=std::make_shared<std::vector<uint8_t>>(w*h);
        for(uint32_t y=0;y<h;++y)for(uint32_t x=0;x<w;++x){const float u=(float(x)+0.5f)/w,v=(float(y)+0.5f)/h;
            const float border=std::clamp((0.5f-std::fabs(u-0.5f))*12.f,0.f,1.f)*std::clamp((0.5f-std::fabs(v-0.5f))*6.f,0.f,1.f);
            // Fine stains (2 to 6 units) and thin cracks (the Fable comparison, 08:30: the patches read as pink blush).
            const float patches=std::clamp((0.45f*Noise(u*18.f,v*4.5f,41)+0.35f*Noise(u*48.f,v*12.f,42)+0.20f*Noise(u*110.f,v*28.f,47)-0.40f)*3.f,0.f,1.f);
            const float crack=std::clamp(1.f-std::fabs(Noise(u*30.f,v*7.5f,48)-0.5f)*28.f,0.f,1.f)*0.8f;
            const float specks=Lattice(int(x),int(y),43)>0.985f?0.7f:0.f;
            (*bytes)[y*w+x]=uint8_t(std::lround(255.f*std::clamp((patches+crack+specks)*border,0.f,1.f)));}
        return ov::Image{0x52454B26ull,1,w,h,ov::Format::Coverage8,std::move(bytes)};}();
    return burn;}
struct Ember {float x{},y{},vx{},vy{},lift{},heat{},life{},age{},size{},flicker{},warm{};bool big{},front{};uint8_t flake{};};   // front: drawn over the title (Thomas, 02:44)
struct Smoke {float x{},y{},size{},drift{},rise{},alpha{},turn{},phase{};};
// 320 embers (Thomas, 08:58: 120 to 170 seen at a time; r12e-film13 showed about 60 of 200).
std::array<Ember,320> embers;std::array<Smoke,10> clouds;bool seeded{};
// The field's half width in 720-line units, the output's (at least 16:9's 640: the second step review's M2, wider outputs filled).
float field_half=640.f;
// An ember from below the screen (`anywhere`: at a random height, the first frame's); 9 % large and blurred, lower and slower.
void Spawn(Ember& e,bool anywhere) noexcept {
    e.big=Random()<0.09f;const float kind=Random();   // big ones 9 %; then 70 % small, 20 % medium (the Fable comparison)
    // Upwind (the wind blows to the right, Thomas 02:18): from below the screen (its right 200 units left out), or one in three from its left edge.
    if(!anywhere && Random()<0.33f){e.x=-field_half-30.f;e.y=200.f+560.f*Random();}
    else{e.x=anywhere?(field_half-40.f)*(2.f*Random()-1.f):-field_half+(2.f*field_half-200.f)*Random();e.y=anywhere?80.f+680.f*Random():740.f+80.f*Random();}
    e.lift=e.big?50.f+50.f*Random():60.f+70.f*Random();e.vx=(Random()-0.5f)*20.f;e.vy=-e.lift*(0.6f+0.4f*Random());
    // Their life 8 to 16 s (r12e-film7: they died low at 4 to 10 s, the top empty); the Fable review's H1: this line's statements once
    // sat after a comment.
    e.life=e.big?9.f+6.f*Random():8.f+8.f*Random();e.age=anywhere?e.life*0.8f*Random():0.f;e.heat=1.f-e.age/e.life;
    e.size=e.big?9.f+8.f*Random():kind<0.78f?1.4f+1.2f*Random():2.6f+2.2f*Random();
    e.warm=Random();e.flicker=97.f*Random();e.flake=uint8_t(Random()*3.999f);e.front=!e.big && Random()<0.12f;
    if(e.big){e.x=(Random()<0.5f?-1.f:1.f)*(field_half-160.f*Random());if(!anywhere)e.y=740.f+80.f*Random();}}   // the big ones at the edges
// The smoke banks: along the bottom (the deepest), up the left and right sides, a few higher at the edges; none at the centre's top.
// Each bank placed wholly below the output (the final review's M1: never a pop at the bottom edge); Seed spreads the first ones along their way.
void PlaceCloud(Smoke& c,size_t i) noexcept {
    const size_t bank=i%4;   // 0, 1 the bottom; 2 the left side; 3 the right side
    c.size=320.f+280.f*Random();c.y=720.f+c.size*0.5f+120.f*Random();
    if(bank<2){const float side=bank==0?-1.f:1.f;c.x=side*(field_half*0.45f+field_half*0.5f*Random());}
    else{const float side=bank==2?-1.f:1.f;c.x=side*(field_half-60.f-160.f*Random());}
    c.drift=(Random()-0.5f)*8.f;c.rise=6.f+9.f*Random();c.alpha=0.045f+0.035f*Random();c.turn=Random()<0.5f?0.f:1.f;c.phase=6.2832f*Random();}   // the alpha: Thomas, 08:55, too much smoke at 0.11 to 0.17
void Seed() noexcept {
    for(auto& e:embers)Spawn(e,true);
    // The banks spread along their path at the start (Thomas, 2026-11-12 11:30: two dense blocks on each side at first, the drifting
    // and thinning later liked): each placed below the output, then moved up a random part of its way to its fade-out (y 380).
    for(size_t i=0;i<clouds.size();++i){auto& c=clouds[i];PlaceCloud(c,i);c.y-=Random()*(c.y-380.f);}
    seeded=true;}
// `over`: the title's overlay frame (alpha blended, after the word): about one small or medium ember in eight (12 %, never a large one)
// is drawn there, in front of the title (Thomas,
// 2026-11-12 02:44: « les particules peuvent se placer devant le titre »; a screen blend leaves nothing on the white letters).
// `box`: a message box's eased presence (0 to 1): the glow around the titles and the embers over the box's area go out behind it
// (Thomas, 2026-11-12 09:43: the box « plus très bien lisible à cause du orange en fond »).
std::shared_ptr<ov::Frame> Build(uint32_t width,uint32_t height,float dt,float strength,float t,ov::Frame* over,float box){
    const float s=float(height)/720.f,cx=float(width)*0.5f;field_half=(std::max)(640.f,cx/s);
    if(!seeded)Seed();
    dt=std::clamp(dt,0.f,0.1f);
    auto frame=std::make_shared<ov::Frame>();frame->width=width;frame->height=height;
    frame->images.push_back(CloudImage(false));frame->images.push_back(Dot());frame->images.push_back(CloudImage(true));
    frame->images.push_back(SignImage());frame->images.push_back(FlakeImage());frame->images.push_back(HotImage());   // 3, 4, 5
    // The sign (the mock-up's, in 720-line units: the ring's centre 287 units down, its radius 161; the sword from 18 units down to the
    // ring's centre), behind the title (the screen blend leaves the white logo as it is), breathing slowly.
    {const float breath=0.8f+0.2f*Noise(t*0.5f,3.5f,23),a=0.85f*strength*breath*(1.f-0.6f*box),ring=161.f/0.86f,cy=287.f;   // fainter behind a box (the step review's M2)
        frame->quads.push_back({cx-ring*s,(cy-ring)*s,cx+ring*s,(cy+ring)*s,0,0,384,384,{1.f,0.42f,0.10f,a},3});
        frame->quads.push_back({cx-9.f*s,18.f*s,cx+9.f*s,cy*s,0,0,32,32,{1.f,0.38f,0.08f,a*0.35f},1});   // the sword's glow
        frame->quads.push_back({cx-1.6f*s,18.f*s,cx+1.6f*s,cy*s,0,0,32,32,{1.f,0.55f,0.18f,a},1});       // its blade, a burning line
        frame->quads.push_back({cx-22.f*s,(cy-ring*0.86f-4.f)*s,cx+22.f*s,(cy-ring*0.86f+4.f)*s,0,0,32,32,{1.f,0.5f,0.15f,a*0.8f},1});}   // its guard on the ring
    // No glow behind the titles (Thomas, 2026-11-12 11:32: « Enlève la tâche orangée derrière le titre aussi, la coloration des lettres
    // suffit »; it had been the 08:01 crimson glow, then the Fable comparison's brown-orange one): only a faint warmth behind the ring.
    frame->quads.push_back({cx-170.f*s,(230.f-110.f)*s,cx+170.f*s,(230.f+110.f)*s,0,0,32,32,{0.45f,0.22f,0.08f,0.22f*strength*(1.f-0.9f*box)},1});
    // A warm glow along the bottom (the fire below the mock-up's smoke): two wide soft dots at the corners and one at the centre.
    for(int k=0;k<3;++k){const float gx=k==1?0.f:(k==0?-1.f:1.f)*(field_half-120.f),gw=k==1?520.f:420.f,gy=760.f,gh=k==1?240.f:320.f,ga=k==1?0.04f:0.09f;
        frame->quads.push_back({cx+(gx-gw)*s,(gy-gh)*s,cx+(gx+gw)*s,(gy+gh)*s,0,0,32,32,{1.f,0.36f,0.08f,ga*strength},1});}
    const float gust_smoke=0.3f*70.f*(0.25f+1.1f*Noise(t*0.22f,0.5f,11));   // the smoke with a third of the wind
    // The smoke: a dark warm body, then its rim in orange (the edges lit from below), fading out as it rises past the logo's height.
    for(size_t i=0;i<clouds.size();++i){auto& c=clouds[i];c.phase+=dt*0.15f;c.x+=(c.drift+gust_smoke+std::sin(c.phase)*4.f)*dt;c.y-=c.rise*dt;
        // The Fable review's M3: recycled once faded out at the top (rise 0), or blown wholly off the right (back off the left).
        if(c.y<380.f)PlaceCloud(c,i);   // faded out at y 380 (never in front of the titles)
        else if(c.x-c.size*0.5f>field_half)c.x=-field_half-c.size*0.5f;
        const float rise=std::clamp((c.y-380.f)/260.f,0.f,1.f),a=c.alpha*strength*rise*std::clamp(t/4.f,0.f,1.f);if(a<=0.002f)continue;   // the smoke in over 4 s
        const float x0=cx+(c.x-c.size*0.5f)*s,y0=(c.y-c.size*0.5f)*s,x1=cx+(c.x+c.size*0.5f)*s,y1=(c.y+c.size*0.5f)*s;
        const float u0=c.turn*128.f,u1=128.f-u0;   // every other cloud mirrored (fewer repeats of one image)
        const float lit=std::clamp((c.y-260.f)/420.f,0.f,1.f);   // the lower, the more lit from below
        frame->quads.push_back({x0,y0,x1,y1,u0,0,u1,128,{0.50f,0.30f+0.04f*lit,0.17f,a},0});   // brown
        frame->quads.push_back({x0,y0,x1,y1,u0,0,u1,128,{1.f,0.50f+0.08f*lit,0.20f,a*(0.35f+0.45f*lit)},2});}
    // The embers: a red halo, a short trail, an ash flake and a hot core each; the large ones a soft blur.
    // The air: a wind to the right shared by every ember (its gusts slow), and eddies (two noise fields drifting up through the field); each
    // ember's velocity eases toward the air's plus its own buoyancy (inertia: no ember turns at once), the buoyancy its heat's.
    // The wind (Thomas, 2026-11-12 02:18: « comme un vent qui souffle sur les braises »): steady to the right, its gusts doubling it or
    // dropping it to a breath, every few seconds.
    const float gust=70.f*(0.25f+1.1f*Noise(t*0.22f,0.5f,11))+22.f*(Noise(t*0.9f,2.5f,15)-0.5f);
    const int32_t over_dot=over?ov::ImageIndex(*over,Dot()):-1,over_flake=over?ov::ImageIndex(*over,FlakeImage()):-1,over_hot=over?ov::ImageIndex(*over,HotImage()):-1;
    for(auto& e:embers){
        e.age+=dt;e.heat=1.f-e.age/e.life;
        if(e.heat<=0.f || e.y<20.f || std::fabs(e.x)>field_half+60.f){Spawn(e,false);continue;}
        const float ex=e.x*0.0055f,ey=e.y*0.0055f;
        const float swirl_x=90.f*(Noise(ex+t*0.07f,ey+t*0.28f,12)-0.5f),swirl_y=50.f*(Noise(ex+7.3f,ey+t*0.28f,13)-0.5f);
        const float want_x=gust+swirl_x,want_y=-e.lift*(0.7f+0.3f*e.heat)+swirl_y;
        const float ease_x=(std::min)(1.f,dt*(e.big?0.8f:1.8f)),ease_y=(std::min)(1.f,dt*(e.big?0.6f:1.2f));
        e.vx+=(want_x-e.vx)*ease_x;e.vy+=(want_y-e.vy)*ease_y;e.x+=e.vx*dt;e.y+=e.vy*dt;
        // Seen: born over 0.4 s, gone as it cools, dimmer below the screen's bottom edge and above the logo's top; a flicker as an
        // ember's own noise (not a sine).
        const float born=std::clamp(e.age/0.4f,0.f,1.f),edges=std::clamp((780.f-e.y)/100.f,0.f,1.f)*std::clamp((e.y-20.f)/(e.big?300.f:160.f),0.f,1.f);
        const float flicker=0.6f+0.4f*Noise(t*7.f+e.flicker,e.flicker,14),glow=std::pow((std::max)(e.heat,0.f),0.45f);
        // Few at full brightness (warm^3), fewer over the logo's block (x within 510, y 240 to 460: the Fable comparison).
        const float middle=std::clamp((510.f-std::fabs(e.x))/60.f,0.f,1.f)*std::clamp((e.y-240.f)/40.f,0.f,1.f)*std::clamp((460.f-e.y)/40.f,0.f,1.f);
        const float boxed_area=box*std::clamp((360.f-std::fabs(e.x))/40.f,0.f,1.f)*std::clamp((e.y-240.f)/30.f,0.f,1.f)*std::clamp((460.f-e.y)/30.f,0.f,1.f);
        const float a=strength*born*edges*flicker*glow*(0.8f+0.2f*e.warm*e.warm)*(1.f-0.35f*middle)*(1.f-0.95f*boxed_area);if(a<=0.004f)continue;
        const std::array<float,3> colour{1.f,0.24f+0.40f*glow*(0.5f+0.5f*e.warm),0.04f+0.16f*glow*e.warm};   // cooler: redder
        const float px=cx+e.x*s,py=e.y*s,shrink=0.55f+0.45f*glow;
        ov::Frame& to=e.front && over && over_dot>=0 && over_flake>=0 && over_hot>=0?*over:*frame;   // a full image list: behind
        const int32_t dot_at=&to==frame.get()?1:over_dot,flake_at=&to==frame.get()?4:over_flake,hot_at=&to==frame.get()?5:over_hot;
        if(e.big){const float r=e.size*s*shrink;frame->quads.push_back({px-r,py-r,px+r,py+r,0,0,32,32,{colour[0],colour[1]*0.85f,colour[2],a*0.55f},1});continue;}
        // A red halo, its trail (three fainter flakes behind it along its motion), the orange flake, then the hot core (white-yellow, the
        // hotter the bigger, flaring with the flicker).
        const float core=e.size*s*shrink,halo=core*2.6f,u0=32.f*float(e.flake);
        to.quads.push_back({px-halo,py-halo,px+halo,py+halo,0,0,32,32,{1.f,0.22f+0.12f*glow,0.04f,a*0.30f},dot_at});
        for(int k=1;k<=3;++k){const float back=0.02f*float(k),bx=px-e.vx*back*s,by=py-e.vy*back*s,r=core*(1.f-0.22f*float(k));   // its trail, along the wind
            to.quads.push_back({bx-r,by-r,bx+r,by+r,u0,0,u0+32.f,32,{1.f,colour[1]*0.7f,colour[2]*0.5f,a*(0.55f-0.15f*float(k))},flake_at});}
        to.quads.push_back({px-core,py-core,px+core,py+core,u0,0,u0+32.f,32,{colour[0],colour[1],colour[2],a},flake_at});
        const float hot=core*(0.35f+0.25f*glow)*(0.85f+0.3f*flicker);
        to.quads.push_back({px-hot,py-hot,px+hot,py+hot,0,0,16,16,{1.f,0.88f+0.1f*glow,0.55f+0.35f*glow,a*glow},hot_at});}
    return frame;}
}
uint64_t TickAge(uint64_t tick,uint64_t now) noexcept {return now>tick?now-tick:0;}
// The logo's grace (the final review's L1: a stall at the title does not end the visit) and a cover's hold (its M1: the Load Game list's
// confirm to the loading screen was 0.69 s in r12e-film4, the word came back between them; the last step review's M1: 1.5 s, room for
// a slower load; a return from the list shows the word again 1.5 s later), one for the step and the title (the last step review's L2).
constexpr uint64_t kTitleGraceMs=500,kCoverHoldMs=1500;   // a tick stored after `now` is fresh (the step review's L1)
bool BuildTitleScreen(uint32_t width,uint32_t height) {
    // `intro`: the intro's clock starts at the visit's first uncovered frame (a title that opens under a cover keeps its fade-in and its
    // glow for when the cover closes; the offline notices came 14.7 s after the title's first frames in r12e-film2).
    static LARGE_INTEGER since{},last{};static float presence{},quiet{},boxq{},dimq{},coverq{};static bool at_title{},shown{},intro{};   // quiet: 1 under a page or a box
    const uint64_t logo=rekindled_title_logo_tick.load(std::memory_order_relaxed),cover=rekindled_title_cover_tick.load(std::memory_order_relaxed),now=GetTickCount64();
    // The final review's L1: 500 ms of grace for the logo (a stall at the title does not end the visit); M1: a cover holds the word off
    // until kCoverHoldMs after its last advance (the Load Game list's confirm to the loading screen: 0.69 s in r12e-film4, the word came back).
    const uint64_t dim=rekindled_title_dim_tick.load(std::memory_order_relaxed);
    const bool title=logo && TickAge(logo,now)<kTitleGraceMs,covered=cover && TickAge(cover,now)<kCoverHoldMs,dimmed=dim && TickAge(dim,now)<kCoverHoldMs;   // the Fable review's M1: no rise in the confirm-to-loading gap
    const uint64_t notice=rekindled_title_notice_tick.load(std::memory_order_relaxed);const bool boxed=notice && TickAge(notice,now)<400;
    if(!title && !shown && !at_title)return false;   // the final review's L4: in play, no QPC read (one tick count, two loads)
    LARGE_INTEGER clock{},frequency{};QueryPerformanceCounter(&clock);QueryPerformanceFrequency(&frequency);   // the step review's L3: a smooth clock
    if(!title)at_title=false;
    else if(!at_title){at_title=true;intro=false;last=clock;presence=0.f;quiet=boxq=dimq=coverq=0.f;}   // the last step review's L3: no state carried over
    if(at_title && !covered && !intro && fonts[0].cache){intro=true;since=clock;presence=1.f;}   // the final review's L2: once the font is read
    const float dt=float(double(clock.QuadPart-last.QuadPart)/double(frequency.QuadPart));last=clock;
    // Under a title page the title is dimmed to 0.3 (Thomas, 02:46): the screen layer (the ring, embers and smoke) stays at 0.3 over the
    // page, while the word and the tint fade out with the page's own ease (`dimq`, I-0140 tranche 32); the 0.3 also shapes the word's
    // return after a page (from 0.09, `fade`). A loading screen hides it.
    if(at_title){const float target=covered?0.f:dimmed?0.3f:1.f;presence=presence>target?(std::max)(target,presence-dt/0.15f):(std::min)(target,presence+dt/0.3f);}
    // A loading screen's cover: the title's layers kept until the tint's own ease is over (the final review's Low: it cleared at about 35 %
    // when a loading screen followed a page, `presence` reaching 0 first).
    if(!at_title || !intro || (presence<=0.f && coverq>=1.f) || !fonts[0].cache || !width || !height){   // no word before its intro
        if(shown){shown=false;renderer.SetOverlay(nullptr);renderer.SetScreenLayer(nullptr);renderer.SetTintLayer(nullptr);char row[96]{};sprintf_s(row,"{\"event\":\"native_rekindled_title\",\"shown\":false,\"covered\":%s}\n",at_title?"true":"false");Log(row);}
        return false;}
    if(!shown){shown=true;Log("{\"event\":\"native_rekindled_title\",\"shown\":true}\n");}
    const float t=float(double(clock.QuadPart-since.QuadPart)/double(frequency.QuadPart));
    auto frame=std::make_shared<ov::Frame>();frame->width=width;frame->height=height;
    const float s=float(height)/720.f,cx=float(width)*0.5f,baseline=446.f*s;
    ov::GlyphCache* const chain_one[]{fonts[0].cache.get()};const ov::Chain title_chain(chain_one);
    ov::TextStyle letter;letter.pixels=std::round(52.f*s);letter.scale=s;letter.shadow.on=true;letter.shadow.distance=0;
    letter.shadow.blur=uint32_t(ov::GlyphCache::kPad);letter.shadow.alpha=1.f;
    const char32_t word[]=U"REKINDLED";constexpr size_t letters=9;
    const float spacing=22.f*s;float widths[letters]{},total=0;
    for(size_t i=0;i<letters;++i){widths[i]=ov::Measure(title_chain,std::u32string_view(&word[i],1),letter.pixels);total+=widths[i];}
    total+=spacing*float(letters-1);
    // The fade-in (1.2 s, after 0.6 s: the logo's own fade) and the glow's pass (from 2.4 s, 2.2 s across, from 150 units before the
    // word to 150 units after it: no letter lit at its start or end, the step review's L2; its band about one letter).
    // Our layers are drawn over DS3's UI (the Fable review's M2: the box's text stays readable). Under a page the word fades out (DS3's
    // own logo stays, dimmed by the page; the final review's M2), the embers, smoke and ring stay at 0.3 (`presence`; I-0140 tranche 34),
    // the tint and its burn fade out with it (tranche 32, below). Under a message box (Thomas, 2026-11-12 09:43, 11:30 and 12:25) the word blurred rather than darkened (its sharp glyphs
    // fade out, its blurred copy, the shadow atlas, fades in, back to sharp when the box closes), the rules at 35 %, the ring at 0.4, the
    // embers and the warmth out over the box's area, the front embers behind, the burn off. All eased over 0.2 s (`quiet`, `boxq`,
    // `dimq`), the loading screens' `coverq` over 0.15 s.
    quiet=std::clamp(quiet+((dimmed || boxed)?1.f:-1.f)*dt/0.2f,0.f,1.f);boxq=std::clamp(boxq+(boxed?1.f:-1.f)*dt/0.2f,0.f,1.f);dimq=std::clamp(dimq+(dimmed?1.f:-1.f)*dt/0.2f,0.f,1.f);coverq=std::clamp(coverq+(covered?1.f:-1.f)*dt/0.15f,0.f,1.f);   // boxq: the last step review's M1
    const float fade=std::clamp((t-0.6f)/1.2f,0.f,1.f)*(dimmed?1.f:presence*presence)*(1.f-dimq);
    // The glow again every 10 s after its first pass (Thomas, 2026-11-12 02:45).
    const float sweep=(t<2.4f?t-2.4f:std::fmod(t-2.4f,10.f))/2.2f,band_x=cx-total*0.5f-150.f*s+sweep*(total+300.f*s),band=sweep>=0.f && sweep<=1.f?1.f:0.f;
    const std::array<float,4> ember{0.93f,0.50f,0.22f,1.f},bright{1.f,0.86f,0.62f,1.f};
    // The rules: from 28 units outside the word, 150 units long, fading outward (title_fx::RuleImage: smooth along and across), centred
    // where the old 1.6-unit bars were (17 units over the baseline, less 0.8).
    const int32_t rule_image=ov::ImageIndex(*frame,title_fx::RuleImage());
    if(rule_image>=0){const float rule_c=baseline-16.2f*s,inner=total*0.5f+28.f*s,outer=inner+150.f*s;
        for(int side=-1;side<=1;side+=2){const float x0=side<0?cx-outer:cx+inner,x1=side<0?cx-inner:cx+outer,u0=side<0?256.f:0.f;
            frame->quads.push_back({x0,rule_c-4.f*s,x1,rule_c+4.f*s,u0,0,256.f-u0,32,{ember[0],ember[1],ember[2],fade*0.85f*(1.f-0.65f*boxq)},rule_image});}}
    float pen=cx-total*0.5f;
    for(size_t i=0;i<letters;++i){const float centre=pen+widths[i]*0.5f;
        const float g=band*std::exp(-std::pow((centre-band_x)/(60.f*s),2.f));
        ov::TextStyle style=letter;
        for(int c=0;c<3;++c)style.colour[c]=ember[c]+(bright[c]-ember[c])*g;
        style.colour[3]=fade*(1.f-boxq);
        const size_t before=frame->quads.size();
        ov::DrawText(*frame,title_chain,std::u32string_view(&word[i],1),pen,baseline,style);
        // The shadow's quad as a glow: crimson (Thomas, 2026-11-12 08:01: « cramoisi, brûlé »), brighter in the band.
        if(frame->quads.size()==before+2){auto& halo=frame->quads[before];halo.colour={0.72f+0.2f*boxq,0.10f+0.3f*g+0.3f*boxq,0.04f+0.1f*boxq,fade*((0.6f+0.35f*g)*(1.f-boxq)+0.40f*boxq)};
            const float grow=3.f*s*boxq;halo.x0-=grow;halo.y0-=grow;halo.x1+=grow;halo.y1+=grow;}
        pen+=widths[i]+spacing;}
    // The second tranche: the embers and the smoke (the screen layer, over the UI), fading in over 2 s from the word's start (no-title-embers.flag the A/B).
    static const bool no_embers=ModuleFlag(L"no-title-embers.flag");
    // A screen blend over everything DS3's UI drew, so under a page it also colours the page (I-0140 tranche 32's final review: the Load
    // Game list's first two rows' panels R x1.03, G x1.015 against the original, the ring's arc across them). Thomas's choice (2026-11-21
    // 23:07, « option 3 »): kept over the page at 0.3 (`presence`), as before tranche 33 took it off; his ideal (« option 2 ») is the
    // layer drawn under the page, which needs the movies' draw order read first. The layer dropped only while nothing of it shows.
    if(!no_embers){const float strength=std::clamp(t/2.f,0.f,1.f)*presence;
        auto embers=title_fx::Build(width,height,dt,strength,t,quiet>0.f?nullptr:frame.get(),boxq);
        renderer.SetScreenLayer(strength>0.f?std::move(embers):nullptr);}
    // The logo tinted (Thomas, 2026-11-12 02:14, « légèrement colorer Dark Souls III en orangé, comme ma maquette »): a multiply over its
    // letters, a warm cream at the edges, a deeper orange at the centre (the dot stretched over it), with the title's fade.
    // The layer is a multiply over everything DS3's UI drew, so under a page it also coloured the page itself: I-0140 tranche 32 measured
    // the Load Game list's panels and portraits in a band about 400 to 610 px at 1920x1080 (the lower half of its second visible row,
    // its third row whole; the third row's panel at G x0.92 and B x0.86) against the original, the same place in every list. Under a
    // page it now fades out as the word does (`dimq`), leaving the page and the logo the page dims as DS3 draws them; full under a
    // message box (Thomas, 2026-11-12 08:01, kept); with a cover's fade (the last step review's M2). Thomas's earlier « half under a
    // page » (08:01) is what this changes: his answer of 2026-11-21 23:07 (« option 3 ») keeps the tint off under a page.
    const float k=std::clamp(t/2.f,0.f,1.f)*(1.f-dimq)*(1.f-coverq);   // the step review's M1: a cover's own ease (no dip)
    if(k<=0.f)renderer.SetTintLayer(nullptr);   // nothing to multiply: no layer drawn
    else{auto tint=std::make_shared<ov::Frame>();tint->width=width;tint->height=height;tint->images.push_back(title_fx::Dot());tint->images.push_back(title_fx::BurnImage());
        tint->quads.push_back({cx-560.f*s,262.f*s,cx+560.f*s,392.f*s,0,0,0,0,{1.f,0.86f,0.70f,0.55f*k},-1});
        tint->quads.push_back({cx-640.f*s,200.f*s,cx+640.f*s,454.f*s,0,0,32,32,{1.f,0.62f,0.38f,0.45f*k},0});
        tint->quads.push_back({cx-580.f*s,258.f*s,cx+580.f*s,396.f*s,0,0,256,64,{0.42f,0.26f,0.16f,0.70f*k*(1.f-boxq)},1});   // the burn, brown-black; off under a box (its text) and a page (k)
        renderer.SetTintLayer(std::move(tint));}
    renderer.SetOverlay(std::move(frame));return true;}
// The engine thread's step, after the UI's frame is prepared. A failure turns the overlay off for the session with one row (R12a's
// final review L1: an optional overlay never ends the native frame).
bool step_failed{};
// I-0138 tranche 6 (a test tool: tone-look-keys.flag, 04 NativeToneLookRequested): a line of text at the top right for 2.5 s after
// a key changed the tone map's look or knee, in the titles' font once the title read it (none before: the journal's row alone).
// Only while no other screen of the overlay shows; true while it draws.
bool BuildTestLabel(uint32_t width,uint32_t height) {
    static const bool keys=ModuleFlag(L"tone-look-keys.flag");if(!keys)return false;   // the final review's L1: no lock taken in play without the flag
    static bool shown{};std::string text;uint64_t tick{};
    {const std::lock_guard hold(native_test_label_lock);if(native_test_label_tick){text=native_test_label;tick=native_test_label_tick;}}
    const bool wanted=tick && TickAge(tick,GetTickCount64())<2500 && fonts[0].cache && width && height;
    if(!wanted){if(shown){shown=false;renderer.SetOverlay(nullptr);}return false;}
    auto frame=std::make_shared<ov::Frame>();frame->width=width;frame->height=height;const float s=float(height)/720.f;
    ov::TextStyle style;style.pixels=std::round(22.f*s);style.scale=s;style.colour={1.f,0.95f,0.8f,1.f};style.shadow.on=true;style.shadow.distance=0;
    style.shadow.blur=uint32_t(ov::GlyphCache::kPad);style.shadow.alpha=1.f;
    const float right=float(width)-20.f*s,left=right-700.f*s;   // at the top right, clear of the HUD's bars
    frame->quads.push_back({left,16.f*s,right,58.f*s,0,0,0,0,{0.f,0.f,0.f,0.6f},-1});
    ov::DrawText(*frame,*fonts[0].cache,ov::Utf8(text.c_str()),left+12.f*s,46.f*s,style);
    shown=true;renderer.SetOverlay(std::move(frame));return true;}
void Step(uint32_t width,uint32_t height) noexcept {
    ReleaseTitleBgm();   // R12h's third tranche (its final review's M2): on the engine thread, where the game makes its BGM requests
    title_crackle::Resume();   // R12e: the crackle held under the page
    if(!Enabled() || step_failed)return;
    // I-0132 R12e: once the precompile screen is over (and no test), the title screen only: the fonts read only while the title's logo
    // shows (a font's parse is 90 to 120 ms on the engine thread: at the title, never in play).
    const uint64_t logo=rekindled_title_logo_tick.load(std::memory_order_relaxed);
    // R12h (the final review's L2): the body family read only while the page is up, not during a warm start's pending first seconds.
    const bool title_only=!TestRequested() && (precompile_screen_over || !precompile_page_shown),at_title=logo && TickAge(logo,GetTickCount64())<kTitleGraceMs;
    try{
        {static bool said{};if(!said){said=true;char row[96]{};sprintf_s(row,"{\"event\":\"native_overlay_engine_thread\",\"thread\":%lu}\n",GetCurrentThreadId());Log(row);}}
        if(!observer_refused.load(std::memory_order_relaxed) && (!title_only || at_title))ReadFonts(title_only);
        if(TestRequested())FindPieces();   // the final review's L1: the menu pieces only for the overlay test
        // I-0132 R12d: the precompile screen first; then the overlay test, or R12e's title screen.
        // The step review's L1: R12e's title only once the page is over (never before it, even while the output is 0x0 or the first
        // 3 s run: a warm start's work is over within about 1 s); the overlay test as before.
        if(!BuildPrecompileScreen(width,height)){if(TestRequested())BuildTest(width,height);else if(precompile_screen_over && !BuildTitleScreen(width,height))BuildTestLabel(width,height);}
        // The counters' rows while a screen can show (the step review's L6: none in play).
        // The tint layer's counts too (the final review's Low).
        static uint64_t frames{};if((!title_only || at_title) && (++frames%600)==0){const auto c=renderer.TakeOverlayCounts(),screen=renderer.TakeScreenLayerCounts(),tint=renderer.TakeTintLayerCounts();char row[800]{};
            sprintf_s(row,"{\"event\":\"native_overlay\",\"frames\":%llu,\"quads\":%llu,\"uploads\":%llu,\"upload_bytes\":%llu,\"refused\":%llu,\"skipped_size\":%llu,\"stale_blocks\":%llu,\"screen_quads\":%llu,\"screen_frames\":%llu,\"screen_refused\":%llu,\"screen_skipped_size\":%llu,\"tint_quads\":%llu,\"tint_frames\":%llu,\"tint_refused\":%llu,\"tint_skipped_size\":%llu,\"missing_glyphs\":%u}\n",
                (unsigned long long)c.frames,(unsigned long long)c.quads,(unsigned long long)c.uploads,(unsigned long long)c.upload_bytes,(unsigned long long)c.refused,
                (unsigned long long)c.skipped_size,(unsigned long long)c.stale_blocks,(unsigned long long)screen.quads,(unsigned long long)screen.frames,(unsigned long long)screen.refused,(unsigned long long)screen.skipped_size,
                (unsigned long long)tint.quads,(unsigned long long)tint.frames,(unsigned long long)tint.refused,(unsigned long long)tint.skipped_size,Missing());Log(row);
            uint32_t refused{};for(auto& slot:fonts)if(slot.cache)refused+=slot.cache->Refused();   // the step review's L3: a full atlas said
            static std::string failure_said;const auto failure=renderer.OverlayFailure();
            if(refused || (!failure.empty() && failure!=failure_said)){std::string text=failure.substr(0,160);for(auto& ch:text)if(ch=='"' || ch=='\\' || static_cast<unsigned char>(ch)<32 || static_cast<unsigned char>(ch)>126)ch='?';
                char more[320]{};sprintf_s(more,"{\"event\":\"native_overlay_state\",\"refused_glyphs\":%u,\"failure\":\"%s\"}\n",refused,text.c_str());Log(more);failure_said=failure;}}
    }catch(const std::exception& e){step_failed=true;precompile_screen_over=true;input_focus::EndPageBlock();try{page_fx::Release();}catch(...){}   /* the second step review's L3: freed too */ renderer.SetOverlay(nullptr);renderer.SetScreenLayer(nullptr);renderer.SetTintLayer(nullptr);char text[121]{};   // no allocation here (Step is noexcept); R12h's L7: no page, no block
        const char* what=e.what();for(size_t k=0;what && what[k] && k<120;++k){const char ch=what[k];text[k]=(ch=='"' || ch=='\\' || static_cast<unsigned char>(ch)<32 || static_cast<unsigned char>(ch)>126)?'?':ch;}
        char row[200]{};sprintf_s(row,"{\"event\":\"native_overlay_state\",\"step_failed\":\"%s\"}\n",text);Log(row);}
    catch(...){step_failed=true;precompile_screen_over=true;input_focus::EndPageBlock();try{page_fx::Release();}catch(...){}   /* the second step review's L3: freed too */ renderer.SetOverlay(nullptr);renderer.SetScreenLayer(nullptr);renderer.SetTintLayer(nullptr);Log("{\"event\":\"native_overlay_state\",\"step_failed\":\"unknown\"}\n");}}
}
