#include "ADS1115Manager.h"
#include <Arduino.h>
#include "hardware_map.h"

// Static member initialization
Adafruit_ADS1115 ADS1115Manager::ads[ADS_DEVICE_COUNT];
uint8_t ADS1115Manager::drdyPins[ADS_DEVICE_COUNT] = {0, 0, 0, 0};
bool ADS1115Manager::deviceEnabled[ADS_DEVICE_COUNT] = {false, false, false, false};
bool ADS1115Manager::deviceInitialized[ADS_DEVICE_COUNT] = {false, false, false, false};
volatile TAdcReading ADS1115Manager::readings[ADS_DEVICE_COUNT][ADS_CHANNEL_COUNT];
uint8_t ADS1115Manager::currentDevice = 0;
uint8_t ADS1115Manager::currentChannel = 0;
bool ADS1115Manager::conversionStarted = false;
uint32_t ADS1115Manager::conversionStartTime = 0;

// MUX values for single-ended channels
const uint16_t ADS1115Manager::MUX_SINGLE[4] = {
    ADS1X15_REG_CONFIG_MUX_SINGLE_0,
    ADS1X15_REG_CONFIG_MUX_SINGLE_1,
    ADS1X15_REG_CONFIG_MUX_SINGLE_2,
    ADS1X15_REG_CONFIG_MUX_SINGLE_3
};

void ADS1115Manager::initialize(const AppConfig* config) {
    // Initialize reading buffers
    for (uint8_t d = 0; d < ADS_DEVICE_COUNT; d++) {
        for (uint8_t c = 0; c < ADS_CHANNEL_COUNT; c++) {
            readings[d][c].rawValue = 0;
            readings[d][c].timestamp = 0;
            readings[d][c].valid = false;
        }
    }

    // Determine which ADS devices need to be enabled based on input usage
    // Check temp inputs using C-Next hardware_map
    for (uint8_t i = 0; i < TEMP_INPUT_COUNT; i++) {
        if (config->tempInputs[i].assignedSpn != 0) {
            uint8_t dev = hardware_map_tempDevice(i);
            deviceEnabled[dev] = true;
        }
    }

    // Check pressure inputs using C-Next hardware_map
    for (uint8_t i = 0; i < PRESSURE_INPUT_COUNT; i++) {
        if (config->pressureInputs[i].assignedSpn != 0) {
            uint8_t dev = hardware_map_pressureDevice(i);
            deviceEnabled[dev] = true;
        }
    }

    // Initialize each enabled ADS1115 device using fixed hardware mappings
    for (uint8_t d = 0; d < ADS_DEVICE_COUNT; d++) {
        drdyPins[d] = ADS_DRDY_PINS[d];

        if (!deviceEnabled[d]) {
            deviceInitialized[d] = false;
            continue;
        }

        // Configure DRDY pin as input
        pinMode(drdyPins[d], INPUT);

        // Initialize ADS1115 at fixed address
        uint8_t addr = ADS_I2C_ADDRESSES[d];
        if (ads[d].begin(addr)) {
            deviceInitialized[d] = true;

            // Set gain to +/- 4.096V (default, good for 0-5V sensors)
            ads[d].setGain(GAIN_ONE);

            // Set data rate to 128 SPS (default)
            ads[d].setDataRate(RATE_ADS1115_128SPS);

            Serial.print("ADS1115 @ 0x");
            Serial.print(addr, HEX);
            Serial.print(" initialized, DRDY pin D");
            Serial.println(drdyPins[d]);
        } else {
            deviceInitialized[d] = false;
            Serial.print("ADS1115 @ 0x");
            Serial.print(addr, HEX);
            Serial.println(" FAILED to initialize");
        }
    }

    // Find first enabled device to start with
    currentDevice = 0;
    currentChannel = 0;
    conversionStarted = false;

    // Skip to first enabled device
    while (currentDevice < ADS_DEVICE_COUNT && !deviceInitialized[currentDevice]) {
        currentDevice++;
    }

    // Start first conversion if we have any devices
    if (currentDevice < ADS_DEVICE_COUNT) {
        startConversion();
    }
}

bool ADS1115Manager::update() {
    // No devices available
    if (currentDevice >= ADS_DEVICE_COUNT) {
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
        advanceChannel();

        // Start next conversion immediately
        if (currentDevice < ADS_DEVICE_COUNT) {
            startConversion();
        }
        return true;
    }

    // Check for timeout
    if (millis() - conversionStartTime > CONVERSION_TIMEOUT_MS) {
        Serial.print("ADS1115 timeout on device ");
        Serial.print(currentDevice);
        Serial.print(" channel ");
        Serial.println(currentChannel);

        // Mark reading as invalid and move on
        readings[currentDevice][currentChannel].valid = false;
        advanceChannel();

        if (currentDevice < ADS_DEVICE_COUNT) {
            startConversion();
        }
    }

    return false;
}

void ADS1115Manager::startConversion() {
    if (currentDevice >= ADS_DEVICE_COUNT || !deviceInitialized[currentDevice]) {
        conversionStarted = false;
        return;
    }

    // Start single-shot conversion on current channel
    ads[currentDevice].startADCReading(MUX_SINGLE[currentChannel], false);

    conversionStarted = true;
    conversionStartTime = millis();
}

bool ADS1115Manager::isConversionComplete() {
    if (!conversionStarted || currentDevice >= ADS_DEVICE_COUNT) {
        return false;
    }

    // Check DRDY pin first (active low when data ready)
    if (digitalRead(drdyPins[currentDevice]) == LOW) {
        return true;
    }

    // Fallback to polling the conversion complete flag
    return ads[currentDevice].conversionComplete();
}

void ADS1115Manager::readResult() {
    if (currentDevice >= ADS_DEVICE_COUNT || !deviceInitialized[currentDevice]) {
        return;
    }

    int16_t result = ads[currentDevice].getLastConversionResults();
    uint32_t now = millis();

    // Store in buffer with interrupt protection
    noInterrupts();
    readings[currentDevice][currentChannel].rawValue = result;
    readings[currentDevice][currentChannel].timestamp = now;
    readings[currentDevice][currentChannel].valid = true;
    interrupts();

    conversionStarted = false;
}

void ADS1115Manager::advanceChannel() {
    // Move to next channel
    currentChannel++;

    // If we've done all channels on this device, move to next device
    if (currentChannel >= ADS_CHANNEL_COUNT) {
        currentChannel = 0;
        currentDevice++;

        // Skip disabled/uninitialized devices
        while (currentDevice < ADS_DEVICE_COUNT && !deviceInitialized[currentDevice]) {
            currentDevice++;
        }

        // Wrap around to first device if we've gone through all
        if (currentDevice >= ADS_DEVICE_COUNT) {
            currentDevice = 0;

            // Find first enabled device
            while (currentDevice < ADS_DEVICE_COUNT && !deviceInitialized[currentDevice]) {
                currentDevice++;
            }
        }
    }
}

TAdcReading ADS1115Manager::getReading(uint8_t device, uint8_t channel) {
    TAdcReading copy;

    if (device >= ADS_DEVICE_COUNT || channel >= ADS_CHANNEL_COUNT) {
        copy.rawValue = 0;
        copy.timestamp = 0;
        copy.valid = false;
        return copy;
    }

    // Thread-safe copy (manual copy from volatile)
    noInterrupts();
    copy.rawValue = readings[device][channel].rawValue;
    copy.timestamp = readings[device][channel].timestamp;
    copy.valid = readings[device][channel].valid;
    interrupts();

    return copy;
}

float ADS1115Manager::getVoltage(uint8_t device, uint8_t channel) {
    TAdcReading reading = getReading(device, channel);

    if (!reading.valid) {
        return 0.0f;
    }

    // Convert raw value to voltage
    // At GAIN_ONE, full scale is +/- 4.096V
    // ADS1115 is 16-bit signed, so 32767 = 4.096V
    return ads[device].computeVolts(reading.rawValue);
}

bool ADS1115Manager::isDeviceEnabled(uint8_t device) {
    if (device >= ADS_DEVICE_COUNT) {
        return false;
    }
    return deviceEnabled[device] && deviceInitialized[device];
}
