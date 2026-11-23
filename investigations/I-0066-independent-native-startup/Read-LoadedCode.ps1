[CmdletBinding()]
param([Parameter(Mandatory)][int]$TargetProcessId,
    [Parameter(Mandatory)][uint32[]]$Rva,
    [ValidateRange(16,512)][int]$Bytes=512,
    [Parameter(Mandatory)][string]$Output)
$ErrorActionPreference='Stop'
$expected='C:\Program Files (x86)\Steam\steamapps\common\DARK SOULS III\Game\DarkSoulsIII.exe'
$target=Get-Process -Id $TargetProcessId
if($target.Path -ne $expected -or (Get-FileHash -LiteralPath $expected).Hash -ne 'EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C'){ throw 'Read-only source identity mismatch' }
if(Test-Path -LiteralPath $Output){ throw 'Never overwrite acquisition' }
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class LoadedCodeRead {
 [DllImport("kernel32.dll",SetLastError=true)] public static extern IntPtr OpenProcess(uint access,bool inherit,int pid);
 [DllImport("kernel32.dll",SetLastError=true)] public static extern bool ReadProcessMemory(IntPtr process,IntPtr address,byte[] bytes,UIntPtr length,out UIntPtr read);
 [DllImport("kernel32.dll")] public static extern bool CloseHandle(IntPtr handle);
}
'@
$handle=[LoadedCodeRead]::OpenProcess(0x1010,$false,$TargetProcessId)
if($handle -eq [IntPtr]::Zero){ throw 'Read-only process access unavailable' }
try {
    $image=$target.MainModule.BaseAddress.ToInt64()
    $size=$target.MainModule.ModuleMemorySize
    $records=@(foreach($offset in $Rva){
        if($offset -gt $size-$Bytes){ throw 'Acquisition exceeds pinned module' }
        $data=[byte[]]::new($Bytes);$count=[UIntPtr]::Zero
        if(-not [LoadedCodeRead]::ReadProcessMemory($handle,[IntPtr]($image+$offset),$data,[UIntPtr]$Bytes,[ref]$count) -or $count.ToUInt64() -ne $Bytes){ throw 'Incomplete loaded code read' }
        [ordered]@{rva=$offset;bytes=$Bytes;sha256=[Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($data));hex=[Convert]::ToHexString($data).ToLowerInvariant()}
    })
    [ordered]@{created_utc=[DateTime]::UtcNow.ToString('o');pid=$TargetProcessId;image_base=$image;game_sha256=(Get-FileHash -LiteralPath $expected).Hash;read_only=$true;records=$records} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $Output -Encoding utf8
} finally { [void][LoadedCodeRead]::CloseHandle($handle) }
