#ifndef OSSM_SERIALCOMMANDHANDLER_H
#define OSSM_SERIALCOMMANDHANDLER_H

#include "AppConfig.h"
#include "AppData.h"

class SerialCommandHandler {
   public:
    // Initialize with config and data pointers
    static void initialize(AppConfig* config, AppData* data);

    // Call from main loop to process incoming serial commands
    static void update();

   private:
    static AppConfig* config;
    static AppData* appData;

    // Command buffer
    static char cmdBuffer[128];
    static uint8_t cmdIndex;

    // Process a complete command line
    static void processCommand(const char* cmd);

    // Command handlers
    static void cmdHelp();
    static void cmdStatus();
    static void cmdConfig();
    static void cmdSave();
    static void cmdReset();
    static void cmdSetSpn(const char* args);
    static void cmdSetSensor(const char* args);
    static void cmdSetAddress(const char* args);

    // Helper to print SPN status
    static void printSpnStatus();

    // Helper to print sensor config
    static void printSensorConfig();
};

#endif  // OSSM_SERIALCOMMANDHANDLER_H
