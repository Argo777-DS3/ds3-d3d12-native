[CmdletBinding()]
param([string]$Exe='C:/Program Files (x86)/Steam/steamapps/common/DARK SOULS III/Game/DarkSoulsIII.exe',
    [Parameter(Mandatory)][string]$Out)
# I-0066 (menu tests preparation, 2026-10-16): static census of the Scaleform menu pages named in the executable. Reads the exe file
# only (nothing is launched, nothing written next to it). Output: <Out>/menu-pages.tsv (page, RVA of its UTF-16 name) and
# <Out>/menu-page-xrefs.tsv (page, code|data, RVA of the referencing LEA or 8-byte pointer, section). A page with no row in the second
# file has no direct reference in the image (dead or opened through a name built at run time).
$ErrorActionPreference='Stop'
$bytes=[IO.File]::ReadAllBytes($Exe)
$pe=[BitConverter]::ToInt32($bytes,0x3c);$count=[BitConverter]::ToUInt16($bytes,$pe+6);$optional=[BitConverter]::ToUInt16($bytes,$pe+20);$table=$pe+24+$optional
$sections=for($i=0;$i -lt $count;$i++){$o=$table+$i*40;[pscustomobject]@{virtual=[BitConverter]::ToUInt32($bytes,$o+8);rva=[BitConverter]::ToUInt32($bytes,$o+12);raw=[BitConverter]::ToUInt32($bytes,$o+16);at=[BitConverter]::ToUInt32($bytes,$o+20)}}
$text=[Text.Encoding]::Latin1.GetString($bytes);$pages=[ordered]@{}
foreach($m in [regex]::Matches($text,'(?:[0-9A-Za-z_/\.]\x00){6,80}')){
    $name=[Text.Encoding]::Unicode.GetString($bytes,$m.Index,$m.Length)
    if($name -notmatch '^(?:[Ww]in/)?\d\d_\d\d\d' -or $pages.Contains($name)){continue}
    $section=$sections|Where-Object{$m.Index -ge $_.at -and $m.Index -lt $_.at+$_.raw}|Select-Object -First 1
    if($section){$pages[$name]='{0:X}' -f ($section.rva+$m.Index-$section.at)}}
New-Item -ItemType Directory -Force -Path $Out|Out-Null
$list=Join-Path $Out 'menu-pages.tsv';$pages.GetEnumerator()|Sort-Object Name|ForEach-Object{"$($_.Name)`t$($_.Value)"}|Set-Content -LiteralPath $list
Add-Type -TypeDefinition @'
using System;using System.Collections.Generic;using System.IO;using System.Text;
public static class MenuPageXrefs{
 public static string Run(string exe,string pages){
  var b=File.ReadAllBytes(exe);int e=BitConverter.ToInt32(b,0x3c);int n=BitConverter.ToUInt16(b,e+6);int opt=BitConverter.ToUInt16(b,e+20);int so=e+24+opt;
  ulong imageBase=BitConverter.ToUInt64(b,e+24+24);var secs=new List<uint[]>();var names=new List<string>();
  for(int i=0;i<n;i++){int o=so+i*40;names.Add(Encoding.ASCII.GetString(b,o,8).TrimEnd('\0'));
   secs.Add(new uint[]{BitConverter.ToUInt32(b,o+8),BitConverter.ToUInt32(b,o+12),BitConverter.ToUInt32(b,o+16),BitConverter.ToUInt32(b,o+20),BitConverter.ToUInt32(b,o+36)});}
  var targets=new Dictionary<uint,string>();
  foreach(var line in File.ReadAllLines(pages)){var p=line.Split('\t');targets[Convert.ToUInt32(p[1],16)]=p[0];}
  var sb=new StringBuilder();
  for(int i=0;i<n;i++){var s=secs[i];uint va=s[1],rp=s[3],len=Math.Min(s[0],s[2]);string nm;
   if((s[4]&0x20000000)!=0){
    for(uint k=0;k+7<=len;k++){uint off=rp+k;
     if((b[off]==0x48||b[off]==0x4C)&&b[off+1]==0x8D&&(b[off+2]&0xC7)==0x05){
      uint t=(uint)((long)va+k+7+BitConverter.ToInt32(b,(int)off+3));
      if(targets.TryGetValue(t,out nm))sb.AppendLine(nm+"\tcode\t"+(va+k).ToString("X")+"\t"+names[i]);}}
   }else{
    for(uint k=0;k+8<=len;k+=8){ulong v=BitConverter.ToUInt64(b,(int)(rp+k));
     if(v>=imageBase&&v<imageBase+0x8000000&&targets.TryGetValue((uint)(v-imageBase),out nm))sb.AppendLine(nm+"\tdata\t"+(va+k).ToString("X")+"\t"+names[i]);}}}
  return sb.ToString();}
}
'@
Set-Content -LiteralPath (Join-Path $Out 'menu-page-xrefs.tsv') -Value ([MenuPageXrefs]::Run($Exe,$list))
"pages $($pages.Count) -> $Out"
