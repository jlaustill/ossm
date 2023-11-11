//
// Created by jlaustill on 10/30/2023
//

#ifndef OSSM_APPDATA_H
#define OSSM_APPDATA_H

struct AppData {
  float humidity = 0.0f;
  float ambientTemperatureC = 0.0f;
  float absoluteBarometricpressurehPa = 0.0f;
  float oilTemperatureC = 0.0f;
  float oilPressurekPa = 0.0f;
  float coolantTemperatureC = 0.0f;
  float coolantPressurekPa = 0.0f;
  float transmissionTemperatureC = 0.0f;
  float transmissionPressurekPa = 0.0f;
  float boostPressurekPa = 0.0f;
  float boostTemperatureC = 0.0f;
  float cacPressure = 0.0f;
  float cacTemperature = 0.0f;
  float intakePressure = 0.0f;
  float intakeTemperature = 0.0f;
  float fuelPressure = 0.0f;
  float fuelTemperature = 0.0f;
  float engineBayTemperature = 0.0f;
  double egtTemperature = 0.0f;
};

#endif  // OSSM_APPDATA_H