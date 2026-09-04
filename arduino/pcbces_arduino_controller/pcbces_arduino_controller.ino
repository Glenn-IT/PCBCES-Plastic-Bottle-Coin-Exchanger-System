/*
 * PCBCES - Master Capstone Reverse Vending Machine Controller
 * Plastic Bottle Coin Exchanger System with GSM Bin Full SMS Telemetry
 * 
 * Hardware:
 * - Arduino Uno R3
 * - 16x2 I2C LCD (PCF8574)
 * - Single Smart Push Button & Active Buzzer
 * - HX711 + 1kg Straight Bar Load Cell
 * - HC-SR04+ Ultrasonic & IR Entry Sensor
 * - LJ12A3 (Metal Reject) & LJC18A3 (Plastic Verify) via 10k/4.7k dividers
 * - MG996R Metal Gear Servo (Accept/Reject Trapdoor)
 * - 12V Coin Hopper & 5V Relay (3.00 PHP Payout)
 * - SIM800L GSM Module (Bin Full SMS Dispatcher)
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <SoftwareSerial.h>
#include "HX711.h"
#include "config.h"

// Hardware Instances
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo trapdoor;
HX711 scale;
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

// Button timing
unsigned long btnPressStartTime = 0;
bool lastBtnState = HIGH;

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

// Ultrasonic reading
long readChamberDistance() {
  digitalWrite(PIN_ULTRASONIC_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_ULTRASONIC_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_ULTRASONIC_TRIG, LOW);
  long duration = pulseIn(PIN_ULTRASONIC_ECHO, HIGH, 30000);
  if (duration == 0) return 999;
  return duration * 0.034 / 2;
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
  lcd.print("SELECT BOTTLE:");
  lcd.setCursor(0, 1);
  if (selectedType == TYPE_1_5L) {
    lcd.print("> 1.5L  (5 pcs)");
  } else {
    lcd.print("> Mismo (10pcs)");
  }
}

void showProgressLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(selectedType == TYPE_1_5L ? "1.5L PET Deposit" : "Mismo Deposit   ");
  lcd.setCursor(0, 1);
  lcd.print("Progress: ");
  lcd.print(currentDepositCount);
  lcd.print("/");
  lcd.print(requiredQuota);
}

void setup() {
  Serial.begin(115200);
  gsm.begin(9600);

  // Pin Modes
  pinMode(PIN_BUTTON_MENU, INPUT_PULLUP);
  pinMode(PIN_IR_ENTRY, INPUT);
  pinMode(PIN_CAP_PLASTIC, INPUT);
  pinMode(PIN_IND_METAL, INPUT);
  pinMode(PIN_COIN_PULSE, INPUT_PULLUP);
  pinMode(PIN_ULTRASONIC_TRIG, OUTPUT);
  pinMode(PIN_ULTRASONIC_ECHO, INPUT);
  
  pinMode(PIN_RELAY_HOPPER, OUTPUT);
  digitalWrite(PIN_RELAY_HOPPER, HIGH); // Relay OFF

  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);

  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED_RED, LOW);
  digitalWrite(PIN_LED_GREEN, LOW);

  // Attach Interrupt for Hopper
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
  lcd.print(" Capstone Ready ");
  soundBeep(120);

  // HX711 Setup
  scale.begin(PIN_LOADCELL_DT, PIN_LOADCELL_SCK);
  scale.set_scale(LOADCELL_CAL_FACTOR);
  scale.tare();

  delay(1200);
  showMenuLCD();
  Serial.println(F("[MASTER] PCBCES Fully Initialized."));
}

void loop() {
  switch (currentState) {
    case STATE_STANDBY_MENU: {
      digitalWrite(PIN_LED_GREEN, HIGH);
      digitalWrite(PIN_LED_RED, LOW);
      
      bool btn = digitalRead(PIN_BUTTON_MENU);
      if (lastBtnState == HIGH && btn == LOW) {
        btnPressStartTime = millis();
        delay(50);
      }
      if (lastBtnState == LOW && btn == HIGH) {
        unsigned long dur = millis() - btnPressStartTime;
        if (dur < 1000) {
          // Toggle
          selectedType = (selectedType == TYPE_1_5L) ? TYPE_MISMO : TYPE_1_5L;
          requiredQuota = (selectedType == TYPE_1_5L) ? BOTTLE_1_5L_QUOTA : BOTTLE_MISMO_QUOTA;
          soundBeep(50);
          showMenuLCD();
        }
      }
      // Hold 1.5s to confirm
      if (btn == LOW && (millis() - btnPressStartTime >= 1500)) {
        soundBeep(200);
        currentDepositCount = 0;
        currentState = STATE_WAIT_INSERTION;
        showProgressLCD();
        while (digitalRead(PIN_BUTTON_MENU) == LOW);
      }
      lastBtnState = btn;
      break;
    }

    case STATE_WAIT_INSERTION: {
      // Check IR Entry Sensor
      if (digitalRead(PIN_IR_ENTRY) == LOW) {
        // Bottle detected in entry chute!
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("BOTTLE DETECTED");
        lcd.setCursor(0, 1);
        lcd.print("Scanning Sensors");
        delay(800); // Allow bottle to settle on tray
        currentState = STATE_VALIDATE_BOTTLE;
      }
      break;
    }

    case STATE_VALIDATE_BOTTLE: {
      // 1. Metal Check (LJ12A3)
      bool isMetal = (digitalRead(PIN_IND_METAL) == LOW);
      if (isMetal) {
        Serial.println(F("[VALIDATION] REJECT: Metal detected!"));
        currentState = STATE_REJECT_EJECT;
        break;
      }

      // 2. Weight Check (HX711)
      float weight = scale.get_units(5);
      Serial.print(F("[VALIDATION] Measured Weight: "));
      Serial.print(weight);
      Serial.println(F(" g"));

      if (weight > WEIGHT_FRAUD_LIMIT || weight < 10.0) {
        Serial.println(F("[VALIDATION] REJECT: Weight out of valid empty range!"));
        currentState = STATE_REJECT_EJECT;
        break;
      }

      // 3. Height Check (HC-SR04)
      long height = readChamberDistance();
      Serial.print(F("[VALIDATION] Chamber Height: "));
      Serial.print(height);
      Serial.println(F(" cm"));

      bool validHeight = false;
      if (selectedType == TYPE_1_5L && height >= HEIGHT_1_5L_MIN && height <= HEIGHT_1_5L_MAX) {
        validHeight = true;
      } else if (selectedType == TYPE_MISMO && height >= HEIGHT_MISMO_MIN && height <= HEIGHT_MISMO_MAX) {
        validHeight = true;
      }

      if (!validHeight) {
        Serial.println(F("[VALIDATION] REJECT: Bottle height mismatch!"));
        currentState = STATE_REJECT_EJECT;
        break;
      }

      // All verified!
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
      delay(1500);
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
      Serial.println(F("[TRAPDOOR] Opening flap (180 deg) to return bottle..."));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("BOTTLE REJECTED!");
      lcd.setCursor(0, 1);
      lcd.print("Pls Try Again");
      soundError();

      trapdoor.write(SERVO_REJECT_ANGLE);
      delay(2000);
      trapdoor.write(SERVO_STANDBY_ANGLE);
      delay(500);

      showProgressLCD();
      currentState = STATE_WAIT_INSERTION;
      break;
    }

    case STATE_PAYOUT_COINS: {
      Serial.println(F("[PAYOUT] Quota reached! Dispensing 3.00 PHP..."));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("QUOTA REACHED!");
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
      lcd.print("SYSTEM PAUSED   ");
      soundError();

      sendBinFullSMS();

      // Lock until admin clears and presses button for 5 seconds
      while (true) {
        if (digitalRead(PIN_BUTTON_MENU) == LOW) {
          unsigned long holdStart = millis();
          while (digitalRead(PIN_BUTTON_MENU) == LOW) {
            if (millis() - holdStart >= 5000) {
              totalBinBottles = 0;
              currentDepositCount = 0;
              soundSuccess();
              currentState = STATE_STANDBY_MENU;
              showMenuLCD();
              return;
            }
          }
        }
        delay(100);
      }
      break;
    }
  }
}