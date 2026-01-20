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

    // Get atmospheric pressure for PSIG conversion (from BME280 or default)
    static float getAtmosphericPressurekPa();

    // Update AppData based on SPN
    static void updateAppDataForSpn(uint16_t spn, float value);
};

#endif  // OSSM_SENSORPROCESSOR_H
