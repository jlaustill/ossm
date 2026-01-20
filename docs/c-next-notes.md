# C-Next Conversion Notes

Technical notes and patterns learned during C-Next conversion.

## Potential C-Next Transpiler Improvements

These could be filed as GitHub issues for the c-next transpiler:

### 1. Use C++ casts instead of C-style casts

**Current generated code:**
```cpp
(*crc) = crc32_crcByte(crc, (uint8_t*)&buf[0]);
```

**Preferred:**
```cpp
(*crc) = crc32_crcByte(crc, static_cast<uint8_t*>(&buf[0]));
```

### 2. Add `const` to unmodified pointer parameters

**Current generated code:**
```cpp
uint8_t j1939_encode_lowByte(uint16_t* value) {
    return (((*value)) & 0xFFU);
}
```

**Preferred:**
```cpp
uint8_t j1939_encode_lowByte(const uint16_t* value) {
    return (((*value)) & 0xFFU);
}
```

## C-Next Syntax Patterns

### Comparison operator
C-Next uses `=` for equality (not `==`):
```cnx
if (cfg.pressureType = PRESSURE_TYPE_PSIA) { ... }
```
Transpiles to:
```cpp
if (cfg->pressureType == PRESSURE_TYPE_PSIA) { ... }
```

### Scope member access
Must use `this.` prefix for scope-level constants and functions:
```cnx
scope sensor_convert {
    const f32 VREF <- 5.0;

    public f32 ntcTemperature(f32 voltage, const TTempInputConfig cfg) {
        if (voltage >= this.VREF) { ... }  // Must use this.VREF
    }
}
```

### MISRA 13.5 compliance
No function calls allowed in `if` conditions:
```cnx
// ERROR: Function call in condition
if (this.isTempSpnEnabled(cfg, spn)) { ... }

// CORRECT: Store result first
bool tempEnabled <- this.isTempSpnEnabled(cfg, spn);
if (tempEnabled) { ... }
```

### Bit slice extraction
Clean syntax for byte extraction:
```cnx
public u8 lowByte(u16 value) {
    return value[0, 8];   // bits 0-7
}

public u8 highByte(u16 value) {
    return value[8, 8];   // bits 8-15
}
```
Transpiles to:
```cpp
return (((*value)) & 0xFFU);
return (((*value) >> 8) & 0xFFU);
```

### External C functions
Include system headers to use C library functions:
```cnx
#include <math.h>

// Now log() is available
f32 lnR <- log(r_ntc);
```

### Wrapper pattern for pointer parameters
C-Next generates pointer parameters for value types. Use thin wrappers to preserve clean call sites:
```cpp
// In C++ code that calls C-Next functions:
static inline bool isSpnEnabled(const AppConfig* cfg, uint16_t spn) {
    return spn_check_isSpnEnabled(cfg, &spn);
}
```

## Build Integration

The `.cnx` files are compiled with the `cnext` CLI tool:
```bash
cd src && cnext *.cnx
```

This generates `.cpp` and `.h` files that are compiled by PlatformIO alongside the rest of the codebase.
