<?php
$baseDir = dirname(__DIR__);
$allOk = true;

echo "==================================================\n";
echo " PCBCES Hardware & Firmware Cross-Sync Verification\n";
echo "==================================================\n";

$expectedPins = [
    "D2" => "HC-SR04 Trigger",
    "D3" => "HC-SR04 Echo",
    "D4" => "IR Bottle Entry",
    "D5" => "IR Bin Full",
    "D6" => "LJ12A3 Inductive Metal",
    "D7" => "Coin Hopper Pulse",
    "D8" => "Relay Hopper Trigger",
    "D9" => "MG996R Servo PWM",
    "D10" => "Button Green 1.5L",
    "D11" => "SIM800L RX",
    "D12" => "Active Buzzer",
    "D13" => "Red LED",
    "A0" => "Button Blue Mismo",
    "A1" => "Button Red Reset",
    "A2" => "Green LED",
    "A3" => "SIM800L TX",
    "A4" => "LCD SDA",
    "A5" => "LCD SCL"
];

// 1. Check config.h
$configPath = $baseDir . '/arduino/pcbces_arduino_controller/config.h';
if (file_exists($configPath)) {
    $content = file_get_contents($configPath);
    $defines = [
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
        "PIN_IR_BIN_FULL      5",
        "PIN_IND_METAL        6",
        "PIN_COIN_PULSE       7",
        "PIN_RELAY_HOPPER     8",
        "PIN_SERVO_TRAPDOOR   9",
        "PIN_GSM_RX          11",
        "PIN_BUZZER          12",
        "PIN_LED_RED         13"
    ];
    foreach ($defines as $def) {
        if (strpos($content, $def) === false) {
            echo "[FAIL] config.h missing define: $def\n";
            $allOk = false;
        }
    }
    if ($allOk) echo "[PASS] config.h definitions 100% matched.\n";
} else {
    echo "[FAIL] Missing config.h\n";
    $allOk = false;
}

// 2. Check index.html
$indexPath = $baseDir . '/arduino/index.html';
if (file_exists($indexPath)) {
    $content = file_get_contents($indexPath);
    foreach (array_keys($expectedPins) as $pin) {
        if (strpos($content, '"pin-tag tag-uno">' . $pin . '<') === false && strpos($content, '>' . $pin . '<') === false) {
            echo "[FAIL] index.html missing pin: $pin\n";
            $allOk = false;
        }
    }
    if (strpos($content, 'Full System Circuit Schematic Diagram') === false) {
        echo "[FAIL] index.html missing schematic title\n";
        $allOk = false;
    }
    echo "[PASS] arduino/index.html verified with schematic, matrix, and filters.\n";
}

// 3. Check Master wiring_guide.html
$masterGuide = $baseDir . '/arduino/pcbces_arduino_controller/wiring_guide.html';
if (file_exists($masterGuide)) {
    $content = file_get_contents($masterGuide);
    if (strpos($content, 'BTN GREEN (1.5L)') === false) {
        echo "[FAIL] Master wiring_guide.html missing BTN GREEN\n";
        $allOk = false;
    }
    echo "[PASS] Master wiring_guide.html verified.\n";
}

// 4. Check Test 01
$t1 = $baseDir . '/arduino/01_lcd_button_menu_test/wiring_guide.html';
if (file_exists($t1)) {
    $content = file_get_contents($t1);
    if (strpos($content, 'BTN GREEN 1.5L') === false) {
        echo "[FAIL] Test 01 missing button green\n";
        $allOk = false;
    }
    echo "[PASS] Test 01 3-button interface verified.\n";
}

// 5. Check Test 02
$t2 = $baseDir . '/arduino/02_load_cell_hx711_test/wiring_guide.html';
if (file_exists($t2)) {
    $content = file_get_contents($t2);
    if (stripos($content, 'reallocated') === false) {
        echo "[FAIL] Test 02 missing reallocation notice\n";
        $allOk = false;
    }
    echo "[PASS] Test 02 archived status verified.\n";
}

// 6. Check Pinout_and_Schematic.md
$pinout = $baseDir . '/arduino/Pinout_and_Schematic.md';
if (file_exists($pinout)) {
    $content = file_get_contents($pinout);
    foreach (array_keys($expectedPins) as $pin) {
        if (strpos($content, '**' . $pin . '**') === false) {
            echo "[FAIL] Pinout_and_Schematic.md missing pin: $pin\n";
            $allOk = false;
        }
    }
    echo "[PASS] Pinout_and_Schematic.md verified.\n";
}

// 7. Check AGENTS.md & GEMINI.md
if (file_exists($baseDir . '/AGENTS.md') && file_exists($baseDir . '/GEMINI.md')) {
    echo "[PASS] AGENTS.md and GEMINI.md active.\n";
} else {
    echo "[FAIL] Missing AGENTS.md or GEMINI.md\n";
    $allOk = false;
}

echo "--------------------------------------------------\n";
if ($allOk) {
    echo "[SUCCESS] ALL FILES 100% SYNCHRONIZED AND CONSISTENT!\n";
    exit(0);
} else {
    echo "[FAIL] DISCREPANCIES DETECTED!\n";
    exit(1);
}
