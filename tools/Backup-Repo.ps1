[CmdletBinding()]
param([string]$Destination='D:\DS3-Backup',[switch]$NoEvidence,[switch]$Quiet)
# Thomas, 2026-10-23: a full copy of the project on the second SSD (the whole repository is too heavy for GitHub). Robocopy mirrors
# the `DS3 Decompilation` folder, `.git` and the worktrees included, to <Destination>\DS3 Decompilation, then the Evidence Store
# (`DS3_evidences`, outside Git, reached in the repository only through the `evidence` junctions) to <Destination>\DS3_evidences
# unless -NoEvidence. The source is only read: /MIR deletes only what the copy holds and the source no longer has. Junctions are
# not followed (/XJ), so the store is copied once and never through a worktree. Started by the desktop shortcut in a hidden
# window; the log goes to <Destination>\logs (the last twenty kept) and a message box says how it ended unless -Quiet.
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
# From a worktree (.claude\worktrees\<name>\tools) the copy is still of the main folder.
if($root -match '^(.*)\\\.claude\\worktrees\\[^\\]+$'){$root=$Matches[1]}
$evidence=Join-Path (Split-Path -Parent $root) 'DS3_evidences'
function Done([string]$text,[bool]$ok){
  if($log){Add-Content -LiteralPath $log -Value "`r`n$(Get-Date -Format s) $text"}
  if($mutex){try{$mutex.ReleaseMutex()}catch{}}
  if(-not $Quiet){Add-Type -AssemblyName System.Windows.Forms
    [void][System.Windows.Forms.MessageBox]::Show($text,'Sauvegarde DS3',0,$(if($ok){64}else{16}))}
  exit $(if($ok){0}else{1})}
$log=$null;$mutex=$null
$dest=[IO.Path]::GetFullPath($Destination)
if(-not (Test-Path -LiteralPath (Split-Path -Qualifier $dest))){Done "Le disque $(Split-Path -Qualifier $dest) est introuvable : rien n'est copié." $false}
foreach($s in @($root,$evidence)){if($dest.TrimEnd('\').StartsWith($s.TrimEnd('\'),[StringComparison]::OrdinalIgnoreCase) -or
  $s.TrimEnd('\').StartsWith($dest.TrimEnd('\'),[StringComparison]::OrdinalIgnoreCase)){Done "La destination $dest recouvre la source $s : rien n'est copié." $false}}
$mutex=[Threading.Mutex]::new($false,'Global\DS3-Backup-Repo')
if(-not $mutex.WaitOne(0)){Done 'Une sauvegarde tourne déjà.' $false}
$logs=Join-Path $dest 'logs'
New-Item -ItemType Directory -Force -Path $logs | Out-Null
$log=Join-Path $logs "backup-$(Get-Date -Format yyyyMMdd-HHmmss).log"
Get-ChildItem -LiteralPath $logs -Filter 'backup-*.log' | Sort-Object Name -Descending | Select-Object -Skip 19 | Remove-Item -Force
Set-Content -LiteralPath $log -Value "$(Get-Date -Format s) source $root -> $dest"
if(Get-Process git -ErrorAction SilentlyContinue){Add-Content -LiteralPath $log -Value 'Note: a git process was running at the start.'}
$pairs=@(,@($root,(Join-Path $dest 'DS3 Decompilation')))
if(-not $NoEvidence -and (Test-Path -LiteralPath $evidence)){$pairs+=,@($evidence,(Join-Path $dest 'DS3_evidences'))}
$codes=@()
foreach($p in $pairs){
  # /MIR the copy follows the source; /XJ junctions not followed; /COPY:DAT /DCOPY:DAT data, attributes, times; /R:2 /W:2 a
  # locked file is retried briefly then skipped (counted as failed in the log); /MT:8 eight copy threads; the file lists stay
  # out of the log (/NFL /NDL), the summary and the errors stay in it.
  & robocopy $p[0] $p[1] /MIR /XJ /COPY:DAT /DCOPY:DAT /R:2 /W:2 /MT:8 /NP /NFL /NDL "/UNILOG+:$log" | Out-Null
  $codes+=$LASTEXITCODE}
$failed=@($codes | Where-Object {$_ -ge 8}).Count
$size=[math]::Round(((Get-ChildItem -LiteralPath $dest -Recurse -File -Force -ErrorAction SilentlyContinue | Measure-Object Length -Sum).Sum)/1GB,1)
$free=[math]::Round((Get-PSDrive ($dest.Substring(0,1))).Free/1GB)
Set-Content -LiteralPath (Join-Path $dest 'last-backup.txt') -Value "$(Get-Date -Format s) robocopy $($codes -join ',') ; $size Go copiés ; $free Go libres"
if($failed){Done "Sauvegarde incomplète (codes robocopy $($codes -join ', ')) : voir $log" $false}
Done "Sauvegarde terminée : $size Go dans $dest, $free Go libres sur le disque." $true
