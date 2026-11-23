[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$ProgramDir,
    [Parameter(Mandatory = $true)][string]$Out,
    [string[]]$Bundles = @('gxrenderershader', 'gxposteffect', 'gxdecal', 'gxffxshader'),
    [string[]]$Extensions = @('.ppo', '.vpo', '.cpo'),
    [string]$Pattern = '(?i)(inv.?(view|proj|vp)|proj|viewproj|wvp|cur.?to.?prev|prev|screen.?size|depth.?(compute|range|param)|texel|jitter|g_vp|vp_)'
)
# I-0068 part BQ: the variables of every constant buffer of the GX renderer, post-effect, decal and FFX programmes whose name says
# "matrix / projection / screen size / depth parameter", with the compiler's used flag: where an upscaler's jitter and render size
# have to be fed. RDEF text of the Microsoft disassembler (the same d3dcompiler_47.dll the native read analysis uses). Read-only.
$ErrorActionPreference = 'Stop'
if (-not ('D3DC2' -as [type])) {
    Add-Type -TypeDefinition @'
using System; using System.Runtime.InteropServices;
public static class D3DC2 {
  [DllImport("d3dcompiler_47.dll")] static extern int D3DDisassemble(byte[] src, UIntPtr size, uint flags, string comments, out IntPtr blob);
  [UnmanagedFunctionPointer(CallingConvention.StdCall)] delegate IntPtr GetPtr(IntPtr self);
  [UnmanagedFunctionPointer(CallingConvention.StdCall)] delegate UIntPtr GetSize(IntPtr self);
  public static string Disasm(byte[] b) {
    IntPtr blob; int hr = D3DDisassemble(b, (UIntPtr)b.Length, 0, null, out blob); if (hr != 0) return "ERR " + hr;
    IntPtr vtbl = Marshal.ReadIntPtr(blob);
    var gp = (GetPtr)Marshal.GetDelegateForFunctionPointer(Marshal.ReadIntPtr(vtbl, 3 * IntPtr.Size), typeof(GetPtr));
    var gs = (GetSize)Marshal.GetDelegateForFunctionPointer(Marshal.ReadIntPtr(vtbl, 4 * IntPtr.Size), typeof(GetSize));
    return Marshal.PtrToStringAnsi(gp(blob), (int)(ulong)gs(blob));
  }
}
'@
}
$rows = New-Object System.Collections.Generic.List[string]
$rows.Add("programme`tstage`tbuffer`tregister`tvariable`toffset`tsize`tused")
$n = 0
foreach ($f in (Get-ChildItem -LiteralPath $ProgramDir -File | Where-Object { $Extensions -contains $_.Extension })) {
    $bundle = ($f.Name -split '__')[0]; if ($Bundles -notcontains $bundle) { continue }
    $n++
    $text = [D3DC2]::Disasm([IO.File]::ReadAllBytes($f.FullName)); if ($text.StartsWith('ERR')) { continue }
    $lines = $text -split "`r?`n"; $cur = $null; $regOf = @{}; $vars = New-Object System.Collections.ArrayList
    foreach ($l in $lines) {
        if ($l -match '^// cbuffer (\S+)') { $cur = $Matches[1]; continue }
        if ($cur -and $l -match '^//\s+\S+\s+(\S+?)(?:\[\d+\])?;\s+// Offset:\s+(\d+) Size:\s+(\d+)\s*(\[unused\])?') { [void]$vars.Add([pscustomobject]@{ b = $cur; n = $Matches[1]; o = [int]$Matches[2]; s = [int]$Matches[3]; used = -not $Matches[4] }); continue }
        if ($l -match '^// \}') { $cur = $null; continue }
        if ($l -match '^// (\S+)\s+cbuffer\s+NA\s+NA\s+(?:cb)?(\d+)') { $regOf[$Matches[1]] = [int]$Matches[2] }
    }
    $stage = $f.Extension.TrimStart('.')
    foreach ($v in $vars) { if ($v.n -match $Pattern) { $rows.Add("$($f.BaseName)`t$stage`t$($v.b)`t$($regOf[$v.b])`t$($v.n)`t$($v.o)`t$($v.s)`t$([int]$v.used)") } }
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
[IO.File]::WriteAllLines($Out, $rows)
"scanned $n programmes, $($rows.Count - 1) rows -> $Out"
