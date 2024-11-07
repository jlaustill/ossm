#ifndef OSSM_J1939Bus_H
#define OSSM_J1939Bus_H

#include <J1939.h>

class J1939Bus {
    public:
        static void initialize();
        static void sendPgn65269(float ambientTemperatureC, float airInletTemperatureC, float barometricPressureHpa);
};


#endif