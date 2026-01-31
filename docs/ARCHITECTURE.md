# OSSM Architecture

Technical documentation of OSSM's layered architecture and design decisions.

## Overview

OSSM uses a three-layer architecture that separates hardware abstraction from business logic and output formatting:

```
┌─────────────────────────────────────────────────────────┐
│                    Application Layer                     │
│  main.cnx → Ossm scope (setup/loop orchestration)       │
└─────────────────────────────────────────────────────────┘
                           │
        ┌──────────────────┼──────────────────┐
        ▼                  ▼                  ▼
┌───────────────┐  ┌───────────────┐  ┌───────────────┐
│  Domain Layer │  │ Display Layer │  │Interface Layer│
│               │  │               │  │               │
│ SensorProcessor  │ J1939Bus      │  │ SerialCommand │
│ CommandHandler│  │ J1939Encode   │  │    Handler    │
│               │  │ SpnInfo/Check │  │               │
└───────────────┘  └───────────────┘  └───────────────┘
        │                  │
        └────────┬─────────┘
                 ▼
┌─────────────────────────────────────────────────────────┐
│                      Data Layer                          │
│                                                          │
│  ADS1115Manager  MAX31856Manager  BME280Manager         │
│  ConfigStorage                                           │
└─────────────────────────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────┐
│                      Hardware                            │
│  4x ADS1115 ADCs │ MAX31856 TC │ BME280 │ FlexCAN      │
└─────────────────────────────────────────────────────────┘
```

## Layers

### Data Layer (`src/Data/`)

Hardware abstraction - each manager handles one sensor type:

| Module | Hardware | Responsibility |
|--------|----------|----------------|
| `ADS1115Manager` | 4x ADS1115 ADCs | Non-blocking continuous reads, DRDY detection |
| `MAX31856Manager` | Thermocouple | SPI communication, fault detection |
| `BME280Manager` | Ambient sensor | I2C reads for temp, humidity, pressure |
| `ConfigStorage` | EEPROM | Load/save configuration, defaults |

**Key pattern**: All sensor managers use non-blocking reads. They advance a state machine on each `update()` call rather than blocking.

### Domain Layer (`src/Domain/`)

Business logic - converts raw readings to engineering units:

| Module | Responsibility |
|--------|----------------|
| `Ossm` | Main orchestration - setup, loop, timing |
| `SensorProcessor` | Raw ADC → temperature/pressure values |
| `CommandHandler` | Process configuration commands |
| `SerialCommandHandler` | Parse serial input, dispatch to CommandHandler |

**Key pattern**: `SensorProcessor` maps hardware inputs to SPNs based on `AppConfig`. Only enabled SPNs are processed.

### Display Layer (`src/Display/`)

Output formatting and protocol encoding:

| Module | Responsibility |
|--------|----------------|
| `J1939Bus` | CAN bus init, message transmission |
| `J1939Encode` | Pack sensor values into J1939 format |
| `J1939Decode` | Parse incoming J1939 commands |
| `SpnInfo` | SPN metadata (scaling, offsets) |
| `SpnCheck` | Validate SPN assignments |
| `HardwareMap` | Input number → ADC channel mapping |

---

## Data Flow

### Sensor Reading Flow

```
1. IntervalTimer fires (50ms)
   └─► sensorUpdateReady = true

2. loop() detects flag
   └─► processSensorUpdates()
       ├─► ADS1115Manager.update()    // Reads ADC state machine
       ├─► MAX31856Manager.update()   // Reads thermocouple
       ├─► BME280Manager.update()     // Reads ambient
       └─► SensorProcessor.processAllInputs(appData)
           └─► Converts raw → engineering units
           └─► Updates appData struct

3. sendJ1939Messages() (500ms/1000ms)
   └─► J1939Bus.sendPgnXXXX(appData values)
       └─► J1939Encode packs bytes
       └─► FlexCAN transmits
```

### Configuration Flow

```
Serial input "1,0,175,1,3"
   └─► SerialCommandHandler.update()
       └─► Parses bytes
       └─► CommandHandler.enableSpn(appConfig, ...)
           └─► Updates appConfig.tempConfig[3].enabled = true
           └─► Updates appConfig.tempConfig[3].spn = 175

Save command "6"
   └─► ConfigStorage.saveConfig(appConfig)
       └─► CRC32 calculated
       └─► Written to EEPROM
```

---

## Key Data Structures

### AppData (`src/Display/AppData.cnx`)

Runtime sensor values - updated every 50ms:

```c-next
struct AppData {
    f32 oilTemperatureC;
    f32 coolantTemperatureC;
    f32 oilPressurekPa;
    // ... all sensor readings
}
```

### AppConfig (`src/AppConfig.cnx`)

Persistent configuration - stored in EEPROM:

```c-next
struct AppConfig {
    TempInputConfig tempConfig[8];    // temp1-temp8
    PressureInputConfig presConfig[7]; // pres1-pres7
    bool egtEnabled;
    bool bme280Enabled;
    u8 sourceAddress;  // J1939 SA (default 149)
}
```

---

## Timing

| Event | Interval | Source |
|-------|----------|--------|
| Sensor polling | 50ms | IntervalTimer (hardware timer) |
| Fast PGNs (65270, 65263, 65190) | 500ms | elapsedMillis in loop |
| Slow PGNs (65269, 65262, 65129, 65189, 65164) | 1000ms | elapsedMillis in loop |

The IntervalTimer runs in interrupt context and only sets a flag. Actual sensor reads happen in `loop()` to avoid blocking interrupts.

---

## Hardware Mapping

### ADS1115 Addresses and Channels

| ADC | I2C Addr | DRDY Pin | Inputs |
|-----|----------|----------|--------|
| 0 | 0x48 | D0 | temp1, temp2, pres1, pres2 |
| 1 | 0x49 | D1 | temp3, temp4, pres3, pres4 |
| 2 | 0x4A | D4 | temp5, temp6, pres5, pres6 |
| 3 | 0x4B | D5 | temp7, temp8, pres7, - |

### Other Peripherals

| Device | Interface | Pins |
|--------|-----------|------|
| MAX31856 | SPI | CS=D10, DRDY=D2, FAULT=D3 |
| BME280 | I2C | 0x76 (shared I2C bus) |
| CAN | FlexCAN_T4 | CANL=D7, CANH=D8 |

---

## C-Next

OSSM is written in C-Next, a safer C dialect that transpiles to C++. Key syntax differences:

- Assignment: `x <- 5` (not `x = 5`)
- Equality: `x = 5` (not `x == 5`)
- Scopes: `scope Foo { }` → generates `Foo_methodName()` C functions
- Fixed-width types: `u8`, `i32`, `f32` (not `uint8_t`, `int`, `float`)

See the [C-Next documentation](https://github.com/jlaustill/c-next2) for full syntax reference.

---

## Design Decisions

### Non-blocking Sensor Reads

**Problem**: Blocking I2C/SPI reads cause missed CAN messages.

**Solution**: Each sensor manager uses a state machine. `update()` advances one step per call, never blocking. DRDY pins signal when data is ready.

### IntervalTimer for Polling

**Problem**: `millis()` in loop is jittery under load.

**Solution**: Hardware IntervalTimer fires every 50ms. Runs in ISR context, so it only sets a flag. Actual work happens in `loop()`.

### EEPROM Configuration

**Problem**: Recompiling to change sensor assignments is slow.

**Solution**: All configuration stored in EEPROM with CRC32 validation. Serial commands modify config at runtime. First boot uses safe defaults.

### Input-Based Model

**Problem**: Users shouldn't need to know ADC addresses and channels.

**Solution**: Abstract inputs (temp1-temp8, pres1-pres7) map to hardware internally. Users just assign SPNs to inputs.
