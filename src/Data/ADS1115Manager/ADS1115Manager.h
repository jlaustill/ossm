#ifndef OSSM_ADS1115MANAGER_H
#define OSSM_ADS1115MANAGER_H

#include <Adafruit_ADS1X15.h>
#include "AppConfig.h"
#include "Data/types/TAdcReading.h"

#define ADS_CHANNEL_COUNT 4

class ADS1115Manager {
   public:
    // Initialize all ADS1115 devices based on config
    static void initialize(const AppConfig* config);

    // Call from main loop or IntervalTimer to advance conversions
    // Returns true if any new data was read
    static bool update();

    // Get latest reading for a specific device/channel (thread-safe copy)
    static TAdcReading getReading(uint8_t device, uint8_t channel);

    // Get voltage from reading
    static float getVoltage(uint8_t device, uint8_t channel);

    // Check if a specific device is initialized and enabled
    static bool isDeviceEnabled(uint8_t device);

   private:
    // ADS1115 device instances
    static Adafruit_ADS1115 ads[ADS_DEVICE_COUNT];

    // Device configuration
    static uint8_t drdyPins[ADS_DEVICE_COUNT];
    static bool deviceEnabled[ADS_DEVICE_COUNT];
    static bool deviceInitialized[ADS_DEVICE_COUNT];

    // Reading buffers (volatile for interrupt safety)
    static volatile TAdcReading readings[ADS_DEVICE_COUNT][ADS_CHANNEL_COUNT];

    // State machine for round-robin cycling
    static uint8_t currentDevice;
    static uint8_t currentChannel;
    static bool conversionStarted;
    static uint32_t conversionStartTime;

    // Timeout for conversion (should be ~8ms at 128 SPS, use 15ms for safety)
    static const uint32_t CONVERSION_TIMEOUT_MS = 15;

    // Start conversion on current device/channel
    static void startConversion();

    // Check if current conversion is complete (via DRDY or polling)
    static bool isConversionComplete();

    // Read result and store in buffer
    static void readResult();

    // Advance to next device/channel
    static void advanceChannel();

    // MUX values for single-ended channels
    static const uint16_t MUX_SINGLE[4];
};

#endif  // OSSM_ADS1115MANAGER_H
