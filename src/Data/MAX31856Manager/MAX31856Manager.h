#ifndef OSSM_MAX31856MANAGER_H
#define OSSM_MAX31856MANAGER_H

#include <Adafruit_MAX31856.h>
#include "AppConfig.h"

class MAX31856Manager {
   public:
    // Initialize the MAX31856 based on config
    static void initialize(const AppConfig* config);

    // Call from main loop to advance conversion state machine
    // Returns true if a new reading is available
    static bool update();

    // Get the latest temperature reading in Celsius
    static float getTemperatureC();

    // Get the cold junction (ambient) temperature in Celsius
    static float getColdJunctionC();

    // Check if the device is enabled and initialized
    static bool isEnabled();

    // Check for faults (returns fault code, 0 = no fault)
    static uint8_t getFaultStatus();

   private:
    // MAX31856 instance
    static Adafruit_MAX31856* thermocouple;

    // Configuration
    static uint8_t csPin;
    static uint8_t drdyPin;
    static uint8_t faultPin;
    static bool enabled;
    static bool initialized;

    // State machine for non-blocking reads
    static bool conversionStarted;
    static uint32_t conversionStartTime;

    // Latest readings
    static float temperatureC;
    static float coldJunctionC;
    static uint8_t faultCode;
    static bool readingValid;

    // Timeout for conversion (typ 155ms for 60Hz rejection, use 200ms)
    static const uint32_t CONVERSION_TIMEOUT_MS = 200;

    // Start a new conversion
    static void startConversion();

    // Check if conversion is complete (via DRDY pin)
    static bool isConversionComplete();

    // Read result and store
    static void readResult();
};

#endif  // OSSM_MAX31856MANAGER_H
