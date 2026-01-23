# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## NEVER DO WORKAROUNDS

**CRITICAL: NEVER implement workarounds. EVER.**

When you encounter a bug, limitation, or issue in a tool, library, or transpiler:

1. **DO NOT** try to work around it
2. **DO NOT** create wrapper functions to hide the problem
3. **DO NOT** temporarily rename/move/modify files to bypass issues
4. **DO NOT** suggest "creative solutions" that mask the underlying problem

**INSTEAD:**
- Document the bug thoroughly with minimal reproducers
- File the bug report on the appropriate issue tracker
- Wait for the fix
- Ask the user if you're unsure whether something counts as a workaround

Workarounds create technical debt, hide real issues from maintainers, and waste time going in circles. If something is broken, it needs to be fixed properly - not papered over.

## Project Overview

Open Source Sensor Module (OSSM) - An embedded firmware for Teensy 4.1 that reads automotive sensors and transmits data over CAN bus using J1939 protocol. The module emulates a secondary PCM/ECM and supports up to 19 sensors (temperatures, pressures, humidity, etc.).

## J1939 Standards Compliance

**IMPORTANT**: All J1939 protocol implementation MUST follow the SAE J1939 standard exactly. Do not make assumptions about byte order, scaling factors, offsets, or message formats. Key rules:

- **Byte order**: J1939 uses little-endian (LSB first) for multi-byte values
- **Scaling/offsets**: Use the exact values defined in J1939-71 for each SPN
- **PGN formats**: Follow the standard byte positions for each Parameter Group

If you are unsure about any J1939 specification detail (byte order, scaling, offset, PGN format, etc.), **ask the user** rather than guessing. Deviating from the standard will cause interoperability issues with other J1939 devices on the bus.

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
- **CAN Bus**: OSSM v0.0.2 uses CAN1 (pins D22/D23) via FlexCAN_T4 for J1939 transmission at 250kbps.
- **Timed Transmission**: Sensor data sent at fixed intervals (500ms for pressure PGNs, 1000ms for temperature PGNs).
- **J1939 Protocol**: All sensor data encoded per SAE J1939 SPNs with appropriate scaling/offsets.

### Hardware Dependencies

- **Teensy 4.0** with CAN transceiver (CAN1 on D22/D23)
- **ADS1115** ADCs (I2C addresses 0x4B, etc.) for analog sensor inputs
- **BME280** for ambient conditions
- **MAX31856** for thermocouple (EGT)
