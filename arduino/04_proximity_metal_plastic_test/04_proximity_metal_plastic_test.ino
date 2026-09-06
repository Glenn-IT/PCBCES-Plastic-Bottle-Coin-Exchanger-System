/*
 * PCBCES - Test 04: 12V LJ12A3 Inductive Metal Proximity Sensor Test
 * (Note: LJC18A3 Capacitive Sensor on D5 removed/omitted; Pin D5 is now Spare GPIO)
 * 
 * Sensor Specification: LJ12A3-4-Z/BX (NPN Normally Open, 4mm Sensing Distance)
 * Wire Connections:
 * - BN (Brown): +12V DC Power Rail (Positive)
 * - BU (Blue):  Common Star GND (Negative / 5-36VDC Ground)
 * - BK (Black): NO Signal Output -> 10k/(4.7k or 5k) Resistor Divider -> Arduino Pin D6
 * 
 * Voltage Divider Circuit:
 * - LJ12A3 Black Wire -> 10k Ohm Resistor -> Node (Arduino Pin D6)
 * - Node (Arduino Pin D6) -> 4.7k or 5k Ohm Resistor -> Common Star GND
 * - Output Voltage:
 *     * With 5k Ohm:   12V * (5k / 15k)     = 4.00V DC (Safe 5V TTL Logic HIGH)
 *     * With 4.7k Ohm: 12V * (4.7k / 14.7k) = ~3.84V DC (Safe 5V TTL Logic HIGH)
 * 
 * Logic Operation:
 * - No Metal:    Sensor output is pulled to 12V -> Scaled to ~3.8V-4.0V -> D6 reads HIGH (Ready)
 * - Metal Found: NPN switch pulls to GND -> Scaled to ~0.0V -> D6 reads LOW (Reject)
 * - Rear Red LED on sensor illuminates when metal is within ~2-4mm.
 * 
 * Indicators:
 * - Red LED   -> D13 (Metal alert / instant reject)
 * - Green LED -> A2  (Clear / ready indicator)
 * - Pin D5    -> Spare / Unassigned GPIO
 * 
 * Last Updated: 2026-09-06 16:46:00 (+08:00)
 */

const int IND_METAL_PIN   = 6; // LJ12A3 via 10k/(4.7k or 5k) voltage divider
const int RED_LED_PIN     = 13;
const int GREEN_LED_PIN   = A2;

void setup() {
  Serial.begin(115200);
  pinMode(IND_METAL_PIN, INPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);

  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);

  Serial.println(F("=========================================================="));
  Serial.println(F(" PCBCES Test 04: 12V LJ12A3 Inductive Metal Sensor Test    "));
  Serial.println(F(" Wiring: BN->+12V | BU->GND | BK->10k/(4.7k or 5k)->D6    "));
  Serial.println(F(" Logic: HIGH (~3.8V-4.0V) = Clear | LOW (~0.0V) = Metal   "));
  Serial.println(F(" (LJC18A3 Capacitive Sensor Omitted - Pin D5 is Spare)    "));
  Serial.println(F("=========================================================="));
}

void loop() {
  // LJ12A3 NPN NO sensor pulls to GND (LOW) when metal is within sensing field
  int rawState = digitalRead(IND_METAL_PIN);
  bool metalDetected = (rawState == LOW);

  Serial.print(F("Pin D6: ["));
  Serial.print(rawState == HIGH ? F("HIGH (~3.8V-4.0V)") : F("LOW  (~0.0V)     "));
  Serial.print(F("] | Inductive Status: "));

  if (metalDetected) {
    // Metal Detected -> Trigger Red LED & Alert
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, LOW);
    Serial.println(F("[REJECT] --> METALLIC OBJECT DETECTED!"));
  } else {
    // Non-Metal / Plastic Bottle -> Green LED Ready
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, HIGH);
    Serial.println(F("[READY ] --> Clear / Non-Metal (Plastic Bottle)"));
  }

  delay(300);
}