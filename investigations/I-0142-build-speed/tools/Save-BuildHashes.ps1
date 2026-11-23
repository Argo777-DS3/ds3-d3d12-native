# SHA-256 of every binary and generated header of a build folder (I-0142 serial-against-Ninja comparison).
param([string]$Folder,[string]$OutFile)
$ErrorActionPreference='Stop'
$rows=Get-ChildItem -LiteralPath $Folder -Recurse -File | Where-Object { $_.Extension -in '.dll','.exe','.obj','.h','.inc','.lib','.exp' -and $_.FullName -notmatch '[\\/](rtxdi|nrd-deps)[\\/]' -and $_.FullName -notmatch '[\\/]D3D12[\\/]' } |
    ForEach-Object { [pscustomobject]@{name=$_.FullName.Substring($Folder.Length).TrimStart('\','/');sha=(Get-FileHash -LiteralPath $_.FullName).Hash;bytes=$_.Length;time=$_.LastWriteTimeUtc.ToString('o')} } | Sort-Object name
$rows | ConvertTo-Json | Set-Content -LiteralPath $OutFile -Encoding utf8
"files: $($rows.Count)"
