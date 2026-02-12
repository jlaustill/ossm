# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## ALWAYS REVIEW BEFORE FILING ISSUES

**CRITICAL: NEVER file a GitHub issue without reviewing with the user first. ZERO EXCEPTIONS.**

Before filing any bug report:
1. Create the minimal reproducer
2. Verify the bug reproduces
3. **STOP and show the user** the proposed issue title and body
4. Wait for user approval before running `gh issue create`

This applies to ALL repositories, not just this one.

## NEVER DO WORKAROUNDS

**CRITICAL: NEVER implement workarounds. EVER. ZERO EXCEPTIONS.**

We face bugs HEAD ON. When you encounter a bug, limitation, or issue in a tool, library, or transpiler:

1. **DO NOT** try to work around it
2. **DO NOT** create wrapper functions to hide the problem
3. **DO NOT** temporarily rename/move/modify files to bypass issues
4. **DO NOT** suggest "creative solutions" that mask the underlying problem
5. **DO NOT** use alternative approaches just to avoid a bug
6. **DO NOT** flatten data structures, split files, or restructure code to dodge transpiler limitations

**INSTEAD:**
- Document the bug thoroughly with minimal reproducers
- File the bug report on the appropriate issue tracker
- Wait for the fix
- Move on to other work that isn't blocked
- Ask the user if you're unsure whether something counts as a workaround

Workarounds create technical debt, hide real issues from maintainers, and waste time going in circles. If something is broken, it needs to be fixed properly - not papered over. There are no "temporary" workarounds - they become permanent debt.

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
├── main.cpp                  # Arduino entry point (delegates to Ossm scope)
├── AppConfig.cnx             # Configuration struct, constants, input types
├── Domain/
│   ├── ossm.cnx              # Main setup/loop, timer-driven sensor polling
│   ├── Hardware.cnx           # Hardware scope: pin init, hasHardware flags
│   ├── SensorProcessor.cnx   # Reads ADC/thermocouple/BME280 → SensorValues
│   ├── CommandHandler.cnx    # Unified command processing (EValueId dispatch)
│   └── SerialCommandHandler.cnx  # Serial transport, forwards to CommandHandler
├── Display/
│   ├── J1939Bus.cnx          # CAN bus init, data-driven sendPgnGeneric()
│   ├── J1939Encode.cnx       # J1939 byte encoding per SPN
│   └── ...                   # Crc32, ValueName, HardwareMap, etc.
└── Data/
    ├── ConfigStorage.cnx     # EEPROM load/save/validate (owns fallback-to-defaults)
    ├── SensorValues.cnx      # Central sensor readings store
    ├── ADS1115Manager.cnx    # ADS1115 ADC state machine
    ├── MAX31856Manager.cnx   # Thermocouple driver
    ├── BME280Manager.cnx     # Ambient conditions driver
    └── types/                # TAdcReading, EValueId, TPgnConfig, etc.
```

### Key Patterns

- **Sensor Configuration**: Inputs configured via `AppConfig` with `EValueId` assignments. Only assigned inputs are read and transmitted.
- **CAN Bus**: Uses CAN1 (pins D22/D23) via FlexCAN_T4 for J1939 transmission at 250kbps.
- **Timed Transmission**: Sensor data sent at fixed intervals (500ms for pressure PGNs, 1000ms for temperature PGNs).
- **J1939 Protocol**: All sensor data encoded per SAE J1939 SPNs with appropriate scaling/offsets.

### Hardware Dependencies

- **Teensy 4.0** with CAN transceiver (CAN1 on D22/D23)
- **ADS1115** ADCs (I2C addresses 0x4B, etc.) for analog sensor inputs
- **BME280** for ambient conditions
- **MAX31856** for thermocouple (EGT)

## C-Next Transpiler Workflow

### Include Paths
- Use angle brackets for includes: `#include <AppConfig.cnx>` not `"../../AppConfig.cnx"`
- Transpiler resolves paths relative to `src/` directory
- Include files in subdirectories use relative paths from file location: `#include "types/TAdcReading.cnx"`

### Output Format
- C-Next generates `.cpp` when C++ is in use, `cnext.config.json` has `cppRequired: true`, or `--cpp` flag passed; otherwise `.c`
- Use `cnext --clean` to remove stale generated files when output format changes

### Bug Reproducers
- Create minimal reproducers in `~/code/c-next2/bugs/issue-<name>/` before filing
- Include both the `.cnx` source and generated output showing the bug
