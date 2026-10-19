# Robot Car app (Android)

<img src="icon-512.png" width="96" align="right" alt="app icon">

A remote control made for this car: hold-to-drive arrows, a button for every mode,
and a live panel that shows what the car is doing. Your phone buzzes when guard mode
sees an intruder or when the car's battery is nearly empty.

## Download

**[⬇ Download RobotCar.apk](https://github.com/mayaanmalik2486/robot-car/raw/main/App/RobotCar.apk)** (41 KB)

Send this link to anyone - they can download it straight onto their phone:
`https://github.com/mayaanmalik2486/robot-car/raw/main/App/RobotCar.apk`
(The app also has a **Share this app** button that sends the link.)

Works on **Android phones** (Android 5 and newer). It does **not** work on iPhones: Apple doesn't
let apps talk to HC-05-type Bluetooth modules.

## Install

1. Open the link on the phone and download `RobotCar.apk`. Chrome may warn that this kind of
   file can be harmful - tap **Download anyway** (it's this app, made for your car).
2. Tap the downloaded file. Android asks to allow installing apps from Chrome (or Files):
   tap **Settings**, switch **Allow from this source** on, go back, tap **Install**.
3. If **Play Protect** says it doesn't know the app: tap **More details > Install anyway**.
   (It's not on the Play Store, so Google hasn't scanned it.)

## First time

1. Switch the car on.
2. On the phone: **Settings > Bluetooth > pair new device > HC-05**. The PIN is **1234** (or **0000**).
3. Open **Robot Car**, tap **Connect**, allow the Bluetooth permission, choose **HC-05**.
   Next time it connects by itself.

## The buttons

| Button | Letter sent | What the car does |
|---|---|---|
| ▲ ▼ (hold) | F / B | forward / backward until you let go |
| ↺ ↻ (hold) | L / R | spin left / right |
| ↖ ↗ (hold) | G / H | curve left / right (forward while turning) |
| STOP | S | stops everything |
| Slow / Full | Q / W | speed |
| Turbo | T | a 1.5 s full-power burst |
| Horn | Y | beep beeeep |
| Dance | 4 | dance to the song (tap again ON a beat to line it up) |
| Search / Chaos | I / 1 | the self-driving modes |
| Donut / 180° turn / Brake | 3 / U / u | |
| Auto forward / back | 2 / J | drives by itself, stops at obstacles |
| Follow me | M | keeps about 25 cm from you |
| Goalkeeper / KICK! | K / k | saves the ball / shoots now |
| Guard | A | alarm: walk away during the 5 beeps |
| Music | N | your hand plays notes |
| Gestures | E | tap, double tap, hold close, hold further |
| Pet | P | a little robot pet |
| Football | V | the distance sensor off (it never stops for things) |
| Sound | X | mute / unmute |

The car answers with a status line (`~MODE|1010`: what it is doing, then muted, football,
slow, low battery), which the app shows at the top.

## For developers

- `source/` - the app (plain Java, no extra libraries): `MainActivity.java` + `AndroidManifest.xml` + the icon
- `tools/IconGen.java` - draws the icon
- `build-app.ps1` - builds and signs `RobotCar.apk` (needs Java 17 + the Android SDK; the signing key is kept
  outside GitHub, in `Documents\Arduino\robot-car-app-signing`)
