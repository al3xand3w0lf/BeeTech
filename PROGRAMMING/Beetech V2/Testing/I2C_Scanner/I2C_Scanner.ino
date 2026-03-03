#include <Wire.h>

void setup() {
  Serial.begin(115200);
  Wire.begin(); // SDA/SCL Standardpins
  Serial.println("Scanning...");
}

void loop() {
  for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.print("Gefunden bei 0x");
      Serial.println(address, HEX);
      delay(5);
    }
  }
  Serial.println("Scan fertig.");
  delay(2000);
}
