[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Path,          # a DXBC file (.vpo/.ppo/.dpo/.hpo of the recovered shader banks)
    [ValidateSet('Interface', 'Disassembly', 'Cb')][string]$Mode = 'Interface',
    [int]$Register = -1                                      # Mode Cb: the constant buffer register whose instructions are listed (for example 7)
)
# I-0068 part AP: a read-only reader of one compiled shader. 'Interface' prints the constant buffers with their variables
# (RDEF), the input signature (ISGN) and the instruction-level "unused" flags of the Microsoft disassembler; 'Disassembly' prints
# the full listing; 'Cb' prints the instructions that read constant buffer <Register>. Nothing is written. Uses the system
# d3dcompiler_47.dll (the same disassembler the native read analysis uses, native/world/cb_reads.cpp).
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
$bytes = [IO.File]::ReadAllBytes($Path)   # ReadAllBytes: bracketed names such as GXFlver_X_GBuf@[cl].vpo must not be treated as wildcards
$text = [D3DC2]::Disasm($bytes)
switch ($Mode) {
    'Disassembly' { $text }
    'Interface' {
        $lines = $text -split "`n"
        $lines | Where-Object { $_ -match '^//' -or $_ -match 'dcl_(constantbuffer|input|output)' } | ForEach-Object { $_.TrimEnd() }
    }
    'Cb' {
        if ($Register -lt 0) { throw 'Mode Cb needs -Register' }
        ($text -split "`n") | Where-Object { $_ -match "cb$Register\[" -and $_ -notmatch '^\s*//' } | ForEach-Object { $_.Trim() }
    }
}
