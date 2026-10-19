# IR remote add-on

The car's programs don't include the IR remote any more (to make room for the play modes).
This folder adds it back to **any** of them - the main program or any song file - in 4 small steps.
Every remote button "presses" a phone-app letter, so the remote does what the app does.

## The remote buttons

| Button | Does | Button | Does |
|---|---|---|---|
| UP / DOWN | forward / backward (while held) | LEFT / RIGHT | spin (while held) |
| OK | stop everything | 1 | 180-degree turn |
| 2 | slow / full speed | 3 | smooth brake |
| 4 | turbo | 5 | dance (again = line up with the beat) |
| 6 | chaos | 7 / 8 | curve left / right |
| 9 | search | * / # | auto forward / auto backward |
| 0 | donut | | |

Want a different job for a button? Change its letter in `IR_Remote_Addon.h` (`irLetter()`).
The letters are listed at the top of the main program ("BLUETOOTH LETTERS").

## Wiring

IR receiver on the sensor shield's row **8**: signal (S) to **D8**, V to 5 V, G to GND.

## Library

Arduino IDE: **Tools > Manage Libraries**, search **IRremote**, install it (by Armin Joachimsmeyer, version 4.x).

## Test the remote first (optional)

Upload `IR_Remote_Addon.ino` (this folder) on its own, open the Serial Monitor at **115200** baud and press the
buttons: it shows each button and the letter it sends. The car doesn't move with this test.

## Add it to a program - 4 steps

**1.** Copy `IR_Remote_Addon.h` into the program's folder, next to its `.ino` file
(for example into `Robot_Car_Main`). Close and reopen the program in the Arduino IDE: it shows as a second tab.

**2.** Near the top, under the other `#include` lines, add:

```cpp
#include "IR_Remote_Addon.h"  // the IR remote add-on
```

**3.** In `setup()`, add `irSetup();` on the line **just before** `setupBuzzer();`:

```cpp
    irSetup();
    setupBuzzer();             // (the IR add-on's IrReceiver.begin() goes just before this line)
```

**4.** In `loop()`, under `handleBluetooth();`, add:

```cpp
    irUpdate();                // remote commands
```

Upload. Done.

## "Sketch too big"?

The IR library is about 2 KB. With it:

| Program | Size with the remote | |
|---|---|---|
| Robot_Car_Main | 31,594 bytes (97.9 %) | fits as it is |
| the biggest song file (Get Lucky) | 32,410 bytes (100.5 %) | too big: switch the pet off (below) |
| that song file with the pet off | 31,348 bytes (97.2 %) | fits |

To make room, switch a play mode off near the top of the program (section 3):
`const bool HAS_PET = true;` → `false` frees about 1 KB (that's enough for every song file).
`HAS_GESTURE` frees about 0.5 KB, `HAS_GUARD` about 0.5 KB.
