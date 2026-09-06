/*
 * PCBCES - Test 02: [ARCHIVED / OMITTED IN ACTIVE BUILD]
 * Note: The load cell weight feature was archived from the active machine in favor of
 * non-contact ultrasonic & dielectric proximity sensing to increase durability and
 * eliminate calibration drift. 
 * IMPORTANT: Pins A0 and A1 are reallocated in the active master machine:
 *   - Pin A0: Button Blue (Mismo Bottle Mode - 10 pcs = 3 PHP)
 *   - Pin A1: Button Red  (System Restart / Cancel Transaction)
 *
 * Hardware: Arduino Uno, HX711 24-Bit ADC Module, 1kg Load Cell
 * 
 * Bench Test Pin Connections (When testing this isolated module only):
 * - HX711 VCC -> 5V Rail
 * - HX711 GND -> Common GND Rail
 * - HX711 DT  -> A0 (Reallocated to Button Blue in master build)
 * - HX711 SCK -> A1 (Reallocated to Button Red in master build)
 * 
 * Purpose:
 * - Historical reference for bench scale calibration testing
 * 
 * Last Updated: 2026-09-06 14:30:41 (+08:00)
 */

#include "HX711.h"

const int LOADCELL_DOUT_PIN = A0;
const int LOADCELL_SCK_PIN = A1;

HX711 scale;

// Calibration factor (Adjust with a known weight, e.g., 100g or 200g object)
// Common factor for standard 1kg load cell is around -400 to -450 or +400
float calibration_factor = 420.0; 

void setup() {
  Serial.begin(115200);
  Serial.println(F("========================================"));
  Serial.println(F(" PCBCES Test 02: HX711 Load Cell Tester "));
  Serial.println(F("========================================"));

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  
  Serial.println(F("Taring... Please ensure NO bottle is on the tray."));
  delay(1000);
  scale.set_scale();
  scale.tare(); // Reset scale to 0
  
  scale.set_scale(calibration_factor);
  Serial.println(F("Tare complete! Ready for weight test."));
  Serial.println(F("Commands via Serial:"));
  Serial.println(F(" '+' or '-' to adjust calibration factor by 10"));
  Serial.println(F(" 't' to re-tare"));
}

void loop() {
  if (scale.is_ready()) {
    float weight = scale.get_units(5); // Average 5 readings
    if (weight < 0.5 && weight > -0.5) weight = 0.0; // Deadband filter

    Serial.print(F("Weight: "));
    Serial.print(weight, 1);
    Serial.print(F(" g | Status: "));

    if (weight < 5.0) {
      Serial.println(F("Tray Empty"));
    } else if (weight >= 15.0 && weight <= 30.0) {
      Serial.println(F("VALID MISMO (~22g empty)"));
    } else if (weight >= 38.0 && weight <= 60.0) {
      Serial.println(F("VALID 1.5L (~45g empty)"));
    } else if (weight > 70.0) {
      Serial.println(F("REJECT: Bottle contains liquid or heavy foreign object!"));
    } else {
      Serial.println(F("Unknown / Irregular weight"));
    }
  } else {
    Serial.println(F("HX711 not detected. Check DT (A0) and SCK (A1) wiring!"));
  }

  // Handle serial calibration inputs
  if (Serial.available()) {
    char ch = Serial.read();
    if (ch == '+') {
      calibration_factor += 10;
      scale.set_scale(calibration_factor);
      Serial.print(F("Cal factor: ")); Serial.println(calibration_factor);
    } else if (ch == '-') {
      calibration_factor -= 10;
      scale.set_scale(calibration_factor);
      Serial.print(F("Cal factor: ")); Serial.println(calibration_factor);
    } else if (ch == 't' || ch == 'T') {
      scale.tare();
      Serial.println(F("Re-tared to 0.0g"));
    }
  }

  delay(500);
}