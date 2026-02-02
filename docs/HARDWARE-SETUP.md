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

OSSM uses four 12-pin Deutsch connectors (A, B, C, D).

### Connector A

| Pin | Function | Notes |
|-----|----------|-------|
| 1 | Ground | Chassis ground |
| 2 | pres2 Signal | Pressure sensor 2 |
| 3 | temp2 + | Temperature 2 positive |
| 4 | temp1 + | Temperature 1 positive |
| 5 | pres2 + | Pressure 2 power (5V) |
| 6 | pres7 + | Pressure 7 power (5V) |
| 7 | 12V Power | Main power input |
| 8 | 5V Output | For BME280 |
| 9 | Ground | Sensor ground |
| 10 | pres2 - | Pressure 2 ground |
| 11 | temp2 - | Temperature 2 negative |
| 12 | temp1 - | Temperature 1 negative |

### Connector B

| Pin | Function | Notes |
|-----|----------|-------|
| 1 | temp3 - | Temperature 3 negative |
| 2 | temp3 Signal | Temperature 3 |
| 3 | pres1 Signal | Pressure sensor 1 |
| 4 | temp4 + | Temperature 4 positive |
| 5 | pres5 + | Pressure 5 power (5V) |
| 6 | pres1 + | Pressure 1 power (5V) |
| 7 | pres5 + | Pressure 5 power (5V) |
| 8 | pres1 - | Pressure 1 ground |
| 9 | temp4 - | Temperature 4 negative |
| 10 | pres5 - | Pressure 5 ground |
| 11 | temp8 Signal | Temperature 8 |
| 12 | pres6 Signal | Pressure sensor 6 |

### Connector C

| Pin | Function | Notes |
|-----|----------|-------|
| 1 | temp5 + | Temperature 5 positive |
| 2 | pres3 + | Pressure 3 power (5V) |
| 3 | pres6 + | Pressure 6 power (5V) |
| 4 | pres4 Signal | Pressure sensor 4 |
| 5 | temp7 + | Temperature 7 positive |
| 6 | temp6 + | Temperature 6 positive |
| 7 | pres4 Signal | Pressure 4 (duplicate?) |
| 8 | pres4 - | Pressure 4 ground |
| 9 | temp6 - | Temperature 6 negative |
| 10 | temp8 - | Temperature 8 negative |
| 11 | pres7 - | Pressure 7 ground |
| 12 | pres4 + | Pressure 4 power (5V) |

### Connector D

| Pin | Function | Notes |
|-----|----------|-------|
| 1 | SCL | I2C clock for BME280 |
| 2 | SDA | I2C data for BME280 |
| 3 | Ground | BME280 ground |
| 4 | temp5 - | Temperature 5 negative |
| 5 | pres3 - | Pressure 3 ground |
| 6 | pres3 Signal | Pressure sensor 3 |
| 7 | CANL | CAN bus low |
| 8 | CANH | CAN bus high |
| 9 | EGT - | Thermocouple negative |
| 10 | EGT + | Thermocouple positive |
| 11 | pres6 - | Pressure 6 ground |
| 12 | temp8 - | Temperature 8 negative |

---

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
2. Reconfigure and save (command 6)
3. If persistent, EEPROM may be worn
