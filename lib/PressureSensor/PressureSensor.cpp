//
// Created by jlaustill on 5/27/23.
//

#include "PressureSensor.h"

#include <Arduino.h>

void PressureSensor::updateSensor() {
  this->rawValue = ads.computeVolts(ads.readADC_SingleEnded(ChannelId));
}

double showDecimals(const double& x, const int& numDecimals) {
  int y = x;
  double z = x - y;
  double m = pow(10, numDecimals);
  double q = z * m;
  double r = round(q);

  return static_cast<double>(y) + (1.0 / m) * r;
}

double onlyPositive(const double& x) {
  if (x < 0) {
    return 0.0f;
  }

  return x;
}

float PressureSensor::getPressureInPsi() {
  if (!initialized) {
    return 0.0f;
  }

//   Serial.println("RawValue->" + (String)this->rawValue);

  this->updateSensor();
  float zeroVoltage = .32f;
  float maxVoltage = 3.0f;
  float returnValue = onlyPositive(showDecimals(
      map(this->rawValue, zeroVoltage, maxVoltage, 0.0f, this->PsiMax), 1));
  return returnValue;
}

float PressureSensor::getPressureInkPa() {
  float pressureInPsi = this->getPressureInPsi();
  float pressureInkPa = pressureInPsi * 6.894757f;
  return pressureInkPa;
}