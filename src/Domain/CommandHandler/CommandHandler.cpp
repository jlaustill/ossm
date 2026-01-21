#include "CommandHandler.h"
#include "Data/ConfigStorage/ConfigStorage.h"
#include "Data/ADS1115Manager/ADS1115Manager.h"
#include "Data/MAX31856Manager/MAX31856Manager.h"
#include "Data/BME280Manager/BME280Manager.h"
#include "presets.h"

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

    if (!presets_isValidNtcPreset(preset)) {
        return TCommandResult::error(ECommandError::INVALID_PRESET);
    }

    TTempInputConfig& cfg = config->tempInputs[input - 1];
    cfg.coeffA = presets_ntcCoeffA(preset);
    cfg.coeffB = presets_ntcCoeffB(preset);
    cfg.coeffC = presets_ntcCoeffC(preset);
    cfg.resistorValue = presets_ntcResistor(preset);

    return TCommandResult::ok();
}

TCommandResult CommandHandler::applyPressurePreset(uint8_t input, uint8_t preset) {
    if (input < 1 || input > PRESSURE_INPUT_COUNT) {
        return TCommandResult::error(ECommandError::INVALID_PRESSURE_INPUT);
    }

    if (!presets_isValidPressurePreset(preset)) {
        return TCommandResult::error(ECommandError::INVALID_PRESET);
    }

    TPressureInputConfig& cfg = config->pressureInputs[input - 1];

    if (presets_isBarPreset(preset)) {
        // Bar presets (PSIA) - value is centibar
        cfg.maxPressure = presets_barPresetValue(preset);
        cfg.pressureType = PRESSURE_TYPE_PSIA;
    } else {
        // PSI presets (PSIG)
        cfg.maxPressure = presets_psiPresetValue(preset);
        cfg.pressureType = PRESSURE_TYPE_PSIG;
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

TCommandResult CommandHandler::queryTempSpns(uint8_t subQuery) {
    // Return 3 temp SPN assignments per sub-query (6 bytes = 3 x 16-bit SPNs)
    // subQuery 0: temp1-3, subQuery 1: temp4-6, subQuery 2: temp7-8
    uint8_t responseData[6];
    uint8_t startIdx = subQuery * 3;

    for (uint8_t i = 0; i < 3; i++) {
        uint8_t idx = startIdx + i;
        if (idx < TEMP_INPUT_COUNT) {
            uint16_t spn = config->tempInputs[idx].assignedSpn;
            responseData[i * 2] = (spn >> 8) & 0xFF;      // high byte
            responseData[i * 2 + 1] = spn & 0xFF;          // low byte
        } else {
            responseData[i * 2] = 0xFF;
            responseData[i * 2 + 1] = 0xFF;
        }
    }

    return TCommandResult::withData(responseData, 6);
}

TCommandResult CommandHandler::queryPresSpns(uint8_t subQuery) {
    // Return 3 pres SPN assignments per sub-query (6 bytes = 3 x 16-bit SPNs)
    // subQuery 0: pres1-3, subQuery 1: pres4-6, subQuery 2: pres7
    uint8_t responseData[6];
    uint8_t startIdx = subQuery * 3;

    for (uint8_t i = 0; i < 3; i++) {
        uint8_t idx = startIdx + i;
        if (idx < PRESSURE_INPUT_COUNT) {
            uint16_t spn = config->pressureInputs[idx].assignedSpn;
            responseData[i * 2] = (spn >> 8) & 0xFF;      // high byte
            responseData[i * 2 + 1] = spn & 0xFF;          // low byte
        } else {
            responseData[i * 2] = 0xFF;
            responseData[i * 2 + 1] = 0xFF;
        }
    }

    return TCommandResult::withData(responseData, 6);
}
