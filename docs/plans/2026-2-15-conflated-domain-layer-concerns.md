# Conflated Domain Layer Concerns

**Date:** 2026-02-15
**Status:** Analysis Complete
**Target:** Domain layer refactoring

## Overview

This document identifies files in the Domain layer (`src/Domain/`) that conflate multiple concerns, based on architectural review.

## Files Analyzed

### Well-Focused (No Refactoring Needed)

| File                         | Primary Responsibility                                      | Notes                                                               |
|------------------------------|-------------------------------------------------------------|---------------------------------------------------------------------|
| `ossm.cnx`                   | Main orchestrator                                           | setup/loop with timing audit, delegates to subsystems              |
| `Hardware.cnx`               | Hardware initialization                                     | Sets hardware flags, initializes drivers                           |
| `SensorProcessor.cnx`        | Sensor polling                                              | Reads ADC/thermocouple/BME280, stores to SensorValues              |
| `CommandHandler.cnx`         | Unified command processing                                  | EValueId dispatch, no transport knowledge                          |
| `J1939CommandHandler.cnx`    | CAN transport                                               | Polls CAN buffer, forwards to CommandHandler                       |
| `SerialCommandHandler.cnx`   | Serial transport                                            | Parses serial text, forwards to CommandHandler                     |

---

### Conflated Files Requiring Refactoring

#### 1. `ossm.cnx` - Medium Priority

**Current Responsibilities:**
- Main entry point (setup/loop)
- **Timing audit code** (maxSensor, maxSerial, maxJ1939, maxTotal)
- **USB serial wait logic** (3-second timeout)
- Subsystem initialization orchestration
- Subsystem update calls

**Code Locations:**

| Lines | Function              | Concern                           |
|-------|-----------------------|-----------------------------------|
| 18-23 | Timing audit variables | **Debug instrumentation**        |
| 30-35 | USB serial wait       | **Startup convenience (debug)**  |
| 53-91 | Main loop timing      | **Debug timing audit**           |

**Issues Identified:**

1. **Timing audit code is debug instrumentation that stays in release**
   - `maxSensor`, `maxSerial`, `maxJ1939`, `maxTotal` - only used for debugging
   - `reportTimer` - debug timing interval
   - This bloats the main loop for infrequently-used debug info

2. **USB serial wait is convenient for development but should be configurable**
   - Lines 30-35: 3-second wait for USB serial
   - Should be removed for production or made conditional

**Recommended Resolution:**

Option A (preferred): Move timing audit to conditional debug block
```c-next
#ifdef DEBUG_TIMING
    elapsedMicros loopTimer;
    const u32 LOOP_INTERVAL_US <- 500;  // 500μs (2kHz)
    elapsedMillis reportTimer;
    u32 maxSensor <- 0;
    u32 maxSerial <- 0;
    u32 maxJ1939 <- 0;
    u32 maxTotal <- 0;

    public void loop() {
        // timing audit code here
    }
#endif
```

Option B: Create a separate `DebugMonitor.cnx` module

Option C: Move timing audit to a separate debug-only transport (serial-only debug PGN)

---

#### 2. `CommandHandler.cnx` - Medium Priority

**Current Responsibilities:**
- Command dispatch (enable/disable/set/apply commands)
- **Value categorization** (`getValueCategory`)
- **Hardware re-initialization** after config changes
- **Presets integration** (NTC, pressure)
- EEPROM save/reset operations

**Code Locations:**

| Lines      | Function             | Concern                            |
|------------|----------------------|------------------------------------|
| 34-63      | `getValueCategory`   | **Value categorization logic**    |
| 111, 149   | Hardware.initialize  | **Hardware re-init after each command** |
| 182-207    | NTC/pressure presets | **Presets application**           |

**Issues Identified:**

1. **Hardware re-initialization after every config change is inefficient**
   - Lines 111, 149: `Hardware.initialize(appConfig)` called for every command
   - Each call re-scans all config values and re-initializes all drivers
   - Should batch changes and re-init once

2. **`getValueCategory` is essentially a switch statement that could be data-driven**
   - Lines 34-63: Long switch statement categorizing EValueId values
   - If new values are added, must update this function
   - Could be derived from EValueId enum or config tables

3. **Mix of domain logic and side effects**
   - Some functions modify config directly (`enableValue`, `disableValue`)
   - Some functions trigger hardware re-init as side effect
   - This makes testing difficult (hard to isolate command processing)

**Recommended Resolution:**

Option A (preferred): Split into two modules
```c-next
// Domain/CommandProcessor.cnx - pure command processing
scope CommandProcessor {
    ECommandResult process(const u8[8] data);
    ECommandResult enableValue(const u8[8] data);
    ECommandResult disableValue(const u8[8] data);
    // ... all processing functions
    // NO hardware re-init, NO save/load
}

// Domain/CommandExecutor.cnx - side effects
scope CommandExecutor {
    void applyChanges();  // Call after CommandProcessor
    void saveConfig();
}
```

Option B: Add a "dirty" flag system
```c-next
scope CommandHandler {
    bool configDirty <- false;

    ECommandResult process(const u8[8] data) {
        // ... process command, set configDirty = true
    }

    void commit() {
        if (configDirty) {
            Hardware.initialize(appConfig);
            ConfigStorage.saveConfig(appConfig);
            configDirty <- false;
        }
    }
}
```

---

#### 3. `J1939CommandHandler.cnx` - High Priority

**Current Responsibilities:**
- Outbound PGN scheduling (fixed intervals)
- Inbound CAN message polling
- Command response formatting
- **Query handling** (value counts, value assignments, config)
- **NTC parameter decoding** (float bytes from CAN)

**Issues Identified:**

1. **Query handling duplicates SerialCommandHandler**
   - Lines 68-139: Query handling (same as SerialCommandHandler)
   - Query types 0, 1, 2, 4 duplicated
   - Should be in CommandHandler or a shared module

2. **Float parsing is transport-specific logic**
   - Line 146: `FloatBytes.fromBytesLE(...)` for NTC param
   - This is transport/encoding knowledge
   - Should be in Display layer or abstracted

3. **Hardcoded PGN intervals are configuration**
   - Lines 18-37: 500ms vs 1000ms intervals
   - PGN 65190, 65189, 65263 at 500ms
   - PGN 65262, 65269 at 1000ms
   - Should be data-driven (use TPgnConfig)

4. **`sendScheduledPgns` mixes scheduling with PGN selection**
   - Logic about which PGNs to send when
   - Should be driven by PGN_CONFIGS table

**Recommended Resolution:**

Split `J1939CommandHandler.cnx`:
```c-next
// Domain/J1939QueryHandler.cnx - query processing
scope J1939QueryHandler {
    void handleQuery(const u8[8] data);
    void handleNtcParam(const u8[8] data);
    void processCommand(const u8[8] data);
    // Move query handling here
}

// Domain/J1939Scheduler.cnx - PGN scheduling
scope J1939Scheduler {
    void sendScheduledPgns();
    // Use TPgnConfig for intervals, not hardcoded
}

// Domain/J1939CommandHandler.cnx - thin transport
scope J1939CommandHandler {
    public void update() {
        J1939Scheduler.sendScheduledPgns();
        J1939QueryHandler.update();
    }
}
```

---

#### 4. `SerialCommandHandler.cnx` - High Priority

**Current Responsibilities:**
- Serial text parsing (SeaDash.Parse)
- Command dispatch (forward to CommandHandler)
- **Query handling** (same as J1939)
- **Sensor reading output** (print enabled values, live values)
- **EEPROM dump** (diagnostic)
- **AD1115 debug output** (printDebugInfo)
- **Fault reporting** (EGT fault decoding)

**Issues Identified:**

1. **Query handling duplicated with J1939CommandHandler**
   - Lines 74-108: `handleQuery()` (same as J1939)
   - Query types 0, 4 duplicated
   - Should be in CommandHandler or shared module

2. **Sensor reading output mixes display with command handling**
   - Lines 112-224: `handleReadSensors()` prints sensor values
   - This is diagnostic/debug output
   - Should be in a separate DebugHandler module

3. **EEPROM dump is pure diagnostics**
   - Lines 265-294: `handleDumpEeprom()`
   - Should be in DebugHandler module

4. **AD1115 debug info is hardware-specific**
   - Line 317: `ADS1115Manager.printDebugInfo()`
   - Should be in DebugHandler module

5. **Fault reporting is diagnostic, not command processing**
   - Lines 229-263: `reportFaults()`
   - Called after every command (line 330)
   - Should be optional debug output

**Recommended Resolution:**

Split `SerialCommandHandler.cnx`:
```c-next
// Domain/SerialTransport.cnx - parse serial, forward commands
scope SerialTransport {
    public void update();
    // Parse text, convert to u8[8], call CommandHandler.process
}

// Domain/DebugHandler.cnx - diagnostic output
scope DebugHandler {
    void printEnabledValues();
    void printLiveSensors();
    void dumpEeprom();
    void reportFaults();
    // Move all diagnostic output here
}

// Domain/SerialCommandHandler.cnx - thin transport
scope SerialCommandHandler {
    public void update() {
        SerialTransport.update();
        // Optional: DebugHandler.reportFaults() after commands
    }
}
```

---

## Architecture Concerns

### Cross-Cutting Concerns

| Concern                    | Found In                                           | Issue                               |
|----------------------------|----------------------------------------------------|-------------------------------------|
| Query handling             | J1939CommandHandler, SerialCommandHandler          | Duplicated logic                    |
| Diagnostics/debug output   | SerialCommandHandler, J1939CommandHandler          | Mixed with command processing       |
| Fault reporting            | SerialCommandHandler                               | Called after every command          |
| PGN scheduling             | J1939CommandHandler                                | Hardcoded instead of data-driven    |

### Layering

```
Domain layer imports Display layer:
  CommandHandler.cnx → Display/Presets.cnx (Acceptable - presets are data)
  CommandHandler.cnx → Display/InputValid.cnx (Acceptable - validation)
  SerialCommandHandler.cnx → Display/FaultDecode.cnx (Acceptable - fault decoding)
  SerialCommandHandler.cnx → Display/ValueName.cnx (Acceptable - display names)
  SerialCommandHandler.cnx → Display/FloatBytes.cnx (Questionable - transport encoding)
  J1939CommandHandler.cnx → Display/FloatBytes.cnx (Questionable - transport encoding)

Domain layer imports Data layer:
  All files properly use Data layer
```

---

## Refactoring Priority

| Priority | File                             | Effort | Impact                            |
|----------|----------------------------------|--------|-----------------------------------|
| High     | `J1939CommandHandler.cnx`        | High   | DRY, data-driven scheduling       |
| High     | `SerialCommandHandler.cnx`       | High   | DRY, separating diagnostics       |
| Medium   | `CommandHandler.cnx`             | Medium | Batch config changes, clean separation |
| Low      | `ossm.cnx`                       | Low    | Remove debug code from release    |

---

## Implementation Plan

### Phase 1: Eliminate Duplicates

1. Extract `CommandHandler.cnx`:
   - `handleQuery()` - move query handling
   - `handleNtcParam()` - move NTC param handling

2. Create `src/Domain/SharedQueryHandler.cnx`:
   - `handleQuery()` - shared query logic
   - `handleNtcParam()` - shared NTC param

3. Update `J1939CommandHandler.cnx` and `SerialCommandHandler.cnx` to use SharedQueryHandler

### Phase 2: Separate Diagnostics

1. Create `src/Domain/DebugHandler.cnx`:
   - `printEnabledValues()`
   - `printLiveSensors()`
   - `dumpEeprom()`
   - `reportFaults()`
   - `printADS1115Debug()`

2. Update `SerialCommandHandler.cnx` to delegate to DebugHandler

### Phase 3: Data-Driven Scheduling (J1939)

1. Use `PGN_CONFIGS` table for scheduling instead of hardcoded intervals
2. Create `J1939Scheduler` module

### Phase 4: Batch Config Changes (CommandHandler)

1. Add `configDirty` flag
2. Remove `Hardware.initialize()` from individual commands
3. Add `applyChanges()` method

### Phase 5: Conditional Debug (ossm)

1. Wrap timing audit in `#ifdef DEBUG_TIMING`
2. Or create `DebugMonitor.cnx` module

---

## Related Documentation

- Display layer analysis: `docs/plans/2026-2-15-conflated-display-concerns.md`
- Data layer analysis: `docs/plans/2026-2-15-conflated-data-layer-concerns.md`
- Architecture: `docs/ARCHITECTURE.md`
