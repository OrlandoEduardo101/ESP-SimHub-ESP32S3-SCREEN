
#include <Arduino.h>
#include <EspSimHub.h>
#include "USB.h"
#include "USBHIDGamepad.h"
#include <NeoPixelBus.h>

#define DEVICE_NAME "ESP-SimHub-ButtonBox"

// Disable WiFi and display features for ButtonBox
#define INCLUDE_WIFI false
#define INCLUDE_RGB_LEDS_NEOPIXELBUS

// Debug port (used by EspSimHub library)
Stream* DebugPort = nullptr;

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
    {13, 38},  // Encoder 2: GP13/GP38 (troca p/ evitar solda difícil em 14)
    {39, 40},  // Encoder 3: GP39/GP40 (troca p/ evitar solda difícil em 15/16)
    {17, 18}   // Encoder 4: GP17/GP18
};

// Encoder mode: axes (default) or virtual button clicks
bool encoderButtonMode = false;
const unsigned long MODE_HOLD_MS = 1500;
unsigned long comboHoldStart = 0;

// Virtual button pulse tracking for encoder-as-buttons (CW/CCW)
struct VirtualButtonPulse {
    uint8_t id;
    bool active;
    unsigned long releaseAt;
};

// Buttons 26-33 reserved for encoder pulses (2 per encoder)
VirtualButtonPulse encoderPulses[NUM_ENCODERS * 2] = {
    {26, false, 0}, {27, false, 0},
    {28, false, 0}, {29, false, 0},
    {30, false, 0}, {31, false, 0},
    {32, false, 0}, {33, false, 0}
};

// LED Strip Configuration
#define LED_COUNT 5
#define LED_DATA_PIN 45  // GP45 (WS2812B DIN)
#define LED_ONBOARD_PIN 21  // GP21 (RGB onboard - optional)
const uint8_t LED_BRIGHTNESS = 48;

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
                        if (encoderPulses[pulseIdxCCW].active) {
                            buttons &= ~(1UL << (encoderPulses[pulseIdxCCW].id - 1));
                            encoderPulses[pulseIdxCCW].active = false;
                        }

                        encoderPulses[pulseIdxCW].active = true;
                        encoderPulses[pulseIdxCW].releaseAt = millis() + 30;
                        buttons |= (1UL << (encoderPulses[pulseIdxCW].id - 1));
                    } else {
                        if (encoderPulses[pulseIdxCW].active) {
                            buttons &= ~(1UL << (encoderPulses[pulseIdxCW].id - 1));
                            encoderPulses[pulseIdxCW].active = false;
                        }

                        encoderPulses[pulseIdxCCW].active = true;
                        encoderPulses[pulseIdxCCW].releaseAt = millis() + 30;
                        buttons |= (1UL << (encoderPulses[pulseIdxCCW].id - 1));
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

    // 5th LED shows "alive" status and mode
    if (LED_COUNT >= 5) {
        if (encoderButtonMode) {
            // Amber = encoder button mode
            ledStrip.SetPixelColor(4, scaledColor(180, 90, 10));
        } else {
            // Cyan-ish = axis mode
            ledStrip.SetPixelColor(4, scaledColor(40, 120, 160));
        }
    }

    ledStrip.Show();
}

// ==========================================
// SIMHUB PROTOCOL INTEGRATION
// ==========================================

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
    
    // Initialize Serial (USB CDC on ESP32-S3)
    Serial.begin(115200);
    delay(2000);  // Give time for USB HID and serial to enumerate
    
    Serial.println("\n========================================");
    Serial.println("ESP32-S3 SimHub ButtonBox Firmware");
    Serial.println("Version: j (ButtonBox Edition + USB HID)");
    Serial.println("========================================");
    Serial.print("Device: ");
    Serial.println(DEVICE_NAME);
    Serial.print("Free Heap: ");
    Serial.println(ESP.getFreeHeap());
    Serial.println("USB HID Gamepad: Enabled (32 buttons + 4 axes)");
    Serial.println("========================================\n");
    
    // Initialize components
    setupButtonMatrix();
    setupEncoders();
    setupLEDs();
    
    Serial.println("[Startup] LED Test Sequence - ENABLED");
    
    Serial.println("\n[System] ButtonBox Ready!");
    Serial.println("Waiting for SimHub connection...\n");
}

void loop() {
    // Scan inputs
    scanButtonMatrix();
    handleModeToggle();
    scanEncoders();
    updateLEDs();
    releaseVirtualButtonPulses();
    
    // Handle SimHub commands
    if (FlowSerialAvailable() > 0) {
        int r = FlowSerialTimedRead();
        
        if (r == MESSAGE_HEADER) {
            char cmd = FlowSerialTimedRead();
            
            switch (cmd) {
                case '1': Command_Hello(); break;
                case '0': Command_Features(); break;
                case 'N': Command_DeviceName(); FlowSerialWrite(0x15); break;
                case 'I': Command_UniqueId(); FlowSerialWrite(0x15); break;
                case 'J': Command_ButtonsCount(); break;
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