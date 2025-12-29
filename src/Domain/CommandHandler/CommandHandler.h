#ifndef OSSM_COMMANDHANDLER_H
#define OSSM_COMMANDHANDLER_H

#include <stdint.h>
#include "AppConfig.h"
#include "AppData.h"

// Result struct for command operations
struct TCommandResult {
    uint8_t errorCode;      // 0 = OK, 1-12 = error codes
    uint8_t data[6];        // Optional response data
    uint8_t dataLen;        // Length of response data

    // Convenience constructor for simple OK/error responses
    TCommandResult() : errorCode(0), dataLen(0) {
        for (int i = 0; i < 6; i++) data[i] = 0;
    }

    static TCommandResult ok() {
        return TCommandResult();
    }

    static TCommandResult error(uint8_t code) {
        TCommandResult r;
        r.errorCode = code;
        return r;
    }

    static TCommandResult withData(const uint8_t* d, uint8_t len) {
        TCommandResult r;
        r.dataLen = (len > 6) ? 6 : len;
        for (uint8_t i = 0; i < r.dataLen; i++) {
            r.data[i] = d[i];
        }
        return r;
    }
};

// Error codes (preserved from existing J1939 implementation)
namespace ECommandError {
    constexpr uint8_t OK = 0;
    constexpr uint8_t UNKNOWN_COMMAND = 1;
    constexpr uint8_t PARSE_FAILED = 2;
    constexpr uint8_t UNKNOWN_SPN = 3;
    constexpr uint8_t INVALID_TEMP_INPUT = 4;
    constexpr uint8_t INVALID_PRESSURE_INPUT = 5;
    constexpr uint8_t INVALID_NTC_PARAM = 6;
    constexpr uint8_t INVALID_TC_TYPE = 7;
    constexpr uint8_t INVALID_QUERY_TYPE = 8;
    constexpr uint8_t SAVE_FAILED = 9;
    constexpr uint8_t INVALID_PRESET = 10;
}

class CommandHandler {
public:
    // Initialize with config and data pointers
    static void initialize(AppConfig* cfg, AppData* data);

    // Core command methods (return TCommandResult)
    static TCommandResult enableSpn(uint16_t spn, bool enable, uint8_t input);
    static TCommandResult setPressureRange(uint8_t input, uint16_t maxPsi);
    static TCommandResult setTcType(uint8_t type);
    static TCommandResult applyNtcPreset(uint8_t input, uint8_t preset);
    static TCommandResult applyPressurePreset(uint8_t input, uint8_t preset);
    static TCommandResult setNtcParam(uint8_t input, uint8_t param, float value);
    static TCommandResult save();
    static TCommandResult reset();
    static TCommandResult querySpnCounts();
    static TCommandResult queryFullConfig();
    static TCommandResult queryTempSpns(uint8_t subQuery);
    static TCommandResult queryPresSpns(uint8_t subQuery);

    // Getters for config/data (for handlers that need direct access)
    static AppConfig* getConfig() { return config; }
    static AppData* getAppData() { return appData; }

private:
    static AppConfig* config;
    static AppData* appData;
};

#endif  // OSSM_COMMANDHANDLER_H
