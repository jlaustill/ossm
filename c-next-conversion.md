# C-Next Conversion Status

C-Next transpiles to standard C/C++, providing memory safety while generating code compatible with existing toolchains.

**C-Next Version:** 0.1.14

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

**v0.1.14 improvements (issues fixed):**
- ✅ `cstyleCast` - Now generates `static_cast<>()` (#267 FIXED in v0.1.13)
- ✅ `constParameterPointer` - Now uses pass-by-value for small types (#269 FIXED in v0.1.13)
- ✅ Header/impl signature mismatch (#280 FIXED in v0.1.14)
- ✅ const on modified pointer params (#281 FIXED in v0.1.14)
- ⚠️ extern const mismatch - Still requires manual fix (extern declarations missing const)

**Notes:**
- New cppcheck warnings are style issues in C-Next generated code, not safety issues
- Flawfinder/MISRA hits remain in non-converted modules (candidates for future conversion)
- C-Next enforces 27 safety checks at transpile time (see [docs/static-analysis.md](docs/static-analysis.md))

## Conversion Progress

| Module | Lines | Status | Purpose |
|--------|-------|--------|---------|
| `crc32.cnx` | 76 | ✅ Done | CRC32 checksum calculation |
| `j1939_encode.cnx` | 71 | ✅ Done | J1939 SPN encoding (temp, pressure, humidity) |
| `j1939_decode.cnx` | 118 | ✅ Done | J1939 command message parsing |
| `sensor_convert.cnx` | 93 | ✅ Done | Sensor physics (Steinhart-Hart, pressure) |
| `spn_check.cnx` | 83 | ✅ Done | SPN enable checking (MISRA 13.5 compliant) |
| `presets.cnx` | 119 | ✅ Done | NTC and pressure sensor preset lookups |
| `spn_category.cnx` | 90 | ✅ Done | SPN category lookup (replaces KNOWN_SPNS loops) |
| `hardware_map.cnx` | 110 | ✅ Done | ADS1115 device/channel mapping for sensors |
| `byte_utils.cnx` | 120 | ✅ Done | Byte manipulation utilities (endian conversion, bit ops) |
| `float_bytes.cnx` | 96 | ✅ Done | IEEE 754 float byte reconstruction (replaces union punning) |
| **Total Converted** | **976** | | |

## Files Modified

| File | Changes |
|------|---------|
| `src/Display/J1939Bus.cpp` | Uses `j1939_encode_*`, `j1939_decode_*`, `spn_check_*`, and `float_bytes_*` |
| `src/Domain/SensorProcessor/SensorProcessor.cpp` | Uses `sensor_convert_*` and `hardware_map_*` |
| `src/Data/ConfigStorage/ConfigStorage.cpp` | Uses `crc32_calculateChecksum` |
| `src/Data/ADS1115Manager/ADS1115Manager.cpp` | Uses `hardware_map_*` for device lookup |
| `src/Domain/CommandHandler/CommandHandler.cpp` | Uses `presets_*` and `spn_category_*` |
| `src/Interface/SerialCommandHandler/SerialCommandHandler.cpp` | Uses `spn_category_*` for SPN category lookup |

## Next Candidates

1. `updateAppDataForSpn()` - SPN-to-AppData field mapping
2. SPN label lookup (human-readable names)
3. Float byte reconstruction (IEEE 754)

## Firmware Size

| Metric | Before | After | Delta |
|--------|--------|-------|-------|
| FLASH code | 46.5KB | 47.5KB | +896 bytes |
| RAM1 variables | 22.9KB | 21.9KB | -1KB |
| RAM2 variables | 12.4KB | 12.4KB | 0 |

*10 C-Next modules now provide memory-safe implementations of core functionality*

---

*Technical details: [docs/static-analysis.md](docs/static-analysis.md) | [docs/c-next-notes.md](docs/c-next-notes.md)*
