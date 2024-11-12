#include "./AmbientSensors.h"

AmbientSensors::AmbientSensors(bool *isInitialized) {
  unsigned status;

  status = bme.begin(0x76);
  if (!status) {
    Serial.println(
        "Could not find a valid BME280 sensor, check wiring, address, sensor "
        "ID!");
    Serial.print("SensorID was: 0x");
    Serial.println(bme.sensorID(), 16);
    Serial.print(
        "        ID of 0xFF probably means a bad address, a BMP 180 or BMP "
        "085\n");
    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("        ID of 0x60 represents a BME 280.\n");
    Serial.print("        ID of 0x61 represents a BME 680.\n");
  } else {
    *isInitialized = true;
  }
}

void AmbientSensors::printValues() {
  Serial.print("Temperature(PID 15) = ");
  Serial.print(bme.readTemperature());
  Serial.println(" °C");

  Serial.print("Pressure(PID 51) = ");

  Serial.print(bme.readPressure() / 100.0F);
  Serial.println(" hPa");

  Serial.print("Humidity = ");
  Serial.print(bme.readHumidity());
  Serial.println(" %");

  Serial.println();
}

float AmbientSensors::getTemperatureC() { return bme.readTemperature(); }

float AmbientSensors::getHumidity() { return bme.readHumidity(); }

float AmbientSensors::getPressurekPa() { return bme.readPressure() / 1000.0F; }