# OSSM Serial Command Reference

Complete reference for configuring OSSM via USB serial interface.

## Connection

- **Baud rate**: 115200
- **Data bits**: 8
- **Parity**: None
- **Stop bits**: 1

## Command Format

Commands use **byte format** - all values are 0-255. Multi-byte values (SPNs, PSI) are sent as `highByte,lowByte`.

Example: SPN 175 = 0x00AF → send as `0,175`

## Command Summary

| Cmd | Name | Format | Description |
|-----|------|--------|-------------|
| 1 | Enable/Disable SPN | `1,spnHi,spnLo,enable,input?` | Enable (1) or disable (0) an SPN |
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

### Command 1: Enable/Disable SPN

```
1,spnHi,spnLo,enable,input
```

| Parameter | Description |
|-----------|-------------|
| spnHi | SPN high byte (SPN >> 8) |
| spnLo | SPN low byte (SPN & 0xFF) |
| enable | 1 = enable, 0 = disable |
| input | 1-8 for temp SPNs, 1-7 for pressure SPNs (not needed for EGT/BME280) |

**Examples:**
```
1,0,175,1,3   # Enable SPN 175 (oil temp) on temp3
1,0,175,0     # Disable SPN 175
1,0,173,1     # Enable SPN 173 (EGT) - no input needed
1,0,171,1     # Enable BME280 (enables SPNs 171, 108, 354)
1,0,102,1,6   # Enable SPN 102 (boost) on pres6
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
| 0 | All enabled SPNs with input mappings |
| 4 | Full configuration dump |

**Examples:**
```
5,0    # List enabled SPNs
5,4    # Full config dump
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
# Enable SPN 175 (oil temp) on temp3
1,0,175,1,3

# Set temp3 to AEM sensor preset
8,3,0

# Enable SPN 100 (oil pressure) on pres1
1,0,100,1,1

# Set pres1 to 150 PSI
3,1,0,150

# Save configuration
6

# Verify readings
10
```

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
