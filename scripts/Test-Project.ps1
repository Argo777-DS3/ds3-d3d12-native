[CmdletBinding()]
param(
    # I-0083: the merge step's switch, kept for the callers; since D-0227 it requires no gate record.
    [switch]$MergeGate
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$errors = [System.Collections.Generic.List[string]]::new()
$retention = Get-Content -LiteralPath (Join-Path $repoRoot '.project/EVIDENCE-RETENTION.json') -Raw | ConvertFrom-Json
$firstRetained = [int]$retention.first_retained_investigation
$historicalLinks = 0
# The 2026-10-13 cleanup (Thomas) removed the older investigations from main; they live in the archive tag.
$archive = Get-Content -LiteralPath (Join-Path $repoRoot '.project/ARCHIVE.json') -Raw | ConvertFrom-Json
$firstKept = [int]$archive.first_kept_investigation
$archivedLinks = 0

$requiredFiles = @(
    'CLAUDE.md',
    'README.md',
    '.project/INDEX.md',
    '.project/STATE.md',
    '.project/ROADMAP.md',
    '.project/WORKFLOW.md',
    'knowledge/LEDGER.md',
    'decisions/LOG.md',
    'decisions/IN-FORCE.md',
    '.project/TOOLS.md',
    '.project/METHOD.md',
    'investigations/README.md',
    'evidence/INDEX.md',
    'evidence/README.md'
)

foreach ($relativePath in $requiredFiles) {
    if (-not (Test-Path -LiteralPath (Join-Path $repoRoot $relativePath) -PathType Leaf)) {
        $errors.Add("Missing required file: $relativePath")
    }
}

# D-0217: every skill and subagent WORKFLOW lists exists, with its front matter.
$workflowText = Get-Content -LiteralPath (Join-Path $repoRoot '.project/WORKFLOW.md') -Raw -Encoding utf8
foreach ($entry in [regex]::Matches($workflowText, '(?m)^\|\s*`([a-z0-9-]+)`\s*\|\s*(skill|subagent)\s*\|')) {
    $relative = if ($entry.Groups[2].Value -eq 'skill') { ".claude/skills/$($entry.Groups[1].Value)/SKILL.md" } else { ".claude/agents/$($entry.Groups[1].Value).md" }
    $path = Join-Path $repoRoot $relative
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { $errors.Add("WORKFLOW lists $($entry.Groups[2].Value) '$($entry.Groups[1].Value)' but $relative is missing") }
    elseif ((Get-Content -LiteralPath $path -TotalCount 2)[1] -notmatch "^name:\s*$($entry.Groups[1].Value)\s*$") { $errors.Add("$relative does not start with its front matter (name: $($entry.Groups[1].Value))") }
}

# Parse every PowerShell helper before relying on it, and Thomas's launchers (2026-11-02: an unescaped quote in tools/Launch-DS3-DX12.ps1
# closed the play launcher's window at once for a morning).
$powerShellFiles = @('scripts', 'tools') | ForEach-Object { Get-ChildItem -LiteralPath (Join-Path $repoRoot $_) -File -Filter '*.ps1' }
foreach ($scriptFile in $powerShellFiles) {
    $tokens = $null
    $parseErrors = $null
    [void][System.Management.Automation.Language.Parser]::ParseFile(
        $scriptFile.FullName,
        [ref]$tokens,
        [ref]$parseErrors
    )
    foreach ($parseError in $parseErrors) {
        $errors.Add("PowerShell parse error in $($scriptFile.Name) at line $($parseError.Extent.StartLineNumber): $($parseError.Message)")
    }
}

$allowedStates = @('planned', 'active', 'blocked', 'complete', 'abandoned')
$investigationRoot = Join-Path $repoRoot 'investigations'
$investigations = Get-ChildItem -LiteralPath $investigationRoot -Directory |
    Where-Object { $_.Name -ne '_template' }

$seenIds = @{}
foreach ($directory in $investigations) {
    foreach ($name in @('CONTRACT.md', 'RESULT.md', 'status.json')) {
        if (-not (Test-Path -LiteralPath (Join-Path $directory.FullName $name) -PathType Leaf)) {
            $errors.Add("$($directory.Name): missing $name")
        }
    }

    $statusPath = Join-Path $directory.FullName 'status.json'
    if (-not (Test-Path -LiteralPath $statusPath -PathType Leaf)) { continue }

    try {
        $status = Get-Content -LiteralPath $statusPath -Raw -Encoding utf8 | ConvertFrom-Json
    }
    catch {
        $errors.Add("$($directory.Name): invalid status.json: $($_.Exception.Message)")
        continue
    }

    if ($status.id -notmatch '^I-[0-9]{4}$') {
        $errors.Add("$($directory.Name): invalid investigation id '$($status.id)'")
    }
    elseif ($seenIds.ContainsKey($status.id)) {
        $errors.Add("Duplicate investigation id: $($status.id)")
    }
    else {
        $seenIds[$status.id] = $true
    }

    if ($directory.Name -notlike "$($status.id)-*") {
        $errors.Add("$($directory.Name): directory does not match status id $($status.id)")
    }
    if ($allowedStates -notcontains $status.state) {
        $errors.Add("$($directory.Name): invalid state '$($status.state)'")
    }
    if ([string]::IsNullOrWhiteSpace($status.owner)) {
        $errors.Add("$($directory.Name): owner must not be empty")
    }
    if ($status.budget.iterations_max -lt 1) {
        $errors.Add("$($directory.Name): iterations_max must be positive")
    }
    if ($status.budget.iterations_used -lt 0 -or $status.budget.iterations_used -gt $status.budget.iterations_max) {
        $errors.Add("$($directory.Name): iterations_used is outside the declared budget")
    }

    $manifestPath = Join-Path $repoRoot "evidence/$($status.id)/manifest.json"
    if ([int]$status.id.Substring(2) -lt $firstRetained) { continue }
    if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
        $errors.Add("$($directory.Name): missing evidence manifest evidence/$($status.id)/manifest.json")
    }
    else {
        try {
            $manifest = Get-Content -LiteralPath $manifestPath -Raw -Encoding utf8 | ConvertFrom-Json
            if ($manifest.investigation_id -ne $status.id) {
                $errors.Add("$($directory.Name): evidence manifest investigation_id mismatch")
            }
        }
        catch {
            $errors.Add("$($directory.Name): invalid evidence manifest: $($_.Exception.Message)")
        }
    }
}

# Check repository-local Markdown links. Ignore URLs and anchors.
# The timing project imported under timing/ (Thomas, 2026-10-21) keeps its own original layout: its links are not checked
# (timing/IMPORT.md), only the import note itself.
# Repository audit (2026-10-25): the folders that are not this checkout's Markdown are skipped before the walk, not after it:
# .git, the sessions' worktrees under .claude/ (other branches), the local game inputs, the evidence junction and timing/.
$skippedRoots = @('.git', '.claude', 'game', 'evidence', 'timing')
$markdownFiles = @(Get-ChildItem -LiteralPath $repoRoot -File -Filter '*.md') +
    @(Get-ChildItem -LiteralPath $repoRoot -Directory -Force | Where-Object { $skippedRoots -notcontains $_.Name } |
        ForEach-Object { Get-ChildItem -LiteralPath $_.FullName -Recurse -File -Filter '*.md' }) +
    @(Get-Item -LiteralPath (Join-Path $repoRoot 'timing/IMPORT.md') -ErrorAction SilentlyContinue)
foreach ($file in $markdownFiles) {
    $content = Get-Content -LiteralPath $file.FullName -Raw -Encoding utf8
    $matches = [regex]::Matches($content, '\[[^\]]+\]\(([^)]+)\)')
    foreach ($match in $matches) {
        $target = $match.Groups[1].Value.Split('#')[0]
        if ([string]::IsNullOrWhiteSpace($target) -or $target -match '^[a-zA-Z][a-zA-Z0-9+.-]*:') { continue }
        $decodedTarget = [System.Uri]::UnescapeDataString($target)
        $resolved = Join-Path $file.DirectoryName $decodedTarget
        if (-not (Test-Path -LiteralPath $resolved)) {
            $absolute = [IO.Path]::GetFullPath($resolved)
            $evidencePrefix = [IO.Path]::GetFullPath((Join-Path $repoRoot 'evidence')) + [IO.Path]::DirectorySeparatorChar
            if ($absolute.StartsWith($evidencePrefix, [StringComparison]::OrdinalIgnoreCase)) {
                $evidenceRelative = $absolute.Substring($evidencePrefix.Length)
                if ($evidenceRelative -match '^I-(\d{4})([\\/]|$)' -and [int]$Matches[1] -lt $firstRetained) {
                    $historicalLinks++
                    continue
                }
            }
            $investigationPrefix = [IO.Path]::GetFullPath($investigationRoot) + [IO.Path]::DirectorySeparatorChar
            if ($absolute.StartsWith($investigationPrefix, [StringComparison]::OrdinalIgnoreCase)) {
                $investigationRelative = $absolute.Substring($investigationPrefix.Length)
                if ($investigationRelative -match '^I-(\d{4})-' -and [int]$Matches[1] -lt $firstKept) {
                    $archivedLinks++
                    continue
                }
            }
            $relativeFile = [System.IO.Path]::GetRelativePath($repoRoot, $file.FullName)
            $errors.Add("Broken local link in ${relativeFile}: $target")
        }
    }
}


# D-0227 (Thomas, 2026-10-28, full production): no replayed path gate record is required on main or before a merge (I-0083's
# check removed); .project/REPLAY-GATE.json and scripts/Invoke-ReplayGate.ps1 stay as tools, run on Thomas's request.

# Repository audit (2026-10-25): the resume checkpoint stays short. Its rows are rewritten at each checkpoint, the history
# goes to the workstream's RESULT; a long row is reported (a warning, not a failure).
$stateText = Get-Content -LiteralPath (Join-Path $repoRoot '.project/STATE.md') -Raw -Encoding utf8
foreach ($row in [regex]::Matches($stateText, '(?m)^\|\s*([^|]+?)\s*\|\s*(.+?)\s*\|\s*$')) {
    $words = @($row.Groups[2].Value -split '\s+' | Where-Object { $_ }).Count
    if ($words -gt 150 -and $row.Groups[1].Value -notmatch '^(Item|#|-+)$') {
        Write-Warning "STATE.md row « $($row.Groups[1].Value) » has $words words (over 150): move its history to the workstream's RESULT."
    }
}
# D-0291 (Thomas, 2026-11-15): the control files stay short. STATE's « Updated » line names the last checkpoint alone, and each
# file has a size target (WORKFLOW « Canonical sources »); over it, a warning: move the history to the RESULTs, the LOG or
# .project/archive/<date>/, never delete it.
if ([regex]::IsMatch($stateText, '(?m)^Updated: .*(; 20\d\d-\d\d-\d\d \d\d:\d\d|\b[Bb]efore it\b)')) {
    Write-Warning "STATE.md's « Updated » line holds more than one checkpoint: keep the last one only (D-0291)."
}
$sizeTargets = [ordered]@{
    '.project/STATE.md' = 14KB; '.project/WORKFLOW.md' = 36KB; '.project/ROADMAP.md' = 24KB
    'decisions/IN-FORCE.md' = 40KB; 'investigations/README.md' = 28KB; 'CLAUDE.md' = 3.5KB
}
foreach ($target in $sizeTargets.GetEnumerator()) {
    $path = Join-Path $repoRoot $target.Key
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { continue }
    $size = (Get-Item -LiteralPath $path).Length
    if ($size -gt $target.Value) {
        Write-Warning "$($target.Key) is $([math]::Round($size / 1KB, 1)) KB, over its $([math]::Round($target.Value / 1KB, 1)) KB target (D-0291): move history out, archive rather than delete."
    }
}

# R13 cF.5: the render and prepare functions' thread-locals and mutable statics against the reviewed allow-list.
$affinity = & (Join-Path $PSScriptRoot 'Test-ThreadAffinity.ps1') 2>&1
if ($LASTEXITCODE) { $errors.Add("Thread affinity check failed:`n$($affinity -join "`n")") }

if ($errors.Count -gt 0) {
    $errors | ForEach-Object { Write-Error $_ }
    exit 1
}

Write-Output "PASS: project structure, investigation records, budgets, manifests, and local links are consistent."
Write-Output "Checked $($requiredFiles.Count) required files, $($investigations.Count) investigations, and $($markdownFiles.Count) Markdown files."
Write-Output "Retention: evidence before I-$('{0:D4}' -f $firstRetained) intentionally removed; $historicalLinks historical links are unavailable and excluded from the local-link check."
Write-Output "Archive: investigations before I-$('{0:D4}' -f $firstKept) are in tag $($archive.tag); $archivedLinks links point there and are excluded from the local-link check."
Write-Output "Scope note: this audit does not validate Dark Souls III behavior, runtime safety, performance, or visual quality."
