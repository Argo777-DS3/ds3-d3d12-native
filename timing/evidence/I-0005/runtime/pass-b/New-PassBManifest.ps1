[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$Root = $PSScriptRoot
$Excluded = @('manifest.json', 'SHA256SUMS.txt')

$Files = Get-ChildItem -LiteralPath $Root -Recurse -File |
    Where-Object {
        $Relative = $_.FullName.Substring($Root.Length + 1).Replace('\', '/')
        $Relative -notin $Excluded -and
        $Relative -notlike '__pycache__/*' -and
        $Relative -notlike 'instrumentation/obj/*' -and
        $Relative -notlike 'instrumentation/bin/*.pdb'
    } |
    Sort-Object FullName

$Entries = foreach ($File in $Files) {
    $Relative = $File.FullName.Substring($Root.Length + 1).Replace('\', '/')
    [ordered]@{
        path = $Relative
        size = $File.Length
        sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $File.FullName).Hash
    }
}

$Manifest = [ordered]@{
    schema = 'I-0005-pass-b-manifest-v1'
    investigation = 'I-0005'
    build = [ordered]@{
        appid = 374320
        buildid = 10167187
        executable_version = '1.15.2.0'
        executable_sha256 = 'EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C'
    }
    runtime = [ordered]@{
        offline = $true
        game_directory_modified = $false
        intervention = 'central +0x354 requested FPS float and +0x358 enable byte, memory only'
        present_override_used = $false
        original_state_restored = $true
        display_context = 'G-Sync enabled per operator; process-side Present evidence only'
    }
    primary_sessions = @(
        '20260919T094057Z-pid12600',
        '20260919T094304Z-pid19080',
        '20260919T094516Z-pid27388'
    )
    files = $Entries
}

$Manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $Root 'manifest.json') -Encoding utf8NoBOM
$Entries | ForEach-Object { '{0} *{1}' -f $_.sha256, $_.path } |
    Set-Content -LiteralPath (Join-Path $Root 'SHA256SUMS.txt') -Encoding ascii

[pscustomobject]@{
    Root = $Root
    Files = $Entries.Count
    Bytes = ($Entries | Measure-Object -Property size -Sum).Sum
    ManifestSHA256 = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $Root 'manifest.json')).Hash
    ChecksumsSHA256 = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $Root 'SHA256SUMS.txt')).Hash
}
