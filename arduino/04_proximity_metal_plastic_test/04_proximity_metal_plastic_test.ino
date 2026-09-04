/*
 * PCBCES - Test 04: 12V Proximity Sensors (Metal Reject & Plastic Detect)
 * Hardware: 
 * - LJ12A3-4-Z/BX Inductive Proximity Sensor (Metal detection)
 * - LJC18A3-B-Z/BX Capacitive Proximity Sensor (Plastic detection)
 * - 10k & 4.7k Resistor Dividers (Drop 12V signals to safe ~3.8V logic)
 * - Red LED -> D13, Green LED -> A2
 * 
 * Pin Connections:
 * - LJ12A3 (Metal) -> Resistor Divider -> Arduino D6
 * - LJC18A3 (Plastic) -> Resistor Divider -> Arduino D5
 * - Red LED -> D13
 * - Green LED -> A2
 */

const int CAP_PLASTIC_PIN = 5; // LJC18A3 via voltage divider
const int IND_METAL_PIN   = 6; // LJ12A3 via voltage divider
const int RED_LED_PIN     = 13;
const int GREEN_LED_PIN   = A2;

void setup() {
  Serial.begin(115200);
  pinMode(CAP_PLASTIC_PIN, INPUT);
  pinMode(IND_METAL_PIN, INPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);

  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);

  Serial.println(F("=================================================="));
  Serial.println(F(" PCBCES Test 04: 12V Proximity Sensor Validation "));
  Serial.println(F("=================================================="));
}

void loop() {
  // NPN sensors pull LOW or HIGH depending on internal transistor
  // Usually NPN NO drops to LOW when activated, or HIGH via divider
  bool metalDetected = (digitalRead(IND_METAL_PIN) == LOW);
  bool plasticDetected = (digitalRead(CAP_PLASTIC_PIN) == LOW);

  Serial.print(F("Inductive [Metal]: "));
  Serial.print(metalDetected ? F("TRIGGERED (METAL!) ") : F("CLEAR             "));
  Serial.print(F("| Capacitive [Plastic]: "));
  Serial.println(plasticDetected ? F("TRIGGERED (PLASTIC)") : F("CLEAR              "));

  if (metalDetected) {
    // Immediate Reject
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, LOW);
    Serial.println(F("--> ALERT: Metal detected! Instant Reject."));
  } else if (plasticDetected) {
    // Valid Plastic
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, HIGH);
    Serial.println(F("--> OK: Pure plastic bottle verified."));
  } else {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, LOW);
  }

  delay(350);
}