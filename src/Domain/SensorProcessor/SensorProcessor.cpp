#include "SensorProcessor.h"
#include <Arduino.h>
#include <math.h>
#include "Data/ADS1115Manager/ADS1115Manager.h"
#include "Data/MAX31856Manager/MAX31856Manager.h"
#include "Data/BME280Manager/BME280Manager.h"

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

        // Get hardware mapping for this input
        uint8_t device = TEMP_HARDWARE_MAP[i].adsDevice;
        uint8_t channel = TEMP_HARDWARE_MAP[i].adsChannel;

        // Get voltage from ADS1115
        float voltage = ADS1115Manager::getVoltage(device, channel);

        // Convert to temperature
        float tempC = convertNtcTemperature(voltage, inputCfg);

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

        // Get hardware mapping for this input
        uint8_t device = PRESSURE_HARDWARE_MAP[i].adsDevice;
        uint8_t channel = PRESSURE_HARDWARE_MAP[i].adsChannel;

        // Get voltage from ADS1115
        float voltage = ADS1115Manager::getVoltage(device, channel);

        // Convert to pressure in kPa
        float pressurekPa = convertPressure(voltage, inputCfg);

        // Update AppData based on assigned SPN
        updateAppDataForSpn(inputCfg.assignedSpn, pressurekPa);
    }
}

float SensorProcessor::convertPressure(float voltage, const TPressureInputConfig& cfg) {
    // Voltage range: 0.5V = 0, 4.5V = max
    if (voltage < PRESSURE_VOLTAGE_MIN) {
        return 0.0f;
    }
    if (voltage > PRESSURE_VOLTAGE_MAX) {
        voltage = PRESSURE_VOLTAGE_MAX;
    }

    // Calculate voltage ratio (0.0 to 1.0)
    float ratio = (voltage - PRESSURE_VOLTAGE_MIN) /
                  (PRESSURE_VOLTAGE_MAX - PRESSURE_VOLTAGE_MIN);

    // Convert to kPa based on pressure type
    if (cfg.pressureType == PRESSURE_TYPE_PSIA) {
        // Bar sensor (absolute)
        // maxPressure storage: <= 20000 = centibar, > 20000 = 20000 + bar
        if (cfg.maxPressure > 20000) {
            // Large bar values: stored as 20000 + bar_value
            float maxBar = static_cast<float>(cfg.maxPressure - 20000);
            float bar = ratio * maxBar;
            return clampPositive(bar * BAR_TO_KPA);
        } else {
            // Normal values stored in centibar (0.01 bar per unit)
            // 1 centibar = 1 kPa, so maxPressure in centibar = max kPa
            float maxkPa = static_cast<float>(cfg.maxPressure);
            return clampPositive(ratio * maxkPa);
        }
    } else {
        // PSI sensor (gauge): maxPressure is in PSI
        float maxPsi = static_cast<float>(cfg.maxPressure);
        float psi = ratio * maxPsi;
        float gaugekPa = psi * PSI_TO_KPA;
        float atmkPa = getAtmosphericPressurekPa();
        return clampPositive(gaugekPa + atmkPa);
    }
}

float SensorProcessor::getAtmosphericPressurekPa() {
    // Use BME280 reading if available, otherwise standard atmosphere
    if (appData != nullptr && appData->absoluteBarometricpressurekPa > 0.0f) {
        return appData->absoluteBarometricpressurekPa;
    }
    return STANDARD_ATMOSPHERE_KPA;
}

float SensorProcessor::convertNtcTemperature(float voltage, const TTempInputConfig& cfg) {
    // NTC thermistor in voltage divider with known resistor to VREF
    // V = VREF * R_ntc / (R_known + R_ntc)
    // R_ntc = R_known * V / (VREF - V)

    if (voltage <= 0.0f || voltage >= VREF) {
        return -273.15f;  // Error value
    }

    float r_ntc = cfg.resistorValue * voltage / (VREF - voltage);

    // Steinhart-Hart equation: 1/T = A + B*ln(R) + C*(ln(R))^3
    float lnR = log(r_ntc);
    float lnR3 = lnR * lnR * lnR;

    float invT = cfg.coeffA + cfg.coeffB * lnR + cfg.coeffC * lnR3;

    // Convert from Kelvin to Celsius
    float tempK = 1.0f / invT;
    return tempK - 273.15f;
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

float SensorProcessor::clampPositive(float x) {
    return x < 0.0f ? 0.0f : x;
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
