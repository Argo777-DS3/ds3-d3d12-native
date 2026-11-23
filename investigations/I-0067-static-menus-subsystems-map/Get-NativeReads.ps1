# I-0067: read-only inventory of native Read(address,&variable,size) calls. Root = a folder holding the native/ sources of the pilot branch
# (e.g. extracted with git show); writes a TSV with the declared destination type and flags size/type mismatches.
param([Parameter(Mandatory)][string]$Root, [Parameter(Mandatory)][string]$Out)
# Lists every Read(addr,&var,size) in the native sources with the declared type of var and flags size/type mismatches.
$sizes = @{ 'uint8_t'=1;'BYTE'=1;'bool'=1;'char'=1;'int8_t'=1;'uint16_t'=2;'int16_t'=2;'WORD'=2;'wchar_t'=2;
  'uint32_t'=4;'int32_t'=4;'DWORD'=4;'float'=4;'int'=4;'LONG'=4;'UINT'=4;'unsigned'=4;
  'uintptr_t'=8;'uint64_t'=8;'int64_t'=8;'LONG64'=8;'double'=8;'HWND'=8;'size_t'=8;'ULONGLONG'=8 }
$typeAlt = ($sizes.Keys | Sort-Object Length -Descending | ForEach-Object { [regex]::Escape($_) }) -join '|'
$rows = [Collections.Generic.List[object]]::new()
foreach ($f in Get-ChildItem -Recurse -File $Root -Include *.cpp,*.h,*.inc) {
  $lines = [IO.File]::ReadAllLines($f.FullName)
  for ($i = 0; $i -lt $lines.Count; $i++) {
    foreach ($m in [regex]::Matches($lines[$i], 'Read\(\s*([^,;()]+(?:\([^()]*\))?[^,;()]*)\s*,\s*&\s*([A-Za-z_]\w*)\s*,\s*(sizeof\(\w+\)|\d+|0x[0-9A-Fa-f]+)\s*\)')) {
      $var = $m.Groups[2].Value; $size = $m.Groups[3].Value; $type = ''
      for ($j = $i; $j -ge [Math]::Max(0, $i - 120) -and -not $type; $j--) {
        $d = [regex]::Match($lines[$j], "\b($typeAlt)\b(?:\s*\*?\s*[A-Za-z_]\w*(?:\[[^\]]*\])?\s*(?:\{[^}]*\}|=[^,;]*)?\s*,)*\s*\b$var\b\s*(?:\{|=|;|,|\[)")
        if ($d.Success) { $type = $d.Groups[1].Value }
      }
      $n = if ($size -match '^sizeof') { 'sizeof' } elseif ($size -match '^0x') { [Convert]::ToInt32($size.Substring(2),16) } else { [int]$size }
      $flag = ''
      if ($type -and $n -ne 'sizeof' -and $sizes[$type] -ne $n) { $flag = "SIZE $n vs $type" }
      $rows.Add([pscustomobject]@{ file = $f.FullName.Substring($Root.Length + 1).Replace('\','/'); line = $i + 1; addr = $m.Groups[1].Value.Trim(); var = $var; type = $type; size = $size; flag = $flag })
    }
  }
}
$rows | Export-Csv -Delimiter "`t" -NoTypeInformation -LiteralPath $Out
"reads $($rows.Count); flagged $(@($rows | Where-Object flag).Count); untyped $(@($rows | Where-Object { -not $_.type }).Count)"
