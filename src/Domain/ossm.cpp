#include "ossm.h"
#include "configuration.h"


#include <AppData.h>
#include <Arduino.h>

#include <PressureSensor.h>
#include <TempSensor.h>

#include "Display/J1939Bus.h"

bool ossm::isAds4Initialized = false;

AppData ossm::appData;

TempSensor boostTempSensor;
PressureSensor boostPressureSensor;
PressureSensor fuelPressureSensor;

elapsedMillis oneSecondMillis;
elapsedMillis halfSecondMillis;

void ossm::setup() {
  Serial.begin(115200);

  J1939Bus::initialize(&ossm::appData);

  boostTempSensor =
      TempSensor(10050, AEM_TEMP_SENSOR_A, AEM_TEMP_SENSOR_B, AEM_TEMP_SENSOR_C,
                 0x4B, 3, 0, &ossm::isAds4Initialized);
                 
  boostPressureSensor = PressureSensor(0x4B, 1, 150, &ossm::isAds4Initialized);  
  fuelPressureSensor = PressureSensor(0x4B, 0, 100, &ossm::isAds4Initialized);
  Serial.println("isAds4Initialized->" + String(ossm::isAds4Initialized));
} // ossm::setup

void ossm::loop() {  
  // Every .5 seconds
  if (halfSecondMillis >= 500) {
    ossm::appData.boostPressurekPa = boostPressureSensor.getPressureInkPa();
    ossm::appData.fuelPressurekPa = fuelPressureSensor.getPressureInkPa();
    J1939Bus::sendPgn65270(
        0xFF, ossm::appData.boostTemperatureC,
        0xFF, ossm::appData.boostPressurekPa);
        
    J1939Bus::sendPgn65263(ossm::appData.fuelPressurekPa,
                           0xFF,
                           0xFF);
    halfSecondMillis = 0;
  }

  // Every 1 second
  if (oneSecondMillis >= 1000) {
    // Serial.print("boost pressure: ");
    // Serial.print(ossm::appData.boostPressurekPa);
    // Serial.print(" kPa, boost temperature: ");
    // Serial.print(ossm::appData.boostTemperatureC);
    // Serial.print(" C, fuel pressure: ");
    // Serial.print(ossm::appData.fuelPressurekPa);
    // Serial.print(" kPa last millis ");
    // Serial.print(lastHalfSecondMillis);
    // Serial.print(" this millis ");
    // Serial.println(thisMillis);
    ossm::appData.boostTemperatureC = boostTempSensor.getTempCelsius();
    J1939Bus::sendPgn65129(ossm::appData.boostTemperatureC,
                           0xFF);

    oneSecondMillis = 0;
  }
}