# OSSM Serial Command Reference

Complete reference for configuring OSSM via USB serial interface.

## Connection

- **Baud rate**: 115200
- **Data bits**: 8
- **Parity**: None
- **Stop bits**: 1

## Command Format

Commands use **byte format** - all values are 0-255. Multi-byte values (PSI) are sent as `highByte,lowByte`.

## Command Summary

| Cmd | Name | Format | Description |
|-----|------|--------|-------------|
| 1 | Assign Input to Value | `1,inputType,inputNum,valueId` | Assign a sensor input to a physical value |
| 3 | Set Pressure Range | `3,input,psiHi,psiLo` | Set pressure sensor max PSI |
| 4 | Set TC Type | `4,type` | Set thermocouple type (0-7) |
| 5 | Query Config | `5,query_type` | Query configuration |
| 6 | Save | `6` | Save configuration to EEPROM |
| 7 | Reset | `7` | Reset to factory defaults |
| 8 | NTC Preset | `8,input,preset` | Apply NTC sensor preset |
| 9 | Pressure Preset | `9,input,preset` | Apply pressure sensor preset |
| 10 | Read Sensors | `10,type` | Read live sensor values |

> **Note:** Command 2 (Set NTC Param) was removed. Use Command 8 (NTC Preset) instead.

---

## Command Details

### Command 1: Assign Input to Value

```
1,inputType,inputNum,valueId
```

| Parameter | Description |
|-----------|-------------|
| inputType | 0 = temperature input, 1 = pressure input |
| inputNum | 1-8 for temp, 1-7 for pressure |
| valueId | EValueId enum value (see table below) |

To disable an input, use `valueId = 255` (or any value >= VALUE_ID_COUNT).

**EValueId Values:**

| ID | Name | Description |
|----|------|-------------|
| 0 | AMBIENT_PRES | Ambient/barometric pressure |
| 1 | AMBIENT_TEMP | Ambient air temperature |
| 2 | AMBIENT_HUMIDITY | Relative humidity |
| 3 | TURBO1_COMP_INLET_PRES | Turbo 1 compressor inlet pressure |
| 4 | TURBO1_COMP_INLET_TEMP | Turbo 1 compressor inlet temperature |
| 5 | TURBO1_COMP_OUTLET_PRES | Turbo 1 compressor outlet pressure |
| 6 | TURBO1_COMP_OUTLET_TEMP | Turbo 1 compressor outlet temperature |
| 7 | TURBO1_TURB_INLET_TEMP | EGT - turbo 1 turbine inlet temperature |
| 8 | CAC1_INLET_PRES | Charge air cooler 1 inlet pressure |
| 9 | CAC1_INLET_TEMP | Charge air cooler 1 inlet temperature |
| 10 | CAC1_OUTLET_PRES | Charge air cooler 1 outlet pressure |
| 11 | CAC1_OUTLET_TEMP | Charge air cooler 1 outlet temperature |
| 12 | MANIFOLD1_ABS_PRES | Intake manifold 1 absolute pressure |
| 13 | MANIFOLD1_TEMP | Intake manifold 1 temperature |
| 14 | OIL_PRES | Engine oil pressure |
| 15 | OIL_TEMP | Engine oil temperature |
| 16 | COOLANT_PRES | Coolant pressure |
| 17 | COOLANT_TEMP | Coolant temperature |
| 18 | FUEL_PRES | Fuel delivery pressure |
| 19 | FUEL_TEMP | Fuel temperature |
| 20 | ENGINE_BAY_TEMP | Engine bay ambient temperature |

**Examples:**
```
1,0,3,15     # Assign temp3 to OIL_TEMP (15)
1,0,2,17     # Assign temp2 to COOLANT_TEMP (17)
1,1,1,14     # Assign pres1 to OIL_PRES (14)
1,1,6,12     # Assign pres6 to MANIFOLD1_ABS_PRES (12)
1,0,3,255    # Disable temp3
```

**Auto-enabled SPNs:**

When you assign an input to a value, all SPNs that source from that value are automatically enabled. For example, assigning any input to `MANIFOLD1_ABS_PRES` (12) enables both:
- SPN 102 (Boost Pressure, 2 kPa/bit resolution)
- SPN 4817 (High Resolution Manifold Pressure, 0.1 kPa/bit)

See [SPN Reference](SPN-REFERENCE.md) for the complete mapping of values to SPNs.

---

### Command 3: Set Pressure Range

```
3,input,psiHi,psiLo
```

| Parameter | Description |
|-----------|-------------|
| input | Pressure input 1-7 |
| psiHi | Max PSI high byte (PSI >> 8) |
| psiLo | Max PSI low byte (PSI & 0xFF) |

**Example:** Set pres1 to 150 PSI:
```
3,1,0,150     # 150 = 0x0096
```

---

### Command 4: Set Thermocouple Type

```
4,type
```

| Type | Thermocouple |
|------|--------------|
| 0 | B-type |
| 1 | E-type |
| 2 | J-type |
| 3 | K-type (default) |
| 4 | N-type |
| 5 | R-type |
| 6 | S-type |
| 7 | T-type |

**Example:**
```
4,3    # Set to K-type (default)
```

---

### Command 5: Query Configuration

```
5,query_type
```

| Query Type | Returns |
|------------|---------|
| 0 | All assigned values with input mappings |
| 4 | Full configuration dump |

**Examples:**
```
5,0    # List assigned values
5,4    # Full config dump
```

**Sample Query Output (5,0):**
```
=== Assigned Values ===
temp3: OIL_TEMP
pres1: OIL_PRES
pres6: MANIFOLD1_ABS_PRES
BME280: AMBIENT_PRES, AMBIENT_TEMP, AMBIENT_HUMIDITY

=== Active SPNs (auto-enabled) ===
SPN 175 (OIL_TEMP) -> PGN 65262
SPN 100 (OIL_PRES) -> PGN 65263
SPN 102 (MANIFOLD1_ABS_PRES) -> PGN 65270
SPN 108 (AMBIENT_PRES) -> PGN 65269
SPN 171 (AMBIENT_TEMP) -> PGN 65269
SPN 354 (AMBIENT_HUMIDITY) -> PGN 65164
```

---

### Command 6: Save Configuration

```
6
```

Saves current configuration to EEPROM. Configuration persists across power cycles.

---

### Command 7: Factory Reset

```
7
```

Resets all configuration to factory defaults:
- All SPNs disabled
- Default NTC calibration values
- Default pressure ranges
- J1939 source address reset to 149

---

### Command 8: NTC Preset

```
8,input,preset
```

| Parameter | Description |
|-----------|-------------|
| input | Temperature input 1-8 |
| preset | Preset number (see table) |

| Preset | Sensor |
|--------|--------|
| 0 | AEM 30-2012 |
| 1 | Bosch |
| 2 | GM |

**Example:**
```
8,3,0    # Set temp3 to AEM preset
```

---

### Command 9: Pressure Preset

```
9,input,preset
```

| Parameter | Description |
|-----------|-------------|
| input | Pressure input 1-7 |
| preset | Preset number (see table) |

| Preset | Max PSI |
|--------|---------|
| 0 | 100 PSI |
| 1 | 150 PSI |
| 2 | 200 PSI |

**Example:**
```
9,6,1    # Set pres6 to 150 PSI
```

---

### Command 10: Read Live Sensors

```
10[,sensorType]
```

| Type | Description |
|------|-------------|
| 0 | All active sensors (default) |
| 1 | EGT only |
| 2 | Temperature sensors |
| 3 | Pressure sensors |
| 4 | BME280 ambient |

**Examples:**
```
10        # Read all active sensors
10,1      # Read EGT temperature
10,2      # Read all temperature sensors
10,3      # Read all pressure sensors
10,4      # Read BME280 (ambient temp, humidity, barometric)
```

**Note:** Sensor faults are displayed at the end of ALL command responses.

---

## Quick Start Example

Configure oil temp on temp3 and oil pressure on pres1:

```
# Assign temp3 to OIL_TEMP (15)
1,0,3,15

# Set temp3 to AEM sensor preset
8,3,0

# Assign pres1 to OIL_PRES (14)
1,1,1,14

# Set pres1 to 150 PSI
3,1,0,150

# Save configuration
6

# Verify readings
10
```

This auto-enables SPN 175 (oil temp) on PGN 65262 and SPN 100 (oil pressure) on PGN 65263.

---

## Troubleshooting

**No response to commands:**
- Check baud rate is 115200
- Ensure USB cable supports data (not charge-only)
- Try unplugging and reconnecting

**Sensor shows fault:**
- Check wiring connections
- Verify sensor is compatible (NTC for temp, 0.5-4.5V for pressure)
- Use `10,X` to read specific sensor type

**Configuration not saving:**
- Run command `6` after making changes
- EEPROM has limited write cycles - avoid excessive saves
