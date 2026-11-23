[CmdletBinding()]
param(
    [string]$MovieDir = 'C:\Program Files (x86)\Steam\steamapps\common\DARK SOULS III\Game\movie',
    [Parameter(Mandatory = $true)][string]$Out
)
# I-0068: the header of each Bink movie the game ships (read-only; only the first 44 bytes of each file are read).
# Bink 1 header ("BIK" + revision): file size - 8, frame count, largest frame, frame count, width, height, frame rate
# numerator and denominator, flags, audio track count. Duration = frames * denominator / numerator.
$ErrorActionPreference = 'Stop'
$rows = foreach ($f in Get-ChildItem -LiteralPath $MovieDir -Filter *.bik | Sort-Object Name) {
    $b = [byte[]]::new(44); $s = [IO.File]::OpenRead($f.FullName)
    try { [void]$s.Read($b, 0, 44) } finally { $s.Close() }
    $u = { param($o) [BitConverter]::ToUInt32($b, $o) }
    $sig = [Text.Encoding]::ASCII.GetString($b, 0, 4)
    $frames = & $u 8; $num = & $u 28; $den = & $u 32
    [string]::Format([Globalization.CultureInfo]::InvariantCulture, (@('{0}','{1}','{2}','{3}','{4}','{5}','{6}','{7}','0x{8:X}','{9}','{10:F3}') -join "`t"), $f.Name, $f.Length, $sig, $frames, (& $u 20), (& $u 24), $num, $den, (& $u 36), (& $u 40), ($frames * $den / $num))
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null
@("file`tbytes`tsignature`tframes`twidth`theight`tfps_num`tfps_den`tflags`taudio_tracks`tduration_s") + $rows | Set-Content -LiteralPath $Out -Encoding utf8
"rows=$(@($rows).Count) -> $Out"
