/*
 * PCBCES - Test 06: 12V Coin Hopper & 5V Relay Payout Test
 * Hardware: Arduino Uno, 5V Relay Module, 12V Coin Hopper, 10k/4.7k Voltage Divider
 * Last Updated: 2026-09-06 14:30:41 (+08:00)
 * 
 * Pin Connections:
 * - 5V Relay IN -> D8 (Active LOW or HIGH depending on relay board)
 * - Hopper Coin Signal -> 10k/4.7k Resistor Divider -> D7 (Interrupt / Pulse In)
 * - Hopper Motor Power -> 12V PSU via Relay COM & NO contacts
 * 
 * Objective:
 * - Accurately dispense exactly 3 coins (3 x 1 Peso) and immediately stop motor!
 */

const int RELAY_PIN = 8;
const int COIN_PULSE_PIN = 7;

volatile int coinsDispensed = 0;
const int TARGET_COINS = 3; // 3.00 Pesos (3 x 1 Peso coins)
bool isDispensing = false;

// Interrupt Service Routine for Coin Pulse
void countCoinPulse() {
  static unsigned long lastPulseTime = 0;
  unsigned long now = millis();
  if (now - lastPulseTime > 60) { // 60ms debounce for optical/hopper switch
    coinsDispensed++;
    lastPulseTime = now;
  }
}

void startPayout(int target = 3) {
  Serial.print(F("Starting Payout of "));
  Serial.print(target);
  Serial.println(F(" Coins (3.00 PHP)..."));

  coinsDispensed = 0;
  isDispensing = true;
  
  // Turn Relay ON to start 12V hopper motor
  // Most 5V relay modules are Active LOW (LOW = ON, HIGH = OFF)
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
  // Attach interrupt or polling
  attachInterrupt(digitalPinToInterrupt(COIN_PULSE_PIN), countCoinPulse, FALLING);

  Serial.println(F("=================================================="));
  Serial.println(F(" PCBCES Test 06: 12V Coin Hopper Payout Testing   "));
  Serial.println(F("=================================================="));
  Serial.println(F("Type 'd' in Serial Monitor to dispense 3 coins!"));
}

void loop() {
  if (isDispensing) {
    Serial.print(F("Dispensing... Current Count: "));
    Serial.println(coinsDispensed);

    if (coinsDispensed >= TARGET_COINS) {
      stopPayout();
    }
    delay(50);
  }

  if (Serial.available()) {
    char ch = Serial.read();
    if ((ch == 'd' || ch == 'D') && !isDispensing) {
      startPayout(TARGET_COINS);
    }
  }
}