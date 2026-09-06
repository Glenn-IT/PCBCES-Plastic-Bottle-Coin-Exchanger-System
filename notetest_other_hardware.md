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
   - **GREEN (D10):** 1.5L Bottle Mode (5 pcs = ₱3.00)
   - **BLUE (A0):** Mismo Bottle Mode (10 pcs = ₱3.00)
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
* TowerPro MG996R High-Torque Metal Gear Servo (180° rotation)
* 5V Regulated Power Rail (minimum 2.5A peak current capacity)

### Real Chassis Installation & Calibration:
1. **Horn Alignment & Angle Calibration:**
   - **0° (Standby Position):** Trapdoor flap must be **100% horizontal**, perfectly supporting the inserted bottle and providing a flat bounce surface for the top HC-SR04 sensor.
   - **90° (Accept Position):** Flap swings down/open completely to let the bottle drop into the internal collection bin under gravity.
   - **180° (Reject Position):** Flap tilts forward to divert rejected items (cans, wrong size bottles) out through the front return chute.
2. **Mechanical Stopper & Strain Relief:**
   - Install a small mechanical bumper or ledge under the flap at 0° so that heavy 1.5L bottles filled with liquid rest on the frame, **NOT solely on the servo gear teeth**.
3. **Power Rail Decoupling:**
   - **NEVER power the MG996R directly from the Arduino 5V pin!** Use the LM2596 buck converter 5V rail.
   - Place a **470 µF to 1000 µF 16V electrolytic capacitor** across the servo 5V and GND terminals to prevent power brownouts during rapid flap rotation.

---

## 4. Test 06: 12V Coin Hopper & 5V Relay Payout Module

### Hardware Components:
* 12V DC Coin Hopper (Dispenses ₱1.00 coins)
* 5V Single-Channel Relay Module (Pin D8, active LOW)
* 10kΩ / 4.7kΩ or 5kΩ Voltage Divider (Pin D7 interrupt)

### Real Chassis Installation & Calibration:
1. **Pulse Line Interfacing (Pin D7):**
   - The optical count switch inside the hopper pulls the signal line LOW when a coin passes through the exit eye.
   - Verify the 10k/(4.7k or 5k) voltage divider scales the pulse to **~3.8V – 4.0V DC** into Pin D7.
   - Software interrupt debounce is calibrated to **60 ms** to eliminate contact bounce.
2. **Hopper Dispense Ramp Angle:**
   - Mount the hopper securely with at least a **30°–45° gravity ramp** leading to the exterior coin return cup.
   - Ensure the coin exit path has no sharp edges or burrs that could cause coin jams.
3. **Electrical Noise Isolation (Flyback Diode):**
   - The hopper DC motor creates inductive flyback spikes when turned OFF.
   - Verify that the relay module includes an optocoupler and flyback protection diode. Ensure all 12V, 5V, and Arduino grounds meet at a single **common star ground point**.

---

## 5. Test 07: SIM800L GSM SMS Module

### Hardware Components:
* SIM800L GPRS/GSM Module
* 4.0V–4.3V Power Rail (from dedicated LM2596 buck converter or 1N4007 silicon diode drop)
* Arduino D11 (RX) & A3 (TX) SoftwareSerial

### Real Chassis Installation & Calibration:
1. **Antenna Placement in Metal Enclosure (Crucial):**
   - **WARNING:** A steel or aluminum chassis acts as a **Faraday cage** and will block cellular signals!
   - If using a metal machine cabinet, replace the small onboard spring antenna with an **external IPEX-to-SMA cable** and mount a magnetic / stub GSM antenna **OUTSIDE the cabinet roof**.
2. **2A Burst Decoupling Capacitor:**
   - When the SIM800L transmits SMS bursts, it draws up to **2.0A for several milliseconds**.
   - Solder a **1000 µF to 2200 µF low-ESR capacitor** directly between the SIM800L module's `VCC` and `GND` pins to prevent sudden brownout module reboots.
3. **SIM Card Preparation:**
   - Ensure the micro-SIM card has:
     * PIN lock **disabled**.
     * Active prepaid load / SMS balance.
     * Valid Philippines format recipient number in `config.h` (e.g., `+639XXXXXXXXX`).

---

## 6. Pre-Flight Integration Checklist (Chassis Transfer)

| Subsystem | Key Verification Task | Acceptance Criteria |
|---|---|---|
| **Power Bus** | Measure 12V PSU and 5V Buck rails with DMM | 12V $\pm$ 0.2V, 5.0V $\pm$ 0.1V |
| **Grounding** | Check continuity between all GND pins | $< 0.2\,\Omega$ resistance across all grounds |
| **Chamber Height** | Run `03_ultrasonic_ir_dimension_test.ino` | Stable baseline reading with 0 cm height |
| **Inductive Metal** | Test aluminum can and plastic bottle at sensor face | Cans trigger D6 LOW; plastic ignored |
| **Servo Angles** | Test flap positions at 0°, 90°, and 180° | Smooth travel, no motor humming at 0° rest |
| **Hopper Payout** | Run `06_coin_hopper_relay_test.ino` | Dispenses exactly 3 coins (₱3.00) and halts |
| **GSM Signal** | Run `07_sim800l_gsm_sms_test.ino` | Returns `+CSQ: > 14` and sends test SMS |
