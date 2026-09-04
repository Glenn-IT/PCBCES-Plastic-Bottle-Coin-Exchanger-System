/*
 * PCBCES - Test 01: 16x2 I2C LCD & Single Push Button Smart Menu Test
 * Hardware: Arduino Uno, 16x2 LCD (PCF8574 I2C), Momentary Button, Active Buzzer
 * 
 * Pin Connections:
 * - LCD SDA -> A4
 * - LCD SCL -> A5
 * - Push Button -> D10 (Internal PULLUP to GND)
 * - Active Buzzer -> D12 (HIGH = Beep)
 * 
 * Behavior:
 * - Click button: Toggle selection between 1.5L and Mismo
 * - Hold button for 1.5 seconds: Confirm selection and start deposit counter
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Set the LCD address to 0x27 or 0x3F for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int BTN_PIN = 10;
const int BUZZER_PIN = 12;

enum BottleType { BOTTLE_1_5L, BOTTLE_MISMO };
BottleType selectedType = BOTTLE_1_5L;

bool isSelecting = true;
int depositCount = 0;
int targetCount = 5;

unsigned long btnPressTime = 0;
bool lastBtnState = HIGH;

void beep(int durationMs = 80) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(durationMs);
  digitalWrite(BUZZER_PIN, LOW);
}

void updateMenuDisplay() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SELECT BOTTLE:");
  lcd.setCursor(0, 1);
  if (selectedType == BOTTLE_1_5L) {
    lcd.print("> 1.5L  (5 pcs)");
  } else {
    lcd.print("> Mismo (10pcs)");
  }
}

void updateCountDisplay() {
  lcd.clear();
  lcd.setCursor(0, 0);
  if (selectedType == BOTTLE_1_5L) {
    lcd.print("Insert 1.5L PET");
  } else {
    lcd.print("Insert Mismo PET");
  }
  lcd.setCursor(0, 1);
  lcd.print("Count: ");
  lcd.print(depositCount);
  lcd.print("/");
  lcd.print(targetCount);
  lcd.print(" Pcs");
}

void setup() {
  Serial.begin(115200);
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  lcd.init();
  lcd.backlight();
  
  lcd.setCursor(0, 0);
  lcd.print(" PCBCES SYSTEM  ");
  lcd.setCursor(0, 1);
  lcd.print(" Capstone Init  ");
  beep(150);
  delay(1200);

  updateMenuDisplay();
  Serial.println(F("[TEST 01] LCD & Button Ready. Click D10 to toggle, hold 1.5s to confirm."));
}

void loop() {
  bool currentBtnState = digitalRead(BTN_PIN);

  // Button pressed (falling edge)
  if (lastBtnState == HIGH && currentBtnState == LOW) {
    btnPressTime = millis();
    delay(50); // debounce
  }

  // Button released (rising edge)
  if (lastBtnState == LOW && currentBtnState == HIGH) {
    unsigned long pressDuration = millis() - btnPressTime;

    if (pressDuration < 1000) {
      // Short Click: Toggle selection or increment count
      if (isSelecting) {
        selectedType = (selectedType == BOTTLE_1_5L) ? BOTTLE_MISMO : BOTTLE_1_5L;
        targetCount = (selectedType == BOTTLE_1_5L) ? 5 : 10;
        beep(50);
        updateMenuDisplay();
        Serial.print(F("Toggled option: "));
        Serial.println(selectedType == BOTTLE_1_5L ? "1.5L" : "Mismo");
      } else {
        // In counting mode: simulated bottle insert
        depositCount++;
        beep(80);
        updateCountDisplay();
        Serial.print(F("Simulated Insert: "));
        Serial.print(depositCount);
        Serial.print(F("/"));
        Serial.println(targetCount);

        if (depositCount >= targetCount) {
          delay(400);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("TARGET REACHED!");
          lcd.setCursor(0, 1);
          lcd.print("PAYOUT 3.00 PHP");
          beep(200); delay(100); beep(200);
          delay(3000);
          // Reset
          isSelecting = true;
          depositCount = 0;
          updateMenuDisplay();
        }
      }
    }
    delay(50);
  }

  // Long Press check while button is still held down
  if (currentBtnState == LOW && isSelecting) {
    if (millis() - btnPressTime >= 1500) {
      isSelecting = false;
      depositCount = 0;
      beep(200);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("MODE CONFIRMED!");
      delay(1000);
      updateCountDisplay();
      Serial.println(F("Mode confirmed! Ready for bottle deposit."));
      while (digitalRead(BTN_PIN) == LOW); // wait for release
    }
  }

  lastBtnState = currentBtnState;
}