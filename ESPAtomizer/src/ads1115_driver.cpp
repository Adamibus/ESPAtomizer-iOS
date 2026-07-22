/*
 * ADS1115 I2C Thermocouple Driver - Implementation
 */

#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "ads1115_driver.h"

static bool ads1115_initialized = false;
static uint8_t ads1115_i2c_addr = ADS1115_I2C_ADDR;

static bool ads1115_select_address(uint8_t addr)
{
    ads1115_i2c_addr = addr;
    return true;
}

static bool ads1115_probe_addresses()
{
    const uint8_t possible[] = { ADS1115_I2C_ADDR, 0x49, 0x4A, 0x4B };
    for (uint8_t addr : possible) {
        Wire.beginTransmission(addr);
        int result = Wire.endTransmission();
        if (result == 0) {
            if (addr != ads1115_i2c_addr) {
                Serial.printf("[ADS1115] Found device at alternate address 0x%02X\n", addr);
            }
            ads1115_i2c_addr = addr;
            return true;
        }
        Serial.printf("[ADS1115] probe address 0x%02X returned code=%d\n", addr, result);
    }
    return false;
}

#if EXTERNAL_ADS1115_FALLBACK
#if defined(__has_include)
    #if __has_include(EXTERNAL_ADS1115_HEADER)
        #include EXTERNAL_ADS1115_HEADER
        #define EXTERNAL_ADS1115_LIB_AVAILABLE 1
    #elif __has_include(<Adafruit_ADS1X15.h>)
        #include <Adafruit_ADS1X15.h>
        #define EXTERNAL_ADS1115_LIB_AVAILABLE 1
        #define EXTERNAL_ADS1115_TYPE Adafruit_ADS1115
        #define EXTERNAL_ADS_LIB_ADAFRUIT 1
    #elif __has_include(<ADS1115_WE.h>)
        #include <ADS1115_WE.h>
        #define EXTERNAL_ADS1115_LIB_AVAILABLE 1
        #define EXTERNAL_ADS1115_TYPE ADS1115_WE
        #define EXTERNAL_ADS_LIB_WE 1
    #elif __has_include(<ads1115.h>)
        #include <ads1115.h>
        #define EXTERNAL_ADS1115_LIB_AVAILABLE 1
        #define EXTERNAL_ADS1115_TYPE ADS1115
        #define EXTERNAL_ADS_LIB_GENERIC 1
    #endif
#endif
#endif

#if defined(EXTERNAL_ADS1115_LIB_AVAILABLE) && defined(EXTERNAL_ADS1115_TYPE)
static EXTERNAL_ADS1115_TYPE extAds;
static bool extAdsInitialized = false;
#endif

static bool tryExternalAdsInitialization()
{
#if defined(EXTERNAL_ADS1115_LIB_AVAILABLE) && defined(EXTERNAL_ADS1115_TYPE)
    if (extAdsInitialized) return true;
    Serial.println(F("[ADS1115] Trying external ADS1115 library fallback..."));
    bool ok = false;
  #if defined(EXTERNAL_ADS_LIB_ADAFRUIT)
    ok = extAds.begin();
  #elif defined(EXTERNAL_ADS_LIB_WE)
    ok = extAds.init();
  #else
    ok = extAds.begin();
  #endif
    if (ok) {
        extAdsInitialized = true;
        ads1115_initialized = true;
        Serial.println(F("[ADS1115] External ADS1115 library initialized as fallback"));
        return true;
    }
    Serial.println(F("[ADS1115] External ADS1115 fallback failed"));
#endif
    return false;
}

bool ads1115_init()
{
    // Assume I2C/Wire was initialized by board setup (setup() calls Wire.begin())
    // Use the same reliable 100 kHz bus speed as the diagnostic scan.
    // Some ADS1115 modules or wiring do not tolerate 400 kHz on this board.
    Wire.setClock(100000);  // 100 kHz I2C speed

    bool present = ads1115_is_present();
    if (!present) {
        Serial.printf("[ADS1115] WARNING: Device not found at default address 0x%02X\n", ADS1115_I2C_ADDR);
        if (ads1115_probe_addresses()) {
            Serial.printf("[ADS1115] Using alternate I2C address 0x%02X\n", ads1115_i2c_addr);
            present = true;
        }
    }

    if (!present) {
        if (tryExternalAdsInitialization()) {
            return true;
        }
        return false;
    }

    uint16_t config = ADS1115_MUX_AIN0_GND      // Channel 0 (AIN0 vs GND)
                    | ADS1115_PGA_4_096V         // ±4.096V range
                    | ADS1115_MODE_SINGLE        // Single-shot mode
                    | ADS1115_RATE_128SPS        // 128 samples/second
                    | ADS1115_COMP_TRAD          // Traditional comparator
                    | ADS1115_COMP_ACTVLOW       // Active low
                    | ADS1115_COMP_NOLOCK        // Non-latching
                    | ADS1115_CQUE_NONE;         // Comparator disabled

    if (!ads1115_write_register(ADS1115_REG_CONFIG, config)) {
        Serial.println("[ADS1115] WARNING: Failed to write config register to built-in ADS1115");
        if (tryExternalAdsInitialization()) {
            return true;
        }
        return false;
    }

    ads1115_initialized = true;
    Serial.printf("[ADS1115] Initialized successfully at 0x%02X\n", ads1115_i2c_addr);
    return true;
}

int16_t ads1115_read_raw()
{
    if (!ads1115_initialized) {
        if (tryExternalAdsInitialization()) {
#if defined(EXTERNAL_ADS1115_LIB_AVAILABLE) && defined(EXTERNAL_ADS1115_TYPE)
            int16_t v = extAds.readADC_SingleEnded(ADS1115_CHANNEL);
            return v;
#else
            Serial.println(F("[ADS1115] ERROR: External ADS1115 fallback is not available for raw reads"));
            return 0;
#endif
        }
#if defined(EXTERNAL_ADS1115_LIB_AVAILABLE) && defined(EXTERNAL_ADS1115_TYPE)
        if (extAdsInitialized) {
#if defined(EXTERNAL_ADS_LIB_ADAFRUIT)
            int16_t v = extAds.readADC_SingleEnded(ADS1115_CHANNEL);
            return v;
#elif defined(EXTERNAL_ADS_LIB_WE)
            int16_t v = extAds.getRawResult(ADS1115_CHANNEL);
            return v;
#else
            int16_t v = extAds.readADC_SingleEnded(ADS1115_CHANNEL);
            return v;
#endif
        }
#endif
        Serial.println(F("[ADS1115] ERROR: Driver not initialized"));
        return 0;
    }

    uint16_t config = ads1115_read_register(ADS1115_REG_CONFIG);
    config |= ADS1115_OS_SINGLE;  // Start single conversion
    ads1115_write_register(ADS1115_REG_CONFIG, config);

    uint32_t timeout = millis() + 100;
    while (millis() < timeout) {
        uint16_t status = ads1115_read_register(ADS1115_REG_CONFIG);
        // OS bit set when conversion complete (0x8000). Break when set.
        if ((status & 0x8000) != 0) break;  // OS bit set when done
        delay(1);
    }
    if (millis() >= timeout) {
        Serial.println(F("[ADS1115] WARNING: Conversion timeout"));
    }

    return (int16_t)ads1115_read_register(ADS1115_REG_CONVERSION);
}

float ads1115_read_temperature()
{
    if (!ads1115_initialized) {
        if (tryExternalAdsInitialization()) {
#if defined(EXTERNAL_ADS1115_LIB_AVAILABLE) && defined(EXTERNAL_ADS1115_TYPE)
            int32_t raw = 0;
  #if defined(EXTERNAL_ADS_LIB_ADAFRUIT)
            raw = extAds.readADC_SingleEnded(ADS1115_CHANNEL);
  #elif defined(EXTERNAL_ADS_LIB_WE)
            raw = extAds.getRawResult(ADS1115_CHANNEL);
  #else
            raw = extAds.readADC_SingleEnded(ADS1115_CHANNEL);
  #endif
            float voltage_mv = (float)raw * (ADS1115_VREF_MV / 32767.0f);
            float temperature_c = voltage_mv * 1000.0f / 41.0f;
            temperature_c += ADS1115_TC_OFFSET_THERMOCOUPLE;
            return temperature_c;
#else
            Serial.println(F("[ADS1115] ERROR: External ADS1115 fallback is not available for temperature reads"));
            return NAN;
#endif
        }
#if defined(EXTERNAL_ADS1115_LIB_AVAILABLE) && defined(EXTERNAL_ADS1115_TYPE)
        if (extAdsInitialized) {
  #if defined(EXTERNAL_ADS_LIB_ADAFRUIT)
            int32_t raw = extAds.readADC_SingleEnded(ADS1115_CHANNEL);
  #elif defined(EXTERNAL_ADS_LIB_WE)
            int32_t raw = extAds.getRawResult(ADS1115_CHANNEL);
  #else
            int32_t raw = extAds.readADC_SingleEnded(ADS1115_CHANNEL);
  #endif
            float voltage_mv = (float)raw * (ADS1115_VREF_MV / 32767.0f);
            float temperature_c = voltage_mv * 1000.0f / 41.0f;
            temperature_c += ADS1115_TC_OFFSET_THERMOCOUPLE;
            return temperature_c;
        }
#endif
        Serial.println(F("[ADS1115] ERROR: Driver not initialized"));
        return NAN;
    }

    int32_t sum = 0;
    for (int i = 0; i < ADS1115_SAMPLES; i++) {
        sum += ads1115_read_raw();
        delay(10);
    }
    int16_t raw_avg = sum / ADS1115_SAMPLES;

    float voltage_mv = (float)raw_avg * (ADS1115_VREF_MV / 32767.0f);
    float temperature_c = voltage_mv * 1000.0f / 41.0f;  // 41 µV/°C
    temperature_c += ADS1115_TC_OFFSET_THERMOCOUPLE;
    return temperature_c;
}

bool ads1115_write_register(uint8_t reg, uint16_t val)
{
    Wire.beginTransmission(ads1115_i2c_addr);
    Wire.write(reg);
    Wire.write((uint8_t)(val >> 8));
    Wire.write((uint8_t)(val & 0xFF));
    int result = Wire.endTransmission();
    if (result != 0) {
        Serial.printf("[ADS1115] I2C write error (addr=0x%02X, reg=0x%02X, code=%d)\n", ads1115_i2c_addr, reg, result);
        return false;
    }
    return true;
}

uint16_t ads1115_read_register(uint8_t reg)
{
    Wire.beginTransmission(ads1115_i2c_addr);
    Wire.write(reg);
    int result = Wire.endTransmission();
    if (result != 0) {
        Serial.printf("[ADS1115] I2C write error (addr=0x%02X, reg=0x%02X, code=%d)\n", ads1115_i2c_addr, reg, result);
        return 0;
    }

    Wire.requestFrom((uint8_t)ads1115_i2c_addr, (uint8_t)2);
    int avail = Wire.available();
    if (avail < 2) {
        Serial.printf("[ADS1115] I2C read error: not enough bytes (available=%d)\n", avail);
        return 0;
    }

    uint8_t msb = Wire.read();
    uint8_t lsb = Wire.read();
    return ((uint16_t)msb << 8) | lsb;
}

bool ads1115_is_present()
{
    Wire.beginTransmission(ads1115_i2c_addr);
    int result = Wire.endTransmission();
    if (result != 0) {
        Serial.printf("[ADS1115] probe I2C error: code=%d (addr=0x%02X)\n", result, ads1115_i2c_addr);
    }
    return (result == 0);
}
