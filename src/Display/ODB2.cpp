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
double egtTemp;
double coolantTemp;
double coolantPressure;
double cacTemp;
double cacPressure;
double intakeTemp;
double intakePressure;
double fuelTemp;
double fuelPressure;
double engineBayTemp;
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
      case 5:
        waterTempResponse.buf[3] = OBD2::appData->coolantTemperatureC + 40;
        Can1.write(waterTempResponse);
        break;
      case 15:
        ambientTemperatureResponse.buf[3] =
            OBD2::appData->ambientTemperatureC + 40;
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
            OBD2::appData->absoluteBarometricpressurehPa / 10;
        Can1.write(odb2AbsoluteBarometricPressureResponse);
        break;
      case 64:
        Serial.println("ODB2 pids 65-96 requested");
        Can1.write(supportedPidsSixtyFiveToNinetySixResponse);
        break;
      case 92:
        oilTempResponse.buf[3] = OBD2::appData->oilTemperatureC + 40;
        Can1.write(oilTempResponse);
        break;
      case 96:
        Serial.println("ODB2 pids 97-128 requested");
        Can1.write(supportedPidsNinetySevenToOneHundredTwentyEightResponse);
        break;
      case 112:
        boost = OBD2::appData->boostPressurekPa;
        boost *= 1.003;
        boost *= 220;
        boost -= 10;
        boostResponse.buf[6] = boost < 0 ? 0 : highByte((long)boost);
        boostResponse.buf[7] = boost < 0 ? 0 : lowByte((long)boost);
        Can1.write(boostResponse);
        break;
      case 128:
        Serial.println("ODB2 pids 129-160 requested");
        Can1.write(
            supportedPidsOneHundredTwentyNineToOneHundredOneHundredFiftyResponse);
        break;
      case 243:
        engineBayTemp = OBD2::appData->engineBayTemperatureC;
        engineBayTemp *= 100;
        engineBayTemp += 32767;

        osEngineBayResponse.buf[3] = highByte((long)engineBayTemp);
        osEngineBayResponse.buf[4] = lowByte((long)engineBayTemp);

        Can1.write(osEngineBayResponse);
        break; 
      case 244:
        fuelTemp = OBD2::appData->intakeTemperatureC;
        fuelTemp *= 100;
        fuelTemp += 32767;

        fuelPressure = OBD2::appData->intakePressurekPa;
        fuelPressure *= 10;

        osFuelResponse.buf[3] = highByte((long)fuelTemp);
        osFuelResponse.buf[4] = lowByte((long)fuelTemp);
        osFuelResponse.buf[5] = highByte((long)fuelPressure);
        osFuelResponse.buf[6] = lowByte((long)fuelPressure);

        Can1.write(osFuelResponse);
        break; 
      case 245:
        intakeTemp = OBD2::appData->intakeTemperatureC;
        intakeTemp *= 100;
        intakeTemp += 32767;

        intakePressure = OBD2::appData->intakePressurekPa;
        intakePressure *= 10;

        osIntakeResponse.buf[3] = highByte((long)intakeTemp);
        osIntakeResponse.buf[4] = lowByte((long)intakeTemp);
        osIntakeResponse.buf[5] = highByte((long)intakePressure);
        osIntakeResponse.buf[6] = lowByte((long)intakePressure);

        Can1.write(osIntakeResponse);
        break; 
      case 246:
        cacTemp = OBD2::appData->cacTemperatureC;
        cacTemp *= 100;
        cacTemp += 32767;

        cacPressure = OBD2::appData->cacPressurekPa;
        cacPressure *= 10;

        osCacResponse.buf[3] = highByte((long)cacTemp);
        osCacResponse.buf[4] = lowByte((long)cacTemp);
        osCacResponse.buf[5] = highByte((long)cacPressure);
        osCacResponse.buf[6] = lowByte((long)cacPressure);

        Can1.write(osCacResponse);
        break;  
      case 247:
        coolantTemp = OBD2::appData->coolantTemperatureC;
        coolantTemp *= 100;
        coolantTemp += 32767;

        coolantPressure = OBD2::appData->coolantPressurekPa;
        coolantPressure *= 10;

        osCoolantResponse.buf[3] = highByte((long)coolantTemp);
        osCoolantResponse.buf[4] = lowByte((long)coolantTemp);
        osCoolantResponse.buf[5] = highByte((long)coolantPressure);
        osCoolantResponse.buf[6] = lowByte((long)coolantPressure);

        Can1.write(osCoolantResponse);
        break;
      case 248:
        egtTemp = OBD2::appData->egtTemperatureC;
        egtTemp *= 10;
        egtTemp += 32767;

        osEgtResponse.buf[3] = highByte((long)egtTemp);
        osEgtResponse.buf[4] = lowByte((long)egtTemp);

        Can1.write(osEgtResponse);
        break;
      case 249:
        ambientHumidity = OBD2::appData->humidity;
        ambientHumidity *= 655.35;

        osAmbientHumidityResponse.buf[3] = highByte((long)ambientHumidity);
        osAmbientHumidityResponse.buf[4] = lowByte((long)ambientHumidity);

        Can1.write(osAmbientHumidityResponse);
        break;
      case 250:
        ambientAirTemp = OBD2::appData->ambientTemperatureC;
        ambientAirTemp *= 100;
        ambientAirTemp += 32767;

        ambientAirPressure = OBD2::appData->absoluteBarometricpressurehPa;
        ambientAirPressure *= 10;

        osAmbientConditionsResponse.buf[3] = highByte((long)ambientAirTemp);
        osAmbientConditionsResponse.buf[4] = lowByte((long)ambientAirTemp);
        osAmbientConditionsResponse.buf[5] = highByte((long)ambientAirPressure);
        osAmbientConditionsResponse.buf[6] = lowByte((long)ambientAirPressure);

        Can1.write(osAmbientConditionsResponse);
        break;
      case 253:
        oilTemp = OBD2::appData->oilTemperatureC;
        oilTemp *= 100;
        oilTemp += 32767;

        oilPres = OBD2::appData->oilPressurekPa;
        oilPres *= 10;

        osOilResponse.buf[3] = highByte((long)oilTemp);
        osOilResponse.buf[4] = lowByte((long)oilTemp);
        osOilResponse.buf[5] = highByte((long)oilPres);
        osOilResponse.buf[6] = lowByte((long)oilPres);

        Can1.write(osOilResponse);
        break;
      case 254:
        manifoldTemp = OBD2::appData->boostTemperatureC;
        manifoldTemp *= 100;
        manifoldTemp += 32767;

        manifoldPres = OBD2::appData->boostPressurekPa;
        manifoldPres *= 10;

        osManifoldResponse.buf[3] = highByte((long)manifoldTemp);
        osManifoldResponse.buf[4] = lowByte((long)manifoldTemp);
        osManifoldResponse.buf[5] = highByte((long)manifoldPres);
        osManifoldResponse.buf[6] = lowByte((long)manifoldPres);

        Can1.write(osManifoldResponse);
        break;
      case 255:
        transTemp = OBD2::appData->transmissionTemperatureC;
        transTemp *= 100;
        transTemp += 32767;

        transPres = OBD2::appData->transmissionPressurekPa;
        transPres *= 10;

        osTransmissionResponse.buf[3] = highByte((long)transTemp);
        osTransmissionResponse.buf[4] = lowByte((long)transTemp);
        osTransmissionResponse.buf[5] = highByte((long)transPres);
        osTransmissionResponse.buf[6] = lowByte((long)transPres);

        Can1.write(osTransmissionResponse);
        break;
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