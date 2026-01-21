#include "ossm.h"

#include <Arduino.h>
#include "app_data.h"
#include <AppConfig.h>

#include "Data/ConfigStorage/ConfigStorage.h"
#include "Data/ADS1115Manager/ADS1115Manager.h"
#include "Data/MAX31856Manager/MAX31856Manager.h"
#include "Data/BME280Manager/BME280Manager.h"
#include "Domain/SensorProcessor/SensorProcessor.h"
#include "Domain/CommandHandler/CommandHandler.h"
#include "Display/J1939Bus.h"
#include "Interface/SerialCommandHandler/SerialCommandHandler.h"

// Static member initialization
AppData ossm::appData;
AppConfig ossm::appConfig;
IntervalTimer ossm::sensorTimer;
volatile bool ossm::sensorUpdateReady = false;
elapsedMillis ossm::halfSecondMillis;
elapsedMillis ossm::oneSecondMillis;

void ossm::setup() {
    Serial.begin(115200);

    // Wait for Serial in debug mode (comment out for production)
    // while (!Serial && millis() < 3000) {}

    Serial.println("OSSM Initializing...");

    // Load configuration from EEPROM (or use defaults)
    if (!ConfigStorage::loadConfig(&appConfig)) {
        Serial.println("Loading default configuration");
        ConfigStorage::loadDefaults(&appConfig);
        // Save defaults to EEPROM for next boot
        ConfigStorage::saveConfig(&appConfig);
    } else {
        Serial.println("Configuration loaded from EEPROM");
    }

    // Initialize managers with configuration
    ADS1115Manager::initialize(&appConfig);
    MAX31856Manager::initialize(&appConfig);
    BME280Manager::initialize(&appConfig);
    SensorProcessor::initialize(&appConfig, &appData);
    CommandHandler::initialize(&appConfig, &appData);

    // Initialize J1939 bus
    J1939Bus::initialize(&appData, &appConfig);

    // Initialize serial command handler
    SerialCommandHandler::initialize(&appConfig, &appData);

    // Start sensor polling timer (50ms interval)
    sensorTimer.begin(sensorTimerCallback, SENSOR_TIMER_INTERVAL_US);

    Serial.println("OSSM Ready");
}

void ossm::loop() {
    // Process sensor updates when timer fires
    if (sensorUpdateReady) {
        processSensorUpdates();
        sensorUpdateReady = false;
    }

    // Process serial commands
    SerialCommandHandler::update();

    // Send J1939 messages on their own timing
    sendJ1939Messages();
}

void ossm::sensorTimerCallback() {
    // This runs in interrupt context - keep it minimal
    // Just set a flag to process in main loop
    sensorUpdateReady = true;
}

void ossm::processSensorUpdates() {
    // Poll ADS1115 devices (non-blocking, advances state machine)
    ADS1115Manager::update();

    // Poll MAX31856 thermocouple (non-blocking)
    MAX31856Manager::update();

    // Poll BME280 ambient sensor (reads every 1 second)
    BME280Manager::update();

    // Process all sensor readings and update AppData
    SensorProcessor::processAllInputs();
}

void ossm::sendJ1939Messages() {
    // Every 500ms - send fast-updating data
    if (halfSecondMillis >= 500) {
        // PGN 65270 - Inlet/Exhaust Conditions 1
        // Air inlet pressure, air inlet temp, EGT, boost pressure
        J1939Bus::sendPgn65270(
            appData.airInletPressurekPa,
            appData.airInletTemperatureC,
            appData.egtTemperatureC,
            appData.boostPressurekPa);

        // PGN 65263 - Engine Fluid Level/Pressure 1
        // Fuel pressure, oil pressure, coolant pressure
        J1939Bus::sendPgn65263(
            appData.fuelPressurekPa,
            appData.oilPressurekPa,
            appData.coolantPressurekPa);

        // PGN 65190 - Turbocharger Information 2
        // Turbo 1 boost, turbo 2 boost
        J1939Bus::sendPgn65190(
            appData.boostPressurekPa,
            appData.cacInletPressurekPa);

        halfSecondMillis = 0;
    }

    // Every 1000ms - send slower-updating data
    if (oneSecondMillis >= 1000) {
        // PGN 65269 - Ambient Conditions
        // Ambient temp, air inlet temp, barometric pressure
        J1939Bus::sendPgn65269(
            appData.ambientTemperatureC,
            appData.airInletTemperatureC,
            appData.absoluteBarometricpressurekPa);

        // PGN 65262 - Engine Temperature 1
        // Coolant temp, fuel temp, oil temp
        J1939Bus::sendPgn65262(
            appData.coolantTemperatureC,
            appData.fuelTemperatureC,
            appData.oilTemperatureC);

        // PGN 65129 - Engine Temperature 2
        // Intake manifold temp, coolant temp
        J1939Bus::sendPgn65129(
            appData.boostTemperatureC,
            appData.coolantTemperatureC);

        // PGN 65189 - Turbocharger Information 1
        // Intake manifold temps 2, 3, 4
        J1939Bus::sendPgn65189(
            appData.cacInletTemperatureC,
            appData.transferPipeTemperatureC,
            appData.engineBayTemperatureC);

        // PGN 65164 - Electronic Engine Controller 6
        J1939Bus::sendPgn65164();

        oneSecondMillis = 0;
    }
}

AppData* ossm::getAppData() {
    return &appData;
}

AppConfig* ossm::getAppConfig() {
    return &appConfig;
}
