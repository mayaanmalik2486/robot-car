#include <Arduino.h>
#include <Servo.h>

// ============================================================================
// Hardware configuration
// ============================================================================

const uint8_t LEFT_MOTOR_IN1_PIN = 2;
const uint8_t LEFT_MOTOR_IN2_PIN = 3;
const uint8_t RIGHT_MOTOR_IN1_PIN = 4;
const uint8_t RIGHT_MOTOR_IN2_PIN = 5;

const uint8_t LEFT_LINE_SENSOR_PIN = 6;
const uint8_t RIGHT_LINE_SENSOR_PIN = 7;
const uint8_t IR_RECEIVER_PIN = 8;
const uint8_t SERVO_PIN = 9;
const uint8_t ULTRASONIC_TRIG_PIN = 11;
const uint8_t ULTRASONIC_ECHO_PIN = 12;

// ============================================================================
// Tuning configuration
// ============================================================================

const bool DEBUG = false;

// The L298N ENA and ENB jumpers are installed.  The driver therefore runs at
// full electrical power whenever a direction is commanded; these nominal
// values describe the requested operating point for diagnostics and preserve
// a clean interface for a future PWM-capable motor driver.
const uint8_t DRIVE_SPEED = 255;
const uint8_t REVERSE_SPEED = 210;
const uint8_t TURN_SPEED = 220;

const uint8_t SERVO_MIN_ANGLE = 15;
const uint8_t SERVO_MAX_ANGLE = 165;
const uint8_t SERVO_CENTER_ANGLE = 90;
const uint8_t SERVO_STEP_DEGREES = 2;
const uint16_t SERVO_STEP_DELAY_MS = 11;

const uint16_t OBSTACLE_DISTANCE_CM = 30;
const uint16_t CRITICAL_DISTANCE_CM = 15;
const uint16_t NARROW_GAP_DISTANCE_CM = 35;
const uint16_t MAX_VALID_DISTANCE_CM = 100;
const uint16_t MIN_VALID_DISTANCE_CM = 2;

const uint16_t ULTRASONIC_TIMEOUT_US = 6000;
const uint8_t MEDIAN_SAMPLE_COUNT = 3;
const uint8_t FILTER_GROUP_COUNT = 3;
const uint8_t MIN_VALID_FILTER_GROUPS = 2;
const uint16_t ULTRASONIC_SAMPLE_GAP_MS = 3;

const uint16_t SCAN_DELAY_MS = 55;
const uint16_t FRONT_CHECK_INTERVAL_MS = 75;
const uint16_t STARTUP_SETTLE_TIME_MS = 500;
const uint16_t STOP_SETTLE_TIME_MS = 120;
const uint16_t RECOVERY_RETRY_DELAY_MS = 250;
const uint8_t MAX_SENSOR_RECOVERY_ATTEMPTS = 2;
const uint8_t MAX_CONSECUTIVE_INVALID_READINGS = 3;
const uint8_t MAX_INVALID_SCAN_READINGS = 3;

// No wheel encoders are installed.  Reverse distance is consequently
// represented by a calibrated time-per-centimeter estimate.
const uint16_t REVERSE_DISTANCE_CM = 20;
const uint16_t REVERSE_TIME_PER_CM_MS = 24;
const unsigned long REVERSE_TIME_MS =
  (unsigned long)REVERSE_DISTANCE_CM * REVERSE_TIME_PER_CM_MS;
const unsigned long DEAD_END_REVERSE_TIME_MS = REVERSE_TIME_MS + 180UL;

const unsigned long TURN_NINETY_TIME_MS = 450;
const unsigned long TURN_180_TIME_MS = 850;
const uint16_t MOTOR_DIRECTION_DELAY_MS = 80;
const uint8_t MOTOR_RAMP_STEP_COUNT = 2;
const uint16_t MOTOR_RAMP_DELAY_MS = 35;

const int8_t NO_DIRECTION = -1;
const uint8_t SCAN_DIRECTION_COUNT = 5;
const uint8_t SCAN_ANGLES[SCAN_DIRECTION_COUNT] = {15, 45, 90, 135, 165};

// Direction scoring weights.  Larger values represent stronger preferences.
const int16_t SCORE_DISTANCE_WEIGHT = 2;
const int16_t SCORE_FRONT_BONUS = 55;
const int16_t SCORE_NEAR_ANGLE_BONUS = 20;
const int16_t SCORE_SIDE_ANGLE_PENALTY = 8;
const int16_t SCORE_OPEN_NEIGHBOR_BONUS = 32;
const int16_t SCORE_CLOSED_NEIGHBOR_PENALTY = 28;
const int16_t SCORE_NARROW_PENALTY = 75;
const int16_t SCORE_CAUTION_PENALTY = 115;
const int16_t SCORE_CRITICAL_PENALTY = 800;
const int16_t SCORE_INVALID_PENALTY = 900;
const int16_t SCORE_REPEAT_TURN_PENALTY = 90;
const uint8_t MAX_REPEATED_TURNS = 3;

// ============================================================================
// Types and state
// ============================================================================

enum RobotState : uint8_t {
  STATE_STARTUP,
  STATE_DRIVE,
  STATE_STOP,
  STATE_REVERSE,
  STATE_SCAN,
  STATE_TURN,
  STATE_ESCAPE,
  STATE_RECOVER
};

enum MotionMode : uint8_t {
  MOTION_STOPPED,
  MOTION_FORWARD,
  MOTION_BACKWARD,
  MOTION_TURN_LEFT,
  MOTION_TURN_RIGHT,
  MOTION_SPIN_180
};

enum NavigationAction : uint8_t {
  ACTION_NONE,
  ACTION_AVOID_OBSTACLE
};

enum EscapePhase : uint8_t {
  ESCAPE_WAIT,
  ESCAPE_REVERSE,
  ESCAPE_TURN,
  ESCAPE_FINISH
};

struct ScanResult {
  uint16_t distanceCm[SCAN_DIRECTION_COUNT];
  bool valid[SCAN_DIRECTION_COUNT];
};

Servo scannerServo;
ScanResult environmentScan;

RobotState currentState = STATE_STARTUP;
MotionMode currentMotion = MOTION_STOPPED;
NavigationAction navigationAction = ACTION_NONE;
EscapePhase escapePhase = ESCAPE_WAIT;

uint8_t currentServoAngle = SERVO_CENTER_ANGLE;
uint8_t currentNominalMotorSpeed = 0;
uint16_t frontDistanceCm = 0;
uint8_t consecutiveInvalidReadings = 0;
uint8_t sensorRecoveryAttempts = 0;
uint8_t invalidScanReadings = 0;
int8_t selectedDirection = NO_DIRECTION;

bool stateEntryPending = true;
bool sensorFailureLatched = false;
int8_t lastTurnDirection = 0;
uint8_t repeatedTurnCount = 0;

unsigned long stateDeadline = 0;
unsigned long lastFrontCheckAt = 0;

// ============================================================================
// Forward declarations
// ============================================================================

void driveForward();
void driveBackward();
void stopMotors();
void turnLeft();
void turnRight();
void spin180();
void rampMotorSpeed(MotionMode targetMotion, uint8_t targetSpeed);

void moveServoSmooth(uint8_t targetAngle);
void lookCenter();
void centerServo();
void lookLeft();
void lookRight();
void lookAtAngle(uint8_t targetAngle);
void lookAt(uint8_t targetAngle);

uint16_t readDistance();
uint16_t rawDistance();
uint16_t filteredDistance();
uint16_t readFilteredDistance();

void scanEnvironment();
int8_t chooseBestDirection();
int8_t chooseDirection();
void reverseAndTurn();
void deadEndEscape();
void avoidObstacle();

void autonomousDrive();
void updateRobot();
void emergencyStop();

// ============================================================================
// Utility and diagnostics
// ============================================================================

const __FlashStringHelper* stateName(RobotState state) {
  switch (state) {
    case STATE_STARTUP: return F("STARTUP");
    case STATE_DRIVE: return F("DRIVE");
    case STATE_STOP: return F("STOP");
    case STATE_REVERSE: return F("REVERSE");
    case STATE_SCAN: return F("SCAN");
    case STATE_TURN: return F("TURN");
    case STATE_ESCAPE: return F("ESCAPE");
    case STATE_RECOVER: return F("RECOVER");
  }
  return F("UNKNOWN");
}

const __FlashStringHelper* motionName(MotionMode motion) {
  switch (motion) {
    case MOTION_STOPPED: return F("STOPPED");
    case MOTION_FORWARD: return F("FORWARD");
    case MOTION_BACKWARD: return F("BACKWARD");
    case MOTION_TURN_LEFT: return F("TURN_LEFT");
    case MOTION_TURN_RIGHT: return F("TURN_RIGHT");
    case MOTION_SPIN_180: return F("SPIN_180");
  }
  return F("UNKNOWN");
}

bool deadlineReached() {
  return (long)(millis() - stateDeadline) >= 0;
}

uint8_t clampServoAngle(int16_t angle) {
  if (angle < SERVO_MIN_ANGLE) {
    return SERVO_MIN_ANGLE;
  }
  if (angle > SERVO_MAX_ANGLE) {
    return SERVO_MAX_ANGLE;
  }
  return (uint8_t)angle;
}

uint16_t absoluteDifference(uint16_t first, uint16_t second) {
  return first > second ? first - second : second - first;
}

void debugPrintState() {
  if (!DEBUG) {
    return;
  }
  Serial.print(F("STATE: "));
  Serial.println(stateName(currentState));
}

void debugPrintServoAngle() {
  if (!DEBUG) {
    return;
  }
  Serial.print(F("Servo angle: "));
  Serial.println(currentServoAngle);
}

void debugPrintMotor() {
  if (!DEBUG) {
    return;
  }
  Serial.print(F("Motor mode: "));
  Serial.print(motionName(currentMotion));
  Serial.print(F(" nominal speed: "));
  Serial.println(currentNominalMotorSpeed);
}

void debugPrintDistance(uint16_t distanceCm, bool valid) {
  if (!DEBUG) {
    return;
  }
  Serial.print(F("Distance: "));
  if (valid) {
    Serial.print(distanceCm);
    Serial.println(F(" cm"));
  } else {
    Serial.println(F("invalid"));
  }
}

void debugPrintDecision(int8_t direction) {
  if (!DEBUG) {
    return;
  }
  Serial.print(F("Decision: "));
  if (direction == NO_DIRECTION) {
    Serial.println(F("dead end"));
    return;
  }
  Serial.print(F("angle "));
  Serial.print(SCAN_ANGLES[direction]);
  Serial.println(F(" degrees"));
}

void transitionTo(RobotState nextState) {
  currentState = nextState;
  stateDeadline = millis();
  stateEntryPending = true;
  debugPrintState();
}

// ============================================================================
// Motor driver
// ============================================================================

void writeMotorPins(bool leftEnabled, bool rightEnabled, MotionMode motion) {
  // First coast both sides so a direction change never changes polarity while
  // the old command is still present on the H-bridge inputs.
  digitalWrite(LEFT_MOTOR_IN1_PIN, LOW);
  digitalWrite(LEFT_MOTOR_IN2_PIN, LOW);
  digitalWrite(RIGHT_MOTOR_IN1_PIN, LOW);
  digitalWrite(RIGHT_MOTOR_IN2_PIN, LOW);

  if (leftEnabled) {
    switch (motion) {
      case MOTION_FORWARD:
      case MOTION_TURN_RIGHT:
        digitalWrite(LEFT_MOTOR_IN1_PIN, HIGH);
        digitalWrite(LEFT_MOTOR_IN2_PIN, LOW);
        break;
      case MOTION_BACKWARD:
      case MOTION_TURN_LEFT:
      case MOTION_SPIN_180:
        digitalWrite(LEFT_MOTOR_IN1_PIN, LOW);
        digitalWrite(LEFT_MOTOR_IN2_PIN, HIGH);
        break;
      default:
        break;
    }
  }

  if (rightEnabled) {
    // The right motors are wired blue-to-OUT3 and red-to-OUT4, opposite to
    // the left side.  These complementary logic levels compensate for that
    // intentional mirrored wiring.
    switch (motion) {
      case MOTION_FORWARD:
      case MOTION_TURN_LEFT:
        digitalWrite(RIGHT_MOTOR_IN1_PIN, LOW);
        digitalWrite(RIGHT_MOTOR_IN2_PIN, HIGH);
        break;
      case MOTION_BACKWARD:
      case MOTION_TURN_RIGHT:
      case MOTION_SPIN_180:
        digitalWrite(RIGHT_MOTOR_IN1_PIN, HIGH);
        digitalWrite(RIGHT_MOTOR_IN2_PIN, LOW);
        break;
      default:
        break;
    }
  }
}

void stopMotors() {
  writeMotorPins(false, false, MOTION_STOPPED);
  currentMotion = MOTION_STOPPED;
  currentNominalMotorSpeed = 0;
  debugPrintMotor();
}

void rampMotorSpeed(MotionMode targetMotion, uint8_t targetSpeed) {
  if (targetMotion == MOTION_STOPPED) {
    stopMotors();
    return;
  }

  if (currentMotion == targetMotion && currentNominalMotorSpeed == targetSpeed) {
    return;
  }

  if (currentMotion != MOTION_STOPPED) {
    stopMotors();
    delay(MOTOR_DIRECTION_DELAY_MS);
  }

  uint8_t steps = MOTOR_RAMP_STEP_COUNT == 0 ? 1 : MOTOR_RAMP_STEP_COUNT;
  for (uint8_t step = 1; step <= steps; ++step) {
    // Since ENA/ENB are permanently enabled, there is no legal speed setting
    // between stopped and full speed.  Staging the two sides limits the
    // simultaneous startup current while retaining full-speed operation.
    bool enableLeft = true;
    bool enableRight = steps == 1 || step >= 2;
    if (steps > 2 && step < steps) {
      enableRight = false;
    }
    writeMotorPins(enableLeft, enableRight, targetMotion);
    currentMotion = targetMotion;
    currentNominalMotorSpeed = (uint8_t)(((uint16_t)targetSpeed * step) / steps);
    debugPrintMotor();
    delay(MOTOR_RAMP_DELAY_MS);
  }
  currentNominalMotorSpeed = targetSpeed;
  debugPrintMotor();
}

void driveForward() {
  rampMotorSpeed(MOTION_FORWARD, DRIVE_SPEED);
}

void driveBackward() {
  rampMotorSpeed(MOTION_BACKWARD, REVERSE_SPEED);
}

void turnLeft() {
  rampMotorSpeed(MOTION_TURN_LEFT, TURN_SPEED);
}

void turnRight() {
  rampMotorSpeed(MOTION_TURN_RIGHT, TURN_SPEED);
}

void spin180() {
  rampMotorSpeed(MOTION_SPIN_180, TURN_SPEED);
}

// ============================================================================
// Servo scanner
// ============================================================================

void moveServoSmooth(uint8_t targetAngle) {
  targetAngle = clampServoAngle(targetAngle);
  if (targetAngle == currentServoAngle) {
    scannerServo.write(currentServoAngle);
    return;
  }

  int16_t angle = currentServoAngle;
  int16_t target = targetAngle;
  int16_t direction = target > angle ? SERVO_STEP_DEGREES : -SERVO_STEP_DEGREES;

  while (angle != target) {
    angle += direction;
    if ((direction > 0 && angle > target) || (direction < 0 && angle < target)) {
      angle = target;
    }
    currentServoAngle = clampServoAngle(angle);
    scannerServo.write(currentServoAngle);
    debugPrintServoAngle();
    delay(SERVO_STEP_DELAY_MS);
  }
}

void lookAtAngle(uint8_t targetAngle) {
  moveServoSmooth(targetAngle);
}

void lookAt(uint8_t targetAngle) {
  lookAtAngle(targetAngle);
}

void lookCenter() {
  lookAtAngle(SERVO_CENTER_ANGLE);
}

void centerServo() {
  lookCenter();
}

void lookLeft() {
  lookAtAngle(SERVO_MIN_ANGLE);
}

void lookRight() {
  lookAtAngle(SERVO_MAX_ANGLE);
}

// ============================================================================
// Ultrasonic sensor and filtering
// ============================================================================

uint16_t readDistance() {
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);

  unsigned long echoDuration = pulseIn(
    ULTRASONIC_ECHO_PIN,
    HIGH,
    ULTRASONIC_TIMEOUT_US
  );

  if (echoDuration == 0) {
    return 0;
  }

  uint16_t distanceCm = (uint16_t)(echoDuration / 58UL);
  if (distanceCm < MIN_VALID_DISTANCE_CM ||
      distanceCm > MAX_VALID_DISTANCE_CM) {
    return 0;
  }
  return distanceCm;
}

uint16_t rawDistance() {
  return readDistance();
}

uint16_t readMedianSample() {
  uint16_t samples[MEDIAN_SAMPLE_COUNT];
  for (uint8_t index = 0; index < MEDIAN_SAMPLE_COUNT; ++index) {
    samples[index] = rawDistance();
    if (index + 1 < MEDIAN_SAMPLE_COUNT) {
      delay(ULTRASONIC_SAMPLE_GAP_MS);
    }
  }

  for (uint8_t pass = 0; pass < MEDIAN_SAMPLE_COUNT - 1; ++pass) {
    for (uint8_t index = 0; index < MEDIAN_SAMPLE_COUNT - pass - 1; ++index) {
      if (samples[index] > samples[index + 1]) {
        uint16_t temporary = samples[index];
        samples[index] = samples[index + 1];
        samples[index + 1] = temporary;
      }
    }
  }

  uint16_t median = samples[MEDIAN_SAMPLE_COUNT / 2];
  if (median < MIN_VALID_DISTANCE_CM || median > MAX_VALID_DISTANCE_CM) {
    return 0;
  }
  return median;
}

uint16_t filteredDistance() {
  uint32_t totalDistance = 0;
  uint8_t validGroups = 0;

  for (uint8_t group = 0; group < FILTER_GROUP_COUNT; ++group) {
    uint16_t median = readMedianSample();
    if (median >= MIN_VALID_DISTANCE_CM && median <= MAX_VALID_DISTANCE_CM) {
      totalDistance += median;
      ++validGroups;
    }
  }

  if (validGroups < MIN_VALID_FILTER_GROUPS) {
    return 0;
  }
  return (uint16_t)(totalDistance / validGroups);
}

uint16_t readFilteredDistance() {
  return filteredDistance();
}

// ============================================================================
// Navigation scoring and scanning
// ============================================================================

bool isDirectionOpen(uint8_t directionIndex) {
  if (directionIndex >= SCAN_DIRECTION_COUNT ||
      !environmentScan.valid[directionIndex]) {
    return false;
  }
  return environmentScan.distanceCm[directionIndex] > OBSTACLE_DISTANCE_CM;
}

int16_t scoreDirection(uint8_t directionIndex) {
  if (directionIndex >= SCAN_DIRECTION_COUNT) {
    return -32768;
  }

  if (!environmentScan.valid[directionIndex]) {
    return -SCORE_INVALID_PENALTY;
  }

  uint16_t distance = environmentScan.distanceCm[directionIndex];
  int16_t score = (int16_t)distance * SCORE_DISTANCE_WEIGHT;

  if (distance <= CRITICAL_DISTANCE_CM) {
    score -= SCORE_CRITICAL_PENALTY;
  } else if (distance <= OBSTACLE_DISTANCE_CM) {
    score -= SCORE_CAUTION_PENALTY;
  }

  if (directionIndex == SCAN_DIRECTION_COUNT / 2 && isDirectionOpen(directionIndex)) {
    score += SCORE_FRONT_BONUS;
  }

  uint16_t angularOffset = absoluteDifference(
    SCAN_ANGLES[directionIndex],
    SERVO_CENTER_ANGLE
  );
  uint8_t offsetSteps = (uint8_t)(angularOffset / 45U);
  score += (int16_t)(SCORE_NEAR_ANGLE_BONUS -
                     (int16_t)offsetSteps * SCORE_SIDE_ANGLE_PENALTY);

  if (distance < NARROW_GAP_DISTANCE_CM) {
    score -= SCORE_NARROW_PENALTY;
  }

  for (int8_t neighborOffset = -1; neighborOffset <= 1; ++neighborOffset) {
    if (neighborOffset == 0) {
      continue;
    }
    int8_t neighbor = (int8_t)directionIndex + neighborOffset;
    if (neighbor < 0 || neighbor >= SCAN_DIRECTION_COUNT) {
      continue;
    }
    if (isDirectionOpen((uint8_t)neighbor)) {
      score += SCORE_OPEN_NEIGHBOR_BONUS;
    } else {
      score -= SCORE_CLOSED_NEIGHBOR_PENALTY;
    }
  }

  int8_t candidateTurnDirection = 0;
  if (directionIndex < SCAN_DIRECTION_COUNT / 2) {
    candidateTurnDirection = -1;
  } else if (directionIndex > SCAN_DIRECTION_COUNT / 2) {
    candidateTurnDirection = 1;
  }

  if (candidateTurnDirection != 0 &&
      candidateTurnDirection == lastTurnDirection &&
      repeatedTurnCount >= MAX_REPEATED_TURNS) {
    score -= SCORE_REPEAT_TURN_PENALTY;
  }

  return score;
}

void scanEnvironment() {
  invalidScanReadings = 0;

  for (uint8_t index = 0; index < SCAN_DIRECTION_COUNT; ++index) {
    if (index == 0) {
      lookLeft();
    } else if (index == SCAN_DIRECTION_COUNT - 1) {
      lookRight();
    } else {
      lookAt(SCAN_ANGLES[index]);
    }
    delay(SCAN_DELAY_MS);

    uint16_t distance = readFilteredDistance();
    environmentScan.distanceCm[index] = distance;
    environmentScan.valid[index] = distance != 0;
    if (!environmentScan.valid[index]) {
      ++invalidScanReadings;
    }
    debugPrintDistance(distance, environmentScan.valid[index]);
  }

  lookCenter();
}

int8_t chooseBestDirection() {
  int8_t bestDirection = NO_DIRECTION;
  int16_t bestScore = -32768;

  for (uint8_t index = 0; index < SCAN_DIRECTION_COUNT; ++index) {
    if (!isDirectionOpen(index)) {
      continue;
    }

    int16_t score = scoreDirection(index);
    if (bestDirection == NO_DIRECTION || score > bestScore) {
      bestDirection = (int8_t)index;
      bestScore = score;
    }
  }

  debugPrintDecision(bestDirection);
  return bestDirection;
}

int8_t chooseDirection() {
  return chooseBestDirection();
}

// ============================================================================
// Navigation actions
// ============================================================================

void reverseAndTurn() {
  navigationAction = ACTION_AVOID_OBSTACLE;
  stopMotors();
  transitionTo(STATE_STOP);
  stateDeadline = millis() + STOP_SETTLE_TIME_MS;
}

void avoidObstacle() {
  if (currentState != STATE_DRIVE) {
    return;
  }
  reverseAndTurn();
}

void deadEndEscape() {
  stopMotors();
  escapePhase = ESCAPE_WAIT;
  transitionTo(STATE_ESCAPE);
  stateDeadline = millis() + STOP_SETTLE_TIME_MS;
}

// ============================================================================
// Safety and state handlers
// ============================================================================

void emergencyStop() {
  stopMotors();
  centerServo();
  sensorRecoveryAttempts = 0;
  sensorFailureLatched = false;
  transitionTo(STATE_RECOVER);
  stateDeadline = millis() + RECOVERY_RETRY_DELAY_MS;
}

void handleStartup() {
  if (stateEntryPending) {
    stateEntryPending = false;
    stopMotors();
    centerServo();
    stateDeadline = millis() + STARTUP_SETTLE_TIME_MS;
    return;
  }

  if (!deadlineReached()) {
    return;
  }

  uint16_t distance = readFilteredDistance();
  if (distance == 0) {
    emergencyStop();
    return;
  }

  frontDistanceCm = distance;
  consecutiveInvalidReadings = 0;
  transitionTo(STATE_DRIVE);
}

void handleDrive() {
  if (stateEntryPending) {
    stateEntryPending = false;
    centerServo();
    driveForward();
    lastFrontCheckAt = 0;
    consecutiveInvalidReadings = 0;
    return;
  }

  unsigned long now = millis();
  if (now - lastFrontCheckAt < FRONT_CHECK_INTERVAL_MS) {
    return;
  }
  lastFrontCheckAt = now;

  uint16_t distance = readFilteredDistance();
  bool valid = distance != 0;
  debugPrintDistance(distance, valid);

  if (!valid) {
    ++consecutiveInvalidReadings;
    if (consecutiveInvalidReadings >= MAX_CONSECUTIVE_INVALID_READINGS) {
      emergencyStop();
    }
    return;
  }

  consecutiveInvalidReadings = 0;
  frontDistanceCm = distance;
  if (frontDistanceCm <= OBSTACLE_DISTANCE_CM) {
    avoidObstacle();
  }
}

void handleStop() {
  if (stateEntryPending) {
    stateEntryPending = false;
    stopMotors();
  }

  if (sensorFailureLatched || !deadlineReached()) {
    return;
  }

  if (navigationAction == ACTION_AVOID_OBSTACLE) {
    transitionTo(STATE_REVERSE);
  } else {
    transitionTo(STATE_DRIVE);
  }
}

void handleReverse() {
  if (stateEntryPending) {
    stateEntryPending = false;
    driveBackward();
    stateDeadline = millis() + REVERSE_TIME_MS;
    return;
  }

  if (!deadlineReached()) {
    return;
  }

  stopMotors();
  transitionTo(STATE_SCAN);
}

void handleScan() {
  if (!stateEntryPending) {
    return;
  }

  stateEntryPending = false;
  scanEnvironment();

  if (invalidScanReadings >= MAX_INVALID_SCAN_READINGS) {
    emergencyStop();
    return;
  }

  selectedDirection = chooseDirection();
  if (selectedDirection == NO_DIRECTION) {
    deadEndEscape();
    return;
  }

  transitionTo(STATE_TURN);
}

void updateTurnHistory() {
  int8_t newTurnDirection = 0;
  if (selectedDirection < SCAN_DIRECTION_COUNT / 2) {
    newTurnDirection = -1;
  } else if (selectedDirection > SCAN_DIRECTION_COUNT / 2) {
    newTurnDirection = 1;
  }

  if (newTurnDirection != 0 && newTurnDirection == lastTurnDirection) {
    ++repeatedTurnCount;
  } else if (newTurnDirection != 0) {
    repeatedTurnCount = 1;
  } else {
    repeatedTurnCount = 0;
  }
  lastTurnDirection = newTurnDirection;
}

void handleTurn() {
  if (stateEntryPending) {
    stateEntryPending = false;
    stopMotors();
    delay(MOTOR_DIRECTION_DELAY_MS);

    if (selectedDirection == NO_DIRECTION) {
      deadEndEscape();
      return;
    }

    if (selectedDirection < SCAN_DIRECTION_COUNT / 2) {
      turnLeft();
      stateDeadline = millis() + TURN_NINETY_TIME_MS;
    } else if (selectedDirection > SCAN_DIRECTION_COUNT / 2) {
      turnRight();
      stateDeadline = millis() + TURN_NINETY_TIME_MS;
    } else {
      stateDeadline = millis() + STOP_SETTLE_TIME_MS;
    }
    return;
  }

  if (!deadlineReached()) {
    return;
  }

  stopMotors();
  updateTurnHistory();
  centerServo();
  navigationAction = ACTION_NONE;
  transitionTo(STATE_DRIVE);
}

void handleEscape() {
  if (stateEntryPending) {
    stateEntryPending = false;
    stopMotors();
    escapePhase = ESCAPE_WAIT;
    stateDeadline = millis() + STOP_SETTLE_TIME_MS;
    return;
  }

  if (!deadlineReached()) {
    return;
  }

  switch (escapePhase) {
    case ESCAPE_WAIT:
      driveBackward();
      escapePhase = ESCAPE_REVERSE;
      stateDeadline = millis() + DEAD_END_REVERSE_TIME_MS;
      break;

    case ESCAPE_REVERSE:
      stopMotors();
      escapePhase = ESCAPE_TURN;
      stateDeadline = millis() + MOTOR_DIRECTION_DELAY_MS;
      break;

    case ESCAPE_TURN:
      spin180();
      escapePhase = ESCAPE_FINISH;
      stateDeadline = millis() + TURN_180_TIME_MS;
      break;

    case ESCAPE_FINISH:
      stopMotors();
      navigationAction = ACTION_NONE;
      transitionTo(STATE_DRIVE);
      break;
  }
}

void handleRecover() {
  if (stateEntryPending) {
    stateEntryPending = false;
    stopMotors();
    centerServo();
    sensorRecoveryAttempts = 0;
    stateDeadline = millis() + RECOVERY_RETRY_DELAY_MS;
    return;
  }

  if (!deadlineReached()) {
    return;
  }

  uint16_t distance = readFilteredDistance();
  if (distance != 0) {
    frontDistanceCm = distance;
    consecutiveInvalidReadings = 0;
    sensorFailureLatched = false;
    transitionTo(STATE_DRIVE);
    return;
  }

  if (sensorRecoveryAttempts + 1 < MAX_SENSOR_RECOVERY_ATTEMPTS) {
    ++sensorRecoveryAttempts;
    stateDeadline = millis() + RECOVERY_RETRY_DELAY_MS;
    return;
  }

  sensorFailureLatched = true;
  stopMotors();
  centerServo();
  transitionTo(STATE_STOP);
}

void updateRobot() {
  switch (currentState) {
    case STATE_STARTUP:
      handleStartup();
      break;
    case STATE_DRIVE:
      handleDrive();
      break;
    case STATE_STOP:
      handleStop();
      break;
    case STATE_REVERSE:
      handleReverse();
      break;
    case STATE_SCAN:
      handleScan();
      break;
    case STATE_TURN:
      handleTurn();
      break;
    case STATE_ESCAPE:
      handleEscape();
      break;
    case STATE_RECOVER:
      handleRecover();
      break;
  }
}

void autonomousDrive() {
  updateRobot();
}

// ============================================================================
// Arduino entry points
// ============================================================================

void setup() {
  pinMode(LEFT_MOTOR_IN1_PIN, OUTPUT);
  pinMode(LEFT_MOTOR_IN2_PIN, OUTPUT);
  pinMode(RIGHT_MOTOR_IN1_PIN, OUTPUT);
  pinMode(RIGHT_MOTOR_IN2_PIN, OUTPUT);

  pinMode(LEFT_LINE_SENSOR_PIN, INPUT);
  pinMode(RIGHT_LINE_SENSOR_PIN, INPUT);
  pinMode(IR_RECEIVER_PIN, INPUT);
  pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);
  pinMode(ULTRASONIC_ECHO_PIN, INPUT);

  if (DEBUG) {
    Serial.begin(115200);
    Serial.println(F("Autonomous 4WD robot starting"));
  }

  stopMotors();
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);

  scannerServo.attach(SERVO_PIN);
  currentServoAngle = SERVO_CENTER_ANGLE;
  scannerServo.write(currentServoAngle);

  transitionTo(STATE_STARTUP);
}

void loop() {
  autonomousDrive();
}

// END OF FILE
