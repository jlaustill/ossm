#ifndef OSSM_J1939Bus_H
#define OSSM_J1939Bus_H

#include <AppData.h>
#include <FlexCAN_T4.h>
#include <J1939.h>

class J1939Bus {
 public:
  static void initialize(AppData *appData);
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
  static void sniffData(const CAN_message_t &msg);
};

#endif