#ifndef OSSM_J1939Bus_H
#define OSSM_J1939Bus_H

#include <J1939.h>

class J1939Bus {
    public:
        static void initialize();
        static void sendPgn65269(float ambientTemperatureC, float airInletTemperatureC, float barometricPressurekPa);
        static void sendPgn65270(float airInletPressurekPa, float airInletTemperatureC, float egtTemperatureC, float boostPressurekPa);
        static void sendPgn65262(float engineCoolantTemperature, float engineFuelTemperature, float engineOilTemperature);
};


#endif