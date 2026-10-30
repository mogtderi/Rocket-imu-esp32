# Rocket IMU: ESP32 + GY-801

Attitude (roll / pitch / yaw) estimation for an experimental rocket, with a live 3D viewer on the PC.
Inspired by the "Drone Flight Controller using MPU6050 + ESP32" project, adapted for a **GY-801** module and rocket flight conditions.

![wiring](docs/wiring.svg)

> ✅ Tested on real hardware (ESP32 + GY-801). A short demo video is in `docs/demo.gif` — see below.

## Hardware
- ESP32 dev board
- GY-801 10DOF module (ADXL345, L3G4200D, HMC5883L, BMP180)
- Breadboard and 4 jumper wires (see [docs/wiring.md](docs/wiring.md))

## Quick start
1. Wire the board as shown above.
2. Upload `firmware/i2c_scanner` and check that `0x1E, 0x53, 0x69, 0x77` are found.
3. Upload `firmware/rocket_imu` (keep the rocket still for ~3 s while the gyro calibrates).
4. Run the viewer on your PC:
   ```bash
   pip install -r tools/requirements.txt
   python tools/visualizer.py COM3            # Linux: /dev/ttyUSB0
   python tools/visualizer.py COM3 --log flight.csv
   ```

Serial format (115200 baud): `roll,pitch,yaw` in degrees, about 200 lines per second. Lines starting with `#` are status messages.

## How it works
- ADXL345 set to +-16 g, L3G4200D set to 2000 dps, both at 200 Hz.
- Complementary filter: gyro integration plus accelerometer correction.
- The accelerometer is ignored when total acceleration is not near 1 g (motor burn), so the gyro carries the estimate.
- Yaw comes from gyro integration only and drifts over time.

## Roadmap
- [ ] HMC5883L magnetometer for yaw correction
- [ ] BMP180 altitude and apogee detection
- [ ] Launch detection and onboard logging (LittleFS / SD)
- [ ] Madgwick / Mahony filter
- [ ] Wireless telemetry (ESP-NOW / UDP)

## Safety
Test everything on the ground before flight. Do not use this to trigger recovery or control surfaces until it is validated, and follow your local rocketry regulations.

## License
MIT
