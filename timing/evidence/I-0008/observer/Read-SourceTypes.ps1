[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][int]$TargetPid,
    [Parameter(Mandatory=$true)][string]$CaptureFile,
    
    [Parameter(Mandatory=$true)][string]$OutputJson
)

$ErrorActionPreference = 'Stop'

if (-not ('SpatialOwnerReader.NativeMethods' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
namespace SpatialOwnerReader {
    public static class NativeMethods {
        [DllImport("kernel32.dll", SetLastError=true)]
        public static extern IntPtr OpenProcess(uint access, bool inherit, int processId);
        [DllImport("kernel32.dll", SetLastError=true)]
        public static extern bool ReadProcessMemory(IntPtr process, IntPtr address,
            byte[] buffer, UIntPtr size, out UIntPtr read);
        [DllImport("kernel32.dll")]
        public static extern bool CloseHandle(IntPtr handle);
    }
}
'@
}

function Convert-HexPointer([string]$Value) {
    return [Convert]::ToUInt64($Value.Substring(2), 16)
}

$moduleRows=@((Get-Process -Id $TargetPid).Modules | ForEach-Object { [pscustomobject]@{Name=$_.ModuleName;Base=[UInt64]$_.BaseAddress.ToInt64();Size=[UInt64]$_.ModuleMemorySize} })
$game=$moduleRows|Where-Object Name -eq 'DarkSoulsIII.exe'|Select-Object -First 1
if(-not $game){throw 'Game not found'}
$raw=[IO.File]::ReadAllBytes((Resolve-Path $CaptureFile))
$count=[BitConverter]::ToUInt64($raw,16)
$sourceAddresses=@(for($i=0;$i -lt $count;$i++) {
    $offset=72+$i*280
    $site=[BitConverter]::ToUInt64($raw,$offset+24)
    if($site -eq 0x1078330 -or $site -eq 0xF1B070){[BitConverter]::ToUInt64($raw,$offset+32)}
})|Sort-Object -Unique
$handle = [SpatialOwnerReader.NativeMethods]::OpenProcess(0x0410, $false, $TargetPid)
if ($handle -eq [IntPtr]::Zero) { throw "OpenProcess failed: $([Runtime.InteropServices.Marshal]::GetLastWin32Error())" }

function Read-Bytes([UInt64]$Address, [int]$Count) {
    $buffer = [byte[]]::new($Count)
    [UIntPtr]$read = [UIntPtr]::Zero
    $ok = [SpatialOwnerReader.NativeMethods]::ReadProcessMemory(
        $handle, [IntPtr]::new([Int64]$Address), $buffer, [UIntPtr]::new([UInt64]$Count), [ref]$read)
    if (-not $ok -or $read.ToUInt64() -ne [UInt64]$Count) { return $null }
    return $buffer
}

function Read-U64([UInt64]$Address) {
    $bytes = Read-Bytes $Address 8
    if ($null -eq $bytes) { return $null }
    return [BitConverter]::ToUInt64($bytes, 0)
}

function Resolve-Address([UInt64]$Address) {
    $module = $moduleRows | Where-Object {
        $Address -ge $_.Base -and $Address -lt ($_.Base + $_.Size)
    } | Select-Object -First 1
    if (-not $module) { return [pscustomobject]@{ module=$null; rva=$null } }
    return [pscustomobject]@{
        module = $module.Name
        rva = ('0x{0:X}' -f ($Address - $module.Base))
    }
}

function Read-Rtti([UInt64]$Object) {
    $vtable = Read-U64 $Object
    if ($null -eq $vtable -or $vtable -eq 0) {
        return [pscustomobject]@{ readable=$false; vtable=$null; module=$null; rva=$null; type_name=$null }
    }
    $resolved = Resolve-Address $vtable
    $typeName = $null
    $locator = Read-U64 ($vtable - 8)
    if ($null -ne $locator -and $locator -ne 0) {
        $col = Read-Bytes $locator 24
        if ($null -ne $col -and [BitConverter]::ToUInt32($col,0) -eq 1) {
            $typeRva = [BitConverter]::ToUInt32($col,12)
            $selfRva = [BitConverter]::ToUInt32($col,20)
            if (($game.Base + $selfRva) -eq $locator) {
                $nameBytes = Read-Bytes ($game.Base + $typeRva + 16) 256
                if ($null -ne $nameBytes) {
                    $end = [Array]::IndexOf($nameBytes,[byte]0)
                    if ($end -lt 0) { $end = $nameBytes.Length }
                    $typeName = [Text.Encoding]::ASCII.GetString($nameBytes,0,$end)
                }
            }
        }
    }
    return [pscustomobject]@{
        readable=$true
        vtable=('0x{0:X16}' -f $vtable)
        module=$resolved.module
        rva=$resolved.rva
        type_name=$typeName
    }
}

try {
    $sources=@(foreach($address in $sourceAddresses){[pscustomobject]@{source=('0x{0:X16}' -f $address);rtti=Read-Rtti $address}})
    $result=[ordered]@{schema='I-0008-post-capture-source-rtti';process_id=$TargetPid;captured_after_stop=$true;read_only_access='PROCESS_QUERY_INFORMATION|PROCESS_VM_READ';image_base=('0x{0:X16}' -f $game.Base);sources=$sources;limit='Post-capture pointer reads, not in-handler vtables. Object lifetime/reuse is not proved.'}
    $json=$result|ConvertTo-Json -Depth 8
    [IO.File]::WriteAllText((Join-Path (Get-Location) $OutputJson),$json+"`n",[Text.UTF8Encoding]::new($false))
    $json
}finally{[void][SpatialOwnerReader.NativeMethods]::CloseHandle($handle)}
