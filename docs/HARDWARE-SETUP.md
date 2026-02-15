# OSSM Hardware Setup

Physical installation and wiring guide for OSSM.

## Requirements

### OSSM Module

- Teensy 4.0 based PCB
- 4x ADS1115 16-bit ADCs
- MAX31856 thermocouple interface
- BME280 ambient sensor (optional, external)
- CAN transceiver

### Power

- **Input voltage**: 12V DC (nominal 10-16V)
- **Current draw**: ~150mA typical
- **Connection**: Pin A7 (12V), Pin A1/A9 (Ground)

### CAN Bus

- **Protocol**: J1939 @ 250 kbps
- **Termination**: 120Ω resistor required at each end of bus
- **Connections**: CANL (D7), CANH (D8)

---

## Connector Pinout

OSSM uses four 12-pin Deutsch connectors (A, C, D, B) (Left to Right).

| Pin A            | Pin C                | Pin D                | Pin B                |
|------------------|----------------------|----------------------|----------------------|
| 1. Ground        | 1. temp3 +           | 1. SCL (BME280)      | 1. temp1 -           |
| 2. pres7 Signal  | 2. pres5 +           | 2. SDA (BME280)      | 2. temp1 +           |
| 3. temp7 +       | 3. pres6 +           | 3. Ground (BME280)   | 3. pres1 Signal      |
| 4. temp8 +       | 4. pres4 Signal      | 4. temp3 -           | 4. temp2 +           |
| 5. pres7 +       | 5. temp6 +           | 5. pres5 -           | 5. pres2 Signal      |
| 6. pres4 +       | 6. temp5 +           | 6. pres5 Signal      | 6. pres1 +           |
| 7. **12V Power** | 7. pres3 Signal      | 7. **CANL**          | 7. pres2 +           |
| 8. 5V (BME280)   | 8. pres3 -           | 8. **CANH**          | 8. pres1 -           |
| 9. Ground        | 9. temp5 -           | 9. EGT -             | 9. temp2 -           |
| 10. pres7 -      | 10. temp6 -          | 10. EGT +            | 10. pres2 -          |
| 11. temp7 -      | 11. pres4 -          | 11. pres6 -          | 11. temp4 +          |
| 12. temp8 -      | 12. pres3 +          | 12. temp4 -          | 12. pres6 Signal     |

## Sensor Wiring

### NTC Temperature Sensors (temp1-temp8)

NTC thermistors are two-wire resistive sensors.

```
Sensor + ────────┬──── temp[X] +
                 │
              [NTC]
                 │
Sensor - ────────┴──── temp[X] -
```

**Supported sensors** (use command 8 to select preset):
- AEM 30-2012 (preset 0)
- Bosch (preset 1)
- GM (preset 2)

### Pressure Sensors (pres1-pres7)

Three-wire 0.5-4.5V analog sensors.

```
5V Power ─────────── pres[X] +
Signal Out ──────── pres[X] Signal
Ground ──────────── pres[X] -
```

**Configuration**:
- Set max PSI with command 3 or preset 9
- Sensors must output 0.5V @ 0 PSI, 4.5V @ max PSI

### EGT Thermocouple

K-type thermocouple (default) connected to MAX31856.

```
TC + (yellow) ────── EGT + (D10)
TC - (red) ────────── EGT - (D9)
```

**Other thermocouple types**: Use command 4 to change type (B, E, J, K, N, R, S, T).

### BME280 Ambient Sensor

External I2C sensor for ambient conditions.

```
VCC ────── 5V (A8)
GND ────── Ground (D3)
SCL ────── SCL (D1)
SDA ────── SDA (D2)
```

---

## CAN Bus Wiring

### Basic Connection

```
OSSM CANL (D7) ────── Vehicle CANL
OSSM CANH (D8) ────── Vehicle CANH
```

### Termination

J1939 requires 120Ω termination resistors at each end of the bus. If OSSM is at the end of the bus, add a 120Ω resistor between CANL and CANH.

### Multiple Devices

```
       120Ω                              120Ω
    ┌───/\/\/───┐                    ┌───/\/\/───┐
    │           │                    │           │
────┴───────────┴────────────────────┴───────────┴────
    CANH                                         CANH
────┬───────────┬────────────────────┬───────────┬────
    │           │                    │           │
    │    [ECU]  │      [OSSM]       │  [Gauge]  │
    │           │                    │           │
```

---

## Installation Tips

### Mounting

- Mount in a dry, protected location
- Avoid direct engine heat
- Ensure adequate ventilation
- Use weatherproof connectors

### Wire Routing

- Keep sensor wires away from ignition wires
- Use shielded cable for long runs (>3m)
- Avoid running parallel to high-current wires

### Grounding

- Use a single ground point when possible
- Avoid ground loops between sensors
- Connect ground to chassis near battery negative

### Temperature Sensors

- Install in threaded bosses or use clamp adapters
- Use thermal paste for good contact
- Avoid air pockets in fluid sensors

### Pressure Sensors

- Install with port facing down when possible (prevents debris)
- Use snubbers for pressure spikes
- Check max pressure rating exceeds expected range

---

## Troubleshooting

### No CAN Communication

1. Check 12V power at A7
2. Verify CANL/CANH not swapped
3. Check termination resistors
4. Verify baud rate (250 kbps)

### Sensor Reading "Fault"

1. Check wiring continuity
2. Verify sensor is compatible type
3. Check for shorts to ground/power
4. Try different input to isolate

### Erratic Readings

1. Check for loose connections
2. Shield sensor wires from EMI
3. Add capacitor across sensor (100nF)
4. Check ground connections

### EEPROM Issues

1. Run factory reset (command 7)
2. Reconfigure (auto-saved after each change)
3. If persistent, EEPROM may be worn
