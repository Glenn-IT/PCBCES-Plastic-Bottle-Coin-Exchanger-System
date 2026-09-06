/*
 * PCBCES - Test 06: Coin Hopper & 5V Relay Payout Test
 * Hardware: Arduino Uno, 5V Relay Module, 220V/12V Coin Hopper, YT LP-08 A01 Sensor Board
 * Last Updated: 2026-09-06 20:15:00 (+08:00)
 * 
 * Clean Edge-Detection Polling (No PCINT / No Serial Ghost Triggering)
 * 
 * Objectives:
 * - Accurately detect each coin pulse as it passes the optical sensor
 * - Dispense 3 coins (₱3.00 for 290 ML quota)
 * - Dispense 20 coins (₱20.00 for 1.5L/1.75L quota)
 */

const int RELAY_PIN = 8;
const int COIN_PULSE_PIN = 7;

volatile int coinsDispensed = 0;
int currentTargetCoins = 3;
bool isDispensing = false;

int lastPinState = -1;
unsigned long lastPulseTime = 0;

void startPayout(int target) {
  currentTargetCoins = target;
  coinsDispensed = 0;
  isDispensing = true;
  lastPinState = digitalRead(COIN_PULSE_PIN);

  Serial.println();
  Serial.print(F(">>> STARTING PAYOUT: Target = "));
  Serial.print(target);
  Serial.print(F(" Coins ("));
  Serial.print(target);
  Serial.println(F(".00 PHP)"));
  Serial.println(F(">>> Relay ON -> Waiting for coins..."));

  // Turn Relay ON (Active LOW)
  digitalWrite(RELAY_PIN, LOW);
}

void stopPayout() {
  // Cut power to hopper motor
  digitalWrite(RELAY_PIN, HIGH);
  isDispensing = false;

  Serial.println();
  Serial.println(F("=================================================="));
  Serial.print(F(">>> PAYOUT COMPLETE! Total Coins Counted: "));
  Serial.println(coinsDispensed);
  Serial.println(F(">>> Relay OFF -> Motor Stopped."));
  Serial.println(F("=================================================="));
  Serial.println(F("Send '1' to dispense 3 coins, or '2' to dispense 20 coins:"));
}

void setup() {
  Serial.begin(115200);
  
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Relay OFF at startup

  pinMode(COIN_PULSE_PIN, INPUT_PULLUP);
  lastPinState = digitalRead(COIN_PULSE_PIN);

  Serial.println(F("=================================================="));
  Serial.println(F(" PCBCES Test 06: Coin Hopper Payout Testing       "));
  Serial.println(F(" Hardware: Pin D8 (Relay) | Pin D7 (Coin Pulse)  "));
  Serial.println(F("=================================================="));
  Serial.print(F("Initial Pin D7 State: "));
  Serial.println(lastPinState == HIGH ? F("HIGH") : F("LOW"));
  Serial.println(F("Drop a coin through the sensor now to test detection!"));
  Serial.println(F("Commands:"));
  Serial.println(F(" '1' or 'd' -> Dispense 3 Coins  (3.00 PHP - 290 ML Quota)"));
  Serial.println(F(" '2'        -> Dispense 20 Coins (20.00 PHP - 1.5L/1.75L Quota)"));
  Serial.println(F("--------------------------------------------------"));
}

void loop() {
  // Read Pin 7 state
  int currentPinState = digitalRead(COIN_PULSE_PIN);

  // Detect state change
  if (currentPinState != lastPinState) {
    unsigned long now = millis();

    // 50ms optical debounce to filter switch bounce / optical chatter
    if (now - lastPulseTime > 50) {
      coinsDispensed++;
      lastPulseTime = now;

      Serial.print(F("--> [COIN DETECTED!] Transition to: "));
      Serial.print(currentPinState == HIGH ? F("HIGH") : F("LOW"));
      Serial.print(F(" | Count = "));
      Serial.print(coinsDispensed);

      if (isDispensing) {
        Serial.print(F(" / "));
        Serial.println(currentTargetCoins);

        if (coinsDispensed >= currentTargetCoins) {
          stopPayout();
        }
      } else {
        Serial.println(F(" (Manual drop test)"));
      }
    }
    lastPinState = currentPinState;
  }

  // Handle Serial Commands
  if (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\r' || ch == '\n' || ch == ' ') return;

    if (!isDispensing) {
      if (ch == '1' || ch == 'd' || ch == 'D') {
        startPayout(3);
      } else if (ch == '2') {
        startPayout(20);
      }
    } else {
      if (ch == 'x' || ch == 'X' || ch == 's' || ch == 'S') {
        Serial.println(F(">>> EMERGENCY STOP!"));
        stopPayout();
      }
    }
  }
}