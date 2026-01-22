#include "SensorProcessor.h"
#include <Arduino.h>
#include "Data/ADS1115Manager/ADS1115Manager.h"
#include "Data/MAX31856Manager/MAX31856Manager.h"
#include "Data/BME280Manager/BME280Manager.h"
#include "Display/SensorConvert.h"
#include "Display/HardwareMap.h"

// Static member initialization
const AppConfig* SensorProcessor::config = nullptr;
AppData* SensorProcessor::appData = nullptr;

void SensorProcessor::initialize(const AppConfig* cfg, AppData* data) {
    config = cfg;
    appData = data;
}

void SensorProcessor::processAllInputs() {
    if (config == nullptr || appData == nullptr) {
        return;
    }

    processTempInputs();
    processPressureInputs();
    processEgt();
    processBme280();
}

void SensorProcessor::processTempInputs() {
    for (uint8_t i = 0; i < TEMP_INPUT_COUNT; i++) {
        const TTempInputConfig& inputCfg = config->tempInputs[i];

        // Skip disabled inputs
        if (inputCfg.assignedSpn == 0) {
            continue;
        }

        // Get hardware mapping using C-Next module
        uint8_t device = HardwareMap_tempDevice(i);
        uint8_t channel = HardwareMap_tempChannel(i);

        // Get voltage from ADS1115
        float voltage = ADS1115Manager::getVoltage(device, channel);

        // Convert to temperature
        float tempC = SensorConvert_ntcTemperature(voltage, &inputCfg);

        // Update AppData based on assigned SPN
        updateAppDataForSpn(inputCfg.assignedSpn, tempC);
    }
}

void SensorProcessor::processPressureInputs() {
    for (uint8_t i = 0; i < PRESSURE_INPUT_COUNT; i++) {
        const TPressureInputConfig& inputCfg = config->pressureInputs[i];

        // Skip disabled inputs
        if (inputCfg.assignedSpn == 0) {
            continue;
        }

        // Get hardware mapping using C-Next module
        uint8_t device = HardwareMap_pressureDevice(i);
        uint8_t channel = HardwareMap_pressureChannel(i);

        // Get voltage from ADS1115
        float voltage = ADS1115Manager::getVoltage(device, channel);

        // Convert to pressure in kPa
        float pressurekPa = SensorConvert_pressure(voltage, &inputCfg, getAtmosphericPressurekPa());

        // Update AppData based on assigned SPN
        updateAppDataForSpn(inputCfg.assignedSpn, pressurekPa);
    }
}

float SensorProcessor::getAtmosphericPressurekPa() {
    // Use BME280 reading if available, otherwise standard atmosphere
    if (appData != nullptr && appData->absoluteBarometricpressurekPa > 0.0f) {
        return appData->absoluteBarometricpressurekPa;
    }
    return SensorConvert_defaultAtmosphericPressure();
}

void SensorProcessor::updateAppDataForSpn(uint16_t spn, float value) {
    // Map SPN to AppData field
    switch (spn) {
        // Temperature SPNs
        case 175:  // Engine Oil Temperature
            appData->oilTemperatureC = value;
            break;
        case 110:  // Engine Coolant Temperature
        case 1637: // Engine Coolant Temperature (High Resolution)
            appData->coolantTemperatureC = value;
            break;
        case 174:  // Fuel Temperature
            appData->fuelTemperatureC = value;
            break;
        case 105:  // Intake Manifold 1 Temperature (Boost Temp)
        case 1363: // Intake Manifold 1 Air Temperature (High Resolution)
            appData->boostTemperatureC = value;
            break;
        case 1131: // Intake Manifold 2 Temperature (CAC Inlet)
            appData->cacInletTemperatureC = value;
            break;
        case 1132: // Intake Manifold 3 Temperature (Transfer Pipe)
            appData->transferPipeTemperatureC = value;
            break;
        case 1133: // Intake Manifold 4 Temperature (Air Inlet)
            appData->airInletTemperatureC = value;
            break;
        case 172:  // Air Inlet Temperature
            appData->airInletTemperatureC = value;
            break;
        case 441:  // Engine Bay Temperature
            appData->engineBayTemperatureC = value;
            break;

        // Pressure SPNs (value is already in kPa)
        case 100:  // Engine Oil Pressure
            appData->oilPressurekPa = value;
            break;
        case 109:  // Coolant Pressure
            appData->coolantPressurekPa = value;
            break;
        case 94:   // Fuel Delivery Pressure
            appData->fuelPressurekPa = value;
            break;
        case 102:  // Boost Pressure
            appData->boostPressurekPa = value;
            break;
        case 106:  // Air Inlet Pressure
            appData->airInletPressurekPa = value;
            break;
        case 1127: // Turbocharger 1 Boost Pressure (CAC Inlet)
            appData->cacInletPressurekPa = value;
            break;
        case 1128: // Turbocharger 2 Boost Pressure (Transfer Pipe)
            appData->transferPipePressurekPa = value;
            break;

        default:
            // Unknown SPN, ignore
            break;
    }
}

void SensorProcessor::processEgt() {
    if (config->egtEnabled && MAX31856Manager::isEnabled()) {
        appData->egtTemperatureC = MAX31856Manager::getTemperatureC();
    }
}

void SensorProcessor::processBme280() {
    if (config->bme280Enabled && BME280Manager::isEnabled()) {
        appData->ambientTemperatureC = BME280Manager::getTemperatureC();
        appData->humidity = BME280Manager::getHumidity();
        appData->absoluteBarometricpressurekPa = BME280Manager::getPressurekPa();
    }
}
