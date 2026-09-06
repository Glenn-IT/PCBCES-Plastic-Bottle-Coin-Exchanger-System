Coin Hopper for Coin Changer / Bill changer / Coin counting machine (220V AC / 12V DC)
IR Infrared Obstacle Avoidance Sensor (Bin-Full Sensor on Pin D5: alerts admin via SMS when storage bin is full)
LJ12A34Z/BX Inductive Proximity Sensor Probe Cable NPN 3-Wire Metal Induction Photoelectric Switch Sensor LJ12A3-4-Z/BX (Pin D6)
IR Infrared Obstacle Avoidance Sensor Module (Chute Entry Sensor on Pin D4)
[REMOVED/OMITTED] Load Cell Amplifier HX711 & Straight Bar Load Cell Weight Sensor 1kg (Replaced with non-contact Ultrasonic & Inductive Proximity sensing for high durability and zero mechanical wear)
Dedicated 3-Button Control Interface (Green: 1.5L/1.75L on D10, Blue: 290 ML / Mismo on A0, Red: Cancel/Restart on A1)
Digital Robot Servo Motor (180 Rotation) – MG996R MG996 360°
DIYMORE 2315 active mechanical buzzer 6V 85dB mini electronic alarm buzzer
Ultrasonic Sensor HC-SR04+ 3.3V Compatible
Arduino Uno
Green and Red Light Bulb
16x2 LCD module Blue
LM2596 Buck Converter DC-DC Step Down Converter Power Supply Module
S-120-12 Usual 120w 12vdc 10a Single Group Switching Power Supply Ac 110v / 220v To Dc 12v

This are my hardware components now what im building is

A Plastic bottle coin exchanger system, where the people put 1.5L/1.75L bottle or a 290 ML (Mismo) bottle search here on the PH ok
1.5L / 1.75L 5 bottles is equal to 20 pesos coins (₱4.00 per bottle, 20 coins x ₱1)
290 ML 10 bottles is equal to 3 pesos coins (3 coins x ₱1)

The user selects via 3 dedicated buttons:
- Button Green (D10): Selects 1.5L / 1.75L bottle mode (5 pcs quota = 20 PHP)
- Button Blue (A0): Selects 290 ML mode (10 pcs quota = 3 PHP)
- Button Red (A1): System restart / cancel transaction at any time, resetting count and returning to start.
When a bottle is inserted, sensors verify non-metal and correct dimensions. If valid, the servo trapdoor opens 90 degrees to drop the bottle into the bin. If invalid, the buzzer alerts, red LED blinks, and LCD prompts 'Pls try again'. Once the target count is satisfied, the 12V hopper dispenses the coins (20 coins for 1.5L/1.75L, 3 coins for 290 ML).
