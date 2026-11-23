# Disk guard (Thomas, 2026-11-08, D-0260): the evidence store's disposable cache (D-0142), rotated at each merge into main (skill
# merge-workstream) and whenever C: runs low. Without -Apply it only counts (a dry run). It deletes:
#   - the build folders of Build-GameBootstrap.ps1 -Out whose newest file is older than -BuildHours (48 h). A build folder carries
#     ds3-build.marker (written by the build since D-0260) or, older, a build.cmd naming game_bootstrap_ctl. Run-StartupTrace.ps1
#     writes last-used.txt into the build folder each run, so a folder a thread still runs on stays. Its module and .pdb go with it
#     (an old crash dump of that build is then no longer symbolised).
#   - the raw frames (.rgba*, .r11g11b10, .r16, .r32, .raw) older than -RawDays (7 days), outside build folders; their PNG sheets,
#     logs, JSONL and measures stay.
# A build folder stays whole when anything inside it, at any depth, is: a REPORT.*, manifest.json, provenance.json, baseline.json or
# startup.jsonl; a keep.txt; a folder named i66-* or i100-tr7-night or saying « original » or « reference »; a path listed in
# _maintenance/rotation-keep.txt or given to -Keep; or the target of a link in the Markdown of the main checkout or of any worktree
# (a link to the folder itself or into it, wildcards and braces read as prefixes). A raw frame stays when it is kept, protected or
# linked to itself (a link to its run folder does not keep it: D-0142). Never the launcher's I-0066/artifacts/game-bootstrap-build,
# _maintenance, _retired-worktrees or _studies. Each deleted file is written to _maintenance/cleanup-rotation.jsonl once deleted.
# Before each build folder and every 500 frames it checks that neither Dark Souls III nor a compiler runs, and stops if one does.
param([switch]$Apply,[int]$BuildHours=48,[int]$RawDays=7,[string]$Keep='')
$ErrorActionPreference='Stop'
[Diagnostics.Process]::GetCurrentProcess().PriorityClass='Idle'
$root=Split-Path -Parent $PSScriptRoot
$ev=(Get-Item -LiteralPath (Join-Path $root 'evidence') -Force -ErrorAction SilentlyContinue).Target
if(-not $ev){$ev='C:\Users\User\Desktop\DS3_evidences'}
$ev=([IO.DirectoryInfo]::new(@($ev)[0])).FullName.TrimEnd('\')
if(-not (Test-Path -LiteralPath (Join-Path $ev '_maintenance'))){throw "Evidence store not found: $ev"}
function Busy{ [bool](Get-Process DarkSoulsIII,cl,link -ErrorAction SilentlyContinue) }
if(Busy){throw 'Dark Souls III or a compiler is running: nothing done'}
$launcher='I-0066\artifacts\game-bootstrap-build'
$protected='(^|\\)(i66-[^\\]*|i100-tr7-night)(\\|$)|original|reference'
$reports='^(REPORT\..*|manifest\.json|provenance\.json|baseline\.json|startup\.jsonl)$'
function StoreRel([string]$p){   # a path given relative to the store, through a checkout's evidence junction or absolute
    $k=$p.Trim().Trim('"').Replace('/','\').TrimEnd('\')
    if($k.StartsWith($ev,[StringComparison]::OrdinalIgnoreCase)){return $k.Substring($ev.Length).TrimStart('\')}
    return ($k -replace '^.*?(^|\\)(evidence|DS3_evidences)\\','' -replace '^\.\\','')}
$keepRel=[Collections.Generic.List[string]]::new()
foreach($k in @($Keep -split '[,;]' | Where-Object { $_.Trim() })){ $r=StoreRel $k
    if(-not (Test-Path -LiteralPath (Join-Path $ev $r))){throw "Kept path not found in the evidence store: $k"}; $keepRel.Add($r) }
$keepFile=Join-Path $ev '_maintenance\rotation-keep.txt'
if(Test-Path -LiteralPath $keepFile){ foreach($k in Get-Content -LiteralPath $keepFile | Where-Object { $_ -notmatch '^\s*(#|$)' }){ $r=StoreRel $k
    if(-not (Test-Path -LiteralPath (Join-Path $ev $r))){throw "A line of _maintenance/rotation-keep.txt names no folder of the store: $k (correct or remove the line)"}
    $keepRel.Add($r) } }
# The links of every checkout (the main one and each worktree, with its branch's unmerged documents) into the store.
$checkouts=@(git -C $root worktree list --porcelain | Where-Object { $_ -like 'worktree *' } | ForEach-Object { $_.Substring(9).Replace('/','\') })
if($LASTEXITCODE -or -not $checkouts.Count){throw 'git worktree list failed: the checkouts cannot be read, refusing to rotate'}
$linked=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach($c in $checkouts){ if(-not (Test-Path -LiteralPath $c)){continue}
    foreach($md in Get-ChildItem -LiteralPath $c -Recurse -Filter *.md -File -ErrorAction SilentlyContinue){
        if($md.FullName.Substring($c.Length) -match '\\(\.git|\.claude|evidence)\\'){continue}
        foreach($m in [regex]::Matches([string](Get-Content -LiteralPath $md.FullName -Raw),'(?:evidence|DS3_evidences)[/\\]([^)\s`>"''\]]+)')){
            [void]$linked.Add($m.Groups[1].Value.Replace('/','\').TrimEnd('\','.',',',';',':')) } } }
if($linked.Count -lt 100){throw "Only $($linked.Count) evidence links found in the checkouts' Markdown: refusing to rotate"}
$raw='.rgba','.rgba16f','.rgba8','.rgba32f','.r11g11b10','.r16','.r32','.raw'
$buildLimit=(Get-Date).AddHours(-$BuildHours); $rawLimit=(Get-Date).AddDays(-$RawDays)
$opt=[IO.EnumerationOptions]::new();$opt.RecurseSubdirectories=$true;$opt.IgnoreInaccessible=$true;$opt.AttributesToSkip=[IO.FileAttributes]::ReparsePoint
$all=[Collections.Generic.List[object]]::new()
$roots=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$held=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)   # folders holding a keep.txt or a report file
$keepDirs=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)   # folders holding a keep.txt
$dirSet=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)   # every folder holding a file
foreach($f in [IO.DirectoryInfo]::new($ev).EnumerateFiles('*',$opt)){
    $rel=$f.FullName.Substring($ev.Length+1); $dir=[IO.Path]::GetDirectoryName($rel)
    if($rel -match '^_'){continue}
    [void]$dirSet.Add($dir)
    if($f.Name -eq 'keep.txt'){ [void]$keepDirs.Add($dir) }
    if($f.Name -eq 'keep.txt' -or $f.Name -match $reports){ [void]$held.Add($dir) }
    if($f.Name -eq 'ds3-build.marker'){ [void]$roots.Add($dir) }
    elseif($f.Name -eq 'build.cmd' -and (Select-String -LiteralPath $f.FullName -Pattern 'game_bootstrap_ctl' -SimpleMatch -Quiet)){ [void]$roots.Add($dir) }
    $all.Add([pscustomobject]@{rel=$rel;dir=$dir;file=$f})}
[void]$roots.Remove($launcher)
function Ancestors([string]$dir){ while($dir){ $dir; $dir=[IO.Path]::GetDirectoryName($dir) } }
function Under([string]$path,[string]$folder){ $path -ieq $folder -or $path.StartsWith($folder+'\',[StringComparison]::OrdinalIgnoreCase) }
# A link written with a wildcard or braces (« build-m* », « build-{1,2} ») names every folder its prefix may stand for.
# Only a folder pattern whose wildcard sits in its last name after at least three plain characters counts (« I-0066/artifacts/*/x »
# is a generic recipe, not a citation).
$linkPatterns=@($linked | Where-Object { $_ -match '[*{]' } | ForEach-Object {
    $p=($_ -replace '\{[^}]*\}','*') -replace '\*+','*'
    while($p){ $parent=[IO.Path]::GetDirectoryName($p); $leaf=[IO.Path]::GetFileName($p)
        if($leaf -match '^[^*]{3,}\*' -and $parent -notmatch '\*'){ $p }
        $p=$parent } } | Sort-Object -Unique)
function Linked([string]$b){ @($linked | Where-Object { Under $_ $b }).Count -or @($linkPatterns | Where-Object { $b -like $_ }).Count }
function Spared([string]$dir){   # a folder kept (keep.txt in it or above, rotation-keep.txt, -Keep), protected or the launcher's
    foreach($a in Ancestors $dir){ if($a -ieq $launcher -or $keepDirs.Contains($a)){return $true} }
    if($dir -match $protected){return $true}
    foreach($k in $keepRel){ if(Under $dir $k){return $true} }
    return $false}
function RootOf([string]$dir){ foreach($a in Ancestors $dir){ if($roots.Contains($a)){return $a} }; return $null }   # the nearest build folder
$builds=@{};$frames=[Collections.Generic.List[IO.FileInfo]]::new();$skipped=[Collections.Generic.HashSet[string]]::new()
foreach($x in $all){
    $b=RootOf $x.dir
    if($b){ if(-not $builds[$b]){$builds[$b]=[Collections.Generic.List[object]]::new()}; $builds[$b].Add($x); continue }
    if($raw -notcontains $x.file.Extension.ToLowerInvariant() -or $x.file.LastWriteTime -ge $rawLimit){continue}
    if((Spared $x.dir) -or $linked.Contains($x.rel)){continue}   # a frame a document links to itself stays
    $frames.Add($x.file)}
$flagged=@(@($held) + @($dirSet | Where-Object { $_ -match $protected }) | Sort-Object -Unique)   # folders holding a report or keep.txt, or protected
$old=@(foreach($b in ($builds.Keys | Sort-Object)){
    if(($builds[$b].file | Measure-Object LastWriteTime -Maximum).Maximum -ge $buildLimit){continue}
    $hit=@($flagged | Where-Object { Under $_ $b }).Count -or (Spared $b) -or   # any folder under it, nested builds included
        @($keepRel | Where-Object { Under $_ $b }).Count -or (Linked $b)
    if($hit){ [void]$skipped.Add($b); continue }
    $b})
$log=Join-Path $ev '_maintenance\cleanup-rotation.jsonl';$n=0;$bytes=0;$failed=0;$stopped=$false;$foldersDone=0
function Drop([IO.FileInfo]$f,[string]$kind){
    if($Apply){
        if(-not (Test-Path -LiteralPath $f.FullName)){return}
        try{ [IO.File]::Delete($f.FullName) }catch{ $script:failed++; return }
        try{ Add-Content -LiteralPath $log -Encoding utf8 -Value (@{date=(Get-Date).ToString('o');kind=$kind;path=$f.FullName.Substring($ev.Length+1);bytes=$f.Length;last_write=$f.LastWriteTime.ToString('o')} | ConvertTo-Json -Compress) }
        catch{ Write-Warning "Audit line not written for $($f.FullName): $_" } }
    $script:n++;$script:bytes+=$f.Length}
foreach($b in $old){
    if($Apply -and (Busy)){$stopped=$true;break}
    $d=Join-Path $ev $b
    $newest=(Get-ChildItem -LiteralPath $d -Recurse -File -Force -ErrorAction SilentlyContinue | Measure-Object LastWriteTime -Maximum).Maximum
    if($newest -and $newest -ge $buildLimit){ [void]$skipped.Add($b); continue }   # a run or a build started meanwhile
    foreach($x in $builds[$b]){Drop $x.file 'build'}
    if($Apply -and (Test-Path -LiteralPath $d)){   # the emptied folders, deepest first
        Get-ChildItem -LiteralPath $d -Directory -Recurse -Force | Sort-Object {$_.FullName.Length} -Descending |
            Where-Object {-not (Get-ChildItem -LiteralPath $_.FullName -Force)} | Remove-Item -Force -ErrorAction SilentlyContinue
        if(-not (Get-ChildItem -LiteralPath $d -Force)){Remove-Item -LiteralPath $d -Force -ErrorAction SilentlyContinue} }
    $foldersDone++}
$i=0
if(-not $stopped){ foreach($f in $frames){ if($Apply -and ($i++ % 500) -eq 0 -and (Busy)){$stopped=$true;break}; Drop $f 'raw-frame' } }
$free=[IO.DriveInfo]::new([IO.Path]::GetPathRoot($ev)).AvailableFreeSpace
if($stopped){Write-Output 'Stopped: Dark Souls III or a compiler started (no build folder is left half deleted).'}
if(-not $Apply -and $old.Count){Write-Output ("Build folders to delete: {0}" -f ($old -join ', '))}
if($skipped.Count){Write-Output ("Old build folders kept (a report, keep, link or protected folder inside, or used meanwhile): {0}" -f (($skipped | Sort-Object) -join ', '))}
Write-Output ("{0}: {1} files, {2:N1} GB, hard-linked DLLs counted ({3} of {4} build folders older than {5} h; raw frames older than {6} days: {7}){8}; {9:N1} GB free on {10}" -f
    $(if($Apply){'Deleted'}else{'Dry run, would delete'}),$n,($bytes/1GB),$foldersDone,$old.Count,$BuildHours,$RawDays,$frames.Count,
    $(if($failed){"; $failed files could not be deleted"}else{''}),($free/1GB),[IO.Path]::GetPathRoot($ev))
