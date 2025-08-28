// 433MHz ASK Joystick RX -> L298N Motor Control (Differential/Tank)
// Board: Arduino Uno (Receiver)
// RF RX DATA -> D11, VCC -> 5V, GND -> GND
// L298N: ENA->D5 (PWM), IN1->D8, IN2->D7, ENB->D6 (PWM), IN3->D4, IN4->D3

#include <RH_ASK.h>
#include <SPI.h>

RH_ASK driver(2000, 11, 12, 10, false);

// Packet from TX
struct __attribute__((packed)) JoyPacket {
  uint16_t x;   // 0..1023 (VRx)
  uint16_t y;   // 0..1023 (VRy)
  uint8_t  sw;  // 1=pressed, 0=released
};

// L298N pins (your map)
const uint8_t ENA = 5;  // LEFT  PWM
const uint8_t IN1 = 8;
const uint8_t IN2 = 7;
const uint8_t ENB = 6;  // RIGHT PWM
const uint8_t IN3 = 4;
const uint8_t IN4 = 3;

// ---- Tuning ----
const bool INVERT_X     = false;  // steer
const bool INVERT_Y     = true;   // forward stick = forward drive
const bool INVERT_LEFT  = false;
const bool INVERT_RIGHT = false;

const int  DEADZONE         = 30;
const uint8_t MIN_START_PWM = 45;
const uint8_t MAX_PWM       = 255;

const uint8_t  RAMP_STEP      = 6;     // PWM counts per tick
const uint16_t LOOP_MS        = 20;    // ~50 Hz control
const uint16_t FAILSAFE_MS    = 300;   // lost-link -> zero command
const uint16_t BRAKE_AFTER_MS = 900;   // after idle, apply brake at zero

// State
int prevLeftPwm  = 0;
int prevRightPwm = 0;
int targetLeftPct  = 0;  // -100..100
int targetRightPct = 0;  // -100..100
unsigned long lastRxMs = 0;

static inline int clampPercent(int v){ if(v>100)return 100; if(v<-100)return -100; return v; }

int toPercent(uint16_t raw, bool invert){
  int centered = (int)raw - 512;
  if (abs(centered) < DEADZONE) centered = 0;
  float pct = (centered / 511.0f) * 100.0f;
  if (invert) pct = -pct;
  if (pct>100)pct=100; if (pct<-100)pct=-100;
  return (int)pct;
}

uint8_t pctToPwm(int pct){
  int apct = abs(pct);
  if (apct == 0) return 0;
  int pwm = (apct * MAX_PWM) / 100;
  if (MIN_START_PWM>0 && pwm<MIN_START_PWM) pwm = MIN_START_PWM;
  if (pwm > MAX_PWM) pwm = MAX_PWM;
  return (uint8_t)pwm;
}

int slew(int current, int target, int step){
  if (current < target) { current += step; if (current>target) current = target; }
  else if (current > target) { current -= step; if (current<target) current = target; }
  return current;
}

void driveChannel(uint8_t en, uint8_t inA, uint8_t inB, int pct, bool invertSide, int &prevPwm, bool brakeAtZero){
  if (invertSide) pct = -pct;
  uint8_t targetPwm = pctToPwm(pct);
  int newPwm = slew(prevPwm, targetPwm, RAMP_STEP);
  prevPwm = newPwm;

  if (pct > 0)       { digitalWrite(inA,HIGH); digitalWrite(inB,LOW);  }
  else if (pct < 0 ) { digitalWrite(inA,LOW);  digitalWrite(inB,HIGH); }
  else {
    if (brakeAtZero) { digitalWrite(inA,HIGH); digitalWrite(inB,HIGH); }  // brake
    else             { digitalWrite(inA,LOW);  digitalWrite(inB,LOW);  }  // coast
  }
  analogWrite(en, newPwm);
}

void setup(){
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ENA,OUTPUT); pinMode(IN1,OUTPUT); pinMode(IN2,OUTPUT);
  pinMode(ENB,OUTPUT); pinMode(IN3,OUTPUT); pinMode(IN4,OUTPUT);
  digitalWrite(IN1,LOW); digitalWrite(IN2,LOW);
  digitalWrite(IN3,LOW); digitalWrite(IN4,LOW);
  analogWrite(ENA,0); analogWrite(ENB,0);

  Serial.begin(115200);
  if (!driver.init()) Serial.println(F("RH_ASK init failed (RX)"));
  else                Serial.println(F("RX ready"));

  lastRxMs = millis();
}

void loop(){
  // 1) Receive as fast as possible
  uint8_t buf[RH_ASK_MAX_MESSAGE_LEN]; uint8_t buflen = sizeof(buf);
  if (driver.recv(buf, &buflen)) {
    if (buflen == sizeof(JoyPacket)) {
      JoyPacket pkt; memcpy(&pkt, buf, sizeof(pkt));
      lastRxMs = millis();
      int STR = toPercent(pkt.x, INVERT_X);
      int THR = toPercent(pkt.y, INVERT_Y);
      targetLeftPct  = clampPercent(THR + STR);
      targetRightPct = clampPercent(THR - STR);
    }
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }

  // 2) Failsafe on idle link
  unsigned long now = millis();
  bool linkIdle = (now - lastRxMs > FAILSAFE_MS);
  if (linkIdle) { targetLeftPct = 0; targetRightPct = 0; }

  // 3) Control update
  static unsigned long lastLoop=0;
  if (now - lastLoop >= LOOP_MS) {
    lastLoop = now;
    bool brakeAtZero = (now - lastRxMs > BRAKE_AFTER_MS);
    driveChannel(ENA, IN1, IN2, targetLeftPct,  INVERT_LEFT,  prevLeftPwm,  brakeAtZero);
    driveChannel(ENB, IN3, IN4, targetRightPct, INVERT_RIGHT, prevRightPwm, brakeAtZero);

    // Debug ~5 Hz
    static uint8_t dbg=0;
    if (++dbg >= (1000/(5*LOOP_MS))) {
      dbg=0;
      Serial.print(F("L=")); Serial.print(targetLeftPct);
      Serial.print(F("% R=")); Serial.print(targetRightPct);
      Serial.print(F("% PWM(")); Serial.print(prevLeftPwm);
      Serial.print(F(",")); Serial.print(prevRightPwm);
      Serial.print(F(") linkIdle=")); Serial.println(linkIdle?F("Y"):F("N"));
    }
  }
}
