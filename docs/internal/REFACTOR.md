# OSSM Refactoring Roadmap

This document tracks opportunities to DRY up the codebase across three libraries.

## Architecture Philosophy
- **sea-dash**: Generic utilities like lodash - useful for any project
- **J1939**: Protocol-specific library - SPN/PGN encoding/decoding, CAN-agnostic
- **OSSM**: Project-specific code

---

## 1. sea-dash Extensions

### 1.1 Buffer Utilities
- [ ] **Status**: Not started
- **File**: `sea-dash/include/Buffer.hpp`
- **Occurrences in OSSM**: 16+
- **Description**: Byte buffer read/write utilities for big/little endian

```cpp
namespace SeaDash::Buffer {
    template<typename T> void writeU16BE(T* buf, size_t offset, uint16_t value);
    template<typename T> void writeU16LE(T* buf, size_t offset, uint16_t value);
    template<typename T> uint16_t readU16BE(const T* buf, size_t offset);
    template<typename T> uint16_t readU16LE(const T* buf, size_t offset);
    template<typename T> void fill(T* buf, size_t len, T value);
}
```

### 1.2 Token Parser
- [x] **Status**: Complete (sea-dash 0.0.6)
- **Files**: `sea-dash/include/Parse.hpp`, `sea-dash/src/Parse.cpp`
- **Occurrences in OSSM**: 8+
- **Description**: CSV/delimiter token parsing class

```cpp
namespace SeaDash::Parse {
    class TokenParser {
    public:
        TokenParser(char* buffer, char delimiter = ',');
        bool nextU8(uint8_t& out);
        bool nextU16(uint16_t& out);
        bool nextFloat(float& out);
        bool hasMore() const;
        char* nextRaw();
    };
}
```

---

## 2. J1939 Library Extensions

### 2.1 SPN Encoding/Decoding
- [ ] **Status**: Not started
- **Files**: `J1939/include/J1939Spn.h`, `J1939/src/J1939Spn.cpp`
- **Occurrences in OSSM**: 15+
- **Description**: Standard J1939 SPN encoding formulas

```cpp
namespace J1939 {
    // Temperature (1-byte, +40 offset)
    uint8_t encodeTemp8(float tempC);
    float decodeTemp8(uint8_t encoded);

    // High-res temperature (2-byte Kelvin, 0.03125K resolution)
    uint16_t encodeTempHiRes(float tempC);
    float decodeTempHiRes(uint16_t encoded);

    // Pressure (configurable scale factor)
    uint8_t encodePressure8(float kPa, float scaleFactor);
    uint16_t encodePressure16(float kPa, float scaleFactor);

    // Humidity (0.4% resolution)
    uint8_t encodeHumidity(float percent);
    float decodeHumidity(uint8_t encoded);
}
```

### 2.2 PGN Constants (Future)
- [ ] **Status**: Not started
- **Description**: Common PGN definitions and field layouts

---

## 3. OSSM Utilities

### 3.1 SPN Lookup Utils
- [ ] **Status**: Not started
- **File**: `src/Utils/SpnUtils.h`
- **Duplicated in**: `J1939Bus.cpp`, `SerialCommandHandler.cpp`
- **Description**: SPN category lookup and input search

```cpp
ESpnCategory getSpnCategory(uint16_t spn);
int8_t findTempInputBySpn(const AppConfig* cfg, uint16_t spn);
int8_t findPressureInputBySpn(const AppConfig* cfg, uint16_t spn);
```

### 3.2 Preset Utils
- [ ] **Status**: Not started
- **File**: `src/Utils/PresetUtils.h`
- **Duplicated in**: `J1939Bus.cpp`, `SerialCommandHandler.cpp`
- **Description**: NTC and pressure preset application

```cpp
bool applyNtcPreset(TTempInputConfig& cfg, uint8_t preset);
bool applyPressurePreset(TPressureInputConfig& cfg, uint8_t preset);
```

### 3.3 Command Processor
- [ ] **Status**: Not started
- **File**: `src/Utils/CommandProcessor.h`
- **Duplicated in**: `J1939Bus.cpp`, `SerialCommandHandler.cpp`
- **Description**: Unified command handling (J1939 and Serial just parse, then call this)

```cpp
class CommandProcessor {
public:
    static uint8_t enableSpn(AppConfig* cfg, uint16_t spn, bool enable, uint8_t input);
    static uint8_t setNtcParam(AppConfig* cfg, uint8_t input, uint8_t param, float value);
    static uint8_t setPressureRange(AppConfig* cfg, uint8_t input, uint16_t maxPsi);
    static uint8_t setThermocoupleType(AppConfig* cfg, uint8_t type);
    static uint8_t applyNtcPreset(AppConfig* cfg, uint8_t input, uint8_t preset);
    static uint8_t applyPressurePreset(AppConfig* cfg, uint8_t input, uint8_t preset);
};
```

---

## Suggested Order

1. **sea-dash 1.1** - Buffer utilities (simplest, most reusable)
2. **J1939 2.1** - SPN encoding (high value for OSSM cleanup)
3. **OSSM 3.1** - SPN lookup utils (small, isolated)
4. **OSSM 3.2** - Preset utils (small, isolated)
5. **sea-dash 1.2** - Token parser (medium complexity)
6. **OSSM 3.3** - Command processor (largest refactor, depends on others)

---

## Notes

_Add discussion notes for each item as we work through them._
