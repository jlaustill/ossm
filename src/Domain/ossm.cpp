#include "ossm.h"

#include <Adafruit_MAX31855.h>
#include <AppData.h>
#include <Arduino.h>
#include <PressureSensor.h>
#include <TempSensor.h>

#include "Display/J1939Bus.h"

#define AEM_TEMP_SENSOR_A 1.485995686e-03
#define AEM_TEMP_SENSOR_B 2.279654266e-04
#define AEM_TEMP_SENSOR_C 1.197578033e-07
#define MAXCS 10

bool ossm::isAds1Initialized = false;
bool ossm::isAds2Initialized = false;
bool ossm::isAds3Initialized = false;
bool ossm::isAds4Initialized = false;
bool ossm::isEgtInitialized = false;
bool ossm::isBmeInitialized = false;

AppData ossm::appData;
AmbientSensors *ossm::ambientSensors;
TempSensor oilTempSensor;
PressureSensor oilPressureSensor;
TempSensor coolantTempSensor;
PressureSensor coolantPressureSensor;
TempSensor tranferPipeTempSensor;
PressureSensor transferPipePressureSensor;
TempSensor boostTempSensor;
PressureSensor boostPressureSensor;
PressureSensor cacPressureSensor;
TempSensor cacTempSensor;
PressureSensor intakePressureSensor;
TempSensor intakeTempSensor;
PressureSensor fuelPressureSensor;
TempSensor fuelTempSensor;
TempSensor engineBayTempSensor;
Adafruit_MAX31855 thermocouple(MAXCS);

uint32_t lastMillis;

void ossm::setup() {
  lastMillis = millis();
  Serial.begin(115200);
  while (!Serial);  // time to get serial running

  J1939Bus::initialize();

  ossm::ambientSensors = new AmbientSensors(&ossm::isBmeInitialized);

  oilTempSensor =
      TempSensor(10000, AEM_TEMP_SENSOR_A, AEM_TEMP_SENSOR_B, AEM_TEMP_SENSOR_C,
                 0x48, 0, 0, &ossm::isAds1Initialized);
  oilPressureSensor = PressureSensor(0x48, 1, 150);
  coolantTempSensor =
      TempSensor(10000, AEM_TEMP_SENSOR_A, AEM_TEMP_SENSOR_B, AEM_TEMP_SENSOR_C,
                 0x48, 2, 0, &ossm::isAds1Initialized);
  coolantPressureSensor = PressureSensor(0x48, 3, 50);

  tranferPipeTempSensor =
      TempSensor(10000, AEM_TEMP_SENSOR_A, AEM_TEMP_SENSOR_B, AEM_TEMP_SENSOR_C,
                 0x49, 0, 0, &ossm::isAds2Initialized);
  transferPipePressureSensor = PressureSensor(0x49, 1, 500);
  boostTempSensor =
      TempSensor(10000, AEM_TEMP_SENSOR_A, AEM_TEMP_SENSOR_B, AEM_TEMP_SENSOR_C,
                 0x49, 2, 0, &ossm::isAds2Initialized);
  boostPressureSensor = PressureSensor(0x49, 3, 150);

  cacTempSensor =
      TempSensor(10000, AEM_TEMP_SENSOR_A, AEM_TEMP_SENSOR_B, AEM_TEMP_SENSOR_C,
                 0x4A, 1, 0, &ossm::isAds3Initialized);
  cacPressureSensor = PressureSensor(0x4A, 0, 30);
  intakeTempSensor =
      TempSensor(10000, AEM_TEMP_SENSOR_A, AEM_TEMP_SENSOR_B, AEM_TEMP_SENSOR_C,
                 0x4A, 2, 0, &ossm::isAds3Initialized);
  intakePressureSensor = PressureSensor(0x4A, 3, 30);

  fuelTempSensor =
      TempSensor(10000, AEM_TEMP_SENSOR_A, AEM_TEMP_SENSOR_B, AEM_TEMP_SENSOR_C,
                 0x4B, 1, 0, &ossm::isAds4Initialized);
  fuelPressureSensor = PressureSensor(0x4B, 0, 100);
  engineBayTempSensor =
      TempSensor(10000, AEM_TEMP_SENSOR_A, AEM_TEMP_SENSOR_B, AEM_TEMP_SENSOR_C,
                 0x4B, 2, 0, &ossm::isAds4Initialized);

  if (!thermocouple.begin()) {
    Serial.println("ERROR INITING THERMOCOUPLE");
  } else {
    ossm::isEgtInitialized = true;
    Serial.println("Thermocouple initialized");
  }

  Serial.println("isAds1Initialized->" + String(ossm::isAds1Initialized));
  Serial.println("isAds2Initialized->" + String(ossm::isAds2Initialized));
  Serial.println("isAds3Initialized->" + String(ossm::isAds3Initialized));
  Serial.println("isAds4Initialized->" + String(ossm::isAds4Initialized));
  Serial.println("isEgtInitialized->" + String(ossm::isEgtInitialized));
  Serial.println("isBmeInitialized->" + String(ossm::isBmeInitialized));
}

void ossm::loop() {
  if (ossm::isBmeInitialized == true) {
    ossm::appData.absoluteBarometricpressurehPa =
        ossm::ambientSensors->getPressureHPa();
    ossm::appData.humidity = ossm::ambientSensors->getHumidity();
    ossm::appData.ambientTemperatureC = ossm::ambientSensors->getTemperatureC();
  }

  if (ossm::isAds1Initialized) {
    ossm::appData.oilTemperatureC = oilTempSensor.getTempCelsius();
    ossm::appData.oilPressurekPa = oilPressureSensor.getPressureInkPa();
    ossm::appData.coolantTemperatureC = coolantTempSensor.getTempCelsius();
    ossm::appData.coolantPressurekPa = coolantPressureSensor.getPressureInkPa();
  }

  if (ossm::isAds2Initialized == true) {
    ossm::appData.tranferPipePressurekPa =
        transferPipePressureSensor.getPressureInkPa();
    ossm::appData.transferPipeTemperatureC =
        tranferPipeTempSensor.getTempCelsius();
    ossm::appData.boostPressurekPa = boostPressureSensor.getPressureInkPa();
    ossm::appData.boostTemperatureC = boostTempSensor.getTempCelsius();
  }

  if (ossm::isAds3Initialized == true) {
    ossm::appData.cacInletPressurekPa = cacPressureSensor.getPressureInkPa();
    ossm::appData.cacInletTemperatureC = cacTempSensor.getTempCelsius();
    ossm::appData.airInletPressurekPa = intakePressureSensor.getPressureInkPa();
    ossm::appData.airInletTemperatureC = intakeTempSensor.getTempCelsius();
  }

  if (ossm::isAds4Initialized == true) {
    ossm::appData.fuelPressurekPa = fuelPressureSensor.getPressureInkPa();
    ossm::appData.fuelTemperatureC = fuelTempSensor.getTempCelsius();
    ossm::appData.engineBayTemperatureC = engineBayTempSensor.getTempCelsius();
  }

  if (ossm::isEgtInitialized) {
    ossm::appData.egtTemperatureC = thermocouple.readCelsius();
  }

  if (ossm::isEgtInitialized) {
    Serial.println("AppData: egtTemperature->" +
                   String(ossm::appData.egtTemperatureC) + "°C");
  }

  // Every 1 second
  if (lastMillis - millis() >= 1000) {
    if (ossm::isBmeInitialized == true) {
      Serial.println("AppData: Barometric Pressure->" +
                     String(ossm::appData.absoluteBarometricpressurehPa) +
                     "hPa, Humidity->" + String(ossm::appData.humidity) +
                     "%, Ambient Temperature->" +
                     String(ossm::appData.ambientTemperatureC) + "°C");
    }

    if (ossm::isAds1Initialized == true) {
      Serial.println(
          "AppData: Oil Temperature->" + String(ossm::appData.oilTemperatureC) +
          "°C, Oil Pressure->" + String(ossm::appData.oilPressurekPa) +
          "kPa, Coolant Temperature->" +
          String(ossm::appData.coolantTemperatureC) + "°C, Coolant Pressure->" +
          String(ossm::appData.coolantPressurekPa) + "kPa");
    }

    if (ossm::isAds2Initialized == true) {
      Serial.println("AppData: Transmission Pressure->" +
                     String(ossm::appData.tranferPipePressurekPa) +
                     "kPa, Transmission Temperature->" +
                     String(ossm::appData.transferPipeTemperatureC) +
                     "°C, Boost Pressure->" +
                     String(ossm::appData.boostPressurekPa) +
                     "kPa, Boost Temperature->" +
                     String(ossm::appData.boostTemperatureC) + "°C");
    }

    if (ossm::isAds3Initialized == true) {
      Serial.println(
          "AppData: CAC Pressure->" +
          String(ossm::appData.cacInletPressurekPa) + "kPa, CAC Temperature->" +
          String(ossm::appData.cacInletTemperatureC) + "°C, Intake Pressure->" +
          String(ossm::appData.airInletPressurekPa) +
          "kPa, Intake Temperature->" +
          String(ossm::appData.airInletTemperatureC) + "°C");
    }

    if (ossm::isAds4Initialized == true) {
      Serial.println(
          "AppData: Fuel Pressure->" + String(ossm::appData.fuelPressurekPa) +
          "kPa, Fuel Temperature->" + String(ossm::appData.fuelTemperatureC) +
          "°C, Engine Bay Temperature->" +
          String(ossm::appData.engineBayTemperatureC) + "°C");
    }

    J1939Bus::sendPgn65269(ossm::appData.ambientTemperatureC,
                           ossm::appData.airInletTemperatureC,
                           ossm::appData.absoluteBarometricpressurehPa);
  }
}