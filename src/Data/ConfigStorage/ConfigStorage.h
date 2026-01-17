#ifndef OSSM_CONFIGSTORAGE_H
#define OSSM_CONFIGSTORAGE_H

#include "AppConfig.h"

#define EEPROM_CONFIG_ADDRESS 0

class ConfigStorage {
   public:
    // Load configuration from EEPROM into provided struct
    // Returns true if valid config was loaded, false if defaults were used
    static bool loadConfig(AppConfig* config);

    // Save configuration to EEPROM
    // Returns true on success
    static bool saveConfig(const AppConfig* config);

    // Load default configuration values
    static void loadDefaults(AppConfig* config);

    // Validate configuration structure
    static bool validateConfig(const AppConfig* config);

};

#endif  // OSSM_CONFIGSTORAGE_H
