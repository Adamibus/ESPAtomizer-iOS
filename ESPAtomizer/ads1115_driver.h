/*
 * ADS1115 I2C Thermocouple Driver
 * 
 * 16-bit I2C analog-to-digital converter for K-type thermocouple measurement
 * on the ESPAtomizer PCB v3.
 * 
 * Communicates with ADS1115 IC at I2C address 0x48 (ADDR pin tied to GND).
 * Uses GPIO19 (SCL) and GPIO20 (SDA) — shared with OLED display.
 * 
 * K-type thermocouple: ~41 µV/°C, ambient reference built into converter logic.
 * 
 * Features:
 *   - 16-bit resolution ADC
 *   - I2C communication (standard and fast modes)
 *   - Multi-master I2C (coexists with OLED at 0x3C)
 *   - Single-ended or differential measurement
 *   - Programmable gain (PGA) for different voltage ranges
 *   - Conversion complete flag (polling-based reading)
 * 
 * Usage:
 *   ads1115_init();              // Initialize I2C and ADS1115
 *   float temp = ads1115_read_temperature();  // Read temperature in °C
 */

#ifndef ADS1115_DRIVER_H
#define ADS1115_DRIVER_H

#include <Arduino.h>
#include <Wire.h>

// ============================================================================
// Configuration Macros (define in config.h or here)
// ============================================================================

#ifndef ADS1115_I2C_ADDR
#define ADS1115_I2C_ADDR 0x48  // ADDR pin tied to GND → address 0x48
#endif

#ifndef ADS1115_CHANNEL
#define ADS1115_CHANNEL 0      // Channel 0 (AIN0) for single-ended, or 0-3 for differential
#endif

#ifndef ADS1115_VREF_MV
#define ADS1115_VREF_MV 4096.0f   // Internal reference in mV (GAIN_ONE uses ±4.096V)
#endif

#ifndef ADS1115_SAMPLES
#define ADS1115_SAMPLES 4        // Number of samples to average for noise reduction
#endif

// ============================================================================
// ADS1115 Register Addresses
// ============================================================================

#define ADS1115_REG_CONVERSION  0x00  // Conversion result register
#define ADS1115_REG_CONFIG      0x01  // Configuration register
#define ADS1115_REG_LO_THRESH   0x02  // Low threshold register
#define ADS1115_REG_HI_THRESH   0x03  // High threshold register
// Backward-compatible aliases
#define ADS1115_REG_THRESH_LOW  ADS1115_REG_LO_THRESH
#define ADS1115_REG_THRESH_HIGH ADS1115_REG_HI_THRESH

// ============================================================================
// ADS1115 Configuration Bits (for REG_CONFIG, address 0x01)
// ============================================================================

// Bits 15:14 — Input multiplexer (channel selection)
#define ADS1115_MUX_AIN0_AIN1   0x0000  // Differential: AIN0 - AIN1
#define ADS1115_MUX_AIN0_AIN3   0x1000  // Differential: AIN0 - AIN3
#define ADS1115_MUX_AIN1_AIN3   0x2000  // Differential: AIN1 - AIN3
#define ADS1115_MUX_AIN2_AIN3   0x3000  // Differential: AIN2 - AIN3
#define ADS1115_MUX_AIN0_GND    0x4000  // Single-ended: AIN0 vs GND
#define ADS1115_MUX_AIN1_GND    0x5000  // Single-ended: AIN1 vs GND
#define ADS1115_MUX_AIN2_GND    0x6000  // Single-ended: AIN2 vs GND
#define ADS1115_MUX_AIN3_GND    0x7000  // Single-ended: AIN3 vs GND

// Bits 13:11 — Programmable Gain Amplifier (PGA)
#define ADS1115_PGA_6_144V      0x0000  // ±6.144V range
#define ADS1115_PGA_4_096V      0x0200  // ±4.096V range (for 3.3V supplies)
#define ADS1115_PGA_2_048V      0x0400  // ±2.048V range
#define ADS1115_PGA_1_024V      0x0600  // ±1.024V range
#define ADS1115_PGA_0_512V      0x0800  // ±0.512V range
#define ADS1115_PGA_0_256V      0x0A00  // ±0.256V range

// Bits 10:8 — Operation mode
#define ADS1115_MODE_CONTINUOUS 0x0000  // Continuous conversion mode
#define ADS1115_MODE_SINGLE     0x0100  // Single-shot conversion mode

// Bits 7:5 — Data rate (samples per second)
#define ADS1115_RATE_8SPS       0x0000  // 8 samples/second
#define ADS1115_RATE_16SPS      0x0020  // 16 samples/second
#define ADS1115_RATE_32SPS      0x0040  // 32 samples/second
#define ADS1115_RATE_64SPS      0x0060  // 64 samples/second
#define ADS1115_RATE_128SPS     0x0080  // 128 samples/second (default)
#define ADS1115_RATE_250SPS     0x00A0  // 250 samples/second
#define ADS1115_RATE_475SPS     0x00C0  // 475 samples/second
#define ADS1115_RATE_860SPS     0x00E0  // 860 samples/second

// Bit 4 — Comparator mode (not used in this driver)
#define ADS1115_COMP_TRAD       0x0000  // Traditional comparator

// Bit 3 — Comparator polarity (not used in this driver)
#define ADS1115_COMP_ACTVLOW    0x0000  // Active low

// Bit 2 — Comparator latch (not used in this driver)
#define ADS1115_COMP_NOLOCK     0x0000  // Non-latching

// Bits 1:0 — Comparator queue
#define ADS1115_CQUE_NONE       0x0003  // Disable comparator

// Bit 15 — Operational status / Start conversion
#define ADS1115_OS_NOTBUSY      0x8000  // Not converting (or conversion complete)
#define ADS1115_OS_BUSY         0x0000  // Currently converting
#define ADS1115_OS_SINGLE       0x8000  // Start a single conversion (write only)

// ============================================================================
// K-type Thermocouple Constants
// ============================================================================

// K-type thermocouple: approximately 41 µV per °C
// At room temperature (~25°C), the voltage is ~1.0 mV
// ADS1115 with ±4.096V range and 16-bit: 4096 / 32767 = 0.125 mV per LSB
// Therefore: Temperature(°C) ≈ (Raw_ADC / 32767) * 4096 / 0.041
//            ≈ (Raw_ADC / 32767) * 99854 µV / (41 µV/°C)
//            ≈ Raw_ADC * 3.04

#define ADS1115_TC_GAIN_THERMOCOUPLE 3.04  // ~mV to °C conversion factor (approximate)
#define ADS1115_TC_OFFSET_THERMOCOUPLE -120.0 // Offset to account for ambient reference (~0°C at 0V)

// ============================================================================
// Function Declarations
// ============================================================================

/**
 * Initialize ADS1115 I2C communication and configure for thermocouple measurement.
 * 
 * Setup:
 *   - Initializes Wire library on GPIO20 (SDA) and GPIO19 (SCL)
 *   - Configures ADS1115 in single-shot mode for channel 0
 *   - Sets PGA to ±4.096V range (suitable for 3.3V supply)
 *   - Sets data rate to 128 SPS
 * 
 * @return true if I2C device responds at 0x48, false otherwise
 */
bool ads1115_init();

/**
 * Read raw 16-bit ADC value from ADS1115 conversion register.
 * 
 * Performs:
 *   1. Write single-shot conversion bit to config register
 *   2. Poll for conversion complete (bit 15 high in config register)
 *   3. Read conversion result from conversion register
 *   4. Return raw 16-bit signed integer (-32768 to +32767)
 * 
 * @return Raw ADC value (16-bit signed)
 */
int16_t ads1115_read_raw();

/**
 * Read temperature from K-type thermocouple via ADS1115.
 * 
 * Performs:
 *   1. Read raw ADC value (with averaging over ADS1115_SAMPLES samples)
 *   2. Convert to voltage: V(mV) = (raw / 32767) * (±4.096V)
 *   3. Convert to temperature: T(°C) ≈ V(mV) / 0.041 (K-type sensitivity)
 *   4. Apply calibration offset for ambient reference
 *   5. Return temperature in °C
 * 
 * @return Temperature in °C (floating-point)
 */
float ads1115_read_temperature();

/**
 * Write 16-bit value to specified ADS1115 register.
 * 
 * @param reg  Register address (0x00-0x03)
 * @param val  16-bit value to write
 * @return true if write successful, false if I2C error
 */
bool ads1115_write_register(uint8_t reg, uint16_t val);

/**
 * Read 16-bit value from specified ADS1115 register.
 * 
 * @param reg  Register address (0x00-0x03)
 * @return 16-bit register value (0 if I2C error)
 */
uint16_t ads1115_read_register(uint8_t reg);

/**
 * Check if ADS1115 is responding on I2C bus.
 * 
 * @return true if device ACKs address 0x48, false otherwise
 */
bool ads1115_is_present();

// Aliases for compatibility with existing sketch function names
#define readThermocoupleTemp_C ads1115_read_temperature
#define isThermocoupleValid() ads1115_is_present()

#endif // ADS1115_DRIVER_H

