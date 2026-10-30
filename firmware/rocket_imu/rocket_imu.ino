// Rocket attitude estimation: ESP32 + GY-801 (ADXL345 accel + L3G4200D gyro)
// Output over Serial @115200: "roll,pitch,yaw" in degrees, ~200 Hz
// Wiring: SDA -> GPIO21, SCL -> GPIO22
#include <Wire.h>

// ---------- Sensor addresses ----------
const uint8_t ADXL345 = 0x53;
const uint8_t L3G4200D = 0x69;

// ---------- Settings ----------
const float ACC_SCALE  = 0.0039f;   // g/LSB  (ADXL345 full-resolution, +-16 g)
const float GYRO_SCALE = 0.070f;    // dps/LSB (L3G4200D, 2000 dps)
const int   CAL_SAMPLES = 500;      // gyro calibration samples (keep rocket still!)
const float ALPHA = 0.98f;          // complementary filter gyro weight

float gBias[3] = {0, 0, 0};
float roll = 0, pitch = 0, yaw = 0;
unsigned long tPrev;

void writeReg(uint8_t dev, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(dev);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

// Read 6 bytes (X,Y,Z little-endian) starting at reg
bool read6(uint8_t dev, uint8_t reg, int16_t *out) {
  Wire.beginTransmission(dev);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(dev, (uint8_t)6) != 6) return false;
  for (int i = 0; i < 3; i++) {
    uint8_t lo = Wire.read();
    uint8_t hi = Wire.read();
    out[i] = (int16_t)(hi << 8 | lo);
  }
  return true;
}

bool readSensors(float *a, float *g) {
  int16_t ra[3], rg[3];
  if (!read6(ADXL345, 0x32, ra)) return false;
  if (!read6(L3G4200D, 0x28 | 0x80, rg)) return false;   // 0x80 = auto-increment
  for (int i = 0; i < 3; i++) {
    a[i] = ra[i] * ACC_SCALE;
    g[i] = rg[i] * GYRO_SCALE;
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  Wire.setClock(400000);

  // ADXL345: measure mode, full resolution +-16 g, 200 Hz
  writeReg(ADXL345, 0x2D, 0x08);
  writeReg(ADXL345, 0x31, 0x0B);
  writeReg(ADXL345, 0x2C, 0x0B);

  // L3G4200D: 200 Hz, normal mode, XYZ on, BDU on, 2000 dps
  writeReg(L3G4200D, 0x20, 0x4F);
  writeReg(L3G4200D, 0x23, 0xA0);

  delay(100);

  Wire.beginTransmission(L3G4200D);
  Wire.write(0x0F);
  Wire.endTransmission(false);
  Wire.requestFrom(L3G4200D, (uint8_t)1);
  if (Wire.read() != 0xD3) Serial.println("# WARNING: L3G4200D WHO_AM_I mismatch");

  // Gyro calibration
  Serial.println("# Calibrating gyro, keep still...");
  float a[3], g[3];
  int n = 0;
  for (int i = 0; i < CAL_SAMPLES; i++) {
    if (readSensors(a, g)) {
      for (int k = 0; k < 3; k++) gBias[k] += g[k];
      n++;
    }
    delay(5);
  }
  if (n > 0) for (int k = 0; k < 3; k++) gBias[k] /= n;

  // Initial attitude from accelerometer
  if (readSensors(a, g)) {
    roll  = atan2(a[1], a[2]) * RAD_TO_DEG;
    pitch = atan2(-a[0], sqrt(a[1]*a[1] + a[2]*a[2])) * RAD_TO_DEG;
  }
  Serial.println("# Ready");
  tPrev = micros();
}

void loop() {
  float a[3], g[3];
  if (!readSensors(a, g)) { delay(5); return; }
  for (int k = 0; k < 3; k++) g[k] -= gBias[k];

  unsigned long now = micros();
  float dt = (now - tPrev) / 1e6f;
  tPrev = now;

  float accRoll  = atan2(a[1], a[2]) * RAD_TO_DEG;
  float accPitch = atan2(-a[0], sqrt(a[1]*a[1] + a[2]*a[2])) * RAD_TO_DEG;

  // Trust the accelerometer only when total acceleration is close to 1 g
  // (during motor burn it is not, so the gyro takes over).
  float aMag = sqrt(a[0]*a[0] + a[1]*a[1] + a[2]*a[2]);
  float alpha = (aMag > 0.85f && aMag < 1.15f) ? ALPHA : 1.0f;

  roll  = alpha * (roll  + g[0] * dt) + (1 - alpha) * accRoll;
  pitch = alpha * (pitch + g[1] * dt) + (1 - alpha) * accPitch;
  yaw  += g[2] * dt;   // drifts: no magnetometer fusion yet

  Serial.printf("%.2f,%.2f,%.2f\n", roll, pitch, yaw);
  delay(5);
}
