# Robot Car - the web remote (computers)

The same remote as the Android app, in a web page: the same screen, the same smooth steering, the
same GameSir Cyclone 2 map - for **Chrome** or **Edge** on a Windows / Mac / Linux computer or a
Chromebook. It also works with the **keyboard** and the **mouse**.

## Open it

- **On this computer:** double-click `Web\index.html` in the `robot-car` folder (it opens in your
  browser - use Chrome or Edge).
- **From anywhere:** once GitHub Pages is switched on for this repo (repo **Settings > Pages >
  Deploy from a branch > main / (root)**), it is at
  `https://mayaanmalik2486.github.io/robot-car/Web/`.

## Connect to the car

1. Pair the car's **HC-05** with the computer first. Windows: **Settings > Bluetooth & devices >
   Add device > Bluetooth > HC-05**, PIN **1234** (or 0000).
2. Click **Connect** and pick the car in the list (it may be called **HC-05** or a **COM** port).
   Next time it connects without asking.

Safari and Firefox can't do this (no "Web Serial"), and iPhones / iPads can't talk to an HC-05 at
all. On an Android phone, use the app instead - it is made for phones.

## Drive

| With | How |
|---|---|
| **Mouse / finger** | drag the stick (or the two sticks: Settings > 2 sticks) |
| **Keyboard** | **W A S D** or the arrow keys (they speed up smoothly), **Space** STOP, **Enter** dance, **H** horn, **K** kick, **T** turbo, **B** brake, **R** radar, **M** mute, **F** football, **Q** fast / slow |
| **GameSir Cyclone 2** | plug its 2.4 GHz dongle into the computer (light **green**, XInput) or use a USB cable, then press any button so the browser sees it. Left stick = throttle, right stick = steering, and every button as in the [app's table](../App/README.md#the-gamesir-cyclone-2) |

Click the controller button at the top for the live drawing of the controller and to change any
button. **Right-click the green screen** for the link log.

## For developers

- `logic.js` - the driving brain, the controller map, the car's lines and the layout: the same rules
  as the Android app's Java, written again in JavaScript.
- `test-logic.js` - checks `logic.js` against the app's Java: the same layout for 69 screen sizes and
  styles, the same wheel speeds for 882 stick positions, and exactly the same codes at the same
  moments in 6 long random drives. Run `App\tests\run-tests.ps1` first (it writes the Java side),
  then `node Web\test-logic.js`.
- `app.js` - the page itself: Web Serial (the HC-05), the Gamepad API (the controller), the keyboard,
  the sticks, the screen, settings and the controller drawing.
