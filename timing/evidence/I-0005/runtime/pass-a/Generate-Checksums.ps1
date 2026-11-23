[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$Output = Join-Path $Root 'SHA256SUMS.txt'

$lines = Get-ChildItem -LiteralPath $Root -Recurse -File |
    Where-Object {
        $_.FullName -ne $Output -and
        $_.FullName -notmatch '[\\/]obj[\\/]' -and
        $_.Extension -ne '.pdb'
    } |
    Sort-Object FullName |
    ForEach-Object {
        $relative = $_.FullName.Substring($Root.Length).TrimStart('\').Replace('\', '/')
        $hash = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
        "$hash  $relative"
    }

[System.IO.File]::WriteAllText($Output, ($lines -join "`n") + "`n", [System.Text.UTF8Encoding]::new($false))
Write-Output $Output
