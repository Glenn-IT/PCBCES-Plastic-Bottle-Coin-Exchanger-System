#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
// PCBCES PIN CONFIGURATION MATRIX (Arduino Uno ATmega328P)
// Last Updated: 2026-09-06 18:35:00 (+08:00)
// =============================================================================

// Dedicated 3-Button User Interface Pins (INPUT_PULLUP to GND)
#define PIN_BTN_GREEN       10   // Button Green: Select 1.5L / 1.75L Bottle Mode (5 pcs = 20 PHP)
#define PIN_BTN_BLUE        A0   // Button Blue:  Select 290 ML Bottle Mode (10 pcs = 3 PHP)
#define PIN_BTN_RED         A1   // Button Red:   System Restart / Cancel Transaction

// Analog & I2C Pins
#define PIN_LED_GREEN       A2   // Green LED (Acceptance / Ready Indicator)
#define PIN_GSM_TX          A3   // SoftwareSerial TX (To GSM SIM800L RX)
#define PIN_I2C_SDA         A4   // 16x2 LCD I2C Data
#define PIN_I2C_SCL         A5   // 16x2 LCD I2C Clock

// Digital Pins
#define PIN_ULTRASONIC_TRIG  2   // HC-SR04 Trigger Pulse (Measures bottle length)
#define PIN_ULTRASONIC_ECHO  3   // HC-SR04 Echo Return Pulse
#define PIN_IR_ENTRY         4   // IR Obstacle Avoidance Sensor (Active LOW entry trigger)
#define PIN_SPARE_D5         5   // Spare / Unassigned GPIO (LJC18A3 Capacitive Sensor omitted)
#define PIN_IND_METAL        6   // LJ12A3 Inductive Metal Sensor (Metal rejection via 10k/4.7k or 5k divider)
#define PIN_COIN_PULSE       7   // 12V Coin Hopper Counter (via 10k/4.7k or 5k divider)
#define PIN_RELAY_HOPPER     8   // 5V Relay Module (Controls 12V Hopper Motor)
#define PIN_SERVO_TRAPDOOR   9   // MG996R PWM Trapdoor Flap
#define PIN_GSM_RX          11   // SoftwareSerial RX (From GSM SIM800L TX)
#define PIN_BUZZER          12   // DIYMORE Active 5V Buzzer
#define PIN_LED_RED         13   // Red LED (Rejection / Fault Indicator)

// =============================================================================
// THRESHOLDS & SPECIFICATIONS
// =============================================================================
#define BOTTLE_1_5L_QUOTA       5    // 5 bottles = 20.00 PHP (4.00 PHP per bottle)
#define BOTTLE_290ML_QUOTA      10   // 10 bottles = 3.00 PHP (290 ML Mode)
#define COINS_PAYOUT_1_5L       20   // 20 x 1.00 Peso coins = 20.00 PHP
#define COINS_PAYOUT_290ML      3    // 3 x 1.00 Peso coins = 3.00 PHP
#define COINS_PAYOUT_TARGET     COINS_PAYOUT_290ML // Backward-compatible default (3 PHP)
#define MAX_BIN_CAPACITY        30   // Trigger Bin-Full SMS when 30 bottles deposited

// Servo Angles
#define SERVO_STANDBY_ANGLE     0    // Flap Closed (Horizontal Cradle Rest & Scanning)
#define SERVO_ACCEPT_ANGLE      90   // Drop into internal storage bin
#define SERVO_REJECT_ANGLE      0    // Flap Stays Closed at 0° (Item remains on cradle for manual removal)

// Ultrasonic Vertical Distance Thresholds (Ceiling Sensor to Bottle Cap)
// Total Chamber Height (Bottom Trapdoor to Ceiling HC-SR04): Calibrated to 43 cm
// Empty Chamber baseline echo: ~41 cm to 45 cm
// 1.5L / 1.75L Bottle (Height ~30-33 cm): Cap is NEAR sensor -> Distance: 7 cm to 15 cm
// 290 ML Bottle (Height ~16-17 cm): Cap is FAR from sensor -> Distance: 26 cm to 27 cm
#define CHAMBER_TOTAL_HEIGHT_CM 43   // Calibrated physical distance from trapdoor base to HC-SR04
#define DIST_1_5L_MIN           7    // Minimum distance from top sensor to 1.5L/1.75L cap (~36 cm max bottle height)
#define DIST_1_5L_MAX           15   // Maximum distance from top sensor to 1.5L/1.75L cap (~28 cm min bottle height)
#define DIST_290ML_MIN          26   // Minimum distance from top sensor to 290 ML cap (~17 cm max bottle height)
#define DIST_290ML_MAX          27   // Maximum distance from top sensor to 290 ML cap (~16 cm min bottle height)

// Backwards-compatible aliases
#define BOTTLE_MISMO_QUOTA      BOTTLE_290ML_QUOTA
#define DIST_MISMO_MIN          DIST_290ML_MIN
#define DIST_MISMO_MAX          DIST_290ML_MAX
#define HEIGHT_1_5L_MIN         DIST_1_5L_MIN
#define HEIGHT_1_5L_MAX         DIST_1_5L_MAX
#define HEIGHT_MISMO_MIN        DIST_MISMO_MIN
#define HEIGHT_MISMO_MAX        DIST_MISMO_MAX
#define HEIGHT_290ML_MIN        DIST_290ML_MIN
#define HEIGHT_290ML_MAX        DIST_290ML_MAX

// Admin SMS Recipient (Replace with actual phone number)
#define ADMIN_PHONE_NUMBER      "+639123456789"

#endif
