[CmdletBinding()]
param(
    [string]$Programs = 'C:\Users\User\.codex\I0063-cache\programs',   # I-0063 programme cache (licensed bytes, outside the repository)
    [Parameter(Mandatory = $true)][string]$Out                          # output TSV, one row per hull or domain programme
)
# I-0068 part AW: which variables of cbTessScene and cbPNTriangles every hull (.hpo) and domain (.dpo) programme of the cache really
# reads, from the compiler's own reflection comments ("[unused]" marks a variable the code never reads), plus the constant-buffer
# registers. Disassembly by the system d3dcompiler_47.dll. Read-only.
$ErrorActionPreference = 'Stop'
if (-not ('D3DC3' -as [type])) {
    Add-Type -TypeDefinition @'
using System; using System.Runtime.InteropServices;
public static class D3DC3 {
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
function Used($t, $buffer) {
    # variables of the named cbuffer: "name@offset" when read, "-" prefix when marked [unused]
    $m = [regex]::Match($t, "(?s)// cbuffer $buffer\r?\n// \{(.*?)// \}")
    if (-not $m.Success) { return '' }
    $r = New-Object System.Collections.ArrayList
    foreach ($line in ($m.Groups[1].Value -split "\r?\n")) {
        $v = [regex]::Match($line, '//\s+\w+\s+(\w+)(\[\d+\])?;\s*//\s*Offset:\s*(\d+)\s+Size:\s*(\d+)(\s+\[unused\])?')
        if ($v.Success) { [void]$r.Add(("{0}{1}@{2}" -f $(if ($v.Groups[5].Success) { '-' } else { '' }), $v.Groups[1].Value, $v.Groups[3].Value)) }
    }
    return ($r -join ' ')
}
$rows = New-Object System.Collections.ArrayList
[void]$rows.Add("programme`tstage`ttess_reg`ttessScene_vars`tpnTriangles_vars`tgui2_components`teye_components`tview_components")
foreach ($f in Get-ChildItem -LiteralPath $Programs -File | Where-Object { ($_.Extension -in '.hpo', '.dpo') -and $_.Name -notmatch '^gxffxshader' }) {
    $t = [D3DC3]::Disasm([IO.File]::ReadAllBytes($f.FullName)); if ($t.StartsWith('ERR')) { continue }
    $reg = [regex]::Match($t, 'cbTessScene\s+cbuffer\s+NA\s+NA\s+(cb\d+)').Groups[1].Value
    $comp = { param($text, $idx) $s = @{}; foreach ($m in [regex]::Matches($text, ('cb5\[' + $idx + '\]\.(\w+)'))) { foreach ($c in $m.Groups[1].Value.ToCharArray()) { $s[[string]$c] = 1 } }; (($s.Keys | Sort-Object) -join '') }
    [void]$rows.Add(("{0}`t{1}`t{2}`t{3}`t{4}`t{5}`t{6}`t{7}" -f ($f.Name -replace '^.*?__', ''), $f.Extension.TrimStart('.'), $reg, (Used $t 'cbTessScene'), (Used $t 'cbPNTriangles'), (& $comp $t 6), (& $comp $t 4), (& $comp $t 5)))
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
[IO.File]::WriteAllLines($Out, $rows)
"rows $($rows.Count - 1) -> $Out"
