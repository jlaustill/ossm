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

void J1939Bus::sendPgn65269(float ambientTemperatureC, float airInletTemperatureC, float barometricPressureHpa) {
  J1939 message = J1939();
  message.setPgn(65269);
  message.setPriority(6);
  message.setSourceAddress(149);

  CAN_message_t msg;
  msg.id = message.canId;
  msg.buf[0] = 255;
  msg.buf[0] = 255;
  msg.buf[0] = 255;
  msg.buf[0] = 255;
  msg.buf[0] = 255;
  msg.buf[0] = 255;
  msg.buf[0] = 255;
  msg.buf[0] = 255;

  Can1.write(msg);
}

#endif