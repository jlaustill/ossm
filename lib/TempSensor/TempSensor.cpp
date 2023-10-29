//
// Created by jlaustill on 4/6/22.
// Updated on 5/4/23

#include "TempSensor.h"
#include <Arduino.h>

void TempSensor::computeKelvin() {
    this->kelvin = (1.0f /
            (this->A +
             this->B * log(TempSensor::computedResistorValue) +
             this->C * pow(log(TempSensor::computedResistorValue), 3)
            ));
}

void TempSensor::updateSensor() {
    this->readVoltage = ads.computeVolts(ads.readADC_SingleEnded(ChannelId));
    this->computeResistorValue();
    this->computeKelvin();
}

float TempSensor::getTempKelvin() {
    this->updateSensor();
    return this->kelvin;
}

float TempSensor::getTempCelsius() {
    this->updateSensor();
    return this->kelvin - 273.15f;
}

float TempSensor::getTempFahrenheit() {
    this->updateSensor();
    return ((this->kelvin - 273.15f) * 9.0f) / 5.0f + 32.0f;
}

void TempSensor::computeResistorValue() {
    // This tests accurate 
    // 328.4 -> 326 measured
    // 994 -> 988 measured
    // 92,600 -> 92,500 measured
    // I think this is within the error rate of my voltmeter
    // REALLY need an accurate 3.3vref, need to figure out how to measure that to keep this consistent with voltage sways
    this->computedResistorValue = (this->readVoltage * this->KnownResistorValue) / (3.3225f - this->readVoltage);
    this->computedResistorValue -= this->WiringResistance; // adjusting for resistance in the wiring maybe?
    // Serial.println("computed resistance value? " + (String)this->computedResistorValue + " ReadVoltage? " + (String)this->readVoltage + " wiring resistance? " + (String)this->WiringResistance);
}