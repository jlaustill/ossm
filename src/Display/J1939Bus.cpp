#ifndef OSSM_J1939Bus_CPP
#define OSSM_J1939Bus_CPP

#include "J1939Bus.h"

#include <AppData.h>
#include <AppConfig.h>
#include <FlexCAN_T4.h>

#include <SeaDash.hpp>

FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> CanCumminsBus;
FlexCAN_T4<CAN3, RX_SIZE_256, TX_SIZE_16> CanPrivate;

AppData *J1939Bus::appData;
const AppConfig *J1939Bus::config;
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
  J1939Message message = J1939Message();
  message.setCanId(msg.id);
  message.setData(msg.buf);

  if (message.pgn == 59904) {
    CanCumminsBus.write(msg);
  }
}

void J1939Bus::initialize(AppData *currentData, const AppConfig *cfg) {
  Serial.println("J1939 Bus initializing");

  appData = currentData;
  config = cfg;

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

  // Initialize buffer to 0xFF (not available)
  for (int i = 0; i < 8; i++) msg.buf[i] = 0xFF;

  float intakeTempOffset = engineIntakeManifold1AirTemperatureC + 273.0;
  intakeTempOffset /= 0.03125;
  float coolantTempOffset = engineCoolantTemperatureC + 273.0;
  coolantTempOffset /= 0.03125;

  if (config->j1939Spns.spn1363_intakeManifold1AirTemp) {
    msg.buf[0] = highByte(static_cast<uint16_t>(intakeTempOffset));   // 1636
    msg.buf[1] = lowByte(static_cast<uint16_t>(intakeTempOffset));    // 1636
  }
  if (config->j1939Spns.spn110_engineCoolantTemp) {
    msg.buf[2] = highByte(static_cast<uint16_t>(coolantTempOffset));  // 1637
    msg.buf[3] = lowByte(static_cast<uint16_t>(coolantTempOffset));   // 1637
  }

  CanPrivate.write(msg);
}

void J1939Bus::sendPgn65164() {
  J1939Message message = J1939Message();
  message.setPgn(65164);
  message.setPriority(6);
  message.setSourceAddress(config->j1939SourceAddress);

  CAN_message_t msg;
  msg.flags.extended = 1;
  msg.id = message.canId;

  // Initialize buffer to 0xFF (not available)
  for (int i = 0; i < 8; i++) msg.buf[i] = 0xFF;

  if (config->j1939Spns.spn441_engineBayTemp) {
    msg.buf[0] = static_cast<uint8_t>(appData->engineBayTemperatureC + 40);  // 441
  }
  if (config->j1939Spns.spn354_relativeHumidity) {
    msg.buf[6] = static_cast<uint8_t>(appData->humidity / .4);  // 354
  }

  CanPrivate.write(msg);
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

  // Initialize buffer to 0xFF (not available)
  for (int i = 0; i < 8; i++) msg.buf[i] = 0xFF;

  if (config->j1939Spns.spn1131_intakeManifold2Temp) {
    msg.buf[0] = static_cast<uint8_t>(engineIntakeManifold2TemperatureC + 40);  // 1131
  }
  if (config->j1939Spns.spn1132_intakeManifold3Temp) {
    msg.buf[1] = static_cast<uint8_t>(engineIntakeManifold3TemperatureC + 40);  // 1132
  }
  if (config->j1939Spns.spn1133_intakeManifold4Temp) {
    msg.buf[2] = static_cast<uint8_t>(engineIntakeManifold4TemperatureC + 40);  // 1133
  }

  CanPrivate.write(msg);
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

  // Initialize buffer to 0xFF (not available)
  for (int i = 0; i < 8; i++) msg.buf[i] = 0xFF;

  float boost1Offset = engineTurbocharger1BoostPressurekPa / 0.125;
  float boost2Offset = engineTurbocharger2BoostPressurekPa / 0.125;

  if (config->j1939Spns.spn1127_turbo1BoostPressure) {
    msg.buf[0] = highByte(static_cast<uint16_t>(boost1Offset));  // 1127
    msg.buf[1] = lowByte(static_cast<uint16_t>(boost1Offset));   // 1127
  }
  if (config->j1939Spns.spn1128_turbo2BoostPressure) {
    msg.buf[2] = highByte(static_cast<uint16_t>(boost2Offset));  // 1128
    msg.buf[3] = lowByte(static_cast<uint16_t>(boost2Offset));   // 1128
  }

  CanPrivate.write(msg);
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

  // Initialize buffer to 0xFF (not available)
  for (int i = 0; i < 8; i++) msg.buf[i] = 0xFF;

  float oilTempOffset = engineOilTemperatureC + 273.0;
  oilTempOffset /= 0.03125;

  if (config->j1939Spns.spn110_engineCoolantTemp) {
    msg.buf[0] = static_cast<uint8_t>(engineCoolantTemperatureC + 40);  // 110
  }
  if (config->j1939Spns.spn174_fuelTemp) {
    msg.buf[1] = static_cast<uint8_t>(engineFuelTemperatureC + 40);     // 174
  }
  if (config->j1939Spns.spn175_engineOilTemp) {
    msg.buf[2] = highByte(static_cast<uint16_t>(oilTempOffset));        // 175
    msg.buf[3] = lowByte(static_cast<uint16_t>(oilTempOffset));         // 175
  }

  CanPrivate.write(msg);
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

  // Initialize buffer to 0xFF (not available)
  for (int i = 0; i < 8; i++) msg.buf[i] = 0xFF;

  if (config->j1939Spns.spn94_fuelDeliveryPressure) {
    msg.buf[0] = static_cast<uint8_t>(engineFuelDeliveryPressurekPa / 4);  // 94
  }
  if (config->j1939Spns.spn100_engineOilPressure) {
    msg.buf[3] = static_cast<uint8_t>(engineOilPressurekPa / 4);  // 100
  }
  if (config->j1939Spns.spn109_coolantPressure) {
    msg.buf[6] = static_cast<uint8_t>(engineCoolantPressurekPa / 2);  // 109
  }

  CanPrivate.write(msg);
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

  // Initialize buffer to 0xFF (not available)
  for (int i = 0; i < 8; i++) msg.buf[i] = 0xFF;

  float ambientAirTempOffset = ambientTemperatureC + 273.0;
  ambientAirTempOffset /= 0.03125;

  if (config->j1939Spns.spn108_barometricPressure) {
    msg.buf[0] = static_cast<uint8_t>(barometricPressurekPa * 2);  // 108
  }
  if (config->j1939Spns.spn171_ambientAirTemp) {
    msg.buf[3] = highByte(static_cast<uint16_t>(ambientAirTempOffset));  // 171
    msg.buf[4] = lowByte(static_cast<uint16_t>(ambientAirTempOffset));   // 171
  }
  if (config->j1939Spns.spn172_airInletTemp) {
    msg.buf[5] = static_cast<uint8_t>(airInletTemperatureC + 40);        // 172
  }

  CanPrivate.write(msg);
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

  // Initialize buffer to 0xFF (not available)
  for (int i = 0; i < 8; i++) msg.buf[i] = 0xFF;

  float egtOffset = egtTemperatureC + 273.0;
  egtOffset /= 0.03125;

  if (config->j1939Spns.spn102_boostPressure) {
    msg.buf[1] = static_cast<uint8_t>(boostPressurekPa / 2);       // 102
  }
  if (config->j1939Spns.spn1363_intakeManifold1AirTemp) {
    msg.buf[2] = static_cast<uint8_t>(airInletTemperatureC + 40);  // 105
  }
  if (config->j1939Spns.spn106_airInletPressure) {
    msg.buf[3] = static_cast<uint8_t>(airInletPressurekPa / 2);    // 106
  }
  if (config->j1939Spns.spn173_exhaustGasTemp) {
    msg.buf[5] = highByte(static_cast<uint16_t>(egtOffset));  // 173
    msg.buf[6] = lowByte(static_cast<uint16_t>(egtOffset));   // 173
  }

  CanPrivate.write(msg);
}

#endif
