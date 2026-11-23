[CmdletBinding(PositionalBinding = $false)]
param(
    [string]$Dir = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'evidence\I-0067\queries\phantom'),
    [string]$Out = ''
)
# I-0067: decodes PhantomParam (102 rows of 0x30 bytes) and RoleParam (42 rows of 0x80 bytes) from the raw dword dumps of
# Get-ParamRows.ps1 into the quantities the renderer uses, with the layout read from the row accessors FUN_140E35F30..FUN_140E363E0:
#   +0x00/+0x04/+0x08/+0x0C/+0x10 float weights a of edge, front, diffuse, specular, light
#   +0x14..16 edge rgb, +0x17..19 front rgb, +0x1A..1C diffuse rgb, +0x1D..1F specular rgb, +0x20..22 light rgb (bytes, /255)
#   +0x24 fade float, +0x28 float (no accessor found), +0x2C kind byte (0..3), +0x2D edge-subtract byte, +0x2E front-subtract byte
# and FUN_14022E930's cbMatDynParam rows 26..30 (+0x1A0..+0x1EF): colour = (r*r*a*s, g*g*a*s, b*b*a*s, a), s = -1 for subtract on edge and front only.
$ErrorActionPreference = 'Stop'
[Threading.Thread]::CurrentThread.CurrentCulture = [Globalization.CultureInfo]::InvariantCulture
function Read-Rows([string]$file, [int]$size) {
    $rows = @{}
    foreach ($l in Get-Content -LiteralPath (Join-Path $Dir $file) -Encoding utf8) {
        if ($l.StartsWith('#') -or $l.StartsWith('id')) { continue }
        $p = $l -split "`t"; $id = [int]$p[0]; $o = [Convert]::ToInt32($p[1], 16); $v = [Convert]::ToUInt32($p[2], 16)
        if (-not $rows.ContainsKey($id)) { $rows[$id] = New-Object byte[] $size }
        [Array]::Copy([BitConverter]::GetBytes($v), 0, $rows[$id], $o, 4)
    }
    $rows
}
function Col([byte[]]$b, [int]$wo, [int]$co, [double]$s) {
    $a = [double][BitConverter]::ToSingle($b, $wo); $c = 0..2 | ForEach-Object { $b[$co + $_] / 255.0 }
    '({0:0.###} {1:0.###} {2:0.###} {3:0.###})' -f ($c[0]*$c[0]*$a*$s), ($c[1]*$c[1]*$a*$s), ($c[2]*$c[2]*$a*$s), $a
}
$ph = Read-Rows 'phantomparam.tsv' 0x30
$lines = @(('id','kind','fade','f28','edgeSub','frontSub','Edge(b2+416)','Front(432)','Diff(448)','Spec(464)','Light(480)') -join "`t")
foreach ($id in ($ph.Keys | Sort-Object)) {
    $b = $ph[$id]; $es = if ($b[0x2D]) { -1.0 } else { 1.0 }; $fs = if ($b[0x2E]) { -1.0 } else { 1.0 }
    $lines += (@($id, $b[0x2C], ('{0:0.###}' -f [BitConverter]::ToSingle($b, 0x24)), ('{0:0.###}' -f [BitConverter]::ToSingle($b, 0x28)), $b[0x2D], $b[0x2E],
        (Col $b 0x00 0x14 $es), (Col $b 0x04 0x17 $fs), (Col $b 0x08 0x1A 1), (Col $b 0x0C 0x1D 1), (Col $b 0x10 0x20 1)) -join "`t")
}
$ro = Read-Rows 'roleparam.tsv' 0x80
$rl = @(('role','phantom(+4)','byRank(+4C/50/54)','signSfx(+30)','pairSfx(+34)','+38','+3C','+40','+44','+48','+68') -join "`t")
foreach ($id in ($ro.Keys | Sort-Object)) {
    $b = $ro[$id]; $i = { param($o) [BitConverter]::ToInt32($b, $o) }
    $rl += (@($id, (& $i 4), ('{0}/{1}/{2}' -f (& $i 0x4C), (& $i 0x50), (& $i 0x54)), (& $i 0x30), (& $i 0x34), (& $i 0x38), (& $i 0x3C), (& $i 0x40), (& $i 0x44), (& $i 0x48), (& $i 0x68)) -join "`t")
}
if ($Out) { $lines | Set-Content (Join-Path $Out 'phantomparam-decoded.tsv') -Encoding utf8; $rl | Set-Content (Join-Path $Out 'roleparam-decoded.tsv') -Encoding utf8 }
$lines; ''; $rl
