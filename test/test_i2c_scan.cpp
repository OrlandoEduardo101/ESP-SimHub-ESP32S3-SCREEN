// Test I2C scan - verifica se dispositivos estão respondendo no barramento
// Upload este código para ver quais endereços I2C estão ativos

#include <Arduino.h>
#include <Wire.h>

#define I2C_SDA 8
#define I2C_SCL 9

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n========================================");
    Serial.println("I2C SCANNER - ESP32-S3 ButtonBox");
    Serial.println("========================================");
    Serial.println("SDA: GPIO 8");
    Serial.println("SCL: GPIO 9");
    Serial.println("========================================\n");

    Wire.begin(I2C_SDA, I2C_SCL);
    delay(100);

    Serial.println("Scanning I2C bus...\n");

    uint8_t found = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        uint8_t error = Wire.endTransmission();

        if (error == 0) {
            Serial.printf("✓ Device found at 0x%02X", addr);

            // Identifica dispositivos conhecidos
            if (addr == 0x20) {
                Serial.print(" → MCP23017 (Matrix)");
            } else if (addr == 0x40) {
                Serial.print(" → PCA9685 (LEDs)");
            } else if (addr >= 0x48 && addr <= 0x4F) {
                Serial.print(" → ADS1115/PCF8591 (ADC)");
            } else if (addr == 0x68) {
                Serial.print(" → MPU6050/DS3231 (IMU/RTC)");
            } else {
                Serial.print(" → Unknown device");
            }
            Serial.println();
            found++;
        } else if (error == 4) {
            Serial.printf("⚠️  Error at 0x%02X (unknown error)\n", addr);
        }

        delay(5);
    }

    Serial.println("\n========================================");
    if (found == 0) {
        Serial.println("❌ NO I2C devices found!");
        Serial.println("\nCheck:");
        Serial.println("  - SDA/SCL wiring");
        Serial.println("  - VCC (3.3V) connected");
        Serial.println("  - GND connected");
        Serial.println("  - RESET pin (MCP23017 pin 18) → 3.3V");
        Serial.println("  - Address pins (A0/A1/A2) → GND for 0x20");
    } else {
        Serial.printf("✓ Found %d device(s)\n", found);
        Serial.println("\nExpected devices:");
        Serial.println("  0x20 → MCP23017 (button matrix)");
        Serial.println("  0x40 → PCA9685 (LED driver)");
    }
    Serial.println("========================================\n");
}

void loop() {
    delay(5000);
    Serial.println("Re-scanning in 5s... (press RESET to stop)");

    uint8_t found = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            found++;
        }
        delay(5);
    }

    Serial.printf("[%lu] Active devices: %d\n", millis()/1000, found);
}
