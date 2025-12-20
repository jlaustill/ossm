#include "BME280Manager.h"
#include <Arduino.h>
#include <Wire.h>

// Static member initialization
Adafruit_BME280 BME280Manager::bme;
uint8_t BME280Manager::i2cAddress = 0x76;
bool BME280Manager::enabled = false;
bool BME280Manager::initialized = false;
float BME280Manager::temperatureC = 0.0f;
float BME280Manager::humidity = 0.0f;
float BME280Manager::pressurekPa = 0.0f;
bool BME280Manager::readingValid = false;
uint32_t BME280Manager::lastReadTime = 0;

void BME280Manager::initialize(const AppConfig* config) {
    enabled = config->bme280Enabled;

    if (!enabled) {
        Serial.println("BME280 disabled in config");
        return;
    }

    // Fixed I2C address (0x76 is standard for BME280)
    i2cAddress = 0x76;

    if (bme.begin(i2cAddress)) {
        initialized = true;

        // Configure for weather monitoring (low power, low oversampling)
        bme.setSampling(
            Adafruit_BME280::MODE_NORMAL,     // Normal mode (continuous)
            Adafruit_BME280::SAMPLING_X1,     // Temperature oversampling x1
            Adafruit_BME280::SAMPLING_X1,     // Pressure oversampling x1
            Adafruit_BME280::SAMPLING_X1,     // Humidity oversampling x1
            Adafruit_BME280::FILTER_OFF,      // No filtering
            Adafruit_BME280::STANDBY_MS_1000  // 1 second standby
        );

        Serial.print("BME280 initialized at 0x");
        Serial.println(i2cAddress, HEX);

        // Do initial read
        update();
    } else {
        initialized = false;
        Serial.print("BME280 FAILED to initialize at 0x");
        Serial.print(i2cAddress, HEX);
        Serial.println(" - check wiring");

        // Print sensor ID for debugging
        Serial.print("  Sensor ID: 0x");
        Serial.println(bme.sensorID(), HEX);
        Serial.println("  ID 0xFF = bad address or no sensor");
        Serial.println("  ID 0x56-0x58 = BMP280");
        Serial.println("  ID 0x60 = BME280");
        Serial.println("  ID 0x61 = BME680");
    }
}

bool BME280Manager::update() {
    if (!enabled || !initialized) {
        return false;
    }

    // Only read periodically to reduce I2C traffic
    uint32_t now = millis();
    if (now - lastReadTime < READ_INTERVAL_MS) {
        return false;
    }

    // Read all values (BME280 reads are fast, ~1ms)
    temperatureC = bme.readTemperature();
    humidity = bme.readHumidity();

    // Convert from Pa to kPa
    pressurekPa = bme.readPressure() / 1000.0f;

    // Check for valid readings (NaN check)
    if (isnan(temperatureC) || isnan(humidity) || isnan(pressurekPa)) {
        readingValid = false;
        return false;
    }

    readingValid = true;
    lastReadTime = now;

    return true;
}

float BME280Manager::getTemperatureC() {
    if (!readingValid) {
        return -273.15f;  // Return absolute zero as error indicator
    }
    return temperatureC;
}

float BME280Manager::getHumidity() {
    if (!readingValid) {
        return 0.0f;
    }
    return humidity;
}

float BME280Manager::getPressurekPa() {
    if (!readingValid) {
        return 0.0f;
    }
    return pressurekPa;
}

bool BME280Manager::isEnabled() {
    return enabled && initialized;
}
