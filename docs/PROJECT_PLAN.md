# Plastic Bottle Coin Exchanger System (PCBCES)
## Capstone Thesis Project Plan & Hardware Architecture

---

### 1. Executive Summary & Feasibility Analysis

**Can we do this?**
**Yes, absolutely.** The proposed Reverse Vending Machine (RVM) concept with **GSM/SMS Notification** is a classic, high-scoring undergraduate capstone thesis project.

The system logic follows an intuitive sequence:
1. **Standby / Selection (Dedicated 3-Button Control):**
   - **Button Green:** Direct selection for **1.5L / 1.75L Mode** (5 bottles = ₱20.00).
   - **Button Blue:** Direct selection for **290 ML Mode** (10 bottles = ₱3.00).
   - **Button Red:** System Restart / Cancel current transaction at any time.
   - The 16x2 LCD displays: `GRN:1.5L BLU:290` / `RED:Cancel/Reset`.
2. **Deposit Progress Tracking:** The LCD displays live progress (e.g., `1.5L/1.75L(5pcs)` or `290 ML  (10 pcs)`). User can press Red at any point to cancel.
3. **Sensor Verification (Non-Contact Classification Architecture):**
   - **Metal Detection (LJ12A3-4-Z/BX):** Rejects metallic cans or foreign objects.
   - **Insertion Detection (IR Obstacle Sensor):** Detects object placed into the inspection cradle.
   - **Dimensional Discrimination (HC-SR04 Ultrasonic):** Mounted at the ceiling of the 43 cm vertical chamber, measuring top-down distance to the bottle cap to distinguish tall 1.5L / 1.75L bottles (7–15 cm to ceiling) from shorter 290 ML bottles (26–27 cm to ceiling). *(Note: LJC18A3 Capacitive Sensor and HX711 Load Cell omitted).*
4. **Accept / Reject Mechanism (MG996R Servo):**
   - **Valid Bottle:** Servo opens the trapdoor to 90° to drop the bottle into the internal storage bin; returns to 0° standby; count increments (e.g., `1/5`), green light blinks.
   - **Invalid Bottle:** Servo flap holds at 0° (stays closed); buzzer sounds alert, red light blinks, LCD prompts `"Pls Remove Item"` for customer manual retrieval from entry chute.
5. **Coin Payout:** Once the target count is satisfied (5 bottles for 1.5L/1.75L = ₱20.00 [20 coins] or 10 bottles for 290 ML = ₱3.00 [3 coins]), the system triggers the Coin Hopper (220V AC / 12V DC) to dispense the exact target, emits a completion beep, and resets to Standby.
6. **Bin Full Monitoring & GSM SMS Alert:**
   - When the internal bin fills up (detected via sensor and/or total bottle count capacity, e.g. 50 bottles), the system locks the chute, displays `"BIN FULL / SYSTEM PAUSED"` on the LCD, and **sends an SMS alert to the admin's phone number**:
     `"ALERT: PCBCES Storage Bin is FULL! Please empty the collection bin to resume operations."`

---

### 2. Complete Hardware Inventory & Requirements

| Component | Status | Notes / Role in System |
|---|---|---|
| **Arduino Uno** | Available | Main Microcontroller |
| **16x2 LCD + PCF8574 I2C Backpack** | Available (Soldered) | Uses A4 (SDA) and A5 (SCL) |
| **GSM SIM Module (e.g. SIM800L / SIM900A)** | **Newly Added** | Sends SMS alerts when bin is full to admin's phone |
| **Coin Hopper (220V AC / 12V DC)** | Available | Dispenses ₱1 coins (switched by 5V Relay, optical count on D7) |
| **5V Relay Module** | Available | Switches 220V AC Live (or 12V DC) to the Coin Hopper |
| **LJC18A3-B-Z/BX Capacitive Sensor** | **Omitted** | Omitted to prevent sensor drift & simplify wiring; D5 is spare |
| **LJ12A3-4-Z/BX Inductive Sensor** | Available | 12V powered, detects & rejects metal |
| **IR Obstacle Avoidance Sensor** | Available | Bottle chute entry trigger or bin level check |
| **HX711 Load Cell + 1kg Bar** | **Omitted / Retired** | Omitted in favor of high-reliability non-contact classification |
| **HC-SR04+ Ultrasonic Sensor** | Available | Bottle height differentiation or bin depth check |
| **MG996R Servo Motor** | Available | Flap / trapdoor sorting mechanism |
| **DIYMORE 2315 Active Buzzer** | Available | Audible error and success alerts |
| **Green & Red Indicator Lights** | Available | Visual status indicators |
| **LM2596 Buck Converter** | Available | Steps 12V down to 5V (and 4V for SIM800L) |
| **S-120-12 12V 10A PSU** | Available | Main power supply for entire machine |
| **Voltage Divider Resistors (10k, 4.7k, 2k)** | Available | Safely steps down 12V sensor/hopper signals to ~3.8V |
| **1000µF Capacitor & 1N4007 Diode** | Available | Power rail stabilization (vital for GSM bursts & servo) |
| **Breadboard & Power Distribution Block** | Available | Common 5V & GND distribution |
| **Dedicated Buttons (Green, Blue, Red)** | Available | Green (1.5L/1.75L), Blue (290 ML), Red (Cancel/Reset) |

---

### 3. GSM SIM Module Power & Wiring Considerations

> [!CAUTION]
> **CRITICAL GSM POWER REQUIREMENT (SIM800L):**
> - The SIM800L operates at **3.7V – 4.4V** (optimal: **4.0V**).
> - During SMS transmission and network handshake, it draws **up to 2A bursts of current**.
> - **DO NOT power SIM800L from the Arduino 5V or 3.3V pin** (it will cause immediate brownout, reboot, or network drop).
> - **Solution:** Use your **LM2596 Buck Converter** or insert a **1N4007 Diode** in series from 5V (which drops ~0.7V, giving 4.3V) with your **1000µF capacitor** directly across the SIM module's VCC and GND pins.
> - Always connect the SIM module GND to the Arduino Common GND.

---

### 4. Updated Arduino Uno Pin Allocation

Every pin on the Arduino Uno is carefully budgeted:

| Arduino Pin | Connected Component | Signal Type | Notes |
|---|---|---|---|
| **D0 (RX) / D1 (TX)** | Hardware Serial / USB | Serial Comm | Reserved for PC / USB Debugging / XAMPP |
| **D2 (Trig)** | HC-SR04 Ultrasonic | Digital Out | Bottle height check |
| **D3 (Echo)** | HC-SR04 Ultrasonic | Digital In | Bottle height echo: 1.5L near (7-15cm), 290 ML far (26-27cm) |
| **D4** | IR Obstacle Sensor | Digital In | Insertion trigger (detects bottle inserted) |
| **D5** | Spare / Unassigned GPIO | Unassigned | Reserved for future expansion (LJC18A3 omitted) |
| **D6** | LJ12A3 Inductive Sensor | Digital In | Via 10k/4.7k voltage divider (12V -> ~3.8V) |
| **D7** | Coin Hopper Pulse Line | Digital In (INT) | Via 10k/4.7k voltage divider (Counts ₱1 pulses) |
| **D8** | 5V Relay Module | Digital Out | Turns ON/OFF 12V Hopper motor |
| **D9 (PWM)** | MG996R Servo Motor | PWM Out | Trapdoor: 0° Standby / Reject, 90° Accept Drop |
| **D10** | Button Green (1.5L) | Digital In (PULLUP)| Selects 1.5L/1.75L mode (5 pcs quota = ₱20 payout) |
| **D11** | GSM SIM Module TX -> Arduino RX | SoftwareSerial RX | Receives AT response from SIM module |
| **D12** | Active Buzzer | Digital Out | High = Beep, Low = Silent |
| **D13** | Red LED / Bulb | Digital Out | Rejection / Error |
| **A0** | Button Blue (Mismo / 290 ML) | Digital In (PULLUP)| Selects 290 ML mode (10 pcs quota = ₱3 payout) |
| **A1** | Button Red (Restart/Cancel) | Digital In (PULLUP)| Cancels active transaction / resets system |
| **A2** | Green LED / Bulb | Digital Out | Acceptance / Ready |
| **A3** | Arduino TX -> GSM SIM Module RX | SoftwareSerial TX | Sends AT commands to SIM module |
| **A4 (SDA)** | 16x2 LCD (I2C Backpack) | I2C Data | 5V Logic |
| **A5 (SCL)** | 16x2 LCD (I2C Backpack) | I2C Clock | 5V Logic |

---

### 5. Bin Full Detection Strategy

To guarantee reliable bin full detection for your thesis presentation:
1. **Physical Level Sensor:** When bottles in the bin pile up and reach the top, the bin level sensor triggers.
2. **Software Capacity Counter (Safety Backup):** Every accepted bottle increments the internal count (e.g. `binBottleCount++`). If it reaches the maximum capacity (e.g., `MAX_BIN_CAPACITY = 30`), it automatically triggers the Bin Full state even if bottles settle unevenly.
3. **GSM SMS Trigger:**
   - Sends: `"ALERT: PCBCES Storage Bin is FULL (Count: 30)! Please empty the collection bin. Machine paused."`
   - Sets LCD message: `"BIN IS FULL" / "PLS EMPTY BIN"`.
   - Disables bottle acceptance until emptied / reset button pressed.

---

### 6. Recommended Project Structure

```text
PCBCES-Plastic-Bottle-Coin-Exchanger-System/
│
├── docs/
│   ├── PROJECT_PLAN.md              # Blueprint, feasibility, and architecture (this file)
│   ├── WIRING_PINOUT.md             # Complete step-by-step wiring guide
│   └── STATE_MACHINE.md             # System logic & flowchart for thesis paper
│
├── firmware/
│   └── pcbces_firmware/
│       ├── pcbces_firmware.ino       # Main Arduino sketch (setup & loop)
│       ├── config.h                 # Pin definitions, admin phone number & thresholds
│       ├── sensors.h / .cpp         # Bottle validation (weight, height, metal check)
│       ├── display.h / .cpp         # 16x2 I2C LCD screen management
│       ├── dispenser.h / .cpp       # 12V Coin Hopper pulse counting & relay control
│       └── gsm_notifier.h / .cpp    # GSM SIM module SMS sending & AT command driver
│
├── web/                             # (Optional Capstone Thesis Web Dashboard)
│   ├── index.php                    # Simple admin page: total bottles & coins dispensed
│   ├── db.php                       # Database connection
│   └── api/
│       └── log_transaction.php      # Logs transaction events
│
├── Note.md                          # Original project notes
└── README.md                        # Quick project overview
```
