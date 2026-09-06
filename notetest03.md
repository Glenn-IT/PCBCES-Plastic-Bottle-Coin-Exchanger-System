# PCBCES — Test 03: Vertical Chamber Calibration & Real Chassis Transfer Guide

> **Document:** `notetest03.md`  
> **Purpose:** Detailed instructions for calibrating the **HC-SR04 Ultrasonic Dimensioning Sensor** and **IR Obstacle Entry Sensor** when moving from the temporary cardboard prototype to the permanent PCBCES reverse vending machine chassis.  
> **Last Updated:** 2026-09-06 15:23:00 (+08:00)

---

## 1. Why Calibration is Required on the Real Chassis

In your temporary cardboard box setup, the physical baseline was calibrated to **43 cm**:
* Ultrasonic sound absorption of cardboard differs from acrylic, sheet metal, or wood.
* The permanent chassis may have a different chamber height (e.g., 40 cm, 45 cm, 50 cm, or 55 cm depending on sheet metal cuts and trapdoor clearance).
* The inner width and surface reflectance of the real drop chute will affect ultrasonic cone dispersion (15°–30° beam spread).
* Ambient light conditions inside the machine enclosure will require adjusting the IR optical threshold.

---

## 2. Physical Installation Rules for Real Chassis

Before turning on the Arduino, verify the following mechanical rules:

### A. HC-SR04 Ceiling Mounting
1. **Dead-Center & Square Alignment:**
   - Mount the HC-SR04 transducer pair at the **exact geometric center of the chute ceiling**.
   - Ensure the sensor face is **100% perpendicular (90° square)** to the vertical walls and the trapdoor floor. Any tilt will bounce sound waves into side walls instead of back to the receiver.
2. **Clear Sound Cone (15°–30° Spread):**
   - At a 45 cm depth, the ultrasonic beam expands to approximately **20–25 cm in diameter**.
   - Ensure the inner chute walls are smooth. Keep bolt heads, rivets, brackets, and wiring tucked outside the chute or countersunk flat.

### B. IR Entry Sensor Chute Mounting
1. **Flush Wall Mount (Crucial):**
   - Cut a small aperture in the chute wall just large enough for the IR transmitter and receiver lenses.
   - **DO NOT allow the sensor PCB or cylinder to protrude into the chamber!** If it sticks out, the HC-SR04 sound cone will hit the IR sensor head and report false distances.
2. **Optimal Placement Height:**
   - Position the IR sensor beam horizontally across the chute at a height where **both 1.5L and Mismo bottles will reliably break the beam** when resting upright or sliding onto the cradle.

### C. Trapdoor Base Flap (MG996R)
1. **Horizontal Rest Angle:**
   - When the servo is at `0° Standby`, the flap must rest **firmly horizontal** (flat) to provide a clean, perpendicular acoustic bounce for the ultrasonic echo.
   - If using smooth acrylic or metal for the flap, affix a thin non-reflective textured sticker or silicone pad if sound reflections scatter.

---

## 3. Step-by-Step Live Calibration Procedure

### Phase 1: Determine the Real Chamber Baseline ($H_{\text{chamber}}$)

1. Ensure the chamber is **completely empty** (no bottles, tools, or hands inside).
2. Power the Arduino and open **Arduino IDE Serial Monitor** at `115200 baud`.
3. Read the reported `Chamber Distance`:
   - It will display: `Chamber Distance: XX cm`.
   - Take note of this stable reading (e.g., `45 cm`).
4. This value is your new **`CHAMBER_HEIGHT_CM`** (or `CHAMBER_TOTAL_HEIGHT_CM`).

---

### Phase 2: Calibrate 1.5L / 1.75L Bottle Mode

Because the sensor looks down from the ceiling:
$$\text{Distance to Cap} = H_{\text{chamber}} - \text{Bottle Height}$$

1. Place an actual **1.5L Coke / Royal / Sprite bottle** upright on the trapdoor flap.
2. Ensure the bottle breaks the IR beam (`IR Entry: [BOTTLE PRESENT]`).
3. Read the `Chamber Distance` on the Serial Monitor.
4. Test with **2 to 3 different common brands** of 1.5L and 1.75L bottles (they vary slightly between 30 cm and 34 cm tall).
5. Record the minimum and maximum distances observed:
   * Example: Observed readings between `9 cm` and `13 cm`.
6. Add a **±2 cm safety tolerance margin**:
   * `DIST_1_5L_MIN = Observed_Min - 2` (e.g., 9 - 2 = 7 cm)
   * `DIST_1_5L_MAX = Observed_Max + 2` (e.g., 13 + 2 = 15 cm)

---

### Phase 3: Calibrate 290 ML / Mismo (250–350 mL) Bottle Mode

1. Place an actual **290 ML bottle** (e.g., Coca-Cola 290 ML Mismo, Sprite Mismo, or C2 355 mL) upright on the trapdoor flap.
2. Read the `Chamber Distance` on the Serial Monitor.
3. Test with **2 to 3 common 290 ML / mini bottles** (they vary between 16 cm and 20 cm tall).
4. Record the observed distance to cap:
   * Example: Observed readings between `24 cm` and `26 cm`.
5. Add a **safety tolerance margin**:
   * `DIST_290ML_MIN = Observed_Min - 1` (e.g., 24 - 1 = 23 cm or 24 cm; alias: `DIST_MISMO_MIN`)
   * `DIST_290ML_MAX = Observed_Max + 1` (e.g., 26 + 1 = 27 cm; alias: `DIST_MISMO_MAX`)

---

### Phase 4: Calibrate the IR Sensor Potentiometer

The blue IR sensor board has a small trimmer potentiometer screw:
1. **With Entry Chute Empty:**
   - Turn the screw counter-clockwise until the green/yellow detection LED on the sensor board turns **OFF**.
   - Serial monitor must show `IR Entry: [CLEAR         ]`.
2. **With Transparent Bottle Inserted:**
   - Insert a clear plastic bottle.
   - Slowly turn the screw clockwise until the detection LED turns **ON solidly**.
   - Verify that Serial Monitor instantly switches to `IR Entry: [BOTTLE PRESENT]`.
3. **Ambient Light Check:**
   - Shine external light (or turn on room lights / close chassis door) and verify that shadows or enclosure lights do NOT cause false triggers when the chute is empty.

---

## 5. Code Synchronization Checklist

Whenever you adjust these values for your real chassis, update them in **both** locations:

### 1. In Bench Test Sketch (`arduino/03_ultrasonic_ir_dimension_test/03_ultrasonic_ir_dimension_test.ino`):
```cpp
const int CHAMBER_HEIGHT_CM = <YOUR_REAL_BASELINE>; 
const int DIST_1_5L_MIN     = <YOUR_1.5L_MIN>;      
const int DIST_1_5L_MAX     = <YOUR_1.5L_MAX>;
const int DIST_MISMO_MIN    = <YOUR_MISMO_MIN>;    
const int DIST_MISMO_MAX    = <YOUR_MISMO_MAX>;
```

### 2. In Master Controller Config (`arduino/pcbces_arduino_controller/config.h`):
```cpp
#define CHAMBER_TOTAL_HEIGHT_CM <YOUR_REAL_BASELINE>
#define DIST_1_5L_MIN           <YOUR_1.5L_MIN>
#define DIST_1_5L_MAX           <YOUR_1.5L_MAX>
#define DIST_MISMO_MIN          <YOUR_MISMO_MIN>
#define DIST_MISMO_MAX          <YOUR_MISMO_MAX>
```

### 3. Timestamp Update:
Update the DateTime Stamp comment in both `.ino` sketch headers:
```cpp
* Last Updated: YYYY-MM-DD HH:MM:SS (+08:00)
```

### 4. Run Cross-Sync Verification:
Run the repository verification script in terminal:
```bash
php scripts/verify_hardware_sync.php
```

---

## 6. Summary Cheat Sheet for Chassis Transfer

| Parameter | Cardboard Prototype | Typical Real Metal Chassis | How to Measure |
|---|---|---|---|
| **Chamber Baseline** | `43 cm` | `40 cm to 55 cm` | Empty chamber reading on Serial Monitor |
| **1.5L Distance Range** | `7 cm to 15 cm` | Baseline - (30 to 33 cm) | Cap distance with 1.5L bottle inside |
| **290 ML / Mismo Distance Range** | `26 cm to 27 cm` | Baseline - (16 to 17 cm) | Cap distance with 290 ML bottle inside |
| **IR Sensor Position** | Lower right wall | Flush on lower chute entry | Flush with inner wall, no protrusion |
| **Trapdoor Base** | Cardboard flap | Acrylic/sheet metal on MG996R | Must rest 100% horizontal in Standby (0°) |
