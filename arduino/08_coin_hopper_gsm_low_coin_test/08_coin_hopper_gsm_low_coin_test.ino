/*
 * PCBCES - Test 08: Coin Hopper & GSM SMS Low-Coin / Jam Alert Test
 * Hardware: Arduino Uno, 5V Relay, 12V/220V Coin Hopper, SIM900A / SIM800L GSM Module
 * Last Updated: 2026-09-18 22:20:00 (+08:00)
 * 
 * Pin Connections:
 * - Pin D8  -> 5V Relay Module IN (Switches Coin Hopper Motor Power)
 * - Pin D7  -> Coin Hopper Pulse Line (Optical sensor falling edge, 1 pulse = 1 PHP)
 * - Pin D11 -> GSM TX Pin (SIM900A 5VT / SIM800L TX) -> SoftwareSerial RX
 * - Pin A3  -> GSM RX Pin (SIM900A 5VR / SIM800L RX) -> SoftwareSerial TX
 * - Pin D12 -> Active 5V Buzzer (Audio Alert on Timeout/Jam)
 * - Pin D13 -> Red LED (Visual Fault Alert)
 * - Pin A2  -> Green LED (Dispensing Indicator)
 * 
 * Safety & Autonomous Alert Logic:
 * - Motor auto-cutoff: If the hopper motor is running and NO coin pulse is detected
 *   for 5 SECONDS (5000 ms), the motor is instantly halted to prevent dry spinning.
 * - Automated SMS Dispatch: When the 5-second timeout fires, the GSM module automatically
 *   dispatches an emergency SMS alert to the administrator phone:
 *   "ALERT: PCBCES Coin Hopper is EMPTY or JAMMED! No coin dispensed for 5 seconds..."
 */

#include <SoftwareSerial.h>

// SoftwareSerial (RX on D11 from GSM TX, TX on A3 to GSM RX)
SoftwareSerial gsm(11, A3);

// Hardware Pins
const int PIN_RELAY_HOPPER = 8;
const int PIN_COIN_PULSE   = 7;
const int PIN_BUZZER       = 12;
const int PIN_LED_RED      = 13;
const int PIN_LED_GREEN    = A2;

// Safety Cutoff Threshold
const unsigned long COIN_TIMEOUT_MS = 5000; // 5 seconds with no coin pulse triggers shutdown & SMS

// Target Administrator Phone Numbers (Philippines format: +639XXXXXXXXX or 09XXXXXXXXX)
const char ADMIN_PHONE_1[] = "+639634299114";
const char ADMIN_PHONE_2[] = "+639242074903";

// Runtime Variables
volatile int coinsDispensed = 0;
int currentTargetCoins = 3;
bool isDispensing = false;

int lastPinState = HIGH;
unsigned long lastPulseTime = 0;
unsigned long payoutStartTime = 0;

void soundBeep(int ms = 80) {
  digitalWrite(PIN_BUZZER, HIGH);
  delay(ms);
  digitalWrite(PIN_BUZZER, LOW);
}

void soundAlarm() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(PIN_BUZZER, HIGH);
    digitalWrite(PIN_LED_RED, HIGH);
    delay(150);
    digitalWrite(PIN_BUZZER, LOW);
    digitalWrite(PIN_LED_RED, LOW);
    delay(100);
  }
}

void sendLowCoinToNumber(const char* recipient, int dispensed, int target) {
  Serial.print(F("[GSM ALERT] Recipient: "));
  Serial.println(recipient);

  // Switch GSM to text mode
  gsm.println("AT+CMGF=1");
  delay(400);

  // Set recipient phone number
  gsm.print("AT+CMGS=\"");
  gsm.print(recipient);
  gsm.println("\"");
  delay(400);

  // Compose SMS body
  gsm.print("ALERT: PCBCES Coin Hopper is EMPTY or JAMMED! No coin dispensed for 5 seconds during payout. Dispensed: ");
  gsm.print(dispensed);
  gsm.print("/");
  gsm.print(target);
  gsm.print(" coins. Motor stopped. Please refill 1-peso coins.");
  delay(400);

  // Send Ctrl+Z (ASCII 26) to trigger network transmission
  gsm.write(26);
  Serial.println(F("[GSM ALERT] Waiting for cellular confirmation..."));

  unsigned long startWait = millis();
  while (millis() - startWait < 5000) {
    while (gsm.available()) {
      char c = gsm.read();
      Serial.write(c);
    }
  }
}

void sendLowCoinSMS(int dispensed, int target) {
  Serial.println(F("\n=================================================="));
  Serial.println(F("[GSM ALERT] Dispatching Low-Coin SMS to Admins... "));
  Serial.println(F("=================================================="));
  sendLowCoinToNumber(ADMIN_PHONE_1, dispensed, target);
  delay(2000);
  sendLowCoinToNumber(ADMIN_PHONE_2, dispensed, target);
  Serial.println(F("\n[GSM ALERT] SMS transmission cycle complete for all admins!"));
}

void startPayout(int target) {
  currentTargetCoins = target;
  coinsDispensed = 0;

  Serial.println();
  Serial.println(F("--------------------------------------------------"));
  Serial.print(F(">>> STARTING PAYOUT: Target = "));
  Serial.print(target);
  Serial.print(F(" Coins (₱"));
  Serial.print(target);
  Serial.println(F(".00 PHP)"));
  Serial.println(F(">>> Relay ON -> Monitoring optical pulses on D7..."));
  Serial.println(F(">>> 5-Second Timeout Active: Will stop & send SMS if empty."));
  Serial.println(F("--------------------------------------------------"));

  digitalWrite(PIN_LED_GREEN, HIGH);
  digitalWrite(PIN_LED_RED, LOW);
  
  // Turn Relay ON (Active LOW triggers 12V/220V hopper motor)
  digitalWrite(PIN_RELAY_HOPPER, LOW);
  payoutStartTime = millis();
  isDispensing = true;

  // Mask electrical relay contact transient
  delay(50);
  lastPinState = digitalRead(PIN_COIN_PULSE);
  lastPulseTime = millis();
}

void stopPayout() {
  // Turn Hopper Relay OFF
  digitalWrite(PIN_RELAY_HOPPER, HIGH);
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_RED, LOW);
  isDispensing = false;

  soundBeep(100);
  delay(100);
  soundBeep(100);

  Serial.println();
  Serial.println(F("=================================================="));
  Serial.print(F(">>> PAYOUT SUCCESS! Dispensed: "));
  Serial.print(coinsDispensed);
  Serial.print(F(" / "));
  Serial.print(currentTargetCoins);
  Serial.println(F(" Coins."));
  Serial.println(F(">>> Relay OFF -> Motor Stopped. Hopper Healthy."));
  Serial.println(F("=================================================="));
  Serial.println(F("Enter '1' (3 coins) or '2' (20 coins) to test again."));
}

void timeoutPayout() {
  // Cut power to hopper motor immediately
  digitalWrite(PIN_RELAY_HOPPER, HIGH);
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_RED, HIGH);
  isDispensing = false;

  Serial.println();
  Serial.println(F("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"));
  Serial.println(F(">>> [SAFETY TIMEOUT] NO COIN DETECTED FOR 5 SECONDS!"));
  Serial.print(F(">>> Coins Dispensed: "));
  Serial.print(coinsDispensed);
  Serial.print(F(" / "));
  Serial.println(currentTargetCoins);
  Serial.println(F(">>> Motor HALTED to protect hardware."));
  Serial.println(F(">>> Triggering automated SMS alert via GSM module..."));
  Serial.println(F("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"));

  // Sound alarm
  soundAlarm();

  // Trigger automated SMS alert to admin phone
  sendLowCoinSMS(coinsDispensed, currentTargetCoins);

  Serial.println(F("\nRefill coins into hopper, then enter '1' or '2' to test again."));
}

bool autoSyncGSMBaud() {
  const long candidateBauds[] = { 9600, 115200, 38400, 19200, 57600, 4800 };
  const int count = sizeof(candidateBauds) / sizeof(candidateBauds[0]);

  Serial.println(F("[GSM] Probing communication baud rates..."));

  for (int i = 0; i < count; i++) {
    long testBaud = candidateBauds[i];
    Serial.print(F("  -> Testing "));
    Serial.print(testBaud);
    Serial.print(F(" baud... "));

    gsm.begin(testBaud);
    delay(150);
    while (gsm.available()) gsm.read();

    bool detected = false;
    for (int attempt = 0; attempt < 3; attempt++) {
      gsm.print("AT\r\n");
      unsigned long start = millis();
      String resp = "";
      while (millis() - start < 450) {
        while (gsm.available()) {
          char c = gsm.read();
          resp += c;
          if (resp.indexOf("OK") != -1) {
            detected = true;
            break;
          }
        }
        if (detected) break;
      }
      if (detected) break;
      delay(150);
    }

    if (detected) {
      Serial.println(F("[LOCKED! Handshake OK]"));
      if (testBaud != 9600) {
        Serial.println(F("  -> Reconfiguring module to 9600 baud for stable SoftwareSerial..."));
        gsm.print("AT+IPR=9600\r\n");
        delay(250);
        gsm.print("AT&W\r\n");
        delay(250);
        gsm.begin(9600);
        delay(150);
        while (gsm.available()) gsm.read();
        Serial.println(F("  -> Module baud locked to 9600 permanently!"));
      }
      return true;
    } else {
      Serial.println(F("[No reply]"));
    }
  }

  // Fallback: Default to 9600 and train autobaud
  Serial.println(F("[WARN] No standard response. Training auto-baud at 9600 baud..."));
  gsm.begin(9600);
  delay(200);
  for (int i = 0; i < 5; i++) {
    gsm.print("AT\r\n");
    delay(300);
  }
  while (gsm.available()) gsm.read();
  return false;
}

void setup() {
  Serial.begin(115200);

  // Relay Setup (Active LOW: HIGH = OFF)
  pinMode(PIN_RELAY_HOPPER, OUTPUT);
  digitalWrite(PIN_RELAY_HOPPER, HIGH);

  // Coin Sensor Input (Active LOW optical pulse)
  pinMode(PIN_COIN_PULSE, INPUT_PULLUP);
  lastPinState = digitalRead(PIN_COIN_PULSE);

  // Audio / Visual Indicators
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);

  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED_RED, LOW);
  digitalWrite(PIN_LED_GREEN, LOW);

  // Synchronize GSM Auto-Baud (SIM900A / SIM800L)
  delay(2500);
  autoSyncGSMBaud();

  Serial.println(F("=================================================="));
  Serial.println(F(" PCBCES Test 08: Hopper & GSM Low-Coin / Jam Test "));
  Serial.println(F(" Relay: D8 | Pulse: D7 | GSM: 5VT->D11, 5VR<-A3   "));
  Serial.println(F(" Logic: 5-Second No-Coin Cutoff -> Automated SMS  "));
  Serial.println(F("=================================================="));
  Serial.println(F("Commands:"));
  Serial.println(F(" '1' -> Test Dispense 3 Coins  (₱3.00 Payout)"));
  Serial.println(F(" '2' -> Test Dispense 20 Coins (₱20.00 Payout)"));
  Serial.println(F(" 'm' -> Manual Low-Coin SMS Test (Direct Trigger)"));
  Serial.println(F(" 's' -> Query GSM Signal Quality (AT+CSQ)"));
  Serial.println(F(" 'n' -> Query GSM Network Registration (AT+CREG?)"));
  Serial.println(F("=================================================="));
}

void loop() {
  // Handle active dispensing state
  if (isDispensing) {
    int curState = digitalRead(PIN_COIN_PULSE);
    unsigned long now = millis();

    // Falling edge pulse detection (Coin passing optical slot)
    // Masks first 100ms inrush noise and requires 35ms pulse separation
    if (curState != lastPinState) {
      if (curState == LOW && (now - payoutStartTime > 100) && (now - lastPulseTime > 35)) {
        coinsDispensed++;
        lastPulseTime = now;
        soundBeep(40);

        Serial.print(F("[COIN DETECTED] Count: "));
        Serial.print(coinsDispensed);
        Serial.print(F(" / "));
        Serial.print(currentTargetCoins);
        Serial.print(F(" (Elapsed: "));
        Serial.print(now - payoutStartTime);
        Serial.println(F(" ms)"));
      }
      lastPinState = curState;
    }

    // Condition A: Target reached successfully!
    if (coinsDispensed >= currentTargetCoins) {
      stopPayout();
    }
    // Condition B: 5 seconds with NO coin detected -> Low coin / Empty hopper timeout!
    else if (now - lastPulseTime >= COIN_TIMEOUT_MS) {
      timeoutPayout();
    }
  }

  // Serial Monitor Command Handler
  if (Serial.available()) {
    char c = Serial.read();
    if (!isDispensing) {
      if (c == '1') {
        startPayout(3);
      } else if (c == '2') {
        startPayout(20);
      } else if (c == 'm' || c == 'M') {
        sendLowCoinSMS(0, 3);
      } else if (c == 's' || c == 'S') {
        Serial.println(F("\n>> AT+CSQ (Signal Quality)"));
        gsm.println("AT+CSQ");
      } else if (c == 'n' || c == 'N') {
        Serial.println(F("\n>> AT+CREG? (Network Status)"));
        gsm.println("AT+CREG?");
      } else {
        gsm.write(c);
      }
    } else {
      if (c == 'x' || c == 'X' || c == 'q' || c == 'Q') {
        Serial.println(F("\n>>> Manual Abort Requested. Stopping Motor."));
        digitalWrite(PIN_RELAY_HOPPER, HIGH);
        digitalWrite(PIN_LED_GREEN, LOW);
        isDispensing = false;
      }
    }
  }

  // Forward GSM responses to Serial Monitor
  if (gsm.available()) {
    Serial.write(gsm.read());
  }
}
