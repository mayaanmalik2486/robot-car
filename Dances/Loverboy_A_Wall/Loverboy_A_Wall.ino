/*
======================================================
       ROBOT CAR  -  v9    dance song: Loverboy
======================================================

  WHAT THIS ROBOT DOES
  - You drive it with the Robot Car phone app (App folder - or any "Bluetooth
    RC car" app), through the HC-05. The IR remote is an add-on now.
  - It has self-driving modes that use the ultrasonic distance sensor
    (HC-SR04) so it stops before it bumps into things, some fun moves
    (dance, chaos, donut, curve, turbo) and a SEARCH mode that turns the
    sensor with the servo to look left, ahead and right, then turns toward
    the side with the most room until the way ahead is open (boxed in: it
    turns 90 degrees at a time until it finds an opening).
  - DANCE follows a whole song with real choreography, in time with the
    beat: the song's own dance steps (signature moves), a routine for each
    part of the song that comes back every time that part does, poses at
    the ends of phrases and spins that go exactly once round. The buzzer
    plays a tune that follows the song's chords. The song and its
    choreography are in section 3c (THE SONG).
  - SEARCH and CHAOS keep going by themselves until you send another command
    (holding or releasing the phone button doesn't stop them).
  - A small OLED screen shows what the robot is doing and how far away
    things are. While the servo moves (hello wave, SEARCH) it shows two
    cute eyes that look where the sensor looks and blink.
  - A buzzer plays a jingle for each mode, a tune for the dance,
    parking-sensor beeps that speed up as you get close, and warning beeps.
  - When you switch it on, and when your phone sends its first command,
    it says hello: "Hello!" on the screen + a tune + a little servo wave.
  - PLAY MODES: follow me, goalkeeper (with a kick), guard (an alarm), music
    (your hand plays notes), gestures and a little robot pet - see section 16b.
  - RADAR: stands still, sweeps the sensor and draws a radar map on the screen
    (and sends it to the app).
  - It tells the app what it is doing, and warns when the battery is nearly empty.

  BLUETOOTH LETTERS (sent by the phone app - the Robot Car app has a button for each)
    F forward        B backward       L spin left      R spin right
    G curve left     H curve right    S stop
    (CURVE = drive forward while turning, like a car going round a bend)
    Q slow speed     W full speed
    2 auto forward (stops at obstacles)       J auto backward
    3 donut          U 180-degree turn         u smooth brake
    4 dance          Y horn           1 chaos          I search (looks around)
    X (the "!" button) mute / unmute every sound
    V football mode on / off: the distance sensor is switched OFF (like X for the buzzer),
      so it never stops for obstacles (push a ball, bump into things). SEARCH and CHAOS
      still use the sensor. Press V again to switch the sensor back on.
    (4 again while dancing: tap it ON a beat to line the dance up with the music)
    T turbo (a full-power burst)
    PLAY MODES (they use the distance sensor, even in football mode):
    M follow me      K goalkeeper (k = kick!)      A guard (alarm)
    N music (your hand plays notes)      E gestures (tap / hold your hand)      P pet
    O radar: the sensor sweeps left and right and the screen draws a radar map
      (the app draws it too)
    ? the app asks what the car is doing (the car answers "~MODE|flags",
      then "#S:" + the song name)
    The car also warns when the battery is nearly empty (screen + sad tune).

  IR REMOTE: taken out to make room for the play modes. To add it back, see
    the IR_Remote_Addon folder (a guide + the code to paste in).

  WIRING  (Arduino UNO + Sensor Shield V5) - see the PINS section below
    L298N   ENA D5   ENB D6   IN1 D2   IN2 D4   IN3 D7   IN4 D10
    HC-05   TXD -> D3 (Arduino listens)    RXD -> A0 (Arduino talks)
    Servo D9      HC-SR04 TRIG D11 / ECHO D12      (D8 is free - the IR add-on uses it)
    Buzzer (3-pin board) A1           OLED (I2C) SDA A4 / SCL A5
    Full wiring and power guide: POWER_AND_WIRING.md (in this folder).

  LIBRARIES
    U8g2 (by oliver),
    plus Servo, SoftwareSerial and Wire (come with the Arduino IDE).

  HOW THE PROGRAM IS ORGANISED (top to bottom)
    1 libraries   2 pins   3 settings   3b dance moves
    3c THE SONG (tempo, the song's dance steps, the choreography from start to end)
    5 robot "memory" (4 was the IR remote's codes - see the IR add-on)
    6 motors      7 buzzer   8 OLED screen   9 distance sensor   10 servo
   11 greeting    12 timed moves   13 stop / start helpers
   15 Bluetooth (the letters from the app)   16 self-driving modes
   16b play modes (follow me, goalkeeper, guard, music, gestures, pet, turbo, battery)
   17 setup() and loop()
======================================================
*/


//======================================================
// 1. LIBRARIES - ready-made code we borrow
//======================================================

#include <Arduino.h>
#include <Servo.h>            // moves the SG90 servo
#include <SoftwareSerial.h>   // extra serial port for the HC-05
#include <Wire.h>             // I2C, used by the OLED
#include <U8x8lib.h>          // OLED text drawing (part of the U8g2 library)

// Debug messages on the computer's Serial Monitor (USB cable). They are OFF to save
// about 3 KB of memory for the car's modes - the car works exactly the same either way.
// To see them, change the 0 to 1 (with every mode on, the program may then be too big).
#ifndef DEBUG_MESSAGES
#define DEBUG_MESSAGES 0
#endif

#if DEBUG_MESSAGES
#define DEBUG Serial
#else
struct NoDebug                // stands in for Serial and does nothing: the messages are left out
{
    void begin(long) {}
    template <class T> void print(T) {}
    template <class T> void println(T) {}
    void println() {}
};
NoDebug DEBUG;
#endif


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
const int CURVE_INNER_PERCENT = -30;  // CURVE (forward while turning): the inner wheels run at this % of the speed.
                                      // Lower = sharper turn, higher = wider. Below 0 the inner wheels turn
                                      // BACKWARD (-30 = backward at 30 %) for a really sharp turn. (60 = gentle curve)

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

const unsigned long BRAKE_STEP_MS = 40;        // smooth brake: lower the speed every 40 ms ...
const int BRAKE_STEP_PWM = 10;                 // ... by this much

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

// PLAY MODES (letters from the app). They all use the distance sensor, even in football mode.
// Switch one off (false) to free memory - for example to make room for the IR add-on.
const bool HAS_FOLLOW = true;          // M  follow me
const bool HAS_KEEPER = true;          // K  goalkeeper  (k = kick: punch the ball)
const bool HAS_GUARD = true;           // A  guard
const bool HAS_MUSIC = true;           // N  music: your hand plays notes
const bool HAS_GESTURE = true;         // E  gesture control
const bool HAS_PET = true;             // P  pet
const bool HAS_RADAR = true;           // O  radar

const int SEE_CM = 150;                // play modes: something closer than this is "seen"

// FOLLOW ME: it keeps about this far from your hand or feet
const int FOLLOW_NEAR_CM = 18;         // closer than this: it backs away
const int FOLLOW_FAR_CM = 30;          // farther than this: it comes after you
const int FOLLOW_LOST_CM = 90;         // nothing closer than this: it lost you, and looks around
const int FOLLOW_SPEED_PERCENT = 65;

// GOALKEEPER: put it in front of the goal, facing the player
const int KEEPER_SAVE_CM = 30;         // the ball comes this close: punch it away
const unsigned long KEEPER_PUNCH_MS = 220;   // the punch dashes forward this long, then comes back

// GUARD: counts down, learns what the room looks like, then watches it
const byte GUARD_ARM_SECONDS = 5;      // time for you to walk away before it starts
const int GUARD_CLOSER_CM = 25;        // something this much closer than the room was = an intruder
const unsigned long GUARD_CALM_MS = 5000;    // the alarm stops this long after the intruder left

// MUSIC: move your hand in front of the sensor (closer = higher note)
const int MUSIC_NEAR_CM = 5;
const int MUSIC_FAR_CM = 60;           // farther than this: silence

// GESTURES: your hand in front of the sensor
const int GESTURE_ZONE_CM = 35;        // closer than this: it sees your hand
const int GESTURE_NEAR_CM = 12;        // held closer than this: push (it backs away)
const unsigned long GESTURE_TAP_MS = 450;    // in and out quicker than this = a tap
const unsigned long GESTURE_HOLD_MS = 700;   // held this long close = push; twice as long further away = "come here"

// PET
const unsigned long PET_BORED_MS = 25000;    // nobody played with it for this long: bored
const unsigned long PET_SLEEP_MS = 45000;    // ... and this long: it falls asleep
const int PET_HAND_CM = 25;                  // a hand closer than this (while it sits) = petting
const int PET_SPEED_PERCENT = 50;            // how fast it wanders

// RADAR: the car stands still; the sensor sweeps from right to left and back,
// 10 degrees a step, and the screen draws a dot for everything it sees
const int RADAR_RANGE_CM = 150;              // the edge of the radar picture
const unsigned long RADAR_STEP_MS = 160;     // time per step (servo move + a fresh measurement)
const byte RADAR_STEPS = 11;                 // 11 directions: 50 degrees right ... ahead ... 50 degrees left

// TURBO: a full-power burst forward (it still stops for obstacles, unless football mode is on)
const unsigned long TURBO_MS = 1500;

// LOW BATTERY: the Arduino measures its own power supply (no extra wires needed). The
// motor driver keeps that at 5 V until the battery is nearly empty, so the warning
// comes late - charge the battery as soon as you see it.
const unsigned long BATTERY_CHECK_MS = 2000;
const int BATTERY_DROP_MV = 300;             // warn if the power falls this much below the start ...
const int BATTERY_LOW_MV = 4300;             // ... or below this (millivolts)

// Phone apps send the letter while you hold a button and 'S' when you let go.
// For this long after a mode letter, that 'S' is ignored so the mode keeps going.
const unsigned long BUTTON_RELEASE_IGNORE_MS = 1500;


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
const int DANCE_SOFT_PERCENT = 60;             // SOFT moves (smooth, slow-motion style) run at 60 % of the dance speed
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
// 3b. DANCE MOVES
//
//     The dance follows a beat grid: every beat is split into 2 steps,
//     so one bar of 4 beats = 8 steps ("1 & 2 & 3 & 4 &"). Each step is
//     one move, written like  FWD,  FWD+SOFT  or  SPIN_L+ROUND.
//     Each song has its OWN dance steps (SONG_MOVES, one bar each) and its
//     choreography (SONG_MAP): which step to dance in which bar, from the
//     start of the song to the end - both are in section 3c.
//======================================================

// Moves
const byte POSE = 0;       // stand still (the head bobs to the beat)
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
const byte DASH = 11;      // forward at full speed (a hit)
const byte LOOK_L = 12;    // stand still, sensor head looks left
const byte LOOK_R = 13;    // stand still, sensor head looks right
const byte HOLD = 14;      // keep doing the last move
const byte FREEZE = 15;    // sharp stop, head straight ahead: strike a pose

// Add these to a move
const byte SOFT = 0x80;    // FWD+SOFT: smooth and slow, like slow motion (DANCE_SOFT_PERCENT of the speed)
const byte ROUND = 0x40;   // SPIN_L+ROUND: spin exactly once round and stop facing the front again
                           // (timed like the 180 turn - TURN180_TIME. Give it a few HOLD steps after it.)

const byte STEPS_PER_BAR = 8;

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
//     To change the dance: edit a step in SONG_MOVES (8 moves = one bar),
//     or pick a different step for some bars in SONG_MAP.
//======================================================

// ----- SONG START (each song file has its own version of this part) -----
// Song: Loverboy - A-Wall
// Version this dance is timed to (3:45 long): original (2019 single)
// HOW TO START: play the song, and press dance (phone 4 / remote 5) on the
// first beat - about 0.4 s after the song starts. The dance lasts the whole song
// (112 bars, about 3:44). Tap dance again on a beat to line it up if it drifts.
// Tempo: 120 BPM (checked in several places).
// Song parts (estimated - where the dance changes): 0:00 INTRO, 0:14 VERSE, 0:46 CHORUS,
//  1:04 CHORUS, 1:18 VERSE, 1:50 CHORUS, 2:08 CHORUS, 2:24 SOLO, 2:38 CHORUS, 2:56
//  CHORUS, 3:10 CHORUS, 3:28 CHORUS, 3:40 OUTRO
const char SONG_NAME[] PROGMEM = "Loverboy";
const float SONG_BPM = 120;                        // beats per minute
const unsigned long SONG_FIRST_BEAT_MS = 400;        // where the first beat is in the song (so the clock on the screen matches your player)
const int DANCE_SPEED = 210;                       // motor speed for the dance moves (0-255)
const int DANCE_POWER_SPEED = 210;                 // motor speed in the POWER parts (the explosive bits)
const char SONG_SHOUT[] PROGMEM = "";              // a special moment: this text on the screen + a zap sound ("" = none)
const int SONG_SHOUT_STEPS[3] = { -1, -1, -1 };    // ... at these steps of the dance (2 steps per beat, 0 = the first step, -1 = not used)
const bool DANCE_HEAD_BOB = true;                  // the sensor head also bobs left/right on the straight moves
const byte DANCE_SOUND = DANCE_SOUND_TUNE;         // buzzer: DANCE_SOUND_TUNE, DANCE_SOUND_DRUMS or DANCE_SOUND_OFF

// The song's key and chords (for the buzzer tune) - C major, checked (C major: C Cmaj7 F Dm)
const byte SONG_KEY = 60;                          // key note: 60 C, 61 C#, 62 D, 63 D#, 64 E, 65 F, 66 F#, 67 G, 68 G#, 69 A, 70 A#, 71 B
const bool SONG_MINOR = false;                     // true = minor key
// Chord progressions, one chord per bar, as steps of the scale (1 = the key's own chord)
const byte SONG_CHORDS[][4] PROGMEM = {
    { 1, 1, 4, 2 },                                // 0 verse
    { 1, 4, 2, 1 },                                // 1 chorus
    { 4, 4, 1, 1 },                                // 2 bridge
};


// THE CHOREOGRAPHY
// Dreamy funk: easy side-to-side sways and nods, a hustle-and-spin routine in every
//  chorus and a looser groove after it.
// Each part dances its routine (one step per bar) and starts it again every time that part comes back:
// INTRO: breathe, look_lr, sway, look_rl, nod, nod, button
// VERSE: two_step, nod, two_step, swagger, glide_l, nod, glide_r, button
// CHORUS: hustle, bounce, hustle, spin_pose_l, hustle, bounce, zigzag, spin_pose_r,
//  fill_hit
// SOLO: float, hustle, float, spin_lr
// OUTRO: sway, freeze

// This song's dance steps: one bar each, 8 half-beat steps ("1 & 2 & 3 & 4 &")
const byte M_BREATHE     =  0;   // gentle head bob on beats 1 and 3
const byte M_LOOK_LR     =  1;   // look left, look right
const byte M_SWAY        =  2;   // slow sway left and right
const byte M_LOOK_RL     =  3;   // look right, look left
const byte M_NOD         =  4;   // head bob on every beat
const byte M_BUTTON      =  5;   // bob, bob, POSE (end of a phrase)
const byte M_TWO_STEP    =  6;   // two-step side to side
const byte M_SWAGGER     =  7;   // step forward, look left, step back, look right
const byte M_GLIDE_L     =  8;   // smooth glide forward-left and back
const byte M_GLIDE_R     =  9;   // smooth glide forward-right and back
const byte M_HUSTLE      = 10;   // disco hustle
const byte M_BOUNCE      = 11;   // bounce
const byte M_SPIN_POSE_L = 12;   // spin round to the left into a pose
const byte M_ZIGZAG      = 13;   // zig-zag shuffle
const byte M_SPIN_POSE_R = 14;   // spin round to the right into a pose
const byte M_FILL_HIT    = 15;   // bob, bob, hit, pose
const byte M_MELT        = 16;   // slow shoulder melt
const byte M_FLOAT       = 17;   // float left, then right
const byte M_SPIN_LR     = 18;   // spin left, spin back
const byte M_RISER       = 19;   // shimmy, hit, pose (into the big part)
const byte M_FINALE      = 20;   // final spin round, and round the other way
const byte SONG_MOVES[][STEPS_PER_BAR] PROGMEM = {
    { POSE,         HOLD,         HOLD,         HOLD,         POSE,         HOLD,         HOLD,         HOLD          },   // M_BREATHE
    { LOOK_L,       HOLD,         HOLD,         HOLD,         LOOK_R,       HOLD,         HOLD,         HOLD          },   // M_LOOK_LR
    { TWIST_L+SOFT, HOLD,         HOLD,         HOLD,         TWIST_R+SOFT, HOLD,         HOLD,         HOLD          },   // M_SWAY
    { LOOK_R,       HOLD,         HOLD,         HOLD,         LOOK_L,       HOLD,         HOLD,         HOLD          },   // M_LOOK_RL
    { POSE,         HOLD,         POSE,         HOLD,         POSE,         HOLD,         POSE,         HOLD          },   // M_NOD
    { POSE,         HOLD,         POSE,         HOLD,         FREEZE,       HOLD,         HOLD,         HOLD          },   // M_BUTTON
    { TWIST_L,      POSE,         TWIST_R,      POSE,         TWIST_L,      POSE,         TWIST_R,      POSE          },   // M_TWO_STEP
    { FWD,          HOLD,         POSE,         LOOK_L,       BACK,         HOLD,         POSE,         LOOK_R        },   // M_SWAGGER
    { SWAY_L+SOFT,  HOLD,         HOLD,         HOLD,         BACK_L+SOFT,  HOLD,         HOLD,         HOLD          },   // M_GLIDE_L
    { SWAY_R+SOFT,  HOLD,         HOLD,         HOLD,         BACK_R+SOFT,  HOLD,         HOLD,         HOLD          },   // M_GLIDE_R
    { SWAY_L,       BACK_L,       POSE,         HOLD,         SWAY_R,       BACK_R,       POSE,         HOLD          },   // M_HUSTLE
    { FWD,          BACK,         POSE,         HOLD,         BACK,         FWD,          POSE,         HOLD          },   // M_BOUNCE
    { SPIN_L+ROUND, HOLD,         HOLD,         HOLD,         FREEZE,       HOLD,         HOLD,         HOLD          },   // M_SPIN_POSE_L
    { SWAY_L,       BACK_L,       SWAY_R,       BACK_R,       SWAY_L,       BACK_L,       SWAY_R,       BACK_R        },   // M_ZIGZAG
    { SPIN_R+ROUND, HOLD,         HOLD,         HOLD,         FREEZE,       HOLD,         HOLD,         HOLD          },   // M_SPIN_POSE_R
    { POSE,         HOLD,         POSE,         HOLD,         DASH,         BACK,         FREEZE,       HOLD          },   // M_FILL_HIT
    { TWIST_L+SOFT, HOLD,         TWIST_R+SOFT, HOLD,         TWIST_L+SOFT, HOLD,         TWIST_R+SOFT, HOLD          },   // M_MELT
    { SWAY_L+SOFT,  HOLD,         BACK_L+SOFT,  HOLD,         SWAY_R+SOFT,  HOLD,         BACK_R+SOFT,  HOLD          },   // M_FLOAT
    { SPIN_L,       HOLD,         HOLD,         HOLD,         SPIN_R,       HOLD,         HOLD,         HOLD          },   // M_SPIN_LR
    { TWIST_L,      TWIST_R,      TWIST_L,      TWIST_R,      DASH,         BACK,         FREEZE,       HOLD          },   // M_RISER
    { SPIN_L+ROUND, HOLD,         HOLD,         HOLD,         SPIN_R+ROUND, HOLD,         HOLD,         HOLD          },   // M_FINALE
};

// The song from start to end: { dance step, how many bars, part name, chords, tune pattern }
const byte SONG_MAP[][5] PROGMEM = {
    { M_BREATHE,      1, INTRO,        0, T_CALM   },
    { M_LOOK_LR,      1, INTRO,        0, T_FLOAT  },
    { M_SWAY,         1, INTRO,        0, T_CALM   },
    { M_LOOK_RL,      1, INTRO,        0, T_FLOAT  },
    { M_NOD,          1, INTRO,        0, T_CALM   },
    { M_NOD,          1, INTRO,        0, T_FLOAT  },
    { M_BUTTON,       1, INTRO,        0, T_CALM   },
    { M_TWO_STEP,     1, VERSE,        0, T_CALM   },
    { M_NOD,          1, VERSE,        0, T_FLOAT  },
    { M_TWO_STEP,     1, VERSE,        0, T_CALM   },
    { M_SWAGGER,      1, VERSE,        0, T_FLOAT  },
    { M_GLIDE_L,      1, VERSE,        0, T_CALM   },
    { M_NOD,          1, VERSE,        0, T_FLOAT  },
    { M_GLIDE_R,      1, VERSE,        0, T_CALM   },
    { M_BUTTON,       1, VERSE,        0, T_FLOAT  },
    { M_TWO_STEP,     1, VERSE,        0, T_CALM   },
    { M_NOD,          1, VERSE,        0, T_FLOAT  },
    { M_TWO_STEP,     1, VERSE,        0, T_CALM   },
    { M_SWAGGER,      1, VERSE,        0, T_FLOAT  },
    { M_GLIDE_L,      1, VERSE,        0, T_CALM   },
    { M_NOD,          1, VERSE,        0, T_FLOAT  },
    { M_GLIDE_R,      1, VERSE,        0, T_CALM   },
    { M_BUTTON,       1, VERSE,        0, T_FLOAT  },
    { M_HUSTLE,       1, CHORUS,       1, T_HOOK_A },
    { M_BOUNCE,       1, CHORUS,       1, T_HOOK_B },
    { M_HUSTLE,       1, CHORUS,       1, T_HOOK_A },
    { M_SPIN_POSE_L,  1, CHORUS,       1, T_HOOK_B },
    { M_HUSTLE,       1, CHORUS,       1, T_HOOK_A },
    { M_BOUNCE,       1, CHORUS,       1, T_HOOK_B },
    { M_ZIGZAG,       1, CHORUS,       1, T_HOOK_A },
    { M_SPIN_POSE_R,  1, CHORUS,       1, T_HOOK_B },
    { M_FILL_HIT,     1, CHORUS,       1, T_HOOK_A },
    { M_TWO_STEP,     1, CHORUS,       1, T_GROOVE },
    { M_MELT,         1, CHORUS,       1, T_GROOVE },
    { M_TWO_STEP,     1, CHORUS,       1, T_PULSE  },
    { M_NOD,          1, CHORUS,       1, T_PULSE  },
    { M_TWO_STEP,     1, CHORUS,       1, T_GROOVE },
    { M_MELT,         1, CHORUS,       1, T_GROOVE },
    { M_TWO_STEP,     1, CHORUS,       1, T_PULSE  },
    { M_TWO_STEP,     1, VERSE,        0, T_GROOVE },
    { M_NOD,          1, VERSE,        0, T_GROOVE },
    { M_TWO_STEP,     1, VERSE,        0, T_PULSE  },
    { M_SWAGGER,      1, VERSE,        0, T_PULSE  },
    { M_GLIDE_L,      1, VERSE,        0, T_GROOVE },
    { M_NOD,          1, VERSE,        0, T_GROOVE },
    { M_GLIDE_R,      1, VERSE,        0, T_PULSE  },
    { M_BUTTON,       1, VERSE,        0, T_PULSE  },
    { M_TWO_STEP,     1, VERSE,        0, T_GROOVE },
    { M_NOD,          1, VERSE,        0, T_GROOVE },
    { M_TWO_STEP,     1, VERSE,        0, T_PULSE  },
    { M_SWAGGER,      1, VERSE,        0, T_PULSE  },
    { M_GLIDE_L,      1, VERSE,        0, T_GROOVE },
    { M_NOD,          1, VERSE,        0, T_GROOVE },
    { M_GLIDE_R,      1, VERSE,        0, T_PULSE  },
    { M_BUTTON,       1, VERSE,        0, T_PULSE  },
    { M_HUSTLE,       1, CHORUS,       1, T_HOOK_A },
    { M_BOUNCE,       1, CHORUS,       1, T_HOOK_B },
    { M_HUSTLE,       1, CHORUS,       1, T_HOOK_A },
    { M_SPIN_POSE_L,  1, CHORUS,       1, T_HOOK_B },
    { M_HUSTLE,       1, CHORUS,       1, T_HOOK_A },
    { M_BOUNCE,       1, CHORUS,       1, T_HOOK_B },
    { M_ZIGZAG,       1, CHORUS,       1, T_HOOK_A },
    { M_SPIN_POSE_R,  1, CHORUS,       1, T_HOOK_B },
    { M_FILL_HIT,     1, CHORUS,       1, T_HOOK_A },
    { M_TWO_STEP,     1, CHORUS,       1, T_GROOVE },
    { M_MELT,         1, CHORUS,       1, T_GROOVE },
    { M_TWO_STEP,     1, CHORUS,       1, T_PULSE  },
    { M_NOD,          1, CHORUS,       1, T_PULSE  },
    { M_TWO_STEP,     1, CHORUS,       1, T_GROOVE },
    { M_MELT,         1, CHORUS,       1, T_GROOVE },
    { M_TWO_STEP,     1, CHORUS,       1, T_PULSE  },
    { M_NOD,          1, CHORUS,       1, T_PULSE  },
    { M_FLOAT,        1, SOLO,         2, T_GROOVE },
    { M_HUSTLE,       1, SOLO,         2, T_GROOVE },
    { M_FLOAT,        1, SOLO,         2, T_PULSE  },
    { M_SPIN_LR,      1, SOLO,         2, T_PULSE  },
    { M_FLOAT,        1, SOLO,         2, T_GROOVE },
    { M_HUSTLE,       1, SOLO,         2, T_GROOVE },
    { M_RISER,        1, SOLO,         2, T_PULSE  },
    { M_HUSTLE,       1, CHORUS,       1, T_HOOK_A },
    { M_BOUNCE,       1, CHORUS,       1, T_HOOK_B },
    { M_HUSTLE,       1, CHORUS,       1, T_ARP    },
    { M_SPIN_POSE_L,  1, CHORUS,       1, T_HOOK_B },
    { M_HUSTLE,       1, CHORUS,       1, T_HOOK_A },
    { M_BOUNCE,       1, CHORUS,       1, T_HOOK_B },
    { M_ZIGZAG,       1, CHORUS,       1, T_ARP    },
    { M_SPIN_POSE_R,  1, CHORUS,       1, T_HOOK_B },
    { M_FILL_HIT,     1, CHORUS,       1, T_HOOK_A },
    { M_TWO_STEP,     1, CHORUS,       1, T_HOOK_A },
    { M_MELT,         1, CHORUS,       1, T_HOOK_B },
    { M_TWO_STEP,     1, CHORUS,       1, T_HOOK_A },
    { M_NOD,          1, CHORUS,       1, T_HOOK_B },
    { M_TWO_STEP,     1, CHORUS,       1, T_HOOK_A },
    { M_MELT,         1, CHORUS,       1, T_HOOK_B },
    { M_RISER,        1, CHORUS,       1, T_HOOK_A },
    { M_HUSTLE,       1, CHORUS,       1, T_HOOK_A },
    { M_BOUNCE,       1, CHORUS,       1, T_HOOK_B },
    { M_HUSTLE,       1, CHORUS,       1, T_ARP    },
    { M_SPIN_POSE_L,  1, CHORUS,       1, T_HOOK_B },
    { M_HUSTLE,       1, CHORUS,       1, T_HOOK_A },
    { M_BOUNCE,       1, CHORUS,       1, T_HOOK_B },
    { M_ZIGZAG,       1, CHORUS,       1, T_ARP    },
    { M_SPIN_POSE_R,  1, CHORUS,       1, T_HOOK_B },
    { M_FILL_HIT,     1, CHORUS,       1, T_HOOK_A },
    { M_TWO_STEP,     1, CHORUS,       1, T_HOOK_A },
    { M_MELT,         1, CHORUS,       1, T_HOOK_B },
    { M_TWO_STEP,     1, CHORUS,       1, T_HOOK_A },
    { M_NOD,          1, CHORUS,       1, T_HOOK_B },
    { M_TWO_STEP,     1, CHORUS,       1, T_HOOK_A },
    { M_MELT,         1, CHORUS,       1, T_HOOK_B },
    { M_SWAY,         1, OUTRO,        0, T_CALM   },
    { M_FINALE,       1, OUTRO,        0, T_END    },
};
// ----- SONG END -----

const byte SONG_PARTS = sizeof(SONG_MAP) / sizeof(SONG_MAP[0]);
const unsigned long DANCE_STEP_US = 60000000.0 / (SONG_BPM * 2);   // one step, in microseconds


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
    CURVE_LEFT,
    CURVE_RIGHT,
    DANCE,
    CHAOS,
    SEARCH,
    TURN180,
    BRAKING,
    FOLLOW,          // the play modes (keep them last in this list)
    KEEPER,
    GUARD,
    MUSIC,
    GESTURE,
    PET,
    RADAR,
    TURBO
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
unsigned long danceTurnStopMs = 0; // a ROUND spin stops at this time (millis, 0 = no ROUND spin going)
bool danceAfterDash = false;       // the last driving move was a DASH (the next BACK snaps back at full speed)
unsigned long danceNextUs = 0;     // when the next step is due (micros)
unsigned long danceBeatUs = 0;     // when the last beat was (micros), to line up with your taps
unsigned long danceStartMs = 0;    // when the dance started (for the clock on the screen)

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

// Play modes: the step they are on, timers, and what the sensor saw
byte playStep = 0;
unsigned long playTimer = 0;         // when the current step (or timed move) started
unsigned long playMoveMs = 0;        // how long the current timed move lasts
unsigned long playStopAt = 0;        // a timed move stops at this moment (0 = none) - yield() stops it on time
unsigned long playOther = 0;         // a second timer (taps, head moves, sounds)
unsigned long playSince = 0;         // gestures: hand seen since / pet: last played with / guard: intruder last seen
byte playLook = 0;                   // which way the sensor looks (in LOOK_ANGLES)
byte playCount = 0;                  // taps / countdown / the note playing
bool playFlag = false;               // guard: sweeping toward the left / gestures: moving forward
int playSeen[5];                     // what the sensor saw at each angle
int playCm = 0;                      // guard: how far away the intruder was
byte radarCm[RADAR_STEPS];           // radar: what the sensor saw in each direction (cm, 255 = nothing)
bool radarDirty = false;             // radar: a new dot - draw the picture again

const byte FOLLOW_TRACK = 0;         // follow me: you're in front - keep the distance
const byte FOLLOW_LOOK = 1;          // lost you: looking around
const byte FOLLOW_TURN = 2;          // turning toward you
const byte KEEPER_SCAN = 0;          // goalkeeper: start looking for the ball
const byte KEEPER_LOOK = 1;          // looking around for it
const byte KEEPER_TURN = 2;          // turning to face it
const byte KEEPER_WATCH = 3;         // watching it
const byte KEEPER_PUNCH = 4;         // punching it away ...
const byte KEEPER_BACK = 5;          // ... and back into the goal
const byte GUARD_ARM = 0;            // guard: counting down
const byte GUARD_LEARN = 1;          // learning what the room looks like
const byte GUARD_WATCH = 2;          // watching
const byte GUARD_TURN = 3;           // turning to face the intruder
const byte GUARD_ALARM = 4;          // siren!
const byte GESTURE_WATCH = 0;        // gestures: waiting for your hand
const byte GESTURE_MOVE = 1;         // doing a move
const byte GESTURE_CLEAR = 2;        // waiting for your hand to go away
const byte GESTURE_WIGGLE = 3;       // happy wiggle
const byte PET_WANDER = 0;           // pet: a little walk
const byte PET_TURN = 1;             // turning away from something
const byte PET_SIT = 2;              // sitting, looking around
const byte PET_BORED = 3;
const byte PET_SLEEP = 4;
const byte PET_HAPPY = 5;            // being petted

// Battery
int batteryStartMv = 0;              // the power supply when it started (millivolts)
bool batteryLow = false;

// Bluetooth button handling
unsigned long modeLetterAt = 0;    // when the last mode letter (I, 1, 2 ...) arrived

// Distance sensor
int lastDistance = NO_ECHO;        // newest measurement (shown on the OLED)
bool obstacleWarned = false;       // already beeped for this obstacle

// Bluetooth
bool btGreeted = false;            // true once the phone has sent its first letter
bool muted = false;                // phone X: all buzzer sounds off (press again: on)
unsigned long muteToggledAt = 0;   // (so one press doesn't toggle twice)
bool footballMode = false;         // phone V: the distance sensor is off (except in SEARCH and CHAOS)
unsigned long footballToggledAt = 0;

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
void curveLeft()
{
    setSpeed(percentOf(currentSpeed, abs(CURVE_INNER_PERCENT)), currentSpeed);
    setWheels(CURVE_INNER_PERCENT < 0 ? DIR_BACKWARD : DIR_FORWARD, DIR_FORWARD);
    goingForward = true;
    forwardSpeed = currentSpeed;
}

// Curve right while driving forward (right wheels slower)
void curveRight()
{
    setSpeed(currentSpeed, percentOf(currentSpeed, abs(CURVE_INNER_PERCENT)));
    setWheels(DIR_FORWARD, CURVE_INNER_PERCENT < 0 ? DIR_BACKWARD : DIR_FORWARD);
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
//    HOW THE SOUND IS MADE: Timer2 ticks about 20,000 times a second, and a
//    tiny interrupt flips the buzzer pin at the right moments (like tone(),
//    but it can play notes in the background while everything else runs).
//    The IR add-on's receiver uses the same Timer2 settings, so both work together.
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

// Call once in setup() (after IrReceiver.begin() if you use the IR add-on)
void setupBuzzer()
{
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, BUZZER_OFF_LEVEL);

    buzzerPort = portOutputRegister(digitalPinToPort(BUZZER_PIN));
    buzzerMask = digitalPinToBitMask(BUZZER_PIN);

    // Timer2: "count up to OCR2A, clock divided by 8" (the IR add-on's receiver
    // sets it up exactly like this - if it already did, keep its settings)
    bool countsToOcr2a = (TCCR2A & _BV(WGM21)) && !(TCCR2B & _BV(WGM22));
    bool divideBy8 = (TCCR2B & 0x07) == _BV(CS21);

    if (!countsToOcr2a || !divideBy8)
    {
        TCCR2A = _BV(WGM21);
        TCCR2B = _BV(CS21);
        OCR2A = 100;                     // 16 MHz / 8 / 101 = about 19,800 ticks a second
    }

    buzzerTickHz = F_CPU / 8 / (OCR2A + 1UL);
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
const uint16_t TUNE_TADA[] PROGMEM = {            // "ta-da-da-DAAA" (end of a dance)
    NOTE_C5, 100, NOTE_E5, 100, NOTE_G5, 100, NOTE_C6, 300,
    END_OF_TUNE
};

const uint16_t TUNE_HORN[] PROGMEM = {            // car horn: "beep beeeep"
    NOTE_A5, 140,  REST, 70,  NOTE_A5, 320,
    END_OF_TUNE
};

// Play modes
const uint16_t TUNE_SAVE[] PROGMEM = {            // goalkeeper: "SAVE!"
    NOTE_C6, 40, NOTE_G6, 40, NOTE_C7, 120,
    END_OF_TUNE
};
const uint16_t TUNE_SIREN[] PROGMEM = {           // guard alarm: wee-woo wee-woo
    1320, 160, 880, 160, 1320, 160, 880, 160,
    END_OF_TUNE
};
const uint16_t TUNE_HAPPY[] PROGMEM = {           // a happy chirp
    NOTE_C6, 60, NOTE_E6, 60, NOTE_G6, 60, NOTE_C7, 120,
    END_OF_TUNE
};
const uint16_t TUNE_PURR[] PROGMEM = {            // purring
    262, 45, 233, 45, 262, 45, 233, 45, 262, 45, 233, 45, REST, 60,
    END_OF_TUNE
};
const uint16_t TUNE_SIGH[] PROGMEM = {            // a bored sigh
    NOTE_G5, 150, NOTE_E5, 200, NOTE_C5, 350,
    END_OF_TUNE
};
const uint16_t TUNE_SNORE[] PROGMEM = {           // snoring
    196, 350, REST, 250, 175, 450,
    END_OF_TUNE
};
const uint16_t TUNE_TURBO[] PROGMEM = {           // turbo whoosh
    NOTE_C6, 30, NOTE_E6, 30, NOTE_G6, 30, NOTE_C7, 30, NOTE_E6, 30, NOTE_G6, 30, NOTE_C7, 150,
    END_OF_TUNE
};
const uint16_t TUNE_BATTERY_LOW[] PROGMEM = {     // battery nearly empty: sad falling notes
    NOTE_E5, 200, NOTE_C5, 200, NOTE_A4, 400,
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
void updatePlayStop();

// Arduino calls yield() again and again while delay() is waiting.
// We use that spare time to keep the music playing in time, the dance
// moves exactly on the beat, and to stop the 180 turn and the play modes' timed moves at exactly the right moment.
void yield()
{
    updateSound();
    updateDance();
    update180();
    updatePlayStop();
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

// Football mode (phone V) switches the distance sensor off - except in SEARCH, CHAOS and
// the play modes, which need it to see where they're going
bool sensorOff()
{
    switch (robotState)
    {
    case SEARCH:
    case CHAOS:
    case FOLLOW:
    case KEEPER:
    case GUARD:
    case MUSIC:
    case GESTURE:
    case PET:
    case RADAR:
        return false;              // these need to see

    default:
        return footballMode;
    }
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
    case CURVE_LEFT:      return blocked ? PSTR("BLOCKED") : PSTR("CURVE L");
    case CURVE_RIGHT:     return blocked ? PSTR("BLOCKED") : PSTR("CURVE R");
    case DANCE:           return PSTR("DANCE");
    case CHAOS:           return PSTR("CHAOS!");
    case TURN180:         return PSTR("TURN 180");
    case BRAKING:         return PSTR("BRAKING");
    case FOLLOW:          return PSTR("FOLLOW");
    case KEEPER:          return playStep >= KEEPER_PUNCH ? PSTR("SAVE!") : PSTR("KEEPER");
    case GUARD:           return playStep == GUARD_ARM ? PSTR("ARMING") : playStep >= GUARD_TURN ? PSTR("INTRUDER") : PSTR("GUARD");
    case MUSIC:           return PSTR("MUSIC");
    case GESTURE:         return PSTR("GESTURE");
    case PET:             return playStep == PET_SLEEP ? PSTR("Zzz...") : playStep == PET_BORED ? PSTR("BORED")
                               : playStep == PET_HAPPY ? PSTR("PURR <3") : PSTR("PET");
    case TURBO:           return PSTR("TURBO!");
    case RADAR:           return PSTR("RADAR");
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
    if (sensorOff())
    {
        flashText(buf, PSTR("SENS OFF"), 8);
        return;
    }

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

    if (HAS_RADAR && area == 0 && robotState == RADAR)    // "RADAR near  42cm"
    {
        byte nearest = 255;
        for (byte k = 0; k < RADAR_STEPS; k++)
            nearest = min(nearest, radarCm[k]);

        flashText(buf, PSTR("RADAR"), 16);
        if (nearest < RADAR_RANGE_CM)
        {
            strcpy_P(buf + 5, PSTR(" near "));
            itoa(nearest, buf + 11, 10);
            strcat_P(buf, PSTR("cm"));
            byte length = strlen(buf);
            while (length < 16)
                buf[length++] = ' ';
            buf[16] = '\0';
        }
        return;
    }

    if (area == 0)
    {
        flashText(buf, btGreeted ? PSTR("BT:OK") : PSTR("BT:--"), 5);
        if (batteryLow)
            strcat_P(buf, PSTR(" LOW BATT!"));
        else if (footballMode)
            strcat_P(buf, PSTR(" FOOTBALL!"));
        else if (muted)
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
        case CURVE_LEFT:
            eyeTarget[E_X] = toLeft;
            break;

        case MANUAL_RIGHT:
        case CURVE_RIGHT:
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

            if (danceMove == FREEZE)                    // striking a pose: a cool, steady stare
            {
                eyeTarget[E_Y] = 0;
                eyeTarget[E_X] = 0;
                eyeTarget[E_HAPPY] = 8;
                eyeTarget[E_H] = EYE_H - 4;
            }
            break;
        }

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

        case FOLLOW:                                    // happy, keeping an eye on you
            eyeTarget[E_HAPPY] = 12;
            eyeTarget[E_X] = servoLook(toLeft);
            break;

        case KEEPER:                                    // a focused goalkeeper, watching the ball
            eyeTarget[E_ANGRY] = 8;
            eyeTarget[E_H] = EYE_H - 8;
            eyeTarget[E_X] = servoLook(toLeft);
            if (playStep >= KEEPER_PUNCH)               // the save: big eyes
            {
                eyeTarget[E_ANGRY] = 0;
                eyeTarget[E_H] = EYE_H + 10;
            }
            break;

        case GUARD:                                     // on guard: narrowed, following the sensor
            eyeTarget[E_H] = EYE_H - 10;
            eyeTarget[E_X] = servoLook(toLeft);
            if (playStep >= GUARD_TURN)                 // INTRUDER: furious, flashing
            {
                eyeTarget[E_ANGRY] = 18;
                eyeTarget[E_H] = EYE_H + 6;
                eyeTarget[E_X] = ((now / 120) % 2) ? 4 : -4;
            }
            break;

        case MUSIC:                                     // happy, the eyes go up with the note
            if (playCount <= 10)
            {
                eyeTarget[E_HAPPY] = 18;
                eyeTarget[E_Y] = 5 - playCount;
            }
            break;

        case GESTURE:                                   // wide awake, watching your hand
            eyeTarget[E_W] = EYE_W - 4;
            eyeTarget[E_H] = EYE_H + 6;
            break;

        case PET:
            if (playStep == PET_SLEEP)                  // asleep
            {
                eyeTarget[E_OPEN] = 8;
                eyeTarget[E_Y] = 6;
            }
            else if (playStep == PET_BORED)             // bored: droopy
            {
                eyeTarget[E_OPEN] = 55;
                eyeTarget[E_TIRED] = 10;
                eyeTarget[E_Y] = 4;
            }
            else if (playStep == PET_HAPPY)             // being petted: ^ ^
            {
                eyeTarget[E_HAPPY] = 30;
            }
            else
            {
                eyeTarget[E_HAPPY] = 10;
                eyeTarget[E_X] = servoLook(toLeft);
            }
            break;

        case TURBO:                                     // turbo: fierce
            eyeTarget[E_ANGRY] = 14;
            eyeTarget[E_W] = EYE_W + 4;
            eyeTarget[E_H] = EYE_H - 8;
            eyeTarget[E_Y] = -3;
            break;

        case BRAKING:                                   // slowing down: squinting
            eyeTarget[E_TIRED] = 10;
            eyeTarget[E_OPEN] = 60;
            break;

        case RADAR:                                     // (the radar picture is on the screen instead)
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

// RADAR PICTURE (in the eye area, rows 1 - 6): the car at the bottom middle, dotted
// rings at half and full range, the sweep line, and a dot for everything the sensor saw.
const byte RADAR_X = 64;             // where the car is on the screen
const byte RADAR_Y = 55;
const byte RADAR_R = 46;             // the full range, in pixels
const byte RADAR_SIN[6] PROGMEM = {0, 11, 22, 32, 41, 49};   // sin and cos of 0, 10 ... 50 degrees (x 64)
const byte RADAR_COS[6] PROGMEM = {64, 63, 60, 55, 49, 41};

// The servo angle for radar direction k (0 = 50 degrees right, 5 = ahead, 10 = 50 degrees left)
int radarAngle(byte k)
{
    int degrees = ((int)k - 5) * 10;
    return SERVO_CENTER + (SERVO_LEFT > SERVO_CENTER ? degrees : -degrees);
}

// Puts a dot r pixels away in direction k into one screen row (page = its 128 columns).
// big = a 3x3 dot, else 1 pixel.
void radarDot(uint8_t* page, byte row, byte k, byte r, bool big)
{
    int8_t side = (int8_t)k - 5;                 // + = left
    byte a = abs(side);
    byte sx = (r * pgm_read_byte(RADAR_SIN + a)) >> 6;
    byte x = RADAR_X + (side > 0 ? -sx : sx);
    byte y = RADAR_Y - ((r * pgm_read_byte(RADAR_COS + a)) >> 6);

    for (int8_t dx = -big; dx <= big; dx++)
    {
        for (int8_t dy = -big; dy <= big; dy++)
        {
            byte py = y + dy - row * 8;          // (outside this row: a big number, skipped)
            if (py < 8)
                page[x + dx] |= 1 << py;
        }
    }
}

// Draws the radar picture, one screen row at a time
void drawRadar()
{
    uint8_t page[128];

    for (byte row = EYES_TOP_ROW; row <= EYES_BOTTOM_ROW; row++)
    {
        memset(page, 0, sizeof(page));

        for (byte k = 0; k < RADAR_STEPS; k++)
        {
            radarDot(page, row, k, RADAR_R / 2, false);         // the rings (dotted)
            radarDot(page, row, k, RADAR_R, false);
            if (radarCm[k] < RADAR_RANGE_CM)                    // something there: a big dot
                radarDot(page, row, k, (int)radarCm[k] * RADAR_R / RADAR_RANGE_CM, true);
        }
        for (byte r = 6; r < RADAR_R; r += 5)                   // the sweep line: where the sensor looks now
            radarDot(page, row, playLook, r, false);
        radarDot(page, row, 5, 1, true);                        // the car

        oled.drawTile(0, row, 16, page);
        for (byte tile = 0; tile < 16; tile++)                  // (so the eyes get drawn again afterwards)
            setTileState((row - EYES_TOP_ROW) * 16 + tile, TILE_PART);
    }
}

// Called from loop(): animates the eyes, or updates one text row
void updateDisplay()
{
    if (!oledFound || turning180 || playStopAt)
        return;

    // While dancing, don't start drawing just before the next step: a picture
    // can take up to 60 ms, and the step would come late (off the beat)
    if (danceMode && (long)(danceNextUs - micros()) < (long)DANCE_DRAW_GUARD_US)
        return;
    if (danceTurnStopMs && (long)(danceTurnStopMs - millis()) < (long)(DANCE_DRAW_GUARD_US / 1000))   // ... or just before a ROUND spin stops
        return;

    if (HAS_RADAR && robotState == RADAR)
    {
        if (radarDirty)                  // the radar picture instead of the eyes
        {
            drawRadar();
            radarDirty = false;
            eyesDrawn = false;           // (the eyes are drawn again afterwards)
            return;
        }
    }
    else
    {
        setEyeTargets();

        if (millis() - eyesDrawnAt >= EYES_FRAME_MS && (stepEyes() || !eyesDrawn))
        {
            drawEyes();
            eyesDrawn = true;
            eyesDrawnAt = millis();
            return;
        }
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
        DEBUG.println(F("OLED not found - check the SDA/SCL wires"));
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

    DEBUG.println(F("OLED ready"));
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
        || robotState == CURVE_LEFT || robotState == CURVE_RIGHT
        || (robotState == CHAOS && chaosStep == CHAOS_STRAIGHT)
        || (robotState == SEARCH && searchStep == AVOID_DRIVE)
        || robotState == TURBO;
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
    if (sensorOff())                         // football mode: no pings at all, nothing is ever "in the way"
    {
        lastDistance = NO_ECHO;
        return NO_ECHO;
    }

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
// loop(). The timed moves (180 turn, dance) were tuned with it,
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

// Where the play modes look when they look around: right ... straight ... left
const int LOOK_ANGLES[5] = {SERVO_RIGHT, (SERVO_RIGHT + SERVO_CENTER) / 2, SERVO_CENTER,
                            (SERVO_CENTER + SERVO_LEFT) / 2, SERVO_LEFT};


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

    DEBUG.println(fromPhone ? F("HELLO PHONE") : F("HELLO"));
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

    chaosMode = false;
    chaosStep = CHAOS_STRAIGHT;

    searchMode = false;
    searchStep = AVOID_DRIVE;

    if (robotState == MUSIC)           // music mode: its note stops too
        stopSound();

    robotState = IDLE;
    stop();

    lookCenter();

    playStopAt = 0;                    // (no play-mode move waiting to stop)

    nextBlinkAt = millis();            // the eyes blink at every new command

    DEBUG.println(F("STOP"));
}

// Switches off every mode that drives by itself (auto forward/backward, donut,
// dance, chaos, search, play modes) without stopping the motors - the button that
// called this decides what the motors do next. Used by the smooth brake and the
// IR add-on, so a running dance or search can't fight with them.
void cancelAutoModes()
{
    dizzyIfDonut();
    playStopAt = 0;
    autoForward = false;
    autoBackward = false;
    donutMode = false;

    if (danceMode || searchMode || robotState >= FOLLOW)
        lookCenter();                  // the sensor head may be turned (play modes are last in the list)

    if (robotState == MUSIC)
        stopSound();

    danceMode = false;
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

void sendStatus();                  // (section 15 - this line just lets us use it here)

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
    danceTurnStopMs = 0;
    danceAfterDash = false;
    robotState = DANCE;
    danceNextUs = micros();            // the beat starts now (when you pressed dance) ...
    danceBeatUs = danceNextUs;
    danceStartMs = millis();
    sendStatus();                      // ... then tell the app (no messages while dancing)

    DEBUG.print(F("DANCE to: "));
    DEBUG.print((const __FlashStringHelper*)SONG_NAME);
    DEBUG.print(F("  BPM "));
    DEBUG.println((int)(SONG_BPM + 0.5));
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

    DEBUG.print(F("DANCE beat moved by ms: "));
    DEBUG.println(shift / 1000);
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

// Starts one of the play modes (what they do is in section 16b)
void startPlayMode(RobotState mode)
{
    emergencyStop();
    robotState = mode;
    playStep = 0;
    playTimer = millis();
    playOther = millis();
    playSince = millis();
    playCount = 0;
    playFlag = false;
}


//======================================================
// 15. BLUETOOTH - letters from the phone app
//======================================================

// THE APP'S STATUS LINE: the car tells the app what it is doing - "~MODE|1010" + a new line.
// (MODE = what the screen shows; then 1 or 0 for: muted, football mode, slow, low battery.)
// Sending pauses the Arduino for a moment, so it only sends when something changed,
// at most 4 times a second, and never while dancing or turning.
void sendStatus()
{
    BT.print('~');
    BT.print((const __FlashStringHelper*)actionName());
    BT.print('|');
    BT.print(muted ? '1' : '0');
    BT.print(footballMode ? '1' : '0');
    BT.print(slowMode ? '1' : '0');
    BT.print(batteryLow ? '1' : '0');
    BT.print('\n');
}

void updateStatus()
{
    static const char* sentName = NULL;
    static byte sentFlags = 255;
    static unsigned long sentAt = 0;

    if (!btGreeted || danceMode || turning180 || millis() - sentAt < 250)
        return;

    const char* name = actionName();
    byte flags = muted | footballMode << 1 | slowMode << 2 | batteryLow << 3;

    if (name == sentName && flags == sentFlags)
        return;

    sentName = name;
    sentFlags = flags;
    sentAt = millis();
    sendStatus();
}

void keeperPunch();                 // (section 16b - this line just lets us use it here)

// The same driving letter again (some apps repeat it while you hold the button): just keep going
bool sameDriving(char c)
{
    switch (c)
    {
        case 'F': return robotState == MANUAL_FORWARD;
        case 'B': return robotState == MANUAL_BACKWARD;
        case 'L': return robotState == MANUAL_LEFT;
        case 'R': return robotState == MANUAL_RIGHT;
        case 'G': return robotState == CURVE_LEFT;
        case 'H': return robotState == CURVE_RIGHT;
    }
    return false;
}

// Letters that start a self-driving mode
bool isModeLetter(char c)
{
    return c == 'I' || c == '1' || c == '2' || c == 'J' || c == '3' || c == '4' || c == 'U'
        || c == 'M' || c == 'K' || c == 'A' || c == 'N' || c == 'E' || c == 'P' || c == 'T' || c == 'O';
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
        case 'U': return turning180;
        case 'M': return robotState == FOLLOW;
        case 'K': return robotState == KEEPER;
        case 'A': return robotState == GUARD;
        case 'N': return robotState == MUSIC;
        case 'E': return robotState == GESTURE;
        case 'P': return robotState == PET;
        case 'T': return robotState == TURBO;
        case 'O': return robotState == RADAR;
    }
    return false;
}

void handleCommand(char c);         // (just below)

// A letter from the phone app
void handleBluetooth()
{
    if (BT.available())
        handleCommand(BT.read());
}

// Does what one letter asks for. (The IR add-on turns remote buttons into these letters too.)
void handleCommand(char c)
{
    DEBUG.print(F("Received: "));
    DEBUG.println(c);

    if (sameDriving(c))
        return;

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
        DEBUG.println(F("(button let go - mode keeps going)"));
        return;
    }

    switch (c)
    {
        case 'F':                          // forward
            DEBUG.println(F("FORWARD"));
            emergencyStop();
            robotState = MANUAL_FORWARD;
            forward(currentSpeed);
            break;

        case 'B':                          // backward
            DEBUG.println(F("BACKWARD"));
            emergencyStop();
            robotState = MANUAL_BACKWARD;
            backward(currentSpeed);
            break;

        case 'L':                          // spin left
            DEBUG.println(F("LEFT"));
            emergencyStop();
            left();
            robotState = MANUAL_LEFT;
            break;

        case 'R':                          // spin right
            DEBUG.println(F("RIGHT"));
            emergencyStop();
            right();
            robotState = MANUAL_RIGHT;
            break;

        case 'G':                          // curve left (forward while turning left)
            DEBUG.println(F("CURVE LEFT"));
            emergencyStop();
            robotState = CURVE_LEFT;
            break;

        case 'H':                          // curve right (forward while turning right)
            DEBUG.println(F("CURVE RIGHT"));
            emergencyStop();
            robotState = CURVE_RIGHT;
            break;

        case 'S':                          // stop
            DEBUG.println(F("STOP"));
            emergencyStop();
            break;

        case 'Q':                          // slow speed
            setSlowMode(true);
            DEBUG.println(F("Slow Mode ON"));
            break;

        case 'W':                          // full speed
            setSlowMode(false);
            DEBUG.println(F("Full Speed"));
            break;

        case 'X':                          // the "!" button: mute / unmute all sounds
        case 'x':                          // (apps send X and x in turn - each press flips it)
            if (millis() - muteToggledAt >= MUTE_PRESS_GAP_MS)
            {
                muted = !muted;
                muteToggledAt = millis();
                if (muted)
                    stopSound();
                DEBUG.println(muted ? F("MUTED") : F("SOUND ON"));
            }
            break;

        case 'V':                          // football mode on / off (the sensor is ignored)
        case 'v':                          // (apps send V and v in turn - each press flips it)
            if (millis() - footballToggledAt >= MUTE_PRESS_GAP_MS)
            {
                footballMode = !footballMode;
                footballToggledAt = millis();
                playBeep(footballMode ? NOTE_A5 : NOTE_BEEP, 120);
                DEBUG.println(footballMode ? F("FOOTBALL MODE: sensor off") : F("SENSOR ON"));
            }
            break;

        case '2':                          // auto forward
            DEBUG.println(F("AUTO FORWARD"));
            emergencyStop();
            setAutoForward();
            break;

        case 'J':                          // auto backward
            DEBUG.println(F("AUTO BACKWARD"));
            emergencyStop();
            setAutoBackward();
            break;

        case '3':                          // donut
            DEBUG.println(F("DONUT"));
            emergencyStop();
            setDonut();
            break;

        case 'U':                          // 180-degree turn
            DEBUG.println(F("180 TURN"));
            emergencyStop();
            start180();
            break;

        case 'u':                          // smooth brake
            DEBUG.println(F("BRAKE"));
            beginBrake();
            break;

        case '4':                          // dance
            DEBUG.println(F("DANCE"));
            startDance();
            break;

        case 'Y':                          // horn
            playSound(TUNE_HORN);
            break;

        case '1':                          // chaos
            DEBUG.println(F("CHAOS"));
            startChaos();
            break;

        case 'T':                          // turbo: a full-power burst
            startPlayMode(TURBO);
            playSound(TUNE_TURBO);
            break;

        case 'M':                          // follow me
            if (HAS_FOLLOW)
            {
                startPlayMode(FOLLOW);
                playSound(TUNE_AUTO_FORWARD);
            }
            break;

        case 'K':                          // goalkeeper
            if (HAS_KEEPER)
            {
                startPlayMode(KEEPER);
                playSound(TUNE_SEARCH);
            }
            break;

        case 'k':                          // kick: punch the ball away now
            if (HAS_KEEPER)
            {
                if (robotState != KEEPER)
                    startPlayMode(KEEPER);
                keeperPunch();
            }
            break;

        case 'A':                          // guard
            if (HAS_GUARD)
            {
                startPlayMode(GUARD);
                playCount = GUARD_ARM_SECONDS;
            }
            break;

        case 'N':                          // music: your hand plays notes
            if (HAS_MUSIC)
            {
                startPlayMode(MUSIC);
                stopSound();
                playCount = 255;           // (no note yet)
            }
            break;

        case 'E':                          // gesture control
            if (HAS_GESTURE)
            {
                startPlayMode(GESTURE);
                playSince = 0;
                playSound(TUNE_HAPPY);
            }
            break;

        case 'P':                          // pet
            if (HAS_PET)
            {
                startPlayMode(PET);
                playMoveMs = 8000;         // a walk first
                playSound(TUNE_HAPPY);
            }
            break;

        case 'O':                          // radar
            if (HAS_RADAR)
            {
                startPlayMode(RADAR);
                memset(radarCm, 255, sizeof(radarCm));
                playLook = 5;              // starts looking ahead, then sweeps
                radarDirty = true;
                playSound(TUNE_HAPPY);
            }
            break;

        case '?':                          // the app asks what the car is doing
            sendStatus();
            BT.print(F("#S:"));            // ... and which song this car dances to
            BT.print((const __FlashStringHelper*)SONG_NAME);
            BT.print('\n');
            break;

        case 'I':                          // search
            DEBUG.println(F("SEARCH"));
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

    case CURVE_LEFT:
        if (filteredDistance() > STOP_DISTANCE)
            curveLeft();
        else
            brakeStop();
        break;

    case CURVE_RIGHT:
        if (filteredDistance() > STOP_DISTANCE)
            curveRight();
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

// Both sides of the car turn opposite ways: a spin or twist on the spot
void danceTurn(int speed, bool toLeft)
{
    setSpeed(speed, speed);
    setWheels(toLeft ? DIR_BACKWARD : DIR_FORWARD, toLeft ? DIR_FORWARD : DIR_BACKWARD);
    goingForward = false;
}

// How long one full turn takes at this speed (the 180 turn is timed at the normal turn speed)
unsigned long fullTurnMs(int speed)
{
    return TURN180_TIME * 2 * percentOf(FULL_SPEED, TURN_SPEED_PERCENT) / speed;
}

// Does one dance move (and turns the sensor head with it).
// soft = the smooth, slow-motion version of the move; round = a spin of exactly one turn.
void doDanceMove(byte move, bool soft, bool round)
{
    if (move == HOLD)                          // keep doing the last move
        return;

    danceTurnStopMs = 0;                       // a new move ends any ROUND spin

    // never dance forward into something
    bool goesForward = (move == FWD || move == DASH || move == SWAY_L || move == SWAY_R);
    if (goesForward && lastDistance <= STOP_DISTANCE)
        move = POSE;

    danceMove = move;

    // a step back right after a DASH snaps back just as hard, so the car ends where it started
    bool backFull = (move == BACK && danceAfterDash && !soft);
    if (move != POSE && move != FREEZE && move != LOOK_L && move != LOOK_R)
        danceAfterDash = (move == DASH);

    int speed = danceSpeedNow();               // faster in the POWER parts
    if (soft)
        speed = percentOf(speed, DANCE_SOFT_PERCENT);
    int twist = percentOf(speed, DANCE_TWIST_PERCENT);
    int inner = percentOf(speed, DANCE_CURVE_PERCENT);
    int head = SERVO_CENTER;

    switch (move)
    {
    case FWD:     forward(speed);         break;
    case BACK:    backward(backFull ? FULL_SPEED : speed);   break;
    case DASH:    forward(FULL_SPEED);    break;
    case SPIN_L:  danceTurn(speed, true);    head = SERVO_LEFT;   break;
    case SPIN_R:  danceTurn(speed, false);   head = SERVO_RIGHT;  break;
    case TWIST_L: danceTurn(twist, true);    head = SERVO_LEFT;   break;
    case TWIST_R: danceTurn(twist, false);   head = SERVO_RIGHT;  break;
    case SWAY_L:  danceWheels(inner, speed, DIR_FORWARD);         head = SERVO_LEFT;   break;
    case SWAY_R:  danceWheels(speed, inner, DIR_FORWARD);         head = SERVO_RIGHT;  break;
    case BACK_L:  danceWheels(inner, speed, DIR_BACKWARD);        break;
    case BACK_R:  danceWheels(speed, inner, DIR_BACKWARD);        break;
    case LOOK_L:  stop();   head = SERVO_LEFT;   break;
    case LOOK_R:  stop();   head = SERVO_RIGHT;  break;
    case FREEZE:  stop();   if (DANCE_HEAD) scanner.write(SERVO_CENTER);   return;   // a pose: head dead ahead, no bob
    default:      stop();                 break;            // POSE
    }

    if (round && (move == SPIN_L || move == SPIN_R))
        danceTurnStopMs = (millis() + fullTurnMs(speed)) | 1;  // (| 1: never exactly 0)

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

// The drum beat (DANCE_SOUND_DRUMS): kick on beats 1 and 3, clap on 2 and 4, hi-hat in between
void playDrumStep()
{
    if (danceStep % 2)
        playBeep(NOTE_BEEP, 12);
    else if (danceStep % 4 == 0)
        playSound(SOUND_KICK);
    else
        playSound(SOUND_CLAP);
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

        DEBUG.print(F("DANCE part "));
        DEBUG.println(dancePart);
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
    playSound(TUNE_TADA);
    DEBUG.println(F("DANCE finished"));
}

void updateDance()
{
    if (!danceMode)
        return;

    if (danceTurnStopMs && (long)(millis() - danceTurnStopMs) >= 0)   // a ROUND spin has gone once round: stop, facing the front
    {
        danceTurnStopMs = 0;
        stop();
    }

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

    byte step = pgm_read_byte(&SONG_MAP[dancePart][0]);        // which of the song's dance steps (SONG_MOVES)
    byte code = pgm_read_byte(&SONG_MOVES[step][danceStep]);

    doDanceMove(code & 0x0F, code & SOFT, code & ROUND);

    // the song's special moment (e.g. "FEEDBACK!"): text on the screen + a zap
    bool shoutNow = false;
    for (byte i = 0; i < 3; i++)
        if (SONG_SHOUT_STEPS[i] >= 0 && danceStepCount == (unsigned int)SONG_SHOUT_STEPS[i])
            shoutNow = true;

    if (shoutNow)
    {
        shoutUntil = millis() + SHOUT_MS;
        playSound(SOUND_ZAP);
        DEBUG.println((const __FlashStringHelper*)SONG_SHOUT);
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
        playDrumStep();
    }

    if (!nextDanceStep())
        dancePart = SONG_PARTS;                // the end comes after this last step
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
    DEBUG.println(toLeft ? F("SEARCH -> turn left until open") : F("SEARCH -> turn right until open"));

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
    DEBUG.println(toLeft ? F("SEARCH boxed in -> 90 left") : F("SEARCH boxed in -> 90 right"));

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
            DEBUG.print(F("SEARCH ahead: "));
            DEBUG.println(ahead);
        }

        if (ahead <= STOP_DISTANCE)
        {
            DEBUG.print(F("SEARCH blocked at "));
            DEBUG.println(ahead);
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
                DEBUG.println(F("SEARCH stuck - backing out"));
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

            DEBUG.print(F("SEARCH looked  L="));   // debug: what it saw around it
            DEBUG.print(leftDistance);
            DEBUG.print(F("  C="));
            DEBUG.print(centerDistance);
            DEBUG.print(F("  R="));
            DEBUG.println(rightDistance);

            chooseClearestWay();
        }
        break;

    case AVOID_CORNER_CHECK:                   // ... stand still a moment, then look straight ahead
        if (millis() - avoidTimer >= SERVO_SETTLE_MS)
        {
            int ahead = filteredDistance();

            if (ahead >= SAFE_DISTANCE)
            {
                DEBUG.print(F("SEARCH opening found: "));
                DEBUG.println(ahead);
                searchDriveOn();               // an opening: go that way
            }
            else if (cornerTurns < MAX_CORNER_TURNS)
            {
                startCornerTurn(searchTurnLeft);   // still blocked: another 90 degrees
            }
            else
            {
                DEBUG.println(F("SEARCH turned all the way round - looking again"));
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
            DEBUG.print(F("SEARCH open way found: "));
            DEBUG.println(ahead);

            for (byte i = 0; i < SEARCH_TURN_EXTRA_STEPS; i++)
                turnStep();                    // a little more, so the whole car gets past the edge

            searchDriveOn();                   // the way is open: drive on
        }
        else if (turnSteps >= SEARCH_TURN_MAX_STEPS)
        {
            DEBUG.println(F("SEARCH no open way yet - looking again"));
            lookLeft();
            avoidTimer = millis();
            searchStep = AVOID_LOOK_LEFT;
        }
        break;
    }
    }
}


//======================================================
// 16b. PLAY MODES - follow me, goalkeeper, guard, music, gestures, pet, turbo,
//      and the low-battery check.
//      They use the newest distance reading (lastDistance - measured on every
//      pass of loop()), and the sensor stays on for them even in football mode.
//======================================================

// The nearest thing seen while looking around (its place in LOOK_ANGLES), or 255 for nothing
byte nearestLook(int maxCm)
{
    byte best = 255;

    for (byte i = 0; i < 5; i++)
        if (playSeen[i] < maxCm && (best == 255 || playSeen[i] < playSeen[best]))
            best = i;

    return best;
}

// A timed move: the motors stop exactly after ms (yield() and loop() stop them on time,
// and the screen waits meanwhile - like the 180 turn). playMoveDone() says when it's over.
void playMoveFor(unsigned long ms)
{
    playMoveMs = ms;
    playTimer = millis();
    playStopAt = (playTimer + ms) | 1;     // (| 1: never exactly 0)
}

void updatePlayStop()
{
    if (playStopAt && (long)(millis() - playStopAt) >= 0)
    {
        playStopAt = 0;
        stop();
    }
}

// Turn the car toward where the sensor looked (a timed turn - playMoveDone() says when it's over)
void playTurnToward(int angle)
{
    int degrees = angle - SERVO_CENTER;

    if ((degrees > 0) == (SERVO_LEFT > SERVO_CENTER))
        left();
    else
        right();

    playMoveFor(TURN90_TIME * abs(degrees) / 90);
}

bool playMoveDone()
{
    return millis() - playTimer >= playMoveMs;
}

// Look around: the sensor looks at the 5 angles one after the other.
// lookAroundDone() is true once it has seen them all (they're in playSeen[]).
void startLookAround()
{
    playLook = 0;
    scanner.write(LOOK_ANGLES[0]);
    playTimer = millis();
}

bool lookAroundDone()
{
    if (millis() - playTimer < SERVO_SETTLE_MS + 60)   // the sensor is still turning (+ one fresh measurement)
        return false;

    playSeen[playLook] = lastDistance;

    if (++playLook < 5)
    {
        scanner.write(LOOK_ANGLES[playLook]);
        playTimer = millis();
        return false;
    }

    lookCenter();
    return true;
}

// FOLLOW ME: keeps between FOLLOW_NEAR_CM and FOLLOW_FAR_CM from you.
// Lost you? It looks around and turns toward the nearest thing it sees.
void updateFollow()
{
    int d = lastDistance;
    int speed = percentOf(currentSpeed, FOLLOW_SPEED_PERCENT);

    switch (playStep)
    {
    case FOLLOW_TRACK:
        if (d < FOLLOW_NEAR_CM)
            backward(speed);
        else if (d <= FOLLOW_FAR_CM)
            stop();
        else if (d <= FOLLOW_LOST_CM)
            forward(speed);
        else
        {
            stop();

            if (millis() - playTimer > 1500)      // lost you for a moment: look around
            {
                startLookAround();
                playStep = FOLLOW_LOOK;
            }
            break;
        }

        playTimer = millis();                     // (it can see you)
        break;

    case FOLLOW_LOOK:
        if (lookAroundDone())
        {
            byte you = nearestLook(FOLLOW_LOST_CM);

            if (you == 255)                       // nobody there: wait a moment, then look again
            {
                playTimer = millis();
                playStep = FOLLOW_TRACK;
            }
            else
            {
                playTurnToward(LOOK_ANGLES[you]);
                playStep = FOLLOW_TURN;
            }
        }
        break;

    case FOLLOW_TURN:
        if (playMoveDone())
        {
            stop();
            playTimer = millis();
            playStep = FOLLOW_TRACK;
        }
        break;
    }
}

// GOALKEEPER: looks for the ball, turns to face it, and punches it away when it comes close
void keeperPunch()
{
    forward(FULL_SPEED);
    playMoveFor(KEEPER_PUNCH_MS);
    playStep = KEEPER_PUNCH;
    playSound(TUNE_SAVE);
}

void updateKeeper()
{
    int d = lastDistance;

    switch (playStep)
    {
    case KEEPER_SCAN:                             // start looking for the ball
        startLookAround();
        playStep = KEEPER_LOOK;
        break;

    case KEEPER_LOOK:
        if (lookAroundDone())
        {
            byte ball = nearestLook(SEE_CM);

            if (ball == 255)                      // no ball: look again
            {
                playStep = KEEPER_SCAN;
            }
            else
            {
                playTurnToward(LOOK_ANGLES[ball]);
                playStep = KEEPER_TURN;
            }
        }
        break;

    case KEEPER_TURN:                             // turning to face the ball
        if (playMoveDone())
        {
            stop();
            playTimer = millis();
            playStep = KEEPER_WATCH;
        }
        break;

    case KEEPER_WATCH:                            // eyes on the ball
        if (d <= KEEPER_SAVE_CM)
            keeperPunch();
        else if (d <= SEE_CM)
            playTimer = millis();
        else if (millis() - playTimer > 1500)     // lost the ball: look for it again
            playStep = KEEPER_SCAN;
        break;

    case KEEPER_PUNCH:                            // punch: dash at the ball ...
        if (playMoveDone())
        {
            backward(FULL_SPEED);
            playMoveFor(KEEPER_PUNCH_MS);
            playStep = KEEPER_BACK;
        }
        break;

    case KEEPER_BACK:                             // ... and back into the goal
        if (playMoveDone())
        {
            stop();
            playTimer = millis();
            playStep = KEEPER_WATCH;
        }
        break;
    }
}

// GUARD: counts down (walk away!), learns what the room looks like, then sweeps the
// sensor back and forth. Something new and closer = INTRUDER: siren, and it turns to face them.
void updateGuard()
{
    int d = lastDistance;

    switch (playStep)
    {
    case GUARD_ARM:                               // countdown: one beep a second
        if (millis() - playTimer < 1000)
            break;

        playTimer = millis();

        if (playCount > 0)
        {
            playCount--;
            playBeep(NOTE_A5, 60);
            break;
        }

        startLookAround();
        playStep = GUARD_LEARN;
        break;

    case GUARD_LEARN:                             // learning the room
        if (!lookAroundDone())
            break;

        playLook = 2;                             // start watching from the middle ...
        playFlag = true;                          // ... sweeping to the left first
        playCount = 0;
        playTimer = millis();
        playStep = GUARD_WATCH;
        playBeep(NOTE_C6, 150);
        break;

    case GUARD_WATCH:                             // sweeping, comparing with the room
        if (millis() - playTimer < SERVO_SETTLE_MS + 60)
            break;

        if (d + GUARD_CLOSER_CM < playSeen[playLook])
        {
            if (++playCount < 2)                  // check once more before raising the alarm
                break;

            DEBUG.println(F("GUARD: INTRUDER"));
            playCm = d;
            lookCenter();
            playTurnToward(LOOK_ANGLES[playLook]);
            playStep = GUARD_TURN;
            playSound(TUNE_SIREN);
            break;
        }

        playCount = 0;

        if (playLook == 4)
            playFlag = false;
        else if (playLook == 0)
            playFlag = true;

        playLook += playFlag ? 1 : -1;
        scanner.write(LOOK_ANGLES[playLook]);
        playTimer = millis();
        break;

    case GUARD_TURN:                              // turning to face the intruder
        if (playMoveDone())
        {
            stop();
            playTimer = millis();
            playSince = millis();
            playStep = GUARD_ALARM;
        }
        break;

    case GUARD_ALARM:                             // siren until the intruder has gone for a while
        if (!soundPlaying())
            playSound(TUNE_SIREN);

        if (d <= playCm + 40)                     // still there
            playSince = millis();

        // gone for a while (or 20 s of siren): learn the room again (the car has turned)
        if (millis() - playSince > GUARD_CALM_MS || millis() - playTimer > 20000)
        {
            stopSound();
            startLookAround();
            playStep = GUARD_LEARN;
        }
        break;
    }
}

// MUSIC: your hand plays notes - closer = higher. The notes are a "pentatonic" scale,
// so whatever you play sounds nice.
const uint16_t MUSIC_NOTES[] PROGMEM = {523, 587, 659, 784, 880, 1047, 1175, 1319, 1568, 1760, 2093};

void updateMusic()
{
    int d = lastDistance;
    byte note = 255;                              // 255 = no hand = silence

    if (d <= MUSIC_FAR_CM)
        note = constrain((long)(MUSIC_FAR_CM - d) * 11 / (MUSIC_FAR_CM - MUSIC_NEAR_CM), 0, 10);

    if (note == playCount)
        return;

    playCount = note;
    buzzerNote(note == 255 ? 0 : pgm_read_word(&MUSIC_NOTES[note]));
}

// GESTURES: one tap = spin round, two taps = happy wiggle, hold your hand close = it backs
// away, hold it a bit further away = it comes to you.
void gestureMove(unsigned long ms)
{
    playMoveFor(ms);
    playStep = GESTURE_MOVE;
}

void updateGesture()
{
    int d = lastDistance;
    bool hand = d < GESTURE_ZONE_CM;
    unsigned long now = millis();
    int speed = percentOf(currentSpeed, 55);

    switch (playStep)
    {
    case GESTURE_WATCH:
        if (hand)
        {
            if (playSince == 0)
                playSince = now;                  // a hand appeared

            unsigned long held = now - playSince;

            if (held >= GESTURE_HOLD_MS && d < GESTURE_NEAR_CM)              // pushed: back away
            {
                playFlag = false;
                backward(speed);
                gestureMove(500);
                playCount = 0;
            }
            else if (held >= GESTURE_HOLD_MS * 2 && d >= GESTURE_NEAR_CM)    // held further away: come here
            {
                playFlag = true;
                forward(speed);
                gestureMove(1000);
                playCount = 0;
            }
            break;
        }

        if (playSince != 0 && now - playSince < GESTURE_TAP_MS)   // in and out quickly: a tap
        {
            playCount++;
            playOther = now;
        }
        playSince = 0;

        if (playCount > 0 && now - playOther > 600)               // the taps are over: do the trick
        {
            if (playCount == 1)                   // one tap: spin round once
            {
                playFlag = false;
                left();
                gestureMove(TURN180_TIME * 2);
            }
            else                                  // two or more: a happy wiggle
            {
                playTimer = now;
                playStep = GESTURE_WIGGLE;
                playSound(TUNE_HAPPY);
            }
            playCount = 0;
        }
        break;

    case GESTURE_MOVE:                            // doing a move ("come here" stops at your hand)
        if (playMoveDone() || (playFlag && d < GESTURE_NEAR_CM))
        {
            stop();
            playStopAt = 0;
            playStep = GESTURE_CLEAR;
        }
        break;

    case GESTURE_WIGGLE:                          // twist left, right, left ...
    {
        unsigned long t = now - playTimer;

        if (t >= 6 * 130UL)
        {
            stop();
            playStep = GESTURE_CLEAR;
        }
        else
        {
            danceTurn(percentOf(currentSpeed, 70), (t / 130) % 2 == 0);
        }
        break;
    }

    case GESTURE_CLEAR:                           // wait for the hand to go before the next gesture
        if (!hand)
        {
            playSince = 0;
            playStep = GESTURE_WATCH;
        }
        break;
    }
}

// PET: wanders about slowly, sits and looks around, gets bored, falls asleep.
// A hand in front of it while it sits = petting: it wakes up, wiggles and purrs.
void petStep(byte step, unsigned long ms)
{
    playStep = step;
    playMoveMs = ms;
    playTimer = millis();
    playOther = millis();
}

void petHappy()
{
    stop();
    lookCenter();
    playSince = millis();
    petStep(PET_HAPPY, 0);
    playSound(TUNE_HAPPY);
}

void updatePet()
{
    int d = lastDistance;
    unsigned long now = millis();
    unsigned long inStep = now - playTimer;
    bool lookingAhead = scanner.read() == SERVO_CENTER && now - playOther >= SERVO_SETTLE_MS + 60;

    switch (playStep)
    {
    case PET_WANDER:                              // a little walk
        if (d < 35)                               // something in the way: turn away from it
        {
            if (random(2))
                left();
            else
                right();
            petStep(PET_TURN, random(300, 700));
            break;
        }

        forward(percentOf(currentSpeed, PET_SPEED_PERCENT));

        if (inStep >= playMoveMs)                 // walked enough: sit down
        {
            stop();
            petStep(PET_SIT, 6000);
        }
        break;

    case PET_TURN:
        if (inStep >= playMoveMs)
        {
            stop();
            petStep(PET_WANDER, random(5000, 10000));
        }
        break;

    case PET_SIT:                                 // sitting, looking around
        if (lookingAhead && d < PET_HAND_CM)      // a hand!
        {
            petHappy();
            break;
        }

        if (now - playOther >= 1500)              // turn the head: somewhere, then back to the middle
        {
            playOther = now;
            scanner.write(scanner.read() == SERVO_CENTER ? LOOK_ANGLES[random(5)] : SERVO_CENTER);
        }

        if (inStep >= playMoveMs)
        {
            lookCenter();

            if (now - playSince > PET_BORED_MS)   // nobody played with it for ages: bored
            {
                petStep(PET_BORED, 0);
                playSound(TUNE_SIGH);
            }
            else
            {
                petStep(PET_WANDER, random(5000, 10000));
            }
        }
        break;

    case PET_BORED:
    case PET_SLEEP:
        if (inStep > SERVO_SETTLE_MS + 60 && d < PET_HAND_CM)    // a hand!
        {
            petHappy();
            break;
        }

        if (playStep == PET_BORED)
        {
            if (now - playOther >= 8000)          // a sigh now and then
            {
                playOther = now;
                playSound(TUNE_SIGH);
            }

            if (now - playSince > PET_SLEEP_MS)   // ... and off to sleep
                petStep(PET_SLEEP, 0);
        }
        else if (now - playOther >= 4000)         // asleep: a snore now and then
        {
            playOther = now;
            playSound(TUNE_SNORE);
        }
        break;

    case PET_HAPPY:                               // being petted: purr while your hand is there
        if (d < PET_HAND_CM + 10)
            playSince = now;

        if (!soundPlaying())
            playSound(TUNE_PURR);

        if (inStep < 6 * 120UL)                   // a happy wiggle first
            danceTurn(percentOf(currentSpeed, 70), (inStep / 120) % 2 == 0);
        else
            stop();

        if (now - playSince > 2000)               // you stopped petting it: off it goes again
        {
            stop();
            stopSound();
            petStep(PET_WANDER, random(4000, 8000));
        }
        break;
    }
}

// TURBO: full power for TURBO_MS (it still stops for obstacles, unless football mode is on)
void updateTurbo()
{
    if (lastDistance <= STOP_DISTANCE)
    {
        brakeStop();
        robotState = IDLE;
        return;
    }

    forward(FULL_SPEED);

    if (millis() - playTimer >= TURBO_MS)
    {
        stop();
        robotState = IDLE;
    }
}

// RADAR: every step, keep what the sensor saw, tell the app ("#R:direction,cm"),
// and turn the sensor 10 degrees on (right to left, then back)
void updateRadar()
{
    if (millis() - playTimer < RADAR_STEP_MS)
        return;

    radarCm[playLook] = min(lastDistance, 255);
    radarDirty = true;

    char line[10] = "#R:";               // e.g. "#R:f42" = straight ahead (f), 42 cm
    line[3] = 'a' + playLook;            // a = 50 degrees right ... f = ahead ... k = 50 degrees left
    itoa(lastDistance, line + 4, 10);
    BT.print(line);
    BT.print('\n');

    if (playLook == RADAR_STEPS - 1)
        playFlag = false;                // reached the left end: back to the right
    else if (playLook == 0)
        playFlag = true;
    playLook = playFlag ? playLook + 1 : playLook - 1;

    scanner.write(radarAngle(playLook));
    playTimer = millis();
}

// Runs the play mode that is on (called from loop())
void updatePlayModes()
{
    switch (robotState)
    {
    case FOLLOW:   if (HAS_FOLLOW) updateFollow();     break;
    case KEEPER:   if (HAS_KEEPER) updateKeeper();     break;
    case GUARD:    if (HAS_GUARD) updateGuard();       break;
    case MUSIC:    if (HAS_MUSIC) updateMusic();       break;
    case GESTURE:  if (HAS_GESTURE) updateGesture();   break;
    case PET:      if (HAS_PET) updatePet();           break;
    case RADAR:    if (HAS_RADAR) updateRadar();       break;
    case TURBO:    updateTurbo();                      break;
    default:       break;
    }
}

// LOW BATTERY: measures the Arduino's own power supply against its built-in 1.1 V
// reference. When it has stayed low for 10 seconds: "LOW BATT!" on the screen and a
// sad tune (again every minute).
int readSupplyMv()
{
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);   // the 1.1 V reference, measured against the supply
    delayMicroseconds(500);                   // let it settle

    ADCSRA |= _BV(ADSC);                      // (the first reading after switching is not accurate)
    while (ADCSRA & _BV(ADSC))
    {
    }

    ADCSRA |= _BV(ADSC);
    while (ADCSRA & _BV(ADSC))
    {
    }

    unsigned int raw = ADC;
    return raw ? 1125300L / raw : 0;          // 1.1 V x 1023 x 1000
}

void updateBattery()
{
    static unsigned long checkedAt = 0;
    static byte lowChecks = 0;
    static unsigned long warnedAt = 0;

    if (millis() - checkedAt < BATTERY_CHECK_MS)
        return;

    checkedAt = millis();

    int mv = readSupplyMv();

    if (mv < 2500)                            // not a real reading
        return;

    if (batteryStartMv == 0)                  // the first reading: remember the starting level
    {
        batteryStartMv = mv;
        return;
    }

    bool low = mv < batteryStartMv - BATTERY_DROP_MV || mv < BATTERY_LOW_MV;

    if (!low)
        lowChecks = 0;
    else if (lowChecks < 10)
        lowChecks++;

    if (lowChecks >= 5 && (!batteryLow || millis() - warnedAt >= 60000))
    {
        batteryLow = true;
        warnedAt = millis();
        playSound(TUNE_BATTERY_LOW);
        DEBUG.println(F("LOW BATTERY"));
    }
}


//======================================================
// 17. SETUP (runs once) AND LOOP (runs forever)
//======================================================

void setup()
{
    DEBUG.begin(SERIAL_BAUD);
    BT.begin(BT_BAUD);

    randomSeed(analogRead(A3) ^ micros());   // A3 is unused, its noise makes CHAOS different each time

    // The numbers 1-7 are start-up progress markers in the Serial Monitor
    DEBUG.println(F("1"));

    pinMode(ENA, OUTPUT);
    pinMode(ENB, OUTPUT);

    DEBUG.println(F("2"));

    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);

    DEBUG.println(F("3"));

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    DEBUG.println(F("4"));

    scanner.attach(SERVO_PIN);

    DEBUG.println(F("5"));

    lookCenter();

    DEBUG.println(F("6"));

    stop();

    DEBUG.println(F("7"));

    setupBuzzer();             // (the IR add-on's IrReceiver.begin() goes just before this line)

    setupDisplay();

    startGreeting(false);      // power-on hello: screen + fanfare + servo wave

    DEBUG.println(F("READY"));
}

void loop()
{
    handleBluetooth();         // phone commands

    updateDriving();           // auto forward/backward, donut, curve

    updateDance();

    update180();

    updateBrake();

    checkObstacle();           // distance check (also sets loop timing)

    handleBluetooth();         // (again: the distance check takes a while - this answers the phone sooner)

    updateChaos();

    runObstacleAvoidance();    // SEARCH mode

    updatePlayStop();          // a play mode's timed move: stop on time

    updatePlayModes();         // follow me, goalkeeper, guard, music, gestures, pet, turbo

    updateBattery();           // low battery check

    updateStatus();            // tell the app what the car is doing

    updateSound();             // next note of the tune

    updateParkingBeeps();      // beep-beep-beep when getting close

    updateGreeting();          // hello screen timer + servo wave

    updateDizzy();             // dizzy head wobble after a donut

    updateDisplay();           // OLED: one small part per pass

}
