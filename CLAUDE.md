# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Open Source Sensor Module (OSSM) - An embedded firmware for Teensy 4.1 that reads automotive sensors and transmits data over CAN bus using J1939 protocol. The module emulates a secondary PCM/ECM and supports up to 19 sensors (temperatures, pressures, humidity, etc.).

## Build Commands

```bash
# Build the project
pio run

# Upload to Teensy
pio run --target upload

# Clean build artifacts
pio run --target clean

# Monitor serial output (115200 baud)
pio device monitor
```

## Architecture

```
src/
├── main.cpp              # Arduino entry point (setup/loop delegates to ossm class)
├── configuration.h       # Sensor enable flags (#define SPN_xxx to enable specific sensors)
├── Domain/
│   └── ossm.cpp/h        # Main application logic, sensor initialization, timed data collection
├── Display/
│   └── J1939Bus.cpp/h    # CAN bus communication, J1939 message formatting, dual-bus repeater
└── Data/
    └── AmbientSensors/   # BME280 sensor handling (humidity, barometric pressure, ambient temp)

lib/
├── PressureSensor/       # ADS1115 ADC-based pressure sensor driver
└── TempSensor/           # ADS1115 ADC-based temperature sensor driver (Steinhart-Hart thermistor)

include/
└── AppData.h             # Central data structure holding all sensor readings
```

### Key Patterns

- **Sensor Configuration**: Sensors are enabled/disabled via preprocessor defines in `configuration.h`. Only defined SPNs will be transmitted.
- **Dual CAN Bus**: Uses FlexCAN_T4 with CAN2 (Cummins bus) and CAN3 (Private bus). Messages are sniffed from Cummins bus and repeated to private bus with optional modifications.
- **Timed Transmission**: Sensor data sent at fixed intervals (500ms for pressure PGNs, 1000ms for temperature PGNs).
- **J1939 Protocol**: All sensor data encoded per SAE J1939 SPNs with appropriate scaling/offsets.

### Hardware Dependencies

- **Teensy 4.1** with dual CAN transceivers
- **ADS1115** ADCs (I2C addresses 0x4B, etc.) for analog sensor inputs
- **BME280** for ambient conditions
- **MAX31855** for thermocouple (EGT)
