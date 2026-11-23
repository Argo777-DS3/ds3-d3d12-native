[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$ProgramDir,   # the programme cache (I-0063: programs\<bundle>__<name>.<ext>)
    [Parameter(Mandatory = $true)][string[]]$Pattern,    # file name patterns without extension, for example 'GXFlver_ColDifSpcBumpEmiIbl_Bloody_*' 'Veill_NoLight*'
    [Parameter(Mandatory = $true)][string]$Out           # output TSV, one row per (programme, constant buffer)
)
# I-0068 part AR: the native read analysis (native/world/cb_reads.cpp AnalyzeConservativeListing) replayed over cached DXBC programmes.
# For every .vpo/.ppo (vertex and pixel; hull and domain are not drawn by the native model path) matching a pattern, the disassembly
# of the Microsoft compiler is scanned: a constant-buffer operand cbN[k] with an immediate row k is a FIXED read of that row (components
# from its swizzle); cbN[<anything else>] is a DYNAMIC read, which the native analysis turns into "the whole buffer is read"
# (see FOLIAGE-NATIVE §7). RDEF gives each buffer's name, register and declared size and each variable's offset, size and the compiler's
# "used" flag. Output columns: programme, stage, buffer, register, declared bytes, fixed rows read (row:components), dynamic yes/no,
# the used variables (name@offset:bytes). Read-only.
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
[void]$rows.Add("programme`tstage`tbuffer`tregister`tdeclared_bytes`tfixed_rows`tdynamic`tused_variables")
$files = foreach ($p in $Pattern) { Get-ChildItem -LiteralPath $ProgramDir -File | Where-Object { $_.Name -like "*__$p.vpo" -or $_.Name -like "*__$p.ppo" -or $_.Name -like "*__$p@*.vpo" -or $_.Name -like "*__$p@*.ppo" } }
foreach ($f in ($files | Sort-Object FullName -Unique)) {
    $text = [D3DC2]::Disasm([IO.File]::ReadAllBytes($f.FullName))   # ReadAllBytes: names with brackets must not be wildcards
    if ($text.StartsWith('ERR')) { [void]$rows.Add("$($f.Name)`t?`tDISASM-FAILED`t`t`t`t`t"); continue }
    $lines = $text -split "`r?`n"
    # RDEF: buffers and variables
    $buffers = @{}; $cur = $null; $regOf = @{}
    foreach ($l in $lines) {
        if ($l -match '^// cbuffer (\S+)') { $cur = $Matches[1]; $buffers[$cur] = @{ bytes = 0; vars = New-Object System.Collections.ArrayList }; continue }
        if ($cur -and $l -match '^//\s+\S+\s+(\S+);\s+// Offset:\s+(\d+) Size:\s+(\d+)\s*(\[unused\])?') { [void]$buffers[$cur].vars.Add([pscustomobject]@{ n = $Matches[1]; o = [int]$Matches[2]; s = [int]$Matches[3]; used = -not $Matches[4] }); continue }
        if ($cur -and $l -match '^// \}') { $cur = $null; continue }
        if ($l -match '^// (\S+)\s+cbuffer\s+NA\s+NA\s+(?:cb)?(\d+)\s+1') { $regOf[$Matches[1]] = [int]$Matches[2] }
        if ($l -match '^// (\S+)\s+cbuffer\s+NA\s+NA\s+(\d+)\s+(\d+)') { $regOf[$Matches[1]] = [int]$Matches[2] }
        if ($l -match '^//\s+(\S+)\s+cbuffer\s+NA\s+NA\s+(\d+)') { $regOf[$Matches[1]] = [int]$Matches[2] }
    }
    # sizes: dcl_constantbuffer cbN[rows]
    $rowsDecl = @{}
    foreach ($l in $lines) { if ($l -match 'dcl_constantbuffer\s+(?:CB|cb)(\d+)\[(\d+)\]') { $rowsDecl[[int]$Matches[1]] = [int]$Matches[2] } }
    $fixed = @{}; $dyn = @{}
    foreach ($l in $lines) {
        if ($l -match '^\s*//' -or $l -match '^\s*dcl_') { continue }
        foreach ($m in [regex]::Matches($l, '(?i)\bcb(\d+)\[([^\]]+)\](?:\.([xyzw]+))?')) {
            $reg = [int]$m.Groups[1].Value; $idx = $m.Groups[2].Value.Trim(); $sw = $m.Groups[3].Value
            if ($idx -match '^\d+$') { $k = "$reg"; if (-not $fixed.ContainsKey($k)) { $fixed[$k] = @{} }; $row = [int]$idx; if (-not $fixed[$k].ContainsKey($row)) { $fixed[$k][$row] = '' }; if ($sw) { $fixed[$k][$row] += $sw } else { $fixed[$k][$row] += 'xyzw' } }
            else { $dyn["$reg"] = $true }
        }
    }
    $stage = if ($f.Extension -eq '.vpo') { 'vs' } else { 'ps' }
    $regs = @($fixed.Keys) + @($dyn.Keys) | Sort-Object { [int]$_ } -Unique
    foreach ($rg in $regs) {
        $name = ($regOf.GetEnumerator() | Where-Object { $_.Value -eq [int]$rg } | Select-Object -First 1).Key
        $rd = ''
        if ($fixed.ContainsKey($rg)) { $rd = (($fixed[$rg].Keys | Sort-Object { [int]$_ } | ForEach-Object { $comp = (($fixed[$rg][$_].ToCharArray() | Sort-Object -Unique) -join ''); "${_}:$comp" }) -join ' ') }
        $used = ''; $declared = ''
        if ($name -and $buffers.ContainsKey($name)) { $used = (($buffers[$name].vars | Where-Object used | ForEach-Object { "$($_.n)@$($_.o):$($_.s)" }) -join ' ') }
        if ($rowsDecl.ContainsKey([int]$rg)) { $declared = 16 * $rowsDecl[[int]$rg] }
        [void]$rows.Add(("{0}`t{1}`t{2}`t{3}`t{4}`t{5}`t{6}`t{7}" -f $f.Name, $stage, $name, $rg, $declared, $rd, [int]($dyn.ContainsKey($rg)), $used))
    }
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
[IO.File]::WriteAllLines($Out, $rows)
"rows $($rows.Count - 1) -> $Out"
