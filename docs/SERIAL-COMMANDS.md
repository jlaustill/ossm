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
| 1 | Enable Value | `1,valueId[,input]` | Enable a value (BME280, EGT, or assign input) |
| 2 | Disable Value | `2,valueId` | Disable a value |
| 3 | Set Pressure Range | `3,input,psiHi,psiLo` | Set pressure sensor max PSI |
| 4 | Set TC Type | `4,type` | Set thermocouple type (0-7) |
| 5 | Query Config | `5,query_type` | Query configuration |
| 7 | NTC Preset | `7,input,preset` | Apply NTC sensor preset |
| 8 | Pressure Preset | `8,input,preset` | Apply pressure sensor preset |
| 9 | Read Sensors | `9[,type]` | Read live sensor values |

**Note:** All configuration changes are automatically saved to EEPROM. No explicit save command needed.

---

## Command Details

### Command 1: Enable Value

```
1,valueId[,input]
```

| Parameter | Description |
|-----------|-------------|
| valueId | EValueId enum value (see table below) |
| input | Sensor input number (optional, required for temperature/pressure only) |

**Values without input (BME280, EGT):**
- `valueId 0,1,2` = BME280 (AMBIENT_PRES, AMBIENT_TEMP, AMBIENT_HUMIDITY)
- `valueId 7` = EGT (MAX31856 thermocouple)

**Values requiring input (temperature/pressure sensors):**
- For temperature: `input` = 1-8 (temp1-temp8)
- For pressure: `input` = 1-7 (pres1-pres7)

**Examples:**
```
1,0        # Enable BME280 (AMBIENT_PRES)
1,1        # Enable BME280 (AMBIENT_TEMP)
1,2        # Enable BME280 (AMBIENT_HUMIDITY)
1,7        # Enable EGT (MAX31856)

1,15,1     # Assign temp1 to OIL_TEMP (15)
1,17,2     # Assign temp2 to COOLANT_TEMP (17)
1,14,1     # Assign pres1 to OIL_PRES (14)
1,12,6     # Assign pres6 to MANIFOLD1_ABS_PRES (12)
1,255,1    # Disable temp1 (assign to invalid value)
```

**Auto-enabled SPNs:**

When you enable a value, all SPNs that source from that value are automatically enabled. For example:
- `1,0` or `1,1` or `1,2` enables all BME280 SPNs (108, 171, 354)
- `1,7` enables EGT SPN 173
- `1,12` enables both SPN 102 and SPN 4817 (same value, different resolutions)

See [SPN Reference](SPN-REFERENCE.md) for the complete mapping of values to SPNs.

---

### Command 2: Disable Value

```
2,valueId
```

Disable a previously enabled value. This removes the value from any sensor inputs.

**Examples:**
```
2,7        # Disable EGT
2,14       # Disable OIL_PRES (removes it from any pressure input)
```

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
BME280: enabled
EGT: enabled

=== Active SPNs (auto-enabled) ===
SPN 175 (OIL_TEMP) -> PGN 65262
SPN 100 (OIL_PRES) -> PGN 65263
SPN 102 (MANIFOLD1_ABS_PRES) -> PGN 65270
SPN 108 (AMBIENT_PRES) -> PGN 65269
SPN 171 (AMBIENT_TEMP) -> PGN 65269
SPN 354 (AMBIENT_HUMIDITY) -> PGN 65164
```

---

### Command 7: NTC Preset

```
7,input,preset
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
7,3,0    # Set temp3 to AEM preset
```

---

### Command 8: Pressure Preset

```
8,input,preset
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
8,6,1    # Set pres6 to 150 PSI
```

---

### Command 9: Read Live Sensors

```
9[,sensorType]
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
9        # Read all active sensors
9,1      # Read EGT temperature
9,2      # Read all temperature sensors
9,3      # Read all pressure sensors
9,4      # Read BME280 (ambient temp, humidity, barometric)
```

**Note:** Sensor faults are displayed at the end of ALL command responses.

**Note:** All configuration changes are automatically saved to EEPROM. No explicit save command needed.

---

## Quick Start Example

Configure oil temp on temp3 and oil pressure on pres1:

```
# Assign temp3 to OIL_TEMP (15)
1,15,3

# Set temp3 to AEM sensor preset
7,3,0

# Assign pres1 to OIL_PRES (14)
1,14,1

# Set pres1 to 150 PSI
3,1,0,150

# Verify readings
9
```

---

## EGT and BME280 Setup

**Enable EGT:**
```
# Set thermocouple type (K-type is default)
4,3

# Enable EGT
1,7
```

**Enable BME280:**
```
# Enable any BME280 value (they're all enabled together)
1,0    # or 1,1 or 1,2
```

Note: BME280 is a single sensor that provides ambient temperature, humidity, and barometric pressure. Enabling any one value enables all three.

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
- All changes are automatically saved to EEPROM after each command
- EEPROM has limited write cycles - avoid excessive command spamming

**Unknown command error:**
- Ensure command format matches: `1,valueId[,input]`
- For BME280/EGT, no input number is needed: `1,7` not `1,7,1`
