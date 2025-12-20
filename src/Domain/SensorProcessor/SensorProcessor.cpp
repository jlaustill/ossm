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

void SensorProcessor::processAllSensors() {
    if (config == nullptr || appData == nullptr) {
        return;
    }

    for (uint8_t i = 0; i < MAX_SENSORS; i++) {
        processSensor(i);
    }
}

void SensorProcessor::processSensor(uint8_t sensorIndex) {
    if (config == nullptr || appData == nullptr || sensorIndex >= MAX_SENSORS) {
        return;
    }

    const TSensorConfig& sensorCfg = config->sensors[sensorIndex];

    // Skip disabled sensors
    if (sensorCfg.sensorType == SENSOR_DISABLED) {
        return;
    }

    float processedValue = 0.0f;

    switch (sensorCfg.sensorType) {
        // Pressure sensors (ADS1115-based)
        case PRESSURE_0_100PSI:
        case PRESSURE_0_150PSI:
        case PRESSURE_0_200PSI: {
            // Get voltage from ADS1115Manager
            if (sensorCfg.adsDevice < ADS_DEVICE_COUNT &&
                sensorCfg.adsChannel < ADS_CHANNEL_COUNT) {
                float voltage = ADS1115Manager::getVoltage(
                    sensorCfg.adsDevice, sensorCfg.adsChannel);
                processedValue = convertPressure(voltage, sensorCfg);
            }
            break;
        }

        // NTC Temperature sensors (ADS1115-based)
        case TEMP_NTC_AEM:
        case TEMP_NTC_BOSCH:
        case TEMP_NTC_GM: {
            if (sensorCfg.adsDevice < ADS_DEVICE_COUNT &&
                sensorCfg.adsChannel < ADS_CHANNEL_COUNT) {
                float voltage = ADS1115Manager::getVoltage(
                    sensorCfg.adsDevice, sensorCfg.adsChannel);
                processedValue = convertNtcTemperature(voltage, sensorCfg);
            }
            break;
        }

        // Thermocouple (MAX31856)
        case TEMP_THERMOCOUPLE_K: {
            if (MAX31856Manager::isEnabled()) {
                processedValue = MAX31856Manager::getTemperatureC();
                // Skip update if invalid reading (returns -273.15)
                if (processedValue < -270.0f) {
                    return;
                }
            } else {
                return;
            }
            break;
        }

        // BME280 ambient sensors
        case AMBIENT_TEMP: {
            if (BME280Manager::isEnabled()) {
                processedValue = BME280Manager::getTemperatureC();
                if (processedValue < -270.0f) {
                    return;  // Invalid reading
                }
            } else {
                return;
            }
            break;
        }

        case AMBIENT_HUMIDITY: {
            if (BME280Manager::isEnabled()) {
                processedValue = BME280Manager::getHumidity();
                if (processedValue <= 0.0f) {
                    return;  // Invalid reading
                }
            } else {
                return;
            }
            break;
        }

        case AMBIENT_PRESSURE: {
            if (BME280Manager::isEnabled()) {
                processedValue = BME280Manager::getPressurekPa();
                if (processedValue <= 0.0f) {
                    return;  // Invalid reading
                }
            } else {
                return;
            }
            break;
        }

        default:
            return;
    }

    // Update the appropriate AppData field
    updateAppDataField(sensorCfg.appDataIndex, processedValue);
}

float SensorProcessor::convertPressure(float voltage, const TSensorConfig& cfg) {
    // cfg.coeffA = minimum voltage (typically 0.5V)
    // cfg.coeffB = maximum voltage (typically 4.5V)
    // cfg.coeffC = maximum PSI

    float zeroVoltage = cfg.coeffA;
    float maxVoltage = cfg.coeffB;
    float maxPsi = cfg.coeffC;

    // Map voltage to PSI
    float psi = mapFloat(voltage, zeroVoltage, maxVoltage, 0.0f, maxPsi);

    // Clamp to positive values
    psi = clampPositive(psi);

    // Convert PSI to kPa
    float kPa = psi * PSI_TO_KPA;

    return kPa;
}

float SensorProcessor::convertNtcTemperature(float voltage, const TSensorConfig& cfg) {
    // Steinhart-Hart equation coefficients
    // cfg.coeffA = A coefficient
    // cfg.coeffB = B coefficient
    // cfg.coeffC = C coefficient
    // cfg.resistorValue = known resistor value in voltage divider
    // cfg.wiringResistance = wiring resistance compensation

    float A = cfg.coeffA;
    float B = cfg.coeffB;
    float C = cfg.coeffC;
    float knownResistor = cfg.resistorValue;
    float wiringResistance = cfg.wiringResistance;

    // Prevent division by zero
    if (voltage >= VREF || voltage <= 0.0f) {
        return -273.15f;  // Return absolute zero as error indicator
    }

    // Calculate thermistor resistance using voltage divider formula
    // Vout = Vin * R_thermistor / (R_known + R_thermistor)
    // Solving for R_thermistor: R = (V * R_known) / (Vref - V)
    float resistance = (voltage * knownResistor) / (VREF - voltage);
    resistance -= wiringResistance;

    // Prevent invalid resistance values
    if (resistance <= 0.0f) {
        return -273.15f;
    }

    // Steinhart-Hart equation: 1/T = A + B*ln(R) + C*ln(R)^3
    float lnR = log(resistance);
    float lnR3 = lnR * lnR * lnR;
    float invKelvin = A + (B * lnR) + (C * lnR3);

    // Prevent division by zero
    if (invKelvin <= 0.0f) {
        return -273.15f;
    }

    float kelvin = 1.0f / invKelvin;

    // Convert to Celsius
    float celsius = kelvin - 273.15f;

    return celsius;
}

void SensorProcessor::updateAppDataField(uint8_t index, float value) {
    if (appData == nullptr) {
        return;
    }

    switch (index) {
        case IDX_HUMIDITY:
            appData->humidity = value;
            break;
        case IDX_AMBIENT_TEMP:
            appData->ambientTemperatureC = value;
            break;
        case IDX_BAROMETRIC_PRESSURE:
            appData->absoluteBarometricpressurekPa = value;
            break;
        case IDX_OIL_TEMP:
            appData->oilTemperatureC = value;
            break;
        case IDX_OIL_PRESSURE:
            appData->oilPressurekPa = value;
            break;
        case IDX_COOLANT_TEMP:
            appData->coolantTemperatureC = value;
            break;
        case IDX_COOLANT_PRESSURE:
            appData->coolantPressurekPa = value;
            break;
        case IDX_TRANSFER_PIPE_TEMP:
            appData->transferPipeTemperatureC = value;
            break;
        case IDX_TRANSFER_PIPE_PRESSURE:
            appData->tranferPipePressurekPa = value;
            break;
        case IDX_BOOST_PRESSURE:
            appData->boostPressurekPa = value;
            break;
        case IDX_BOOST_TEMP:
            appData->boostTemperatureC = value;
            break;
        case IDX_CAC_INLET_PRESSURE:
            appData->cacInletPressurekPa = value;
            break;
        case IDX_CAC_INLET_TEMP:
            appData->cacInletTemperatureC = value;
            break;
        case IDX_AIR_INLET_PRESSURE:
            appData->airInletPressurekPa = value;
            break;
        case IDX_AIR_INLET_TEMP:
            appData->airInletTemperatureC = value;
            break;
        case IDX_FUEL_PRESSURE:
            appData->fuelPressurekPa = value;
            break;
        case IDX_FUEL_TEMP:
            appData->fuelTemperatureC = value;
            break;
        case IDX_ENGINE_BAY_TEMP:
            appData->engineBayTemperatureC = value;
            break;
        case IDX_EGT_TEMP:
            appData->egtTemperatureC = static_cast<double>(value);
            break;
        default:
            break;
    }
}

float SensorProcessor::mapFloat(float x, float inMin, float inMax, float outMin, float outMax) {
    if (inMax == inMin) {
        return outMin;  // Prevent division by zero
    }
    return (x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

float SensorProcessor::clampPositive(float x) {
    return (x < 0.0f) ? 0.0f : x;
}
