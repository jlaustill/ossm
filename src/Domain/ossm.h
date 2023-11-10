#ifndef OSSM_H
#define OSSM_H

#include <AppData.h>

#include "Data/AmbientSensors/AmbientSensors.h"

class ossm {
 public:
  static void setup();
  static void loop();
  static bool isAds1Initialized;
  static bool isAds2Initialized;
  static bool isAds3Initialized;
  static bool isAds4Initialized;
  static bool isEgtInitialized;
  static bool isBmeInitialized;

 private:
  static AppData appData;
  static AmbientSensors *ambientSensors;
  static const unsigned long delayTime = 1000;
};

#endif  // OSSM_H
