# Wiring & Pin Map

## Transmitter (UNO + Joystick + FS1000A)

| Module | Signal | UNO Pin |
|---|---|---|
| Joystick | VRx | A0 |
| Joystick | VRy | A1 |
| Joystick | SW  | D2 (INPUT_PULLUP) |
| FS1000A  | DATA | D12 |
| FS1000A  | VCC  | 5V |
| FS1000A  | GND  | GND |
| (optional) | ANT | ~17.3 cm wire |

## Receiver (UNO + 433 MHz RX + L298N)

| Module | Signal | UNO Pin |
|---|---|---|
| 433 MHz RX | DATA | D11 |
| 433 MHz RX | VCC  | 5V (buck) |
| 433 MHz RX | GND  | GND |
| L298N | ENA | D5 (PWM) ← *remove ENA jumper* |
| L298N | IN1 | D8 |
| L298N | IN2 | D7 |
| L298N | ENB | D6 (PWM) ← *remove ENB jumper* |
| L298N | IN3 | D4 |
| L298N | IN4 | D3 |

**Power**
- Battery +12V → L298N +12V  
- Battery − → L298N GND  
- Buck 5V → UNO 5V (RX) + RX module  
- Common GND between UNO, L298N, and buck (star point)

## ASCII Layout (Receiver side)

```
[ Battery 12V ]----(+12V)----[ L298N ]---- OUT1/2 -> Left motors
       |                      |             OUT3/4 -> Right motors
      GND---------------------+-- GND
                              |
                           [ Buck 5V ]----> UNO 5V + RX VCC
                              |
                             GND
```


[acess the full ckt here](https://app.cirkitdesigner.com/project/091b2a7f-d129-472b-a75a-bb8575c8d020)

## Notes
- Add ≥1000 µF across L298N +12V/GND, 470–1000 µF at buck input, and 0.1 µF near each module.
- Twist each motor’s two wires and keep them away from the RF receiver & antenna.
- Keep TX and RX antennas vertical and at least 30 cm apart while testing.
