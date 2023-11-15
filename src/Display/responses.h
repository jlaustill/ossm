#include <FlexCAN_T4.h>

uint8_t emptyBytePad = 0xCC;
uint32_t ecmResponseId = 2025;
uint8_t currentDataService = 0x41;

CAN_message_t supportedPidsOneToThirtyTwoResponse = {
    .id = ecmResponseId,
    .buf = {6, currentDataService, 0, B10011000, B00011100, B10000000,
            B00010011, emptyBytePad}};
CAN_message_t supportedPidsThirtyThreeToSixtyFourResponse = {
    .id = ecmResponseId,
    .buf = {6, currentDataService, 32, B00000000, B00000000, B00000000,
            B00000001, emptyBytePad}};
CAN_message_t supportedPidsSixtyFiveToNinetySixResponse = {
    .id = ecmResponseId,
    .buf = {6, currentDataService, 64, B00000000, B00000000, B10000000,
            B00010001, emptyBytePad}};
CAN_message_t supportedPidsNinetySevenToOneHundredTwentyEightResponse = {
    .id = ecmResponseId,
    .buf = {6, currentDataService, 96, B00000000, B00000001, B00000000,
            B00000001, emptyBytePad}};
CAN_message_t
    supportedPidsOneHundredTwentyNineToOneHundredOneHundredFiftyResponse = {
        .id = ecmResponseId,
        .buf = {6, currentDataService, 96, B00000000, B00000000, B00000000,
                B00000001, emptyBytePad}};

CAN_message_t waterTempResponse = {
    .id = ecmResponseId,
    .buf = {4, currentDataService, 5, 0, emptyBytePad, emptyBytePad,
            emptyBytePad, emptyBytePad}};

CAN_message_t ambientTemperatureResponse = {
    .id = ecmResponseId,
    .buf = {4, currentDataService, 15, 0, emptyBytePad, emptyBytePad,
            emptyBytePad, emptyBytePad}};

CAN_message_t obdStandardResponse = {
    .id = ecmResponseId,
    .buf = {4, currentDataService, 28, 1, emptyBytePad, emptyBytePad,
            emptyBytePad, emptyBytePad}};

CAN_message_t odb2AbsoluteBarometricPressureResponse = {
    .id = ecmResponseId,
    .buf = {4, currentDataService, 51, 0, emptyBytePad, emptyBytePad,
            emptyBytePad, emptyBytePad}};

CAN_message_t oilTempResponse = {
    .id = ecmResponseId,
    .buf = {4, currentDataService, 92, 0, emptyBytePad, emptyBytePad,
            emptyBytePad, emptyBytePad}};

CAN_message_t boostResponse = {
    .id = ecmResponseId, .buf = {7, currentDataService, 112, 0, 0, 0, 0, 0}};

CAN_message_t osEngineBayResponse = {
    .id = ecmResponseId, .buf = {7, currentDataService, 243, 0, 0, 0, 0, 0}};

CAN_message_t osFuelResponse = {
    .id = ecmResponseId, .buf = {7, currentDataService, 244, 0, 0, 0, 0, 0}};

CAN_message_t osIntakeResponse = {
    .id = ecmResponseId, .buf = {7, currentDataService, 245, 0, 0, 0, 0, 0}};

CAN_message_t osCacResponse = {
    .id = ecmResponseId, .buf = {7, currentDataService, 246, 0, 0, 0, 0, 0}};

CAN_message_t osCoolantResponse = {
    .id = ecmResponseId, .buf = {7, currentDataService, 247, 0, 0, 0, 0, 0}};

CAN_message_t osEgtResponse = {
    .id = ecmResponseId, .buf = {5, currentDataService, 248, 0, 0, 0, 0, 0}};

CAN_message_t osAmbientHumidityResponse = {
    .id = ecmResponseId,
    .buf = {5, currentDataService, 249, 0, emptyBytePad, emptyBytePad,
            emptyBytePad, emptyBytePad}};

CAN_message_t osAmbientConditionsResponse = {
    .id = ecmResponseId,
    .buf = {7, currentDataService, 250, 0, emptyBytePad, emptyBytePad,
            emptyBytePad, emptyBytePad}};

CAN_message_t osOilResponse = {
    .id = ecmResponseId, .buf = {7, currentDataService, 253, 0, 0, 0, 0, 0}};

CAN_message_t osManifoldResponse = {
    .id = ecmResponseId, .buf = {7, currentDataService, 254, 0, 0, 0, 0, 0}};

CAN_message_t osTransmissionResponse = {
    .id = ecmResponseId, .buf = {7, currentDataService, 255, 0, 0, 0, 0, 0}};