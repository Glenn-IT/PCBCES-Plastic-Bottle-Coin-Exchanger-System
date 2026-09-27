/*
 * PCBCES - Test 05: MG995 360° Continuous Rotation Servo Timed Workaround
 * Hardware: Arduino Uno, MG995 360° Continuous Servo, LM2596 5V Rail + 1000uF Cap
 * 
 * Pin Connections:
 * - Servo Signal (Orange/White) -> D9 (PWM)
 * - Servo Power  (Red)          -> LM2596 5V Rail (NOT Arduino 5V pin!)
 * - Servo Ground (Brown/Black)  -> Common Star GND Rail
 * 
 * Safety & Decoupling:
 * - 1000uF (16V to 50V rated) electrolytic capacitor connected across Servo +5V and GND.
 * 
 * 360° Continuous Servo Principle:
 * - write(90) -> STOP / Neutral (Motor cuts drive)
 * - write(<90) (e.g. 70)  -> Rotate forward (Opening direction)
 * - write(>90) (e.g. 110) -> Rotate reverse (Closing direction)
 * - Because a 360° motor lacks angle feedback, movement is controlled by TIMING (e.g. 300ms).
 * 
 * Interactive Console Commands:
 * Send '1' -> Full Drop Cycle: Open (300ms) -> Pause 1.5s -> Close (300ms) -> Stop
 * Send 'o' -> Open pulse only (rotates forward for configured duration, then stops)
 * Send 'c' -> Close pulse only (rotates reverse for configured duration, then stops)
 * Send 's' -> Emergency STOP (write 90 immediately)
 * Send '+' -> Increase rotation pulse duration (+50ms)
 * Send '-' -> Decrease rotation pulse duration (-50ms)
 * 
 * Last Updated: 2026-09-27 15:35:00 (+08:00)
 */

#include <Servo.h>

Servo trapdoorServo;
const int SERVO_PIN = 9;

// Continuous Servo Calibration Settings
const int STOP_CMD   = 90;   // Neutral stop signal (typically 90)
const int OPEN_CMD   = 70;   // Forward rotation speed (toward 90° open)
const int CLOSE_CMD  = 110;  // Reverse rotation speed (toward 0° closed)

// Timing: Calibrated by user on real chassis
int pulseDurationMs = 900;   // Calibrated 900 ms for ~90 degrees swing

void stopMotor() {
  trapdoorServo.write(STOP_CMD);
  delay(60);
  if (trapdoorServo.attached()) {
    trapdoorServo.detach();
  }
}

void openTrapdoor() {
  if (!trapdoorServo.attached()) {
    trapdoorServo.attach(SERVO_PIN);
  }
  Serial.print(F(" -> Opening flap (driving for "));
  Serial.print(pulseDurationMs);
  Serial.println(F(" ms)..."));
  trapdoorServo.write(OPEN_CMD);
  delay(pulseDurationMs);
  stopMotor();
  Serial.println(F(" -> Open motion finished. Motor stopped & detached."));
}

void closeTrapdoor() {
  if (!trapdoorServo.attached()) {
    trapdoorServo.attach(SERVO_PIN);
  }
  Serial.print(F(" -> Closing flap (driving reverse for "));
  Serial.print(pulseDurationMs);
  Serial.println(F(" ms)..."));
  trapdoorServo.write(CLOSE_CMD);
  delay(pulseDurationMs);
  stopMotor();
  Serial.println(F(" -> Close motion finished. Flap locked & detached."));
}

void setup() {
  Serial.begin(115200);
  trapdoorServo.attach(SERVO_PIN);
  
  // Immediately send STOP command so the 360° servo does not spin on boot!
  stopMotor();

  Serial.println(F("=================================================="));
  Serial.println(F(" PCBCES Test 05: MG995 360° Continuous Trapdoor  "));
  Serial.println(F("=================================================="));
  Serial.println(F("Motor Status: STOPPED (Neutral 90 sent)"));
  Serial.print(F("Current Rotation Pulse Duration: "));
  Serial.print(pulseDurationMs);
  Serial.println(F(" ms"));
  Serial.println(F("Commands:"));
  Serial.println(F(" '1' -> Full Drop Cycle (Open -> Wait 1.5s -> Close)"));
  Serial.println(F(" 'o' -> Test OPEN pulse"));
  Serial.println(F(" 'c' -> Test CLOSE pulse"));
  Serial.println(F(" 's' -> Emergency STOP (write 90)"));
  Serial.println(F(" '+' -> Increase pulse duration (+50ms)"));
  Serial.println(F(" '-' -> Decrease pulse duration (-50ms)"));
  Serial.println(F("=================================================="));
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();

    if (cmd == '1') {
      Serial.println(F("\n[ACTION] Triggering Full Bottle Acceptance Drop..."));
      openTrapdoor();
      Serial.println(F(" -> Waiting 1.5s for bottle to drop under gravity..."));
      delay(1500);
      closeTrapdoor();
      Serial.println(F("[CYCLE COMPLETE] Ready for next bottle."));
    } 
    else if (cmd == 'o' || cmd == 'O') {
      Serial.println(F("\n[MANUAL] Open Pulse:"));
      openTrapdoor();
    } 
    else if (cmd == 'c' || cmd == 'C') {
      Serial.println(F("\n[MANUAL] Close Pulse:"));
      closeTrapdoor();
    } 
    else if (cmd == 's' || cmd == 'S') {
      Serial.println(F("\n[EMERGENCY] STOP command sent!"));
      stopMotor();
    } 
    else if (cmd == '+') {
      pulseDurationMs += 50;
      Serial.print(F("[CALIBRATION] Pulse duration increased to: "));
      Serial.print(pulseDurationMs);
      Serial.println(F(" ms"));
    } 
    else if (cmd == '-') {
      if (pulseDurationMs > 50) pulseDurationMs -= 50;
      Serial.print(F("[CALIBRATION] Pulse duration decreased to: "));
      Serial.print(pulseDurationMs);
      Serial.println(F(" ms"));
    }
  }
}