// 433MHz ASK Joystick TX (FS1000A) -> RH_ASK
// Board: Arduino Uno (Transmitter)
// Pins: FS1000A DATA->D12, VCC->5V, GND->GND
//       Joystick: VRx->A0, VRy->A1, SW->D2 (INPUT_PULLUP)
#include <RH_ASK.h>
#include <SPI.h> // Some cores require this even if unused

// RH_ASK(speed, rxPin, txPin, pttPin, pttInverted)
// Using defaults RX=11, TX=12, PTT=10. We use TX=12 here.
RH_ASK driver(2000, 11, 12, 10, false);

const uint8_t PIN_VRX = A0; // steer
const uint8_t PIN_VRY = A1; // throttle
const uint8_t PIN_SW  = 2;  // push button (active LOW with pullup)

struct __attribute__((packed)) JoyPacket {
  uint16_t x;   // 0..1023 (VRx)
  uint16_t y;   // 0..1023 (VRy)
  uint8_t  sw;  // 1=pressed, 0=released
};

void setup() {
  pinMode(PIN_SW, INPUT_PULLUP);
  Serial.begin(115200);
  if (!driver.init()) {
    Serial.println(F("RH_ASK init failed (TX)"));
  } else {
    Serial.println(F("TX ready"));
  }
}

void loop() {
  JoyPacket pkt;
  pkt.x  = analogRead(PIN_VRX);
  pkt.y  = analogRead(PIN_VRY);
  pkt.sw = (digitalRead(PIN_SW) == LOW) ? 1 : 0;

  driver.send(reinterpret_cast<const uint8_t*>(&pkt), sizeof(pkt));
  driver.waitPacketSent();

  // Debug (optional)
  Serial.print(F("Sent x=")); Serial.print(pkt.x);
  Serial.print(F(" y="));     Serial.print(pkt.y);
  Serial.print(F(" sw="));    Serial.println(pkt.sw);

  delay(50); // ~20 packets/sec
}
