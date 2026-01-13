# Changelog

All notable changes to the OSSM firmware will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.2] - 2026-01-13

### Fixed
- Runtime temp/pressure sensor enable now takes effect immediately without requiring reboot
- SPN reassignment and temp2/pres1 hardware mapping corrected
- ADS1115 at 0x49 hardware mapping corrected per schematic

## [0.1.1] - (Previous release)

### Added
- J1939 SPN query support
- Standardized label format
- PSIA/PSIG pressure type support
- Static analysis with cppcheck integration

### Fixed
- CAN bus configuration for v0.0.2 hardware
- Static analysis warnings and MISRA compliance issues

## [0.1.0] - (Initial release)

### Added
- Initial firmware release for OSSM hardware
- Support for 8 temperature inputs (NTC thermistors)
- Support for 7 pressure inputs (analog sensors)
- EGT support via MAX31856 thermocouple
- BME280 ambient sensor support (temperature, humidity, barometric pressure)
- J1939 protocol implementation for CAN bus transmission
- EEPROM-based configuration storage
- Serial command interface for runtime configuration
- Non-blocking sensor reads with IntervalTimer
