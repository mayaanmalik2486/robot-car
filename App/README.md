# Robot Car app (Android)

<img src="icon-512.png" width="96" align="right" alt="app icon">

A remote for this car that looks like a game controller, played sideways (landscape):
a **thumbstick** on the left, four **face buttons** on the right, and a little green **screen**
in the middle that shows what the car is doing, with the mode buttons under it.
Your phone buzzes when guard mode sees an intruder or when the car's battery is nearly empty.
A **real game controller** works too (see below).

## Download

**[⬇ Download RobotCar.apk](https://github.com/mayaanmalik2486/robot-car/raw/main/App/RobotCar.apk)** (45 KB)

Send this link to anyone - they can download it straight onto their phone:
`https://github.com/mayaanmalik2486/robot-car/raw/main/App/RobotCar.apk`
(The app also has a **Share app** button in ❔ help that sends the link.)

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
3. Open **Robot Car**, tap **🔗**, allow the Bluetooth permission, choose **HC-05**.
   Next time it connects by itself.

## The controls

| Control | Letter sent | What the car does |
|---|---|---|
| 🕹 stick up / down | F / B | forward / backward until you let go |
| 🕹 stick left / right | L / R | spin left / right |
| 🕹 stick up-left / up-right | G / H | curve left / right (forward while turning) |
| 🛑 STOP (top right) | S | stops everything |
| 🐇 / 🐢 (top left) | W / Q | fast / slow |
| 🚀 turbo (green) | T | a 1.5 s full-power burst |
| 📯 horn (red) | Y | beep beeeep |
| ⚽ kick (blue) | k | shoots the ball now |
| 💃 dance (yellow) | 4 | dance to the song (tap again ON a beat to line it up) |
| 🔍 search / 😈 chaos | I / 1 | the self-driving modes |
| 🍩 donut / ↩️ 180° / 🅿️ brake | 3 / U / u | |
| ⏫ auto / ⏬ reverse | 2 / J | drives by itself, stops at obstacles |
| 🚶 follow me | M | keeps about 25 cm from you |
| 🥅 keeper | K | goalkeeper: saves the ball |
| 🚨 guard | A | alarm: walk away during the 5 beeps |
| 🎵 music | N | your hand plays notes |
| ✋ gesture | E | tap, double tap, hold close, hold further |
| 🐶 pet | P | a little robot pet |
| ⚽ football | V | the distance sensor off (it never stops for things) |
| 🔊 sound | X | mute / unmute |

Hold any mode button to see its name. Tap ❔ for help, 🔗 to connect, ⏏ to disconnect.

## Game controller (GameSir Cyclone 2, Xbox, PS, ...)

The controller can't talk to the car by itself (the HC-05 only understands phones and computers),
but it can drive **through the phone**: connect the controller to the phone (Bluetooth, or its
USB dongle with a USB-C adapter), open the app, connect to the car - then:

| Controller | Does |
|---|---|
| left stick or D-pad | drive (diagonals = curve) |
| A / B / X / Y | turbo / horn / kick / dance |
| LB / RB | fast-slow / STOP |
| LT / RT | brake / turbo |
| START / SELECT | search / football |
| left stick click | sound on/off |

On the Cyclone 2, use its Android / "Xbox" (X-input) mode if it asks.

The car answers with a status line (`~MODE|1010`: what it is doing, then muted, football,
slow, low battery), which the app shows on its little screen.

## For developers

- `source/` - the app (plain Java, no extra libraries): `MainActivity.java` + `AndroidManifest.xml` + the icon
- `tools/IconGen.java` - draws the icon
- `build-app.ps1` - builds and signs `RobotCar.apk` (needs Java 17 + the Android SDK; the signing key is kept
  outside GitHub, in `Documents\Arduino\robot-car-app-signing`)
