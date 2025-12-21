#ifndef OSSM_SERIALCOMMANDHANDLER_H
#define OSSM_SERIALCOMMANDHANDLER_H

#include "AppConfig.h"
#include "AppData.h"
#include <SeaDash.hpp>

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
    static SeaDash::Parse::ParseResult parsed;

    // Process a complete command string
    static void processCommand(const char* cmd);

    // Command handlers (byte format: all params are uint8_t, max 8 bytes total)
    // Format: cmd,param1,param2,... where 16-bit values use highByte,lowByte
    static void handleEnableSpn();       // Cmd 1: spnHi,spnLo,enable,input
    static void handleSetPressureRange();// Cmd 3: input,psiHi,psiLo
    static void handleSetTcType();       // Cmd 4: type
    static void handleQuery();           // Cmd 5: queryType
    static void handleSave();            // Cmd 6: (no params)
    static void handleReset();           // Cmd 7: (no params)
    static void handleNtcPreset();       // Cmd 8: input,preset
    static void handlePressurePreset();  // Cmd 9: input,preset
};

#endif  // OSSM_SERIALCOMMANDHANDLER_H
