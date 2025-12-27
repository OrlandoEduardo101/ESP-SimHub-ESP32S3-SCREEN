
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager

// ==========================================
// CONFIGURAÇÃO WIFI
// ==========================================
const uint16_t SIMHUB_PORT = 20777;            // Porta padrão SimHub

// WiFiManager: configura WiFi via portal web (192.168.4.1)
// Segure botões 1+5 por 3s para resetar WiFi e abrir portal
WiFiManager wifiManager;
const unsigned long WIFI_RESET_HOLD_MS = 3000;

// Fallback: credenciais hardcoded caso WiFiManager falhe
const char* FALLBACK_SSID = "Orlando_tplink";      // Backup
const char* FALLBACK_PASSWORD = "83185003";       // Backup

// Opcional: IP fixo (deixe false para DHCP automático)
const bool USE_STATIC_IP = true;                // true = usa IP fixo, false = DHCP
IPAddress STATIC_IP(192, 168, 0, 223);         // <-- ALTERE para sua sub-rede
IPAddress GATEWAY(192, 168, 0, 1);
IPAddress SUBNET(255, 255, 255, 0);
IPAddress PRIMARY_DNS(8, 8, 8, 8);
IPAddress SECONDARY_DNS(1, 1, 1, 1);

// WiFi + USB HID: HID Gamepad via USB + SimHub via WiFi TCP
// FlowSerial será redirecionado para WiFiClient quando conectado
WiFiServer simhubServer(SIMHUB_PORT);
WiFiClient simhubClient;
bool wifiConnected = false;

#define FlowSerialBegin(baud) // WiFi não precisa de begin()
#define FlowSerialFlush() if(simhubClient.connected()) simhubClient.flush()

#include <EspSimHub.h>
#include "USB.h"
#include "USBHIDGamepad.h"
#include <NeoPixelBus.h>

#define DEVICE_NAME "ESP-SimHub-ButtonBox"

// Debug port (pode usar Serial ou criar outro)
Stream* DebugPort = &Serial;

// ==========================================
// USB HID GAMEPAD CONFIGURATION
// ==========================================

USBHIDGamepad Gamepad;

// Gamepad state tracking
int8_t axisX = 0, axisY = 0, axisZ = 0, axisRZ = 0, axisRX = 0, axisRY = 0;
uint32_t buttons = 0;

// ==========================================
// PINOUT CONFIGURATION - ESP32-S3-Zero
// ==========================================

// Matrix Configuration (4x5 = 20 buttons)
#define MATRIX_COLS 5
#define MATRIX_ROWS 4
const uint8_t colPins[MATRIX_COLS] = {4, 2, 3, 1, 5};   // P4,P2,P3,P1,P5 (OUTPUT)
const uint8_t rowPins[MATRIX_ROWS] = {8, 7, 9, 6};      // P8,P7,P9,P6 (INPUT_PULLUP)

// Rotary Encoders (4 encoders)
#define NUM_ENCODERS 4
struct EncoderPins {
    uint8_t pinA;
    uint8_t pinB;
};

const EncoderPins encoderPins[NUM_ENCODERS] = {
    {11, 12},  // Encoder 1: GP11/GP12
    {13, 38},  // Encoder 2: GP13/GP38
    {40, 41},  // Encoder 3: GP40/GP41 (ajustado para espaçamento melhor)
    {17, 18}   // Encoder 4: GP17/GP18
};

// Encoder mode: axes (default) or virtual button clicks
bool encoderButtonMode = false;
const unsigned long MODE_HOLD_MS = 1500;
unsigned long comboHoldStart = 0;

// WiFi status tracking
enum WiFiStatus {
    WIFI_DISCONNECTED,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    WIFI_ERROR
};
WiFiStatus currentWifiStatus = WIFI_DISCONNECTED;
unsigned long lastWifiCheck = 0;
const unsigned long WIFI_CHECK_INTERVAL = 5000; // Check every 5s

// Virtual button pulse tracking for encoder-as-buttons (CW/CCW)
struct VirtualButtonPulse {
    uint8_t id;
    bool active;
    unsigned long releaseAt;
};

// Botões virtuais dos encoders: 21-28 (após os 20 botões físicos)
// Encoder 0 (Z):  CW=21, CCW=22
// Encoder 1 (Rx): CW=23, CCW=24
// Encoder 2 (Ry): CW=25, CCW=26
// Encoder 3 (Rz): CW=27, CCW=28
VirtualButtonPulse encoderPulses[NUM_ENCODERS * 2] = {
    {21, false, 0}, {22, false, 0},  // Encoder 0 (Z)
    {23, false, 0}, {24, false, 0},  // Encoder 1 (Rx)
    {25, false, 0}, {26, false, 0},  // Encoder 2 (Ry)
    {27, false, 0}, {28, false, 0}   // Encoder 3 (Rz)
};

// LED Strip Configuration
#define LED_COUNT 5
#define LED_DATA_PIN 45  // GP45 (WS2812B DIN)
#define LED_ONBOARD_PIN 21  // GP21 (RGB onboard - optional)
const uint8_t LED_BRIGHTNESS = 75;

bool simhubLedControl = false;  // Flag: true = SimHub controla, false = local
unsigned long lastSimhubLedUpdate = 0;
const unsigned long SIMHUB_LED_TIMEOUT = 2000;  // 2s sem dados = volta ao controle local

inline RgbColor scaledColor(uint8_t r, uint8_t g, uint8_t b) {
    // Scale down colors using a global brightness cap
    return RgbColor((r * LED_BRIGHTNESS) / 255, (g * LED_BRIGHTNESS) / 255, (b * LED_BRIGHTNESS) / 255);
}

// ==========================================
// BUTTON MATRIX HANDLING
// ==========================================

// Button state tracking
bool buttonStates[MATRIX_ROWS][MATRIX_COLS] = {false};
bool prevButtonStates[MATRIX_ROWS][MATRIX_COLS] = {false};
unsigned long lastDebounceTime[MATRIX_ROWS][MATRIX_COLS] = {0};
const unsigned long DEBOUNCE_DELAY = 50;  // 50ms debounce

// Button numbering: 1-20 (row-major order)
// Row 0 (P8): Buttons 1-5, Row 1 (P7): Buttons 6-10
// Row 2 (P9): Buttons 11-15, Row 3 (P6): Buttons 16-20
inline uint8_t getButtonNumber(uint8_t row, uint8_t col) {
    return (row * MATRIX_COLS) + col + 1;
}

inline bool isButtonPressed(uint8_t buttonNum) {
    uint8_t idx = buttonNum - 1;
    return buttonStates[idx / MATRIX_COLS][idx % MATRIX_COLS];
}

void setupButtonMatrix() {
    // Configure column pins as OUTPUT (set HIGH initially)
    for (uint8_t col = 0; col < MATRIX_COLS; col++) {
        pinMode(colPins[col], OUTPUT);
        digitalWrite(colPins[col], HIGH);
    }
    
    // Configure row pins as INPUT_PULLUP
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        pinMode(rowPins[row], INPUT_PULLUP);
    }
    
    Serial.println("[ButtonMatrix] Initialized 5x5 matrix (25 buttons)");
}

void scanButtonMatrix() {
    unsigned long currentTime = millis();
    
    // Scan each column
    for (uint8_t col = 0; col < MATRIX_COLS; col++) {
        // Set current column LOW (active), others HIGH (inactive)
        for (uint8_t c = 0; c < MATRIX_COLS; c++) {
            digitalWrite(colPins[c], (c == col) ? LOW : HIGH);
        }
        
        delayMicroseconds(10);  // Small delay for signal stabilization
        
        // Read all rows for this column
        for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
            bool reading = (digitalRead(rowPins[row]) == LOW);  // Pressed = LOW
            
            // Debounce logic
            if (reading != prevButtonStates[row][col]) {
                lastDebounceTime[row][col] = currentTime;
            }
            
            if ((currentTime - lastDebounceTime[row][col]) > DEBOUNCE_DELAY) {
                // If state has changed after debounce
                if (reading != buttonStates[row][col]) {
                    buttonStates[row][col] = reading;
                    
                    uint8_t buttonNum = getButtonNumber(row, col);
                    if (reading) {
                        Serial.print("[Button] Pressed: ");
                        Serial.println(buttonNum);
                        if (buttonNum <= 32) {
                            buttons |= (1UL << (buttonNum - 1));
                            Gamepad.send(axisX, axisY, axisZ, axisRZ, axisRX, axisRY, 0, buttons);
                        }
                    } else {
                        Serial.print("[Button] Released: ");
                        Serial.println(buttonNum);
                        if (buttonNum <= 32) {
                            buttons &= ~(1UL << (buttonNum - 1));
                            Gamepad.send(axisX, axisY, axisZ, axisRZ, axisRX, axisRY, 0, buttons);
                        }
                    }
                }
            }
            
            prevButtonStates[row][col] = reading;
        }
    }
    
    // Reset all columns to HIGH after scan
    for (uint8_t col = 0; col < MATRIX_COLS; col++) {
        digitalWrite(colPins[col], HIGH);
    }
}

// ==========================================
// ROTARY ENCODER HANDLING
// ==========================================

struct EncoderState {
    int8_t lastEncoded;
    int32_t encoderValue;
    bool lastA;
    bool lastB;
    unsigned long lastChangeTime;
    float smoothedValue;  // Filtered value
    int8_t lastSentValue; // Last value actually sent to gamepad
    int8_t accumulator;   // Tracks partial steps for stable direction
};

EncoderState encoderStates[NUM_ENCODERS];
const unsigned long ENCODER_DEBOUNCE_US = 800;  // 800us debounce
const float ENCODER_SMOOTHING = 0.5;  // Smoothing factor (higher = more responsive)
const int8_t ENCODER_THRESHOLD = 1;  // Minimum change to update axis
// Quadrature transition table: rows are previous state (bits 3-2) + current state (bits 1-0)
const int8_t ENC_TABLE[16] = {
    0, -1,  1, 0,
    1,  0,  0,-1,
   -1,  0,  0, 1,
    0,  1, -1, 0
};

void setupEncoders() {
    for (uint8_t i = 0; i < NUM_ENCODERS; i++) {
        pinMode(encoderPins[i].pinA, INPUT_PULLUP);
        pinMode(encoderPins[i].pinB, INPUT_PULLUP);
        
        encoderStates[i].lastEncoded = 0;
        encoderStates[i].encoderValue = 0;
        encoderStates[i].lastA = digitalRead(encoderPins[i].pinA);
        encoderStates[i].lastB = digitalRead(encoderPins[i].pinB);
        encoderStates[i].lastChangeTime = 0;
        encoderStates[i].smoothedValue = 0.0;
        encoderStates[i].lastSentValue = 0;
        encoderStates[i].accumulator = 0;
    }
    
    Serial.println("[Encoders] Initialized 4 rotary encoders");
}

void scanEncoders() {
    for (uint8_t i = 0; i < NUM_ENCODERS; i++) {
        bool currentA = digitalRead(encoderPins[i].pinA);
        bool currentB = digitalRead(encoderPins[i].pinB);
        
        // Detect state change
        if (currentA != encoderStates[i].lastA || currentB != encoderStates[i].lastB) {
            unsigned long now = micros();
            
            // Minimal debounce
            if (now - encoderStates[i].lastChangeTime < ENCODER_DEBOUNCE_US) {
                continue;
            }
            
            encoderStates[i].lastChangeTime = now;
            
            // Encode current state and use transition table for stable direction
            int8_t encoded = (currentA << 1) | currentB;
            uint8_t state = ((encoderStates[i].lastEncoded << 2) | encoded) & 0x0F;
            encoderStates[i].accumulator += ENC_TABLE[state];
            encoderStates[i].lastEncoded = encoded;

            int8_t step = 0;
            if (encoderStates[i].accumulator >= 2) {
                step = 1;
                encoderStates[i].accumulator = 0;
            } else if (encoderStates[i].accumulator <= -2) {
                step = -1;
                encoderStates[i].accumulator = 0;
            }

            if (step != 0) {
                encoderStates[i].encoderValue += step;

                if (encoderButtonMode) {
                    uint8_t pulseIdxCW = i * 2;
                    uint8_t pulseIdxCCW = (i * 2) + 1;

                    if (step > 0) {
                        // Direita/CW - cancelar CCW se ativo
                        if (encoderPulses[pulseIdxCCW].active) {
                            buttons &= ~(1UL << (encoderPulses[pulseIdxCCW].id - 1));
                            encoderPulses[pulseIdxCCW].active = false;
                        }

                        encoderPulses[pulseIdxCW].active = true;
                        encoderPulses[pulseIdxCW].releaseAt = millis() + 30;
                        buttons |= (1UL << (encoderPulses[pulseIdxCW].id - 1));
                        
                        Serial.print("[Encoder ");
                        Serial.print(i);
                        Serial.print("] CW → Button ");
                        Serial.println(encoderPulses[pulseIdxCW].id);
                    } else {
                        // Esquerda/CCW - cancelar CW se ativo
                        if (encoderPulses[pulseIdxCW].active) {
                            buttons &= ~(1UL << (encoderPulses[pulseIdxCW].id - 1));
                            encoderPulses[pulseIdxCW].active = false;
                        }

                        encoderPulses[pulseIdxCCW].active = true;
                        encoderPulses[pulseIdxCCW].releaseAt = millis() + 30;
                        buttons |= (1UL << (encoderPulses[pulseIdxCCW].id - 1));
                        
                        Serial.print("[Encoder ");
                        Serial.print(i);
                        Serial.print("] CCW → Button ");
                        Serial.println(encoderPulses[pulseIdxCCW].id);
                    }

                    Gamepad.send(axisX, axisY, axisZ, axisRZ, axisRX, axisRY, 0, buttons);
                } else {
                    // Apply exponential smoothing filter (same for both directions)
                    encoderStates[i].smoothedValue = (ENCODER_SMOOTHING * encoderStates[i].encoderValue) + 
                                                      ((1.0 - ENCODER_SMOOTHING) * encoderStates[i].smoothedValue);

                    int8_t smoothed = (int8_t)round(encoderStates[i].smoothedValue);
                    int8_t val = constrain(smoothed, -127, 127);

                    // Only update if change exceeds threshold
                    if (abs(val - encoderStates[i].lastSentValue) >= ENCODER_THRESHOLD) {
                        encoderStates[i].lastSentValue = val;

                        if (i == 0) axisZ = val;
                        else if (i == 1) axisRX = val;
                        else if (i == 2) axisRY = val;
                        else if (i == 3) axisRZ = val;
                        Gamepad.send(axisX, axisY, axisZ, axisRZ, axisRX, axisRY, 0, buttons);
                    }
                }
            }
        }
        
        encoderStates[i].lastA = currentA;
        encoderStates[i].lastB = currentB;
    }
}

// ==========================================
// LED STRIP HANDLING (WS2812B)
// ==========================================

// WS2812B control
NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt0Ws2812xMethod> ledStrip(LED_COUNT, LED_DATA_PIN);

void setupLEDs() {
    ledStrip.Begin();
    ledStrip.ClearTo(RgbColor(0, 0, 0));
    ledStrip.Show();

    // Simple startup chase to confirm LEDs are alive
    for (uint8_t i = 0; i < LED_COUNT; i++) {
        ledStrip.SetPixelColor(i, scaledColor(0, 200, 0));
        ledStrip.Show();
        delay(80);
    }
    delay(150);
    ledStrip.ClearTo(RgbColor(0, 0, 0));
    ledStrip.Show();

    Serial.print("[LEDs] Initialized ");
    Serial.print(LED_COUNT);
    Serial.println(" WS2812B LEDs on GP45");
}

void updateLEDs() {
    // Check if SimHub control timed out
    if (simhubLedControl && (millis() - lastSimhubLedUpdate > SIMHUB_LED_TIMEOUT)) {
        simhubLedControl = false;
        Serial.println("[LEDs] SimHub timeout - reverting to local control");
    }
    
    // Only update locally if SimHub is not controlling
    if (!simhubLedControl) {
        // LEDs 1-4 mirror encoder direction: green = CW, red = CCW, blue = idle
        for (uint8_t i = 0; i < NUM_ENCODERS && i < LED_COUNT; i++) {
            int32_t val = constrain(encoderStates[i].encoderValue, -127, 127);
            uint8_t intensity = min<uint8_t>(120, abs(val) * 4);

            if (val > 0) {
                ledStrip.SetPixelColor(i, scaledColor(0, intensity, 0));
            } else if (val < 0) {
                ledStrip.SetPixelColor(i, scaledColor(intensity, 0, 0));
            } else {
                ledStrip.SetPixelColor(i, scaledColor(0, 0, 80));
            }
        }

        // 5th LED shows WiFi status + encoder mode
        if (LED_COUNT >= 5) {
            unsigned long now = millis();
            
            switch (currentWifiStatus) {
                case WIFI_DISCONNECTED:
                    // Off = desconectado
                    ledStrip.SetPixelColor(4, scaledColor(0, 0, 0));
                    break;
                    
                case WIFI_CONNECTING:
                    // Piscando azul = conectando
                    if ((now / 250) % 2 == 0) {
                        ledStrip.SetPixelColor(4, scaledColor(0, 0, 200));
                    } else {
                        ledStrip.SetPixelColor(4, scaledColor(0, 0, 0));
                    }
                    break;
                    
                case WIFI_CONNECTED:
                    // Verde (sem SimHub) ou Ciano (com SimHub)
                    if (simhubClient.connected()) {
                        // Ciano = WiFi OK + SimHub conectado
                        ledStrip.SetPixelColor(4, scaledColor(0, 200, 200));
                    } else {
                        // Verde = WiFi OK, aguardando SimHub
                        ledStrip.SetPixelColor(4, scaledColor(0, 200, 0));
                    }
                    break;
                    
                case WIFI_ERROR:
                    // Piscando vermelho = erro
                    if ((now / 500) % 2 == 0) {
                        ledStrip.SetPixelColor(4, scaledColor(200, 0, 0));
                    } else {
                        ledStrip.SetPixelColor(4, scaledColor(0, 0, 0));
                    }
                    break;
            }
        }

        ledStrip.Show();
    }
}

// ==========================================
// WIFI MANAGEMENT
// ==========================================

void setupWiFi() {
    Serial.println("[WiFi] Starting WiFiManager...");
    Serial.println("[WiFi] Hold buttons 1+5 for 3s to reset WiFi credentials");
    
    // Configure WiFiManager
    wifiManager.setConfigPortalTimeout(180);  // 3 minutos timeout portal
    wifiManager.setConnectTimeout(30);        // 30s timeout conexão
    wifiManager.setAPCallback([](WiFiManager *myWiFiManager) {
        Serial.println("");
        Serial.println("============================================================");
        Serial.println("[WiFi] Portal de Configuração Aberto!");
        Serial.println("[WiFi] 1. Conecte ao WiFi: ESP-SimHub-Config");
        Serial.println("[WiFi] 2. Abra navegador em: 192.168.4.1");
        Serial.println("[WiFi] 3. Configure sua rede WiFi");
        Serial.println("============================================================");
        Serial.println("");
        currentWifiStatus = WIFI_CONNECTING;
    });
    
    // Static IP se configurado
    if (USE_STATIC_IP) {
        wifiManager.setSTAStaticIPConfig(STATIC_IP, GATEWAY, SUBNET);
        Serial.print("[WiFi] Static IP configurado: ");
        Serial.println(STATIC_IP);
    }
    
    currentWifiStatus = WIFI_CONNECTING;
    
    // Tenta conectar (usa credenciais salvas ou abre portal)
    // SSID do portal: "ESP-SimHub-Config"
    if (!wifiManager.autoConnect("ESP-SimHub-Config")) {
        Serial.println("[WiFi] WiFiManager timeout - tentando fallback...");
        
        // Fallback: tenta credenciais hardcoded
        WiFi.mode(WIFI_STA);
        if (USE_STATIC_IP) {
            WiFi.config(STATIC_IP, GATEWAY, SUBNET, PRIMARY_DNS, SECONDARY_DNS);
        }
        WiFi.begin(FALLBACK_SSID, FALLBACK_PASSWORD);
        
        unsigned long startAttempt = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
            delay(100);
            Serial.print(".");
        }
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.println("============================================================");
        Serial.println("[WiFi] ✓ Conectado com sucesso!");
        Serial.print("[WiFi] SSID: ");
        Serial.println(WiFi.SSID());
        Serial.print("[WiFi] IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("[WiFi] Gateway: ");
        Serial.println(WiFi.gatewayIP());
        Serial.println("============================================================");
        
        simhubServer.begin();
        Serial.println("[WiFi] SimHub TCP server started on port " + String(SIMHUB_PORT));
        
        currentWifiStatus = WIFI_CONNECTED;
        wifiConnected = true;
    } else {
        Serial.println("");
        Serial.println("[WiFi] ✗ Connection FAILED!");
        currentWifiStatus = WIFI_ERROR;
        wifiConnected = false;
    }
}

void checkWiFiConnection() {
    unsigned long now = millis();
    
    if (now - lastWifiCheck < WIFI_CHECK_INTERVAL) {
        return;
    }
    lastWifiCheck = now;
    
    // Check WiFi status
    if (WiFi.status() != WL_CONNECTED) {
        if (wifiConnected) {
            Serial.println("[WiFi] Connection lost! Reconnecting...");
            wifiConnected = false;
            currentWifiStatus = WIFI_CONNECTING;
            WiFi.reconnect();
        }
    } else {
        if (!wifiConnected) {
            Serial.println("[WiFi] Reconnected! IP: " + String(WiFi.localIP()));
            wifiConnected = true;
            currentWifiStatus = WIFI_CONNECTED;
        }
    }
    
    // Check for new SimHub client connections
    if (wifiConnected && !simhubClient.connected()) {
        WiFiClient newClient = simhubServer.available();
        if (newClient) {
            Serial.println("[SimHub] Client connected from " + newClient.remoteIP().toString());
            simhubClient = newClient;
        }
    }
    
    // Disconnect dead clients
    if (simhubClient.connected() && !simhubClient.available()) {
        // Client might be dead - check with timeout
        static unsigned long lastActivity = millis();
        if (millis() - lastActivity > 30000) { // 30s timeout
            Serial.println("[SimHub] Client timeout - disconnecting");
            simhubClient.stop();
        }
    } else if (simhubClient.available()) {
        // Reset activity timer
        static unsigned long lastActivity = millis();
        lastActivity = millis();
    }
}

// ==========================================
// SIMHUB PROTOCOL INTEGRATION
// ==========================================

// Redirecionar Stream para WiFiClient (usado por ArqSerial)
#define StreamRead simhubClient.read
#define StreamFlush simhubClient.flush
#define StreamWrite simhubClient.write
#define StreamPrint simhubClient.print
#define StreamAvailable simhubClient.available

#include <FlowSerialRead.h>

#define MESSAGE_HEADER 0x03
#define VERSION 'j'

// Button state array for SimHub (20 buttons)
uint8_t buttonCount = 20;

void Command_Hello() {
    FlowSerialTimedRead();  // Read trailer byte
    delay(10);
    FlowSerialPrint(VERSION);
    FlowSerialFlush();
    Serial.println("[SimHub] Hello command received");
}

void Command_Features() {
    delay(10);
    FlowSerialPrint("N");  // Device Name
    FlowSerialPrint("I");  // Unique ID
    FlowSerialPrint("J");  // Buttons support
    FlowSerialPrint("L");  // RGB LEDs support
    FlowSerialPrint("X");  // Expanded support
    FlowSerialPrint("\n");
    FlowSerialFlush();
    Serial.println("[SimHub] Features command received");
}

void Command_DeviceName() {
    FlowSerialPrint(DEVICE_NAME);
    FlowSerialPrint("\n");
    FlowSerialFlush();
}

void Command_UniqueId() {
    auto id = getUniqueId();
    FlowSerialPrint(id);
    FlowSerialPrint("\n");
    FlowSerialFlush();
}

void Command_ButtonsCount() {
    FlowSerialWrite((byte)(buttonCount));
    FlowSerialFlush();
    Serial.print("[SimHub] Button count sent: ");
    Serial.println(buttonCount);
}

void Command_RGBLEDSCount() {
    FlowSerialWrite((byte)(LED_COUNT));
    FlowSerialFlush();
    Serial.print("[SimHub] LED count sent: ");
    Serial.println(LED_COUNT);
}

void Command_RGBLEDSData() {
    // Read LED count from SimHub
    byte ledCount = FlowSerialTimedRead();
    
    simhubLedControl = true;
    lastSimhubLedUpdate = millis();
    
    for (byte i = 0; i < ledCount && i < LED_COUNT; i++) {
        byte r = FlowSerialTimedRead();
        byte g = FlowSerialTimedRead();
        byte b = FlowSerialTimedRead();
        
        // Apply directly without brightness scaling (SimHub controls brightness)
        ledStrip.SetPixelColor(i, RgbColor(r, g, b));
    }
    
    ledStrip.Show();
    FlowSerialWrite(0x15);  // ACK
}

void idle(bool critical) {
    yield();  // Feed watchdog
}

// ==========================================
// MODE TOGGLE (Axes <-> Button clicks)
// Hold buttons 11 + 20 for MODE_HOLD_MS to toggle
// ==========================================

void flashModeChange() {
    // Brief blink to signal mode change
    ledStrip.ClearTo(encoderButtonMode ? scaledColor(180, 90, 10) : scaledColor(40, 120, 160));
    ledStrip.Show();
    delay(120);
    ledStrip.ClearTo(RgbColor(0, 0, 0));
    ledStrip.Show();
    delay(80);
}

void handleModeToggle() {
    bool comboPressed = isButtonPressed(4) && isButtonPressed(15);

    if (comboPressed) {
        if (comboHoldStart == 0) {
            comboHoldStart = millis();
        } else if (millis() - comboHoldStart >= MODE_HOLD_MS) {
            encoderButtonMode = !encoderButtonMode;
            comboHoldStart = UINT32_MAX; // prevent retrigger until release

            // Reset axes when entering button mode to avoid stale values
            if (encoderButtonMode) {
                axisZ = axisRX = axisRY = axisRZ = 0;
                Gamepad.send(axisX, axisY, axisZ, axisRZ, axisRX, axisRY, 0, buttons);
            }

            Serial.print("[Mode] Encoder mode changed to: ");
            Serial.println(encoderButtonMode ? "Button Pulses" : "Axes");
            flashModeChange();
        }
    } else {
        comboHoldStart = 0;
    }
}

void releaseVirtualButtonPulses() {
    unsigned long now = millis();
    bool changed = false;

    for (auto &pulse : encoderPulses) {
        if (pulse.active && now >= pulse.releaseAt) {
            buttons &= ~(1UL << (pulse.id - 1));
            pulse.active = false;
            changed = true;
        }
    }

    if (changed) {
        Gamepad.send(axisX, axisY, axisZ, axisRZ, axisRX, axisRY, 0, buttons);
    }
}

// ==========================================
// MAIN SETUP AND LOOP
// ==========================================

void setup(void) {
    // Initialize USB HID Gamepad
    Gamepad.begin();
    USB.begin();
    
    // Initialize Serial for debug (USB CDC)
    Serial.begin(115200);
    delay(2000);  // Give time for USB to enumerate
    
    Serial.println("\n========================================");
    Serial.println("ESP32-S3 SimHub ButtonBox Firmware");
    Serial.println("Version: k (WiFi + USB HID)");
    Serial.println("========================================");
    Serial.print("Device: ");
    Serial.println(DEVICE_NAME);
    Serial.print("Free Heap: ");
    Serial.println(ESP.getFreeHeap());
    Serial.println("USB HID Gamepad: Enabled (32 buttons + 4 rotation axes)");
    Serial.println("========================================\n");
    
    // Initialize components
    setupButtonMatrix();
    setupEncoders();
    setupLEDs();
    
    Serial.println("[Startup] LED Test Sequence - ENABLED");
    
    // Connect WiFi for SimHub communication
    setupWiFi();
    
    Serial.println("\n[System] ButtonBox Ready!");
    if (wifiConnected) {
        Serial.println("[SimHub] Waiting for TCP connection on port " + String(SIMHUB_PORT) + "...");
        Serial.println("[SimHub] Configure no SimHub: IP " + WiFi.localIP().toString() + " porta " + String(SIMHUB_PORT));
    } else {
        Serial.println("[Warning] WiFi not connected - SimHub LEDs unavailable");
        Serial.println("[Warning] USB HID Gamepad will still work!");
    }
    Serial.println("");
}

void loop() {
    // Check WiFi and SimHub client connections
    checkWiFiConnection();
    
    // WiFi Reset: segure botões 1+5 por 3s
    static unsigned long wifiResetHoldStart = 0;
    if (isButtonPressed(1) && isButtonPressed(5)) {
        if (wifiResetHoldStart == 0) {
            wifiResetHoldStart = millis();
        } else if (millis() - wifiResetHoldStart >= WIFI_RESET_HOLD_MS) {
            Serial.println("\n[WiFi] RESET - Apagando credenciais salvas...");
            wifiManager.resetSettings();
            delay(1000);
            Serial.println("[WiFi] Reiniciando ESP32...");
            ESP.restart();
        }
    } else {
        wifiResetHoldStart = 0;
    }
    
    // Scan inputs
    scanButtonMatrix();
    handleModeToggle();
    scanEncoders();
    updateLEDs();
    releaseVirtualButtonPulses();
    
    // Handle SimHub commands (only if client is connected)
    if (simhubClient.connected() && FlowSerialAvailable() > 0) {
        int r = FlowSerialTimedRead();
        
        if (r == MESSAGE_HEADER) {
            char cmd = FlowSerialTimedRead();
            
            switch (cmd) {
                case '1': Command_Hello(); break;
                case '0': Command_Features(); break;
                case 'N': Command_DeviceName(); FlowSerialWrite(0x15); break;
                case 'I': Command_UniqueId(); FlowSerialWrite(0x15); break;
                case 'J': Command_ButtonsCount(); break;
                case '2': Command_RGBLEDSCount(); break;
                case '3': Command_RGBLEDSData(); break;
                default:
                    // Unknown command - send ACK
                    FlowSerialWrite(0x15);
                    break;
            }
        }
    }
    
    yield();  // Feed watchdog
    delay(1);  // Small delay to prevent tight loop
} 