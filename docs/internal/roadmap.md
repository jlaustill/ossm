# OSSM Async Sensor Architecture Roadmap

## Summary
Restructure OSSM from blocking ADC reads with preprocessor config to IntervalTimer-based async reading with EEPROM-configurable sensor mappings. Configuration can be updated at runtime via Serial USB or J1939 requests.

## Hardware Configuration

Reference: `hardware/V0.0.2/Schematic_OSSM_2025-12-18.pdf`

### Device Overview
| Device  | I2C/Pin               | DRDY Pin                  | Notes                               |
|---------|-----------------------|---------------------------|-------------------------------------|
| ADS1115 | 0x48                  | D0                        | 4 channels                          |
| ADS1115 | 0x49                  | D1                        | 4 channels                          |
| ADS1115 | 0x4A                  | D4                        | 4 channels                          |
| ADS1115 | 0x4B                  | D5                        | 3 channels used                     |
| MAX31856 | SPI (CS=D10)         | D2 (DRDY), D3 (FAULT)     | EGT thermocouple                    |
| BME280  | I2C                   | N/A                       | Ambient temp, humidity, barometric pressure |

### ADS1115 Channel Mapping (from schematic)
| ADS Addr | DRDY Pin | Ch0 (A0) | Ch1 (A1) | Ch2 (A2) | Ch3 (A3) |
|----------|----------|----------|----------|----------|----------|
| 0x48     | D0       | temp1    | temp2    | pres1    | pres2    |
| 0x49     | D1       | temp3    | temp4    | pres5    | pres6    |
| 0x4A     | D4       | pres3    | temp5    | temp6    | pres4    |
| 0x4B     | D5       | pres7    | temp7    | temp8    | (unused) |

### Sensor Totals
| Type                | Count | Source                      |
|---------------------|-------|-----------------------------|
| Temperature (NTC)   | 8     | temp1-8 via ADS1115         |
| Pressure            | 7     | pres1-7 via ADS1115         |
| EGT (thermocouple)  | 1     | MAX31856                    |
| Ambient temp        | 1     | BME280                      |
| Humidity            | 1     | BME280                      |
| Barometric pressure | 1     | BME280                      |
| **Total**           | **19** |                             |

## New File Structure
```
src/
├── Data/                          # Hardware abstraction layer
│   ├── types/
│   │   ├── ESensorType.h          # Enum: PRESSURE_0_100PSI, TEMP_NTC_AEM, etc.
│   │   ├── TSensorConfig.h        # Sensor config entry struct
│   │   └── TAdcReading.h          # Raw ADC value + timestamp
│   ├── ADS1115Manager/            # Centralized 4x ADS1115 management
│   │   ├── ADS1115Manager.h
│   │   └── ADS1115Manager.cpp
│   ├── MAX31856Manager/           # Thermocouple management
│   │   ├── MAX31856Manager.h
│   │   └── MAX31856Manager.cpp
│   ├── ConfigStorage/             # EEPROM read/write
│   │   ├── ConfigStorage.h
│   │   └── ConfigStorage.cpp
│   └── BME280Manager/             # Ambient sensor management (integrated)
│       ├── BME280Manager.h
│       └── BME280Manager.cpp
│
├── Domain/                        # Business logic layer
│   ├── types/
│   │   └── TSensorReading.h       # Processed value with unit
│   ├── SensorProcessor/           # Raw ADC → engineering units
│   │   ├── SensorProcessor.h
│   │   └── SensorProcessor.cpp
│   ├── ossm.h                     # Modified: IntervalTimer orchestration
│   └── ossm.cpp
│
├── Display/                       # CAN output layer (existing)
│   ├── J1939Bus.h
│   └── J1939Bus.cpp               # Modified: runtime config instead of #ifdef

include/
├── AppData.h                      # Existing: runtime sensor values
└── AppConfig.h                    # NEW: EEPROM-stored configuration
```

## Key New Structures

### AppConfig (EEPROM-stored)
```cpp
struct AppConfig {
    uint32_t magic;              // 0x4F53534D validates EEPROM
    uint8_t version;
    uint8_t j1939SourceAddress;

    struct { uint8_t i2cAddress; uint8_t drdyPin; bool enabled; } adsDevices[4];
    struct { uint8_t drdyPin; uint8_t faultPin; uint8_t csPin; bool enabled; } thermocouple;

    TSensorConfig sensors[19];   // Each sensor: type, adsDevice, adsChannel, calibration
    uint32_t checksum;
};
```

### TSensorConfig (per-sensor)
```cpp
struct TSensorConfig {
    ESensorType sensorType;      // PRESSURE_0_100PSI, TEMP_NTC_AEM, etc.
    uint8_t adsDevice;           // 0-3, or 0xFF for non-ADS
    uint8_t adsChannel;          // 0-3
    uint8_t appDataIndex;        // Index into AppData fields
    float coeffA, coeffB, coeffC; // Calibration (Steinhart-Hart or pressure scaling)
    float resistorValue;
    float wiringResistance;
};
```

## ADC Reading Strategy
- **Pattern**: Non-blocking polling (not true ISR due to I2C limitations)
- **Timing**: 50ms IntervalTimer fires → polls each ADS for `conversionComplete()` → reads result → starts next channel
- **Cycle**: Round-robin through all 16 channels (~125ms full cycle at 128 SPS)
- **Data Safety**: `noInterrupts()`/`interrupts()` guards around volatile buffer copies

## Implementation Phases

### Phase 1: Foundation
- [ ] Create type definitions: `ESensorType.h`, `TSensorConfig.h`, `TAdcReading.h`
- [ ] Create `AppConfig.h` with defaults matching current `configuration.h`
- [ ] Create `ConfigStorage` class (EEPROM load/save/validate)
- [ ] Add Adafruit MAX31856 library to `platformio.ini`

### Phase 2: ADS1115Manager
- [ ] Create `ADS1115Manager` class with static methods
- [ ] Implement single-device operation (0x4B first)
- [ ] Add round-robin channel cycling across all 4 devices
- [ ] Add DRDY pin polling (optional interrupt enhancement later)

### Phase 3: SensorProcessor
- [ ] Create `SensorProcessor` class
- [ ] Migrate pressure conversion logic from `lib/PressureSensor/`
- [ ] Migrate NTC temperature conversion logic from `lib/TempSensor/`
- [ ] Support all ESensorType variants

### Phase 4: IntervalTimer Integration
- [ ] Add `IntervalTimer sensorTimer` to ossm class
- [ ] Create 50ms callback: poll ADCs, process sensors
- [ ] Update J1939Bus with separate send timing

### Phase 5: MAX31856 Integration
- [ ] Create `MAX31856Manager` class
- [ ] Configure D2 (DRDY) and D3 (FAULT) pins
- [ ] Add thermocouple reading to SensorProcessor

### Phase 6: BME280 Integration
- [ ] Create `BME280Manager` class (migrate from AmbientSensors)
- [ ] Integrate into SensorProcessor for ambient readings
- [ ] Remove old AmbientSensors code

### Phase 7: Runtime Configuration Interfaces
- [ ] Update `J1939Bus.cpp` to use AppConfig instead of `#ifdef`
- [ ] Add Serial USB command interface for config changes
- [ ] Add J1939 request handler for config updates (proprietary PGN)
- [ ] Test EEPROM persistence across reboots

### Phase 8: Cleanup
- [ ] Delete `lib/PressureSensor/` and `lib/TempSensor/`
- [ ] Archive or remove `configuration.h`
- [ ] Delete `src/Data/AmbientSensors/` (replaced by BME280Manager)
- [ ] Update README.md

## Critical Files to Modify
1. `src/Domain/ossm.cpp` - Replace elapsedMillis with IntervalTimer
2. `src/Display/J1939Bus.cpp` - Remove `#ifdef`, use AppConfig
3. `platformio.ini` - Add MAX31856 library

## Data Flow
```
Hardware (ADS1115 x4, MAX31856, BME280)
           ↓
    ADS1115Manager / MAX31856Manager / BME280Manager (raw sensor buffers)
           ↓
    ConfigStorage (EEPROM → AppConfig)
           ↓
    SensorProcessor (raw → engineering units using AppConfig calibration)
           ↓
    AppData (19 float values)
           ↓
    J1939Bus (PGN messages on 500ms/1000ms schedule)

Configuration Updates:
    Serial USB ─────┐
                    ├──→ ConfigStorage ──→ EEPROM
    J1939 Request ──┘
```

## Notes
- ADS1115 at 128 SPS = ~8ms per conversion; 15 active channels = ~120ms full cycle
- EEPROM size: ~640 bytes (Teensy 4.1 has 4284 bytes available)
- Keep IntervalTimer callback fast - no Serial.print, no blocking I2C
- One spare ADC channel available on 0x4B (A3) for future expansion
