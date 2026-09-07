# PCBCES — Integrated Demo Chassis Bench Test Guide

> **Sketch Location:** [`arduino/pcbces_demo_chassis_test/pcbces_demo_chassis_test.ino`](file:///C:/xampp/htdocs/PCBCES-Plastic-Bottle-Coin-Exchanger-System/arduino/pcbces_demo_chassis_test/pcbces_demo_chassis_test.ino)  
> **Target Rig:** Temporary cardboard / plywood demo rig or breadboard bench setup  
> **Modules Active:** Tests 01, 03, 04, 05, and 06  
> **Excluded:** SIM800L GSM (Test 07) & Load Cell (Test 02)  
> **Last Updated:** 2026-09-07 17:56:00 (+08:00)

---

## 1. Overview & Purpose

This sketch is the **temporary integrated master controller** designed for testing your complete reverse vending machine workflow on your **demo chassis / breadboard** before building the permanent sheet metal cabinet.

It runs the complete full-cycle operation:
1. **User Selection:** 3 dedicated arcade buttons (Green D10, Blue A0, Red A1) with 16x2 I2C LCD prompts.
2. **Bottle Detection:** Active LOW IR entry sensor on Pin D4.
3. **Multi-Sensor Inspection:**
   - **Metal Check:** LJ12A3 inductive proximity sensor on Pin D6 (rejects soda cans/metals immediately).
   - **Size Discrimination:** HC-SR04 ultrasonic sensor on Pins D2/D3 measuring ceiling-to-bottle-cap distance.
4. **Physical Sorting:** MG996R servo motor on Pin D9 holding 0° for Standby/Reject, and swinging 90° to drop valid plastic bottles into the collection bin.
5. **Coin Reward Payout:** 5V relay on Pin D8 powering the 12V hopper motor, while Pin D7 counts coin exit pulses.
6. **5-Second Motor Safety Cutoff:** If the hopper runs dry or has no coins, it shuts off the relay automatically after 5 seconds to protect the motor from running dry!

---

## 2. Wiring Summary (Your Existing Breadboard Setup)

| Pin | Hardware Attached | Signal Function |
|---|---|---|
| **D2** | HC-SR04 Trigger | 10µs ultrasonic start pulse |
| **D3** | HC-SR04 Echo | Vertical distance to bottle cap |
| **D4** | IR Entry Sensor | Active LOW bottle entry beam detect |
| **D6** | LJ12A3 Inductive Metal | Active LOW metal detection (via 10k/4.7k or 5k divider) |
| **D7** | Coin Hopper Pulse Line | Optical falling-edge interrupt/polling (1 pulse = ₱1) |
| **D8** | 5V Relay Module | Active LOW control for 12V Hopper motor power |
| **D9** | MG996R Servo (PWM) | 0° Standby / Reject Hold, 90° Accept Drop |
| **D10** | Button Green (1.5L) | INPUT_PULLUP (5 bottles quota = ₱20 payout) |
| **D12** | Active 5V Buzzer | Audio prompts (Acknowledge / Alarm) |
| **D13** | Red LED | Visual rejection / fault indicator |
| **A0** | Button Blue (290 ML) | INPUT_PULLUP (10 bottles quota = ₱3 payout) |
| **A1** | Button Red (Cancel/Reset) | INPUT_PULLUP (Abort transaction at any time) |
| **A2** | Green LED | Ready / Success indicator |
| **A4** | 16x2 LCD SDA | Hardware I2C Data (Address `0x27`) |
| **A5** | 16x2 LCD SCL | Hardware I2C Clock |

---

## 3. Serial Monitor Diagnostics & Simulation Keys

Open Serial Monitor at **115200 baud**. You can operate the entire machine using your physical breadboard buttons and sensors, **OR** use these keyboard shortcuts:

* `g` $\rightarrow$ Select 1.5L / 1.75L Mode (5 pcs quota = ₱20)
* `b` $\rightarrow$ Select 290 ML Mode (10 pcs quota = ₱3)
* `r` $\rightarrow$ Cancel / Reset transaction and return to Standby
* `d` $\rightarrow$ Print **Live Sensor Diagnostics** (shows current ultrasonic distance, IR state, metal state, and coin pin state)

---

## 4. Calibrating Your Demo Chamber Height

If your cardboard/demo box has a different ceiling height than the standard 43 cm:
1. Turn on the machine with an empty chamber and press `d` in the Serial Monitor.
2. Note the measured ceiling-to-trapdoor distance (e.g. `38 cm` or `43 cm`).
3. If needed, you can fine-tune these four lines near the top of [`pcbces_demo_chassis_test.ino`](file:///C:/xampp/htdocs/PCBCES-Plastic-Bottle-Coin-Exchanger-System/arduino/pcbces_demo_chassis_test/pcbces_demo_chassis_test.ino#L60-L65):
   ```cpp
   int chamberTotalHeightCm = 43;
   int dist15LMin  = 7;   // Cap is 7 to 18 cm from top sensor
   int dist15LMax  = 18;
   int dist290Min  = 26;  // Cap is 26 to 27 cm from top sensor
   int dist290Max  = 27;
   ```
