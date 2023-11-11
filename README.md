# Open Source Sensor Module

This software is designed to emulate a second PCM/ECM and read a bunch of sensors and respond to ODB2 requests. Some of the sensors won't have standard ODB2 requests associated with them, so this repo starts the idea of an open source set of ODB2 PIDS, and will provide the dbc and associated things needed to use those PIDS.

## Sensors

This Sensor Module can read and provide data for up to 19 sensors.

1. Ambient Temp
2. Relative Humidity
3. Absolute Barometric Pressure
4. EGT
5. Oil Temperature
6. Oil Pressure
7. Coolant Temperature
8. Coolant Pressure
9. Tranmission Temperature
10. Transmission Pressure(line pressure)
11. Boost Pressure
12. Boost Temperature
13. CAC Pressure
14. CAC Temperature
15. Intake Temperature
16. Intake Pressure
17. Fuel Pressure
18. Fuel Temperature
19. Engine Bay Temperature

## Pinout

The hardware has 48 pins on 4 plugs and is pinned out as follows.

| Pin A                   | Pin B                    | Pin C                     | Pin D                           |
| ----------------------- | ------------------------ | ------------------------- | ------------------------------- |
| 1. Fuel Pressure -      | 1. Oil Temperature +     | 1. CAC Pressure +         | 1. Transmission Temperature -   |
| 2. Fuel Pressure Signal | 2. Oil Pressure Signal   | 2. CAC Temperature +      | 2. Transmission Pressure -      |
| 3. Fuel Temp +          | 3. Coolant Temperature + | 3. Intake Temperature +   | 3. Boost Temperature -          |
| 4. Engine Bay Temp +    | 4. Coolant Pressure +    | 4. Intake Pressure Signal | 4. Ground                       |
| 5. Fuel Temp -          | 5. Oil Temperature -     | 5. Intake Pressure +      | 5. Egt+                         |
| 6. Engine Bay Temp -    | 6. Oil Pressure -        | 6. Boost Pressure +       | 6. Egt-                         |
| 7. 12 Volt Power        | 7. Coolant Temperature - | 7. Intake Pressure -      | 7. Boost Pressure +             |
| 8. 5 Volts for BME280   | 8. Coolant Pressure -    | 8. CAC Pressure Signal    | 8. Transmission Temperature +   |
| 9. Ground               | 9. Oil Pressure +        | 9. CAC Pressure -         | 9. Transmission Pressure Signal |
| 10. Ground for BME280   | 10. Coolant Pressure +   | 10. CAC Temperature -     | 10. Boost Pressure Signal       |
| 11. SCL for BME280      | 11. CanH                 | 11. Intake Temperature -  | 11. Boost Temperature +         |
| 12. SDA for BME280      | 12. CanL                 | 12. Fuel Pressure +       | 12. Transmission Pressure +     |

## ODB2/CanBus

This module returns the data collected from the sensors via CanBus. It will use all the standard PIDs, but also supports a custom set of PIDs that are designed for real-time gauges and accurate data and not smog enforcement. They are documented in the following table.

| ID (hex) | ID (Dec) | Bytes returned | Description                  | Min Value        | Max Value      | Scale  | Offset | Units | Formula                     |
| -------- | -------- | -------------- | ---------------------------  | ---------------- | -------------- | -----  | ------ | ----- | --------------------------- |
| 0XFF     | 255      | 7              | Transmission Temperature     | -327.67(-557.8F) | 327.68(621.8F) | .01    | -32767 | °C    | ((256*A + B) - 32767) / 100 |
| 0XFF     | 255      | 7              | Transmission Pressure(line)  | 0                | 6553.5(950psi) | .1     | 0      | kPa   | (256*C + D) / 10            |
| 0XFE     | 254      | 7              | Boost Temperature            | -327.67(-557.8F) | 327.68(621.8F) | .01    | -32767 | °C    | ((256*A + B) - 32767) / 100 |
| 0XFE     | 254      | 7              | Boost Pressure               | 0                | 6553.5(950psi) | .1     | 0      | kPa   | (256*C + D) / 10            |
| 0XFD     | 253      | 7              | Oil Temperature              | -327.67(-557.8F) | 327.68(621.8F) | .01    | -32767 | °C    | ((256*A + B) - 32767) / 100 |
| 0XFD     | 253      | 7              | Oil Pressure                 | 0                | 6553.5(950psi) | .1     | 0      | kPa   | (256*C + D) / 10            |
| 0XFA     | 250      | 7              | Ambient Temperature          | -327.67(-557.8F) | 327.68(621.8F) | .01    | -32767 | °C    | ((256*A + B) - 32767) / 100 |
| 0XFA     | 250      | 7              | Absolute Barometric Pressure | 0                | 6553.5(95psi)  | .1     | 0      | hPa   | (256*C + D) / 10            |
| 0XF9     | 249      | 7              | Ambient Humidity             | 0                | 100            | 655.35 | 0      | hPa   | (256*A + B) / 655.35        |
