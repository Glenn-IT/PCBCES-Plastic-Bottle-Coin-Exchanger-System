# PCBCES — Hardware Bench-to-Chassis Calibration Guide (Tests 01, 04, 05, 06, 07)

> **Document:** `notetest_other_hardware.md`  
> **Purpose:** Detailed calibration, electrical protection, and mechanical installation guidelines when transferring bench-tested hardware modules into the permanent reverse vending machine chassis.  
> **Last Updated:** 2026-09-06 15:23:00 (+08:00)

---

## 1. Test 01: 16x2 I2C LCD & 3 Dedicated Pushbuttons

### Hardware Components:
* 16x2 Blue Character LCD with PCF8574 I2C backpack (`0x27`)
* 3 Industrial / Arcade Pushbuttons (Green D10, Blue A0, Red A1) with internal pullup resistors to GND

### Real Chassis Installation & Calibration:
1. **LCD Contrast Adjustment:**
   - On the rear of the I2C backpack, turn the small blue trimmer potentiometer with a small screwdriver until characters are razor-sharp with no ghosting or block fading under indoor/outdoor chassis lighting.
2. **Button Wire Harness & Noise Filtering:**
   - When routing button wires from the front door panel to the Arduino Uno, twist each signal wire with a GND wire (twisted pair).
   - If wire runs exceed 50 cm, electrical noise from the hopper motor or relay can cause ghost presses. The master firmware already includes a **50 ms debounce delay**, but keeping wires shielded or twisted is critical.
3. **Physical Button Layout Standard:**
   - **GREEN (D10):** 1.5L / 1.75L Bottle Mode (5 pcs = ₱20.00)
   - **BLUE (A0):** 290 ML Bottle Mode (10 pcs = ₱3.00)
   - **RED (A1):** Cancel Transaction / System Reset

---

## 2. Test 04: LJ12A3-4-Z/BX Inductive Metal Sensor

### Hardware Components:
* 12V LJ12A3-4-Z/BX NPN NO Proximity Sensor
* 10kΩ / 4.7kΩ or 5kΩ Resistor Voltage Divider (Scales 12V output to safe ~3.8V – 4.0V into Pin D6)

### Real Chassis Installation & Calibration:
1. **Critical Mounting Distance (2–3 mm Air Gap):**
   - The rated sensing distance for iron is **4 mm**, but for aluminum cans (soda/beer cans), the sensing distance drops to **~1.5 mm – 2 mm**.
   - Mount the blue sensing head of the LJ12A3 so that inserted items slide within **2 mm to 3 mm** of the sensor face.
2. **Chassis Material Clearance (No Metal Around Sensor Body):**
   - **WARNING:** If the chassis chute is constructed from steel or aluminum, do NOT screw the LJ12A3 directly into a metal hole without clearance! The metal chute will permanently trigger the sensor!
   - Mount the sensor through an **acrylic plate, 3D-printed plastic collar, or wood insert** with at least **15 mm clearance** from surrounding metal framing.
3. **Voltage Divider Verification:**
   - Before connecting the black signal wire to Arduino Pin D6, measure with a multimeter at the divider junction:
     * Sensor un-triggered: **~3.8V – 4.0V DC** (safe logic HIGH: ~3.84V with 4.7kΩ, or 4.00V with 5kΩ).
     * Sensor triggered: **~0.0V DC** (logic LOW).

---

## 3. Test 05: MG996R Metal Gear Trapdoor Servo

### Hardware Components:
* TowerPro MG996R High-Torque Metal Gear Servo (0° Standby/Reject, 90° Accept)
* 5V Regulated Power Rail (minimum 2.5A peak current capacity)

### Real Chassis Installation & Calibration:
1. **Horn Alignment & Angle Calibration:**
   - **0° (Standby & Reject Position):** Trapdoor flap must be **100% horizontal**, perfectly supporting the inserted bottle on the cradle. If the bottle is rejected (metal detected, size mismatch), the flap **holds firmly at 0° (stays closed)** so the customer can manually retrieve the item from the entry chute while the buzzer sounds and Red LED blinks.
   - **90° (Accept Position):** Flap swings down/open completely to let the verified plastic bottle drop into the internal collection bin under gravity, then immediately returns to 0° Standby.
   - **Note on 180°:** The previous 180° forward tilt is retired in favor of front-chute manual retrieval at 0° closed flap.
2. **Mechanical Stopper & Strain Relief:**
   - Install a small mechanical bumper or ledge under the flap at 0° so that heavy 1.5L bottles filled with liquid rest on the frame, **NOT solely on the servo gear teeth**.
3. **Power Rail Decoupling:**
   - **NEVER power the MG996R directly from the Arduino 5V pin!** Use the LM2596 buck converter 5V rail.
   - Place a **470 µF to 1000 µF 16V electrolytic capacitor** across the servo 5V and GND terminals to prevent power brownouts during rapid flap rotation.

---

## 4. Test 06: Coin Hopper & 5V Relay Payout Module (220V AC / 12V DC)

### Hardware Components:
* Coin Hopper (Dispenses ₱1.00 coins — standard models use 220V AC or 12V DC motor)
* 5V Single-Channel Relay Module (Pin D8, active LOW, switches 220V AC Live line or 12V DC + line)
* Optical Coin Sensor:
  * **YT LP-08 A01 Sensor Board (5V DC TTL):** Directly powered by 5V DC and GND; SIG connects directly to Arduino Pin D7 (Normally HIGH idle, active LOW on coin detect). No voltage divider required.
  * *Legacy 12V Pulse Sensors:* Scaled to safe ~3.8V via 10kΩ / 4.7kΩ divider.

### Real Chassis Installation & Calibration:
1. **Pulse Line Interfacing (Pin D7):**
   - The optical sensor switch inside the hopper pulls the signal line LOW when a coin passes through the exit eye.
   - For 5V sensor boards (YT LP-08 A01), connect SIG directly to Pin D7.
   - Software edge polling with **60 ms lockout debounce** eliminates double-counting on exit.
2. **220V AC Mains Wiring & Safety:**
   - 220V AC Wall Plug Live wire enters Relay `COM`.
   - Relay `NO` (Normally Open) connects to Hopper motor Live wire (Brown).
   - 220V AC Wall Plug Neutral wire connects directly to Hopper motor Neutral wire (Blue) with heatshrink or terminal block.
   - Ground wire (Green/Yellow) connects directly to chassis earth ground.
3. **Hopper Dispense Ramp Angle:**
   - Mount the hopper securely with at least a **30°–45° gravity ramp** leading to the exterior coin return cup.
   - Ensure the coin exit path has no sharp edges or burrs that could cause coin jams.
4. **Electrical Noise Isolation (Flyback & AC Snubbing):**
   - Verify that the relay module includes optocoupler isolation to isolate AC line inductive kickback from the 5V Arduino microcontroller.
   - Ensure Arduino and 5V DC sensor grounds share a solid ground reference.

---

## 5. Test 07: SIM900A / SIM800L GSM SMS Module

### Hardware Components:
* **SIMCom SIM900A (Chip Model: S2-1040U-Z1K0H)** / SIM800L GSM/GPRS Module
* 5.0V DC Power Rail (from LM2596 DC-DC Buck Converter; 2.0A peak burst capability)
* 1000 µF to 2200 µF 16V Low-ESR Decoupling Capacitor across module VCC & GND
* Arduino SoftwareSerial: **5VT &rarr; Pin D11 (RX)** and **5VR &larr; Pin A3 (TX)**
* External SMA GSM Antenna

### Real Chassis Installation & Calibration:
1. **Direct 5.0V Rail & Burst Decoupling Capacitor:**
   - The SIM900A breakout board has onboard 5V regulation and logic level shifting (`5VT` and `5VR`).
   - Power the `VCC` pin directly from the **LM2596 5.0V rail** (do NOT use the Arduino 5V pin!).
   - Solder or place a **1000 µF to 2200 µF low-ESR capacitor** directly across `VCC` and `GND` at the module header to absorb the 2.0A cellular burst transmission spikes without dropping voltage.
2. **Logic Level Wiring (5VT & 5VR):**
   - Connect module **`5VT`** (5V TTL Transmit) &rarr; Arduino Uno **`D11`** (SoftwareSerial RX).
   - Connect module **`5VR`** (5V TTL Receive) &larr; Arduino Uno **`A3`** (SoftwareSerial TX).
   - Common ground: Connect module **`GND`** to the common system ground rail (shared with Arduino Uno and LM2596 Buck).
3. **Antenna Placement in Metal Enclosure (Crucial):**
   - **WARNING:** A steel or aluminum chassis acts as a **Faraday cage** and will block cellular signals!
   - Mount an **external SMA magnetic / stub GSM antenna OUTSIDE the machine cabinet roof** using a chassis-mount bulkhead extension cable.
4. **SIM Card Preparation & Status LED:**
   - Ensure the SIM card (Smart, Globe, TNT, or TM) has:
     * PIN lock **disabled** (test in a phone first).
     * Active prepaid load / SMS balance.
     * Valid Philippines recipient phone number in sketch (e.g., `+639XXXXXXXXX`).
   - Observe the onboard **NET LED**:
     * Fast flash (~800ms): Module is searching for a network.
     * Slow steady flash (~3000ms / 3 seconds): Module is registered on the cellular network and ready for SMS dispatch!

---

## 6. Test 08: Coin Hopper & GSM SMS Low-Coin / Jam Alert System

### Hardware Components:
* 12V/220V Coin Hopper & 5V Relay (Pin D8)
* Coin Hopper Optical Pulse Sensor (Pin D7)
* SIMCom SIM900A (S2-1040U-Z1K0H) / SIM800L GSM Module (Pins D11 & A3)
* Active 5V Buzzer (Pin D12) & Red Fault LED (Pin D13)

### Real Chassis Installation & Calibration:
1. **5-Second Dry-Run Motor Protection:**
   - If the coin hopper runs out of coins during a dispense cycle, allowing the motor to spin continuously will overheat the motor and strip internal gears.
   - The safety firmware measures the duration since the last optical falling pulse on Pin D7. If **5.0 seconds elapses with 0 coins detected**, Pin D8 instantly cuts relay power to stop the hopper motor.
2. **Automated Low-Coin / Jam Emergency Dispatch:**
   - Once the 5-second timer trips, the controller commands the GSM modem to send an SMS text alert to the machine administrator phone:
     `"ALERT: PCBCES Coin Hopper is EMPTY or JAMMED! No coin dispensed for 5 seconds during payout..."`
   - Simultaneously, the machine sounds an acoustic alarm on Pin D12 and illuminates the Red Fault LED on Pin D13.
3. **Recovery Procedure:**
   - The technician replenishes 1-peso coins into the hopper hopper bowl and restarts or resets the machine via Button Red (Pin A1).

---

## 7. Pre-Flight Integration Checklist (Chassis Transfer)

| Subsystem | Key Verification Task | Acceptance Criteria |
|---|---|---|
| **Power Bus** | Measure 12V PSU and 5V Buck rails with DMM | 12V $\pm$ 0.2V, 5.0V $\pm$ 0.1V |
| **Grounding** | Check continuity between all GND pins | $< 0.2\,\Omega$ resistance across all grounds |
| **Chamber Height** | Run `03_ultrasonic_ir_dimension_test.ino` | Stable baseline reading with 0 cm height |
| **Inductive Metal** | Test aluminum can and plastic bottle at sensor face | Cans trigger D6 LOW; plastic ignored |
| **Servo Angles** | Test flap positions at 0° (Standby/Reject) and 90° (Accept) | Smooth travel, no motor humming at 0° rest |
| **Hopper Payout** | Run `06_coin_hopper_relay_test.ino` | Dispenses exactly 3 coins (₱3.00) or 20 coins (₱20.00) and halts |
| **GSM Signal** | Run `07_sim800l_gsm_sms_test.ino` | Returns `+CSQ: > 14` and sends test SMS |
| **Hopper + GSM** | Run `08_coin_hopper_gsm_low_coin_test.ino` | 5s empty hopper cutoff triggers motor halt & emergency SMS |

