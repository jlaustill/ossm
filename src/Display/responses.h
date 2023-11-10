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
CAN_message_t supportedPidsOneHundredTwentyNineToOneHundredOneHundredFiftyResponse = {
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



CAN_message_t osAmbientHumidityResponse = {
    .id = ecmResponseId,
    .buf = {5, currentDataService, 249, 0, emptyBytePad, emptyBytePad,
            emptyBytePad, emptyBytePad}};


CAN_message_t osAmbientConditionsResponse = {
    .id = ecmResponseId,
    .buf = {7, currentDataService, 250, 0, emptyBytePad, emptyBytePad,
            emptyBytePad, emptyBytePad}};

CAN_message_t oilResponse = {
    .id = ecmResponseId, .buf = {7, currentDataService, 253, 0, 0, 0, 0, 0}};

CAN_message_t manifoldResponse = {
    .id = ecmResponseId, .buf = {7, currentDataService, 254, 0, 0, 0, 0, 0}};

CAN_message_t transResponse = {
    .id = ecmResponseId, .buf = {7, currentDataService, 255, 0, 0, 0, 0, 0}};