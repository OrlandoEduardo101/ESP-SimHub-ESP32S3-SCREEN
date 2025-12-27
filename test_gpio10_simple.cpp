/*
 * Simple GPIO 10 Test - Blink LED
 * 
 * Replace src/main.cpp temporarily with this file to test GPIO 10
 * 
 * SETUP:
 * - WS2812B: Connect DIN to GPIO 10, VCC to 5V, GND to GND
 * - Simple LED: Connect GPIO 10 -> [470Ω] -> LED -> GND
 * 
 * EXPECTED RESULT:
 * - LED blinks every 500ms
 * - Serial monitor shows HIGH/LOW messages at 115200 baud
 * 
 * If LED blinks: GPIO 10 is working! ✓
 * If no blink: GPIO 10 may be damaged or not connected ✗
 */

#include <Arduino.h>

#define TEST_PIN 10
#define BLINK_INTERVAL 500  // 500ms

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n========================================");
    Serial.println("GPIO 10 Test - Simple Blink");
    Serial.println("========================================");
    Serial.println("Pin: GPIO 10");
    Serial.println("Interval: 500ms ON / 500ms OFF");
    Serial.println("========================================\n");
    
    pinMode(TEST_PIN, OUTPUT);
    Serial.println("[SETUP] GPIO 10 configured as OUTPUT");
    Serial.println("[SETUP] Starting blink test...\n");
}

void loop() {
    // Turn LED ON
    digitalWrite(TEST_PIN, HIGH);
    Serial.println("[GPIO 10] HIGH (3.3V) - LED should be ON");
    delay(BLINK_INTERVAL);
    
    // Turn LED OFF
    digitalWrite(TEST_PIN, LOW);
    Serial.println("[GPIO 10] LOW  (0V)   - LED should be OFF");
    delay(BLINK_INTERVAL);
}
