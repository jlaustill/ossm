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

Hardware abstraction and centralized value storage:

| Module | Hardware | Responsibility |
|--------|----------|----------------|
| `ADS1115Manager` | 4x ADS1115 ADCs | Non-blocking continuous reads, DRDY detection |
| `MAX31856Manager` | Thermocouple | SPI communication, fault detection |
| `BME280Manager` | Ambient sensor | I2C reads for temp, humidity, pressure |
| `ConfigStorage` | EEPROM | Load/save configuration, defaults |
| `SensorValues` | - | Central storage indexed by EValueId |
| `J1939Config` | - | SPN/PGN encoding tables |

**Key pattern**: All sensor managers use non-blocking reads. They advance a state machine on each `update()` call rather than blocking.

**SensorValues scope** provides:
- `values[]` - Sensor readings indexed by EValueId
- `hasHardware[]` - Tracks which values have physical inputs assigned
- `get(id)` / `set(id, value)` - Accessor methods

### Domain Layer (`src/Domain/`)

Business logic - converts raw readings to engineering units:

| Module | Responsibility |
|--------|----------------|
| `Ossm` | Main orchestration - setup, loop, timing |
| `SensorProcessor` | Raw ADC → temperature/pressure values |
| `CommandHandler` | Process configuration commands |
| `SerialCommandHandler` | Parse serial input, dispatch to CommandHandler |

**Key pattern**: `SensorProcessor` converts raw readings to engineering units. Values are then copied to `SensorValues` indexed by `EValueId`. The J1939 encoder reads from `SensorValues` using the SPN config tables.

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

3. updateSensorValues()
   └─► Copies appData values to SensorValues by EValueId
       └─► SensorValues.set(OIL_TEMP, appData.oilTemperatureC)
       └─► SensorValues.set(OIL_PRES, appData.oilPressurekPa)
       └─► ... (all mapped values)

4. sendJ1939Messages() (500ms/1000ms)
   └─► J1939Bus.sendPgnGeneric(pgn)
       └─► Iterates SPN_CONFIGS[] for this PGN
       └─► For each SPN with hardware assigned:
           └─► J1939Encode.encode(value, resolution, offset)
           └─► Places encoded bytes in buffer
       └─► FlexCAN transmits
```

### Configuration Flow

```
Serial input "1,0,3,15" (assign temp3 to OIL_TEMP)
   └─► SerialCommandHandler.update()
       └─► Parses bytes: inputType=0, inputNum=3, valueId=15
       └─► CommandHandler.assignValue(appConfig, ...)
           └─► Updates appConfig.tempConfig[3].assignedValue = OIL_TEMP
           └─► SensorValues.setHasHardware(OIL_TEMP, true)

Save command "6"
   └─► ConfigStorage.saveConfig(appConfig)
       └─► CRC32 calculated
       └─► Written to EEPROM
```

### J1939 Encoding Flow

```
J1939Bus.sendPgnGeneric(65262) called
   └─► Initialize 8-byte buffer with 0xFF (Not Available)
   └─► For each SPN in SPN_CONFIGS[]:
       └─► If SPN.pgn != 65262: skip
       └─► If !SensorValues.hasHardware[SPN.source]: skip
       └─► value = SensorValues.get(SPN.source)
       └─► encoded = (value + offset) / resolution
       └─► Place at buffer[bytePos - 1]
   └─► Transmit buffer on CAN bus
```

---

## Key Data Structures

### EValueId (`src/Data/types/EValueId.cnx`)

Physical measurement locations - the primary abstraction for sensor data:

```c-next
enum EValueId {
    AMBIENT_PRES, AMBIENT_TEMP, AMBIENT_HUMIDITY,
    TURBO1_COMP_INLET_PRES, TURBO1_COMP_INLET_TEMP,
    TURBO1_COMP_OUTLET_PRES, TURBO1_COMP_OUTLET_TEMP,
    TURBO1_TURB_INLET_TEMP,   // EGT
    CAC1_INLET_PRES, CAC1_INLET_TEMP, CAC1_OUTLET_PRES, CAC1_OUTLET_TEMP,
    MANIFOLD1_ABS_PRES, MANIFOLD1_TEMP,
    OIL_PRES, OIL_TEMP, COOLANT_PRES, COOLANT_TEMP,
    FUEL_PRES, FUEL_TEMP, ENGINE_BAY_TEMP,
    VALUE_ID_COUNT  // Sentinel for array sizing
}
```

### SensorValues (`src/Data/SensorValues.cnx`)

Centralized storage for all sensor values, indexed by EValueId:

```c-next
scope SensorValues {
    f32 values[EValueId.VALUE_ID_COUNT];
    bool hasHardware[EValueId.VALUE_ID_COUNT];

    public f32 get(EValueId id);
    public void set(EValueId id, f32 value);
    public bool getHasHardware(EValueId id);
    public void setHasHardware(EValueId id, bool has);
}
```

### TSpnConfig (`src/Data/types/TSpnConfig.cnx`)

SPN encoding parameters for data-driven J1939 output:

```c-next
struct TSpnConfig {
    u16 spn;          // J1939 SPN number
    u16 pgn;          // Which PGN this SPN belongs to
    u8 bytePos;       // Start byte (1-indexed per J1939 docs)
    u8 dataLength;    // 1 or 2 bytes
    f32 resolution;   // kPa/bit, °C/bit, etc.
    f32 offset;       // Added before scaling
    EValueId source;  // Which value to encode
}
```

### AppData (`src/Display/AppData.cnx`)

Runtime sensor values - updated every 50ms (legacy, being phased out):

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

**Solution**: Abstract inputs (temp1-temp8, pres1-pres7) map to hardware internally. Users assign inputs to physical measurement locations (EValueId).

### Data-Driven J1939 Encoding

**Problem**: Adding new SPNs required writing new encoding functions. Same value at different resolutions (e.g., boost at 2 kPa and 0.1 kPa) duplicated code.

**Solution**: Configuration tables define SPN encoding parameters. One generic encoder handles all SPNs:

```
SPN_CONFIGS[] = [
    { spn: 102, pgn: 65270, resolution: 2.0, source: MANIFOLD1_ABS_PRES },
    { spn: 4817, pgn: 64976, resolution: 0.1, source: MANIFOLD1_ABS_PRES },
    // Same source, different resolutions - no code duplication
]
```

Adding new SPNs requires only a config entry, no new code. The `sendPgnGeneric()` function iterates configs and encodes any SPN whose source has hardware assigned.

### Value-Based Configuration

**Problem**: Users had to know J1939 SPN numbers to configure sensors. SPN names don't clearly indicate physical location.

**Solution**: EValueId enum uses clear physical names (OIL_TEMP, MANIFOLD1_PRES) that match where sensors are actually installed. SPNs are automatically enabled when their source value has hardware. Users think in terms of measurement locations, not protocol details.
