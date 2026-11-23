[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][int]$TargetPid,
    [Parameter(Mandatory=$true)][string]$SpatialCsv,
    [Parameter(Mandatory=$true)][string]$ModulesCsv,
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

$moduleRows = Import-Csv -LiteralPath $ModulesCsv | ForEach-Object {
    [pscustomobject]@{
        Name = $_.module
        Base = Convert-HexPointer $_.base
        Size = [UInt64]$_.size
    }
}
$game = $moduleRows | Where-Object Name -eq 'DarkSoulsIII.exe' | Select-Object -First 1
if (-not $game) { throw 'DarkSoulsIII.exe is absent from the module snapshot' }

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
    $rows = Import-Csv -LiteralPath $SpatialCsv | Where-Object site -eq 'forward_entry'
    $owners = $rows | Group-Object owner | ForEach-Object {
        $owner = Convert-HexPointer $_.Name
        $player = Read-U64 ($owner + 0x90)
        $modules = if ($null -ne $player -and $player -ne 0) { Read-U64 ($player + 0x1F90) } else { $null }
        $module28 = if ($null -ne $modules -and $modules -ne 0) { Read-U64 ($modules + 0x28) } else { $null }
        [pscustomobject]@{
            owner = $_.Name
            captured_forward_entries = $_.Count
            owner_rtti = Read-Rtti $owner
            player = if ($null -ne $player) { '0x{0:X16}' -f $player } else { $null }
            player_rtti = if ($null -ne $player -and $player -ne 0) { Read-Rtti $player } else { $null }
            module28_object = if ($null -ne $module28) { '0x{0:X16}' -f $module28 } else { $null }
            module28_rtti = if ($null -ne $module28 -and $module28 -ne 0) { Read-Rtti $module28 } else { $null }
        }
    }
    $result = [ordered]@{
        schema = 'I-0007-spatial-owner-rtti-v1'
        process_id = $TargetPid
        captured_after_instrument_stop = $true
        read_only_process_access = 'PROCESS_QUERY_INFORMATION|PROCESS_VM_READ'
        image_base = ('0x{0:X16}' -f $game.Base)
        owner_count = @($owners).Count
        owners = @($owners)
        limit = 'Post-capture live read. Unreadable or reused objects remain unknown; RTTI names are exact-build implementation names, not gameplay semantics.'
    }
    $json = $result | ConvertTo-Json -Depth 8
    [IO.File]::WriteAllText((Join-Path (Get-Location) $OutputJson), $json + "`n", [Text.UTF8Encoding]::new($false))
    $json
}
finally {
    [void][SpatialOwnerReader.NativeMethods]::CloseHandle($handle)
}
