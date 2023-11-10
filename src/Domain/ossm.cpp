#include "ossm.h"

#include <Adafruit_MAX31855.h>
#include <AppData.h>
#include <Arduino.h>
#include <Display/ODB2.h>
#include <PressureSensor.h>
#include <TempSensor.h>

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
TempSensor transmissionTempSensor;
PressureSensor transmissionPressureSensor;
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

void ossm::setup() {
  Serial.begin(115200);
  while (!Serial)
    ;  // time to get serial running

  ossm::ambientSensors = new AmbientSensors(&ossm::isBmeInitialized);
  OBD2::initialize(&ossm::appData);

  oilTempSensor =
      TempSensor(10000, AEM_TEMP_SENSOR_A, AEM_TEMP_SENSOR_B, AEM_TEMP_SENSOR_C,
                 0x48, 0, 0, &ossm::isAds1Initialized);
  oilPressureSensor = PressureSensor(0x48, 1, 30);
  coolantTempSensor =
      TempSensor(10000, AEM_TEMP_SENSOR_A, AEM_TEMP_SENSOR_B, AEM_TEMP_SENSOR_C,
                 0x48, 2, 0, &ossm::isAds1Initialized);
  coolantPressureSensor = PressureSensor(0x48, 3, 50);

  transmissionTempSensor =
      TempSensor(10000, AEM_TEMP_SENSOR_A, AEM_TEMP_SENSOR_B, AEM_TEMP_SENSOR_C,
                 0x49, 0, 0, &ossm::isAds2Initialized);
  transmissionPressureSensor = PressureSensor(0x49, 1, 500);
  boostTempSensor =
      TempSensor(10000, AEM_TEMP_SENSOR_A, AEM_TEMP_SENSOR_B, AEM_TEMP_SENSOR_C,
                 0x49, 2, 0, &ossm::isAds2Initialized);
  boostPressureSensor = PressureSensor(0x49, 3, 150);

  cacTempSensor =
      TempSensor(10000, AEM_TEMP_SENSOR_A, AEM_TEMP_SENSOR_B, AEM_TEMP_SENSOR_C,
                 0x4A, 1, 0, &ossm::isAds3Initialized);
  cacPressureSensor = PressureSensor(0x4A, 0, 150);
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
    ossm::appData.absoluteBarometricpressure =
        ossm::ambientSensors->getPressureHPa();
    ossm::appData.humidity = ossm::ambientSensors->getHumidity();
    ossm::appData.ambientTemperature = ossm::ambientSensors->getTemperatureC();
  }

  if (ossm::isAds1Initialized) {
    ossm::appData.oilTemperature = oilTempSensor.getTempCelsius();
    ossm::appData.oilPressure = oilPressureSensor.getPressureInPsi();
    ossm::appData.coolantTemperature = coolantTempSensor.getTempCelsius();
    ossm::appData.coolantPressure = coolantPressureSensor.getPressureInPsi();
  }

  if (ossm::isAds2Initialized == true) {
    ossm::appData.transmissionPressurekPa =
        transmissionPressureSensor.getPressureInkPa();
    ossm::appData.transmissionTemperatureC =
        transmissionTempSensor.getTempCelsius();
    ossm::appData.boostPressure = boostPressureSensor.getPressureInPsi();
    ossm::appData.boostTemperature = boostTempSensor.getTempCelsius();
  }

  if (ossm::isAds3Initialized == true) {
    ossm::appData.cacPressure = cacPressureSensor.getPressureInPsi();
    ossm::appData.cacTemperature = cacTempSensor.getTempCelsius();
    ossm::appData.intakePressure = intakePressureSensor.getPressureInPsi();
    ossm::appData.intakeTemperature = intakeTempSensor.getTempCelsius();
  }

  if (ossm::isAds4Initialized == true) {
    ossm::appData.fuelPressure = fuelPressureSensor.getPressureInPsi();
    ossm::appData.fuelTemperature = fuelTempSensor.getTempCelsius();
    ossm::appData.engineBayTemperature = engineBayTempSensor.getTempCelsius();
  }

  if (ossm::isEgtInitialized) {
    ossm::appData.egtTemperature = thermocouple.readCelsius();
  }

  if (ossm::isEgtInitialized) {
    Serial.println("AppData: egtTemperature->" +
                   String(ossm::appData.egtTemperature) + "°C");
  }

  if (ossm::isBmeInitialized == true) {
    Serial.println("AppData: Barometric Pressure->" +
                   String(ossm::appData.absoluteBarometricpressure) +
                   "hPa, Humidity->" + String(ossm::appData.humidity) +
                   "%, Ambient Temperature->" +
                   String(ossm::appData.ambientTemperature) + "°C");
  }

  if (ossm::isAds1Initialized == true) {
    Serial.println(
        "AppData: Oil Temperature->" + String(ossm::appData.oilTemperature) +
        "°C, Oil Pressure->" + String(ossm::appData.oilPressure) +
        "psi, Coolant Temperature->" +
        String(ossm::appData.coolantTemperature) + "°C, Coolant Pressure->" +
        String(ossm::appData.coolantPressure) + "psi");
  }

  if (ossm::isAds2Initialized == true) {
    Serial.println(
        "AppData: Transmission Pressure->" +
        String(ossm::appData.transmissionPressurekPa) +
        "kPa, Transmission Temperature->" +
        String(ossm::appData.transmissionTemperatureC) + "°C, Boost Pressure->" +
        String(ossm::appData.boostPressure) + "psi, Boost Temperature->" +
        String(ossm::appData.boostTemperature) + "°C");
  }

  if (ossm::isAds3Initialized == true) {
    Serial.println(
        "AppData: CAC Pressure->" + String(ossm::appData.cacPressure) +
        "psi, CAC Temperature->" + String(ossm::appData.cacTemperature) +
        "°C, Intake Pressure->" + String(ossm::appData.intakePressure) +
        "psi, Intake Temperature->" + String(ossm::appData.intakeTemperature) +
        "°C");
  }

  if (ossm::isAds4Initialized == true) {
    Serial.println(
        "AppData: Fuel Pressure->" + String(ossm::appData.fuelPressure) +
        "psi, Fuel Temperature->" + String(ossm::appData.fuelTemperature) +
        "°C, Engine Bay Temperature->" +
        String(ossm::appData.engineBayTemperature) + "°C");
  }

  delay(ossm::delayTime);
}