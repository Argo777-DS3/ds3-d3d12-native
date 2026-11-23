[CmdletBinding()]
param([Parameter(Mandatory)][string]$Baseline)
$ErrorActionPreference='Stop'
if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){ throw 'Game still running; restoration refused' }
$b=Get-Content -LiteralPath $Baseline -Raw | ConvertFrom-Json
$out=Split-Path -Parent (Resolve-Path -LiteralPath $Baseline).Path
$post=@(for($i=0;$i -lt $b.files.Count;$i++){
    $r=$b.files[$i]
    if(Test-Path -LiteralPath $r.source){
        $hash=(Get-FileHash -LiteralPath $r.source).Hash
        if($hash -ne $r.sha256){
            $copy=Join-Path (Split-Path -Parent $r.backup) "recovery-post-$i-$([IO.Path]::GetFileName($r.source))"
            if(Test-Path -LiteralPath $copy){ throw 'Post-play preservation already exists; inspect before repeating' }
            Copy-Item -LiteralPath $r.source -Destination $copy
            [ordered]@{source=$r.source;preserved=$copy;sha256=$hash}
        }
    }
})
foreach($r in $b.files){
    if((Get-FileHash -LiteralPath $r.backup).Hash -ne $r.sha256){ throw 'Protected backup changed' }
    Copy-Item -LiteralPath $r.backup -Destination $r.source -Force
    if((Get-FileHash -LiteralPath $r.source).Hash -ne $r.sha256){ throw 'Restore hash mismatch' }
}
$changed=@($b.installed | Where-Object { -not (Test-Path -LiteralPath $_.path) -or (Get-FileHash -LiteralPath $_.path).Hash -ne $_.sha256 })
$game=Split-Path -Parent $b.installed[0].path
$extra=@(Get-ChildItem -LiteralPath $game -File | Where-Object { $_.Extension -in @('.exe','.dll','.addon64') -and $_.FullName -notin $b.installed.path })
$report=[ordered]@{restored_utc=[DateTime]::UtcNow.ToString('o');files=$b.files;post_play=$post;installed_changed=$changed;
    installed_extra=@($extra | ForEach-Object FullName);ok=($changed.Count -eq 0 -and $extra.Count -eq 0)}
$report | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath "$out/restoration.json" -Encoding utf8
if(-not $report.ok){ throw 'Installed components changed' }
Write-Output 'Save/configuration/component restoration verified; game absent.'
