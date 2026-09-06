/*
 * PCBCES - Test 05: MG996R Metal Gear Servo Trapdoor Angle Test
 * Hardware: Arduino Uno, MG996R High-Torque Servo, LM2596 5V Rail + 1000uF Cap
 * 
 * Pin Connections:
 * - Servo Signal (Orange/White) -> D9 (PWM)
 * - Servo Power  (Red)          -> LM2596 5V Rail (NOT Arduino 5V pin!)
 * - Servo Ground (Brown/Black)  -> Common GND Rail
 * 
 * Safety:
 * - 1000uF (16V to 50V rated) electrolytic capacitor connected across Servo +5V and GND to absorb 2.5A current spikes.
 * 
 * Sorting Motion Standard:
 * - 0°  -> Standby / Scanning / Rejection Hold (Flap closed: supports bottle; invalid items stay on cradle for manual removal)
 * - 90° -> Accept Drop (Flap swings down to drop valid bottle into lower storage bin, then returns to 0°)
 * 
 * Interactive Console:
 * Send '0' -> Standby / Rejection Hold (0 degrees)
 * Send '1' -> Accept Sequence (90 degrees drop -> returns to 0 degrees standby)
 * Send 'a' -> Auto-cycle test (0° Standby -> 0° Reject Hold -> 90° Accept Drop -> 0° Standby)
 * 
 * Last Updated: 2026-09-06 18:15:00 (+08:00)
 */

#include <Servo.h>

Servo trapdoorServo;
const int SERVO_PIN = 9;

void setup() {
  Serial.begin(115200);
  trapdoorServo.attach(SERVO_PIN);
  
  // Start at Standby closed position (0 degrees)
  trapdoorServo.write(0);

  Serial.println(F("================================================"));
  Serial.println(F(" PCBCES Test 05: MG996R Sorting Servo Mechanism "));
  Serial.println(F("================================================"));
  Serial.println(F("Send via Serial Monitor:"));
  Serial.println(F(" '0' -> Standby / Reject Hold (0 deg - Flap Closed)"));
  Serial.println(F(" '1' -> Accept Drop (90 deg -> returns to 0 deg)"));
  Serial.println(F(" 'a' -> Auto-cycle test (Standby 0° -> Reject 0° -> Accept 90° -> 0°)"));
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();

    if (cmd == '0') {
      Serial.println(F("Command '0': STANDBY / REJECT HOLD (0 deg - Flap Closed)"));
      trapdoorServo.write(0);
    } else if (cmd == '1') {
      Serial.println(F("Command '1': ACCEPT SEQUENCE TRIGGERED"));
      Serial.println(F(" -> Opening trapdoor to 90 deg (dropping bottle into bin)..."));
      trapdoorServo.write(90);
      delay(1500); // Hold open for bottle to fall into internal storage bin
      Serial.println(F(" -> Returning trapdoor to 0 deg (Standby closed position)..."));
      trapdoorServo.write(0);
      Serial.println(F(" -> Flap locked at 0 deg. Ready for next bottle."));
    } else if (cmd == '2') {
      Serial.println(F("[NOTICE] 180 deg tilt is RETIRED. Rejection holds at 0 deg (closed cradle) for manual user retrieval."));
      trapdoorServo.write(0);
    } else if (cmd == 'a' || cmd == 'A') {
      Serial.println(F("------------------------------------------------"));
      Serial.println(F("Starting Auto-Cycle Test..."));
      
      Serial.println(F("Step 1: Standby / Scanning State (0 deg)"));
      trapdoorServo.write(0);
      delay(1000);

      Serial.println(F("Step 2: Rejection Simulation (Invalid Item Detected)"));
      Serial.println(F(" -> Flap STAYS at 0 deg (Item remains on cradle for manual removal)"));
      trapdoorServo.write(0);
      delay(2000);

      Serial.println(F("Step 3: Acceptance Simulation (Valid Plastic Bottle)"));
      Serial.println(F(" -> Flap rotates to 90 deg (Drop into bin)..."));
      trapdoorServo.write(90);
      delay(1500);

      Serial.println(F("Step 4: Returning Flap to 0 deg Standby..."));
      trapdoorServo.write(0);
      delay(1000);
      
      Serial.println(F("Auto-cycle test complete. Ready for next command."));
      Serial.println(F("------------------------------------------------"));
    }
  }
}