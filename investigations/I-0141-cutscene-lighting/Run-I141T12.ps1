# I-0141 tranche 12's slot (the ASUS, offline, restored by hash, no measure), one series in this order:
#  1-2. the ceremony scene 32000000 natively then in the original, Run-I141SmokePix -Dump -Light -ArmMs <ms> -Life: the light press
#       made by the module at the same moment of the scene's own clock in both modes, and every effect entity's life;
#  3.   Moonlight Vortex (Run-I140Spells -Families thomas-arts) natively then in the original, both filmed, to pair the streak and
#       the green ground at the same moment (I-0140's e89 pair had no film of the original left).
# The log is $Log; its last line « SERIES-END » is written in a finally block, so a waiter never hangs.
param([Parameter(Mandatory)][string]$Build,[int]$ArmMs=19000,[Parameter(Mandatory)][string]$Log,[switch]$NoSpells,[Parameter(Mandatory)][ValidatePattern('^[a-z0-9]+$')][string]$Tag,[ValidatePattern('^i141-t[0-9]+$')][string]$Prefix='i141-t12')
$ErrorActionPreference='Stop'
$here=$PSScriptRoot;$repo=Split-Path -Parent (Split-Path -Parent $here)
try{
    if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){throw 'DarkSoulsIII already runs'}
    # Every run folder checked before the first run (the step review of ae14b1dcc, M1): Run-I140Spells does not refuse an existing one.
    $names=@('nat','orig'|ForEach-Object{"$Prefix-$Tag-c32000000-$_"});if(-not $NoSpells){$names+=@('nat','orig'|ForEach-Object{"$Prefix-$Tag-mv-thomas-arts-$_"})}
    foreach($n in $names){if(Test-Path -LiteralPath "$repo/evidence/I-0066/artifacts/$n"){throw "run $n exists: give another -Tag"}}
    foreach($mode in 'nat','orig'){
        $smoke=@{Build=$Build;RunName="$Prefix-$Tag-c32000000-$mode";Dump=$true;Light=$true;ArmMs=$ArmMs;Life=$true;Log=$Log}
        if($mode -eq 'orig'){$smoke.Original=$true}
        $out=& "$here/Run-I141SmokePix.ps1" @smoke;if("$out" -notmatch 'EXIT 0\b'){throw "smoke $mode did not end with EXIT 0: the series stops"} }
    if(-not $NoSpells){
        & "$repo/investigations/I-0140-effect-families/Run-I140Spells.ps1" -Prefix "$Prefix-$Tag-mv" -BuildFolder $Build -Families 'thomas-arts' -Modes 'Native','Reference' *>&1 | ForEach-Object { "$_" | Add-Content $Log } }
}catch{ "THROW $_" | Add-Content $Log }
finally{ "SERIES-END $(Get-Date -Format HH:mm:ss)" | Add-Content $Log }
