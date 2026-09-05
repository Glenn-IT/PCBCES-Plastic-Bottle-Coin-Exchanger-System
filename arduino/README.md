# PCBCES — Arduino Firmware & Hardware Verification Suites

This directory contains standalone, isolated bench-testing suites and the master integrated firmware for the **Plastic Bottle Coin Exchanger System (PCBCES)** reverse vending machine.

## Directory Structure
- `index.html`: Interactive master hub with Full System Circuit Schematic Diagram, Complete 20-Pin Assignment Matrix, Power Architecture, and links to all modular wiring guides.
- `01_lcd_button_menu_test/`: 16x2 I2C LCD and 3-Button dedicated interface test (Green 1.5L, Blue Mismo, Red Reset).
- `02_load_cell_hx711_test/`: [ARCHIVED / OMITTED] Former HX711 weight test (retired in active build; Pins A0/A1 reallocated to Blue and Red buttons).
- `03_ultrasonic_ir_dimension_test/`: HC-SR04 ultrasonic vertical top-down profiler (40cm chamber ceiling-to-cap distance) and IR bottle entry detector.
- `04_proximity_metal_plastic_test/`: 12V LJ12A3 inductive metal rejection and LJC18A3 capacitive dielectric plastic verification via 10k/4.7k resistor dividers.
- `05_mg996r_servo_trapdoor_test/`: MG996R metal gear sorting servo mechanism (0° Standby, 90° Accept, 180° Reject).
- `06_coin_hopper_relay_test/`: 12V Coin hopper motor, 5V relay switch, and D7 pulse interrupt payout test.
- `07_sim800l_gsm_sms_test/`: SIM800L GSM SMS bin-full dispatcher test with dedicated 4.3V buffer power rail.
- `pcbces_arduino_controller/`: Master full reverse vending machine capstone production firmware and master schematic guide.
- `Pinout_and_Schematic.md`: Technical pinout table, voltage divider formulas, and power rail reference.