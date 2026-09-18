/*
 * PCBCES - Test 07: GSM Module SMS Bin-Full Notification Test
 * Hardware: Arduino Uno, SIMCom SIM900A (S2-1040U-Z1K0H) / SIM800L GSM/GPRS Module, 5V 2A Power Rail
 * Last Updated: 2026-09-18 22:20:00 (+08:00)
 * 
 * Pin Connections (SIM900A Mini V3.9.2 / V4.0):
 * - SIM900A 5VT (Module TX) -> Arduino Uno D11 (SoftwareSerial RX)
 * - SIM900A 5VR (Module RX) -> Arduino Uno A3  (SoftwareSerial TX)
 * - SIM900A VCC (5V / VCC5) -> LM2596 5.0V Buck Output (2A Peak, with 1000uF Buffer Cap)
 * - SIM900A GND             -> Common System Ground Rail (Arduino GND + LM2596 GND)
 * 
 * Note for SIM800L Users (Alternative Module):
 * - SIM800L TX -> Arduino D11, SIM800L RX -> Arduino A3, VCC -> 4.3V Diode Drop Rail
 * 
 * Operation:
 * - Automatically performs auto-baud rate synchronization on startup (9600 baud)
 * - Checks AT command communication and SIM card detection (AT+CPIN?)
 * - Checks signal strength (AT+CSQ) and network registration (AT+CREG?)
 * - Sends SMS alert: "ALERT: PCBCES Storage Bin is FULL! Please empty bin."
 * - Interactive passthrough mode: Type any AT command into Serial Monitor!
 */

#include <SoftwareSerial.h>

// SoftwareSerial pins (RX on D11, TX on A3)
// D11 connects to SIM900A 5VT pin
// A3 connects to SIM900A 5VR pin
SoftwareSerial gsm(11, A3);

// Target Administrator Phone Numbers (Philippines format: +639XXXXXXXXX or 09XXXXXXXXX)
const char ADMIN_PHONE_1[] = "+639634299114";
const char ADMIN_PHONE_2[] = "+639242074903";
const char* const ADMIN_PHONES[] = { ADMIN_PHONE_1, ADMIN_PHONE_2 };
const int NUM_ADMIN_PHONES = 2;

void sendSMS(const char* number, const char* message) {
  Serial.println(F("\n--------------------------------------------------"));
  Serial.print(F("[SMS] Dispatching Bin-Full SMS to: "));
  Serial.println(number);
  Serial.println(F("--------------------------------------------------"));

  // Set SMS mode to text format (AT+CMGF=1)
  gsm.println("AT+CMGF=1");
  delay(500);

  // Set recipient phone number
  gsm.print("AT+CMGS=\"");
  gsm.print(number);
  gsm.println("\"");
  delay(500);

  // Send message body
  gsm.print(message);
  delay(500);

  // Send Ctrl+Z (ASCII 26) to tell module to transmit SMS
  gsm.write(26);
  Serial.println(F("[SMS] Waiting for cellular network confirmation..."));
  
  // Wait up to 6 seconds for network acknowledge
  unsigned long startWait = millis();
  while (millis() - startWait < 6000) {
    while (gsm.available()) {
      char c = gsm.read();
      Serial.write(c);
    }
  }

  Serial.println(F("\n[SMS] Transmission command complete. Check phone for SMS!"));
}

void sendAllAdmins(const char* message) {
  Serial.println(F("\n=================================================="));
  Serial.println(F(" [BROADCAST] Dispatching SMS to All Admin Phones  "));
  Serial.println(F("=================================================="));
  for (int i = 0; i < NUM_ADMIN_PHONES; i++) {
    sendSMS(ADMIN_PHONES[i], message);
    if (i < NUM_ADMIN_PHONES - 1) {
      Serial.println(F("[BROADCAST] Waiting 2.5s buffer before next transmission..."));
      delay(2500);
    }
  }
  Serial.println(F("=================================================="));
  Serial.println(F(" [BROADCAST] All transmissions completed!         "));
  Serial.println(F("=================================================="));
}

void printGSMResponse(unsigned long timeoutMs = 1500) {
  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    while (gsm.available()) {
      char c = gsm.read();
      // Print readable ASCII characters, newline and carriage return
      if ((c >= 32 && c <= 126) || c == '\r' || c == '\n') {
        Serial.write(c);
      } else if ((uint8_t)c == 0xFF || (uint8_t)c == 0x00) {
        // Suppress pure framing noise
      } else {
        Serial.write(c);
      }
    }
  }
}

// Probes common GSM baud rates, finds active rate, and locks module to 9600 baud
bool autoSyncGSMBaud() {
  const long candidateBauds[] = { 9600, 115200, 38400, 19200, 57600, 4800 };
  const int count = sizeof(candidateBauds) / sizeof(candidateBauds[0]);

  Serial.println(F("[GSM] Probing communication baud rates..."));

  for (int i = 0; i < count; i++) {
    long testBaud = candidateBauds[i];
    Serial.print(F("  -> Testing "));
    Serial.print(testBaud);
    Serial.print(F(" baud... "));

    gsm.begin(testBaud);
    delay(150);
    while (gsm.available()) gsm.read();

    bool detected = false;
    for (int attempt = 0; attempt < 3; attempt++) {
      gsm.print("AT\r\n");
      unsigned long start = millis();
      String resp = "";
      while (millis() - start < 450) {
        while (gsm.available()) {
          char c = gsm.read();
          resp += c;
          if (resp.indexOf("OK") != -1) {
            detected = true;
            break;
          }
        }
        if (detected) break;
      }
      if (detected) break;
      delay(150);
    }

    if (detected) {
      Serial.println(F("[LOCKED! Handshake OK]"));
      if (testBaud != 9600) {
        Serial.println(F("  -> Reconfiguring module to 9600 baud for stable SoftwareSerial..."));
        gsm.print("AT+IPR=9600\r\n");
        delay(250);
        gsm.print("AT&W\r\n");
        delay(250);
        gsm.begin(9600);
        delay(150);
        while (gsm.available()) gsm.read();
        Serial.println(F("  -> Module baud locked to 9600 permanently!"));
      }
      return true;
    } else {
      Serial.println(F("[No reply]"));
    }
  }

  // Fallback: Default to 9600 and train autobaud
  Serial.println(F("[WARN] No standard response. Training auto-baud at 9600 baud..."));
  gsm.begin(9600);
  delay(200);
  for (int i = 0; i < 5; i++) {
    gsm.print("AT\r\n");
    delay(300);
  }
  while (gsm.available()) gsm.read();
  return false;
}

void setup() {
  Serial.begin(115200);

  Serial.println(F("=================================================="));
  Serial.println(F(" PCBCES Test 07: SIM900A / SIM800L GSM Tester    "));
  Serial.println(F(" Module: SIMCom SIM900A (S2-1040U-Z1K0H)         "));
  Serial.println(F(" Pinout: 5VT -> Uno D11 (RX) | 5VR -> Uno A3 (TX)"));
  Serial.println(F(" Power : 5.0V from LM2596 Buck (2A Burst Capable) "));
  Serial.println(F(" Phone 1: +639634299114                           "));
  Serial.println(F(" Phone 2: +639242074903                           "));
  Serial.println(F("=================================================="));
  Serial.println(F("[INFO] Initializing communication with GSM module..."));

  // Give GSM module 2.5 seconds to settle internal baseband
  delay(2500);

  // Probe and synchronize baud rate
  bool connected = autoSyncGSMBaud();

  if (!connected) {
    Serial.println(F("\n[CHECKLIST] Module did not respond with 'OK':"));
    Serial.println(F(" 1. Check Power: VCC must be 5.0V (2A burst capable)."));
    Serial.println(F(" 2. Check 1000uF capacitor across VCC and GND."));
    Serial.println(F(" 3. Check Ground: Common GND between LM2596 and Uno GND."));
    Serial.println(F(" 4. Check Pins: 5VT -> Uno D11 (RX), 5VR -> Uno A3 (TX)."));
    Serial.println(F(" 5. Check NET LED on SIM900A (Fast blink = searching)."));
  }

  // Test AT communication
  Serial.println(F("\n[TEST 1] Handshake (AT)..."));
  gsm.println("AT");
  printGSMResponse(1000);

  // Test SIM Card Ready
  Serial.println(F("\n[TEST 2] Checking SIM Card (AT+CPIN?)..."));
  gsm.println("AT+CPIN?");
  printGSMResponse(1200);

  // Test Signal Quality
  Serial.println(F("\n[TEST 3] Checking Cellular Signal (AT+CSQ)..."));
  gsm.println("AT+CSQ");
  printGSMResponse(1200);

  // Test Network Registration
  Serial.println(F("\n[TEST 4] Network Registration Status (AT+CREG?)..."));
  gsm.println("AT+CREG?");
  printGSMResponse(1200);

  Serial.println(F("\n=================================================="));
  Serial.println(F(" READY! Available Commands (type in Serial Monitor):"));
  Serial.println(F("  't' -> Send Test SMS to BOTH Phones (1 & 2)     "));
  Serial.println(F("  '1' -> Send Test SMS to Phone 1 (+639634299114) "));
  Serial.println(F("  '2' -> Send Test SMS to Phone 2 (+639242074903) "));
  Serial.println(F("  's' -> Query Signal Strength (AT+CSQ)           "));
  Serial.println(F("  'n' -> Query Network Registration (AT+CREG?)    "));
  Serial.println(F("  'c' -> Check SIM PIN Status (AT+CPIN?)          "));
  Serial.println(F("  'o' -> Check Network Operator / Carrier (AT+COPS?)"));
  Serial.println(F("  Type any custom AT command directly (e.g. ATI)  "));
  Serial.println(F("=================================================="));
}

void loop() {
  // Forward commands from Serial Monitor to GSM module
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 't' || c == 'T') {
      sendAllAdmins("ALERT: PCBCES Storage Bin is FULL! Please empty the collection bin to resume bottle deposits.");
    } else if (c == '1') {
      sendSMS(ADMIN_PHONE_1, "ALERT: PCBCES Storage Bin is FULL! (Admin Phone 1 Test: 09634299114). Please empty bin.");
    } else if (c == '2') {
      sendSMS(ADMIN_PHONE_2, "ALERT: PCBCES Storage Bin is FULL! (Admin Phone 2 Test: 09242074903). Please empty bin.");
    } else if (c == 's' || c == 'S') {
      Serial.println(F("\n>> AT+CSQ (Signal Quality)"));
      gsm.println("AT+CSQ");
    } else if (c == 'n' || c == 'N') {
      Serial.println(F("\n>> AT+CREG? (Network Registration)"));
      gsm.println("AT+CREG?");
    } else if (c == 'c' || c == 'C') {
      Serial.println(F("\n>> AT+CPIN? (SIM Card Status)"));
      gsm.println("AT+CPIN?");
    } else if (c == 'o' || c == 'O') {
      Serial.println(F("\n>> AT+COPS? (Cellular Operator)"));
      gsm.println("AT+COPS?");
    } else {
      // Direct raw passthrough
      gsm.write(c);
    }
  }

  // Forward GSM module responses to Serial Monitor
  if (gsm.available()) {
    Serial.write(gsm.read());
  }
}