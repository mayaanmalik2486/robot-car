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

Left Line Sensor  -> A0
Right Line Sensor -> A1

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

const byte LEFT_LINE = A0;
const byte RIGHT_LINE = A1;



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

const int SERVO_LEFT = 15;
const int SERVO_CENTER = 90;
const int SERVO_RIGHT = 165;



//======================================================
// DISTANCES
//======================================================

const int STOP_DISTANCE = 25;

const int CLEAR_DISTANCE = 40;



//======================================================
// TIMERS
//======================================================

const unsigned long TURN180_TIME = 440;

const unsigned long TURN20_TIME = 150;

const unsigned long LOOK_TIME = 2000;

const unsigned long SERVO_TIME = 350;

const unsigned long PING_TIME = 80;


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

bool obstacleLock = false;



int currentSpeed = FULL_SPEED;

int brakePWM = FULL_SPEED;



unsigned long timer1 = 0;
unsigned long timer2 = 0;
unsigned long timer3 = 0;


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
    int turnPWM = currentSpeed * 0.60;

    analogWrite(ENA, turnPWM);
    analogWrite(ENB, turnPWM);

    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);

    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
}

void turnRight()
{
    int turnPWM = currentSpeed * 0.60;

    analogWrite(ENA, turnPWM);
    analogWrite(ENB, turnPWM);

    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);

    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
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

    pinMode(ENA, OUTPUT);
    pinMode(ENB, OUTPUT);

    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);

    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    pinMode(LEFT_LINE, INPUT);
    pinMode(RIGHT_LINE, INPUT);

    scanner.attach(SERVO_PIN);

    lookCenter();

    stopMotors();

    IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);

    Serial.println();
    Serial.println("==============================");
    Serial.println("ROBOT V3 READY");
    Serial.println("==============================");
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

    braking = false;
    turning180 = false;

    obstacleLock = false;

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

        robotState = MANUAL_FORWARD;

        

        driveForward(currentSpeed);

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

        start180();

        break;

    case CMD_3:

        beginBrake();

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

        driveForward(currentSpeed);

        break;

    case AUTO_BACKWARD:

        driveBackward(currentSpeed);

        break;

    case DONUT:

        donut();

        break;

    default:

        break;
    }
}
//======================================================
// PART 3
// SMART OBSTACLE AVOIDANCE + MAIN LOOP
//======================================================




//======================================================
// LOOP
//======================================================

void loop()
{
    updateIR();

    updateDriving();

    update180();

    updateBrake();

    if (millis() - lastIRTime > 150 &&
        robotState != AUTO_FORWARD &&
        robotState != AUTO_BACKWARD &&
        robotState != DONUT &&
        robotState != TURN180 &&
        robotState != BRAKING)
    {
        stopMotors();
    }
}