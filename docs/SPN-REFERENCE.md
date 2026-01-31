# OSSM SPN Reference

Complete reference for all Suspect Parameter Numbers (SPNs) supported by OSSM.

## Overview

OSSM transmits sensor data using J1939 protocol over CAN bus. Each sensor reading is assigned a standardized SPN that other J1939 devices (gauges, ECUs, data loggers) can interpret.

On first boot, **all SPNs are disabled**. Enable SPNs via serial commands or J1939 PGN 65280.

---

## Temperature SPNs

Assign to temperature inputs `temp1` through `temp8`.

| SPN | Name | Input | J1939 Scaling | PGN |
|-----|------|-------|---------------|-----|
| 175 | Engine Oil Temperature | temp1-8 | 1°C/bit, -40°C offset | 65262 |
| 110 | Engine Coolant Temperature | temp1-8 | 1°C/bit, -40°C offset | 65262 |
| 174 | Fuel Temperature | temp1-8 | 1°C/bit, -40°C offset | 65262 |
| 105 | Intake Manifold 1 Temperature | temp1-8 | 1°C/bit, -40°C offset | 65270 |
| 1131 | Intake Manifold 2 Temperature | temp1-8 | 1°C/bit, -40°C offset | 65189 |
| 1132 | Intake Manifold 3 Temperature | temp1-8 | 1°C/bit, -40°C offset | 65189 |
| 1133 | Intake Manifold 4 Temperature | temp1-8 | 1°C/bit, -40°C offset | 65189 |
| 172 | Air Inlet Temperature | temp1-8 | 1°C/bit, -40°C offset | 65269 |
| 441 | Auxiliary Temperature 1 | temp1-8 | 1°C/bit, -40°C offset | 65164 |

### Auto-Enabled High-Resolution SPNs

| Primary SPN | Hi-Res SPN | Resolution | PGN |
|-------------|------------|------------|-----|
| 110 (Coolant) | 1637 | 0.03125°C/bit | 65129 |
| 105 (Intake 1) | 1363 | 0.03125°C/bit | 65129 |

When SPN 110 or 105 is enabled, the corresponding high-resolution SPN is automatically enabled.

---

## Pressure SPNs

Assign to pressure inputs `pres1` through `pres7`.

| SPN | Name | Input | J1939 Scaling | PGN |
|-----|------|-------|---------------|-----|
| 100 | Engine Oil Pressure | pres1-7 | 4 kPa/bit | 65263 |
| 109 | Coolant Pressure | pres1-7 | 2 kPa/bit | 65263 |
| 94 | Fuel Delivery Pressure | pres1-7 | 4 kPa/bit | 65263 |
| 102 | Boost Pressure | pres1-7 | 2 kPa/bit | 65270 |
| 106 | Air Inlet Pressure | pres1-7 | 2 kPa/bit | 65270 |
| 1127 | Turbocharger 1 Boost Pressure | pres1-7 | 2 kPa/bit | 65190 |
| 1128 | Turbocharger 2 Boost Pressure | pres1-7 | 2 kPa/bit | 65190 |

---

## EGT SPN

No input assignment required - uses dedicated MAX31856 thermocouple input.

| SPN | Name | J1939 Scaling | PGN |
|-----|------|---------------|-----|
| 173 | Exhaust Gas Temperature | 0.03125°C/bit, -273°C offset | 65270 |

Enable with: `1,0,173,1`

---

## BME280 SPNs

No input assignment required - uses dedicated BME280 sensor. Enabling any one enables all three.

| SPN | Name | J1939 Scaling | PGN |
|-----|------|---------------|-----|
| 171 | Ambient Air Temperature | 0.03125°C/bit, -273°C offset | 65269 |
| 108 | Barometric Pressure | 0.5 kPa/bit | 65269 |
| 354 | Relative Humidity | 0.5%/bit | 65164 |

Enable with: `1,0,171,1`

---

## PGN Reference

Parameter Group Numbers (PGNs) that OSSM transmits:

| PGN | Name | Interval | SPNs |
|-----|------|----------|------|
| 65262 | Engine Temperature 1 | 1000ms | 175, 110, 174 |
| 65263 | Engine Fluid Level/Pressure | 500ms | 100, 109, 94 |
| 65269 | Ambient Conditions | 1000ms | 171, 172, 108 |
| 65270 | Inlet/Exhaust Conditions | 500ms | 173, 102, 105, 106 |
| 65129 | Engine Temperature 2 | 1000ms | 1637, 1363 |
| 65189 | Engine Temperature 3 | 1000ms | 1131, 1132, 1133 |
| 65190 | Turbocharger | 500ms | 1127, 1128 |
| 65164 | Auxiliary I/O | On Request | 354, 441 |

### Transmission Behavior

- Only enabled SPNs are transmitted
- Disabled SPNs are filled with `0xFF` (Not Available per J1939)
- PGNs are only transmitted if at least one SPN in the group is enabled

---

## Example Configurations

### Basic Engine Monitoring

```
# Oil temperature on temp1
1,0,175,1,1

# Coolant temperature on temp2 (auto-enables hi-res SPN 1637)
1,0,110,1,2

# Oil pressure on pres1
1,0,100,1,1

# Save
6
```

### Turbo Monitoring

```
# Intake manifold temp on temp1
1,0,105,1,1

# EGT
1,0,173,1

# Boost pressure on pres1
1,0,102,1,1

# Turbo 1 boost on pres2
1,0,127,1,2

# Save
6
```

### Full Ambient

```
# Enable BME280 (ambient temp, barometric, humidity)
1,0,171,1

# Air inlet temp on temp1
1,0,172,1,1

# Save
6
```

---

## J1939 Technical Notes

### Byte Order

J1939 uses **little-endian** (LSB first) for multi-byte values.

### Scaling Formula

To convert raw J1939 value to engineering units:

```
Engineering Value = (Raw Value × Resolution) + Offset
```

### Source Address

OSSM uses J1939 source address **149** by default. This can be changed via EEPROM configuration.
