// Local (no RF) L298N Differential Drive Test using Joystick
// Board: Arduino Uno

// ---- Joystick pins
const uint8_t PIN_VRX = A0;  // X axis (steer)
const uint8_t PIN_VRY = A1;  // Y axis (throttle)
const uint8_t PIN_SW  = 2;   // push button (active LOW with INPUT_PULLUP)

// ---- L298N Channel A (LEFT side)
const uint8_t ENA = 5;  // PWM
const uint8_t IN1 = 8;
const uint8_t IN2 = 7;

// ---- L298N Channel B (RIGHT side)
const uint8_t ENB = 6;  // PWM
const uint8_t IN3 = 4;
const uint8_t IN4 = 3;

// ---- Tuning
const bool INVERT_X = false;     // flip steer direction if needed
const bool INVERT_Y = true;      // forward stick = forward drive (to match final)
const bool INVERT_LEFT = false;  // if left motors spin opposite, set true
const bool INVERT_RIGHT = false; // if right motors spin opposite, set true

const int DEADZONE = 30;         // ADC counts around center
const uint8_t MIN_START_PWM = 45;// overcome static friction
const uint8_t MAX_PWM = 255;     // cap PWM

const uint8_t RAMP_STEP = 6;     // PWM counts per update
const uint16_t LOOP_MS = 20;     // ~50 Hz

// ---- State
int prevLeftPwm = 0;
int prevRightPwm = 0;

int toPercent(int raw, bool invert) {
  int centered = raw - 512;  // -512..+511 nominal
  if (abs(centered) < DEADZONE) centered = 0;
  float pct = (centered / 511.0f) * 100.0f; // -100..+100
  if (invert) pct = -pct;
  if (pct > 100) pct = 100;
  if (pct < -100) pct = -100;
  return (int)pct;
}
int clampPercent(int v) { if(v>100)return 100; if(v<-100)return -100; return v; }

uint8_t pctToPwm(int pct) {
  int apct = abs(pct);
  if (apct == 0) return 0;
  int pwm = (apct * MAX_PWM) / 100; // linear
  if (MIN_START_PWM > 0 && pwm < MIN_START_PWM) pwm = MIN_START_PWM;
  if (pwm > MAX_PWM) pwm = MAX_PWM;
  return (uint8_t)pwm;
}
int slew(int current, int target, int step) {
  if (current < target) { current += step; if (current > target) current = target; }
  else if (current > target) { current -= step; if (current < target) current = target; }
  return current;
}

void driveChannel(uint8_t en, uint8_t inA, uint8_t inB, int pct, bool invertSide, int &prevPwm, bool brakeAtZero) {
  if (invertSide) pct = -pct;
  uint8_t targetPwm = pctToPwm(pct);
  int newPwm = slew(prevPwm, targetPwm, RAMP_STEP);
  prevPwm = newPwm;

  if (pct > 0)       { digitalWrite(inA, HIGH); digitalWrite(inB, LOW); }
  else if (pct < 0 ) { digitalWrite(inA, LOW);  digitalWrite(inB, HIGH); }
  else {
    if (brakeAtZero) { digitalWrite(inA, HIGH); digitalWrite(inB, HIGH); } // brake
    else             { digitalWrite(inA, LOW);  digitalWrite(inB, LOW);  } // coast
  }
  analogWrite(en, newPwm);
}

void setup() {
  pinMode(PIN_SW, INPUT_PULLUP);

  pinMode(ENA, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  analogWrite(ENA, 0); analogWrite(ENB, 0);

  Serial.begin(115200);
  Serial.println(F("Local L298N test ready. Raise wheels."));
}

void loop() {
  static unsigned long lastLoop = 0;
  unsigned long now = millis();
  if (now - lastLoop < LOOP_MS) return;
  lastLoop = now;

  int xRaw = analogRead(PIN_VRX);
  int yRaw = analogRead(PIN_VRY);
  int STR = toPercent(xRaw, INVERT_X);
  int THR = toPercent(yRaw, INVERT_Y);

  int leftPct  = clampPercent(THR + STR);
  int rightPct = clampPercent(THR - STR);

  bool brakeAtZero = true;
  driveChannel(ENA, IN1, IN2, leftPct,  INVERT_LEFT,  prevLeftPwm,  brakeAtZero);
  driveChannel(ENB, IN3, IN4, rightPct, INVERT_RIGHT, prevRightPwm, brakeAtZero);

  // Debug
  static uint8_t dbg=0;
  if (++dbg >= (1000/(5*LOOP_MS))) {
    dbg=0;
    Serial.print(F("THR=")); Serial.print(THR);
    Serial.print(F(" STR=")); Serial.print(STR);
    Serial.print(F(" L="));   Serial.print(leftPct);
    Serial.print(F(" R="));   Serial.println(rightPct);
  }
}
