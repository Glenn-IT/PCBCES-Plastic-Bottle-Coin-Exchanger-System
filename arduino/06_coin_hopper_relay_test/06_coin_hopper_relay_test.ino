/*
 * PCBCES - Test 06: Coin Hopper & 5V Relay Payout Test
 * Hardware: Arduino Uno, 5V Relay Module, 220V/12V Coin Hopper, YT LP-08 A01 Sensor Board
 * Last Updated: 2026-09-06 20:00:00 (+08:00)
 * 
 * Pin Connections:
 * - 5V Relay IN         -> D8 (Active LOW relay trigger)
 * - Hopper Coin Signal  -> D7 (Pin Change Interrupt & Fast Digital Polling)
 * - Hopper Sensor VCC   -> 5V Rail
 * - Hopper Sensor GND   -> Arduino GND
 * 
 * Note for Arduino Uno:
 * Pin 7 is NOT an external INT0/INT1 pin. This sketch uses Pin Change Interrupt (PCINT23)
 * AND ultra-fast edge polling in loop() to guarantee 100% reliable coin pulse detection!
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

volatile unsigned long lastPulseTime = 0;
int lastPinState = HIGH;

// Pin Change Interrupt Service Routine for Port D (Pins 0-7, PCINT16-23)
ISR(PCINT2_vect) {
  uint8_t pinVal = (PIND & (1 << PIND7)) ? HIGH : LOW;
  unsigned long now = millis();
  
  // Detect falling edge (or pulse transition) with 40ms debounce
  if (pinVal == LOW && (now - lastPulseTime > 40)) {
    coinsDispensed++;
    lastPulseTime = now;
  }
}

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
  Serial.println(F(">>> Relay ON -> Dispensing coins... Drop/dispense coins now!"));

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
  Serial.println(F("Ready for next command ('1' for 3 coins, '2' for 20 coins):"));
}

void setup() {
  Serial.begin(115200);
  
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Ensure relay is OFF at startup

  pinMode(COIN_PULSE_PIN, INPUT_PULLUP);
  lastPinState = digitalRead(COIN_PULSE_PIN);

  // Configure Pin Change Interrupt for Pin 7 (PCINT23 on Port D)
  PCICR |= (1 << PCIE2);     // Enable PCINT for Port D
  PCMSK2 |= (1 << PCINT23);  // Enable mask for Pin 7

  Serial.println(F("=================================================="));
  Serial.println(F(" PCBCES Test 06: Coin Hopper Payout Testing       "));
  Serial.println(F(" Hardware: Pin D8 (Relay) | Pin D7 (Coin Pulse)  "));
  Serial.println(F(" Mode: Pin Change Interrupt + Fast Edge Polling   "));
  Serial.println(F("=================================================="));
  Serial.print(F("Initial Pin D7 State: "));
  Serial.println(lastPinState == HIGH ? F("HIGH (Idle)") : F("LOW (Active)"));
  Serial.println(F("Tip: You can pass a coin anytime to test the sensor!"));
  Serial.println(F("Commands:"));
  Serial.println(F(" '1' or 'd' -> Dispense 3 Coins  (3.00 PHP - 290 ML Quota)"));
  Serial.println(F(" '2'        -> Dispense 20 Coins (20.00 PHP - 1.5L/1.75L Quota)"));
  Serial.println(F("--------------------------------------------------"));
}

void loop() {
  // Ultra-fast zero-delay edge polling backup
  int currentPinState = digitalRead(COIN_PULSE_PIN);
  if (currentPinState != lastPinState) {
    unsigned long now = millis();
    
    // Check for transition with 40ms debounce
    if (now - lastPulseTime > 40) {
      if (currentPinState == LOW) { // Falling edge
        // Only increment if ISR hasn't already counted this pulse within 40ms
        coinsDispensed++;
        lastPulseTime = now;

        Serial.print(F("--> [COIN DETECTED!] Count = "));
        Serial.print(coinsDispensed);
        if (isDispensing) {
          Serial.print(F(" / "));
          Serial.println(currentTargetCoins);
        } else {
          Serial.println(F(" (Manual drop test)"));
        }
      }
    }
    lastPinState = currentPinState;
  }

  // Check if payout target reached
  if (isDispensing) {
    if (coinsDispensed >= currentTargetCoins) {
      stopPayout();
    }
  }

  // Handle Serial Commands
  if (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\r' || ch == '\n' || ch == ' ') return; // ignore whitespace

    if (!isDispensing) {
      if (ch == '1' || ch == 'd' || ch == 'D') {
        startPayout(3);
      } else if (ch == '2') {
        startPayout(20);
      } else {
        Serial.print(F("Unknown command: "));
        Serial.println(ch);
      }
    } else {
      if (ch == 'x' || ch == 'X' || ch == 's' || ch == 'S') {
        Serial.println(F(">>> EMERGENCY STOP REQUESTED!"));
        stopPayout();
      }
    }
  }
}