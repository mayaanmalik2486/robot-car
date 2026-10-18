/*
======================================================
                ROBOT V3
======================================================

Arduino UNO
Sensor Shield V5

Verified Wiring

ENA  -> D5
ENB  -> D6

IN1  -> D2
IN2  -> D4
IN3  -> D7
IN4  -> D10

IR Receiver -> D8

Servo -> D9

HC-SR04
TRIG -> D11
ECHO -> D12



======================================================
*/

#include <Arduino.h>
#include <Servo.h>
#include <IRremote.hpp>
#include <SoftwareSerial.h>


//======================================================
// MOTOR PINS
//======================================================

const byte ENA = 5;
const byte ENB = 6;

const byte IN1 = 2;
const byte IN2 = 4;

const byte IN3 = 7;
const byte IN4 = 10;



//======================================================
// SENSOR PINS
//======================================================

const byte IR_PIN = 8;

const byte SERVO_PIN = 9;

const byte TRIG_PIN = 11;
const byte ECHO_PIN = 12;





//======================================================
// REMOTE CODES
//======================================================

#define CMD_UP      0x18
#define CMD_DOWN    0x52
#define CMD_LEFT    0x08
#define CMD_RIGHT   0x5A
#define CMD_OK      0x1C

#define CMD_1       0x45
#define CMD_2       0x46
#define CMD_3       0x47

#define CMD_4       0x44
#define CMD_5       0x40
#define CMD_6       0x43
#define CMD_7       0x07
#define CMD_8       0x15
#define CMD_9       0x09

#define CMD_STAR    0x16
#define CMD_0       0x19
#define CMD_HASH    0x0D



//======================================================
// SPEEDS
//======================================================

const int FULL_SPEED = 255;

const int SLOW_SPEED = FULL_SPEED*0.60;





//======================================================
// SERVO
//======================================================

const int SERVO_LEFT = 140;
const int SERVO_CENTER = 90;
const int SERVO_RIGHT = 40;



//======================================================
// DISTANCES
//======================================================

const int STOP_DISTANCE = 30;
const int SAFE_DISTANCE = 40;



//======================================================
// TIMERS
//======================================================

const unsigned long TURN180_TIME = 440;

const unsigned long TURN20_TIME = 150;

unsigned long scanServoTimer = 0;

const unsigned long SERVO_SETTLE_TIME = 35;

unsigned long lastBTCommand = 0;

const unsigned long BT_TIMEOUT = 400;

//======================================================
// STATES
//======================================================

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

    ZIGZAG,

    SEARCH,

    TURN180,

    BRAKING,

    SCANNING,

    WAITING,

    LOCKED
};

RobotState robotState = IDLE;



//======================================================
// OBJECTS
//======================================================

Servo scanner;
SoftwareSerial BT(3, A0);   // RX, TX


//======================================================
// VARIABLES
//======================================================

bool slowMode = false;

int leftSpeed = FULL_SPEED;
int rightSpeed = FULL_SPEED;


bool autoForward = false;

bool autoBackward = false;

bool donutMode = false;

bool turning180 = false;

bool braking = false;

bool danceMode = false;

byte danceStep = 0;

unsigned long danceTimer = 0;

bool demoMode = false;

byte demoStep = 0;

unsigned long demoTimer = 0;

bool zigzagMode = false;

byte zigzagStep = 0;

unsigned long zigzagTimer = 0;

int currentDistance = 999;

unsigned long distanceTimer = 0;

const unsigned long DISTANCE_INTERVAL = 50;

int currentSpeed = 0;

int targetSpeed = FULL_SPEED;

const int ACCEL_STEP = 8;

int brakePWM = FULL_SPEED;

int leftMotor = 0;
int rightMotor = 0;

unsigned long timer1 = 0;
// Bluetooth watchdog
unsigned long lastBluetoothPacket = 0;

bool timedTurn = false;

unsigned long timedTurnStart = 0;

bool timedTurnLeft = false;

bool driftLeftMode = false;
bool driftRightMode = false;


bool searchMode = false;

byte searchStep = 0;

unsigned long searchTimer = 0;

const byte SCAN_STEP = 6;

const byte TOTAL_SCANS = 26;

const byte GAP_REQUIRED = 12;   // 12 consecutive readings

int scanDistance[TOTAL_SCANS];

byte scanIndex = 0;

int bestGapStart = -1;
int bestGapEnd = -1;
int bestGapSize = 0;
int bestGapScore = 0;

int targetAngle = SERVO_CENTER;


//======================================================
// MOTOR FUNCTIONS
//======================================================

void stopMotors()
{
    applyMotorSpeeds(0, 0);
}

void driveForward(int pwm)
{
    applyMotorSpeeds(pwm, 0);
}

void driveBackward(int pwm)
{
    applyMotorSpeeds(-pwm, 0);
}

void driveCurve(int leftPWM, int rightPWM)
{
    applyMotorSpeeds(leftPWM,rightPWM);
}

void applyMotorSpeeds(int left, int right)
{
    left = constrain(left, -255, 255);
    right = constrain(right, -255, 255);

    // ---------- LEFT MOTOR ----------
    if (left > 0)
    {
        analogWrite(ENA, left);
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, HIGH);
    }
    else if (left < 0)
    {
        analogWrite(ENA, -left);
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
    }
    else
    {
        analogWrite(ENA, 0);
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, LOW);
    }

    // ---------- RIGHT MOTOR ----------
    if (right > 0)
    {
        analogWrite(ENB, right);
        digitalWrite(IN3, HIGH);
        digitalWrite(IN4, LOW);
    }
    else if (right < 0)
    {
        analogWrite(ENB, -right);
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, HIGH);
    }
    else
    {
        analogWrite(ENB, 0);
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, LOW);
    }
}

void updateTimedTurn()
{
    if (!timedTurn)
        return;

    if (millis() - timedTurnStart >= TURN20_TIME)
    {
        timedTurn = false;

        stopMotors();

        robotState = IDLE;
    }
}

void driveRobot(int throttle, int steering)
{
    int left = throttle + steering;
    int right = throttle - steering;

    left = constrain(left, -255, 255);
    right = constrain(right, -255, 255);

    applyMotorSpeeds(left, right);
}

void driveArcade(char direction, int speedPercent, char steerDir, int steerPercent)
{
    int throttle = map(speedPercent, 0, 99, 0, currentSpeed);

    int turn = map(steerPercent, 0, 99, 0, throttle);

    if(direction == 'B')
        throttle = -throttle;

    if(steerDir == 'L')
        turn = -turn;

    int left = throttle + turn;
    int right = throttle - turn;

    left = constrain(left, -255, 255);
    right = constrain(right, -255, 255);

    applyMotorSpeeds(left, right);
}


void turnLeft()
{
    int turnPWM = danceMode ? 200 : currentSpeed * 0.85;

    driveRobot(0, -turnPWM);
}

void turnRight()
{
    int turnPWM = danceMode ? 200 : currentSpeed * 0.85;

    driveRobot(0, turnPWM);
}

void driftLeft()
{
    driveRobot(currentSpeed, -90);
}

void driftRight()
{
    driveRobot(currentSpeed, 90);
}

void donut()
{
    analogWrite(ENA, danceMode ? 200 : currentSpeed);
    analogWrite(ENB, danceMode ? 200 : currentSpeed);

    digitalWrite(IN1,LOW);
    digitalWrite(IN2,HIGH);

    digitalWrite(IN3,LOW);
    digitalWrite(IN4,HIGH);
}

void spin180()
{
    analogWrite(ENA,currentSpeed);
    analogWrite(ENB,currentSpeed);

    digitalWrite(IN1,LOW);
    digitalWrite(IN2,HIGH);

    digitalWrite(IN3,LOW);
    digitalWrite(IN4,HIGH);
}
//======================================================
// ULTRASONIC FUNCTIONS
//======================================================

int readDistance()
{
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);

    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);

    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH, 8000);

    if (duration == 0)
        return 999;

    return duration * 0.0343 / 2.0;
}

void swap(int &a, int &b)
{
    int t = a;
    a = b;
    b = t;
}

int filteredDistance()
{
    int d1 = readDistance();
    int d2 = readDistance();
    int d3 = readDistance();

    int values[3] = { d1, d2, d3 };

    if(values[0] > values[1])
        swap(values[0], values[1]);

    if(values[1] > values[2])
        swap(values[1], values[2]);

    if(values[0] > values[1])
        swap(values[0], values[1]);

    return values[1];
}

//======================================================
// DISTANCE UPDATE (Non-blocking)
//======================================================

void updateDistance()
{
    if (millis() - distanceTimer >= DISTANCE_INTERVAL)
    {
        distanceTimer = millis();

        currentDistance = filteredDistance();

        // Uncomment for debugging if needed
        // Serial.print("Distance: ");
        // Serial.println(currentDistance);
    }
}

void checkObstacle()
{
    if(currentDistance > STOP_DISTANCE)
        return;

    switch(robotState)
    {
        case MANUAL_FORWARD:
        case AUTO_FORWARD:
        case DRIFT_LEFT:
        case DRIFT_RIGHT:
        case SEARCH:
        case ZIGZAG:

            stopMotors();
            break;

        default:
            break;
    }
}



//======================================================
// SERVO FUNCTIONS
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

void lookAngle(int angle)
{
    angle = constrain(angle, 15, 165);
    scanner.write(angle);
}

//======================================================
// SETUP
//======================================================

void setup()
{
    Serial.begin(115200);
    BT.begin(9600);
    Serial.println("1");

    pinMode(ENA, OUTPUT);
    pinMode(ENB, OUTPUT);

    Serial.println("2");

    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);

    Serial.println("3");

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    Serial.println("4");

    scanner.attach(SERVO_PIN);

    Serial.println("5");

    lookCenter();

    Serial.println("6");

    stopMotors();

    Serial.println("7");

    IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);

    Serial.println("READY");
}


//======================================================
// BRAKING
//======================================================

void beginBrake()
{
    braking = true;

    brakePWM = currentSpeed;

    robotState = BRAKING;
}

void updateBrake()
{
    static unsigned long lastStep = 0;

    if (!braking)
        return;

    if (millis() - lastStep < 40)
        return;

    lastStep = millis();

    brakePWM -= 10;

    if (brakePWM < 0)
        brakePWM = 0;

    analogWrite(ENA, brakePWM);
    analogWrite(ENB, brakePWM);

    if (brakePWM == 0)
    {
        braking = false;

        stopMotors();

        robotState = IDLE;
    }
}



//======================================================
// 180 TURN
//======================================================

void start180()
{
    turning180 = true;
    timer1 = millis();
    robotState = TURN180;
    spin180();
}

void update180()
{
    if (!turning180)
        return;

    if (millis() - timer1 >= TURN180_TIME)
    {
        turning180 = false;
        stopMotors();

        if (!demoMode)
            robotState = IDLE;
    }
}


//======================================================
// EMERGENCY STOP
//======================================================

void emergencyStop()
{
    autoForward = false;
    autoBackward = false;
    donutMode = false;

    danceMode = false;
    danceStep = 0;

    braking = false;
    turning180 = false;

    demoMode = false;
    demoStep = 0;
    
    zigzagMode = false;
    zigzagStep = 0;

    driftLeftMode = false;
    driftRightMode = false;

    searchMode = false;
    searchStep = 0;

    robotState = IDLE;
    stopMotors();

    lookCenter();

    Serial.println("STOP");

    currentSpeed = 0;
    targetSpeed = FULL_SPEED;
}



//======================================================
// HANDLE REMOTE
//======================================================

void handleRemote(byte cmd)
{
    switch (cmd)
    {

    case CMD_OK:

        emergencyStop();

        break;

    case CMD_2:

        slowMode = !slowMode;

        if (slowMode)
        {
            targetSpeed = SLOW_SPEED;
            Serial.println("Slow Mode ON");
        }
        else
        {
            targetSpeed = FULL_SPEED;
            Serial.println("Slow Mode OFF");
        }

        break;

    case CMD_UP:

      autoForward = false;
      autoBackward = false;
      donutMode = false;

      if (currentDistance > STOP_DISTANCE)
      {
          robotState = MANUAL_FORWARD;
          driveForward(currentSpeed);
      }
      else
      {
          stopMotors();
          robotState = IDLE;
      }

      break;

    case CMD_DOWN:

        autoForward = false;
        autoBackward = false;
        donutMode = false;

        robotState = MANUAL_BACKWARD;

        

        driveBackward(currentSpeed);

        break;

    case CMD_LEFT:

      autoForward = false;
      autoBackward = false;
      donutMode = false;

      turnLeft();

      timedTurn = true;
      timedTurnLeft = true;
      timedTurnStart = millis();

      robotState = MANUAL_LEFT;

      break;

    case CMD_RIGHT:

      autoForward = false;
      autoBackward = false;
      donutMode = false;

      turnRight();

      timedTurn = true;
      timedTurnLeft = false;
      timedTurnStart = millis();

      robotState = MANUAL_RIGHT;

      break;

    case CMD_STAR:

        autoForward = true;
        autoBackward = false;
        donutMode = false;

        robotState = AUTO_FORWARD;

        break;

    case CMD_HASH:

        autoForward = false;
        autoBackward = true;
        donutMode = false;

        robotState = AUTO_BACKWARD;

        break;

    case CMD_0:

        autoForward = false;
        autoBackward = false;
        donutMode = true;

        robotState = DONUT;

        break;

    case CMD_1:

      if (!turning180)
        start180();

      break;

    case CMD_3:

        beginBrake();

        break;

    case CMD_4:

        emergencyStop();

        demoMode = true;
        demoStep = 0;
        demoTimer = millis();

        robotState = DEMO;

        break;

    case CMD_5:

        emergencyStop();

        danceMode = true;

        danceStep = 0;

        danceTimer = millis();

        robotState = DANCE;

        break; 

    case CMD_6:

        emergencyStop();

        zigzagMode = true;

        zigzagStep = 0;

        zigzagTimer = millis();

        robotState = ZIGZAG;

        break;  

    case CMD_7:

        emergencyStop();

        robotState = DRIFT_LEFT;

        break; 

    case CMD_8:

        emergencyStop();

        robotState = DRIFT_RIGHT;

        break;


    case CMD_9:

        emergencyStop();

        searchMode = true;

        searchStep = 0;

        searchTimer = millis();

        robotState = SEARCH;

        break;    
    }
}



//======================================================
// IR UPDATE
//======================================================

unsigned long lastIRTime = 0;

void updateIR()
{
    if (!IrReceiver.decode())
        return;

    handleRemote(IrReceiver.decodedIRData.command);

    lastIRTime = millis();

    IrReceiver.resume();
}


String btBuffer = "";

void processBluetoothCommand(String cmd)
{
    lastBTCommand = millis();
    cmd.trim();

    // -------- OLD COMMANDS --------

    // ---------- Steering Packet ----------
    if(cmd.length()==6)
    {
        char dir=cmd[0];

        int speed=cmd.substring(1,3).toInt();

        char steerDir=cmd[3];

        int steer=cmd.substring(4,6).toInt();

        if((dir=='F' || dir=='B') &&
        (steerDir=='L' || steerDir=='R'))
        {
            driveArcade(dir,speed,steerDir,steer);

            return;
        }
    }
    


    if(cmd=="F")
    {
        emergencyStop();
        robotState=MANUAL_FORWARD;
        driveCurve(currentSpeed, currentSpeed);
        return;
    }

    if(cmd=="B")
    {
        emergencyStop();
        robotState=MANUAL_BACKWARD;
        driveBackward(currentSpeed);
        return;
    }

    if(cmd=="L")
    {
        emergencyStop();
        driveCurve(currentSpeed*0.45, currentSpeed);
        robotState=MANUAL_LEFT;
        return;
    }

    if(cmd=="R")
    {
        emergencyStop();
        driveCurve(currentSpeed, currentSpeed*0.45);
        robotState=MANUAL_RIGHT;
        return;
    }

    if(cmd=="S")
    {
        emergencyStop();
        return;
    }

    if(cmd=="Q")
    {
        slowMode=true;
        targetSpeed = SLOW_SPEED;
        return;
    }

    if(cmd=="W")
    {
        slowMode=false;
        targetSpeed = FULL_SPEED;
        return;
    }

    if(cmd=="A")
    {
        emergencyStop();
        autoForward=true;
        robotState=AUTO_FORWARD;
        return;
    }

    if(cmd=="X")
    {
        emergencyStop();
        autoBackward=true;
        robotState=AUTO_BACKWARD;
        return;
    }

    if(cmd=="D")
    {
        emergencyStop();
        donutMode=true;
        robotState=DONUT;
        return;
    }

    if(cmd=="T")
    {
        emergencyStop();
        start180();
        return;
    }

    if(cmd=="K")
    {
        beginBrake();
        return;
    }

    if(cmd=="N")
    {
        emergencyStop();

        danceMode=true;
        danceStep=0;
        danceTimer=millis();

        robotState=DANCE;
        return;
    }

    if(cmd=="P")
    {
        emergencyStop();

        demoMode=true;
        demoStep=0;
        demoTimer=millis();

        robotState=DEMO;
        return;
    }

    if(cmd=="Z")
    {
        emergencyStop();

        zigzagMode=true;
        zigzagStep=0;
        zigzagTimer=millis();

        robotState=ZIGZAG;
        return;
    }

    if(cmd=="H")
    {
        emergencyStop();

        searchMode=true;
        searchStep=0;

        robotState=SEARCH;
        return;
    }
}

void updateBluetoothWatchdog()
{
    if (millis() - lastBTCommand < BT_TIMEOUT)
        return;

    switch(robotState)
    {
        case MANUAL_FORWARD:
        case MANUAL_BACKWARD:
        case MANUAL_LEFT:
        case MANUAL_RIGHT:

            stopMotors();
            robotState = IDLE;
            break;

        default:
            break;
    }
}

void updateBluetooth()
{
    while (BT.available())
    {
        String command = BT.readStringUntil('\n');
        command.trim();

        if (command.length() == 0)
            continue;

        lastBluetoothPacket = millis();

        // Steering mode packet (e.g. F50R30)
        if (command.length() == 6)
        {
            char moveDir = command[0];
            int moveSpeed = command.substring(1, 3).toInt();

            char steerDir = command[3];
            int steerAmount = command.substring(4, 6).toInt();

            int throttle = map(moveSpeed, 0, 99, 0, currentSpeed);
            int steering = map(steerAmount, 0, 99, 0, currentSpeed);

            if (moveDir == 'B')
                throttle = -throttle;

            if (steerDir == 'L')
                steering = -steering;

            driveRobot(throttle, steering);
            robotState = MANUAL_FORWARD;
        }
        else
        {
            processBluetoothCommand(command);
        }
    }
}



//======================================================
// CONTINUOUS DRIVING
//======================================================

void updateDriving()
{
    switch (robotState)
    {

    case AUTO_FORWARD:

        if(currentDistance>STOP_DISTANCE)
        {
            driveCurve(currentSpeed, currentSpeed);
        }
        else
        {
            stopMotors();
        }

        break;

   case AUTO_BACKWARD:

        if(currentDistance > STOP_DISTANCE)
            driveBackward(currentSpeed);
        else
            stopMotors();

        break;

    case DONUT:

        donut();

        break;

      

    case DRIFT_LEFT:

        if(currentDistance>STOP_DISTANCE)
            driftLeft();
        else
            stopMotors();

        break;

    case DRIFT_RIGHT:

        if(currentDistance>STOP_DISTANCE)
            driftRight();
        else
            stopMotors();

    break;

  default:

          break;
    }
}

void updateDance()
{
    if (!danceMode)
        return;

    if (millis() - danceTimer < 215)
        return;

    danceTimer = millis();

    switch (danceStep)
    {
        case 0:

            driveForward(200);

            break;

        case 1:

            driveBackward(200);

            break;

        case 2:

            turnLeft();

            break;

        case 3:

            turnRight();

            break;

        case 4:

            donut();

            break;

        case 5:

            driveForward(currentSpeed);

            break;

        case 6:

            turnLeft();

            break;

        case 7:

            turnRight();

            break;
    }

    danceStep++;

    if (danceStep > 7)
        danceStep = 0;
}

void updateDemo()
{
    if (!demoMode)
        return;

    switch (demoStep)
    {
        //----------------------------
        // Forward
        //----------------------------
        case 0:

            driveForward(currentSpeed);

            if (millis() - demoTimer >= 400)
            {
                stopMotors();
                demoTimer = millis();
                demoStep = 1;
            }

            break;

        //----------------------------
        // Brake
        //----------------------------
        case 1:

            stopMotors();

            if (millis() - demoTimer >= 600)
            {
                demoTimer = millis();
                demoStep = 2;
            }

            break;

        //----------------------------
        // Reverse
        //----------------------------
        case 2:

            driveBackward(currentSpeed);

            if (millis() - demoTimer >= 375)
            {
                stopMotors();
                demoTimer = millis();
                demoStep = 3;
            }

            break;

        //----------------------------
        // Spin
        //----------------------------
        //----------------------------
// Spin
//------  ----------------------
        case 3:

            donut();

            if (millis() - demoTimer >= 2000)
            {
                stopMotors();

                start180();

                demoStep = 4;
            }

            break;


        //----------------------------
        // Wait for 180
        //----------------------------
        case 4:

            if (!turning180)
            {
                robotState = DEMO;   // put demo back in control

                demoTimer = millis();

                demoStep = 5;
            }

    break;
        //----------------------------
        // Final Launch
        //----------------------------
        case 5:

            driveForward(currentSpeed);

            if (millis() - demoTimer >= 375)
            {
                stopMotors();

                demoMode = false;
                demoStep = 0;

                robotState = IDLE;
            }

            break;
    }
}


//======================================================
// PART 3
// SMART OBSTACLE AVOIDANCE + MAIN LOOP
//======================================================


void updateZigzag()
{
    if (!zigzagMode)
        return;

    switch (zigzagStep)
    {
        //--------------------------
        // Forward
        //--------------------------
        case 0:

    if (currentDistance <= STOP_DISTANCE)
    {
        stopMotors();

        zigzagTimer = millis();

        zigzagStep = 4;

        break;
    }

    driveForward(currentSpeed);

    if (millis() - zigzagTimer >= 600)
    {
        zigzagTimer = millis();
        zigzagStep = 1;
    }

    break;

        //--------------------------
        // Left
        //--------------------------
        case 1:

            turnLeft();

            if (millis() - zigzagTimer >= 105)
            {
                zigzagTimer = millis();
                zigzagStep = 2;
            }

            break;

        //--------------------------
        // Forward
        //--------------------------
        case 2:

        if (currentDistance <= STOP_DISTANCE)
        {
            stopMotors();

            zigzagTimer = millis();

            zigzagStep = 4;

            break;
        }

        driveForward(currentSpeed);

        if (millis() - zigzagTimer >= 600)
        {
            zigzagTimer = millis();
            zigzagStep = 3;
        }

        break;

        //--------------------------
        // Right
        //--------------------------
        case 3:

            turnRight();

            if (millis() - zigzagTimer >= 105)
            {
                zigzagTimer = millis();
                zigzagStep = 0;
            }

            break;

        //--------------------------
        // Reverse Away
        //--------------------------

        case 4:

            driveBackward(currentSpeed);

            if (millis() - zigzagTimer >= 180)
            {
                zigzagTimer = millis();
                zigzagStep = 5;
            }

            break;

        //--------------------------
        // Turn Away
        //--------------------------
        case 5:

            turnRight();

            if (millis() - zigzagTimer >= 350)
            {
                zigzagTimer = millis();
                zigzagStep = 0;
            }

            break;  

    }
}    

void updateAcceleration()
{
    if(currentSpeed < targetSpeed)
    {
        currentSpeed += ACCEL_STEP;

        if(currentSpeed > targetSpeed)
            currentSpeed = targetSpeed;
    }

    else if(currentSpeed > targetSpeed)
    {
        currentSpeed -= ACCEL_STEP;

        if(currentSpeed < targetSpeed)
            currentSpeed = targetSpeed;
    }
}

//======================================================
// LOOP
//======================================================

void updateSearch()
{
    if (!searchMode)
        return;

    switch (searchStep)
    {
    //--------------------------------------------------
    // 0. Drive forward
    //--------------------------------------------------
        //--------------------------------------------------
    // 0. Drive forward
    //--------------------------------------------------
    case 0:

        lookCenter();

        if (currentDistance > STOP_DISTANCE)
        {
            driveForward(currentSpeed);
        }
        else
        {
            stopMotors();

            driveBackward(currentSpeed);

            delay(120);

            stopMotors();

            scanIndex = 0;
            searchStep = 1;
            scanServoTimer = millis();
        }

        break;

    //--------------------------------------------------
    // 1. Scan 15° -> 165°
    //--------------------------------------------------
    case 1:

        if(millis() - scanServoTimer >= SERVO_SETTLE_TIME)
        {
            scanServoTimer = millis();

            lookAngle(15 + scanIndex * SCAN_STEP);

            scanDistance[scanIndex] = currentDistance;

            scanIndex++;

            if(scanIndex >= TOTAL_SCANS)
            {
                scanIndex = 0;
                searchStep = 2;
            }
        }

        break;
    //--------------------------------------------------
    // 2. Find best opening
    //--------------------------------------------------
    case 2:

        bestGapStart = -1;
        bestGapEnd = -1;
        bestGapSize = 0;
        bestGapScore = 0;

        {
            int start = -1;

            for (int i = 0; i <= TOTAL_SCANS; i++)
            {
                bool open = (i < TOTAL_SCANS && scanDistance[i] >= SAFE_DISTANCE);

                if (open)
                {
                    if (start == -1)
                        start = i;
                }
                else if (start != -1)
                {
                    int end = i - 1;
                    int size = end - start + 1;

                    int total = 0;
                    for (int j = start; j <= end; j++)
                        total += scanDistance[j];

                    int average = total / size;
                    int score = average + size * 10;

                    if (score > bestGapScore)
                    {
                        bestGapScore = score;
                        bestGapSize = size;
                        bestGapStart = start;
                        bestGapEnd = end;
                    }

                    start = -1;
                }
            }
        }

        searchStep = 3;
        break;

    //--------------------------------------------------
    // 3. Turn toward best gap
    //--------------------------------------------------
    case 3:

        if (bestGapSize >= GAP_REQUIRED)
        {
            int centerIndex = (bestGapStart + bestGapEnd) / 2;
            targetAngle = 15 + centerIndex * SCAN_STEP;

            int angleError = targetAngle - SERVO_CENTER;

            int turnTime = abs(angleError) * 3;

            if (turnTime < 40)
                turnTime = 40;

            if (angleError < -5)
            {
                turnRight();
                delay(turnTime);
            }
            else if (angleError > 5)
            {
                turnLeft();
                delay(turnTime);
            }

            stopMotors();
        }
        else
        {
            turnRight();
            delay(120);
            stopMotors();
        }

        lookCenter();
        searchStep = 0;

        break;
    }
}
void loop()
{

    updateBluetooth();

    updateDistance();

    updateAcceleration();

    updateBluetoothWatchdog();

    updateIR();

    updateDriving();

    updateDance();

    updateDemo();

    update180();

    updateTimedTurn();

    updateBrake();

    checkObstacle();

    updateZigzag();

    updateSearch();
    /*

    if (millis() - lastIRTime > 150 &&
        robotState != AUTO_FORWARD &&
        robotState != AUTO_BACKWARD &&
        robotState != DONUT &&
        robotState != DANCE &&
        robotState != DEMO &&
        robotState != ZIGZAG &&
        robotState != DRIFT_LEFT &&
        robotState != DRIFT_RIGHT &&
        robotState != SEARCH &&
        robotState != TURN180 &&
        robotState != BRAKING)
    {
          stopMotors();
    }
    */
}
