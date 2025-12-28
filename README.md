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
│   ├── J1939Bus.h                 # J1939 message transmission
│   └── J1939Bus.cpp               # Includes PGN 65280 command handler
│
└── Interface/                     # User interface layer
    └── SerialCommandHandler/      # Serial USB commands (byte format)

include/
├── AppData.h                      # Runtime sensor values
└── AppConfig.h                    # Input-based configuration structure
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

## Input Model

OSSM uses a simple input-based configuration. You don't need to know about ADCs or hardware details.

| Input Type | Count | Description |
|------------|-------|-------------|
| **temp1-temp8** | 8 | NTC temperature sensor inputs |
| **pres1-pres7** | 7 | Pressure sensor inputs (0.5-4.5V) |
| **EGT** | 1 | Thermocouple input (MAX31856) |
| **BME280** | 1 | Ambient temp, humidity, barometric pressure |

On first boot, **all inputs are disabled**. Use serial commands to enable SPNs and assign them to inputs.

## Quick Start

1. Connect via USB serial at 115200 baud
2. Enable an SPN on an input (SPNs sent as highByte,lowByte):
   ```
   1,0,175,1,3    # Enable SPN 175 (0x00AF oil temp) on temp3
   1,0,100,1,1    # Enable SPN 100 (0x0064 oil pressure) on pres1
   1,0,173,1      # Enable SPN 173 (0x00AD EGT)
   1,0,171,1      # Enable BME280 (SPNs 171, 108, 354)
   ```
3. Save configuration: `6`
4. Query enabled SPNs: `5,0`

## Serial Commands

Connect via USB serial at 115200 baud. Commands use **byte format** - all values are 0-255, and 16-bit values (SPNs, PSI) are sent as highByte,lowByte.

| Cmd | Name | Format | Description |
|-----|------|--------|-------------|
| 1 | Enable/Disable SPN | `1,spnHi,spnLo,enable,input?` | Enable (1) or disable (0) an SPN |
| 3 | Set Pressure Range | `3,input,psiHi,psiLo` | Set pressure sensor range |
| 4 | Set TC Type | `4,type` | Set thermocouple type (0-7) |
| 5 | Query Config | `5,query_type` | Query configuration |
| 6 | Save | `6` | Save configuration to EEPROM |
| 7 | Reset | `7` | Reset to factory defaults |
| 8 | NTC Preset | `8,input,preset` | Apply NTC sensor preset |
| 9 | Pressure Preset | `9,input,preset` | Apply pressure sensor preset |
| 10 | Read Sensors | `10,type` | Read live sensor values |

> **Note:** Command 2 (Set NTC Param) was removed. Use Command 8 (NTC Preset) instead.

### Command 1: Enable/Disable SPN

```
1,spnHi,spnLo,enable,input
```

| Parameter | Description |
|-----------|-------------|
| spnHi | SPN high byte (SPN >> 8) |
| spnLo | SPN low byte (SPN & 0xFF) |
| enable | 1 = enable, 0 = disable |
| input | 1-8 for temp SPNs, 1-7 for pressure SPNs (not needed for EGT/BME280) |

**Examples:**
```
1,0,175,1,3   # Enable SPN 175 (0x00AF oil temp) on temp3
1,0,175,0     # Disable SPN 175
1,0,173,1     # Enable SPN 173 (0x00AD EGT) - no input needed
1,0,171,1     # Enable BME280 (enables SPNs 171, 108, 354)
1,0,102,1,6   # Enable SPN 102 (0x0066 boost) on pres6
```

### Command 3: Set Pressure Range

```
3,input,psiHi,psiLo
```

| Parameter | Description |
|-----------|-------------|
| input | Pressure input 1-7 |
| psiHi | Max PSI high byte (PSI >> 8) |
| psiLo | Max PSI low byte (PSI & 0xFF) |

**Example:** Set pres1 to 150 PSI (0x0096):
```
3,1,0,150     # 150 PSI = 0x0096
```

### Command 5: Query Configuration

| Query Type | Returns |
|------------|---------|
| 0 | All enabled SPNs with input mappings |
| 4 | Full configuration dump |

### Command 8: NTC Presets

| Preset | Sensor |
|--------|--------|
| 0 | AEM 30-2012 |
| 1 | Bosch |
| 2 | GM |

**Example:** `8,3,0` - Set temp3 to AEM preset

### Command 9: Pressure Presets

| Preset | Max PSI |
|--------|---------|
| 0 | 100 PSI |
| 1 | 150 PSI |
| 2 | 200 PSI |

**Example:** `9,6,1` - Set pres6 to 150 PSI

### Command 10: Read Live Sensors

```
10[,sensorType]
```

| Type | Description |
|------|-------------|
| 0 | All active sensors (default) |
| 1 | EGT only |
| 2 | Temperature sensors |
| 3 | Pressure sensors |
| 4 | BME280 ambient |

**Examples:**
```
10        # Read all active sensors
10,1      # Read EGT temperature
10,2      # Read all temperature sensors
10,3      # Read all pressure sensors
10,4      # Read BME280 (ambient temp, humidity, barometric)
```

**Note:** Any sensor faults are displayed at the end of ALL command responses.

### Thermocouple Types (Command 4)

| Type | Thermocouple |
|------|--------------|
| 0 | B-type |
| 1 | E-type |
| 2 | J-type |
| 3 | K-type (default) |
| 4 | N-type |
| 5 | R-type |
| 6 | S-type |
| 7 | T-type |

## SPN Reference

### Temperature SPNs (assign to temp1-temp8)

| SPN | Description | Notes |
|-----|-------------|-------|
| 175 | Engine Oil Temperature | |
| 110 | Engine Coolant Temperature | Auto-enables SPN 1637 (hi-res) |
| 174 | Fuel Temperature | |
| 105 | Intake Manifold 1 Temperature | Auto-enables SPN 1363 (hi-res) |
| 1131 | Intake Manifold 2 Temperature | |
| 1132 | Intake Manifold 3 Temperature | |
| 1133 | Intake Manifold 4 Temperature | |
| 172 | Air Inlet Temperature | |
| 441 | Auxiliary Temperature 1 | Engine bay temp |

### Pressure SPNs (assign to pres1-pres7)

| SPN | Description |
|-----|-------------|
| 100 | Engine Oil Pressure |
| 109 | Coolant Pressure |
| 94 | Fuel Delivery Pressure |
| 102 | Boost Pressure |
| 106 | Air Inlet Pressure |
| 1127 | Turbocharger 1 Boost Pressure |
| 1128 | Turbocharger 2 Boost Pressure |

### EGT SPN (no input assignment)

| SPN | Description |
|-----|-------------|
| 173 | Exhaust Gas Temperature |

### BME280 SPNs (grouped, no input assignment)

Enabling any one enables all three:

| SPN | Description |
|-----|-------------|
| 171 | Ambient Air Temperature |
| 108 | Barometric Pressure |
| 354 | Relative Humidity |

## Pinout

The hardware has 48 pins on 4 plugs and is pinned out as follows.

| Pin A                    | Pin B                    | Pin C                     | Pin D                  |
| ------------------------ | ------------------------ | ------------------------- | ---------------------- |
| 1. Ground                | 1. temp3 -               | 1. temp5 +                | 1. SCL for BME280      |
| 2. pres2 Signal          | 2. temp3 Signal          | 2. pres3 +                | 2. SDA for BME280      |
| 3. temp2 +               | 3. pres1 Signal          | 3. pres6 +                | 3. Ground for BME280   |
| 4. temp1 +               | 4. temp4 +               | 4. pres4 Signal           | 4. temp5 -             |
| 5. pres2 +               | 5. pres5 +               | 5. temp7 +                | 5. pres3 -             |
| 6. pres7 +               | 6. pres1 +               | 6. temp6 +                | 6. pres3 Signal        |
| 7. 12 Volt Power         | 7. pres5 +               | 7. pres4 Signal           | 7. CANL                |
| 8. 5 Volts for BME280    | 8. pres1 -               | 8. pres4 -                | 8. CANH                |
| 9. Ground                | 9. temp4 -               | 9. temp6 -                | 9. EGT -               |
| 10. pres2 -              | 10. pres5 -              | 10. temp8 -               | 10. EGT +              |
| 11. temp2 -              | 11. temp8 Signal         | 11. pres7 -               | 11. pres6 -            |
| 12. temp1 -              | 12. pres6 Signal         | 12. pres4 +               | 12. temp8 -            |

## J1939

This module sends sensor data over CAN bus using J1939 messages. Only enabled SPNs are transmitted.

| PGN | Interval | SPNs |
|-----|----------|------|
| 65269 | 1s | 171 (Ambient Temp), 172 (Air Inlet Temp), 108 (Barometric) |
| 65270 | 0.5s | 173 (EGT), 102 (Boost Pressure), 105 (Intake Temp), 106 (Air Inlet Pressure) |
| 65262 | 1s | 175 (Oil Temp), 110 (Coolant Temp), 174 (Fuel Temp) |
| 65263 | 0.5s | 100 (Oil Pressure), 109 (Coolant Pressure), 94 (Fuel Pressure) |
| 65129 | 1s | 1637 (Coolant Temp Hi-Res), 1363 (Intake Temp Hi-Res) |
| 65189 | 1s | 1131 (Intake 2 Temp), 1132 (Intake 3 Temp), 1133 (Intake 4 Temp) |
| 65190 | 0.5s | 1127 (Turbo 1 Boost), 1128 (Turbo 2 Boost) |
| 65164 | On Request | 354 (Humidity), 441 (Engine Bay Temp) |

## Building

This project uses PlatformIO. To build:

```bash
pio run
```

To upload to Teensy 4.0:

```bash
pio run -t upload
```

## Configuration

On first boot, **all sensors are disabled** and default calibration values are loaded. Use serial commands to:
1. Enable SPNs and assign them to inputs
2. Configure NTC calibration or use presets
3. Set pressure sensor ranges or use presets
4. Save configuration with command `6`

The configuration stored in EEPROM includes:
- J1939 source address (default: 149)
- Per-input SPN assignments (temp1-8, pres1-7)
- Per-input NTC calibration (Steinhart-Hart coefficients A, B, C and resistor value)
- Per-input pressure range (max PSI)
- EGT enable flag and thermocouple type
- BME280 enable flag

## J1939 Command Interface

OSSM also accepts configuration commands over CAN bus using PGN 65280 (0xFF00). Responses are sent on PGN 65281 (0xFF01).

Binary format: `[cmd, param1, param2, ...]`

This allows configuration from other J1939 devices without requiring USB serial access.
