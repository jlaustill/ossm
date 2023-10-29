//
// Created by jlaustill on 4/6/22.
//

#ifndef OPCM_TEMPSENSOR_H
#define OPCM_TEMPSENSOR_H
#include <Adafruit_ADS1X15.h>


class TempSensor {
public:
    TempSensor(float knownResistorValue, float a, float b, float c, uint8_t deviceId, uint8_t channelId, float wiringResistance) {
        Serial.println("Starting to initialize ADS. devicedId: " + (String)deviceId);
        KnownResistorValue = knownResistorValue;
        WiringResistance = wiringResistance;
        A = a;
        B = b;
        C = c;

        DeviceId = deviceId;
        ChannelId = channelId;

        if (!ads.begin(DeviceId)) {
            Serial.println("Failed to initialize ADS.");
        } else {
            Serial.println("Succesfuly initialized ADS.");
        }
    }

    TempSensor() {

    }

    float getTempKelvin();
    float getTempCelsius();
    float getTempFahrenheit();

private:
    float KnownResistorValue;
    float WiringResistance;
    uint8_t DeviceId;
    uint8_t ChannelId;
    float A;
    float B;
    float C;


    float readVoltage;
    float computedResistorValue;
    float kelvin;
    void updateSensor();
    void computeKelvin();
    void computeResistorValue();
    Adafruit_ADS1115 ads;
};


#endif //OPCM_TEMPSENSOR_H