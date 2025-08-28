// Joystick serial monitor demo (2-axis + switch)
// Board: Arduino Uno (local test)
const uint8_t PIN_VRX = A0;  // X axis (left/right)
const uint8_t PIN_VRY = A1;  // Y axis (forward/back)
const uint8_t PIN_SW  = 2;   // push button (active LOW with INPUT_PULLUP)

const bool INVERT_X = false;
const bool INVERT_Y = false;
const int DEADZONE = 30; // ~3%

int toPercent(int raw, bool invert) {
  int centered = raw - 512;              // roughly -512..+511
  if (abs(centered) < DEADZONE) centered = 0;
  float pct = (centered / 511.0) * 100;  // -100..+100
  if (invert) pct = -pct;
  if (pct > 100) pct = 100;
  if (pct < -100) pct = -100;
  return (int)pct;
}

void setup() {
  pinMode(PIN_SW, INPUT_PULLUP);   // pressed = LOW
  Serial.begin(115200);
  delay(50);
  Serial.println(F("Joystick Monitor (X,Y,SW). Move stick; press to see 'SW: PRESS'."));
}

void loop() {
  static unsigned long lastPrintMs = 0;
  static int lastSw = HIGH;

  int sw = digitalRead(PIN_SW);
  if (lastSw == HIGH && sw == LOW) {
    Serial.println(F("SW: PRESS"));
  }
  lastSw = sw;

  unsigned long now = millis();
  if (now - lastPrintMs >= 100) {
    lastPrintMs = now;

    int xRaw = analogRead(PIN_VRX);
    int yRaw = analogRead(PIN_VRY);

    int xPct = toPercent(xRaw, INVERT_X);
    int yPct = toPercent(yRaw, INVERT_Y);

    Serial.print(F("X(raw=")); Serial.print(xRaw);
    Serial.print(F(",%="));    Serial.print(xPct);
    Serial.print(F(")  Y(raw=")); Serial.print(yRaw);
    Serial.print(F(",%="));      Serial.print(yPct);
    Serial.print(F(")  THR="));  Serial.print(yPct);
    Serial.print(F("%  STR="));  Serial.print(xPct);
    Serial.println(F("%"));
  }
}
