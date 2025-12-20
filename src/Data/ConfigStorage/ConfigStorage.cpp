#include "ConfigStorage.h"
#include <EEPROM.h>

bool ConfigStorage::loadConfig(AppConfig* config) {
    EEPROM.get(EEPROM_CONFIG_ADDRESS, *config);

    if (!validateConfig(config)) {
        loadDefaults(config);
        saveConfig(config);
        return false;
    }

    return true;
}

bool ConfigStorage::saveConfig(const AppConfig* config) {
    // Create a copy with updated checksum
    AppConfig configToSave = *config;
    configToSave.checksum = calculateChecksum(&configToSave);

    EEPROM.put(EEPROM_CONFIG_ADDRESS, configToSave);
    return true;
}

bool ConfigStorage::validateConfig(const AppConfig* config) {
    // Check magic number
    if (config->magic != CONFIG_MAGIC) {
        return false;
    }

    // Check version
    if (config->version != CONFIG_VERSION) {
        return false;
    }

    // Verify checksum
    uint32_t calculatedChecksum = calculateChecksum(config);
    if (calculatedChecksum != config->checksum) {
        return false;
    }

    return true;
}

void ConfigStorage::loadDefaults(AppConfig* config) {
    // Clear structure
    memset(config, 0, sizeof(AppConfig));

    // Header
    config->magic = CONFIG_MAGIC;
    config->version = CONFIG_VERSION;
    config->j1939SourceAddress = 149;

    // ADS1115 device configuration (from schematic)
    // ADS @ 0x48, DRDY on D0
    config->adsDevices[0].i2cAddress = 0x48;
    config->adsDevices[0].drdyPin = 0;
    config->adsDevices[0].enabled = true;

    // ADS @ 0x49, DRDY on D1
    config->adsDevices[1].i2cAddress = 0x49;
    config->adsDevices[1].drdyPin = 1;
    config->adsDevices[1].enabled = true;

    // ADS @ 0x4A, DRDY on D4
    config->adsDevices[2].i2cAddress = 0x4A;
    config->adsDevices[2].drdyPin = 4;
    config->adsDevices[2].enabled = true;

    // ADS @ 0x4B, DRDY on D5
    config->adsDevices[3].i2cAddress = 0x4B;
    config->adsDevices[3].drdyPin = 5;
    config->adsDevices[3].enabled = true;

    // MAX31856 thermocouple configuration
    config->thermocouple.csPin = 10;
    config->thermocouple.drdyPin = 2;
    config->thermocouple.faultPin = 3;
    config->thermocouple.enabled = true;

    // BME280 ambient sensor configuration
    config->bme280.i2cAddress = 0x76;
    config->bme280.enabled = true;

    // J1939 SPN enable flags (default based on original configuration.h)
    // Ambient Sensors - disabled by default
    config->j1939Spns.spn171_ambientAirTemp = false;
    config->j1939Spns.spn108_barometricPressure = false;
    config->j1939Spns.spn354_relativeHumidity = false;

    // EGT - disabled by default
    config->j1939Spns.spn173_exhaustGasTemp = false;

    // Intake Position 1 - enabled by default
    config->j1939Spns.spn102_boostPressure = true;
    config->j1939Spns.spn105_intakeManifold1Temp = true;
    config->j1939Spns.spn1363_intakeManifold1AirTemp = true;

    // Intake Positions 2-4 - disabled by default
    config->j1939Spns.spn1131_intakeManifold2Temp = false;
    config->j1939Spns.spn1132_intakeManifold3Temp = false;
    config->j1939Spns.spn1133_intakeManifold4Temp = false;
    config->j1939Spns.spn106_airInletPressure = false;
    config->j1939Spns.spn172_airInletTemp = false;

    // Engine Oil - disabled by default
    config->j1939Spns.spn175_engineOilTemp = false;
    config->j1939Spns.spn100_engineOilPressure = false;

    // Engine Coolant - disabled by default
    config->j1939Spns.spn110_engineCoolantTemp = false;
    config->j1939Spns.spn109_coolantPressure = false;

    // Fuel - fuel pressure enabled by default
    config->j1939Spns.spn174_fuelTemp = false;
    config->j1939Spns.spn94_fuelDeliveryPressure = true;

    // Turbocharger - disabled by default
    config->j1939Spns.spn1127_turbo1BoostPressure = false;
    config->j1939Spns.spn1128_turbo2BoostPressure = false;

    // Other - disabled by default
    config->j1939Spns.spn441_engineBayTemp = false;

    // Default sensor configurations based on schematic channel mapping
    // All sensors disabled by default - user configures via Serial/J1939
    // This provides the hardware mapping, user assigns meaning

    uint8_t sensorIdx = 0;

    // ----- ADS 0x48 channels -----
    // temp1 on 0x48 ch0
    config->sensors[sensorIdx].sensorType = SENSOR_DISABLED;
    config->sensors[sensorIdx].adsDevice = 0;
    config->sensors[sensorIdx].adsChannel = 0;
    config->sensors[sensorIdx].appDataIndex = IDX_OIL_TEMP;  // Default mapping
    config->sensors[sensorIdx].coeffA = AEM_TEMP_COEFF_A;
    config->sensors[sensorIdx].coeffB = AEM_TEMP_COEFF_B;
    config->sensors[sensorIdx].coeffC = AEM_TEMP_COEFF_C;
    config->sensors[sensorIdx].resistorValue = AEM_TEMP_RESISTOR;
    config->sensors[sensorIdx].wiringResistance = 0.0f;
    sensorIdx++;

    // temp2 on 0x48 ch1
    config->sensors[sensorIdx].sensorType = SENSOR_DISABLED;
    config->sensors[sensorIdx].adsDevice = 0;
    config->sensors[sensorIdx].adsChannel = 1;
    config->sensors[sensorIdx].appDataIndex = IDX_COOLANT_TEMP;
    config->sensors[sensorIdx].coeffA = AEM_TEMP_COEFF_A;
    config->sensors[sensorIdx].coeffB = AEM_TEMP_COEFF_B;
    config->sensors[sensorIdx].coeffC = AEM_TEMP_COEFF_C;
    config->sensors[sensorIdx].resistorValue = AEM_TEMP_RESISTOR;
    config->sensors[sensorIdx].wiringResistance = 0.0f;
    sensorIdx++;

    // pres1 on 0x48 ch2
    config->sensors[sensorIdx].sensorType = SENSOR_DISABLED;
    config->sensors[sensorIdx].adsDevice = 0;
    config->sensors[sensorIdx].adsChannel = 2;
    config->sensors[sensorIdx].appDataIndex = IDX_OIL_PRESSURE;
    config->sensors[sensorIdx].coeffA = PRESSURE_VOLTAGE_MIN;
    config->sensors[sensorIdx].coeffB = PRESSURE_VOLTAGE_MAX;
    config->sensors[sensorIdx].coeffC = 100.0f;  // 100 PSI default
    config->sensors[sensorIdx].resistorValue = 0.0f;
    config->sensors[sensorIdx].wiringResistance = 0.0f;
    sensorIdx++;

    // pres2 on 0x48 ch3
    config->sensors[sensorIdx].sensorType = SENSOR_DISABLED;
    config->sensors[sensorIdx].adsDevice = 0;
    config->sensors[sensorIdx].adsChannel = 3;
    config->sensors[sensorIdx].appDataIndex = IDX_COOLANT_PRESSURE;
    config->sensors[sensorIdx].coeffA = PRESSURE_VOLTAGE_MIN;
    config->sensors[sensorIdx].coeffB = PRESSURE_VOLTAGE_MAX;
    config->sensors[sensorIdx].coeffC = 100.0f;
    config->sensors[sensorIdx].resistorValue = 0.0f;
    config->sensors[sensorIdx].wiringResistance = 0.0f;
    sensorIdx++;

    // ----- ADS 0x49 channels -----
    // temp3 on 0x49 ch0
    config->sensors[sensorIdx].sensorType = SENSOR_DISABLED;
    config->sensors[sensorIdx].adsDevice = 1;
    config->sensors[sensorIdx].adsChannel = 0;
    config->sensors[sensorIdx].appDataIndex = IDX_TRANSFER_PIPE_TEMP;
    config->sensors[sensorIdx].coeffA = AEM_TEMP_COEFF_A;
    config->sensors[sensorIdx].coeffB = AEM_TEMP_COEFF_B;
    config->sensors[sensorIdx].coeffC = AEM_TEMP_COEFF_C;
    config->sensors[sensorIdx].resistorValue = AEM_TEMP_RESISTOR;
    config->sensors[sensorIdx].wiringResistance = 0.0f;
    sensorIdx++;

    // temp4 on 0x49 ch1
    config->sensors[sensorIdx].sensorType = SENSOR_DISABLED;
    config->sensors[sensorIdx].adsDevice = 1;
    config->sensors[sensorIdx].adsChannel = 1;
    config->sensors[sensorIdx].appDataIndex = IDX_BOOST_TEMP;
    config->sensors[sensorIdx].coeffA = AEM_TEMP_COEFF_A;
    config->sensors[sensorIdx].coeffB = AEM_TEMP_COEFF_B;
    config->sensors[sensorIdx].coeffC = AEM_TEMP_COEFF_C;
    config->sensors[sensorIdx].resistorValue = AEM_TEMP_RESISTOR;
    config->sensors[sensorIdx].wiringResistance = 0.0f;
    sensorIdx++;

    // pres5 on 0x49 ch2
    config->sensors[sensorIdx].sensorType = SENSOR_DISABLED;
    config->sensors[sensorIdx].adsDevice = 1;
    config->sensors[sensorIdx].adsChannel = 2;
    config->sensors[sensorIdx].appDataIndex = IDX_TRANSFER_PIPE_PRESSURE;
    config->sensors[sensorIdx].coeffA = PRESSURE_VOLTAGE_MIN;
    config->sensors[sensorIdx].coeffB = PRESSURE_VOLTAGE_MAX;
    config->sensors[sensorIdx].coeffC = 100.0f;
    config->sensors[sensorIdx].resistorValue = 0.0f;
    config->sensors[sensorIdx].wiringResistance = 0.0f;
    sensorIdx++;

    // pres6 on 0x49 ch3
    config->sensors[sensorIdx].sensorType = SENSOR_DISABLED;
    config->sensors[sensorIdx].adsDevice = 1;
    config->sensors[sensorIdx].adsChannel = 3;
    config->sensors[sensorIdx].appDataIndex = IDX_BOOST_PRESSURE;
    config->sensors[sensorIdx].coeffA = PRESSURE_VOLTAGE_MIN;
    config->sensors[sensorIdx].coeffB = PRESSURE_VOLTAGE_MAX;
    config->sensors[sensorIdx].coeffC = 150.0f;  // 150 PSI for boost
    config->sensors[sensorIdx].resistorValue = 0.0f;
    config->sensors[sensorIdx].wiringResistance = 0.0f;
    sensorIdx++;

    // ----- ADS 0x4A channels -----
    // pres3 on 0x4A ch0
    config->sensors[sensorIdx].sensorType = SENSOR_DISABLED;
    config->sensors[sensorIdx].adsDevice = 2;
    config->sensors[sensorIdx].adsChannel = 0;
    config->sensors[sensorIdx].appDataIndex = IDX_CAC_INLET_PRESSURE;
    config->sensors[sensorIdx].coeffA = PRESSURE_VOLTAGE_MIN;
    config->sensors[sensorIdx].coeffB = PRESSURE_VOLTAGE_MAX;
    config->sensors[sensorIdx].coeffC = 100.0f;
    config->sensors[sensorIdx].resistorValue = 0.0f;
    config->sensors[sensorIdx].wiringResistance = 0.0f;
    sensorIdx++;

    // temp5 on 0x4A ch1
    config->sensors[sensorIdx].sensorType = SENSOR_DISABLED;
    config->sensors[sensorIdx].adsDevice = 2;
    config->sensors[sensorIdx].adsChannel = 1;
    config->sensors[sensorIdx].appDataIndex = IDX_CAC_INLET_TEMP;
    config->sensors[sensorIdx].coeffA = AEM_TEMP_COEFF_A;
    config->sensors[sensorIdx].coeffB = AEM_TEMP_COEFF_B;
    config->sensors[sensorIdx].coeffC = AEM_TEMP_COEFF_C;
    config->sensors[sensorIdx].resistorValue = AEM_TEMP_RESISTOR;
    config->sensors[sensorIdx].wiringResistance = 0.0f;
    sensorIdx++;

    // temp6 on 0x4A ch2
    config->sensors[sensorIdx].sensorType = SENSOR_DISABLED;
    config->sensors[sensorIdx].adsDevice = 2;
    config->sensors[sensorIdx].adsChannel = 2;
    config->sensors[sensorIdx].appDataIndex = IDX_AIR_INLET_TEMP;
    config->sensors[sensorIdx].coeffA = AEM_TEMP_COEFF_A;
    config->sensors[sensorIdx].coeffB = AEM_TEMP_COEFF_B;
    config->sensors[sensorIdx].coeffC = AEM_TEMP_COEFF_C;
    config->sensors[sensorIdx].resistorValue = AEM_TEMP_RESISTOR;
    config->sensors[sensorIdx].wiringResistance = 0.0f;
    sensorIdx++;

    // pres4 on 0x4A ch3
    config->sensors[sensorIdx].sensorType = SENSOR_DISABLED;
    config->sensors[sensorIdx].adsDevice = 2;
    config->sensors[sensorIdx].adsChannel = 3;
    config->sensors[sensorIdx].appDataIndex = IDX_AIR_INLET_PRESSURE;
    config->sensors[sensorIdx].coeffA = PRESSURE_VOLTAGE_MIN;
    config->sensors[sensorIdx].coeffB = PRESSURE_VOLTAGE_MAX;
    config->sensors[sensorIdx].coeffC = 100.0f;
    config->sensors[sensorIdx].resistorValue = 0.0f;
    config->sensors[sensorIdx].wiringResistance = 0.0f;
    sensorIdx++;

    // ----- ADS 0x4B channels -----
    // pres7 on 0x4B ch0
    config->sensors[sensorIdx].sensorType = SENSOR_DISABLED;
    config->sensors[sensorIdx].adsDevice = 3;
    config->sensors[sensorIdx].adsChannel = 0;
    config->sensors[sensorIdx].appDataIndex = IDX_FUEL_PRESSURE;
    config->sensors[sensorIdx].coeffA = PRESSURE_VOLTAGE_MIN;
    config->sensors[sensorIdx].coeffB = PRESSURE_VOLTAGE_MAX;
    config->sensors[sensorIdx].coeffC = 100.0f;
    config->sensors[sensorIdx].resistorValue = 0.0f;
    config->sensors[sensorIdx].wiringResistance = 0.0f;
    sensorIdx++;

    // temp7 on 0x4B ch1
    config->sensors[sensorIdx].sensorType = SENSOR_DISABLED;
    config->sensors[sensorIdx].adsDevice = 3;
    config->sensors[sensorIdx].adsChannel = 1;
    config->sensors[sensorIdx].appDataIndex = IDX_FUEL_TEMP;
    config->sensors[sensorIdx].coeffA = AEM_TEMP_COEFF_A;
    config->sensors[sensorIdx].coeffB = AEM_TEMP_COEFF_B;
    config->sensors[sensorIdx].coeffC = AEM_TEMP_COEFF_C;
    config->sensors[sensorIdx].resistorValue = AEM_TEMP_RESISTOR;
    config->sensors[sensorIdx].wiringResistance = 0.0f;
    sensorIdx++;

    // temp8 on 0x4B ch2
    config->sensors[sensorIdx].sensorType = SENSOR_DISABLED;
    config->sensors[sensorIdx].adsDevice = 3;
    config->sensors[sensorIdx].adsChannel = 2;
    config->sensors[sensorIdx].appDataIndex = IDX_ENGINE_BAY_TEMP;
    config->sensors[sensorIdx].coeffA = AEM_TEMP_COEFF_A;
    config->sensors[sensorIdx].coeffB = AEM_TEMP_COEFF_B;
    config->sensors[sensorIdx].coeffC = AEM_TEMP_COEFF_C;
    config->sensors[sensorIdx].resistorValue = AEM_TEMP_RESISTOR;
    config->sensors[sensorIdx].wiringResistance = 0.0f;
    sensorIdx++;

    // ----- Non-ADS sensors -----
    // EGT (MAX31856 thermocouple)
    config->sensors[sensorIdx].sensorType = SENSOR_DISABLED;
    config->sensors[sensorIdx].adsDevice = 0xFF;  // Not ADS-based
    config->sensors[sensorIdx].adsChannel = 0xFF;
    config->sensors[sensorIdx].appDataIndex = IDX_EGT_TEMP;
    config->sensors[sensorIdx].coeffA = 0.0f;
    config->sensors[sensorIdx].coeffB = 0.0f;
    config->sensors[sensorIdx].coeffC = 0.0f;
    config->sensors[sensorIdx].resistorValue = 0.0f;
    config->sensors[sensorIdx].wiringResistance = 0.0f;
    sensorIdx++;

    // BME280 - Ambient Temperature
    config->sensors[sensorIdx].sensorType = SENSOR_DISABLED;
    config->sensors[sensorIdx].adsDevice = 0xFF;
    config->sensors[sensorIdx].adsChannel = 0xFF;
    config->sensors[sensorIdx].appDataIndex = IDX_AMBIENT_TEMP;
    config->sensors[sensorIdx].coeffA = 0.0f;
    config->sensors[sensorIdx].coeffB = 0.0f;
    config->sensors[sensorIdx].coeffC = 0.0f;
    config->sensors[sensorIdx].resistorValue = 0.0f;
    config->sensors[sensorIdx].wiringResistance = 0.0f;
    sensorIdx++;

    // BME280 - Humidity
    config->sensors[sensorIdx].sensorType = SENSOR_DISABLED;
    config->sensors[sensorIdx].adsDevice = 0xFF;
    config->sensors[sensorIdx].adsChannel = 0xFF;
    config->sensors[sensorIdx].appDataIndex = IDX_HUMIDITY;
    config->sensors[sensorIdx].coeffA = 0.0f;
    config->sensors[sensorIdx].coeffB = 0.0f;
    config->sensors[sensorIdx].coeffC = 0.0f;
    config->sensors[sensorIdx].resistorValue = 0.0f;
    config->sensors[sensorIdx].wiringResistance = 0.0f;
    sensorIdx++;

    // BME280 - Barometric Pressure
    config->sensors[sensorIdx].sensorType = SENSOR_DISABLED;
    config->sensors[sensorIdx].adsDevice = 0xFF;
    config->sensors[sensorIdx].adsChannel = 0xFF;
    config->sensors[sensorIdx].appDataIndex = IDX_BAROMETRIC_PRESSURE;
    config->sensors[sensorIdx].coeffA = 0.0f;
    config->sensors[sensorIdx].coeffB = 0.0f;
    config->sensors[sensorIdx].coeffC = 0.0f;
    config->sensors[sensorIdx].resistorValue = 0.0f;
    config->sensors[sensorIdx].wiringResistance = 0.0f;

    // Calculate and set checksum
    config->checksum = calculateChecksum(config);
}

uint32_t ConfigStorage::calculateChecksum(const AppConfig* config) {
    // Simple CRC32 implementation
    // Calculate over entire struct except the checksum field itself
    const uint8_t* data = reinterpret_cast<const uint8_t*>(config);
    size_t length = sizeof(AppConfig) - sizeof(config->checksum);

    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }
    return ~crc;
}
