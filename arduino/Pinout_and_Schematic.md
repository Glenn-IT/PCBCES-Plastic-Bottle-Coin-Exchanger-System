# PCBCES — Hardware Pinout & Circuit Schematic Specifications
## Plastic Bottle Coin Exchanger System (Capstone Thesis)

---

### 1. Power Distribution Overview

| Rail / Source | Regulated Voltage | Peak Current Capacity | Connected Modules | Protection Hardware |
|---|---|---|---|---|
| **S-120-12 PSU** | **12.00V DC** | **10.0 Amps** | 12V Coin Hopper Motor, LJC18A3 Capacitive, LJ12A3 Inductive, Buck IN | 1N4007 Diode across relay/hopper |
| **LM2596 Buck** | **5.00V DC** | **3.0 Amps** | Arduino Uno 5V, MG996R Servo, I2C LCD, HX711, HC-SR04, IR, Buzzer | 1000µF 16V Decoupling Capacitor |
| **Diode Drop Rail** | **4.30V DC** | **2.0A Burst** | SIM800L GSM Module (VCC & GND) | 1N4007 + 1000µF Low-ESR Capacitor |
| **Star Ground Rail**| **0.00V (GND)** | N/A | PSU (V-), Buck (OUT-), Arduino (GND), All Sensor Grounds | Single-point common star topology |

---

### 2. Complete Arduino Uno 20-Pin Allocation

| Pin | Type | Device Attached | Voltage Domain | Notes |
|---|---|---|---|---|
| **D0 / D1** | Hardware UART | USB Serial | 5V TTL | Reserved for PC Serial Monitor / Debugging |
| **D2** | Digital Output | HC-SR04 Trigger | 5V Logic | 10µs ultrasonic trigger pulse |
| **D3** | Digital Input | HC-SR04 Echo | 5V Logic | Echo pulse width corresponds to height |
| **D4** | Digital Input | IR Obstacle Avoidance | 5V Logic | Active LOW when bottle blocks beam |
| **D5** | Digital Input | LJC18A3 Capacitive | Scaled to ~3.8V | Connected via 10kΩ/4.7kΩ voltage divider |
| **D6** | Digital Input | LJ12A3 Inductive Metal | Scaled to ~3.8V | Connected via 10kΩ/4.7kΩ voltage divider |
| **D7** | Digital Input (INT) | Coin Hopper Pulse Line | Scaled to ~3.8V | Interrupt counter for dispensed ₱1 coins |
| **D8** | Digital Output | 5V Relay Module | 5V Logic | Switches 12V power to Coin Hopper |
| **D9** | Digital Output (PWM)| MG996R Servo Motor | 5V Logic | 0° Standby, 90° Accept, 180° Reject |
| **D10** | Digital Input (PULL)| Momentary Push Button | 5V Logic | Short click = toggle, 1.5s hold = select |
| **D11** | SoftwareSerial RX | SIM800L TX Pin | 5V / 4V Logic | Receives AT responses |
| **D12** | Digital Output | DIYMORE Active Buzzer | 5V Logic | High = Beep, Low = Silent |
| **D13** | Digital Output | Red Indicator Light | 5V Logic | Visual rejection / fault alert |
| **A0** | Digital Input (DT) | HX711 24-Bit ADC | 5V Logic | Weight data stream |
| **A1** | Digital Output (SCK)| HX711 Clock | 5V Logic | Weight clock synchronization |
| **A2** | Digital Output | Green Indicator Light | 5V Logic | Visual acceptance / ready alert |
| **A3** | SoftwareSerial TX | SIM800L RX Pin | 5V / 4V Logic | Sends AT SMS commands |
| **A4** | Hardware I2C (SDA) | 16x2 LCD (PCF8574) | 5V Logic | Serial Data line |
| **A5** | Hardware I2C (SCL) | 16x2 LCD (PCF8574) | 5V Logic | Serial Clock line |

---

### 3. Resistor Divider Circuit for 12V Signals

```text
Sensor Signal Wire (12V) ───[ 10kΩ Resistor ]───┬───> Arduino Digital Pin (~3.83V safe)
                                                │
                                         [ 4.7kΩ Resistor ]
                                                │
                                             Common GND
```