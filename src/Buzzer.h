#pragma once
#include <Arduino.h>

// Active buzzer on GPIO 11 (EXT_IO2, pin 4 of Extended IO Interface)
// Wiring: GPIO11 -> 1kΩ -> NPN Base | Collector -> Buzzer- | +5V -> Buzzer+ | Emitter -> GND
#define BUZZER_PIN 11

static unsigned long _buzzerOffAt = 0;

inline void buzzerInit() {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
}

// Non-blocking beep. If a longer beep is in progress, this call is ignored.
inline void buzzerBeep(uint32_t durationMs) {
    unsigned long now = millis();
    if (now < _buzzerOffAt) return; // already beeping a longer one — ignore
    digitalWrite(BUZZER_PIN, HIGH);
    _buzzerOffAt = now + durationMs;
}

// Call this every loop iteration to turn off the buzzer at the right time.
inline void buzzerUpdate() {
    if (_buzzerOffAt > 0 && millis() >= _buzzerOffAt) {
        digitalWrite(BUZZER_PIN, LOW);
        _buzzerOffAt = 0;
    }
}
