param([string]$Only = '', [int]$Parallel = 6, [string[]]$Ids = @())
# Builds every generated song sketch and runs the full-song dance test on each.
$s = Split-Path $PSScriptRoot -Parent
$u = "$env:USERPROFILE\Documents\Arduino\libraries"; $c = "$env:LOCALAPPDATA\Arduino15\packages\arduino\hardware\avr\1.8.8\libraries"
$songs = Get-Content "$s\results\songs-made.json" -Raw | ConvertFrom-Json
if ($Only) { $songs = $songs | Where-Object { $_.id -eq $Only } }
if ($Ids.Count) { $songs = $songs | Where-Object { $Ids -contains $_.id } }
$out = "$s\results\songs-test.txt"
"" | Set-Content $out
foreach ($song in $songs) {
  $b = node "$s\tools\build.js" --sketch $song.file --out "$s\build\s\$($song.id)" --lib "$u\Servo" --lib "$u\IRremote" --lib "$u\U8g2" --lib "$c\SoftwareSerial" --lib "$c\Wire" --lib "$c\SPI" 2>&1 | Out-String
  "BUILD $($song.folder): $($b.Trim())" | Add-Content $out
}
$jobs = @()
foreach ($song in $songs) {
  while (@($jobs | Where-Object { $_.State -eq 'Running' }).Count -ge $Parallel) { Start-Sleep -Seconds 5 }
  $bin = "$s\build\s\$($song.id)\$($song.folder)"
  $jobs += Start-Job -Name $song.folder -ArgumentList $s, $song.file, $bin, $song.folder -ScriptBlock {
    param($s, $ino, $bin, $name)
    $env:AVR8JS = "$s\pkg-avr8js\node_modules\avr8js"; $env:SKETCH = $ino; $env:ROOM = '0'
    $o = node "$s\sim\run.js" "$s\sim\scenarios.js" "$s\results\s\$name" $bin 'dance-full-song' 2>&1 | Out-String
    "##### $name $o"
  }
}
$jobs | Wait-Job | Out-Null
($jobs | Receive-Job) -join "`n" | Select-String -Pattern '#####|===|PASS|FAIL' -AllMatches | ForEach-Object { $_.Line } | Add-Content $out
$jobs | Remove-Job -Force
Get-Content $out
