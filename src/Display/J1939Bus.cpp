#ifndef OSSM_J1939Bus_CPP
#define OSSM_J1939Bus_CPP

#include "J1939Bus.h"

#include <AppData.h>
#include <FlexCAN_T4.h>

#include <SeaDash.hpp>

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> Can1;

AppData *J1939Bus::appData;

void J1939Bus::sniffData(const CAN_message_t &msg) {
  J1939 message = J1939();
  message.setCanId(msg.id);
  message.setData(msg.buf);

  if (message.pgn == 59904) {
    uint32_t requestedPgn = message.data[0];
    requestedPgn |= message.data[1] << 8;
    requestedPgn |= message.data[2] << 16;

    if (requestedPgn == 65164) {
      sendPgn65164();
    }
  }
}

void J1939Bus::initialize(AppData *currentData) {
  Serial.println("J1939 Bus initializing");

  appData = currentData;

  Can1.begin();
  Can1.setBaudRate(250 * 1000);
  Can1.setMaxMB(16);
  Can1.enableFIFO();
  Can1.enableFIFOInterrupt();
  Can1.onReceive(sniffData);
  Can1.mailboxStatus();
}

void J1939Bus::sendPgn65129(float engineIntakeManifold1AirTemperatureC,
                            float engineCoolantTemperatureC) {
  J1939 message = J1939();
  message.setPgn(65129);
  message.setPriority(6);
  message.setSourceAddress(149);

  CAN_message_t msg;
  msg.id = message.canId;
  float intakeTempOffset = engineIntakeManifold1AirTemperatureC + 273.0;
  intakeTempOffset /= 0.03125;
  float coolantTempOffset = engineCoolantTemperatureC + 273.0;
  coolantTempOffset /= 0.03125;
  msg.buf[0] = highByte(static_cast<uint16_t>(intakeTempOffset));   // 1636
  msg.buf[1] = lowByte(static_cast<uint16_t>(intakeTempOffset));    // 1636
  msg.buf[2] = highByte(static_cast<uint16_t>(coolantTempOffset));  // 1637
  msg.buf[3] = lowByte(static_cast<uint16_t>(coolantTempOffset));   // 1637
  msg.buf[4] = 255;
  msg.buf[5] = 255;
  msg.buf[6] = 255;
  msg.buf[7] = 255;

  Can1.write(msg);
}

void J1939Bus::sendPgn65164() {
  J1939 message = J1939();
  message.setPgn(65189);
  message.setPriority(6);
  message.setSourceAddress(149);

  CAN_message_t msg;
  msg.id = message.canId;
  msg.buf[0] =
      static_cast<uint8_t>(appData->engineBayTemperatureC + 40);  // 441
  msg.buf[1] = 255;
  msg.buf[2] = 255;
  msg.buf[3] = 255;
  msg.buf[4] = 255;
  msg.buf[5] = 255;
  msg.buf[6] = static_cast<uint8_t>(appData->humidity / .4);  // 354
  msg.buf[7] = 255;

  Can1.write(msg);
}

void J1939Bus::sendPgn65189(float engineIntakeManifold2TemperatureC,
                            float engineIntakeManifold3TemperatureC,
                            float engineIntakeManifold4TemperatureC) {
  J1939 message = J1939();
  message.setPgn(65189);
  message.setPriority(6);
  message.setSourceAddress(149);

  CAN_message_t msg;
  msg.id = message.canId;
  msg.buf[0] =
      static_cast<uint8_t>(engineIntakeManifold2TemperatureC + 40);  // 1131
  msg.buf[1] =
      static_cast<uint8_t>(engineIntakeManifold3TemperatureC + 40);  // 1132
  msg.buf[2] =
      static_cast<uint8_t>(engineIntakeManifold4TemperatureC + 40);  // 1133
  msg.buf[3] = 255;
  msg.buf[4] = 255;
  msg.buf[5] = 255;
  msg.buf[6] = 255;
  msg.buf[7] = 255;

  Can1.write(msg);
}

void J1939Bus::sendPgn65190(float engineTurbocharger1BoostPressurekPa,
                            float engineTurbocharger2BoostPressurekPa) {
  J1939 message = J1939();
  message.setPgn(65190);
  message.setPriority(6);
  message.setSourceAddress(149);

  CAN_message_t msg;
  msg.id = message.canId;
  float boost1Offset = engineTurbocharger1BoostPressurekPa / 0.125;
  float boost2Offset = engineTurbocharger2BoostPressurekPa / 0.125;
  msg.buf[0] = highByte(static_cast<uint16_t>(boost1Offset));  // 1127
  msg.buf[1] = lowByte(static_cast<uint16_t>(boost1Offset));   // 1127
  msg.buf[2] = highByte(static_cast<uint16_t>(boost2Offset));  // 1128
  msg.buf[3] = lowByte(static_cast<uint16_t>(boost2Offset));   // 1128
  msg.buf[4] = 255;
  msg.buf[5] = 255;
  msg.buf[6] = 255;
  msg.buf[7] = 255;

  Can1.write(msg);
}

void J1939Bus::sendPgn65262(float engineCoolantTemperatureC,
                            float engineFuelTemperatureC,
                            float engineOilTemperatureC) {
  J1939 message = J1939();
  message.setPgn(65262);
  message.setPriority(6);
  message.setSourceAddress(149);

  CAN_message_t msg;
  msg.id = message.canId;
  float oilTempOffset = engineOilTemperatureC + 273.0;
  oilTempOffset /= 0.03125;
  msg.buf[0] = static_cast<uint8_t>(engineCoolantTemperatureC + 40);  // 110
  msg.buf[1] = static_cast<uint8_t>(engineFuelTemperatureC + 40);     // 174
  msg.buf[2] = highByte(static_cast<uint16_t>(oilTempOffset));        // 175
  msg.buf[3] = lowByte(static_cast<uint16_t>(oilTempOffset));         // 175
  msg.buf[4] = 255;
  msg.buf[5] = 255;
  msg.buf[6] = 255;
  msg.buf[7] = 255;

  Can1.write(msg);
}

void J1939Bus::sendPgn65263(float engineFuelDeliveryPressurekPa,
                            float engineOilPressurekPa,
                            float engineCoolantPressurekPa) {
  J1939 message = J1939();
  message.setPgn(65263);
  message.setPriority(6);
  message.setSourceAddress(149);

  CAN_message_t msg;
  msg.id = message.canId;
  msg.buf[0] = static_cast<uint8_t>(engineFuelDeliveryPressurekPa / 4);  // 94
  msg.buf[1] = 255;
  msg.buf[2] = 255;
  msg.buf[3] = static_cast<uint8_t>(engineOilPressurekPa / 4);  // 100
  msg.buf[4] = 255;
  msg.buf[5] = 255;
  msg.buf[6] = static_cast<uint8_t>(engineCoolantPressurekPa / 2);  // 109
  msg.buf[7] = 255;

  Can1.write(msg);
}

void J1939Bus::sendPgn65269(float ambientTemperatureC,
                            float airInletTemperatureC,
                            float barometricPressurekPa) {
  J1939 message = J1939();
  message.setPgn(65269);
  message.setPriority(6);
  message.setSourceAddress(149);

  CAN_message_t msg;
  msg.id = message.canId;
  float ambientAirTempOffset = ambientTemperatureC + 273.0;
  ambientAirTempOffset /= 0.03125;
  msg.buf[0] = static_cast<uint8_t>(barometricPressurekPa * 2);  // 108
  msg.buf[1] = 255;
  msg.buf[2] = 255;
  msg.buf[3] = highByte(static_cast<uint16_t>(ambientAirTempOffset));  // 171
  msg.buf[4] = lowByte(static_cast<uint16_t>(ambientAirTempOffset));   // 171
  msg.buf[5] = static_cast<uint8_t>(airInletTemperatureC + 40);        // 172
  msg.buf[6] = 255;
  msg.buf[7] = 255;

  Can1.write(msg);
}

void J1939Bus::sendPgn65270(float airInletPressurekPa,
                            float airInletTemperatureC, float egtTemperatureC,
                            float boostPressurekPa) {
  J1939 message = J1939();
  message.setPgn(65270);
  message.setPriority(6);
  message.setSourceAddress(149);

  CAN_message_t msg;
  msg.id = message.canId;
  float egtOffset = egtTemperatureC + 273.0;
  egtOffset /= 0.03125;
  msg.buf[0] = 255;
  msg.buf[1] = static_cast<uint8_t>(boostPressurekPa / 2);       // 102
  msg.buf[2] = static_cast<uint8_t>(airInletTemperatureC + 40);  // 105
  msg.buf[3] = static_cast<uint8_t>(airInletPressurekPa / 2);    // 106
  msg.buf[4] = 255;
  msg.buf[5] = highByte(static_cast<uint16_t>(egtOffset));  // 173
  msg.buf[6] = lowByte(static_cast<uint16_t>(egtOffset));   // 173
  msg.buf[7] = 255;

  Can1.write(msg);
}

#endif