/*
 * PCBCES - Master Capstone Reverse Vending Machine Controller
 * Plastic Bottle Coin Exchanger System with 3-Button UI & GSM Telemetry
 * Last Updated: 2026-09-06 14:48:00 (+08:00)
 * 
 * Hardware Architecture:
 * - Arduino Uno R3
 * - 16x2 I2C LCD (PCF8574)
 * - 3 Dedicated Control Buttons (INPUT_PULLUP):
 *     * Button Green (Pin 10): 1.5L Bottle Mode (5 pcs = 3 PHP)
 *     * Button Blue  (Pin A0): Mismo Bottle Mode (10 pcs = 3 PHP)
 *     * Button Red   (Pin A1): System Restart / Cancel Transaction
 * - Active 5V Buzzer & Green/Red Status LEDs
 * - HC-SR04+ Ultrasonic (Bottle Length / Size Discrimination)
 * - IR Obstacle Avoidance (Bottle Insertion Beam Trigger)
 * - LJ12A3 Inductive Sensor (Metallic Object Rejection)
 * - MG996R Metal Gear Servo (Accept/Reject Trapdoor Flap)
 * - 12V Coin Hopper & 5V Relay (3.00 PHP Payout)
 * - SIM800L GSM Module (Storage Bin Full SMS Telemetry)
 * - Pin D5: Spare / Unassigned GPIO (LJC18A3 Capacitive Sensor omitted)
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <SoftwareSerial.h>
#include "config.h"

// Hardware Instances
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo trapdoor;
SoftwareSerial gsm(PIN_GSM_RX, PIN_GSM_TX);

// State Machine Definition
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
enum BottleType { TYPE_1_5L, TYPE_MISMO };
BottleType selectedType = TYPE_1_5L;

int currentDepositCount = 0;
int requiredQuota = BOTTLE_1_5L_QUOTA;
int totalBinBottles = 0;
volatile int coinsDispensed = 0;

// Coin Pulse Counter ISR
void onCoinPulse() {
  static unsigned long lastPulse = 0;
  unsigned long now = millis();
  if (now - lastPulse > 60) {
    coinsDispensed++;
    lastPulse = now;
  }
}

// Sound Helpers
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

// Ultrasonic reading (Vertical Top-Down: Ceiling Sensor to Bottle Cap)
// 5-sample median filter with 30ms acoustic decay to eliminate internal chamber echo ringing
long singlePingChamber() {
  digitalWrite(PIN_ULTRASONIC_TRIG, LOW);
  delayMicroseconds(4);
  digitalWrite(PIN_ULTRASONIC_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_ULTRASONIC_TRIG, LOW);

  // 6000us timeout = ~102 cm max range (prevents listening to stray wall reflections)
  long duration = pulseIn(PIN_ULTRASONIC_ECHO, HIGH, 6000);
  if (duration <= 0) return CHAMBER_TOTAL_HEIGHT_CM;
  long cm = duration * 0.034 / 2;
  if (cm < 2 || cm > 60) return CHAMBER_TOTAL_HEIGHT_CM;
  return cm;
}

long readChamberDistance() {
  const int NUM_SAMPLES = 5;
  long samples[NUM_SAMPLES];

  for (int i = 0; i < NUM_SAMPLES; i++) {
    samples[i] = singlePingChamber();
    delay(30); // 30ms inter-ping acoustic dissipation delay
  }

  // Insertion sort to extract true median
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

// GSM SMS Alert
void sendBinFullSMS() {
  Serial.println(F("[GSM] Sending Bin-Full Alert to Admin..."));
  gsm.println("AT+CMGF=1");
  delay(400);
  gsm.print("AT+CMGS=\"");
  gsm.print(ADMIN_PHONE_NUMBER);
  gsm.println("\"");
  delay(400);
  gsm.print("ALERT: PCBCES Storage Bin is FULL (");
  gsm.print(totalBinBottles);
  gsm.print(" bottles)! Machine is paused. Please empty bin.");
  delay(400);
  gsm.write(26); // Ctrl+Z
  delay(3000);
  Serial.println(F("[GSM] SMS Dispatch command executed."));
}

void showMenuLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("GRN:1.5L BLU:MIS");
  lcd.setCursor(0, 1);
  lcd.print("RED:Cancel/Reset");
}

void showProgressLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  if (selectedType == TYPE_1_5L) {
    lcd.print("1.5L PET (5 pcs)");
  } else {
    lcd.print("Mismo PET (10pcs)");
  }
  lcd.setCursor(0, 1);
  lcd.print("Count: ");
  lcd.print(currentDepositCount);
  lcd.print("/");
  lcd.print(requiredQuota);
  lcd.print(" [RED:X]");
}

// Red Button: Cancel Transaction & Return to Standby
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
      delay(1500);
      currentDepositCount = 0;
      currentState = STATE_STANDBY_MENU;
      showMenuLCD();
      while (digitalRead(PIN_BTN_RED) == LOW); // wait for release
      return true;
    }
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  gsm.begin(9600);

  // 3 Dedicated Buttons (Internal Pullups to GND)
  pinMode(PIN_BTN_GREEN, INPUT_PULLUP);
  pinMode(PIN_BTN_BLUE, INPUT_PULLUP);
  pinMode(PIN_BTN_RED, INPUT_PULLUP);

  // Sensors & Actuators
  pinMode(PIN_IR_ENTRY, INPUT);
  pinMode(PIN_IND_METAL, INPUT);
  pinMode(PIN_COIN_PULSE, INPUT_PULLUP);
  pinMode(PIN_ULTRASONIC_TRIG, OUTPUT);
  pinMode(PIN_ULTRASONIC_ECHO, INPUT);
  
  pinMode(PIN_RELAY_HOPPER, OUTPUT);
  digitalWrite(PIN_RELAY_HOPPER, HIGH); // Relay OFF (Active LOW relay)

  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);

  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED_RED, LOW);
  digitalWrite(PIN_LED_GREEN, LOW);

  // Attach Interrupt for Coin Hopper
  attachInterrupt(digitalPinToInterrupt(PIN_COIN_PULSE), onCoinPulse, FALLING);

  // Servo Setup
  trapdoor.attach(PIN_SERVO_TRAPDOOR);
  trapdoor.write(SERVO_STANDBY_ANGLE);

  // LCD Setup
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(" PCBCES SYSTEM  ");
  lcd.setCursor(0, 1);
  lcd.print(" 3-Button Ready ");
  soundBeep(120);

  delay(1200);
  showMenuLCD();
  Serial.println(F("[MASTER] PCBCES Fully Initialized with 3 Dedicated Buttons:"));
  Serial.println(F(" - GREEN (D10): 1.5L Mode"));
  Serial.println(F(" - BLUE  (A0):  Mismo Mode"));
  Serial.println(F(" - RED   (A1):  System Restart / Cancel"));
}

void loop() {
  switch (currentState) {
    case STATE_STANDBY_MENU: {
      digitalWrite(PIN_LED_GREEN, HIGH);
      digitalWrite(PIN_LED_RED, LOW);

      // 1. GREEN BUTTON -> Select 1.5L Mode
      if (digitalRead(PIN_BTN_GREEN) == LOW) {
        delay(50);
        if (digitalRead(PIN_BTN_GREEN) == LOW) {
          selectedType = TYPE_1_5L;
          requiredQuota = BOTTLE_1_5L_QUOTA;
          currentDepositCount = 0;
          soundBeep(100);
          Serial.println(F("[MENU] GREEN pressed: 1.5L Mode Selected (5 pcs)"));
          
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("MODE: 1.5L PET  ");
          lcd.setCursor(0, 1);
          lcd.print("Target: 5 Pcs   ");
          delay(1000);
          
          currentState = STATE_WAIT_INSERTION;
          showProgressLCD();
          while (digitalRead(PIN_BTN_GREEN) == LOW);
          break;
        }
      }

      // 2. BLUE BUTTON -> Select Mismo Mode
      if (digitalRead(PIN_BTN_BLUE) == LOW) {
        delay(50);
        if (digitalRead(PIN_BTN_BLUE) == LOW) {
          selectedType = TYPE_MISMO;
          requiredQuota = BOTTLE_MISMO_QUOTA;
          currentDepositCount = 0;
          soundBeep(100);
          Serial.println(F("[MENU] BLUE pressed: Mismo Mode Selected (10 pcs)"));
          
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("MODE: MISMO PET ");
          lcd.setCursor(0, 1);
          lcd.print("Target: 10 Pcs  ");
          delay(1000);
          
          currentState = STATE_WAIT_INSERTION;
          showProgressLCD();
          while (digitalRead(PIN_BTN_BLUE) == LOW);
          break;
        }
      }

      // 3. RED BUTTON -> Reset Standby Menu
      if (digitalRead(PIN_BTN_RED) == LOW) {
        delay(50);
        if (digitalRead(PIN_BTN_RED) == LOW) {
          soundBeep(150);
          currentDepositCount = 0;
          Serial.println(F("[MENU] RED pressed: Standby Refreshed / Reset."));
          showMenuLCD();
          while (digitalRead(PIN_BTN_RED) == LOW);
          break;
        }
      }
      break;
    }

    case STATE_WAIT_INSERTION: {
      // Check if user pressed RED button to cancel/restart transaction
      if (checkCancelButton()) break;

      // Check IR Entry Sensor
      if (digitalRead(PIN_IR_ENTRY) == LOW) {
        // Bottle detected in entry chute!
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("BOTTLE DETECTED ");
        lcd.setCursor(0, 1);
        lcd.print("Scanning Sensors");
        delay(600); // Allow bottle to settle on cradle
        currentState = STATE_VALIDATE_BOTTLE;
      }
      break;
    }

    case STATE_VALIDATE_BOTTLE: {
      Serial.println(F("[VALIDATION] Commencing multi-sensor verification..."));

      // 1. Metal Rejection Check (LJ12A3 Inductive Sensor)
      // Active LOW when metal is detected
      bool isMetal = (digitalRead(PIN_IND_METAL) == LOW);
      if (isMetal) {
        Serial.println(F("[VALIDATION] REJECT: Metallic object detected!"));
        currentState = STATE_REJECT_EJECT;
        break;
      }

      // 2. Dimensional Vertical Distance Check (HC-SR04 Ultrasonic at Ceiling)
      // Measures distance from ceiling down to the bottle cap to verify correct size
      long distToCap = readChamberDistance();
      long computedBottleHeight = (CHAMBER_TOTAL_HEIGHT_CM > distToCap) ? (CHAMBER_TOTAL_HEIGHT_CM - distToCap) : 0;
      Serial.print(F("[VALIDATION] Top-to-Cap Distance: "));
      Serial.print(distToCap);
      Serial.print(F(" cm (Approx Bottle Height: "));
      Serial.print(computedBottleHeight);
      Serial.println(F(" cm)"));

      bool validDimensions = false;
      if (selectedType == TYPE_1_5L && distToCap >= DIST_1_5L_MIN && distToCap <= DIST_1_5L_MAX) {
        validDimensions = true;
      } else if (selectedType == TYPE_MISMO && distToCap >= DIST_MISMO_MIN && distToCap <= DIST_MISMO_MAX) {
        validDimensions = true;
      }

      if (!validDimensions) {
        Serial.print(F("[VALIDATION] REJECT: Bottle dimensions mismatch for "));
        Serial.println((selectedType == TYPE_1_5L) ? F("1.5L Mode") : F("Mismo Mode"));
        currentState = STATE_REJECT_EJECT;
        break;
      }

      // All criteria passed!
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

      trapdoor.write(SERVO_ACCEPT_ANGLE);
      delay(1500); // Allow bottle to slide into lower collection bin
      trapdoor.write(SERVO_STANDBY_ANGLE);
      delay(500);

      currentDepositCount++;
      totalBinBottles++;

      // Check Bin Capacity
      if (totalBinBottles >= MAX_BIN_CAPACITY) {
        currentState = STATE_BIN_FULL_LOCKED;
        break;
      }

      if (currentDepositCount >= requiredQuota) {
        currentState = STATE_PAYOUT_COINS;
      } else {
        showProgressLCD();
        currentState = STATE_WAIT_INSERTION;
      }
      break;
    }

    case STATE_REJECT_EJECT: {
      Serial.println(F("[TRAPDOOR] Rejection sequence triggered (Hatch remains closed)."));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("BOTTLE REJECTED!");
      lcd.setCursor(0, 1);
      lcd.print("Pls Try Again   ");
      soundError();

      // Flap remains at 0 to prevent drop into bin
      trapdoor.write(SERVO_STANDBY_ANGLE);
      delay(2000);

      showProgressLCD();
      currentState = STATE_WAIT_INSERTION;
      break;
    }

    case STATE_PAYOUT_COINS: {
      Serial.println(F("[PAYOUT] Quota reached! Dispensing 3.00 PHP..."));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("QUOTA REACHED!  ");
      lcd.setCursor(0, 1);
      lcd.print("Dispensing 3 PHP");

      coinsDispensed = 0;
      digitalWrite(PIN_RELAY_HOPPER, LOW); // Start 12V Hopper Motor

      unsigned long payoutStart = millis();
      while (coinsDispensed < COINS_PAYOUT_TARGET) {
        // Safety timeout of 10 seconds
        if (millis() - payoutStart > 10000) {
          Serial.println(F("[PAYOUT ERROR] Hopper timeout (Low coins?)"));
          break;
        }
        delay(20);
      }

      digitalWrite(PIN_RELAY_HOPPER, HIGH); // Stop Hopper Motor
      soundSuccess();

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("PAYOUT COMPLETE!");
      lcd.setCursor(0, 1);
      lcd.print("Thank You! :)   ");
      delay(3000);

      currentDepositCount = 0;
      currentState = STATE_STANDBY_MENU;
      showMenuLCD();
      break;
    }

    case STATE_BIN_FULL_LOCKED: {
      Serial.println(F("[ALERT] Storage Bin is Full. System Locked."));
      digitalWrite(PIN_RELAY_HOPPER, HIGH);
      digitalWrite(PIN_LED_RED, HIGH);
      digitalWrite(PIN_LED_GREEN, LOW);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("BIN IS FULL!    ");
      lcd.setCursor(0, 1);
      lcd.print("RED: System Rst ");
      soundError();

      sendBinFullSMS();

      // Press RED button for 2 seconds to restart after emptying bin
      while (true) {
        if (digitalRead(PIN_BTN_RED) == LOW) {
          unsigned long holdStart = millis();
          while (digitalRead(PIN_BTN_RED) == LOW) {
            if (millis() - holdStart >= 2000) {
              totalBinBottles = 0;
              currentDepositCount = 0;
              soundSuccess();
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("SYSTEM RESET OK ");
              lcd.setCursor(0, 1);
              lcd.print("Resuming Normal ");
              delay(1500);
              currentState = STATE_STANDBY_MENU;
              showMenuLCD();
              return;
            }
          }
        }
        delay(50);
      }
      break;
    }
  }
}
