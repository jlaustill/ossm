# OSSM SPN Reference

Complete reference for all Suspect Parameter Numbers (SPNs) supported by OSSM.

## Overview

OSSM transmits sensor data using J1939 protocol over CAN bus. Each sensor reading is assigned a standardized SPN that other J1939 devices (gauges, ECUs, data loggers) can interpret.

On first boot, **all sensors are disabled**. Assign inputs to physical measurement locations (EValueId) via serial commands or J1939 PGN 65280. **SPNs are automatically enabled** when their source value has hardware assigned.

## Value to SPN Mapping

Each EValueId can enable one or more J1939 SPNs automatically:

| EValueId | ID | Enabled SPNs |
|----------|----|--------------|
| AMBIENT_PRES | 0 | 108 (Barometric Pressure) |
| AMBIENT_TEMP | 1 | 171 (Ambient Air Temperature) |
| AMBIENT_HUMIDITY | 2 | 354 (Relative Humidity) |
| TURBO1_COMP_INLET_PRES | 3 | 106 (Air Inlet Pressure) |
| TURBO1_COMP_INLET_TEMP | 4 | 1131 (Turbo 1 Compressor Inlet Temp) |
| TURBO1_COMP_OUTLET_PRES | 5 | 1127 (Turbo 1 Boost Pressure) |
| TURBO1_COMP_OUTLET_TEMP | 6 | 1132 (Turbo 1 Compressor Outlet Temp) |
| TURBO1_TURB_INLET_TEMP | 7 | 173 (EGT) |
| CAC1_OUTLET_TEMP | 11 | 172 (Air Inlet Temperature) |
| MANIFOLD1_ABS_PRES | 12 | 102 (Boost Pressure) |
| MANIFOLD1_TEMP | 13 | 105, 1363 (Intake Manifold Temp + Hi-Res) |
| OIL_PRES | 14 | 100 (Engine Oil Pressure) |
| OIL_TEMP | 15 | 175 (Engine Oil Temperature) |
| COOLANT_PRES | 16 | 109 (Coolant Pressure) |
| COOLANT_TEMP | 17 | 110, 1637 (Coolant Temp + Hi-Res) |
| FUEL_PRES | 18 | 94 (Fuel Delivery Pressure) |
| FUEL_TEMP | 19 | 174 (Fuel Temperature) |
| ENGINE_BAY_TEMP | 20 | 441 (Engine Bay Temperature) |

---

## Temperature SPNs

Temperature SPNs are auto-enabled when their source EValueId has hardware assigned.

| SPN | Name | Source EValueId | J1939 Scaling | PGN |
|-----|------|-----------------|---------------|-----|
| 175 | Engine Oil Temperature | OIL_TEMP (15) | 1°C/bit, +40°C offset | 65262 |
| 110 | Engine Coolant Temperature | COOLANT_TEMP (17) | 1°C/bit, +40°C offset | 65262 |
| 174 | Fuel Temperature | FUEL_TEMP (19) | 1°C/bit, +40°C offset | 65262 |
| 105 | Intake Manifold 1 Temperature | MANIFOLD1_TEMP (13) | 1°C/bit, +40°C offset | 65270 |
| 1131 | Turbo 1 Compressor Inlet Temp | TURBO1_COMP_INLET_TEMP (4) | 1°C/bit, +40°C offset | 65189 |
| 1132 | Turbo 1 Compressor Outlet Temp | TURBO1_COMP_OUTLET_TEMP (6) | 1°C/bit, +40°C offset | 65189 |
| 172 | Air Inlet Temperature | CAC1_OUTLET_TEMP (11) | 1°C/bit, +40°C offset | 65269 |
| 441 | Engine Bay Temperature | ENGINE_BAY_TEMP (20) | 1°C/bit, +40°C offset | 65164 |

### High-Resolution Temperature SPNs

These are auto-enabled alongside their standard-resolution counterparts:

| Hi-Res SPN | Source EValueId | Resolution | PGN |
|------------|-----------------|------------|-----|
| 1637 | COOLANT_TEMP (17) | 0.03125°C/bit, +273°C | 65129 |
| 1363 | MANIFOLD1_TEMP (13) | 0.03125°C/bit, +273°C | 65129 |

When you assign an input to COOLANT_TEMP or MANIFOLD1_TEMP, both standard and high-resolution SPNs transmit automatically.

---

## Pressure SPNs

Pressure SPNs are auto-enabled when their source EValueId has hardware assigned.

| SPN | Name | Source EValueId | J1939 Scaling | PGN |
|-----|------|-----------------|---------------|-----|
| 100 | Engine Oil Pressure | OIL_PRES (14) | 4 kPa/bit | 65263 |
| 109 | Coolant Pressure | COOLANT_PRES (16) | 2 kPa/bit | 65263 |
| 94 | Fuel Delivery Pressure | FUEL_PRES (18) | 4 kPa/bit | 65263 |
| 102 | Boost Pressure | MANIFOLD1_ABS_PRES (12) | 2 kPa/bit | 65270 |
| 106 | Air Inlet Pressure | TURBO1_COMP_INLET_PRES (3) | 2 kPa/bit | 65270 |
| 108 | Barometric Pressure | AMBIENT_PRES (0) | 0.5 kPa/bit | 65269 |
| 1127 | Turbo 1 Compressor Outlet Pressure | TURBO1_COMP_OUTLET_PRES (5) | 0.125 kPa/bit | 65190 |

---

## EGT SPN

The EGT sensor (MAX31856 thermocouple) maps to `TURBO1_TURB_INLET_TEMP` - the turbine inlet temperature, which is where exhaust gas enters the turbo.

| SPN | Name | Source EValueId | J1939 Scaling | PGN |
|-----|------|-----------------|---------------|-----|
| 173 | Exhaust Gas Temperature | TURBO1_TURB_INLET_TEMP (7) | 0.03125°C/bit, +273°C offset | 65270 |

When the EGT sensor is enabled, `TURBO1_TURB_INLET_TEMP` is marked as having hardware, and SPN 173 is auto-enabled.

---

## BME280 SPNs

The BME280 environmental sensor provides ambient conditions. When enabled, it marks three EValueId values as having hardware:

| SPN | Name | Source EValueId | J1939 Scaling | PGN |
|-----|------|-----------------|---------------|-----|
| 171 | Ambient Air Temperature | AMBIENT_TEMP (1) | 0.03125°C/bit, +273°C offset | 65269 |
| 108 | Barometric Pressure | AMBIENT_PRES (0) | 0.5 kPa/bit | 65269 |
| 354 | Relative Humidity | AMBIENT_HUMIDITY (2) | 0.4%/bit | 65164 |

When BME280 is enabled, all three values have hardware, and all three SPNs are auto-enabled.

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

- SPNs are auto-enabled when their source EValueId has hardware assigned
- SPNs without hardware are filled with `0xFF` (Not Available per J1939)
- PGNs are transmitted at their configured intervals regardless of which SPNs are active

---

## Example Configurations

### Basic Engine Monitoring

```
# Assign temp1 to OIL_TEMP (15) -> auto-enables SPN 175
1,0,1,15

# Assign temp2 to COOLANT_TEMP (17) -> auto-enables SPN 110 and hi-res SPN 1637
1,0,2,17

# Assign pres1 to OIL_PRES (14) -> auto-enables SPN 100
1,1,1,14

# Save
6
```

### Turbo Monitoring

```
# Assign temp1 to MANIFOLD1_TEMP (13) -> auto-enables SPN 105 and hi-res SPN 1363
1,0,1,13

# Enable EGT sensor (in AppConfig) -> TURBO1_TURB_INLET_TEMP (7) -> auto-enables SPN 173

# Assign pres1 to MANIFOLD1_ABS_PRES (12) -> auto-enables SPN 102 (boost)
1,1,1,12

# Assign pres2 to TURBO1_COMP_OUTLET_PRES (5) -> auto-enables SPN 1127 (hi-res boost)
1,1,2,5

# Save
6
```

### Full Ambient

```
# Enable BME280 sensor (in AppConfig) -> marks AMBIENT_PRES (0), AMBIENT_TEMP (1), AMBIENT_HUMIDITY (2) as having hardware
# Auto-enables SPNs 108, 171, 354

# Assign temp1 to CAC1_OUTLET_TEMP (11) -> auto-enables SPN 172 (air inlet temp)
1,0,1,11

# Save
6
```

---

## J1939 Technical Notes

### Byte Order

J1939 uses **little-endian** (LSB first) for multi-byte values.

### Scaling Formulas

**Encoding** (OSSM transmitting - converting engineering units to raw):
```
Raw Value = (Engineering Value + Offset) / Resolution
```

**Decoding** (other devices receiving - converting raw to engineering):
```
Engineering Value = (Raw Value × Resolution) - Offset
```

The offset in TSpnConfig is the value **added** during encoding. For temperatures with "+40 offset", a 25°C reading encodes as `(25 + 40) / 1.0 = 65`.

### Source Address

OSSM uses J1939 source address **149** by default. This can be changed via EEPROM configuration.
