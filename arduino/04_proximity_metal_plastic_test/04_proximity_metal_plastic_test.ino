/*
 * PCBCES - Test 04: 12V LJ12A3 Inductive Metal Proximity Sensor Test
 * (Note: LJC18A3 Capacitive Sensor on D5 removed/omitted; Pin D5 is now Spare GPIO)
 * 
 * Hardware: 
 * - LJ12A3-4-Z/BX Inductive Proximity Sensor (Metal detection)
 * - 10k & 4.7k Resistor Divider (Drops 12V sensor signal to safe ~3.8V logic)
 * - Red LED -> D13 (Metal alert / instant reject)
 * - Green LED -> A2 (Clear / ready indicator)
 * 
 * Pin Connections:
 * - LJ12A3 (Metal) -> 10k/4.7k Resistor Divider -> Arduino D6
 * - Red LED -> D13
 * - Green LED -> A2
 * - (Pin D5: Spare / Unassigned)
 */

const int IND_METAL_PIN   = 6; // LJ12A3 via 10k/4.7k voltage divider
const int RED_LED_PIN     = 13;
const int GREEN_LED_PIN   = A2;

void setup() {
  Serial.begin(115200);
  pinMode(IND_METAL_PIN, INPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);

  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);

  Serial.println(F("=================================================="));
  Serial.println(F(" PCBCES Test 04: 12V LJ12A3 Metal Sensor Test     "));
  Serial.println(F(" (LJC18A3 Capacitive Sensor Omitted - D5 Spare)  "));
  Serial.println(F("=================================================="));
}

void loop() {
  // LJ12A3 NPN NO sensor pulls LOW when metal is detected
  bool metalDetected = (digitalRead(IND_METAL_PIN) == LOW);

  Serial.print(F("Inductive [Metal on D6]: "));
  if (metalDetected) {
    // Immediate Reject
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, LOW);
    Serial.println(F("TRIGGERED (METAL DETECTED!) --> [REJECT]"));
  } else {
    // Clear / Non-metal
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, HIGH);
    Serial.println(F("CLEAR (NON-METAL)           --> [READY]"));
  }

  delay(350);
}