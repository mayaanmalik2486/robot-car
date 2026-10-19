# Ideas and plans

## Next: new modes (planned 2026-10-19)

All of these use only the parts already on the car.

| Mode | What it does |
|---|---|
| **Follow me** | Keeps about 25 cm from your hand or feet: follows when you walk away, backs off when you come close, looks around with the servo if it loses you |
| **Goalkeeper + kick** | Scans for the ball, turns to face it, and punches it away (a quick dash forward and back) when it comes within ~30 cm. Kick button to shoot any time |
| **Guard mode** | Stands still, learns what the room looks like, then sounds a siren, shows angry eyes and "INTRUDER!" and turns to face anything that comes in |
| **Hand theremin** | Your hand's distance plays notes on a nice-sounding scale - a musical instrument |
| **Gesture control** | Hand tap = spin, double tap = happy wiggle, push = back away, pull = come forward |
| **Pet mode** | Wanders slowly, gets bored, falls asleep, wakes up when you wave, purrs when you "pet" it (hand held close) |
| **Turbo** | A 1.5 s full-power burst with a whoosh sound and "TURBO!" on the screen |
| **Low battery warning** | Checks the power by itself (no wires) and warns on the screen with a beep |

Every mode that needs the distance sensor switches it on, even in football mode (like search and chaos).

### Memory plan (Arduino UNO: 32,256 bytes)

| | Main program | Biggest song file (Get Lucky) |
|---|---|---|
| Used now | 29,590 (91.7 %) | 30,448 (94.4 %) |
| Free now | 2,666 | 1,808 |
| The new modes need (estimate) | about 3,600 | about 3,600 |
| Free after switching off the debug messages (saves 2,906) | 5,572 | 4,714 |
| Free after the new modes (estimate) | about 2,000 | about 1,100 |

Measured savings (test builds):

| Change | Saves | What you lose |
|---|---|---|
| Debug messages off (USB "Serial" printing) | **2,906 bytes** + 179 bytes RAM | Only the text messages on the computer's Serial Monitor. The car works the same |
| Demo mode off | 518 bytes | Phone Y / remote 4 demo (that remote button becomes free) |
| Capital-letters-only screen font | 256 bytes | Lowercase letters on the screen (song names show in CAPITALS) |
| Chaos mode off | 688 bytes | Chaos (not recommended - keep it) |
| Drum dance sound | 0 bytes | (already removed automatically, since the dances use the tune) |

To keep things fitting in the future, avoid: pictures / bitmaps on the screen (about 1 KB each), more fonts,
decimal (float) maths while the car runs, the Arduino `String` type, and long debug messages.

## Saved for later

- **Radar screen** - the servo sweeps and the screen draws a radar map
- **Record and replay** - drive a route with the phone, the car repeats it (saved even after switching off)
- **Emoji buttons** - hearts, angry, crying, sunglasses, wink on the screen
- **Horn and sound effects** - horn, police siren, reversing beep, laser
- **The car talks back** - sends its distance and what it's doing to the phone app
- **Games on the screen** - reaction game, "Simon says" with the IR remote
- **Wall follower** - drives along a wall at a fixed distance (mazes)
- **Line follower / table-edge stop** - only if the old line sensors / cliff sensor are still around (free pins A2, A3)
