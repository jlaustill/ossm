#include "CommandHandler.h"
#include "Data/ConfigStorage/ConfigStorage.h"
#include "Data/ADS1115Manager/ADS1115Manager.h"
#include "Data/MAX31856Manager/MAX31856Manager.h"
#include "Data/BME280Manager/BME280Manager.h"

// Static member initialization
AppConfig* CommandHandler::config = nullptr;
AppData* CommandHandler::appData = nullptr;

void CommandHandler::initialize(AppConfig* cfg, AppData* data) {
    config = cfg;
    appData = data;
}

TCommandResult CommandHandler::enableSpn(uint16_t spn, bool enable, uint8_t input) {
    // Find SPN category
    ESpnCategory category = SPN_CAT_UNKNOWN;
    for (size_t i = 0; i < KNOWN_SPN_COUNT; i++) {
        if (KNOWN_SPNS[i].spn == spn) {
            category = KNOWN_SPNS[i].category;
            break;
        }
    }

    if (category == SPN_CAT_UNKNOWN) {
        return TCommandResult::error(ECommandError::UNKNOWN_SPN);
    }

    if (enable) {
        // Enable SPN
        switch (category) {
            case SPN_CAT_TEMPERATURE:
                if (input < 1 || input > TEMP_INPUT_COUNT) {
                    return TCommandResult::error(ECommandError::INVALID_TEMP_INPUT);
                }
                // Clear any existing assignment of this SPN (move, not duplicate)
                for (uint8_t i = 0; i < TEMP_INPUT_COUNT; i++) {
                    if (config->tempInputs[i].assignedSpn == spn) {
                        config->tempInputs[i].assignedSpn = 0;
                    }
                }
                config->tempInputs[input - 1].assignedSpn = spn;
                // Initialize hardware immediately so it works without reboot
                ADS1115Manager::initialize(config);
                break;

            case SPN_CAT_PRESSURE:
                if (input < 1 || input > PRESSURE_INPUT_COUNT) {
                    return TCommandResult::error(ECommandError::INVALID_PRESSURE_INPUT);
                }
                // Clear any existing assignment of this SPN (move, not duplicate)
                for (uint8_t i = 0; i < PRESSURE_INPUT_COUNT; i++) {
                    if (config->pressureInputs[i].assignedSpn == spn) {
                        config->pressureInputs[i].assignedSpn = 0;
                    }
                }
                config->pressureInputs[input - 1].assignedSpn = spn;
                // Initialize hardware immediately so it works without reboot
                ADS1115Manager::initialize(config);
                break;

            case SPN_CAT_EGT:
                config->egtEnabled = true;
                // Initialize hardware immediately so it works without reboot
                MAX31856Manager::initialize(config);
                break;

            case SPN_CAT_BME280:
                config->bme280Enabled = true;
                // Initialize hardware immediately so it works without reboot
                BME280Manager::initialize(config);
                break;

            default:
                break;
        }
    } else {
        // Disable SPN
        switch (category) {
            case SPN_CAT_TEMPERATURE:
                for (uint8_t i = 0; i < TEMP_INPUT_COUNT; i++) {
                    if (config->tempInputs[i].assignedSpn == spn) {
                        config->tempInputs[i].assignedSpn = 0;
                        break;
                    }
                }
                break;

            case SPN_CAT_PRESSURE:
                for (uint8_t i = 0; i < PRESSURE_INPUT_COUNT; i++) {
                    if (config->pressureInputs[i].assignedSpn == spn) {
                        config->pressureInputs[i].assignedSpn = 0;
                        break;
                    }
                }
                break;

            case SPN_CAT_EGT:
                config->egtEnabled = false;
                break;

            case SPN_CAT_BME280:
                config->bme280Enabled = false;
                break;

            default:
                break;
        }
    }

    return TCommandResult::ok();
}

TCommandResult CommandHandler::setPressureRange(uint8_t input, uint16_t maxPressure) {
    if (input < 1 || input > PRESSURE_INPUT_COUNT) {
        return TCommandResult::error(ECommandError::INVALID_PRESSURE_INPUT);
    }

    config->pressureInputs[input - 1].maxPressure = maxPressure;
    return TCommandResult::ok();
}

TCommandResult CommandHandler::setTcType(uint8_t type) {
    if (type > 7) {
        return TCommandResult::error(ECommandError::INVALID_TC_TYPE);
    }

    config->thermocoupleType = static_cast<EThermocoupleType>(type);
    return TCommandResult::ok();
}

TCommandResult CommandHandler::applyNtcPreset(uint8_t input, uint8_t preset) {
    if (input < 1 || input > TEMP_INPUT_COUNT) {
        return TCommandResult::error(ECommandError::INVALID_TEMP_INPUT);
    }

    TTempInputConfig& cfg = config->tempInputs[input - 1];

    switch (preset) {
        case 0:  // AEM
            cfg.coeffA = AEM_TEMP_COEFF_A;
            cfg.coeffB = AEM_TEMP_COEFF_B;
            cfg.coeffC = AEM_TEMP_COEFF_C;
            cfg.resistorValue = AEM_TEMP_RESISTOR;
            break;
        case 1:  // Bosch
            cfg.coeffA = BOSCH_TEMP_COEFF_A;
            cfg.coeffB = BOSCH_TEMP_COEFF_B;
            cfg.coeffC = BOSCH_TEMP_COEFF_C;
            cfg.resistorValue = BOSCH_TEMP_RESISTOR;
            break;
        case 2:  // GM
            cfg.coeffA = GM_TEMP_COEFF_A;
            cfg.coeffB = GM_TEMP_COEFF_B;
            cfg.coeffC = GM_TEMP_COEFF_C;
            cfg.resistorValue = GM_TEMP_RESISTOR;
            break;
        default:
            return TCommandResult::error(ECommandError::INVALID_PRESET);
    }

    return TCommandResult::ok();
}

TCommandResult CommandHandler::applyPressurePreset(uint8_t input, uint8_t preset) {
    if (input < 1 || input > PRESSURE_INPUT_COUNT) {
        return TCommandResult::error(ECommandError::INVALID_PRESSURE_INPUT);
    }

    TPressureInputConfig& cfg = config->pressureInputs[input - 1];

    // Bar presets (0-15): PSIA absolute, value in centibar (1 bar = 100)
    // PSI presets (20-30): PSIG gauge, value in PSI
    if (preset <= 15) {
        // Bar presets (PSIA) - stored in centibar for 0.5 bar precision
        // Note: presets 13-15 are large values stored in bar (flagged by value > 20000)
        static const uint16_t centibarValues[] = {
            100,   // 0: 1 bar
            150,   // 1: 1.5 bar
            200,   // 2: 2 bar
            250,   // 3: 2.5 bar
            300,   // 4: 3 bar
            400,   // 5: 4 bar
            500,   // 6: 5 bar
            700,   // 7: 7 bar
            1000,  // 8: 10 bar
            5000,  // 9: 50 bar
            10000, // 10: 100 bar
            15000, // 11: 150 bar
            20000, // 12: 200 bar
            21000, // 13: 1000 bar (stored as 21000 = flag + bar value)
            22000, // 14: 2000 bar (stored as 22000 = flag + bar value)
            23000  // 15: 3000 bar (stored as 23000 = flag + bar value)
        };
        cfg.maxPressure = centibarValues[preset];
        cfg.pressureType = PRESSURE_TYPE_PSIA;
    } else if (preset >= 20 && preset <= 30) {
        // PSI presets (PSIG)
        static const uint16_t psiValues[] = {
            15,   // 20: 15 PSIG
            30,   // 21: 30 PSIG
            50,   // 22: 50 PSIG
            100,  // 23: 100 PSIG
            150,  // 24: 150 PSIG
            200,  // 25: 200 PSIG
            250,  // 26: 250 PSIG
            300,  // 27: 300 PSIG
            350,  // 28: 350 PSIG
            400,  // 29: 400 PSIG
            500   // 30: 500 PSIG
        };
        cfg.maxPressure = psiValues[preset - 20];
        cfg.pressureType = PRESSURE_TYPE_PSIG;
    } else {
        return TCommandResult::error(ECommandError::INVALID_PRESET);
    }

    return TCommandResult::ok();
}

TCommandResult CommandHandler::setNtcParam(uint8_t input, uint8_t param, float value) {
    if (input < 1 || input > TEMP_INPUT_COUNT) {
        return TCommandResult::error(ECommandError::INVALID_TEMP_INPUT);
    }

    TTempInputConfig& cfg = config->tempInputs[input - 1];

    switch (param) {
        case 0:
            cfg.coeffA = value;
            break;
        case 1:
            cfg.coeffB = value;
            break;
        case 2:
            cfg.coeffC = value;
            break;
        case 3:
            cfg.resistorValue = value;
            break;
        default:
            return TCommandResult::error(ECommandError::INVALID_NTC_PARAM);
    }

    return TCommandResult::ok();
}

TCommandResult CommandHandler::save() {
    if (ConfigStorage::saveConfig(config)) {
        return TCommandResult::ok();
    }
    return TCommandResult::error(ECommandError::SAVE_FAILED);
}

TCommandResult CommandHandler::reset() {
    ConfigStorage::loadDefaults(config);
    return TCommandResult::ok();
}

TCommandResult CommandHandler::querySpnCounts() {
    uint8_t tempCount = 0;
    uint8_t presCount = 0;

    for (uint8_t i = 0; i < TEMP_INPUT_COUNT; i++) {
        if (config->tempInputs[i].assignedSpn != 0) tempCount++;
    }
    for (uint8_t i = 0; i < PRESSURE_INPUT_COUNT; i++) {
        if (config->pressureInputs[i].assignedSpn != 0) presCount++;
    }

    uint8_t responseData[4];
    responseData[0] = tempCount;
    responseData[1] = presCount;
    responseData[2] = config->egtEnabled ? 1 : 0;
    responseData[3] = config->bme280Enabled ? 1 : 0;

    return TCommandResult::withData(responseData, 4);
}

TCommandResult CommandHandler::queryFullConfig() {
    uint8_t responseData[2];
    responseData[0] = config->j1939SourceAddress;
    responseData[1] = static_cast<uint8_t>(config->thermocoupleType);

    return TCommandResult::withData(responseData, 2);
}
