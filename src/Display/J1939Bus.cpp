#ifndef OSSM_J1939Bus_CPP
#define OSSM_J1939Bus_CPP

#include "J1939Bus.h"

#include "AppData.h"
#include <AppConfig.h>
#include <FlexCAN_T4.h>

#include "Domain/CommandHandler/CommandHandler.h"
#include "J1939Encode.h"
#include "J1939Decode.h"
#include "SpnCheck.h"
#include "FloatBytes.h"

// OSSM v0.0.2 uses CAN1 (D22/D23) - single bus for J1939 transmission
FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> CanBus;

AppData *J1939Bus::appData;
AppConfig *J1939Bus::config;

// Direct call to C-Next spn_check (v0.1.13+ uses pass-by-value for small types)
static inline bool isSpnEnabled(const AppConfig* cfg, uint16_t spn) {
    return SpnCheck_isSpnEnabled(cfg, spn);
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

    if (isSpnEnabled(config, 1363)) {
        uint16_t intakeTemp = J1939Encode_temp16bit(engineIntakeManifold1AirTemperatureC);
        msg.buf[0] = J1939Encode_lowByte(intakeTemp);
        msg.buf[1] = J1939Encode_highByte(intakeTemp);
    }
    if (isSpnEnabled(config, 1637)) {
        uint16_t coolantTemp = J1939Encode_temp16bit(engineCoolantTemperatureC);
        msg.buf[2] = J1939Encode_lowByte(coolantTemp);
        msg.buf[3] = J1939Encode_highByte(coolantTemp);
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
        msg.buf[0] = J1939Encode_temp8bit(appData->engineBayTemperatureC);
    }
    if (isSpnEnabled(config, 354)) {
        msg.buf[6] = J1939Encode_humidity(appData->humidity);
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
        msg.buf[0] = J1939Encode_temp8bit(engineIntakeManifold2TemperatureC);
    }
    if (isSpnEnabled(config, 1132)) {
        msg.buf[1] = J1939Encode_temp8bit(engineIntakeManifold3TemperatureC);
    }
    if (isSpnEnabled(config, 1133)) {
        msg.buf[2] = J1939Encode_temp8bit(engineIntakeManifold4TemperatureC);
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

    if (isSpnEnabled(config, 1127)) {
        uint16_t boost1 = J1939Encode_boost16bit(engineTurbocharger1BoostPressurekPa);
        msg.buf[0] = J1939Encode_lowByte(boost1);
        msg.buf[1] = J1939Encode_highByte(boost1);
    }
    if (isSpnEnabled(config, 1128)) {
        uint16_t boost2 = J1939Encode_boost16bit(engineTurbocharger2BoostPressurekPa);
        msg.buf[2] = J1939Encode_lowByte(boost2);
        msg.buf[3] = J1939Encode_highByte(boost2);
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

    if (isSpnEnabled(config, 110)) {
        msg.buf[0] = J1939Encode_temp8bit(engineCoolantTemperatureC);
    }
    if (isSpnEnabled(config, 174)) {
        msg.buf[1] = J1939Encode_temp8bit(engineFuelTemperatureC);
    }
    if (isSpnEnabled(config, 175)) {
        uint16_t oilTemp = J1939Encode_temp16bit(engineOilTemperatureC);
        msg.buf[2] = J1939Encode_lowByte(oilTemp);
        msg.buf[3] = J1939Encode_highByte(oilTemp);
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
        msg.buf[0] = J1939Encode_pressure4kPa(engineFuelDeliveryPressurekPa);
    }
    if (isSpnEnabled(config, 100)) {
        msg.buf[3] = J1939Encode_pressure4kPa(engineOilPressurekPa);
    }
    if (isSpnEnabled(config, 109)) {
        msg.buf[6] = J1939Encode_pressure2kPa(engineCoolantPressurekPa);
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

    if (isSpnEnabled(config, 108)) {
        msg.buf[0] = J1939Encode_barometric(barometricPressurekPa);
    }
    if (isSpnEnabled(config, 171)) {
        uint16_t ambientTemp = J1939Encode_temp16bit(ambientTemperatureC);
        msg.buf[3] = J1939Encode_lowByte(ambientTemp);
        msg.buf[4] = J1939Encode_highByte(ambientTemp);
    }
    if (isSpnEnabled(config, 172)) {
        msg.buf[5] = J1939Encode_temp8bit(airInletTemperatureC);
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

    if (isSpnEnabled(config, 102)) {
        msg.buf[1] = J1939Encode_pressure2kPa(boostPressurekPa);
    }
    if (isSpnEnabled(config, 105)) {
        msg.buf[2] = J1939Encode_temp8bit(airInletTemperatureC);
    }
    if (isSpnEnabled(config, 106)) {
        msg.buf[3] = J1939Encode_pressure2kPa(airInletPressurekPa);
    }
    if (isSpnEnabled(config, 173)) {
        uint16_t egtTemp = J1939Encode_temp16bit(egtTemperatureC);
        msg.buf[5] = J1939Encode_lowByte(egtTemp);
        msg.buf[6] = J1939Encode_highByte(egtTemp);
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
    // Use C-Next decoder for byte parsing
    uint16_t spn = J1939Decode_getSpn(data);
    bool enable = J1939Decode_getEnable(data);
    uint8_t input = J1939Decode_getInput(data);

    TCommandResult result = CommandHandler::enableSpn(spn, enable, input);
    sendConfigResponse(1, result.errorCode, result.data, result.dataLen);
}

void J1939Bus::handleJ1939SetNtcParam(const uint8_t *data) {
    // Use C-Next decoder for byte parsing
    uint8_t input = J1939Decode_getNtcInput(data);
    uint8_t param = J1939Decode_getNtcParam(data);

    // Reconstruct float from bytes using C-Next module (little endian)
    float value = FloatBytes_fromBytesLE(data[3], data[4], data[5], data[6]);

    TCommandResult result = CommandHandler::setNtcParam(input, param, value);
    sendConfigResponse(2, result.errorCode, result.data, result.dataLen);
}

void J1939Bus::handleJ1939SetPressureRange(const uint8_t *data) {
    // Use C-Next decoder for byte parsing
    uint8_t input = J1939Decode_getPressureInput(data);
    uint16_t maxPsi = J1939Decode_getMaxPressure(data);

    TCommandResult result = CommandHandler::setPressureRange(input, maxPsi);
    sendConfigResponse(3, result.errorCode, result.data, result.dataLen);
}

void J1939Bus::handleJ1939SetTcType(const uint8_t *data) {
    // Use C-Next decoder for byte parsing
    uint8_t type = J1939Decode_getTcType(data);

    TCommandResult result = CommandHandler::setTcType(type);
    sendConfigResponse(4, result.errorCode, result.data, result.dataLen);
}

void J1939Bus::handleJ1939Query(const uint8_t *data) {
    // Use C-Next decoder for byte parsing
    uint8_t queryType = J1939Decode_getQueryType(data);
    uint8_t subQuery = J1939Decode_getSubQuery(data);

    TCommandResult result;
    switch (queryType) {
        case 0:
            result = CommandHandler::querySpnCounts();
            break;
        case 1:  // Temp SPN assignments (subQuery 0-2)
            result = CommandHandler::queryTempSpns(subQuery);
            break;
        case 2:  // Pressure SPN assignments (subQuery 0-2)
            result = CommandHandler::queryPresSpns(subQuery);
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
    // Use C-Next decoder for byte parsing
    uint8_t input = J1939Decode_getPresetInput(data);
    uint8_t preset = J1939Decode_getPresetId(data);

    TCommandResult result = CommandHandler::applyNtcPreset(input, preset);
    sendConfigResponse(8, result.errorCode, result.data, result.dataLen);
}

void J1939Bus::handleJ1939PressurePreset(const uint8_t *data) {
    // Use C-Next decoder for byte parsing
    uint8_t input = J1939Decode_getPresetInput(data);
    uint8_t preset = J1939Decode_getPresetId(data);

    TCommandResult result = CommandHandler::applyPressurePreset(input, preset);
    sendConfigResponse(9, result.errorCode, result.data, result.dataLen);
}

#endif
