/*
  ServoOnlyTest.ino
  Safe diagnostic: this program NEVER drives the motors.

  Upload this with the car raised / wheels clear. It should move the ultrasonic
  sensor slowly: centre -> right -> centre -> left -> centre, then repeat.
*/
#include <Servo.h>

const byte SERVO_PIN = 9;
Servo head;

void setup() {
  head.attach(SERVO_PIN);
  head.write(90);
  delay(700);
}

void goSmoothly(int target) {
  int now = head.read();
  int step = (target > now) ? 2 : -2;
  for (int p = now; (step > 0) ? p < target : p > target; p += step) {
    head.write(p);
    delay(20);
  }
  head.write(target);
  delay(500);
}

void loop() {
  goSmoothly(25);
  goSmoothly(90);
  goSmoothly(155);
  goSmoothly(90);
  delay(1000);
}
