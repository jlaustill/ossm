# C-Next Conversion Status

C-Next transpiles to standard C/C++, providing memory safety while generating code compatible with existing toolchains.

**C-Next Version:** 0.1.13

## Static Analysis Results

### BEFORE (main branch - handwritten C++)

| Tool | Warnings | Errors | Notes |
|------|----------|--------|-------|
| cppcheck | 5 | 0 | Style only |
| flawfinder | 5 | 0 | CWE-119/120/126 |
| MISRA C:2012 | 2 | 0 | Rules 12.3, 17.2 |
| rats | 3 | 0 | 2 high, 1 medium |
| scan-build | 0 | 0 | ✅ Clean |
| clang-tidy | — | — | Requires Arduino headers |

**cppcheck breakdown:**
- 5x `unusedFunction` (intentional API surface)

**flawfinder breakdown (SerialCommandHandler.cpp):**
- 2x Level 2 (CWE-119/120): Static `char[]` buffer risks
- 3x Level 1: `read`, `strlen`, `strncpy` boundary checks

**MISRA violations:**
- Rule 12.3: Comma operator in FlexCAN template (J1939Bus.cpp:13)
- Rule 17.2: Recursive call in `handleQuery()` (SerialCommandHandler.cpp:309)

**rats breakdown:**
- 2x High: Fixed size local buffers (SerialCommandHandler.cpp:79, crc32.cpp:28)
- 1x Medium: `read` in loop (SerialCommandHandler.cpp:58)

### AFTER (C-Next v0.1.13 conversion)

| Tool | Warnings | Errors | Notes |
|------|----------|--------|-------|
| cppcheck | 5 | 0 | Same as main! |
| flawfinder | 5 | 0 | Unchanged (non-converted) |
| MISRA C:2012 | 2 | 0 | Unchanged (non-converted) |
| rats | 3 | 0 | Unchanged (non-converted) |
| scan-build | 0 | 0 | ✅ Clean |
| C-Next | 0 | 0 | 27 safety checks pass |

**cppcheck breakdown:**
- 5x `unusedFunction` (unchanged - API surface)

**v0.1.13 improvements (issues fixed):**
- ✅ `cstyleCast` - Now generates `static_cast<>()` (#267 FIXED)
- ✅ `constParameterPointer` - Now uses pass-by-value for small types (#269 FIXED)

**Notes:**
- New cppcheck warnings are style issues in C-Next generated code, not safety issues
- Flawfinder/MISRA hits remain in non-converted modules (candidates for future conversion)
- C-Next enforces 27 safety checks at transpile time (see [docs/static-analysis.md](docs/static-analysis.md))

## Conversion Progress

| Module | Lines | Status | Purpose |
|--------|-------|--------|---------|
| `crc32.cnx` | 76 | ✅ Done | CRC32 checksum calculation |
| `j1939_encode.cnx` | 71 | ✅ Done | J1939 SPN encoding (temp, pressure, humidity) |
| `sensor_convert.cnx` | 93 | ✅ Done | Sensor physics (Steinhart-Hart, pressure) |
| `spn_check.cnx` | 83 | ✅ Done | SPN enable checking (MISRA 13.5 compliant) |
| `presets.cnx` | 119 | ✅ Done | NTC and pressure sensor preset lookups |
| **Total Converted** | **442** | | |

## Files Modified

| File | Changes |
|------|---------|
| `src/Display/J1939Bus.cpp` | Uses `j1939_encode_*` and `spn_check_*` |
| `src/Domain/SensorProcessor/SensorProcessor.cpp` | Uses `sensor_convert_*` |
| `src/Data/ConfigStorage/ConfigStorage.cpp` | Uses `crc32_calculateChecksum` |
| `src/Domain/CommandHandler/CommandHandler.cpp` | Uses `presets_*` for NTC and pressure presets |

## Next Candidates

1. `updateAppDataForSpn()` - SPN-to-AppData field mapping
2. CommandHandler preset application logic
3. J1939 message decoding

## Firmware Size

| Metric | Before | After | Delta |
|--------|--------|-------|-------|
| FLASH code | 46.5KB | 46.8KB | +256 bytes |
| RAM1 variables | 22.9KB | 22.9KB | 0 |
| RAM2 variables | 12.4KB | 12.4KB | 0 |

*Note: v0.1.13 reduced code size by ~128 bytes compared to v0.1.12 due to pass-by-value optimization*

---

*Technical details: [docs/static-analysis.md](docs/static-analysis.md) | [docs/c-next-notes.md](docs/c-next-notes.md)*
