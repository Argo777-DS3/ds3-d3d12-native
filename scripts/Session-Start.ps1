# Claude Code SessionStart hook (repository audit, Thomas 2026-10-25): what a fresh thread needs before its first step, printed
# into its context: its checkout and branch, STATE's resume rows (pilot, workstream, next action), the game and Ghidra running or
# not, the evidence junction, C:'s free space. It reads only; it never fails the session start.
$ErrorActionPreference = 'SilentlyContinue'
[Console]::OutputEncoding = [Text.UTF8Encoding]::new($false)
$root = Split-Path -Parent $PSScriptRoot
try {
    $hookInput = [Console]::In.ReadToEnd() | ConvertFrom-Json -ErrorAction Stop
    if ($hookInput.cwd) {
        $top = [string](git -C $hookInput.cwd rev-parse --show-toplevel 2>$null)
        if ($top -and (Test-Path -LiteralPath (Join-Path $top '.project/STATE.md'))) { $root = $top }
    }
} catch { }

$state = Get-Content -LiteralPath (Join-Path $root '.project/STATE.md') -Raw -Encoding utf8
function Row([string]$name) {
    $m = [regex]::Match($state, "(?m)^\|\s*$([regex]::Escape($name))\s*\|\s*(.+?)\s*\|\s*$")
    if (-not $m.Success) { return '(absent)' }
    $text = $m.Groups[1].Value
    if ($text.Length -gt 700) { $text = $text.Substring(0, 700) + ' …' }
    $text
}
$branch = [string](git -C $root branch --show-current 2>$null)
$game = if (Get-Process DarkSoulsIII -ErrorAction SilentlyContinue) { 'DarkSoulsIII RUNS: a live run is in progress, no build, Ghidra or heavy work' } else { 'not running' }
$ghidra = if (Get-Process java -ErrorAction SilentlyContinue) { 'java runs (headless Ghidra?)' } else { 'no java' }
$evidence = $null -ne (Get-Item -LiteralPath (Join-Path $root 'evidence') -Force -ErrorAction SilentlyContinue)
# D-0260: C:'s free space, with a warning under 100 GB (the evidence store, the builds and the save backups live there).
$freeGb = try { [math]::Round([IO.DriveInfo]::new('C:').AvailableFreeSpace / 1GB) } catch { $null }
$disk = if ($null -eq $freeGb) { 'C: free space unknown' } elseif ($freeGb -lt 100) { "C: $freeGb GB free, LOW (under 100 GB): run scripts/Clear-DiskCache.ps1 -Apply (D-0260) before builds or runs; anything beyond its rotation is Thomas's call" } else { "C: $freeGb GB free" }

@"
DS3 project, session start (scripts/Session-Start.ps1). Use skill resume-thread before any technical step.
Checkout: $root (branch $branch); main at $([string](git -C $root rev-parse --short main 2>$null)).
Pilot branch: $(Row 'Pilot branch')
Active workstream: $(Row 'Active workstream')
Next action: $(Row 'Next action')
PC: game $game; $ghidra. Evidence junction: $(if ($evidence) { 'connected' } else { 'missing: run scripts/Connect-EvidenceStore.ps1' }). Disk: $disk.
Rules: .project/WORKFLOW.md (tools in its « Tools » section); know-how: .project/LIVE-RUN-NOTES.md.
"@
exit 0
