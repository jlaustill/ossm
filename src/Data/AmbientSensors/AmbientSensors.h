
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <SPI.h>
#include <Wire.h>

class AmbientSensors {
 public:
  AmbientSensors(bool *isInitialized);
  ~AmbientSensors();

  void printValues();

  float getTemperatureC();
  float getHumidity();
  float getPressureHPa();

 private:
  Adafruit_BME280 bme;
};
