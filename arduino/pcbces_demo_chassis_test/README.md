# PCBCES — Integrated Demo Chassis Bench Test Guide

> **Sketch Location:** [`arduino/pcbces_demo_chassis_test/pcbces_demo_chassis_test.ino`](file:///C:/xampp/htdocs/PCBCES-Plastic-Bottle-Coin-Exchanger-System/arduino/pcbces_demo_chassis_test/pcbces_demo_chassis_test.ino)  
> **Target Rig:** Temporary cardboard / plywood demo rig or breadboard bench setup  
> **Modules Active:** Tests 01, 03, 04, 05, 06, 07, and 08  
> **Excluded:** Load Cell (Test 02 - permanently archived)  
> **Last Updated:** 2026-09-18 22:36:00 (+08:00)

---

## 1. Overview & Purpose

This sketch is the **temporary integrated master controller** designed for testing your complete reverse vending machine workflow on your **demo chassis / breadboard** before transferring into the permanent machine cabinet.

It runs the complete full-cycle operation:
1. **User Selection:** 3 dedicated arcade buttons (Green D10, Blue A0, Red A1) with 16x2 I2C LCD prompts.
2. **Bottle Detection:** Active LOW IR entry sensor on Pin D4 (with ultrasonic fallback distance trigger).
3. **Multi-Sensor Inspection:**
   - **Size Discrimination:** HC-SR04 ultrasonic sensor on Pins D2/D3 measuring ceiling-to-bottle-cap distance against the calibrated chamber height (32 cm).
   - **Metal Check (Optional):** LJ12A3 inductive proximity sensor on Pin D6 (can be uncommented anytime).
4. **Physical Sorting:** MG996R servo motor on Pin D9 holding 0° for Standby/Reject, and swinging 90° to drop valid plastic bottles into the collection bin.
5. **Coin Reward Payout:** 5V relay on Pin D8 powering the 12V hopper motor, while Pin D7 counts coin exit pulses.
6. **5-Second Motor Safety Cutoff & Automated GSM SMS (Test 08):** If the hopper runs dry or has no coins for 5 seconds during payout, the motor is stopped immediately, and an emergency SMS is automatically dispatched to the admin phones (`+639634299114`, `+639242074903`)!

---

## 2. Wiring Summary (Canonical Uno 20-Pin Lock)

| Pin | Hardware Attached | Voltage Domain | Signal Function |
|---|---|---|---|
| **D2** | HC-SR04 Trigger | 5V Logic | 10µs ultrasonic start pulse |
| **D3** | HC-SR04 Echo | 5V Logic | Vertical distance to bottle cap |
| **D4** | IR Entry Sensor | 5V Logic | Active LOW bottle entry beam detect |
| **D5** | IR Bin-Full Sensor | 5V Logic | Active LOW bottle storage bin full detect |
| **D6** | LJ12A3 Inductive Metal | Scaled ~3.8V | Active LOW metal detection (via 10k/4.7k divider) |
| **D7** | Coin Hopper Pulse Line | Scaled ~3.8V | Optical falling-edge pulse (1 pulse = ₱1 via 10k/4.7k divider) |
| **D8** | 5V Relay Module | 5V Logic | Active LOW control for 12V Hopper motor power |
| **D9** | MG996R Servo (PWM) | 5V Logic | 0° Standby / Reject Hold, 90° Accept Drop |
| **D10** | Button Green (1.5L) | 5V Logic | INPUT_PULLUP (5 bottles quota = ₱20 payout) |
| **D11** | SoftwareSerial RX | 5V / 4.3V TTL | Connects to GSM SIM900A 5VT / SIM800L TX |
| **D12** | Active 5V Buzzer | 5V Logic | Audio prompts (Acknowledge / Alarm) |
| **D13** | Red LED | 5V Logic | Visual rejection / fault indicator |
| **A0** | Button Blue (290 ML) | 5V Logic | INPUT_PULLUP (10 bottles quota = ₱3 payout) |
| **A1** | Button Red (Cancel/Reset) | 5V Logic | INPUT_PULLUP (Abort transaction at any time) |
| **A2** | Green LED | 5V Logic | Ready / Success indicator |
| **A3** | SoftwareSerial TX | 5V / 4.3V TTL | Connects to GSM SIM900A 5VR / SIM800L RX |
| **A4** | 16x2 LCD SDA | 5V Logic | Hardware I2C Data (Address `0x27`) |
| **A5** | 16x2 LCD SCL | 5V Logic | Hardware I2C Clock |

---

## 3. Serial Monitor Diagnostics & Simulation Keys

Open Serial Monitor at **115200 baud**. You can operate the entire machine using your physical breadboard buttons and sensors, **OR** use these keyboard shortcuts:

* `g` $\rightarrow$ Select 1.5L / 1.75L Mode (5 pcs quota = ₱20)
* `b` $\rightarrow$ Select 290 ML Mode (10 pcs quota = ₱3)
* `r` $\rightarrow$ Cancel / Reset transaction and return to Standby
* `d` $\rightarrow$ Print **Live Sensor Diagnostics** (shows current ultrasonic distance, IR state, coin pin state, relay state, and GSM status)
* `s` $\rightarrow$ Send **Test SMS** via GSM module to verify network connectivity
* `t` $\rightarrow$ Trigger **Coin Hopper Payout Test** (3 coins) to bench-test optical pulse detection & 5s timeout

---

## 4. Demo Chamber Height Calibration

The demo chassis is calibrated to **32 cm**:
```cpp
int chamberTotalHeightCm = 32;
int dist15LMin  = 11;   // Cap is 11 to 12 cm from top sensor
int dist15LMax  = 12;
int dist290Min  = 23;   // Cap is 23 to 24 cm from top sensor
int dist290Max  = 24;
```
