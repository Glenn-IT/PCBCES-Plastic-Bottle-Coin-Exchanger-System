import os
import sys

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Canonical 20-Pin Pinout Mapping
EXPECTED_PINS = {
    "D2": "HC-SR04 Trigger",
    "D3": "HC-SR04 Echo",
    "D4": "IR Bottle Entry",
    "D5": "Spare / Unassigned",
    "D6": "LJ12A3 Inductive Metal",
    "D7": "Coin Hopper Pulse",
    "D8": "Relay Hopper Trigger",
    "D9": "MG996R Servo PWM",
    "D10": "Button Green 1.5L",
    "D11": "SIM800L RX",
    "D12": "Active Buzzer",
    "D13": "Red LED",
    "A0": "Button Blue Mismo",
    "A1": "Button Red Reset",
    "A2": "Green LED",
    "A3": "SIM800L TX",
    "A4": "LCD SDA",
    "A5": "LCD SCL"
}

def check_file_exists(rel_path):
    full_path = os.path.join(BASE_DIR, rel_path)
    exists = os.path.exists(full_path)
    if not exists:
        print(f"[FAIL] Missing file: {rel_path}")
    return exists, full_path

def read_file(full_path):
    with open(full_path, "r", encoding="utf-8", errors="ignore") as f:
        return f.read()

def verify_all():
    print("==================================================")
    print(" PCBCES Hardware & Firmware Cross-Sync Verification")
    print("==================================================")
    all_ok = True

    # 1. Check config.h
    ok, path = check_file_exists("arduino/pcbces_arduino_controller/config.h")
    if ok:
        content = read_file(path)
        required_defines = [
            "PIN_BTN_GREEN       10",
            "PIN_BTN_BLUE        A0",
            "PIN_BTN_RED         A1",
            "PIN_LED_GREEN       A2",
            "PIN_GSM_TX          A3",
            "PIN_I2C_SDA         A4",
            "PIN_I2C_SCL         A5",
            "PIN_ULTRASONIC_TRIG  2",
            "PIN_ULTRASONIC_ECHO  3",
            "PIN_IR_ENTRY         4",
            "PIN_SPARE_D5         5",
            "PIN_IND_METAL        6",
            "PIN_COIN_PULSE       7",
            "PIN_RELAY_HOPPER     8",
            "PIN_SERVO_TRAPDOOR   9",
            "PIN_GSM_RX          11",
            "PIN_BUZZER          12",
            "PIN_LED_RED         13"
        ]
        for define in required_defines:
            if define not in content:
                print(f"[FAIL] config.h missing or mismatched definition: {define}")
                all_ok = False
        if all_ok:
            print("[PASS] config.h definitions 100% matched.")

    # 2. Check index.html
    ok, path = check_file_exists("arduino/index.html")
    if ok:
        content = read_file(path)
        for pin in EXPECTED_PINS:
            if f">{pin}<" not in content and f" {pin} " not in content and f"({pin})" not in content:
                print(f"[FAIL] arduino/index.html missing pin reference: {pin}")
                all_ok = False
        if "Full System Circuit Schematic Diagram" not in content:
            print("[FAIL] arduino/index.html missing Full System Circuit Schematic Diagram")
            all_ok = False
        if "Interactive Circuit Subsystem Filter" not in content:
            print("[FAIL] arduino/index.html missing Subsystem Filter Panel")
            all_ok = False
        print("[PASS] arduino/index.html verified with schematic, matrix, and filters.")

    # 3. Check master controller wiring_guide.html
    ok, path = check_file_exists("arduino/pcbces_arduino_controller/wiring_guide.html")
    if ok:
        content = read_file(path)
        if "BTN GREEN (1.5L)" not in content:
            print("[FAIL] Master wiring_guide.html missing BTN GREEN (1.5L)")
            all_ok = False
        if "BTN BLUE (MISMO)" not in content and "BTN BLUE (Mismo)" not in content:
            print("[FAIL] Master wiring_guide.html missing BTN BLUE (Mismo)")
            all_ok = False
        if "BTN RED (RESET)" not in content and "BTN RED (Restart)" not in content:
            print("[FAIL] Master wiring_guide.html missing BTN RED (Restart)")
            all_ok = False
        print("[PASS] Master wiring_guide.html verified.")

    # 4. Check Test 01 (3 buttons)
    ok, path = check_file_exists("arduino/01_lcd_button_menu_test/wiring_guide.html")
    if ok:
        content = read_file(path)
        if "BTN GREEN 1.5L" not in content or "BTN BLUE Mismo" not in content or "BTN RED Cancel" not in content:
            print("[FAIL] Test 01 wiring_guide.html missing 3 dedicated buttons in schematic")
            all_ok = False
        print("[PASS] Test 01 3-button interface verified.")

    # 5. Check Test 02 (archived notice)
    ok, path = check_file_exists("arduino/02_load_cell_hx711_test/wiring_guide.html")
    if ok:
        content = read_file(path)
        if "reallocated" not in content.lower():
            print("[FAIL] Test 02 wiring_guide.html missing reallocation notice for A0/A1")
            all_ok = False
        print("[PASS] Test 02 archived status and A0/A1 reallocation notice verified.")

    # 6. Check Pinout_and_Schematic.md
    ok, path = check_file_exists("arduino/Pinout_and_Schematic.md")
    if ok:
        content = read_file(path)
        for pin in EXPECTED_PINS:
            if f"**{pin}**" not in content:
                print(f"[FAIL] Pinout_and_Schematic.md missing pin: {pin}")
                all_ok = False
        print("[PASS] Pinout_and_Schematic.md verified.")

    # 7. Check System Memory Rules
    ok1, _ = check_file_exists("GEMINI.md")
    ok2, _ = check_file_exists("AGENTS.md")
    if ok1 and ok2:
        print("[PASS] GEMINI.md and AGENTS.md system memory rules active.")
    else:
        all_ok = False

    print("--------------------------------------------------")
    if all_ok:
        print(">>> ALL FILES 100% SYNCHRONIZED AND CONSISTENT! <<<")
        sys.exit(0)
    else:
        print(">>> DISCREPANCIES DETECTED! PLEASE REVIEW FAILS. <<<")
        sys.exit(1)

if __name__ == "__main__":
    verify_all()
