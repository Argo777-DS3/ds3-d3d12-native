# Disk guard (Thomas, 2026-11-08, D-0260): MOVES (never deletes) the per-run save backups that Run-StartupTrace.ps1 writes to
# ~\.ds3-i0039-backups\<run>-<yyyyMMdd-HHmmss>, beyond the newest -KeepNewest (100) or older than -Days when given, to
# D:\DS3-save-backups-archive under the same folder names. Each file is copied into <name>.partial, its SHA-256 checked against the
# original, the folder renamed once complete, and only then the original removed (a copy cut short is redone at the next call). The
# zone-witness saves (shebang-*), the Alpha saves (alpha-*), thomas-*, original-*, i65-*, every kept save (a record.json, from
# Pilot-QuitAndKeepSave.ps1) and every folder without a run timestamp never move: scripts read them by path. Without -Apply it only
# counts. Run-StartupTrace.ps1 calls it with -Apply -Quiet -MaxMove 20 before each run's backup. To undo: move the folders back.
param([switch]$Apply,[int]$KeepNewest=100,[int]$Days=0,[switch]$Quiet,[int]$MaxMove=0,[string]$Archive='D:\DS3-save-backups-archive',[string]$Source=(Join-Path $env:USERPROFILE '.ds3-i0039-backups'))
$ErrorActionPreference='Stop'
if(-not (Test-Path -LiteralPath ([IO.Path]::GetPathRoot($Archive)))){ if(-not $Quiet){Write-Output "No drive for $Archive : nothing moved"}; return }
$runs=@(Get-ChildItem -LiteralPath $Source -Directory -Force | Where-Object {
    $_.Name -notmatch '^(shebang|alpha|thomas|original|i65)' -and $_.Name -match '-\d{8}-\d{6}$' -and
    -not (Test-Path -LiteralPath (Join-Path $_.FullName 'record.json')) } | Sort-Object LastWriteTime -Descending)
# A folder found to differ from its archived copy is marked <name>.differs in the archive and left out from then on.
$runs=@($runs | Where-Object { -not (Test-Path -LiteralPath (Join-Path $Archive "$($_.Name).differs")) })
$move=if($Days -gt 0){ @($runs | Where-Object { $_.LastWriteTime -lt (Get-Date).AddDays(-$Days) }) }else{ @($runs | Select-Object -Skip $KeepNewest) }
if($MaxMove -gt 0){ $move=@($move | Sort-Object LastWriteTime | Select-Object -First $MaxMove) }   # the oldest first; a run's preparation stays short
$n=0;$bytes=0
foreach($d in $move){
    if(Get-Process DarkSoulsIII -ErrorAction SilentlyContinue){ if(-not $Quiet){Write-Output "Stopped: Dark Souls III runs ($n moved)"}; break }
    $files=@(Get-ChildItem -LiteralPath $d.FullName -Recurse -File -Force); $size=($files | Measure-Object Length -Sum).Sum
    if($Apply){
        $target=Join-Path $Archive $d.Name; $partial="$target.partial"
        if(Test-Path -LiteralPath $target){   # an earlier move whose original could not be removed: finish it if every file matches
            $same=@($files | Where-Object { $to=Join-Path $target $_.FullName.Substring($d.FullName.Length+1)
                (Test-Path -LiteralPath $to) -and (Get-FileHash -LiteralPath $to).Hash -eq (Get-FileHash -LiteralPath $_.FullName).Hash }).Count -eq $files.Count
            if($same){ Remove-Item -LiteralPath $d.FullName -Recurse -Force; $n++; $bytes+=$size }
            else{ Set-Content -LiteralPath "$target.differs" -Value "differs from $($d.FullName), $(Get-Date -Format o)"
                Write-Warning "Differs from its archived copy, left in place and marked $target.differs: $($d.FullName)" }
            continue }
        if(Test-Path -LiteralPath $partial){ Remove-Item -LiteralPath $partial -Recurse -Force }   # a copy cut short earlier; its original is still here
        New-Item -ItemType Directory -Force -Path $partial | Out-Null
        foreach($f in $files){ $to=Join-Path $partial $f.FullName.Substring($d.FullName.Length+1)
            New-Item -ItemType Directory -Force -Path (Split-Path -Parent $to) | Out-Null
            Copy-Item -LiteralPath $f.FullName -Destination $to
            if((Get-FileHash -LiteralPath $to).Hash -ne (Get-FileHash -LiteralPath $f.FullName).Hash){ throw "Copy mismatch: $to (the original is kept)" } }
        Rename-Item -LiteralPath $partial -NewName (Split-Path -Leaf $target)
        Remove-Item -LiteralPath $d.FullName -Recurse -Force }
    $n++;$bytes+=$size}
if(-not $Quiet -or $n){ Write-Output ("{0}: {1} run save backups, {2:N1} GB, to {3}" -f $(if($Apply){'Moved'}else{'Dry run, would move'}),$n,($bytes/1GB),$Archive) }
