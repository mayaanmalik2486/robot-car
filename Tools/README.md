# Tools (not needed to use the car)

These are the computer scripts that made the dance files and tested them before they went on
the car. They run the real compiled robot program in an Arduino UNO simulator
([avr8js](https://github.com/wokwi/avr8js)) with a fake phone, IR remote, distance sensor and
screen, and check that every dance move lands on the beat and every buzzer note is right.

| File | What it does |
|---|---|
| `tools/update-all-songs.js` | What `UPDATE_ALL_SONGS.bat` runs: copies `Robot_Car_Main` into every song file, then checks they all compile |
| `tools/choreo.js` | **The choreography**: the dictionary of dance steps and every song's routines |
| `tools/check-choreo.js` | Checks every step brings the car back to its spot and every routine is valid |
| `tools/make-dances.js` | Makes one dance program per song from `Robot_Car_Main`, the song data and the choreography |
| `results/songs.json` | Song data: tempo, key, chords and song parts (intro / verse / chorus...) |
| `tools/build.js` | Compiles a sketch with the Arduino IDE's compiler (Windows) |
| `tools/test-all.ps1` | Tests everything in the simulator: the main program (play modes, buttons, dances...) and every song |
| `tools/test-dances.ps1` | (older) builds every dance and plays each whole song in the simulator |
| `tools/run-sims.ps1` | Runs chosen simulator tests (driving, search room, buttons...) |
| `sim/harness.js` | The simulator: Bluetooth (both ways - it also reads the car's status line), IR, ultrasonic sensor, servo, buzzer, OLED, and any variable in the car's memory |
| `sim/scenarios.js` | The tests: every dance, every play mode (a virtual hand, ball, intruder or wall at chosen distances and angles), the buttons, search, sounds |
| `sim/dancecheck.js` | Works out where every dance move and buzzer note should be |
| `sim/room.js` | A pretend room with walls, for testing search mode |

Setup: Node.js, the Arduino IDE (with Arduino AVR Boards 1.8.8 installed), then
`npm install` inside `pkg-avr8js`.
