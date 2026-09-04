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
 * - 1000uF 16V capacitor connected across Servo + and - to prevent brownouts.
 * 
 * Interactive Console:
 * Send '0' -> Standby / Trapdoor Closed (0 degrees)
 * Send '1' -> Accept / Drop into Bin    (90 degrees)
 * Send '2' -> Reject / Return to User   (180 degrees)
 */

#include <Servo.h>

Servo trapdoorServo;
const int SERVO_PIN = 9;

void setup() {
  Serial.begin(115200);
  trapdoorServo.attach(SERVO_PIN);
  
  // Start at Standby closed position
  trapdoorServo.write(0);

  Serial.println(F("================================================"));
  Serial.println(F(" PCBCES Test 05: MG996R Sorting Servo Mechanism "));
  Serial.println(F("================================================"));
  Serial.println(F("Send via Serial Monitor:"));
  Serial.println(F(" '0' -> Close / Standby (0 deg)"));
  Serial.println(F(" '1' -> Accept / Drop (90 deg)"));
  Serial.println(F(" '2' -> Reject / Eject (180 deg)"));
  Serial.println(F(" 'a' -> Auto-cycle test (Close -> Accept -> Close -> Reject -> Close)"));
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();

    if (cmd == '0') {
      Serial.println(F("Moving to: STANDBY (0 deg)"));
      trapdoorServo.write(0);
    } else if (cmd == '1') {
      Serial.println(F("Moving to: ACCEPT / DROP TO BIN (90 deg)"));
      trapdoorServo.write(90);
    } else if (cmd == '2') {
      Serial.println(F("Moving to: REJECT / RETURN (180 deg)"));
      trapdoorServo.write(180);
    } else if (cmd == 'a' || cmd == 'A') {
      Serial.println(F("Starting Auto-Cycle Test..."));
      
      Serial.println(F("1. Accept Sequence (Valid bottle)"));
      trapdoorServo.write(90);
      delay(1500); // Hold open for bottle to fall
      trapdoorServo.write(0); // Close
      delay(1000);

      Serial.println(F("2. Reject Sequence (Invalid bottle)"));
      trapdoorServo.write(180);
      delay(1500); // Return
      trapdoorServo.write(0); // Return to close
      delay(1000);
      
      Serial.println(F("Auto-cycle test complete. Ready for next command."));
    }
  }
}