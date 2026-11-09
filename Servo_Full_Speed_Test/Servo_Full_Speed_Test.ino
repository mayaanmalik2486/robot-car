/*
  Servo_Full_Speed_Test.ino
  Tests ONLY the servo, at full speed. It never drives the motors.

  The servo (on D9) snaps straight to each position as fast as it can:
  middle -> right -> left -> middle, then quick hops left-right, then repeats.

  A healthy SG90 does each big swing in well under half a second, smoothly,
  and stops dead on each position. If it stalls, clicks, buzzes or stops
  partway, its gears are damaged - replace it.
*/
#include <Servo.h>

const byte SERVO_PIN = 9;
Servo head;

void go(int angle, int waitMs) {
  head.write(angle);           // jump straight there - no slow steps
  delay(waitMs);
}

void setup() {
  head.attach(SERVO_PIN);
  go(90, 800);                 // start in the middle
}

void loop() {
  // big swings
  go(25, 600);                 // right
  go(155, 600);                // left
  go(90, 600);                 // middle

  // quick hops
  for (byte i = 0; i < 4; i++) {
    go(60, 250);
    go(120, 250);
  }
  go(90, 1000);                // middle, rest
}
