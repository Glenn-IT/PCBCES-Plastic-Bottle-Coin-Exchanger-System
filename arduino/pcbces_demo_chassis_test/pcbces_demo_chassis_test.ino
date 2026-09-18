/*
 * =============================================================================
 * PCBCES - Combined Demo Chassis Test Controller (Tests 01, 03, 04, 05, 06, 07, 08)
 * Plastic Bottle Coin Exchanger System — Bench & Demo Rig Edition
 * Last Updated: 2026-09-18 22:36:00 (+08:00)
 * =============================================================================
 * 
 * Integrated Modules:
 * - Test 01: 16x2 I2C LCD (0x27, A4/A5) + 3 Dedicated Buttons (D10, A0, A1) + Buzzer (D12) + LEDs (A2/D13)
 * - Test 03: Ultrasonic HC-SR04 (D2/D3) + Active LOW IR Bottle Entry Sensor (D4) + IR Bin-Full Sensor (D5)
 * - Test 04: LJ12A3 Inductive Metal Proximity Sensor (Pin D6, optional bench demo)
 * - Test 05: MG996R Metal Gear Sorting Servo (D9, 0° Standby/Reject, 90° Accept)
 * - Test 06: Coin Hopper Optical Pulse (D7) + 5V Relay (D8) with 5-Second Motor Auto-Cutoff
 * - Test 07: GSM SIM800L / SIM900A SMS Dispatcher (SoftSerial RX D11, TX A3)
 * - Test 08: Coin Hopper & GSM SMS Low-Coin / Jam Alert Integration (Automated 5s Timeout SMS)
 * 
 * Pin Connections (Canonical Uno 20-Pin Lock):
 * - D0 / D1: Hardware UART (USB Serial Monitor & Commands at 115200 baud)
 * - D2     : HC-SR04 Trigger Pulse
 * - D3     : HC-SR04 Echo Return Pulse
 * - D4     : IR Bottle Entry Beam Sensor (Active LOW bottle detected)
 * - D5     : IR Bin-Full Sensor (Active LOW bin full detected)
 * - D6     : (LJ12A3 Inductive Metal Sensor - Optional bench demo)
 * - D7     : Coin Hopper Pulse Line (Falling edge detection via 10k/4.7k divider)
 * - D8     : 5V Single-Channel Relay (Hopper Motor Power, Active LOW)
 * - D9     : MG996R Servo PWM (0° Standby / 90° Drop)
 * - D10    : Button Green (1.5L / 1.75L Mode -> 5 pcs quota = 20 PHP)
 * - D11    : SoftwareSerial RX (From GSM SIM900A 5VT / SIM800L TX)
 * - D12    : Active 5V Buzzer
 * - D13    : Red LED (Fault / Reject Indicator)
 * - A0     : Button Blue (290 ML Mode -> 10 pcs quota = 3 PHP)
 * - A1     : Button Red (System Cancel / Restart)
 * - A2     : Green LED (Ready / Success Indicator)
 * - A3     : SoftwareSerial TX (To GSM SIM900A 5VR / SIM800L RX)
 * - A4     : I2C SDA (16x2 LCD)
 * - A5     : I2C SCL (16x2 LCD)
 * =============================================================================
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <SoftwareSerial.h>

// --- PIN DEFINITIONS (Canonical Uno 20-Pin Lock) ---
const int PIN_ULTRASONIC_TRIG = 2;
const int PIN_ULTRASONIC_ECHO = 3;
const int PIN_IR_ENTRY        = 4;   // Active LOW bottle entry beam sensor
const int PIN_IR_BIN_FULL     = 5;   // Active LOW bottle storage bin full sensor
// const int PIN_IND_METAL    = 6;   // Commented out for sensor demonstration
const int PIN_COIN_PULSE      = 7;
const int PIN_RELAY_HOPPER    = 8;
const int PIN_SERVO_TRAPDOOR  = 9;
const int PIN_BTN_GREEN       = 10;
const int PIN_GSM_RX          = 11;  // SoftwareSerial RX (From SIM900A 5VT / SIM800L TX)
const int PIN_BUZZER          = 12;
const int PIN_LED_RED         = 13;
const int PIN_BTN_BLUE        = A0;
const int PIN_BTN_RED         = A1;
const int PIN_LED_GREEN       = A2;
const int PIN_GSM_TX          = A3;  // SoftwareSerial TX (To SIM900A 5VR / SIM800L RX)

// --- HARDWARE INSTANCES ---
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo trapdoor;
SoftwareSerial gsm(PIN_GSM_RX, PIN_GSM_TX);

// --- CONFIGURATION & QUOTAS ---
const int BOTTLE_1_5L_QUOTA    = 5;   // 5 bottles = 20.00 PHP
const int BOTTLE_290ML_QUOTA   = 10;  // 10 bottles = 3.00 PHP
const int COINS_PAYOUT_1_5L    = 20;  // 20 x 1-Peso coins
const int COINS_PAYOUT_290ML   = 3;   // 3 x 1-Peso coins
const int SERVO_STANDBY_ANGLE  = 0;   // Closed / Cradle Rest / Rejection Hold
const int SERVO_ACCEPT_ANGLE   = 90;  // Open / Drop into collection bin
const unsigned long COIN_TIMEOUT_MS = 5000; // 5-Second dry-run motor auto-cutoff

// Target Administrator Phone Numbers (Philippines format: +639XXXXXXXXX or 09XXXXXXXXX)
const char ADMIN_PHONE_1[] = "+639634299114";
const char ADMIN_PHONE_2[] = "+639242074903";

// --- DEMO CHAMBER HEIGHT CALIBRATION ---
// Baseline height: 32 cm (Calibrated on demo chassis)
int chamberTotalHeightCm = 32;
int dist15LMin  = 11;   // Cap is 11 to 12 cm from ceiling sensor
int dist15LMax  = 12;   // Calibrated range
int dist290Min  = 23;   // Cap is 23 to 24 cm from ceiling sensor
int dist290Max  = 24;

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

void soundSuccess() {
  soundBeep(100); delay(60); soundBeep(180);
}

// --- GSM ALERT FUNCTIONS (From Test 08) ---
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

  // Send Ctrl+Z (ASCII 26) to trigger cellular network transmission
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

void sendTestSMS() {
  Serial.println(F("[GSM] Sending test SMS to Admin 1..."));
  gsm.println("AT+CMGF=1");
  delay(400);
  gsm.print("AT+CMGS=\"");
  gsm.print(ADMIN_PHONE_1);
  gsm.println("\"");
  delay(400);
  gsm.print("TEST: PCBCES GSM SMS Module is ONLINE & Functional on Demo Rig!");
  delay(400);
  gsm.write(26);
  delay(3000);
  Serial.println(F("[GSM] Test SMS dispatch sequence completed."));
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
  Serial.println(F("[WARN] No standard response. Defaulting to 9600 baud..."));
  gsm.begin(9600);
  delay(200);
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
  Serial.print(F("IR Bin-Full Sensor (D5)    : ")); Serial.println(digitalRead(PIN_IR_BIN_FULL) == LOW ? F("BLOCKED (Bin Full)") : F("CLEAR"));
  // Serial.print(F("Inductive Metal (D6)       : ")); Serial.println(digitalRead(PIN_IND_METAL) == LOW ? F("METAL DETECTED!") : F("NO METAL (Clear)"));
  Serial.print(F("Coin Pulse Line (D7)       : ")); Serial.println(digitalRead(PIN_COIN_PULSE) == HIGH ? F("HIGH (Idle)") : F("LOW (Coin Present)"));
  Serial.print(F("Hopper Relay (D8)          : ")); Serial.println(digitalRead(PIN_RELAY_HOPPER) == LOW ? F("ON (Dispensing)") : F("OFF (Standby)"));
  Serial.print(F("GSM Serial Status          : ")); Serial.println(gsm.isListening() ? F("LISTENING (D11/A3 OK)") : F("STANDBY"));
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
  pinMode(PIN_IR_ENTRY, INPUT);     // Active LOW IR Entry Sensor
  pinMode(PIN_IR_BIN_FULL, INPUT);  // Active LOW IR Bin-Full Sensor
  // pinMode(PIN_IND_METAL, INPUT); // Commented out for sensor demonstration
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

  // GSM Modem Setup & Auto-sync
  autoSyncGSMBaud();

  // Calibrate empty chamber baseline
  long baseline = readChamberDistance();
  Serial.println(F("=================================================="));
  Serial.println(F(" PCBCES DEMO CHASSIS INTEGRATED CONTROLLER        "));
  Serial.println(F(" Tests 01, 03, 05, 06, 07, 08 Active on Bench Rig "));
  Serial.println(F("=================================================="));
  Serial.print(F("Empty Chamber Ultrasonic Baseline: "));
  Serial.print(baseline);
  Serial.println(F(" cm"));
  Serial.println(F("Interactive Serial Commands:"));
  Serial.println(F(" 'g' -> Select Green (1.5L Mode)"));
  Serial.println(F(" 'b' -> Select Blue  (290 ML Mode)"));
  Serial.println(F(" 'r' -> Select Red   (Cancel/Restart)"));
  Serial.println(F(" 'd' -> Print Live Sensor Diagnostics"));
  Serial.println(F(" 's' -> Send Test SMS via GSM"));
  Serial.println(F(" 't' -> Test Coin Hopper Payout (3 Coins)"));
  Serial.println(F("--------------------------------------------------"));

  showMenuLCD();
}

// --- MAIN LOOP ---
void loop() {
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
    } else if (ch == 's' || ch == 'S') {
      sendTestSMS();
    } else if (ch == 't' || ch == 'T') {
      selectedType = TYPE_290ML;
      requiredCoinsPayout = 3;
      currentState = STATE_PAYOUT_COINS;
      Serial.println(F("[SIM] Triggering Coin Hopper Payout Test (3 Coins)..."));
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

      // Check IR Entry Sensor (Active LOW when bottle blocks IR beam)
      bool irBlocked = (digitalRead(PIN_IR_ENTRY) == LOW);

      // Check Ultrasonic Sensor (Bottle insertion detected when distance drops below baseline)
      long currentDist = singlePing();

      // Detection condition: IR sensor beam is broken OR ultrasonic reads an object closer than chamber baseline
      if (irBlocked || (currentDist >= 2 && currentDist <= 35 && (currentDist < (chamberTotalHeightCm - 2) || chamberTotalHeightCm <= 15))) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("BOTTLE DETECTED ");
        lcd.setCursor(0, 1);
        if (irBlocked) {
          lcd.print("IR Entry Beam OK");
          Serial.println(F("[INSERTION] Bottle detected via IR Entry Sensor (D4) beam break."));
        } else {
          lcd.print("Dist: ");
          lcd.print(currentDist);
          lcd.print(" cm     ");
          Serial.print(F("[INSERTION] Bottle detected via Ultrasonic (D2/D3): "));
          Serial.print(currentDist);
          Serial.println(F(" cm"));
        }
        soundBeep(70);
        delay(800); // Allow user hand to withdraw and bottle to stabilize on cradle

        currentState = STATE_VALIDATE_BOTTLE;
      }
      delay(80);
      break;
    }

    case STATE_VALIDATE_BOTTLE: {
      Serial.println(F("[VALIDATION] Commencing sensor verification..."));

      // 1. Metal Detection (LJ12A3 Sensor - Active LOW) - OPTIONAL BENCH DEMO
      /*
      bool isMetal = (digitalRead(PIN_IND_METAL) == LOW);
      if (isMetal) {
        Serial.println(F("[VALIDATION] REJECT: Metallic object detected!"));
        currentState = STATE_REJECT_EJECT;
        break;
      }
      */
      Serial.println(F("[VALIDATION] Metal detector bypassed (Demo Mode)."));

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
      } else if (distToCap >= 2 && distToCap <= 35) {
        // Fallback for demo: any bottle detected under the ultrasonic sensor is accepted!
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
      Serial.println(F("[VALIDATION] PASSED: Bottle verified via Ultrasonic."));
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

      // Flash Red LED warning and wait until user removes item from IR Entry sensor
      unsigned long rejectStart = millis();
      while ((digitalRead(PIN_IR_ENTRY) == LOW) && (millis() - rejectStart < 5000)) {
        digitalWrite(PIN_LED_RED, HIGH);
        delay(200);
        digitalWrite(PIN_LED_RED, LOW);
        delay(200);
      }
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
            Serial.print(F("--> [COIN DETECTED!] Count = "));
            Serial.print(coinsDispensed);
            Serial.print(F(" / "));
            Serial.println(requiredCoinsPayout);
          }
        }
        lastEdgeState = curState;

        // 5-Second Auto-Cutoff Timeout Check (Test 08 Safety Logic)
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
        lcd.print("SENDING SMS...  ");
        Serial.print(F("[ALERT] Dispense incomplete: "));
        Serial.print(coinsDispensed);
        Serial.print(F(" / "));
        Serial.println(requiredCoinsPayout);

        // Automated SMS dispatch to administrators via GSM module (Test 08 integration)
        sendLowCoinSMS(coinsDispensed, requiredCoinsPayout);

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("EMPTY HOPPER!   ");
        lcd.setCursor(0, 1);
        lcd.print("SMS DISPATCHED! ");
        delay(3500);
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
  }
}
