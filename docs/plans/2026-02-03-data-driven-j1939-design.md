# Data-Driven J1939 Encoding Design

## Overview

Replace hardcoded J1939 encoding functions with a config-driven approach. One sensor value can map to multiple SPNs with different resolutions. Adding new SPNs/PGNs requires only config entries, no new code.

## Section 1: Value IDs

Physical measurement locations, named unambiguously by position in the air/exhaust path:

```c-next
enum EValueId {
    // ─── Ambient (atmospheric reference) ───
    AMBIENT_PRES,
    AMBIENT_TEMP,
    AMBIENT_HUMIDITY,

    // ─── Turbo 1 ───
    TURBO1_COMP_INLET_PRES,
    TURBO1_COMP_INLET_TEMP,
    TURBO1_COMP_OUTLET_PRES,
    TURBO1_COMP_OUTLET_TEMP,
    TURBO1_TURB_INLET_PRES,
    TURBO1_TURB_INLET_TEMP,
    TURBO1_TURB_OUTLET_PRES,
    TURBO1_TURB_OUTLET_TEMP,

    // ─── Turbo 2 (compound setups) ───
    TURBO2_COMP_INLET_PRES,
    TURBO2_COMP_INLET_TEMP,
    TURBO2_COMP_OUTLET_PRES,
    TURBO2_COMP_OUTLET_TEMP,
    TURBO2_TURB_INLET_PRES,
    TURBO2_TURB_INLET_TEMP,
    TURBO2_TURB_OUTLET_PRES,
    TURBO2_TURB_OUTLET_TEMP,

    // ─── Charge Air Cooler 1 ───
    CAC1_INLET_PRES,
    CAC1_INLET_TEMP,
    CAC1_OUTLET_PRES,
    CAC1_OUTLET_TEMP,

    // ─── Charge Air Cooler 2 ───
    CAC2_INLET_PRES,
    CAC2_INLET_TEMP,
    CAC2_OUTLET_PRES,
    CAC2_OUTLET_TEMP,

    // ─── Intake Manifold 1 ───
    MANIFOLD1_ABS_PRES,
    MANIFOLD1_TEMP,

    // ─── Intake Manifold 2 (V-engine) ───
    MANIFOLD2_ABS_PRES,
    MANIFOLD2_TEMP,

    // ─── Engine fluids ───
    OIL_PRES,
    OIL_TEMP,
    COOLANT_PRES,
    COOLANT_TEMP,
    FUEL_PRES,
    FUEL_TEMP
}

// Runtime value storage (all pressures in kPa, temps in °C, humidity in %)
f32 sensorValues[EValueId.length];
```

## Section 2: SPN Config

Each SPN defines its encoding parameters and which value it sources:

```c-next
struct TSpnConfig {
    u16 spn;              // SPN number (e.g., 4817)
    u32 pgn;              // Which PGN this SPN belongs to (e.g., 64976)
    u8 bytePos;           // Start byte in PGN (1-indexed per J1939 docs)
    u8 length;            // Data length: 1 or 2 bytes
    f32 resolution;       // kPa/bit, °C/bit, etc.
    f32 offset;           // Added before scaling (e.g., +40 for temp)
    EValueId source;      // Which value to encode
}

// Example configs (const, lives in flash)
const TSpnConfig SPN_CONFIGS[] <- [
    // SPN 4817 - Engine Intake Manifold #1 Absolute Pressure (High Resolution)
    { spn: 4817, pgn: 64976, bytePos: 6, length: 2, resolution: 0.1, offset: 0, source: MANIFOLD1_ABS_PRES },

    // SPN 102 - Boost Pressure (legacy, lower resolution)
    { spn: 102, pgn: 65270, bytePos: 2, length: 1, resolution: 2.0, offset: 0, source: MANIFOLD1_ABS_PRES },

    // SPN 1127 - Turbocharger 1 Compressor Outlet Pressure
    { spn: 1127, pgn: 65190, bytePos: 1, length: 2, resolution: 0.125, offset: 0, source: TURBO1_COMP_OUTLET_PRES },

    // SPN 175 - Oil Temperature
    { spn: 175, pgn: 65262, bytePos: 4, length: 1, resolution: 1.0, offset: 40, source: OIL_TEMP },

    // ... more SPNs
];
```

Note: `MANIFOLD1_ABS_PRES` is used by both SPN 4817 (high-res) and SPN 102 (legacy) - same value, different encodings.

## Section 3: Generic Encoder

Single encoding function driven by config:

```c-next
scope J1939Encode {
    // Generic encoding: (value + offset) / resolution
    // Returns u16 to handle both 1-byte and 2-byte SPNs
    public u16 encode(f32 value, f32 resolution, f32 offset) {
        f32 scaled <- (value + offset) / resolution;
        return (u16)scaled;  // Will clamp properly once C-Next #632 is fixed
    }

    // Encode an SPN using its config
    public u16 encodeSpn(const TSpnConfig config, f32 value) {
        return this.encode(value, config.resolution, config.offset);
    }
}
```

## Section 4: Generic PGN Sender

One function sends any PGN by finding its SPNs in config:

```c-next
scope J1939Bus {
    // Send any PGN by number - finds all SPNs for this PGN and encodes them
    public void sendPgn(u32 pgn) {
        u8 buf[8];
        this.fillBuffer(buf);  // Fill with 0xFF (J1939 "not available")

        // Find and encode all SPNs belonging to this PGN
        for (u8 i <- 0; i < SPN_CONFIGS.length; i <- i + 1) {
            if (SPN_CONFIGS[i].pgn != pgn) {
                continue;
            }

            TSpnConfig config <- SPN_CONFIGS[i];

            // Check if this SPN is enabled
            if (!this.isSpnEnabled(config.spn)) {
                continue;
            }

            // Get source value and encode
            f32 value <- sensorValues[config.source];
            u16 encoded <- global.J1939Encode.encodeSpn(config, value);

            // Place in buffer (bytePos is 1-indexed per J1939 docs)
            u8 pos <- config.bytePos - 1;
            buf[pos] <- (u8)(encoded & 0xFF);
            if (config.length = 2) {
                buf[pos + 1] <- (u8)((encoded >> 8) & 0xFF);
            }
        }

        this.sendMessage(pgn, buf);
    }
}
```

## Section 5: Hardware Input to Value Mapping

Users assign physical inputs to values. SPNs are enabled implicitly when their source value has hardware:

```c-next
// Input configuration - maps physical input to a value
struct TPressureInputConfig {
    EValueId assignedValue;   // Which value this input feeds (or -1 if disabled)
    u16 maxPressure;          // Sensor range
    EPressureType pressureType;
}

struct TTempInputConfig {
    EValueId assignedValue;   // Which value this input feeds (or -1 if disabled)
    f32 coeffA;               // Steinhart-Hart coefficients
    f32 coeffB;
    f32 coeffC;
    f32 resistorValue;
}

// Track which values have hardware assigned
bool valueHasHardware[EValueId.length];

// During init, mark values that have hardware
void initValueHardwareMap(const AppConfig config) {
    for (u8 i <- 0; i < PRESSURE_INPUT_COUNT; i <- i + 1) {
        if (config.pressureInputs[i].assignedValue >= 0) {
            valueHasHardware[config.pressureInputs[i].assignedValue] <- true;
        }
    }
    for (u8 i <- 0; i < TEMP_INPUT_COUNT; i <- i + 1) {
        if (config.tempInputs[i].assignedValue >= 0) {
            valueHasHardware[config.tempInputs[i].assignedValue] <- true;
        }
    }
    // BME280 provides these when enabled
    if (config.bme280Enabled) {
        valueHasHardware[AMBIENT_PRES] <- true;
        valueHasHardware[AMBIENT_TEMP] <- true;
        valueHasHardware[AMBIENT_HUMIDITY] <- true;
    }
}

// Check if SPN is enabled (its source value has hardware)
bool isSpnEnabled(u16 spn) {
    for (u8 i <- 0; i < SPN_CONFIGS.length; i <- i + 1) {
        if (SPN_CONFIGS[i].spn = spn) {
            return valueHasHardware[SPN_CONFIGS[i].source];
        }
    }
    return false;
}
```

## Section 6: PGN Config

PGN definitions with J1939-specified transmission rates:

```c-next
struct TPgnConfig {
    u32 pgn;
    u16 intervalMs;      // From J1939 spec (0 = on request only)
    u8 dataLength;       // Usually 8, but spec defines it
    u8 priority;         // Default priority from spec
}

const TPgnConfig PGN_CONFIGS[] <- [
    // PGN 65270 - Inlet/Exhaust Conditions 1
    { pgn: 65270, intervalMs: 500, dataLength: 8, priority: 6 },

    // PGN 64976 - Inlet/Exhaust Conditions 2
    { pgn: 64976, intervalMs: 500, dataLength: 8, priority: 6 },

    // PGN 65263 - Engine Fluid Level/Pressure 1
    { pgn: 65263, intervalMs: 500, dataLength: 8, priority: 6 },

    // PGN 65269 - Ambient Conditions
    { pgn: 65269, intervalMs: 1000, dataLength: 8, priority: 6 },

    // PGN 65262 - Engine Temperature 1
    { pgn: 65262, intervalMs: 1000, dataLength: 8, priority: 6 },

    // ... more PGNs with their J1939-defined rates
]

// Runtime tracking
u32 lastSendTime[PGN_CONFIGS.length];

public void updateJ1939() {
    u32 now <- millis();

    for (u8 i <- 0; i < PGN_CONFIGS.length; i <- i + 1) {
        if (PGN_CONFIGS[i].intervalMs = 0) {
            continue;  // On-request only
        }

        u32 elapsed <- now - lastSendTime[i];
        if (elapsed >= PGN_CONFIGS[i].intervalMs) {
            this.sendPgn(PGN_CONFIGS[i].pgn);
            lastSendTime[i] <- now;
        }
    }
}
```

## Section 7: Serial Command Interface

Commands change from SPN-based to value-based:

```c-next
// Old: 1,spnHi,spnLo,enable,input
//      "Enable SPN 4817 on pres1"

// New: 1,inputType,inputNum,valueId
//      "Assign pres1 to MANIFOLD1_ABS_PRES"

// inputType: 0 = pressure, 1 = temp
// inputNum: 1-7 for pressure, 1-8 for temp
// valueId: enum value from EValueId

// Example: Assign pres1 to MANIFOLD1_ABS_PRES
// 1,0,1,28   (28 = MANIFOLD1_ABS_PRES enum value)

// Query command shows which values have hardware:
// "=== Assigned Values ==="
// "pres1: MANIFOLD1_ABS_PRES"
// "temp1: OIL_TEMP"
// "BME280: AMBIENT_PRES, AMBIENT_TEMP, AMBIENT_HUMIDITY"
//
// "=== Active SPNs (auto-enabled) ==="
// "SPN 4817 (MANIFOLD1_ABS_PRES) -> PGN 64976"
// "SPN 102 (MANIFOLD1_ABS_PRES) -> PGN 65270"
// "SPN 175 (OIL_TEMP) -> PGN 65262"
```

## Data Flow Summary

1. User assigns `pres1 → MANIFOLD1_ABS_PRES`
2. `valueHasHardware[MANIFOLD1_ABS_PRES] = true`
3. Any SPN with `source: MANIFOLD1_ABS_PRES` is now active
4. `updateJ1939()` iterates PGN_CONFIGS, calls `sendPgn()` at specified intervals
5. `sendPgn()` finds all active SPNs for that PGN, encodes using config, places in buffer

## Benefits

- One value, multiple SPN outputs (different resolutions/PGNs)
- Adding SPNs = add config entry, no new code
- Adding PGNs = add config entry, no new function
- J1939 spec details live in config, not scattered in code
- Cleaner user experience: assign inputs to named locations, system handles the rest
- Future-ready for OBD2 PIDs using same pattern

## Dependencies

- ~~Blocked on C-Next issue #632 (u8 clamp on cast) for safe encoding~~ **RESOLVED in v0.1.55**

## Status

**Ready for implementation** - All blockers resolved as of 2026-02-04.
