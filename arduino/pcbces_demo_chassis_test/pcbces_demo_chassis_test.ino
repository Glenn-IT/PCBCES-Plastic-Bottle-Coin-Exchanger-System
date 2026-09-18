/*
 * =============================================================================
 * PCBCES - Combined Demo Chassis Test Controller (Tests 01, 03, 04, 05, 06, 07, 08)
 * Plastic Bottle Coin Exchanger System — Bench & Demo Rig Edition
 * Last Updated: 2026-09-18 22:28:00 (+08:00)
 * =============================================================================
 * 
 * Hardware Status for this Demo Rig:
 * - Test 01: [ACTIVE] 16x2 I2C LCD (0x27, A4/A5) + 3 Dedicated Buttons (D10, A0, A1) + Buzzer (D12) + LEDs (A2/D13)
 * - Test 03: [ACTIVE] Ultrasonic HC-SR04 (D2/D3) for bottle insertion & size detection
 * - Test 04: [BYPASSED] LJ12A3 Inductive Metal Sensor (Commented out — not wired yet)
 * - Test 05: [BYPASSED] MG996R Servo (Disabled — motor damaged; using manual bottle deposit/removal)
 * - Test 06: [ACTIVE] Coin Hopper Pulse (D7) + 5V Relay (D8) with 5-Second Motor Auto-Cutoff
 * - Test 07: [ACTIVE] SIM900A / SIM800L GSM SMS Module (5VT -> D11 RX, 5VR -> A3 TX)
 * - Test 08: [ACTIVE] Automated 5-Second Low-Coin / Jam SMS Alert to Admin Phones
 * 
 * Canonical Uno 20-Pin Connections:
 * - D2  : HC-SR04 Trigger Pulse
 * - D3  : HC-SR04 Echo Return Pulse
 * - D4  : (IR Entry Sensor - Optional / Bypassed for demo)
 * - D5  : (IR Bin-Full Sensor - Optional / Bypassed for demo)
 * - D6  : (LJ12A3 Inductive Metal - Commented out for demo)
 * - D7  : Coin Hopper Pulse Line (Falling edge detection, 1 pulse = 1 PHP)
 * - D8  : 5V Relay Module (Switches Hopper Motor Power, Active LOW)
 * - D9  : (MG996R Servo PWM - Disabled for demo due to damaged motor)
 * - D10 : Button Green (1.5L / 1.75L Mode -> 5 pcs quota = 20 PHP)
 * - D11 : SoftwareSerial RX (Connects to SIM900A 5VT / SIM800L TX)
 * - D12 : Active 5V Buzzer
 * - D13 : Red LED (Fault / Reject / Timeout Indicator)
 * - A0  : Button Blue (290 ML Mode -> 10 pcs quota = 3 PHP)
 * - A1  : Button Red (System Cancel / Restart)
 * - A2  : Green LED (Ready / Success Indicator)
 * - A3  : SoftwareSerial TX (Connects to SIM900A 5VR / SIM800L RX)
 * - A4  : I2C SDA (16x2 LCD)
 * - A5  : I2C SCL (16x2 LCD)
 * =============================================================================
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
// #include <Servo.h> // [TEST 05 DISABLED]: Servo motor damaged; manual bottle removal active

// --- PIN DEFINITIONS ---
const int PIN_ULTRASONIC_TRIG = 2;
const int PIN_ULTRASONIC_ECHO = 3;
// const int PIN_IR_ENTRY        = 4; // Commented out for ultrasonic-only demo
// const int PIN_IND_METAL       = 6; // [TEST 04 COMMENTED OUT]: Not wired yet
const int PIN_COIN_PULSE      = 7;
const int PIN_RELAY_HOPPER    = 8;
// const int PIN_SERVO_TRAPDOOR  = 9; // [TEST 05 DISABLED]: Motor damaged
const int PIN_BTN_GREEN       = 10;
const int PIN_GSM_RX          = 11; // SoftwareSerial RX (From SIM900A 5VT)
const int PIN_BUZZER          = 12;
const int PIN_LED_RED         = 13;
const int PIN_BTN_BLUE        = A0;
const int PIN_BTN_RED         = A1;
const int PIN_LED_GREEN       = A2;
const int PIN_GSM_TX          = A3; // SoftwareSerial TX (To SIM900A 5VR)

// --- HARDWARE INSTANCES ---
LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial gsm(PIN_GSM_RX, PIN_GSM_TX);
// Servo trapdoor; // [TEST 05 DISABLED]

// --- CONFIGURATION & QUOTAS ---
const int BOTTLE_1_5L_QUOTA    = 5;   // 5 bottles = 20.00 PHP
const int BOTTLE_290ML_QUOTA   = 10;  // 10 bottles = 3.00 PHP
const int COINS_PAYOUT_1_5L    = 20;  // 20 x 1-Peso coins
const int COINS_PAYOUT_290ML   = 3;   // 3 x 1-Peso coins
const unsigned long COIN_TIMEOUT_MS = 5000; // 5-Second dry-run motor auto-cutoff

// Target Administrator Phone Numbers
const char ADMIN_PHONE_1[] = "+639634299114";
const char ADMIN_PHONE_2[] = "+639242074903";

// --- DEMO CHAMBER HEIGHT CALIBRATION ---
int chamberTotalHeightCm = 31; // Calibrated baseline height
int dist15LMin  = 6;   // Cap is near ceiling sensor
int dist15LMax  = 16;  // Generous range for demo practice
int dist290Min  = 18;  // Cap is further from ceiling sensor
int dist290Max  = 28;

// --- STATE MACHINE ---
enum MachineState {
  STATE_STANDBY_MENU,
  STATE_WAIT_INSERTION,
  STATE_VALIDATE_BOTTLE,
  STATE_ACCEPT_DROP,
  STATE_REJECT_EJECT,
  STATE_PAYOUT_COINS
};

MachineState currentState = STATE_STANDBY_MENU;
enum BottleType { TYPE_1_5L, TYPE_290ML };
BottleType selectedType = TYPE_1_5L;

int currentDepositCount = 0;
int requiredQuota = BOTTLE_1_5L_QUOTA;
int requiredCoinsPayout = COINS_PAYOUT_1_5L;
volatile int coinsDispensed = 0;

// --- AUDIO HELPERS ---
void soundBeep(int ms = 80) {
  digitalWrite(PIN_BUZZER, HIGH);
  delay(ms);
  digitalWrite(PIN_BUZZER, LOW);
}

void soundError() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(PIN_BUZZER, HIGH);
    digitalWrite(PIN_LED_RED, HIGH);
    delay(100);
    digitalWrite(PIN_BUZZER, LOW);
    digitalWrite(PIN_LED_RED, LOW);
    delay(80);
  }
}

void soundSuccess() {
  soundBeep(100); delay(60); soundBeep(180);
}

// --- GSM COMMUNICATION & SMS DISPATCH ---
void printGSMResponse(unsigned long timeoutMs = 1200) {
  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    while (gsm.available()) {
      char c = gsm.read();
      if ((c >= 32 && c <= 126) || c == '\r' || c == '\n') {
        Serial.write(c);
      }
    }
  }
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
    delay(120);
    while (gsm.available()) gsm.read();

    bool detected = false;
    for (int attempt = 0; attempt < 3; attempt++) {
      gsm.print("AT\r\n");
      unsigned long start = millis();
      String resp = "";
      while (millis() - start < 400) {
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
      delay(120);
    }

    if (detected) {
      Serial.println(F("[LOCKED! Handshake OK]"));
      if (testBaud != 9600) {
        Serial.println(F("  -> Locking module to 9600 baud for stable SoftwareSerial..."));
        gsm.print("AT+IPR=9600\r\n");
        delay(200);
        gsm.print("AT&W\r\n");
        delay(200);
        gsm.begin(9600);
        delay(120);
        while (gsm.available()) gsm.read();
      }
      return true;
    } else {
      Serial.println(F("[No reply]"));
    }
  }

  Serial.println(F("[WARN] GSM auto-baud training at 9600 baud..."));
  gsm.begin(9600);
  delay(150);
  for (int i = 0; i < 4; i++) {
    gsm.print("AT\r\n");
    delay(250);
  }
  while (gsm.available()) gsm.read();
  return false;
}

void sendLowCoinToNumber(const char* recipient, int dispensed, int target) {
  Serial.print(F("[GSM] Sending Low-Coin SMS to: "));
  Serial.println(recipient);

  gsm.println("AT+CMGF=1");
  delay(350);

  gsm.print("AT+CMGS=\"");
  gsm.print(recipient);
  gsm.println("\"");
  delay(350);

  gsm.print("ALERT: PCBCES Coin Hopper is EMPTY or JAMMED! No coin dispensed for 5 seconds during payout. Dispensed: ");
  gsm.print(dispensed);
  gsm.print("/");
  gsm.print(target);
  gsm.print(" coins. Motor stopped. Please refill 1-peso coins.");
  delay(350);

  gsm.write(26); // Ctrl+Z
  Serial.println(F("[GSM] Waiting for SMS network delivery confirmation..."));

  unsigned long startWait = millis();
  while (millis() - startWait < 4500) {
    while (gsm.available()) {
      char c = gsm.read();
      if ((c >= 32 && c <= 126) || c == '\r' || c == '\n') {
        Serial.write(c);
      }
    }
  }
}

void sendLowCoinSMS(int dispensed, int target) {
  Serial.println(F("\n=================================================="));
  Serial.println(F("[GSM ALERT] Dispatching Low-Coin SMS to Admins... "));
  Serial.println(F("=================================================="));
  sendLowCoinToNumber(ADMIN_PHONE_1, dispensed, target);
  delay(1500);
  sendLowCoinToNumber(ADMIN_PHONE_2, dispensed, target);
  Serial.println(F("\n[GSM ALERT] Low-Coin SMS broadcast cycle completed!"));
}

// --- ULTRASONIC SENSOR FILTER ---
long singlePing() {
  digitalWrite(PIN_ULTRASONIC_TRIG, LOW);
  delayMicroseconds(4);
  digitalWrite(PIN_ULTRASONIC_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_ULTRASONIC_TRIG, LOW);

  long duration = pulseIn(PIN_ULTRASONIC_ECHO, HIGH, 7000); // 7ms timeout = ~120 cm
  if (duration <= 0) return chamberTotalHeightCm;
  long cm = duration * 0.034 / 2;
  if (cm < 2 || cm > 70) return chamberTotalHeightCm;
  return cm;
}

long readChamberDistance() {
  const int NUM_SAMPLES = 5;
  long samples[NUM_SAMPLES];

  for (int i = 0; i < NUM_SAMPLES; i++) {
    samples[i] = singlePing();
    delay(20);
  }

  // Insertion sort for median
  for (int i = 1; i < NUM_SAMPLES; i++) {
    long key = samples[i];
    int j = i - 1;
    while (j >= 0 && samples[j] > key) {
      samples[j + 1] = samples[j];
      j--;
    }
    samples[j + 1] = key;
  }
  return samples[NUM_SAMPLES / 2];
}

// --- DISPLAY HELPERS ---
void showMenuLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("GRN:1.5L BLU:290");
  lcd.setCursor(0, 1);
  lcd.print("RED:Cancel/Reset");
}

void showProgressLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  if (selectedType == TYPE_1_5L) {
    lcd.print("1.5L/1.75L(5pcs)");
  } else {
    lcd.print("290 ML  (10 pcs)");
  }
  lcd.setCursor(0, 1);
  lcd.print("Count: ");
  lcd.print(currentDepositCount);
  lcd.print("/");
  lcd.print(requiredQuota);
  lcd.print(" [RED:X]");
}

// --- RED BUTTON CANCEL CHECK ---
bool checkCancelButton() {
  if (digitalRead(PIN_BTN_RED) == LOW) {
    delay(50); // debounce
    if (digitalRead(PIN_BTN_RED) == LOW) {
      Serial.println(F("[SYSTEM] Red Button Pressed: Transaction Cancelled / Restarting..."));
      soundBeep(250);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" TRANSACTION    ");
      lcd.setCursor(0, 1);
      lcd.print(" CANCELLED / RST");
      delay(1000);
      currentDepositCount = 0;
      currentState = STATE_STANDBY_MENU;
      showMenuLCD();
      while (digitalRead(PIN_BTN_RED) == LOW);
      return true;
    }
  }
  return false;
}

// --- LIVE DIAGNOSTICS PRINT ---
void printDiagnostics() {
  long d = readChamberDistance();
  Serial.println(F("\n--- [LIVE SENSOR DIAGNOSTICS] ---"));
  Serial.print(F("Ceiling-to-Object Distance : ")); Serial.print(d); Serial.println(F(" cm"));
  // Serial.print(F("Inductive Metal (D6)       : ")); Serial.println(F("BYPASSED (Not installed)"));
  // Serial.print(F("Servo Flap (D9)            : ")); Serial.println(F("BYPASSED (Manual removal active)"));
  Serial.print(F("Coin Pulse Line (D7)       : ")); Serial.println(digitalRead(PIN_COIN_PULSE) == HIGH ? F("HIGH (Idle)") : F("LOW (Coin Present)"));
  Serial.print(F("Hopper Relay (D8)          : ")); Serial.println(digitalRead(PIN_RELAY_HOPPER) == LOW ? F("ON (Dispensing)") : F("OFF (Standby)"));
  Serial.println(F("--------------------------------"));
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);

  // Dedicated 3-Buttons (INPUT_PULLUP)
  pinMode(PIN_BTN_GREEN, INPUT_PULLUP);
  pinMode(PIN_BTN_BLUE, INPUT_PULLUP);
  pinMode(PIN_BTN_RED, INPUT_PULLUP);

  // Sensors
  // pinMode(PIN_IND_METAL, INPUT); // [TEST 04 COMMENTED OUT]: Not wired yet
  pinMode(PIN_COIN_PULSE, INPUT_PULLUP);
  pinMode(PIN_ULTRASONIC_TRIG, OUTPUT);
  pinMode(PIN_ULTRASONIC_ECHO, INPUT);

  // Relay (Active LOW -> Start with relay OFF/HIGH)
  pinMode(PIN_RELAY_HOPPER, OUTPUT);
  digitalWrite(PIN_RELAY_HOPPER, HIGH);

  // Audio / Visual Indicators
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);

  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED_RED, LOW);
  digitalWrite(PIN_LED_GREEN, HIGH);

  // [TEST 05 DISABLED]: Servo motor damaged — manual drop/removal active
  // trapdoor.attach(PIN_SERVO_TRAPDOOR);

  // LCD Setup
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(" PCBCES DEMO RIG");
  lcd.setCursor(0, 1);
  lcd.print("Bench Controller");
  soundBeep(120);

  // Synchronize GSM Auto-Baud & Handshake (SIM900A / SIM800L)
  delay(2000);
  autoSyncGSMBaud();

  // Calibrate empty chamber baseline
  chamberTotalHeightCm = readChamberDistance();

  Serial.println(F("=================================================="));
  Serial.println(F(" PCBCES DEMO CHASSIS CONTROLLER (GSM SMS ENABLED) "));
  Serial.println(F(" Tests 01, 03, 06, 07, 08 Active                  "));
  Serial.println(F(" Metal (Test 04): Bypassed | Servo (Test 05): Manual"));
  Serial.println(F("=================================================="));
  Serial.print(F("Ultrasonic Baseline: "));
  Serial.print(chamberTotalHeightCm);
  Serial.println(F(" cm"));
  Serial.println(F("Interactive Serial Shortcuts:"));
  Serial.println(F(" 'g' -> Select Green (1.5L Mode -> 5 pcs = 20 PHP)"));
  Serial.println(F(" 'b' -> Select Blue  (290 ML Mode -> 10 pcs = 3 PHP)"));
  Serial.println(F(" 'r' -> Cancel / Restart Transaction"));
  Serial.println(F(" '1' -> Test Hopper Dispense 3 Coins (₱3.00)"));
  Serial.println(F(" '2' -> Test Hopper Dispense 20 Coins (₱20.00)"));
  Serial.println(F(" 'm' -> Test Low-Coin / Jam SMS Dispatch"));
  Serial.println(F(" 's' -> Query GSM Signal (AT+CSQ)"));
  Serial.println(F(" 'd' -> Print Live Sensor Diagnostics"));
  Serial.println(F("--------------------------------------------------"));

  showMenuLCD();
}

// --- MAIN LOOP ---
void loop() {
  // Serial Commands Handler
  if (Serial.available()) {
    char ch = Serial.read();
    if (ch == 'g' || ch == 'G') {
      selectedType = TYPE_1_5L;
      requiredQuota = BOTTLE_1_5L_QUOTA;
      requiredCoinsPayout = COINS_PAYOUT_1_5L;
      currentDepositCount = 0;
      soundBeep(100);
      currentState = STATE_WAIT_INSERTION;
      showProgressLCD();
      Serial.println(F("[SIM] 1.5L / 1.75L Mode selected via Serial."));
    } else if (ch == 'b' || ch == 'B') {
      selectedType = TYPE_290ML;
      requiredQuota = BOTTLE_290ML_QUOTA;
      requiredCoinsPayout = COINS_PAYOUT_290ML;
      currentDepositCount = 0;
      soundBeep(100);
      currentState = STATE_WAIT_INSERTION;
      showProgressLCD();
      Serial.println(F("[SIM] 290 ML Mode selected via Serial."));
    } else if (ch == 'r' || ch == 'R') {
      soundBeep(250);
      currentDepositCount = 0;
      currentState = STATE_STANDBY_MENU;
      showMenuLCD();
      Serial.println(F("[SIM] Cancel / Reset via Serial."));
    } else if (ch == '1') {
      requiredCoinsPayout = 3;
      currentState = STATE_PAYOUT_COINS;
    } else if (ch == '2') {
      requiredCoinsPayout = 20;
      currentState = STATE_PAYOUT_COINS;
    } else if (ch == 'm' || ch == 'M') {
      sendLowCoinSMS(0, 3);
    } else if (ch == 's' || ch == 'S') {
      Serial.println(F("\n>> AT+CSQ (Signal Quality)"));
      gsm.println("AT+CSQ");
      printGSMResponse(1000);
    } else if (ch == 'd' || ch == 'D') {
      printDiagnostics();
    }
  }

  // --- STATE MACHINE ---
  switch (currentState) {
    case STATE_STANDBY_MENU: {
      digitalWrite(PIN_LED_GREEN, HIGH);
      digitalWrite(PIN_LED_RED, LOW);

      // 1. GREEN BUTTON -> 1.5L Mode
      if (digitalRead(PIN_BTN_GREEN) == LOW) {
        delay(50);
        if (digitalRead(PIN_BTN_GREEN) == LOW) {
          selectedType = TYPE_1_5L;
          requiredQuota = BOTTLE_1_5L_QUOTA;
          requiredCoinsPayout = COINS_PAYOUT_1_5L;
          currentDepositCount = 0;
          soundBeep(100);
          Serial.println(F("[MENU] GREEN Pressed: 1.5L / 1.75L Mode Selected (5 pcs = 20 PHP)"));

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("MODE: 1.5L/1.75L");
          lcd.setCursor(0, 1);
          lcd.print("Target: 5 (20P) ");
          delay(900);

          currentState = STATE_WAIT_INSERTION;
          showProgressLCD();
          while (digitalRead(PIN_BTN_GREEN) == LOW);
          break;
        }
      }

      // 2. BLUE BUTTON -> 290 ML Mode
      if (digitalRead(PIN_BTN_BLUE) == LOW) {
        delay(50);
        if (digitalRead(PIN_BTN_BLUE) == LOW) {
          selectedType = TYPE_290ML;
          requiredQuota = BOTTLE_290ML_QUOTA;
          requiredCoinsPayout = COINS_PAYOUT_290ML;
          currentDepositCount = 0;
          soundBeep(100);
          Serial.println(F("[MENU] BLUE Pressed: 290 ML Mode Selected (10 pcs = 3 PHP)"));

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("MODE: 290 ML PET");
          lcd.setCursor(0, 1);
          lcd.print("Target: 10 (3P) ");
          delay(900);

          currentState = STATE_WAIT_INSERTION;
          showProgressLCD();
          while (digitalRead(PIN_BTN_BLUE) == LOW);
          break;
        }
      }

      // 3. RED BUTTON -> Refresh Standby
      if (digitalRead(PIN_BTN_RED) == LOW) {
        delay(50);
        if (digitalRead(PIN_BTN_RED) == LOW) {
          soundBeep(150);
          currentDepositCount = 0;
          showMenuLCD();
          while (digitalRead(PIN_BTN_RED) == LOW);
          break;
        }
      }
      break;
    }

    case STATE_WAIT_INSERTION: {
      if (checkCancelButton()) break;

      // Check Ultrasonic Sensor (Bottle insertion detected when distance < baseline)
      long currentDist = singlePing();

      // If object detected closer than chamber baseline
      if (currentDist >= 2 && currentDist <= 35 && (currentDist < (chamberTotalHeightCm - 2) || chamberTotalHeightCm <= 15)) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("BOTTLE DETECTED ");
        lcd.setCursor(0, 1);
        lcd.print("Dist: ");
        lcd.print(currentDist);
        lcd.print(" cm     ");
        soundBeep(70);
        delay(500);

        currentState = STATE_VALIDATE_BOTTLE;
      }
      delay(80);
      break;
    }

    case STATE_VALIDATE_BOTTLE: {
      Serial.println(F("[VALIDATION] Checking bottle dimensions via Ultrasonic..."));

      // 1. Metal Detection (LJ12A3 Sensor) - [COMMENTED OUT FOR DEMO PER REQUEST]
      /*
      bool isMetal = (digitalRead(PIN_IND_METAL) == LOW);
      if (isMetal) {
        Serial.println(F("[VALIDATION] REJECT: Metallic object detected!"));
        currentState = STATE_REJECT_EJECT;
        break;
      }
      */
      Serial.println(F("[VALIDATION] Metal detector bypassed (Test 04 not wired yet)."));

      // 2. Ultrasonic Vertical Distance Verification
      long distToCap = readChamberDistance();
      Serial.print(F("[VALIDATION] Ceiling-to-Cap Distance: "));
      Serial.print(distToCap);
      Serial.println(F(" cm"));

      bool validDimensions = false;
      if (selectedType == TYPE_1_5L && distToCap >= dist15LMin && distToCap <= dist15LMax) {
        validDimensions = true;
      } else if (selectedType == TYPE_290ML && distToCap >= dist290Min && distToCap <= dist290Max) {
        validDimensions = true;
      } else if (distToCap >= 2 && distToCap <= 35) {
        // Generous tolerance for demo practice
        validDimensions = true;
      }

      if (!validDimensions) {
        Serial.print(F("[VALIDATION] REJECT: Bottle size mismatch! Read: "));
        Serial.print(distToCap);
        Serial.println(F(" cm"));
        currentState = STATE_REJECT_EJECT;
        break;
      }

      Serial.println(F("[VALIDATION] PASSED: Bottle verified."));
      currentState = STATE_ACCEPT_DROP;
      break;
    }

    case STATE_ACCEPT_DROP: {
      // [TEST 05 BYPASSED]: Motor is damaged. Prompt user to manually push/drop bottle into bin!
      Serial.println(F("[MANUAL ACCEPT] Bottle accepted! (Servo disabled: please manually drop bottle into bin)."));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("BOTTLE ACCEPTED!");
      lcd.setCursor(0, 1);
      lcd.print("Drop into Bin ->");
      digitalWrite(PIN_LED_GREEN, HIGH);
      soundSuccess();

      delay(1500); // Buffer for user to manually push/drop bottle into collection bin

      currentDepositCount++;
      Serial.print(F("[PROGRESS] Deposit Count: "));
      Serial.print(currentDepositCount);
      Serial.print(F(" / "));
      Serial.println(requiredQuota);

      if (currentDepositCount >= requiredQuota) {
        currentState = STATE_PAYOUT_COINS;
      } else {
        showProgressLCD();
        currentState = STATE_WAIT_INSERTION;
      }
      break;
    }

    case STATE_REJECT_EJECT: {
      // Rejection: manual removal from cradle
      Serial.println(F("[MANUAL REJECT] Bottle rejected. Please remove item manually from cradle."));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("BOTTLE REJECTED!");
      lcd.setCursor(0, 1);
      lcd.print("Pls Remove Item ");
      soundError();

      for (int i = 0; i < 4; i++) {
        digitalWrite(PIN_LED_RED, HIGH);
        delay(150);
        digitalWrite(PIN_LED_RED, LOW);
        delay(150);
      }
      delay(300);

      showProgressLCD();
      currentState = STATE_WAIT_INSERTION;
      break;
    }

    case STATE_PAYOUT_COINS: {
      Serial.print(F("[PAYOUT] Quota reached! Dispensing "));
      Serial.print(requiredCoinsPayout);
      Serial.println(F(".00 PHP..."));

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("QUOTA REACHED!  ");
      lcd.setCursor(0, 1);
      if (requiredCoinsPayout >= 10) {
        lcd.print("Dispensing 20PHP");
      } else {
        lcd.print("Dispensing 3 PHP");
      }

      coinsDispensed = 0;
      digitalWrite(PIN_RELAY_HOPPER, LOW); // Turn Relay ON (Active LOW triggers Hopper motor)
      delay(50); // Inrush transient settle

      unsigned long payoutStart = millis();
      unsigned long lastPulseTime = millis();
      int lastEdgeState = digitalRead(PIN_COIN_PULSE);
      bool timedOut = false;

      while (coinsDispensed < requiredCoinsPayout) {
        int curState = digitalRead(PIN_COIN_PULSE);

        // Detect Falling edge (HIGH -> LOW on coin optical slot detect)
        if (lastEdgeState == HIGH && curState == LOW) {
          unsigned long now = millis();
          if ((now - payoutStart > 100) && (now - lastPulseTime > 35)) {
            coinsDispensed++;
            lastPulseTime = now;
            soundBeep(40);
            Serial.print(F("--> [COIN DETECTED!] Count = "));
            Serial.print(coinsDispensed);
            Serial.print(F(" / "));
            Serial.println(requiredCoinsPayout);
          }
        }
        lastEdgeState = curState;

        // 5-Second Auto-Cutoff Timeout Check (Low-Coin / Jam Protection)
        if (millis() - lastPulseTime >= COIN_TIMEOUT_MS) {
          timedOut = true;
          Serial.println();
          Serial.println(F("=================================================="));
          Serial.println(F(">>> [TIMEOUT ALERT] NO COINS DETECTED FOR 5 SECONDS!"));
          Serial.println(F(">>> Hopper is EMPTY or JAMMED. Motor auto-stopped!"));
          Serial.println(F(">>> Triggering automated Low-Coin SMS to Admins..."));
          Serial.println(F("=================================================="));
          break;
        }
        delay(2);
      }

      // Cut power to hopper motor
      digitalWrite(PIN_RELAY_HOPPER, HIGH);

      if (timedOut || coinsDispensed < requiredCoinsPayout) {
        soundError();
        digitalWrite(PIN_LED_RED, HIGH);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("EMPTY HOPPER!   ");
        lcd.setCursor(0, 1);
        lcd.print("REFILL 1P COINS ");

        // Dispatch emergency SMS alert to both administrator phones via GSM module
        sendLowCoinSMS(coinsDispensed, requiredCoinsPayout);

        delay(3000);
        digitalWrite(PIN_LED_RED, LOW);
      } else {
        soundSuccess();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("PAYOUT COMPLETE!");
        lcd.setCursor(0, 1);
        lcd.print("Thank You! :)   ");
        Serial.println(F("[PAYOUT] Success! All coins dispensed."));
        delay(2500);
      }

      currentDepositCount = 0;
      currentState = STATE_STANDBY_MENU;
      showMenuLCD();
      break;
    }
  }
}
