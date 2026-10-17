
#include <Arduino.h>

// VORTEX FLF Rev 2 - Robust starting firmware
// ESP32 + 8-channel QTR-RC-style sensor + TB6612FNG
// Arduino-ESP32 Core 3.x
//
// Verify all GPIOs and the actual sensor board before use.
// Sensor array ordered LEFT -> RIGHT.

const int SENSORS = 8;
const int sensorPins[SENSORS] =
    {13, 14, 16, 17, 18, 19, 21, 22};

const int PWMA = 25, AIN1 = 26, AIN2 = 27;
const int PWMB = 32, BIN1 = 33, BIN2 = 23;
const int STBY = 4;

// Change to true only if your module's output polarity
// has been confirmed to be opposite.
const bool INVERT_MOTOR_LEFT = false;
const bool INVERT_MOTOR_RIGHT = false;

// ---------- Adjustable settings ----------
const int BASE_SPEED = 95;
const int MIN_SPEED = 55;
const int MAX_SPEED = 170;
const int SEARCH_SPEED = 65;

float KP = 15.0;
float KD = 8.0;

const unsigned int RC_TIMEOUT_US = 2500;
const unsigned long CALIBRATION_MS = 4000;
const unsigned long LOST_STOP_MS = 1400;
const unsigned long JUNCTION_CONFIRM_MS = 25;
const unsigned long SENSOR_FAULT_MS = 500;

// Intersection policy:
// STRAIGHT: continue straight at a detected wide crossing.
// STOP: stop at a confirmed crossing for manual inspection.
// This does NOT identify a required branch automatically.
enum JunctionPolicy { STRAIGHT, STOP_AT_JUNCTION };
const JunctionPolicy JUNCTION_POLICY = STRAIGHT;

// ---------- Internal state ----------
unsigned int calMin[SENSORS];
unsigned int calMax[SENSORS];
unsigned int raw[SENSORS];
int strength[SENSORS];

float lastError = 0;
float filteredDerivative = 0;

unsigned long lostAt = 0;
unsigned long junctionAt = 0;
unsigned long invalidAt = 0;

bool calibrated = false;
bool wasLost = false;
bool junctionPending = false;
bool junctionLatched = false;

// ---------- RC sensor reading ----------
unsigned int readRC(int pin) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);
  delayMicroseconds(10);

  pinMode(pin, INPUT);
  unsigned long start = micros();

  while (digitalRead(pin) == HIGH) {
    if ((unsigned long)(micros() - start) >= RC_TIMEOUT_US) {
      return RC_TIMEOUT_US;
    }
  }

  return (unsigned int)(micros() - start);
}

void readAllSensors() {
  for (int i = 0; i < SENSORS; i++) {
    raw[i] = readRC(sensorPins[i]);
  }
}

// ---------- Per-sensor calibration ----------
void calibrateSensors() {
  Serial.println("Calibration: move sensor array over black AND white.");
  Serial.println("Keep motors stopped; calibration runs for 4 seconds.");

  for (int i = 0; i < SENSORS; i++) {
    calMin[i] = RC_TIMEOUT_US;
    calMax[i] = 0;
  }

  unsigned long start = millis();

  while (millis() - start < CALIBRATION_MS) {
    readAllSensors();

    for (int i = 0; i < SENSORS; i++) {
      if (raw[i] < calMin[i]) calMin[i] = raw[i];
      if (raw[i] > calMax[i]) calMax[i] = raw[i];
    }
    delay(2);
  }

  calibrated = true;

  for (int i = 0; i < SENSORS; i++) {
    Serial.printf("Sensor %d: min=%u max=%u\n",
                  i, calMin[i], calMax[i]);

    // Refuse to pretend calibration succeeded when
    // a sensor has not observed enough variation.
    if (calMax[i] < calMin[i] + 100) {
      calibrated = false;
    }
  }

  if (!calibrated) {
    Serial.println("Calibration failed. Check sensor wiring/surface.");
  } else {
    Serial.println("Calibration finished.");
  }
}

// Normalized black confidence, 0..1000.
// Assumes black produces a longer RC discharge time.
void normalizeSensors() {
  for (int i = 0; i < SENSORS; i++) {
    unsigned int span = calMax[i] - calMin[i];

    if (span < 100) {
      strength[i] = 0;
      continue;
    }

    long value = (long)(raw[i] - calMin[i]) * 1000L / span;
    strength[i] = constrain((int)value, 0, 1000);
  }
}

// ---------- Motor control ----------
void setMotor(int pwm, int in1, int in2,
              int speedValue, bool invert) {
  speedValue = constrain(speedValue, -255, 255);
  if (invert) speedValue = -speedValue;

  if (speedValue > 0) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
  } else if (speedValue < 0) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
  } else {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
  }

  ledcWrite(pwm, abs(speedValue));
}

void drive(int left, int right) {
  setMotor(PWMA, AIN1, AIN2, left, INVERT_MOTOR_LEFT);
  setMotor(PWMB, BIN1, BIN2, right, INVERT_MOTOR_RIGHT);
}

void stopRobot() {
  drive(0, 0);
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);

  pinMode(STBY, OUTPUT);
  digitalWrite(STBY, LOW); // Disable driver during setup

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  ledcAttach(PWMA, 20000, 8);
  ledcAttach(PWMB, 20000, 8);

  stopRobot();

  // Never drive during calibration.
  calibrateSensors();

  if (!calibrated) {
    Serial.println("LOCKOUT: fix sensors, then restart.");
    while (true) {
      stopRobot();
      delay(100);
    }
  }

  digitalWrite(STBY, HIGH);
  Serial.println("Ready for supervised low-speed testing.");
}

// ---------- Main controller ----------
void loop() {
  readAllSensors();
  normalizeSensors();

  int active = 0;
  long weightedSum = 0;
  long totalStrength = 0;

  for (int i = 0; i < SENSORS; i++) {
    // Ignore weak detections. Tune after inspecting sensor logs.
    if (strength[i] >= 500) active++;

    weightedSum += (long)(strength[i]) *
                   (-7000 + i * 2000);
    totalStrength += strength[i];
  }

  // Detect a dead/stuck sensor or missing sensor signal.
  bool allTimedOut = true;
  for (int i = 0; i < SENSORS; i++) {
    if (raw[i] < RC_TIMEOUT_US) allTimedOut = false;
  }

  if (allTimedOut) {
    if (invalidAt == 0) invalidAt = millis();

    if (millis() - invalidAt >= SENSOR_FAULT_MS) {
      stopRobot();
      Serial.println("Sensor fault: motors stopped.");
      return;
    }
  } else {
    invalidAt = 0;
  }

  // All sensors are on white: line has been lost.
  if (totalStrength < 250) {
    if (!wasLost) {
      wasLost = true;
      lostAt = millis();
    }

    if (millis() - lostAt >= LOST_STOP_MS) {
      stopRobot();
      Serial.println("Line not recovered: stopped.");
      return;
    }

    // Search progressively farther, in last known direction.
    int search = SEARCH_SPEED;
    unsigned long lostDuration = millis() - lostAt;
    if (lostDuration > 400) search = 85;
    if (lostDuration > 800) search = 100;

    if (lastError < 0) {
      drive(-search, search);
    } else {
      drive(search, -search);
    }
    return;
  }

  wasLost = false;

  float error = (float)weightedSum / totalStrength / 1000.0f;

  // Detect a wide region using several strong sensors.
  bool wideRegion = active >= 6;

  if (wideRegion) {
    if (!junctionPending) {
      junctionPending = true;
      junctionAt = millis();
    }

    if (millis() - junctionAt >= JUNCTION_CONFIRM_MS) {
      junctionLatched = true;

      if (JUNCTION_POLICY == STOP_AT_JUNCTION) {
        stopRobot();
        Serial.println("Junction detected: stopped.");
        return;
      }

      drive(BASE_SPEED, BASE_SPEED);
      lastError = error;
      return;
    }
  } else {
    junctionPending = false;
    junctionLatched = false;
  }

  // Filter derivative to reduce sensor noise.
  float derivative = error - lastError;
  filteredDerivative =
      0.7f * filteredDerivative + 0.3f * derivative;

  float correction = KP * error + KD * filteredDerivative;
  lastError = error;

  // Slow down when the line is far from the centre.
  int speed = BASE_SPEED - (int)(abs(error) * 4.0f);
  speed = constrain(speed, MIN_SPEED, BASE_SPEED);

  int left = constrain(speed + (int)correction,
                       -MAX_SPEED, MAX_SPEED);
  int right = constrain(speed - (int)correction,
                        -MAX_SPEED, MAX_SPEED);

  drive(left, right);
}
