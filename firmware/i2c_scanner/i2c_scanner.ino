// I2C scanner for ESP32 (SDA = GPIO21, SCL = GPIO22)
// Expected on a GY-801: 0x1E (HMC5883L), 0x53 (ADXL345), 0x69 (L3G4200D), 0x77 (BMP085/BMP180)
#include <Wire.h>

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  delay(500);
  Serial.println("Scanning I2C bus...");
  int found = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("Found device at 0x%02X\n", addr);
      found++;
    }
  }
  Serial.printf("Done. %d device(s) found.\n", found);
}

void loop() {}
