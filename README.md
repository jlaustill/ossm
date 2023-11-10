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

A1. Fuel Pressure -
A2. Fuel Pressure Signal
A3. Fuel Temp +
A4. Engine Bay Temp +
A5. Fuel Temp -
A6. Engine Bay Temp -
A7. 12 Volt Power
A8. 5 Volts for BME280
A9. Ground
A10. Ground for BME280
A11. SCL for BME280
A12. SDA for BME280

B1. Oil Temperature +
B2. Oil Pressure Signal
B3. Coolant Temperature +
B4. Coolant Pressure +
B5. Oil Temperature -
B6. Oil Pressure -
B7. Coolant Temperature -
B8. Coolant Pressure -
B9. Oil Pressure +
B10. Coolant Pressure +
B11. CanH
B12. CanL

C1. CAC Pressure +
C2. CAC Temperature +
C3. Intake Temperature +
C4. Intake Pressure Signal
C5. Intake Pressure +
C6. Boost Pressure +
C7. Intake Pressure -
C8. CAC Pressure Signal
C9. CAC Pressure -
C10. CAC Temperature -
C11. Intake Temperature -
C12. Fuel Pressure +

D1. Transmission Temperature -
D2. Transmission Pressure -
D3. Boost Temperature -
D4. Ground
D5. Egt+
D6. Egt-
D7. Boost Pressure -
D8. Transmission Temperature +
D9. Transmission Pressure Signal
D10. Boost Pressure Signal
D11. Boost Temperature +
D12. Transmission Pressure +


