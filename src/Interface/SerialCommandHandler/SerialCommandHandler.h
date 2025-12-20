#ifndef OSSM_SERIALCOMMANDHANDLER_H
#define OSSM_SERIALCOMMANDHANDLER_H

#include "AppConfig.h"
#include "AppData.h"

class SerialCommandHandler {
   public:
    // Initialize with config and data pointers
    static void initialize(AppConfig* cfg, AppData* data);

    // Call from main loop to process serial input
    static void update();

   private:
    static AppConfig* config;
    static AppData* appData;
    static char cmdBuffer[128];
    static uint8_t cmdIndex;

    // Process a complete command string
    static void processCommand(const char* cmd);

    // Command handlers (CSV format)
    static void handleEnableSpn();       // Cmd 1: Enable/Disable SPN
    static void handleSetNtcParam();     // Cmd 2: Set NTC calibration param
    static void handleSetPressureRange();// Cmd 3: Set pressure range
    static void handleSetTcType();       // Cmd 4: Set thermocouple type
    static void handleQuery();           // Cmd 5: Query configuration
    static void handleSave();            // Cmd 6: Save to EEPROM
    static void handleReset();           // Cmd 7: Reset to defaults
    static void handleNtcPreset();       // Cmd 8: Apply NTC preset
    static void handlePressurePreset();  // Cmd 9: Apply pressure preset
};

#endif  // OSSM_SERIALCOMMANDHANDLER_H
