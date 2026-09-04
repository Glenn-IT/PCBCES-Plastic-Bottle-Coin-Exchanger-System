# Hardware Wiring & Power Distribution Guide
## PCBCES Capstone Project (with GSM Module)

---

### 1. Power Architecture (CRITICAL)

You have two main power rails + dedicated supply for the GSM module:
1. **12V DC Rail (High Current)**: Supplied by S-120-12 (120W 12V 10A PSU).
   - Powers the **Coin Hopper Motor** (switched via Relay).
   - Powers the **LJC18A3** capacitive proximity sensor.
   - Powers the **LJ12A3** inductive proximity sensor.
   - Powers the **LM2596 Buck Converter** input.
2. **5V DC Rail (Regulated)**: Supplied by **LM2596 Buck Converter** output (tuned with a multimeter to **5.0V**).
   - Powers **Arduino Uno** (into `5V` pin).
   - Powers **MG996R Servo Motor** (MUST NOT be powered from Arduino 5V board pin; use the 5V bus).
   - Powers **16x2 LCD I2C Backpack**, **HC-SR04**, **IR sensor**, **Buzzer**, and **Relay coil**.
3. **GSM SIM Module Power (SIM800L: 3.7V – 4.4V)**:
   - **DO NOT** connect to Arduino 5V or 3.3V pin (it will drop network or cause Arduino to reboot during SMS transmit).
   - **Best Method:** From the 5V rail, put a **1N4007 Diode** in series (Anode to 5V, Cathode to SIM800L VCC). This drops ~0.7V, giving a clean ~4.3V.
   - Connect your **1000µF 16V capacitor** directly across SIM800L `VCC` and `GND` to supply the 2A instantaneous burst current required for GSM transmission.

> [!IMPORTANT]
> **COMMON GROUND:** The negative (`-V` / `GND`) of the 12V power supply, the output GND of the LM2596 buck converter, the GSM module GND, and all Arduino `GND` pins **MUST be connected together**.

---

### 2. Voltage Divider Wiring (12V Sensors to 5V Arduino)

The `LJ12A3`, `LJC18A3`, and the Coin Hopper pulse line output 12V logic. Use your **10kΩ** and **4.7kΩ** resistors to step the voltage down safely:

```text
Sensor Signal Wire (12V) ───[ 10kΩ Resistor ]───┬───> Arduino Digital Pin (~3.8V safe)
                                                │
                                         [ 4.7kΩ Resistor ]
                                                │
                                             Common GND
```

---

### 3. Complete Step-by-Step Wiring Table

| Component | Pin / Wire | Connects To | Notes |
|---|---|---|---|
| **16x2 LCD (I2C)** | VCC | 5V Bus | From LM2596 5V rail |
| | GND | Common GND | |
| | SDA | Arduino **A4** | Hardware I2C SDA |
| | SCL | Arduino **A5** | Hardware I2C SCL |
| **Pins A0 & A1** | Header | Free / Available | Reserved for expansion (Weight sensor removed for durability) |
| **HC-SR04 Ultrasonic** | VCC | 5V Bus | |
| | GND | Common GND | |
| | Trig | Arduino **D2** | Trigger pulse |
| | Echo | Arduino **D3** | Echo return (height) |
| **IR Obstacle Sensor** | VCC | 5V Bus | |
| | GND | Common GND | |
| | OUT | Arduino **D4** | Active LOW when bottle blocks beam |
| **LJC18A3 Capacitive** | Brown (+V) | 12V Rail | |
| | Blue (GND) | Common GND | |
| | Black (Signal) | Voltage Divider (10k/4.7k) -> Arduino **D5** | Non-metal/plastic presence |
| **LJ12A3 Inductive** | Brown (+V) | 12V Rail | |
| | Blue (GND) | Common GND | |
| | Black (Signal) | Voltage Divider (10k/4.7k) -> Arduino **D6** | Metal rejection trigger |
| **12V Coin Hopper** | 12V VCC | 12V Rail via Relay (COM to 12V+, NO to Hopper+) | Switched power |
| | GND | Common GND | |
| | Coin Signal | Voltage Divider (10k/4.7k) -> Arduino **D7** | 1 pulse = 1 coin dispensed |
| **5V Relay Module** | VCC | 5V Bus | |
| | GND | Common GND | |
| | IN | Arduino **D8** | Relay trigger |
| **MG996R Servo** | Red (+V) | 5V Bus (with 1000µF capacitor across + & -) | High torque servo |
| | Brown (GND) | Common GND | |
| | Orange (Signal)| Arduino **D9 (PWM)** | Controls trapdoor gate |
| **Push Button** | Leg 1 | Arduino **D10** | Single-button smart menu (`INPUT_PULLUP`) |
| | Leg 2 | Common GND | |
| **GSM SIM Module** | VCC | 4.3V Rail (via 1N4007 from 5V + 1000µF cap) | Needs 3.7V - 4.4V with 2A burst |
| | GND | Common GND | |
| | SIM TX | Arduino **D11 (SoftwareSerial RX)** | Receives AT responses |
| | SIM RX | Arduino **A3 (SoftwareSerial TX)** (via 1k/2k divider or direct) | Sends AT commands |
| **Active Buzzer** | Positive (+) | Arduino **D12** | Active buzzer beeps when HIGH |
| | Negative (-) | Common GND | |
| **Red Light / LED** | Anode (+) | Arduino **D13** (via 220Ω resistor) | Rejection indicator |
| | Cathode (-) | Common GND | |
| **Green Light / LED** | Anode (+) | Arduino **A2** (via 220Ω resistor) | Acceptance indicator |
| | Cathode (-) | Common GND | |

---

### 4. GSM SIM Module Operating Tips for Philippines (PH)

1. **SIM Card Type:** Use a standard **Smart / TNT** or **Globe / TM** prepaid nano/micro SIM with an adapter.
2. **SIM Load:** Ensure the SIM has at least ₱10–₱20 regular load or an active unli-text promo so AT commands can successfully dispatch SMS.
3. **Network LED Indicator on SIM Module:**
   - Fast blink (every 1 sec): Searching for 2G network.
   - Slow blink (every 3 sec): Successfully registered to network! (Ready to send SMS).
