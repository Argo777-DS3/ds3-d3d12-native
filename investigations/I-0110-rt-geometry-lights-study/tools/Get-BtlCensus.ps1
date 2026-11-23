[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Dir,   # recovered *.btl.dcx (Recover-NamedGameData.ps1 of I-0068)
    [Parameter(Mandatory = $true)][string]$Out    # TSV, one row per light
)
# I-0110: every light of every light bank (BTL), read with the pinned SoulsFormats 1.0.1, plus the raw record words the
# library leaves unnamed (numbers in the invariant culture, vector parts separated by ;, tabs and line breaks in names
# replaced by spaces). Read-only; the game is not started; nothing is written but $Out.
$ErrorActionPreference = 'Stop'
Add-Type -Path 'C:\Users\User\.codex\tools\SoulsFormats-1.0.1\lib\net462\SoulsFormats.dll'
$t = [SoulsFormats.BTL+Light]
$fields = $t.GetFields('Public,Instance') | Sort-Object Name
$rows = New-Object System.Collections.ArrayList
[void]$rows.Add((@('file','index') + ($fields | ForEach-Object Name)) -join "`t")
$inv = [Globalization.CultureInfo]::InvariantCulture
function Format-Value($v) {
    if ($null -eq $v) { return '' }
    if ($v -is [System.Numerics.Vector3]) { return [string]::Format($inv, '{0:G6};{1:G6};{2:G6}', $v.X, $v.Y, $v.Z) }
    if ($v -is [System.Drawing.Color]) { return ('{0};{1};{2};{3}' -f $v.R, $v.G, $v.B, $v.A) }
    if ($v -is [byte[]]) { return (($v | ForEach-Object { $_.ToString('X2') }) -join '') }
    if ($v -is [array]) { return (($v | ForEach-Object { [string]::Format($inv, '{0}', $_) }) -join ';') }
    if ($v -is [string]) { return ($v -replace "[`t`r`n]", ' ') }
    return [string]::Format($inv, '{0}', $v)
}
foreach ($f in Get-ChildItem -LiteralPath $Dir -Filter *.btl.dcx | Sort-Object Name) {
    $btl = [SoulsFormats.BTL]::Read($f.FullName)
    for ($i = 0; $i -lt $btl.Lights.Count; $i++) {
        $l = $btl.Lights[$i]
        [void]$rows.Add((@(($f.Name -replace '\.btl\.dcx$', ''), $i) + ($fields | ForEach-Object { Format-Value ($_.GetValue($l)) })) -join "`t")
    }
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
[IO.File]::WriteAllLines($Out, $rows)
"lights $($rows.Count - 1) -> $Out"
