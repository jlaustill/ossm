#ifndef OSSM_TADCREADING_H
#define OSSM_TADCREADING_H

#include <stdint.h>

// Raw ADC reading with metadata
struct TAdcReading {
    int16_t rawValue;      // Raw ADC value from ADS1115 (-32768 to 32767)
    uint32_t timestamp;    // millis() when reading was taken
    bool valid;            // Whether reading is valid/initialized
};

#endif  // OSSM_TADCREADING_H
