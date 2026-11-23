# Read-only: disassembles DXBC blobs embedded in a file at given offsets with the system
# d3dcompiler_47.dll (D3DDisassemble), one .asm file per offset.
# Usage: pwsh -NoProfile -File Get-DxbcDisassembly.ps1 -Path <exe> -OutDir <dir> -Offsets <hex,hex,...> [-Names <name,name,...>]
param([Parameter(Mandatory)][string]$Path, [Parameter(Mandatory)][string]$OutDir, [Parameter(Mandatory)][string[]]$Offsets, [string[]]$Names)
$src = @'
using System; using System.Runtime.InteropServices;
public static class Dis {
  [DllImport("d3dcompiler_47.dll")] static extern int D3DDisassemble(byte[] src, UIntPtr size, uint flags, string comments, out IntPtr blob);
  [UnmanagedFunctionPointer(CallingConvention.StdCall)] delegate IntPtr GetPtr(IntPtr self);
  public static string Run(byte[] code) {
    IntPtr blob; int hr = D3DDisassemble(code, (UIntPtr)code.Length, 0, null, out blob);
    if (hr != 0) return "error 0x" + hr.ToString("X");
    IntPtr vt = Marshal.ReadIntPtr(blob);
    var ptr = (GetPtr)Marshal.GetDelegateForFunctionPointer(Marshal.ReadIntPtr(vt, 3 * IntPtr.Size), typeof(GetPtr));
    var size = (GetPtr)Marshal.GetDelegateForFunctionPointer(Marshal.ReadIntPtr(vt, 4 * IntPtr.Size), typeof(GetPtr));
    return Marshal.PtrToStringAnsi(ptr(blob), (int)size(blob));
  }
}
'@
Add-Type -TypeDefinition $src
$bytes = [IO.File]::ReadAllBytes((Resolve-Path $Path).Path)
New-Item -ItemType Directory -Force $OutDir | Out-Null
$all = $Offsets -join ',' -split ','
$nm = if ($Names) { $Names -join ',' -split ',' } else { @() }
for ($k = 0; $k -lt $all.Count; $k++) {
    $off = [Convert]::ToInt64($all[$k], 16); $size = [BitConverter]::ToUInt32($bytes, [int]$off + 24)
    $code = New-Object byte[] $size; [Array]::Copy($bytes, $off, $code, 0, $size)
    $name = if ($k -lt $nm.Count) { $nm[$k] } else { $all[$k] }
    Set-Content -Path (Join-Path $OutDir "$name.asm") -Value ([Dis]::Run($code)) -Encoding utf8
}
"$($all.Count) disassembled"
