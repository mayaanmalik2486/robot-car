# Builds the Robot Car app: App\RobotCar.apk
# Needs (already set up on this PC): Java 17 + the Android SDK in C:\Users\DELL\android-build,
# and the signing key in Documents\Arduino\robot-car-app-signing (kept out of GitHub).
# Run:  powershell -ExecutionPolicy Bypass -File App\build-app.ps1
$ErrorActionPreference = 'Stop'
$ab = "$env:USERPROFILE\android-build"
$bt = "$ab\sdk\build-tools\34.0.0"
$jar = "$ab\sdk\platforms\android-34\android.jar"
$key = "$env:USERPROFILE\Documents\Arduino\robot-car-app-signing"
$env:JAVA_HOME = "$ab\jdk"
$env:PATH = "$ab\jdk\bin;$env:PATH"

$src = Join-Path $PSScriptRoot 'source'
$out = Join-Path $PSScriptRoot 'build'
$apk = Join-Path $PSScriptRoot 'RobotCar.apk'
if (Test-Path $out) { Remove-Item -Recurse -Force $out }
New-Item -ItemType Directory -Force "$out\classes" | Out-Null

function Run($exe, [string[]]$argv) {
  & $exe @argv
  if ($LASTEXITCODE -ne 0) { throw "failed: $exe $argv" }
}

Write-Host '1/6 resources (icon + manifest)'
Run "$bt\aapt2.exe" @('compile', '--dir', "$src\res", '-o', "$out\res.zip")
Run "$bt\aapt2.exe" @('link', '-o', "$out\app.apk", '-I', $jar, '--manifest', "$src\AndroidManifest.xml", "$out\res.zip")

Write-Host '2/6 compile the Java code'
$java = (Get-ChildItem "$src\java" -Recurse -Filter *.java).FullName
Run 'javac' (@('-encoding', 'UTF-8', '-source', '8', '-target', '8', '-Xlint:-options', '-bootclasspath', "$jar;$bt\core-lambda-stubs.jar", '-d', "$out\classes") + $java)

Write-Host '3/6 turn it into Android code (dex)'
$classes = (Get-ChildItem "$out\classes" -Recurse -Filter *.class).FullName
Run "$bt\d8.bat" (@('--release', '--lib', $jar, '--min-api', '21', '--output', $out) + $classes)

Write-Host '4/6 put it in the app'
Push-Location $out
Run "$bt\aapt.exe" @('add', 'app.apk', 'classes.dex')
Pop-Location

Write-Host '5/6 align + sign'
Run "$bt\zipalign.exe" @('-f', '-p', '4', "$out\app.apk", "$out\aligned.apk")
Run "$bt\apksigner.bat" @('sign', '--ks', "$key\robotcar.jks", '--ks-pass', "file:$key\password.txt", '--out', $apk, "$out\aligned.apk")

Write-Host '6/6 check'
Run "$bt\apksigner.bat" @('verify', '--verbose', $apk)
Run "$bt\aapt2.exe" @('dump', 'badging', $apk)
Remove-Item -Recurse -Force $out
Write-Host "DONE: $apk ($([math]::Round((Get-Item $apk).Length / 1KB)) KB)"
