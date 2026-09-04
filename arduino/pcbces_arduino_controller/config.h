#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
// PCBCES PIN CONFIGURATION MATRIX (Arduino Uno ATmega328P)
// =============================================================================

// Analog & I2C Pins
#define PIN_LOADCELL_DT     A0   // HX711 Serial Data
#define PIN_LOADCELL_SCK    A1   // HX711 Serial Clock
#define PIN_LED_GREEN       A2   // Green LED (Acceptance / Ready Indicator)
#define PIN_GSM_TX          A3   // SoftwareSerial TX (To GSM SIM800L RX)
#define PIN_I2C_SDA         A4   // 16x2 LCD I2C Data
#define PIN_I2C_SCL         A5   // 16x2 LCD I2C Clock

// Digital Pins
#define PIN_ULTRASONIC_TRIG  2   // HC-SR04 Trigger Pulse
#define PIN_ULTRASONIC_ECHO  3   // HC-SR04 Echo Return Pulse
#define PIN_IR_ENTRY         4   // IR Obstacle Avoidance Sensor (Active LOW)
#define PIN_CAP_PLASTIC      5   // LJC18A3 Capacitive Sensor (via 10k/4.7k divider)
#define PIN_IND_METAL        6   // LJ12A3 Inductive Metal Sensor (via 10k/4.7k divider)
#define PIN_COIN_PULSE       7   // 12V Coin Hopper Counter (via 10k/4.7k divider)
#define PIN_RELAY_HOPPER     8   // 5V Relay Module (Controls 12V Hopper Motor)
#define PIN_SERVO_TRAPDOOR   9   // MG996R PWM Trapdoor Flap
#define PIN_BUTTON_MENU     10   // Momentary Push Button (INPUT_PULLUP)
#define PIN_GSM_RX          11   // SoftwareSerial RX (From GSM SIM800L TX)
#define PIN_BUZZER          12   // DIYMORE Active 5V Buzzer
#define PIN_LED_RED         13   // Red LED (Rejection / Fault Indicator)

// =============================================================================
// THRESHOLDS & SPECIFICATIONS
// =============================================================================
#define BOTTLE_1_5L_QUOTA       5    // 5 bottles = 3.00 PHP
#define BOTTLE_MISMO_QUOTA      10   // 10 bottles = 3.00 PHP
#define COINS_PAYOUT_TARGET     3    // 3 x 1.00 Peso coins = 3.00 PHP
#define MAX_BIN_CAPACITY        30   // Trigger Bin-Full SMS when 30 bottles deposited

// Servo Angles
#define SERVO_STANDBY_ANGLE     0    // Flap Closed
#define SERVO_ACCEPT_ANGLE      90   // Drop into internal storage bin
#define SERVO_REJECT_ANGLE      180  // Return chute to user

// HX711 Calibration
#define LOADCELL_CAL_FACTOR     420.0f
#define WEIGHT_1_5L_MIN         38.0f // grams
#define WEIGHT_1_5L_MAX         60.0f
#define WEIGHT_MISMO_MIN        16.0f
#define WEIGHT_MISMO_MAX        32.0f
#define WEIGHT_FRAUD_LIMIT      70.0f // Bottle contains liquid or rocks

// Ultrasonic Height Ranges (cm)
#define HEIGHT_1_5L_MIN         26
#define HEIGHT_1_5L_MAX         35
#define HEIGHT_MISMO_MIN        16
#define HEIGHT_MISMO_MAX        24

// Admin SMS Recipient (Replace with actual phone number)
#define ADMIN_PHONE_NUMBER      "+639123456789"

#endif