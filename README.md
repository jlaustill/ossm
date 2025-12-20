# Open Source Sensor Module

![OSSM Hardware](images/OSSMOutside.jpg)
![OSSM Hardware](images/OSSMInside.jpg)

This software is designed to emulate a second PCM/ECM and read a bunch of sensors and send the data over the appropriate J1939 PGNs. Everything in this repo is learned from reverse engineering, using data found on github, things shared on facebook, etc. So do your own testing if you decide to use it.

## Architecture Overview

OSSM uses a layered architecture with non-blocking sensor reads and runtime configuration:

```
Hardware (ADS1115 x4, MAX31856, BME280)
           |
    Data Layer (Hardware abstraction)
           |
    Domain Layer (Business logic)
           |
    Display/Interface Layer (CAN output, Serial commands)
```

### Key Features

- **Non-blocking sensor reads**: ADS1115 continuous mode with DRDY pin detection
- **50ms IntervalTimer**: Deterministic sensor polling independent of main loop
- **Runtime configuration**: All settings stored in EEPROM, no recompilation needed
- **Dual CAN bus**: Primary (Cummins) and Private CAN networks
- **Serial command interface**: Configure sensors and SPNs over USB

## File Structure

```
src/
├── Data/                          # Hardware abstraction layer
│   ├── types/
│   │   ├── ESensorType.h          # Sensor type enumeration
│   │   ├── TSensorConfig.h        # Per-sensor configuration struct
│   │   └── TAdcReading.h          # Raw ADC reading with timestamp
│   ├── ADS1115Manager/            # 4x ADS1115 ADC management
│   ├── MAX31856Manager/           # Thermocouple management
│   ├── BME280Manager/             # Ambient sensor management
│   └── ConfigStorage/             # EEPROM configuration
│
├── Domain/                        # Business logic layer
│   ├── SensorProcessor/           # Raw ADC → engineering units
│   ├── ossm.h                     # Main orchestration
│   └── ossm.cpp
│
├── Display/                       # CAN output layer
│   ├── J1939Bus.h
│   └── J1939Bus.cpp
│
└── Interface/                     # User interface layer
    └── SerialCommandHandler/      # Serial USB commands

include/
├── AppData.h                      # Runtime sensor values
└── AppConfig.h                    # EEPROM configuration structure
```

## Hardware Configuration

| Device | Address/Pin | DRDY Pin | Notes |
|--------|-------------|----------|-------|
| ADS1115 | 0x48 | D0 | 4 ADC channels |
| ADS1115 | 0x49 | D1 | 4 ADC channels |
| ADS1115 | 0x4A | D4 | 4 ADC channels |
| ADS1115 | 0x4B | D5 | 4 ADC channels |
| MAX31856 | SPI (CS=D10) | D2 (DRDY), D3 (FAULT) | K-type thermocouple |
| BME280 | I2C 0x76 | N/A | Temp, humidity, pressure |

## Serial Commands

Connect via USB serial at 115200 baud. Available commands:

| Command | Description |
|---------|-------------|
| `help` | Show all available commands |
| `status` | Display current sensor readings |
| `config` | Show current configuration |
| `save` | Save configuration to EEPROM |
| `reset` | Reset to default configuration |
| `address <n>` | Set J1939 source address (0-253) |
| `spn <name> <0\|1>` | Enable/disable a SPN |
| `sensor <idx> <type>` | Set sensor type for an input |

### SPN Names

`boost`, `egt`, `ambient`, `humidity`, `barometric`, `oiltemp`, `oilpres`,
`coolanttemp`, `coolantpres`, `fueltemp`, `fuelpres`, `enginebay`,
`intake1`, `intake2`, `intake3`, `intake4`, `airinlet`, `turbo1`, `turbo2`

### Sensor Types

| Type | ID | Description |
|------|-----|-------------|
| DISABLED | 0 | Sensor disabled |
| PRESSURE_0_100PSI | 1 | 0-100 PSI pressure sensor |
| PRESSURE_0_150PSI | 2 | 0-150 PSI pressure sensor |
| PRESSURE_0_200PSI | 3 | 0-200 PSI pressure sensor |
| TEMP_NTC_AEM | 10 | AEM NTC temperature sensor |
| TEMP_NTC_BOSCH | 11 | Bosch NTC temperature sensor |
| TEMP_NTC_GM | 12 | GM NTC temperature sensor |
| TEMP_THERMOCOUPLE_K | 20 | K-type thermocouple |
| AMBIENT_TEMP | 30 | BME280 temperature |
| AMBIENT_HUMIDITY | 31 | BME280 humidity |
| AMBIENT_PRESSURE | 32 | BME280 pressure |

## Sensors

This Sensor Module can read and provide data for up to 19 sensors.

1. Ambient Temp
2. Relative Humidity
3. Absolute Barometric Pressure
4. EGT
5. Oil Temperature
6. Oil Pressure
7. Coolant Temperature
8. Coolant Pressure
9. Transfer Pipe Temperature
10. Transfer Pipe Pressure
11. Boost Pressure
12. Boost Temperature
13. CAC Inlet Pressure
14. CAC Inlet Temperature
15. Air Inlet Temperature
16. Air Inlet Pressure
17. Fuel Pressure
18. Fuel Temperature
19. Engine Bay Temperature

## Pinout

The hardware has 48 pins on 4 plugs and is pinned out as follows.

| Pin A                   | Pin B                      | Pin C                        | Pin D                            |
| ----------------------- | -------------------------- | ---------------------------- | -------------------------------  |
| 1. Ground               | 1. Oil Temperature -       | 1. Trans Temp +              | 1. SCL for BME280                |
| 2. Fuel Pressure Signal | 2. Oil Temperature Signal  | 2. Trans Pressure +          | 2. SDA for BME280                |
| 3. Fuel Temp +          | 3. Oil Pressure Signal     | 3. Boost Pressure +          | 3. Ground for BME280             |
| 4. Engine Bay Temp +    | 4. Coolant Temperature +   | 4. Air Inlet Pressure Signal | 4. Trans Temp -                  |
| 5. Fuel Pressure +      | 5. Coolant Pressure +      | 5. Air Inlet Temp +          | 5. Trans Pressure -              |
| 6. Intake Pressure +    | 6. Oil Pressure +          | 6. CAC Temperature +         | 6. Trans Pressure Signal         |
| 7. 12 Volt Power        | 7. Coolant Pressure +      | 7. CAC Inlet Pressure Signal | 7. CANL                          |
| 8. 5 Volts for BME280   | 8. Oil Pressure -          | 8. CAC Inlet Pressure -      | 8. CANH                          |
| 9. Ground               | 9. Coolant Temperature -   | 9. CAC Inlet Temperature -   | 9. EGT -                         |
| 10. Fuel Pressure -     | 10. Coolant Pressure -     | 10. Intake Temperature -     | 10. EGT +                        |
| 11. Fuel Temp -         | 11. Boost Temperature Sig  | 11. Intake Pressure -        | 11. Boost Pressure -             |
| 12. Engine Bay Temp -   | 12. Boost Pressure Sig     | 12. CAC Inlet Pressure +     | 12. Boost Temperature -          |

## J1939

This module sends the sensor data over canbus using J1939 messages.

PGN 65269 - 1 seconds
    spn171 Ambient Air Temperature "Sensor 1"
    spn172 Air Inlet Temperature "Sensor 15"
    spn108 Barometric Pressure "Sensor 3"

PGN 65270 - .5 seconds
    spn173 Exhaust Gas Temperature "Sensor 4"
    spn102 Boost Pressure "Sensor 11"
    spn105 Intake Manifold 1 Temperature "Sensor 12" (Boost Temperature)
    spn106 Air Inlet Pressure "Sensor 16"

PGN 65262 - 1 seconds
    spn175 Engine Oil Temperature 1 "Sensor 5"
    spn110 Engine Coolant Temperature "Sensor 7"
    spn174 Fuel Temperature "Sensor 18"

PGN 65263 - .5 seconds
    spn100 Engine Oil Pressure "Sensor 6"
    spn109 Coolant Pressure "Sensor 8"
    spn94  Fuel Delivery Pressure "Sensor 17"

PGN 65129 - 1 seconds
    spn1637 Engine Coolant Temperature (High Resolution) "Sensor 7" (Coolant Temperature)
    spn1363 Intake Manifold 1 Air Temperature (High Resolution) "Sensor 12" (Boost Temperature)

PGN 65189 - 1 seconds
    spn1131 Intake Manifold 2 Temperature "Sensor 14" (CAC Inlet Temperature)
    spn1132 Intake Manifold 3 Temperature "Sensor 9" (Transfer Pipe Temperature)
    spn1133 Intake Manifold 4 Temperature "Sensor 15" (Air Inlet Temperature)

PGN 65190 - .5 seconds
    spn1127 Turbocharger 1 Boost Pressure "Sensor 13"
    spn1128 Turbocharger 2 Boost Pressure "Sensor 10"

PGN 65164 - On Request
    spn354 Relative Humidity "Sensor 2"
    spn441 Auxiliary Temperature 1 "Sensor 19" (Engine Bay Temperature)

## Building

This project uses PlatformIO. To build:

```bash
pio run
```

To upload to Teensy 4.1:

```bash
pio run -t upload
```

## Configuration

On first boot, default configuration is loaded and saved to EEPROM. Use serial commands to modify settings, then `save` to persist changes.

The configuration includes:
- J1939 source address (default: 149)
- Per-sensor type and calibration
- SPN enable/disable flags for each J1939 parameter
- ADS1115 device settings
- Thermocouple and BME280 settings
