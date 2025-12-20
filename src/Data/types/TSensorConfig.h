#ifndef OSSM_TSENSORCONFIG_H
#define OSSM_TSENSORCONFIG_H

#include <stdint.h>
#include "ESensorType.h"

// Configuration entry for a single sensor
struct TSensorConfig {
    ESensorType sensorType;   // What type of sensor (determines conversion)
    uint8_t adsDevice;        // Which ADS1115 (0-3), 0xFF = not ADS-based
    uint8_t adsChannel;       // Which channel (0-3)
    uint8_t appDataIndex;     // Index into AppData fields (0-18)

    // Calibration coefficients (meaning depends on sensorType)
    // For NTC: Steinhart-Hart A, B, C coefficients
    // For pressure: A=zero offset voltage, B=span voltage, C=max PSI
    float coeffA;
    float coeffB;
    float coeffC;

    // NTC-specific values
    float resistorValue;      // Known resistor value in voltage divider
    float wiringResistance;   // Wiring resistance compensation
};

#endif  // OSSM_TSENSORCONFIG_H
