# OSSM QA Checklist

Reusable testing checklist for OSSM firmware releases.

## Pre-Testing Setup

- [ ] OSSM powered via 12V (Pin A7)
- [ ] CAN bus connected (CANL: D7, CANH: D8)
- [ ] USB serial connected at 115200 baud
- [ ] CAN bus analyzer/logger available
- [ ] Test sensors available

---

## Hardware Sensors

### Temperature Inputs (temp1-temp8)

| Input | Status | Reading | Notes |
|-------|--------|---------|-------|
| temp1 | [ ] | ___°C | |
| temp2 | [ ] | ___°C | |
| temp3 | [ ] | ___°C | |
| temp4 | [ ] | ___°C | |
| temp5 | [ ] | ___°C | |
| temp6 | [ ] | ___°C | |
| temp7 | [ ] | ___°C | |
| temp8 | [ ] | ___°C | |

**Test procedure**: Connect NTC sensor, enable SPN via serial (`1,0,175,1,X`), read with `10,2`

### Pressure Inputs (pres1-pres7)

| Input | Status | Reading | Notes |
|-------|--------|---------|-------|
| pres1 | [ ] | ___ PSI | |
| pres2 | [ ] | ___ PSI | |
| pres3 | [ ] | ___ PSI | |
| pres4 | [ ] | ___ PSI | |
| pres5 | [ ] | ___ PSI | |
| pres6 | [ ] | ___ PSI | |
| pres7 | [ ] | ___ PSI | |

**Test procedure**: Connect pressure sensor, set range (`3,X,0,150`), enable SPN (`1,0,100,1,X`), read with `10,3`

### EGT (MAX31856)

- [ ] Thermocouple connected and reading
- [ ] Temperature reading reasonable: ___°C
- [ ] Fault detection works (disconnect TC, verify fault reported)
- [ ] Thermocouple type setting works (command 4)

**Test procedure**: Enable EGT (`1,0,173,1`), read with `10,1`

### BME280 Ambient Sensor

- [ ] Ambient temperature reading: ___°C
- [ ] Relative humidity reading: ___%
- [ ] Barometric pressure reading: ___ kPa

**Test procedure**: Enable BME280 (`1,0,171,1`), read with `10,4`

---

## Configuration System

### Serial Commands

- [ ] Command 1: Enable/disable SPN works
- [ ] Command 3: Set pressure range works
- [ ] Command 4: Set thermocouple type works
- [ ] Command 5: Query configuration works
- [ ] Command 6: Save to EEPROM works
- [ ] Command 7: Factory reset works
- [ ] Command 8: NTC preset works
- [ ] Command 9: Pressure preset works
- [ ] Command 10: Read sensors works

### J1939 Commands (PGN 65280)

- [ ] Enable SPN via CAN command
- [ ] Disable SPN via CAN command
- [ ] Response received on PGN 65281

### EEPROM Persistence

- [ ] Enable sensors and save config
- [ ] Power cycle device
- [ ] Verify config restored correctly
- [ ] Verify sensors still transmitting

---

## J1939 Output

### PGN Transmission

| PGN | Interval | Status | SPNs Verified |
|-----|----------|--------|---------------|
| 65269 | 1s | [ ] | 171, 172, 108 |
| 65270 | 0.5s | [ ] | 173, 102, 105, 106 |
| 65262 | 1s | [ ] | 175, 110, 174 |
| 65263 | 0.5s | [ ] | 100, 109, 94 |
| 65129 | 1s | [ ] | 1637, 1363 |
| 65189 | 1s | [ ] | 1131, 1132, 1133 |
| 65190 | 0.5s | [ ] | 1127, 1128 |

**Test procedure**: Enable relevant SPNs, monitor CAN bus with analyzer

### Data Accuracy

- [ ] Temperature values match sensor readings (within 1°C)
- [ ] Pressure values match sensor readings (within 1 PSI)
- [ ] Scaling/offsets correct per J1939 spec
- [ ] Only enabled SPNs transmitted (disabled = 0xFF fill)

---

## Edge Cases

### Sensor Faults

- [ ] Disconnected NTC shows fault, not garbage
- [ ] Disconnected pressure sensor handled gracefully
- [ ] Disconnected thermocouple triggers MAX31856 fault
- [ ] BME280 communication failure handled

### Boundary Conditions

- [ ] Temperature at lower limit (-40°C or sensor min)
- [ ] Temperature at upper limit (sensor max)
- [ ] Pressure at 0 PSI
- [ ] Pressure at max range

### Multiple Sensors

- [ ] All 8 temperature inputs enabled simultaneously
- [ ] All 7 pressure inputs enabled simultaneously
- [ ] All sensor types enabled together (temp + pressure + EGT + BME280)
- [ ] No timing conflicts or missed readings

---

## Final Verification

- [ ] All enabled SPNs appear on CAN bus
- [ ] No unexpected CAN traffic
- [ ] Serial interface responsive during CAN transmission
- [ ] No memory leaks after extended operation (1+ hour)

## Sign-Off

**Tester**: _______________________

**Date**: _______________________

**Firmware Version**: _______________________

**Hardware Version**: _______________________

**Result**: [ ] PASS / [ ] FAIL

**Notes**:
