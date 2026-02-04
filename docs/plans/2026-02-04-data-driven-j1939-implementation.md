# Data-Driven J1939 Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Replace hardcoded J1939 encoding with config-driven approach where one sensor value can map to multiple SPNs.

**Architecture:** Introduce `EValueId` enum for physical locations, `TSpnConfig[]` for SPN encoding parameters, and generic `encode()`/`sendPgn()` functions. Existing `AppData` struct remains for backward compatibility during migration.

**Tech Stack:** C-Next, FlexCAN_T4, J1939 protocol

---

## Phase 1: Foundation Types

### Task 1: Create EValueId enum

**Files:**
- Create: `src/Data/types/EValueId.cnx`

**Step 1: Create the enum file**

```c-next
// Physical measurement locations
// Named by position in air/exhaust path, not by J1939 SPN names

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
    TURBO1_TURB_INLET_TEMP,   // EGT - turbine inlet

    // ─── Charge Air Cooler 1 ───
    CAC1_INLET_PRES,
    CAC1_INLET_TEMP,
    CAC1_OUTLET_PRES,
    CAC1_OUTLET_TEMP,

    // ─── Intake Manifold 1 ───
    MANIFOLD1_ABS_PRES,
    MANIFOLD1_TEMP,

    // ─── Engine fluids ───
    OIL_PRES,
    OIL_TEMP,
    COOLANT_PRES,
    COOLANT_TEMP,
    FUEL_PRES,
    FUEL_TEMP,

    // ─── Engine bay ───
    ENGINE_BAY_TEMP,

    // Sentinel for array sizing
    VALUE_ID_COUNT
}
```

**Step 2: Build to verify syntax**

Run: `pio run 2>&1 | head -20`
Expected: Transpilation succeeds (file not yet included anywhere)

**Step 3: Commit**

```bash
git add src/Data/types/EValueId.cnx
git commit -m "feat: add EValueId enum for physical measurement locations"
```

---

### Task 2: Create TSpnConfig struct

**Files:**
- Create: `src/Data/types/TSpnConfig.cnx`

**Step 1: Create the struct file**

```c-next
// SPN encoding configuration
// Maps J1939 SPNs to physical values with encoding parameters

#include "EValueId.cnx"

struct TSpnConfig {
    u16 spn;              // SPN number (e.g., 4817)
    u16 pgn;              // Which PGN this SPN belongs to
    u8 bytePos;           // Start byte in PGN (1-indexed per J1939 docs)
    u8 length;            // Data length: 1 or 2 bytes
    f32 resolution;       // kPa/bit, °C/bit, etc.
    f32 offset;           // Added before scaling (e.g., +40 for temp)
    EValueId source;      // Which value to encode
}
```

**Step 2: Build to verify syntax**

Run: `pio run 2>&1 | head -20`
Expected: Transpilation succeeds

**Step 3: Commit**

```bash
git add src/Data/types/TSpnConfig.cnx
git commit -m "feat: add TSpnConfig struct for SPN encoding parameters"
```

---

### Task 3: Create TPgnConfig struct

**Files:**
- Create: `src/Data/types/TPgnConfig.cnx`

**Step 1: Create the struct file**

```c-next
// PGN transmission configuration
// Defines PGN timing and properties per J1939 spec

struct TPgnConfig {
    u16 pgn;             // PGN number
    u16 intervalMs;      // Transmission interval (0 = on request only)
    u8 dataLength;       // Usually 8
    u8 priority;         // Default priority (usually 6)
}
```

**Step 2: Build to verify syntax**

Run: `pio run 2>&1 | head -20`
Expected: Transpilation succeeds

**Step 3: Commit**

```bash
git add src/Data/types/TPgnConfig.cnx
git commit -m "feat: add TPgnConfig struct for PGN timing"
```

---

## Phase 2: Config Tables

### Task 4: Create SPN config table

**Files:**
- Create: `src/Data/J1939Config.cnx`

**Step 1: Create config file with SPN table**

```c-next
// J1939 Configuration Tables
// SPN and PGN definitions for data-driven encoding

#include "types/EValueId.cnx"
#include "types/TSpnConfig.cnx"
#include "types/TPgnConfig.cnx"

// SPN encoding configurations
// Each entry maps a J1939 SPN to a physical value with encoding parameters
const TSpnConfig SPN_CONFIGS[] <- [
    // ─── PGN 65269 - Ambient Conditions ───
    // SPN 108 - Barometric Pressure (0.5 kPa/bit)
    { spn: 108, pgn: 65269, bytePos: 1, length: 1, resolution: 0.5, offset: 0.0, source: EValueId.AMBIENT_PRES },
    // SPN 171 - Ambient Air Temperature (0.03125°C/bit, +273 offset)
    { spn: 171, pgn: 65269, bytePos: 4, length: 2, resolution: 0.03125, offset: 273.0, source: EValueId.AMBIENT_TEMP },
    // SPN 172 - Air Inlet Temperature (1°C/bit, +40 offset)
    { spn: 172, pgn: 65269, bytePos: 6, length: 1, resolution: 1.0, offset: 40.0, source: EValueId.CAC1_OUTLET_TEMP },

    // ─── PGN 65270 - Inlet/Exhaust Conditions 1 ───
    // SPN 102 - Boost Pressure (2 kPa/bit) - legacy, uses manifold pressure
    { spn: 102, pgn: 65270, bytePos: 2, length: 1, resolution: 2.0, offset: 0.0, source: EValueId.MANIFOLD1_ABS_PRES },
    // SPN 105 - Intake Manifold Temperature (1°C/bit, +40 offset)
    { spn: 105, pgn: 65270, bytePos: 3, length: 1, resolution: 1.0, offset: 40.0, source: EValueId.MANIFOLD1_TEMP },
    // SPN 106 - Air Inlet Pressure (2 kPa/bit)
    { spn: 106, pgn: 65270, bytePos: 4, length: 1, resolution: 2.0, offset: 0.0, source: EValueId.TURBO1_COMP_INLET_PRES },
    // SPN 173 - Exhaust Gas Temperature (0.03125°C/bit, +273 offset)
    { spn: 173, pgn: 65270, bytePos: 6, length: 2, resolution: 0.03125, offset: 273.0, source: EValueId.TURBO1_TURB_INLET_TEMP },

    // ─── PGN 65262 - Engine Temperature 1 ───
    // SPN 110 - Coolant Temperature (1°C/bit, +40 offset)
    { spn: 110, pgn: 65262, bytePos: 1, length: 1, resolution: 1.0, offset: 40.0, source: EValueId.COOLANT_TEMP },
    // SPN 174 - Fuel Temperature (1°C/bit, +40 offset)
    { spn: 174, pgn: 65262, bytePos: 2, length: 1, resolution: 1.0, offset: 40.0, source: EValueId.FUEL_TEMP },
    // SPN 175 - Oil Temperature (1°C/bit, +40 offset)
    { spn: 175, pgn: 65262, bytePos: 3, length: 1, resolution: 1.0, offset: 40.0, source: EValueId.OIL_TEMP },

    // ─── PGN 65263 - Engine Fluid Level/Pressure 1 ───
    // SPN 94 - Fuel Delivery Pressure (4 kPa/bit)
    { spn: 94, pgn: 65263, bytePos: 1, length: 1, resolution: 4.0, offset: 0.0, source: EValueId.FUEL_PRES },
    // SPN 100 - Oil Pressure (4 kPa/bit)
    { spn: 100, pgn: 65263, bytePos: 4, length: 1, resolution: 4.0, offset: 0.0, source: EValueId.OIL_PRES },
    // SPN 109 - Coolant Pressure (2 kPa/bit)
    { spn: 109, pgn: 65263, bytePos: 7, length: 1, resolution: 2.0, offset: 0.0, source: EValueId.COOLANT_PRES },

    // ─── PGN 65164 - Engine Temperature 3 ───
    // SPN 441 - Engine Bay Temperature (1°C/bit, +40 offset)
    { spn: 441, pgn: 65164, bytePos: 1, length: 1, resolution: 1.0, offset: 40.0, source: EValueId.ENGINE_BAY_TEMP },
    // SPN 354 - Relative Humidity (0.4%/bit)
    { spn: 354, pgn: 65164, bytePos: 7, length: 1, resolution: 0.4, offset: 0.0, source: EValueId.AMBIENT_HUMIDITY },

    // ─── PGN 65190 - Turbocharger Information 5 ───
    // SPN 1127 - Turbo 1 Compressor Outlet Pressure (0.125 kPa/bit)
    { spn: 1127, pgn: 65190, bytePos: 1, length: 2, resolution: 0.125, offset: 0.0, source: EValueId.TURBO1_COMP_OUTLET_PRES },

    // ─── PGN 65129 - Engine Temperature 2 ───
    // SPN 1363 - Intake Manifold 1 Temperature High Resolution (0.03125°C/bit, +273)
    { spn: 1363, pgn: 65129, bytePos: 1, length: 2, resolution: 0.03125, offset: 273.0, source: EValueId.MANIFOLD1_TEMP },
    // SPN 1637 - Coolant Temperature High Resolution (0.03125°C/bit, +273)
    { spn: 1637, pgn: 65129, bytePos: 3, length: 2, resolution: 0.03125, offset: 273.0, source: EValueId.COOLANT_TEMP },

    // ─── PGN 65189 - Turbocharger Information 4 ───
    // SPN 1131 - Turbo 1 Compressor Inlet Temperature (1°C/bit, +40)
    { spn: 1131, pgn: 65189, bytePos: 1, length: 1, resolution: 1.0, offset: 40.0, source: EValueId.TURBO1_COMP_INLET_TEMP },
    // SPN 1132 - Turbo 1 Compressor Outlet Temperature (1°C/bit, +40)
    { spn: 1132, pgn: 65189, bytePos: 2, length: 1, resolution: 1.0, offset: 40.0, source: EValueId.TURBO1_COMP_OUTLET_TEMP }
];

const u8 SPN_CONFIG_COUNT <- 20;

// PGN transmission configurations
const TPgnConfig PGN_CONFIGS[] <- [
    { pgn: 65129, intervalMs: 1000, dataLength: 8, priority: 6 },  // Engine Temperature 2
    { pgn: 65164, intervalMs: 1000, dataLength: 8, priority: 6 },  // Engine Temperature 3
    { pgn: 65189, intervalMs: 500, dataLength: 8, priority: 6 },   // Turbocharger Info 4
    { pgn: 65190, intervalMs: 500, dataLength: 8, priority: 6 },   // Turbocharger Info 5
    { pgn: 65262, intervalMs: 1000, dataLength: 8, priority: 6 },  // Engine Temperature 1
    { pgn: 65263, intervalMs: 500, dataLength: 8, priority: 6 },   // Engine Fluid Level/Pressure 1
    { pgn: 65269, intervalMs: 1000, dataLength: 8, priority: 6 },  // Ambient Conditions
    { pgn: 65270, intervalMs: 500, dataLength: 8, priority: 6 }    // Inlet/Exhaust Conditions 1
];

const u8 PGN_CONFIG_COUNT <- 8;
```

**Step 2: Build to verify syntax**

Run: `pio run 2>&1 | head -30`
Expected: Transpilation succeeds

**Step 3: Commit**

```bash
git add src/Data/J1939Config.cnx
git commit -m "feat: add J1939 SPN and PGN config tables"
```

---

## Phase 3: Value Storage & Encoder

### Task 5: Create SensorValues storage scope

**Files:**
- Create: `src/Data/SensorValues.cnx`

**Step 1: Create the sensor values storage**

```c-next
// Centralized sensor value storage
// All values stored in standard units (kPa, °C, %)

#include "types/EValueId.cnx"

scope SensorValues {
    // Runtime value storage
    f32 values[EValueId.VALUE_ID_COUNT];

    // Track which values have hardware assigned
    bool hasHardware[EValueId.VALUE_ID_COUNT];

    public void initialize() {
        for (u8 i <- 0; i < EValueId.VALUE_ID_COUNT; i <- i + 1) {
            this.values[i] <- 0.0;
            this.hasHardware[i] <- false;
        }
    }

    public f32 get(EValueId id) {
        return this.values[id];
    }

    public void set(EValueId id, f32 value) {
        this.values[id] <- value;
    }

    public void setHasHardware(EValueId id, bool has) {
        this.hasHardware[id] <- has;
    }

    public bool getHasHardware(EValueId id) {
        return this.hasHardware[id];
    }
}
```

**Step 2: Build to verify**

Run: `pio run 2>&1 | head -20`
Expected: Transpilation succeeds

**Step 3: Commit**

```bash
git add src/Data/SensorValues.cnx
git commit -m "feat: add SensorValues scope for centralized value storage"
```

---

### Task 6: Add generic encode function to J1939Encode

**Files:**
- Modify: `src/Display/J1939Encode.cnx`

**Step 1: Add generic encode function**

Add at line 5 (after scope declaration, before existing functions):

```c-next
    // Generic encoding: (value + offset) / resolution
    // Returns u16 to handle both 1-byte and 2-byte SPNs
    // Clamping is handled by C-Next's default overflow behavior
    public u16 encode(f32 value, f32 resolution, f32 offset) {
        f32 scaled <- (value + offset) / resolution;
        return (u16)scaled;
    }
```

**Step 2: Build to verify**

Run: `pio run 2>&1 | head -20`
Expected: Build succeeds

**Step 3: Commit**

```bash
git add src/Display/J1939Encode.cnx
git commit -m "feat: add generic encode() function to J1939Encode"
```

---

## Phase 4: Generic PGN Sender

### Task 7: Add sendPgnGeneric to J1939Bus

**Files:**
- Modify: `src/Display/J1939Bus.cnx`

**Step 1: Add include for new types at top of file (after line 14)**

```c-next
#include <Data/J1939Config.cnx>
#include <Data/SensorValues.cnx>
```

**Step 2: Add isValueEnabled helper (after fillBuffer function, ~line 39)**

```c-next
    // Check if a value has hardware assigned
    bool isValueEnabled(EValueId valueId) {
        return global.SensorValues.getHasHardware(valueId);
    }
```

**Step 3: Add sendPgnGeneric function (after sendMessage, ~line 52)**

```c-next
    // Generic PGN sender - finds all SPNs for this PGN and encodes them
    public void sendPgnGeneric(u16 pgn) {
        u8 buf[8];
        this.fillBuffer(buf);

        // Find and encode all SPNs belonging to this PGN
        for (u8 i <- 0; i < SPN_CONFIG_COUNT; i <- i + 1) {
            TSpnConfig cfg <- SPN_CONFIGS[i];

            if (cfg.pgn != pgn) {
                continue;
            }

            // Check if this value has hardware
            if (!this.isValueEnabled(cfg.source)) {
                continue;
            }

            // Get source value and encode
            f32 value <- global.SensorValues.get(cfg.source);
            u16 encoded <- global.J1939Encode.encode(value, cfg.resolution, cfg.offset);

            // Place in buffer (bytePos is 1-indexed per J1939 docs)
            u8 pos <- cfg.bytePos - 1;
            buf[pos] <- (u8)(encoded & 0xFF);
            if (cfg.length = 2) {
                buf[pos + 1] <- (u8)((encoded >> 8) & 0xFF);
            }
        }

        this.sendMessage(pgn, buf);
    }
```

**Step 4: Build to verify**

Run: `pio run 2>&1 | head -30`
Expected: Build succeeds

**Step 5: Commit**

```bash
git add src/Display/J1939Bus.cnx
git commit -m "feat: add sendPgnGeneric for config-driven J1939 transmission"
```

---

## Phase 5: Integration

### Task 8: Update ossm.cnx to populate SensorValues

**Files:**
- Modify: `src/Domain/ossm.cnx`

**Step 1: Add include for SensorValues**

Add after existing includes:
```c-next
#include <Data/SensorValues.cnx>
```

**Step 2: Add helper to copy AppData to SensorValues**

Add a new function in the ossm scope:
```c-next
    // Copy current sensor readings to centralized storage
    void updateSensorValues() {
        global.SensorValues.set(EValueId.AMBIENT_PRES, this.appData.absoluteBarometricpressurekPa);
        global.SensorValues.set(EValueId.AMBIENT_TEMP, this.appData.ambientTemperatureC);
        global.SensorValues.set(EValueId.AMBIENT_HUMIDITY, this.appData.humidity);
        global.SensorValues.set(EValueId.OIL_PRES, this.appData.oilPressurekPa);
        global.SensorValues.set(EValueId.OIL_TEMP, this.appData.oilTemperatureC);
        global.SensorValues.set(EValueId.COOLANT_PRES, this.appData.coolantPressurekPa);
        global.SensorValues.set(EValueId.COOLANT_TEMP, this.appData.coolantTemperatureC);
        global.SensorValues.set(EValueId.FUEL_PRES, this.appData.fuelPressurekPa);
        global.SensorValues.set(EValueId.FUEL_TEMP, this.appData.fuelTemperatureC);
        global.SensorValues.set(EValueId.ENGINE_BAY_TEMP, this.appData.engineBayTemperatureC);
        global.SensorValues.set(EValueId.TURBO1_TURB_INLET_TEMP, this.appData.egtTemperatureC);
        global.SensorValues.set(EValueId.TURBO1_COMP_OUTLET_PRES, this.appData.boostPressurekPa);
        global.SensorValues.set(EValueId.TURBO1_COMP_OUTLET_TEMP, this.appData.boostTemperatureC);
        global.SensorValues.set(EValueId.CAC1_INLET_PRES, this.appData.cacInletPressurekPa);
        global.SensorValues.set(EValueId.CAC1_INLET_TEMP, this.appData.cacInletTemperatureC);
        global.SensorValues.set(EValueId.CAC1_OUTLET_TEMP, this.appData.airInletTemperatureC);
        global.SensorValues.set(EValueId.TURBO1_COMP_INLET_PRES, this.appData.airInletPressurekPa);
        global.SensorValues.set(EValueId.MANIFOLD1_ABS_PRES, this.appData.boostPressurekPa);
        global.SensorValues.set(EValueId.MANIFOLD1_TEMP, this.appData.boostTemperatureC);
    }
```

**Step 3: Initialize SensorValues in setup**

In the `setup()` function, add:
```c-next
        global.SensorValues.initialize();
```

**Step 4: Call updateSensorValues before J1939 transmission**

Find where J1939 PGNs are sent and add call to `this.updateSensorValues()` before transmission.

**Step 5: Build and test**

Run: `pio run 2>&1 | head -30`
Expected: Build succeeds

**Step 6: Commit**

```bash
git add src/Domain/ossm.cnx
git commit -m "feat: integrate SensorValues with ossm data flow"
```

---

### Task 9: Add test PGN using generic sender

**Files:**
- Modify: `src/Display/J1939Bus.cnx`

**Step 1: Add test function to use generic sender for one PGN**

Add after existing sendPgn functions:
```c-next
    // Test: Send PGN 65269 using generic encoder
    public void sendPgn65269Generic() {
        this.sendPgnGeneric(65269);
    }
```

**Step 2: Build and upload**

Run: `pio run --target upload`

**Step 3: Test via serial**

Verify PGN 65269 data matches expected encoding.

**Step 4: Commit**

```bash
git add src/Display/J1939Bus.cnx
git commit -m "feat: add test function for generic PGN sender"
```

---

## Phase 6: Migration (Future)

### Task 10: Migrate remaining PGNs to generic sender

For each existing `sendPgnXXXXX` function:
1. Verify SPN config table entries match current encoding
2. Replace hardcoded function call with `sendPgnGeneric(XXXXX)`
3. Test that output matches
4. Remove old function once verified

This task is deferred until the generic sender is validated on hardware.

---

## Verification Checklist

- [ ] EValueId enum compiles
- [ ] TSpnConfig struct compiles
- [ ] TPgnConfig struct compiles
- [ ] J1939Config tables compile
- [ ] SensorValues scope compiles
- [ ] Generic encode function works
- [ ] sendPgnGeneric produces correct J1939 output
- [ ] Hardware test validates PGN 65269 encoding
