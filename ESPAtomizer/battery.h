// battery.h - Battery helper API
#ifndef BATTERY_H
#define BATTERY_H

#include "config.h"
#include <Arduino.h>

#if USE_BAT
#include <string>

// Header-only battery helpers (consolidated implementation)
// Note: functions/variables use internal linkage to avoid ODR issues
// when this header is included from multiple translation units.
static double batteryVoltage = NAN;
static int batteryPercent = -1;
static bool batteryLow = false;

static inline void initBattery() {
	pinMode(BAT_PIN, INPUT);
#if defined(ARDUINO_ARCH_ESP32)
	#if defined(ADC_ATTEN_DB_11)
		analogSetPinAttenuation(BAT_PIN, ADC_ATTEN_DB_11);
	#elif defined(ADC_ATTENDB_MAX)
		analogSetPinAttenuation(BAT_PIN, ADC_ATTENDB_MAX);
	#endif
#endif
}

static inline void sampleBattery() {
	uint32_t acc = 0;
	for (int i = 0; i < BAT_SAMPLES; ++i) { acc += analogRead(BAT_PIN); delay(1); }
	uint32_t batteryRaw = acc / (BAT_SAMPLES > 0 ? BAT_SAMPLES : 1);
	double v_adc = (batteryRaw / 4095.0) * 3.3;
	double ratio = (BAT_R1 + BAT_R2) / BAT_R2;
	batteryVoltage = v_adc * ratio;
	double p = (batteryVoltage - BAT_MIN_V) / (BAT_MAX_V - BAT_MIN_V) * 100.0;
	if (p < 0) p = 0; if (p > 100) p = 100;
	batteryPercent = (int)(p + 0.5);
	batteryLow = (batteryVoltage > 0 && batteryVoltage < BAT_CUTOFF_V);

#if USE_BLE
	if ( ::chBat && ::bleServer && ::bleServer->getConnectedCount() > 0) {
		char bbuf[32]; snprintf(bbuf, sizeof(bbuf), "%.2f", batteryVoltage);
		std::string bv(bbuf);
		::chBat->setValue(bv);
		::chBat->notify();
	}
#endif

	// Print battery debugging info to serial for bring-up visibility
	Serial.printf("[BATT] v=%.3fV pct=%d low=%d\n", batteryVoltage, batteryPercent, batteryLow?1:0);
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
