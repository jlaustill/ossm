#ifndef OSSM_J1939Bus_H
#define OSSM_J1939Bus_H

#include "app_data.h"
#include <AppConfig.h>
#include <FlexCAN_T4.h>
#include <J1939Message.h>

class J1939Bus {
 public:
  static void initialize(AppData *appData, AppConfig *config);
  static void sendPgn65269(float ambientTemperatureC,
                           float airInletTemperatureC,
                           float barometricPressurekPa);
  static void sendPgn65270(float airInletPressurekPa,
                           float airInletTemperatureC, float egtTemperatureC,
                           float boostPressurekPa);
  static void sendPgn65262(float engineCoolantTemperatureC,
                           float engineFuelTemperatureC,
                           float engineOilTemperatureC);
  static void sendPgn65263(float engineFuelDeliveryPressurekPa,
                           float engineOilPressurekPa,
                           float engineCoolantPressurekPa);
  static void sendPgn65129(float engineIntakeManifold1AirTemperatureC,
                           float engineCoolantTemperatureC);
  static void sendPgn65189(float engineIntakeManifold2TemperatureC,
                           float engineIntakeManifold3TemperatureC,
                           float engineIntakeManifold4TemperatureC);
  static void sendPgn65190(float engineTurbocharger1BoostPressurekPa,
                           float engineTurbocharger2BoostPressurekPa);
  static void sendPgn65164();

 private:
  static AppData *appData;
  static AppConfig *config;  // Non-const to allow command handler to modify
  static void sniffDataPrivate(const CAN_message_t &msg);

  // J1939 Command Interface (PGN 65280/65281)
  static void processConfigCommand(const uint8_t *data, uint8_t len);
  static void sendConfigResponse(uint8_t cmd, uint8_t errCode, const uint8_t *data, uint8_t dataLen);

  // Command handlers
  static void handleJ1939EnableSpn(const uint8_t *data);
  static void handleJ1939SetNtcParam(const uint8_t *data);
  static void handleJ1939SetPressureRange(const uint8_t *data);
  static void handleJ1939SetTcType(const uint8_t *data);
  static void handleJ1939Query(const uint8_t *data);
  static void handleJ1939Save();
  static void handleJ1939Reset();
  static void handleJ1939NtcPreset(const uint8_t *data);
  static void handleJ1939PressurePreset(const uint8_t *data);
};

#endif