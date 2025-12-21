#include "SerialCommandHandler.h"
#include <Arduino.h>
#include "Data/ConfigStorage/ConfigStorage.h"

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
        default:
            Serial.println("ERR,1,Unknown command");
            break;
    }
}

void SerialCommandHandler::handleEnableSpn() {
    // Format: 1,spnHi,spnLo,enable[,input]
    // parsed.data[0] = cmd (1)
    // parsed.data[1] = spnHi
    // parsed.data[2] = spnLo
    // parsed.data[3] = enable
    // parsed.data[4] = input (optional)

    if (parsed.count < 4) {
        Serial.println("ERR,2,Usage: 1,spnHi,spnLo,enable[,input]");
        return;
    }

    uint16_t spn = (static_cast<uint16_t>(parsed.data[1]) << 8) | parsed.data[2];
    bool enable = (parsed.data[3] != 0);
    uint8_t input = (parsed.count > 4) ? parsed.data[4] : 0;

    // Find SPN category
    ESpnCategory category = SPN_CAT_UNKNOWN;
    for (size_t i = 0; i < KNOWN_SPN_COUNT; i++) {
        if (KNOWN_SPNS[i].spn == spn) {
            category = KNOWN_SPNS[i].category;
            break;
        }
    }

    if (category == SPN_CAT_UNKNOWN) {
        Serial.print("ERR,3,Unknown SPN: ");
        Serial.println(spn);
        return;
    }

    if (enable) {
        // Enable SPN
        switch (category) {
            case SPN_CAT_TEMPERATURE:
                if (input < 1 || input > TEMP_INPUT_COUNT) {
                    Serial.println("ERR,4,Input 1-8 required for temp SPN");
                    return;
                }
                config->tempInputs[input - 1].assignedSpn = spn;
                Serial.print("OK,SPN ");
                Serial.print(spn);
                Serial.print(" enabled on temp");
                Serial.println(input);
                break;

            case SPN_CAT_PRESSURE:
                if (input < 1 || input > PRESSURE_INPUT_COUNT) {
                    Serial.println("ERR,5,Input 1-7 required for pressure SPN");
                    return;
                }
                config->pressureInputs[input - 1].assignedSpn = spn;
                Serial.print("OK,SPN ");
                Serial.print(spn);
                Serial.print(" enabled on pres");
                Serial.println(input);
                break;

            case SPN_CAT_EGT:
                config->egtEnabled = true;
                Serial.println("OK,EGT enabled");
                break;

            case SPN_CAT_BME280:
                config->bme280Enabled = true;
                Serial.println("OK,BME280 enabled (SPNs 171, 108, 354)");
                break;

            default:
                break;
        }
    } else {
        // Disable SPN
        switch (category) {
            case SPN_CAT_TEMPERATURE:
                // Find which input has this SPN
                for (uint8_t i = 0; i < TEMP_INPUT_COUNT; i++) {
                    if (config->tempInputs[i].assignedSpn == spn) {
                        config->tempInputs[i].assignedSpn = 0;
                        Serial.print("OK,SPN ");
                        Serial.print(spn);
                        Serial.print(" disabled (was temp");
                        Serial.print(i + 1);
                        Serial.println(")");
                        return;
                    }
                }
                Serial.println("OK,SPN was not enabled");
                break;

            case SPN_CAT_PRESSURE:
                for (uint8_t i = 0; i < PRESSURE_INPUT_COUNT; i++) {
                    if (config->pressureInputs[i].assignedSpn == spn) {
                        config->pressureInputs[i].assignedSpn = 0;
                        Serial.print("OK,SPN ");
                        Serial.print(spn);
                        Serial.print(" disabled (was pres");
                        Serial.print(i + 1);
                        Serial.println(")");
                        return;
                    }
                }
                Serial.println("OK,SPN was not enabled");
                break;

            case SPN_CAT_EGT:
                config->egtEnabled = false;
                Serial.println("OK,EGT disabled");
                break;

            case SPN_CAT_BME280:
                config->bme280Enabled = false;
                Serial.println("OK,BME280 disabled");
                break;

            default:
                break;
        }
    }
}

void SerialCommandHandler::handleSetPressureRange() {
    // Format: 3,input,psiHi,psiLo
    // parsed.data[0] = cmd (3)
    // parsed.data[1] = input
    // parsed.data[2] = psiHi
    // parsed.data[3] = psiLo

    if (parsed.count < 4) {
        Serial.println("ERR,2,Usage: 3,input,psiHi,psiLo");
        return;
    }

    uint8_t input = parsed.data[1];
    uint16_t maxPsi = (static_cast<uint16_t>(parsed.data[2]) << 8) | parsed.data[3];

    if (input < 1 || input > PRESSURE_INPUT_COUNT) {
        Serial.println("ERR,5,Input must be 1-7");
        return;
    }

    config->pressureInputs[input - 1].maxPsi = maxPsi;
    Serial.print("OK,pres");
    Serial.print(input);
    Serial.print(" maxPsi=");
    Serial.println(maxPsi);
}

void SerialCommandHandler::handleSetTcType() {
    // Format: 4,type
    // parsed.data[0] = cmd (4)
    // parsed.data[1] = type

    if (parsed.count < 2) {
        Serial.println("ERR,2,Usage: 4,type (0-7)");
        return;
    }

    uint8_t type = parsed.data[1];
    if (type > 7) {
        Serial.println("ERR,7,Type must be 0-7");
        return;
    }

    config->thermocoupleType = static_cast<EThermocoupleType>(type);
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
    if (ConfigStorage::saveConfig(config)) {
        Serial.println("OK");
    } else {
        Serial.println("ERR,9,Save failed");
    }
}

void SerialCommandHandler::handleReset() {
    Serial.println("Resetting to defaults...");
    ConfigStorage::loadDefaults(config);
    Serial.println("OK,Use '6' to save");
}

void SerialCommandHandler::handleNtcPreset() {
    // Format: 8,input,preset
    // parsed.data[0] = cmd (8)
    // parsed.data[1] = input
    // parsed.data[2] = preset

    if (parsed.count < 3) {
        Serial.println("ERR,2,Usage: 8,input,preset (0=AEM,1=Bosch,2=GM)");
        return;
    }

    uint8_t input = parsed.data[1];
    uint8_t preset = parsed.data[2];

    if (input < 1 || input > TEMP_INPUT_COUNT) {
        Serial.println("ERR,4,Input must be 1-8");
        return;
    }

    TTempInputConfig& cfg = config->tempInputs[input - 1];

    switch (preset) {
        case 0:  // AEM
            cfg.coeffA = AEM_TEMP_COEFF_A;
            cfg.coeffB = AEM_TEMP_COEFF_B;
            cfg.coeffC = AEM_TEMP_COEFF_C;
            cfg.resistorValue = AEM_TEMP_RESISTOR;
            Serial.print("OK,temp");
            Serial.print(input);
            Serial.println(" set to AEM preset");
            break;
        case 1:  // Bosch
            cfg.coeffA = BOSCH_TEMP_COEFF_A;
            cfg.coeffB = BOSCH_TEMP_COEFF_B;
            cfg.coeffC = BOSCH_TEMP_COEFF_C;
            cfg.resistorValue = BOSCH_TEMP_RESISTOR;
            Serial.print("OK,temp");
            Serial.print(input);
            Serial.println(" set to Bosch preset");
            break;
        case 2:  // GM
            cfg.coeffA = GM_TEMP_COEFF_A;
            cfg.coeffB = GM_TEMP_COEFF_B;
            cfg.coeffC = GM_TEMP_COEFF_C;
            cfg.resistorValue = GM_TEMP_RESISTOR;
            Serial.print("OK,temp");
            Serial.print(input);
            Serial.println(" set to GM preset");
            break;
        default:
            Serial.println("ERR,10,Preset must be 0-2");
            break;
    }
}

void SerialCommandHandler::handlePressurePreset() {
    // Format: 9,input,preset
    // parsed.data[0] = cmd (9)
    // parsed.data[1] = input
    // parsed.data[2] = preset

    if (parsed.count < 3) {
        Serial.println("ERR,2,Usage: 9,input,preset (0=100,1=150,2=200)");
        return;
    }

    uint8_t input = parsed.data[1];
    uint8_t preset = parsed.data[2];

    if (input < 1 || input > PRESSURE_INPUT_COUNT) {
        Serial.println("ERR,5,Input must be 1-7");
        return;
    }

    uint16_t psiValues[] = {100, 150, 200};
    if (preset > 2) {
        Serial.println("ERR,10,Preset must be 0-2");
        return;
    }

    config->pressureInputs[input - 1].maxPsi = psiValues[preset];
    Serial.print("OK,pres");
    Serial.print(input);
    Serial.print(" set to ");
    Serial.print(psiValues[preset]);
    Serial.println(" PSI");
}
