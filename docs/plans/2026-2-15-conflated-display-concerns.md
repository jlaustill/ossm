# Conflated Display Layer Concerns

**Date:** 2026-02-15
**Status:** Analysis Complete
**Target:** Display layer refactoring

## Overview

This document identifies files in the Display layer (`src/Display/`) that conflate multiple concerns, based on architectural review.

## Files Analyzed

### Well-Focused (No Refactoring Needed)

| File | Primary Responsibility | Notes |
|------|----------------------|-------|
| `J1939Encode.cnx` | SPN encoding per SAE J1939-71 | Pure encoding: temp16bit, pressure4kPa, etc. |
| `SensorConvert.cnx` | Physics calculations | NTC thermistor (Steinhart-Hart), pressure voltage-to-kPa |
| `FaultDecode.cnx` | MAX31856 fault bit interpretation | Pure bit manipulation and checking |
| `ValueName.cnx` | Human-readable value names | Lookup table for EValueId labels |
| `ByteUtils.cnx` | Byte manipulation utilities | Endianness, extraction, clamping |
| `FloatBytes.cnx` | Float/byte conversion | IEEE 754 reconstruction for J1939 commands |
| `InputValid.cnx` | Input validation | Range checking for all command inputs |
| `Presets.cnx` | Sensor preset lookup tables | NTC coefficients, pressure presets, TC types |
| `Crc32.cnx` | CRC32 checksum calculation | Memory-safe checksum over AppConfig struct |

---

### Conflated Files Requiring Refactoring

#### 1. `J1939Bus.cnx` - High Priority

**Current Responsibilities:**
- CAN hardware initialization and configuration
- CAN message transmission (sendMessage)
- PGN scheduling and encoding (sendPgnGeneric)
- Config command buffering from CAN receive
- Value enablement checking (isValueEnabled)
- CAN message sniffing/receiving (sniffDataPrivateISR)

**Code Locations:**

| Lines | Function | Concern |
|-------|----------|---------|
| 40-51 | `sendMessage` | Transmission |
| 55-82 | `sendPgnGeneric` | **Encoding + Scheduling + Transmission** |
| 86-97 | `hasPendingCommand` / `getPendingCommand` | Config command buffering |
| 101-119 | `sniffDataPrivateISR` | CAN receive |
| 123-137 | `initialize` | Hardware setup |

**Issues Identified:**

1. **`sendPgnGeneric` conflates encoding with transmission**
   - Iterates SPN_CONFIGS (scheduling concern)
   - Calls `J1939Encode.encode()` (encoding concern)
   - Writes to CAN bus (transmission concern)
   - Should be: `J1939Sender.encodeAndSendPgn(pgn)`

2. **`isValueEnabled` accesses SensorValues directly**
   - Lines 36-38: `SensorValues.current[valueId].hasHardware`
   - Tightly couples J1939Bus to SensorValues module
   - Should be injected or passed as configuration

3. **Hardware address configuration mixed with bus operations**
   - `appConfig.j1939SourceAddress` used in `sendMessage`
   - Bus initialization hardcodes 250kbps
   - Source address should be configured at initialization, not scattered

**Proposed Split:**

```
J1939Bus.cnx (after refactoring):
- CAN hardware initialization
- Message receive buffering
- Source address getter/setter
- No encoding, no scheduling

J1939Sender.cnx (new file):
- PGN scheduling (which PGNs when)
- Encoding integration
- Transmission interface
```

---

#### 2. `HardwareMap.cnx` - Low Priority

**Current Responsibilities:**
- Temperature input to ADS1115 device/channel mapping
- Pressure input to ADS1115 device/channel mapping
- I2C address lookup
- Input validation (duplicates `InputValid.cnx`)

**Code Locations:**

| Lines | Function | Concern |
|-------|----------|---------|
| 13-30 | `tempDevice` | Hardware mapping |
| 34-47 | `tempChannel` | Hardware mapping |
| 51-67 | `pressureDevice` | Hardware mapping |
| 71-83 | `pressureChannel` | Hardware mapping |
| 87-93 | `adsI2cAddress` | Hardware mapping |
| 96-107 | `isValidTempInput`, `isValidPressureInput` | **Validation (duplicate)** |

**Issues Identified:**

1. **Duplicate validation functions**
   - `HardwareMap.isValidTempInput()` (lines 96-100)
   - `HardwareMap.isValidPressureInput()` (lines 102-107)
   - Duplicate of `InputValid.isValidTempInput()` and `InputValid.isValidPressureInput()`

2. **Mixed abstraction levels**
   - Device/channel mapping (low-level hardware)
   - Input validation (high-level sanity check)

**Proposed Resolution:**

Option A (preferred): Remove validation functions from `HardwareMap.cnx` - they're already in `InputValid.cnx`

Option B: Move all validation to `HardwareMap.cnx` (if it's considered the authoritative source for input boundaries)

---

## Refactoring Priority

| Priority | File | Effort | Impact |
|----------|------|--------|--------|
| High | `J1939Bus.cnx` | Medium | Decouples transport from encoding |
| Low | `HardwareMap.cnx` | Low | Removes duplicate validation |

---

## Implementation Plan

### Phase 1: Split J1939Bus

1. Create new `src/Display/J1939Sender.cnx`
2. Move `sendPgnGeneric` to `J1939Sender`
3. Extract encoding calls from `sendPgnGeneric` if needed
4. Keep `sendMessage`, `hasPendingCommand`, `getPendingCommand` in `J1939Bus`
5. Update callers to use new module

### Phase 2: Clean HardwareMap

1. Remove `isValidTempInput`, `isValidPressureInput` from `HardwareMap.cnx`
2. Update callers to use `InputValid` module
3. Add deprecation notice if needed

---

## Related Documentation

- Architecture: `docs/ARCHITECTURE.md`
- J1939 Protocol: `docs/SPN-REFERENCE.md`
- Current CLAUDE.md architecture section
