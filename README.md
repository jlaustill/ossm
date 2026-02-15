# Open Source Sensor Module

![OSSM Hardware](images/OSSMOutside.jpg)

OSSM is an open-source automotive sensor module that reads temperature, pressure, and environmental sensors and transmits data over CAN bus using the J1939 protocol. It emulates a secondary ECM, allowing gauges and data loggers to display sensor data alongside factory instrumentation.

## Features

- **8 temperature inputs** - NTC thermistor sensors
- **7 pressure inputs** - 0.5-4.5V analog sensors
- **EGT input** - K-type thermocouple (configurable for other types)
- **Ambient sensing** - Temperature, humidity, barometric pressure (BME280)
- **J1939 protocol** - Standard CAN bus communication at 250 kbps
- **Runtime configuration** - No recompilation needed, settings stored in EEPROM
- **CAN bus configuration** - Configure over J1939 without USB access

## Hardware Requirements

- OSSM PCB (Teensy 4.0 based)
- 12V power supply
- CAN bus connection (CANL/CANH with 120Ω termination)
- Compatible sensors

## Pinout

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

## Quick Start

1. **Connect power**: 12V to pin A7, ground to A1 or A9
2. **Connect CAN bus**: CANL to D7, CANH to D8
3. **Wire sensors**: Connect NTC/pressure sensors to desired inputs
4. **Configure via J1939**: Send commands on PGN 65280 (see below)
5. **Verify output**: Monitor PGNs for sensor data

On first boot, all sensors are disabled. Assign inputs to physical measurement locations (values) using J1939 commands. SPNs are automatically enabled based on which values have hardware assigned.

## J1939 Configuration

OSSM accepts configuration commands on **PGN 65280 (0xFF00)** and responds on **PGN 65281 (0xFF01)**.

### Assign Input to Value

```
Byte 0: 0x01 (Assign command)
Byte 1: Input type (0 = temp, 1 = pressure)
Byte 2: Input number (1-8 for temp, 1-7 for pressure)
Byte 3: Value ID (see table below)
```

**Example**: Assign temp3 to OIL_TEMP (15):
```
01 00 03 0F
```

### Save Configuration

```
Byte 0: 0x06 (Save command)
```

Saves current configuration to EEPROM.

### Common Value IDs

| ID | Name                 | Description                    | Auto-enabled SPNs |
|----|----------------------|--------------------------------|-------------------|
| 15 | OIL_TEMP             | Engine oil temperature         | 175               |
| 17 | COOLANT_TEMP         | Coolant temperature            | 110, 1637         |
| 14 | OIL_PRES             | Engine oil pressure            | 100               |
| 12 | MANIFOLD1_ABS_PRES   | Intake manifold pressure       | 102               |
| 7  | TURBO1_TURB_INLET_TEMP | EGT (turbine inlet)          | 173               |

See [Serial Commands](docs/SERIAL-COMMANDS.md) for complete EValueId table.
See [SPN Reference](docs/SPN-REFERENCE.md) for value-to-SPN mappings.

## J1939 Output

OSSM transmits sensor data on standard J1939 PGNs:

| PGN   | Interval | Data                                     |
|-------|----------|------------------------------------------|
| 65262 | 1s       | Oil temp, coolant temp, fuel temp        |
| 65263 | 500ms    | Oil pressure, coolant pressure, fuel pressure |
| 65269 | 1s       | Ambient temp, air inlet temp, barometric |
| 65270 | 500ms    | EGT, boost, intake temp, inlet pressure  |

Only enabled SPNs are transmitted. Disabled SPNs show as 0xFF (Not Available).

## Building from Source

```bash
# Install PlatformIO
pip install platformio

# Build
pio run

# Upload to Teensy
pio run -t upload
```

## Documentation

- [Serial Commands](docs/SERIAL-COMMANDS.md) - USB serial configuration interface
- [SPN Reference](docs/SPN-REFERENCE.md) - Complete SPN and PGN details
- [Hardware Setup](docs/HARDWARE-SETUP.md) - Wiring and installation guide
- [Architecture](docs/ARCHITECTURE.md) - Technical design documentation
- [QA Checklist](docs/QA-CHECKLIST.md) - Testing procedures

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for development setup and guidelines.

## License

MIT License - see [LICENSE](LICENSE) for details.

---

**Note**: J1939 implementation is based on reverse engineering and publicly available information. Test thoroughly before relying on this for critical applications.
