#ifndef OSSM_SENSORPROCESSOR_H
#define OSSM_SENSORPROCESSOR_H

#include "AppConfig.h"
#include "AppData.h"
#include "Data/types/TAdcReading.h"

class SensorProcessor {
   public:
    // Initialize with config and data pointers
    static void initialize(const AppConfig* config, AppData* appData);

    // Process all enabled sensors and update AppData
    static void processAllSensors();

    // Process a single sensor by index
    static void processSensor(uint8_t sensorIndex);

   private:
    static const AppConfig* config;
    static AppData* appData;

    // Conversion functions for different sensor types
    static float convertPressure(float voltage, const TSensorConfig& cfg);
    static float convertNtcTemperature(float voltage, const TSensorConfig& cfg);

    // Update specific AppData field by index
    static void updateAppDataField(uint8_t index, float value);

    // Helper: map float value from one range to another
    static float mapFloat(float x, float inMin, float inMax, float outMin, float outMax);

    // Helper: clamp value to non-negative
    static float clampPositive(float x);

    // PSI to kPa conversion factor
    static constexpr float PSI_TO_KPA = 6.894757f;

    // Reference voltage for voltage divider (5V supply)
    static constexpr float VREF = 5.0f;
};

#endif  // OSSM_SENSORPROCESSOR_H
