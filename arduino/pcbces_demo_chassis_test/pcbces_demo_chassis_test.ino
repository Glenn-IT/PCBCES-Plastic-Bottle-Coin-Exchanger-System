/*
 * =============================================================================
 * PCBCES - Combined Demo Chassis Test Controller (Tests 01, 03, 05, 06, 07, 08)
 * Plastic Bottle Coin Exchanger System — Bench & Demo Rig Edition
 * Last Updated: 2026-09-18 23:20:00 (+08:00)
 * =============================================================================
 * 
 * Integrated Modules:
 * - Test 01: 16x2 I2C LCD (0x27, A4/A5) + 3 Dedicated Buttons (D10, A0, A1) + Buzzer (D12) + LEDs (A2/D13)
 * - Test 03: Ultrasonic HC-SR04 (D2/D3) + IR Entry Sensor (D4) [Calibrated to 32 cm Chamber]
 * - Test 04: LJ12A3 Inductive Metal Proximity Sensor (D6) -> [COMMENTED OUT / NOT WIRED IN DEMO RIG]
 * - Test 05: MG996R Metal Gear Sorting Servo (D9, 0° Standby/Reject, 90° Accept)
 * - Test 06: Coin Hopper Optical Pulse (D7) + 5V Relay (D8) with 5-Second Motor Auto-Cutoff
 * - Test 07: IR Bin-Full Sensor (D5) -> 10-Second Continuous Trigger Safety -> Automated SMS to Admins
 * - Test 08: SIM900A / SIM800L GSM Module (D11/A3) -> Automated Low-Coin & Bin-Full SMS Alerts
 * 
 * Admin Phone Numbers (Synchronized with Test 07, Test 08 & config.h):
 * - Primary:   +639634299114
 * - Secondary: +639242074903
 * 
 * Pin Connections (Canonical Uno 20-Pin Lock):
 * - D2  : HC-SR04 Trigger Pulse
 * - D3  : HC-SR04 Echo Return Pulse
 * - D4  : IR Bottle Entry Beam Sensor (Active LOW)
 * - D5  : IR Bin-Full Storage Sensor (Active LOW: 10-Second debounced SMS trigger)
 * - D6  : LJ12A3 Inductive Metal Sensor [Commented Out / Unconnected]
 * - D7  : Coin Hopper Pulse Line (Falling edge detection via divider)
 * - D8  : 5V Single-Channel Relay (Hopper Motor Power, Active LOW)
 * - D9  : MG996R Servo PWM (0° Standby / 90° Drop)
 * - D10 : Button Green (1.5L / 1.75L Mode -> 5 pcs quota = 20 PHP)
 * - D11 : SoftwareSerial RX (from GSM TX: SIM900A 5VT / SIM800L TX)
 * - D12 : Active 5V Buzzer
 * - D13 : Red LED (Fault / Reject Indicator)
 * - A0  : Button Blue (290 ML Mode -> 10 pcs quota = 3 PHP)
 * - A1  : Button Red (System Cancel / Restart)
 * - A2  : Green LED (Ready / Success Indicator)
 * - A3  : SoftwareSerial TX (to GSM RX: SIM900A 5VR / SIM800L RX)
 * - A4  : I2C SDA (16x2 LCD)
 * - A5  : I2C SCL (16x2 LCD)
 * =============================================================================
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <SoftwareSerial.h>

// --- PIN DEFINITIONS ---
const int PIN_ULTRASONIC_TRIG = 2;
const int PIN_ULTRASONIC_ECHO = 3;
const int PIN_IR_ENTRY        = 4;
const int PIN_IR_BIN_FULL     = 5;   // IR Obstacle Avoidance Sensor: Bin-Full detection (Active LOW)
// const int PIN_IND_METAL    = 6;   // Inductive Metal Sensor (Commented out: hardware not connected)
const int PIN_COIN_PULSE      = 7;
const int PIN_RELAY_HOPPER    = 8;
const int PIN_SERVO_TRAPDOOR  = 9;
const int PIN_BTN_GREEN       = 10;
const int PIN_GSM_RX          = 11;  // D11 connects to SIM900A 5VT / SIM800L TX
const int PIN_BUZZER          = 12;
const int PIN_LED_RED         = 13;
const int PIN_BTN_BLUE        = A0;
const int PIN_BTN_RED         = A1;
const int PIN_LED_GREEN       = A2;
const int PIN_GSM_TX          = A3;  // A3 connects to SIM900A 5VR / SIM800L RX

// --- HARDWARE INSTANCES ---
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo trapdoor;
SoftwareSerial gsm(PIN_GSM_RX, PIN_GSM_TX); // RX on D11, TX on A3

// Target Administrator Phone Numbers (Philippines format: +639XXXXXXXXX or 09XXXXXXXXX)
const char ADMIN_PHONE_1[] = "+639634299114";
const char ADMIN_PHONE_2[] = "+639242074903";

// --- CONFIGURATION & QUOTAS ---
const int BOTTLE_1_5L_QUOTA    = 5;   // 5 bottles = 20.00 PHP
const int BOTTLE_290ML_QUOTA   = 10;  // 10 bottles = 3.00 PHP
const int COINS_PAYOUT_1_5L    = 20;  // 20 x 1-Peso coins
const int COINS_PAYOUT_290ML   = 3;   // 3 x 1-Peso coins
const int SERVO_STANDBY_ANGLE  = 0;   // Closed / Cradle Rest / Rejection Hold
const int SERVO_ACCEPT_ANGLE   = 90;  // Open / Drop into collection bin
const unsigned long COIN_TIMEOUT_MS = 5000; // 5-Second dry-run motor auto-cutoff

// --- IR SENSOR BIN FULL TRIGGER THRESHOLD ---
const unsigned long BIN_FULL_HOLD_TIME_MS = 10000; // IR sensor must stay triggered for 10 continuous seconds
unsigned long binBlockedStartTime = 0;

// --- DEMO CHAMBER HEIGHT CALIBRATION ---
int chamberTotalHeightCm = 32;
int dist15LMin  = 11;   // Cap is 11 to 12 cm from ceiling sensor
int dist15LMax  = 12;  // Generous range for demo chassis tolerance
int dist290Min  = 23;  // Cap is 23 to 24 cm from ceiling sensor
int dist290Max  = 24;

// --- STATE MACHINE ---
enum MachineState {
  STATE_STANDBY_MENU,
  STATE_WAIT_INSERTION,
  STATE_VALIDATE_BOTTLE,
  STATE_ACCEPT_DROP,
  STATE_REJECT_EJECT,
  STATE_PAYOUT_COINS,
  STATE_BIN_FULL_LOCKED
};

MachineState currentState = STATE_STANDBY_MENU;
enum BottleType { TYPE_1_5L, TYPE_290ML };
BottleType selectedType = TYPE_1_5L;

int currentDepositCount = 0;
int requiredQuota = BOTTLE_1_5L_QUOTA;
int requiredCoinsPayout = COINS_PAYOUT_1_5L;
volatile int coinsDispensed = 0;

// Coin Pulse Edge Detection
int lastCoinPinState = HIGH;

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

void soundAlarm() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(PIN_BUZZER, HIGH);
    digitalWrite(PIN_LED_RED, HIGH);
    delay(150);
    digitalWrite(PIN_BUZZER, LOW);
    digitalWrite(PIN_LED_RED, LOW);
    delay(100);
  }
}

void soundSuccess() {
  soundBeep(100); delay(60); soundBeep(180);
}

// --- GSM SMS DISPATCH FUNCTIONS ---
void sendSMSToRecipient(const char* recipient, const char* message) {
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
  gsm.print(message);
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

void sendBinFullSMS() {
  Serial.println(F("\n=================================================="));
  Serial.println(F("[GSM ALERT] Storage Bin FULL for 10s! Dispatching SMS..."));
  Serial.println(F("=================================================="));
  const char* msg = "ALERT: PCBCES Storage Bin is FULL! IR Bin Sensor blocked for 10 seconds. Machine is locked. Please empty bin.";
  sendSMSToRecipient(ADMIN_PHONE_1, msg);
  delay(2000);
  sendSMSToRecipient(ADMIN_PHONE_2, msg);
  Serial.println(F("\n[GSM ALERT] Bin-Full SMS transmission complete for all admins!"));
}

void sendLowCoinSMS(int dispensed, int target) {
  Serial.println(F("\n=================================================="));
  Serial.println(F("[GSM ALERT] Dispatching Low-Coin SMS to Admins... "));
  Serial.println(F("=================================================="));
  char msg[140];
  snprintf(msg, sizeof(msg), "ALERT: PCBCES Coin Hopper is EMPTY or JAMMED! No coin dispensed for 5 seconds during payout. Dispensed: %d/%d coins. Motor stopped. Please refill 1-peso coins.", dispensed, target);
  sendSMSToRecipient(ADMIN_PHONE_1, msg);
  delay(2000);
  sendSMSToRecipient(ADMIN_PHONE_2, msg);
  Serial.println(F("\n[GSM ALERT] Low-Coin SMS transmission complete for all admins!"));
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
    delay(25);
  }

  // Median filter sort
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
      delay(1200);
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
  Serial.println(F("--- [LIVE SENSOR DIAGNOSTICS] ---"));
  Serial.print(F("Ceiling-to-Object Distance : ")); Serial.print(d); Serial.println(F(" cm"));
  Serial.print(F("IR Entry Sensor (D4)       : ")); Serial.println(digitalRead(PIN_IR_ENTRY) == LOW ? F("BLOCKED (Bottle Present)") : F("CLEAR"));
  Serial.print(F("IR Bin-Full Sensor (D5)    : ")); Serial.println(digitalRead(PIN_IR_BIN_FULL) == LOW ? F("BLOCKED (Bin Full)") : F("CLEAR (OK)"));
  Serial.println(F("Inductive Metal (D6)       : [COMMENTED OUT / NOT CONNECTED]"));
  Serial.print(F("Coin Pulse Line (D7)       : ")); Serial.println(digitalRead(PIN_COIN_PULSE) == HIGH ? F("HIGH (Idle)") : F("LOW (Coin Present)"));
  Serial.print(F("Hopper Relay (D8)          : ")); Serial.println(digitalRead(PIN_RELAY_HOPPER) == LOW ? F("ON (Dispensing)") : F("OFF (Standby)"));
  Serial.println(F("--------------------------------"));
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);

  // Buttons (INPUT_PULLUP)
  pinMode(PIN_BTN_GREEN, INPUT_PULLUP);
  pinMode(PIN_BTN_BLUE, INPUT_PULLUP);
  pinMode(PIN_BTN_RED, INPUT_PULLUP);

  // Sensors
  pinMode(PIN_IR_ENTRY, INPUT);
  pinMode(PIN_IR_BIN_FULL, INPUT); // IR Obstacle Avoidance Sensor: Storage Bin Full (Active LOW)
  // pinMode(PIN_IND_METAL, INPUT); // Commented out: Metal sensor not installed on demo chassis
  pinMode(PIN_COIN_PULSE, INPUT_PULLUP);
  pinMode(PIN_ULTRASONIC_TRIG, OUTPUT);
  pinMode(PIN_ULTRASONIC_ECHO, INPUT);

  // Relay (Active LOW -> Start with relay OFF/HIGH)
  pinMode(PIN_RELAY_HOPPER, OUTPUT);
  digitalWrite(PIN_RELAY_HOPPER, HIGH);

  // Indicators
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);

  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED_RED, LOW);
  digitalWrite(PIN_LED_GREEN, HIGH);

  // Servo Setup
  trapdoor.attach(PIN_SERVO_TRAPDOOR);
  trapdoor.write(SERVO_STANDBY_ANGLE);

  // LCD Setup
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(" PCBCES DEMO RIG");
  lcd.setCursor(0, 1);
  lcd.print("Bench Controller");
  soundBeep(120);
  delay(1200);

  // Calibrate empty chamber baseline
  long baseline = readChamberDistance();
  Serial.println(F("=================================================="));
  Serial.println(F(" PCBCES DEMO CHASSIS INTEGRATED CONTROLLER        "));
  Serial.println(F(" Tests 01 + 03 + 05 + 06 + 07 + 08 Active         "));
  Serial.println(F(" (Metal Sensor D6 Commented Out / Not Wired)      "));
  Serial.println(F("=================================================="));
  Serial.print(F("Empty Chamber Ultrasonic Baseline: "));
  Serial.print(baseline);
  Serial.println(F(" cm"));
  Serial.println(F("Interactive Serial Commands:"));
  Serial.println(F(" 'g' -> Select Green (1.5L Mode)"));
  Serial.println(F(" 'b' -> Select Blue  (290 ML Mode)"));
  Serial.println(F(" 'r' -> Select Red   (Cancel/Restart)"));
  Serial.println(F(" 'd' -> Print Live Sensor Diagnostics"));
  Serial.println(F(" 'f' -> Manual Bin-Full SMS Test to Admins"));
  Serial.println(F(" 'm' -> Manual Low-Coin SMS Test to Admins"));
  Serial.println(F(" 's' -> Query GSM Signal Quality (AT+CSQ)"));
  Serial.println(F(" 'n' -> Query GSM Network Status (AT+CREG?)"));
  Serial.println(F("--------------------------------------------------"));

  // Synchronize GSM Module
  autoSyncGSMBaud();

  showMenuLCD();
}

// --- MAIN LOOP ---
void loop() {
  // --- IR BIN-FULL SENSOR CONTINUOUS MONITORING (PIN D5) ---
  // If the IR sensor is triggered (Active LOW) and STAYS triggered continuously for 10 SECONDS (10000ms),
  // lock the system and dispatch automated emergency SMS to admins.
  if (digitalRead(PIN_IR_BIN_FULL) == LOW) {
    if (binBlockedStartTime == 0) {
      binBlockedStartTime = millis();
      Serial.println(F("[BIN SENSOR] IR Bin-Full Sensor (D5) triggered. Starting 10-second timer..."));
    } else if (millis() - binBlockedStartTime >= BIN_FULL_HOLD_TIME_MS) {
      if (currentState != STATE_BIN_FULL_LOCKED) {
        Serial.println(F("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"));
        Serial.println(F(">>> [BIN FULL ALERT] IR Sensor BLOCKED for 10 SECONDS!"));
        Serial.println(F(">>> Storage bin is FULL. Locking system and notifying admins."));
        Serial.println(F("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"));
        currentState = STATE_BIN_FULL_LOCKED;
      }
    }
  } else {
    if (binBlockedStartTime != 0 && currentState != STATE_BIN_FULL_LOCKED) {
      Serial.println(F("[BIN SENSOR] Object cleared before 10s elapsed. Timer reset."));
      binBlockedStartTime = 0;
    }
  }

  // Handle Serial Key Shortcuts for testing without pressing physical buttons
  if (Serial.available()) {
    char ch = Serial.read();
    if (ch == 'g' || ch == 'G') {
      // Simulate Green Button
      selectedType = TYPE_1_5L;
      requiredQuota = BOTTLE_1_5L_QUOTA;
      requiredCoinsPayout = COINS_PAYOUT_1_5L;
      currentDepositCount = 0;
      soundBeep(100);
      currentState = STATE_WAIT_INSERTION;
      showProgressLCD();
      Serial.println(F("[SIM] 1.5L / 1.75L Mode selected via Serial."));
    } else if (ch == 'b' || ch == 'B') {
      // Simulate Blue Button
      selectedType = TYPE_290ML;
      requiredQuota = BOTTLE_290ML_QUOTA;
      requiredCoinsPayout = COINS_PAYOUT_290ML;
      currentDepositCount = 0;
      soundBeep(100);
      currentState = STATE_WAIT_INSERTION;
      showProgressLCD();
      Serial.println(F("[SIM] 290 ML Mode selected via Serial."));
    } else if (ch == 'r' || ch == 'R') {
      // Simulate Red Button
      soundBeep(250);
      currentDepositCount = 0;
      currentState = STATE_STANDBY_MENU;
      showMenuLCD();
      Serial.println(F("[SIM] Cancel / Reset via Serial."));
    } else if (ch == 'd' || ch == 'D') {
      printDiagnostics();
    } else if (ch == 'f' || ch == 'F') {
      Serial.println(F("[SIM] Triggering Manual Bin-Full SMS Test..."));
      sendBinFullSMS();
    } else if (ch == 'm' || ch == 'M') {
      Serial.println(F("[SIM] Triggering Manual Low-Coin SMS Test..."));
      sendLowCoinSMS(0, requiredCoinsPayout);
    } else if (ch == 's' || ch == 'S') {
      Serial.println(F("\n>> AT+CSQ (Signal Quality)"));
      gsm.println("AT+CSQ");
    } else if (ch == 'n' || ch == 'N') {
      Serial.println(F("\n>> AT+CREG? (Network Status)"));
      gsm.println("AT+CREG?");
    }
  }

  // --- STATE MACHINE EXECUTION ---
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
          delay(1000);

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
          delay(1000);

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

      // Check IR Entry Sensor (Active LOW when bottle blocks beam)
      if (digitalRead(PIN_IR_ENTRY) == LOW) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("BOTTLE DETECTED ");
        lcd.setCursor(0, 1);
        lcd.print("Align Bottle 2s ");
        soundBeep(70);
        delay(1000);

        lcd.setCursor(0, 1);
        lcd.print("Scanning in 1s..");
        delay(1000);

        lcd.setCursor(0, 1);
        lcd.print("Scanning Sensors");
        currentState = STATE_VALIDATE_BOTTLE;
      }
      break;
    }

    case STATE_VALIDATE_BOTTLE: {
      Serial.println(F("[VALIDATION] Commencing multi-sensor verification..."));

      // 1. Metal Detection (LJ12A3 Sensor - Active LOW)
      // [COMMENTED OUT FOR DEMO CHASSIS - Sensor not wired/installed]
      /*
      bool isMetal = (digitalRead(PIN_IND_METAL) == LOW);
      if (isMetal) {
        Serial.println(F("[VALIDATION] REJECT: Metallic object detected!"));
        currentState = STATE_REJECT_EJECT;
        break;
      }
      */
      Serial.println(F("[VALIDATION] Metal check bypassed (LJ12A3 not connected on demo rig)."));

      // 2. Ultrasonic Height Verification (Distance from ceiling down to bottle cap)
      long distToCap = readChamberDistance();
      Serial.print(F("[VALIDATION] Ceiling-to-Cap Distance: "));
      Serial.print(distToCap);
      Serial.println(F(" cm"));

      bool validDimensions = false;
      if (selectedType == TYPE_1_5L && distToCap >= dist15LMin && distToCap <= dist15LMax) {
        validDimensions = true;
      } else if (selectedType == TYPE_290ML && distToCap >= dist290Min && distToCap <= dist290Max) {
        validDimensions = true;
      }

      if (!validDimensions) {
        Serial.print(F("[VALIDATION] REJECT: Bottle size mismatch! Read: "));
        Serial.print(distToCap);
        Serial.println(F(" cm"));
        currentState = STATE_REJECT_EJECT;
        break;
      }

      // PASSED SENSORS!
      Serial.println(F("[VALIDATION] PASSED: Valid plastic bottle verified."));
      currentState = STATE_ACCEPT_DROP;
      break;
    }

    case STATE_ACCEPT_DROP: {
      Serial.println(F("[TRAPDOOR] Opening flap (90 deg) to accept bottle..."));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("BOTTLE ACCEPTED!");
      digitalWrite(PIN_LED_GREEN, HIGH);
      soundSuccess();

      // Open servo trapdoor to 90 degrees
      trapdoor.write(SERVO_ACCEPT_ANGLE);
      delay(1500); // Allow bottle to drop into collection bin
      trapdoor.write(SERVO_STANDBY_ANGLE);
      delay(500);

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
      Serial.println(F("[TRAPDOOR] Rejection triggered: Flap holds at 0 deg (Item remains on cradle for manual removal)."));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("BOTTLE REJECTED!");
      lcd.setCursor(0, 1);
      lcd.print("Pls Remove Item ");
      soundError();

      // Flap holds at 0 degrees
      trapdoor.write(SERVO_STANDBY_ANGLE);

      // Flash Red LED until user retrieves the bottle
      unsigned long rejectStart = millis();
      while (digitalRead(PIN_IR_ENTRY) == LOW && (millis() - rejectStart < 6000)) {
        digitalWrite(PIN_LED_RED, HIGH);
        delay(200);
        digitalWrite(PIN_LED_RED, LOW);
        delay(200);
      }
      digitalWrite(PIN_LED_RED, LOW);
      delay(400);

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
      digitalWrite(PIN_RELAY_HOPPER, LOW); // Relay ON (Active LOW)
      delay(50); // Inrush transient settle

      unsigned long payoutStart = millis();
      unsigned long lastPulseTime = millis();
      int lastEdgeState = digitalRead(PIN_COIN_PULSE);
      bool timedOut = false;

      while (coinsDispensed < requiredCoinsPayout) {
        int curState = digitalRead(PIN_COIN_PULSE);

        // Detect Falling edge (HIGH -> LOW on coin detection)
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

        // 5-Second Auto-Cutoff Timeout Check
        if (millis() - lastPulseTime >= COIN_TIMEOUT_MS) {
          timedOut = true;
          Serial.println();
          Serial.println(F("=================================================="));
          Serial.println(F(">>> [TIMEOUT ALERT] NO COINS DETECTED FOR 5 SECONDS!"));
          Serial.println(F(">>> Hopper is EMPTY or JAMMED. Motor auto-stopped!"));
          Serial.println(F("=================================================="));
          break;
        }
        delay(2);
      }

      // Cut power to hopper motor
      digitalWrite(PIN_RELAY_HOPPER, HIGH);

      if (timedOut || coinsDispensed < requiredCoinsPayout) {
        soundError();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("EMPTY HOPPER!   ");
        lcd.setCursor(0, 1);
        lcd.print("REFILL 1P COINS ");
        Serial.print(F("[ALERT] Dispense incomplete: "));
        Serial.print(coinsDispensed);
        Serial.print(F(" / "));
        Serial.println(requiredCoinsPayout);

        // Automated Emergency GSM SMS to Administrator phones
        sendLowCoinSMS(coinsDispensed, requiredCoinsPayout);

        delay(3000);
      } else {
        soundSuccess();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("PAYOUT COMPLETE!");
        lcd.setCursor(0, 1);
        lcd.print("Thank You! :)   ");
        Serial.println(F("[PAYOUT] Success! All coins dispensed."));
        delay(3000);
      }

      currentDepositCount = 0;
      currentState = STATE_STANDBY_MENU;
      showMenuLCD();
      break;
    }

    case STATE_BIN_FULL_LOCKED: {
      // 1. Safe actuators
      digitalWrite(PIN_RELAY_HOPPER, HIGH); // Ensure hopper motor is OFF
      trapdoor.write(SERVO_STANDBY_ANGLE);  // Keep flap firmly closed (0 deg)
      digitalWrite(PIN_LED_GREEN, LOW);
      digitalWrite(PIN_LED_RED, HIGH);      // Red alert LED
      soundAlarm();

      // 2. LCD Notification
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("BIN IS FULL!    ");
      lcd.setCursor(0, 1);
      lcd.print("DISPATCHING SMS ");

      // 3. Automated SMS alert to admins
      sendBinFullSMS();

      // 4. Update LCD prompt
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("BIN IS FULL!    ");
      lcd.setCursor(0, 1);
      lcd.print("HOLD RED: RESET ");

      // 5. System Lock Loop: waits for bin empty & 2s hold on Red button
      while (true) {
        // Forward Serial / GSM passthrough
        if (Serial.available()) {
          char c = Serial.read();
          if (c == 'd' || c == 'D') printDiagnostics();
          else if (c == 's' || c == 'S') { Serial.println(F("\n>> AT+CSQ")); gsm.println("AT+CSQ"); }
          else if (c == 'n' || c == 'N') { Serial.println(F("\n>> AT+CREG?")); gsm.println("AT+CREG?"); }
          else gsm.write(c);
        }
        if (gsm.available()) Serial.write(gsm.read());

        // Check if Red button is held for 2 seconds to reset
        if (digitalRead(PIN_BTN_RED) == LOW) {
          unsigned long holdStart = millis();
          bool longPressed = false;
          while (digitalRead(PIN_BTN_RED) == LOW) {
            if (millis() - holdStart >= 2000) {
              longPressed = true;
              break;
            }
            delay(50);
          }

          if (longPressed) {
            if (digitalRead(PIN_IR_BIN_FULL) == LOW) {
              soundError();
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("BIN STILL FULL! ");
              lcd.setCursor(0, 1);
              lcd.print("Empty bin first!");
              Serial.println(F("[RESET FAIL] Cannot reset: IR Bin-Full Sensor is still blocked!"));
              delay(2500);
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("BIN IS FULL!    ");
              lcd.setCursor(0, 1);
              lcd.print("HOLD RED: RESET ");
            } else {
              soundSuccess();
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("SYSTEM RESET OK ");
              lcd.setCursor(0, 1);
              lcd.print("Resuming Normal ");
              Serial.println(F("[RESET SUCCESS] Bin cleared and system reset to Standby."));
              delay(1500);
              binBlockedStartTime = 0;
              currentDepositCount = 0;
              currentState = STATE_STANDBY_MENU;
              showMenuLCD();
              while (digitalRead(PIN_BTN_RED) == LOW);
              break;
            }
          }
        }
        delay(50);
      }
      break;
    }
  }

  // Forward GSM responses to Serial Monitor
  if (gsm.available()) {
    Serial.write(gsm.read());
  }
}
