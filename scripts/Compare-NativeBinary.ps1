param(
    [Parameter(Mandatory)][string]$Before,
    [Parameter(Mandatory)][string]$After,
    # Code cleanup (D-0191): -Objects compares two COFF objects section by section, independent of their order (each
    # section keyed by name and COMDAT symbol, hashed with its relocations' targets), so that moved definitions can be
    # proved unchanged; without it, two PE images are compared section by section with the debug directory masked.
    [switch]$Objects,
    # With -Objects: print the data and relocations of the sections whose key contains this text (to explain a difference).
    [string]$Detail
)
# Proves that a source change left the machine code unchanged: the debug information (line numbers, PDB identity)
# is excluded; everything the processor executes or reads is compared byte for byte.
$ErrorActionPreference='Stop'
if(-not ('NativeBinaryCompare' -as [type])){Add-Type -TypeDefinition @'
using System;using System.IO;using System.Text;using System.Linq;using System.Collections.Generic;using System.Security.Cryptography;
public static class NativeBinaryCompare{
  public static string Detail;public static List<string> Details=new List<string>();
  static uint U32(byte[] b,int o){return BitConverter.ToUInt32(b,o);}
  static ushort U16(byte[] b,int o){return BitConverter.ToUInt16(b,o);}
  // PE: the raw bytes of every section, with the header time stamp, the checksum and the debug directory's data masked.
  public static List<string> Pe(string path){
    var b=File.ReadAllBytes(path);int nt=(int)U32(b,0x3c);int count=U16(b,nt+6);int optSize=U16(b,nt+20);int opt=nt+24;
    Array.Clear(b,nt+8,4);Array.Clear(b,opt+64,4);
    uint debugRva=U32(b,opt+112+6*8),debugSize=U32(b,opt+112+6*8+4);int table=opt+optSize;
    Func<uint,int> FileOf=rva=>{for(int s=0;s<count;s++){int h=table+s*40;uint va=U32(b,h+12),vs=U32(b,h+8),raw=U32(b,h+20);if(rva>=va&&rva<va+Math.Max(vs,U32(b,h+16)))return (int)(raw+rva-va);}return -1;};
    if(debugRva!=0){int d=FileOf(debugRva);for(int e=0;e<debugSize/28;e++){int entry=d+e*28;Array.Clear(b,entry+4,4);uint size=U32(b,entry+16);uint raw=U32(b,entry+24);Array.Clear(b,(int)raw,(int)size);}}
    var result=new List<string>();
    for(int s=0;s<count;s++){int h=table+s*40;string name=Encoding.ASCII.GetString(b,h,8).TrimEnd('\0');int raw=(int)U32(b,h+20),size=(int)U32(b,h+16);
      using(var sha=SHA256.Create())result.Add(name+" "+size+" "+Convert.ToHexString(sha.ComputeHash(b,raw,size)));}
    return result;}
  // COFF object (normal or /bigobj), independent of the order in which the compiler emitted its definitions: one line per
  // COMDAT section (keyed by its COMDAT symbol) and, inside every other non-debug section, one line per defined symbol (the
  // bytes up to the next symbol). A relocation names its target symbol (a section symbol: the section's key), with the offset
  // inside the target's chunk; compiler-numbered guard names (?$S<n>@) are normalised, their numbers following the source's
  // file structure.
  static string Norm(string n){return System.Text.RegularExpressions.Regex.Replace(n,@"\?\$S\d+@","?$S#@");}
  public static List<string> Obj(string path){
    var b=File.ReadAllBytes(path);bool big=U16(b,0)==0&&U16(b,2)==0xFFFF;
    int sections,headerSize,symSize;uint symPtr,symCount;
    if(big){sections=(int)U32(b,44);symPtr=U32(b,48);symCount=U32(b,52);headerSize=56;symSize=20;}
    else{sections=U16(b,2);symPtr=U32(b,8);symCount=U32(b,12);headerSize=20+U16(b,16);symSize=18;}
    int strings=(int)(symPtr+symCount*symSize);
    Func<int,string> Str=o=>{int e=o;while(b[e]!=0)e++;return Encoding.UTF8.GetString(b,o,e-o);};
    Func<int,string> SymName=o=>U32(b,o)==0?Str(strings+(int)U32(b,o+4)):Encoding.UTF8.GetString(b,o,8).TrimEnd('\0');
    var secName=new string[sections+1];var secKey=new string[sections+1];var secComdat=new bool[sections+1];
    for(int s=1;s<=sections;s++){int h=headerSize+(s-1)*40;string n=Encoding.ASCII.GetString(b,h,8).TrimEnd('\0');if(n.StartsWith("/"))n=Str(strings+int.Parse(n.Substring(1)));secName[s]=n;secComdat[s]=(U32(b,h+36)&0x1000)!=0;}
    var symbols=new List<Tuple<string,int,int,uint>>();   // name, section, storage class, value
    var comdat=new Dictionary<int,string>();
    var defined=new Dictionary<int,List<Tuple<uint,string>>>();   // non-COMDAT section -> its defined symbols (value, name)
    for(uint i=0;i<symCount;i++){int o=(int)(symPtr+i*symSize);string n=Norm(SymName(o));int sec=big?(int)U32(b,o+12):(short)U16(b,o+12);int cls=b[o+(big?18:16)];int aux=b[o+(big?19:17)];uint value=U32(b,o+8);
      symbols.Add(Tuple.Create(n,sec,cls,value));
      for(int a=0;a<aux;a++)symbols.Add(Tuple.Create("",0,0,0u));
      if(sec>0&&sec<=sections&&n!=secName[sec]&&(cls==2||cls==3)){
        if(secComdat[sec]){if(!comdat.ContainsKey(sec))comdat[sec]=n;}
        else{List<Tuple<uint,string>> list;if(!defined.TryGetValue(sec,out list))defined[sec]=list=new List<Tuple<uint,string>>();list.Add(Tuple.Create(value,n));}}
      i+=(uint)aux;}
    for(int s=1;s<=sections;s++)secKey[s]=secName[s]+(comdat.ContainsKey(s)?"|"+comdat[s]:"");
    foreach(var list in defined.Values)list.Sort((x,y)=>x.Item1!=y.Item1?x.Item1.CompareTo(y.Item1):string.CompareOrdinal(x.Item2,y.Item2));
    // The chunk of a non-COMDAT section holding an offset: the last defined symbol at or before it.
    Func<int,uint,Tuple<uint,string>> ChunkOf=(sec,offset)=>{List<Tuple<uint,string>> list;if(!defined.TryGetValue(sec,out list))return Tuple.Create(0u,"");
      Tuple<uint,string> found=Tuple.Create(0u,"");foreach(var d in list){if(d.Item1<=offset)found=d;else break;}return found;};
    var keys=new Dictionary<string,int>();var result=new List<string>();
    Action<string,byte[],string> Add=(key,data,relocs)=>{int k;keys.TryGetValue(key,out k);keys[key]=k+1;
      if(Detail!=null&&key.Contains(Detail))Details.Add(key+" data "+Convert.ToHexString(data)+" relocations "+relocs);
      using(var sha=SHA256.Create()){var all=data.Concat(Encoding.UTF8.GetBytes(relocs)).ToArray();result.Add(key+"#"+k+" "+data.Length+" "+Convert.ToHexString(sha.ComputeHash(all)));}};
    for(int s=1;s<=sections;s++){
      int h=headerSize+(s-1)*40;string n=secName[s];if(n.StartsWith(".debug")||n==".chks64")continue;   // debug information and its checksums
      uint chr=U32(b,h+36);int size=(int)U32(b,h+16),raw=(int)U32(b,h+20),rel=(int)U32(b,h+24);int relCount=U16(b,h+32);
      var data=new byte[size];if((chr&0x80)==0&&size>0)Array.Copy(b,raw,data,0,size);
      var relocs=new List<Tuple<uint,string>>();
      // More than 65,535 relocations (IMAGE_SCN_LNK_NRELOC_OVFL): the first entry holds the count, itself included.
      int firstReloc=0;if((chr&0x01000000)!=0&&relCount==0xFFFF){relCount=(int)U32(b,rel);firstReloc=1;}
      for(int r=firstReloc;r<relCount;r++){int o=rel+r*10;uint off=U32(b,o);var t=symbols[(int)U32(b,o+4)];int type=U16(b,o+8);string target;
        if(t.Item3==6&&t.Item2>0&&t.Item2<=sections)target="@"+secKey[t.Item2]+"+"+t.Item4;   // a label: its section and offset
        else if(t.Item2>0&&t.Item2<=sections&&t.Item3!=2&&!(t.Item1!=secName[t.Item2]&&secComdat[t.Item2])){
          if(t.Item1==secName[t.Item2])target="@"+secKey[t.Item2];   // a section symbol: the addend stays in the bytes
          else{var c=secComdat[t.Item2]?null:ChunkOf(t.Item2,t.Item4);target=c==null||c.Item2==""?t.Item1+"@"+secKey[t.Item2]+"+"+t.Item4:"@"+secName[t.Item2]+"|"+c.Item2+"+"+(t.Item4-c.Item1);}}
        else target=t.Item1;
        relocs.Add(Tuple.Create(off,type+":"+target));}
      List<Tuple<uint,string>> chunks;
      if(secComdat[s]||!defined.TryGetValue(s,out chunks)||chunks.Count==0){
        Add(secKey[s],data,string.Join(";",relocs.Select(x=>x.Item1+":"+x.Item2)));continue;}
      // Non-COMDAT: the bytes before the first symbol, then one chunk per symbol.
      var starts=new List<Tuple<uint,string>>{Tuple.Create(0u,"")};starts.AddRange(chunks.Where(c=>c.Item1>0||c.Item2!=""));
      for(int c=0;c<starts.Count;c++){uint from=starts[c].Item1,to=c+1<starts.Count?starts[c+1].Item1:(uint)size;if(to<from)to=from;
        if(starts[c].Item2==""&&to==from)continue;
        // The alignment padding before the next symbol (zero bytes past the last relocation, fewer than 16) depends on that
        // neighbour, not on this definition.
        uint end=to,lastReloc=relocs.Where(x=>x.Item1>=from&&x.Item1<to).Select(x=>x.Item1+4).DefaultIfEmpty(from).Max();
        while(end>from&&end>lastReloc&&to-end<15&&data[end-1]==0)end--;
        var piece=new byte[end-from];Array.Copy(data,(int)from,piece,0,(int)(end-from));
        var mine=relocs.Where(x=>x.Item1>=from&&x.Item1<to).Select(x=>(x.Item1-from)+":"+x.Item2);
        Add(n+"|"+starts[c].Item2,piece,string.Join(";",mine));}}
    result.Sort(StringComparer.Ordinal);return result;}
}
'@}
if($Detail){[NativeBinaryCompare]::Detail=$Detail}
$a=if($Objects){[NativeBinaryCompare]::Obj($Before)}else{[NativeBinaryCompare]::Pe($Before)}
$b=if($Objects){[NativeBinaryCompare]::Obj($After)}else{[NativeBinaryCompare]::Pe($After)}
if($Detail){[NativeBinaryCompare]::Details;return}
$setA=[Collections.Generic.HashSet[string]]::new([string[]]$a);$setB=[Collections.Generic.HashSet[string]]::new([string[]]$b)
$onlyBefore=@($a | Where-Object {-not $setB.Contains($_)});$onlyAfter=@($b | Where-Object {-not $setA.Contains($_)})
"sections: before $($a.Count), after $($b.Count), differing: before $($onlyBefore.Count), after $($onlyAfter.Count)"
$onlyBefore | Select-Object -First 40 | ForEach-Object {"  - $_"}
$onlyAfter | Select-Object -First 40 | ForEach-Object {"  + $_"}
if($onlyBefore.Count -or $onlyAfter.Count){exit 1}
"PASS identical machine code and data: $(Split-Path -Leaf $Before) = $(Split-Path -Leaf $After) (debug information excluded)"
