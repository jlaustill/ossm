#ifndef OSSM_APPCONFIG_H
#define OSSM_APPCONFIG_H

#include <stdint.h>
#include "Data/types/ESensorType.h"
#include "Data/types/TSensorConfig.h"

#define CONFIG_MAGIC 0x4F53534D  // "OSSM" in ASCII
#define CONFIG_VERSION 1
#define MAX_SENSORS 19
#define ADS_DEVICE_COUNT 4

// AppData field indices for appDataIndex mapping
enum EAppDataIndex : uint8_t {
    IDX_HUMIDITY = 0,
    IDX_AMBIENT_TEMP = 1,
    IDX_BAROMETRIC_PRESSURE = 2,
    IDX_OIL_TEMP = 3,
    IDX_OIL_PRESSURE = 4,
    IDX_COOLANT_TEMP = 5,
    IDX_COOLANT_PRESSURE = 6,
    IDX_TRANSFER_PIPE_TEMP = 7,
    IDX_TRANSFER_PIPE_PRESSURE = 8,
    IDX_BOOST_PRESSURE = 9,
    IDX_BOOST_TEMP = 10,
    IDX_CAC_INLET_PRESSURE = 11,
    IDX_CAC_INLET_TEMP = 12,
    IDX_AIR_INLET_PRESSURE = 13,
    IDX_AIR_INLET_TEMP = 14,
    IDX_FUEL_PRESSURE = 15,
    IDX_FUEL_TEMP = 16,
    IDX_ENGINE_BAY_TEMP = 17,
    IDX_EGT_TEMP = 18
};

// ADS1115 device configuration
struct TAdsDeviceConfig {
    uint8_t i2cAddress;   // 0x48, 0x49, 0x4A, 0x4B
    uint8_t drdyPin;      // D0, D1, D4, D5
    bool enabled;
};

// MAX31856 thermocouple configuration
struct TThermocoupleConfig {
    uint8_t csPin;        // Chip select (D10)
    uint8_t drdyPin;      // Data ready (D2)
    uint8_t faultPin;     // Fault output (D3)
    bool enabled;
};

// BME280 ambient sensor configuration
struct TBme280Config {
    uint8_t i2cAddress;   // 0x76 or 0x77
    bool enabled;
};

// J1939 SPN (Suspect Parameter Number) enable flags
// These control which data fields are transmitted on CAN
struct TJ1939SpnConfig {
    // Ambient Sensors
    bool spn171_ambientAirTemp;        // Ambient air temperature
    bool spn108_barometricPressure;    // Barometric pressure
    bool spn354_relativeHumidity;      // Relative humidity

    // EGT
    bool spn173_exhaustGasTemp;        // Exhaust gas temperature

    // Intake Position 1
    bool spn102_boostPressure;         // Boost pressure
    bool spn105_intakeManifold1Temp;   // Intake manifold 1 temperature
    bool spn1363_intakeManifold1AirTemp; // Intake manifold 1 air temperature

    // Intake Positions 2-4
    bool spn1131_intakeManifold2Temp;  // Intake manifold 2 temperature
    bool spn1132_intakeManifold3Temp;  // Intake manifold 3 temperature
    bool spn1133_intakeManifold4Temp;  // Intake manifold 4 temperature
    bool spn106_airInletPressure;      // Air inlet pressure
    bool spn172_airInletTemp;          // Air inlet temperature

    // Engine Oil
    bool spn175_engineOilTemp;         // Engine oil temperature
    bool spn100_engineOilPressure;     // Engine oil pressure

    // Engine Coolant
    bool spn110_engineCoolantTemp;     // Engine coolant temperature
    bool spn109_coolantPressure;       // Coolant pressure

    // Fuel
    bool spn174_fuelTemp;              // Fuel temperature
    bool spn94_fuelDeliveryPressure;   // Fuel delivery pressure

    // Turbocharger
    bool spn1127_turbo1BoostPressure;  // Turbocharger 1 boost pressure
    bool spn1128_turbo2BoostPressure;  // Turbocharger 2 boost pressure

    // Other
    bool spn441_engineBayTemp;         // Engine bay temperature
};

// Main configuration structure (stored in EEPROM)
struct AppConfig {
    uint32_t magic;               // CONFIG_MAGIC validates EEPROM contents
    uint8_t version;              // Configuration version for migration
    uint8_t j1939SourceAddress;   // J1939 source address (default 149)
    uint8_t reserved[2];          // Padding for alignment

    // Hardware device configuration
    TAdsDeviceConfig adsDevices[ADS_DEVICE_COUNT];
    TThermocoupleConfig thermocouple;
    TBme280Config bme280;

    // J1939 SPN enable flags
    TJ1939SpnConfig j1939Spns;

    // Sensor mappings (19 sensors)
    TSensorConfig sensors[MAX_SENSORS];

    // CRC32 for validation
    uint32_t checksum;
};

// Default AEM temperature sensor Steinhart-Hart coefficients
#define AEM_TEMP_COEFF_A 1.485995686e-03f
#define AEM_TEMP_COEFF_B 2.279654266e-04f
#define AEM_TEMP_COEFF_C 1.197578033e-07f
#define AEM_TEMP_RESISTOR 10050.0f

// Default pressure sensor voltage range (0.5V-4.5V typical)
#define PRESSURE_VOLTAGE_MIN 0.5f
#define PRESSURE_VOLTAGE_MAX 4.5f

#endif  // OSSM_APPCONFIG_H
