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
  float cacPressurekPa = 0.0f;
  float cacTemperatureC = 0.0f;
  float intakePressurekPa = 0.0f;
  float intakeTemperatureC = 0.0f;
  float fuelPressurekPa = 0.0f;
  float fuelTemperatureC = 0.0f;
  float engineBayTemperatureC = 0.0f;
  double egtTemperatureC = 0.0f;
};

#endif  // OSSM_APPDATA_H