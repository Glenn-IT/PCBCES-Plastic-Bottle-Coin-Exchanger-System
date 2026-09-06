/*
 * PCBCES - Test 06: 12V Coin Hopper & 5V Relay Payout Test
 * Hardware: Arduino Uno, 5V Relay Module, 12V Coin Hopper, 10k/4.7k Voltage Divider
 * Last Updated: 2026-09-06 18:35:00 (+08:00)
 * 
 * Pin Connections:
 * - 5V Relay IN -> D8 (Active LOW relay trigger)
 * - Hopper Coin Signal -> 10k/4.7k or 5k Resistor Divider -> D7 (Interrupt / Pulse In)
 * - Hopper Motor Power -> 12V PSU via Relay COM & NO contacts
 * 
 * Objectives:
 * - Accurately dispense 3 coins (3 x 1 Peso = ₱3.00 for 290 ML quota)
 * - Accurately dispense 20 coins (20 x 1 Peso = ₱20.00 for 1.5L/1.75L quota)
 */

const int RELAY_PIN = 8;
const int COIN_PULSE_PIN = 7;

volatile int coinsDispensed = 0;
int currentTargetCoins = 3;
bool isDispensing = false;

// Interrupt Service Routine for Coin Pulse
void countCoinPulse() {
  static unsigned long lastPulseTime = 0;
  unsigned long now = millis();
  if (now - lastPulseTime > 60) { // 60ms debounce for optical switch
    coinsDispensed++;
    lastPulseTime = now;
  }
}

void startPayout(int target) {
  currentTargetCoins = target;
  Serial.print(F("Starting Payout of "));
  Serial.print(target);
  Serial.print(F(" Coins ("));
  Serial.print(target);
  Serial.println(F(".00 PHP)..."));

  coinsDispensed = 0;
  isDispensing = true;
  
  // Turn Relay ON to start 12V hopper motor (Active LOW)
  digitalWrite(RELAY_PIN, LOW); 
}

void stopPayout() {
  // Cut 12V power to hopper motor
  digitalWrite(RELAY_PIN, HIGH);
  isDispensing = false;

  Serial.print(F("PAYOUT COMPLETE! Total Coins Counted: "));
  Serial.println(coinsDispensed);
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Ensure relay is OFF on startup

  pinMode(COIN_PULSE_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(COIN_PULSE_PIN), countCoinPulse, FALLING);

  Serial.println(F("=================================================="));
  Serial.println(F(" PCBCES Test 06: 12V Coin Hopper Payout Testing   "));
  Serial.println(F("=================================================="));
  Serial.println(F("Send via Serial Monitor:"));
  Serial.println(F(" '1' or 'd' -> Dispense 3 Coins  (3.00 PHP - 290 ML Quota)"));
  Serial.println(F(" '2'        -> Dispense 20 Coins (20.00 PHP - 1.5L/1.75L Quota)"));
}

void loop() {
  if (isDispensing) {
    Serial.print(F("Dispensing... Current Count: "));
    Serial.println(coinsDispensed);

    if (coinsDispensed >= currentTargetCoins) {
      stopPayout();
    }
    delay(50);
  }

  if (Serial.available()) {
    char ch = Serial.read();
    if (!isDispensing) {
      if (ch == '1' || ch == 'd' || ch == 'D') {
        startPayout(3);
      } else if (ch == '2') {
        startPayout(20);
      }
    }
  }
}