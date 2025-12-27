#!/usr/bin/env python3
"""
Test GPIO 10 on ESP32-S3
Creates a simple blink program to verify GPIO 10 is working
"""

import os
import tempfile
import subprocess

# Simple Arduino sketch to test GPIO 10
test_sketch = """
// Test GPIO 10 - Simple Blink
// Upload this to test if GPIO 10 is working

#define TEST_PIN 10
#define BLINK_INTERVAL 500  // 500ms ON, 500ms OFF

void setup() {
    Serial.begin(115200);
    pinMode(TEST_PIN, OUTPUT);
    Serial.println("========================================");
    Serial.println("GPIO 10 Test Started");
    Serial.println("If LED blinks, GPIO 10 is working!");
    Serial.println("========================================");
}

void loop() {
    digitalWrite(TEST_PIN, HIGH);
    Serial.println("[GPIO 10] HIGH (LED ON)");
    delay(BLINK_INTERVAL);
    
    digitalWrite(TEST_PIN, LOW);
    Serial.println("[GPIO 10] LOW (LED OFF)");
    delay(BLINK_INTERVAL);
}
"""

# Create temporary Arduino sketch
temp_dir = tempfile.mkdtemp()
sketch_path = os.path.join(temp_dir, "test_gpio10")
os.makedirs(sketch_path, exist_ok=True)

sketch_file = os.path.join(sketch_path, "test_gpio10.ino")
with open(sketch_file, 'w') as f:
    f.write(test_sketch)

print("=" * 80)
print("GPIO 10 Test Sketch Generator")
print("=" * 80)
print(f"\nTest sketch created at: {sketch_file}\n")
print("INSTRUCTIONS:")
print("=" * 80)
print("\n1. Connect your test setup:")
print("   - WS2812B LED: DIN to GPIO 10, VCC to 5V, GND to GND")
print("   - OR simple LED: GPIO 10 -> [470Ω resistor] -> LED -> GND")
print("\n2. Upload the test sketch:")
print(f"   arduino-cli compile --fqbn esp32:esp32:esp32s3 {sketch_path}")
print(f"   arduino-cli upload --fqbn esp32:esp32:esp32s3 -p COM11 {sketch_path}")
print("\n3. OR copy the code below and use PlatformIO/Arduino IDE:\n")
print("-" * 80)
print(test_sketch)
print("-" * 80)
print("\n4. Open Serial Monitor at 115200 baud:")
print("   pio device monitor -b 115200")
print("\n5. Expected behavior:")
print("   - Serial monitor shows: [GPIO 10] HIGH... [GPIO 10] LOW...")
print("   - LED blinks every 500ms")
print("   - If using WS2812B, first LED may show dim/random colors")
print("\n6. If LED doesn't blink:")
print("   - GPIO 10 may be damaged or not connected")
print("   - Try different GPIO (like GPIO 11)")
print("   - Check continuity with multimeter")
print("\n" + "=" * 80)
