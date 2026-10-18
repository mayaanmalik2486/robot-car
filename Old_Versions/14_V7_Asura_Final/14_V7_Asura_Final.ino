/*
  ASURA V7 FINAL
  Arduino Uno R3 / L298N / HC-05 / HC-SR04 / SG90 / VS1838B

  The hardware pinout in this file is intentionally fixed to the supplied
  wiring.  Bluetooth is the primary control source; an IR command is accepted
  only when Bluetooth has been inactive for a short priority window, except
  for the IR emergency-stop key.

  Bluetooth controller commands
    F B L R              Manual forward, backward, curve left, curve right
    G I H J              Forward-left, forward-right, backward-left, backward-right
    S                    Emergency stop
    1 2 3 4              25, 50, 75 and 100 percent cruise-speed presets
    Y U u                Auto forward, auto backward, search

  Legacy Bluetooth aliases retained from the supplied V7 sketch
    Q/W                  Slow/full speed
    A/X                  Auto forward/auto backward
    D                    Donut
    T                    Timed 180 degree turn
    K                    Smooth brake
    N/P/Z                Dance/demo/zigzag
    M/V/O                Drift left/drift right/search

  Steering packets are accepted without requiring a newline:
    F99R00, F60L25, B45R50
  Packet format: direction, two throttle digits, steering direction,
  two steering digits.  Invalid packets are discarded.
*/

#include <Arduino.h>
#include <Servo.h>
#include <IRremote.hpp>
#include <SoftwareSerial.h>

// -----------------------------------------------------------------------------
// Build options
// -----------------------------------------------------------------------------

#define DEBUG_ENABLED 0

#if DEBUG_ENABLED
#define DEBUG_LINE(message) Serial.println(F(message))
#else
#define DEBUG_LINE(message) do { } while (0)
#endif

// -----------------------------------------------------------------------------
// Exact hardware pin assignment
// -----------------------------------------------------------------------------

const byte PIN_LEFT_ENABLE = 5;       // L298N ENA, PWM
const byte PIN_RIGHT_ENABLE = 6;      // L298N ENB, PWM
const byte PIN_LEFT_IN1 = 2;          // L298N IN1
const byte PIN_LEFT_IN2 = 4;          // L298N IN2
const byte PIN_RIGHT_IN3 = 7;         // L298N IN3
const byte PIN_RIGHT_IN4 = 10;        // L298N IN4

const byte PIN_IR_RECEIVER = 8;
const byte PIN_SERVO = 9;
const byte PIN_ULTRASONIC_TRIGGER = 11;
const byte PIN_ULTRASONIC_ECHO = 12;

const byte PIN_BT_RX = 3;             // Arduino RX, connected to HC-05 TX
const byte PIN_BT_TX = A0;            // Arduino TX, connected to HC-05 RX

// -----------------------------------------------------------------------------
// IR command values from the supplied remote
// -----------------------------------------------------------------------------

const byte IR_UP = 0x18;
const byte IR_DOWN = 0x52;
const byte IR_LEFT = 0x08;
const byte IR_RIGHT = 0x5A;
const byte IR_OK = 0x1C;

const byte IR_1 = 0x45;
const byte IR_2 = 0x46;
const byte IR_3 = 0x47;
const byte IR_4 = 0x44;
const byte IR_5 = 0x40;
const byte IR_6 = 0x43;
const byte IR_7 = 0x07;
const byte IR_8 = 0x15;
const byte IR_9 = 0x09;
const byte IR_STAR = 0x16;
const byte IR_0 = 0x19;
const byte IR_HASH = 0x0D;

// -----------------------------------------------------------------------------
// Tunable motion, sensor, and timing values
// -----------------------------------------------------------------------------

const int PWM_MAX = 255;
const int SPEED_FULL = PWM_MAX;
const int SPEED_SLOW = 153;
const int SPEED_DANCE = 200;
const int SPEED_TURN_MIN = 110;

const int SERVO_LEFT = 140;
const int SERVO_CENTER = 90;
const int SERVO_RIGHT = 40;
const int SERVO_MIN = 15;
const int SERVO_MAX = 165;

const int STOP_DISTANCE_CM = 30;
const int SAFE_DISTANCE_CM = 40;
const int NO_ECHO_DISTANCE_CM = 999;

const unsigned long MOTOR_UPDATE_INTERVAL_MS = 20;
const int MOTOR_RAMP_STEP = 12;

const unsigned long ULTRASONIC_PING_INTERVAL_MS = 17;
const unsigned long ULTRASONIC_ECHO_TIMEOUT_US = 8000UL;

const unsigned long SERVO_SETTLE_TIME_MS = 70;
const unsigned long TURN_180_TIME_MS = 440;
const unsigned long IR_TURN_PULSE_TIME_MS = 150;

const unsigned long BLUETOOTH_MANUAL_TIMEOUT_MS = 500;
const unsigned long IR_MANUAL_TIMEOUT_MS = 800;
const unsigned long BLUETOOTH_PRIORITY_WINDOW_MS = 1500;
const unsigned long BLUETOOTH_PACKET_WAIT_MS = 45;

const unsigned long DANCE_STEP_TIME_MS = 215;

const unsigned long DEMO_FORWARD_TIME_MS = 400;
const unsigned long DEMO_PAUSE_TIME_MS = 600;
const unsigned long DEMO_REVERSE_TIME_MS = 375;
const unsigned long DEMO_DONUT_TIME_MS = 2000;
const unsigned long DEMO_LAUNCH_TIME_MS = 375;

const unsigned long ZIGZAG_FORWARD_TIME_MS = 600;
const unsigned long ZIGZAG_TURN_TIME_MS = 105;
const unsigned long ZIGZAG_REVERSE_TIME_MS = 180;
const unsigned long ZIGZAG_ESCAPE_TURN_TIME_MS = 350;

const unsigned long SEARCH_REVERSE_TIME_MS = 120;
const unsigned long SEARCH_FALLBACK_TURN_TIME_MS = 120;
const byte SEARCH_SCAN_STEP_DEGREES = 6;
const byte SEARCH_SCAN_COUNT = 26;
const byte SEARCH_MIN_GAP_SAMPLES = 12;
const unsigned long SEARCH_TURN_PER_DEGREE_MS = 3;
const unsigned long SEARCH_MIN_TURN_TIME_MS = 40;

// -----------------------------------------------------------------------------
// Robot state and input ownership
// -----------------------------------------------------------------------------

enum RobotState
{
  IDLE,
  MANUAL_FORWARD,
  MANUAL_BACKWARD,
  MANUAL_LEFT,
  MANUAL_RIGHT,
  AUTO_FORWARD,
  AUTO_BACKWARD,
  SEARCH,
  DRIFT_LEFT,
  DRIFT_RIGHT,
  DONUT,
  DANCE,
  DEMO,
  ZIGZAG,
  TURN180,
  BRAKING
};

enum ControlSource
{
  CONTROL_NONE,
  CONTROL_BLUETOOTH,
  CONTROL_IR
};

enum EchoState
{
  ECHO_IDLE,
  ECHO_WAIT_FOR_RISE,
  ECHO_WAIT_FOR_FALL
};

enum SearchPhase
{
  SEARCH_DRIVE_FORWARD,
  SEARCH_REVERSE,
  SEARCH_WAIT_FOR_SAMPLE,
  SEARCH_CHOOSE_GAP,
  SEARCH_TURN_TO_GAP
};

RobotState robotState = IDLE;
ControlSource activeControl = CONTROL_NONE;
EchoState echoState = ECHO_IDLE;
SearchPhase searchPhase = SEARCH_DRIVE_FORWARD;

// -----------------------------------------------------------------------------
// Hardware objects
// -----------------------------------------------------------------------------

Servo scannerServo;
SoftwareSerial bluetooth(PIN_BT_RX, PIN_BT_TX);

// -----------------------------------------------------------------------------
// Shared state.  There are no per-mode enable booleans: robotState owns mode.
// -----------------------------------------------------------------------------

int cruiseSpeed = SPEED_FULL;
int targetSpeed = 0;
int currentSpeed = 0;

int motorTargetLeft = 0;
int motorTargetRight = 0;
int motorAppliedLeft = 0;
int motorAppliedRight = 0;
unsigned long lastMotorUpdateAt = 0;

int manualThrottlePercent = 0;
int manualSteeringPercent = 0;
int manualPwmLimit = SPEED_FULL;

byte modePhase = 0;
unsigned long stateEnteredAt = 0;
unsigned long phaseEnteredAt = 0;
unsigned long stateDeadlineAt = 0;

unsigned long lastBluetoothCommandAt = 0;
unsigned long lastIrCommandAt = 0;

int servoAngle = SERVO_CENTER;
unsigned long lastServoMoveAt = 0;

int currentDistance = NO_ECHO_DISTANCE_CM;
unsigned long distanceUpdatedAt = 0;
int distanceSamples[3] = {NO_ECHO_DISTANCE_CM, NO_ECHO_DISTANCE_CM, NO_ECHO_DISTANCE_CM};
byte distanceSampleWriteIndex = 0;
byte distanceSampleCount = 0;
unsigned long nextPingAt = 0;
unsigned long echoStateStartedAt = 0;
unsigned long echoRiseAt = 0;

int searchDistances[SEARCH_SCAN_COUNT];
byte searchScanIndex = 0;
int searchTargetAngle = SERVO_CENTER;
int searchBestGapStart = -1;
int searchBestGapEnd = -1;
int searchBestGapSize = 0;
int searchBestGapScore = 0;

char bluetoothPacket[6];
byte bluetoothPacketLength = 0;
unsigned long bluetoothPacketStartedAt = 0;

// -----------------------------------------------------------------------------
// Small utility helpers
// -----------------------------------------------------------------------------

bool elapsedSince(unsigned long now, unsigned long then, unsigned long interval)
{
  return (unsigned long)(now - then) >= interval;
}

int magnitudeOf(int value)
{
  return (value < 0) ? -value : value;
}

int maximumOf(int first, int second)
{
  return (first > second) ? first : second;
}

int moveToward(int current, int destination, int step)
{
  if (current < destination)
  {
    current += step;
    return (current > destination) ? destination : current;
  }

  if (current > destination)
  {
    current -= step;
    return (current < destination) ? destination : current;
  }

  return current;
}

int percentOfPwm(int percent, int pwmLimit)
{
  percent = constrain(percent, 0, 100);
  pwmLimit = constrain(pwmLimit, 0, PWM_MAX);
  return (int)(((long)percent * pwmLimit) / 100L);
}

bool isDigitCharacter(char value)
{
  return value >= '0' && value <= '9';
}

bool isManualState(RobotState state)
{
  return state == MANUAL_FORWARD ||
         state == MANUAL_BACKWARD ||
         state == MANUAL_LEFT ||
         state == MANUAL_RIGHT;
}

bool bluetoothHasPriority(unsigned long now)
{
  return activeControl == CONTROL_BLUETOOTH &&
         !elapsedSince(now, lastBluetoothCommandAt, BLUETOOTH_PRIORITY_WINDOW_MS);
}

// -----------------------------------------------------------------------------
// Centralized L298N motor output
// -----------------------------------------------------------------------------

void writeMotorSide(byte enablePin, byte directionPinA, byte directionPinB, int value)
{
  value = constrain(value, -PWM_MAX, PWM_MAX);

  if (value > 0)
  {
    analogWrite(enablePin, value);
    digitalWrite(directionPinA, LOW);
    digitalWrite(directionPinB, HIGH);
  }
  else if (value < 0)
  {
    analogWrite(enablePin, -value);
    digitalWrite(directionPinA, HIGH);
    digitalWrite(directionPinB, LOW);
  }
  else
  {
    analogWrite(enablePin, 0);
    digitalWrite(directionPinA, LOW);
    digitalWrite(directionPinB, LOW);
  }
}

void writeAppliedMotorSpeeds()
{
  writeMotorSide(PIN_LEFT_ENABLE, PIN_LEFT_IN1, PIN_LEFT_IN2, motorAppliedLeft);
  writeMotorSide(PIN_RIGHT_ENABLE, PIN_RIGHT_IN3, PIN_RIGHT_IN4, motorAppliedRight);
}

void applyMotorSpeeds(int left, int right)
{
  motorTargetLeft = constrain(left, -PWM_MAX, PWM_MAX);
  motorTargetRight = constrain(right, -PWM_MAX, PWM_MAX);
  targetSpeed = maximumOf(magnitudeOf(motorTargetLeft), magnitudeOf(motorTargetRight));
}

void stopMotors()
{
  applyMotorSpeeds(0, 0);
}

void stopMotorsImmediately()
{
  motorTargetLeft = 0;
  motorTargetRight = 0;
  motorAppliedLeft = 0;
  motorAppliedRight = 0;
  targetSpeed = 0;
  currentSpeed = 0;
  writeAppliedMotorSpeeds();
}

void updateMotorController(unsigned long now)
{
  if (!elapsedSince(now, lastMotorUpdateAt, MOTOR_UPDATE_INTERVAL_MS))
  {
    return;
  }

  lastMotorUpdateAt = now;
  motorAppliedLeft = moveToward(motorAppliedLeft, motorTargetLeft, MOTOR_RAMP_STEP);
  motorAppliedRight = moveToward(motorAppliedRight, motorTargetRight, MOTOR_RAMP_STEP);
  currentSpeed = maximumOf(magnitudeOf(motorAppliedLeft), magnitudeOf(motorAppliedRight));
  writeAppliedMotorSpeeds();
}

void requestForward(int speed)
{
  speed = constrain(speed, 0, PWM_MAX);
  applyMotorSpeeds(speed, speed);
}

void requestBackward(int speed)
{
  speed = constrain(speed, 0, PWM_MAX);
  applyMotorSpeeds(-speed, -speed);
}

void requestPivotLeft(int speed)
{
  speed = constrain(speed, 0, PWM_MAX);
  applyMotorSpeeds(-speed, speed);
}

void requestPivotRight(int speed)
{
  speed = constrain(speed, 0, PWM_MAX);
  applyMotorSpeeds(speed, -speed);
}

void requestDriftLeft(int speed)
{
  speed = constrain(speed, 0, PWM_MAX);
  applyMotorSpeeds((speed * 60) / 100, speed);
}

void requestDriftRight(int speed)
{
  speed = constrain(speed, 0, PWM_MAX);
  applyMotorSpeeds(speed, (speed * 60) / 100);
}

void requestDonut(int speed)
{
  requestPivotRight(speed);
}

// -----------------------------------------------------------------------------
// Servo control
// -----------------------------------------------------------------------------

void lookAngle(int angle)
{
  angle = constrain(angle, SERVO_MIN, SERVO_MAX);

  if (angle == servoAngle)
  {
    return;
  }

  scannerServo.write(angle);
  servoAngle = angle;
  lastServoMoveAt = millis();
}

void lookCenter()
{
  lookAngle(SERVO_CENTER);
}

void lookLeft()
{
  lookAngle(SERVO_LEFT);
}

void lookRight()
{
  lookAngle(SERVO_RIGHT);
}

// -----------------------------------------------------------------------------
// Non-blocking HC-SR04 service and median distance cache
// -----------------------------------------------------------------------------

void sortThree(int &first, int &second, int &third)
{
  int temporary;

  if (first > second)
  {
    temporary = first;
    first = second;
    second = temporary;
  }

  if (second > third)
  {
    temporary = second;
    second = third;
    third = temporary;
  }

  if (first > second)
  {
    temporary = first;
    first = second;
    second = temporary;
  }
}

int medianDistanceSample()
{
  int first = distanceSamples[0];
  int second = distanceSamples[1];
  int third = distanceSamples[2];
  sortThree(first, second, third);
  return second;
}

void finishUltrasonicSample(int centimetres)
{
  centimetres = constrain(centimetres, 0, NO_ECHO_DISTANCE_CM);
  distanceSamples[distanceSampleWriteIndex] = centimetres;
  distanceSampleWriteIndex = (distanceSampleWriteIndex + 1) % 3;

  if (distanceSampleCount < 3)
  {
    distanceSampleCount++;
  }

  if (distanceSampleCount == 3)
  {
    currentDistance = medianDistanceSample();
  }
  else
  {
    currentDistance = centimetres;
  }

  distanceUpdatedAt = millis();
  echoState = ECHO_IDLE;
  nextPingAt = distanceUpdatedAt + ULTRASONIC_PING_INTERVAL_MS;
}

void beginUltrasonicPing()
{
  digitalWrite(PIN_ULTRASONIC_TRIGGER, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_ULTRASONIC_TRIGGER, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_ULTRASONIC_TRIGGER, LOW);

  echoStateStartedAt = micros();
  echoState = ECHO_WAIT_FOR_RISE;
}

void updateDistanceSensor(unsigned long now)
{
  unsigned long nowMicros = micros();

  if (echoState == ECHO_IDLE)
  {
    if ((long)(now - nextPingAt) >= 0)
    {
      beginUltrasonicPing();
    }
    return;
  }

  if (echoState == ECHO_WAIT_FOR_RISE)
  {
    if (digitalRead(PIN_ULTRASONIC_ECHO) == HIGH)
    {
      echoRiseAt = nowMicros;
      echoState = ECHO_WAIT_FOR_FALL;
    }
    else if ((unsigned long)(nowMicros - echoStateStartedAt) >= ULTRASONIC_ECHO_TIMEOUT_US)
    {
      finishUltrasonicSample(NO_ECHO_DISTANCE_CM);
    }
    return;
  }

  if (digitalRead(PIN_ULTRASONIC_ECHO) == LOW)
  {
    unsigned long pulseWidth = nowMicros - echoRiseAt;
    int centimetres = (int)((pulseWidth * 343UL) / 20000UL);
    finishUltrasonicSample(centimetres);
  }
  else if ((unsigned long)(nowMicros - echoRiseAt) >= ULTRASONIC_ECHO_TIMEOUT_US)
  {
    finishUltrasonicSample(NO_ECHO_DISTANCE_CM);
  }
}

bool forwardPathClear()
{
  return currentDistance > STOP_DISTANCE_CM;
}

// -----------------------------------------------------------------------------
// State transitions and common manual-drive helpers
// -----------------------------------------------------------------------------

void clearManualDrive()
{
  manualThrottlePercent = 0;
  manualSteeringPercent = 0;
  manualPwmLimit = cruiseSpeed;
}

void enterState(RobotState nextState, ControlSource source)
{
  robotState = nextState;
  activeControl = source;
  modePhase = 0;
  stateEnteredAt = millis();
  phaseEnteredAt = stateEnteredAt;
  stateDeadlineAt = 0;
}

RobotState manualStateFor(int throttlePercent, int steeringPercent)
{
  if (throttlePercent > 0)
  {
    return (steeringPercent < 0) ? MANUAL_LEFT :
           (steeringPercent > 0) ? MANUAL_RIGHT : MANUAL_FORWARD;
  }

  if (throttlePercent < 0)
  {
    return (steeringPercent < 0) ? MANUAL_LEFT :
           (steeringPercent > 0) ? MANUAL_RIGHT : MANUAL_BACKWARD;
  }

  return (steeringPercent < 0) ? MANUAL_LEFT :
         (steeringPercent > 0) ? MANUAL_RIGHT : IDLE;
}

void startManualArcade(int throttlePercent, int steeringPercent, int pwmLimit,
                       ControlSource source)
{
  manualThrottlePercent = constrain(throttlePercent, -100, 100);
  manualSteeringPercent = constrain(steeringPercent, -100, 100);
  manualPwmLimit = constrain(pwmLimit, 0, PWM_MAX);

  RobotState requestedState = manualStateFor(manualThrottlePercent, manualSteeringPercent);

  if (requestedState == IDLE)
  {
    clearManualDrive();
    enterState(BRAKING, source);
    return;
  }

  enterState(requestedState, source);
}

void requestManualArcadeMotion()
{
  int throttle = (manualThrottlePercent * manualPwmLimit) / 100;
  int steering;

  if (manualThrottlePercent == 0)
  {
    steering = (manualSteeringPercent * manualPwmLimit) / 100;
  }
  else
  {
    steering = (manualSteeringPercent * magnitudeOf(throttle)) / 100;
  }

  applyMotorSpeeds(throttle + steering, throttle - steering);
}

void startAutoForward(ControlSource source)
{
  clearManualDrive();
  enterState(AUTO_FORWARD, source);
}

void startAutoBackward(ControlSource source)
{
  clearManualDrive();
  enterState(AUTO_BACKWARD, source);
}

void startDriftLeft(ControlSource source)
{
  clearManualDrive();
  enterState(DRIFT_LEFT, source);
}

void startDriftRight(ControlSource source)
{
  clearManualDrive();
  enterState(DRIFT_RIGHT, source);
}

void startDonut(ControlSource source)
{
  clearManualDrive();
  enterState(DONUT, source);
}

void startDance(ControlSource source)
{
  clearManualDrive();
  enterState(DANCE, source);
}

void startDemo(ControlSource source)
{
  clearManualDrive();
  enterState(DEMO, source);
}

void startZigzag(ControlSource source)
{
  clearManualDrive();
  enterState(ZIGZAG, source);
}

void startTurn180(ControlSource source)
{
  clearManualDrive();
  enterState(TURN180, source);
}

void startBrake(ControlSource source)
{
  clearManualDrive();
  enterState(BRAKING, source);
}

void startSearch(ControlSource source)
{
  clearManualDrive();
  enterState(SEARCH, source);
  searchPhase = SEARCH_DRIVE_FORWARD;
  searchScanIndex = 0;
  searchTargetAngle = SERVO_CENTER;
  searchBestGapStart = -1;
  searchBestGapEnd = -1;
  searchBestGapSize = 0;
  searchBestGapScore = 0;
  lookCenter();
}

void emergencyStop()
{
  clearManualDrive();
  robotState = IDLE;
  activeControl = CONTROL_NONE;
  modePhase = 0;
  stateDeadlineAt = 0;
  searchPhase = SEARCH_DRIVE_FORWARD;
  stopMotorsImmediately();
  lookCenter();
  DEBUG_LINE("Emergency stop");
}

void setCruiseSpeed(int speed)
{
  cruiseSpeed = constrain(speed, 0, PWM_MAX);
}

void setSpeedPreset(byte preset)
{
  switch (preset)
  {
    case 1:
      setCruiseSpeed(percentOfPwm(25, PWM_MAX));
      break;

    case 2:
      setCruiseSpeed(percentOfPwm(50, PWM_MAX));
      break;

    case 3:
      setCruiseSpeed(percentOfPwm(75, PWM_MAX));
      break;

    case 4:
      setCruiseSpeed(SPEED_FULL);
      break;

    default:
      break;
  }
}

// -----------------------------------------------------------------------------
// Autonomous behavior state handlers
// -----------------------------------------------------------------------------

void updateDance(unsigned long now)
{
  if (elapsedSince(now, phaseEnteredAt, DANCE_STEP_TIME_MS))
  {
    modePhase = (modePhase + 1) % 8;
    phaseEnteredAt = now;
  }

  switch (modePhase)
  {
    case 0:
      requestForward(SPEED_DANCE);
      break;

    case 1:
      requestBackward(SPEED_DANCE);
      break;

    case 2:
      requestPivotLeft(SPEED_DANCE);
      break;

    case 3:
      requestPivotRight(SPEED_DANCE);
      break;

    case 4:
      requestDonut(SPEED_DANCE);
      break;

    case 5:
      requestForward(cruiseSpeed);
      break;

    case 6:
      requestPivotLeft(SPEED_DANCE);
      break;

    default:
      requestPivotRight(SPEED_DANCE);
      break;
  }
}

void advanceDemoPhase(unsigned long now)
{
  modePhase++;
  phaseEnteredAt = now;
}

void updateDemo(unsigned long now)
{
  switch (modePhase)
  {
    case 0:
      requestForward(cruiseSpeed);
      if (elapsedSince(now, phaseEnteredAt, DEMO_FORWARD_TIME_MS))
      {
        advanceDemoPhase(now);
      }
      break;

    case 1:
      stopMotors();
      if (elapsedSince(now, phaseEnteredAt, DEMO_PAUSE_TIME_MS))
      {
        advanceDemoPhase(now);
      }
      break;

    case 2:
      requestBackward(cruiseSpeed);
      if (elapsedSince(now, phaseEnteredAt, DEMO_REVERSE_TIME_MS))
      {
        advanceDemoPhase(now);
      }
      break;

    case 3:
      requestDonut(cruiseSpeed);
      if (elapsedSince(now, phaseEnteredAt, DEMO_DONUT_TIME_MS))
      {
        advanceDemoPhase(now);
      }
      break;

    case 4:
      requestPivotRight(cruiseSpeed);
      if (elapsedSince(now, phaseEnteredAt, TURN_180_TIME_MS))
      {
        advanceDemoPhase(now);
      }
      break;

    default:
      requestForward(cruiseSpeed);
      if (elapsedSince(now, phaseEnteredAt, DEMO_LAUNCH_TIME_MS))
      {
        clearManualDrive();
        enterState(IDLE, activeControl);
      }
      break;
  }
}

void startZigzagEscape(unsigned long now)
{
  modePhase = 4;
  phaseEnteredAt = now;
}

void updateZigzag(unsigned long now)
{
  switch (modePhase)
  {
    case 0:
      if (!forwardPathClear())
      {
        startZigzagEscape(now);
      }
      else
      {
        requestForward(cruiseSpeed);
        if (elapsedSince(now, phaseEnteredAt, ZIGZAG_FORWARD_TIME_MS))
        {
          advanceDemoPhase(now);
        }
      }
      break;

    case 1:
      requestPivotLeft(cruiseSpeed);
      if (elapsedSince(now, phaseEnteredAt, ZIGZAG_TURN_TIME_MS))
      {
        advanceDemoPhase(now);
      }
      break;

    case 2:
      if (!forwardPathClear())
      {
        startZigzagEscape(now);
      }
      else
      {
        requestForward(cruiseSpeed);
        if (elapsedSince(now, phaseEnteredAt, ZIGZAG_FORWARD_TIME_MS))
        {
          advanceDemoPhase(now);
        }
      }
      break;

    case 3:
      requestPivotRight(cruiseSpeed);
      if (elapsedSince(now, phaseEnteredAt, ZIGZAG_TURN_TIME_MS))
      {
        modePhase = 0;
        phaseEnteredAt = now;
      }
      break;

    case 4:
      requestBackward(cruiseSpeed);
      if (elapsedSince(now, phaseEnteredAt, ZIGZAG_REVERSE_TIME_MS))
      {
        advanceDemoPhase(now);
      }
      break;

    default:
      requestPivotRight(cruiseSpeed);
      if (elapsedSince(now, phaseEnteredAt, ZIGZAG_ESCAPE_TURN_TIME_MS))
      {
        modePhase = 0;
        phaseEnteredAt = now;
      }
      break;
  }
}

void resetSearchGap()
{
  searchBestGapStart = -1;
  searchBestGapEnd = -1;
  searchBestGapSize = 0;
  searchBestGapScore = 0;
}

void evaluateSearchGap(int startIndex, int endIndex)
{
  int sampleCount = endIndex - startIndex + 1;
  int totalDistance = 0;

  for (int index = startIndex; index <= endIndex; index++)
  {
    totalDistance += searchDistances[index];
  }

  int averageDistance = totalDistance / sampleCount;
  int score = averageDistance + sampleCount * 10;

  if (score > searchBestGapScore)
  {
    searchBestGapScore = score;
    searchBestGapStart = startIndex;
    searchBestGapEnd = endIndex;
    searchBestGapSize = sampleCount;
  }
}

void chooseBestSearchGap()
{
  resetSearchGap();
  int openStart = -1;

  for (byte index = 0; index <= SEARCH_SCAN_COUNT; index++)
  {
    bool isOpen = index < SEARCH_SCAN_COUNT &&
                  searchDistances[index] >= SAFE_DISTANCE_CM;

    if (isOpen && openStart < 0)
    {
      openStart = index;
    }
    else if (!isOpen && openStart >= 0)
    {
      evaluateSearchGap(openStart, index - 1);
      openStart = -1;
    }
  }
}

void beginSearchTurn(unsigned long now)
{
  if (searchBestGapSize >= SEARCH_MIN_GAP_SAMPLES)
  {
    int centerIndex = (searchBestGapStart + searchBestGapEnd) / 2;
    searchTargetAngle = SERVO_MIN + centerIndex * SEARCH_SCAN_STEP_DEGREES;
    int angleError = searchTargetAngle - SERVO_CENTER;
    unsigned long turnTime = (unsigned long)magnitudeOf(angleError) * SEARCH_TURN_PER_DEGREE_MS;

    if (turnTime < SEARCH_MIN_TURN_TIME_MS)
    {
      turnTime = SEARCH_MIN_TURN_TIME_MS;
    }

    stateDeadlineAt = now + turnTime;
  }
  else
  {
    searchTargetAngle = SERVO_CENTER;
    stateDeadlineAt = now + SEARCH_FALLBACK_TURN_TIME_MS;
  }

  searchPhase = SEARCH_TURN_TO_GAP;
  phaseEnteredAt = now;
}

void startNextSearchMeasurement(unsigned long now)
{
  int scanAngle = SERVO_MIN + searchScanIndex * SEARCH_SCAN_STEP_DEGREES;
  lookAngle(scanAngle);
  searchPhase = SEARCH_WAIT_FOR_SAMPLE;
  phaseEnteredAt = now;
}

void updateSearch(unsigned long now)
{
  switch (searchPhase)
  {
    case SEARCH_DRIVE_FORWARD:
      lookCenter();
      if (forwardPathClear())
      {
        requestForward(cruiseSpeed);
      }
      else
      {
        searchPhase = SEARCH_REVERSE;
        phaseEnteredAt = now;
      }
      break;

    case SEARCH_REVERSE:
      requestBackward(cruiseSpeed);
      if (elapsedSince(now, phaseEnteredAt, SEARCH_REVERSE_TIME_MS))
      {
        stopMotors();
        searchScanIndex = 0;
        startNextSearchMeasurement(now);
      }
      break;

    case SEARCH_WAIT_FOR_SAMPLE:
      stopMotors();
      if (elapsedSince(now, lastServoMoveAt, SERVO_SETTLE_TIME_MS) &&
          (long)(distanceUpdatedAt - lastServoMoveAt) >= 0)
      {
        searchDistances[searchScanIndex] = currentDistance;
        searchScanIndex++;

        if (searchScanIndex >= SEARCH_SCAN_COUNT)
        {
          searchPhase = SEARCH_CHOOSE_GAP;
          phaseEnteredAt = now;
        }
        else
        {
          startNextSearchMeasurement(now);
        }
      }
      break;

    case SEARCH_CHOOSE_GAP:
      stopMotors();
      chooseBestSearchGap();
      beginSearchTurn(now);
      break;

    default:
      if (searchBestGapSize >= SEARCH_MIN_GAP_SAMPLES)
      {
        if (searchTargetAngle < SERVO_CENTER - 5)
        {
          requestPivotRight(cruiseSpeed);
        }
        else if (searchTargetAngle > SERVO_CENTER + 5)
        {
          requestPivotLeft(cruiseSpeed);
        }
        else
        {
          stopMotors();
        }
      }
      else
      {
        requestPivotRight(cruiseSpeed);
      }

      if ((long)(now - stateDeadlineAt) >= 0)
      {
        stopMotors();
        lookCenter();
        searchPhase = SEARCH_DRIVE_FORWARD;
        phaseEnteredAt = now;
      }
      break;
  }
}

// -----------------------------------------------------------------------------
// Main RobotState dispatcher
// -----------------------------------------------------------------------------

void updateRobotState(unsigned long now)
{
  switch (robotState)
  {
    case IDLE:
      stopMotors();
      break;

    case MANUAL_FORWARD:
    case MANUAL_BACKWARD:
    case MANUAL_LEFT:
    case MANUAL_RIGHT:
      if (manualThrottlePercent > 0 && !forwardPathClear())
      {
        stopMotors();
      }
      else
      {
        requestManualArcadeMotion();
      }

      if (stateDeadlineAt != 0 && (long)(now - stateDeadlineAt) >= 0)
      {
        clearManualDrive();
        enterState(BRAKING, activeControl);
      }
      break;

    case AUTO_FORWARD:
      if (forwardPathClear())
      {
        requestForward(cruiseSpeed);
      }
      else
      {
        stopMotors();
      }
      break;

    case AUTO_BACKWARD:
      if (forwardPathClear())
      {
        requestBackward(cruiseSpeed);
      }
      else
      {
        stopMotors();
      }
      break;

    case SEARCH:
      updateSearch(now);
      break;

    case DRIFT_LEFT:
      if (forwardPathClear())
      {
        requestDriftLeft(cruiseSpeed);
      }
      else
      {
        stopMotors();
      }
      break;

    case DRIFT_RIGHT:
      if (forwardPathClear())
      {
        requestDriftRight(cruiseSpeed);
      }
      else
      {
        stopMotors();
      }
      break;

    case DONUT:
      requestDonut(cruiseSpeed);
      break;

    case DANCE:
      updateDance(now);
      break;

    case DEMO:
      updateDemo(now);
      break;

    case ZIGZAG:
      updateZigzag(now);
      break;

    case TURN180:
      requestPivotRight(cruiseSpeed);
      if (elapsedSince(now, stateEnteredAt, TURN_180_TIME_MS))
      {
        enterState(BRAKING, activeControl);
      }
      break;

    case BRAKING:
      stopMotors();
      if (motorAppliedLeft == 0 && motorAppliedRight == 0)
      {
        enterState(IDLE, activeControl);
      }
      break;
  }
}

// -----------------------------------------------------------------------------
// Bluetooth command processing
// -----------------------------------------------------------------------------

void acceptBluetoothCommand(unsigned long now)
{
  activeControl = CONTROL_BLUETOOTH;
  lastBluetoothCommandAt = now;
}

void handleBluetoothPacket(unsigned long now)
{
  int throttlePercent = (bluetoothPacket[1] - '0') * 10 + (bluetoothPacket[2] - '0');
  int steeringPercent = (bluetoothPacket[4] - '0') * 10 + (bluetoothPacket[5] - '0');

  if (bluetoothPacket[0] == 'B')
  {
    throttlePercent = -throttlePercent;
  }

  if (bluetoothPacket[3] == 'L')
  {
    steeringPercent = -steeringPercent;
  }

  acceptBluetoothCommand(now);
  startManualArcade(throttlePercent, steeringPercent, SPEED_FULL, CONTROL_BLUETOOTH);
}

bool handleBluetoothSingleCommand(char command, unsigned long now)
{
  switch (command)
  {
    case 'F':
      acceptBluetoothCommand(now);
      startManualArcade(100, 0, cruiseSpeed, CONTROL_BLUETOOTH);
      return true;

    case 'B':
      acceptBluetoothCommand(now);
      startManualArcade(-100, 0, cruiseSpeed, CONTROL_BLUETOOTH);
      return true;

    case 'L':
      acceptBluetoothCommand(now);
      startManualArcade(100, -55, cruiseSpeed, CONTROL_BLUETOOTH);
      return true;

    case 'R':
      acceptBluetoothCommand(now);
      startManualArcade(100, 55, cruiseSpeed, CONTROL_BLUETOOTH);
      return true;

    case 'G':
      acceptBluetoothCommand(now);
      startManualArcade(100, -35, cruiseSpeed, CONTROL_BLUETOOTH);
      return true;

    case 'I':
      acceptBluetoothCommand(now);
      startManualArcade(100, 35, cruiseSpeed, CONTROL_BLUETOOTH);
      return true;

    case 'H':
      acceptBluetoothCommand(now);
      startManualArcade(-100, -35, cruiseSpeed, CONTROL_BLUETOOTH);
      return true;

    case 'J':
      acceptBluetoothCommand(now);
      startManualArcade(-100, 35, cruiseSpeed, CONTROL_BLUETOOTH);
      return true;

    case 'S':
      acceptBluetoothCommand(now);
      emergencyStop();
      return true;

    case '1':
    case '2':
    case '3':
    case '4':
      acceptBluetoothCommand(now);
      setSpeedPreset(command - '0');
      return true;

    case 'Y':
    case 'A':
      acceptBluetoothCommand(now);
      startAutoForward(CONTROL_BLUETOOTH);
      return true;

    case 'U':
    case 'X':
      acceptBluetoothCommand(now);
      startAutoBackward(CONTROL_BLUETOOTH);
      return true;

    case 'u':
    case 'O':
      acceptBluetoothCommand(now);
      startSearch(CONTROL_BLUETOOTH);
      return true;

    case 'Q':
      acceptBluetoothCommand(now);
      setCruiseSpeed(SPEED_SLOW);
      return true;

    case 'W':
      acceptBluetoothCommand(now);
      setCruiseSpeed(SPEED_FULL);
      return true;

    case 'D':
      acceptBluetoothCommand(now);
      startDonut(CONTROL_BLUETOOTH);
      return true;

    case 'T':
      acceptBluetoothCommand(now);
      startTurn180(CONTROL_BLUETOOTH);
      return true;

    case 'K':
      acceptBluetoothCommand(now);
      startBrake(CONTROL_BLUETOOTH);
      return true;

    case 'N':
      acceptBluetoothCommand(now);
      startDance(CONTROL_BLUETOOTH);
      return true;

    case 'P':
      acceptBluetoothCommand(now);
      startDemo(CONTROL_BLUETOOTH);
      return true;

    case 'Z':
      acceptBluetoothCommand(now);
      startZigzag(CONTROL_BLUETOOTH);
      return true;

    case 'M':
      acceptBluetoothCommand(now);
      startDriftLeft(CONTROL_BLUETOOTH);
      return true;

    case 'V':
      acceptBluetoothCommand(now);
      startDriftRight(CONTROL_BLUETOOTH);
      return true;

    default:
      return false;
  }
}

void resetBluetoothPacket()
{
  bluetoothPacketLength = 0;
  bluetoothPacketStartedAt = 0;
}

bool validPacketCharacterAt(char character, byte position)
{
  switch (position)
  {
    case 0:
      return character == 'F' || character == 'B';

    case 1:
    case 2:
    case 4:
    case 5:
      return isDigitCharacter(character);

    case 3:
      return character == 'L' || character == 'R';

    default:
      return false;
  }
}

void processBluetoothByte(char character, unsigned long now)
{
  if (character == '\r' || character == '\n' || character == ' ' || character == '\t')
  {
    if (bluetoothPacketLength == 1)
    {
      handleBluetoothSingleCommand(bluetoothPacket[0], now);
    }
    resetBluetoothPacket();
    return;
  }

  if (bluetoothPacketLength == 0)
  {
    if (character == 'F' || character == 'B')
    {
      bluetoothPacket[0] = character;
      bluetoothPacketLength = 1;
      bluetoothPacketStartedAt = now;
    }
    else
    {
      handleBluetoothSingleCommand(character, now);
    }
    return;
  }

  if (validPacketCharacterAt(character, bluetoothPacketLength))
  {
    bluetoothPacket[bluetoothPacketLength] = character;
    bluetoothPacketLength++;

    if (bluetoothPacketLength == 6)
    {
      handleBluetoothPacket(now);
      resetBluetoothPacket();
    }
    return;
  }

  if (bluetoothPacketLength == 1)
  {
    handleBluetoothSingleCommand(bluetoothPacket[0], now);
  }

  resetBluetoothPacket();
  processBluetoothByte(character, now);
}

void updateBluetooth(unsigned long now)
{
  while (bluetooth.available() > 0)
  {
    processBluetoothByte((char)bluetooth.read(), now);
  }

  if (bluetoothPacketLength > 0 &&
      elapsedSince(now, bluetoothPacketStartedAt, BLUETOOTH_PACKET_WAIT_MS))
  {
    if (bluetoothPacketLength == 1)
    {
      handleBluetoothSingleCommand(bluetoothPacket[0], now);
    }
    resetBluetoothPacket();
  }
}

// -----------------------------------------------------------------------------
// IR remote service
// -----------------------------------------------------------------------------

void acceptIrCommand(unsigned long now)
{
  activeControl = CONTROL_IR;
  lastIrCommandAt = now;
}

void startIrTimedTurn(bool turnLeft, unsigned long now)
{
  startManualArcade(0, turnLeft ? -100 : 100, cruiseSpeed, CONTROL_IR);
  stateDeadlineAt = now + IR_TURN_PULSE_TIME_MS;
}

void handleIrCommand(byte command, unsigned long now)
{
  if (command == IR_OK)
  {
    emergencyStop();
    return;
  }

  if (bluetoothHasPriority(now))
  {
    return;
  }

  switch (command)
  {
    case IR_2:
      acceptIrCommand(now);
      setCruiseSpeed((cruiseSpeed == SPEED_SLOW) ? SPEED_FULL : SPEED_SLOW);
      break;

    case IR_UP:
      acceptIrCommand(now);
      startManualArcade(100, 0, cruiseSpeed, CONTROL_IR);
      break;

    case IR_DOWN:
      acceptIrCommand(now);
      startManualArcade(-100, 0, cruiseSpeed, CONTROL_IR);
      break;

    case IR_LEFT:
      acceptIrCommand(now);
      startIrTimedTurn(true, now);
      break;

    case IR_RIGHT:
      acceptIrCommand(now);
      startIrTimedTurn(false, now);
      break;

    case IR_STAR:
      acceptIrCommand(now);
      startAutoForward(CONTROL_IR);
      break;

    case IR_HASH:
      acceptIrCommand(now);
      startAutoBackward(CONTROL_IR);
      break;

    case IR_0:
      acceptIrCommand(now);
      startDonut(CONTROL_IR);
      break;

    case IR_1:
      acceptIrCommand(now);
      startTurn180(CONTROL_IR);
      break;

    case IR_3:
      acceptIrCommand(now);
      startBrake(CONTROL_IR);
      break;

    case IR_4:
      acceptIrCommand(now);
      startDemo(CONTROL_IR);
      break;

    case IR_5:
      acceptIrCommand(now);
      startDance(CONTROL_IR);
      break;

    case IR_6:
      acceptIrCommand(now);
      startZigzag(CONTROL_IR);
      break;

    case IR_7:
      acceptIrCommand(now);
      startDriftLeft(CONTROL_IR);
      break;

    case IR_8:
      acceptIrCommand(now);
      startDriftRight(CONTROL_IR);
      break;

    case IR_9:
      acceptIrCommand(now);
      startSearch(CONTROL_IR);
      break;

    default:
      break;
  }
}

void updateIrReceiver(unsigned long now)
{
  if (!IrReceiver.decode())
  {
    return;
  }

  byte command = IrReceiver.decodedIRData.command;
  handleIrCommand(command, now);
  IrReceiver.resume();
}

// -----------------------------------------------------------------------------
// Control watchdogs
// -----------------------------------------------------------------------------

void updateControlWatchdogs(unsigned long now)
{
  if (!isManualState(robotState))
  {
    return;
  }

  if (activeControl == CONTROL_BLUETOOTH &&
      elapsedSince(now, lastBluetoothCommandAt, BLUETOOTH_MANUAL_TIMEOUT_MS))
  {
    emergencyStop();
    return;
  }

  if (activeControl == CONTROL_IR &&
      elapsedSince(now, lastIrCommandAt, IR_MANUAL_TIMEOUT_MS))
  {
    emergencyStop();
  }
}

// -----------------------------------------------------------------------------
// Arduino setup and scheduler
// -----------------------------------------------------------------------------

void setup()
{
  Serial.begin(115200);
  bluetooth.begin(9600);

  pinMode(PIN_LEFT_ENABLE, OUTPUT);
  pinMode(PIN_RIGHT_ENABLE, OUTPUT);
  pinMode(PIN_LEFT_IN1, OUTPUT);
  pinMode(PIN_LEFT_IN2, OUTPUT);
  pinMode(PIN_RIGHT_IN3, OUTPUT);
  pinMode(PIN_RIGHT_IN4, OUTPUT);

  pinMode(PIN_ULTRASONIC_TRIGGER, OUTPUT);
  pinMode(PIN_ULTRASONIC_ECHO, INPUT);
  digitalWrite(PIN_ULTRASONIC_TRIGGER, LOW);

  scannerServo.attach(PIN_SERVO);
  scannerServo.write(SERVO_CENTER);
  servoAngle = SERVO_CENTER;
  lastServoMoveAt = millis();

  IrReceiver.begin(PIN_IR_RECEIVER, ENABLE_LED_FEEDBACK);

  stopMotorsImmediately();
  nextPingAt = millis();
  lastMotorUpdateAt = millis();

  DEBUG_LINE("ASURA V7 FINAL ready");
}

void loop()
{
  unsigned long now = millis();

  updateBluetooth(now);
  updateIrReceiver(now);
  updateDistanceSensor(now);
  updateControlWatchdogs(now);
  updateRobotState(now);
  updateMotorController(now);
}
