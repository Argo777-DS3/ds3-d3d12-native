[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$ProgramDir,     # the programme cache (I-0063: programs\<bundle>__<name>.<ext>)
    [Parameter(Mandatory = $true)][string]$Out,            # output TSV, one row per programme that reads cbSceneParam
    [string[]]$Extensions = @('.ppo', '.vpo'),
    [string]$Bundle = '*'                                  # bundle prefix filter, for example 'gxposteffect'
)
# I-0068 part BQ: which programmes read which rows of cbSceneParam. The rows of interest for an upscaler (row = byte offset / 16):
# 1 FC_ScreenSize, 2 FC_DepthComputeParam, 3 SC_CameraPos, 4 VC_ClipPlane, 5-7 FC_MatrixView, 8-11 FC_MatrixInvViewProj,
# 12-15 FC_MatrixInvProj, 16-19 FC_MatrixProj, 20-23 FC_MatrixInvView, 24-27 VC_MatrixViewProj (camera-relative), 55 FC_FrustumNear.
# Same replay of the Microsoft disassembler as Scan-ProgramReads.ps1 (fixed rows by immediate index, dynamic index = whole buffer).
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
$rows.Add("programme`tstage`tregister`trows`tdynamic")
$files = Get-ChildItem -LiteralPath $ProgramDir -File | Where-Object { $Extensions -contains $_.Extension -and $_.Name -like "$Bundle`__*" }
$n = 0
foreach ($f in $files) {
    $n++
    $text = [D3DC2]::Disasm([IO.File]::ReadAllBytes($f.FullName))
    if ($text.StartsWith('ERR')) { continue }
    if ($text.IndexOf('cbSceneParam') -lt 0) { continue }
    $lines = $text -split "`r?`n"
    $reg = -1
    foreach ($l in $lines) { if ($l -match '^// cbSceneParam\s+cbuffer\s+NA\s+NA\s+(?:cb)?(\d+)') { $reg = [int]$Matches[1]; break } }
    if ($reg -lt 0) { continue }
    $fixed = New-Object System.Collections.Generic.SortedSet[int]; $dyn = $false
    $re = [regex]"(?i)\bcb$reg\[([^\]]+)\]"
    foreach ($l in $lines) {
        if ($l -match '^\s*//' -or $l -match '^\s*dcl_') { continue }
        foreach ($m in $re.Matches($l)) { $idx = $m.Groups[1].Value.Trim(); if ($idx -match '^\d+$') { [void]$fixed.Add([int]$idx) } else { $dyn = $true } }
    }
    $stage = if ($f.Extension -eq '.vpo') { 'vs' } elseif ($f.Extension -eq '.ppo') { 'ps' } else { $f.Extension.TrimStart('.') }
    $rows.Add("$($f.BaseName)`t$stage`t$reg`t$($fixed -join ',')`t$([int]$dyn)")
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
[IO.File]::WriteAllLines($Out, $rows)
"scanned $n programmes, $($rows.Count - 1) read cbSceneParam -> $Out"
