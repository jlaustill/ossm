#include "MAX31856Manager.h"
#include <Arduino.h>
#include <SPI.h>

// Static member initialization
Adafruit_MAX31856* MAX31856Manager::thermocouple = nullptr;
uint8_t MAX31856Manager::csPin = 10;
uint8_t MAX31856Manager::drdyPin = 2;
uint8_t MAX31856Manager::faultPin = 3;
bool MAX31856Manager::enabled = false;
bool MAX31856Manager::initialized = false;
bool MAX31856Manager::conversionStarted = false;
uint32_t MAX31856Manager::conversionStartTime = 0;
float MAX31856Manager::temperatureC = 0.0f;
float MAX31856Manager::coldJunctionC = 0.0f;
uint8_t MAX31856Manager::faultCode = 0;
bool MAX31856Manager::readingValid = false;

void MAX31856Manager::initialize(const AppConfig* config) {
    enabled = config->egtEnabled;

    if (!enabled) {
        Serial.println("MAX31856 disabled in config");
        return;
    }

    // Fixed pin configuration
    csPin = 10;    // D10 for chip select
    drdyPin = 2;   // D2 for data ready
    faultPin = 3;  // D3 for fault

    // Configure pins
    pinMode(drdyPin, INPUT);
    pinMode(faultPin, INPUT);

    // Create MAX31856 instance with hardware SPI
    thermocouple = new Adafruit_MAX31856(csPin);

    if (thermocouple->begin()) {
        initialized = true;

        // Configure thermocouple type from config
        thermocouple->setThermocoupleType(
            static_cast<max31856_thermocoupletype_t>(config->thermocoupleType));

        // Set noise rejection (60Hz for North America)
        thermocouple->setNoiseFilter(MAX31856_NOISE_FILTER_60HZ);

        // Use automatic conversion mode for continuous readings
        // Or use one-shot for lower power
        thermocouple->setConversionMode(MAX31856_ONESHOT);

        Serial.print("MAX31856 initialized on CS pin D");
        Serial.print(csPin);
        Serial.print(", DRDY D");
        Serial.print(drdyPin);
        Serial.print(", FAULT D");
        Serial.println(faultPin);

        // Start first conversion
        startConversion();
    } else {
        initialized = false;
        Serial.println("MAX31856 FAILED to initialize");
    }
}

bool MAX31856Manager::update() {
    if (!enabled || !initialized) {
        return false;
    }

    // No conversion in progress, start one
    if (!conversionStarted) {
        startConversion();
        return false;
    }

    // Check if conversion is complete
    if (isConversionComplete()) {
        readResult();

        // Start next conversion immediately
        startConversion();
        return true;
    }

    // Check for timeout
    if (millis() - conversionStartTime > CONVERSION_TIMEOUT_MS) {
        Serial.println("MAX31856 conversion timeout");
        readingValid = false;
        conversionStarted = false;

        // Try to start a new conversion
        startConversion();
    }

    return false;
}

void MAX31856Manager::startConversion() {
    if (!enabled || !initialized || thermocouple == nullptr) {
        conversionStarted = false;
        return;
    }

    // Trigger a one-shot conversion
    thermocouple->triggerOneShot();

    conversionStarted = true;
    conversionStartTime = millis();
}

bool MAX31856Manager::isConversionComplete() {
    if (!conversionStarted || !enabled || !initialized) {
        return false;
    }

    // Check DRDY pin (active low when data ready)
    if (digitalRead(drdyPin) == LOW) {
        return true;
    }

    // Fallback: poll the conversion complete status
    return thermocouple->conversionComplete();
}

void MAX31856Manager::readResult() {
    if (!enabled || !initialized || thermocouple == nullptr) {
        return;
    }

    // Check for faults first
    faultCode = thermocouple->readFault();

    if (faultCode != 0) {
        // Log fault details
        Serial.print("MAX31856 fault: 0x");
        Serial.println(faultCode, HEX);

        if (faultCode & MAX31856_FAULT_CJRANGE) {
            Serial.println("  Cold Junction Range Fault");
        }
        if (faultCode & MAX31856_FAULT_TCRANGE) {
            Serial.println("  Thermocouple Range Fault");
        }
        if (faultCode & MAX31856_FAULT_CJHIGH) {
            Serial.println("  Cold Junction High Fault");
        }
        if (faultCode & MAX31856_FAULT_CJLOW) {
            Serial.println("  Cold Junction Low Fault");
        }
        if (faultCode & MAX31856_FAULT_TCHIGH) {
            Serial.println("  Thermocouple High Fault");
        }
        if (faultCode & MAX31856_FAULT_TCLOW) {
            Serial.println("  Thermocouple Low Fault");
        }
        if (faultCode & MAX31856_FAULT_OVUV) {
            Serial.println("  Over/Under Voltage Fault");
        }
        if (faultCode & MAX31856_FAULT_OPEN) {
            Serial.println("  Thermocouple Open Fault");
        }

        readingValid = false;
    } else {
        // Read temperatures
        temperatureC = thermocouple->readThermocoupleTemperature();
        coldJunctionC = thermocouple->readCJTemperature();
        readingValid = true;
    }

    conversionStarted = false;
}

float MAX31856Manager::getTemperatureC() {
    if (!readingValid) {
        return -273.15f;  // Return absolute zero as error indicator
    }
    return temperatureC;
}

float MAX31856Manager::getColdJunctionC() {
    if (!readingValid) {
        return -273.15f;
    }
    return coldJunctionC;
}

bool MAX31856Manager::isEnabled() {
    return enabled && initialized;
}

uint8_t MAX31856Manager::getFaultStatus() {
    return faultCode;
}
