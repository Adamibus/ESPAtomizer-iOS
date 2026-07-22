// ESPAtomizer.ino - Example Integration with ADS1115 Driver
// 
// This snippet shows how to integrate the ADS1115 driver into the main sketch.
// 
// IMPORTANT: This is a reference example. You'll need to adapt this to your
// existing ESPAtomizer.ino code structure and flow.
// This is an EXAMPLE FILE ONLY — not compiled by default.

#if 0  // DISABLED: Example code only, not for direct compilation

#include "config.h"
#include "ads1115_driver.h"  // Include the ADS1115 I2C driver

// =============================================================================
// SETUP FUNCTION - Initialize Hardware
// =============================================================================

void setup()
{
    Serial.begin(115200);
    delay(100);
    
    Serial.println("\n\n[ESPAtomizer] Starting initialization...");
    
    // Initialize GPIO pins (encoder, MOSFET, battery ADC, etc.)
    pinMode(BAT_PIN, INPUT);            // Battery ADC input
    pinMode(OUTPUT_PIN, OUTPUT);        // Heater PWM output
    pinMode(ENC_PIN_A, INPUT_PULLUP);   // Encoder A
    pinMode(ENC_PIN_B, INPUT_PULLUP);   // Encoder B
    pinMode(ENC_PIN_SW, INPUT_PULLUP);  // Encoder switch
    
    // Initialize OLED display on I2C bus
    // Your OLED initialization code here...
    // (The I2C bus is already configured by ads1115_init() below)
    
    // Initialize ADS1115 thermocouple converter on I2C bus
    if (!ads1115_init()) {
        Serial.println("[ERROR] Failed to initialize ADS1115!");
        // Handle initialization failure (may want to continue without thermocouple)
        // OR halt the device and wait for user intervention
        Serial.println("[WARNING] Continuing without thermocouple measurement...");
        // while(1); // Uncomment to halt on ADS1115 failure
    }
    
    // Initialize other components (PID, BLE, WiFi, etc.)
    // Your other initialization code here...
    
    Serial.println("[ESPAtomizer] Initialization complete!");
    
    // Print pin assignments for verification
    Serial.printf("Pins: BAT=%d, OUT=%d, ENC_A=%d, ENC_B=%d, ENC_SW=%d, SDA=%d, SCL=%d, ADS1115=0x48(I2C)\n",
                  BAT_PIN, OUTPUT_PIN, ENC_PIN_A, ENC_PIN_B, ENC_PIN_SW, OLED_SDA, OLED_SCL);
}

// =============================================================================
// LOOP FUNCTION - Main Control Loop
// =============================================================================

void loop()
{
    // Read thermocouple temperature via ADS1115 I2C
    float temperature_c = ads1115_read_temperature();
    
    if (temperature_c > -999.0) {  // Check for valid reading (error return is -999.0)
        Serial.printf("[TEMP] %.2f °C\n", temperature_c);
        // Use temperature_c in your PID controller or display logic
    } else {
        Serial.println("[ERROR] Failed to read thermocouple temperature");
    }
    
    // Read battery voltage (ADC)
    int battery_raw = analogRead(BAT_PIN);
    // Convert to voltage: battery_raw * (3.3 / 4095) * voltage_divider_ratio
    
    // Handle encoder input (rotation A/B, switch press/release)
    // Your encoder handling code here...
    
    // Update OLED display
    // Your OLED update code here (same I2C bus as ADS1115, different address)
    
    // PWM control for heater (OUTPUT_PIN on GPIO16)
    // Your PWM/PID logic here...
    
    // BLE/WiFi communication with remote control
    // Your wireless control code here...
    
    delay(100);  // Main loop timing (adjust as needed for your application)
}

// =============================================================================
// OPTIONAL: Temperature Reading Function (Wrapper)
// =============================================================================

// You may want to wrap the ADS1115 call for your existing code:

float getTemperatureC()
{
    return ads1115_read_temperature();
}

// Or with error handling:

float getTemperatureCWithFallback(float fallback_value)
{
    float temp = ads1115_read_temperature();
    if (temp < -999.0) {
        return fallback_value;  // Use fallback on error
    }
    return temp;
}

// =============================================================================
// OPTIONAL: Debug Function
// =============================================================================

void debugADS1115()
{
    Serial.println("\n[DEBUG] Testing ADS1115...");
    
    // Check if device is present
    if (!ads1115_is_present()) {
        Serial.println("  ❌ ADS1115 NOT found at address 0x48 on I2C bus");
        return;
    }
    Serial.println("  ✅ ADS1115 found at address 0x48");
    
    // Read raw ADC value
    int16_t raw = ads1115_read_raw();
    Serial.printf("  Raw ADC: %d\n", raw);
    
    // Read temperature
    float temp = ads1115_read_temperature();
    Serial.printf("  Temperature: %.2f °C\n", temp);
    
    Serial.println("[DEBUG] ADS1115 test complete\n");
}

// Call this function from your serial monitor command handler:
// Example: `if (command == "debug") { debugADS1115(); }`

#endif  // END EXAMPLE CODE GUARD
