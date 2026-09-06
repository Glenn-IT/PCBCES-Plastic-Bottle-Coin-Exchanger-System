/*
 * PCBCES - Test 01: 16x2 I2C LCD & 3-Button Dedicated Interface Test
 * Hardware: Arduino Uno, 16x2 LCD (PCF8574 I2C), 3x Push Buttons, Active Buzzer
 * 
 * Pin Connections:
 * - LCD SDA            -> A4
 * - LCD SCL            -> A5
 * - Button Green (1.5L)-> D10 (Internal PULLUP to GND)
 * - Button Blue (Mismo)-> A0  (Internal PULLUP to GND)
 * - Button Red (Reset) -> A1  (Internal PULLUP to GND)
 * - Active Buzzer      -> D12 (HIGH = Beep)
 * 
 * Behavior:
 * - Standby: LCD shows "GRN:1.5L BLU:MIS" / "RED:Cancel/Reset"
 * - Press Green: Starts 1.5L deposit mode (Quota: 5 pcs)
 * - Press Blue:  Starts Mismo deposit mode (Quota: 10 pcs)
 * - Press Red:   At ANY time, cancels current transaction, resets count, returns to start
 * 
 * Last Updated: 2026-09-06 14:30:41 (+08:00)
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int PIN_BTN_GREEN = 10;
const int PIN_BTN_BLUE  = A0;
const int PIN_BTN_RED   = A1;
const int PIN_BUZZER    = 12;

enum BottleType { BOTTLE_1_5L, BOTTLE_MISMO };
BottleType selectedType = BOTTLE_1_5L;

bool isStandby = true;
int depositCount = 0;
int targetCount = 5;

void beep(int durationMs = 80) {
  digitalWrite(PIN_BUZZER, HIGH);
  delay(durationMs);
  digitalWrite(PIN_BUZZER, LOW);
}

void showStandbyMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("GRN:1.5L BLU:MIS");
  lcd.setCursor(0, 1);
  lcd.print("RED:Cancel/Reset");
}

void showCountingScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  if (selectedType == BOTTLE_1_5L) {
    lcd.print("1.5L PET (5 pcs)");
  } else {
    lcd.print("Mismo PET (10pcs)");
  }
  lcd.setCursor(0, 1);
  lcd.print("Count: ");
  lcd.print(depositCount);
  lcd.print("/");
  lcd.print(targetCount);
  lcd.print(" [RED:X]");
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BTN_GREEN, INPUT_PULLUP);
  pinMode(PIN_BTN_BLUE, INPUT_PULLUP);
  pinMode(PIN_BTN_RED, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  lcd.init();
  lcd.backlight();
  
  lcd.setCursor(0, 0);
  lcd.print(" PCBCES SYSTEM  ");
  lcd.setCursor(0, 1);
  lcd.print(" 3-Button Bench ");
  beep(150);
  delay(1200);

  showStandbyMenu();
  Serial.println(F("[TEST 01] 3-Button Interface Ready:"));
  Serial.println(F(" - GREEN (D10): 1.5L Mode (5 pcs)"));
  Serial.println(F(" - BLUE  (A0):  Mismo Mode (10 pcs)"));
  Serial.println(F(" - RED   (A1):  System Restart / Cancel"));
}

void loop() {
  // 1. RED BUTTON: CANCEL / RESET (ACTIVE IN ALL MODES)
  if (digitalRead(PIN_BTN_RED) == LOW) {
    delay(50); // debounce
    if (digitalRead(PIN_BTN_RED) == LOW) {
      beep(200);
      Serial.println(F("RED Button Pressed -> Transaction Cancelled / System Reset!"));
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" TRANSACTION    ");
      lcd.setCursor(0, 1);
      lcd.print(" CANCELLED / RST");
      delay(1200);

      depositCount = 0;
      isStandby = true;
      showStandbyMenu();
      while (digitalRead(PIN_BTN_RED) == LOW);
      return;
    }
  }

  // 2. STANDBY MODE SELECTION
  if (isStandby) {
    // GREEN BUTTON: Select 1.5L
    if (digitalRead(PIN_BTN_GREEN) == LOW) {
      delay(50);
      if (digitalRead(PIN_BTN_GREEN) == LOW) {
        selectedType = BOTTLE_1_5L;
        targetCount = 5;
        depositCount = 0;
        isStandby = false;
        beep(100);
        Serial.println(F("GREEN Button Pressed -> 1.5L Mode Selected (Target: 5)"));
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("MODE: 1.5L PET  ");
        lcd.setCursor(0, 1);
        lcd.print("Target: 5 Pcs   ");
        delay(1000);
        showCountingScreen();
        while (digitalRead(PIN_BTN_GREEN) == LOW);
        return;
      }
    }

    // BLUE BUTTON: Select Mismo
    if (digitalRead(PIN_BTN_BLUE) == LOW) {
      delay(50);
      if (digitalRead(PIN_BTN_BLUE) == LOW) {
        selectedType = BOTTLE_MISMO;
        targetCount = 10;
        depositCount = 0;
        isStandby = false;
        beep(100);
        Serial.println(F("BLUE Button Pressed -> Mismo Mode Selected (Target: 10)"));
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("MODE: MISMO PET ");
        lcd.setCursor(0, 1);
        lcd.print("Target: 10 Pcs  ");
        delay(1000);
        showCountingScreen();
        while (digitalRead(PIN_BTN_BLUE) == LOW);
        return;
      }
    }
  } 
  else {
    // 3. IN TRANSACTION (Simulate bottle insert using GREEN button)
    if (digitalRead(PIN_BTN_GREEN) == LOW) {
      delay(50);
      if (digitalRead(PIN_BTN_GREEN) == LOW) {
        depositCount++;
        beep(80);
        showCountingScreen();
        Serial.print(F("Simulated Insert: "));
        Serial.print(depositCount);
        Serial.print(F("/"));
        Serial.println(targetCount);

        if (depositCount >= targetCount) {
          delay(400);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("TARGET REACHED! ");
          lcd.setCursor(0, 1);
          lcd.print("PAYOUT 3.00 PHP ");
          beep(150); delay(80); beep(200);
          delay(3000);
          
          depositCount = 0;
          isStandby = true;
          showStandbyMenu();
        }
        while (digitalRead(PIN_BTN_GREEN) == LOW);
      }
    }
  }
}
