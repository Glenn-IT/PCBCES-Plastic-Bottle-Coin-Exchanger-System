/*
 * PCBCES - Test 03: Ultrasonic HC-SR04 & IR Obstacle Sensor Test
 * Hardware: Arduino Uno, HC-SR04+ Ultrasonic Sensor, IR Obstacle Sensor
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

long getDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout (~5m max)
  if (duration == 0) return 999; // out of range
  return duration * 0.034 / 2;
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(IR_PIN, INPUT);

  Serial.println(F("================================================"));
  Serial.println(F(" PCBCES Test 03: Bottle Height & Entry Detection "));
  Serial.println(F("================================================"));
}

void loop() {
  bool bottleAtEntry = (digitalRead(IR_PIN) == LOW);
  long dist = getDistanceCm();

  Serial.print(F("IR Entry: ["));
  Serial.print(bottleAtEntry ? F("BOTTLE PRESENT") : F("CLEAR         "));
  Serial.print(F("] | Chamber Distance: "));
  Serial.print(dist);
  Serial.print(F(" cm | Classification: "));

  if (bottleAtEntry) {
    if (dist >= 26 && dist <= 35) {
      Serial.println(F("1.5 LITER BOTTLE DETECTED"));
    } else if (dist >= 16 && dist <= 23) {
      Serial.println(F("MISMO BOTTLE DETECTED"));
    } else {
      Serial.println(F("UNKNOWN / UNALIGNED BOTTLE"));
    }
  } else {
    Serial.println(F("Waiting for insertion..."));
  }

  delay(400);
}