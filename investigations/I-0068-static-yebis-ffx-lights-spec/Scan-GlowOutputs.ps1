[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$ProgramDir,   # the programme cache (I-0063: programs\<bundle>__<name>.ppo)
    [Parameter(Mandatory = $true)][string]$Out           # output TSV, one row per pixel programme with two or more outputs
)
# I-0068 part AS: which pixel programmes write a second render target, and what they write there. For every .ppo with at least two
# outputs (dcl_output o1..): the programme's output count, the instructions that write o5.w / o5.xyz (the G-buffer emissive target
# GBuffer5, whose alpha is the weight EmissiveToGlowPS multiplies the colour by) and o1.w / o1.xyz (the forward and FFX glow target),
# and the class of the o5.w source: 'MTDword cb3[N].w' (a cbMtdParam word, for the Glow families FC_GlowScale.w), 'constant', or
# 'computed' (a register). Disassembly by the system d3dcompiler_47.dll (as Read-Dxbc.ps1). Read-only.
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
$rows = New-Object System.Collections.ArrayList
[void]$rows.Add("programme`toutputs`to5_w_class`to5_w`to5_xyz`to1_w`to1_xyz")
foreach ($f in Get-ChildItem -LiteralPath $ProgramDir -Filter *.ppo) {
    $t = [D3DC2]::Disasm([IO.File]::ReadAllBytes($f.FullName)); if ($t.StartsWith('ERR')) { continue }
    $n = 0; foreach ($m in [regex]::Matches($t, 'dcl_output o(\d+)')) { $n = [Math]::Max($n, [int]$m.Groups[1].Value + 1) }
    if ($n -lt 2) { continue }
    function Writes($pat) { (([regex]::Matches($t, "(?m)^\s*(?:mov|movc|mul|mad|and|min|max|add|sqrt)\w*\s+$pat[^\r\n]*") | ForEach-Object { $_.Value.Trim() }) -join ' ; ') }
    $w5 = Writes 'o5\.w'; $x5 = Writes 'o5\.xyz'; $w1 = Writes 'o1\.w'; $x1 = Writes 'o1\.xyz'
    $cls = if (-not $w5) { 'none' } elseif ($w5 -match 'cb3\[(\d+)\]\.w') { "MTDword cb3[$($Matches[1])].w" } elseif ($w5 -match 'l\(\s*[0-9.]+\s*\)' -and $w5 -notmatch 'r\d') { 'constant' } else { 'computed' }
    [void]$rows.Add(("{0}`t{1}`t{2}`t{3}`t{4}`t{5}`t{6}" -f ($f.Name -replace '^.*?__', ''), $n, $cls, $w5, $x5, $w1, $x1))
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
[IO.File]::WriteAllLines($Out, $rows)
"rows $($rows.Count - 1) -> $Out"
