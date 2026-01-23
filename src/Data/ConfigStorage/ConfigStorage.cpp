#include "ConfigStorage.h"
#include <EEPROM.h>
#include "Display/Crc32.h"  // cnext-generated memory-safe checksum

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
    configToSave.checksum = Crc32_calculateChecksum(&configToSave);

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
    uint32_t calculatedChecksum = Crc32_calculateChecksum(config);
    if (calculatedChecksum != config->checksum) {
        return false;
    }

    return true;
}

void ConfigStorage::loadDefaults(AppConfig* config) {
    // Clear structure using value initialization (portable for floats)
    *config = AppConfig{};

    // Header
    config->magic = CONFIG_MAGIC;
    config->version = CONFIG_VERSION;
    config->j1939SourceAddress = 149;

    // All temperature inputs disabled by default
    // Pre-load with AEM NTC coefficients so they're ready when enabled
    for (int i = 0; i < TEMP_INPUT_COUNT; i++) {
        config->tempInputs[i].assignedSpn = 0;  // Disabled
        config->tempInputs[i].coeffA = AEM_TEMP_COEFF_A;
        config->tempInputs[i].coeffB = AEM_TEMP_COEFF_B;
        config->tempInputs[i].coeffC = AEM_TEMP_COEFF_C;
        config->tempInputs[i].resistorValue = AEM_TEMP_RESISTOR;
    }

    // All pressure inputs disabled by default
    // Pre-load with 100 PSIG range (gauge pressure)
    for (int i = 0; i < PRESSURE_INPUT_COUNT; i++) {
        config->pressureInputs[i].assignedSpn = 0;  // Disabled
        config->pressureInputs[i].maxPressure = 100;
        config->pressureInputs[i].pressureType = PRESSURE_TYPE_PSIG;
        config->pressureInputs[i].reserved = 0;
    }

    // EGT disabled by default
    config->egtEnabled = false;
    config->thermocoupleType = TC_TYPE_K;

    // BME280 disabled by default
    config->bme280Enabled = false;

    // Calculate and set checksum
    config->checksum = Crc32_calculateChecksum(config);
}

