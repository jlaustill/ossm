#ifndef OSSM_H
#define OSSM_H

#include <Arduino.h>
#include <AppData.h>
#include <AppConfig.h>

class ossm {
 public:
  static void setup();
  static void loop();

  // Access to shared data
  static AppData* getAppData();
  static AppConfig* getAppConfig();

 private:
  static AppData appData;
  static AppConfig appConfig;

  // IntervalTimer for sensor polling (50ms)
  static IntervalTimer sensorTimer;
  static volatile bool sensorUpdateReady;

  // Timing for J1939 messages
  static elapsedMillis halfSecondMillis;
  static elapsedMillis oneSecondMillis;

  // Sensor timer callback (called from interrupt context)
  static void sensorTimerCallback();

  // Process sensor updates (called from loop)
  static void processSensorUpdates();

  // Send J1939 messages based on timing
  static void sendJ1939Messages();

  // Timer intervals in microseconds
  static constexpr uint32_t SENSOR_TIMER_INTERVAL_US = 50000;  // 50ms
};

#endif  // OSSM_H
