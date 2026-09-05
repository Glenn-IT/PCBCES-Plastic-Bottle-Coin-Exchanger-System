/*
 * PCBCES - Test 03: Ultrasonic HC-SR04 & IR Obstacle Sensor Test
 * Vertical Top-Down Chamber Setup (Ceiling Sensor to Floor Trapdoor: 40 cm)
 * Hardware: Arduino Uno, HC-SR04+ Ultrasonic Sensor, IR Obstacle Sensor
 * 
 * Vertical Measurement Logic:
 * - Total Chamber Height (Ceiling HC-SR04 to Trapdoor Base): 40 cm
 * - Empty Chamber: ~38 cm - 42 cm
 * - 1.5L / 1.7L Bottle (~30-33 cm height): Cap is NEAR sensor -> 5 cm to 13 cm
 * - Mismo Bottle (~18-19 cm height): Cap is FAR from sensor -> 18 cm to 24 cm
 * 
 * Pin Connections:
 * - HC-SR04 Trig -> D2
 * - HC-SR04 Echo -> D3
 * - IR Sensor OUT -> D4 (Active LOW when bottle is present)
 * - VCC -> 5V Rail
 * - GND -> Common GND Rail
 */

const int TRIG_PIN = 2;
const int ECHO_PIN = 3;
const int IR_PIN = 4;

const int CHAMBER_HEIGHT_CM = 40;
const int DIST_1_5L_MIN = 5;
const int DIST_1_5L_MAX = 13;
const int DIST_MISMO_MIN = 18;
const int DIST_MISMO_MAX = 24;

long getDistanceCm() {
  long readings[3];
  for (int i = 0; i < 3; i++) {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout (~5m max)
    readings[i] = (duration == 0) ? 999 : (duration * 0.034 / 2);
    delay(10);
  }
  // Median of 3 to remove flutter
  long a = readings[0], b = readings[1], c = readings[2];
  if ((a >= b && a <= c) || (a <= b && a >= c)) return a;
  if ((b >= a && b <= c) || (b <= a && b >= c)) return b;
  return c;
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(IR_PIN, INPUT);

  Serial.println(F("=========================================================="));
  Serial.println(F(" PCBCES Test 03: Vertical Chamber Distance & Height Check "));
  Serial.println(F(" Total Height: 40 cm (Ceiling Sensor to Base Trapdoor)     "));
  Serial.println(F(" 1.5L / 1.7L: 5 - 13 cm to cap | Mismo: 18 - 24 cm to cap "));
  Serial.println(F("=========================================================="));
}

void loop() {
  bool bottleAtEntry = (digitalRead(IR_PIN) == LOW);
  long distToCap = getDistanceCm();
  long approxHeight = (CHAMBER_HEIGHT_CM > distToCap) ? (CHAMBER_HEIGHT_CM - distToCap) : 0;

  Serial.print(F("IR Entry: ["));
  Serial.print(bottleAtEntry ? F("BOTTLE PRESENT") : F("CLEAR         "));
  Serial.print(F("] | Ceiling-to-Cap: "));
  Serial.print(distToCap);
  Serial.print(F(" cm (~Height: "));
  Serial.print(approxHeight);
  Serial.print(F(" cm) | Classification: "));

  if (bottleAtEntry) {
    if (distToCap >= DIST_1_5L_MIN && distToCap <= DIST_1_5L_MAX) {
      Serial.println(F("1.5L / 1.7L BOTTLE DETECTED (NEAR)"));
    } else if (distToCap >= DIST_MISMO_MIN && distToCap <= DIST_MISMO_MAX) {
      Serial.println(F("MISMO BOTTLE DETECTED (FAR)"));
    } else {
      Serial.println(F("UNKNOWN / UNALIGNED BOTTLE"));
    }
  } else {
    Serial.println(F("Waiting for insertion..."));
  }

  delay(400);
}