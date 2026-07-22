// battery.h - Battery helper API
#ifndef BATTERY_H
#define BATTERY_H

#include "config.h"
#include "StateManager.h"
#include <Arduino.h>

extern GlobalState gState;

#if USE_BAT
#include <string>

// Header-only battery helpers (consolidated implementation)
// Note: functions/variables use internal linkage to avoid ODR issues
// when this header is included from multiple translation units.
// Single definitions are provided in the main translation unit; header exposes externs.

static inline void initBattery() {
	pinMode(BAT_PIN, INPUT);
#if defined(ARDUINO_ARCH_ESP32)
	#if defined(ADC_ATTEN_DB_11)
		analogSetPinAttenuation(BAT_PIN, ADC_ATTEN_DB_11);
	#elif defined(ADC_ATTENDB_MAX)
		analogSetPinAttenuation(BAT_PIN, ADC_ATTENDB_MAX);
	#endif
#endif
	// Diagnostic: show battery ADC pin configuration
	Serial.printf("[BATT] ADC pin=%d (A0=%d), R1=%.0f, R2=%.0f, ratio=%.2f\n", 
	              BAT_PIN, (int)A0, BAT_R1, BAT_R2, (BAT_R1 + BAT_R2) / BAT_R2);
	
	// Take 10 quick readings to check for floating pin
	Serial.println(F("[BATT] ADC stability check (10 samples):"));
	uint32_t samples[10];
	uint32_t minVal = 4095, maxVal = 0;
	for (int i = 0; i < 10; i++) {
		samples[i] = analogRead(BAT_PIN);
		if (samples[i] < minVal) minVal = samples[i];
		if (samples[i] > maxVal) maxVal = samples[i];
		delay(5);
	}
	uint32_t spread = maxVal - minVal;
	Serial.printf("[BATT] min=%lu max=%lu spread=%lu\n", (unsigned long)minVal, (unsigned long)maxVal, (unsigned long)spread);
	if (spread > 500) {
		Serial.println(F("[BATT] WARNING: ADC readings unstable - pin may be floating (not connected)!"));
	} else if (maxVal < 100) {
		Serial.println(F("[BATT] WARNING: ADC reads near zero - check if BAT_ADC net is connected"));
	} else {
		Serial.println(F("[BATT] ADC appears stable"));
	}
}

static inline void sampleBattery() {
	#if TEST_MODE
	  // Provide a stable simulated battery reading for logic tests
	  gState.battery.voltage = 3.90;
	  gState.battery.percent = 75;
	  gState.battery.lowCutoff = false;
	  Serial.printf("[BATT] (TEST_MODE) v=%.3fV pct=%d low=%d\n", gState.battery.voltage, gState.battery.percent, gState.battery.lowCutoff?1:0);
	  return;
	#endif
	uint32_t acc = 0;
	for (int i = 0; i < BAT_SAMPLES; ++i) { acc += analogRead(BAT_PIN); delay(1); }
	uint32_t batteryRaw = acc / (BAT_SAMPLES > 0 ? BAT_SAMPLES : 1);
	double v_adc = (batteryRaw / 4095.0) * 3.3;
	double ratio = (BAT_R1 + BAT_R2) / BAT_R2;
	gState.battery.voltage = v_adc * ratio;
	double p = (gState.battery.voltage - BAT_MIN_V) / (BAT_MAX_V - BAT_MIN_V) * 100.0;
	if (p < 0) p = 0; if (p > 100) p = 100;
	gState.battery.percent = (int)(p + 0.5);
	gState.battery.lowCutoff = (gState.battery.voltage > 0 && gState.battery.voltage < BAT_CUTOFF_V);

#if USE_BLE && !TEST_MODE
	if (gState.ble.chBat && gState.ble.server && gState.ble.server->getConnectedCount() > 0) {
		char bbuf[32]; snprintf(bbuf, sizeof(bbuf), "%.2f", gState.battery.voltage);
		std::string bv(bbuf);
		gState.ble.chBat->setValue(bv);
		gState.ble.chBat->notify();
	}
#endif

	// Print battery debugging info to serial for bring-up visibility
	Serial.printf("[BATT] v=%.3fV pct=%d low=%d\n", gState.battery.voltage, gState.battery.percent, gState.battery.lowCutoff?1:0);
}

static inline void printBatteryDebug() {
	uint32_t acc = 0;
	for (int i = 0; i < BAT_SAMPLES; ++i) { acc += analogRead(BAT_PIN); delay(2); }
	uint32_t batteryRaw = acc / (BAT_SAMPLES > 0 ? BAT_SAMPLES : 1);
	double v_adc = (batteryRaw / 4095.0) * 3.3;
	double ratio = (BAT_R1 + BAT_R2) / BAT_R2;
	double vbat = v_adc * ratio;
	double p = (vbat - BAT_MIN_V) / (BAT_MAX_V - BAT_MIN_V) * 100.0;
	if (p < 0) p = 0; if (p > 100) p = 100;
	Serial.printf("[BATT] raw=%lu v_adc=%.3f V vbat=%.3f V pct=%d%%\n", (unsigned long)batteryRaw, v_adc, vbat, (int)(p + 0.5));
}

static inline void batteryChargeCheck(unsigned long durationMs = 20000UL, unsigned long intervalMs = 2000UL) {
	Serial.printf("[BATT] Charging check for %lums (interval %lums)\n", durationMs, intervalMs);
	int steps = (int)((durationMs + intervalMs - 1) / intervalMs);
	double first = NAN;
	double last = NAN;
	for (int i = 0; i < steps; ++i) {
		uint32_t acc = 0;
		for (int k = 0; k < BAT_SAMPLES; ++k) { acc += analogRead(BAT_PIN); delay(2); }
		uint32_t raw = acc / (BAT_SAMPLES > 0 ? BAT_SAMPLES : 1);
		double v_adc = (raw / 4095.0) * 3.3;
		double vbat = v_adc * ((BAT_R1 + BAT_R2) / BAT_R2);
		if (i == 0) first = vbat;
		last = vbat;
		Serial.printf("[BATT] t=%ldms raw=%lu vbat=%.3f V\n", (long)(i * intervalMs), (unsigned long)raw, vbat);
		if (i + 1 < steps) delay(intervalMs);
	}
	if (!isnan(first) && !isnan(last)) {
		double delta = last - first;
		Serial.printf("[BATT] Delta over test=%.3f V -> %s\n", delta, (delta > 0.005) ? "Charging" : ((delta < -0.005) ? "Discharging" : "Stable"));
	}
}

#endif

#endif // BATTERY_H
