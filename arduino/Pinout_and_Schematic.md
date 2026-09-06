# PCBCES — Hardware Pinout & Circuit Schematic Specifications
## Plastic Bottle Coin Exchanger System (Capstone Thesis)

---

### 1. Power Distribution Overview

| Rail / Source | Regulated Voltage | Peak Current Capacity | Connected Modules | Protection Hardware |
|---|---|---|---|---|
| **S-120-12 PSU** | **12.00V DC** | **10.0 Amps** | 12V Coin Hopper Motor, LJ12A3 Inductive, Buck IN | 1N4007 Diode across relay/hopper |
| **LM2596 Buck** | **5.00V DC** | **3.0 Amps** | Arduino Uno 5V, MG996R Servo, I2C LCD, HC-SR04, IR, Buzzer | 1000µF 16V Decoupling Capacitor |
| **Diode Drop Rail** | **4.30V DC** | **2.0A Burst** | SIM800L GSM Module (VCC & GND) | 1N4007 + 1000µF Low-ESR Capacitor |
| **Star Ground Rail**| **0.00V (GND)** | N/A | PSU (V-), Buck (OUT-), Arduino (GND), All Sensor Grounds | Single-point common star topology |

---

### 2. Complete Arduino Uno 20-Pin Allocation

| Pin | Type | Device Attached | Voltage Domain | Primary Role | Bench Test Reference |
|---|---|---|---|---|---|
| **D0 / D1** | Hardware UART | USB Serial | 5V TTL | Reserved for PC Serial Monitor / Web Bridge | [Serial Monitor](file:///C:/xampp/htdocs/PCBCES-Plastic-Bottle-Coin-Exchanger-System/arduino/index.html) |
| **D2** | Digital Output | HC-SR04 Trigger | 5V Logic | 10µs ultrasonic pulse down into 43cm chamber | [Test 03 Guide](file:///C:/xampp/htdocs/PCBCES-Plastic-Bottle-Coin-Exchanger-System/arduino/03_ultrasonic_ir_dimension_test/wiring_guide.html) |
| **D3** | Digital Input | HC-SR04 Echo | 5V Logic | Top-down echo to bottle cap: 1.5L/1.75L near (7-15cm), 290 ML far (26-27cm) | [Test 03 Guide](file:///C:/xampp/htdocs/PCBCES-Plastic-Bottle-Coin-Exchanger-System/arduino/03_ultrasonic_ir_dimension_test/wiring_guide.html) |
| **D4** | Digital Input | IR Obstacle Avoidance | 5V Logic | Active LOW bottle insertion beam detector | [Test 03 Guide](file:///C:/xampp/htdocs/PCBCES-Plastic-Bottle-Coin-Exchanger-System/arduino/03_ultrasonic_ir_dimension_test/wiring_guide.html) |
| **D5** | Unassigned / Spare | Spare GPIO | 5V Logic | Reserved for future expansion (LJC18A3 Capacitive omitted) | [Test 04 Guide](file:///C:/xampp/htdocs/PCBCES-Plastic-Bottle-Coin-Exchanger-System/arduino/04_proximity_metal_plastic_test/wiring_guide.html) |
| **D6** | Digital Input | LJ12A3 Inductive Metal | Scaled to ~3.8V | Detects metallic objects / instant reject | [Test 04 Guide](file:///C:/xampp/htdocs/PCBCES-Plastic-Bottle-Coin-Exchanger-System/arduino/04_proximity_metal_plastic_test/wiring_guide.html) |
| **D7** | Digital Input | Coin Hopper Pulse Line | 5V TTL / ~3.8V | Optical falling edge pulse counter (1 pulse = ₱1) | [Test 06 Guide](file:///C:/xampp/htdocs/PCBCES-Plastic-Bottle-Coin-Exchanger-System/arduino/06_coin_hopper_relay_test/wiring_guide.html) |
| **D8** | Digital Output | 5V Relay Module | 5V Logic | Switches 220V AC Live (or 12V DC) to Coin Hopper motor | [Test 06 Guide](file:///C:/xampp/htdocs/PCBCES-Plastic-Bottle-Coin-Exchanger-System/arduino/06_coin_hopper_relay_test/wiring_guide.html) |
| **D9** | Digital Output (PWM)| MG996R Servo Motor | 5V Logic | 0° Standby / Reject, 90° Accept Drop | [Test 05 Guide](file:///C:/xampp/htdocs/PCBCES-Plastic-Bottle-Coin-Exchanger-System/arduino/05_mg996r_servo_trapdoor_test/wiring_guide.html) |
| **D10** | Digital Input (PULL)| Button Green (1.5L) | 5V Logic | Direct selection for 1.5L / 1.75L Mode (5 pcs quota = ₱20.00) | [Test 01 Guide](file:///C:/xampp/htdocs/PCBCES-Plastic-Bottle-Coin-Exchanger-System/arduino/01_lcd_button_menu_test/wiring_guide.html) |
| **D11** | SoftwareSerial RX | SIM800L TX Pin | 4.0V - 4.3V Logic | Receives AT command responses | [Test 07 Guide](file:///C:/xampp/htdocs/PCBCES-Plastic-Bottle-Coin-Exchanger-System/arduino/07_sim800l_gsm_sms_test/wiring_guide.html) |
| **D12** | Digital Output | Active Buzzer | 5V Logic | Audio prompts (Success beep / Alarm tone) | [Test 01 Guide](file:///C:/xampp/htdocs/PCBCES-Plastic-Bottle-Coin-Exchanger-System/arduino/01_lcd_button_menu_test/wiring_guide.html) |
| **D13** | Digital Output | Red Indicator LED | 5V Logic | Visual rejection warning / system fault alert | [Test 04 Guide](file:///C:/xampp/htdocs/PCBCES-Plastic-Bottle-Coin-Exchanger-System/arduino/04_proximity_metal_plastic_test/wiring_guide.html) |
| **A0** | Digital Input (PULL)| Button Blue (Mismo / 290 ML) | 5V Logic | Direct selection for 290 ML Mode (10 pcs quota = ₱3.00)| [Test 01 Guide](file:///C:/xampp/htdocs/PCBCES-Plastic-Bottle-Coin-Exchanger-System/arduino/01_lcd_button_menu_test/wiring_guide.html) |
| **A1** | Digital Input (PULL)| Button Red (Restart/Cancel)| 5V Logic | Cancels transaction, resets count & restarts | [Test 01 Guide](file:///C:/xampp/htdocs/PCBCES-Plastic-Bottle-Coin-Exchanger-System/arduino/01_lcd_button_menu_test/wiring_guide.html) |
| **A2** | Digital Output | Green Indicator LED | 5V Logic | Visual acceptance / ready status | [Test 04 Guide](file:///C:/xampp/htdocs/PCBCES-Plastic-Bottle-Coin-Exchanger-System/arduino/04_proximity_metal_plastic_test/wiring_guide.html) |
| **A3** | SoftwareSerial TX | SIM800L RX Pin | 4.0V - 4.3V Logic | Sends AT SMS dispatch commands | [Test 07 Guide](file:///C:/xampp/htdocs/PCBCES-Plastic-Bottle-Coin-Exchanger-System/arduino/07_sim800l_gsm_sms_test/wiring_guide.html) |
| **A4** | Hardware I2C (SDA) | 16x2 LCD (PCF8574) | 5V Logic | Serial Data communication (Addr 0x27) | [Test 01 Guide](file:///C:/xampp/htdocs/PCBCES-Plastic-Bottle-Coin-Exchanger-System/arduino/01_lcd_button_menu_test/wiring_guide.html) |
| **A5** | Hardware I2C (SCL) | 16x2 LCD (PCF8574) | 5V Logic | Serial Clock synchronization | [Test 01 Guide](file:///C:/xampp/htdocs/PCBCES-Plastic-Bottle-Coin-Exchanger-System/arduino/01_lcd_button_menu_test/wiring_guide.html) |

---

### 3. Resistor Divider Circuit for 12V Signals

```text
Sensor Signal Wire (12V) ───[ 10kΩ Resistor ]───┬───> Arduino Digital Pin (~3.83V safe)
                                                │
                                         [ 4.7kΩ Resistor ]
                                                │
                                             Common GND
```

> **Calculation**: $V_{\text{out}} = 12\text{V} \times \frac{4.7\text{k}\Omega}{10\text{k}\Omega + 4.7\text{k}\Omega} = 12\text{V} \times 0.3197 = 3.836\text{V}$ (Safe for ATmega328P 5V CMOS inputs).

---

### 4. Architectural Notes

1. **Dedicated 3-Button User Interface**:
   - `Green Button (D10)`: Instantly starts **1.5L / 1.75L Mode** (5 bottles = ₱20.00 payout).
   - `Blue Button (A0)`: Instantly starts **290 ML Mode** (10 bottles = ₱3.00 payout).
   - `Red Button (A1)`: Aborts active transaction, resets count to 0, returns trapdoor to standby, and displays reset confirmation.
2. **Archived Load Cell Feature (Test 02)**:
   - The HX711 1kg load cell was archived in favor of ultrasonic non-contact length profiling and inductive metal rejection to prevent mechanical wear and calibration drift. (LJC18A3 capacitive sensor omitted; D5 spare).
   - Pins `A0` and `A1` were permanently reallocated to the **Blue** and **Red** control buttons in the production machine.
3. **Master Interactive Controller**:
   - Interactive full schematic, wiring step-by-step checklist, and state machine firmware are available at [`arduino/pcbces_arduino_controller/wiring_guide.html`](file:///C:/xampp/htdocs/PCBCES-Plastic-Bottle-Coin-Exchanger-System/arduino/pcbces_arduino_controller/wiring_guide.html).