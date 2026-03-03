#include <Arduino.h>
#include "USB.h"
#include "USBHID.h"
#include "USBHIDConsumerControl.h"
#include <Preferences.h>
#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include <Adafruit_PWMServoDriver.h>

#define DEVICE_NAME "ESP-ButtonBox-WHEEL"

// I2C pins
#define I2C_SDA 8
#define I2C_SCL 9

// ================================
// USB HID GAMEPAD (Custom: 64 buttons + 10 axes)
// ================================

// Custom gamepad descriptor (64 buttons + 10 axes + 1 HAT/POV)
static const uint8_t customGamepadDescriptor[] = {
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x05,       // Usage (Gamepad)
    0xA1, 0x01,       // Collection (Application)
    0x85, HID_REPORT_ID_GAMEPAD, // Report ID (3 from framework enum)

    // --- 64 Buttons ---
    0x05, 0x09,       // Usage Page (Button)
    0x19, 0x01,       // Usage Minimum (Button 1)
    0x29, 0x40,       // Usage Maximum (Button 64)
    0x15, 0x00,       // Logical Minimum (0)
    0x25, 0x01,       // Logical Maximum (1)
    0x95, 0x40,       // Report Count (64)
    0x75, 0x01,       // Report Size (1)
    0x81, 0x02,       // Input (Data,Var,Abs)

    // --- HAT/POV Switch (4 bits) ---
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x39,       // Usage (Hat Switch)
    0x15, 0x01,       // Logical Minimum (1)
    0x25, 0x08,       // Logical Maximum (8)
    0x35, 0x00,       // Physical Minimum (0)
    0x46, 0x3B, 0x01, // Physical Maximum (315)
    0x65, 0x14,       // Unit (Degrees)
    0x75, 0x04,       // Report Size (4)
    0x95, 0x01,       // Report Count (1)
    0x81, 0x42,       // Input (Data,Var,Abs,Null)

    // --- 4 bits padding (to round to byte) ---
    0x75, 0x04,       // Report Size (4)
    0x95, 0x01,       // Report Count (1)
    0x81, 0x01,       // Input (Const) - padding

    // --- 10 Axes ---
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
    0xC0              // End Collection
};

// Custom gamepad device class (for USBHID::addDevice registration)
class CustomGamepad : public USBHIDDevice {
private:
    USBHID hid;
public:
    CustomGamepad() : hid() {
        static bool initialized = false;
        if (!initialized) {
            initialized = true;
            hid.addDevice(this, sizeof(customGamepadDescriptor));
        }
    }
    void begin() { hid.begin(); }
    bool sendReport(const void* data, size_t len) {
        return hid.SendReport(HID_REPORT_ID_GAMEPAD, data, len);
    }
    uint16_t _onGetDescriptor(uint8_t* dst) override {
        memcpy(dst, customGamepadDescriptor, sizeof(customGamepadDescriptor));
        return sizeof(customGamepadDescriptor);
    }
};

// Global HID device instances
CustomGamepad Gamepad;
USBHIDConsumerControl ConsumerControl;


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
static const uint8_t MATRIX_HID_MAX = 37; // slots usados: 1-5 enc SWs, 9-14 LEDs esq, 17-22 LEDs dir, 25-30 5-way+SHIFT, 33-37 borb+tras+extra

// ================================
// PINOUT - ESP32-S3-WROOM1 N8R8
// ================================

// Matrix 8x8 via MCP23017
#define MATRIX_COLS 8
#define MATRIX_ROWS 8
Adafruit_MCP23X17 mcp;

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

// UART to WT32 (also used for debug output via CH340 on Mac)
HardwareSerial ButtonBoxSerial(1);
static const uint32_t UART_BAUD = 115200;
static const int UART_TX_PIN = 43;

// Debug UART (goes through CH340 to Mac for monitoring)
// With USB_MODE=0, Serial = USB CDC (goes to native USB/Windows), NOT CH340
// So we use ButtonBoxSerial for debug output
#define DBG(msg) ButtonBoxSerial.println(msg)
#define DBGF(...) { char _dbuf[128]; snprintf(_dbuf, sizeof(_dbuf), __VA_ARGS__); ButtonBoxSerial.println(_dbuf); }

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
static const uint8_t BUTTON_SHIFT = 30; // Slot 30 (internal only)

// 5-way joystick slots (HAT/POV + center button)
// Directions are consumed by HAT logic and NOT reported as individual HID buttons
static const uint8_t FIVEWAY_UP     = 25;  // Slot 25 → HAT N
static const uint8_t FIVEWAY_DOWN   = 26;  // Slot 26 → HAT S
static const uint8_t FIVEWAY_LEFT   = 27;  // Slot 27 → HAT W
static const uint8_t FIVEWAY_RIGHT  = 28;  // Slot 28 → HAT E
static const uint8_t FIVEWAY_CENTER = 29;  // Slot 29 → HID button 29 (OK/confirm)

// HAT switch value (0 = released/null, 1-8 = directions)
// 1=N, 2=NE, 3=E, 4=SE, 5=S, 6=SW, 7=W, 8=NW
uint8_t hatValue = 0;

bool isFivewayDirection(uint8_t buttonNum) {
    return buttonNum == FIVEWAY_UP || buttonNum == FIVEWAY_DOWN ||
           buttonNum == FIVEWAY_LEFT || buttonNum == FIVEWAY_RIGHT;
}

void updateHatFromMatrix() {
    bool up    = isButtonPressed(FIVEWAY_UP);
    bool down  = isButtonPressed(FIVEWAY_DOWN);
    bool left  = isButtonPressed(FIVEWAY_LEFT);
    bool right = isButtonPressed(FIVEWAY_RIGHT);

    if (up && right)       hatValue = 2;  // NE
    else if (up && left)   hatValue = 8;  // NW
    else if (down && right) hatValue = 4; // SE
    else if (down && left)  hatValue = 6; // SW
    else if (up)            hatValue = 1; // N
    else if (right)         hatValue = 3; // E
    else if (down)          hatValue = 5; // S
    else if (left)          hatValue = 7; // W
    else                    hatValue = 0; // Released (null state)
}

// ================================
// PCA9685 FRONT BUTTON LEDs
// ================================
// GPB1 (laranja) slots 9–14 : Extra1-4, RADIO, FLASH → CH 0-5
// GPB2 (cinza)   slots 17–22: Frontal 1-6            → CH 6-11
// GPB3 (verde)   slots 25–30: 5-way + SHIFT           → sem LED
// GPB4 (marrom)  slots 33–37: borb, tras, extra       → sem LED
// Adjust SLOT_TO_LED_CH[] to match your physical wiring.

#define LED_COUNT           12
#define LED_PCA_ADDR        0x40
#define LED_PWM_FREQ        1000      // Hz — eliminates flicker
#define LED_MAX             4095      // 12-bit max
#define LED_BREATH_PERIOD   2500      // ms per full breath cycle
#define LED_BREATH_AMP      900       // ±amplitude around idle level
#define LED_FLASH_MS        120       // button flash duration (ms)
#define LED_SWEEP_STEP_MS   35        // ms between sweep steps
#define LED_SHIFT_BLINK_MS  200       // SHIFT blink half-period (ms)

Adafruit_PWMServoDriver ledDriver(LED_PCA_ADDR);
bool          ledAvailable       = false;
uint16_t      ledIdleBase        = 2048;   // breathing center; updated by BRIGHT
bool          ledFlashActive[LED_COUNT];
unsigned long ledFlashEnd[LED_COUNT];
bool          ledSweepActive      = false;
int8_t        ledSweepPos         = 0;
int8_t        ledSweepDir         = 1;     // +1 = L→R, -1 = R→L
unsigned long ledSweepNext        = 0;
bool          ledShiftBlink       = false;
unsigned long ledShiftBlinkNext   = 0;

// slot index → PCA9685 channel; -1 means no LED on that slot
static const int8_t SLOT_TO_LED_CH[38] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1,  //  0– 8  (unused, encoder SWs, GPB0 livres)
     0,  1,  2,  3,  4,  5,              //  9–14  Extra1-4, RADIO, FLASH → CH 0-5  (GPB1)
    -1, -1,                               // 15–16  (GPB1 livres)
     6,  7,  8,  9, 10, 11,              // 17–22  Frontal 1-6 → CH 6-11            (GPB2)
    -1, -1,                               // 23–24  (GPB2 livres)
    -1, -1, -1, -1, -1, -1, -1, -1,      // 25–32  (5-way, SHIFT, GPB3 livres)
    -1, -1, -1, -1, -1                    // 33–37  (borboletas, traseiros, extra)   (GPB4)
};

inline void ledSetCh(uint8_t ch, uint16_t brightness) {
    if (!ledAvailable || ch >= LED_COUNT) return;
    ledDriver.setPWM(ch, 0, brightness);
}

void setupLEDs() {
    memset(ledFlashActive, false, sizeof(ledFlashActive));
    memset(ledFlashEnd,    0,     sizeof(ledFlashEnd));
    // Safe I2C probe before calling begin()
    Wire.beginTransmission(LED_PCA_ADDR);
    if (Wire.endTransmission() != 0) {
        DBG("[WARN] PCA9685 not found at 0x40 - front LEDs disabled");
        ledAvailable = false;
        return;
    }
    ledDriver.begin();
    ledDriver.setPWMFreq(LED_PWM_FREQ);
    ledAvailable = true;
    DBG("[I2C] PCA9685 LED driver OK at 0x40");
}

void ledBootSweep() {
    if (!ledAvailable) return;
    // All off
    for (uint8_t i = 0; i < LED_COUNT; i++) ledSetCh(i, 0);
    delay(300);
    // Close inward: symmetric pairs from outside → center
    // Left col top→bottom (CH0-5), Right col bottom→top (CH11-6)
    for (uint8_t step = 0; step < 6; step++) {
        ledSetCh(step,      LED_MAX);
        ledSetCh(11 - step, LED_MAX);
        delay(LED_SWEEP_STEP_MS * 2);
    }
    // Three quick blinks
    for (uint8_t b = 0; b < 3; b++) {
        for (uint8_t i = 0; i < LED_COUNT; i++) ledSetCh(i, 0);
        delay(80);
        for (uint8_t i = 0; i < LED_COUNT; i++) ledSetCh(i, LED_MAX);
        delay(80);
    }
    // Fade down to idle level
    for (int32_t br = LED_MAX; br >= (int32_t)ledIdleBase; br -= 80) {
        for (uint8_t i = 0; i < LED_COUNT; i++) ledSetCh(i, (uint16_t)br);
        delay(12);
    }
    for (uint8_t i = 0; i < LED_COUNT; i++) ledSetCh(i, ledIdleBase);
}

void ledTriggerFlash(uint8_t slot) {
    if (!ledAvailable || slot >= 38) return;
    int8_t ch = SLOT_TO_LED_CH[slot];
    if (ch < 0) return;
    ledFlashActive[(uint8_t)ch] = true;
    ledFlashEnd[(uint8_t)ch]    = millis() + LED_FLASH_MS;
}

void ledTriggerSweep(bool leftToRight) {
    if (!ledAvailable) return;
    ledSweepActive = true;
    ledSweepDir    = leftToRight ? 1 : -1;
    ledSweepPos    = leftToRight ? 0 : (LED_COUNT - 1);
    ledSweepNext   = millis() + LED_SWEEP_STEP_MS;
}

void ledApplyBrightness(int16_t bright255) {
    // Map BRIGHT (15–255) → PCA9685 idle level (300–3600)
    ledIdleBase = (uint16_t)map(constrain((int)bright255, 15, 255), 15, 255, 300, 3600);
}

void ledUpdate() {
    if (!ledAvailable) return;
    unsigned long now = millis();
    bool shiftActive = isButtonPressed(BUTTON_SHIFT);

    // SHIFT mode: alternating even/odd channel blink
    if (shiftActive) {
        if (now >= ledShiftBlinkNext) {
            ledShiftBlink     = !ledShiftBlink;
            ledShiftBlinkNext = now + LED_SHIFT_BLINK_MS;
        }
        for (uint8_t i = 0; i < LED_COUNT; i++) {
            bool on = ledShiftBlink ? (i % 2 == 0) : (i % 2 == 1);
            ledSetCh(i, on ? LED_MAX : 0);
        }
        return;
    }
    ledShiftBlink = false;

    // Breathing sine value
    float phase    = (float)(now % LED_BREATH_PERIOD) / (float)LED_BREATH_PERIOD;
    float sineVal  = sinf(phase * TWO_PI);
    uint16_t breathBr = (uint16_t)constrain(
        (int32_t)ledIdleBase + (int32_t)(sineVal * LED_BREATH_AMP), 0, LED_MAX);

    // Advance sweep position
    if (ledSweepActive && now >= ledSweepNext) {
        ledSweepPos += ledSweepDir;
        if (ledSweepPos < 0 || ledSweepPos >= LED_COUNT) {
            ledSweepActive = false;
        } else {
            ledSweepNext = now + LED_SWEEP_STEP_MS;
        }
    }

    // Apply brightness per channel
    for (uint8_t ch = 0; ch < LED_COUNT; ch++) {
        if (ledFlashActive[ch] && now >= ledFlashEnd[ch]) {
            ledFlashActive[ch] = false;
        }
        uint16_t brightness;
        if (ledFlashActive[ch]) {
            brightness = LED_MAX;
        } else if (ledSweepActive) {
            bool lit = (ledSweepDir > 0) ? (ch <= (uint8_t)ledSweepPos)
                                         : (ch >= (uint8_t)ledSweepPos);
            brightness = lit ? LED_MAX : breathBr;
        } else {
            brightness = breathBr;
        }
        ledSetCh(ch, brightness);
    }
}

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
const unsigned long MODE_HOLD_MS = 1500;       // Long press for ENC mode toggle
const unsigned long SHORT_PRESS_MS = 500;      // Short press for presets
unsigned long comboHoldStart = 0;
unsigned long mfcPressStart = 0;               // Track MFC press time for short/long detection
bool clutchSwapped = false;         // per-press guard: prevents re-trigger while paddles held
bool clutchChannelsSwapped = false; // persistent: hall A↔B outputs are inverted

// Virtual buttons
struct VirtualButtonPulse {
    uint8_t id;
    bool active;
    unsigned long releaseAt;
};

// Encoders 2..9 in button mode use 16 buttons: 40-55 (NO conflict with matrix 1-27)
VirtualButtonPulse encoderPulses[16] = {
    {40, false, 0}, {41, false, 0},
    {42, false, 0}, {43, false, 0},
    {44, false, 0}, {45, false, 0},
    {46, false, 0}, {47, false, 0},
    {48, false, 0}, {49, false, 0},
    {50, false, 0}, {51, false, 0},
    {52, false, 0}, {53, false, 0},
    {54, false, 0}, {55, false, 0}
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
int8_t fuelValue = 50;         // 0-100 (LEAN <-> RICH)

enum ErsMode : uint8_t {
    ERS_BALANCED = 0,
    ERS_HARVEST,
    ERS_DEPLOY,
    ERS_HOTLAP,
    ERS_COUNT
};

uint8_t ersMode = ERS_BALANCED;
const char* ersModeNames[ERS_COUNT] = {
    "BALANCED",
    "HARVEST",
    "DEPLOY",
    "HOTLAP"
};

// Virtual buttons for MFC menu items (all IDs must be <= 64 to fit HID descriptor)
// IDs 60-64: TC2/TC3/TYRE (CW/CCW pairs)
// IDs 35-39: TYRE DN, VOL_A UP/DN, VOL_B UP/DN (remapped from 65-69 which exceeded 64-btn limit)
VirtualButtonPulse mfcVirtualButtons[10] = {
    {60, false, 0}, {61, false, 0}, // TC2 UP/DN
    {62, false, 0}, {63, false, 0}, // TC3 UP/DN
    {64, false, 0}, {35, false, 0}, // TYRE UP / TYRE DN (35 was 65)
    {36, false, 0}, {37, false, 0}, // VOL_A UP/DN (36-37 were 66-67)
    {38, false, 0}, {39, false, 0}  // VOL_B UP/DN (38-39 were 68-69)
};

// Matrix button slots for multimedia (when VOL_SYS active)
static const uint8_t BUTTON_RADIO = 10;  // Slot 10 = MUTE
static const uint8_t BUTTON_FLASH = 11;  // Slot 11 = PLAY/PAUSE

// ================================
// UTILITIES
// ================================

typedef struct __attribute__((packed)) {
    uint64_t buttons;
    uint8_t hat;        // HAT/POV: 4 bits value + 4 bits padding
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
        hatValue,   // 0 = null/released, 1-8 = directions
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
    Gamepad.sendReport(&report, sizeof(report));
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

void sendConsumerControl(uint16_t code) {
    ConsumerControl.press(code);
    delay(10);
    ConsumerControl.release();
}

void saveConfig() {
    prefs.putUChar("clMode", (uint8_t)clutchCfg.mode);
    prefs.putUShort("h1min", clutchCfg.hallMinA);
    prefs.putUShort("h1max", clutchCfg.hallMaxA);
    prefs.putUShort("h2min", clutchCfg.hallMinB);
    prefs.putUShort("h2max", clutchCfg.hallMaxB);
    prefs.putUChar("bite", clutchCfg.bitePoint);
    prefs.putBool("encBtn", encoderButtonMode);
    prefs.putUChar("ersMode", ersMode);
    prefs.putUChar("fuelVal", (uint8_t)fuelValue);
}

void loadConfig() {
    clutchCfg.mode = (ClutchMode)prefs.getUChar("clMode", CLUTCH_DUAL);
    clutchCfg.hallMinA = prefs.getUShort("h1min", 0);
    clutchCfg.hallMaxA = prefs.getUShort("h1max", 4095);
    clutchCfg.hallMinB = prefs.getUShort("h2min", 0);
    clutchCfg.hallMaxB = prefs.getUShort("h2max", 4095);
    clutchCfg.bitePoint = prefs.getUChar("bite", 60);
    encoderButtonMode = prefs.getBool("encBtn", false);
    ersMode = prefs.getUChar("ersMode", ERS_BALANCED);
    if (ersMode >= ERS_COUNT) {
        ersMode = ERS_BALANCED;
    }
    fuelValue = prefs.getUChar("fuelVal", 50);
    if (fuelValue < 0) fuelValue = 0;
    if (fuelValue > 100) fuelValue = 100;
}

int8_t mapHallToAxis(uint16_t raw, uint16_t minV, uint16_t maxV) {
    if (maxV <= minV) return 0;
    int16_t val = raw - minV;
    if (val < 0) val = 0;
    if (val > (int16_t)(maxV - minV)) val = maxV - minV;
    return (int8_t)map(val, 0, maxV - minV, -127, 127);
}

// Forward declarations
void triggerVirtualButton(uint8_t btnId, uint16_t durationMs = 100);
void uartSend(const char* cat, const char* func, const char* val);

bool shouldReportMatrixButton(uint8_t buttonNum) {
    // Exclude SHIFT (slot 30) and 5-way directions (slots 25-28) from HID buttons
    // Directions are handled by HAT switch instead
    if (isFivewayDirection(buttonNum)) return false;
    return buttonNum <= MATRIX_HID_MAX;
}

// ================================
// MATRIX
// ================================
bool mcpAvailable = false;

void setupButtonMatrix() {
    Wire.begin(I2C_SDA, I2C_SCL);

    if (!mcp.begin_I2C(0x20)) {
        DBG("[ERROR] MCP23017 NOT FOUND at 0x20!");
        DBG("[WARN] Continuing without matrix - USB HID will still work");
        mcpAvailable = false;
        return;  // Don't halt - let USB still enumerate
    }

    mcpAvailable = true;
    DBG("[I2C] MCP23017 found at 0x20");

    for (uint8_t col = 0; col < MATRIX_COLS; col++) {
        mcp.pinMode(col, OUTPUT);      // GPA0..GPA7
        mcp.digitalWrite(col, HIGH);   // colunas inativas em HIGH
    }

    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        mcp.pinMode(8 + row, INPUT_PULLUP); // GPB0..GPB7
    }
}

void scanButtonMatrix() {
    if (!mcpAvailable) return;  // Skip if MCP23017 not found
    unsigned long currentTime = millis();

    for (uint8_t col = 0; col < MATRIX_COLS; col++) {
        mcp.digitalWrite(col, LOW);
        delayMicroseconds(10);

        for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
            bool reading = (mcp.digitalRead(8 + row) == LOW);

            if (reading != prevButtonStates[row][col]) {
                lastDebounceTime[row][col] = currentTime;
            }

            if ((currentTime - lastDebounceTime[row][col]) > DEBOUNCE_DELAY) {
                if (reading != buttonStates[row][col]) {
                    buttonStates[row][col] = reading;

                    uint8_t buttonNum = getButtonNumber(row, col);

                    // Trigger LED flash on press
                    if (reading) {
                        ledTriggerFlash(buttonNum);
                    }

                    // 5-way directions → update HAT switch (not individual buttons)
                    if (isFivewayDirection(buttonNum)) {
                        updateHatFromMatrix();
                        sendGamepad();
                    } else if (buttonNum <= HID_MAX_BUTTONS && shouldReportMatrixButton(buttonNum)) {
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

        mcp.digitalWrite(col, HIGH);
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
    bool shiftPressed = isButtonPressed(BUTTON_SHIFT);

    // SHIFT + MFC rotation: jump 2 items instead of 1
    if (shiftPressed && !mfcAdjustMode) {
        step *= 2;  // Jump 2 items
    }

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
            ledApplyBrightness(brightnessValue);
        } else if (item == MFC_PAGE) {
            pageValue += step;
            if (pageValue < 0) pageValue = 6;
            if (pageValue > 6) pageValue = 0;
            uartSend("PAGE", step > 0 ? "NEXT" : "PREV", "");
        } else if (item == MFC_VOL_SYS) {
            // HID Consumer Control Volume (implemented separately)
            sendConsumerControl(step > 0 ? 0xE9 : 0xEA); // Volume Up/Down
        } else if (item == MFC_VOL_A) {
            // Virtual buttons 66 (UP) / 67 (DN)
            triggerVirtualButton(step > 0 ? 66 : 67);
            sendGamepad();
        } else if (item == MFC_VOL_B) {
            // Virtual buttons 68 (UP) / 69 (DN)
            triggerVirtualButton(step > 0 ? 68 : 69);
            sendGamepad();
        } else if (item == MFC_TC2) {
            // Virtual buttons 60 (UP) / 61 (DN)
            triggerVirtualButton(step > 0 ? 60 : 61);
            sendGamepad();
        } else if (item == MFC_TC3) {
            // Virtual buttons 62 (UP) / 63 (DN)
            triggerVirtualButton(step > 0 ? 62 : 63);
            sendGamepad();
        } else if (item == MFC_TYRE) {
            // Virtual buttons 64 (UP) / 65 (DN)
            triggerVirtualButton(step > 0 ? 64 : 65);
            sendGamepad();
        } else if (item == MFC_ERS) {
            ersMode = (ersMode + 1) % ERS_COUNT;
            saveConfig();
            uartSend("ERS", "MODE", ersModeNames[ersMode]);
        } else if (item == MFC_FUEL) {
            int newFuel = fuelValue + step;
            if (newFuel < 0) newFuel = 0;
            if (newFuel > 100) newFuel = 100;
            fuelValue = newFuel;
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", fuelValue);
            uartSend("FUEL", "VAL", buf);
        }
    } else {
        // Navigation mode - scroll through menu items
        mfcIndex += step;
        if (mfcIndex < 0) mfcIndex = MFC_COUNT - 1;
        if (mfcIndex >= MFC_COUNT) mfcIndex = 0;
        uartSend("MFC", "NAV", mfcMenuNames[mfcIndex]);
        ledTriggerSweep(step > 0);  // CW → L→R sweep, CCW → R→L sweep
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

    // Release encoder button pulses (buttons 40-55)
    for (auto &pulse : encoderPulses) {
        if (pulse.active && now >= pulse.releaseAt) {
            buttons &= ~(1ULL << (pulse.id - 1));
            pulse.active = false;
            changed = true;
        }
    }

    // Release MFC menu virtual buttons (buttons 60-69)
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

    // Apply persistent channel swap (SHIFT+both-paddles combo toggles this)
    int8_t finalA = clutchChannelsSwapped ? outB : outA;
    int8_t finalB = clutchChannelsSwapped ? outA : outB;
    if (finalA != axisZ || finalB != axisRZ) {
        axisZ = finalA;
        axisRZ = finalB;
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

void triggerVirtualButton(uint8_t btnId, uint16_t durationMs) {
    for (uint8_t i = 0; i < 10; i++) {
        if (mfcVirtualButtons[i].id == btnId) {
            mfcVirtualButtons[i].active = true;
            mfcVirtualButtons[i].releaseAt = millis() + durationMs;
            buttons |= (1ULL << (btnId - 1)); // set HID bit (cleared by releaseVirtualButtonPulses)
            break;
        }
    }
}

void handleMfcPress() {
    bool mfcPressed = isButtonPressed(BUTTON_MFC);
    bool shiftPressed = isButtonPressed(BUTTON_SHIFT);

    // SHIFT + MFC: timing-based disambiguation
    if (mfcPressed && shiftPressed) {
        if (mfcPressStart == 0) {
            mfcPressStart = millis();
        } else {
            unsigned long holdTime = millis() - mfcPressStart;

            // Long press (>= 1.5s): Toggle ENC_MODE (any item)
            if (holdTime >= MODE_HOLD_MS) {
                encoderButtonMode = !encoderButtonMode;
                mfcPressStart = UINT32_MAX;  // Prevent repeat

                if (encoderButtonMode) {
                    axisX = axisY = axisRX = axisRY = 0;
                    axisSlider = axisDial = axisVx = axisVy = 0;
                }
                saveConfig();
                uartSend("MODE", "ENC", encoderButtonMode ? "BTN" : "AXIS");
                sendGamepad();
            }
        }
    } else if (!mfcPressed && mfcPressStart > 0 && mfcPressStart != UINT32_MAX) {
        // Released after short hold: check for preset
        unsigned long holdTime = millis() - mfcPressStart;

        if (holdTime < MODE_HOLD_MS && holdTime > 50) {  // Valid short press
            MfcMenuItem item = (MfcMenuItem)mfcIndex;

            // Short press presets (item-specific)
            if (item == MFC_PAGE) {
                pageValue = 0;  // Reset to page 0
                uartSend("PAGE", "RESET", "0");
            } else if (item == MFC_BRIGHT) {
                brightnessValue = 220;  // Reset to default
                uartSend("BRIGHT", "VAL", "220");
            } else if (item == MFC_TC2) {
                triggerVirtualButton(60);  // TC2 UP preset
                sendGamepad();
            } else if (item == MFC_TC3) {
                triggerVirtualButton(62);  // TC3 UP preset
                sendGamepad();
            } else if (item == MFC_TYRE) {
                triggerVirtualButton(64);  // TYRE UP preset
                sendGamepad();
            }
        }

        mfcPressStart = 0;
    } else if (!mfcPressed && !shiftPressed) {
        mfcPressStart = 0;
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
            item == MFC_TC2 || item == MFC_TC3 || item == MFC_TYRE ||
            item == MFC_FUEL) {

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
                } else if (item == MFC_FUEL) {
                    uartSend("MFC", "ADJUST", "FUEL");
                }
            } else {
                // Exiting adjust mode
                if (item == MFC_BITE) {
                    adjustingBite = false;
                    uartSend("MFC", "CONFIRM", "BITE");
                } else if (item == MFC_BRIGHT) {
                    uartSend("MFC", "CONFIRM", "BRIGHT");
                } else if (item == MFC_FUEL) {
                    saveConfig();
                    uartSend("MFC", "CONFIRM", "FUEL");
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
            } else if (item == MFC_ERS) {
                ersMode = (ersMode + 1) % ERS_COUNT;
                saveConfig();
                uartSend("ERS", "MODE", ersModeNames[ersMode]);
            } else if (item == MFC_RESET) {
                clutchCfg.mode = CLUTCH_DUAL;
                clutchCfg.bitePoint = 60;
                clutchCfg.hallMinA = 0;
                clutchCfg.hallMaxA = 4095;
                clutchCfg.hallMinB = 0;
                clutchCfg.hallMaxB = 4095;
                encoderButtonMode = false;
                ersMode = ERS_BALANCED;
                fuelValue = 50;
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
        clutchSwapped = false;
        return;
    }

    bool clutchA = axisZ > 100;
    bool clutchB = axisRZ > 100;

    // Short combo: Both paddles pressed simultaneously (swap clutches)
    if (clutchA && clutchB && !clutchSwapped) {
        // Toggle persistent channel inversion (updateClutches applies it every frame)
        clutchChannelsSwapped = !clutchChannelsSwapped;
        clutchSwapped = true;
        uartSend("CLUTCH", "SWAP", clutchChannelsSwapped ? "ON" : "OFF");
        sendGamepad();
        return;
    }

    // Long combo: Keep pressed for 2s to cycle mode
    if (clutchA && clutchB && clutchSwapped) {
        if (clutchComboStart == 0) {
            clutchComboStart = millis();
        } else if (millis() - clutchComboStart >= 2000) {
            cycleClutchMode();
            clutchComboStart = UINT32_MAX;
            clutchSwapped = false;  // Reset after mode cycle
        }
    } else {
        clutchComboStart = 0;
    }
}

// ================================
// SETUP + LOOP
// ================================
void setup() {
    // FIRST: Initialize debug UART through CH340 (GPIO 43)
    // This is the ONLY way to see debug on Mac with USB_MODE=0
    ButtonBoxSerial.begin(UART_BAUD, SERIAL_8N1, -1, UART_TX_PIN);
    delay(100);
    DBG("========================================");
    DBG(">>> ESP-ButtonBox-WHEEL FIRMWARE <<<");
    DBG(">>> main_wheel.cpp BOOTING <<<");
    DBG("========================================");

    // Set USB device identity BEFORE any begin() calls
    // Custom VID/PID to avoid Windows caching old device names
    DBG("[USB] Setting VID=0x303A PID=0x8172");
    USB.VID(0x303A);   // Espressif VID
    USB.PID(0x8172);   // Custom PID for ButtonBox Wheel (fresh — Windows cache dodge)
    USB.productName("ESP-ButtonBox-WHEEL");
    USB.manufacturerName("SimRacing DIY");
    USB.firmwareVersion(1);
    USB.usbVersion(0x0200);  // USB 2.0
    DBG("[USB] Product: ESP-ButtonBox-WHEEL, Manufacturer: SimRacing DIY");

    // Register HID devices then start USB stack
    DBG("[USB] Gamepad.begin()...");
    Gamepad.begin();
    DBG("[USB] ConsumerControl.begin()...");
    ConsumerControl.begin();
    DBG("[USB] USB.begin()...");
    USB.begin();
    DBG("[USB] USB stack started OK");

    Serial.begin(115200);
    delay(1500);
    DBG("[BOOT] Post-delay, initializing peripherals...");

    prefs.begin("wheel", false);
    loadConfig();
    DBG("[BOOT] Preferences loaded");

    analogReadResolution(12);

    DBG("[I2C] Setting up MCP23017 button matrix...");
    setupButtonMatrix();
    DBG("[I2C] MCP23017 OK");

    DBG("[I2C] Setting up PCA9685 front LEDs...");
    setupLEDs();
    ledBootSweep();

    setupEncoders();
    DBG("[BOOT] Encoders OK");

    uartSend("SYS", "BOOT", DEVICE_NAME);
    DBG("========================================");
    DBG(">>> BOOT COMPLETE - WHEEL RUNNING <<<");
    DBG("========================================");
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
    ledUpdate();

    delay(1);
}
