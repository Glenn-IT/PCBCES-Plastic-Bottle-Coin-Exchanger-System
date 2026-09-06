/*
 * PCBCES - Test 03: Ultrasonic HC-SR04 & IR Obstacle Sensor Test
 * Vertical Top-Down Chamber Setup (Ceiling Sensor to Floor Trapdoor: 43 cm Calibrated)
 * Hardware: Arduino Uno, HC-SR04+ Ultrasonic Sensor, IR Obstacle Sensor
 * 
 * Vertical Measurement Logic:
 * - Total Chamber Height (Ceiling HC-SR04 to Trapdoor Base): 43 cm
 * - Empty Chamber: ~41 cm - 45 cm
 * - 1.5L / 1.75L Bottle (~30-33 cm height): Cap is NEAR sensor -> 7 cm to 15 cm
 * - 290 ML Bottle (~16-17 cm height): Cap is FAR from sensor -> 26 cm to 27 cm
 * 
 * Pin Connections:
 * - HC-SR04 Trig -> D2
 * - HC-SR04 Echo -> D3
 * - IR Sensor OUT (Entry)   -> D4 (Active LOW when bottle is present)
 * - IR Sensor OUT (Bin-Full)-> D5 (Active LOW when bottle storage bin is full)
 * - VCC                     -> 5V Rail
 * - GND                     -> Common GND Rail
 * 
 * Last Updated: 2026-09-07 02:43:00 (+08:00)
 */

const int TRIG_PIN = 2;
const int ECHO_PIN = 3;
const int IR_PIN = 4;
const int IR_BIN_PIN = 5;

const int CHAMBER_HEIGHT_CM = 43; // Physical distance from ceiling HC-SR04 to floor trapdoor
const int DIST_1_5L_MIN = 7;      // 1.5L/1.75L cap is ~7-15 cm from ceiling (~28-36 cm tall bottle)
const int DIST_1_5L_MAX = 15;
const int DIST_290ML_MIN = 26;    // 290 ML cap is ~26-27 cm from ceiling (~16-17 cm tall bottle)
const int DIST_290ML_MAX = 27;

// Backward-compatibility aliases
const int DIST_MISMO_MIN = DIST_290ML_MIN;
const int DIST_MISMO_MAX = DIST_290ML_MAX;

// Single ping with bounded timeout (~102 cm max) to prevent stray echo listening
long singlePingCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(4);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // 6000us timeout = ~102 cm max range (plenty for 43 cm chamber)
  long duration = pulseIn(ECHO_PIN, HIGH, 6000);
  if (duration <= 0) return CHAMBER_HEIGHT_CM; // Timeout = sound dissipated or empty floor
  long cm = duration * 0.034 / 2;
  if (cm < 2 || cm > 60) return CHAMBER_HEIGHT_CM;
  return cm;
}

// 5-Sample Median Filter with 30ms Acoustic Decay Delay
long getDistanceCm() {
  const int NUM_SAMPLES = 5;
  long samples[NUM_SAMPLES];

  for (int i = 0; i < NUM_SAMPLES; i++) {
    samples[i] = singlePingCm();
    delay(30); // 30ms inter-ping acoustic dissipation delay (kills chamber echo ringing)
  }

  // Insertion sort to extract true median (rejects acoustic flutter & sidewall glitches)
  for (int i = 1; i < NUM_SAMPLES; i++) {
    long key = samples[i];
    int j = i - 1;
    while (j >= 0 && samples[j] > key) {
      samples[j + 1] = samples[j];
      j--;
    }
    samples[j + 1] = key;
  }

  return samples[NUM_SAMPLES / 2]; // Return median (index 2)
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(IR_PIN, INPUT);
  pinMode(IR_BIN_PIN, INPUT);

  Serial.println(F("=========================================================="));
  Serial.println(F(" PCBCES Test 03: Height & Dual IR Sensors (Entry & Bin)   "));
  Serial.println(F(" Total Height: 43 cm (Ceiling Sensor to Base Trapdoor)     "));
  Serial.println(F(" Pins: D2 Trig | D3 Echo | D4 IR Entry | D5 IR Bin Full   "));
  Serial.println(F("=========================================================="));
}

void loop() {
  bool bottleAtEntry = (digitalRead(IR_PIN) == LOW);
  bool binIsFull = (digitalRead(IR_BIN_PIN) == LOW);
  long distToCap = getDistanceCm();
  long approxHeight = (CHAMBER_HEIGHT_CM > distToCap) ? (CHAMBER_HEIGHT_CM - distToCap) : 0;

  Serial.print(F("IR Entry (D4): ["));
  Serial.print(bottleAtEntry ? F("BOTTLE PRESENT") : F("CLEAR         "));
  Serial.print(F("] | Bin Full (D5): ["));
  Serial.print(binIsFull ? F("BIN FULL!") : F("CLEAR    "));
  Serial.print(F("] | Cap Dist: "));
  Serial.print(distToCap);
  Serial.print(F(" cm | "));

  if (bottleAtEntry) {
    if (distToCap >= DIST_1_5L_MIN && distToCap <= DIST_1_5L_MAX) {
      Serial.println(F("1.5L / 1.75L BOTTLE DETECTED (NEAR)"));
    } else if (distToCap >= DIST_290ML_MIN && distToCap <= DIST_290ML_MAX) {
      Serial.println(F("290 ML BOTTLE DETECTED (FAR)"));
    } else if (distToCap >= (CHAMBER_HEIGHT_CM - 3)) {
      Serial.println(F("EMPTY / OBSTACLE AT BEAM BUT NO BOTTLE"));
    } else {
      Serial.println(F("UNKNOWN / UNALIGNED BOTTLE"));
    }
  } else {
    Serial.println(F("Waiting for bottle insertion..."));
  }

  delay(400);
}