param([Parameter(Mandatory)][string]$Path, [int]$ScanWords = 4000, [switch]$AllThreads, [string]$Dump, [switch]$Ranges, [switch]$ThreadInfo)
$ErrorActionPreference = 'Stop'
$b = [IO.File]::ReadAllBytes($Path)
function U32($o) { [BitConverter]::ToUInt32($b, [int]$o) }
function U64($o) { [BitConverter]::ToUInt64($b, [int]$o) }
function U16($o) { [BitConverter]::ToUInt16($b, [int]$o) }
if ((U32 0) -ne 0x504D444D) { throw 'not a minidump' }
$n = U32 8; $dir = U32 12
$streams = @{}
for ($i = 0; $i -lt $n; $i++) { $e = $dir + 12 * $i; $t = U32 $e; $streams[[int]$t] = @{ Size = (U32 ($e + 4)); Rva = (U32 ($e + 8)) } }
"streams: " + (($streams.Keys | Sort-Object) -join ',')
function MdString($rva) { $len = U32 $rva; [Text.Encoding]::Unicode.GetString($b, [int]$rva + 4, [int]$len) }
# modules
$mods = @()
if ($streams.ContainsKey(4)) { $r = $streams[4].Rva; $c = U32 $r
  for ($i = 0; $i -lt $c; $i++) { $m = $r + 4 + 108 * $i
    $mods += [pscustomobject]@{ Base = (U64 $m); Size = (U32 ($m + 8)); Name = [IO.Path]::GetFileName((MdString (U32 ($m + 20)))) } } }
function Sym([uint64]$a) { foreach ($m in $mods) { if ($a -ge $m.Base -and $a -lt ($m.Base + $m.Size)) { return ('{0}+0x{1:X}' -f $m.Name, ($a - $m.Base)) } }; return $null }
# memory ranges
$mem = New-Object System.Collections.Generic.List[object]
if ($streams.ContainsKey(9)) { $r = $streams[9].Rva; $c = U64 $r; $baseRva = U64 ($r + 8); $off = $baseRva
  for ($i = 0; $i -lt $c; $i++) { $d = $r + 16 + 16 * $i; $st = U64 $d; $sz = U64 ($d + 8); $mem.Add([pscustomobject]@{ Start = $st; Size = $sz; Rva = $off }); $off += $sz } }
if ($streams.ContainsKey(5)) { $r = $streams[5].Rva; $c = U32 $r
  for ($i = 0; $i -lt $c; $i++) { $d = $r + 4 + 16 * $i; $mem.Add([pscustomobject]@{ Start = (U64 $d); Size = [uint64](U32 ($d + 8)); Rva = [uint64](U32 ($d + 12)) }) } }
function ReadVa([uint64]$va) { foreach ($m in $mem) { if ($va -ge $m.Start -and $va + 8 -le $m.Start + $m.Size) { return (U64 ($m.Rva + ($va - $m.Start))) } }; return $null }
"memory ranges: $($mem.Count), bytes " + (($mem | Measure-Object Size -Sum).Sum)
# threads
$threads = @{}
if ($streams.ContainsKey(3)) { $r = $streams[3].Rva; $c = U32 $r
  for ($i = 0; $i -lt $c; $i++) { $t = $r + 4 + 48 * $i
    $threads[[uint32](U32 $t)] = [pscustomobject]@{ Id = (U32 $t); StackStart = (U64 ($t + 24)); StackSize = (U32 ($t + 32)); CtxSize = (U32 ($t + 40)); CtxRva = (U32 ($t + 44)) } } }
"threads in dump: $($threads.Count)"
function CtxRegs($rva) { [pscustomobject]@{ Rip = (U64 ($rva + 0xF8)); Rsp = (U64 ($rva + 0x98)); Rbp = (U64 ($rva + 0xA0)); Rcx = (U64 ($rva + 0x80)); Rdx = (U64 ($rva + 0x88)) } }
function ScanStack([uint64]$rsp, [int]$words) {
  $hits = @(); for ($k = 0; $k -lt $words; $k++) { $v = ReadVa ($rsp + 8 * $k); if ($null -eq $v) { if ($k -eq 0) { return ,@('(no stack memory at rsp)') }; break }; $s = Sym $v; if ($s) { $hits += ('  [rsp+0x{0:X}] {1}' -f (8 * $k), $s) } }; return ,$hits }
# exception
if ($streams.ContainsKey(6)) { $r = $streams[6].Rva; $tid = U32 $r; $er = $r + 8
  $code = U32 $er; $addr = U64 ($er + 16); $np = U32 ($er + 24)
  $ps = @(); for ($i = 0; $i -lt $np; $i++) { $ps += ('0x{0:X}' -f (U64 ($er + 32 + 8 * $i))) }
  $ctxRva = U32 ($r + 8 + 152 + 4)
  ('exception thread {0} code 0x{1:X8} at 0x{2:X} ({3}) params [{4}]' -f $tid, $code, $addr, (Sym $addr), ($ps -join ', '))
  "exception thread present in thread list: $($threads.ContainsKey([uint32]$tid))"
  $c = CtxRegs $ctxRva
  ('context rip 0x{0:X} ({1}) rsp 0x{2:X} rcx 0x{3:X} rdx 0x{4:X}' -f $c.Rip, (Sym $c.Rip), $c.Rsp, $c.Rcx, $c.Rdx)
  'stack scan of the exception thread (values that fall in a module):'
  ScanStack $c.Rsp $ScanWords | Select-Object -First 80
}
'threads (rip):'
foreach ($t in $threads.Values) { $c = CtxRegs $t.CtxRva; ('  {0,6} rip {1}  stack 0x{2:X}+0x{3:X}' -f $t.Id, (Sym $c.Rip), $t.StackStart, $t.StackSize)
  if ($AllThreads) { ScanStack $c.Rsp 600 | Select-Object -First 25 } }
'modules:'; $mods | ForEach-Object { '  0x{0:X} 0x{1:X} {2}' -f $_.Base, $_.Size, $_.Name }

if ($Dump) { $va = [uint64]$Dump; 'qwords at 0x{0:X}:' -f $va
  for ($k = 0; $k -lt 64; $k++) { $v = ReadVa ($va + 8 * $k); if ($null -eq $v) { '  +0x{0:X3} (absent)' -f (8*$k); break }; '  +0x{0:X3} 0x{1:X16} {2}' -f (8 * $k), $v, (Sym $v) } }
if ($Ranges) { 'ranges:'; $mem | Sort-Object Start | ForEach-Object { '  0x{0:X} +0x{1:X} {2}' -f $_.Start, $_.Size, (Sym $_.Start) } }

if ($ThreadInfo -and $streams.ContainsKey(17)) { $r = $streams[17].Rva; $hs = U32 $r; $es = U32 ($r + 4); $c = U32 ($r + 8)
  $exTid = if ($streams.ContainsKey(6)) { U32 $streams[6].Rva } else { 0 }
  'thread info (id, flags, exit status, created, exited, start):'
  $rows = for ($i = 0; $i -lt $c; $i++) { $e = $r + $hs + $es * $i
    $ct = U64 ($e + 16); $xt = U64 ($e + 24)
    [pscustomobject]@{ Id = (U32 $e); Flags = ('0x{0:X}' -f (U32 ($e + 4))); Exit = ('0x{0:X}' -f (U32 ($e + 12)));
      Created = $(if ($ct) { [DateTime]::FromFileTimeUtc([int64]$ct).ToString('HH:mm:ss.fff') } else { '' });
      Exited = $(if ($xt) { [DateTime]::FromFileTimeUtc([int64]$xt).ToString('HH:mm:ss.fff') } else { '' });
      Start = (Sym (U64 ($e + 48))); Mark = $(if ((U32 $e) -eq $exTid) { '<== fault' } else { '' }) } }
  $rows | Sort-Object Created | Format-Table -AutoSize | Out-String -Width 220 }

