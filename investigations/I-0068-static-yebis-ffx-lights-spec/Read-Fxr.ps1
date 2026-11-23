[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Fxr,          # one .fxr file, for example f000030002.fxr of frpg_sfxbnd_commoneffects_effect.ffxbnd
    [string]$Out,                                          # optional output text file (otherwise the annotated dump is returned)
    [string]$SchemaDir = 'C:\Users\User\.codex\I0033-tools\fxr-32.1.0\package\dist',   # actions.json and enums.json of the @cccode/fxr package (field and property names, enum names)
    [switch]$Raw                                           # the unannotated dump (numbers only)
)
# I-0068 part AQ: reads one DS3 FXR3 effect read-only and prints its node tree with, for every action, the field and property
# names of the @cccode/fxr schema (DarkSouls3 structure) and the enum names. Fields equal to 0, -1 or -2 are omitted
# (so an omitted enum field IS the value 0: BlendMode.Unk0, for instance). Values are printed as float|int guesses where
# the type is not known ("f0.5|i1056964608"). Nothing is written but $Out.
$ErrorActionPreference = 'Stop'
if (-not ('FxrDump' -as [type])) { Add-Type -Path (Join-Path $PSScriptRoot 'FxrDump.cs') }
$dump = [FxrDump]::Dump([IO.File]::ReadAllBytes($Fxr))
if ($Raw) { $text = $dump } else {
    $a = Get-Content (Join-Path $SchemaDir 'actions.json') -Raw | ConvertFrom-Json; $by = @{}; foreach ($x in $a) { $by[[int]$x.type] = $x }
    $en = Get-Content (Join-Path $SchemaDir 'enums.json') -Raw | ConvertFrom-Json
    function EnumName($enumName, $value) { $e = $en.$enumName; if (-not $e) { return $null }; foreach ($m in $e.members.PSObject.Properties) { if ([int]$m.Value.value -eq [int]$value) { return $m.Name } } return $null }
    $lines = $dump -split "`r?`n"; $n = $lines.Count; $nodeNo = 0; $res = New-Object System.Collections.ArrayList
    for ($i = 0; $i -lt $n; $i++) { $line = $lines[$i]
        if ($line -match '^(\s*)NODE type=(\d+) configs=(\d+) actions=(\d+) children=(\d+)') { [void]$res.Add("$($Matches[1])NODE #$nodeNo type=$($Matches[2]) children=$($Matches[5])"); $nodeNo++; continue }
        if ($line -match '^(\s*)CONFIG type=(\d+)') { [void]$res.Add("$($Matches[1])CONFIG $($Matches[2])"); continue }
        if ($line -match '^(\s*)ACTION (\d+) fields1=(\d+)') { $ind = $Matches[1]; $t = [int]$Matches[2]; if ($t -eq 0) { continue }
            $x = $by[$t]; $nm = if ($x) { $x.name } else { '?' }; $s = if ($x) { $x.structure.DarkSouls3 } else { $null }
            [void]$res.Add("$ind$t $nm")
            $j = $i + 1; while ($j -lt $n -and $lines[$j] -notmatch '^\s*(NODE|CONFIG|ACTION) ') { $l = $lines[$j].Trim()
                if ($l -match '^F([12]) \[(.*)\]$') { $g1 = $Matches[1]; $vals = $Matches[2] -split ', '; $names = if ($s) { if ($g1 -eq '1') { $s.fields1 } else { $s.fields2 } } else { @() }
                    $k = 0; $o = @(); foreach ($v in $vals) { $nmn = if ($k -lt $names.Count) { $names[$k] } else { "#$k" }
                        if ($v -ne '' -and $v -ne '0' -and $v -ne 'i-1' -and $v -ne 'i-2') { $disp = $v -replace '\|i-?\d+', ''
                            if ($x -and $x.properties.$nmn -and $x.properties.$nmn.enum -and $v -match '^i(-?\d+)$') { $e2 = EnumName $x.properties.$nmn.enum $Matches[1]; if ($e2) { $disp = "$v=$e2" } }
                            $o += "$nmn=$disp" }
                        $k++ }
                    if ($o.Count) { [void]$res.Add("$ind  F${g1}: " + ($o -join ' ')) } }
                elseif ($l -match '^P([12])\.(\d+) (.*)$') { $names = if ($s) { if ($Matches[1] -eq '1') { $s.properties1 } else { $s.properties2 } } else { @() }; $idx = [int]$Matches[2]; $nmn = if ($idx -lt $names.Count) { $names[$idx] } else { "#$idx" }
                    if ($l -notmatch 'fn=(Zero|One) \[\]$' -or $l -match 'mods=') { [void]$res.Add("$ind  P$($Matches[1]) ${nmn}: $($Matches[3])") } }
                elseif ($l -match '^MOD') { [void]$res.Add("$ind      $l") }
                $j++ } } }
    $text = $res -join "`n"
}
if ($Out) { New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Out) | Out-Null; [IO.File]::WriteAllText($Out, $text); "$Fxr -> $Out" } else { $text }
