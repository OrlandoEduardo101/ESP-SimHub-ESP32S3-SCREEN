// Código minimalista para testar boot do ESP32-S3-Zero
#include <Arduino.h>

void setup() {
    // Aguardar para evitar watchdog
    delay(2000);
    
    // Inicializar Serial DEPOIS do delay
    Serial.begin(115200);
    delay(500);
    
    Serial.println("\n\n");
    Serial.println("==================================");
    Serial.println("ESP32-S3-Zero Minimal Test");
    Serial.println("==================================");
    Serial.print("Chip Model: ");
    Serial.println(ESP.getChipModel());
    Serial.print("Chip Cores: ");
    Serial.println(ESP.getChipCores());
    Serial.print("CPU Freq: ");
    Serial.print(ESP.getCpuFreqMHz());
    Serial.println(" MHz");
    Serial.print("Free Heap: ");
    Serial.println(ESP.getFreeHeap());
    Serial.println("==================================");
    Serial.println("Setup complete! Boot successful!");
    Serial.println("==================================\n");
}

void loop() {
    static unsigned long lastPrint = 0;
    
    if (millis() - lastPrint >= 2000) {
        lastPrint = millis();
        Serial.print("Running... uptime: ");
        Serial.print(millis() / 1000);
        Serial.println(" seconds");
    }
}
