# Draws the app's main screen to PNG pictures on this PC, with the app's real view code
# (see Preview.java) - to check the look without a phone, and to run every drawing path once.
# The pictures go to Tools\build\pv\out.
# Run:  powershell -ExecutionPolicy Bypass -File App\tests\preview\make-previews.ps1
$ErrorActionPreference = 'Stop'
$ab = "$env:USERPROFILE\android-build"
$env:PATH = "$ab\jdk\bin;$env:PATH"
$app = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$repo = Split-Path $app -Parent
$src = "$app\source\java\io\github\mayaanmalik2486\robotcar"
$work = "$repo\Tools\build\pv"
if (Test-Path $work) { Remove-Item -Recurse -Force $work }
New-Item -ItemType Directory -Force "$work\classes", "$work\out" | Out-Null

& node "$PSScriptRoot\make-shim.js" "$work\shim"
$shim = (Get-ChildItem "$work\shim" -Recurse -Filter *.java).FullName
# the app's own classes that draw (no Activity, no Bluetooth)
$views = @('Car', 'CarState', 'DriveLogic', 'Layout', 'ControllerMap', 'PadState', 'ControllerInput', 'Ui', 'PadButton', 'SceneView',
  'StickView', 'WheelView', 'RadarView', 'ControllerView', 'Boards') | ForEach-Object { "$src\$_.java" }
& javac -encoding UTF-8 -Xlint:-options -d "$work\classes" @shim @views "$PSScriptRoot\Preview.java"
if ($LASTEXITCODE -ne 0) { throw 'the preview did not compile' }
& java -cp "$work\classes" io.github.mayaanmalik2486.robotcar.Preview "$work\out"
if ($LASTEXITCODE -ne 0) { exit 1 }
