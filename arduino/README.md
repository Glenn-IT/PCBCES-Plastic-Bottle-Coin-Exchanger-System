# PCBCES — Arduino Firmware & Hardware Verification Suites

This directory contains standalone, isolated bench-testing suites and the master integrated firmware for the **Plastic Bottle Coin Exchanger System (PCBCES)** reverse vending machine.

## Directory Structure
- `index.html`: Interactive master hub linking all wiring guides.
- `01_lcd_button_menu_test/`: 16x2 I2C LCD and push button smart menu test.
- `02_load_cell_hx711_test/`: [ARCHIVED / OMITTED] Former HX711 weight test (retired in active build).
- `03_ultrasonic_ir_dimension_test/`: HC-SR04 ultrasonic and IR entry bottle classifier test.
- `04_proximity_metal_plastic_test/`: 12V Inductive and Capacitive proximity sensors test.
- `05_mg996r_servo_trapdoor_test/`: MG996R metal gear sorting servo mechanism test.
- `06_coin_hopper_relay_test/`: 12V Coin hopper motor and ₱3 payout test.
- `07_sim800l_gsm_sms_test/`: SIM800L GSM SMS bin-full dispatcher test.
- `pcbces_arduino_controller/`: Master full capstone production firmware.
- `Pinout_and_Schematic.md`: Technical pinout table and power rail reference.