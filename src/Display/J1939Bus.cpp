#ifndef OSSM_J1939Bus_CPP
#define OSSM_J1939Bus_CPP

#include "J1939Bus.h"

#include <FlexCAN_T4.h>

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> Can1;

void J1939Bus::initialize() {
  Serial.println("J1939 Bus initializing");

  Can1.begin();
  Can1.setBaudRate(250 * 1000);
  Can1.setMaxMB(16);
  Can1.enableFIFO();
  Can1.enableFIFOInterrupt();
//   Can1.onReceive(BusSniff);
  Can1.mailboxStatus();
}

void J1939Bus::sendPgn65129(float engineIntakeManifold1AirTemperatureC, float engineCoolantTemperatureC) {
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
  msg.buf[0] = highByte(static_cast<uint16_t>(intakeTempOffset)); // 1636
  msg.buf[1] = lowByte(static_cast<uint16_t>(intakeTempOffset)); // 1636
  msg.buf[2] = highByte(static_cast<uint16_t>(coolantTempOffset)); // 1637
  msg.buf[3] = lowByte(static_cast<uint16_t>(coolantTempOffset)); // 1637
  msg.buf[4] = 255;
  msg.buf[5] = 255;
  msg.buf[6] = 255;
  msg.buf[7] = 255;

  Can1.write(msg);
}

void J1939Bus::sendPgn65262(float engineCoolantTemperatureC, float engineFuelTemperatureC, float engineOilTemperatureC) {
  J1939 message = J1939();
  message.setPgn(65262);
  message.setPriority(6);
  message.setSourceAddress(149);

  CAN_message_t msg;
  msg.id = message.canId;
  float oilTempOffset = engineOilTemperatureC + 273.0;
  oilTempOffset /= 0.03125;
  msg.buf[0] = static_cast<uint8_t>(engineCoolantTemperatureC + 40); // 110
  msg.buf[1] = static_cast<uint8_t>(engineFuelTemperatureC + 40); // 174
  msg.buf[2] = highByte(static_cast<uint16_t>(oilTempOffset)); // 175
  msg.buf[3] = lowByte(static_cast<uint16_t>(oilTempOffset)); // 175
  msg.buf[4] = 255;
  msg.buf[5] = 255;
  msg.buf[6] = 255;
  msg.buf[7] = 255;

  Can1.write(msg);
}

void J1939Bus::sendPgn65263(float engineFuelDeliveryPressurekPa, float engineOilPressurekPa, float engineCoolantPressurekPa){
  J1939 message = J1939();
  message.setPgn(65263);
  message.setPriority(6);
  message.setSourceAddress(149);

  CAN_message_t msg;
  msg.id = message.canId;
  msg.buf[0] = static_cast<uint8_t>(engineFuelDeliveryPressurekPa / 4); // 94
  msg.buf[1] = 255;
  msg.buf[2] = 255;
  msg.buf[3] = static_cast<uint8_t>(engineOilPressurekPa / 4); // 100
  msg.buf[4] = 255;
  msg.buf[5] = 255;
  msg.buf[6] = static_cast<uint8_t>(engineCoolantPressurekPa / 2); // 109
  msg.buf[7] = 255;

  Can1.write(msg);
}

void J1939Bus::sendPgn65269(float ambientTemperatureC, float airInletTemperatureC, float barometricPressurekPa) {
  J1939 message = J1939();
  message.setPgn(65269);
  message.setPriority(6);
  message.setSourceAddress(149);

  CAN_message_t msg;
  msg.id = message.canId;
  float ambientAirTempOffset = ambientTemperatureC + 273.0;
  ambientAirTempOffset /= 0.03125;
  msg.buf[0] = static_cast<uint8_t>(barometricPressurekPa * 2); // 108
  msg.buf[1] = 255; 
  msg.buf[2] = 255;
  msg.buf[3] = highByte(static_cast<uint16_t>(ambientAirTempOffset)); // 171
  msg.buf[4] = lowByte(static_cast<uint16_t>(ambientAirTempOffset)); // 171
  msg.buf[5] = static_cast<uint8_t>(airInletTemperatureC + 40); // 172
  msg.buf[6] = 255;
  msg.buf[7] = 255;

  Can1.write(msg);
}

void J1939Bus::sendPgn65270(float airInletPressurekPa, float airInletTemperatureC, float egtTemperatureC, float boostPressurekPa) {
  J1939 message = J1939();
  message.setPgn(65270);
  message.setPriority(6);
  message.setSourceAddress(149);

  CAN_message_t msg;
  msg.id = message.canId;
  float egtOffset = egtTemperatureC + 273.0;
  egtOffset /= 0.03125;
  msg.buf[0] = 255;
  msg.buf[1] = static_cast<uint8_t>(boostPressurekPa / 2); // 102
  msg.buf[2] = static_cast<uint8_t>(airInletTemperatureC + 40); // 105
  msg.buf[3] = static_cast<uint8_t>(airInletPressurekPa / 2); // 106
  msg.buf[4] = 255; 
  msg.buf[5] = highByte(static_cast<uint16_t>(egtOffset)); // 173
  msg.buf[6] = lowByte(static_cast<uint16_t>(egtOffset)); // 173
  msg.buf[7] = 255;

  Can1.write(msg);

}

#endif