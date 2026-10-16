# Robot car: complete wiring guide (every part)

This is how every wire on the robot should go, matching the code. Parts marked
**(already wired)** were working before; if yours is wired the same way, leave it.

On the **Sensor Shield V5**, every pin has a row of 3 pins:
**S** = signal, **V** = +5 V, **G** = ground. The letters are printed on the shield.
Digital pins D0–D13 are the long rows on the right; analog pins A0–A5 are the rows
above the "ANALOG IN 0 1 2 3 4 5" label at the bottom left.

---

## 1. Power

### Main battery: 3×18650 (powers motors + Arduino) (already wired)
| From | To |
|---|---|
| 18650 pack **+** | on/off switch → L298N **+12V / VS** screw terminal |
| 18650 pack **−** | L298N **GND** screw terminal |
| L298N **GND** screw terminal | Arduino **GND** (the grounds must be joined) |
| L298N **+5V** screw terminal | Arduino **5V** pin (this powers the Arduino) |
| L298N **5V-EN jumper** (next to the terminals) | leave it **ON** |

### Servo battery: 4×AA (powers ONLY the servo)
| From | To |
|---|---|
| AA pack **red (+)** | servo **red** wire (joined on a mini breadboard or twisted + taped) |
| AA pack **black (−)** | **A2 row, G pin** (joins the grounds) |

The servo's red wire never touches the shield. The jumper next to the shield's blue
screw terminal stays as it is, and nothing goes on the blue terminal.

#### Can I use 4×1.2 V rechargeable (NiMH) cells instead of 4×1.5 V?
**Yes, and they're actually a little better.**
- 4×1.2 V = 4.8 V (about 5.6 V right after charging). The SG90 is made for 4.8–6 V,
  so this is its normal voltage.
- The servo will be a tiny bit slower and weaker than on fresh 1.5 V cells. You won't
  notice it moving a light sensor.
- NiMH cells keep a steadier voltage and handle the servo's current spikes better than
  alkaline cells, which sag as they empty.
- Use 2000 mAh or bigger cells, all 4 the same brand and age, and charge them with a NiMH charger.

---

## 2. Motor driver (L298N) (already wired)

Take off the two small jumper caps on **ENA** and **ENB** on the L298N (the wires go there instead).

| L298N pin | Arduino pin | Shield |
|---|---|---|
| **ENA** (left wheels speed) | D5 | D5 row **S** |
| **IN1** | D2 | D2 row **S** |
| **IN2** | D4 | D4 row **S** |
| **IN3** | D7 | D7 row **S** |
| **IN4** | D10 | D10 row **S** |
| **ENB** (right wheels speed) | D6 | D6 row **S** |

| L298N output | Motors |
|---|---|
| **OUT1 + OUT2** | both **left** motors (front-left and rear-left, wired side by side) |
| **OUT3 + OUT4** | both **right** motors |

If one side spins the wrong way when you press F, swap that side's two wires at the
OUT terminal.

---

## 3. Bluetooth (HC-05) (already wired)

| HC-05 pin | To |
|---|---|
| **VCC** | a **V** pin (5 V), e.g. D3 row **V** |
| **GND** | a **G** pin, e.g. D3 row **G** |
| **TXD** | **D3 row S** (the Arduino listens here) |
| **RXD** | **A0 row S** (the Arduino talks here). Best with a 1 kΩ + 2 kΩ divider, see note |
| **STATE / EN** | not used |

Note: the HC-05's RXD pin likes 3.3 V. The safe way is a 1 kΩ resistor from A0 to RXD
and a 2 kΩ resistor from RXD to GND. The robot never sends anything to the phone,
so it works without the divider too; if yours works, leave it.

Because it's on D3/A0 (not pins 0/1), you **don't** need to unplug it to upload.

---

## 4. IR receiver (already wired)

| IR receiver pin | To |
|---|---|
| **OUT / S** | **D8 row S** |
| **VCC / +** | **D8 row V** |
| **GND / −** | **D8 row G** |

The "L" LED on the Arduino flickers when it sees the remote.

---

## 5. Ultrasonic sensor (HC-SR04) (already wired)

| HC-SR04 pin | To |
|---|---|
| **VCC** | **D11 row V** |
| **Trig** | **D11 row S** |
| **Echo** | **D12 row S** |
| **GND** | **D12 row G** |

It's mounted on the servo horn, facing forward when the servo is at the centre (90°).
Leave a loose loop in its 4 wires so they don't pull when it turns.

---

## 6. Servo (SG90)

Push 3 male-to-female jumper wires into the servo's plug (don't plug it straight onto the shield).

| Servo wire | To |
|---|---|
| **orange / yellow** (signal) | **D9 row S** |
| **brown / black** (ground) | **D9 row G** |
| **red** (+) | **AA pack red (+)**, NOT the shield |

D9's **V** pin stays empty.

If in SEARCH mode it looks **right first** instead of left, swap the numbers in the
code settings: `SERVO_LEFT = 40;` and `SERVO_RIGHT = 140;`.

---

## 7. OLED screen (0.96", I2C)

Your OLED's pins are in the order **GND, VCC, SCL, SDA**.

| OLED pin | To |
|---|---|
| **GND** | **A4 row G** |
| **VCC** | **A4 row V** |
| **SCL** | **A5 row S** |
| **SDA** | **A4 row S** |

SDA goes to A4 and SCL goes to A5. Swapping them gives a blank screen and
`OLED not found` in the Serial Monitor.

---

## 8. Buzzer (3-pin passive buzzer board)

| Buzzer pin | To |
|---|---|
| **S** (I/O, SIG) | **A1 row S** |
| **+** (VCC, often the middle pin) | **A1 row V** |
| **−** (GND) | **A1 row G** |

If the buzzer board gets warm or clicks when it should be quiet, change
`BUZZER_OFF_LEVEL` to `HIGH` in the code settings.

---

## 9. Everything at a glance

| Arduino pin | Shield row | What's on it |
|---|---|---|
| D0, D1 | — | free (USB upload + Serial Monitor) |
| D2 | S | L298N IN1 |
| D3 | S (+V, G) | HC-05 TXD (+ HC-05 power) |
| D4 | S | L298N IN2 |
| D5 | S | L298N ENA |
| D6 | S | L298N ENB |
| D7 | S | L298N IN3 |
| D8 | S, V, G | IR receiver |
| D9 | S, G (V empty) | servo signal + ground |
| D10 | S | L298N IN4 |
| D11 | S, V | HC-SR04 Trig + VCC |
| D12 | S, G | HC-SR04 Echo + GND |
| D13 | — | on-board "L" LED (IR activity) |
| A0 | S | HC-05 RXD |
| A1 | S, V, G | buzzer |
| A2 | G | AA pack − (servo ground) |
| A3 | — | free (left floating: its noise is used to randomise CHAOS mode) |
| A4 | S, V, G | OLED SDA + power |
| A5 | S | OLED SCL |

```
   3x18650 --switch--> L298N 12V          L298N OUT1/OUT2 --> left motors
                        L298N GND -------- Arduino GND        L298N OUT3/OUT4 --> right motors
                        L298N 5V  -------> Arduino 5V
                        ENA IN1 IN2 IN3 IN4 ENB --> D5 D2 D4 D7 D10 D6 (S pins)

   +--------------------- Sensor Shield V5 -----------------------+
   | D3  S<- HC-05 TXD   V,G -> HC-05 power   A0 S -> HC-05 RXD    |
   | D8  S,V,G <- IR receiver                                      |
   | D9  S <- servo orange   G <- servo brown   (V empty)          |
   | D11 S <- HC-SR04 Trig  V <- VCC    D12 S <- Echo  G <- GND     |
   | A1  S,V,G <- buzzer                                           |
   | A4  S <- OLED SDA  V <- OLED VCC  G <- OLED GND               |
   | A5  S <- OLED SCL                                             |
   | A2  G <------------------------------------+                  |
   +--------------------------------------------|------------------+
                                                |
   4xAA pack:  (-) black -----------------------+
               (+) red   ---------> servo red wire (NOT the shield)
```

---

## 10. Safe test order

Put the car on a box so the wheels are off the ground for steps 1–6.

1. Upload with only USB connected. Serial Monitor at **115200**: `1 … 7`, `OLED ready`, `HELLO`, `READY`.
2. AA pack on, press reset: the servo waves and stops facing forward.
3. OLED shows Hello!, then "BT:-- SPEED:FULL / READY / xx cm"; your hand changes the cm number.
4. The fanfare plays at every reset.
5. Remote: press 2, and you should see `IR button 0x46` / `Slow Mode ON` and SPEED:SLOW on the OLED.
6. Phone: connect and press a button, and you should get the "Phone connected!" screen + jingle.
7. On the floor, slow speed (Q) first, then try SEARCH (I) and CHAOS (1).

---

## 11. Safety

- Li-ion 18650s can catch fire if shorted, punctured, over-charged or run completely flat.
  Use a proper charger, don't leave them charging alone, stop using damaged or puffy cells,
  and switch the car off when you're done.
- Red = +, black = −. Check before connecting any battery; backwards can destroy the servo instantly.
- Switch everything off before plugging or unplugging wires.
- If the robot restarts at random, or the little regulator on the L298N gets too hot to touch,
  the L298N's 5 V supply is overloaded. Tell me and I'll show you a cheap fix.

---

## 12. Libraries (Arduino IDE → Tools → Manage Libraries)

IRremote 4.7.1 ✓, U8g2 2.36.19 ✓, Servo ✓, SoftwareSerial / Wire / SPI (come with the IDE) ✓.
All already installed. Memory used: flash 77%, RAM 60%.
