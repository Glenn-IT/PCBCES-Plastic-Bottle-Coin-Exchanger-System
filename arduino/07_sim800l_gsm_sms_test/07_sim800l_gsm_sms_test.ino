/*
 * PCBCES - Test 07: SIM800L GSM Module SMS Bin-Full Notification Test
 * Hardware: Arduino Uno, SIM800L GSM/GPRS Module, 4.3V Power Rail
 * Last Updated: 2026-09-06 14:30:41 (+08:00)
 * 
 * Pin Connections:
 * - SIM800L TX -> Arduino D11 (SoftwareSerial RX)
 * - SIM800L RX -> Arduino A3  (SoftwareSerial TX)
 * - SIM800L VCC -> 4.3V Power Rail (via 1N4007 from 5V + 1000uF Cap)
 * - SIM800L GND -> Common GND Rail
 * 
 * Operation:
 * - Checks AT command responsiveness
 * - Checks signal strength (AT+CSQ) and network registration (AT+CREG?)
 * - Sends SMS alert: "ALERT: PCBCES Storage Bin is FULL! Please empty bin."
 */

#include <SoftwareSerial.h>

// SoftwareSerial pins (RX on D11, TX on A3)
SoftwareSerial gsm(11, A3);

// REPLACE WITH YOUR TEST PHONE NUMBER (Philippines format: +639XXXXXXXXX or 09XXXXXXXXX)
const char ADMIN_PHONE[] = "+639123456789";

void sendSMS(const char* number, const char* message) {
  Serial.print(F("Dispatching SMS to "));
  Serial.println(number);

  gsm.println("AT+CMGF=1"); // Text mode
  delay(500);

  gsm.print("AT+CMGS=\"");
  gsm.print(number);
  gsm.println("\"");
  delay(500);

  gsm.print(message);
  delay(500);

  gsm.write(26); // ASCII code 26 = Ctrl+Z to send message
  delay(4000); // Wait for SMS gateway response

  Serial.println(F("SMS command transmitted! Check admin phone."));
}

void setup() {
  Serial.begin(115200);
  gsm.begin(9600);

  Serial.println(F("=================================================="));
  Serial.println(F(" PCBCES Test 07: SIM800L GSM Module Tester       "));
  Serial.println(F("=================================================="));
  Serial.println(F("Checking AT communication..."));

  delay(2000);
  gsm.println("AT");
  delay(1000);
  
  while (gsm.available()) {
    Serial.write(gsm.read());
  }

  Serial.println(F("\nCommands:"));
  Serial.println(F(" 't' -> Send Test SMS Alert"));
  Serial.println(F(" 's' -> Check Signal Quality (AT+CSQ)"));
  Serial.println(F(" 'n' -> Check Network Registration (AT+CREG?)"));
}

void loop() {
  // Forward serial monitor to GSM
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 't' || c == 'T') {
      sendSMS(ADMIN_PHONE, "ALERT: PCBCES Storage Bin is FULL! Please empty the collection bin to resume operations.");
    } else if (c == 's' || c == 'S') {
      gsm.println("AT+CSQ");
    } else if (c == 'n' || c == 'N') {
      gsm.println("AT+CREG?");
    } else {
      gsm.write(c);
    }
  }

  // Forward GSM responses to Serial Monitor
  if (gsm.available()) {
    Serial.write(gsm.read());
  }
}