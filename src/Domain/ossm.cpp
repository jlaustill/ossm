#include "ossm.h"
#include "configuration.h"

#if defined(SPN_173_EXHAUST_GAS_TEMPERATURE)
#include <Adafruit_MAX31855.h>
#endif
#include <AppData.h>
#include <Arduino.h>
#if defined(SPN_102_BOOST_PRESSURE) || defined(SPN_106_AIR_INLET_PRESSURE) || defined(SPN_109_COOLANT_PRESSURE) || defined(SPN_94_FUEL_DELIVERY_PRESSURE) || defined(SPN_1127_TURBOCHARGER_1_BOOST_PRESSURE) || defined(SPN_1128_TURBOCHARGER_2_BOOST_PRESSURE)
#include <PressureSensor.h>
#endif
#if defined(SPN_172_AIR_INLET_TEMPERATURE) || defined(SPN_105_INTAKE_MANIFOLD_1_TEMPERATURE) || defined(SPN_175_ENGINE_OIL_TEMPERATURE) || defined(SPN_110_ENGINE_COOLANT_TEMPERATURE) || defined(SPN_174_FUEL_TEMPERATURE) || defined(SPN_1637_ENGINE_COOLANT_TEMPERATURE) || defined(SPN_1363_INTAKE_MANIFOLD_1_AIR_TEMPERATURE) || defined(SPN_1131_INTAKE_MANIFOLD_2_AIR_TEMPERATURE) || defined(SPN_1132_INTAKE_MANIFOLD_3_AIR_TEMPERATURE) || defined(SPN_1133_INTAKE_MANIFOLD_4_AIR_TEMPERATURE) || defined(SPN_441_ENGINE_BAY_TEMPERATURE)
#include <TempSensor.h>
#endif

#include "Display/J1939Bus.h"

bool ossm::isAds1Initialized = false;
bool ossm::isAds2Initialized = false;
bool ossm::isAds3Initialized = false;
bool ossm::isAds4Initialized = false;
bool ossm::isEgtInitialized = false;
bool ossm::isBmeInitialized = false;

AppData ossm::appData;
#if defined(SPN_171_AMBIENT_AIR_TEMP) || defined(SPN_108_BAROMETRIC_PRESSURE) || defined(SPN_354_RELATIVE_HUMIDITY)
AmbientSensors *ossm::ambientSensors;
#endif
#if defined(SPN_175_ENGINE_OIL_TEMPERATURE)
TempSensor oilTempSensor;
#endif
#if defined(SPN_100_ENGINE_OIL_PRESSURE)
PressureSensor oilPressureSensor;
#endif
#if defined(SPN_110_ENGINE_COOLANT_TEMPERATURE)
TempSensor coolantTempSensor;
#endif
#if defined(SPN_109_COOLANT_PRESSURE)
PressureSensor coolantPressureSensor;
#endif
#if defined(SPN_1132_INTAKE_MANIFOLD_3_AIR_TEMPERATURE)
TempSensor tranferPipeTempSensor;
#endif
#if defined(SPN_1128_TURBOCHARGER_2_BOOST_PRESSURE)
PressureSensor transferPipePressureSensor;
#endif
#if defined(SPN_105_INTAKE_MANIFOLD_1_TEMPERATURE)
TempSensor boostTempSensor;
#endif
#if defined(SPN_102_BOOST_PRESSURE)
PressureSensor boostPressureSensor;
#endif
#if defined(SPN_1127_TURBOCHARGER_1_BOOST_PRESSURE)
PressureSensor cacPressureSensor;
#endif
#if defined(SPN_1131_INTAKE_MANIFOLD_2_AIR_TEMPERATURE)
TempSensor cacTempSensor;
#endif
#if defined(SPN_106_AIR_INLET_PRESSURE)
PressureSensor intakePressureSensor;
#endif
#if defined(SPN_172_AIR_INLET_TEMPERATURE)
TempSensor intakeTempSensor;
#endif
#if defined(SPN_94_FUEL_DELIVERY_PRESSURE)
PressureSensor fuelPressureSensor;
#endif
#if defined(SPN_174_FUEL_TEMPERATURE)
TempSensor fuelTempSensor;
#endif
#if defined(SPN_441_ENGINE_BAY_TEMPERATURE)
TempSensor engineBayTempSensor;
#endif
#if defined(SPN_173_EXHAUST_GAS_TEMPERATURE)
Adafruit_MAX31855 thermocouple(MAXCS);
#endif

uint32_t lastOneSecondMillis;
uint32_t lastHalfSecondMillis;
uint32_t thisMillis;

void ossm::setup() {
  lastOneSecondMillis = millis();
  lastHalfSecondMillis = millis();
  thisMillis = millis();
  Serial.begin(115200);

  J1939Bus::initialize(&ossm::appData);

#if defined(SPN_171_AMBIENT_AIR_TEMP) || defined(SPN_108_BAROMETRIC_PRESSURE) || defined(SPN_354_RELATIVE_HUMIDITY)
  ossm::ambientSensors = new AmbientSensors(&ossm::isBmeInitialized);
#endif
#if defined(SPN_175_ENGINE_OIL_TEMPERATURE)
  oilTempSensor =
      TempSensor(10000, AEM_TEMP_SENSOR_A, AEM_TEMP_SENSOR_B, AEM_TEMP_SENSOR_C,
                 0x48, 0, 0, &ossm::isAds1Initialized);
#endif
#if defined(SPN_100_ENGINE_OIL_PRESSURE)
  oilPressureSensor = PressureSensor(0x48, 1, 150);
#endif
#if defined(SPN_110_ENGINE_COOLANT_TEMPERATURE)
  coolantTempSensor =
      TempSensor(10000, AEM_TEMP_SENSOR_A, AEM_TEMP_SENSOR_B, AEM_TEMP_SENSOR_C,
                 0x48, 2, 0, &ossm::isAds1Initialized);
#endif
#if defined(SPN_109_COOLANT_PRESSURE)
  coolantPressureSensor = PressureSensor(0x48, 3, 50);
#endif

#if defined(SPN_1132_INTAKE_MANIFOLD_3_AIR_TEMPERATURE)
  tranferPipeTempSensor =
      TempSensor(10000, AEM_TEMP_SENSOR_A, AEM_TEMP_SENSOR_B, AEM_TEMP_SENSOR_C,
                 0x49, 0, 0, &ossm::isAds2Initialized);
#endif
#if defined(SPN_1128_TURBOCHARGER_2_BOOST_PRESSURE)
  transferPipePressureSensor = PressureSensor(0x49, 1, 58);
#endif
#if defined(SPN_1363_INTAKE_MANIFOLD_1_AIR_TEMPERATURE)
  boostTempSensor =
      TempSensor(10000, AEM_TEMP_SENSOR_A, AEM_TEMP_SENSOR_B, AEM_TEMP_SENSOR_C,
                 0x49, 3, 0, &ossm::isAds2Initialized);
#endif
#if defined(SPN_102_BOOST_PRESSURE)
  boostPressureSensor = PressureSensor(0x49, 2, 150);
#endif

#if defined(SPN_1131_INTAKE_MANIFOLD_2_AIR_TEMPERATURE)
  cacTempSensor =
      TempSensor(10000, AEM_TEMP_SENSOR_A, AEM_TEMP_SENSOR_B, AEM_TEMP_SENSOR_C,
                 0x4A, 1, 0, &ossm::isAds3Initialized);
#endif
#if defined(SPN_1127_TURBOCHARGER_1_BOOST_PRESSURE)
  cacPressureSensor = PressureSensor(0x4A, 0, 150);
#endif
#if defined(SPN_172_AIR_INLET_TEMPERATURE)
  intakeTempSensor =
      TempSensor(10000, AEM_TEMP_SENSOR_A, AEM_TEMP_SENSOR_B, AEM_TEMP_SENSOR_C,
                 0x4A, 2, 0, &ossm::isAds3Initialized);
#endif
#if defined(SPN_102_BOOST_PRESSURE)
  intakePressureSensor = PressureSensor(0x4A, 3, 30);
#endif

#if defined(SPN_174_FUEL_TEMPERATURE)
  fuelTempSensor =
      TempSensor(10000, AEM_TEMP_SENSOR_A, AEM_TEMP_SENSOR_B, AEM_TEMP_SENSOR_C,
                 0x4B, 1, 0, &ossm::isAds4Initialized);
#endif
#if defined(SPN_94_FUEL_DELIVERY_PRESSURE)
  fuelPressureSensor = PressureSensor(0x4B, 0, 100, &ossm::isAds4Initialized);
#endif
#if defined(SPN_441_ENGINE_BAY_TEMPERATURE)
  engineBayTempSensor =
      TempSensor(10000, AEM_TEMP_SENSOR_A, AEM_TEMP_SENSOR_B, AEM_TEMP_SENSOR_C,
                 0x4B, 2, 0, &ossm::isAds4Initialized);
#endif

#if defined(SPN_173_EXHAUST_GAS_TEMPERATURE)
  if (!thermocouple.begin()) {
    Serial.println("ERROR INITING THERMOCOUPLE");
  } else {
    ossm::isEgtInitialized = true;
    Serial.println("Thermocouple initialized");
  }
#endif

#if defined(SPN_175_ENGINE_OIL_TEMPERATURE) || defined(SPN_100_ENGINE_OIL_PRESSURE) || defined(SPN_110_ENGINE_COOLANT_TEMPERATURE) || defined(SPN_109_COOLANT_PRESSURE)
  Serial.println("isAds1Initialized->" + String(ossm::isAds1Initialized));
#endif
#if defined(SPN_102_BOOST_PRESSURE) || defined(SPN_1128_TURBOCHARGER_2_BOOST_PRESSURE) || defined(SPN_1132_INTAKE_MANIFOLD_3_AIR_TEMPERATURE) || defined(SPN_1363_INTAKE_MANIFOLD_1_AIR_TEMPERATURE)
  Serial.println("isAds2Initialized->" + String(ossm::isAds2Initialized));
#endif
#if defined(SPN_172_AIR_INLET_TEMPERATURE) || defined(SPN_1127_TURBOCHARGER_1_BOOST_PRESSURE) || defined(SPN_1131_INTAKE_MANIFOLD_2_AIR_TEMPERATURE) || defined(SPN_106_AIR_INLET_PRESSURE) || defined(SPN_1133_INTAKE_MANIFOLD_4_AIR_TEMPERATURE)
  Serial.println("isAds3Initialized->" + String(ossm::isAds3Initialized));
#endif
#if defined(SPN_94_FUEL_DELIVERY_PRESSURE) || defined(SPN_174_FUEL_TEMPERATURE) || defined(SPN_441_ENGINE_BAY_TEMPERATURE)
  Serial.println("isAds4Initialized->" + String(ossm::isAds4Initialized));
#endif
  Serial.println("isEgtInitialized->" + String(ossm::isEgtInitialized));
#if defined(SPN_171_AMBIENT_AIR_TEMP) || defined(SPN_108_BAROMETRIC_PRESSURE)
  Serial.println("isBmeInitialized->" + String(ossm::isBmeInitialized));
#endif
}

void ossm::loop() {
  thisMillis = millis();

#if defined(SPN_171_AMBIENT_AIR_TEMP) || defined(SPN_108_BAROMETRIC_PRESSURE) || defined(SPN_354_RELATIVE_HUMIDITY)
  if (ossm::isBmeInitialized == true) {
    ossm::appData.absoluteBarometricpressurekPa =
        ossm::ambientSensors->getPressurekPa();
    ossm::appData.humidity = ossm::ambientSensors->getHumidity();
    ossm::appData.ambientTemperatureC = ossm::ambientSensors->getTemperatureC();
  }
#endif

#if defined(SPN_175_ENGINE_OIL_TEMPERATURE) || defined(SPN_100_ENGINE_OIL_PRESSURE) || defined(SPN_110_ENGINE_COOLANT_TEMPERATURE) || defined(SPN_109_COOLANT_PRESSURE)
  if (ossm::isAds1Initialized) {
    ossm::appData.oilTemperatureC = oilTempSensor.getTempCelsius();
    ossm::appData.oilPressurekPa = oilPressureSensor.getPressureInkPa();
    ossm::appData.coolantTemperatureC = coolantTempSensor.getTempCelsius();
    ossm::appData.coolantPressurekPa = coolantPressureSensor.getPressureInkPa();
  }
#endif
#if defined(SPN_102_BOOST_PRESSURE) || defined(SPN_1128_TURBOCHARGER_2_BOOST_PRESSURE) || defined(SPN_1132_INTAKE_MANIFOLD_3_AIR_TEMPERATURE) || defined(SPN_1363_INTAKE_MANIFOLD_1_AIR_TEMPERATURE)
  if (ossm::isAds2Initialized == true) {
    ossm::appData.tranferPipePressurekPa =
        transferPipePressureSensor.getPressureInkPa();
    ossm::appData.transferPipeTemperatureC =
        tranferPipeTempSensor.getTempCelsius();
#if defined(SPN_102_BOOST_PRESSURE)
    ossm::appData.boostPressurekPa = boostPressureSensor.getPressureInkPa();
#endif
    ossm::appData.boostTemperatureC = boostTempSensor.getTempCelsius();
  }
#endif

#if defined(SPN_172_AIR_INLET_TEMPERATURE) || defined(SPN_1127_TURBOCHARGER_1_BOOST_PRESSURE) || defined(SPN_1131_INTAKE_MANIFOLD_2_AIR_TEMPERATURE) || defined(SPN_106_AIR_INLET_PRESSURE) || defined(SPN_1133_INTAKE_MANIFOLD_4_AIR_TEMPERATURE)
  if (ossm::isAds3Initialized == true) {
    ossm::appData.cacInletPressurekPa = cacPressureSensor.getPressureInkPa();
    ossm::appData.cacInletTemperatureC = cacTempSensor.getTempCelsius();
    ossm::appData.airInletPressurekPa = intakePressureSensor.getPressureInkPa();
    ossm::appData.airInletTemperatureC = intakeTempSensor.getTempCelsius();
  }
#endif

#if defined(SPN_94_FUEL_DELIVERY_PRESSURE) || defined(SPN_174_FUEL_TEMPERATURE) || defined(SPN_441_ENGINE_BAY_TEMPERATURE)
  if (ossm::isAds4Initialized == true) {
#if defined(SPN_94_FUEL_DELIVERY_PRESSURE)
    ossm::appData.fuelPressurekPa = fuelPressureSensor.getPressureInkPa();
#endif
#if defined(SPN_174_FUEL_TEMPERATURE)
    ossm::appData.fuelTemperatureC = fuelTempSensor.getTempCelsius();
#endif
#if defined(SPN_441_ENGINE_BAY_TEMPERATURE)
    ossm::appData.engineBayTemperatureC = engineBayTempSensor.getTempCelsius();
#endif
  }
#endif

#if defined(SPN_173_EXHAUST_GAS_TEMPERATURE)
  if (ossm::isEgtInitialized) {
    ossm::appData.egtTemperatureC = thermocouple.readCelsius();
  }
#endif

  // Every .5 seconds
  if (lastHalfSecondMillis - thisMillis >= 500) {
#if defined(SPN_173_EXHAUST_GAS_TEMPERATURE) || defined(SPN_102_BOOST_PRESSURE) || defined(SPN_106_AIR_INLET_PRESSURE) || defined(SPN_1363_INTAKE_MANIFOLD_1_AIR_TEMPERATURE)
    J1939Bus::sendPgn65270(
        ossm::appData.airInletPressurekPa, ossm::appData.boostTemperatureC,
        ossm::appData.egtTemperatureC, ossm::appData.boostPressurekPa);
#endif
#if defined(SPN_94_FUEL_DELIVERY_PRESSURE) || defined(SPN_100_ENGINE_OIL_PRESSURE) || defined(SPN_109_COOLANT_PRESSURE)
    J1939Bus::sendPgn65263(ossm::appData.fuelPressurekPa,
                           ossm::appData.oilPressurekPa,
                           ossm::appData.coolantPressurekPa);
#endif
#if defined(SPN_1127_TURBOCHARGER_1_BOOST_PRESSURE) || defined(SPN_1128_TURBOCHARGER_2_BOOST_PRESSURE)
    J1939Bus::sendPgn65190(ossm::appData.cacInletPressurekPa,
                           ossm::appData.tranferPipePressurekPa);
#endif
    lastHalfSecondMillis = thisMillis;
  }

  // Every 1 second
  if (lastOneSecondMillis - thisMillis >= 1000) {
    if (ossm::isBmeInitialized == true) {
      // Serial.println("AppData: Barometric Pressure->" +
      //                String(ossm::appData.absoluteBarometricpressurekPa) +
      //                "hPa, Humidity->" + String(ossm::appData.humidity) +
      //                "%, Ambient Temperature->" +
      //                String(ossm::appData.ambientTemperatureC) + "°C");
    }

    if (ossm::isAds1Initialized == true) {
      // Serial.println(
      //     "AppData: Oil Temperature->" +
      //     String(ossm::appData.oilTemperatureC) + "°C, Oil Pressure->" +
      //     String(ossm::appData.oilPressurekPa) + "kPa, Coolant Temperature->"
      //     + String(ossm::appData.coolantTemperatureC) + "°C, Coolant
      //     Pressure->" + String(ossm::appData.coolantPressurekPa) + "kPa");
    }

    if (ossm::isAds2Initialized == true) {
      // Serial.println("AppData: Transmission Pressure->" +
      //                String(ossm::appData.tranferPipePressurekPa) +
      //                "kPa, Transmission Temperature->" +
      //                String(ossm::appData.transferPipeTemperatureC) +
      //                "°C, Boost Pressure->" +
      //                String(ossm::appData.boostPressurekPa) +
      //                "kPa, Boost Temperature->" +
      //                String(ossm::appData.boostTemperatureC) + "°C");
    }

    if (ossm::isAds3Initialized == true) {
      // Serial.println(
      //     "AppData: CAC Pressure->" +
      //     String(ossm::appData.cacInletPressurekPa) + "kPa, CAC
      //     Temperature->" + String(ossm::appData.cacInletTemperatureC) + "°C,
      //     Intake Pressure->" + String(ossm::appData.airInletPressurekPa) +
      //     "kPa, Intake Temperature->" +
      //     String(ossm::appData.airInletTemperatureC) + "°C");
    }

    if (ossm::isAds4Initialized == true) {
      // Serial.println(
      //     "AppData: Fuel Pressure->" + String(ossm::appData.fuelPressurekPa)
      //     + "kPa, Fuel Temperature->" +
      //     String(ossm::appData.fuelTemperatureC) + "°C, Engine Bay Temperature->" + String(ossm::appData.engineBayTemperatureC) +
      //     "°C");
    }
#if defined(SPN_173_EXHAUST_GAS_TEMPERATURE)
    // Serial.println("EGT??? " + (String)ossm::isEgtInitialized);
    // Serial.println("EGT? " + (String)ossm::appData.egtTemperatureC);
#endif
#if defined(SPN_171_AMBIENT_AIR_TEMP) || defined(SPN_172_AIR_INLET_TEMPERATURE) || defined(SPN_108_BAROMETRIC_PRESSURE)
    J1939Bus::sendPgn65269(ossm::appData.ambientTemperatureC,
                           ossm::appData.airInletTemperatureC,
                           ossm::appData.absoluteBarometricpressurekPa);
#endif                           
#if defined(SPN_1363_INTAKE_MANIFOLD_1_AIR_TEMPERATURE) || defined(SPN_110_ENGINE_COOLANT_TEMPERATURE)
    J1939Bus::sendPgn65129(ossm::appData.boostTemperatureC,
                           ossm::appData.coolantTemperatureC);
#endif
#if defined(SPN_110_ENGINE_COOLANT_TEMPERATURE) || defined(SPN_174_FUEL_TEMPERATURE) || defined(SPN_175_ENGINE_OIL_TEMPERATURE)
    J1939Bus::sendPgn65262(ossm::appData.coolantTemperatureC,
                           ossm::appData.fuelTemperatureC,
                           ossm::appData.oilTemperatureC);
#endif
#if defined(SPN_1131_INTAKE_MANIFOLD_2_AIR_TEMPERATURE) || defined(SPN_1132_INTAKE_MANIFOLD_3_AIR_TEMPERATURE) || defined(SPN_1133_INTAKE_MANIFOLD_4_AIR_TEMPERATURE)
    J1939Bus::sendPgn65189(ossm::appData.cacInletTemperatureC,
                           ossm::appData.transferPipeTemperatureC,
                           ossm::appData.airInletTemperatureC);
#endif

    lastOneSecondMillis = thisMillis;
  }
}