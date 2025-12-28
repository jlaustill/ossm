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
                config->tempInputs[input - 1].assignedSpn = spn;
                // Initialize hardware immediately so it works without reboot
                ADS1115Manager::initialize(config);
                break;

            case SPN_CAT_PRESSURE:
                if (input < 1 || input > PRESSURE_INPUT_COUNT) {
                    return TCommandResult::error(ECommandError::INVALID_PRESSURE_INPUT);
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

TCommandResult CommandHandler::setPressureRange(uint8_t input, uint16_t maxPsi) {
    if (input < 1 || input > PRESSURE_INPUT_COUNT) {
        return TCommandResult::error(ECommandError::INVALID_PRESSURE_INPUT);
    }

    config->pressureInputs[input - 1].maxPsi = maxPsi;
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

    const uint16_t psiValues[] = {100, 150, 200};
    if (preset > 2) {
        return TCommandResult::error(ECommandError::INVALID_PRESET);
    }

    config->pressureInputs[input - 1].maxPsi = psiValues[preset];
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
