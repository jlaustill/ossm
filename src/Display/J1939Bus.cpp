#ifndef OSSM_J1939Bus_CPP
#define OSSM_J1939Bus_CPP

#include "J1939Bus.h"

#include <AppData.h>
#include <AppConfig.h>
#include <FlexCAN_T4.h>

#include <SeaDash.hpp>
#include "Domain/CommandHandler/CommandHandler.h"

// OSSM v0.0.2 uses CAN1 (D22/D23) - single bus for J1939 transmission
FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> CanBus;

AppData *J1939Bus::appData;
AppConfig *J1939Bus::config;
volatile bool warmedUp = false;
volatile uint16_t rpms = 0;
volatile float maxTiming = 12.0f;
volatile float maxFuel = 40.95f;
volatile float throttlePercent = 0.0f;
volatile float load = 0.0f;
volatile float newTiming = 0.0f;
volatile float newFuel = 0.0f;
volatile uint32_t shortFuelValue = 0;
volatile unsigned short shortTimingValue = 0;
volatile uint16_t maxOfThrottleAndLoad = 0;

// Helper: Check if a temperature SPN is enabled (assigned to any input)
static bool isTempSpnEnabled(const AppConfig* cfg, uint16_t spn) {
    for (uint8_t i = 0; i < TEMP_INPUT_COUNT; i++) {
        if (cfg->tempInputs[i].assignedSpn == spn) {
            return true;
        }
    }
    return false;
}

// Helper: Check if a pressure SPN is enabled (assigned to any input)
static bool isPressureSpnEnabled(const AppConfig* cfg, uint16_t spn) {
    for (uint8_t i = 0; i < PRESSURE_INPUT_COUNT; i++) {
        if (cfg->pressureInputs[i].assignedSpn == spn) {
            return true;
        }
    }
    return false;
}

// Helper: Check if any SPN is enabled
static bool isSpnEnabled(const AppConfig* cfg, uint16_t spn) {
    // Check temperature SPNs
    if (isTempSpnEnabled(cfg, spn)) return true;

    // Check pressure SPNs
    if (isPressureSpnEnabled(cfg, spn)) return true;

    // Check EGT
    if (spn == 173 && cfg->egtEnabled) return true;

    // Check BME280 SPNs
    if ((spn == 171 || spn == 108 || spn == 354) && cfg->bme280Enabled) return true;

    // Check hi-res SPNs (auto-enabled with their standard counterparts)
    if (spn == 1637 && isTempSpnEnabled(cfg, 110)) return true;  // Hi-res coolant
    if (spn == 1363 && isTempSpnEnabled(cfg, 105)) return true;  // Hi-res intake

    return false;
}

void UpdateMaxTiming(float timing) {
    if (rpms < 1200) {
        maxTiming = timing;
    } else {
        maxTiming = 30.0f;
    }
}

void J1939Bus::sniffDataCumminsBus(const CAN_message_t &msg) {
    J1939Message message = J1939Message();
    message.setCanId(msg.id);
    message.setData(msg.buf);

    switch (message.canId) {
        case 256:
            if (warmedUp) {
                rpms = static_cast<uint16_t>(((uint16_t)message.data[7] << 8) + (uint16_t)message.data[6]);
                rpms /= 4;
                float timing = (float)((uint16_t)message.data[5] << 8) + (uint16_t)message.data[4];
                timing /= 128.0f;
                float fuel = (float)((uint16_t)message.data[1] << 8) + (uint16_t)message.data[0];
                fuel /= 40.95f;
                CAN_message_t copyMsg = msg;
                UpdateMaxTiming(timing);
                maxOfThrottleAndLoad = static_cast<uint16_t>(max(throttlePercent, load));
                newTiming = SeaDash::Floats::mapf<float>((float)maxOfThrottleAndLoad, 0.0f, 100.0f, timing, maxTiming);
                newTiming *= 128.0f;
                shortTimingValue = (unsigned short)newTiming;
                copyMsg.buf[4] = lowByte(shortTimingValue);
                copyMsg.buf[5] = highByte(shortTimingValue);
                newFuel = SeaDash::Floats::mapf<float>(maxOfThrottleAndLoad, 0.0f, 100.0f, fuel, maxFuel);
                newFuel *= 40.95f;
                shortFuelValue = (uint32_t)newFuel;
                copyMsg.buf[0] = lowByte(shortFuelValue);
                copyMsg.buf[1] = highByte(shortFuelValue);
                CanBus.write(copyMsg);
            }
            break;
    }

    if (message.pgn == 65262) {
        int16_t waterTemp = (int16_t)message.data[0] - 40;
        if (waterTemp >= 65) {
            warmedUp = true;
        }
    }

    if (message.pgn == 59904) {
        uint32_t requestedPgn = message.data[0];
        requestedPgn |= message.data[1] << 8;
        requestedPgn |= message.data[2] << 16;

        if (requestedPgn == 65164) {
            sendPgn65164();
        } else {
            CanBus.write(msg);
        }
    } else {
        CanBus.write(msg);
    }
}

void J1939Bus::sniffDataPrivate(const CAN_message_t &msg) {
    J1939Message message = J1939Message();
    message.setCanId(msg.id);
    message.setData(msg.buf);

    // PGN 65280 (0xFF00) - Configuration commands
    if (message.pgn == 65280) {
        processConfigCommand(msg.buf, 8);
        return;
    }

    if (message.pgn == 59904) {
        CanBus.write(msg);
    }
}

void J1939Bus::initialize(AppData *currentData, AppConfig *cfg) {
    Serial.println("J1939 Bus initializing");

    appData = currentData;
    config = cfg;

    CanBus.begin();
    CanBus.setBaudRate(250 * 1000);
    CanBus.setMaxMB(16);
    CanBus.enableFIFO();
    CanBus.enableFIFOInterrupt();
    CanBus.onReceive(sniffDataPrivate);  // Handle incoming J1939 config commands
    CanBus.mailboxStatus();

    Serial.print("J1939 Source Address: ");
    Serial.println(config->j1939SourceAddress);
}

void J1939Bus::sendPgn65129(float engineIntakeManifold1AirTemperatureC,
                            float engineCoolantTemperatureC) {
    J1939Message message = J1939Message();
    message.setPgn(65129);
    message.setPriority(6);
    message.setSourceAddress(config->j1939SourceAddress);

    CAN_message_t msg;
    msg.flags.extended = 1;
    msg.id = message.canId;
    msg.len = 8;

    for (int i = 0; i < 8; i++) msg.buf[i] = 0xFF;

    float intakeTempOffset = engineIntakeManifold1AirTemperatureC + 273.0f;
    intakeTempOffset /= 0.03125f;
    float coolantTempOffset = engineCoolantTemperatureC + 273.0f;
    coolantTempOffset /= 0.03125f;

    if (isSpnEnabled(config, 1363)) {
        msg.buf[0] = highByte(static_cast<uint16_t>(intakeTempOffset));
        msg.buf[1] = lowByte(static_cast<uint16_t>(intakeTempOffset));
    }
    if (isSpnEnabled(config, 1637)) {
        msg.buf[2] = highByte(static_cast<uint16_t>(coolantTempOffset));
        msg.buf[3] = lowByte(static_cast<uint16_t>(coolantTempOffset));
    }

    CanBus.write(msg);
}

void J1939Bus::sendPgn65164() {
    J1939Message message = J1939Message();
    message.setPgn(65164);
    message.setPriority(6);
    message.setSourceAddress(config->j1939SourceAddress);

    CAN_message_t msg;
    msg.flags.extended = 1;
    msg.id = message.canId;
    msg.len = 8;

    for (int i = 0; i < 8; i++) msg.buf[i] = 0xFF;

    if (isSpnEnabled(config, 441)) {
        msg.buf[0] = static_cast<uint8_t>(appData->engineBayTemperatureC + 40);
    }
    if (isSpnEnabled(config, 354)) {
        msg.buf[6] = static_cast<uint8_t>(appData->humidity / 0.4f);
    }

    CanBus.write(msg);
}

void J1939Bus::sendPgn65189(float engineIntakeManifold2TemperatureC,
                            float engineIntakeManifold3TemperatureC,
                            float engineIntakeManifold4TemperatureC) {
    J1939Message message = J1939Message();
    message.setPgn(65189);
    message.setPriority(6);
    message.setSourceAddress(config->j1939SourceAddress);

    CAN_message_t msg;
    msg.flags.extended = 1;
    msg.id = message.canId;
    msg.len = 8;

    for (int i = 0; i < 8; i++) msg.buf[i] = 0xFF;

    if (isSpnEnabled(config, 1131)) {
        msg.buf[0] = static_cast<uint8_t>(engineIntakeManifold2TemperatureC + 40);
    }
    if (isSpnEnabled(config, 1132)) {
        msg.buf[1] = static_cast<uint8_t>(engineIntakeManifold3TemperatureC + 40);
    }
    if (isSpnEnabled(config, 1133)) {
        msg.buf[2] = static_cast<uint8_t>(engineIntakeManifold4TemperatureC + 40);
    }

    CanBus.write(msg);
}

void J1939Bus::sendPgn65190(float engineTurbocharger1BoostPressurekPa,
                            float engineTurbocharger2BoostPressurekPa) {
    J1939Message message = J1939Message();
    message.setPgn(65190);
    message.setPriority(6);
    message.setSourceAddress(config->j1939SourceAddress);

    CAN_message_t msg;
    msg.flags.extended = 1;
    msg.id = message.canId;
    msg.len = 8;

    for (int i = 0; i < 8; i++) msg.buf[i] = 0xFF;

    float boost1Offset = engineTurbocharger1BoostPressurekPa / 0.125f;
    float boost2Offset = engineTurbocharger2BoostPressurekPa / 0.125f;

    if (isSpnEnabled(config, 1127)) {
        msg.buf[0] = highByte(static_cast<uint16_t>(boost1Offset));
        msg.buf[1] = lowByte(static_cast<uint16_t>(boost1Offset));
    }
    if (isSpnEnabled(config, 1128)) {
        msg.buf[2] = highByte(static_cast<uint16_t>(boost2Offset));
        msg.buf[3] = lowByte(static_cast<uint16_t>(boost2Offset));
    }

    CanBus.write(msg);
}

void J1939Bus::sendPgn65262(float engineCoolantTemperatureC,
                            float engineFuelTemperatureC,
                            float engineOilTemperatureC) {
    J1939Message message = J1939Message();
    message.setPgn(65262);
    message.setPriority(6);
    message.setSourceAddress(config->j1939SourceAddress);

    CAN_message_t msg;
    msg.flags.extended = 1;
    msg.id = message.canId;
    msg.len = 8;

    for (int i = 0; i < 8; i++) msg.buf[i] = 0xFF;

    float oilTempOffset = engineOilTemperatureC + 273.0f;
    oilTempOffset /= 0.03125f;

    if (isSpnEnabled(config, 110)) {
        msg.buf[0] = static_cast<uint8_t>(engineCoolantTemperatureC + 40);
    }
    if (isSpnEnabled(config, 174)) {
        msg.buf[1] = static_cast<uint8_t>(engineFuelTemperatureC + 40);
    }
    if (isSpnEnabled(config, 175)) {
        msg.buf[2] = highByte(static_cast<uint16_t>(oilTempOffset));
        msg.buf[3] = lowByte(static_cast<uint16_t>(oilTempOffset));
    }

    CanBus.write(msg);
}

void J1939Bus::sendPgn65263(float engineFuelDeliveryPressurekPa,
                            float engineOilPressurekPa,
                            float engineCoolantPressurekPa) {
    J1939Message message = J1939Message();
    message.setPgn(65263);
    message.setPriority(6);
    message.setSourceAddress(config->j1939SourceAddress);

    CAN_message_t msg;
    msg.flags.extended = 1;
    msg.id = message.canId;
    msg.len = 8;

    for (int i = 0; i < 8; i++) msg.buf[i] = 0xFF;

    if (isSpnEnabled(config, 94)) {
        msg.buf[0] = static_cast<uint8_t>(engineFuelDeliveryPressurekPa / 4);
    }
    if (isSpnEnabled(config, 100)) {
        msg.buf[3] = static_cast<uint8_t>(engineOilPressurekPa / 4);
    }
    if (isSpnEnabled(config, 109)) {
        msg.buf[6] = static_cast<uint8_t>(engineCoolantPressurekPa / 2);
    }

    CanBus.write(msg);
}

void J1939Bus::sendPgn65269(float ambientTemperatureC,
                            float airInletTemperatureC,
                            float barometricPressurekPa) {
    J1939Message message = J1939Message();
    message.setPgn(65269);
    message.setPriority(6);
    message.setSourceAddress(config->j1939SourceAddress);

    CAN_message_t msg;
    msg.flags.extended = 1;
    msg.id = message.canId;
    msg.len = 8;

    for (int i = 0; i < 8; i++) msg.buf[i] = 0xFF;

    float ambientAirTempOffset = ambientTemperatureC + 273.0f;
    ambientAirTempOffset /= 0.03125f;

    if (isSpnEnabled(config, 108)) {
        msg.buf[0] = static_cast<uint8_t>(barometricPressurekPa * 2);
    }
    if (isSpnEnabled(config, 171)) {
        msg.buf[3] = highByte(static_cast<uint16_t>(ambientAirTempOffset));
        msg.buf[4] = lowByte(static_cast<uint16_t>(ambientAirTempOffset));
    }
    if (isSpnEnabled(config, 172)) {
        msg.buf[5] = static_cast<uint8_t>(airInletTemperatureC + 40);
    }

    CanBus.write(msg);
}

void J1939Bus::sendPgn65270(float airInletPressurekPa,
                            float airInletTemperatureC, float egtTemperatureC,
                            float boostPressurekPa) {
    J1939Message message = J1939Message();
    message.setPgn(65270);
    message.setPriority(6);
    message.setSourceAddress(config->j1939SourceAddress);

    CAN_message_t msg;
    msg.flags.extended = 1;
    msg.id = message.canId;
    msg.len = 8;

    for (int i = 0; i < 8; i++) msg.buf[i] = 0xFF;

    float egtOffset = egtTemperatureC + 273.0f;
    egtOffset /= 0.03125f;

    if (isSpnEnabled(config, 102)) {
        msg.buf[1] = static_cast<uint8_t>(boostPressurekPa / 2);
    }
    if (isSpnEnabled(config, 105)) {
        msg.buf[2] = static_cast<uint8_t>(airInletTemperatureC + 40);
    }
    if (isSpnEnabled(config, 106)) {
        msg.buf[3] = static_cast<uint8_t>(airInletPressurekPa / 2);
    }
    if (isSpnEnabled(config, 173)) {
        msg.buf[5] = highByte(static_cast<uint16_t>(egtOffset));
        msg.buf[6] = lowByte(static_cast<uint16_t>(egtOffset));
    }

    CanBus.write(msg);
}

// ============================================================================
// J1939 Command Interface (PGN 65280 / 65281)
// ============================================================================

void J1939Bus::sendConfigResponse(uint8_t cmd, uint8_t errCode, const uint8_t *data, uint8_t dataLen) {
    J1939Message message = J1939Message();
    message.setPgn(65281);  // Response PGN
    message.setPriority(6);
    message.setSourceAddress(config->j1939SourceAddress);

    CAN_message_t msg;
    msg.flags.extended = 1;
    msg.id = message.canId;
    msg.len = 8;

    // Response format: [cmd, errCode, data...]
    msg.buf[0] = cmd;
    msg.buf[1] = errCode;  // 0 = OK, >0 = error code

    for (uint8_t i = 0; i < 6 && i < dataLen; i++) {
        msg.buf[2 + i] = data ? data[i] : 0xFF;
    }
    for (uint8_t i = dataLen; i < 6; i++) {
        msg.buf[2 + i] = 0xFF;
    }

    CanBus.write(msg);
}

void J1939Bus::processConfigCommand(const uint8_t *data, uint8_t len) {
    if (len < 1) return;

    uint8_t cmd = data[0];

    switch (cmd) {
        case 1:  // Enable/Disable SPN
            handleJ1939EnableSpn(data);
            break;
        case 2:  // Set NTC Param
            handleJ1939SetNtcParam(data);
            break;
        case 3:  // Set Pressure Range
            handleJ1939SetPressureRange(data);
            break;
        case 4:  // Set Thermocouple Type
            handleJ1939SetTcType(data);
            break;
        case 5:  // Query
            handleJ1939Query(data);
            break;
        case 6:  // Save
            handleJ1939Save();
            break;
        case 7:  // Reset
            handleJ1939Reset();
            break;
        case 8:  // NTC Preset
            handleJ1939NtcPreset(data);
            break;
        case 9:  // Pressure Preset
            handleJ1939PressurePreset(data);
            break;
        default:
            sendConfigResponse(cmd, 1, nullptr, 0);  // Error: Unknown command
            break;
    }
}

void J1939Bus::handleJ1939EnableSpn(const uint8_t *data) {
    // Format: [1, spn_high, spn_low, enable, input?]
    uint16_t spn = (static_cast<uint16_t>(data[1]) << 8) | data[2];
    bool enable = (data[3] != 0);
    uint8_t input = data[4];

    TCommandResult result = CommandHandler::enableSpn(spn, enable, input);
    sendConfigResponse(1, result.errorCode, result.data, result.dataLen);
}

void J1939Bus::handleJ1939SetNtcParam(const uint8_t *data) {
    // Format: [2, input, param, b0, b1, b2, b3] - IEEE 754 float
    uint8_t input = data[1];
    uint8_t param = data[2];

    // Reconstruct float from bytes (little endian)
    union {
        float f;
        uint8_t bytes[4];
    } converter;
    converter.bytes[0] = data[3];
    converter.bytes[1] = data[4];
    converter.bytes[2] = data[5];
    converter.bytes[3] = data[6];

    TCommandResult result = CommandHandler::setNtcParam(input, param, converter.f);
    sendConfigResponse(2, result.errorCode, result.data, result.dataLen);
}

void J1939Bus::handleJ1939SetPressureRange(const uint8_t *data) {
    // Format: [3, input, psi_high, psi_low]
    uint8_t input = data[1];
    uint16_t maxPsi = (static_cast<uint16_t>(data[2]) << 8) | data[3];

    TCommandResult result = CommandHandler::setPressureRange(input, maxPsi);
    sendConfigResponse(3, result.errorCode, result.data, result.dataLen);
}

void J1939Bus::handleJ1939SetTcType(const uint8_t *data) {
    // Format: [4, type]
    uint8_t type = data[1];

    TCommandResult result = CommandHandler::setTcType(type);
    sendConfigResponse(4, result.errorCode, result.data, result.dataLen);
}

void J1939Bus::handleJ1939Query(const uint8_t *data) {
    // Format: [5, query_type]
    uint8_t queryType = data[1];

    TCommandResult result;
    switch (queryType) {
        case 0:
            result = CommandHandler::querySpnCounts();
            break;
        case 4:
            result = CommandHandler::queryFullConfig();
            break;
        default:
            result = TCommandResult::error(ECommandError::INVALID_QUERY_TYPE);
            break;
    }
    sendConfigResponse(5, result.errorCode, result.data, result.dataLen);
}

void J1939Bus::handleJ1939Save() {
    TCommandResult result = CommandHandler::save();
    sendConfigResponse(6, result.errorCode, result.data, result.dataLen);
}

void J1939Bus::handleJ1939Reset() {
    TCommandResult result = CommandHandler::reset();
    sendConfigResponse(7, result.errorCode, result.data, result.dataLen);
}

void J1939Bus::handleJ1939NtcPreset(const uint8_t *data) {
    // Format: [8, input, preset]
    uint8_t input = data[1];
    uint8_t preset = data[2];

    TCommandResult result = CommandHandler::applyNtcPreset(input, preset);
    sendConfigResponse(8, result.errorCode, result.data, result.dataLen);
}

void J1939Bus::handleJ1939PressurePreset(const uint8_t *data) {
    // Format: [9, input, preset]
    uint8_t input = data[1];
    uint8_t preset = data[2];

    TCommandResult result = CommandHandler::applyPressurePreset(input, preset);
    sendConfigResponse(9, result.errorCode, result.data, result.dataLen);
}

#endif
