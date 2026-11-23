[CmdletBinding(PositionalBinding = $false)]
param(
    [Parameter(Mandatory = $true)][string]$Dir,      # folder of recovered menu .gfx files (uncompressed GFX)
    [Parameter(Mandatory = $true)][string]$Out,      # output directory: placements.tsv, strings.tsv, tree.txt
    [string[]]$Tree = @()                            # optional "page.gfx:spriteId:depth" display-list dumps written to tree.txt (0 = root timeline)
)
# I-0069 (2026-10-11): where each filter and cacheAsBitmap placement of the menu movies sits, so that a witness page can be chosen
# from what the page really instantiates. For every PlaceObject2/3 carrying a filter list or the cache-as-bitmap flag it records the
# containing sprite (or the root timeline), the sprite's SymbolClass/ExportAssets names, whether a static chain of placements leads from the
# root timeline to it (and at which root frame), the placement's depth, name, colour-transform multiply alpha, and the filter parameters
# (all eight filter ids, including colour matrix). It also records which names of the filter and colour classes appear in the movie's ABC
# constant pools (none do in the shipped movies). With -Tree it dumps the display list of a sprite. Read-only; no game process.
$ErrorActionPreference = 'Stop'
Add-Type -TypeDefinition @'
using System; using System.Collections.Generic; using System.IO; using System.Text;
public class GfxCtx {
  public class Place { public int Container, Depth, Char, Frame; public string Name=""; public bool Cache; public int Blend; public int ClipDepth; public int MulA=-1;
    public string Filters=""; public bool HasMulAlphaZero; public int Kind; public int Ratio=-1; }
  public class Sp { public int Id, Frames; public List<Place> Places=new List<Place>(); public List<int> Children=new List<int>(); }
  public Dictionary<int,Sp> Sprites=new Dictionary<int,Sp>();
  public Sp Root=new Sp();
  public Dictionary<int,string> Names=new Dictionary<int,string>();
  public Dictionary<int,string> Types=new Dictionary<int,string>();
  public List<string> Strings=new List<string>();
  byte[] d; int p;
  int U8(){return d[p++];} int U16(){int v=d[p]|(d[p+1]<<8);p+=2;return v;} uint U32(){uint v=BitConverter.ToUInt32(d,p);p+=4;return v;}
  float F32(){float v=BitConverter.ToSingle(d,p);p+=4;return v;}
  string Str(){int s=p;while(d[p]!=0)p++;string r=Encoding.UTF8.GetString(d,s,p-s);p++;return r;}
  int bit;
  uint Bits(int n){uint v=0;for(int i=0;i<n;i++){int b=bit>>3;int sh=7-(bit&7);v=(v<<1)|(uint)((d[b]>>sh)&1);bit++;}return v;}
  void BeginBits(){bit=p*8;} void EndBits(){bit=(bit+7)&~7;p=bit>>3;}
  void Matrix(){BeginBits();if(Bits(1)==1){int n=(int)Bits(5);Bits(n);Bits(n);}if(Bits(1)==1){int n=(int)Bits(5);Bits(n);Bits(n);}int t=(int)Bits(5);Bits(t);Bits(t);EndBits();}
  int[] Cxform(bool alpha){BeginBits();uint hasAdd=Bits(1),hasMul=Bits(1);int n=(int)Bits(4);int[] mul=null;
    if(hasMul==1){mul=new int[4];int cnt=alpha?4:3;for(int i=0;i<cnt;i++){uint v=Bits(n);mul[i]=(int)((v&(1u<<(n-1)))!=0?(int)(v|(uint.MaxValue<<n)):(int)v);}if(!alpha)mul[3]=256;}
    if(hasAdd==1){int cnt=alpha?4:3;for(int i=0;i<cnt;i++)Bits(n);}EndBits();return mul;}
  string Filters(){int n=U8();var s=new List<string>();
    for(int i=0;i<n;i++){int id=U8();
      switch(id){
        case 0:{uint c=U32();float bx=U32()/65536f,by=U32()/65536f,a=(int)U32()/65536f,di=(int)U32()/65536f;float st=U16()/256f;int f=U8();
          s.Add("dropshadow rgba="+c.ToString("x8")+" blur="+bx+"x"+by+" angle="+a+" dist="+di+" strength="+st+" inner="+((f>>7)&1)+" knockout="+((f>>6)&1)+" composite="+((f>>5)&1)+" passes="+(f&31));break;}
        case 1:{float bx=U32()/65536f,by=U32()/65536f;int f=U8();s.Add("blur blur="+bx+"x"+by+" passes="+(f>>3));break;}
        case 2:{uint c=U32();float bx=U32()/65536f,by=U32()/65536f;float st=U16()/256f;int f=U8();
          s.Add("glow rgba="+c.ToString("x8")+" blur="+bx+"x"+by+" strength="+st+" inner="+((f>>7)&1)+" knockout="+((f>>6)&1)+" composite="+((f>>5)&1)+" passes="+(f&31));break;}
        case 3:p+=27;s.Add("bevel");break;
        case 4:case 7:{int nc=U8();p+=nc*4+nc+8+4+4+2+1;s.Add(id==4?"gradientglow":"gradientbevel");break;}
        case 5:{int mx=U8(),my=U8();p+=8+4*mx*my+4+1;s.Add("convolution");break;}
        case 6:{var m=new List<string>();for(int k=0;k<20;k++)m.Add(F32().ToString("0.###"));s.Add("colormatrix "+string.Join(",",m));break;}
        default:throw new Exception("filter id "+id);}}
    return string.Join(" | ",s);}
  void Tags(Sp sp,int end){int frame=1;
    while(p<end){int h=U16();int type=h>>6;int len=h&63;if(len==63)len=(int)U32();int next=p+len;
      try{
      if(type==39){int id=U16();int fr=U16();var child=new Sp{Id=id,Frames=fr};Sprites[id]=child;Types[id]="sprite";Tags(child,next);}
      else if(type==26||type==70){int flags=U8();int flags3=0;if(type==70)flags3=U8();int depth=U16();
        var pl=new Place{Container=sp.Id,Depth=depth,Frame=frame,Kind=type};
        if(type==70&&((flags3&0x08)!=0||((flags3&0x10)!=0&&(flags&2)!=0)))Str();   // class name precedes the character id
        if((flags&2)!=0){pl.Char=U16();sp.Children.Add(pl.Char);}
        if((flags&4)!=0)Matrix();
        if((flags&8)!=0){var mul=Cxform(true);if(mul!=null){pl.MulA=mul[3];if(mul[3]==0)pl.HasMulAlphaZero=true;}}
        if((flags&16)!=0)pl.Ratio=U16();
        if((flags&32)!=0)pl.Name=Str();
        if((flags&64)!=0)pl.ClipDepth=U16();
        if(type==70){
          if((flags3&1)!=0)pl.Filters=Filters();
          if((flags3&2)!=0)pl.Blend=U8();
          if((flags3&4)!=0){pl.Cache=U8()!=0;}
        }
        sp.Places.Add(pl);}
      else if(type==1)frame++;
      else if(type==37){Types[U16()]="edittext";}
      else if(type==11||type==33){Types[U16()]="statictext";}
      else if(type==2||type==22||type==32||type==83){Types[U16()]="shape";}
      else if(type==6||type==20||type==21||type==35||type==36||type==90){Types[U16()]="bitmap";}
      else if(type==56){int n=U16();for(int i=0;i<n;i++){int id=U16();Names[id]="export:"+Str();}}
      else if(type==76){int n=U16();for(int i=0;i<n;i++){int id=U16();string nm=Str();Names[id]=(Names.ContainsKey(id)?Names[id]+";":"")+"class:"+nm;}}
      else if(type==82){U32();Str();ScanAbc(p,next);}
      }catch(Exception){}
      p=next;}
  }
  void ScanAbc(int s,int e){
    var txt=Encoding.ASCII.GetString(d,s,e-s);
    foreach(var key in new[]{"ColorMatrixFilter","BlurFilter","DropShadowFilter","GlowFilter","BevelFilter","cacheAsBitmap","cacheAsBitmapMatrix","filters","ColorTransform","colorTransform"}){
      int i=txt.IndexOf(key,StringComparison.Ordinal);int n=0;while(i>=0){n++;i=txt.IndexOf(key,i+key.Length,StringComparison.Ordinal);}
      if(n>0)Strings.Add(key+"="+n);}}
  public void Load(string path){d=File.ReadAllBytes(path);
    if(d[0]!='G'||d[1]!='F'||d[2]!='X')throw new Exception("not an uncompressed GFX");
    p=8;BeginBits();int nb=(int)Bits(5);Bits(nb);Bits(nb);Bits(nb);Bits(nb);EndBits();p+=4;
    Root.Id=0;Tags(Root,d.Length);}
  public string Tree(int id,int depthLimit,string indent){var sb=new StringBuilder();Sp s=id==0?Root:(Sprites.ContainsKey(id)?Sprites[id]:null);if(s==null)return "";
    foreach(var pl in s.Places){
      sb.Append(indent+"f"+pl.Frame+" d"+pl.Depth+" char="+pl.Char+(Names.ContainsKey(pl.Char)?" ["+Names[pl.Char]+"]":"")+(pl.Name!=""?" name="+pl.Name:"")
        +(pl.Blend!=0?" blend="+pl.Blend:"")+(pl.ClipDepth!=0?" clipdepth="+pl.ClipDepth:"")+(pl.MulA>=0?" mulA="+pl.MulA:"")+(pl.Cache?" CACHE":"")
        +(pl.Filters!=""?" filters="+pl.Filters:"")+(Sprites.ContainsKey(pl.Char)?" (sprite, frames="+Sprites[pl.Char].Frames+")":"")+"\n");
      if(depthLimit>0&&Sprites.ContainsKey(pl.Char))sb.Append(Tree(pl.Char,depthLimit-1,indent+"  "));}
    return sb.ToString();}
  // lowest root-timeline frame from which a character is reachable by static placement chains
  public Dictionary<int,int> Reach(){var best=new Dictionary<int,int>();var q=new Queue<KeyValuePair<int,int>>();
    foreach(var pl in Root.Places)if(pl.Char!=0){if(!best.ContainsKey(pl.Char)||pl.Frame<best[pl.Char]){best[pl.Char]=pl.Frame;q.Enqueue(new KeyValuePair<int,int>(pl.Char,pl.Frame));}}
    while(q.Count>0){var kv=q.Dequeue();Sp s;if(!Sprites.TryGetValue(kv.Key,out s))continue;
      foreach(var c in s.Children)if(!best.ContainsKey(c)){best[c]=kv.Value;q.Enqueue(new KeyValuePair<int,int>(c,kv.Value));}}
    return best;}
}
'@
New-Item -ItemType Directory -Force -Path $Out | Out-Null
$rows = New-Object System.Collections.ArrayList
[void]$rows.Add("page`tcontainer`tcontainer_names`troot_reach_frame`tinstanced_by_class`tdepth`tinstance_name`tcharacter`tcharacter_type`tcache`talpha_mul_zero`tfilters")
$strings = New-Object System.Collections.ArrayList
[void]$strings.Add("page`tabc_mentions")
foreach ($f in Get-ChildItem -LiteralPath $Dir -Filter *.gfx | Sort-Object Name) {
    $g = New-Object GfxCtx
    try { $g.Load($f.FullName) } catch { [void]$rows.Add("$($f.Name)`t!error $($_.Exception.Message)"); continue }
    $reach = $g.Reach()
    $all = New-Object System.Collections.ArrayList
    foreach ($pl in $g.Root.Places) { [void]$all.Add($pl) }
    foreach ($s in $g.Sprites.Values) { foreach ($pl in $s.Places) { [void]$all.Add($pl) } }
    foreach ($pl in $all) {
        if (-not $pl.Filters -and -not $pl.Cache) { continue }
        $cname = if ($pl.Container -eq 0) { 'root' } else { "sprite $($pl.Container)" }
        $names = if ($g.Names.ContainsKey($pl.Container)) { $g.Names[$pl.Container] } else { '' }
        $rf = if ($pl.Container -eq 0) { $pl.Frame } elseif ($reach.ContainsKey($pl.Container)) { $reach[$pl.Container] } else { -1 }
        $cls = if ($names -like '*class:*') { 'yes' } else { 'no' }
        $ctype = if ($g.Types.ContainsKey($pl.Char)) { $g.Types[$pl.Char] } else { "" }
        [void]$rows.Add(("{0}`t{1}`t{2}`t{3}`t{4}`t{5}`t{6}`t{7}`t{8}`t{9}`t{10}`t{11}" -f $f.Name, $cname, $names, $rf, $cls, $pl.Depth, $pl.Name, $pl.Char, $ctype, $pl.Cache, $pl.HasMulAlphaZero, $pl.Filters))
    }
    [void]$strings.Add("$($f.Name)`t$($g.Strings -join ' ')")
}
[IO.File]::WriteAllLines((Join-Path $Out 'placements.tsv'), $rows)
[IO.File]::WriteAllLines((Join-Path $Out 'strings.tsv'), $strings)
"placements $($rows.Count - 1); pages $($strings.Count - 1) -> $Out"
if ($Tree.Count) {
    $lines = New-Object System.Collections.ArrayList
    foreach ($spec in ($Tree -split ',')) {
        $parts = $spec -split ':'; if ($parts.Count -lt 2) { continue }
        $g = New-Object GfxCtx; $g.Load((Join-Path $Dir $parts[0]))
        $limit = if ($parts.Count -gt 2) { [int]$parts[2] } else { 3 }
        [void]$lines.Add("== $($parts[0]) sprite $($parts[1]) depth $limit")
        foreach ($l in ($g.Tree([int]$parts[1], $limit, '') -split "`n")) { if ($l) { [void]$lines.Add($l) } }
    }
    [IO.File]::WriteAllLines((Join-Path $Out 'tree.txt'), $lines)
}
