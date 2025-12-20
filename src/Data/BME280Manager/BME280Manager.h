#ifndef OSSM_BME280MANAGER_H
#define OSSM_BME280MANAGER_H

#include <Adafruit_BME280.h>
#include "AppConfig.h"

class BME280Manager {
   public:
    // Initialize the BME280 based on config
    static void initialize(const AppConfig* config);

    // Call from main loop to update readings
    // BME280 reads are fast, so this is not strictly async
    // Returns true if readings were updated
    static bool update();

    // Get the latest temperature reading in Celsius
    static float getTemperatureC();

    // Get the latest humidity reading in %
    static float getHumidity();

    // Get the latest pressure reading in kPa
    static float getPressurekPa();

    // Check if the device is enabled and initialized
    static bool isEnabled();

   private:
    // BME280 instance
    static Adafruit_BME280 bme;

    // Configuration
    static uint8_t i2cAddress;
    static bool enabled;
    static bool initialized;

    // Latest readings
    static float temperatureC;
    static float humidity;
    static float pressurekPa;
    static bool readingValid;

    // Timing for periodic reads (BME280 doesn't need as frequent updates)
    static uint32_t lastReadTime;
    static const uint32_t READ_INTERVAL_MS = 1000;  // Read every 1 second
};

#endif  // OSSM_BME280MANAGER_H
