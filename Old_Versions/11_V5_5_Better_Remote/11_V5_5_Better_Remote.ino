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

const int SERVO_LEFT = 165;
const int SERVO_CENTER = 90;
const int SERVO_RIGHT = 15;



//======================================================
// DISTANCES
//======================================================

const int STOP_DISTANCE = 25;
const int SAFE_DISTANCE = 40;



//======================================================
// TIMERS
//======================================================

const unsigned long TURN180_TIME = 440;

const unsigned long TURN20_TIME = 150;



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



//======================================================
// VARIABLES
//======================================================

bool slowMode = false;

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



int currentSpeed = FULL_SPEED;

int brakePWM = FULL_SPEED;



unsigned long timer1 = 0;


bool driftLeftMode = false;
bool driftRightMode = false;


bool searchMode = false;

byte searchStep = 0;

unsigned long searchTimer = 0;

const byte SCAN_STEP = 3;

const byte TOTAL_SCANS = 51;

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
    analogWrite(ENA,0);
    analogWrite(ENB,0);

    digitalWrite(IN1,LOW);
    digitalWrite(IN2,LOW);

    digitalWrite(IN3,LOW);
    digitalWrite(IN4,LOW);
}

void driveForward(int pwm)
{
    analogWrite(ENA,pwm);
    analogWrite(ENB,pwm);

    digitalWrite(IN1,LOW);
    digitalWrite(IN2,HIGH);

    digitalWrite(IN3,HIGH);
    digitalWrite(IN4,LOW);
}

void driveBackward(int pwm)
{
    analogWrite(ENA,pwm);
    analogWrite(ENB,pwm);

    digitalWrite(IN1,HIGH);
    digitalWrite(IN2,LOW);

    digitalWrite(IN3,LOW);
    digitalWrite(IN4,HIGH);
}

void turnLeft()
{
    int turnPWM = currentSpeed * 0.85;

    analogWrite(ENA, turnPWM);
    analogWrite(ENB, turnPWM);

    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);

    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
}

void turnRight()
{
    int turnPWM = currentSpeed * 0.85;

    analogWrite(ENA, turnPWM);
    analogWrite(ENB, turnPWM);

    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);

    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
}

void driftLeft()
{
    analogWrite(ENA, currentSpeed * 0.60);   // Left wheel slower
    analogWrite(ENB, currentSpeed);          // Right wheel full speed

    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);

    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
}

void driftRight()
{
    analogWrite(ENA, currentSpeed);          // Left wheel full speed
    analogWrite(ENB, currentSpeed * 0.60);   // Right wheel slower

    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);

    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
}

void donut()
{
    analogWrite(ENA,currentSpeed);
    analogWrite(ENB,currentSpeed);

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

    long duration = pulseIn(ECHO_PIN, HIGH, 30000);

    if (duration == 0)
        return 999;

    return duration * 0.0343 / 2.0;
}

int filteredDistance()
{
    int a = readDistance();
    delay(15);

    int b = readDistance();
    delay(15);

    int c = readDistance();

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

void checkObstacle()
{
    int distance = filteredDistance();

    if (distance <= STOP_DISTANCE)
    {
        if (robotState == MANUAL_FORWARD)
        {
            stopMotors();
            robotState = IDLE;
        }
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



//======================================================
// SETUP
//======================================================

void setup()
{
    Serial.begin(115200);
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
            currentSpeed = SLOW_SPEED;
            Serial.println("Slow Mode ON");
        }
        else
        {
            currentSpeed = FULL_SPEED;
            Serial.println("Slow Mode OFF");
        }

        break;

    case CMD_UP:

      autoForward = false;
      autoBackward = false;
      donutMode = false;

      if (filteredDistance() > STOP_DISTANCE)
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

      delay(TURN20_TIME);

      stopMotors();

      robotState = IDLE;

      break;

    case CMD_RIGHT:

      autoForward = false;
      autoBackward = false;
      donutMode = false;

      turnRight();

      delay(TURN20_TIME);

      stopMotors();

      robotState = IDLE;

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



//======================================================
// CONTINUOUS DRIVING
//======================================================

void updateDriving()
{
    switch (robotState)
    {

    case AUTO_FORWARD:

      if (filteredDistance() > STOP_DISTANCE)
      {
          driveForward(currentSpeed);
      }
      else
      {
          stopMotors();
      }

      break;

    case AUTO_BACKWARD:

        driveBackward(currentSpeed);

        break;

    case DONUT:

        donut();

        break;

      

    case DRIFT_LEFT:

        if(filteredDistance()>STOP_DISTANCE)
            driftLeft();
        else
            stopMotors();

        break;

    case DRIFT_RIGHT:

        if(filteredDistance()>STOP_DISTANCE)
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

    if (millis() - danceTimer < 450)
        return;

    danceTimer = millis();

    switch (danceStep)
    {
        case 0:

            driveForward(currentSpeed);

            break;

        case 1:

            driveBackward(currentSpeed);

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

    if (filteredDistance() <= STOP_DISTANCE)
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

        if (filteredDistance() <= STOP_DISTANCE)
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

//======================================================
// LOOP
//======================================================

void updateSearch()
{
    if (!searchMode)
        return;

    switch (searchStep)
    {
        //------------------------
        // Drive Forward
        //------------------------
        case 0:

            lookCenter();
            delay(150);   // allow servo to center
            {
                driveForward(currentSpeed);
            }
            else
            {
                stopMotors();

                searchTimer = millis();

                searchStep = 1;
            }

            break;

        //------------------------
        // Look Left
        //------------------------
        case 1:

            scanner.write(15 + scanIndex * SCAN_STEP);

            delay(70);

            scanDistance[scanIndex] = filteredDistance();

            scanIndex++;

            if (scanIndex >= TOTAL_SCANS)
            {
                scanIndex = 0;
                searchStep = 2;
            }

            break;

        //------------------------
        // Look Center
        //------------------------
        case 2:

            bestGapStart = -1;
            bestGapEnd = -1;
            bestGapSize = 0;
            bestGapScore = 0;

            int start = -1;

            for (int i = 0; i < TOTAL_SCANS; i++)
            {
                if (scanDistance[i] >= SAFE_DISTANCE)
                {
                    if (start == -1)
                        start = i;
                }
                else
                {
                    if (start != -1)
                    {
                        int end = i - 1;
                        int size = end - start + 1;

                        int total = 0;
                        for (int j = start; j <= end; j++)
                            total += scanDistance[j];

                        int average = total / size;

                        int score = average + (size * 10);

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

            // Handle a gap that reaches the last scan
            if (start != -1)
            {
                int end = TOTAL_SCANS - 1;
                int size = end - start + 1;

                int total = 0;
                for (int j = start; j <= end; j++)
                    total += scanDistance[j];

                int average = total / size;

                int score = average + (size * 10);

                if (score > bestGapScore)
                {
                    bestGapScore = score;
                    bestGapSize = size;
                    bestGapStart = start;
                    bestGapEnd = end;
        }
    }

    searchStep = 3;

    break;

        //------------------------
        // Look Right
        //------------------------
        case 3:

            if (bestGapSize >= GAP_REQUIRED)
            {
                int centerIndex = (bestGapStart + bestGapEnd) / 2;
                targetAngle = 15 + centerIndex * SCAN_STEP;

                int angleError = targetAngle - SERVO_CENTER;
                int turnTime = abs(angleError) * 4;

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

        //------------------------
        // Wait
        //------------------------
                //------------------------
        // Choose Best Direction
        //------------------------
        
    }
}
void loop()
{
    updateIR();

    updateDriving();

    updateDance();

    updateDemo();

    update180();

    updateBrake();

    checkObstacle();

    updateZigzag();

    updateSearch();

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
}
