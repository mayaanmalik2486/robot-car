/*
======================================================
       ROBOT CAR  -  v9    dance song: Baba Bandook - Ali Azmat
======================================================

  WHAT THIS ROBOT DOES
  - You drive it with a "Bluetooth RC car" phone app (through the HC-05).
  - You can also drive it with the small 17-button IR remote.
  - It has self-driving modes that use the ultrasonic distance sensor
    (HC-SR04) so it stops before it bumps into things, some fun moves
    (dance, demo, chaos, donut, drift) and a SEARCH mode that turns the
    sensor with the servo to look left, ahead and right, then turns toward
    the side with the most room until the way ahead is open (boxed in: it
    turns 90 degrees at a time until it finds an opening).
  - DANCE follows a whole song: calm moves in the calm parts, its best
    moves in the chorus / drop, in time with the beat, while the buzzer
    plays the drum beat. The song is set in section 3c (THE SONG).
  - SEARCH and CHAOS keep going by themselves until you send another command
    (holding or releasing the phone button doesn't stop them).
  - A small OLED screen shows what the robot is doing and how far away
    things are. While the servo moves (hello wave, SEARCH) it shows two
    cute eyes that look where the sensor looks and blink.
  - A buzzer plays a jingle for each mode, drum beats for the dance,
    parking-sensor beeps that speed up as you get close, and warning beeps.
  - When you switch it on, and when your phone sends its first command,
    it says hello: "Hello!" on the screen + a tune + a little servo wave.

  BLUETOOTH LETTERS (sent by the phone app)
    F forward        B backward       L spin left      R spin right
    G drift left     H drift right    S stop
    Q slow speed     W full speed
    2 auto forward (stops at obstacles)       J auto backward
    3 donut          U 180-degree turn         u smooth brake
    4 dance          Y demo           1 chaos          I search (looks around)
    X (the "!" button) mute / unmute every sound
    (4 again while dancing: tap it ON a beat to line the dance up with the music)

  IR REMOTE BUTTONS
    OK   stop everything             UP   forward (only if the way is clear)
    DOWN backward                    LEFT / RIGHT  short turn (150 ms)
    1 180-degree turn    2 slow/full speed    3 smooth brake
    4 demo   5 dance (again while dancing = line up with the beat)
    6 chaos  7 drift left   8 drift right   9 search
    * auto forward       # auto backward      0 donut
    Holding an arrow button keeps it going; other buttons act once per press.
    The buzzer borrows the IR receiver's timer, so keep IrReceiver.begin() in
    setup() even if you unplug the IR receiver.

  WIRING  (Arduino UNO + Sensor Shield V5) - see the PINS section below
    L298N   ENA D5   ENB D6   IN1 D2   IN2 D4   IN3 D7   IN4 D10
    HC-05   TXD -> D3 (Arduino listens)    RXD -> A0 (Arduino talks)
    IR receiver D8      Servo D9      HC-SR04 TRIG D11 / ECHO D12
    Buzzer (3-pin board) A1           OLED (I2C) SDA A4 / SCL A5
    The little "L" LED on the board (D13) flickers when IR signals arrive.
    Full wiring and power guide: POWER_AND_WIRING.md (in this folder).

  LIBRARIES
    IRremote 4.x (by Armin Joachimsmeyer), U8g2 (by oliver),
    plus Servo, SoftwareSerial and Wire (come with the Arduino IDE).

  HOW THE PROGRAM IS ORGANISED (top to bottom)
    1 libraries   2 pins   3 settings   3b dance moves + patterns
    3c THE SONG (tempo + the dance from start to end)
    4 remote codes   5 robot "memory"
    6 motors      7 buzzer   8 OLED screen   9 distance sensor   10 servo
   11 greeting    12 timed moves   13 stop / start helpers   14 IR remote
   15 Bluetooth   16 self-driving modes   17 setup() and loop()
======================================================
*/


//======================================================
// 1. LIBRARIES - ready-made code we borrow
//======================================================

#include <Arduino.h>
#include <Servo.h>            // moves the SG90 servo
#define DECODE_NEC            // only understand NEC remotes (yours is one): saves ~4.8 KB and TV remotes can't drive the car
#include <IRremote.hpp>       // reads the IR remote (version 4.x)
#include <SoftwareSerial.h>   // extra serial port for the HC-05
#include <Wire.h>             // I2C, used by the OLED
#include <U8x8lib.h>          // OLED text drawing (part of the U8g2 library)


//======================================================
// 2. PINS - every wire to the Arduino, all in one place
//======================================================

// Motor driver (L298N). Motor A = left wheels, Motor B = right wheels.
const byte ENA = 5;          // left wheels speed  (must be a PWM pin)
const byte ENB = 6;          // right wheels speed (must be a PWM pin)
const byte IN1 = 2;          // left wheels direction
const byte IN2 = 4;          // left wheels direction
const byte IN3 = 7;          // right wheels direction
const byte IN4 = 10;         // right wheels direction

// Sensors and servo
const byte IR_PIN = 8;       // IR receiver signal
const byte SERVO_PIN = 9;    // servo that turns the distance sensor
const byte TRIG_PIN = 11;    // HC-SR04 "trigger": Arduino -> sensor
const byte ECHO_PIN = 12;    // HC-SR04 "echo":    sensor -> Arduino

// Bluetooth module (HC-05). It uses "software serial" pins, so the
// USB cable (pins 0 and 1) stays free for uploading and the Serial Monitor.
const byte BT_RX_PIN = 3;    // Arduino listens here  <- HC-05 TXD
const byte BT_TX_PIN = A0;   // Arduino talks here    -> HC-05 RXD

// Buzzer (passive, 3-pin board)
const byte BUZZER_PIN = A1;

// OLED screen (I2C). These two pins are fixed by the UNO's I2C hardware,
// the Wire library uses them automatically - listed here so you know.
const byte OLED_SDA_PIN = A4;
const byte OLED_SCL_PIN = A5;


//======================================================
// 3. SETTINGS - numbers you can tweak
//======================================================

// Serial speeds
const long SERIAL_BAUD = 115200;      // USB Serial Monitor
const long BT_BAUD = 9600;            // HC-05 default speed

// Motor speeds: 0 = stopped, 255 = full power
const int FULL_SPEED = 255;
const int SLOW_SPEED_PERCENT = 60;
const int SLOW_SPEED = FULL_SPEED * SLOW_SPEED_PERCENT / 100;   // = 153
const int TURN_SPEED_PERCENT = 85;    // spins use 85 % of the current speed
const int DRIFT_SLOW_PERCENT = 60;    // inner wheels run at 60 % when drifting

// Servo angles (degrees) for the distance-sensor head.
// If "look left" points right on your robot, swap these two numbers.
const int SERVO_LEFT = 140;
const int SERVO_CENTER = 90;
const int SERVO_RIGHT = 40;

// Distances (cm)
const int STOP_DISTANCE = 30;         // closer than this = stop
const int SAFE_DISTANCE = 40;         // farther than this = "open" when looking around

// Distance sensor timing
const unsigned long ECHO_TIMEOUT_US = 11000;   // give up waiting for an echo after 11 ms (about 1.9 m)
const int NO_ECHO = 999;                       // value used when nothing echoes back
const unsigned long PING_GAP_MS = 15;          // pause between the 3 pings of one measurement

// Timed moves (milliseconds)
// Spin time for a 180-degree turn. (The old 440 really ran about 490 ms because
// the turn was only stopped once per loop - that was the 190-200 degrees. Now it
// stops on time, and 455 ms = 180 degrees on this car. Still off? Change it a little.)
const unsigned long TURN180_TIME = 455;
const unsigned long TURN90_TIME = TURN180_TIME / 2;   // and for a 90-degree turn (SEARCH when boxed in)
const unsigned long IR_TURN_TIME = 150;        // one tap of IR LEFT/RIGHT turns this long

const unsigned long BRAKE_STEP_MS = 40;        // smooth brake: lower the speed every 40 ms ...
const int BRAKE_STEP_PWM = 10;                 // ... by this much

const unsigned long DEMO_FORWARD_MS = 400;
const unsigned long DEMO_PAUSE_MS = 600;
const unsigned long DEMO_REVERSE_MS = 375;
const unsigned long DEMO_SPIN_MS = 2000;
const unsigned long DEMO_LAUNCH_MS = 375;

// CHAOS mode: random straight runs, random turns, spins and wiggles
// (every choice is random, so it doesn't drive in circles like the old zigzag)
const unsigned long CHAOS_STRAIGHT_MIN_MS = 400;   // each straight run lasts between ...
const unsigned long CHAOS_STRAIGHT_MAX_MS = 1300;  // ... these two times
const unsigned long CHAOS_TURN_MIN_MS = 120;       // each turn lasts between ...
const unsigned long CHAOS_TURN_MAX_MS = 450;       // ... these two times
const unsigned long CHAOS_SPIN_MIN_MS = 400;       // a spin trick lasts between ...
const unsigned long CHAOS_SPIN_MAX_MS = 900;       // ... these two times
const unsigned long CHAOS_BACKUP_MS = 250;         // obstacle: back up this long, then turn
const int CHAOS_SPIN_CHANCE = 15;                  // % chance of a spin after a straight run
const int CHAOS_WIGGLE_CHANCE = 15;                // % chance of a little reverse wiggle

// CHAOS drives a bit slower than full speed, so it crashes less
const int AUTO_SPEED_PERCENT = 75;

// SEARCH mode: stop, look left / right / ahead with the servo, then turn
// toward the side with the most room UNTIL THE SENSOR SEES OPEN SPACE
const unsigned long SEARCH_BACKUP_MS = 120;    // back up a little after stopping
const unsigned long SERVO_SETTLE_MS = 250;     // time for a 50-degree servo move
const unsigned long SERVO_SWING_MS = 350;      // time for the 100-degree swing left -> right
const unsigned long SEARCH_TURN_STEP_MS = 60;  // turn toward the open side in small steps (about 20 degrees each):
                                               // turn, stop, measure while standing still, repeat
const byte SEARCH_TURN_MAX_STEPS = 18;         // no open way after this many steps (all the way round): look again
const byte SEARCH_TURN_EXTRA_STEPS = 1;        // after it sees open space: one more step, so the whole car gets past the edge
const int SEARCH_OPEN_ENOUGH = 100;            // open = this much is clear ahead (or 3/4 of what it saw on that side)
const unsigned long SEARCH_STUCK_MS = 1500;    // driving, but the distance ahead hasn't changed for this long ...
const int SEARCH_STUCK_CM = 4;                 // ... (by less than this) = stuck on something the sensor can't see
const byte SEARCH_NO_ECHO_CHECKS = 4;          // seeing nothing at all ahead for 4 checks in a row (6 s) = probably stuck too
const int SEARCH_STUCK_NEAR_CM = 100;          // the 1.5 s stuck check only counts when the thing ahead is closer than this
const byte SEARCH_STUCK_FAR_CHECKS = 3;        // farther away: unchanged for 3 checks in a row (4.5 s). (In a narrow corridor
                                               // the sensor sees the side walls, so the reading hardly changes while driving.)
const unsigned long SEARCH_STUCK_BACKUP_MS = 300;  // when stuck, back up this long (then it always turns, never straight on)

// Phone apps send the letter while you hold a button and 'S' when you let go.
// For this long after a mode letter, that 'S' is ignored so the mode keeps going.
const unsigned long BUTTON_RELEASE_IGNORE_MS = 1500;

// IR remote
const unsigned long IR_REPEAT_WINDOW_MS = 300; // a "held down" signal must follow the press within this time

// Buzzer: if your buzzer board gets warm or clicks while it should be silent,
// it is the kind that turns on with LOW - change this to HIGH.
const byte BUZZER_OFF_LEVEL = LOW;
const unsigned long MUTE_PRESS_GAP_MS = 300;   // the mute button (phone X) ignores repeats closer than this

// Parking-sensor beeps: while driving forward toward something, beep faster
// and faster the closer it gets (set to false to switch them off)
const bool PARKING_BEEPS = true;
const int PARK_BEEP_FROM_CM = 80;              // start beeping when closer than this
const unsigned long PARK_GAP_NEAR_MS = 90;     // time between beeps when very close ...
const unsigned long PARK_GAP_FAR_MS = 600;     // ... and when just in range
const unsigned int PARK_BEEP_HZ = 1650;
const unsigned int PARK_BEEP_MS = 35;

// Brake jerk: when it has to stop for an obstacle while driving forward,
// it gives the wheels a short burst in reverse so it stops much shorter
// (scaled down when it was driving slower). 0 = switch it off.
const unsigned long ACTIVE_BRAKE_MS = 70;

// OLED eyes (always on screen; their mood changes with what the robot does)
const bool EYES_FLIP = false;                  // if the eyes look the opposite way to the robot, set true
const unsigned long BLINK_MS = 150;            // how long a blink lasts
const unsigned long BLINK_MIN_GAP_MS = 2000;   // time between blinks: random, between ...
const unsigned long BLINK_MAX_GAP_MS = 4500;   // ... these two
const unsigned long EYES_FRAME_MS = 60;        // eye animation: at most one new picture every 60 ms
const unsigned long SURPRISE_MS = 1000;        // big "whoa!" eyes after an emergency stop
const unsigned long SLEEPY_AFTER_MS = 25000;   // doing nothing this long = sleepy eyes
const unsigned long DIZZY_AFTER_DONUT_MS = 1300;  // after you stop a donut: dizzy eyes and a wobbly head this long
const unsigned long DIZZY_WOBBLE_MS = 130;     // the head wobbles left/right this often while dizzy
const int DIZZY_WOBBLE_DEGREES = 25;           // ... by this much

// OLED and greeting
const unsigned long OLED_REFRESH_MS = 250;     // redraw each part of the screen at most 4 times a second
const unsigned long GREETING_SCREEN_MS = 3000; // how long "Hello!" stays on the screen
const unsigned long WAVE_STEP_MS = 220;        // speed of the hello wave

// Dance
const bool DANCE_HEAD = true;                  // the sensor head turns with the dance moves (false = it stays still)
const int DANCE_TWIST_PERCENT = 70;            // TWIST moves turn at 70 % of the dance speed
const int DANCE_CURVE_PERCENT = 35;            // SWAY / BACK curves: the inner wheels run at 35 %
const unsigned long DANCE_TAP_GAP_MS = 400;    // a new tap of the dance button (not just holding it)
const unsigned long SONG_NAME_SCROLL_MS = 350; // a long song name scrolls one letter this often
const unsigned long SHOUT_MS = 2000;           // the special moment's text stays on the screen this long
const unsigned long SHOUT_SOUND_MS = 300;      // (the zap sound plays over the tune for this long)
const unsigned long DANCE_DRAW_GUARD_US = 70000;  // no screen drawing in the last 70 ms before a dance step
const int DANCE_HEAD_BOB_DEGREES = 25;         // head bob: how far the head turns on the straight moves
const byte DANCE_TUNE_OCTAVES = 2;             // the tune plays 2 octaves above middle C (the buzzer is loudest up there) ...
const byte DANCE_BASS_OCTAVES = 1;             // ... and the bass 1 octave above it
const byte DANCE_TUNE_HIGHEST = 96;            // no tune note higher than this (MIDI 96 = C7, about 2100 Hz)
const unsigned long DANCE_NOTE_GAP_MS = 15;    // a tiny gap between notes so each one is heard
const unsigned long DANCE_BASS_MAX_MS = 140;   // bass notes are short and punchy


//======================================================
// 3b. DANCE MOVES, DRUM SOUNDS AND DANCE PATTERNS
//
//     The dance follows a beat grid: every beat is split into 2 steps,
//     so one bar of 4 beats = 8 steps. Each step is one move + one drum
//     sound, written like  FWD+KICK  (forward, with a bass drum).
//     A pattern is one bar (8 steps). The song (next section) says which
//     pattern to dance for how many bars, from the start of the song to the end.
//======================================================

// Moves
const byte POSE = 0;       // stand still
const byte FWD = 1;        // forward
const byte BACK = 2;       // backward
const byte SPIN_L = 3;     // spin left on the spot
const byte SPIN_R = 4;     // spin right on the spot
const byte TWIST_L = 5;    // smaller, slower turn left (a shimmy)
const byte TWIST_R = 6;    // smaller, slower turn right
const byte SWAY_L = 7;     // curve forward to the left
const byte SWAY_R = 8;     // curve forward to the right
const byte BACK_L = 9;     // curve backward - exactly undoes SWAY_L
const byte BACK_R = 10;    // curve backward - exactly undoes SWAY_R
const byte DASH = 11;      // forward at full speed
const byte LOOK_L = 12;    // stand still, sensor head looks left
const byte LOOK_R = 13;    // stand still, sensor head looks right
const byte HOLD = 14;      // keep doing the last move

// Drum sounds (the buzzer plays the beat - drums only, no tunes)
const byte QUIET = 0x00;   // no sound
const byte TICK = 0x10;    // hi-hat "ts"
const byte TAP = 0x20;     // wood block "tok"
const byte KICK = 0x30;    // bass drum "boom"
const byte CLAP = 0x40;    // snare / clap "pah"
const byte HIT = 0x50;     // big accent "ta-daa"

const byte STEPS_PER_BAR = 8;

// The pattern library. Steps 1, 3, 5, 7 are the beats you clap to.
// Patterns that move the car also bring it back, so it dances on the spot.
const byte P_STILL = 0;    // calm: stand still, soft pulse
const byte P_LOOK = 1;     // calm: look left, look right
const byte P_PULSE = 2;    // calm: stand still, heartbeat drum
const byte P_ROCK = 3;     // gentle: rock left and right
const byte P_GLIDE_L = 4;  // gentle: glide forward-left and back
const byte P_GLIDE_R = 5;  // gentle: glide forward-right and back
const byte P_MARCH = 6;    // steady: step forward, step back
const byte P_STEP = 7;     // groove: forward-back on the beat
const byte P_WIGGLE = 8;   // groove: shimmy left-right
const byte P_SWING = 9;    // groove: swing left, swing right
const byte P_BOUNCE = 10;  // lively: bounce forward and back
const byte P_SHUFFLE = 11; // lively: zig-zag shuffle
const byte P_PUMP = 12;    // hype: fast forward-back (head-bang)
const byte P_PARTY = 13;   // hype: spin, spin, bounce, pose
const byte P_SPIN = 14;    // hype: big spins both ways
const byte P_DROP = 15;    // hype: dash, spins, accents (for drops and big choruses)
const byte P_BHANGRA = 16; // lively: shoulder shimmy + bounce, dhol-style drums
const byte P_BUILD = 17;   // build-up: drums get busier, shimmy gets faster
const byte P_FILL = 18;    // end of a part: spin with a drum roll, pose
const byte P_FINALE = 19;  // the very last bar: spin and a big final pose
const byte P_JAZZ = 20;    // jazzy: little shimmy, then a cool look left and right
const byte P_SNAP = 21;    // jazzy: stand, look around on the off-beats (finger-snap feel)
const byte P_SLIDE = 22;   // jazzy: slow slide forward, pose, slide back, pose

const byte DANCE_PATTERNS[][STEPS_PER_BAR] PROGMEM = {
    { POSE+TAP,     HOLD,         HOLD,         HOLD,         HOLD+TICK,    HOLD,         HOLD,         HOLD        },  // P_STILL
    { LOOK_L+TAP,   HOLD,         HOLD,         HOLD,         LOOK_R+TAP,   HOLD,         HOLD,         HOLD        },  // P_LOOK
    { POSE+KICK,    HOLD,         HOLD+TICK,    HOLD,         HOLD+KICK,    HOLD,         HOLD+TICK,    HOLD        },  // P_PULSE
    { TWIST_L+KICK, HOLD,         POSE+TICK,    HOLD,         TWIST_R+KICK, HOLD,         POSE+TICK,    HOLD        },  // P_ROCK
    { SWAY_L+KICK,  HOLD,         HOLD+TICK,    HOLD,         BACK_L+KICK,  HOLD,         HOLD+TICK,    HOLD        },  // P_GLIDE_L
    { SWAY_R+KICK,  HOLD,         HOLD+TICK,    HOLD,         BACK_R+KICK,  HOLD,         HOLD+TICK,    HOLD        },  // P_GLIDE_R
    { FWD+KICK,     HOLD,         POSE+TAP,     HOLD,         BACK+KICK,    HOLD,         POSE+TAP,     HOLD        },  // P_MARCH
    { FWD+KICK,     POSE+TICK,    BACK+CLAP,    POSE+TICK,    FWD+KICK,     POSE+TICK,    BACK+CLAP,    POSE+TICK   },  // P_STEP
    { TWIST_L+KICK, TWIST_R+TICK, TWIST_L+CLAP, TWIST_R+TICK, TWIST_L+KICK, TWIST_R+TICK, TWIST_L+CLAP, TWIST_R+TICK},  // P_WIGGLE
    { SWAY_L+KICK,  HOLD+TICK,    BACK_L+CLAP,  HOLD+TICK,    SWAY_R+KICK,  HOLD+TICK,    BACK_R+CLAP,  HOLD+TICK   },  // P_SWING
    { FWD+KICK,     BACK+TICK,    POSE+CLAP,    HOLD+TICK,    BACK+KICK,    FWD+TICK,     POSE+CLAP,    HOLD+TICK   },  // P_BOUNCE
    { SWAY_L+KICK,  BACK_L+TICK,  SWAY_R+CLAP,  BACK_R+TICK,  SWAY_L+KICK,  BACK_L+TICK,  SWAY_R+CLAP,  BACK_R+TICK },  // P_SHUFFLE
    { FWD+KICK,     BACK+TICK,    FWD+CLAP,     BACK+TICK,    FWD+KICK,     BACK+TICK,    FWD+CLAP,     BACK+TICK   },  // P_PUMP
    { SPIN_L+KICK,  HOLD+TICK,    SPIN_R+CLAP,  HOLD+TICK,    FWD+KICK,     BACK+TICK,    POSE+HIT,     HOLD        },  // P_PARTY
    { SPIN_R+KICK,  HOLD+TICK,    HOLD+CLAP,    HOLD+TICK,    SPIN_L+KICK,  HOLD+TICK,    HOLD+CLAP,    POSE+TICK   },  // P_SPIN
    { DASH+HIT,     BACK+KICK,    POSE+TICK,    HOLD+TICK,    SPIN_L+CLAP,  HOLD+TICK,    SPIN_R+KICK,  POSE+CLAP   },  // P_DROP
    { TWIST_L+KICK, TWIST_R+TAP,  TWIST_L+TAP,  TWIST_R+CLAP, FWD+KICK,     BACK+TAP,     FWD+TAP,      BACK+CLAP   },  // P_BHANGRA
    { POSE+TICK,    HOLD+TICK,    TWIST_L+TICK, TWIST_R+TICK, TWIST_L+CLAP, TWIST_R+CLAP, TWIST_L+CLAP, TWIST_R+CLAP},  // P_BUILD
    { SPIN_R+CLAP,  HOLD+CLAP,    HOLD+CLAP,    HOLD+CLAP,    SPIN_L+KICK,  HOLD+KICK,    POSE+HIT,     HOLD        },  // P_FILL
    { SPIN_L+KICK,  HOLD+KICK,    HOLD+KICK,    HOLD+KICK,    POSE+HIT,     HOLD,         HOLD,         HOLD        },  // P_FINALE
    { TWIST_L+TAP,  HOLD,         TWIST_R+TICK, HOLD,         POSE+TAP,     LOOK_L,       HOLD+TICK,    LOOK_R      },  // P_JAZZ
    { POSE+TAP,     LOOK_L,       HOLD+TICK,    POSE,         HOLD+TAP,     LOOK_R,       HOLD+TICK,    POSE        },  // P_SNAP
    { FWD+KICK,     HOLD,         HOLD+TICK,    POSE,         BACK+TAP,     HOLD,         HOLD+TICK,    POSE        },  // P_SLIDE
};

// Names of the song parts (shown on the screen while dancing)
const byte INTRO = 0;
const byte VERSE = 1;
const byte PRE = 2;        // pre-chorus
const byte CHORUS = 3;
const byte HOOK = 4;
const byte BUILD = 5;
const byte DROP = 6;
const byte BRIDGE = 7;
const byte BREAK = 8;      // breakdown
const byte SOLO = 9;       // instrumental / solo
const byte OUTRO = 10;
const byte POWER = 0x80;   // add to a part name (CHORUS+POWER): that part dances at DANCE_POWER_SPEED - the explosive bits

const char PART_NAMES[][8] PROGMEM = {
    "INTRO", "VERSE", "PRE-CHO", "CHORUS", "HOOK", "BUILD", "DROP!", "BRIDGE", "BREAK", "SOLO", "OUTRO"
};

// THE BUZZER TUNE. While dancing, the buzzer plays a tune that follows the
// song's chords (in the song's key): bass notes on the main beats and a
// melody on top. Each step of a tune pattern is one of these:
const byte SIL = 0x00;          // silence
const byte TIE = 0x10;          // keep the last note going
const byte ROOT = 0x20;         // melody notes, counted up the scale from the chord's root note:
const byte SECOND = 0x21;
const byte THIRD = 0x22;
const byte FOURTH = 0x23;
const byte FIFTH = 0x24;
const byte SIXTH = 0x25;
const byte SEVENTH = 0x26;
const byte HIGH_ROOT = 0x27;    // the root one octave higher
const byte HIGH_SECOND = 0x28;
const byte HIGH_THIRD = 0x29;
const byte HIGH_FIFTH = 0x2B;
const byte BASS = 0x30;         // bass note: the chord's root, lower down
const byte BASS_FIFTH = 0x34;   // bass: the chord's fifth

const byte T_CALM = 0;          // slow: a bass note and a few long notes
const byte T_FLOAT = 1;         // slow: rising long notes
const byte T_PULSE = 2;         // steady bass on every beat, a note in between
const byte T_GROOVE = 3;        // bouncy bass + short notes
const byte T_ARP = 4;           // lively: chord notes running up and down
const byte T_HOOK_A = 5;        // catchy two-bar melody (part A) ...
const byte T_HOOK_B = 6;        // ... and part B
const byte T_DRIVE = 7;         // hype: driving bass with high hits
const byte T_RISE = 8;          // build-up: climbing the scale
const byte T_DARK = 9;          // mysterious: low, slow, minor-sounding steps
const byte T_END = 10;          // the last bar: one long high note

const byte TUNE_PATTERNS[][STEPS_PER_BAR] PROGMEM = {
    { BASS,      TIE,        TIE,       TIE,        FIFTH,     TIE,        THIRD,     TIE        },  // T_CALM
    { BASS,      TIE,        THIRD,     TIE,        FIFTH,     TIE,        HIGH_ROOT, TIE        },  // T_FLOAT
    { BASS,      FIFTH,      BASS,      THIRD,      BASS,      FIFTH,      BASS,      HIGH_ROOT  },  // T_PULSE
    { BASS,      SIL,        FIFTH,     THIRD,      BASS_FIFTH, SIL,       HIGH_ROOT, FIFTH      },  // T_GROOVE
    { BASS,      THIRD,      FIFTH,     HIGH_ROOT,  HIGH_THIRD, HIGH_ROOT, FIFTH,     THIRD      },  // T_ARP
    { HIGH_ROOT, TIE,        FIFTH,     SIXTH,      FIFTH,     TIE,        THIRD,     TIE        },  // T_HOOK_A
    { THIRD,     FIFTH,      HIGH_ROOT, TIE,        HIGH_SECOND, HIGH_ROOT, FIFTH,    TIE        },  // T_HOOK_B
    { BASS,      BASS,       HIGH_ROOT, BASS,       BASS_FIFTH, BASS,      HIGH_THIRD, HIGH_ROOT },  // T_DRIVE
    { ROOT,      SECOND,     THIRD,     FOURTH,     FIFTH,     SIXTH,      SEVENTH,   HIGH_ROOT  },  // T_RISE
    { BASS,      TIE,        SIL,       ROOT,       TIE,       SECOND,     THIRD,     TIE        },  // T_DARK
    { HIGH_ROOT, TIE,        TIE,       TIE,        TIE,       TIE,        TIE,       SIL        },  // T_END
};

// Steps of the major and minor scales, in semitones
const byte MAJOR_SCALE[7] PROGMEM = {0, 2, 4, 5, 7, 9, 11};
const byte MINOR_SCALE[7] PROGMEM = {0, 2, 3, 5, 7, 8, 10};

// What the buzzer plays while dancing
const byte DANCE_SOUND_TUNE = 0;   // the tune that follows the song's chords
const byte DANCE_SOUND_DRUMS = 1;  // the old drum beeps
const byte DANCE_SOUND_OFF = 2;    // a silent dance


//======================================================
// 3c. THE SONG THIS ROBOT DANCES TO  (phone 4 / remote 5)
//
//     HOW TO DANCE IN TIME: start the song on a speaker or laptop and press
//     dance exactly on the FIRST BEAT. If the robot drifts off the beat later,
//     tap dance again exactly on a beat - it lines itself up with your tap.
//     Moves too fast or too slow for the song? Change SONG_BPM a little.
//======================================================

// ----- SONG START (each song file has its own version of this part) -----
// Song: Baba Bandook - Ali Azmat & Haroon
// Version this dance is timed to (3:53 long): Baba Bandook - Single (Burka Avenger OST),
//  3:53 / 233.1 s on Spotify, Apple Music and Deezer
// HOW TO START: play the song, and press dance (phone 4 / remote 5) on the
// first beat - about 0.3 s after the song starts. The dance lasts the whole song
// (86 bars, about 3:53). Tap dance again on a beat to line it up if it drifts.
// Tempo: 88.5 BPM (fairly sure) - if the moves get ahead of the music, lower SONG_BPM a little; if they fall behind, raise it.
// Song parts (estimated - where the dance changes): 0:00 INTRO, 0:11 VERSE, 0:32 PRE,
//  0:43 CHORUS, 1:05 SOLO, 1:16 VERSE, 1:37 PRE, 1:48 CHORUS, 2:10 BRIDGE, 2:32 SOLO,
//  2:53 CHORUS, 3:15 CHORUS, 3:37 OUTRO
const char SONG_NAME[] PROGMEM = "Baba Bandook - Ali Azmat";
const float SONG_BPM = 88.5;                        // beats per minute
const unsigned long SONG_FIRST_BEAT_MS = 300;        // where the first beat is in the song (so the clock on the screen matches your player)
const int DANCE_SPEED = 240;                       // motor speed for the dance moves (0-255)
const int DANCE_POWER_SPEED = 240;                 // motor speed in the POWER parts (the explosive bits)
const char SONG_SHOUT[] PROGMEM = "";              // a special moment: this text on the screen + a zap sound ("" = none)
const int SONG_SHOUT_STEPS[3] = { -1, -1, -1 };    // ... at these steps of the dance (2 steps per beat, 0 = the first step, -1 = not used)
const bool DANCE_HEAD_BOB = true;                  // the sensor head also bobs left/right on the straight moves
const byte DANCE_SOUND = DANCE_SOUND_TUNE;         // buzzer: DANCE_SOUND_TUNE, DANCE_SOUND_DRUMS or DANCE_SOUND_OFF

// The song's key and chords (for the buzzer tune) - D major, D major (fairly sure)
const byte SONG_KEY = 62;                          // key note: 60 C, 61 C#, 62 D, 63 D#, 64 E, 65 F, 66 F#, 67 G, 68 G#, 69 A, 70 A#, 71 B
const bool SONG_MINOR = false;                     // true = minor key
// Chord progressions, one chord per bar, as steps of the scale (1 = the key's own chord)
const byte SONG_CHORDS[][4] PROGMEM = {
    { 1, 4, 1, 5 },                                // 0 verse
    { 1, 4, 5, 4 },                                // 1 chorus
    { 6, 4, 1, 5 },                                // 2 bridge
};

// The song from start to end: { dance pattern, how many bars, part name, chords, tune pattern }
const byte SONG_MAP[][5] PROGMEM = {
    { P_PULSE,     1, INTRO,        0, T_CALM   },
    { P_PULSE,     1, INTRO,        0, T_FLOAT  },
    { P_STEP,      1, INTRO,        0, T_CALM   },
    { P_WIGGLE,    1, INTRO,        0, T_FLOAT  },
    { P_BOUNCE,    1, VERSE,        0, T_GROOVE },
    { P_SWING,     1, VERSE,        0, T_GROOVE },
    { P_BOUNCE,    1, VERSE,        0, T_PULSE  },
    { P_SWING,     1, VERSE,        0, T_PULSE  },
    { P_BOUNCE,    1, VERSE,        0, T_GROOVE },
    { P_SWING,     1, VERSE,        0, T_GROOVE },
    { P_BOUNCE,    1, VERSE,        0, T_PULSE  },
    { P_SWING,     1, VERSE,        0, T_PULSE  },
    { P_BOUNCE,    1, PRE,          0, T_GROOVE },
    { P_SWING,     1, PRE,          0, T_GROOVE },
    { P_BOUNCE,    1, PRE,          0, T_PULSE  },
    { P_FILL,      1, PRE,          0, T_PULSE  },
    { P_DROP,      1, CHORUS,       1, T_HOOK_A },
    { P_PARTY,     1, CHORUS,       1, T_HOOK_B },
    { P_BOUNCE,    1, CHORUS,       1, T_HOOK_A },
    { P_SPIN,      1, CHORUS,       1, T_HOOK_B },
    { P_SHUFFLE,   1, CHORUS,       1, T_HOOK_A },
    { P_PARTY,     1, CHORUS,       1, T_HOOK_B },
    { P_BOUNCE,    1, CHORUS,       1, T_HOOK_A },
    { P_SPIN,      1, CHORUS,       1, T_HOOK_B },
    { P_BOUNCE,    1, SOLO,         2, T_GROOVE },
    { P_SWING,     1, SOLO,         2, T_GROOVE },
    { P_BOUNCE,    1, SOLO,         2, T_PULSE  },
    { P_SWING,     1, SOLO,         2, T_PULSE  },
    { P_BOUNCE,    1, VERSE,        0, T_GROOVE },
    { P_SWING,     1, VERSE,        0, T_GROOVE },
    { P_BOUNCE,    1, VERSE,        0, T_PULSE  },
    { P_SWING,     1, VERSE,        0, T_PULSE  },
    { P_BOUNCE,    1, VERSE,        0, T_GROOVE },
    { P_SWING,     1, VERSE,        0, T_GROOVE },
    { P_BOUNCE,    1, VERSE,        0, T_PULSE  },
    { P_SWING,     1, VERSE,        0, T_PULSE  },
    { P_BOUNCE,    1, PRE,          0, T_GROOVE },
    { P_SWING,     1, PRE,          0, T_GROOVE },
    { P_BOUNCE,    1, PRE,          0, T_PULSE  },
    { P_FILL,      1, PRE,          0, T_PULSE  },
    { P_DROP,      1, CHORUS,       1, T_HOOK_A },
    { P_PARTY,     1, CHORUS,       1, T_HOOK_B },
    { P_BOUNCE,    1, CHORUS,       1, T_HOOK_A },
    { P_SPIN,      1, CHORUS,       1, T_HOOK_B },
    { P_SHUFFLE,   1, CHORUS,       1, T_HOOK_A },
    { P_PARTY,     1, CHORUS,       1, T_HOOK_B },
    { P_BOUNCE,    1, CHORUS,       1, T_HOOK_A },
    { P_SPIN,      1, CHORUS,       1, T_HOOK_B },
    { P_STEP,      1, BRIDGE,       2, T_CALM   },
    { P_WIGGLE,    1, BRIDGE,       2, T_FLOAT  },
    { P_STEP,      1, BRIDGE,       2, T_CALM   },
    { P_WIGGLE,    1, BRIDGE,       2, T_FLOAT  },
    { P_STEP,      1, BRIDGE,       2, T_CALM   },
    { P_WIGGLE,    1, BRIDGE,       2, T_FLOAT  },
    { P_STEP,      1, BRIDGE,       2, T_CALM   },
    { P_FILL,      1, BRIDGE,       2, T_FLOAT  },
    { P_SHUFFLE,   1, SOLO,         2, T_HOOK_A },
    { P_PARTY,     1, SOLO,         2, T_HOOK_B },
    { P_BOUNCE,    1, SOLO,         2, T_HOOK_A },
    { P_SPIN,      1, SOLO,         2, T_HOOK_B },
    { P_SHUFFLE,   1, SOLO,         2, T_HOOK_A },
    { P_PARTY,     1, SOLO,         2, T_HOOK_B },
    { P_BOUNCE,    1, SOLO,         2, T_HOOK_A },
    { P_FILL,      1, SOLO,         2, T_HOOK_B },
    { P_DROP,      1, CHORUS,       1, T_HOOK_A },
    { P_PARTY,     1, CHORUS,       1, T_HOOK_B },
    { P_SHUFFLE,   1, CHORUS,       1, T_DRIVE  },
    { P_SPIN,      1, CHORUS,       1, T_HOOK_B },
    { P_DROP,      1, CHORUS,       1, T_HOOK_A },
    { P_PARTY,     1, CHORUS,       1, T_HOOK_B },
    { P_SHUFFLE,   1, CHORUS,       1, T_DRIVE  },
    { P_SPIN,      1, CHORUS,       1, T_HOOK_B },
    { P_DROP,      1, CHORUS,       1, T_HOOK_A },
    { P_PARTY,     1, CHORUS,       1, T_HOOK_B },
    { P_SHUFFLE,   1, CHORUS,       1, T_DRIVE  },
    { P_SPIN,      1, CHORUS,       1, T_HOOK_B },
    { P_DROP,      1, CHORUS,       1, T_HOOK_A },
    { P_PARTY,     1, CHORUS,       1, T_HOOK_B },
    { P_SHUFFLE,   1, CHORUS,       1, T_DRIVE  },
    { P_SPIN,      1, CHORUS,       1, T_HOOK_B },
    { P_BOUNCE,    1, OUTRO,        0, T_GROOVE },
    { P_SWING,     1, OUTRO,        0, T_GROOVE },
    { P_BOUNCE,    1, OUTRO,        0, T_PULSE  },
    { P_SWING,     1, OUTRO,        0, T_PULSE  },
    { P_BOUNCE,    1, OUTRO,        0, T_GROOVE },
    { P_FINALE,    1, OUTRO,        0, T_END    },
};
// ----- SONG END -----

const byte SONG_PARTS = sizeof(SONG_MAP) / sizeof(SONG_MAP[0]);
const unsigned long DANCE_STEP_US = 60000000.0 / (SONG_BPM * 2);   // one step, in microseconds


//======================================================
// 4. IR REMOTE BUTTON CODES (from your remote - unchanged)
//======================================================

const byte CMD_UP    = 0x18;
const byte CMD_DOWN  = 0x52;
const byte CMD_LEFT  = 0x08;
const byte CMD_RIGHT = 0x5A;
const byte CMD_OK    = 0x1C;

const byte CMD_1     = 0x45;
const byte CMD_2     = 0x46;
const byte CMD_3     = 0x47;
const byte CMD_4     = 0x44;
const byte CMD_5     = 0x40;
const byte CMD_6     = 0x43;
const byte CMD_7     = 0x07;
const byte CMD_8     = 0x15;
const byte CMD_9     = 0x09;

const byte CMD_STAR  = 0x16;
const byte CMD_0     = 0x19;
const byte CMD_HASH  = 0x0D;


//======================================================
// 5. THE ROBOT'S "MEMORY" - what it is doing right now
//======================================================

// The robot is always in exactly one of these states.
enum RobotState
{
    IDLE,
    MANUAL_FORWARD,
    MANUAL_BACKWARD,
    MANUAL_LEFT,
    MANUAL_RIGHT,
    AUTO_FORWARD,
    AUTO_BACKWARD,
    DONUT,
    DRIFT_LEFT,
    DRIFT_RIGHT,
    DANCE,
    DEMO,
    CHAOS,
    SEARCH,
    TURN180,
    BRAKING
};

RobotState robotState = IDLE;

// Hardware objects
Servo scanner;                              // the servo with the distance sensor
SoftwareSerial BT(BT_RX_PIN, BT_TX_PIN);    // the Bluetooth module
U8X8_SSD1306_128X64_NONAME_HW_I2C oled(U8X8_PIN_NONE);   // the OLED screen

// Speed
bool slowMode = false;
int currentSpeed = FULL_SPEED;

// Driving modes (true = that mode is running)
bool autoForward = false;
bool autoBackward = false;
bool donutMode = false;

// Smooth brake
bool braking = false;
int brakePWM = FULL_SPEED;

// 180-degree turn
bool turning180 = false;
unsigned long turn180Timer = 0;    // when the turn started

// Dance: where we are in the song (part of SONG_MAP, bar in that part, step in that bar)
bool danceMode = false;
byte dancePart = 0;
byte danceBar = 0;
byte danceStep = 0;
byte danceSectionBar = 0;          // bar number inside the current song part (for the chords)
unsigned int danceStepCount = 0;   // steps since the dance started (for the special moment)
unsigned long shoutUntil = 0;      // the special moment's text is on the screen until this time
bool danceHeadBobLeft = false;     // the head bob goes left and right in turn
byte danceMove = POSE;             // the move it is doing right now
unsigned long danceNextUs = 0;     // when the next step is due (micros)
unsigned long danceBeatUs = 0;     // when the last beat was (micros), to line up with your taps
unsigned long danceStartMs = 0;    // when the dance started (for the clock on the screen)

// Demo and chaos: which step they are on and when it started

bool demoMode = false;
byte demoStep = 0;
unsigned long demoTimer = 0;

// CHAOS mode steps
const byte CHAOS_STRAIGHT = 0;
const byte CHAOS_TURN = 1;           // a turn (a long turn is a "spin")
const byte CHAOS_BACKUP = 2;

bool chaosMode = false;
byte chaosStep = CHAOS_STRAIGHT;
unsigned long chaosTimer = 0;
unsigned long chaosDuration = 0;     // how long the current random move lasts
bool chaosTurnLeft = false;          // direction of the current random turn

// SEARCH mode: its steps, and what it saw
const byte AVOID_DRIVE = 0;          // drive forward, looking ahead
const byte AVOID_LOOK_LEFT = 1;
const byte AVOID_LOOK_RIGHT = 2;
const byte AVOID_LOOK_CENTER = 3;
const byte AVOID_CORNER_TURN = 4;    // boxed in: turning 90 degrees ...
const byte AVOID_TURN_TO_OPEN = 5;   // turning toward the open side until the way is clear
const byte AVOID_CORNER_CHECK = 6;   // ... then checking straight ahead for an opening
const byte MAX_CORNER_TURNS = 4;     // 4 x 90 = all the way round: then back up and look again

bool searchMode = false;
byte searchStep = AVOID_DRIVE;
unsigned long avoidTimer = 0;
bool searchTurnLeft = false;         // which way it is turning toward the open side
int searchOpenTarget = 0;            // stop turning when this much is clear ahead (cm)
byte turnSteps = 0;                  // how many small turning steps it has done toward the open side
int stuckCheckDistance = 0;          // the distance ahead at the last "am I stuck?" check
unsigned long stuckCheckAt = 0;      // and when that was
byte noEchoChecks = 0;               // how many checks in a row saw nothing at all ahead
byte sameChecks = 0;                 // how many checks in a row saw the same distance ahead
byte cornerTurns = 0;                // how many 90-degree turns it has done while boxed in
int leftDistance = NO_ECHO;
int rightDistance = NO_ECHO;
int centerDistance = NO_ECHO;

// Bluetooth button handling
unsigned long modeLetterAt = 0;    // when the last mode letter (I, 1, 2 ...) arrived

// Distance sensor
int lastDistance = NO_ECHO;        // newest measurement (shown on the OLED)
bool obstacleWarned = false;       // already beeped for this obstacle

// IR remote
unsigned long lastIRTime = 0;      // when the last IR button was handled
byte lastIRCommand = 0;            // which button that was

// Bluetooth
bool btGreeted = false;            // true once the phone has sent its first letter
bool muted = false;                // phone X: all buzzer sounds off (press again: on)
unsigned long muteToggledAt = 0;   // (so one press doesn't toggle twice)

// Buzzer
const uint16_t* tunePos = NULL;    // where we are in the current tune (NULL = silent)
unsigned long noteStart = 0;
unsigned int noteLength = 0;
volatile uint8_t* buzzerPort;      // the buzzer pin's output register (set in setupBuzzer)
uint8_t buzzerMask = 0;
volatile uint16_t buzzerPhase = 0; // the note generator (see section 7)
volatile uint16_t buzzerStep = 0;
unsigned long buzzerTickHz = 0;    // how often Timer2 ticks (about 19,800 times a second)

// OLED
bool oledFound = false;
char shownTop[17] = "";            // the text on the top and bottom rows right now
char shownBottom[17] = "";
unsigned long textDrawnAt[2] = {0, 0};
byte nextText = 0;
unsigned long surpriseUntil = 0;   // big surprised eyes until this time
unsigned long dizzyUntil = 0;      // dizzy eyes + wobbly head until this time (after a donut)
unsigned long dizzyWobbleAt = 0;   // when the head last wobbled
unsigned long lastActiveAt = 0;    // last time the robot was doing something (sleepy eyes)

// Motors: are we driving forward right now, and how fast? (for the brake jerk)
bool goingForward = false;
int forwardSpeed = 0;

// Greeting
bool greetingOnScreen = false;
bool greetingFromPhone = false;
unsigned long greetingStart = 0;
byte waveStep = 0;                 // 0 = not waving
unsigned long waveTimer = 0;
const byte WAVE_ANGLES[] = {60, 120, 60, 120, SERVO_CENTER};
const byte WAVE_STEPS = 5;


//======================================================
// 6. MOTORS - the only code that talks to the L298N
//======================================================

// Directions for one side of the car
const int DIR_STOP = 0;
const int DIR_FORWARD = 1;
const int DIR_BACKWARD = -1;

// Percent of a speed, using whole numbers (for example 85 % of 255 = 216)
int percentOf(int speed, int percent)
{
    return speed * percent / 100;
}

// Speed of the left and right wheels (0-255)
void setSpeed(int leftSpeed, int rightSpeed)
{
    analogWrite(ENA, leftSpeed);
    analogWrite(ENB, rightSpeed);
}

// Which way each side turns
void setWheels(int leftDir, int rightDir)
{
    // Left side:  forward = IN1 LOW + IN2 HIGH
    digitalWrite(IN1, leftDir == DIR_BACKWARD ? HIGH : LOW);
    digitalWrite(IN2, leftDir == DIR_FORWARD ? HIGH : LOW);

    // Right side: forward = IN3 HIGH + IN4 LOW
    digitalWrite(IN3, rightDir == DIR_FORWARD ? HIGH : LOW);
    digitalWrite(IN4, rightDir == DIR_BACKWARD ? HIGH : LOW);
}

// Spin speed: the dance uses a fixed speed, everything else 85 % of currentSpeed
// The part of the song the dance is in (its name number + maybe POWER)
byte dancePartByte()
{
    return pgm_read_byte(&SONG_MAP[min(dancePart, SONG_PARTS - 1)][2]);
}

// Dance speed right now: faster in the POWER parts (the explosive bits)
int danceSpeedNow()
{
    return (dancePartByte() & POWER) ? DANCE_POWER_SPEED : DANCE_SPEED;
}

int turnSpeed()
{
    return danceMode ? danceSpeedNow() : percentOf(currentSpeed, TURN_SPEED_PERCENT);
}

void stop()
{
    setSpeed(0, 0);
    setWheels(DIR_STOP, DIR_STOP);
    goingForward = false;
}

void forward(int speed)
{
    setSpeed(speed, speed);
    setWheels(DIR_FORWARD, DIR_FORWARD);
    goingForward = true;
    forwardSpeed = speed;
}

void backward(int speed)
{
    setSpeed(speed, speed);
    setWheels(DIR_BACKWARD, DIR_BACKWARD);
    goingForward = false;
}

// Spin on the spot to the left: left wheels back, right wheels forward
void left()
{
    int speed = turnSpeed();
    setSpeed(speed, speed);
    setWheels(DIR_BACKWARD, DIR_FORWARD);
    goingForward = false;
}

// Spin on the spot to the right: left wheels forward, right wheels back
void right()
{
    int speed = turnSpeed();
    setSpeed(speed, speed);
    setWheels(DIR_FORWARD, DIR_BACKWARD);
    goingForward = false;
}

// Curve left while driving forward (left wheels slower)
void driftLeft()
{
    setSpeed(percentOf(currentSpeed, DRIFT_SLOW_PERCENT), currentSpeed);
    setWheels(DIR_FORWARD, DIR_FORWARD);
    goingForward = true;
    forwardSpeed = currentSpeed;
}

// Curve right while driving forward (right wheels slower)
void driftRight()
{
    setSpeed(currentSpeed, percentOf(currentSpeed, DRIFT_SLOW_PERCENT));
    setWheels(DIR_FORWARD, DIR_FORWARD);
    goingForward = true;
    forwardSpeed = currentSpeed;
}

// Spin right on the spot (used by donut mode and the dance)
void donut()
{
    int speed = danceMode ? DANCE_SPEED : currentSpeed;
    setSpeed(speed, speed);
    setWheels(DIR_FORWARD, DIR_BACKWARD);
    goingForward = false;
}

// Spin right at the current speed (used by the 180-degree turn)
void spin180()
{
    setSpeed(currentSpeed, currentSpeed);
    setWheels(DIR_FORWARD, DIR_BACKWARD);
    goingForward = false;
}

// Emergency stop for an obstacle: if it was driving forward, a short burst
// of full reverse kills the speed (a "jerk"), so it stops before it crashes.
// The burst is shorter when it was driving slower. Then the eyes go "whoa!".
void brakeStop()
{
    bool wasMoving = goingForward;

    if (wasMoving && ACTIVE_BRAKE_MS > 0)
    {
        backward(FULL_SPEED);
        delay(ACTIVE_BRAKE_MS * forwardSpeed / FULL_SPEED);
    }

    stop();

    if (wasMoving)
        surpriseUntil = millis() + SURPRISE_MS;
}

// Speed for CHAOS (a bit slower = fewer crashes)
int autoSpeed()
{
    return percentOf(currentSpeed, AUTO_SPEED_PERCENT);
}

// Q / W on the phone and button 2 on the remote use this
void setSlowMode(bool on)
{
    slowMode = on;
    currentSpeed = on ? SLOW_SPEED : FULL_SPEED;
}


//======================================================
// 7. BUZZER - tunes that play in the background
//
//    A tune is a list of {frequency in Hz, length in ms} pairs.
//    0 Hz is a short silence, and END_OF_TUNE marks the end.
//    The tunes live in flash memory (PROGMEM) to save RAM.
//
//    HOW THE SOUND IS MADE: the usual tone() function needs Timer2, but the
//    IR receiver already uses Timer2 (it ticks about 20,000 times a second).
//    So we don't use tone(). Instead a tiny interrupt rides along on the IR
//    receiver's tick and flips the buzzer pin at the right moments.
//    That way the buzzer and the IR remote both work at the same time.
//    (Don't add tone() anywhere - it would fight over Timer2.)
//======================================================

const uint16_t REST = 0;
const uint16_t NOTE_G4 = 392;
const uint16_t NOTE_C5 = 523;
const uint16_t NOTE_E5 = 659;
const uint16_t NOTE_G5 = 784;
const uint16_t NOTE_A5 = 880;
const uint16_t NOTE_C6 = 1047;
const uint16_t WOBBLE_C6 = 1078;     // a little above C6, for a "wobbly" long note
const uint16_t NOTE_E6 = 1319;
const uint16_t NOTE_BEEP = 1980;     // high warning beep (the cleanest high note on this timer)

#define END_OF_TUNE 0, 0

// Power-on fanfare: "ta-na-na-na NAAAA"
const uint16_t TUNE_FANFARE[] PROGMEM = {
    NOTE_G4, 120,  REST, 25,
    NOTE_C5, 120,  REST, 25,
    NOTE_E5, 120,  REST, 25,
    NOTE_G5, 170,  REST, 60,
    NOTE_C6, 55, WOBBLE_C6, 55, NOTE_C6, 55, WOBBLE_C6, 55,    // the long high note,
    NOTE_C6, 55, WOBBLE_C6, 55, NOTE_C6, 55, WOBBLE_C6, 55,    // with a wobble
    NOTE_C6, 55, WOBBLE_C6, 55, NOTE_C6, 110,
    END_OF_TUNE
};

// Phone connected: "da-da-da DAAA, da-DAAAA"
const uint16_t TUNE_HELLO[] PROGMEM = {
    NOTE_C5, 90,  REST, 20,
    NOTE_E5, 90,  REST, 20,
    NOTE_G5, 90,  REST, 20,
    NOTE_C6, 220, REST, 70,
    NOTE_G5, 110, REST, 20,
    NOTE_C6, 55, WOBBLE_C6, 55, NOTE_C6, 55, WOBBLE_C6, 55,
    NOTE_C6, 55, WOBBLE_C6, 55, NOTE_C6, 110,
    END_OF_TUNE
};

// Obstacle near: two quick high beeps
const uint16_t TUNE_OBSTACLE[] PROGMEM = {
    NOTE_BEEP, 70,  REST, 70,  NOTE_BEEP, 70,
    END_OF_TUNE
};

// Turning around: four falling notes, "uh-oh-oh-ohhh"
const uint16_t TUNE_TURN_AROUND[] PROGMEM = {
    NOTE_E6, 90,  NOTE_C6, 90,  NOTE_A5, 90,  NOTE_E5, 220,
    END_OF_TUNE
};

// Runs on every Timer2 tick, but only while a note is playing.
// buzzerPhase is a counter that goes round and round; its top bit
// switches the buzzer on and off. A bigger step = a higher note.
ISR(TIMER2_COMPA_vect)
{
    buzzerPhase += buzzerStep;

    if (buzzerPhase & 0x8000)
        *buzzerPort |= buzzerMask;
    else
        *buzzerPort &= ~buzzerMask;
}

// Play one note (frequency in Hz) until the next call; 0 = silence
void buzzerNote(unsigned int frequency)
{
    if (frequency == 0 || buzzerTickHz == 0 || muted)    // muted (phone X): every sound stays silent
    {
        noInterrupts();
        TIMSK2 &= ~_BV(OCIE2A);           // stop the tick interrupt
        interrupts();
        digitalWrite(BUZZER_PIN, BUZZER_OFF_LEVEL);
        return;
    }

    uint16_t step = (uint32_t)frequency * 65536UL / buzzerTickHz;

    noInterrupts();                       // change these together, safely
    buzzerStep = step;
    TIFR2 = _BV(OCF2A);                   // forget an old, waiting tick
    TIMSK2 |= _BV(OCIE2A);                // start the tick interrupt
    interrupts();
}

// Call once, after IrReceiver.begin() has started Timer2
void setupBuzzer()
{
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, BUZZER_OFF_LEVEL);

    buzzerPort = portOutputRegister(digitalPinToPort(BUZZER_PIN));
    buzzerMask = digitalPinToBitMask(BUZZER_PIN);

    // The IR receiver set Timer2 to "count up to OCR2A, clock divided by 8".
    bool countsToOcr2a = (TCCR2A & _BV(WGM21)) && !(TCCR2B & _BV(WGM22));
    bool divideBy8 = (TCCR2B & 0x07) == _BV(CS21);

    if (countsToOcr2a && divideBy8)
        buzzerTickHz = F_CPU / 8 / (OCR2A + 1UL);
    else
        Serial.println(F("Buzzer: Timer2 not set up by the IR receiver - no sound"));
}

// More notes for the jingles below
const uint16_t NOTE_A4 = 440;
const uint16_t NOTE_BB4 = 466;      // B-flat
const uint16_t NOTE_D5 = 587;
const uint16_t NOTE_EB5 = 622;      // E-flat
const uint16_t NOTE_F5 = 698;
const uint16_t NOTE_B5 = 988;
const uint16_t NOTE_D6 = 1175;
const uint16_t NOTE_F6 = 1397;
const uint16_t NOTE_G6 = 1568;
const uint16_t NOTE_A6 = 1760;
const uint16_t NOTE_B6 = 1976;
const uint16_t NOTE_C7 = 2093;

// A little jingle when each mode starts
const uint16_t TUNE_SEARCH[] PROGMEM = {          // "bee-boo-bee-BEEP"
    NOTE_E6, 80, REST, 20, NOTE_C6, 80, REST, 20, NOTE_E6, 80, REST, 20, NOTE_G6, 180,
    END_OF_TUNE
};
const uint16_t TUNE_CHAOS[] PROGMEM = {           // crazy jumping notes
    NOTE_G6, 40, NOTE_G5, 40, NOTE_E6, 40, NOTE_E5, 40, NOTE_C7, 40, NOTE_C5, 40,
    NOTE_A6, 40, NOTE_A4, 60, REST, 40, NOTE_C7, 140,
    END_OF_TUNE
};
const uint16_t TUNE_AUTO_FORWARD[] PROGMEM = {    // "let's roll!"
    NOTE_G5, 70, NOTE_C6, 70, NOTE_E6, 70, NOTE_G6, 160,
    END_OF_TUNE
};
const uint16_t TUNE_AUTO_BACKWARD[] PROGMEM = {   // truck reversing: beep, beep, beep
    NOTE_B5, 150, REST, 150, NOTE_B5, 150, REST, 150, NOTE_B5, 150,
    END_OF_TUNE
};
const uint16_t TUNE_DONUT[] PROGMEM = {           // a spiral going up
    NOTE_C6, 50, NOTE_D6, 50, NOTE_E6, 50, NOTE_F6, 50, NOTE_G6, 50, NOTE_A6, 50, NOTE_B6, 50, NOTE_C7, 120,
    END_OF_TUNE
};
const uint16_t TUNE_DEMO[] PROGMEM = {            // "ta-da-da-DAAA"
    NOTE_C5, 100, NOTE_E5, 100, NOTE_G5, 100, NOTE_C6, 300,
    END_OF_TUNE
};

// Drum sounds for the dance (a passive buzzer can't do real drums, so these
// are quick little pitch drops and clicks that sound like them)
const uint16_t SOUND_KICK[] PROGMEM = {           // bass drum: a quick drop in pitch
    330, 15,  196, 15,  110, 25,
    END_OF_TUNE
};
const uint16_t SOUND_CLAP[] PROGMEM = {           // clap: three quick high bursts
    NOTE_A6, 8,  REST, 6,  NOTE_G6, 8,  REST, 6,  NOTE_F6, 14,
    END_OF_TUNE
};
const uint16_t SOUND_ZAP[] PROGMEM = {            // the special moment: an electric crackle (like Feedback's lightning)
    1800, 12,  600, 12,  2200, 12,  500, 12,  2600, 12,  450, 12,  2000, 20,  700, 20,
    2400, 30,  REST, 20,  3000, 40,  1500, 60,
    END_OF_TUNE
};
const uint16_t SOUND_HIT[] PROGMEM = {            // big accent: "ta-daa"
    NOTE_C6, 25,  NOTE_G6, 25,  NOTE_C7, 60,
    END_OF_TUNE
};

// Single beeps (used by the parking sensor and the dance)
bool beepPlaying = false;
unsigned long beepStart = 0;
unsigned int beepLength = 0;

// The tune is over
void stopSound()
{
    buzzerNote(0);
    tunePos = NULL;
    beepPlaying = false;
}

// Start the note that tunePos points at
void startNote()
{
    unsigned int frequency = pgm_read_word(tunePos);
    noteLength = pgm_read_word(tunePos + 1);

    if (noteLength == 0)            // END_OF_TUNE
    {
        stopSound();
        return;
    }

    buzzerNote(frequency);          // REST (0 Hz) gives silence
    noteStart = millis();
}

// Start playing a tune (a new tune replaces the one that is playing)
void playSound(const uint16_t* tune)
{
    beepPlaying = false;
    tunePos = tune;
    startNote();
}

// Play one short beep (replaces whatever is playing)
void playBeep(unsigned int frequency, unsigned int ms)
{
    tunePos = NULL;
    buzzerNote(frequency);
    beepPlaying = true;
    beepStart = millis();
    beepLength = ms;
}

bool soundPlaying()
{
    return tunePos != NULL || beepPlaying;
}

// Moves on to the next note when it is time
void updateSound()
{
    if (beepPlaying && millis() - beepStart >= beepLength)
    {
        beepPlaying = false;
        buzzerNote(0);
    }

    if (tunePos == NULL)
        return;

    if (millis() - noteStart < noteLength)
        return;

    tunePos += 2;                   // next {frequency, length} pair
    startNote();
}

void updateDance();                 // (they're further down - these lines just let yield() use them)
void update180();

// Arduino calls yield() again and again while delay() is waiting.
// We use that spare time to keep the music playing in time, the dance
// moves exactly on the beat, and to stop the 180 turn at exactly the right moment.
void yield()
{
    updateSound();
    updateDance();
    update180();
}


//======================================================
// 8. OLED SCREEN: animated robot eyes (0.96" SSD1306, 128 x 64 pixels)
//
//    top row    : "BT:OK SPEED:FULL"  (or "Hello!")
//    middle     : two big rounded eyes. Their mood follows what the robot
//                 does: happy (hello, dance), angry (chaos), focused (search),
//                 sleepy (bored), surprised (emergency stop), dizzy (donut).
//                 They look where the robot turns, bounce to the dance and blink.
//    bottom row : what it's doing + the distance, e.g. "SEARCH   42 cm"
//
//    Drawing takes time, so the eyes are only redrawn when they change, one
//    part of the screen per pass of loop(), and the screen is left alone in
//    the middle of the 180 turn (so the turn still stops on time).
//======================================================

// Eye shape (pixels)
const int EYE_W = 36;                // normal eye width
const int EYE_H = 34;                // normal eye height
const int EYE_MAX_W = 40;
const int EYE_RADIUS = 8;            // how round the corners are
const int EYE_GAP_CENTER = 30;       // each eye's centre is this far from the screen centre
const int EYE_CENTER_Y = 32;
const int LOOK_X = 16;               // how far the eyes move when looking left/right ...
const int LOOK_Y = 5;                // ... and up/down
const byte EYES_TOP_ROW = 1;         // the eyes use rows 1 - 6 (pixels 8 - 55)
const byte EYES_BOTTOM_ROW = 6;
// how far a rounded corner cuts in, for each column from the side (radius 8)
const byte CORNER_INSET[8] = {7, 4, 3, 2, 1, 1, 0, 0};

// The eyes are described by 8 numbers. "Now" moves halfway to "target"
// on every new picture, so the eyes move smoothly.
const byte E_W = 0;                  // width
const byte E_H = 1;                  // height
const byte E_X = 2;                  // looking left/right
const byte E_Y = 3;                  // looking up/down
const byte E_HAPPY = 4;              // bottom cut away by an arch: ^ ^
const byte E_ANGRY = 5;              // inner top corners cut away: frowning
const byte E_TIRED = 6;              // outer top corners droop: sad / sleepy
const byte E_OPEN = 7;               // 100 = open, 0 = closed (blink)
const byte E_WOBBLE = 8;             // dizzy: left eye this many pixels down, right eye up
const byte EYE_VALUES = 9;
int eyeNow[EYE_VALUES] = {EYE_W, EYE_H, 0, 0, 0, 0, 0, 100, 0};
int eyeTarget[EYE_VALUES];

bool eyesDrawn = false;
unsigned long eyesDrawnAt = 0;
unsigned long blinkStart = 0;
unsigned long nextBlinkAt = 2500;
unsigned long nextIdleLookAt = 0;
int idleLookX = 0;
int idleLookY = 0;
// What each 8x8 tile of the eye area looked like last time (2 bits per tile:
// 0 = all dark, 1 = all lit, 2 = partly lit). Tiles that stay all dark or
// all lit are not sent again - that makes drawing much faster.
const byte TILE_DARK = 0;
const byte TILE_LIT = 1;
const byte TILE_PART = 2;
uint8_t tileStates[24];              // 96 tiles (6 rows x 16), 4 tiles per byte

byte getTileState(byte n)
{
    return (tileStates[n >> 2] >> ((n & 3) * 2)) & 3;
}

void setTileState(byte n, byte state)
{
    byte shift = (n & 3) * 2;
    tileStates[n >> 2] = (tileStates[n >> 2] & ~(3 << shift)) | (state << shift);
}

// Copy a text from flash into buf and pad it with spaces to "width" letters
void flashText(char* buf, const char* textP, byte width)
{
    strncpy_P(buf, textP, width);
    buf[width] = '\0';

    byte length = strlen(buf);
    while (length < width)
        buf[length++] = ' ';
}

// Short name of what the robot is doing (max 8 letters, stored in flash)
const char* actionName()
{
    bool blocked = lastDistance <= STOP_DISTANCE;

    switch (robotState)
    {
    case IDLE:            return PSTR("READY");
    case MANUAL_FORWARD:  return blocked ? PSTR("BLOCKED") : PSTR("FORWARD");
    case MANUAL_BACKWARD: return PSTR("BACKWARD");
    case MANUAL_LEFT:     return PSTR("LEFT");
    case MANUAL_RIGHT:    return PSTR("RIGHT");
    case AUTO_FORWARD:    return blocked ? PSTR("BLOCKED") : PSTR("AUTO FWD");
    case AUTO_BACKWARD:   return PSTR("AUTO BCK");
    case DONUT:           return PSTR("DONUT");
    case DRIFT_LEFT:      return blocked ? PSTR("BLOCKED") : PSTR("DRIFT L");
    case DRIFT_RIGHT:     return blocked ? PSTR("BLOCKED") : PSTR("DRIFT R");
    case DANCE:           return PSTR("DANCE");
    case DEMO:            return PSTR("DEMO");
    case CHAOS:           return PSTR("CHAOS!");
    case TURN180:         return PSTR("TURN 180");
    case BRAKING:         return PSTR("BRAKING");
    case SEARCH:
        if (searchStep == AVOID_LOOK_LEFT)   return PSTR("LOOK L");
        if (searchStep == AVOID_LOOK_RIGHT)  return PSTR("LOOK R");
        if (searchStep == AVOID_LOOK_CENTER) return PSTR("LOOK C");
        if (searchStep == AVOID_TURN_TO_OPEN) return searchTurnLeft ? PSTR("TURN L") : PSTR("TURN R");
        if (searchStep == AVOID_CORNER_TURN || searchStep == AVOID_CORNER_CHECK) return PSTR("BOXED IN");
        return PSTR("SEARCH");
    }

    return PSTR("?");
}

// The distance as 8 letters, for example "  42 cm " or " --- cm "
void distanceText(char* buf)
{
    if (lastDistance >= NO_ECHO)
    {
        flashText(buf, PSTR(" --- cm "), 8);
        return;
    }

    char number[7];
    itoa(lastDistance, number, 10);

    byte i = 0;
    while (i + strlen(number) < 4)       // right-align the number in 4 places
        buf[i++] = ' ';

    strcpy(buf + i, number);
    strcat_P(buf, PSTR(" cm "));
}

// The song name, 16 letters wide. A longer name scrolls along like a news ticker.
// The special moment's text (SONG_SHOUT), in the middle of the top row
void shoutText(char* buf)
{
    byte length = min(strlen_P(SONG_SHOUT), 16);
    memset(buf, ' ', 16);
    memcpy_P(buf + (16 - length) / 2, SONG_SHOUT, length);
    buf[16] = '\0';
}

void songNameText(char* buf)
{
    byte length = strlen_P(SONG_NAME);

    if (length <= 16)
    {
        byte left = (16 - length) / 2;             // centre it
        memset(buf, ' ', 16);
        memcpy_P(buf + left, SONG_NAME, length);
        buf[16] = '\0';
        return;
    }

    byte loopLength = length + 4;                  // 4 spaces between the end and the start again
    byte start = ((millis() - danceStartMs) / SONG_NAME_SCROLL_MS) % loopLength;   // starts with the first letter

    for (byte i = 0; i < 16; i++)
    {
        byte n = (start + i) % loopLength;
        buf[i] = (n < length) ? pgm_read_byte(SONG_NAME + n) : ' ';
    }
    buf[16] = '\0';
}

// Bottom row while dancing: the part of the song + where we are in the song,
// for example "CHORUS    1:23  " (it should match the time on your music player)
void danceInfoText(char* buf)
{
    byte part = dancePartByte() & ~POWER;
    flashText(buf, PART_NAMES[part], 8);

    unsigned int seconds = (millis() - danceStartMs + SONG_FIRST_BEAT_MS) / 1000;
    char clock[8];
    itoa(seconds / 60, clock, 10);
    strcat_P(clock, (seconds % 60 < 10) ? PSTR(":0") : PSTR(":"));
    itoa(seconds % 60, clock + strlen(clock), 10);

    byte i = 8;
    while (i + strlen(clock) < 14)                 // right-align the clock
        buf[i++] = ' ';
    strcpy(buf + i, clock);
    strcat_P(buf, PSTR("  "));
}

// What the top row (area 0) or bottom row (area 1) should show
void wantedText(byte area, char* buf)
{
    if (greetingOnScreen)
    {
        if (area == 0)
            flashText(buf, greetingFromPhone ? PSTR("Phone connected!") : PSTR("    Hello! :-)"), 16);
        else
            flashText(buf, greetingFromPhone ? PSTR("    LET'S GO!") : PSTR("   I'm awake!"), 16);
        return;
    }

    if (danceMode)
    {
        if (area == 0 && (long)(shoutUntil - millis()) > 0)
            shoutText(buf);                    // the special moment, e.g. "FEEDBACK!"
        else if (area == 0)
            songNameText(buf);
        else
            danceInfoText(buf);
        return;
    }

    if (area == 0)
    {
        flashText(buf, btGreeted ? PSTR("BT:OK") : PSTR("BT:--"), 5);
        if (muted)
            strcat_P(buf, slowMode ? PSTR(" MUTED SLOW") : PSTR(" MUTED FULL"));
        else
            strcat_P(buf, slowMode ? PSTR(" SPEED:SLOW") : PSTR(" SPEED:FULL"));
    }
    else
    {
        flashText(buf, actionName(), 8);
        distanceText(buf + 8);
    }
}

// Draws a row of small text, but only the letters that changed
void drawText(byte row, const char* text, char* shown)
{
    for (byte i = 0; i < 16; i++)
    {
        if (text[i] == shown[i])
            continue;

        oled.drawGlyph(i, row, text[i]);
        shown[i] = text[i];
    }

    shown[16] = '\0';
}

// Where the eyes look while the servo turns the sensor: only the big
// looks (not the little glances while driving), so the eyes stay calm
int servoLook(int toLeft)
{
    int angle = scanner.read() - SERVO_CENTER;    // + = the sensor looks to the robot's left

    if (SERVO_LEFT < SERVO_CENTER)                // (unless SERVO_LEFT and SERVO_RIGHT were swapped)
        angle = -angle;

    if (abs(angle) < 30)
        return 0;

    return angle * toLeft / 50;
}

// Decides the mood and where to look, from what the robot is doing
void setEyeTargets()
{
    unsigned long now = millis();

    // The eyes face you, so the robot's left is on YOUR right side of the screen
    int toLeft = EYES_FLIP ? -LOOK_X : LOOK_X;

    // normal eyes, looking straight ahead
    eyeTarget[E_W] = EYE_W;
    eyeTarget[E_H] = EYE_H;
    eyeTarget[E_X] = 0;
    eyeTarget[E_Y] = 0;
    eyeTarget[E_HAPPY] = 0;
    eyeTarget[E_ANGRY] = 0;
    eyeTarget[E_TIRED] = 0;
    eyeTarget[E_OPEN] = 100;
    eyeTarget[E_WOBBLE] = 0;

    if (robotState != IDLE)
        lastActiveAt = now;

    if ((long)(surpriseUntil - now) > 0)           // just did an emergency stop: "whoa!"
    {
        eyeTarget[E_W] = EYE_W - 8;
        eyeTarget[E_H] = EYE_H + 10;
    }
    else if (robotState == IDLE && (long)(dizzyUntil - now) > 0)   // just stopped a donut: very dizzy
    {
        byte spot = (now / 90) % 4;                // eyes roll round fast, one up and one down
        eyeTarget[E_X] = (spot == 0) ? toLeft : (spot == 2) ? -toLeft : 0;
        eyeTarget[E_Y] = (spot == 1) ? -LOOK_Y : (spot == 3) ? LOOK_Y : 0;
        eyeTarget[E_WOBBLE] = (spot < 2) ? 6 : -6;
        eyeTarget[E_TIRED] = 8;
        eyeTarget[E_OPEN] = 75;
    }
    else if (greetingOnScreen)                     // hello: happy, looking around with the servo
    {
        eyeTarget[E_HAPPY] = 26;
        eyeTarget[E_X] = servoLook(toLeft);
    }
    else
    {
        switch (robotState)
        {
        case IDLE:
            if (now - lastActiveAt > SLEEPY_AFTER_MS)   // bored: sleepy, half-closed eyes
            {
                eyeTarget[E_OPEN] = 45;
                eyeTarget[E_TIRED] = 6;
                eyeTarget[E_Y] = 4;
            }
            else                                        // look around now and then
            {
                if ((long)(now - nextIdleLookAt) >= 0)
                {
                    nextIdleLookAt = now + random(1500, 4000);
                    byte pick = random(5);
                    idleLookX = (pick == 1) ? toLeft : (pick == 2) ? -toLeft : 0;
                    idleLookY = (pick == 3) ? -LOOK_Y : 0;
                }
                eyeTarget[E_X] = idleLookX;
                eyeTarget[E_Y] = idleLookY;
            }
            break;

        case MANUAL_FORWARD:
            eyeTarget[E_Y] = -2;
            break;

        case AUTO_FORWARD:                              // determined
            eyeTarget[E_Y] = -2;
            eyeTarget[E_ANGRY] = 8;
            break;

        case MANUAL_BACKWARD:
        case AUTO_BACKWARD:                             // looking down, a bit worried
            eyeTarget[E_Y] = LOOK_Y;
            eyeTarget[E_TIRED] = 8;
            break;

        case MANUAL_LEFT:
        case DRIFT_LEFT:
            eyeTarget[E_X] = toLeft;
            break;

        case MANUAL_RIGHT:
        case DRIFT_RIGHT:
            eyeTarget[E_X] = -toLeft;
            break;

        case TURN180:                                   // focused, on a mission: narrowed eyes, firm brow,
            eyeTarget[E_X] = -toLeft;                   // looking where it spins (to the right)
            eyeTarget[E_H] = EYE_H - 12;
            eyeTarget[E_W] = EYE_W + 4;
            eyeTarget[E_ANGRY] = 7;
            break;

        case DONUT:                                     // dizzy: the eyes go round and round, wobbling
        {
            byte spot = (now / 100) % 4;
            eyeTarget[E_X] = (spot == 0) ? toLeft : (spot == 2) ? -toLeft : 0;
            eyeTarget[E_Y] = (spot == 1) ? -LOOK_Y : (spot == 3) ? LOOK_Y : 0;
            eyeTarget[E_WOBBLE] = (spot == 0 || spot == 2) ? 5 : -5;
            eyeTarget[E_TIRED] = 5;
            eyeTarget[E_OPEN] = 85;
            break;
        }

        case DANCE:                                     // happy, bouncing to the beat
        {
            eyeTarget[E_HAPPY] = 24;
            eyeTarget[E_Y] = (danceStep % 2) ? 3 : -4;  // up and down on every step

            if (danceMove == SPIN_L || danceMove == TWIST_L || danceMove == SWAY_L || danceMove == LOOK_L)
                eyeTarget[E_X] = toLeft;
            else if (danceMove == SPIN_R || danceMove == TWIST_R || danceMove == SWAY_R || danceMove == LOOK_R)
                eyeTarget[E_X] = -toLeft;

            if ((long)(shoutUntil - now) > 0)              // the special moment: wide, fierce, electric eyes
            {
                eyeTarget[E_HAPPY] = 0;
                eyeTarget[E_ANGRY] = 10;
                eyeTarget[E_W] = EYE_W - 6;
                eyeTarget[E_H] = EYE_H + 10;
                eyeTarget[E_X] = ((now / 80) % 2) ? 3 : -3;   // crackling
                break;
            }

            byte part = dancePartByte() & ~POWER;
            if (part == CHORUS || part == DROP || part == HOOK || (dancePartByte() & POWER))   // the best bits: big excited eyes
            {
                eyeTarget[E_W] = EYE_W + 4;
                eyeTarget[E_H] = EYE_H + 6;
            }
            break;
        }

        case DEMO:
            eyeTarget[E_HAPPY] = 22;
            break;

        case CHAOS:                                     // mad and serious: deep frown, narrowed eyes,
            eyeTarget[E_ANGRY] = 18;                    // a steady glare (no silly darting around)
            eyeTarget[E_H] = EYE_H - 6;
            eyeTarget[E_Y] = 2;
            if (chaosStep == CHAOS_TURN)
                eyeTarget[E_X] = (chaosTurnLeft ? toLeft : -toLeft) / 2;
            break;

        case SEARCH:                                    // focused, looking where the sensor looks
            eyeTarget[E_ANGRY] = 5;
            eyeTarget[E_X] = servoLook(toLeft);
            if (searchStep == AVOID_TURN_TO_OPEN || searchStep == AVOID_CORNER_TURN)   // ... or where it is turning
                eyeTarget[E_X] = searchTurnLeft ? toLeft : -toLeft;
            break;

        case BRAKING:                                   // slowing down: squinting
            eyeTarget[E_TIRED] = 10;
            eyeTarget[E_OPEN] = 60;
            break;
        }
    }

    // blink every few seconds, at random moments
    if ((long)(now - nextBlinkAt) >= 0)
    {
        blinkStart = now;
        nextBlinkAt = now + random(BLINK_MIN_GAP_MS, BLINK_MAX_GAP_MS);
    }

    if (now - blinkStart < BLINK_MS)
        eyeTarget[E_OPEN] = 0;
}

// Moves every eye number halfway to its target. True if anything changed.
bool stepEyes()
{
    bool changed = false;

    for (byte i = 0; i < EYE_VALUES; i++)
    {
        int diff = eyeTarget[i] - eyeNow[i];

        if (diff == 0)
            continue;

        // big moves: halfway now, the rest next time (2 pictures per move).
        // Blinks and small moves happen in one go.
        int step = (i == E_OPEN || abs(diff) <= 6) ? diff : diff / 2;

        eyeNow[i] += step;
        changed = true;
    }

    return changed;
}

// Draws both eyes from the "now" numbers
void drawEyes()
{
    int w = min(eyeNow[E_W], EYE_MAX_W);
    int h = (long)eyeNow[E_H] * eyeNow[E_OPEN] / 100;
    if (h < 2)
        h = 2;                                   // a closed eye is a thin line

    int r = min(EYE_RADIUS, h / 2);
    int y0 = EYE_CENTER_Y + eyeNow[E_Y] - h / 2;  // top and bottom of the eyes
    int y1 = y0 + h - 1;
    int half = w / 2;

    // the mood cut-outs shrink when the eye closes, so a blink is a clean line
    int happy = (long)eyeNow[E_HAPPY] * eyeNow[E_OPEN] / 100;
    int angry = (long)eyeNow[E_ANGRY] * eyeNow[E_OPEN] / 100;
    int tired = (long)eyeNow[E_TIRED] * eyeNow[E_OPEN] / 100;

    // First work out, for every column of each eye, its top and bottom pixel
    int x0[2];
    uint8_t top[2][EYE_MAX_W];
    uint8_t bottom[2][EYE_MAX_W];

    for (byte eye = 0; eye < 2; eye++)
    {
        int cx = 64 + (eye == 0 ? -EYE_GAP_CENTER : EYE_GAP_CENTER) + eyeNow[E_X];
        x0[eye] = cx - half;

        // dizzy: one eye a bit lower, the other a bit higher
        int dy = (eye == 0) ? eyeNow[E_WOBBLE] : -eyeNow[E_WOBBLE];
        int ey0 = y0 + dy;
        int ey1 = y1 + dy;

        for (int i = 0; i < w; i++)
        {
            int t = ey0;
            int b = ey1;

            int edge = min(i, w - 1 - i);        // rounded corners
            if (edge < r)
            {
                int inset = CORNER_INSET[edge * 8 / r] * r / 8;
                t += inset;
                b -= inset;
            }

            if (happy > 0)                       // happy: arch-shaped bottom -> ◠ ◠
            {
                long dx = i - half;
                int cut = ey1 - happy + happy * dx * dx / ((long)half * half);
                if (b > cut)
                    b = cut;
            }

            if (angry > 0)                       // angry: inner top corner cut away
            {
                int fromInner = (eye == 0) ? (w - 1 - i) : i;
                int cut = ey0 + (long)angry * (w - fromInner) / w;
                if (t < cut)
                    t = cut;
            }

            if (tired > 0)                       // tired: outer top corner droops
            {
                int fromOuter = (eye == 0) ? i : (w - 1 - i);
                int cut = ey0 + (long)tired * (w - fromOuter) / w;
                if (t < cut)
                    t = cut;
            }

            t = max(t, EYES_TOP_ROW * 8);        // stay inside the eye area
            b = min(b, EYES_BOTTOM_ROW * 8 + 7);

            if (t > b)
            {
                t = 255;                         // nothing in this column
                b = 0;
            }

            top[eye][i] = t;
            bottom[eye][i] = b;
        }
    }

    // Then go through the eye area tile by tile (8 x 8 pixels) and send only
    // the tiles that changed. Neighbouring tiles are sent together (up to 4).
    uint8_t run[32];                             // tiles waiting to be sent
    byte runStart = 0;
    byte runLength = 0;

    for (byte row = EYES_TOP_ROW; row <= EYES_BOTTOM_ROW; row++)
    {
        int rowTop = row * 8;

        for (byte tile = 0; tile < 16; tile++)
        {
            uint8_t* bytes = run + runLength * 8;
            bool allDark = true;
            bool allLit = true;

            for (byte i = 0; i < 8; i++)
            {
                int x = tile * 8 + i;
                uint8_t column = 0;

                for (byte eye = 0; eye < 2; eye++)
                {
                    int c = x - x0[eye];

                    if (c < 0 || c >= w)
                        continue;

                    int a = max((int)top[eye][c], rowTop);
                    int b = min((int)bottom[eye][c], rowTop + 7);

                    if (a <= b)                  // pixels a..b of this column are lit
                        column |= (uint8_t)(0xFF << (a - rowTop)) & (uint8_t)(0xFF >> (rowTop + 7 - b));
                }

                bytes[i] = column;

                if (column != 0)
                    allDark = false;
                if (column != 0xFF)
                    allLit = false;
            }

            byte n = (row - EYES_TOP_ROW) * 16 + tile;
            byte state = allDark ? TILE_DARK : allLit ? TILE_LIT : TILE_PART;
            bool same = (state != TILE_PART) && (state == getTileState(n));
            setTileState(n, state);

            if (same)
            {
                if (runLength > 0)               // send what was waiting
                    oled.drawTile(runStart, row, runLength, run);
                runLength = 0;
                continue;
            }

            if (runLength == 0)
                runStart = tile;

            runLength++;

            if (runLength == 4 || tile == 15)    // the waiting tiles are full: send them
            {
                oled.drawTile(runStart, row, runLength, run);
                runLength = 0;
            }
        }
    }
}

// Jump the eyes straight to their target and draw them now
// (used at the start of the 180 turn, when the screen then waits)
void drawEyesNow()
{
    if (!oledFound)
        return;

    setEyeTargets();

    for (byte i = 0; i < EYE_VALUES; i++)
        eyeNow[i] = eyeTarget[i];

    drawEyes();
    eyesDrawn = true;
    eyesDrawnAt = millis();
}

// Called from loop(): animates the eyes, or updates one text row
void updateDisplay()
{
    if (!oledFound || turning180)
        return;

    // While dancing, don't start drawing just before the next step: a picture
    // can take up to 60 ms, and the step would come late (off the beat)
    if (danceMode && (long)(danceNextUs - micros()) < (long)DANCE_DRAW_GUARD_US)
        return;

    setEyeTargets();

    if (millis() - eyesDrawnAt >= EYES_FRAME_MS && (stepEyes() || !eyesDrawn))
    {
        drawEyes();
        eyesDrawn = true;
        eyesDrawnAt = millis();
        return;
    }

    for (byte i = 0; i < 2; i++)
    {
        byte area = (nextText + i) % 2;
        char* shown = (area == 0) ? shownTop : shownBottom;
        char want[17];

        wantedText(area, want);

        if (strcmp(want, shown) != 0 && millis() - textDrawnAt[area] >= OLED_REFRESH_MS)
        {
            drawText(area == 0 ? 0 : 7, want, shown);
            textDrawnAt[area] = millis();
            nextText = (area + 1) % 2;
            return;
        }
    }
}

// Finds the screen (address 0x3C or 0x3D) and starts it
void setupDisplay()
{
    Wire.begin();
    Wire.setWireTimeout(3000, true);     // a loose OLED wire can never freeze the robot

    byte address = 0;

    Wire.beginTransmission(0x3C);
    if (Wire.endTransmission() == 0)
        address = 0x3C;
    else
    {
        Wire.beginTransmission(0x3D);
        if (Wire.endTransmission() == 0)
            address = 0x3D;
    }

    if (address == 0)
    {
        Serial.println(F("OLED not found - check the SDA/SCL wires"));
        return;
    }

    oledFound = true;
    oled.setI2CAddress(address * 2);     // the library wants the address doubled
    oled.setBusClock(400000);            // fast I2C = less time spent drawing
    oled.begin();                        // also clears the screen
    oled.setFont(u8x8_font_chroma48medium8_r);

    memset(shownTop, ' ', 16);           // the screen starts blank (= spaces)
    memset(shownBottom, ' ', 16);
    memset(tileStates, 0, sizeof(tileStates));   // and the eye area starts all dark

    Serial.println(F("OLED ready"));
}


//======================================================
// 9. DISTANCE SENSOR (HC-SR04)
//======================================================

// One ping: returns the distance in cm, or NO_ECHO (999) if nothing answered.
int readDistance()
{
    // A 10-microsecond pulse on TRIG makes the sensor send a "click" of sound
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // ECHO stays HIGH as long as the sound took to come back
    long duration = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);

    if (duration == 0)
        return NO_ECHO;

    // Sound moves 0.0343 cm per microsecond and travels there AND back:
    // cm = duration * 0.0343 / 2 = duration * 343 / 20000 (same result, whole numbers)
    return duration * 343L / 20000L;
}

// The middle one of three numbers
int median3(int a, int b, int c)
{
    if (a > b)
    {
        int t = a;
        a = b;
        b = t;
    }

    if (b > c)
    {
        int t = b;
        b = c;
        c = t;
    }

    if (a > b)
    {
        int t = a;
        a = b;
        b = t;
    }

    return b;
}

// Is the robot driving forward right now (so an obstacle matters)?
bool drivingForward()
{
    return robotState == MANUAL_FORWARD || robotState == AUTO_FORWARD
        || robotState == DRIFT_LEFT || robotState == DRIFT_RIGHT
        || (robotState == CHAOS && chaosStep == CHAOS_STRAIGHT)
        || (robotState == SEARCH && searchStep == AVOID_DRIVE);
}

// Two quick beeps the first time something gets too close while driving forward
void obstacleBeepCheck(int distance)
{
    if (distance > SAFE_DISTANCE)
    {
        obstacleWarned = false;              // the way is clear: ready to warn next time
    }
    else if (distance <= STOP_DISTANCE && !obstacleWarned && drivingForward())
    {
        obstacleWarned = true;
        playSound(TUNE_OBSTACLE);
    }
}

// Parking-sensor beeps: while driving forward toward something, beep
// faster and faster the closer it gets. (Within STOP_DISTANCE the robot
// stops and the double "obstacle" beep takes over.)
unsigned long lastParkBeep = 0;

void updateParkingBeeps()
{
    if (!PARKING_BEEPS || !drivingForward() || soundPlaying())
        return;

    if (lastDistance <= STOP_DISTANCE || lastDistance > PARK_BEEP_FROM_CM)
        return;

    // closer = shorter gap between beeps
    unsigned long gap = PARK_GAP_NEAR_MS
        + (unsigned long)(lastDistance - STOP_DISTANCE) * (PARK_GAP_FAR_MS - PARK_GAP_NEAR_MS)
          / (PARK_BEEP_FROM_CM - STOP_DISTANCE);

    if (millis() - lastParkBeep < gap)
        return;

    lastParkBeep = millis();
    playBeep(PARK_BEEP_HZ, PARK_BEEP_MS);
}

// Three pings and keep the middle one - this ignores the odd bad reading
int filteredDistance()
{
    int a = readDistance();
    delay(PING_GAP_MS);

    int b = readDistance();
    delay(PING_GAP_MS);

    int c = readDistance();

    int distance = median3(a, b, c);

    lastDistance = distance;                 // for the OLED
    obstacleBeepCheck(distance);

    return distance;
}

// Runs on every pass of loop(). While you drive forward by hand (F / UP),
// it stops the car if something is closer than STOP_DISTANCE.
// NOTE: this measurement takes about 40-60 ms, which also sets the rhythm of
// loop(). The timed moves (180 turn, dance, demo) were tuned with it,
// so keep this measurement running every loop.
void checkObstacle()
{
    int distance = filteredDistance();

    if (robotState == MANUAL_FORWARD)
    {
        if (distance <= STOP_DISTANCE)
        {
            brakeStop();                       // jerk-stop (only the first time)
        }
        else
        {
            forward(currentSpeed);
        }
    }
}


//======================================================
// 10. SERVO - points the distance sensor
//======================================================

void lookCenter()
{
    scanner.write(SERVO_CENTER);
}

void lookLeft()
{
    scanner.write(SERVO_LEFT);
}

void lookRight()
{
    scanner.write(SERVO_RIGHT);
}


//======================================================
// 11. GREETING - "Hello!" on the screen + a tune + a servo wave
//======================================================

void startGreeting(bool fromPhone)
{
    greetingFromPhone = fromPhone;
    greetingOnScreen = true;             // the OLED code now shows the hello screen
    greetingStart = millis();

    playSound(fromPhone ? TUNE_HELLO : TUNE_FANFARE);

    if (robotState == IDLE)              // only wave while the car stands still
    {
        waveStep = 1;
        waveTimer = millis();
        scanner.write(WAVE_ANGLES[0]);
    }

    Serial.println(fromPhone ? F("HELLO PHONE") : F("HELLO"));
}

// Called from loop(): after a donut, the head wobbles left and right while
// the robot stands still and is dizzy, then looks straight again
void updateDizzy()
{
    if (dizzyUntil == 0)
        return;

    if (robotState != IDLE || (long)(millis() - dizzyUntil) >= 0)
    {
        dizzyUntil = 0;                  // over (or a new command came): head straight again
        if (robotState == IDLE)
            lookCenter();
        return;
    }

    if (millis() - dizzyWobbleAt < DIZZY_WOBBLE_MS)
        return;

    dizzyWobbleAt = millis();
    bool toLeft = scanner.read() <= SERVO_CENTER;
    scanner.write(SERVO_CENTER + (toLeft ? DIZZY_WOBBLE_DEGREES : -DIZZY_WOBBLE_DEGREES));
}

// Called from loop(): moves the wave along and ends the hello screen
void updateGreeting()
{
    if (greetingOnScreen && millis() - greetingStart >= GREETING_SCREEN_MS)
        greetingOnScreen = false;

    if (waveStep == 0)
        return;

    if (robotState != IDLE)              // the car started moving: stop waving, look ahead
    {
        waveStep = 0;
        lookCenter();
        return;
    }

    if (millis() - waveTimer < WAVE_STEP_MS)
        return;

    waveTimer = millis();

    if (waveStep < WAVE_STEPS)
    {
        scanner.write(WAVE_ANGLES[waveStep]);
        waveStep++;
    }
    else
    {
        waveStep = 0;                    // done - the last position was the centre
    }
}


//======================================================
// 12. TIMED MOVES - smooth brake and 180-degree turn
//     These start a move, and an "update" function called from
//     loop() finishes it when the time is up (no waiting around).
//======================================================

void cancelAutoModes();             // (it's further down, in section 13 - this line just lets us use it here)

// Smooth brake: slows the car down to a stop (any self-driving mode stops too)
void beginBrake()
{
    cancelAutoModes();
    braking = true;
    brakePWM = currentSpeed;
    robotState = BRAKING;
}

// Lowers the speed a little every BRAKE_STEP_MS until the car stops
void updateBrake()
{
    static unsigned long lastStep = 0;

    if (!braking)
        return;

    if (millis() - lastStep < BRAKE_STEP_MS)
        return;

    lastStep = millis();

    brakePWM -= BRAKE_STEP_PWM;

    if (brakePWM < 0)
        brakePWM = 0;

    setSpeed(brakePWM, brakePWM);

    if (brakePWM == 0)
    {
        braking = false;
        stop();
        robotState = IDLE;
    }
}

void start180()
{
    turning180 = true;
    turn180Timer = millis();
    robotState = TURN180;
    spin180();

    playSound(TUNE_TURN_AROUND);

    // Show it right at the start of the turn; after this the screen is left
    // alone until the turn is over, so the turn still stops on time.
    drawEyesNow();
}

// Stops the spin after TURN180_TIME
void update180()
{
    if (!turning180)
        return;

    if (millis() - turn180Timer >= TURN180_TIME)
    {
        turning180 = false;
        stop();

        if (!demoMode)
            robotState = IDLE;
    }
}


//======================================================
// 13. STOP / START HELPERS
//======================================================

// Stopping a donut makes the robot dizzy for a moment (eyes + wobbly head)
void dizzyIfDonut()
{
    if (donutMode)
    {
        dizzyUntil = millis() + DIZZY_AFTER_DONUT_MS;
        dizzyWobbleAt = millis();
    }
}

// Stops every mode and the motors, and centres the sensor
void emergencyStop()
{
    dizzyIfDonut();
    autoForward = false;
    autoBackward = false;
    donutMode = false;

    danceMode = false;
    danceStep = 0;

    braking = false;
    turning180 = false;

    demoMode = false;
    demoStep = 0;

    chaosMode = false;
    chaosStep = CHAOS_STRAIGHT;

    searchMode = false;
    searchStep = AVOID_DRIVE;

    robotState = IDLE;
    stop();

    lookCenter();

    nextBlinkAt = millis();            // the eyes blink at every new command

    Serial.println(F("STOP"));
}

// Switches off every mode that drives by itself (auto forward/backward, donut,
// dance, demo, chaos, search) without stopping the motors - the button that
// called this decides what the motors do next. Used by the IR remote buttons
// and the smooth brake, so a running dance or search can't fight with them.
void cancelAutoModes()
{
    dizzyIfDonut();
    autoForward = false;
    autoBackward = false;
    donutMode = false;

    if (danceMode || searchMode)
        lookCenter();                  // the sensor head may be turned

    danceMode = false;
    demoMode = false;
    chaosMode = false;
    searchMode = false;
}

void setAutoForward()
{
    cancelAutoModes();
    autoForward = true;
    robotState = AUTO_FORWARD;
    playSound(TUNE_AUTO_FORWARD);
}

void setAutoBackward()
{
    cancelAutoModes();
    autoBackward = true;
    robotState = AUTO_BACKWARD;
    playSound(TUNE_AUTO_BACKWARD);
}

void setDonut()
{
    cancelAutoModes();
    donutMode = true;
    robotState = DONUT;
    playSound(TUNE_DONUT);
}

// Press dance on the first beat of the song: the first move starts right away
void startDance()
{
    emergencyStop();
    danceMode = true;
    dancePart = 0;
    danceBar = 0;
    danceStep = 0;
    danceSectionBar = 0;
    danceStepCount = 0;
    shoutUntil = 0;
    danceMove = POSE;
    danceNextUs = micros();
    danceBeatUs = danceNextUs;
    danceStartMs = millis();
    robotState = DANCE;

    Serial.print(F("DANCE to: "));
    Serial.print((const __FlashStringHelper*)SONG_NAME);
    Serial.print(F("  BPM "));
    Serial.println((int)(SONG_BPM + 0.5));
}

// Dance button tapped again while dancing: you tapped ON a beat, so move the
// robot's beat to your tap (fixes it drifting off the music over a long song)
void danceTap()
{
    long beatUs = DANCE_STEP_US * 2;
    long sinceBeat = (long)(micros() - danceBeatUs);

    // Was the tap just after one of our beats, or just before the next one?
    long shift = (sinceBeat < beatUs / 2) ? sinceBeat : sinceBeat - beatUs;

    danceNextUs += shift;
    danceBeatUs += shift;

    Serial.print(F("DANCE beat moved by ms: "));
    Serial.println(shift / 1000);
}

void startDemo()
{
    emergencyStop();
    demoMode = true;
    demoStep = 0;
    demoTimer = millis();
    robotState = DEMO;
    playSound(TUNE_DEMO);
}

void startChaos()
{
    emergencyStop();
    chaosMode = true;
    chaosStep = CHAOS_STRAIGHT;
    chaosTimer = millis();
    chaosDuration = random(CHAOS_STRAIGHT_MIN_MS, CHAOS_STRAIGHT_MAX_MS);
    robotState = CHAOS;
    playSound(TUNE_CHAOS);
}

void searchDriveOn();                // (it's further down, in section 16 - this line just lets us use it here)

void startSearch()
{
    emergencyStop();
    searchMode = true;
    searchDriveOn();
    robotState = SEARCH;
    playSound(TUNE_SEARCH);
}


//======================================================
// 14. IR REMOTE
//======================================================

// What each remote button does
void handleRemote(byte cmd)
{
    switch (cmd)
    {
    case CMD_OK:                       // stop everything
        emergencyStop();
        break;

    case CMD_2:                        // slow / full speed
        setSlowMode(!slowMode);

        if (slowMode)
            Serial.println(F("Slow Mode ON"));
        else
            Serial.println(F("Slow Mode OFF"));

        break;

    case CMD_UP:                       // forward, only if the way is clear
        cancelAutoModes();

        if (filteredDistance() > STOP_DISTANCE)
        {
            robotState = MANUAL_FORWARD;
            forward(currentSpeed);
        }
        else
        {
            stop();
            robotState = IDLE;
        }

        break;

    case CMD_DOWN:                     // backward
        cancelAutoModes();
        robotState = MANUAL_BACKWARD;
        backward(currentSpeed);
        break;

    case CMD_LEFT:                     // short turn left
        cancelAutoModes();
        left();
        delay(IR_TURN_TIME);
        stop();
        robotState = IDLE;
        break;

    case CMD_RIGHT:                    // short turn right
        cancelAutoModes();
        right();
        delay(IR_TURN_TIME);
        stop();
        robotState = IDLE;
        break;

    case CMD_STAR:                     // auto forward
        setAutoForward();
        break;

    case CMD_HASH:                     // auto backward
        setAutoBackward();
        break;

    case CMD_0:                        // donut
        setDonut();
        break;

    case CMD_1:                        // 180-degree turn (stops any other mode first, like the phone's U)
        if (!turning180)
        {
            emergencyStop();
            start180();
        }
        break;

    case CMD_3:                        // smooth brake
        beginBrake();
        break;

    case CMD_4:                        // demo
        startDemo();
        break;

    case CMD_5:                        // dance (pressed while dancing: line up with the beat)
        if (danceMode)
            danceTap();
        else
            startDance();
        break;

    case CMD_6:                        // chaos
        startChaos();
        break;

    case CMD_7:                        // drift left
        emergencyStop();
        robotState = DRIFT_LEFT;
        break;

    case CMD_8:                        // drift right
        emergencyStop();
        robotState = DRIFT_RIGHT;
        break;

    case CMD_9:                        // search
        startSearch();
        break;
    }
}

// Checks if the IR receiver caught a button press
void handleIR()
{
    if (!IrReceiver.decode())
        return;

    byte cmd = IrReceiver.decodedIRData.command;
    bool heldDown = IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT;

    if (heldDown)
    {
        // The remote sends a "still held down" signal about every 0.1 s.
        // Only the arrow buttons keep going while held, and only when the
        // signal follows the press we just handled.
        bool arrow = (cmd == CMD_UP || cmd == CMD_DOWN || cmd == CMD_LEFT || cmd == CMD_RIGHT);

        if (!arrow || cmd != lastIRCommand || millis() - lastIRTime > IR_REPEAT_WINDOW_MS)
        {
            IrReceiver.resume();
            return;
        }
    }
    else
    {
        if (IrReceiver.decodedIRData.protocol != UNKNOWN)   // a real, fresh button press
        {
            Serial.print(F("IR button 0x"));
            Serial.println(cmd, HEX);
        }
    }

    handleRemote(cmd);

    lastIRCommand = cmd;
    lastIRTime = millis();

    IrReceiver.resume();               // get ready for the next button
}


//======================================================
// 15. BLUETOOTH - letters from the phone app
//======================================================

// Letters that start a self-driving mode
bool isModeLetter(char c)
{
    return c == 'I' || c == '1' || c == '2' || c == 'J' || c == '3' || c == '4' || c == 'Y' || c == 'U';
}

// Is the mode for this letter already running?
bool modeAlreadyRunning(char c)
{
    switch (c)
    {
        case 'I': return searchMode;
        case '1': return chaosMode;
        case '2': return robotState == AUTO_FORWARD;
        case 'J': return robotState == AUTO_BACKWARD;
        case '3': return robotState == DONUT;
        case '4': return danceMode;
        case 'Y': return demoMode;
        case 'U': return turning180;
    }
    return false;
}

void handleBluetooth()
{
    if (!BT.available())
        return;

    char c = BT.read();

    Serial.print(F("Received: "));
    Serial.println(c);

    // Phone apps send the letter again and again while you hold a button,
    // and an 'S' when you let go. So:
    //  - the same mode letter again does NOT restart the mode
    //  - an 'S' right after a mode letter is ignored, so the mode keeps going.
    //    Any other button (or 'S' later, or OK on the remote) stops it.
    if (isModeLetter(c))
    {
        bool newTap = millis() - modeLetterAt >= DANCE_TAP_GAP_MS;   // not just the button being held
        modeLetterAt = millis();

        if (modeAlreadyRunning(c))
        {
            if (c == '4' && newTap)        // dance tapped again on a beat: line up with it
                danceTap();
            return;
        }
    }
    else if (c == 'S' && modeLetterAt != 0 && millis() - modeLetterAt < BUTTON_RELEASE_IGNORE_MS)
    {
        Serial.println(F("(button let go - mode keeps going)"));
        return;
    }

    switch (c)
    {
        case 'F':                          // forward
            Serial.println(F("FORWARD"));
            emergencyStop();
            robotState = MANUAL_FORWARD;
            forward(currentSpeed);
            break;

        case 'B':                          // backward
            Serial.println(F("BACKWARD"));
            emergencyStop();
            robotState = MANUAL_BACKWARD;
            backward(currentSpeed);
            break;

        case 'L':                          // spin left
            Serial.println(F("LEFT"));
            emergencyStop();
            left();
            robotState = MANUAL_LEFT;
            break;

        case 'R':                          // spin right
            Serial.println(F("RIGHT"));
            emergencyStop();
            right();
            robotState = MANUAL_RIGHT;
            break;

        case 'G':                          // drift left
            Serial.println(F("DRIFT LEFT"));
            emergencyStop();
            robotState = DRIFT_LEFT;
            break;

        case 'H':                          // drift right
            Serial.println(F("DRIFT RIGHT"));
            emergencyStop();
            robotState = DRIFT_RIGHT;
            break;

        case 'S':                          // stop
            Serial.println(F("STOP"));
            emergencyStop();
            break;

        case 'Q':                          // slow speed
            setSlowMode(true);
            Serial.println(F("Slow Mode ON"));
            break;

        case 'W':                          // full speed
            setSlowMode(false);
            Serial.println(F("Full Speed"));
            break;

        case 'X':                          // the "!" button: mute / unmute all sounds
        case 'x':                          // (apps send X and x in turn - each press flips it)
            if (millis() - muteToggledAt >= MUTE_PRESS_GAP_MS)
            {
                muted = !muted;
                muteToggledAt = millis();
                if (muted)
                    stopSound();
                Serial.println(muted ? F("MUTED") : F("SOUND ON"));
            }
            break;

        case '2':                          // auto forward
            Serial.println(F("AUTO FORWARD"));
            emergencyStop();
            setAutoForward();
            break;

        case 'J':                          // auto backward
            Serial.println(F("AUTO BACKWARD"));
            emergencyStop();
            setAutoBackward();
            break;

        case '3':                          // donut
            Serial.println(F("DONUT"));
            emergencyStop();
            setDonut();
            break;

        case 'U':                          // 180-degree turn
            Serial.println(F("180 TURN"));
            emergencyStop();
            start180();
            break;

        case 'u':                          // smooth brake
            Serial.println(F("BRAKE"));
            beginBrake();
            break;

        case '4':                          // dance
            Serial.println(F("DANCE"));
            startDance();
            break;

        case 'Y':                          // demo
            Serial.println(F("DEMO"));
            startDemo();
            break;

        case '1':                          // chaos
            Serial.println(F("CHAOS"));
            startChaos();
            break;

        case 'I':                          // search
            Serial.println(F("SEARCH"));
            startSearch();
            break;
    }

    // The first letter from the phone means it has just connected: say hello!
    // (The HC-05 is not wired to tell us when a phone connects, so we wait
    //  for the first letter - this works with every HC-05 and every app.)
    if (!btGreeted)
    {
        btGreeted = true;
        startGreeting(true);
    }
}


//======================================================
// 16. SELF-DRIVING MODES
//     Each "update" function is called on every pass of loop().
//     It checks the clock (millis) and moves to the next step
//     when it is time.
//======================================================

// Simple driving modes that just keep going
void updateDriving()
{
    switch (robotState)
    {
    case AUTO_FORWARD:                 // forward, pause at obstacles
        if (filteredDistance() > STOP_DISTANCE)
            forward(currentSpeed);
        else
            brakeStop();
        break;

    case AUTO_BACKWARD:
        backward(currentSpeed);
        break;

    case DONUT:
        donut();
        break;

    case DRIFT_LEFT:
        if (filteredDistance() > STOP_DISTANCE)
            driftLeft();
        else
            brakeStop();
        break;

    case DRIFT_RIGHT:
        if (filteredDistance() > STOP_DISTANCE)
            driftRight();
        else
            brakeStop();
        break;

    default:
        break;
    }
}

// DANCE: follows the song (SONG_MAP) bar by bar, one step every half beat.
// The steps sit on a fixed grid measured in microseconds from the start,
// so the robot never slowly drifts behind the music. This also runs from
// yield(), so the beat stays steady even while the sensor is measuring.

// Both sides of the car: speed and direction (for the curve moves)
void danceWheels(int leftSpeed, int rightSpeed, int dir)
{
    setSpeed(leftSpeed, rightSpeed);
    setWheels(dir, dir);
    goingForward = false;
}

// Does one dance move (and turns the sensor head with it)
void doDanceMove(byte move)
{
    if (move == HOLD)                          // keep doing the last move
        return;

    // never dance forward into something
    bool goesForward = (move == FWD || move == DASH || move == SWAY_L || move == SWAY_R);
    if (goesForward && lastDistance <= STOP_DISTANCE)
        move = POSE;

    danceMove = move;

    int speed = danceSpeedNow();               // faster in the POWER parts
    int twist = percentOf(speed, DANCE_TWIST_PERCENT);
    int inner = percentOf(speed, DANCE_CURVE_PERCENT);
    int head = SERVO_CENTER;

    switch (move)
    {
    case FWD:     forward(speed);         break;
    case BACK:    backward(speed);        break;
    case DASH:    forward(FULL_SPEED);    break;
    case SPIN_L:  left();   head = SERVO_LEFT;   break;    // spins use the dance speed while dancing
    case SPIN_R:  right();  head = SERVO_RIGHT;  break;
    case TWIST_L: setSpeed(twist, twist);  setWheels(DIR_BACKWARD, DIR_FORWARD);  goingForward = false;  head = SERVO_LEFT;   break;
    case TWIST_R: setSpeed(twist, twist);  setWheels(DIR_FORWARD, DIR_BACKWARD);  goingForward = false;  head = SERVO_RIGHT;  break;
    case SWAY_L:  danceWheels(inner, speed, DIR_FORWARD);         head = SERVO_LEFT;   break;
    case SWAY_R:  danceWheels(speed, inner, DIR_FORWARD);         head = SERVO_RIGHT;  break;
    case BACK_L:  danceWheels(inner, speed, DIR_BACKWARD);        break;
    case BACK_R:  danceWheels(speed, inner, DIR_BACKWARD);        break;
    case LOOK_L:  stop();   head = SERVO_LEFT;   break;
    case LOOK_R:  stop();   head = SERVO_RIGHT;  break;
    default:      stop();                 break;            // POSE
    }

    // Head bob: on the straight moves the head turns a bit left, then right
    if (DANCE_HEAD_BOB && head == SERVO_CENTER)
    {
        danceHeadBobLeft = !danceHeadBobLeft;
        head = (danceHeadBobLeft == (SERVO_LEFT > SERVO_CENTER)) ? SERVO_CENTER + DANCE_HEAD_BOB_DEGREES
                                                                 : SERVO_CENTER - DANCE_HEAD_BOB_DEGREES;
    }

    if (DANCE_HEAD)
        scanner.write(head);
}

// The frequency of a note (MIDI number: 60 = middle C, +1 = one semitone higher)
unsigned int noteFrequency(byte midi)
{
    static const uint16_t TOP_OCTAVE[12] PROGMEM = {          // C8 .. B8
        4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902
    };
    return pgm_read_word(&TOP_OCTAVE[midi % 12]) >> (9 - midi / 12);
}

// Plays the tune's note for this step: it follows the chord of this bar.
// A note lasts until the next note (TIE steps keep it going), so it sounds
// like music instead of separate beeps.
void playTuneStep(byte tunePattern, byte chord)
{
    byte code = pgm_read_byte(&TUNE_PATTERNS[tunePattern][danceStep]);

    if (code == TIE)
        return;                                // the last note keeps going

    if (code == SIL)
    {
        buzzerNote(0);
        tunePos = NULL;
        beepPlaying = false;
        return;
    }

    // how many steps this note lasts (itself + the TIE steps after it)
    byte steps = 1;
    while (danceStep + steps < STEPS_PER_BAR
           && pgm_read_byte(&TUNE_PATTERNS[tunePattern][danceStep + steps]) == TIE)
        steps++;

    // the note: count up the scale from the chord's root
    bool bass = (code & 0xF0) == BASS;
    byte scaleSteps = (chord - 1) + (code & 0x0F);
    const byte* scale = SONG_MINOR ? MINOR_SCALE : MAJOR_SCALE;
    byte midi = 60 + SONG_KEY % 12 + 12 * (scaleSteps / 7) + pgm_read_byte(&scale[scaleSteps % 7]);

    midi += bass ? DANCE_BASS_OCTAVES * 12 : DANCE_TUNE_OCTAVES * 12;
    while (midi > DANCE_TUNE_HIGHEST)          // keep the tune in the range the buzzer sounds best
        midi -= 12;

    unsigned long ms = steps * (DANCE_STEP_US / 1000) - DANCE_NOTE_GAP_MS;
    if (bass && ms > DANCE_BASS_MAX_MS)        // bass notes are short and punchy
        ms = DANCE_BASS_MAX_MS;

    playBeep(noteFrequency(midi), ms);
}

// Plays one drum sound
void playDrum(byte sound)
{
    switch (sound)
    {
    case TICK:  playBeep(NOTE_BEEP, 12);    break;
    case TAP:   playBeep(NOTE_A5, 30);      break;
    case KICK:  playSound(SOUND_KICK);      break;
    case CLAP:  playSound(SOUND_CLAP);      break;
    case HIT:   playSound(SOUND_HIT);       break;
    }
}

// Moves on to the next step. Returns false when the song is over.
bool nextDanceStep()
{
    danceStepCount++;

    if (++danceStep < STEPS_PER_BAR)
        return true;

    danceStep = 0;

    if (++danceBar < pgm_read_byte(&SONG_MAP[dancePart][1]))
    {
        danceSectionBar++;
        return true;
    }

    danceBar = 0;

    if (++dancePart < SONG_PARTS)
    {
        // a new song part (or new chords) starts its chord progression from the top
        if ((pgm_read_byte(&SONG_MAP[dancePart][2]) & ~POWER) != (pgm_read_byte(&SONG_MAP[dancePart - 1][2]) & ~POWER)
            || pgm_read_byte(&SONG_MAP[dancePart][3]) != pgm_read_byte(&SONG_MAP[dancePart - 1][3]))
            danceSectionBar = 0;
        else
            danceSectionBar++;

        Serial.print(F("DANCE part "));
        Serial.println(dancePart);
        return true;
    }

    return false;
}

// The song is over: final pose, a "ta-da" and back to normal
void endDance()
{
    danceMode = false;
    dancePart = 0;
    stop();
    lookCenter();
    robotState = IDLE;
    playSound(TUNE_DEMO);
    Serial.println(F("DANCE finished"));
}

void updateDance()
{
    if (!danceMode)
        return;

    long late = (long)(micros() - danceNextUs);

    if (late < 0)                              // not time for the next step yet
        return;

    if (dancePart >= SONG_PARTS)               // the last step has had its time: the end
    {
        endDance();
        return;
    }

    // If something kept the Arduino busy for longer than a whole step,
    // skip the steps we missed so we stay on the song's beat
    while (late >= (long)DANCE_STEP_US)
    {
        late -= DANCE_STEP_US;
        danceNextUs += DANCE_STEP_US;

        if (!nextDanceStep())
        {
            endDance();
            return;
        }
    }

    if (danceStep % 2 == 0)                    // this step is on a beat
        danceBeatUs = danceNextUs;

    danceNextUs += DANCE_STEP_US;

    byte pattern = pgm_read_byte(&SONG_MAP[dancePart][0]);
    byte code = pgm_read_byte(&DANCE_PATTERNS[pattern][danceStep]);

    doDanceMove(code & 0x0F);

    // the song's special moment (e.g. "FEEDBACK!"): text on the screen + a zap
    bool shoutNow = false;
    for (byte i = 0; i < 3; i++)
        if (SONG_SHOUT_STEPS[i] >= 0 && danceStepCount == (unsigned int)SONG_SHOUT_STEPS[i])
            shoutNow = true;

    if (shoutNow)
    {
        shoutUntil = millis() + SHOUT_MS;
        playSound(SOUND_ZAP);
        Serial.println((const __FlashStringHelper*)SONG_SHOUT);
    }

    bool shouting = (long)(shoutUntil - millis()) > (long)(SHOUT_MS - SHOUT_SOUND_MS);   // let the zap play

    if (DANCE_SOUND == DANCE_SOUND_TUNE && !shouting)
    {
        byte chords = pgm_read_byte(&SONG_MAP[dancePart][3]);
        byte chord = pgm_read_byte(&SONG_CHORDS[chords][danceSectionBar % 4]);
        playTuneStep(pgm_read_byte(&SONG_MAP[dancePart][4]), chord);
    }
    else if (DANCE_SOUND == DANCE_SOUND_DRUMS)
    {
        playDrum(code & 0xF0);
    }

    if (!nextDanceStep())
        dancePart = SONG_PARTS;                // the end comes after this last step
}

// Demo: forward, pause, reverse, spin, 180 turn, final launch
void updateDemo()
{
    if (!demoMode)
        return;

    switch (demoStep)
    {
        case 0:                            // forward
            forward(currentSpeed);

            if (millis() - demoTimer >= DEMO_FORWARD_MS)
            {
                stop();
                demoTimer = millis();
                demoStep = 1;
            }
            break;

        case 1:                            // pause
            stop();

            if (millis() - demoTimer >= DEMO_PAUSE_MS)
            {
                demoTimer = millis();
                demoStep = 2;
            }
            break;

        case 2:                            // reverse
            backward(currentSpeed);

            if (millis() - demoTimer >= DEMO_REVERSE_MS)
            {
                stop();
                demoTimer = millis();
                demoStep = 3;
            }
            break;

        case 3:                            // spin, then start the 180 turn
            donut();

            if (millis() - demoTimer >= DEMO_SPIN_MS)
            {
                stop();
                start180();
                demoStep = 4;
            }
            break;

        case 4:                            // wait for the 180 turn to finish
            if (!turning180)
            {
                robotState = DEMO;         // give control back to the demo
                demoTimer = millis();
                demoStep = 5;
            }
            break;

        case 5:                            // final launch forward
            forward(currentSpeed);

            if (millis() - demoTimer >= DEMO_LAUNCH_MS)
            {
                stop();
                demoMode = false;
                demoStep = 0;
                robotState = IDLE;
            }
            break;
    }
}

// CHAOS: random straight runs, random left/right turns, surprise spins and
// little wiggles. At an obstacle it backs up and turns a random way.
// Because every choice is random, it wanders everywhere instead of circling.

// Start a straight run of random length
void chaosStraight()
{
    chaosStep = CHAOS_STRAIGHT;
    chaosTimer = millis();
    chaosDuration = random(CHAOS_STRAIGHT_MIN_MS, CHAOS_STRAIGHT_MAX_MS);
}

// Start a turn to a random side (a long turn = a spin)
void chaosTurn(unsigned long minMs, unsigned long maxMs)
{
    chaosStep = CHAOS_TURN;
    chaosTimer = millis();
    chaosTurnLeft = random(2);
    chaosDuration = random(minMs, maxMs);
}

// Back up for a moment, then a random turn follows
void chaosBackup()
{
    stop();
    chaosStep = CHAOS_BACKUP;
    chaosTimer = millis();
    chaosDuration = CHAOS_BACKUP_MS;
}

// After a straight run: roll the dice for the next move
void chaosNextMove()
{
    int dice = random(100);

    if (dice < CHAOS_SPIN_CHANCE)
        chaosTurn(CHAOS_SPIN_MIN_MS, CHAOS_SPIN_MAX_MS);      // surprise spin
    else if (dice < CHAOS_SPIN_CHANCE + CHAOS_WIGGLE_CHANCE)
        chaosBackup();                                       // little reverse wiggle
    else
        chaosTurn(CHAOS_TURN_MIN_MS, CHAOS_TURN_MAX_MS);      // normal random turn
}

void updateChaos()
{
    if (!chaosMode)
        return;

    unsigned long elapsed = millis() - chaosTimer;

    switch (chaosStep)
    {
    case CHAOS_STRAIGHT:
        if (filteredDistance() <= STOP_DISTANCE)   // obstacle: jerk-stop, back up, turn away
        {
            brakeStop();
            chaosBackup();
            break;
        }

        forward(autoSpeed());

        if (elapsed >= chaosDuration)
            chaosNextMove();
        break;

    case CHAOS_TURN:
        if (chaosTurnLeft)
            left();
        else
            right();

        if (elapsed >= chaosDuration)
            chaosStraight();
        break;

    case CHAOS_BACKUP:
        backward(autoSpeed());

        if (elapsed >= chaosDuration)
            chaosTurn(CHAOS_TURN_MIN_MS * 2, CHAOS_TURN_MAX_MS);   // a bigger turn after backing up
        break;
    }
}

// Drive on (forward, sensor looking ahead). Also starts the "am I stuck?" checks.
void searchDriveOn()
{
    searchStep = AVOID_DRIVE;
    stuckCheckDistance = lastDistance;
    stuckCheckAt = millis();
    noEchoChecks = 0;
    sameChecks = 0;
    cornerTurns = 0;
}

// Something in the way (or stuck): back up a little, then look around
void searchLookAround(bool stuck)
{
    stop();
    backward(currentSpeed);
    delay(stuck ? SEARCH_STUCK_BACKUP_MS : SEARCH_BACKUP_MS);
    stop();

    lookLeft();
    avoidTimer = millis();
    searchStep = AVOID_LOOK_LEFT;
}

// Start turning toward the open side, in small steps, with the sensor looking
// straight ahead, until it sees open space (see AVOID_TURN_TO_OPEN below).
// "Open" means about as much room as it saw when it looked that way
// (3/4 of it, and more than straight ahead had; at least SAFE_DISTANCE, at
// most SEARCH_OPEN_ENOUGH), so it really points into the open space, not
// just past the edge of the obstacle.
void startTurnToOpen(bool toLeft)
{
    Serial.println(toLeft ? F("SEARCH -> turn left until open") : F("SEARCH -> turn right until open"));

    int seen = toLeft ? leftDistance : rightDistance;
    searchOpenTarget = constrain(max(seen / 4 * 3, centerDistance + 10), SAFE_DISTANCE, SEARCH_OPEN_ENOUGH);

    searchTurnLeft = toLeft;
    turnSteps = 0;
    searchStep = AVOID_TURN_TO_OPEN;
}

// One small turning step toward the open side (an exact short move: delay()
// keeps the time precise, and the buzzer keeps playing during it)
void turnStep()
{
    if (searchTurnLeft)
        left();
    else
        right();

    delay(SEARCH_TURN_STEP_MS);
    stop();
}

// Boxed in: spin 90 degrees (at full speed, timed like the 180 turn),
// then check straight ahead (see AVOID_CORNER_TURN / AVOID_CORNER_CHECK below)
void startCornerTurn(bool toLeft)
{
    Serial.println(toLeft ? F("SEARCH boxed in -> 90 left") : F("SEARCH boxed in -> 90 right"));

    searchTurnLeft = toLeft;
    cornerTurns++;
    searchStep = AVOID_CORNER_TURN;

    // a short exact move: delay() keeps the time precise (the buzzer keeps playing)
    setSpeed(currentSpeed, currentSpeed);
    setWheels(toLeft ? DIR_BACKWARD : DIR_FORWARD, toLeft ? DIR_FORWARD : DIR_BACKWARD);
    goingForward = false;
    delay(TURN90_TIME);
    stop();

    avoidTimer = millis();
    searchStep = AVOID_CORNER_CHECK;
}

// After looking left and right: it NEVER goes straight on (the obstacle was
// right there a moment ago). It turns toward the side with more room:
//  - that side is open: turn until the sensor sees the open space, then drive
//  - both sides blocked (boxed in): turn 90 degrees that way, look ahead,
//    and keep turning 90 degrees until it finds an opening
void chooseClearestWay()
{
    bool toLeft = leftDistance >= rightDistance;
    int side = toLeft ? leftDistance : rightDistance;

    if (side >= SAFE_DISTANCE)
    {
        startTurnToOpen(toLeft);
    }
    else
    {
        cornerTurns = 0;
        startCornerTurn(toLeft);
    }
}

// SEARCH (obstacle avoidance) keeps going until another command stops it.
// It drives forward with the sensor looking straight ahead. At an obstacle
// it backs up, looks left, right and ahead with the servo, then turns toward
// the side with the most room until the sensor sees open space, and drives
// on (never straight back at the obstacle). If it is boxed in, it turns 90
// degrees at a time until it finds an opening. If it gets stuck on something
// the sensor can't see, it notices and backs out.
void runObstacleAvoidance()
{
    if (!searchMode)
        return;

    switch (searchStep)
    {
    case AVOID_DRIVE:                          // drive forward, looking straight ahead
    {
        lookCenter();

        int ahead = filteredDistance();

        static unsigned long lastReport = 0;   // debug: what it sees, twice a second
        if (millis() - lastReport >= 500)
        {
            lastReport = millis();
            Serial.print(F("SEARCH ahead: "));
            Serial.println(ahead);
        }

        if (ahead <= STOP_DISTANCE)
        {
            Serial.print(F("SEARCH blocked at "));
            Serial.println(ahead);
            searchLookAround(false);           // something in the way
            break;
        }

        forward(currentSpeed);

        // Stuck? Driving forward should change the distance ahead. If it
        // hasn't changed for a while, a wheel or corner is caught on something.
        // (Seeing nothing at all ahead for a long time also counts: no room
        //  is that big, so it is probably pushing against something.)
        if (millis() - stuckCheckAt >= SEARCH_STUCK_MS)
        {
            noEchoChecks = (ahead >= NO_ECHO) ? noEchoChecks + 1 : 0;
            bool same = ahead < NO_ECHO && abs(ahead - stuckCheckDistance) < SEARCH_STUCK_CM;
            sameChecks = same ? sameChecks + 1 : 0;

            bool stuck = (same && ahead < SEARCH_STUCK_NEAR_CM)
                      || sameChecks >= SEARCH_STUCK_FAR_CHECKS
                      || noEchoChecks >= SEARCH_NO_ECHO_CHECKS;
            stuckCheckDistance = ahead;
            stuckCheckAt = millis();

            if (stuck)
            {
                Serial.println(F("SEARCH stuck - backing out"));
                searchLookAround(true);
            }
        }
        break;
    }

    case AVOID_LOOK_LEFT:                      // wait for the servo, then measure
        if (millis() - avoidTimer >= SERVO_SETTLE_MS)
        {
            leftDistance = filteredDistance();
            lookRight();
            avoidTimer = millis();
            searchStep = AVOID_LOOK_RIGHT;
        }
        break;

    case AVOID_LOOK_RIGHT:
        if (millis() - avoidTimer >= SERVO_SWING_MS)
        {
            rightDistance = filteredDistance();
            lookCenter();
            avoidTimer = millis();
            searchStep = AVOID_LOOK_CENTER;
        }
        break;

    case AVOID_LOOK_CENTER:
        if (millis() - avoidTimer >= SERVO_SETTLE_MS)
        {
            centerDistance = filteredDistance();

            Serial.print(F("SEARCH looked  L="));   // debug: what it saw around it
            Serial.print(leftDistance);
            Serial.print(F("  C="));
            Serial.print(centerDistance);
            Serial.print(F("  R="));
            Serial.println(rightDistance);

            chooseClearestWay();
        }
        break;

    case AVOID_CORNER_CHECK:                   // ... stand still a moment, then look straight ahead
        if (millis() - avoidTimer >= SERVO_SETTLE_MS)
        {
            int ahead = filteredDistance();

            if (ahead >= SAFE_DISTANCE)
            {
                Serial.print(F("SEARCH opening found: "));
                Serial.println(ahead);
                searchDriveOn();               // an opening: go that way
            }
            else if (cornerTurns < MAX_CORNER_TURNS)
            {
                startCornerTurn(searchTurnLeft);   // still blocked: another 90 degrees
            }
            else
            {
                Serial.println(F("SEARCH turned all the way round - looking again"));
                searchLookAround(false);
            }
        }
        break;

    case AVOID_TURN_TO_OPEN:                   // turn a small step, stop, look straight ahead
    {
        turnStep();
        turnSteps++;

        int ahead = filteredDistance();        // measured standing still: accurate

        if (ahead >= searchOpenTarget)
        {
            Serial.print(F("SEARCH open way found: "));
            Serial.println(ahead);

            for (byte i = 0; i < SEARCH_TURN_EXTRA_STEPS; i++)
                turnStep();                    // a little more, so the whole car gets past the edge

            searchDriveOn();                   // the way is open: drive on
        }
        else if (turnSteps >= SEARCH_TURN_MAX_STEPS)
        {
            Serial.println(F("SEARCH no open way yet - looking again"));
            lookLeft();
            avoidTimer = millis();
            searchStep = AVOID_LOOK_LEFT;
        }
        break;
    }
    }
}


//======================================================
// 17. SETUP (runs once) AND LOOP (runs forever)
//======================================================

void setup()
{
    Serial.begin(SERIAL_BAUD);
    BT.begin(BT_BAUD);

    randomSeed(analogRead(A3) ^ micros());   // A3 is unused, its noise makes CHAOS different each time

    // The numbers 1-7 are start-up progress markers in the Serial Monitor
    Serial.println(F("1"));

    pinMode(ENA, OUTPUT);
    pinMode(ENB, OUTPUT);

    Serial.println(F("2"));

    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);

    Serial.println(F("3"));

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    Serial.println(F("4"));

    scanner.attach(SERVO_PIN);

    Serial.println(F("5"));

    lookCenter();

    Serial.println(F("6"));

    stop();

    Serial.println(F("7"));

    // ENABLE_LED_FEEDBACK: the "L" LED flickers when IR signals arrive
    IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);

    setupBuzzer();             // must come after IrReceiver.begin() (it borrows its timer)

    setupDisplay();

    startGreeting(false);      // power-on hello: screen + fanfare + servo wave

    Serial.println(F("READY"));
}

void loop()
{
    handleBluetooth();         // phone commands

    handleIR();                // remote commands

    updateDriving();           // auto forward/backward, donut, drift

    updateDance();

    updateDemo();

    update180();

    updateBrake();

    checkObstacle();           // distance check (also sets loop timing)

    updateChaos();

    runObstacleAvoidance();    // SEARCH mode

    updateSound();             // next note of the tune

    updateParkingBeeps();      // beep-beep-beep when getting close

    updateGreeting();          // hello screen timer + servo wave

    updateDizzy();             // dizzy head wobble after a donut

    updateDisplay();           // OLED: one small part per pass

    /*
    Old idea, switched off: stop the car if no IR button arrived for 150 ms,
    so the IR arrows would only drive while held down.

    if (millis() - lastIRTime > 150 &&
        robotState != AUTO_FORWARD &&
        robotState != AUTO_BACKWARD &&
        robotState != DONUT &&
        robotState != DANCE &&
        robotState != DEMO &&
        robotState != CHAOS &&
        robotState != DRIFT_LEFT &&
        robotState != DRIFT_RIGHT &&
        robotState != SEARCH &&
        robotState != TURN180 &&
        robotState != BRAKING)
    {
          stop();
    }
    */
}
