#ifndef OSSM_APPCONFIG_H
#define OSSM_APPCONFIG_H

#include <stdint.h>

#define CONFIG_MAGIC 0x4F53534D  // "OSSM" in ASCII
#define CONFIG_VERSION 2         // Bumped for new structure

// Number of user-facing inputs
#define TEMP_INPUT_COUNT 8
#define PRESSURE_INPUT_COUNT 7

// ADS1115 device count (internal, fixed)
#define ADS_DEVICE_COUNT 4

// Thermocouple types (MAX31856)
enum EThermocoupleType : uint8_t {
    TC_TYPE_B = 0,
    TC_TYPE_E = 1,
    TC_TYPE_J = 2,
    TC_TYPE_K = 3,  // Default
    TC_TYPE_N = 4,
    TC_TYPE_R = 5,
    TC_TYPE_S = 6,
    TC_TYPE_T = 7
};

// NTC Presets
enum ENtcPreset : uint8_t {
    NTC_PRESET_AEM = 0,
    NTC_PRESET_BOSCH = 1,
    NTC_PRESET_GM = 2
};

// Pressure Presets
enum EPressurePreset : uint8_t {
    PRESSURE_PRESET_100PSI = 0,
    PRESSURE_PRESET_150PSI = 1,
    PRESSURE_PRESET_200PSI = 2
};

// Temperature input configuration (temp1-temp8)
struct TTempInputConfig {
    uint16_t assignedSpn;     // Which SPN this input feeds (0 = disabled)
    float coeffA;             // Steinhart-Hart A coefficient
    float coeffB;             // Steinhart-Hart B coefficient
    float coeffC;             // Steinhart-Hart C coefficient
    float resistorValue;      // Reference resistor in voltage divider (ohms)
};

// Pressure input configuration (pres1-pres7)
struct TPressureInputConfig {
    uint16_t assignedSpn;     // Which SPN this input feeds (0 = disabled)
    uint16_t maxPsi;          // Maximum pressure in PSI
};

// Main configuration structure (stored in EEPROM)
struct AppConfig {
    // Header
    uint32_t magic;               // CONFIG_MAGIC validates EEPROM contents
    uint8_t version;              // Configuration version for migration
    uint8_t j1939SourceAddress;   // J1939 source address (default 149)
    uint8_t reserved[2];          // Padding for alignment

    // Temperature inputs (user-facing: temp1-temp8)
    TTempInputConfig tempInputs[TEMP_INPUT_COUNT];

    // Pressure inputs (user-facing: pres1-pres7)
    TPressureInputConfig pressureInputs[PRESSURE_INPUT_COUNT];

    // Thermocouple (EGT)
    bool egtEnabled;              // SPN 173 enabled
    EThermocoupleType thermocoupleType;
    uint8_t tcReserved[2];        // Padding

    // BME280 (ambient sensors)
    bool bme280Enabled;           // Enables SPNs 171, 108, 354
    uint8_t bme280Reserved[3];    // Padding

    // CRC32 for validation
    uint32_t checksum;
};

// Default AEM temperature sensor Steinhart-Hart coefficients
#define AEM_TEMP_COEFF_A 1.485995686e-03f
#define AEM_TEMP_COEFF_B 2.279654266e-04f
#define AEM_TEMP_COEFF_C 1.197578033e-07f
#define AEM_TEMP_RESISTOR 10050.0f

// Bosch NTC coefficients (placeholder - need real values)
#define BOSCH_TEMP_COEFF_A 1.40e-03f
#define BOSCH_TEMP_COEFF_B 2.37e-04f
#define BOSCH_TEMP_COEFF_C 9.90e-08f
#define BOSCH_TEMP_RESISTOR 2490.0f

// GM NTC coefficients (placeholder - need real values)
#define GM_TEMP_COEFF_A 1.29e-03f
#define GM_TEMP_COEFF_B 2.35e-04f
#define GM_TEMP_COEFF_C 9.00e-08f
#define GM_TEMP_RESISTOR 3000.0f

// Fixed hardware mapping: tempX -> ADS device/channel
// This is internal, not user-configurable
struct THardwareMapping {
    uint8_t adsDevice;   // 0-3 (0x48-0x4B)
    uint8_t adsChannel;  // 0-3
};

// Temperature input hardware mappings (fixed)
// Index 0 = temp1, Index 7 = temp8
static const THardwareMapping TEMP_HARDWARE_MAP[TEMP_INPUT_COUNT] = {
    {0, 0},  // temp1 -> ADS 0x48, channel 0
    {0, 1},  // temp2 -> ADS 0x48, channel 1
    {1, 0},  // temp3 -> ADS 0x49, channel 0
    {1, 1},  // temp4 -> ADS 0x49, channel 1
    {2, 1},  // temp5 -> ADS 0x4A, channel 1
    {2, 2},  // temp6 -> ADS 0x4A, channel 2
    {3, 1},  // temp7 -> ADS 0x4B, channel 1
    {3, 2},  // temp8 -> ADS 0x4B, channel 2
};

// Pressure input hardware mappings (fixed)
// Index 0 = pres1, Index 6 = pres7
static const THardwareMapping PRESSURE_HARDWARE_MAP[PRESSURE_INPUT_COUNT] = {
    {0, 2},  // pres1 -> ADS 0x48, channel 2
    {0, 3},  // pres2 -> ADS 0x48, channel 3
    {2, 0},  // pres3 -> ADS 0x4A, channel 0
    {2, 3},  // pres4 -> ADS 0x4A, channel 3
    {1, 2},  // pres5 -> ADS 0x49, channel 2
    {1, 3},  // pres6 -> ADS 0x49, channel 3
    {3, 0},  // pres7 -> ADS 0x4B, channel 0
};

// ADS1115 device I2C addresses (fixed)
static const uint8_t ADS_I2C_ADDRESSES[4] = {0x48, 0x49, 0x4A, 0x4B};

// ADS1115 DRDY pins (fixed)
static const uint8_t ADS_DRDY_PINS[4] = {0, 1, 4, 5};  // D0, D1, D4, D5

// Known SPNs and their categories
enum ESpnCategory : uint8_t {
    SPN_CAT_TEMPERATURE,    // Requires temp input 1-8
    SPN_CAT_PRESSURE,       // Requires pressure input 1-7
    SPN_CAT_EGT,            // Uses thermocouple
    SPN_CAT_BME280,         // Uses BME280
    SPN_CAT_UNKNOWN
};

// SPN definitions
struct TSpnInfo {
    uint16_t spn;
    ESpnCategory category;
    uint16_t hiResSpn;  // Auto-enabled hi-res SPN (0 if none)
};

// Known SPNs lookup table
static const TSpnInfo KNOWN_SPNS[] = {
    // Temperature SPNs (require temp input)
    {175, SPN_CAT_TEMPERATURE, 0},      // Engine Oil Temperature
    {110, SPN_CAT_TEMPERATURE, 1637},   // Engine Coolant Temp (+hi-res 1637)
    {174, SPN_CAT_TEMPERATURE, 0},      // Fuel Temperature
    {105, SPN_CAT_TEMPERATURE, 1363},   // Intake Manifold 1 Temp (+hi-res 1363)
    {1131, SPN_CAT_TEMPERATURE, 0},     // Intake Manifold 2 Temp
    {1132, SPN_CAT_TEMPERATURE, 0},     // Intake Manifold 3 Temp
    {1133, SPN_CAT_TEMPERATURE, 0},     // Intake Manifold 4 Temp
    {172, SPN_CAT_TEMPERATURE, 0},      // Air Inlet Temperature
    {441, SPN_CAT_TEMPERATURE, 0},      // Engine Bay Temperature

    // Pressure SPNs (require pressure input)
    {100, SPN_CAT_PRESSURE, 0},         // Engine Oil Pressure
    {109, SPN_CAT_PRESSURE, 0},         // Coolant Pressure
    {94, SPN_CAT_PRESSURE, 0},          // Fuel Delivery Pressure
    {102, SPN_CAT_PRESSURE, 0},         // Boost Pressure
    {106, SPN_CAT_PRESSURE, 0},         // Air Inlet Pressure
    {1127, SPN_CAT_PRESSURE, 0},        // Turbocharger 1 Boost Pressure
    {1128, SPN_CAT_PRESSURE, 0},        // Turbocharger 2 Boost Pressure

    // EGT SPN (thermocouple)
    {173, SPN_CAT_EGT, 0},              // Exhaust Gas Temperature

    // BME280 SPNs (grouped)
    {171, SPN_CAT_BME280, 0},           // Ambient Air Temperature
    {108, SPN_CAT_BME280, 0},           // Barometric Pressure
    {354, SPN_CAT_BME280, 0},           // Relative Humidity
};

#define KNOWN_SPN_COUNT (sizeof(KNOWN_SPNS) / sizeof(KNOWN_SPNS[0]))

#endif  // OSSM_APPCONFIG_H
