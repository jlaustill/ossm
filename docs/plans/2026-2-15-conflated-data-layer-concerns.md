# Conflated Data Layer Concerns

**Date:** 2026-02-15
**Status:** Analysis Complete
**Target:** Data layer refactoring

## Overview

This document identifies files in the Data layer (`src/Data/`) that conflate multiple concerns, based on architectural review.

## Files Analyzed

### Well-Focused (No Refactoring Needed)

| File | Primary Responsibility | Notes |
|------|----------------------|-------|
| `SensorValues.cnx` | Central value storage | Simple struct array, no logic |
| `EValueId.cnx` | Physical measurement enum | Pure enum definition |
| `TAdcReading.cnx` | ADC reading struct | Data-only struct |
| `TPgnConfig.cnx` | PGN transmission config | Data-only struct |
| `TSpnConfig.cnx` | SPN encoding config | Data-only struct |
| `J1939Config.cnx` | SPN/PGN configuration tables | Lookup table + helper |
| `ConfigStorage.cnx` | EEPROM persistence | Load/save/validate cycle |
| `ADS1115Manager.cnx` | ADS1115 hardware driver | State machine + I2C |
| `BME280Manager.cnx` | BME280 hardware driver | State machine + I2C |
| `MAX31856Manager.cnx` | MAX31856 hardware driver | State machine + SPI |

---

### Conflated Files Requiring Refactoring

#### 1. `ConfigStorage.cnx` - Medium Priority

**Current Responsibilities:**
- Configuration loading from EEPROM
- Configuration saving to EEPROM
- Configuration validation (CRC check)
- **Default configuration loading** (loadDefaults)
- **Pre-populating default NTC coefficients**
- **Pre-populating default pressure ranges**

**Code Locations:**

| Lines | Function | Concern |
|-------|----------|---------|
| 13-31 | `validateConfig` | Validation only |
| 34-71 | `loadDefaults` | **Default loading + physics presets** |
| 75-82 | `saveConfig` | Save only |
| 86-98 | `loadConfig` | Load + fallback logic |

**Issues Identified:**

1. **`loadDefaults` mixes default config with sensor presets**
   - Lines 44-51: Hardcodes AEM NTC coefficients
   - Lines 54-60: Hardcodes 100 PSIG pressure range
   - These are **default recommendations**, not required defaults
   - A user might want different defaults (e.g., Bosch sensors)

2. **Configuration has two levels of "default"**
   - EEPROM-based defaults (what's stored)
   - Code-based defaults (AEM coefficients, PSIG type)
   - This creates confusion about what the "true" default is

**Recommended Resolution:**

Option A (preferred): Separate default configuration from physics presets
```c-next
// ConfigStorage.cnx - only handles persistence
scope ConfigStorage {
    void loadConfig(AppConfig config) { ... }
    bool saveConfig(AppConfig config) { ... }
    bool validateConfig(AppConfig config) { ... }
}

// Presets.cnx or new DefaultConfig.cnx
scope DefaultConfig {
    void applyDefaults(AppConfig config) {
        // All physics defaults here
        config.tempInputs[i].coeffA <- AEM_TEMP_COEFF_A; // just examples
    }
}
```

Option B: Document that defaults are intentionally opinionated

---

#### 2. `J1939Config.cnx` - Edge Case / Low Priority

**Current Responsibilities:**
- SPN configuration table (20 entries)
- PGN configuration table (8 entries)
- **Lookup helper** (`findSourceForSpn`)

**Issues Identified:**

1. **Lookup helper is minimal but tight coupling to SPN_CONFIGS**
   - `findSourceForSpn` iterates SPN_CONFIGS table
   - If table grows, this becomes O(n) lookup
   - No corresponding reverse lookup (SPN -> index)

2. **Data and logic in same file (acceptable but note)**
   - Configuration tables are pure data
   - Helper function is minimal logic
   - For 20 entries, O(n) is acceptable

**Recommended Resolution:**

Monitor `findSourceForSpn` usage - if called frequently in hot paths, consider:
- Adding reverse index mapping
- Caching last lookup result (if data is static)
- Moving to a faster lookup structure (e.g., sorted table with binary search)

---

#### 3. `ADS1115Manager.cnx` - Medium Priority

**Current Responsibilities:**
- ADS1115 hardware initialization
- ADC conversion state machine
- Reading buffer management with critical sections
- **Hardware mapping integration** (HardwareMap.tempDevice, HardwareMap.pressureDevice)
- **I2C address configuration** via HardwareMap.adsI2cAddress

**Issues Identified:**

1. **HardwareMap dependency creates bidirectional coupling**
   - `ADS1115Manager.cnx` imports `Display/HardwareMap.cnx`
   - `HardwareMap.cnx` imports nothing (no return coupling)
   - This is a layering violation: Data layer shouldn't import Display layer

2. **Physical input to device mapping is inverted**
   - Current: `ADS1115Manager` asks HardwareMap for device/channel
   - Should be: HardwareMap provides device/channel via callback or injection
   - Makes testing difficult (hard to mock hardware layout)

3. **`getVoltage` computes voltage from raw ADC value**
   - Line 234-242: Calls `ads[device].computeVolts()`
   - This is **encoding/decoding logic** - belongs in Display layer
   - ADS1115Manager should return raw ADC values

**Recommended Resolution:**

Split `ADS1115Manager.cnx`:
```c-next
// Data/ADS1115Driver.cnx - pure hardware driver
scope ADS1115Driver {
    // Initialization, state machine, raw reads
    void initialize(const u8 i2cAddrs[ADS_DEVICE_COUNT]);
    bool update();
    i16 getRawReading(u8 device, u8 channel);
}

// Display/ADS1115Converter.cnx - voltage conversion
scope ADS1115Converter {
    f32 rawToVolts(u8 device, i16 rawValue);
    f32 rawToTemperature(u8 device, i16 rawValue, TTempInputConfig cfg);
}
```

This fixes:
- Layering (Data doesn't import Display)
- Separation of concerns (driver vs conversion)
- Testability (easy to test raw ADC handling)

---

#### 4. `SensorValues.cnx` - Architecture Decision Point

**Current Responsibilities:**
- Central value storage (array of TSensorValue)
- Simple initialization

**Issues Identified:**

1. **Accessed directly from Domain layer**
   - `SensorProcessor.cnx`: `SensorValues.current[EValueId.OIL_PRES].value`
   - `Hardware.cnx`: `SensorValues.current[id].hasHardware`
   - `SerialCommandHandler.cnx`: Direct value reads for output
   - No abstraction layer over the storage

2. **No update mechanism**
   - Data is written to by SensorProcessor
   - No notification to interested parties
   - No versioning/invalidation mechanism

3. **Tightly coupled to EValueId**
   - Array size based on EValueId.VALUE_ID_COUNT
   - If EValueId changes, SensorValues must change
   - No decoupling between value identities and storage

**Architectural Options:**

Option A: Keep as-is (simple, transparent)
- Pros: Fast, no indirection, easy to understand
- Cons: Tight coupling, no change notifications

Option B: Add event system
- Pros: Decoupled updates, change notifications
- Cons: More complex, overhead

Option C: Add accessor layer
- Pros: Abstraction, can add validation/notifications
- Cons: Indirection overhead

---

## Dependency Analysis

### Layering Violations

```
Data layer imports Display layer:
  ConfigStorage.cnx → Display/Crc32.cnx
  ADS1115Manager.cnx → Display/HardwareMap.cnx

Display layer imports Data layer:
  None (good - Display is downstream of Data)

Domain layer imports both:
  SensorProcessor.cnx → Data/ADS1115Manager.cnx
  SensorProcessor.cnx → Display/SensorConvert.cnx
  J1939CommandHandler.cnx → Display/J1939Bus.cnx
```

**Problem:** `ADS1115Manager.cnx` (Data) imports `HardwareMap.cnx` (Display)
**Root cause:** Hardware layout knowledge shouldn't be in Display layer

---

## Refactoring Priority

| Priority | File | Effort | Impact |
|----------|------|--------|--------|
| High | `ADS1115Manager.cnx` | High | Fixes layering violation |
| Medium | `ConfigStorage.cnx` | Medium | Removes physics from persistence |
| Low | `J1939Config.cnx` | Low | Monitory for performance |
| Decision needed | `SensorValues.cnx` | N/A | Architecture choice |

---

## Implementation Plan

### Phase 1: Fix Layering (ADS1115Manager)

1. Create `src/Data/ADS1115Driver.cnx` - raw hardware driver
2. Create `src/Display/ADS1115Converter.cnx` - voltage conversion
3. Move `getVoltage` logic to converter
4. Remove HardwareMap import from driver
5. Inject I2C addresses at initialization

### Phase 2: Separate Config Defaults (ConfigStorage)

1. Create `src/Data/DefaultConfig.cnx` or extend `Presets.cnx`
2. Move NTC coefficient defaults to new module
3. Move pressure type defaults to new module
4. Update ConfigStorage.loadDefaults to call DefaultConfig.applyDefaults

### Phase 3: Monitor/Refine (J1939Config)

1. Profile `findSourceForSpn` calls
2. If frequent, add reverse index mapping
3. Consider binary search if table grows >50 entries

### Phase 4: Design SensorValues Abstraction

1. Decide on coupling approach (keep as-is or add abstraction)
2. Document choice in architecture decisions
3. Implement if abstraction is needed

---

## Related Documentation

- Display layer analysis: `docs/plans/2026-2-15-conflated-display-concerns.md`
- Architecture: `docs/ARCHITECTURE.md`
