#ifndef OSSM_ESENSORTYPE_H
#define OSSM_ESENSORTYPE_H

#include <stdint.h>

enum ESensorType : uint8_t {
    SENSOR_DISABLED = 0,

    // Pressure sensors (0.5-4.5V output)
    PRESSURE_0_100PSI = 1,
    PRESSURE_0_150PSI = 2,
    PRESSURE_0_200PSI = 3,

    // NTC Temperature sensors (resistance-based, Steinhart-Hart)
    TEMP_NTC_AEM = 10,    // AEM 30-2012 style
    TEMP_NTC_BOSCH = 11,  // Bosch style
    TEMP_NTC_GM = 12,     // GM style

    // Thermocouple (MAX31856)
    TEMP_THERMOCOUPLE_K = 20,

    // BME280 ambient (fixed assignment, not configurable per-channel)
    AMBIENT_TEMP = 30,
    AMBIENT_HUMIDITY = 31,
    AMBIENT_PRESSURE = 32
};

#endif  // OSSM_ESENSORTYPE_H
