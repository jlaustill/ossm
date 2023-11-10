//
// Created by jlaustill on 7/18/21.
// Recreated by jlaustill on 9/22/23.
//

#include "ODB2.h"

#include <AppData.h>
#include <FlexCAN_T4.h>

#include "./responses.h"

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> Can1;


unsigned long runtime;
double boost;
double oilPres;
double oilTemp;
double manifoldTemp;
double manifoldPres;
double transTemp;
double transPres;
long odometer;
long tripA;
long tripB;

double ambientAirTemp;
double ambientAirPressure;
double ambientHumidity;

byte service;
bool requestIsFromScannerRange = false;

#define thirdByte(w) ((uint8_t)((w) >> 16))
#define fourthByte(w) ((uint8_t)((w) >> 24))

AppData *OBD2::appData;

void OBD2::initialize(AppData *currentData) {
  Serial.println("ODB2 initializing");

  appData = currentData;

  Can1.begin();
  Can1.setBaudRate(500 * 1000);
  Can1.setMaxMB(16);
  Can1.enableFIFO();
  Can1.enableFIFOInterrupt();
  Can1.onReceive(sendData);
  Can1.mailboxStatus();
}

void OBD2::sendData(const CAN_message_t &msg) {
  service = msg.buf[1];
  requestIsFromScannerRange = msg.id >= 2015 && msg.id <= 2023;

  if (requestIsFromScannerRange && service == 1) {
    switch (msg.buf[2]) {
      case 0:
        Serial.println("ODB2 pids 1-32 requested from " + (String)msg.id);
        Can1.write(supportedPidsOneToThirtyTwoResponse);
        break;
      //   case 5:
      //     waterTempResponse.buf[3] = OBD2::appData->coolantTemp;
      //     Can1.write(waterTempResponse);
      //     break;
      case 15:
        ambientTemperatureResponse.buf[3] =
            OBD2::appData->ambientTemperature + 40;
        Can1.write(ambientTemperatureResponse);
        break;
        break;
      case 28:
        Serial.println("ODB2 standard requested");
        Can1.write(obdStandardResponse);
        break;
      case 32:
        Serial.println("ODB2 pids 33-64 requested");
        Can1.write(supportedPidsThirtyThreeToSixtyFourResponse);
        break;
      case 51:
        odb2AbsoluteBarometricPressureResponse.buf[3] =
            OBD2::appData->absoluteBarometricpressure / 10;
        Can1.write(odb2AbsoluteBarometricPressureResponse);
        break;
      case 64:
        Serial.println("ODB2 pids 65-96 requested");
        Can1.write(supportedPidsSixtyFiveToNinetySixResponse);
        break;
        //   case 92:
        //     oilTempResponse.buf[3] = OBD2::appData->oilTempC + 40;
        //     Can1.write(oilTempResponse);
        //     break;
      case 96:
        Serial.println("ODB2 pids 97-128 requested");
        Can1.write(supportedPidsNinetySevenToOneHundredTwentyEightResponse);
        break;
        //   case 112:
        //     boost = OBD2::appData->boost;
        //     boost *= 1.003;
        //     boost *= 220;
        //     boost -= 10;
        //     boostResponse.buf[6] = boost < 0 ? 0 : highByte((long)boost);
        //     boostResponse.buf[7] = boost < 0 ? 0 : lowByte((long)boost);
        //     Can1.write(boostResponse);
        //     break;
      case 128:
        Serial.println("ODB2 pids 129-160 requested");
        Can1.write(
            supportedPidsOneHundredTwentyNineToOneHundredOneHundredFiftyResponse);
        break;
      case 249:
        ambientHumidity = OBD2::appData->humidity;
        ambientHumidity *= 655.35;
        osAmbientHumidityResponse.buf[3] = highByte((long)ambientHumidity);
        osAmbientHumidityResponse.buf[4] = lowByte((long)ambientHumidity);
        Can1.write(osAmbientHumidityResponse);
        break;
      case 250:
        ambientAirTemp = OBD2::appData->ambientTemperature;
        ambientAirTemp *= 100;
        ambientAirTemp += 32767;
        ambientAirPressure = OBD2::appData->absoluteBarometricpressure;
        ambientAirPressure *= 10;
        ambientAirPressure += 32767;
        osAmbientConditionsResponse.buf[3] = highByte((long)ambientAirPressure);
        osAmbientConditionsResponse.buf[4] = lowByte((long)ambientAirPressure);
        osAmbientConditionsResponse.buf[5] = highByte((long)ambientAirTemp);
        osAmbientConditionsResponse.buf[6] = lowByte((long)ambientAirTemp);

        Can1.write(osAmbientConditionsResponse);
        // Serial.println("ODB2 ambient barometric pressure requested, sending "
        // + (String)ambientAirPressure + " high byte: " +
        // (String)osAmbientConditionsResponse.buf[3] + " low byte: " +
        // (String)osAmbientConditionsResponse.buf[4]);
        break;
        //   case 253:
        //     oilPres = OBD2::appData->oilPressureInPsi;
        //     oilPres *= 6.895;
        //     oilPres *= 10;
        //     oilResponse.buf[3] = highByte((long)oilPres);
        //     oilResponse.buf[4] = lowByte((long)oilPres);
        //     oilTemp = OBD2::appData->oilTempC;
        //     oilTemp *= 100;
        //     oilTemp += 32767;
        //     oilResponse.buf[5] = highByte((long)oilTemp);
        //     oilResponse.buf[6] = lowByte((long)oilTemp);
        //     Can1.write(oilResponse);
        //     break;
        //   case 254:
        //     // Serial.println("ODB2 254 requested");
        //     manifoldPres = OBD2::appData->boost;
        //     manifoldTemp = OBD2::appData->manifoldTempC;
        //     manifoldTemp *= 100;
        //     manifoldTemp += 32767;
        //     manifoldResponse.buf[3] = highByte((long)manifoldTemp);
        //     manifoldResponse.buf[4] = lowByte((long)manifoldTemp);
        //     manifoldPres *= 6.895;
        //     manifoldPres *= 10;
        //     manifoldResponse.buf[5] = highByte((long)manifoldPres);
        //     manifoldResponse.buf[6] = lowByte((long)manifoldPres);
        //     Can1.write(manifoldResponse);
        //     break;
        //   case 255:
        //     // Serial.println("ODB2 255 requested");
        //     transPres = OBD2::appData->transmissionPressure;
        //     transTemp = OBD2::appData->transmissionTempC;
        //     transPres *= 6.895;
        //     transPres *= 10;
        //     transResponse.buf[5] = highByte((long)transPres);
        //     transResponse.buf[6] = lowByte((long)transPres);
        //     transTemp *= 100;
        //     transTemp -= 32767;
        //     transResponse.buf[3] = highByte((long)transTemp);
        //     transResponse.buf[4] = lowByte((long)transTemp);
        //     Can1.write(transResponse);
        //     break;
      default:
        // Serial.println("ODB2 unknown request ID: " + (String)msg.id + " " +
        //                (String)msg.buf[0] + " " + (String)msg.buf[1] + " " +
        //                (String)msg.buf[2] + " " + (String)msg.buf[3] + " " +
        //                (String)msg.buf[4] + " " + (String)msg.buf[5] + " " +
        //                (String)msg.buf[6] + " " + (String)msg.buf[7] + " ");
        break;
    }
  } else {
    // Serial.println("ODB2 unknown request ID: " + (String)msg.id + " " +
    //                (String)msg.buf[0] + " " + (String)msg.buf[1] + " " +
    //                (String)msg.buf[2] + " " + (String)msg.buf[3] + " " +
    //                (String)msg.buf[4] + " " + (String)msg.buf[5] + " " +
    //                (String)msg.buf[6] + " " + (String)msg.buf[7] + " ");
  }
}