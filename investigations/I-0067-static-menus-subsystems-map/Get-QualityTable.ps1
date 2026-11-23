[CmdletBinding(PositionalBinding = $false)]
param(
    [Parameter(Mandatory = $true)][string]$Asm,          # <entry>.asm.txt of FUN_1400219B0 (I0067Map output)
    [Parameter(Mandatory = $true)][string]$Out           # output .tsv: offset, 16 hex bytes per row
)
# I-0067: evaluates the straight-line body of FUN_1400219B0 (GX quality-settings table, 0x24C bytes) without running the game.
# It interprets only the instruction forms the function uses (MOV/MOVZX/MOVSS/MOVAPS/LEA/ADD/OR/AND/XOR, one memcpy call),
# on a flat byte memory: stack frame at 0x100000, output buffer at 0x200000. The three constant loads come from
# .rdata (read once with I0067Map x:) and are listed in $rdata. No game process, no network.
$ErrorActionPreference = 'Stop'
$mem = @{}
function WriteBytes([long]$addr, [byte[]]$b) { for ($i = 0; $i -lt $b.Length; $i++) { $script:mem[$addr + $i] = $b[$i] } }
function ReadBytes([long]$addr, [int]$n) { $r = New-Object byte[] $n; for ($i = 0; $i -lt $n; $i++) { if ($script:mem.ContainsKey($addr + $i)) { $r[$i] = $script:mem[$addr + $i] } }; return ,$r }
$rdata = @{
    0x143d7ee7c = [byte[]](0x00,0x00,0x00,0x40)
    0x143d7ee38 = [byte[]](0x00,0x00,0xc0,0x3f)
    0x143d80050 = [byte[]](0x00,0x00,0x00,0x3f,0x00,0x00,0x00,0x3f,0x00,0x00,0x00,0x3f,0x00,0x00,0xc0,0x3f)
    0x143d80060 = [byte[]](0x00,0x00,0x40,0x3f,0x00,0x00,0x40,0x3f,0x00,0x00,0x40,0x3f,0x00,0x00,0xc0,0x3f)
    0x143d80030 = [byte[]](0x66,0x66,0x66,0x3f,0x66,0x66,0x66,0x3f,0x66,0x66,0x66,0x3f,0x9a,0x99,0x99,0x3f)
    0x143d7fef0 = [byte[]](0x00,0x00,0x80,0x3f,0x00,0x00,0x80,0x3f,0x00,0x00,0x80,0x3f,0x00,0x00,0x80,0x3f)
}
$reg = @{ RAX = 0L; RCX = 0x200000L; RDX = 0L; RBX = 0L; R8 = 0L; RBP = 0L; RSP = 0x100008L }
$xmm = @{ XMM0 = (New-Object byte[] 16); XMM1 = (New-Object byte[] 16) }
$obuf = 0x200000L
function RegKey([string]$n) {
    $n = $n.ToUpper()
    switch -Regex ($n) {
        '^(RAX|EAX|AX|AL)$' { return 'RAX' } '^(RCX|ECX|CX|CL)$' { return 'RCX' } '^(RDX|EDX|DX|DL)$' { return 'RDX' }
        '^(RBX|EBX|BX|BL)$' { return 'RBX' } '^(R8|R8D|R8W|R8B)$' { return 'R8' } '^(RBP)$' { return 'RBP' } '^(RSP)$' { return 'RSP' }
        default { return $null }
    }
}
function RegSize([string]$n) {
    $n = $n.ToUpper()
    if ($n -match '^R(AX|CX|DX|BX|BP|SP|8)$') { return 8 } if ($n -match '^(E(AX|CX|DX|BX)|R8D)$') { return 4 } if ($n -match '^((AX|CX|DX|BX)|R8W)$') { return 2 } return 1
}
function EA([string]$m) {
    # [RBP + 0xa8] / [RBP + -0x2c] / [RCX] / [RCX + 0x8]
    $m = $m.Trim('[', ']').Replace(' ', '')
    if ($m -match '^(R[A-Z0-9]+)(?:\+(-?)(0x[0-9a-fA-F]+))?$') {
        $b = $reg[(RegKey $matches[1])]; $d = 0L
        if ($matches[3]) { $d = [Convert]::ToInt64($matches[3].Substring(2), 16); if ($matches[2] -eq '-') { $d = -$d } }
        return $b + $d
    }
    if ($m -match '^0x[0-9a-fA-F]+$') { return [Convert]::ToInt64($m.Substring(2), 16) }
    throw "address form $m"
}
function ImmVal([string]$s) { $s = $s.Trim(); if ($s -match '^-?0x') { $neg = $s.StartsWith('-'); $v = [Convert]::ToUInt64($s.TrimStart('-').Substring(2), 16); if ($neg) { return -[int64]$v } return [int64]$v } return [int64]$s }
function ToBytes([int64]$v, [int]$n) { $b = New-Object byte[] $n; for ($i = 0; $i -lt $n; $i++) { $b[$i] = [byte](($v -shr (8 * $i)) -band 0xff) }; return ,$b }
function FromBytes([byte[]]$b) { $p = New-Object byte[] 8; [Array]::Copy($b, $p, [Math]::Min(8, $b.Length)); return [BitConverter]::ToInt64($p, 0) }
function Size([string]$p) { switch -Regex ($p) { '^byte' { 1 } '^word' { 2 } '^dword' { 4 } '^qword' { 8 } '^xmmword' { 16 } default { 0 } } }

$lines = Get-Content -LiteralPath $Asm
# frame: entry RSP = 0x100008 (call pushed the return address), PUSH RBP, LEA RBP,[RSP-0x170], SUB RSP,0x270
foreach ($l in $lines) {
    if ($l -notmatch '^([0-9a-f]{9})\s+(\S+)\s*(.*)$') { continue }
    $addr = [Convert]::ToInt64($matches[1], 16); $mn = $matches[2].ToUpper(); $ops = $matches[3]
    if ($addr -eq 0x140021f7a) { break }   # the 0xA0-byte block copy follows; handled below
    $o = $ops -split ',(?![^\[]*\])'   # split on commas outside brackets
    switch ($mn) {
        'PUSH' { $reg['RSP'] -= 8 }
        'LEA' { $d = $o[0].Trim(); $reg[(RegKey $d)] = (EA ($o[1] -replace '^.*?(\[.*\])', '$1')) }
        'SUB' { if ($o[0].Trim() -eq 'RSP') { $reg['RSP'] -= (ImmVal $o[1]) } }
        'ADD' { $d = $o[0].Trim(); $k = RegKey $d; $reg[$k] += (ImmVal $o[1]) }
        'XOR' { $d = $o[0].Trim(); $reg[(RegKey $d)] = 0L }
        'NOP' { }
        'MOV' {
            $d = $o[0].Trim(); $s = $o[1].Trim()
            if ($d -match '^(byte|word|dword|qword) ptr (\[.*\])$') {
                $n = Size $matches[1]; $a = EA $matches[2]
                if ($s -match '^-?(0x)?[0-9a-fA-F]+$' -and $s -notmatch '^(AL|AX|EAX|R8B|R8W|R8D)$') { $v = ImmVal $s } else { $v = $reg[(RegKey $s)] }
                WriteBytes $a (ToBytes $v $n)
            } elseif ($d -match '^(R[A-Z0-9]+|E[A-Z]+|[A-D]L|R8[DWB])$') {
                if ($s -match '^(byte|word|dword|qword) ptr (\[.*\])$') {
                    $n = Size $matches[1]; $v = FromBytes (ReadBytes (EA $matches[2]) $n); $reg[(RegKey $d)] = $v
                } elseif ($s -match '^-?(0x)?[0-9a-fA-F]+$' -and $null -eq (RegKey $s)) { $reg[(RegKey $d)] = ImmVal $s } else { $reg[(RegKey $d)] = $reg[(RegKey $s)] }
            }
        }
        'MOVZX' {
            $d = $o[0].Trim(); $s = $o[1].Trim()
            if ($s -match '^(byte|word) ptr (\[.*\])$') { $reg[(RegKey $d)] = FromBytes (ReadBytes (EA $matches[2]) (Size $matches[1])) }
        }
        'MOVSS' {
            $d = $o[0].Trim(); $s = $o[1].Trim()
            if ($d -match '^XMM\d$') {
                $a = if ($s -match '\[(0x[0-9a-fA-F]+)\]') { [Convert]::ToInt64($matches[1].Substring(2), 16) } else { throw "movss src $s" }
                $x = New-Object byte[] 16; [Array]::Copy($rdata[$a], $x, 4); $xmm[$d.ToUpper()] = $x
            } elseif ($d -match '^dword ptr (\[.*\])$') { $a = EA $matches[1]; WriteBytes $a ([byte[]]($xmm[$s.ToUpper()][0..3])) }
        }
        'MOVAPS' {
            $d = $o[0].Trim(); $s = $o[1].Trim()
            if ($d -match '^XMM\d$') {
                if ($s -match '\[(0x[0-9a-fA-F]+)\]') { $a = [Convert]::ToInt64($matches[1].Substring(2), 16); $xmm[$d.ToUpper()] = [byte[]]$rdata[$a] }
                else { throw "movaps src $s" }
            } elseif ($d -match '^xmmword ptr (\[.*\])$') { $a = EA $matches[1]; WriteBytes $a $xmm[$s.ToUpper()] }
        }
        default { throw "unsupported $mn at $($matches[1])" }
    }
}
# the block copy (140021f7a..140021ff9): memcpy(RCX, RBP+0xD0, 0xA0) with RCX already advanced by 0x168
$src = $reg['RBP'] + 0xd0; $dst = $reg['RCX']
WriteBytes $dst (ReadBytes $src 0xa0)
# the tail 140022004..: resume after the copy
$tail = $false
foreach ($l in $lines) {
    if ($l -notmatch '^([0-9a-f]{9})\s+(\S+)\s*(.*)$') { continue }
    $addr = [Convert]::ToInt64($matches[1], 16); $mn = $matches[2].ToUpper(); $ops = $matches[3]
    if ($addr -lt 0x140022004) { continue }
    $o = $ops -split ',(?![^\[]*\])'
    $reg['RBX'] = $obuf
    switch ($mn) {
        'MOV' {
            $d = $o[0].Trim(); $s = $o[1].Trim()
            if ($d -match '^(byte|word|dword|qword) ptr (\[.*\])$') {
                $n = Size $matches[1]; $a = EA $matches[2]
                if ($s -match '^-?(0x)?[0-9a-fA-F]+$' -and $s -notmatch '^(AL|AX|EAX|R8B|R8W|R8D|RAX)$') { $v = ImmVal $s } else { $v = $reg[(RegKey $s)] }
                WriteBytes $a (ToBytes $v $n)
            } elseif ($s -match '^(byte|word|dword|qword) ptr (\[.*\])$') { $reg[(RegKey $d)] = FromBytes (ReadBytes (EA $matches[2]) (Size $matches[1])) }
        }
        'MOVZX' { $s = $o[1].Trim(); if ($s -match '^(byte|word) ptr (\[.*\])$') { $reg[(RegKey $o[0].Trim())] = FromBytes (ReadBytes (EA $matches[2]) (Size $matches[1])) } }
        default { }
    }
}
$sb = New-Object System.Text.StringBuilder
for ($r = 0; $r -lt 0x24c; $r += 16) {
    $n = [Math]::Min(16, 0x24c - $r); $b = ReadBytes ($obuf + $r) $n
    [void]$sb.AppendLine(('{0:x3}' -f $r) + "`t" + (($b | ForEach-Object { '{0:x2}' -f $_ }) -join ' '))
}
[IO.File]::WriteAllText($Out, $sb.ToString())
Write-Output "ok $Out"
