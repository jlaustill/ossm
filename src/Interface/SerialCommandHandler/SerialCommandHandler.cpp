#include "SerialCommandHandler.h"
#include <Arduino.h>
#include "Data/ConfigStorage/ConfigStorage.h"
#include "Data/MAX31856Manager/MAX31856Manager.h"
#include "Data/BME280Manager/BME280Manager.h"
#include "Domain/CommandHandler/CommandHandler.h"

// Static member initialization
AppConfig* SerialCommandHandler::config = nullptr;
AppData* SerialCommandHandler::appData = nullptr;
char SerialCommandHandler::cmdBuffer[128];
uint8_t SerialCommandHandler::cmdIndex = 0;
SeaDash::Parse::ParseResult SerialCommandHandler::parsed = {{0}, 0, false};

void SerialCommandHandler::initialize(AppConfig* cfg, AppData* data) {
    config = cfg;
    appData = data;
    cmdIndex = 0;
    memset(cmdBuffer, 0, sizeof(cmdBuffer));

    Serial.println("OSSM Command Interface Ready (byte format)");
    Serial.println("Commands: 1,spnHi,spnLo,en,input | 5,query | 6 | 7 | 8,in,preset | 9,in,preset");
}

void SerialCommandHandler::update() {
    while (Serial.available()) {
        char c = static_cast<char>(Serial.read());

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

    // Copy command for parsing (parse modifies the buffer)
    char cmdCopy[128];
    strncpy(cmdCopy, cmd, sizeof(cmdCopy) - 1);
    cmdCopy[sizeof(cmdCopy) - 1] = '\0';

    // Parse all tokens as bytes
    parsed = SeaDash::Parse::parse(cmdCopy, ',');
    if (!parsed.success || parsed.count < 1) {
        Serial.println("ERR,1,Parse failed");
        return;
    }

    uint8_t cmdNum = parsed.data[0];

    switch (cmdNum) {
        case 1:  // Enable/Disable SPN
            handleEnableSpn();
            break;
        case 3:  // Set Pressure Range
            handleSetPressureRange();
            break;
        case 4:  // Set Thermocouple Type
            handleSetTcType();
            break;
        case 5:  // Query Configuration
            handleQuery();
            break;
        case 6:  // Save Configuration
            handleSave();
            break;
        case 7:  // Reset to Defaults
            handleReset();
            break;
        case 8:  // NTC Preset
            handleNtcPreset();
            break;
        case 9:  // Pressure Preset
            handlePressurePreset();
            break;
        case 10:  // Read Live Sensors
            handleReadSensors();
            break;
        default:
            Serial.println("ERR,1,Unknown command");
            break;
    }

    // Report any active faults after command output
    reportFaults();
}

void SerialCommandHandler::handleEnableSpn() {
    // Format: 1,spnHi,spnLo,enable[,input]
    if (parsed.count < 4) {
        Serial.println("ERR,2,Usage: 1,spnHi,spnLo,enable[,input]");
        return;
    }

    uint16_t spn = (static_cast<uint16_t>(parsed.data[1]) << 8) | parsed.data[2];
    bool enable = (parsed.data[3] != 0);
    uint8_t input = (parsed.count > 4) ? parsed.data[4] : 0;

    TCommandResult result = CommandHandler::enableSpn(spn, enable, input);

    if (result.errorCode != ECommandError::OK) {
        Serial.print("ERR,");
        Serial.print(result.errorCode);
        switch (result.errorCode) {
            case ECommandError::UNKNOWN_SPN:
                Serial.print(",Unknown SPN: ");
                Serial.println(spn);
                break;
            case ECommandError::INVALID_TEMP_INPUT:
                Serial.println(",Input 1-8 required for temp SPN");
                break;
            case ECommandError::INVALID_PRESSURE_INPUT:
                Serial.println(",Input 1-7 required for pressure SPN");
                break;
            default:
                Serial.println();
                break;
        }
        return;
    }

    // Find category for success message
    ESpnCategory category = SPN_CAT_UNKNOWN;
    for (size_t i = 0; i < KNOWN_SPN_COUNT; i++) {
        if (KNOWN_SPNS[i].spn == spn) {
            category = KNOWN_SPNS[i].category;
            break;
        }
    }

    if (enable) {
        switch (category) {
            case SPN_CAT_TEMPERATURE:
                Serial.print("OK,SPN ");
                Serial.print(spn);
                Serial.print(" enabled on temp");
                Serial.println(input);
                break;
            case SPN_CAT_PRESSURE:
                Serial.print("OK,SPN ");
                Serial.print(spn);
                Serial.print(" enabled on pres");
                Serial.println(input);
                break;
            case SPN_CAT_EGT:
                Serial.println("OK,EGT enabled");
                break;
            case SPN_CAT_BME280:
                Serial.println("OK,BME280 enabled (SPNs 171, 108, 354)");
                break;
            default:
                Serial.println("OK");
                break;
        }
    } else {
        switch (category) {
            case SPN_CAT_EGT:
                Serial.println("OK,EGT disabled");
                break;
            case SPN_CAT_BME280:
                Serial.println("OK,BME280 disabled");
                break;
            default:
                Serial.print("OK,SPN ");
                Serial.print(spn);
                Serial.println(" disabled");
                break;
        }
    }
}

void SerialCommandHandler::handleSetPressureRange() {
    // Format: 3,input,valueHi,valueLo
    // Value meaning depends on pressure type:
    //   PSIA: centibar (100 = 1 bar), or bar + 20000 for large values
    //   PSIG: PSI directly
    // Use presets (cmd 9) for common values
    if (parsed.count < 4) {
        Serial.println("ERR,2,Usage: 3,input,valueHi,valueLo (use preset cmd 9)");
        return;
    }

    uint8_t input = parsed.data[1];
    uint16_t maxPressure = (static_cast<uint16_t>(parsed.data[2]) << 8) | parsed.data[3];

    TCommandResult result = CommandHandler::setPressureRange(input, maxPressure);

    if (result.errorCode != ECommandError::OK) {
        Serial.println("ERR,5,Input must be 1-7");
        return;
    }

    Serial.print("OK,pres");
    Serial.print(input);
    Serial.print(" maxPressure=");
    Serial.println(maxPressure);
}

void SerialCommandHandler::handleSetTcType() {
    // Format: 4,type
    if (parsed.count < 2) {
        Serial.println("ERR,2,Usage: 4,type (0-7)");
        return;
    }

    uint8_t type = parsed.data[1];
    TCommandResult result = CommandHandler::setTcType(type);

    if (result.errorCode != ECommandError::OK) {
        Serial.println("ERR,7,Type must be 0-7");
        return;
    }

    Serial.print("OK,Thermocouple type=");
    Serial.println(type);
}

void SerialCommandHandler::handleQuery() {
    // Format: 5[,queryType]
    // parsed.data[0] = cmd (5)
    // parsed.data[1] = queryType (optional, default 0)

    uint8_t queryType = (parsed.count > 1) ? parsed.data[1] : 0;

    switch (queryType) {
        case 0:  // Enabled SPNs
            Serial.println("=== Enabled SPNs ===");
            for (uint8_t i = 0; i < TEMP_INPUT_COUNT; i++) {
                if (config->tempInputs[i].assignedSpn != 0) {
                    Serial.print("temp");
                    Serial.print(i + 1);
                    Serial.print(": SPN ");
                    Serial.println(config->tempInputs[i].assignedSpn);
                }
            }
            for (uint8_t i = 0; i < PRESSURE_INPUT_COUNT; i++) {
                if (config->pressureInputs[i].assignedSpn != 0) {
                    Serial.print("pres");
                    Serial.print(i + 1);
                    Serial.print(": SPN ");
                    Serial.println(config->pressureInputs[i].assignedSpn);
                }
            }
            if (config->egtEnabled) {
                Serial.println("EGT: SPN 173");
            }
            if (config->bme280Enabled) {
                Serial.println("BME280: SPNs 171, 108, 354");
            }
            break;

        case 4:  // Full config
            Serial.println("=== Full Configuration ===");
            Serial.print("J1939 Address: ");
            Serial.println(config->j1939SourceAddress);
            Serial.print("EGT Enabled: ");
            Serial.println(config->egtEnabled ? "Yes" : "No");
            Serial.print("TC Type: ");
            Serial.println(config->thermocoupleType);
            Serial.print("BME280 Enabled: ");
            Serial.println(config->bme280Enabled ? "Yes" : "No");
            // Fall through to show SPNs
            parsed.data[1] = 0;  // Set queryType to 0 for recursive call
            handleQuery();
            break;

        default:
            Serial.println("ERR,8,Query type 0 or 4");
            break;
    }
}

void SerialCommandHandler::handleSave() {
    Serial.print("Saving configuration... ");
    TCommandResult result = CommandHandler::save();
    if (result.errorCode == ECommandError::OK) {
        Serial.println("OK");
    } else {
        Serial.println("ERR,9,Save failed");
    }
}

void SerialCommandHandler::handleReset() {
    Serial.println("Resetting to defaults...");
    CommandHandler::reset();
    Serial.println("OK,Use '6' to save");
}

void SerialCommandHandler::handleNtcPreset() {
    // Format: 8,input,preset
    if (parsed.count < 3) {
        Serial.println("ERR,2,Usage: 8,input,preset (0=AEM,1=Bosch,2=GM)");
        return;
    }

    uint8_t input = parsed.data[1];
    uint8_t preset = parsed.data[2];

    TCommandResult result = CommandHandler::applyNtcPreset(input, preset);

    if (result.errorCode != ECommandError::OK) {
        if (result.errorCode == ECommandError::INVALID_TEMP_INPUT) {
            Serial.println("ERR,4,Input must be 1-8");
        } else {
            Serial.println("ERR,10,Preset must be 0-2");
        }
        return;
    }

    const char* presetNames[] = {"AEM", "Bosch", "GM"};
    Serial.print("OK,temp");
    Serial.print(input);
    Serial.print(" set to ");
    Serial.print(presetNames[preset]);
    Serial.println(" preset");
}

void SerialCommandHandler::handlePressurePreset() {
    // Format: 9,input,preset
    // Bar presets (0-15): PSIA absolute
    // PSI presets (20-30): PSIG gauge
    if (parsed.count < 3) {
        Serial.println("ERR,2,Usage: 9,input,preset (0-15=bar, 20-30=PSIG)");
        return;
    }

    uint8_t input = parsed.data[1];
    uint8_t preset = parsed.data[2];

    TCommandResult result = CommandHandler::applyPressurePreset(input, preset);

    if (result.errorCode != ECommandError::OK) {
        if (result.errorCode == ECommandError::INVALID_PRESSURE_INPUT) {
            Serial.println("ERR,5,Input must be 1-7");
        } else {
            Serial.println("ERR,10,Preset must be 0-15 (bar) or 20-30 (PSIG)");
        }
        return;
    }

    Serial.print("OK,pres");
    Serial.print(input);
    Serial.print(" set to ");

    if (preset <= 15) {
        // Bar presets
        static const char* barNames[] = {
            "1", "1.5", "2", "2.5", "3", "4", "5", "7",
            "10", "50", "100", "150", "200", "1000", "2000", "3000"
        };
        Serial.print(barNames[preset]);
        Serial.println(" bar (PSIA)");
    } else if (preset >= 20 && preset <= 30) {
        // PSI presets
        static const uint16_t psiValues[] = {
            15, 30, 50, 100, 150, 200, 250, 300, 350, 400, 500
        };
        Serial.print(psiValues[preset - 20]);
        Serial.println(" PSIG");
    }
}

void SerialCommandHandler::handleReadSensors() {
    // Format: 10[,sensorType]
    // parsed.data[0] = cmd (10)
    // parsed.data[1] = sensorType (optional, default 0)
    //   0 = all active sensors
    //   1 = EGT only
    //   2 = temperatures
    //   3 = pressures
    //   4 = BME280

    uint8_t sensorType = (parsed.count > 1) ? parsed.data[1] : 0;

    switch (sensorType) {
        case 0:  // All active sensors
            Serial.println("=== Live Sensor Values ===");
            // EGT
            if (config->egtEnabled) {
                Serial.print("EGT: ");
                if (MAX31856Manager::getFaultStatus() == 0) {
                    Serial.print(appData->egtTemperatureC, 1);
                    Serial.println(" C");
                } else {
                    Serial.println("FAULT");
                }
            }
            // Temperatures
            for (uint8_t i = 0; i < TEMP_INPUT_COUNT; i++) {
                if (config->tempInputs[i].assignedSpn != 0) {
                    Serial.print("temp");
                    Serial.print(i + 1);
                    Serial.print(" (SPN ");
                    Serial.print(config->tempInputs[i].assignedSpn);
                    Serial.print("): ");
                    printTempValueForSpn(config->tempInputs[i].assignedSpn);
                }
            }
            // Pressures
            for (uint8_t i = 0; i < PRESSURE_INPUT_COUNT; i++) {
                if (config->pressureInputs[i].assignedSpn != 0) {
                    Serial.print("pres");
                    Serial.print(i + 1);
                    Serial.print(" (SPN ");
                    Serial.print(config->pressureInputs[i].assignedSpn);
                    Serial.print("): ");
                    printPressureValueForSpn(config->pressureInputs[i].assignedSpn);
                }
            }
            // BME280
            if (config->bme280Enabled) {
                Serial.print("Ambient Temp: ");
                Serial.print(appData->ambientTemperatureC, 1);
                Serial.println(" C");
                Serial.print("Humidity: ");
                Serial.print(appData->humidity, 1);
                Serial.println(" %");
                Serial.print("Barometric: ");
                Serial.print(appData->absoluteBarometricpressurekPa, 2);
                Serial.println(" kPa");
            }
            break;

        case 1:  // EGT only
            if (!config->egtEnabled) {
                Serial.println("ERR,11,EGT not enabled");
                return;
            }
            Serial.print("EGT: ");
            if (MAX31856Manager::getFaultStatus() == 0) {
                Serial.print(appData->egtTemperatureC, 1);
                Serial.println(" C");
            } else {
                Serial.print("FAULT (0x");
                Serial.print(MAX31856Manager::getFaultStatus(), HEX);
                Serial.println(")");
            }
            break;

        case 2:  // Temperatures
            Serial.println("=== Temperature Sensors ===");
            for (uint8_t i = 0; i < TEMP_INPUT_COUNT; i++) {
                if (config->tempInputs[i].assignedSpn != 0) {
                    Serial.print("temp");
                    Serial.print(i + 1);
                    Serial.print(": ");
                    printTempValueForSpn(config->tempInputs[i].assignedSpn);
                }
            }
            break;

        case 3:  // Pressures
            Serial.println("=== Pressure Sensors ===");
            for (uint8_t i = 0; i < PRESSURE_INPUT_COUNT; i++) {
                if (config->pressureInputs[i].assignedSpn != 0) {
                    Serial.print("pres");
                    Serial.print(i + 1);
                    Serial.print(": ");
                    printPressureValueForSpn(config->pressureInputs[i].assignedSpn);
                }
            }
            break;

        case 4:  // BME280
            if (!config->bme280Enabled) {
                Serial.println("ERR,12,BME280 not enabled");
                return;
            }
            Serial.println("=== BME280 Ambient ===");
            Serial.print("Temperature: ");
            Serial.print(appData->ambientTemperatureC, 1);
            Serial.println(" C");
            Serial.print("Humidity: ");
            Serial.print(appData->humidity, 1);
            Serial.println(" %");
            Serial.print("Pressure: ");
            Serial.print(appData->absoluteBarometricpressurekPa, 2);
            Serial.println(" kPa");
            break;

        default:
            Serial.println("ERR,11,Sensor type 0-4");
            break;
    }
}

void SerialCommandHandler::reportFaults() {
    // Check for any active faults
    uint8_t egtFault = MAX31856Manager::getFaultStatus();

    // Only show fault section if there are faults
    if (egtFault != 0) {
        Serial.println("--- FAULTS ---");
        Serial.print("EGT: 0x");
        Serial.print(egtFault, HEX);
        Serial.print(" (");

        // Decode fault bits
        if (egtFault & 0x01) Serial.print("OPEN ");
        if (egtFault & 0x02) Serial.print("OVUV ");
        if (egtFault & 0x04) Serial.print("TC_LOW ");
        if (egtFault & 0x08) Serial.print("TC_HIGH ");
        if (egtFault & 0x10) Serial.print("CJ_LOW ");
        if (egtFault & 0x20) Serial.print("CJ_HIGH ");
        if (egtFault & 0x40) Serial.print("TC_RANGE ");
        if (egtFault & 0x80) Serial.print("CJ_RANGE ");

        Serial.println(")");
    }
}

void SerialCommandHandler::printTempValueForSpn(uint16_t spn) {
    float value = -273.15f;  // Error value
    switch (spn) {
        case 175: value = appData->oilTemperatureC; break;
        case 110:
        case 1637: value = appData->coolantTemperatureC; break;
        case 174: value = appData->fuelTemperatureC; break;
        case 105:
        case 1363: value = appData->boostTemperatureC; break;
        case 1131: value = appData->cacInletTemperatureC; break;
        case 1132: value = appData->transferPipeTemperatureC; break;
        case 1133:
        case 172: value = appData->airInletTemperatureC; break;
        case 441: value = appData->engineBayTemperatureC; break;
    }
    if (value < -200.0f) {
        Serial.println("ERROR");
    } else {
        Serial.print(value, 1);
        Serial.println(" C");
    }
}

void SerialCommandHandler::printPressureValueForSpn(uint16_t spn) {
    float value = 0.0f;
    switch (spn) {
        case 100: value = appData->oilPressurekPa; break;
        case 109: value = appData->coolantPressurekPa; break;
        case 94: value = appData->fuelPressurekPa; break;
        case 102: value = appData->boostPressurekPa; break;
        case 106: value = appData->airInletPressurekPa; break;
        case 1127: value = appData->cacInletPressurekPa; break;
        case 1128: value = appData->transferPipePressurekPa; break;
    }
    Serial.print(value, 2);
    Serial.println(" kPa");
}
