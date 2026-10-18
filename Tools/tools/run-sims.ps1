param([string]$Build, [string[]]$Scenarios, [string]$Sketch = 'C:\Users\DELL\Documents\Arduino\v7.1inthemaking_copy_20260802191438_copy_20260803160702\v7.1inthemaking_copy_20260802191438_copy_20260803160702.ino', [string]$Bin = '')
# Runs scenarios from sim\scenarios.js in parallel against build\<Build>\<name>.bin and prints PASS/FAIL lines.
$s = Split-Path $PSScriptRoot -Parent
if (-not $Bin) { $n = [IO.Path]::GetFileNameWithoutExtension($Sketch); $Bin = "$s\build\$Build\$n" }
$jobs = @()
foreach ($sc in $Scenarios) {
  $jobs += Start-Job -ArgumentList $s, $Sketch, $Bin, $Build, $sc -ScriptBlock {
    param($s, $ino, $bin, $b, $sc)
    $env:AVR8JS = "$s\pkg-avr8js\node_modules\avr8js"; $env:SKETCH = $ino; $env:ROOM = '1'
    $o = node "$s\sim\run.js" "$s\sim\scenarios.js" "$s\results\all-$b" $bin $sc 2>&1 | Out-String
    "##### $b $o"
  }
}
$jobs | Wait-Job -Timeout 900 | Out-Null
($jobs | Receive-Job) -join "`n" | Select-String -Pattern '#####|===|PASS|FAIL' -AllMatches | ForEach-Object { $_.Line }
$jobs | Remove-Job -Force
