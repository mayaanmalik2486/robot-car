/*
  Guardian V1 - obstacle avoiding 4WD Arduino car

  Hardware used (only the parts already on your car):
    Arduino Uno + Sensor Shield V5.0
    L298N motor driver (ENA and ENB jumpers FITTED)
    HC-SR04 ultrasonic sensor on an SG90 servo

  Wiring used:
    L298N IN1 -> D2       L298N IN2 -> D3
    L298N IN3 -> D4       L298N IN4 -> D5
    Servo signal -> D9
    HC-SR04 TRIG -> D11   HC-SR04 ECHO -> D12

  Important: with ENA/ENB jumpers fitted, D2-D5 are direction inputs,
  not speed-control pins. This is why this sketch uses digitalWrite for
  the motors. Do not use analogWrite on these four pins.
*/

#include <Servo.h>

// -------------------- PIN MAP --------------------
const byte IN1_PIN = 2;       // L298N IN1: left motors
const byte IN2_PIN = 3;       // L298N IN2: left motors
const byte IN3_PIN = 4;       // L298N IN3: right motors
const byte IN4_PIN = 5;       // L298N IN4: right motors
const byte SERVO_PIN = 9;
const byte TRIG_PIN = 11;
const byte ECHO_PIN = 12;

// ---------------- EASY SETTINGS ------------------
// Change these only after testing. Times depend on battery charge and floor.
const int OBSTACLE_CM = 24;       // stop and look when something is nearer
const int VERY_CLOSE_CM = 12;     // emergency stop distance
const int CLEAR_CM = 45;          // distance considered a comfortably open path
const int MAX_SCAN_CM = 150;      // distances farther than this count as this

const int SERVO_CENTER = 90;
const int SERVO_RIGHT = 18;       // safe physical right limit
const int SERVO_LEFT = 162;       // safe physical left limit
const int SERVO_STEP = 3;

const unsigned long REVERSE_MS = 230;
const unsigned long GENTLE_TURN_MS = 190;
const unsigned long NORMAL_TURN_MS = 355;
const unsigned long UTURN_MS = 700; // tune this first if U-turn is not 180 degrees

// If forward is backwards on your particular car, swap HIGH and LOW in BOTH
// pairs below. Do not change the wiring while the battery is connected.
const byte LEFT_FORWARD_IN1 = HIGH;
const byte LEFT_FORWARD_IN2 = LOW;
const byte RIGHT_FORWARD_IN3 = HIGH;
const byte RIGHT_FORWARD_IN4 = LOW;

Servo scanner;
int currentServoAngle = SERVO_CENTER;

struct ScanResult {
  int rightWide;
  int rightNear;
  int front;
  int leftNear;
  int leftWide;
};

// ------------------ MOTOR CONTROL -----------------
void stopCar() {
  // Coast: both inputs LOW on each L298N channel.
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, LOW);
  digitalWrite(IN3_PIN, LOW);
  digitalWrite(IN4_PIN, LOW);
}

void forward() {
  digitalWrite(IN1_PIN, LEFT_FORWARD_IN1);
  digitalWrite(IN2_PIN, LEFT_FORWARD_IN2);
  digitalWrite(IN3_PIN, RIGHT_FORWARD_IN3);
  digitalWrite(IN4_PIN, RIGHT_FORWARD_IN4);
}

void backward() {
  digitalWrite(IN1_PIN, LEFT_FORWARD_IN2);
  digitalWrite(IN2_PIN, LEFT_FORWARD_IN1);
  digitalWrite(IN3_PIN, RIGHT_FORWARD_IN4);
  digitalWrite(IN4_PIN, RIGHT_FORWARD_IN3);
}

// From the driver's seat: left wheels forward, right wheels backward.
void spinRight() {
  digitalWrite(IN1_PIN, LEFT_FORWARD_IN1);
  digitalWrite(IN2_PIN, LEFT_FORWARD_IN2);
  digitalWrite(IN3_PIN, RIGHT_FORWARD_IN4);
  digitalWrite(IN4_PIN, RIGHT_FORWARD_IN3);
}

// From the driver's seat: left wheels backward, right wheels forward.
void spinLeft() {
  digitalWrite(IN1_PIN, LEFT_FORWARD_IN2);
  digitalWrite(IN2_PIN, LEFT_FORWARD_IN1);
  digitalWrite(IN3_PIN, RIGHT_FORWARD_IN3);
  digitalWrite(IN4_PIN, RIGHT_FORWARD_IN4);
}

void reverseBriefly() {
  backward();
  delay(REVERSE_MS);
  stopCar();
  delay(100);
}

void turnRightFor(unsigned long timeMs) {
  spinRight();
  delay(timeMs);
  stopCar();
  delay(100);
}

void turnLeftFor(unsigned long timeMs) {
  spinLeft();
  delay(timeMs);
  stopCar();
  delay(100);
}

// ---------------- SERVO AND ULTRASONIC ------------
void moveScannerTo(int target) {
  target = constrain(target, SERVO_RIGHT, SERVO_LEFT);

  if (target > currentServoAngle) {
    for (int angle = currentServoAngle; angle < target; angle += SERVO_STEP) {
      scanner.write(angle);
      delay(14);
    }
  } else {
    for (int angle = currentServoAngle; angle > target; angle -= SERVO_STEP) {
      scanner.write(angle);
      delay(14);
    }
  }

  scanner.write(target);
  currentServoAngle = target;
  delay(90); // let the servo stop shaking before listening
}

int rawDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(3);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // 12 ms is approximately 2 metres. A missing echo means open space.
  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 12000UL);
  if (duration == 0) return MAX_SCAN_CM;

  int cm = (int)(duration / 58UL);
  if (cm < 2) return 2;
  return constrain(cm, 2, MAX_SCAN_CM);
}

int reliableDistanceCm() {
  // Median of three readings: rejects one bad ultrasonic echo.
  int a = rawDistanceCm(); delay(35);
  int b = rawDistanceCm(); delay(35);
  int c = rawDistanceCm();

  if (a > b) { int t = a; a = b; b = t; }
  if (b > c) { int t = b; b = c; c = t; }
  if (a > b) { int t = a; a = b; b = t; }
  return b;
}

int scanAt(int angle) {
  moveScannerTo(angle);
  return reliableDistanceCm();
}

// -------------------- ROBOT BRAIN -----------------
ScanResult scanSurroundings() {
  ScanResult s;

  // Sweep right-to-left so the head looks naturally rather than jumping.
  s.rightWide = scanAt(SERVO_RIGHT);
  s.rightNear = scanAt(52);
  s.front     = scanAt(SERVO_CENTER);
  s.leftNear  = scanAt(128);
  s.leftWide  = scanAt(SERVO_LEFT);

  moveScannerTo(SERVO_CENTER);
  return s;
}

int sideScore(int nearDistance, int wideDistance) {
  // The near diagonal is more important: it prevents turning into a wall.
  return nearDistance * 2 + wideDistance;
}

bool sideIsBlocked(int nearDistance, int wideDistance) {
  return nearDistance < OBSTACLE_CM && wideDistance < OBSTACLE_CM;
}

void avoidObstacle() {
  stopCar();
  delay(120);

  // Back up first so a standing turn does not rub the obstacle with a wheel.
  reverseBriefly();

  ScanResult s = scanSurroundings();
  bool rightBlocked = sideIsBlocked(s.rightNear, s.rightWide);
  bool leftBlocked = sideIsBlocked(s.leftNear, s.leftWide);

  // Dead end: both sides tight. Make a full turn-around.
  if (rightBlocked && leftBlocked) {
    turnRightFor(UTURN_MS);
    return;
  }

  int rightScore = sideScore(s.rightNear, s.rightWide);
  int leftScore = sideScore(s.leftNear, s.leftWide);

  if (rightScore > leftScore) {
    // A wide-open right side needs a normal turn; a diagonal opening needs less.
    turnRightFor((s.rightNear >= CLEAR_CM) ? NORMAL_TURN_MS : GENTLE_TURN_MS);
  } else if (leftScore > rightScore) {
    turnLeftFor((s.leftNear >= CLEAR_CM) ? NORMAL_TURN_MS : GENTLE_TURN_MS);
  } else {
    // Same amount of space: alternate choice using Arduino's changing timer.
    if ((millis() / 1000UL) % 2 == 0) turnRightFor(NORMAL_TURN_MS);
    else turnLeftFor(NORMAL_TURN_MS);
  }
}

void setup() {
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(IN3_PIN, OUTPUT);
  pinMode(IN4_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  stopCar();
  digitalWrite(TRIG_PIN, LOW);

  scanner.attach(SERVO_PIN);
  scanner.write(SERVO_CENTER);
  currentServoAngle = SERVO_CENTER;
  delay(800); // keep still while the servo finds its centre
}

void loop() {
  // The sensor remains centred while driving.
  int frontDistance = reliableDistanceCm();

  if (frontDistance <= VERY_CLOSE_CM) {
    stopCar();
    delay(80);
    avoidObstacle();
  } else if (frontDistance <= OBSTACLE_CM) {
    avoidObstacle();
  } else {
    forward();
  }

  delay(35);
}
