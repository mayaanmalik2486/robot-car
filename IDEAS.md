# Ideas and plans

## Built (2026-10-19)

All of these use only the parts already on the car.

| Mode | Letter | What it does |
|---|---|---|
| **Follow me** | M | Keeps about 25 cm from your hand or feet: follows when you walk away, backs off when you come close, looks around with the servo if it loses you |
| **Goalkeeper + kick** | K, k | Scans for the ball, turns to face it, and punches it away (a quick dash forward and back) when it comes within ~30 cm. Kick = shoot any time |
| **Guard mode** | A | Counts down (5 beeps), learns what the room looks like, then sounds a siren, shows angry eyes and "INTRUDER", turns to face anything that comes in - and the app buzzes your phone |
| **Music (hand theremin)** | N | Your hand's distance plays notes on a nice-sounding (pentatonic) scale |
| **Gesture control** | E | Tap = spin round, double tap = happy wiggle, hold close = it backs away, hold further = it comes to you |
| **Pet mode** | P | Wanders slowly, sits and looks around, gets bored, falls asleep (snoring), wakes up and purrs when you "pet" it (hand held close) |
| **Turbo** | T | A 1.5 s full-power burst with a whoosh sound and "TURBO!" on the screen |
| **Horn** | Y | Beep beeeep (Y was the old demo mode) |
| **Low battery warning** | - | Checks the power by itself (no wires) and warns on the screen, with a sad tune and in the app. Without extra parts it can only notice when the battery is nearly empty |
| **Our own phone app** | - | [App](App): download link, a button for everything, live status from the car |
| **Radar** | O | Stands still, sweeps the sensor 50 degrees each way and draws a radar map on its screen; sends every dot to the app, which draws it too (October 20) |
| **IR remote add-on** | - | [IR_Remote_Addon](IR_Remote_Addon): the remote taken out of the programs, addable back with one file + 3 lines |

Every mode that needs the distance sensor switches it on, even in football mode (like search and chaos).
Each play mode can be switched off (`HAS_FOLLOW`, `HAS_KEEPER`, `HAS_GUARD`, `HAS_MUSIC`, `HAS_GESTURE`,
`HAS_PET` in section 3) to free memory.

### Memory (Arduino UNO: 32,256 bytes)

| | Before | After |
|---|---|---|
| Main program | 29,590 (91.7 %) | 29,742 (92.2 %) - with all the new modes; 30,656 (95.0 %) with radar too |
| Biggest song file | 30,448 (94.4 %) | about 30,600 (95 %); about 31,500 (97.7 %) with radar too |

How it fits: the debug messages were switched off (saved 2.9 KB - the car works the same), the IR remote
was taken out into an add-on (saved 2.1 KB + 430 bytes of RAM) and demo mode was removed (0.5 KB). The new
modes cost about 5.4 KB together: pet 1.0 KB, gestures 0.5 KB, guard 0.5 KB, goalkeeper 0.2 KB, plus shared
code, turbo, battery, the app's status line, eyes and sounds.

To keep things fitting in the future, avoid: pictures / bitmaps on the screen (about 1 KB each), more fonts,
decimal (float) maths while the car runs, the Arduino `String` type, and long debug messages.

## Saved for later

- **Record and replay** - drive a route with the phone, the car repeats it (saved even after switching off)
- **Emoji buttons** - hearts, angry, crying, sunglasses, wink on the screen
- **Horn and sound effects** - more sounds: police siren, reversing beep, laser
- **Games on the screen** - reaction game, "Simon says"
- **Wall follower** - drives along a wall at a fixed distance (mazes)
- **Line follower / table-edge stop** - only if the old line sensors / cliff sensor are still around (free pins A2, A3)
- **A real battery meter** - needs two resistors (a voltage divider into A2)
