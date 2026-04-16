#include <Arduino.h>
#include "driver/gpio.h"  // IDF GPIO — needed to force GPIO_FLOATING after analogSetPinAttenuation
#include "USB.h"
#include "USBHID.h"
#include "USBHIDConsumerControl.h"
#include <Preferences.h>
#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include <Adafruit_PWMServoDriver.h>
#include <NeoPixelBusLg.h>

#define DEVICE_NAME "ESP-ButtonBox-WHEEL"

// I2C pins
#define I2C_SDA 8
#define I2C_SCL 9

// MAX7219 7-segment display (software SPI) — Round wheel local display
#define MAX7219_DIN_PIN 12
#define MAX7219_CLK_PIN 13
#define MAX7219_CS_PIN  4

// WS2812B addressable LEDs — direct control in round wheel mode
#define WS2812_DATA_PIN 10
#define WS2812_LED_COUNT 16

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
    // IMPORTANT: macOS/IOKit sorts HID elements by usage VALUE within a page.
    // Usage values: X=0x30, Y=0x31, Z=0x32, Rx=0x33, Ry=0x34, Rz=0x35, Slider=0x36, Dial=0x37, Vx=0x40, Vy=0x41
    // So macOS assigns pygame axis indices in that order regardless of declaration order.
    // The struct fields MUST match this physical byte order so data lines up correctly:
    // axis 0=X(enc2), 1=Y(enc3), 2=Z(HallA), 3=Rx(enc4), 4=Ry(enc5), 5=Rz(HallB), 6=Slider(enc6), 7=Dial(enc7), 8=Vx(enc8), 9=Vy(enc9)
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x15, 0x81,       // Logical Minimum (-127)
    0x25, 0x7F,       // Logical Maximum (127)
    0x75, 0x08,       // Report Size (8)
    0x95, 0x0A,       // Report Count (10)
    0x09, 0x30,       // Usage (X)      → axis 0 (ENC2 BB)
    0x09, 0x31,       // Usage (Y)      → axis 1 (ENC3 MAP)
    0x09, 0x32,       // Usage (Z)      → axis 2 (Hall A / GPIO1)
    0x09, 0x33,       // Usage (Rx)     → axis 3 (ENC4 TC)
    0x09, 0x34,       // Usage (Ry)     → axis 4 (ENC5 ABS)
    0x09, 0x35,       // Usage (Rz)     → axis 5 (Hall B / GPIO2)
    0x09, 0x36,       // Usage (Slider) → axis 6 (ENC6 Lat.1)
    0x09, 0x37,       // Usage (Dial)   → axis 7 (ENC7 Lat.2)
    0x09, 0x40,       // Usage (Vx)     → axis 8 (ENC8 Lat.3)
    0x09, 0x41,       // Usage (Vy)     → axis 9 (ENC9 Lat.4)
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
static const bool REPORT_SHIFT_IN_HID = true; // debug: expose slot 30 in HID monitor

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
static const uint8_t HALL_A_PIN = 1;  // GPIO1 (ADC1_CH0) — Hall A clutch
static const uint8_t HALL_B_PIN = 2;  // GPIO2 (ADC1_CH1) — Hall B clutch

// UART to WT32 (also used for debug output via CH340 on Mac)
HardwareSerial ButtonBoxSerial(0);
static const uint32_t UART_BAUD = 115200;
static const int UART_RX_PIN = 11;
static const int UART_TX_PIN = 43;

// Debug UART (goes through CH340 to Mac for monitoring)
// With USB_MODE=0, Serial = USB CDC (goes to native USB/Windows), NOT CH340
// So we use ButtonBoxSerial for debug output
#define DBG(msg) ButtonBoxSerial.println(msg)
#define DBGF(...) { char _dbuf[128]; snprintf(_dbuf, sizeof(_dbuf), __VA_ARGS__); ButtonBoxSerial.println(_dbuf); }

void scanI2CBusDebug();
void uartRoundtripTask();
void handleWt32UartRx();

// ================================
// ROUND WHEEL MODE
// ================================
// Auto-detected: when PCA9685 is absent (!ledAvailable), the firmware
// activates local display (MAX7219 + WS2812) and SimHub CDC protocol.
// In F1 mode (PCA9685 present), these features are inactive.
bool roundWheelMode = false;

// SimHub CDC protocol support (ArqSerial reads from Serial = USB CDC)
Stream* DebugPort = nullptr;  // Required by ArqSerial.h (unused, debug goes to ButtonBoxSerial)
#include "FlowSerialRead.h"

// SimHub handshake constants
#define SIMHUB_VERSION 'j'
#define SH_SIGNATURE_0 0x1E
#define SH_SIGNATURE_1 0x98
#define SH_SIGNATURE_2 0x01
#define SH_MESSAGE_HEADER 0x03

// WS2812 LED strip (round wheel mode)
NeoPixelBusLg<NeoGrbFeature, NeoEsp32BitBangWs2812xMethod, NeoGammaTableMethod> ws2812Strip(WS2812_LED_COUNT, WS2812_DATA_PIN);
bool ws2812Active = false;
bool simhubLedControl = false;  // true when SimHub sends direct RGB LED data (command '6')
bool simhubConnected = false;
unsigned long lastSimhubActivity = 0;
static const unsigned long SIMHUB_TIMEOUT_MS = 5000;

// Telemetry data struct (populated from SimHub custom protocol)
struct WheelTelemetry {
    int speed;
    char gear;
    int rpmPercent;
    int rpmRedLine;
    int currentRpm;
    String currentLapTime;
    String lastLapTime;
    String bestLapTime;
    String delta;
    String flag;
    String alertMessage;
    String spotterLeft;
    String spotterRight;
    String shiftLight;
    String drsAvailable;
    String drsActive;
    String tcActive;
    String absActive;
    String position;
    String gapAhead;
    String gapBehind;
    String fuelLaps;
    bool hasData;
};
WheelTelemetry telemetry = {0, 'N', 0, 95, 0, "", "", "", "", "None", "", "0", "0", "0", "0", "0", "0", "0", "0", "--", "--", "0.0", false};

// Forward declaration: pageValue is defined in MFC menu section (~line 1175)
// Used by MAX7219 display update to select which telemetry page to show
extern int8_t pageValue;

// ================================
// MATRIX STATE
// ================================
bool buttonStates[MATRIX_ROWS][MATRIX_COLS] = {false};
bool prevButtonStates[MATRIX_ROWS][MATRIX_COLS] = {false};
unsigned long lastDebounceTime[MATRIX_ROWS][MATRIX_COLS] = {0};
unsigned long lastStableToggleTime[MATRIX_ROWS][MATRIX_COLS] = {0};
const unsigned long DEBOUNCE_DELAY = 30;
const unsigned long MIN_TOGGLE_INTERVAL_MS = 120;

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
static const bool MFC_DEBUG_LOG = true;
static const bool ENCODER_DIR_DEBUG_LOG = true;

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
        scanI2CBusDebug();
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

// Throttle: LED update only every LED_UPDATE_INTERVAL_MS (~60fps)
static const unsigned long LED_UPDATE_INTERVAL_MS = 16;
static unsigned long lastLedUpdateMs = 0;

void ledUpdate() {
    if (!ledAvailable) return;
    unsigned long now = millis();
    if ((now - lastLedUpdateMs) < LED_UPDATE_INTERVAL_MS) return;
    lastLedUpdateMs = now;
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
// MAX7219 7-SEGMENT DISPLAY (Round Wheel)
// ================================
// MAX7219 registers
#define MAX7219_NOOP        0x00
#define MAX7219_DIGIT0      0x01
#define MAX7219_DECODEMODE  0x09
#define MAX7219_INTENSITY   0x0A
#define MAX7219_SCANLIMIT   0x0B
#define MAX7219_SHUTDOWN    0x0C
#define MAX7219_DISPLAYTEST 0x0F

// 7-segment raw patterns for characters (dp-a-b-c-d-e-f-g)
// Used in raw mode (decode=0) for letters not in BCD
static const uint8_t SEG_DASH = 0x01;       // '-'
static const uint8_t SEG_BLANK = 0x00;      // blank
// BCD characters: 0-9 = 0x00-0x09, '-' = 0x0A, 'E' = 0x0B, 'H' = 0x0C, 'L' = 0x0D, 'P' = 0x0E, blank = 0x0F

bool max7219Available = false;

void max7219Send(uint8_t reg, uint8_t data) {
    digitalWrite(MAX7219_CS_PIN, LOW);
    // Send register address (MSB first)
    for (int8_t i = 7; i >= 0; i--) {
        digitalWrite(MAX7219_CLK_PIN, LOW);
        digitalWrite(MAX7219_DIN_PIN, (reg >> i) & 1);
        digitalWrite(MAX7219_CLK_PIN, HIGH);
    }
    // Send data (MSB first)
    for (int8_t i = 7; i >= 0; i--) {
        digitalWrite(MAX7219_CLK_PIN, LOW);
        digitalWrite(MAX7219_DIN_PIN, (data >> i) & 1);
        digitalWrite(MAX7219_CLK_PIN, HIGH);
    }
    digitalWrite(MAX7219_CS_PIN, HIGH);
}

void setupMax7219() {
    pinMode(MAX7219_DIN_PIN, OUTPUT);
    pinMode(MAX7219_CLK_PIN, OUTPUT);
    pinMode(MAX7219_CS_PIN, OUTPUT);
    digitalWrite(MAX7219_CS_PIN, HIGH);

    max7219Send(MAX7219_DISPLAYTEST, 0x00);  // Normal operation
    max7219Send(MAX7219_SCANLIMIT, 0x07);    // Display all 8 digits
    max7219Send(MAX7219_DECODEMODE, 0xFF);   // BCD decode all digits
    max7219Send(MAX7219_INTENSITY, 0x08);    // Medium brightness
    max7219Send(MAX7219_SHUTDOWN, 0x01);     // Normal operation (not shutdown)

    // Clear all digits
    for (uint8_t d = 1; d <= 8; d++) {
        max7219Send(d, 0x0F);  // 0x0F = blank in BCD mode
    }
    max7219Available = true;
    DBG("[MAX7219] Initialized on GPIO DIN=12 CLK=13 CS=4");
}

void max7219SetIntensity(uint8_t level) {
    if (!max7219Available) return;
    max7219Send(MAX7219_INTENSITY, level & 0x0F);  // 0-15
}

// Display: [speed 3 digits] [blank] [gear 1 digit] [blank] [blank] [blank]
// Digits are numbered 1-8 from left to right on display
// MAX7219 digit 1 = leftmost, digit 8 = rightmost
void max7219ShowSpeedGear(int speed, char gear) {
    if (!max7219Available) return;
    // Speed on digits 1-3 (hundreds, tens, units)
    int s = constrain(speed, 0, 999);
    uint8_t d1 = (s >= 100) ? (s / 100) : 0x0F;    // hundreds or blank
    uint8_t d2 = (s >= 10)  ? ((s / 10) % 10) : 0x0F; // tens or blank
    uint8_t d3 = s % 10;                               // units always show
    max7219Send(1, d1);
    max7219Send(2, d2);
    max7219Send(3, d3);

    // Blank separator on digit 4
    max7219Send(4, 0x0F);

    // Gear on digit 5
    if (gear == 'N' || gear == 'n') max7219Send(5, 0x0F);  // blank for neutral
    else if (gear == 'R' || gear == 'r') max7219Send(5, 0x0A);  // '-' for reverse
    else if (gear >= '0' && gear <= '9') max7219Send(5, gear - '0');
    else max7219Send(5, 0x0A);  // '-' for unknown

    // RPM bar indicator on digits 6-8 (simple: show RPM/1000 as number)
    max7219Send(6, 0x0F);
    max7219Send(7, 0x0F);
    max7219Send(8, 0x0F);
}

// Display: lap time "mm:ss.fff" on 8 digits → "mm.ss.fff" (dots as decimal points)
void max7219ShowLapTime(const String &lapTime) {
    if (!max7219Available) return;
    // Parse mm:ss.fff → extract digits
    // Format: "00:00.000" or similar
    String t = lapTime;
    t.replace(":", "");
    t.replace(".", "");
    // Now t should be "0000000" (7 digits for mm ss fff)
    // Display on 8 digits: [m1][m2.][s1][s2.][f1][f2][f3][blank]
    for (uint8_t i = 0; i < 8; i++) {
        if (i < (uint8_t)t.length()) {
            uint8_t digit = t.charAt(i) - '0';
            if (digit > 9) digit = 0x0F;
            // Add decimal point after digit 2 (mm.) and digit 4 (ss.)
            if (i == 1 || i == 3) digit |= 0x80;  // Set DP bit
            max7219Send(i + 1, digit);
        } else {
            max7219Send(i + 1, 0x0F);
        }
    }
}

// Display: Position P## + Gap ahead
void max7219ShowPosition(const String &pos, const String &gap) {
    if (!max7219Available) return;
    // Digit 1: 'P' (0x0E in BCD)
    max7219Send(1, 0x0E);
    // Digits 2-3: position number
    int p = pos.toInt();
    max7219Send(2, (p >= 10) ? (p / 10) : 0x0F);
    max7219Send(3, p % 10);
    // Digit 4: blank separator
    max7219Send(4, 0x0F);
    // Digits 5-8: gap (e.g. "1.2" → " 1.2")
    String g = gap;
    g.replace("-", "");
    float gapVal = g.toFloat();
    int gapInt = (int)(gapVal * 10);  // e.g. 1.2 → 12
    if (gapInt > 9999) gapInt = 9999;
    uint8_t g1 = (gapInt >= 1000) ? (gapInt / 1000) : 0x0F;
    uint8_t g2 = (gapInt >= 100) ? ((gapInt / 100) % 10) : 0x0F;
    uint8_t g3 = (gapInt >= 10) ? ((gapInt / 10) % 10) : 0x0F;
    uint8_t g4 = gapInt % 10;
    // Decimal point on g3 (one decimal place)
    g3 |= 0x80;
    max7219Send(5, g1);
    max7219Send(6, g2);
    max7219Send(7, g3);
    max7219Send(8, g4);
}

void max7219Clear() {
    if (!max7219Available) return;
    for (uint8_t d = 1; d <= 8; d++) max7219Send(d, 0x0F);
}

// MAX7219 display update (throttled)
static const unsigned long MAX7219_UPDATE_MS = 50;  // 20Hz refresh
static unsigned long lastMax7219Update = 0;

void updateMax7219Display() {
    if (!max7219Available || !telemetry.hasData) return;
    unsigned long now = millis();
    if ((now - lastMax7219Update) < MAX7219_UPDATE_MS) return;
    lastMax7219Update = now;

    // pageValue is controlled by MFC menu (MFC_PAGE)
    switch (pageValue) {
        case 0: // Speed + Gear (default)
            max7219ShowSpeedGear(telemetry.speed, telemetry.gear);
            break;
        case 1: // Current lap time
            max7219ShowLapTime(telemetry.currentLapTime);
            break;
        case 2: // Best lap time
            max7219ShowLapTime(telemetry.bestLapTime);
            break;
        case 3: // Position + Gap ahead
            max7219ShowPosition(telemetry.position, telemetry.gapAhead);
            break;
        case 4: // RPM + Gear
            max7219ShowSpeedGear(telemetry.currentRpm / 100, telemetry.gear);  // RPM in hundreds
            break;
        default:
            max7219ShowSpeedGear(telemetry.speed, telemetry.gear);
            break;
    }
}

// ================================
// WS2812 LED STRIP (Round Wheel)
// ================================
// Color definitions
#define WS_COLOR_OFF     RgbColor(0, 0, 0)
#define WS_COLOR_GREEN   RgbColor(0, 255, 0)
#define WS_COLOR_YELLOW  RgbColor(255, 255, 0)
#define WS_COLOR_ORANGE  RgbColor(255, 50, 0)
#define WS_COLOR_RED     RgbColor(255, 0, 0)
#define WS_COLOR_BLUE    RgbColor(0, 0, 255)
#define WS_COLOR_PURPLE  RgbColor(128, 0, 255)
#define WS_COLOR_WHITE   RgbColor(255, 255, 255)
#define WS_COLOR_CYAN    RgbColor(0, 200, 255)

static const unsigned long WS2812_UPDATE_MS = 20;  // 50Hz refresh
static unsigned long lastWs2812Update = 0;

void setupWs2812() {
    ws2812Strip.Begin();
    ws2812Strip.SetLuminance(60);  // Conservative default brightness
    for (int i = 0; i < WS2812_LED_COUNT; i++) {
        ws2812Strip.SetPixelColor(i, WS_COLOR_OFF);
    }
    ws2812Strip.Show();
    ws2812Active = true;
    DBG("[WS2812] Initialized on GPIO10, count=" + String(WS2812_LED_COUNT));
}

// Animate loading pattern when SimHub not connected
void ws2812LoadingAnimation() {
    static int pos = 0;
    for (int i = 0; i < WS2812_LED_COUNT; i++) {
        ws2812Strip.SetPixelColor(i, WS_COLOR_OFF);
    }
    // Bouncing LED
    int actual = pos % (WS2812_LED_COUNT * 2 - 2);
    if (actual >= WS2812_LED_COUNT) actual = (WS2812_LED_COUNT * 2 - 2) - actual;
    ws2812Strip.SetPixelColor(actual, WS_COLOR_BLUE);
    if (actual > 0) ws2812Strip.SetPixelColor(actual - 1, RgbColor(0, 0, 60));
    if (actual < WS2812_LED_COUNT - 1) ws2812Strip.SetPixelColor(actual + 1, RgbColor(0, 0, 60));
    ws2812Strip.Show();
    pos++;
}

// Update LEDs from telemetry data (when SimHub is NOT controlling LEDs directly)
void ws2812UpdateFromTelemetry() {
    if (!telemetry.hasData) return;

    // Clear all
    for (int i = 0; i < WS2812_LED_COUNT; i++) {
        ws2812Strip.SetPixelColor(i, WS_COLOR_OFF);
    }

    // RPM bar: fill LEDs proportionally to RPM percentage
    int numLit = (telemetry.rpmPercent * WS2812_LED_COUNT) / 100;
    if (numLit > WS2812_LED_COUNT) numLit = WS2812_LED_COUNT;

    bool shiftFlash = (telemetry.shiftLight == "1") && ((millis() / 80) % 2 == 0);

    for (int i = 0; i < numLit; i++) {
        float pct = ((float)(i + 1) / WS2812_LED_COUNT) * 100.0f;
        RgbColor color;
        if (pct < 60) color = WS_COLOR_GREEN;
        else if (pct < 80) color = WS_COLOR_YELLOW;
        else if (pct < (float)telemetry.rpmRedLine) color = WS_COLOR_ORANGE;
        else color = shiftFlash ? WS_COLOR_OFF : WS_COLOR_RED;
        ws2812Strip.SetPixelColor(i, color);
    }

    // DRS override: full strip in blue/cyan
    if (telemetry.drsActive == "1") {
        for (int i = 0; i < numLit; i++) ws2812Strip.SetPixelColor(i, WS_COLOR_CYAN);
    } else if (telemetry.drsAvailable == "1") {
        for (int i = 0; i < numLit; i++) ws2812Strip.SetPixelColor(i, WS_COLOR_GREEN);
    }

    // Flag override on edges (first 2 + last 2 LEDs)
    RgbColor flagColor = WS_COLOR_OFF;
    bool flagBlink = false;
    if (telemetry.flag == "Yellow") { flagColor = WS_COLOR_YELLOW; flagBlink = true; }
    else if (telemetry.flag == "Blue") { flagColor = WS_COLOR_BLUE; flagBlink = true; }
    else if (telemetry.flag == "Red") { flagColor = WS_COLOR_RED; }
    else if (telemetry.flag == "Green") { flagColor = WS_COLOR_GREEN; }
    else if (telemetry.flag == "White") { flagColor = WS_COLOR_WHITE; }
    else if (telemetry.flag == "Checkered") { flagColor = WS_COLOR_WHITE; flagBlink = true; }

    if (flagColor.R > 0 || flagColor.G > 0 || flagColor.B > 0) {
        bool show = !flagBlink || ((millis() / 400) % 2 == 0);
        if (show) {
            ws2812Strip.SetPixelColor(0, flagColor);
            ws2812Strip.SetPixelColor(1, flagColor);
            ws2812Strip.SetPixelColor(WS2812_LED_COUNT - 2, flagColor);
            ws2812Strip.SetPixelColor(WS2812_LED_COUNT - 1, flagColor);
        }
    }

    // Spotter indicators (overrides edge LEDs)
    if (telemetry.spotterLeft == "1") {
        ws2812Strip.SetPixelColor(0, WS_COLOR_PURPLE);
        ws2812Strip.SetPixelColor(1, WS_COLOR_PURPLE);
    }
    if (telemetry.spotterRight == "1") {
        ws2812Strip.SetPixelColor(WS2812_LED_COUNT - 2, WS_COLOR_PURPLE);
        ws2812Strip.SetPixelColor(WS2812_LED_COUNT - 1, WS_COLOR_PURPLE);
    }

    ws2812Strip.Show();
}

void updateWs2812LEDs() {
    if (!ws2812Active) return;
    unsigned long now = millis();
    if ((now - lastWs2812Update) < WS2812_UPDATE_MS) return;
    lastWs2812Update = now;

    // If SimHub sends direct LED data (command '6'), skip local control
    if (simhubLedControl) return;

    if (simhubConnected && telemetry.hasData) {
        ws2812UpdateFromTelemetry();
    } else {
        ws2812LoadingAnimation();
    }
}

// ================================
// SIMHUB CDC PROTOCOL HANDLER (Round Wheel)
// ================================

// Read WS2812 LED data sent by SimHub (command '6')
void simhubReadLEDs() {
    int mode = FlowSerialTimedRead();
    while (mode > 0) {
        if (mode == 1) {
            // Full LED data
            for (int j = 0; j < WS2812_LED_COUNT; j++) {
                uint8_t r = FlowSerialTimedRead();
                uint8_t g = FlowSerialTimedRead();
                uint8_t b = FlowSerialTimedRead();
                ws2812Strip.SetPixelColor(j, RgbColor(r, g, b));
            }
        } else if (mode == 2) {
            // Partial LED data
            int startLed = FlowSerialTimedRead();
            int numLeds = FlowSerialTimedRead();
            for (int j = startLed; j < startLed + numLeds && j < WS2812_LED_COUNT; j++) {
                uint8_t r = FlowSerialTimedRead();
                uint8_t g = FlowSerialTimedRead();
                uint8_t b = FlowSerialTimedRead();
                ws2812Strip.SetPixelColor(j, RgbColor(r, g, b));
            }
        } else if (mode == 3) {
            // Repeated color
            int startLed = FlowSerialTimedRead();
            int numLeds = FlowSerialTimedRead();
            uint8_t r = FlowSerialTimedRead();
            uint8_t g = FlowSerialTimedRead();
            uint8_t b = FlowSerialTimedRead();
            for (int j = startLed; j < startLed + numLeds && j < WS2812_LED_COUNT; j++) {
                ws2812Strip.SetPixelColor(j, RgbColor(r, g, b));
            }
        }
        mode = FlowSerialTimedRead();
    }
    ws2812Strip.Show();
    simhubLedControl = true;  // SimHub is controlling LEDs directly
}

// Read custom protocol data from SimHub (command 'P')
// Parses all 72 fields (same order as customProtocol-dashBoard.txt)
void simhubReadCustomProtocol() {
    // BLOCO 1: Telemetria Básica (0-4)
    telemetry.speed = FlowSerialReadStringUntil(';').toInt();
    String gearStr = FlowSerialReadStringUntil(';');
    telemetry.gear = gearStr.length() > 0 ? gearStr.charAt(0) : 'N';
    telemetry.rpmPercent = FlowSerialReadStringUntil(';').toInt();
    telemetry.rpmRedLine = FlowSerialReadStringUntil(';').toInt();
    telemetry.currentRpm = FlowSerialReadStringUntil(';').toInt();

    // BLOCO 2: Cronometragem (5-10)
    telemetry.currentLapTime = FlowSerialReadStringUntil(';');
    telemetry.lastLapTime = FlowSerialReadStringUntil(';');
    telemetry.bestLapTime = FlowSerialReadStringUntil(';');
    telemetry.delta = FlowSerialReadStringUntil(';');
    FlowSerialReadStringUntil(';');  // [9] deltaProgress (skip)
    FlowSerialReadStringUntil(';');  // [10] lapInvalidated (skip)

    // BLOCO 3: Física e Pneus (11-24) — skip all 14 fields
    for (int i = 0; i < 14; i++) FlowSerialReadStringUntil(';');

    // BLOCO 4: Eletrônica (25-31)
    FlowSerialReadStringUntil(';');  // [25] tcLevel
    telemetry.tcActive = FlowSerialReadStringUntil(';');  // [26]
    FlowSerialReadStringUntil(';');  // [27] absLevel
    telemetry.absActive = FlowSerialReadStringUntil(';');  // [28]
    for (int i = 0; i < 3; i++) FlowSerialReadStringUntil(';');  // [29-31]

    // BLOCO 5: Estratégia (32-41)
    telemetry.position = FlowSerialReadStringUntil(';');    // [32]
    FlowSerialReadStringUntil(';');  // [33] opponentsCount
    telemetry.gapAhead = FlowSerialReadStringUntil(';');    // [34]
    telemetry.gapBehind = FlowSerialReadStringUntil(';');   // [35]
    telemetry.fuelLaps = FlowSerialReadStringUntil(';');    // [36]
    FlowSerialReadStringUntil(';');  // [37] fuelPerLap
    FlowSerialReadStringUntil(';');  // [38] sessionTimeLeft
    telemetry.flag = FlowSerialReadStringUntil(';');        // [39]
    telemetry.flag.trim();
    FlowSerialReadStringUntil(';');  // [40] penalties
    FlowSerialReadStringUntil(';');  // [41] cutTrackWarnings

    // BLOCO 6: Alertas (42-43)
    telemetry.alertMessage = FlowSerialReadStringUntil(';');  // [42]
    telemetry.alertMessage.trim();
    FlowSerialReadStringUntil(';');  // [43] popupMessage

    // BLOCO 7: Arduino LEDs (44-47)
    FlowSerialReadStringUntil(';');  // [44] rpmPercent2
    telemetry.spotterLeft = FlowSerialReadStringUntil(';');   // [45]
    telemetry.spotterRight = FlowSerialReadStringUntil(';');  // [46]
    FlowSerialReadStringUntil(';');  // [47] absActive2

    // BLOCO 8: Desgaste e Ambiente (48-61) — read shiftLight, DRS, skip rest
    for (int i = 0; i < 9; i++) FlowSerialReadStringUntil(';');  // [48-56]
    telemetry.shiftLight = FlowSerialReadStringUntil(';');     // [57]
    telemetry.drsAvailable = FlowSerialReadStringUntil(';');   // [58]
    telemetry.drsActive = FlowSerialReadStringUntil(';');      // [59]
    FlowSerialReadStringUntil(';');  // [60] kersLevel
    FlowSerialReadStringUntil(';');  // [61] turboBoost

    // BLOCO 9: 499P (62-67) — skip all
    for (int i = 0; i < 6; i++) FlowSerialReadStringUntil(';');

    // BLOCO 10: Track Map (68-71) — skip all
    for (int i = 0; i < 4; i++) FlowSerialReadStringUntil(';');

    telemetry.hasData = true;
}

// Generate unique ID from ESP32 eFuse MAC (avoids WiFi dependency)
String getWheelUniqueId() {
    uint64_t mac = ESP.getEfuseMac();
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
        (uint8_t)(mac), (uint8_t)(mac >> 8), (uint8_t)(mac >> 16),
        (uint8_t)(mac >> 24), (uint8_t)(mac >> 32), (uint8_t)(mac >> 40));
    return String(macStr);
}

// Process SimHub commands from USB CDC (Serial)
void processSimHubCDC() {
    if (FlowSerialAvailable() > 0) {
        int r = FlowSerialTimedRead();
        if (r == SH_MESSAGE_HEADER) {
            lastSimhubActivity = millis();
            int cmd = FlowSerialTimedRead();

            // Protection: skip if command is ARQ re-read
            if (cmd == 0x01) return;

            switch (cmd) {
                case '1': {  // Hello
                    FlowSerialTimedRead();  // Read trailer
                    delay(10);
                    char v = SIMHUB_VERSION;
                    FlowSerialPrint(v);
                    FlowSerialFlush();
                    DBG("[SIMHUB] Hello handshake OK");
                    break;
                }
                case '0': {  // Features
                    delay(10);
                    FlowSerialPrint("NIP\n");
                    FlowSerialFlush();
                    break;
                }
                case 'N': {  // Device Name
                    FlowSerialPrint(DEVICE_NAME);
                    FlowSerialPrint("\n");
                    FlowSerialFlush();
                    FlowSerialWrite(0x15);
                    break;
                }
                case 'I': {  // Unique ID
                    String uid = getWheelUniqueId();
                    FlowSerialPrint(uid);
                    FlowSerialPrint("\n");
                    FlowSerialFlush();
                    FlowSerialWrite(0x15);
                    break;
                }
                case '4': {  // RGB LED Count
                    FlowSerialWrite((byte)(WS2812_LED_COUNT));
                    FlowSerialFlush();
                    break;
                }
                case '6': {  // RGB LED Data
                    simhubReadLEDs();
                    FlowSerialWrite(0x15);
                    break;
                }
                case 'P': {  // Custom Protocol Data
                    simhubReadCustomProtocol();
                    FlowSerialWrite(0x15);
                    break;
                }
                case 'A': {  // Acq
                    FlowSerialWrite(0x03);
                    FlowSerialFlush();
                    simhubConnected = true;
                    break;
                }
                case 'J': {  // Buttons Count
                    FlowSerialWrite((byte)0);
                    FlowSerialFlush();
                    break;
                }
                case '2': {  // TM1638 Count
                    FlowSerialWrite((byte)0);
                    FlowSerialFlush();
                    break;
                }
                case 'B': {  // Simple Modules Count
                    FlowSerialWrite((byte)0);
                    FlowSerialFlush();
                    break;
                }
                case 'G': {  // Gear Data
                    FlowSerialTimedRead();  // Read gear char
                    FlowSerialWrite(0x15);
                    break;
                }
                case '8': {  // Set Baudrate
                    SetBaudrate();
                    break;
                }
                case 'X': {  // Expanded Commands
                    String xaction = FlowSerialReadStringUntil(' ', '\n');
                    if (xaction == "list") {
                        FlowSerialPrintLn("mcutype");
                        FlowSerialPrintLn("keepalive");
                        FlowSerialPrintLn();
                        FlowSerialFlush();
                    } else if (xaction == "mcutype") {
                        FlowSerialPrint((char)SH_SIGNATURE_0);
                        FlowSerialPrint((char)SH_SIGNATURE_1);
                        FlowSerialPrint((char)SH_SIGNATURE_2);
                        FlowSerialFlush();
                    }
                    FlowSerialWrite(0x15);
                    break;
                }
                default:
                    break;
            }
        }
    }

    // Detect SimHub disconnect (no activity for SIMHUB_TIMEOUT_MS)
    if (simhubConnected && (millis() - lastSimhubActivity) > SIMHUB_TIMEOUT_MS) {
        simhubConnected = false;
        simhubLedControl = false;
        telemetry.hasData = false;
        DBG("[SIMHUB] Connection timeout — switched to idle mode");
    }
}

// ================================
// ENCODERS
// ================================
struct EncoderState {
    int8_t  lastEncoded;
    int32_t encoderValue;
    bool    lastA;
    bool    lastB;
    unsigned long lastChangeTime;
    int8_t  accumulator;
};

EncoderState encoderStates[NUM_ENCODERS];

// Single debounce for contact bounce rejection (µs).
// The 4-step accumulator already rejects directional noise.
static const unsigned long ENCODER_DEBOUNCE_US = 100;

// EC11 encoders produce 4 gray-code transitions per detent click.
// Threshold = 4 → exactly 1 step per physical click. Clean, no half-steps.
static const int8_t ENCODER_DETENT_TRANSITIONS = 4;

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

// SHIFT + ENC2-5 alternate functions. These HID bits are never set by matrix scan:
// 25-28: isFivewayDirection() intercepts them before bits are set (HAT path)
// 56-59: above MATRIX_HID_MAX=37, shouldReportMatrixButton() returns false
VirtualButtonPulse encoderShiftPulses[8] = {
    {25, false, 0}, {26, false, 0}, // SHIFT+ENC2 (BB)  CW/CCW → buttons 25/26
    {27, false, 0}, {28, false, 0}, // SHIFT+ENC3 (MAP) CW/CCW → buttons 27/28
    {56, false, 0}, {57, false, 0}, // SHIFT+ENC4 (TC)  CW/CCW → buttons 56/57
    {58, false, 0}, {59, false, 0}  // SHIFT+ENC5 (ABS) CW/CCW → buttons 58/59
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

// Hall/clutch anti-noise (helps when halls are not connected yet)
static const uint8_t CLUTCH_RAW_FILTER_SHIFT = 4;  // IIR: 1/16 — light smoothing after median; keeps response fast
static const int8_t  CLUTCH_AXIS_DEADZONE = 10;    // minimum HID delta to report (suppresses ~±30 count idle jitter)
static const uint8_t HALL_MEDIAN_SAMPLES = 7;       // odd number — median rejects random spikes regardless of magnitude
bool     clutchFilterInit = false;
int32_t  clutchRawAFiltered = 0;
int32_t  clutchRawBFiltered = 0;
static const bool HALL_RAW_DEBUG = true;
static const unsigned long HALL_RAW_DEBUG_MS = 100;
unsigned long lastHallRawDebugMs = 0;

// Hall sensor presence detection (tested at boot via ADC variance)
bool hallAConnected = false;
bool hallBConnected = false;

// ================================
// MFC MENU (Adjustable Mode)
// ================================
enum MfcMenuItem : uint8_t {
    MFC_CLUTCH = 0,
    MFC_CALIB,
    MFC_ENC_MODE,
    MFC_RESET,
    MFC_BITE,
    MFC_BRIGHT,
    MFC_PAGE,
    MFC_VOL_SYS,
    MFC_VOL_A,
    MFC_VOL_B,
    MFC_TC2,
    MFC_FFB,
    MFC_TYRE,
    MFC_ERS,
    MFC_FUEL,
    MFC_COUNT
};

const char* mfcMenuNames[MFC_COUNT] = {
    "CLUTCH",
    "CALIB",
    "ENC MODE",
    "RESET",
    "BITE",
    "BRIGHT",
    "PAGE",
    "VOL_SYS",
    "VOL_A",
    "VOL_B",
    "TC2",
    "FFB",
    "TYRE",
    "ERS",
    "FUEL"
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
// IDs 60-63: TC2 UP/DN (60/61), FFB UP/DN (62/63); 62/63 also used by SHIFT+TC2 = TC3 function
// IDs 35-39, 64: TYRE UP/DN, VOL_A UP/DN, VOL_B UP/DN (remapped from 65-69 which exceeded limit)
VirtualButtonPulse mfcVirtualButtons[10] = {
    {60, false, 0}, {61, false, 0}, // TC2 UP/DN
    {62, false, 0}, {63, false, 0}, // FFB UP/DN (shared: SHIFT+TC2 also fires 62/63)
    {64, false, 0}, {35, false, 0}, // TYRE UP / TYRE DN (35 was 65)
    {36, false, 0}, {37, false, 0}, // VOL_A UP/DN (36-37 were 66-67)
    {38, false, 0}, {39, false, 0}  // VOL_B UP/DN (38-39 were 68-69)
};

// Matrix button slots for multimedia (when VOL_SYS active)
static const uint8_t BUTTON_RADIO = 13;  // Slot 13 = MUTE   (GPB1/GPA4)
static const uint8_t BUTTON_FLASH = 14;  // Slot 14 = PLAY/PAUSE (GPB1/GPA5)

// ================================
// UTILITIES
// ================================

typedef struct __attribute__((packed)) {
    uint64_t buttons;
    uint8_t hat;        // HAT/POV: 4 bits value + 4 bits padding
    // Axis byte order MUST match HID descriptor declaration order:
    // X(0x30), Y(0x31), Z(0x32), Rx(0x33), Ry(0x34), Rz(0x35), Slider(0x36), Dial(0x37), Vx(0x40), Vy(0x41)
    // macOS sorts pygame axis indices by usage value, so this order matches pygame axis 0-9.
    int8_t x;       // axis 0 – ENC2 (BB)
    int8_t y;       // axis 1 – ENC3 (MAP)
    int8_t z;       // axis 2 – Hall A (GPIO1, Clutch L)
    int8_t rx;      // axis 3 – ENC4 (TC)
    int8_t ry;      // axis 4 – ENC5 (ABS)
    int8_t rz;      // axis 5 – Hall B (GPIO2, Clutch R)
    int8_t slider;  // axis 6 – ENC6 (Lat.1)
    int8_t dial;    // axis 7 – ENC7 (Lat.2)
    int8_t vx;      // axis 8 – ENC8 (Lat.3)
    int8_t vy;      // axis 9 – ENC9 (Lat.4)
} GamepadReport;

void sendGamepad() {
    GamepadReport report = {
        buttons,
        hatValue,   // 0 = null/released, 1-8 = directions
        axisX,      // X  → axis 0
        axisY,      // Y  → axis 1
        axisZ,      // Z  → axis 2 (Hall A)
        axisRX,     // Rx → axis 3 (ENC4)
        axisRY,     // Ry → axis 4 (ENC5)
        axisRZ,     // Rz → axis 5 (Hall B)
        axisSlider, // Slider → axis 6
        axisDial,   // Dial   → axis 7
        axisVx,     // Vx     → axis 8
        axisVy      // Vy     → axis 9
    };
    Gamepad.sendReport(&report, sizeof(report));
}


void uartSend(const char* cat, const char* func, const char* val) {
    if (roundWheelMode) return;  // No WT32 in round wheel mode
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "$%s:%s:%s\n", cat, func, val);
    ButtonBoxSerial.print(buffer);
}

void uartSendInt(const char* cat, const char* func, int value) {
    if (roundWheelMode) return;  // No WT32 in round wheel mode
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "$%s:%s:%d\n", cat, func, value);
    ButtonBoxSerial.print(buffer);
}

// ================================
// UART roundtrip test (Wheel <-> WT32)
// ================================
String wt32RxLine;
uint16_t uartPingSeq = 0;
uint16_t uartPingPendingSeq = 0;
unsigned long uartPingSentAtMs = 0;
unsigned long uartLastPingAtMs = 0;
static const unsigned long UART_PING_INTERVAL_MS = 1500;
static const unsigned long UART_PING_TIMEOUT_MS = 1200;

// WT32 connection state: tracks whether WT32 responds to PINGs
bool wt32Connected = false;
uint8_t wt32PingFailCount = 0;
static const uint8_t WT32_MAX_PING_FAILS = 3;  // Disconnect after 3 consecutive timeouts

void handleWt32UartRx() {
    while (ButtonBoxSerial.available()) {
        char c = (char)ButtonBoxSerial.read();
        if (c == '\r') continue;

        if (c == '\n') {
            if (wt32RxLine.length() > 0) {
                DBGF("[UART] RX raw: %s", wt32RxLine.c_str());
                if (wt32RxLine.startsWith("$WT:PONG:")) {
                    String seqStr = wt32RxLine.substring(9);
                    seqStr.trim();
                    uint16_t seq = (uint16_t)seqStr.toInt();

                    if (uartPingPendingSeq != 0 && seq == uartPingPendingSeq) {
                        unsigned long rtt = millis() - uartPingSentAtMs;
                        DBGF("[UART] PONG seq=%u RTT=%lums", (unsigned)seq, rtt);
                        uartPingPendingSeq = 0;
                        if (!wt32Connected) {
                            wt32Connected = true;
                            DBG("[UART] WT32 connected!");
                        }
                        wt32PingFailCount = 0;
                    } else {
                        DBGF("[UART] PONG unexpected seq=%u pending=%u",
                             (unsigned)seq,
                             (unsigned)uartPingPendingSeq);
                    }
                }
            }
            wt32RxLine = "";
            continue;
        }

        if (wt32RxLine.length() < 80) {
            wt32RxLine += c;
        }
    }
}

void uartRoundtripTask() {
    unsigned long now = millis();

    if (uartPingPendingSeq != 0) {
        if ((now - uartPingSentAtMs) >= UART_PING_TIMEOUT_MS) {
            DBGF("[UART] PING timeout seq=%u", (unsigned)uartPingPendingSeq);
            uartPingPendingSeq = 0;
            wt32PingFailCount++;
            if (wt32Connected && wt32PingFailCount >= WT32_MAX_PING_FAILS) {
                wt32Connected = false;
                DBG("[UART] WT32 disconnected (3 timeouts)");
            }
        }
        return;
    }

    if ((now - uartLastPingAtMs) < UART_PING_INTERVAL_MS) return;
    uartLastPingAtMs = now;

    uartPingSeq++;
    if (uartPingSeq == 0) uartPingSeq = 1;
    uartPingPendingSeq = uartPingSeq;
    uartPingSentAtMs = now;

    uartSendInt("BB", "PING", (int)uartPingPendingSeq);
    DBGF("[UART] PING seq=%u", (unsigned)uartPingPendingSeq);
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
    clutchCfg.mode = CLUTCH_DUAL;
    // Load saved calibration; default to a sensible narrow range (not 0-4095)
    // so both halls register movement even before a proper calibration is done.
    clutchCfg.hallMinA = prefs.getUShort("h1min", 1400);
    clutchCfg.hallMaxA = prefs.getUShort("h1max", 2200);
    clutchCfg.hallMinB = prefs.getUShort("h2min", 1400);
    clutchCfg.hallMaxB = prefs.getUShort("h2max", 2200);

    // Sanity check: if range is too wide (>2000 counts = old 0–4095 default saved to flash),
    // old/corrupt calibration — reset both halls to sensible defaults so movement is visible.
    if (clutchCfg.hallMaxA <= clutchCfg.hallMinA || (clutchCfg.hallMaxA - clutchCfg.hallMinA) > 2000) {
        DBG("[HALL] Hall A cal invalid/too wide — resetting to defaults 1400-2200");
        clutchCfg.hallMinA = 1400; clutchCfg.hallMaxA = 2200;
    }
    if (clutchCfg.hallMaxB <= clutchCfg.hallMinB || (clutchCfg.hallMaxB - clutchCfg.hallMinB) > 2000) {
        DBG("[HALL] Hall B cal invalid/too wide — resetting to defaults 1400-2200");
        clutchCfg.hallMinB = 1400; clutchCfg.hallMaxB = 2200;
    }
    DBGF("[HALL] loadConfig: A[%u-%u] B[%u-%u]",
        clutchCfg.hallMinA, clutchCfg.hallMaxA,
        clutchCfg.hallMinB, clutchCfg.hallMaxB);
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
    // Exclude 5-way directions (slots 25-28) from HID buttons
    // Directions are handled by HAT switch instead
    if (!REPORT_SHIFT_IN_HID && buttonNum == BUTTON_SHIFT) return false;
    if (isFivewayDirection(buttonNum)) return false;
    return buttonNum <= MATRIX_HID_MAX;
}

// ================================
// MATRIX
// ================================
bool mcpAvailable = false;

void scanI2CBusDebug() {
    int sclLevel = digitalRead(I2C_SCL);
    int sdaLevel = digitalRead(I2C_SDA);
    DBGF("[I2C] Line state: SCL=%d SDA=%d (expected both HIGH=1 when idle)", sclLevel, sdaLevel);

    DBG("[I2C] Probing known addresses...");
    bool foundAny = false;

    Wire.beginTransmission(0x20);
    if (Wire.endTransmission() == 0) {
        DBG("[I2C] Found MCP23017 at 0x20");
        foundAny = true;
    } else {
        DBG("[I2C] No response at 0x20 (MCP23017)");
    }

    Wire.beginTransmission(0x40);
    if (Wire.endTransmission() == 0) {
        DBG("[I2C] Found PCA9685 at 0x40");
        foundAny = true;
    } else {
        DBG("[I2C] No response at 0x40 (PCA9685)");
    }

    if (!foundAny) {
        DBG("[I2C] No known devices responded");
    }
}

void setupButtonMatrix() {
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(400000);
    Wire.setTimeOut(20);
    DBG("[I2C] Bus configured: 400kHz, timeout=20ms");

    if (!mcp.begin_I2C(0x20)) {
        DBG("[ERROR] MCP23017 NOT FOUND at 0x20!");
        DBG("[HINT] Check MCP: pin9=3.3V, pin10=GND, pin12=SCL(GPIO9), pin13=SDA(GPIO8), pin18=RESET(3.3V), pin15/16/17=A0/A1/A2(GND)");
        DBG("[HINT] If scan finds none: add 4.7k pull-up on SDA->3.3V and SCL->3.3V");
        scanI2CBusDebug();
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

// Throttle: matrix scan only every MATRIX_SCAN_INTERVAL_MS
static const unsigned long MATRIX_SCAN_INTERVAL_MS = 3;
static unsigned long lastMatrixScanMs = 0;

void scanButtonMatrix() {
    if (!mcpAvailable) return;
    unsigned long currentTime = millis();
    if ((currentTime - lastMatrixScanMs) < MATRIX_SCAN_INTERVAL_MS) return;
    lastMatrixScanMs = currentTime;

    for (uint8_t col = 0; col < MATRIX_COLS; col++) {
        mcp.digitalWrite(col, LOW);
        delayMicroseconds(5);

        // Bulk-read all 8 row pins at once (1 I2C transaction instead of 8)
        uint8_t rowBits = mcp.readGPIO(1); // port B = rows

        for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
            bool reading = !(rowBits & (1 << row)); // active LOW

            if (reading != prevButtonStates[row][col]) {
                lastDebounceTime[row][col] = currentTime;
            }

            if ((currentTime - lastDebounceTime[row][col]) > DEBOUNCE_DELAY) {
                if (reading != buttonStates[row][col]) {
                    if ((currentTime - lastStableToggleTime[row][col]) < MIN_TOGGLE_INTERVAL_MS) {
                        prevButtonStates[row][col] = reading;
                        continue;
                    }

                    buttonStates[row][col] = reading;
                    lastStableToggleTime[row][col] = currentTime;

                    uint8_t buttonNum = getButtonNumber(row, col);

                    // Trigger LED flash on press
                    if (reading) {
                        ledTriggerFlash(buttonNum);
                    }

                    if (buttonNum == BUTTON_SHIFT) {
                        DBG(reading ? "[SHIFT] DOWN" : "[SHIFT] UP");
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

    // SHIFT + ENC2-5 (idx 1-4): trigger alternate function buttons 25-28, 56-59
    if (isButtonPressed(BUTTON_SHIFT) && idx >= 1 && idx <= 4) {
        uint8_t shiftIdx = (idx - 1) * 2;
        triggerPulse(step > 0 ? encoderShiftPulses[shiftIdx] : encoderShiftPulses[shiftIdx + 1]);
        sendGamepad();
        return;
    }

    if (step > 0) {
        triggerPulse(encoderPulses[pulseIdxCW]);
    } else {
        triggerPulse(encoderPulses[pulseIdxCCW]);
    }
    sendGamepad();
}

void handleMfcRotate(int8_t step) {
    bool shiftPressed = isButtonPressed(BUTTON_SHIFT);
    const char* dir = (step > 0) ? "CW" : "CCW";

    if (MFC_DEBUG_LOG) {
        DBGF("[MFC ENC1] ROT dir=%s step=%+d shift=%d adjust=%d idx=%d item=%s",
             dir,
             step,
             shiftPressed ? 1 : 0,
             mfcAdjustMode ? 1 : 0,
             mfcIndex,
             mfcMenuNames[mfcIndex]);
    }

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
            // Round wheel: update MAX7219 intensity and WS2812 luminance
            if (roundWheelMode) {
                uint8_t max7219Int = map(constrain(brightnessValue, 15, 255), 15, 255, 0, 15);
                max7219SetIntensity(max7219Int);
                ws2812Strip.SetLuminance(constrain(brightnessValue, 0, 255));
            }
        } else if (item == MFC_PAGE) {
            pageValue += step;
            if (pageValue < 0) pageValue = 6;
            if (pageValue > 6) pageValue = 0;
            uartSend("PAGE", step > 0 ? "NEXT" : "PREV", "");
        } else if (item == MFC_VOL_SYS) {
            // HID Consumer Control Volume (implemented separately)
            sendConsumerControl(step > 0 ? 0xE9 : 0xEA); // Volume Up/Down
        } else if (item == MFC_VOL_A) {
            // Virtual buttons 36 (UP) / 37 (DN)
            triggerVirtualButton(step > 0 ? 36 : 37);
            sendGamepad();
        } else if (item == MFC_VOL_B) {
            // Virtual buttons 38 (UP) / 39 (DN)
            triggerVirtualButton(step > 0 ? 38 : 39);
            sendGamepad();
        } else if (item == MFC_TC2) {
            // SHIFT+TC2 → TC3 function (62/63); plain → TC2 (60/61)
            triggerVirtualButton(shiftPressed ? (step > 0 ? 62 : 63) : (step > 0 ? 60 : 61));
            sendGamepad();
        } else if (item == MFC_FFB) {
            // Virtual buttons 62 (UP) / 63 (DN)
            triggerVirtualButton(step > 0 ? 62 : 63);
            sendGamepad();
        } else if (item == MFC_TYRE) {
            // Virtual buttons 64 (UP) / 35 (DN)
            triggerVirtualButton(step > 0 ? 64 : 35);
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

        // Only process when a pin actually changed
        if (currentA != encoderStates[i].lastA || currentB != encoderStates[i].lastB) {
            encoderStates[i].lastA = currentA;
            encoderStates[i].lastB = currentB;

            // Minimal contact-bounce debounce
            unsigned long now = micros();
            if ((now - encoderStates[i].lastChangeTime) < ENCODER_DEBOUNCE_US) {
                continue;
            }
            encoderStates[i].lastChangeTime = now;

            // Gray-code state machine
            int8_t encoded = (currentA << 1) | currentB;
            uint8_t tableIdx = ((encoderStates[i].lastEncoded << 2) | encoded) & 0x0F;
            encoderStates[i].accumulator += ENC_TABLE[tableIdx];
            encoderStates[i].lastEncoded = encoded;

            // Full detent reached? (4 consistent transitions = 1 physical click)
            int8_t step = 0;
            if (encoderStates[i].accumulator >= ENCODER_DETENT_TRANSITIONS) {
                step = 1;
                encoderStates[i].accumulator = 0;  // clean reset
            } else if (encoderStates[i].accumulator <= -ENCODER_DETENT_TRANSITIONS) {
                step = -1;
                encoderStates[i].accumulator = 0;  // clean reset
            }

            if (step == 0) continue;

            // --- Valid step detected ---
            encoderStates[i].encoderValue += step;

            if (ENCODER_DIR_DEBUG_LOG) {
                const char* dir = (step > 0) ? "CW" : "CCW";
                if (i == 0) {
                    DBGF("[ENC1 MFC] %s val=%d", dir, (int)encoderStates[i].encoderValue);
                } else {
                    DBGF("[ENC%u] %s val=%d mode=%s",
                         (unsigned)(i + 1), dir,
                         (int)encoderStates[i].encoderValue,
                         encoderButtonMode ? "BTN" : "AXIS");
                }
            }

            if (i == 0) {
                handleMfcRotate(step);
            } else {
                if (encoderButtonMode) {
                    handleEncoderButton(i, step);
                } else {
                    // AXIS mode: SHIFT + ENC2-5 fires virtual button instead of moving axis
                    if (isButtonPressed(BUTTON_SHIFT) && i >= 1 && i <= 4) {
                        handleEncoderButton(i, step);
                    } else {
                        // Direct axis output — no smoothing, no threshold, zero lag
                        int8_t val = (int8_t)constrain(encoderStates[i].encoderValue, -127, 127);
                        handleEncoderAxis(i, val);
                    }
                }
            }
        } else {
            // Pins unchanged — update lastA/lastB for next comparison
            encoderStates[i].lastA = currentA;
            encoderStates[i].lastB = currentB;
        }
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

    // Release MFC menu virtual buttons
    for (auto &pulse : mfcVirtualButtons) {
        if (pulse.active && now >= pulse.releaseAt) {
            buttons &= ~(1ULL << (pulse.id - 1));
            pulse.active = false;
            changed = true;
        }
    }

    // Release SHIFT+encoder alternate function pulses (buttons 25-28, 56-59)
    for (auto &pulse : encoderShiftPulses) {
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
// Insertion sort (in-place) for small arrays
static void sortU16(uint16_t* arr, uint8_t n) {
    for (uint8_t i = 1; i < n; i++) {
        uint16_t key = arr[i];
        int8_t j = i - 1;
        while (j >= 0 && arr[j] > key) { arr[j + 1] = arr[j]; j--; }
        arr[j + 1] = key;
    }
}

// Median of HALL_MEDIAN_SAMPLES readings — immune to spikes of any magnitude
// Settling delay between consecutive reads prevents ADC channel crosstalk.
static uint16_t hallReadMedian(uint8_t pin) {
    uint16_t buf[HALL_MEDIAN_SAMPLES];
    for (uint8_t i = 0; i < HALL_MEDIAN_SAMPLES; i++) {
        buf[i] = analogRead(pin);
        delayMicroseconds(20);
    }
    sortU16(buf, HALL_MEDIAN_SAMPLES);
    return buf[HALL_MEDIAN_SAMPLES / 2];
}

// Detect if a hall sensor is physically connected by reading ADC variance.
// A real sensor outputs a stable voltage (~1.0-3.0V) → low variance.
// A floating pin bounces across the full ADC range → high variance.
// Also checks if readings are stuck at rail (0 or 4095) = no sensor.
static bool detectHallPresence(uint8_t pin, const char* label) {
    static const uint8_t DETECT_SAMPLES = 32;
    static const uint16_t VARIANCE_THRESHOLD = 50000;  // ~224 count stdev
    static const uint16_t RAIL_LOW = 50;
    static const uint16_t RAIL_HIGH = 4045;

    uint32_t sum = 0;
    uint32_t sumSq = 0;
    uint16_t minVal = 4095;
    uint16_t maxVal = 0;
    uint16_t samples[DETECT_SAMPLES];

    for (uint8_t i = 0; i < DETECT_SAMPLES; i++) {
        samples[i] = analogRead(pin);
        sum += samples[i];
        if (samples[i] < minVal) minVal = samples[i];
        if (samples[i] > maxVal) maxVal = samples[i];
        delayMicroseconds(200);  // Spread reads over ~6ms
    }

    uint16_t mean = sum / DETECT_SAMPLES;
    for (uint8_t i = 0; i < DETECT_SAMPLES; i++) {
        int32_t diff = (int32_t)samples[i] - (int32_t)mean;
        sumSq += diff * diff;
    }
    uint32_t variance = sumSq / DETECT_SAMPLES;

    // Check: stuck at rail?
    bool atRail = (mean < RAIL_LOW || mean > RAIL_HIGH);
    // Check: high variance = floating pin
    bool highVariance = (variance > VARIANCE_THRESHOLD);
    // Check: very wide min-max spread
    bool wideSpread = ((maxVal - minVal) > 500);

    bool connected = !atRail && !highVariance && !wideSpread;

    DBGF("[HALL] %s detect: mean=%u min=%u max=%u var=%lu spread=%u → %s",
        label, mean, minVal, maxVal, (unsigned long)variance,
        maxVal - minVal, connected ? "CONNECTED" : "NOT CONNECTED");

    return connected;
}

void updateClutches() {
    // Skip entirely if no hall sensors connected — axes stay at 0
    if (!hallAConnected && !hallBConnected) return;

    // Read Hall with median filter; extra delay between channels prevents crosstalk
    uint16_t rawA = hallAConnected ? hallReadMedian(HALL_A_PIN) : (uint16_t)((clutchCfg.hallMinA + clutchCfg.hallMaxA) / 2);
    delayMicroseconds(50);
    uint16_t rawB = hallBConnected ? hallReadMedian(HALL_B_PIN) : (uint16_t)((clutchCfg.hallMinB + clutchCfg.hallMaxB) / 2);

    if (HALL_RAW_DEBUG) {
        unsigned long nowMs = millis();
        if (nowMs - lastHallRawDebugMs >= HALL_RAW_DEBUG_MS) {
            lastHallRawDebugMs = nowMs;
            int8_t dbgA = mapHallToAxis(rawA, clutchCfg.hallMinA, clutchCfg.hallMaxA);
            int8_t dbgB = mapHallToAxis(rawB, clutchCfg.hallMinB, clutchCfg.hallMaxB);
            DBGF("[HALL RAW] GPIO4=%u GPIO2=%u | filt A=%ld B=%ld | mapped A=%d B=%d | cal A[%u-%u] B[%u-%u]",
                rawA, rawB, clutchRawAFiltered, clutchRawBFiltered, dbgA, dbgB,
                clutchCfg.hallMinA, clutchCfg.hallMaxA,
                clutchCfg.hallMinB, clutchCfg.hallMaxB);
        }
    }

    // Low-pass raw hall readings to suppress floating input jitter
    if (!clutchFilterInit) {
        clutchRawAFiltered = rawA;
        clutchRawBFiltered = rawB;
        clutchFilterInit = true;
    } else {
        clutchRawAFiltered += ((int32_t)rawA - clutchRawAFiltered) >> CLUTCH_RAW_FILTER_SHIFT;
        clutchRawBFiltered += ((int32_t)rawB - clutchRawBFiltered) >> CLUTCH_RAW_FILTER_SHIFT;
    }

    uint16_t filtA = (uint16_t)constrain(clutchRawAFiltered, 0, 4095);
    uint16_t filtB = (uint16_t)constrain(clutchRawBFiltered, 0, 4095);

    if (calibratingHall) {
        if (filtA < clutchCfg.hallMinA) clutchCfg.hallMinA = filtA;
        if (filtA > clutchCfg.hallMaxA) clutchCfg.hallMaxA = filtA;
        if (filtB < clutchCfg.hallMinB) clutchCfg.hallMinB = filtB;
        if (filtB > clutchCfg.hallMaxB) clutchCfg.hallMaxB = filtB;
    }

    int8_t a = mapHallToAxis(filtA, clutchCfg.hallMinA, clutchCfg.hallMaxA);
    int8_t b = mapHallToAxis(filtB, clutchCfg.hallMinB, clutchCfg.hallMaxB);

    int8_t outA = a;
    int8_t outB = b;

    if (clutchCfg.mode == CLUTCH_MIRROR) {
        int8_t avg = (int8_t)((a + b) / 2);
        outA = avg;
        outB = avg;
    } else if (clutchCfg.mode == CLUTCH_BITE) {
        // F1-style launch clutch
        //  Both pressed fully  → anti-stall: output = 127 (fully disengaged)
        //  One released        → held paddle maps 0-127 → 0-bite (fine control)
        //  Both released       → output = 0 (fully engaged), exit launch mode
        static bool launchMode = false;

        int8_t minBite = (int8_t)((clutchCfg.bitePoint * 127) / 100);
        int8_t lo = (a < b) ? a : b;  // less-pressed paddle
        int8_t hi = (a > b) ? a : b;  // more-pressed paddle

        // Enter: both paddles above 80% (~100/127)
        if (a > 100 && b > 100) {
            launchMode = true;
        }

        int8_t combined;
        if (launchMode) {
            if (hi < 10) {
                // Both fully released — disengage launch mode
                launchMode = false;
                combined = 0;
            } else if (lo < 20) {
                // One paddle released (lo ≈ 0): hi paddle controls 0→bite range
                // Threshold of 20 (~16%) avoids toggling on noise near zero
                combined = (int8_t)(((int32_t)hi * minBite) / 127);
            } else {
                // Both still pressed: anti-stall, fully disengaged
                combined = 127;
            }
        } else {
            // Normal driving: use whichever paddle is more pressed
            combined = hi;
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

    // In non-DUAL modes both paddles contribute to a single combined clutch.
    // Collapse to Z only (Rz = 0) for games that accept only one clutch axis.
    if (clutchCfg.mode != CLUTCH_DUAL) {
        finalA = (finalA > finalB) ? finalA : finalB;
        finalB = 0;
    }

    // Report only meaningful clutch changes to avoid noise spam on floating inputs
    if (abs(finalA - axisZ) >= CLUTCH_AXIS_DEADZONE || abs(finalB - axisRZ) >= CLUTCH_AXIS_DEADZONE) {
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

    if (MFC_DEBUG_LOG && mfcPressed != lastMfcPressed) {
        DBGF("[MFC SW] %s shift=%d", mfcPressed ? "DOWN" : "UP", shiftPressed ? 1 : 0);
    }

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
                if (MFC_DEBUG_LOG) {
                    DBGF("[MFC MODE] ENC_MODE=%s (hold=%lums)", encoderButtonMode ? "BTN" : "AXIS", holdTime);
                }
                sendGamepad();
            }
        }
    } else if (!mfcPressed && mfcPressStart > 0 && mfcPressStart != UINT32_MAX) {
        // Released after short hold: check for preset
        unsigned long holdTime = millis() - mfcPressStart;

        if (holdTime < MODE_HOLD_MS && holdTime > 50) {  // Valid short press
            MfcMenuItem item = (MfcMenuItem)mfcIndex;

            if (MFC_DEBUG_LOG) {
                DBGF("[MFC SW] SHORT hold=%lums item=%s", holdTime, mfcMenuNames[item]);
            }

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
            item == MFC_TC2 || item == MFC_FFB || item == MFC_TYRE ||
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
                } else if (item == MFC_FFB) {
                    uartSend("MFC", "ADJUST", "FFB");
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
                if (MFC_DEBUG_LOG) {
                    DBGF("[MFC MODE] ENC_MODE=%s (menu toggle)", encoderButtonMode ? "BTN" : "AXIS");
                }
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
    ButtonBoxSerial.begin(UART_BAUD, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
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
    // Sem encoders/MFC disponíveis, força modo previsível para teste dos halls
    clutchCfg.mode = CLUTCH_DUAL;
    clutchChannelsSwapped = false;
    DBG("[BOOT] Preferences loaded");

    // Configure ADC for Hall sensors (0-3.3V range)
    // analogSetPinAttenuation internally calls IDF adc1_config_channel_atten which
    // re-enables the pad pull-up, overriding any prior pinMode(INPUT).
    // Fix: call gpio_set_pull_mode(GPIO_FLOATING) via IDF AFTER attenuation setup.
    analogReadResolution(12);
    analogSetPinAttenuation(HALL_A_PIN, ADC_11db);  // GPIO1: 0-3.3V
    analogSetPinAttenuation(HALL_B_PIN, ADC_11db);  // GPIO2: 0-3.3V
    gpio_set_pull_mode(GPIO_NUM_1, GPIO_FLOATING);  // disable pull-up re-enabled by IDF ADC init
    gpio_set_pull_mode(GPIO_NUM_2, GPIO_FLOATING);  // same for Hall B
    DBG("[ADC] Hall sensor pins configured (GPIO1=HallA, GPIO2=HallB, 12-bit, 11dB, pull-up disabled)");

    // Detect hall sensor presence (variance test)
    hallAConnected = detectHallPresence(HALL_A_PIN, "HallA(GPIO1)");
    hallBConnected = detectHallPresence(HALL_B_PIN, "HallB(GPIO2)");
    if (!hallAConnected && !hallBConnected) {
        DBG("[HALL] No hall sensors detected — clutch axes locked at 0");
    } else {
        DBGF("[HALL] HallA=%s HallB=%s",
            hallAConnected ? "OK" : "absent",
            hallBConnected ? "OK" : "absent");
    }

    DBG("[I2C] Setting up MCP23017 button matrix...");
    setupButtonMatrix();
    if (mcpAvailable) {
        DBG("[I2C] MCP23017 OK at 0x20");
    }

    DBG("[I2C] Setting up PCA9685 front LEDs...");
    setupLEDs();
    if (ledAvailable) {
        DBG("[I2C] PCA9685 OK at 0x40");
        ledBootSweep();
    }

    setupEncoders();
    DBG("[BOOT] Encoders OK");

    // Round wheel mode: auto-detect based on PCA9685 absence
    roundWheelMode = !ledAvailable;
    if (roundWheelMode) {
        DBG("[MODE] Round wheel mode ACTIVE (no PCA9685 detected)");
        DBG("[MODE] Initializing MAX7219 + WS2812 + SimHub CDC...");
        setupMax7219();
        setupWs2812();
        // Apply saved brightness to MAX7219 and WS2812
        uint8_t max7219Int = map(constrain(brightnessValue, 15, 255), 15, 255, 0, 15);
        max7219SetIntensity(max7219Int);
        ws2812Strip.SetLuminance(constrain(brightnessValue, 0, 255));
        DBG("[MODE] Round wheel peripherals initialized OK");
    } else {
        DBG("[MODE] F1 wheel mode (PCA9685 detected, UART to WT32)");
    }

    uartSend("SYS", "BOOT", DEVICE_NAME);
    if (!roundWheelMode) {
        DBG("[UART] Roundtrip enabled: TX=GPIO43 RX=GPIO11");
    }
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
    // UART to WT32: only in F1 mode (round wheel has no WT32)
    if (!roundWheelMode) {
        handleWt32UartRx();
        uartRoundtripTask();
    }

    // Encoders first — GPIO-only, sub-microsecond, needs highest poll rate
    scanEncoders();

    // I2C-heavy operations are internally throttled
    scanButtonMatrix();       // every 3ms (was every loop = ~28ms I2C stall)
    ledUpdate();              // every 16ms (~60fps, humans can't see faster)

    // Lightweight operations — always run
    handleMfcPress();
    handleMultimediaButtons();
    releaseVirtualButtonPulses();
    updateClutches();
    handleShiftClutchCombo();

    // Round wheel mode: SimHub CDC protocol + local display
    if (roundWheelMode) {
        processSimHubCDC();
        updateMax7219Display();
        updateWs2812LEDs();
    }
}
