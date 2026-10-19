param([switch]$SkipSongs, [string[]]$Only = @(), [string[]]$Scen = @())
# Tests everything in the simulator:
#  - the main program, exactly as uploaded (no debug messages): play modes + the full demo dance
#  - the main program with debug messages (simulator only - too big to upload): the older tests that read them
#  - every song file (as uploaded): the whole-song dance test
# Setup once: Node.js, then "npm install" inside Tools\pkg-avr8js.
# Run: powershell -ExecutionPolicy Bypass -File Tools\tools\test-all.ps1   (results: Tools\build\test-all.txt)
$ErrorActionPreference = 'Continue'
$tools = Split-Path $PSScriptRoot -Parent
$repo = Split-Path $tools -Parent
$work = "$tools\build"
$gen = "$work\gen"
$u = "$env:USERPROFILE\Documents\Arduino\libraries"; $c = "$env:LOCALAPPDATA\Arduino15\packages\arduino\hardware\avr\1.8.8\libraries"
$avr = "$tools\pkg-avr8js\node_modules\avr8js"
$log = "$work\test-all.txt"
New-Item -ItemType Directory -Force $work | Out-Null
"" | Set-Content $log

# 1. make the songs from the main program (into a test folder - the Dances folder is not touched)
Push-Location $tools
if (Test-Path $gen) { Remove-Item -Recurse -Force $gen }
node tools\make-dances.js results\songs.json $gen 2>&1 | Select-String 'WARN|warnings|  ' | ForEach-Object { $_.Line } | Add-Content $log
Pop-Location

# a copy of the main program with a special moment (FEEDBACK! at step 32) for the shout test
$st = "$work\shouttest"; New-Item -ItemType Directory -Force $st | Out-Null
$src = [IO.File]::ReadAllText("$repo\Robot_Car_Main\Robot_Car_Main.ino")
$src = $src.Replace('const char SONG_SHOUT[] PROGMEM = "";', 'const char SONG_SHOUT[] PROGMEM = "FEEDBACK!";')
$src = [regex]::Replace($src, 'const int SONG_SHOUT_STEPS\[3\] = \{[^}]*\};', 'const int SONG_SHOUT_STEPS[3] = { 32, -1, -1 };')
[IO.File]::WriteAllText("$st\shouttest.ino", $src, (New-Object Text.UTF8Encoding $false))

# 2. build
$main = "$repo\Robot_Car_Main\Robot_Car_Main.ino"
$list = @(
  @{ id = 'main'; ino = $main; name = 'Robot_Car_Main'; dbg = $false },
  @{ id = 'maindbg'; ino = $main; name = 'Robot_Car_Main'; dbg = $true },
  @{ id = 'shout'; ino = "$st\shouttest.ino"; name = 'shouttest'; dbg = $true })
$i = 0
if (-not $SkipSongs) {
  foreach ($d in Get-ChildItem $gen -Directory) {
    $i++
    if ($Only.Count -and -not ($Only | Where-Object { $d.Name -like "*$_*" })) { continue }
    $list += @{ id = "s$i"; ino = "$($d.FullName)\$($d.Name).ino"; name = $d.Name; song = $true; dbg = $false }
  }
}
$jobs = @()
foreach ($x in $list) {
  while (@($jobs | Where-Object { $_.State -eq 'Running' }).Count -ge 6) { Start-Sleep -Milliseconds 500 }
  $jobs += Start-Job -ArgumentList $tools, $work, $x.ino, $x.id, $u, $c, $x.dbg -ScriptBlock {
    param($tools, $work, $ino, $id, $u, $c, $dbg)
    $extra = @(); if ($dbg) { $extra = @('--define', 'DEBUG_MESSAGES=1') }
    node "$tools\tools\build.js" --sketch $ino --out "$work\bin\$id" --lib "$u\Servo" --lib "$u\U8g2" --lib "$c\SoftwareSerial" --lib "$c\Wire" --lib "$c\SPI" @extra 2>&1 | Out-String
  }
}
$jobs | Wait-Job | Out-Null
$b = ($jobs | Receive-Job) -join ""
$jobs | Remove-Job
($b -split "`n" | Where-Object { $_ -match '^OK|error|Error' }) | Add-Content $log
"BUILT: $(@($b -split "`n" | Where-Object { $_ -match '^OK' }).Count) of $($list.Count)" | Add-Content $log

# 3. simulate
$rel = @('dance-full-song', 'curve-sharper', 'turbo-horn-status', 'radar', 'app-session', 'follow-me', 'goalkeeper', 'guard', 'music', 'gestures', 'pet')
$dbg = @('dance-tap-resync', 'dance-hold-button', 'football-mode', 'stop-dance-forward', 'stop-dance-brake', 'stop-dance-180', 'stop-search-backward',
  'faces-donut-dizzy-180', 'mute-button', 'search-stuck-no-echo', 'dance-wall-in-front')
if ($Scen.Count) { $rel = @($rel | Where-Object { $Scen -contains $_ }); $dbg = @($dbg | Where-Object { $Scen -contains $_ }) }
$runs = @()
foreach ($sc in $rel) { $runs += @{ label = "main $sc"; ino = $main; bin = "$work\bin\main\Robot_Car_Main"; sc = $sc } }
foreach ($sc in $dbg) { $runs += @{ label = "maindbg $sc"; ino = $main; bin = "$work\bin\maindbg\Robot_Car_Main"; sc = $sc } }
if (-not $Scen.Count -or $Scen -contains 'shout-moment') { $runs += @{ label = 'shout shout-moment'; ino = $list[2].ino; bin = "$work\bin\shout\shouttest"; sc = 'shout-moment' } }
foreach ($x in ($list | Where-Object { $_.song })) { $runs += @{ label = "song $($x.name)"; ino = $x.ino; bin = "$work\bin\$($x.id)\$($x.name)"; sc = 'dance-full-song' } }
$jobs = @()
foreach ($r in $runs) {
  while (@($jobs | Where-Object { $_.State -eq 'Running' }).Count -ge 6) { Start-Sleep -Seconds 2 }
  $jobs += Start-Job -ArgumentList $tools, $avr, $work, $r.ino, $r.bin, $r.sc, $r.label -ScriptBlock {
    param($tools, $avr, $work, $ino, $bin, $sc, $label)
    $env:AVR8JS = $avr; $env:SKETCH = $ino; $env:ROOM = '0'
    $o = node "$tools\sim\run.js" "$tools\sim\scenarios.js" "$work\sim-out\$($label -replace '\W','_')" $bin $sc 2>&1 | Out-String
    "##### $label`n$o"
  }
}
$jobs | Wait-Job | Out-Null
(($jobs | Receive-Job) -join "`n") -split "`n" | Where-Object { $_ -match '#####|===|PASS|FAIL|ALL CHECKS|Error' } | Add-Content $log
$jobs | Remove-Job -Force
$t = Get-Content $log
"PASS lines: $(@($t | Where-Object { $_ -match '^\s*PASS' }).Count)   FAIL lines: $(@($t | Where-Object { $_ -match '^\s*FAIL' }).Count)" | Add-Content $log
"DONE" | Add-Content $log
Get-Content $log | Select-Object -Last 2
