#pragma once
// I-0138 tranche 5b (Thomas, 2026-11-14 12:05: « Je préfère que les réglages du HDR soient dans le menu graphics et pas advanced »):
// more slots for the Graphics tab. The options film menu/win/02_040_optionsetting.gfx (a plain GFX movie, SWF's tag stream) gives the
// tab's sprite « PCGraphic » (character 140) six slots, Item_0_0 to Item_5_0, each a PlaceObject2 of a slot kind 50 px under the one
// before (R12C-STATIC §5); the game's list counts the slots it finds by name (Item_<row>_<column>, FUN_140A40500: up to 64 rows) and a
// row takes the slot of its index (FUN_140A90E20), so a row past the sixth has no clip. Three slots are added to the film's bytes as
// the game hands them to its Scaleform repository (game_bootstrap/16b, the insertion's observer): Item_6_0 of the generic kind 92 (a
// slider), Item_7_0 of the kind 131 (the Advanced Settings button, as the film's Item_4_0) and Item_8_0 of the kind 92 (the tab's own
// Defaults row, as the film's Item_5_0), at y 100, 150 and 200 px, depths 13, 15, 17 (the film's own run on: 1 to 11, -200 to 50 px).
// I-0138 tranche 5d (Thomas, 2026-11-14 20:00: the HDR output's two rows hidden while it is off): the tab then has seven rows and
// its Advanced Settings button sits in the sixth slot, Item_5_0, which the film makes of the generic kind (its button the centred
// one of a Defaults row, 15 px lower and without the sub-page's mark): that slot is given the kind 131 (RetypeSlot), so the button is
// drawn as in its own slot whichever place it takes (a slider has the same widget in both kinds), the Defaults row after it in
// Item_6_0 (generic). With the output on the rows fill the nine slots as before.
// Nothing is written to disk and no file is copied beside the module. A film that is not the shipped one, byte for byte, is left as
// it is: the tab then keeps its five rows and the HDR output has no row (it follows the settings file alone). Header-only, unit-tested (options_film_test) on a synthetic movie; the shipped
// film's own result is pinned by its SHA-256 (the pilot's offline patch of the film read from the game's data, parsed again).
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <vector>

namespace ds3native::options_film {
// The shipped film and the patched one (SHA-256 of the whole file, upper case as ds3content::Sha256 writes it).
inline constexpr size_t kFilmBytes=32917;
inline constexpr char kFilmSha256[]="B632B0AC7BEC6F169AB7621A20817A59CF6458DEB31FEF7F22861523182F8B06";
inline constexpr char kPatchedSha256[]="EB14204A42EE13EE750C02808C501C723DD24213681FF350DC966C224773AB40";
// Where the shipped film holds the sprite « PCGraphic »: its DefineSprite tag's header (a long one: the code and 0x3F, then the length).
inline constexpr size_t kGraphicsSprite=24086;
inline constexpr uint16_t kGraphicsCharacter=140,kSlotGeneric=92,kSlotChoice=131;
inline constexpr uint32_t kFilmSlots=6,kSlotsWanted=9;
struct Slot {uint16_t depth;uint16_t character;int32_t y_px;const char* name;};
inline constexpr Slot kAddedSlots[]{{13,kSlotGeneric,100,"Item_6_0"},{15,kSlotChoice,150,"Item_7_0"},{17,kSlotGeneric,200,"Item_8_0"}};
inline constexpr Slot kRetypedSlot{11,kSlotGeneric,50,"Item_5_0"};   // the film's own sixth slot, given kSlotChoice (tranche 5d)

inline uint16_t U16(const uint8_t* p) noexcept {return uint16_t(p[0]|(p[1]<<8));}
inline uint32_t U32(const uint8_t* p) noexcept {return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
inline void PutU16(uint8_t* p,uint16_t v) noexcept {p[0]=uint8_t(v);p[1]=uint8_t(v>>8);}
inline void PutU32(uint8_t* p,uint32_t v) noexcept {p[0]=uint8_t(v);p[1]=uint8_t(v>>8);p[2]=uint8_t(v>>16);p[3]=uint8_t(v>>24);}
inline constexpr uint16_t kTagPlaceObject2=26,kTagDefineSprite=39,kLongTag=0x3F;

// A PlaceObject2 tag as the film writes its slots: a long header, the flags 0x26 (a name, a matrix, a character), the depth, the
// character, a matrix that only translates (no scale, no rotation; x 0, y in twips, 20 a pixel; the fewest signed bits that hold y,
// as the film's own tags are written), the name and its zero.
inline std::vector<uint8_t> PlaceSlot(const Slot& slot) {
    const int32_t y=slot.y_px*20;
    uint32_t bits=0;if(y){bits=1;while(!(-(int64_t(1)<<(bits-1))<=y && y<(int64_t(1)<<(bits-1))))++bits;}
    // The matrix's bit stream, most significant bit first: 0 (no scale), 0 (no rotation), 5 bits of `bits`, x, y, zero padding.
    uint8_t matrix[12]{};uint32_t at=0;
    const auto put=[&](uint32_t value,uint32_t count){for(uint32_t i=count;i-->0;++at)if((value>>i)&1u)matrix[at>>3]|=uint8_t(0x80u>>(at&7));};
    put(0,2);put(bits,5);put(0,bits);put(uint32_t(y)&((bits>=32?0u:(1u<<bits))-1u),bits);
    const size_t matrix_bytes=(at+7)>>3,name_bytes=std::strlen(slot.name)+1,body=1+2+2+matrix_bytes+name_bytes;
    std::vector<uint8_t> tag(6+body);
    PutU16(tag.data(),uint16_t((kTagPlaceObject2<<6)|kLongTag));PutU32(tag.data()+2,uint32_t(body));
    uint8_t* p=tag.data()+6;*p++=0x26;PutU16(p,slot.depth);p+=2;PutU16(p,slot.character);p+=2;
    std::memcpy(p,matrix,matrix_bytes);p+=matrix_bytes;std::memcpy(p,slot.name,name_bytes);
    return tag;
}
// The film with the slots added at the end of the sprite whose DefineSprite header is at `sprite` (before its ShowFrame): the
// sprite's length and the file's grow by the tags. Nothing unless the bytes are a plain GFX movie of its stated length whose tag at
// `sprite` is a long DefineSprite of the character `character` with one frame, ending in ShowFrame and End.
inline std::optional<std::vector<uint8_t>> AddSlots(std::span<const uint8_t> film,size_t sprite,uint16_t character,std::span<const Slot> slots) {
    if(film.size()<8 || film[0]!='G' || film[1]!='F' || film[2]!='X' || U32(film.data()+4)!=film.size())return std::nullopt;
    if(sprite<8 || sprite+10>film.size() || U16(film.data()+sprite)!=uint16_t((kTagDefineSprite<<6)|kLongTag))return std::nullopt;
    const uint32_t length=U32(film.data()+sprite+2);const size_t body=sprite+6;
    if(length<8 || body+length>film.size() || U16(film.data()+body)!=character || U16(film.data()+body+2)!=1)return std::nullopt;
    const size_t show=body+length-4;   // ShowFrame (tag 1, no body: 40 00) then End (00 00)
    if(U16(film.data()+show)!=0x0040 || U16(film.data()+show+2)!=0)return std::nullopt;
    std::vector<uint8_t> added;for(const Slot& slot:slots){const auto tag=PlaceSlot(slot);added.insert(added.end(),tag.begin(),tag.end());}
    std::vector<uint8_t> out;out.reserve(film.size()+added.size());
    out.insert(out.end(),film.begin(),film.begin()+show);out.insert(out.end(),added.begin(),added.end());out.insert(out.end(),film.begin()+show,film.end());
    PutU32(out.data()+4,uint32_t(out.size()));PutU32(out.data()+sprite+2,uint32_t(length+added.size()));
    return out;
}
// The slot `slot` of the sprite whose DefineSprite header is at `sprite` (one tag of the sprite, byte for byte as PlaceSlot writes
// it) given the character `to`, in place. False, the film untouched, unless exactly one tag of the sprite is that slot's.
inline bool RetypeSlot(std::span<uint8_t> film,size_t sprite,const Slot& slot,uint16_t to) {
    if(sprite+10>film.size() || U16(film.data()+sprite)!=uint16_t((kTagDefineSprite<<6)|kLongTag))return false;
    const size_t end=sprite+6+U32(film.data()+sprite+2);if(end>film.size())return false;
    const auto tag=PlaceSlot(slot);size_t found=0;uint32_t count=0;
    for(size_t at=sprite+10;at+2<=end;){
        const uint16_t head=U16(film.data()+at);size_t length=head&0x3F,body=at+2;
        if(length==kLongTag){if(at+6>end)return false;length=U32(film.data()+at+2);body=at+6;}
        if(body+length>end)return false;
        if(body+length-at==tag.size() && !std::memcmp(film.data()+at,tag.data(),tag.size())){found=at;++count;}
        if((head>>6)==0)break;
        at=body+length;}
    if(count!=1)return false;
    PutU16(film.data()+found+9,to);return true;   // the long header (6), the flags (1), the depth (2), then the character
}
// The named slots a sprite holds (its PlaceObject2 tags with a name « Item_<n>_0 »), for the test.
inline uint32_t CountSlots(std::span<const uint8_t> film,size_t sprite) noexcept {
    if(sprite+10>film.size() || U16(film.data()+sprite)!=uint16_t((kTagDefineSprite<<6)|kLongTag))return 0;
    const size_t end=sprite+6+U32(film.data()+sprite+2);if(end>film.size())return 0;
    uint32_t count=0;
    for(size_t at=sprite+10;at+2<=end;){
        const uint16_t head=U16(film.data()+at);size_t length=head&0x3F,body=at+2;
        if(length==kLongTag){if(at+6>end)return 0;length=U32(film.data()+at+2);body=at+6;}
        if(body+length>end)return 0;
        if((head>>6)==kTagPlaceObject2 && length>=6 && film[body]==0x26){   // a name, a matrix, a character: the slots' own form
            // The matrix: a bit for a scale (then 5 bits of size and two values), the same for a rotation, 5 bits and two values of
            // translation; the name starts at the next whole byte.
            size_t bit=(body+5)*8;const size_t last=(body+length)*8;bool ok=true;
            const auto take=[&](uint32_t n){uint32_t v=0;for(uint32_t i=0;i<n;++i,++bit){if(bit>=last){ok=false;return 0u;}v=(v<<1)|((film[bit>>3]>>(7-(bit&7)))&1u);}return v;};
            for(int part=0;part<2;++part)if(take(1)){const uint32_t n=take(5);bit+=2*size_t(n);}
            {const uint32_t n=take(5);bit+=2*size_t(n);}
            const size_t name=(bit+7)>>3;
            if(ok && name+5<=body+length && !std::memcmp(film.data()+name,"Item_",5) && film[body+length-1]==0)++count;}
        if((head>>6)==0)break;
        at=body+length;}
    return count;
}
}
