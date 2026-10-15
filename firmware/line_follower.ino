/*
  Line-Follower Robot — starter firmware
  Assumed hardware:
    - Arduino Uno/Nano-compatible board
    - Two digital IR line sensors (left and right)
    - L298N-style motor driver
    - ENA -> D5, IN1 -> D8, IN2 -> D9
    - ENB -> D6, IN3 -> D10, IN4 -> D11
    - Left sensor -> D2, right sensor -> D3

  IMPORTANT:
  - Many IR modules output LOW on a dark line, but some output HIGH.
    Set LINE_DETECTED to match your sensor module.
  - Confirm wiring and motor direction before running on the floor.
*/

const int LEFT_SENSOR_PIN  = 2;
const int RIGHT_SENSOR_PIN = 3;

const int ENA = 5;   // Left motor speed (PWM)
const int IN1 = 8;
const int IN2 = 9;

const int ENB = 6;   // Right motor speed (PWM)
const int IN3 = 10;
const int IN4 = 11;

// Change to HIGH if your sensor outputs HIGH when it sees the line.
const int LINE_DETECTED = LOW;

const int BASE_SPEED = 130;  // Range: 0–255
const int TURN_SPEED = 115;

void setup() {
  pinMode(LEFT_SENSOR_PIN, INPUT);
  pinMode(RIGHT_SENSOR_PIN, INPUT);

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  stopMotors();
  Serial.begin(9600);
}

void loop() {
  const bool leftOnLine = (digitalRead(LEFT_SENSOR_PIN) == LINE_DETECTED);
  const bool rightOnLine = (digitalRead(RIGHT_SENSOR_PIN) == LINE_DETECTED);

  Serial.print("Left: ");
  Serial.print(leftOnLine);
  Serial.print("  Right: ");
  Serial.println(rightOnLine);

  if (leftOnLine && rightOnLine) {
    // Both sensors see the line: move forward.
    drive(BASE_SPEED, BASE_SPEED);
  } else if (leftOnLine && !rightOnLine) {
    // Line is toward the left sensor: steer left.
    drive(TURN_SPEED / 2, TURN_SPEED);
  } else if (!leftOnLine && rightOnLine) {
    // Line is toward the right sensor: steer right.
    drive(TURN_SPEED, TURN_SPEED / 2);
  } else {
    // Neither sensor sees the line: stop safely while testing.
    stopMotors();
  }

  delay(10);
}

void drive(int leftSpeed, int rightSpeed) {
  setMotor(ENA, IN1, IN2, leftSpeed);
  setMotor(ENB, IN3, IN4, rightSpeed);
}

void setMotor(int enablePin, int inA, int inB, int speedValue) {
  speedValue = constrain(speedValue, -255, 255);

  if (speedValue > 0) {
    digitalWrite(inA, HIGH);
    digitalWrite(inB, LOW);
    analogWrite(enablePin, speedValue);
  } else if (speedValue < 0) {
    digitalWrite(inA, LOW);
    digitalWrite(inB, HIGH);
    analogWrite(enablePin, -speedValue);
  } else {
    digitalWrite(inA, LOW);
    digitalWrite(inB, LOW);
    analogWrite(enablePin, 0);
  }
}

void stopMotors() {
  setMotor(ENA, IN1, IN2, 0);
  setMotor(ENB, IN3, IN4, 0);
}
