// I-0138 tranche 5b: the options film's added slots (options_film.h). T1: PlaceSlot writes a slot's tag as the shipped film writes
// its own (three of its tags, 21 to 24 bytes each, as read from the game's film by the pilot). T2: on a synthetic movie (a header, a
// tag before, the sprite with two slots, a tag after), AddSlots keeps every byte outside the insertion, grows the two lengths by the
// tags, and the sprite holds its slots and the new ones; the refusals (not a movie, a wrong length, another character, no ShowFrame).
// T3: the three slots added for the Graphics tab are 74 bytes. T4 (tranche 5d): RetypeSlot changes the one slot's character and
// nothing else, and refuses a slot the sprite lacks or holds twice. The shipped film itself is not in the repository: the module
// checks its SHA-256 before and after (kFilmSha256, kPatchedSha256).
#include "options_film.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace of=ds3native::options_film;
namespace {
void Require(bool ok,const char* what){if(!ok){std::printf("FAIL options film: %s\n",what);std::exit(1);}}
std::vector<uint8_t> Hex(const char* text){std::vector<uint8_t> out;for(size_t i=0;text[i] && text[i+1];i+=2)out.push_back(uint8_t(std::stoul(std::string(text+i,2),nullptr,16)));return out;}
void Append(std::vector<uint8_t>& to,const std::vector<uint8_t>& from){to.insert(to.end(),from.begin(),from.end());}
}
int main(){
    // T1: the film's Item_4_0 (kind 131, y 0), Item_5_0 (kind 92, y 50 px) and Item_2_0 (kind 131, y -100 px).
    Require(of::PlaceSlot({9,131,0,"Item_4_0"})==Hex("bf060f0000002609008300004974656d5f345f3000"),"T1 Item_4_0");
    Require(of::PlaceSlot({11,92,50,"Item_5_0"})==Hex("bf0612000000260b005c0016001f404974656d5f355f3000"),"T1 Item_5_0");
    Require(of::PlaceSlot({5,131,-100,"Item_2_0"})==Hex("bf06120000002605008300180010604974656d5f325f3000"),"T1 Item_2_0");
    // T2: a synthetic movie.
    std::vector<uint8_t> film{'G','F','X',0x0B,0,0,0,0};
    Append(film,Hex("4202aabb"));   // a short tag before (code 9, two bytes)
    const size_t sprite=film.size();
    std::vector<uint8_t> body{0x8C,0x00,0x01,0x00};   // character 140, one frame
    Append(body,of::PlaceSlot({1,139,-200,"Item_0_0"}));Append(body,of::PlaceSlot({3,131,-150,"Item_1_0"}));
    Append(body,Hex("40000000"));   // ShowFrame, End
    {uint8_t head[6];of::PutU16(head,uint16_t((of::kTagDefineSprite<<6)|of::kLongTag));of::PutU32(head+2,uint32_t(body.size()));film.insert(film.end(),head,head+6);}
    Append(film,body);
    const size_t after=film.size();Append(film,Hex("ff092a000000"));for(int i=0;i<42;++i)film.push_back(uint8_t(i));Append(film,Hex("0000"));
    of::PutU32(film.data()+4,uint32_t(film.size()));
    Require(of::CountSlots(film,sprite)==2,"T2 the synthetic sprite's two slots");
    const auto out=of::AddSlots(film,sprite,140,of::kAddedSlots);
    Require(out.has_value(),"T2 the slots added");
    size_t added=0;for(const auto& slot:of::kAddedSlots)added+=of::PlaceSlot(slot).size();
    Require(out->size()==film.size()+added && of::U32(out->data()+4)==out->size(),"T2 the file's length");
    Require(of::U32(out->data()+sprite+2)==body.size()+added,"T2 the sprite's length");
    const size_t show=after-4;
    Require(!std::memcmp(out->data()+8,film.data()+8,sprite+2-8) && !std::memcmp(out->data()+sprite+6,film.data()+sprite+6,show-sprite-6),"T2 the bytes before the insertion");
    Require(!std::memcmp(out->data()+show+added,film.data()+show,film.size()-show),"T2 the bytes after the insertion");
    Require(of::CountSlots(*out,sprite)==5,"T2 five slots after");
    {size_t at=show;for(const auto& slot:of::kAddedSlots){const auto tag=of::PlaceSlot(slot);Require(!std::memcmp(out->data()+at,tag.data(),tag.size()),"T2 an added tag in place");at+=tag.size();}}
    // The refusals.
    {auto bad=film;bad[0]='C';Require(!of::AddSlots(bad,sprite,140,of::kAddedSlots),"T2 not a plain movie");}
    {auto bad=film;bad.push_back(0);Require(!of::AddSlots(bad,sprite,140,of::kAddedSlots),"T2 a length that is not the file's");}
    Require(!of::AddSlots(film,sprite,141,of::kAddedSlots),"T2 another character");
    Require(!of::AddSlots(film,sprite+1,140,of::kAddedSlots) && !of::AddSlots(film,after,140,of::kAddedSlots) && !of::AddSlots(film,film.size(),140,of::kAddedSlots),"T2 not a sprite there");
    {auto bad=film;bad[show]=0x41;Require(!of::AddSlots(bad,sprite,140,of::kAddedSlots),"T2 no ShowFrame at the sprite's end");}
    {auto bad=film;bad[sprite+8]=2;Require(!of::AddSlots(bad,sprite,140,of::kAddedSlots),"T2 two frames");}
    // T3: the Graphics tab's three.
    Require(added==74 && of::kFilmSlots+uint32_t(std::size(of::kAddedSlots))==of::kSlotsWanted,"T3 three slots, 74 bytes");
    // T4: the synthetic sprite's second slot retyped.
    {auto retyped=film;const of::Slot second{3,131,-150,"Item_1_0"};
        Require(of::RetypeSlot(retyped,sprite,second,92),"T4 the slot retyped");
        size_t differing=0,first=0;for(size_t i=0;i<film.size();++i)if(film[i]!=retyped[i]){if(!differing)first=i;++differing;}
        const auto tag=of::PlaceSlot({3,92,-150,"Item_1_0"});
        Require(differing==1 && of::U16(retyped.data()+first)==92 && of::CountSlots(retyped,sprite)==2,"T4 one byte differs, the character's; the slots still two");
        Require(std::search(retyped.begin(),retyped.end(),tag.begin(),tag.end())!=retyped.end(),"T4 the slot's tag as PlaceSlot writes it with the new character");
        auto again=retyped;Require(!of::RetypeSlot(again,sprite,second,92) && again==retyped,"T4 a slot the sprite no longer holds: refused, untouched");
        Require(!of::RetypeSlot(retyped,sprite,{5,131,-100,"Item_2_0"},92) && !of::RetypeSlot(retyped,sprite+1,second,92),"T4 an absent slot, not a sprite");
        // The same slot twice in the sprite: refused.
        std::vector<uint8_t> twice{'G','F','X',0x0B,0,0,0,0};const size_t at=twice.size();
        std::vector<uint8_t> inner{0x8C,0x00,0x01,0x00};Append(inner,of::PlaceSlot(second));Append(inner,of::PlaceSlot(second));Append(inner,Hex("40000000"));
        {uint8_t head[6];of::PutU16(head,uint16_t((of::kTagDefineSprite<<6)|of::kLongTag));of::PutU32(head+2,uint32_t(inner.size()));twice.insert(twice.end(),head,head+6);}
        Append(twice,inner);of::PutU32(twice.data()+4,uint32_t(twice.size()));
        const auto kept=twice;Require(!of::RetypeSlot(twice,at,second,92) && twice==kept,"T4 a slot held twice: refused, untouched");
        Require(of::kRetypedSlot.character==of::kSlotGeneric && of::PlaceSlot(of::kRetypedSlot)==Hex("bf0612000000260b005c0016001f404974656d5f355f3000"),"T4 the Graphics tab's sixth slot as the film writes it");}
    std::puts("PASS options film: T1 a slot's tag as the shipped film writes its own (three tags); T2 a synthetic movie keeps its bytes, grows its two lengths and holds five slots, six malformed inputs refused; T3 the Graphics tab's three slots are 74 bytes; T4 a slot retyped in place, four refusals");
    return 0;
}
