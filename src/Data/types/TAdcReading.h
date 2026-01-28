#ifndef TADCREADING_H
#define TADCREADING_H

#include <stdint.h>
#include <stdbool.h>

/*
 * Definition of the ADC reading structure used throughout the project.
 * rawValue   – The signed 16‑bit result from the ADS1115 conversion.
 * timestamp  – Milliseconds (from millis()) when the conversion completed.
 * valid      – Flag indicating whether the reading is fresh/valid.
 */
typedef struct {
    int16_t rawValue;
    uint32_t timestamp;
    bool     valid;
} TAdcReading;

#endif /* TADCREADING_H */
