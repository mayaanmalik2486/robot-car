# Tests the parts of the app that need no phone (plain Java, on this PC):
#   LogicTests  - the car's language, what the car says, the driving brain, the controller map, button state
#   LayoutTests - where everything sits on the screen, for phones, tablets and windows of many sizes
#   api-check   - every Android call against Android 5 (the newer ones are listed: each is behind a version check)
# It also writes Tools\build\app-replay.json: a drive by the real driving brain, which the car
# simulator then plays to the real car program (scenario "app-replay").
# Run:  powershell -ExecutionPolicy Bypass -File App\tests\run-tests.ps1
$ErrorActionPreference = 'Stop'
$ab = "$env:USERPROFILE\android-build"
$env:JAVA_HOME = "$ab\jdk"
$env:PATH = "$ab\jdk\bin;$env:PATH"
$app = Split-Path $PSScriptRoot -Parent
$repo = Split-Path $app -Parent
$src = "$app\source\java\io\github\mayaanmalik2486\robotcar"
$out = "$app\build-tests"
if (Test-Path $out) { Remove-Item -Recurse -Force $out }
New-Item -ItemType Directory -Force $out | Out-Null

# only the classes without Android in them
$pure = @('Car', 'CarState', 'DriveLogic', 'ControllerMap', 'PadState', 'Layout') | ForEach-Object { "$src\$_.java" }
$tests = (Get-ChildItem $PSScriptRoot -Filter *.java).FullName
& javac -encoding UTF-8 -Xlint:-options -d $out @pure @tests
if ($LASTEXITCODE -ne 0) { throw 'the tests did not compile' }

$bad = 0
foreach ($t in $tests) {
  $name = [IO.Path]::GetFileNameWithoutExtension($t)
  Write-Host "`n##### $name"
  & java -cp $out "io.github.mayaanmalik2486.robotcar.$name" $repo
  if ($LASTEXITCODE -ne 0) { $bad++ }
}
# every Android call checked against Android 5 (API 21): the newer ones are listed, to be checked by hand
Write-Host "`n##### Android versions"
$all = (Get-ChildItem "$app\source\java" -Recurse -Filter *.java).FullName
$bt = "$ab\sdk\build-tools\34.0.0"
$jar = "$ab\sdk\platforms\android-34\android.jar"
New-Item -ItemType Directory -Force "$out\apk" | Out-Null
& javac -encoding UTF-8 -source 8 -target 8 -g -Xlint:-options -bootclasspath "$jar;$bt\core-lambda-stubs.jar" -d "$out\apk" @all
& node "$PSScriptRoot\api-check.js" "$out\apk" "$ab\sdk\platforms\android-34\data\api-versions.xml" "$ab\jdk\bin\javap.exe"
Remove-Item -Recurse -Force $out

# the web page's JavaScript brain against the Java one (Golden wrote the Java side)
Write-Host "`n##### Web\test-logic.js"
& node "$repo\Web\test-logic.js"
if ($LASTEXITCODE -ne 0) { $bad++ }
if ($bad) { Write-Host "`nTESTS FAILED"; exit 1 }
Write-Host "`nALL TESTS PASSED"
