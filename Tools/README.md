# Tools (not needed to use the car)

These are the computer scripts that made the dance files and tested them before they went on
the car. They run the real compiled robot program in an Arduino UNO simulator
([avr8js](https://github.com/wokwi/avr8js)) with a fake phone, IR remote, distance sensor and
screen, and check that every dance move lands on the beat and every buzzer note is right.

| File | What it does |
|---|---|
| `tools/make-dances.js` | Makes one dance program per song from `Robot_Car_Main` and the song data |
| `results/songs.json` | Song data: tempo, key, chords and song parts (intro / verse / chorus...) |
| `tools/build.js` | Compiles a sketch with the Arduino IDE's compiler (Windows) |
| `tools/test-dances.ps1` | Builds every dance and plays each whole song in the simulator |
| `tools/run-sims.ps1` | Runs chosen simulator tests (driving, search room, buttons...) |
| `sim/harness.js` | The simulator: Bluetooth, IR, ultrasonic sensor, servo, buzzer and OLED |
| `sim/scenarios.js` | The tests |
| `sim/dancecheck.js` | Works out where every dance move and buzzer note should be |
| `sim/room.js` | A pretend room with walls, for testing search mode |

Setup: Node.js, the Arduino IDE (with Arduino AVR Boards 1.8.8 installed), then
`npm install` inside `pkg-avr8js`.
