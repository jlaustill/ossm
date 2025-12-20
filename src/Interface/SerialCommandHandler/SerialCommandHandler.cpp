#include "SerialCommandHandler.h"
#include <Arduino.h>
#include <string.h>
#include "Data/ConfigStorage/ConfigStorage.h"

// Static member initialization
AppConfig* SerialCommandHandler::config = nullptr;
AppData* SerialCommandHandler::appData = nullptr;
char SerialCommandHandler::cmdBuffer[128];
uint8_t SerialCommandHandler::cmdIndex = 0;

void SerialCommandHandler::initialize(AppConfig* cfg, AppData* data) {
    config = cfg;
    appData = data;
    cmdIndex = 0;
    memset(cmdBuffer, 0, sizeof(cmdBuffer));

    Serial.println("Serial command handler ready. Type 'help' for commands.");
}

void SerialCommandHandler::update() {
    while (Serial.available()) {
        char c = Serial.read();

        if (c == '\n' || c == '\r') {
            if (cmdIndex > 0) {
                cmdBuffer[cmdIndex] = '\0';
                processCommand(cmdBuffer);
                cmdIndex = 0;
                memset(cmdBuffer, 0, sizeof(cmdBuffer));
            }
        } else if (cmdIndex < sizeof(cmdBuffer) - 1) {
            cmdBuffer[cmdIndex++] = c;
        }
    }
}

void SerialCommandHandler::processCommand(const char* cmd) {
    // Skip leading whitespace
    while (*cmd == ' ') cmd++;

    if (strlen(cmd) == 0) return;

    // Parse command and arguments
    char cmdCopy[128];
    strncpy(cmdCopy, cmd, sizeof(cmdCopy) - 1);
    cmdCopy[sizeof(cmdCopy) - 1] = '\0';

    char* token = strtok(cmdCopy, " ");
    if (token == nullptr) return;

    // Convert to lowercase for comparison
    for (char* p = token; *p; p++) *p = tolower(*p);

    if (strcmp(token, "help") == 0 || strcmp(token, "?") == 0) {
        cmdHelp();
    } else if (strcmp(token, "status") == 0) {
        cmdStatus();
    } else if (strcmp(token, "config") == 0) {
        cmdConfig();
    } else if (strcmp(token, "save") == 0) {
        cmdSave();
    } else if (strcmp(token, "reset") == 0) {
        cmdReset();
    } else if (strcmp(token, "spn") == 0) {
        char* args = strtok(nullptr, "");
        cmdSetSpn(args);
    } else if (strcmp(token, "sensor") == 0) {
        char* args = strtok(nullptr, "");
        cmdSetSensor(args);
    } else if (strcmp(token, "address") == 0) {
        char* args = strtok(nullptr, "");
        cmdSetAddress(args);
    } else {
        Serial.print("Unknown command: ");
        Serial.println(token);
        Serial.println("Type 'help' for available commands.");
    }
}

void SerialCommandHandler::cmdHelp() {
    Serial.println();
    Serial.println("=== OSSM Serial Commands ===");
    Serial.println("help          - Show this help");
    Serial.println("status        - Show current sensor readings");
    Serial.println("config        - Show current configuration");
    Serial.println("save          - Save configuration to EEPROM");
    Serial.println("reset         - Reset to default configuration");
    Serial.println("address <n>   - Set J1939 source address (0-253)");
    Serial.println("spn <name> <0|1> - Enable/disable SPN");
    Serial.println("  SPN names: boost, egt, ambient, humidity, barometric,");
    Serial.println("             oiltemp, oilpres, coolanttemp, coolantpres,");
    Serial.println("             fueltemp, fuelpres, enginebay");
    Serial.println("sensor <idx> <type> - Set sensor type (0=disabled)");
    Serial.println();
}

void SerialCommandHandler::cmdStatus() {
    Serial.println();
    Serial.println("=== Current Sensor Readings ===");
    Serial.print("Ambient Temp: "); Serial.print(appData->ambientTemperatureC); Serial.println(" C");
    Serial.print("Humidity: "); Serial.print(appData->humidity); Serial.println(" %");
    Serial.print("Barometric: "); Serial.print(appData->absoluteBarometricpressurekPa); Serial.println(" kPa");
    Serial.print("Boost Temp: "); Serial.print(appData->boostTemperatureC); Serial.println(" C");
    Serial.print("Boost Pressure: "); Serial.print(appData->boostPressurekPa); Serial.println(" kPa");
    Serial.print("Oil Temp: "); Serial.print(appData->oilTemperatureC); Serial.println(" C");
    Serial.print("Oil Pressure: "); Serial.print(appData->oilPressurekPa); Serial.println(" kPa");
    Serial.print("Coolant Temp: "); Serial.print(appData->coolantTemperatureC); Serial.println(" C");
    Serial.print("Coolant Pressure: "); Serial.print(appData->coolantPressurekPa); Serial.println(" kPa");
    Serial.print("Fuel Temp: "); Serial.print(appData->fuelTemperatureC); Serial.println(" C");
    Serial.print("Fuel Pressure: "); Serial.print(appData->fuelPressurekPa); Serial.println(" kPa");
    Serial.print("EGT: "); Serial.print(appData->egtTemperatureC); Serial.println(" C");
    Serial.print("Engine Bay Temp: "); Serial.print(appData->engineBayTemperatureC); Serial.println(" C");
    Serial.println();
}

void SerialCommandHandler::cmdConfig() {
    Serial.println();
    Serial.println("=== Current Configuration ===");
    Serial.print("J1939 Source Address: "); Serial.println(config->j1939SourceAddress);
    Serial.println();
    printSpnStatus();
    Serial.println();
}

void SerialCommandHandler::cmdSave() {
    Serial.print("Saving configuration to EEPROM... ");
    if (ConfigStorage::saveConfig(config)) {
        Serial.println("OK");
    } else {
        Serial.println("FAILED");
    }
}

void SerialCommandHandler::cmdReset() {
    Serial.println("Resetting to default configuration...");
    ConfigStorage::loadDefaults(config);
    Serial.println("Defaults loaded. Use 'save' to persist to EEPROM.");
}

void SerialCommandHandler::cmdSetSpn(const char* args) {
    if (args == nullptr || strlen(args) == 0) {
        printSpnStatus();
        return;
    }

    char argsCopy[64];
    strncpy(argsCopy, args, sizeof(argsCopy) - 1);
    argsCopy[sizeof(argsCopy) - 1] = '\0';

    // Skip leading whitespace
    char* p = argsCopy;
    while (*p == ' ') p++;

    char* name = strtok(p, " ");
    char* value = strtok(nullptr, " ");

    if (name == nullptr) {
        printSpnStatus();
        return;
    }

    if (value == nullptr) {
        Serial.println("Usage: spn <name> <0|1>");
        return;
    }

    bool enable = (atoi(value) != 0);

    // Convert name to lowercase
    for (char* c = name; *c; c++) *c = tolower(*c);

    bool found = true;

    if (strcmp(name, "boost") == 0) {
        config->j1939Spns.spn102_boostPressure = enable;
    } else if (strcmp(name, "egt") == 0) {
        config->j1939Spns.spn173_exhaustGasTemp = enable;
    } else if (strcmp(name, "ambient") == 0) {
        config->j1939Spns.spn171_ambientAirTemp = enable;
    } else if (strcmp(name, "humidity") == 0) {
        config->j1939Spns.spn354_relativeHumidity = enable;
    } else if (strcmp(name, "barometric") == 0) {
        config->j1939Spns.spn108_barometricPressure = enable;
    } else if (strcmp(name, "oiltemp") == 0) {
        config->j1939Spns.spn175_engineOilTemp = enable;
    } else if (strcmp(name, "oilpres") == 0) {
        config->j1939Spns.spn100_engineOilPressure = enable;
    } else if (strcmp(name, "coolanttemp") == 0) {
        config->j1939Spns.spn110_engineCoolantTemp = enable;
    } else if (strcmp(name, "coolantpres") == 0) {
        config->j1939Spns.spn109_coolantPressure = enable;
    } else if (strcmp(name, "fueltemp") == 0) {
        config->j1939Spns.spn174_fuelTemp = enable;
    } else if (strcmp(name, "fuelpres") == 0) {
        config->j1939Spns.spn94_fuelDeliveryPressure = enable;
    } else if (strcmp(name, "enginebay") == 0) {
        config->j1939Spns.spn441_engineBayTemp = enable;
    } else if (strcmp(name, "intake1") == 0) {
        config->j1939Spns.spn1363_intakeManifold1AirTemp = enable;
    } else if (strcmp(name, "intake2") == 0) {
        config->j1939Spns.spn1131_intakeManifold2Temp = enable;
    } else if (strcmp(name, "intake3") == 0) {
        config->j1939Spns.spn1132_intakeManifold3Temp = enable;
    } else if (strcmp(name, "intake4") == 0) {
        config->j1939Spns.spn1133_intakeManifold4Temp = enable;
    } else if (strcmp(name, "airinlet") == 0) {
        config->j1939Spns.spn106_airInletPressure = enable;
        config->j1939Spns.spn172_airInletTemp = enable;
    } else if (strcmp(name, "turbo1") == 0) {
        config->j1939Spns.spn1127_turbo1BoostPressure = enable;
    } else if (strcmp(name, "turbo2") == 0) {
        config->j1939Spns.spn1128_turbo2BoostPressure = enable;
    } else {
        found = false;
        Serial.print("Unknown SPN: ");
        Serial.println(name);
    }

    if (found) {
        Serial.print("SPN ");
        Serial.print(name);
        Serial.print(" = ");
        Serial.println(enable ? "enabled" : "disabled");
    }
}

void SerialCommandHandler::cmdSetSensor(const char* args) {
    if (args == nullptr || strlen(args) == 0) {
        printSensorConfig();
        return;
    }

    char argsCopy[64];
    strncpy(argsCopy, args, sizeof(argsCopy) - 1);
    argsCopy[sizeof(argsCopy) - 1] = '\0';

    char* p = argsCopy;
    while (*p == ' ') p++;

    char* idxStr = strtok(p, " ");
    char* typeStr = strtok(nullptr, " ");

    if (idxStr == nullptr || typeStr == nullptr) {
        Serial.println("Usage: sensor <index> <type>");
        Serial.println("Types: 0=disabled, 1=pres100, 2=pres150, 3=pres200,");
        Serial.println("       10=temp_aem, 11=temp_bosch, 12=temp_gm,");
        Serial.println("       20=thermocouple, 30=ambient_temp, 31=humidity, 32=baro");
        return;
    }

    int idx = atoi(idxStr);
    int type = atoi(typeStr);

    if (idx < 0 || idx >= MAX_SENSORS) {
        Serial.print("Invalid sensor index. Must be 0-");
        Serial.println(MAX_SENSORS - 1);
        return;
    }

    config->sensors[idx].sensorType = static_cast<ESensorType>(type);
    Serial.print("Sensor ");
    Serial.print(idx);
    Serial.print(" type = ");
    Serial.println(type);
}

void SerialCommandHandler::cmdSetAddress(const char* args) {
    if (args == nullptr || strlen(args) == 0) {
        Serial.print("Current J1939 source address: ");
        Serial.println(config->j1939SourceAddress);
        return;
    }

    char argsCopy[16];
    strncpy(argsCopy, args, sizeof(argsCopy) - 1);
    argsCopy[sizeof(argsCopy) - 1] = '\0';

    char* p = argsCopy;
    while (*p == ' ') p++;

    int addr = atoi(p);
    if (addr < 0 || addr > 253) {
        Serial.println("Invalid address. Must be 0-253.");
        return;
    }

    config->j1939SourceAddress = static_cast<uint8_t>(addr);
    Serial.print("J1939 source address = ");
    Serial.println(config->j1939SourceAddress);
}

void SerialCommandHandler::printSpnStatus() {
    Serial.println("J1939 SPN Enable Status:");
    Serial.print("  boost       : "); Serial.println(config->j1939Spns.spn102_boostPressure ? "ON" : "OFF");
    Serial.print("  egt         : "); Serial.println(config->j1939Spns.spn173_exhaustGasTemp ? "ON" : "OFF");
    Serial.print("  ambient     : "); Serial.println(config->j1939Spns.spn171_ambientAirTemp ? "ON" : "OFF");
    Serial.print("  humidity    : "); Serial.println(config->j1939Spns.spn354_relativeHumidity ? "ON" : "OFF");
    Serial.print("  barometric  : "); Serial.println(config->j1939Spns.spn108_barometricPressure ? "ON" : "OFF");
    Serial.print("  oiltemp     : "); Serial.println(config->j1939Spns.spn175_engineOilTemp ? "ON" : "OFF");
    Serial.print("  oilpres     : "); Serial.println(config->j1939Spns.spn100_engineOilPressure ? "ON" : "OFF");
    Serial.print("  coolanttemp : "); Serial.println(config->j1939Spns.spn110_engineCoolantTemp ? "ON" : "OFF");
    Serial.print("  coolantpres : "); Serial.println(config->j1939Spns.spn109_coolantPressure ? "ON" : "OFF");
    Serial.print("  fueltemp    : "); Serial.println(config->j1939Spns.spn174_fuelTemp ? "ON" : "OFF");
    Serial.print("  fuelpres    : "); Serial.println(config->j1939Spns.spn94_fuelDeliveryPressure ? "ON" : "OFF");
    Serial.print("  enginebay   : "); Serial.println(config->j1939Spns.spn441_engineBayTemp ? "ON" : "OFF");
    Serial.print("  intake1     : "); Serial.println(config->j1939Spns.spn1363_intakeManifold1AirTemp ? "ON" : "OFF");
    Serial.print("  intake2     : "); Serial.println(config->j1939Spns.spn1131_intakeManifold2Temp ? "ON" : "OFF");
    Serial.print("  intake3     : "); Serial.println(config->j1939Spns.spn1132_intakeManifold3Temp ? "ON" : "OFF");
    Serial.print("  intake4     : "); Serial.println(config->j1939Spns.spn1133_intakeManifold4Temp ? "ON" : "OFF");
    Serial.print("  airinlet    : "); Serial.println((config->j1939Spns.spn106_airInletPressure && config->j1939Spns.spn172_airInletTemp) ? "ON" : "OFF");
    Serial.print("  turbo1      : "); Serial.println(config->j1939Spns.spn1127_turbo1BoostPressure ? "ON" : "OFF");
    Serial.print("  turbo2      : "); Serial.println(config->j1939Spns.spn1128_turbo2BoostPressure ? "ON" : "OFF");
}

void SerialCommandHandler::printSensorConfig() {
    Serial.println("Sensor Configuration:");
    for (int i = 0; i < MAX_SENSORS; i++) {
        if (config->sensors[i].sensorType != SENSOR_DISABLED) {
            Serial.print("  [");
            Serial.print(i);
            Serial.print("] Type=");
            Serial.print(config->sensors[i].sensorType);
            Serial.print(" ADS=");
            Serial.print(config->sensors[i].adsDevice);
            Serial.print("/");
            Serial.print(config->sensors[i].adsChannel);
            Serial.print(" AppIdx=");
            Serial.println(config->sensors[i].appDataIndex);
        }
    }
}
