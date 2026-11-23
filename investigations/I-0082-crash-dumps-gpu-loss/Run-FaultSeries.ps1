param([Parameter(Mandatory)][string]$Build,[string]$Prefix='i82',[string]$Only='',[string]$NotBefore='')
# I-0082 (DESIGN 5): the fault paths proved one run each, at the title screen (no pilot, no save loaded): a plain start without
# the debug layer, a start with it (debug-layer.flag, its own pipeline library), then the injected crash, removed device and GPU
# hang at the 1,200th presented frame. Each run goes through Run-StartupTrace.ps1 (offline, saves restored by hash); its
# controller row, crash folder and journal rows are summarised after it. -Only: a comma list of run keys (plain, debug, crash,
# removed, hang). -NotBefore HH:mm (the PC's local time) holds the first launch until the announced start.
$ErrorActionPreference='Continue'
if($NotBefore){ $at=[datetime]::ParseExact($NotBefore,'HH:mm',$null); while((Get-Date) -lt $at){ Start-Sleep 5 } }
$inv=Join-Path (Split-Path -Parent $PSScriptRoot) 'I-0066-independent-native-startup'
$root=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$runs=@(
    @{key='plain';flags='';debug=$false},
    @{key='debug';flags='';debug=$true},
    @{key='crash';flags='test-crash';debug=$false},
    @{key='removed';flags='test-device-removed';debug=$false},
    @{key='hang';flags='test-gpu-hang';debug=$false})
if($Only){$keys=$Only -split ',';$runs=@($runs | Where-Object {$_.key -in $keys})}
foreach($run in $runs){
    $name="$Prefix-$($run.key)";"== $name $(Get-Date -Format HH:mm:ss)"
    $native=if($run.debug){@{}}else{@{NoDebugLayer=$true;Measure=$true}}
    & "$inv/Run-StartupTrace.ps1" -RunName $name -Seconds 90 -SteamLaunch -BootstrapMode Native -BuildFolder $Build -EffectFlags $run.flags @native 2>&1 | Select-Object -Last 2
    $out=Join-Path $root "evidence/I-0066/artifacts/$name"
    if(Test-Path "$out/controller.jsonl"){ Get-Content "$out/controller.jsonl" | Select-Object -Last 1 }
    if(Test-Path "$out/startup.jsonl"){ Select-String -LiteralPath "$out/startup.jsonl" -Pattern '"native_debug_layer"|"native_fault_test"|"native_crash_dump"|"native_device_removed"|"native_dred|"native_failure"|"refusal"|"native_pipeline_library"|"native_pipeline_list"' |
        ForEach-Object { $_.Line.Substring(0,[Math]::Min(400,$_.Line.Length)) } }
    if(Test-Path "$out/crashes"){ Get-ChildItem "$out/crashes" | ForEach-Object { "  $($_.Name) $($_.Length)" }
        Get-ChildItem "$out/crashes" -Filter *.txt | ForEach-Object { Get-Content $_.FullName -TotalCount 24 } }
    $t=(Get-Date).AddMinutes(3); while((Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) -and (Get-Date) -lt $t){ Start-Sleep 3 }; Start-Sleep 5
}
"== series end $(Get-Date -Format HH:mm:ss)"
