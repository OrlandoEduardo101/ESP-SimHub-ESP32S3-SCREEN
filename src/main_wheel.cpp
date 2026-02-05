#include <Arduino.h>
#include "USB.h"
#include "USBHID.h"
#include <Preferences.h>

#define DEVICE_NAME "ESP-ButtonBox-WHEEL"

// ================================
// USB HID GAMEPAD + CONSUMER CONTROL
// ================================
USBHID HID;

static const uint8_t HID_REPORT_ID_GAMEPAD = 1;
static const uint8_t HID_REPORT_ID_CONSUMER = 2;

static const uint8_t hidReportDescriptor[] = {
    // === GAMEPAD (Report ID 1) ===
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x05,       // Usage (Gamepad)
    0xA1, 0x01,       // Collection (Application)
    0x85, HID_REPORT_ID_GAMEPAD, // Report ID (1)
    0x05, 0x09,       // Usage Page (Button)
    0x19, 0x01,       // Usage Minimum (Button 1)
    0x29, 0x40,       // Usage Maximum (Button 64)
    0x15, 0x00,       // Logical Minimum (0)
    0x25, 0x01,       // Logical Maximum (1)
    0x95, 0x40,       // Report Count (64)
    0x75, 0x01,       // Report Size (1)
    0x81, 0x02,       // Input (Data,Var,Abs)
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x15, 0x81,       // Logical Minimum (-127)
    0x25, 0x7F,       // Logical Maximum (127)
    0x75, 0x08,       // Report Size (8)
    0x95, 0x0A,       // Report Count (10)
    0x09, 0x30,       // Usage (X)
    0x09, 0x31,       // Usage (Y)
    0x09, 0x32,       // Usage (Z)
    0x09, 0x35,       // Usage (Rz)
    0x09, 0x33,       // Usage (Rx)
    0x09, 0x34,       // Usage (Ry)
    0x09, 0x36,       // Usage (Slider)
    0x09, 0x37,       // Usage (Dial)
    0x09, 0x40,       // Usage (Vx)
    0x09, 0x41,       // Usage (Vy)
    0x81, 0x02,       // Input (Data,Var,Abs)
    0xC0,             // End Collection

    // === CONSUMER CONTROL (Report ID 2) ===
    0x05, 0x0C,       // Usage Page (Consumer)
    0x09, 0x01,       // Usage (Consumer Control)
    0xA1, 0x01,       // Collection (Application)
    0x85, HID_REPORT_ID_CONSUMER, // Report ID (2)
    0x19, 0x00,       // Usage Minimum (0)
    0x2A, 0x3C, 0x02, // Usage Maximum (0x023C)
    0x15, 0x00,       // Logical Minimum (0)
    0x26, 0x3C, 0x02, // Logical Maximum (0x023C)
    0x95, 0x01,       // Report Count (1)
    0x75, 0x10,       // Report Size (16)
    0x81, 0x00,       // Input (Data,Array,Abs)
    0xC0              // End Collection
};

int8_t axisX = 0;      // Enc 2
int8_t axisY = 0;      // Enc 3
int8_t axisZ = 0;      // Clutch A
int8_t axisRZ = 0;     // Clutch B
int8_t axisRX = 0;     // Enc 4
int8_t axisRY = 0;     // Enc 5
int8_t axisSlider = 0; // Enc 6
int8_t axisDial = 0;   // Enc 7
int8_t axisVx = 0;     // Enc 8
int8_t axisVy = 0;     // Enc 9
uint64_t buttons = 0;

static const uint8_t HID_MAX_BUTTONS = 64;
static const uint8_t MATRIX_HID_MAX = 22; // 1-22 reportados ao HID

// ================================
// PINOUT - ESP32-S3-WROOM1 N8R8
// ================================

// Matrix 5x5
#define MATRIX_COLS 5
#define MATRIX_ROWS 5
const uint8_t colPins[MATRIX_COLS] = {4, 5, 6, 7, 8};
const uint8_t rowPins[MATRIX_ROWS] = {9, 10, 11, 12, 13};

// Encoders
#define NUM_ENCODERS 9
struct EncoderPins {
    uint8_t pinA;
    uint8_t pinB;
};

// Order:
// 0 = MFC
// 1 = Enc 2 (X)
// 2 = Enc 3 (Y)
// 3 = Enc 4 (Rx)
// 4 = Enc 5 (Ry)
// 5 = Enc 6 (Slider)
// 6 = Enc 7 (Dial)
// 7 = Enc 8 (Vx)
// 8 = Enc 9 (Vy)
const EncoderPins encoderPins[NUM_ENCODERS] = {
    {14, 15}, // ENC1 MFC
    {16, 17}, // ENC2 BB
    {18, 21}, // ENC3 MAP
    {38, 39}, // ENC4 TC
    {40, 41}, // ENC5 ABS
    {42, 47}, // ENC6 Lateral 1
    {48, 35}, // ENC7 Lateral 2 (test PSRAM)
    {36, 37}, // ENC8 Lateral 3 (test PSRAM)
    {3, 46}   // ENC9 Lateral 4 (strap pins)
};

// Hall sensors
static const uint8_t HALL_A_PIN = 1;
static const uint8_t HALL_B_PIN = 2;

// UART to WT32
HardwareSerial ButtonBoxSerial(1);
static const uint32_t UART_BAUD = 115200;
static const int UART_TX_PIN = 43;

// ================================
// MATRIX STATE
// ================================
bool buttonStates[MATRIX_ROWS][MATRIX_COLS] = {false};
bool prevButtonStates[MATRIX_ROWS][MATRIX_COLS] = {false};
unsigned long lastDebounceTime[MATRIX_ROWS][MATRIX_COLS] = {0};
const unsigned long DEBOUNCE_DELAY = 30;

inline uint8_t getButtonNumber(uint8_t row, uint8_t col) {
    return (row * MATRIX_COLS) + col + 1;
}

inline bool isButtonPressed(uint8_t buttonNum) {
    uint8_t idx = buttonNum - 1;
    return buttonStates[idx / MATRIX_COLS][idx % MATRIX_COLS];
}

// Buttons reserved
static const uint8_t BUTTON_MFC = 1;   // Slot 1
static const uint8_t BUTTON_SHIFT = 25; // Slot 25 (internal only)

// ================================
// ENCODERS
// ================================
struct EncoderState {
    int8_t lastEncoded;
    int32_t encoderValue;
    bool lastA;
    bool lastB;
    unsigned long lastChangeTime;
    float smoothedValue;
    int8_t lastSentValue;
    int8_t accumulator;
};

EncoderState encoderStates[NUM_ENCODERS];
const unsigned long ENCODER_DEBOUNCE_US = 800;
const float ENCODER_SMOOTHING = 0.55;
const int8_t ENCODER_THRESHOLD = 1;
const int8_t ENC_TABLE[16] = {
    0, -1,  1, 0,
    1,  0,  0,-1,
   -1,  0,  0, 1,
    0,  1, -1, 0
};

// Encoder mode (global) for encoders 2..9 (axes <-> buttons)
bool encoderButtonMode = false;
const unsigned long MODE_HOLD_MS = 1500;
unsigned long comboHoldStart = 0;

// Virtual buttons
struct VirtualButtonPulse {
    uint8_t id;
    bool active;
    unsigned long releaseAt;
};

// Encoders 2..9 in button mode use 16 buttons: 23-38
VirtualButtonPulse encoderPulses[16] = {
    {23, false, 0}, {24, false, 0},
    {25, false, 0}, {26, false, 0},
    {27, false, 0}, {28, false, 0},
    {29, false, 0}, {30, false, 0},
    {31, false, 0}, {32, false, 0},
    {33, false, 0}, {34, false, 0},
    {35, false, 0}, {36, false, 0},
    {37, false, 0}, {38, false, 0}
};

// ================================
// CLUTCH CONFIG
// ================================
Preferences prefs;

enum ClutchMode : uint8_t {
    CLUTCH_DUAL = 0,
    CLUTCH_MIRROR = 1,
    CLUTCH_BITE = 2,
    CLUTCH_PROGRESSIVE = 3,
    CLUTCH_SINGLE_L = 4,
    CLUTCH_SINGLE_R = 5
};

struct ClutchConfig {
    ClutchMode mode;
    uint16_t hallMinA;
    uint16_t hallMaxA;
    uint16_t hallMinB;
    uint16_t hallMaxB;
    uint8_t bitePoint; // 0-100
};

ClutchConfig clutchCfg;

bool calibratingHall = false;
bool adjustingBite = false;

// ================================
// MFC MENU (Adjustable Mode)
// ================================
enum MfcMenuItem : uint8_t {
    MFC_CLUTCH = 0,
    MFC_BITE,
    MFC_CALIB,
    MFC_ENC_MODE,
    MFC_BRIGHT,
    MFC_PAGE,
    MFC_VOL_SYS,
    MFC_VOL_A,
    MFC_VOL_B,
    MFC_TC2,
    MFC_TC3,
    MFC_TYRE,
    MFC_ERS,
    MFC_FUEL,
    MFC_RESET,
    MFC_COUNT
};

const char* mfcMenuNames[MFC_COUNT] = {
    "CLUTCH",
    "BITE",
    "CALIB",
    "ENC MODE",
    "BRIGHT",
    "PAGE",
    "VOL_SYS",
    "VOL_A",
    "VOL_B",
    "TC2",
    "TC3",
    "TYRE",
    "ERS",
    "FUEL",
    "RESET"
};

int8_t mfcIndex = 0;
bool mfcAdjustMode = false; // false = navigation, true = adjusting value

// Adjustable values
int16_t brightnessValue = 220; // 15-255 (tela + LEDs)
int8_t pageValue = 0;          // 0-6 pages

// Virtual buttons 50-59 for MFC menu items
VirtualButtonPulse mfcVirtualButtons[10] = {
    {50, false, 0}, {51, false, 0}, // TC2 UP/DN
    {52, false, 0}, {53, false, 0}, // TC3 UP/DN
    {54, false, 0}, {55, false, 0}, // TYRE UP/DN
    {56, false, 0}, {57, false, 0}, // VOL_A UP/DN
    {58, false, 0}, {59, false, 0}  // VOL_B UP/DN
};

// Matrix button slots for multimedia (when VOL_SYS active)
static const uint8_t BUTTON_RADIO = 10;  // Slot 10 = MUTE
static const uint8_t BUTTON_FLASH = 11;  // Slot 11 = PLAY/PAUSE

// ================================
// UTILITIES
// ================================

typedef struct __attribute__((packed)) {
    uint64_t buttons;
    int8_t x;
    int8_t y;
    int8_t z;
    int8_t rz;
    int8_t rx;
    int8_t ry;
    int8_t slider;
    int8_t dial;
    int8_t vx;
    int8_t vy;
} GamepadReport;

void sendGamepad() {
    GamepadReport report = {
        buttons,
        axisX,
        axisY,
        axisZ,
        axisRZ,
        axisRX,
        axisRY,
        axisSlider,
        axisDial,
        axisVx,
        axisVy
    };
    HID.sendReport(HID_REPORT_ID_GAMEPAD, &report, sizeof(report));
}

void sendConsumerControl(uint16_t code) {
    typedef struct __attribute__((packed)) {
        uint16_t usage;
    } ConsumerReport;

    ConsumerReport report = {code};
    HID.sendReport(HID_REPORT_ID_CONSUMER, &report, sizeof(report));

    // Release immediately
    delay(10);
    report.usage = 0;
    HID.sendReport(HID_REPORT_ID_CONSUMER, &report, sizeof(report));
}

void uartSend(const char* cat, const char* func, const char* val) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "$%s:%s:%s\n", cat, func, val);
    ButtonBoxSerial.print(buffer);
}

void uartSendInt(const char* cat, const char* func, int value) {
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "$%s:%s:%d\n", cat, func, value);
    ButtonBoxSerial.print(buffer);
}

void saveConfig() {
    prefs.putUChar("clMode", (uint8_t)clutchCfg.mode);
    prefs.putUShort("h1min", clutchCfg.hallMinA);
    prefs.putUShort("h1max", clutchCfg.hallMaxA);
    prefs.putUShort("h2min", clutchCfg.hallMinB);
    prefs.putUShort("h2max", clutchCfg.hallMaxB);
    prefs.putUChar("bite", clutchCfg.bitePoint);
    prefs.putBool("encBtn", encoderButtonMode);
}

void loadConfig() {
    clutchCfg.mode = (ClutchMode)prefs.getUChar("clMode", CLUTCH_DUAL);
    clutchCfg.hallMinA = prefs.getUShort("h1min", 0);
    clutchCfg.hallMaxA = prefs.getUShort("h1max", 4095);
    clutchCfg.hallMinB = prefs.getUShort("h2min", 0);
    clutchCfg.hallMaxB = prefs.getUShort("h2max", 4095);
    clutchCfg.bitePoint = prefs.getUChar("bite", 60);
    encoderButtonMode = prefs.getBool("encBtn", false);
}

int8_t mapHallToAxis(uint16_t raw, uint16_t minV, uint16_t maxV) {
    if (maxV <= minV + 5) {
        return 0;
    }
    if (raw < minV) raw = minV;
    if (raw > maxV) raw = maxV;
    uint32_t range = maxV - minV;
    uint32_t val = raw - minV;
    uint8_t scaled = (uint8_t)((val * 127UL) / range);
    return (int8_t)scaled; // 0..127
}

bool shouldReportMatrixButton(uint8_t buttonNum) {
    return buttonNum <= MATRIX_HID_MAX;
}

// ================================
// MATRIX
// ================================
void setupButtonMatrix() {
    for (uint8_t col = 0; col < MATRIX_COLS; col++) {
        pinMode(colPins[col], OUTPUT);
        digitalWrite(colPins[col], HIGH);
    }
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        pinMode(rowPins[row], INPUT_PULLUP);
    }
}

void scanButtonMatrix() {
    unsigned long currentTime = millis();

    for (uint8_t col = 0; col < MATRIX_COLS; col++) {
        for (uint8_t c = 0; c < MATRIX_COLS; c++) {
            digitalWrite(colPins[c], (c == col) ? LOW : HIGH);
        }
        delayMicroseconds(8);

        for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
            bool reading = (digitalRead(rowPins[row]) == LOW);

            if (reading != prevButtonStates[row][col]) {
                lastDebounceTime[row][col] = currentTime;
            }

            if ((currentTime - lastDebounceTime[row][col]) > DEBOUNCE_DELAY) {
                if (reading != buttonStates[row][col]) {
                    buttonStates[row][col] = reading;

                    uint8_t buttonNum = getButtonNumber(row, col);
                    if (buttonNum <= HID_MAX_BUTTONS && shouldReportMatrixButton(buttonNum)) {
                        if (reading) {
                            buttons |= (1ULL << (buttonNum - 1));
                        } else {
                            buttons &= ~(1ULL << (buttonNum - 1));
                        }
                        sendGamepad();
                    }
                }
            }

            prevButtonStates[row][col] = reading;
        }
    }

    for (uint8_t col = 0; col < MATRIX_COLS; col++) {
        digitalWrite(colPins[col], HIGH);
    }
}

// ================================
// ENCODERS
// ================================
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
}

void triggerPulse(VirtualButtonPulse& pulse) {
    if (pulse.active) {
        buttons &= ~(1ULL << (pulse.id - 1));
        pulse.active = false;
    }
    pulse.active = true;
    pulse.releaseAt = millis() + 30;
    buttons |= (1ULL << (pulse.id - 1));
}

void handleEncoderAxis(uint8_t idx, int8_t val) {
    if (idx == 1) axisX = val;
    else if (idx == 2) axisY = val;
    else if (idx == 3) axisRX = val;
    else if (idx == 4) axisRY = val;
    else if (idx == 5) axisSlider = val;
    else if (idx == 6) axisDial = val;
    else if (idx == 7) axisVx = val;
    else if (idx == 8) axisVy = val;
    sendGamepad();
}

void handleEncoderButton(uint8_t idx, int8_t step) {
    uint8_t localIndex = idx - 1; // 1..8 -> 0..7
    uint8_t pulseIdxCW = localIndex * 2;
    uint8_t pulseIdxCCW = (localIndex * 2) + 1;

    if (step > 0) {
        triggerPulse(encoderPulses[pulseIdxCW]);
    } else {
        triggerPulse(encoderPulses[pulseIdxCCW]);
    }
    sendGamepad();
}

void handleMfcRotate(int8_t step) {
    if (mfcAdjustMode) {
        // Adjust mode - modify values based on current menu item
        MfcMenuItem item = (MfcMenuItem)mfcIndex;

        if (item == MFC_BITE) {
            int newBite = (int)clutchCfg.bitePoint + step;
            if (newBite < 0) newBite = 0;
            if (newBite > 100) newBite = 100;
            clutchCfg.bitePoint = (uint8_t)newBite;
            saveConfig();
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", clutchCfg.bitePoint);
            uartSend("BITE", "VAL", buf);
        } else if (item == MFC_BRIGHT) {
            int newBright = brightnessValue + (step * 15);
            if (newBright < 15) newBright = 15;
            if (newBright > 255) newBright = 255;
            brightnessValue = newBright;
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", brightnessValue);
            uartSend("BRIGHT", "VAL", buf);
        } else if (item == MFC_PAGE) {
            pageValue += step;
            if (pageValue < 0) pageValue = 6;
            if (pageValue > 6) pageValue = 0;
            uartSend("PAGE", step > 0 ? "NEXT" : "PREV", "");
        } else if (item == MFC_VOL_SYS) {
            // HID Consumer Control Volume (implemented separately)
            sendConsumerControl(step > 0 ? 0xE9 : 0xEA); // Volume Up/Down
        } else if (item == MFC_VOL_A) {
            // Virtual buttons 56 (UP) / 57 (DN)
            triggerVirtualButton(step > 0 ? 56 : 57);
            sendGamepad();
        } else if (item == MFC_VOL_B) {
            // Virtual buttons 58 (UP) / 59 (DN)
            triggerVirtualButton(step > 0 ? 58 : 59);
            sendGamepad();
        } else if (item == MFC_TC2) {
            // Virtual buttons 50 (UP) / 51 (DN)
            triggerVirtualButton(step > 0 ? 50 : 51);
            sendGamepad();
        } else if (item == MFC_TC3) {
            // Virtual buttons 52 (UP) / 53 (DN)
            triggerVirtualButton(step > 0 ? 52 : 53);
            sendGamepad();
        } else if (item == MFC_TYRE) {
            // Virtual buttons 54 (UP) / 55 (DN)
            triggerVirtualButton(step > 0 ? 54 : 55);
            sendGamepad();
        }
    } else {
        // Navigation mode - scroll through menu items
        mfcIndex += step;
        if (mfcIndex < 0) mfcIndex = MFC_COUNT - 1;
        if (mfcIndex >= MFC_COUNT) mfcIndex = 0;
        uartSend("MFC", "NAV", mfcMenuNames[mfcIndex]);
    }
}

void scanEncoders() {
    for (uint8_t i = 0; i < NUM_ENCODERS; i++) {
        bool currentA = digitalRead(encoderPins[i].pinA);
        bool currentB = digitalRead(encoderPins[i].pinB);

        if (currentA != encoderStates[i].lastA || currentB != encoderStates[i].lastB) {
            unsigned long now = micros();
            if (now - encoderStates[i].lastChangeTime < ENCODER_DEBOUNCE_US) {
                continue;
            }
            encoderStates[i].lastChangeTime = now;

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

                if (i == 0) {
                    handleMfcRotate(step);
                } else if (i >= 1 && i <= 8) {
                    if (encoderButtonMode) {
                        handleEncoderButton(i, step);
                    } else {
                        encoderStates[i].smoothedValue = (ENCODER_SMOOTHING * encoderStates[i].encoderValue) +
                                                          ((1.0 - ENCODER_SMOOTHING) * encoderStates[i].smoothedValue);
                        int8_t smoothed = (int8_t)round(encoderStates[i].smoothedValue);
                        int8_t val = constrain(smoothed, -127, 127);

                        if (abs(val - encoderStates[i].lastSentValue) >= ENCODER_THRESHOLD) {
                            encoderStates[i].lastSentValue = val;
                            handleEncoderAxis(i, val);
                        }
                    }
                }
            }
        }

        encoderStates[i].lastA = currentA;
        encoderStates[i].lastB = currentB;
    }
}

void releaseVirtualButtonPulses() {
    unsigned long now = millis();
    bool changed = false;

    // Release encoder button pulses (buttons 23-38)
    for (auto &pulse : encoderPulses) {
        if (pulse.active && now >= pulse.releaseAt) {
            buttons &= ~(1ULL << (pulse.id - 1));
            pulse.active = false;
            changed = true;
        }
    }

    // Release MFC menu virtual buttons (buttons 50-59)
    for (auto &pulse : mfcVirtualButtons) {
        if (pulse.active && now >= pulse.releaseAt) {
            buttons &= ~(1ULL << (pulse.id - 1));
            pulse.active = false;
            changed = true;
        }
    }

    if (changed) {
        sendGamepad();
    }
}

// ================================
// CLUTCH + HALL
// ================================
void updateClutches() {
    uint16_t rawA = analogRead(HALL_A_PIN);
    uint16_t rawB = analogRead(HALL_B_PIN);

    if (calibratingHall) {
        if (rawA < clutchCfg.hallMinA) clutchCfg.hallMinA = rawA;
        if (rawA > clutchCfg.hallMaxA) clutchCfg.hallMaxA = rawA;
        if (rawB < clutchCfg.hallMinB) clutchCfg.hallMinB = rawB;
        if (rawB > clutchCfg.hallMaxB) clutchCfg.hallMaxB = rawB;
    }

    int8_t a = mapHallToAxis(rawA, clutchCfg.hallMinA, clutchCfg.hallMaxA);
    int8_t b = mapHallToAxis(rawB, clutchCfg.hallMinB, clutchCfg.hallMaxB);

    int8_t outA = a;
    int8_t outB = b;

    if (clutchCfg.mode == CLUTCH_MIRROR) {
        int8_t avg = (int8_t)((a + b) / 2);
        outA = avg;
        outB = avg;
    } else if (clutchCfg.mode == CLUTCH_BITE) {
        // F1 Style - Assimétrico com Estado de Largada
        static bool launchMode = false;

        // Detecta modo largada: AMBAS acima de 95% (~120/127)
        if (a > 120 && b > 120) {
            launchMode = true;
        }

        // Sai do modo largada: AMBAS abaixo de 5% (~6/127)
        if (a < 6 && b < 6) {
            launchMode = false;
        }

        int8_t combined = (a > b) ? a : b;  // MAX dos dois
        int8_t minBite = (int8_t)((clutchCfg.bitePoint * 127) / 100);

        // Modo largada: quando UMA é solta, REMAPEIA a outra de 0-100% para 0-bite%
        if (launchMode && (a == 0 || b == 0)) {
            // Remapeia: 100% da paddle → bite%, permitindo controle fino
            combined = (int8_t)((combined * clutchCfg.bitePoint) / 100);
        }

        outA = combined;
        outB = combined;
    } else if (clutchCfg.mode == CLUTCH_PROGRESSIVE) {
        int8_t combined = (a > b) ? a : b;  // MAX dos dois
        int16_t limit = 127 - a;           // limite inverso da esquerda
        if (limit < 0) limit = 0;
        int8_t limitedB = b;
        if (limitedB > limit) limitedB = (int8_t)limit;
        outA = combined;
        outB = limitedB;
    } else if (clutchCfg.mode == CLUTCH_SINGLE_L) {
        outA = a;
        outB = 0;
    } else if (clutchCfg.mode == CLUTCH_SINGLE_R) {
        outA = 0;
        outB = b;
    }

    if (outA != axisZ || outB != axisRZ) {
        axisZ = outA;
        axisRZ = outB;
        sendGamepad();
    }
}

void cycleClutchMode() {
    if (clutchCfg.mode == CLUTCH_DUAL) clutchCfg.mode = CLUTCH_MIRROR;
    else if (clutchCfg.mode == CLUTCH_MIRROR) clutchCfg.mode = CLUTCH_BITE;
    else if (clutchCfg.mode == CLUTCH_BITE) clutchCfg.mode = CLUTCH_PROGRESSIVE;
    else if (clutchCfg.mode == CLUTCH_PROGRESSIVE) clutchCfg.mode = CLUTCH_SINGLE_L;
    else if (clutchCfg.mode == CLUTCH_SINGLE_L) clutchCfg.mode = CLUTCH_SINGLE_R;
    else clutchCfg.mode = CLUTCH_DUAL;

    if (clutchCfg.mode == CLUTCH_DUAL) uartSend("CLUTCH", "MODE", "DUAL");
    else if (clutchCfg.mode == CLUTCH_MIRROR) uartSend("CLUTCH", "MODE", "MIRROR");
    else if (clutchCfg.mode == CLUTCH_BITE) uartSend("CLUTCH", "MODE", "BITE");
    else if (clutchCfg.mode == CLUTCH_PROGRESSIVE) uartSend("CLUTCH", "MODE", "PROGRESSIVE");
    else if (clutchCfg.mode == CLUTCH_SINGLE_L) uartSend("CLUTCH", "MODE", "SINGLE_L");
    else uartSend("CLUTCH", "MODE", "SINGLE_R");

    saveConfig();
}

// ================================
// MFC PRESS HANDLING (Adjustable Mode)
// ================================
bool lastMfcPressed = false;

void triggerVirtualButton(uint8_t btnId, uint16_t durationMs = 100) {
    for (uint8_t i = 0; i < 10; i++) {
        if (mfcVirtualButtons[i].id == btnId) {
            mfcVirtualButtons[i].active = true;
            mfcVirtualButtons[i].releaseAt = millis() + durationMs;
            break;
        }
    }
}

void handleMfcPress() {
    bool mfcPressed = isButtonPressed(BUTTON_MFC);
    bool shiftPressed = isButtonPressed(BUTTON_SHIFT);

    // Long press for encoder mode toggle (shift + MFC)
    if (mfcPressed && shiftPressed) {
        if (comboHoldStart == 0) {
            comboHoldStart = millis();
        } else if (millis() - comboHoldStart >= MODE_HOLD_MS) {
            encoderButtonMode = !encoderButtonMode;
            comboHoldStart = UINT32_MAX;

            if (encoderButtonMode) {
                axisX = axisY = axisRX = axisRY = 0;
                axisSlider = axisDial = axisVx = axisVy = 0;
            }
            saveConfig();
            uartSend("MODE", "ENC", encoderButtonMode ? "BTN" : "AXIS");
            sendGamepad();
        }
    } else {
        comboHoldStart = 0;
    }

    if (shiftPressed) {
        lastMfcPressed = mfcPressed;
        return;
    }

    if (mfcPressed && !lastMfcPressed) {
        MfcMenuItem item = (MfcMenuItem)mfcIndex;

        // Toggle adjust mode for certain items
        if (item == MFC_BITE || item == MFC_BRIGHT || item == MFC_PAGE ||
            item == MFC_VOL_SYS || item == MFC_VOL_A || item == MFC_VOL_B ||
            item == MFC_TC2 || item == MFC_TC3 || item == MFC_TYRE) {

            mfcAdjustMode = !mfcAdjustMode;

            if (mfcAdjustMode) {
                // Entering adjust mode
                if (item == MFC_BITE) {
                    adjustingBite = true;
                    uartSend("MFC", "ADJUST", "BITE");
                } else if (item == MFC_BRIGHT) {
                    uartSend("MFC", "ADJUST", "BRIGHT");
                } else if (item == MFC_PAGE) {
                    uartSend("MFC", "ADJUST", "PAGE");
                } else if (item == MFC_VOL_SYS) {
                    uartSend("MFC", "ADJUST", "VOL_SYS");
                } else if (item == MFC_VOL_A) {
                    uartSend("MFC", "ADJUST", "VOL_A");
                } else if (item == MFC_VOL_B) {
                    uartSend("MFC", "ADJUST", "VOL_B");
                } else if (item == MFC_TC2) {
                    uartSend("MFC", "ADJUST", "TC2");
                } else if (item == MFC_TC3) {
                    uartSend("MFC", "ADJUST", "TC3");
                } else if (item == MFC_TYRE) {
                    uartSend("MFC", "ADJUST", "TYRE");
                }
            } else {
                // Exiting adjust mode
                if (item == MFC_BITE) {
                    adjustingBite = false;
                    uartSend("MFC", "CONFIRM", "BITE");
                } else if (item == MFC_BRIGHT) {
                    uartSend("MFC", "CONFIRM", "BRIGHT");
                }
                uartSend("MFC", "NAV", mfcMenuNames[mfcIndex]);
            }
        } else {
            // One-press actions
            if (item == MFC_CLUTCH) {
                cycleClutchMode();
            } else if (item == MFC_CALIB) {
                calibratingHall = !calibratingHall;
                if (calibratingHall) {
                    clutchCfg.hallMinA = 4095;
                    clutchCfg.hallMaxA = 0;
                    clutchCfg.hallMinB = 4095;
                    clutchCfg.hallMaxB = 0;
                    uartSend("CALIB", "START", "HALL");
                } else {
                    bool invalidA = clutchCfg.hallMaxA <= (clutchCfg.hallMinA + 5);
                    bool invalidB = clutchCfg.hallMaxB <= (clutchCfg.hallMinB + 5);
                    if (invalidA || invalidB) {
                        clutchCfg.hallMinA = 0;
                        clutchCfg.hallMaxA = 4095;
                        clutchCfg.hallMinB = 0;
                        clutchCfg.hallMaxB = 4095;
                        saveConfig();
                        uartSend("CALIB", "INVALID", "HALL");
                    } else {
                        saveConfig();
                        uartSend("CALIB", "DONE", "HALL");
                    }
                }
            } else if (item == MFC_ENC_MODE) {
                encoderButtonMode = !encoderButtonMode;
                if (encoderButtonMode) {
                    axisX = axisY = axisRX = axisRY = 0;
                    axisSlider = axisDial = axisVx = axisVy = 0;
                }
                saveConfig();
                uartSend("MODE", "ENC", encoderButtonMode ? "BTN" : "AXIS");
            } else if (item == MFC_RESET) {
                clutchCfg.mode = CLUTCH_DUAL;
                clutchCfg.bitePoint = 60;
                clutchCfg.hallMinA = 0;
                clutchCfg.hallMaxA = 4095;
                clutchCfg.hallMinB = 0;
                clutchCfg.hallMaxB = 4095;
                encoderButtonMode = false;
                brightnessValue = 220;
                pageValue = 0;
                saveConfig();
                uartSend("SYS", "RESET", "OK");
            }
        }
    }

    lastMfcPressed = mfcPressed;
}

// ================================
// SHIFT + CLUTCH MODE (optional)
// ================================
unsigned long clutchComboStart = 0;
void handleShiftClutchCombo() {
    bool shift = isButtonPressed(BUTTON_SHIFT);
    if (!shift) {
        clutchComboStart = 0;
        return;
    }

    bool clutchA = axisZ > 100;
    bool clutchB = axisRZ > 100;

    if (clutchA && clutchB) {
        if (clutchComboStart == 0) {
            clutchComboStart = millis();
        } else if (millis() - clutchComboStart >= 2000) {
            cycleClutchMode();
            clutchComboStart = UINT32_MAX;
        }
    } else {
        clutchComboStart = 0;
    }
}

// ================================
// SETUP + LOOP
// ================================
void setup() {
    HID.setReportDescriptor(hidReportDescriptor, sizeof(hidReportDescriptor));
    HID.begin();
    USB.begin();

    Serial.begin(115200);
    delay(1500);

    ButtonBoxSerial.begin(UART_BAUD, SERIAL_8N1, -1, UART_TX_PIN);

    prefs.begin("wheel", false);
    loadConfig();

    analogReadResolution(12);
    setupButtonMatrix();
    setupEncoders();

    uartSend("SYS", "BOOT", DEVICE_NAME);
}

// ================================
// MULTIMEDIA BUTTONS (when VOL_SYS active)
// ================================
bool lastRadioPressed = false;
bool lastFlashPressed = false;

void handleMultimediaButtons() {
    // Only active when VOL_SYS is in adjust mode
    if (!mfcAdjustMode || (MfcMenuItem)mfcIndex != MFC_VOL_SYS) {
        lastRadioPressed = false;
        lastFlashPressed = false;
        return;
    }

    bool radioPressed = isButtonPressed(BUTTON_RADIO);
    bool flashPressed = isButtonPressed(BUTTON_FLASH);

    // RADIO = MUTE (0xE2)
    if (radioPressed && !lastRadioPressed) {
        sendConsumerControl(0xE2);
        uartSend("MEDIA", "MUTE", "1");
    }
    lastRadioPressed = radioPressed;

    // FLASH = PLAY/PAUSE (0xCD)
    if (flashPressed && !lastFlashPressed) {
        sendConsumerControl(0xCD);
        uartSend("MEDIA", "PLAY_PAUSE", "1");
    }
    lastFlashPressed = flashPressed;
}

void loop() {
    scanButtonMatrix();
    handleMfcPress();
    handleMultimediaButtons();
    scanEncoders();
    releaseVirtualButtonPulses();
    updateClutches();
    handleShiftClutchCombo();

    delay(1);
}
