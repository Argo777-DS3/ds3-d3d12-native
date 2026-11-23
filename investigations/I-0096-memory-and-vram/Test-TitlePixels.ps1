param([Parameter(Mandatory)][string[]]$Builds,[ValidateRange(1,6)][int]$Rounds=2,[string]$Prefix='i96-pix',[ValidateRange(20,120)][int]$Seconds=60,[string]$NotBefore='')
# I-0096: what does the title show at the network notice's test pixels (x 546, y 418/422/426 of a 1280x720 client at (8,31),
# Pilot-TitleContinue's Orange 418)? Announced live runs (D-0116), no input at all: each build is launched in turn on the
# Shebang zone save, the screen is sampled every 300 ms for -Seconds from the journal's start, every sample that passes the
# pilot's test is saved (screen shot) and counted, then the window is closed and the restoration awaited.
$ErrorActionPreference='Stop'
$repo=Split-Path -Parent (Split-Path -Parent $PSScriptRoot);$inv="$repo/investigations/I-0066-independent-native-startup"
$save='C:/Users/User/.ds3-i0039-backups/shebang-zones-final-20261013-045344/DS30000.sl2';$hash='78948904F13ADD2F1AC79015F9F01F171D5B4F0F01A0A716CE12F884B94265B8'
if($NotBefore){ $at=[datetime]::ParseExact($NotBefore,'HH:mm',$null); while((Get-Date) -lt $at){ Start-Sleep 5 } }
Add-Type -AssemblyName System.Drawing
function Alive{ [bool](Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) }
function Orange([Drawing.Bitmap]$b,[int]$y){ $hits=0; foreach($dy in 0,4,8){ $c=$b.GetPixel(546,$y+$dy); if($c.R -ge 60 -and $c.R -le 140 -and $c.R -gt $c.G+15 -and $c.G -gt $c.B+10){ $hits++ } }; $hits -ge 2 }
$n=0
for($round=1;$round -le $Rounds;$round++){ foreach($build in $Builds){ $n++
    $run="$Prefix-$n";$o="$repo/evidence/I-0066/artifacts/$run"
    $job=Start-Job -ScriptBlock { param($inv,$run,$save,$hash,$build)
        pwsh -NoProfile -File "$inv/Run-StartupTrace.ps1" -RunName $run -Seconds 200 -SteamLaunch -TestSave $save -TestSaveSha256 $hash -BuildFolder $build -BootstrapMode Native -Measure -NoDebugLayer 2>&1 } -ArgumentList $inv,$run,$save,$hash,$build
    $deadline=(Get-Date).AddSeconds(120)
    while(-not (Test-Path -LiteralPath "$o/startup.jsonl") -and (Get-Date) -lt $deadline -and $job.State -eq 'Running'){ Start-Sleep 1 }
    $start=Get-Date;$samples=0;$hits=@()
    New-Item -ItemType Directory -Force -Path "$o/pixels" | Out-Null
    while(((Get-Date)-$start).TotalSeconds -lt $Seconds -and (Alive)){
        $b=New-Object Drawing.Bitmap 1288,751;$g=[Drawing.Graphics]::FromImage($b);$g.CopyFromScreen(0,0,0,0,$b.Size);$g.Dispose();$samples++
        if(Orange $b 418){ $t=[int]((Get-Date)-$start).TotalMilliseconds;$hits+=$t;if($hits.Count -le 6){ $b.Save("$o/pixels/orange-$t.png") } }
        $b.Dispose();Start-Sleep -Milliseconds 300 }
    if(Alive){ pwsh -NoProfile -File "$inv/Pilot-Game.ps1" -Action shot -Path "$o/pixels/last.jpg" | Out-Null; pwsh -NoProfile -File "$inv/Pilot-Game.ps1" -Action click -X 1265 -Y 15 | Out-Null }
    Wait-Job $job -Timeout 400 | Out-Null
    while($job.State -eq 'Running'){ Wait-Job $job -Timeout 60 | Out-Null }
    $ok=if(Test-Path -LiteralPath "$o/restoration.json"){ (Get-Content -LiteralPath "$o/restoration.json" -Raw | ConvertFrom-Json).ok }else{ 'missing' }
    "$run build=$(Split-Path -Leaf $build) samples=$samples orange_hits=$($hits.Count) first_ms=$($hits | Select-Object -First 5) restoration=$ok" } }
