# Robot Car (Arduino UNO)

A 4-wheel-drive Arduino robot car. You drive it from a phone (Bluetooth) or a small IR remote.
It stops before it hits things, searches for open space by itself, does donuts, drifts and
"chaos" mode, shows eyes on a little OLED screen, and **dances to 38 songs** in time with the
music while the buzzer plays along.

## What's in this repo

| Folder | What it is |
|---|---|
| [`Robot_Car_Main`](Robot_Car_Main) | **The main robot program** (latest version). Upload this one for normal use. Also has the full [wiring and power guide](Robot_Car_Main/POWER_AND_WIRING.md). |
| [`Dances`](Dances) | 38 copies of the main program, each with a dance made for one song. The [song list and how to dance in time](Dances/README.md) are in that folder. |
| [`Old_Versions`](Old_Versions) | Every earlier version of the code, oldest first (from the first servo test up to v7.1). |
| [`Tools`](Tools) | Computer scripts that made and tested the dances in a simulator. **You don't need these** to use the car. |

## How to put a program on the car

1. On this page, click the green **Code** button, then **Download ZIP**, and unzip it.
2. Open the `.ino` file you want in the **Arduino IDE** (for example `Robot_Car_Main/Robot_Car_Main.ino`).
3. The first time only, install these libraries in the IDE (**Tools > Manage Libraries**):
   - **IRremote** by Armin Joachimsmeyer (version 4.x)
   - **U8g2** by oliver

   Servo, SoftwareSerial and Wire come with the Arduino IDE.
4. Pick **Tools > Board > Arduino Uno** and your USB port, then click **Upload**.

## Controls

**Phone app** (any "Bluetooth RC car" app through the HC-05):

| Button | Does | Button | Does |
|---|---|---|---|
| F / B | forward / backward | L / R | spin left / right |
| G / H | drift left / right | S | stop |
| Q / W | slow / full speed | 2 / J | auto forward / auto backward (stops at obstacles) |
| 3 | donut | U | 180-degree turn |
| u | smooth brake | 4 | dance (press again ON a beat to line it up) |
| Y | demo | 1 | chaos |
| I | search (looks around and drives to open space) | X ("!") | mute / unmute every sound |

**IR remote:**

| Button | Does | Button | Does |
|---|---|---|---|
| OK | stop everything | UP / DOWN | forward (only if clear) / backward |
| LEFT / RIGHT | short turn | 1 | 180-degree turn |
| 2 | slow / full speed | 3 | smooth brake |
| 4 | demo | 5 | dance (again = line up with the beat) |
| 6 | chaos | 7 / 8 | drift left / right |
| 9 | search | * / # | auto forward / auto backward |
| 0 | donut | | |

## Parts and wiring

Arduino UNO + Sensor Shield V5, L298N motor driver, 4 DC motors, HC-05 Bluetooth,
IR receiver, SG90 servo with an HC-SR04 ultrasonic sensor, passive buzzer, SSD1306 OLED.
Power: 3 x 18650 cells for the motors and Arduino (through the L298N's 5 V), plus 4 x AA for the servo.

| Part | Pins |
|---|---|
| L298N | ENA D5, ENB D6, IN1 D2, IN2 D4, IN3 D7, IN4 D10 |
| HC-05 | TXD to D3, RXD to A0 |
| IR receiver | D8 |
| Servo | D9 |
| HC-SR04 | TRIG D11, ECHO D12 |
| Buzzer | A1 |
| OLED (I2C) | SDA A4, SCL A5 |

Every wire, step by step: [POWER_AND_WIRING.md](Robot_Car_Main/POWER_AND_WIRING.md).
