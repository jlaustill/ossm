#ifndef OSSM_SENSORPROCESSOR_H
#define OSSM_SENSORPROCESSOR_H

#include "AppConfig.h"
#include "AppData.h"

class SensorProcessor {
   public:
    // Initialize with config and data pointers
    static void initialize(const AppConfig* config, AppData* appData);

    // Process all enabled inputs and update AppData
    static void processAllInputs();

   private:
    static const AppConfig* config;
    static AppData* appData;

    // Process temperature inputs
    static void processTempInputs();

    // Process pressure inputs
    static void processPressureInputs();

    // Process EGT from MAX31856
    static void processEgt();

    // Process BME280 ambient sensors
    static void processBme280();

    // Conversion functions
    static float convertPressure(float voltage, uint16_t maxPsi);
    static float convertNtcTemperature(float voltage, const TTempInputConfig& cfg);

    // Update AppData based on SPN
    static void updateAppDataForSpn(uint16_t spn, float value);

    // Helper: clamp value to non-negative
    static float clampPositive(float x);

    // PSI to kPa conversion factor
    static constexpr float PSI_TO_KPA = 6.894757f;

    // Reference voltage for voltage divider (5V supply)
    static constexpr float VREF = 5.0f;

    // Pressure sensor voltage range (0.5V-4.5V typical)
    static constexpr float PRESSURE_VOLTAGE_MIN = 0.5f;
    static constexpr float PRESSURE_VOLTAGE_MAX = 4.5f;
};

#endif  // OSSM_SENSORPROCESSOR_H
