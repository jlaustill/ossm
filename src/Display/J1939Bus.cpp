#ifndef OSSM_J1939Bus_CPP
#define OSSM_J1939Bus_CPP

#include "J1939Bus.h"
#include "configuration.h"

#include <AppData.h>
#include <FlexCAN_T4.h>

#include <SeaDash.hpp>

FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> CanCumminsBus;
FlexCAN_T4<CAN3, RX_SIZE_256, TX_SIZE_16> CanPrivate;

AppData *J1939Bus::appData;
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

void UpdateMaxTiming(float timing) {
  if (rpms < 1200) {
      maxTiming = timing;
  } else {
      maxTiming = 30.0f;
  }
}

void J1939Bus::sniffDataCumminsBus(const CAN_message_t &msg) {
  // Serial.println("Message sniffed");
  J1939Message message = J1939Message();
  message.setCanId(msg.id);
  message.setData(msg.buf);

  switch (message.canId) {
    case 256:
      // update timing and fuel here
      if (warmedUp) {
            rpms = ((uint16_t)message.data[7] << 8) + (uint16_t)message.data[6];
            rpms /= 4;
            float timing = (float)((uint16_t)message.data[5] << 8) + (uint16_t)message.data[4];
            timing /= 128.0f;
            float fuel = (float)((uint16_t)message.data[1] << 8) + (uint16_t)message.data[0];
            fuel /= 40.95f;
            CAN_message_t copyMsg = msg;
            UpdateMaxTiming(timing);
            maxOfThrottleAndLoad = max(throttlePercent, load);
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
            // write the modified message to the bus
            CanCumminsBus.write(copyMsg);
        }
  }

  if (message.pgn == 65262) {
    int16_t waterTemp = (int16_t)message.data[0] - 40;  // SPN 110
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
      CanPrivate.write(msg);
    }
  }
  else {
    CanPrivate.write(msg);
  }
}

void J1939Bus::sniffDataPrivate(const CAN_message_t &msg) {
  // Serial.println("Message sniffed");
  J1939Message message = J1939Message();
  message.setCanId(msg.id);
  message.setData(msg.buf);

  if (message.pgn == 59904) {
    // Serial.println("Sniffed Data on 2 @ " + (String)millis() + " ID: " + (String)msg.id);
    CanCumminsBus.write(msg);
  }
}

void J1939Bus::initialize(AppData *currentData) {
  Serial.println("J1939 Bus initializing");

  appData = currentData;

  CanCumminsBus.begin();
  CanCumminsBus.setBaudRate(250 * 1000);
  CanCumminsBus.setMaxMB(16);
  CanCumminsBus.enableFIFO();
  CanCumminsBus.enableFIFOInterrupt();
  CanCumminsBus.onReceive(sniffDataCumminsBus);
  CanCumminsBus.mailboxStatus();

  CanPrivate.begin();
  CanPrivate.setBaudRate(250 * 1000);
  CanPrivate.setMaxMB(16);
  CanPrivate.enableFIFO();
  CanPrivate.enableFIFOInterrupt();
  CanPrivate.onReceive(sniffDataPrivate);
  CanPrivate.mailboxStatus();
}

void J1939Bus::sendPgn65129(float engineIntakeManifold1AirTemperatureC,
                            float engineCoolantTemperatureC) {
  J1939Message message = J1939Message();
  message.setPgn(65129);
  message.setPriority(6);
  message.setSourceAddress(149);

  CAN_message_t msg;
  msg.flags.extended = 1;
  msg.id = message.canId;
  float intakeTempOffset = engineIntakeManifold1AirTemperatureC + 273.0;
  intakeTempOffset /= 0.03125;
  float coolantTempOffset = engineCoolantTemperatureC + 273.0;
  coolantTempOffset /= 0.03125;
#if defined(SPN_1363_INTAKE_MANIFOLD_1_AIR_TEMPERATURE)
  msg.buf[0] = highByte(static_cast<uint16_t>(intakeTempOffset));   // 1636
  msg.buf[1] = lowByte(static_cast<uint16_t>(intakeTempOffset));    // 1636
#endif
#if defined(SPN_110_ENGINE_COOLANT_TEMPERATURE)
  msg.buf[2] = highByte(static_cast<uint16_t>(coolantTempOffset));  // 1637
  msg.buf[3] = lowByte(static_cast<uint16_t>(coolantTempOffset));   // 1637
#endif

  CanPrivate.write(msg);
}

void J1939Bus::sendPgn65164() {
  J1939Message message = J1939Message();
  message.setPgn(65164);
  message.setPriority(6);
  message.setSourceAddress(149);

  CAN_message_t msg;
  msg.flags.extended = 1;
  msg.id = message.canId;
#if defined(SPN_441_ENGINE_BAY_TEMPERATURE)
  msg.buf[0] =
      static_cast<uint8_t>(appData->engineBayTemperatureC + 40);  // 441
#endif
#if defined(SPN_354_RELATIVE_HUMIDITY)
  msg.buf[6] = static_cast<uint8_t>(appData->humidity / .4);  // 354
#endif

  CanPrivate.write(msg);
}

void J1939Bus::sendPgn65189(float engineIntakeManifold2TemperatureC,
                            float engineIntakeManifold3TemperatureC,
                            float engineIntakeManifold4TemperatureC) {
  J1939Message message = J1939Message();
  message.setPgn(65189);
  message.setPriority(6);
  message.setSourceAddress(149);

  CAN_message_t msg;
  msg.flags.extended = 1;
  msg.id = message.canId;
#if defined(SPN_1131_INTAKE_MANIFOLD_2_AIR_TEMPERATURE)
  msg.buf[0] =
      static_cast<uint8_t>(engineIntakeManifold2TemperatureC + 40);  // 1131
#endif
#if defined(SPN_1132_INTAKE_MANIFOLD_3_AIR_TEMPERATURE)
  msg.buf[1] =
      static_cast<uint8_t>(engineIntakeManifold3TemperatureC + 40);  // 1132
#endif
#if defined(SPN_1133_INTAKE_MANIFOLD_4_AIR_TEMPERATURE)
  msg.buf[2] =
      static_cast<uint8_t>(engineIntakeManifold4TemperatureC + 40);  // 1133
#endif

  CanPrivate.write(msg);
}

void J1939Bus::sendPgn65190(float engineTurbocharger1BoostPressurekPa,
                            float engineTurbocharger2BoostPressurekPa) {
  J1939Message message = J1939Message();
  message.setPgn(65190);
  message.setPriority(6);
  message.setSourceAddress(149);

  CAN_message_t msg;
  msg.flags.extended = 1;
  msg.id = message.canId;
  float boost1Offset = engineTurbocharger1BoostPressurekPa / 0.125;
  float boost2Offset = engineTurbocharger2BoostPressurekPa / 0.125;
#if defined(SPN_1127_TURBOCHARGER_1_BOOST_PRESSURE)
  msg.buf[0] = highByte(static_cast<uint16_t>(boost1Offset));  // 1127
  msg.buf[1] = lowByte(static_cast<uint16_t>(boost1Offset));   // 1127
#endif
#if defined(SPN_1128_TURBOCHARGER_2_BOOST_PRESSURE)
  msg.buf[2] = highByte(static_cast<uint16_t>(boost2Offset));  // 1128
  msg.buf[3] = lowByte(static_cast<uint16_t>(boost2Offset));   // 1128
#endif

  CanPrivate.write(msg);
}

void J1939Bus::sendPgn65262(float engineCoolantTemperatureC,
                            float engineFuelTemperatureC,
                            float engineOilTemperatureC) {
  J1939Message message = J1939Message();
  message.setPgn(65262);
  message.setPriority(6);
  message.setSourceAddress(149);

  CAN_message_t msg;
  msg.flags.extended = 1;
  msg.id = message.canId;
  float oilTempOffset = engineOilTemperatureC + 273.0;
  oilTempOffset /= 0.03125;
#if defined(SPN_110_ENGINE_COOLANT_TEMPERATURE)
  msg.buf[0] = static_cast<uint8_t>(engineCoolantTemperatureC + 40);  // 110
#endif
#if defined(SPN_174_FUEL_TEMPERATURE)
  msg.buf[1] = static_cast<uint8_t>(engineFuelTemperatureC + 40);     // 174
#endif
#if defined(SPN_175_ENGINE_OIL_TEMPERATURE)
  msg.buf[2] = highByte(static_cast<uint16_t>(oilTempOffset));        // 175
  msg.buf[3] = lowByte(static_cast<uint16_t>(oilTempOffset));         // 175
#endif

  CanPrivate.write(msg);
}

void J1939Bus::sendPgn65263(float engineFuelDeliveryPressurekPa,
                            float engineOilPressurekPa,
                            float engineCoolantPressurekPa) {
  J1939Message message = J1939Message();
  message.setPgn(65263);
  message.setPriority(6);
  message.setSourceAddress(149);

  CAN_message_t msg;
  msg.flags.extended = 1;
  msg.id = message.canId;
#if defined(SPN_94_FUEL_DELIVERY_PRESSURE)
  msg.buf[0] = static_cast<uint8_t>(engineFuelDeliveryPressurekPa / 4);  // 94
#endif
#if defined(SPN_100_ENGINE_OIL_PRESSURE)
  msg.buf[3] = static_cast<uint8_t>(engineOilPressurekPa / 4);  // 100
#endif
#if defined(SPN_109_COOLANT_PRESSURE)
  msg.buf[6] = static_cast<uint8_t>(engineCoolantPressurekPa / 2);  // 109
#endif

  CanPrivate.write(msg);
}

void J1939Bus::sendPgn65269(float ambientTemperatureC,
                            float airInletTemperatureC,
                            float barometricPressurekPa) {
  J1939Message message = J1939Message();
  message.setPgn(65269);
  message.setPriority(6);
  message.setSourceAddress(149);

  CAN_message_t msg;
  msg.flags.extended = 1;
  msg.id = message.canId;
  float ambientAirTempOffset = ambientTemperatureC + 273.0;
  ambientAirTempOffset /= 0.03125;
#if defined(SPN_108_BAROMETRIC_PRESSURE)
  msg.buf[0] = static_cast<uint8_t>(barometricPressurekPa * 2);  // 108
#endif
#if defined(SPN_171_AMBIENT_AIR_TEMP)
  msg.buf[3] = highByte(static_cast<uint16_t>(ambientAirTempOffset));  // 171
  msg.buf[4] = lowByte(static_cast<uint16_t>(ambientAirTempOffset));   // 171
#endif
#if defined(SPN_172_AIR_INLET_TEMPERATURE)
  msg.buf[5] = static_cast<uint8_t>(airInletTemperatureC + 40);        // 172
#endif
  CanPrivate.write(msg);
}

void J1939Bus::sendPgn65270(float airInletPressurekPa,
                            float airInletTemperatureC, float egtTemperatureC,
                            float boostPressurekPa) {
  J1939Message message = J1939Message();
  message.setPgn(65270);
  message.setPriority(6);
  message.setSourceAddress(149);

  CAN_message_t msg;
  msg.flags.extended = 1;
  msg.id = message.canId;
  float egtOffset = egtTemperatureC + 273.0;
  egtOffset /= 0.03125;
#if defined(SPN_102_BOOST_PRESSURE)
  msg.buf[1] = static_cast<uint8_t>(boostPressurekPa / 2);       // 102
#endif
#if defined(SPN_1363_INTAKE_MANIFOLD_1_AIR_TEMPERATURE)
  msg.buf[2] = static_cast<uint8_t>(airInletTemperatureC + 40);  // 105
#endif
#if defined(SPN_106_AIR_INLET_PRESSURE)
  msg.buf[3] = static_cast<uint8_t>(airInletPressurekPa / 2);    // 106
#endif
#if defined(SPN_173_EXHAUST_GAS_TEMPERATURE)
  msg.buf[5] = highByte(static_cast<uint16_t>(egtOffset));  // 173
  msg.buf[6] = lowByte(static_cast<uint16_t>(egtOffset));   // 173
#endif
  CanPrivate.write(msg);
}

#endif