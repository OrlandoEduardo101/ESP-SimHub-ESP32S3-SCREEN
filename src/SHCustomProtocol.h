#ifndef __SHCUSTOMPROTOCOL_H__
#define __SHCUSTOMPROTOCOL_H__

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <map>
#include "logo_image.h"  // Logo image array
#include "TrackMaps.h"   // Track map PROGMEM point arrays
#include <Wire.h>
#include <Wire.h>

// Forward declaration for screenLog
extern void screenLog(const String &msg);

// Forward declaration for debugLog
extern void debugLog(const String &msg);

#ifdef INCLUDE_RGB_LEDS_NEOPIXELBUS
void neoPixelBusSetLuminance(uint8_t value);
uint8_t neoPixelBusGetLuminance();
#endif

// WT32-SC01 Plus - ST7796 via 8-bit MCU (8080) parallel interface (320x480)
// IMPORTANT: WT32-SC01 Plus uses 8-bit parallel interface, NOT SPI!
// Pinout according to official WT32-SC01 Plus documentation:
// https://github.com/Cesarbautista10/WT32-SC01-Plus-ESP32
#include <Buzzer.h>
#if 1  // Always use ST7796 for WT32-SC01 Plus
// LCD Interface pins (8-bit MCU 8080)
#define TFT_BL 45    // BL_PWM - Backlight control (active high)
#define TFT_RST 4    // LCD_RESET - LCD reset (multiplexed with touch reset)
#define TFT_RS 0     // LCD_RS - Command/Data selection
#define TFT_WR 47    // LCD_WR - Write clock
#define TFT_TE 48    // LCD_TE - Frame sync (optional, can use -1 if not needed)
// 8-bit data bus (LCD_DB0 to LCD_DB7)
#define TFT_D0 9
#define TFT_D1 46
#define TFT_D2 3
#define TFT_D3 8
#define TFT_D4 18
#define TFT_D5 17
#define TFT_D6 16
#define TFT_D7 15

// Use 8-bit parallel interface for ESP32-S3
// Arduino_ESP32PAR8 supports 8-bit MCU (8080) interface for ESP32, ESP32-S2, and ESP32-S3
// Constructor: dc, cs, wr, rd, d0, d1, d2, d3, d4, d5, d6, d7
// Note: Using PAR8 instead of LCD8 to avoid "no free i80 bus slot" error
// IMPORTANT: Objects created as pointers and initialized in setup() to avoid initialization issues
// Creating them in setup() instead of globally prevents "no free i80 bus slot" error
Arduino_DataBus *bus = nullptr;
Arduino_ST7796 *tft = nullptr;    // Hardware display (init in setup)
Arduino_Canvas *canvas = nullptr;  // Optional PSRAM framebuffer
Arduino_GFX *gfx = nullptr;       // Drawing target (canvas if PSRAM, else tft)

// Touch screen configuration for WT32-SC01 Plus (FT6336U capacitive touch)
// According to: https://github.com/Cesarbautista10/WT32-SC01-Plus-ESP32
#define TOUCH_SDA 6   // I2C_SDA - Touch data
#define TOUCH_SCL 5   // I2C_SCL - Touch clock
#define TOUCH_INT 7   // INT - Touch interrupt (optional)
#define TOUCH_RST 4   // RST - Touch reset (shared with LCD reset)
#define TOUCH_ADDRESS 0x38  // FT6336U I2C address
#define TOUCH_WIDTH SCREEN_WIDTH
#define TOUCH_HEIGHT SCREEN_HEIGHT

// Simple touch point structure
struct TouchPoint {
	int16_t x;
	int16_t y;
	bool touched;
};

bool touchInitialized = false;

#else
// RGB Panel displays (480x272 or 800x480) - not used for WT32-SC01 Plus
#define TFT_BL 2 // backlight pin

// 4827S043 - 480x270, no touch
Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
    40 /* DE */, 41 /* VSYNC */, 39 /* HSYNC */, 42 /* PCLK */,
    45 /* R0 */, 48 /* R1 */, 47 /* R2 */, 21 /* R3 */, 14 /* R4 */,
    5 /* G0 */, 6 /* G1 */, 7 /* G2 */, 15 /* G3 */, 16 /* G4 */, 4 /* G5 */,
    8 /* B0 */, 3 /* B1 */, 46 /* B2 */, 9 /* B3 */, 1 /* B4 */,
    0 /* hsync_polarity */, 1 /* hsync_front_porch */, 1 /* hsync_pulse_width */, 43 /* hsync_back_porch */,
    0 /* vsync_polarity */, 3 /* vsync_front_porch */, 1 /* vsync_pulse_width */, 12 /* vsync_back_porch */,
    1 /* pclk_active_neg */, 10000000 /* prefer_speed */);

// https://github.com/eCrowneEng/ESP-SimHub-ESP32S3-SCREEN/issues/1
// 8048S043 - 800x480, capacitive touch
//Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
//    40 /* DE */, 41 /* VSYNC */, 39 /* HSYNC */, 42 /* PCLK */,
//    45 /* R0 */, 48 /* R1 */, 47 /* R2 */, 21 /* R3 */, 14 /* R4 */,
//    5 /* G0 */, 6 /* G1 */, 7 /* G2 */, 15 /* G3 */, 16 /* G4 */, 4 /* G5 */,
//    8 /* B0 */, 3 /* B1 */, 46 /* B2 */, 9 /* B3 */, 1 /* B4 */,
//    0 /* hsync_polarity */, 8 /* hsync_front_porch */, 4 /* hsync_pulse_width */, 16 /* hsync_back_porch */,
//    0 /* vsync_polarity */, 4 /* vsync_front_porch */, 4 /* vsync_pulse_width */, 4 /* vsync_back_porch */,
//    1 /* pclk_active_neg */, 16000000 /* prefer_speed */);

Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
    PIXEL_WIDTH /* width */, PIXEL_HEIGHT /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */);
#endif

static const int SCREEN_WIDTH = PIXEL_WIDTH;
static const int SCREEN_HEIGHT = PIXEL_HEIGHT;
static const int X_CENTER = SCREEN_WIDTH / 2;
static const int Y_CENTER = SCREEN_HEIGHT / 2;
static const int ROWS = 5;
static const int COLS = 5;
static const int CELL_WIDTH = SCREEN_WIDTH / COLS;
static const int HALF_CELL_WIDTH = CELL_WIDTH / 2;
static const int CONTENT_HEIGHT = SCREEN_HEIGHT - 40;  // Reserve 40px at bottom for padding + indicator
static const int CELL_HEIGHT = CONTENT_HEIGHT / ROWS;
static const int HALF_CELL_HEIGHT = CELL_HEIGHT / 2;
static const int COL[] = {0, CELL_WIDTH, CELL_WIDTH * 2, CELL_WIDTH * 3, CELL_WIDTH * 4, CELL_WIDTH * 6, CELL_WIDTH * 7};
static const int ROW[] = {0, 64, 132, 200, 256};

#include <GFXHelpers.h>

std::map<String, String> prevData;
std::map<String, int32_t> prevColor;

class SHCustomProtocol {
private:
	// Global variables
	int rpmPercent = 50;
	int prev_rpmPercent = 50;
	int rpmRedLineSetting = 95;
	int currentRpms = 0;            // [4] RPM Atual (valor exato)
	String gear = "N";
	String prev_gear;
	String speed = "0";
	String currentLapTime = "00:00.00";
	String lastLapTime = "00:00.00";
	String bestLapTime = "00:00.00";
	String sessionBestLiveDeltaSeconds = "0.000";
	String sessionBestLiveDeltaProgressSeconds = "0.00";
	String tyrePressureFrontLeft = "00.0";
	String tyrePressureFrontRight = "00.0";
	String tyrePressureRearLeft = "00.0";
	String tyrePressureRearRight = "00.0";
	String oilTemperature = "0";
	String waterTemperature = "0";
	String tcLevel = "0";
	String tcActive = "0";
	String absLevel = "0";
	String absActive = "0";
	String tcCut = "0";
	String brakeBias = "0";
	String brake = "0";
	String lapInvalidated = "False";

	// Bloco 5: Estratégia (índices 32-41)
	String position = "0";
	String opponentsCount = "0";
	String driverAheadGap = "--";
	String driverBehindGap = "--";
	String fuelRemainingLaps = "0.0";
	String fuelLitersPerLap = "0.00";
	String sessionTimeLeft = "00:00:00";

	// Alert/Flag variables
	String currentFlag = "None";
	String prevFlag = "None";
	String currentPenalties = "0";
	String prevPenalties = "0";
	String cutTrackWarnings = "0";
	String alertMessage = "";  // [42] Alerta crítico (ENGINE OFF, PIT LIMITER, etc.)
	String popupMessage = "";  // [43] Pop-up temporário (BIAS, TC LEVEL, etc.)
	unsigned long alertStartTime = 0;
	bool alertWasShowing = false;  // Track if alert was displayed to trigger clear
	bool needsFullRedraw = false;  // Flag to trigger full screen redraw after alert
	bool timingFrameDrawn = false; // Flag: timing page static frame already drawn
	bool telemFrameDrawn  = false;
	bool advFrameDrawn    = false;
	bool stratFrameDrawn  = false;
	bool lapsFrameDrawn   = false;
	bool mapFrameDrawn    = false;
	bool mapTrackDrawn    = false;
	String mapLastTrackId = "";
	bool p499FrameDrawn   = false;
	static const unsigned long ALERT_DURATION_MS = 3000;  // Show alert for 3 seconds
	bool popupFromUart = false;
	unsigned long popupFromUartUntil = 0;

	// Bloco 7: Dados para Arduino LEDs (índices 44-47)
	String rpmPercent2 = "0";  // [44] RPM % (repetido)
	String spotterLeft = "0";  // [45] Spotter esquerdo
	String spotterRight = "0";  // [46] Spotter direito
	String absActive2 = "0";   // [47] ABS ativo (repetido)

	// Bloco 8: Desgaste e Ambiente (índices 48-57)
	String tyreWearFrontLeft = "0";
	String tyreWearFrontRight = "0";
	String tyreWearRearLeft = "0";
	String tyreWearRearRight = "0";
	String sector1Time = "00.000";
	String sector2Time = "00.000";
	String sector3Time = "00.000";
	String airTemperature = "0";
	String roadTemperature = "0";

	// Novos campos adicionados (índices 57-61)
	String shiftLightTrigger = "0";  // [57] Shift light trigger (0/1)
	String drsAvailable = "0";       // [58] DRS disponível (0/1)
	String drsActive = "0";          // [59] DRS ativo (0/1)
	String kersLevel = "0";          // [60] Bateria KERS (0-100%)
	String turboBoost = "0.0";       // [61] Pressão turbo (Bar)

	// Bloco 9: Dados 499P (índices 62-67)
	String ersDeployMode = "None";   // [62] ERS Deploy Mode
	String arbFront = "--";          // [63] ARB Front
	String arbRear = "--";           // [64] ARB Rear
	String brkMigration = "--";      // [65] Brake Migration
	String headWind = "0";           // [66] Wind Speed
	String rearBrakeBias = "0.0";    // [67] Rear Brake Bias

	// Bloco 10: Track Map (índices 68-71)
	String trackPositionPercent = "0.000";  // [68] Posição jogador na pista (0.0-1.0)
	String aheadTrackPosition = "0.000";    // [69] Posição carro da frente
	String behindTrackPosition = "0.000";   // [70] Posição carro de trás
	String trackId = "Unknown";             // [71] Nome/ID da pista

	// Temperatura pneus (índices 15-18) - promoted from local vars
	String tyreTemperatureFrontLeft = "0";
	String tyreTemperatureFrontRight = "0";
	String tyreTemperatureRearLeft = "0";
	String tyreTemperatureRearRight = "0";

	// Temperatura freios (índices 19-22) - promoted from local vars
	String brakeTemperatureFrontLeft = "0";
	String brakeTemperatureFrontRight = "0";
	String brakeTemperatureRearLeft = "0";
	String brakeTemperatureRearRight = "0";

	String prevDrsAvailable = "0";   // DRS edge detection for buzzer

	int cellTitleHeight = 0;
	bool hasReceivedData = false;
	bool displayEnabled = true;  // Display enabled for dashboard
	bool loadingScreenShown = false;  // Track if loading screen has been shown
	bool touchInitAttempted = false;  // Track if we've already tried to init touch

	int backlightLevel = 220;  // 0-255
	bool backlightPwmReady = false;
	static const int BACKLIGHT_MIN = 15;
	static const int BACKLIGHT_MAX = 255;
	static const int BACKLIGHT_STEP = 15;
	static const int BACKLIGHT_PWM_CHANNEL = 1;
	static const int BACKLIGHT_PWM_FREQ = 5000;
	static const int BACKLIGHT_PWM_RES = 8;

	// Multi-page dashboard variables
	enum DashboardPage {
		PAGE_RACE = 0,
		PAGE_TIMING = 1,
		PAGE_TELEMETRY = 2,
		PAGE_ADVANCED = 3,       // Advanced telemetry (Motor, Wear, Env, DRS, KERS, Turbo)
		PAGE_RELATIVE = 4,       // Relative/Head-to-head
		PAGE_LAPS = 5,           // Laps/Sectors analysis
		PAGE_MAP = 6,            // Track Map + Strategy
		PAGE_499P = 7            // Ferrari 499P dashboard
	};
	DashboardPage currentPage = PAGE_RACE;
	DashboardPage lastPage = PAGE_RACE;  // Track previous page to detect page changes
	unsigned long lastTouchTime = 0;
	bool prevTouched = false;  // Rising-edge detection: prevents phantom stuck-touch from cycling pages
	static const unsigned long TOUCH_DEBOUNCE_MS = 500;  // Debounce time between page changes

	// Helper function to safely use display
	bool canUseDisplay() {
		return displayEnabled && gfx != nullptr;
	}

	void applyBacklight() {
		#ifdef TFT_BL
		if (TFT_BL >= 0 && TFT_BL < 48) {
			int level = backlightLevel;
			if (level < BACKLIGHT_MIN) level = BACKLIGHT_MIN;
			if (level > BACKLIGHT_MAX) level = BACKLIGHT_MAX;
			if (backlightPwmReady) {
				ledcWrite(BACKLIGHT_PWM_CHANNEL, level);
			} else {
				digitalWrite(TFT_BL, level > 0 ? HIGH : LOW);
			}
		}
		#endif
	}

	// Reset draw cache when changing pages
	void resetDrawCache() {
		prev_gear = "";
		prev_rpmPercent = -1;
		prevData.clear();
		prevColor.clear();
		timingFrameDrawn = false;
		telemFrameDrawn  = false;
		advFrameDrawn    = false;
		stratFrameDrawn  = false;
		lapsFrameDrawn   = false;
		mapFrameDrawn    = false;
		mapTrackDrawn    = false;
		mapLastTrackId   = "";
		p499FrameDrawn   = false;
	}

	// Navigate to next page
	void nextPage() {
		currentPage = (DashboardPage)((currentPage + 1) % 8);
		gfx->fillScreen(BLACK);
		resetDrawCache();
	}

	// Navigate to previous page
	void prevPage() {
		currentPage = (DashboardPage)((currentPage - 1 + 8) % 8);
		gfx->fillScreen(BLACK);
		resetDrawCache();
	}

	// Read touch point from FT6336U
	TouchPoint readTouch() {
		TouchPoint point = {0, 0, false};
		if (!touchInitialized) return point;

		// Read FT6336U touch data from registers
		Wire.beginTransmission(TOUCH_ADDRESS);
		Wire.write(0x02);  // TD_STATUS register
		Wire.endTransmission();

		Wire.requestFrom(TOUCH_ADDRESS, 5);  // Read 5 bytes: status + X high + X low + Y high + Y low
		if (Wire.available() >= 5) {
			uint8_t status = Wire.read();     // TD_STATUS (bit 0 = touch detected)
			uint8_t x_high = Wire.read();     // Touch X High byte
			uint8_t x_low = Wire.read();      // Touch X Low byte
			uint8_t y_high = Wire.read();     // Touch Y High byte
			uint8_t y_low = Wire.read();      // Touch Y Low byte

			// Only process if touch is detected (bit 0 set in status)
			if (status & 0x01) {
				// Extract coordinates - FT6336U stores X as 12-bit value
				point.x = ((x_high & 0x0F) << 8) | x_low;
				point.y = ((y_high & 0x0F) << 8) | y_low;
				point.touched = true;
			}
		}
		return point;
	}

	// Show loading screen with real PNG logo image
	void showLoadingScreen() {
		if (!canUseDisplay()) return;

		Serial.println("Displaying loading screen with logo...");

		// Fill screen with black background
		gfx->fillScreen(BLACK);

		// Calculate logo position - center horizontally, position in upper part of screen
		// Leave space at bottom for "Loading" text
		int logoX = (SCREEN_WIDTH - LOGO_WIDTH) / 2;   // Center horizontally

		// Position logo in upper-middle area, leaving space for text below
		// Calculate space needed for text (estimate ~40 pixels)
		int textAreaHeight = 50;  // Space reserved for loading text
		int availableHeight = SCREEN_HEIGHT - textAreaHeight;
		// int logoY = (availableHeight - LOGO_HEIGHT) / 2;  // Center in available area
		int logoY = ((availableHeight - LOGO_HEIGHT) / 2) + 12;

		// Draw the logo image centered, preserving transparency
		// Pixels with value 0x0000 (black) are treated as transparent and skipped
		for (int y = 0; y < LOGO_HEIGHT; y++) {
			for (int x = 0; x < LOGO_WIDTH; x++) {
				uint16_t pixel = pgm_read_word(&logo_image[y][x]);
				// Skip transparent pixels (black = 0x0000)
				if (pixel != 0x0000) {
					gfx->drawPixel(logoX + x, logoY + y, pixel);
				}
			}
		}

		// Show loading text below the logo
		gfx->setTextColor(WHITE);
		gfx->setTextSize(2);
		int16_t x1, y1;
		uint16_t w, h;
		String loadingText = "Loading...";
		gfx->getTextBounds(loadingText, 0, 0, &x1, &y1, &w, &h);
		int textX = (SCREEN_WIDTH - w) / 2;
		int textY = logoY + LOGO_HEIGHT + 20;  // Position below logo with spacing

		// Draw semi-transparent background for text
		gfx->fillRect(textX - 5, textY - 2, w + 10, h + 4, RGB565(0, 0, 0)); // Black with some transparency effect

		gfx->setCursor(textX, textY);
		gfx->print(loadingText);

		// COMMENTED OUT: Animation with delays was blocking firmware responsiveness
		// The firmware needs to respond to SimHub commands immediately
		// Just show static loading screen instead of animated dots
		// for (int i = 0; i < 3; i++) {
		// 	delay(400);
		// 	// Clear text area
		// 	gfx->fillRect(textX - 5, textY - 2, w + 10, h + 4, RGB565(0, 0, 0));

		// 	String dots = "Loading";
		// 	for (int j = 0; j <= i; j++) {
		// 		dots += ".";
		// 	}
		// 	gfx->getTextBounds(dots, 0, 0, &x1, &y1, &w, &h);
		// 	textX = (SCREEN_WIDTH - w) / 2;
		// 	gfx->setCursor(textX, textY);
		// 	gfx->print(dots);
		// }

		// COMMENTED OUT: Long delay was blocking serial communication
		// delay(800);
		Serial.println("Loading screen completed");
		loadingScreenShown = true;
	}
public:
	void showPopup(const String &msg, uint32_t durationMs = 2000) {
		popupMessage = msg;
		popupFromUart = true;
		popupFromUartUntil = millis() + durationMs;
	}

	void pageNextExternal() {
		buzzerBeep(80);
		nextPage();
	}

	void pagePrevExternal() {
		buzzerBeep(80);
		prevPage();
	}

	void adjustBacklight(int delta) {
		setBacklight(backlightLevel + delta);
	}

	void setBacklight(int level) {
		backlightLevel = level;
		if (backlightLevel < BACKLIGHT_MIN) backlightLevel = BACKLIGHT_MIN;
		if (backlightLevel > BACKLIGHT_MAX) backlightLevel = BACKLIGHT_MAX;
		applyBacklight();
	}

	uint8_t getBacklightPercent() {
		return (uint8_t)((backlightLevel * 100) / 255);
	}

	void adjustLedLuminance(int delta) {
		#ifdef INCLUDE_RGB_LEDS_NEOPIXELBUS
		int current = neoPixelBusGetLuminance();
		int next = current + delta;
		if (next < 1) next = 1;
		if (next > 255) next = 255;
		neoPixelBusSetLuminance((uint8_t)next);
		#endif
	}

	uint8_t getLedLuminance() {
		#ifdef INCLUDE_RGB_LEDS_NEOPIXELBUS
		return neoPixelBusGetLuminance();
		#else
		return 0;
		#endif
	}

	void setup() {
		// Initialize display for dashboard
		displayEnabled = true;

		// NOTE: Touch initialization is deferred to loop() after display is ready
		// This is because screenLog() needs gfx to be fully initialized

		if (displayEnabled) {
			// Create bus and display objects here to avoid "no free i80 bus slot" error
			// Creating them in setup() instead of globally ensures proper initialization order
			if (bus == nullptr) {
				Serial.println("Creating 8-bit parallel bus object...");
				bus = new Arduino_ESP32PAR8(
					TFT_RS,      // DC/RS pin (LCD_RS = GPIO 0)
					-1,          // CS pin (not used for 8080 interface)
					TFT_WR,      // WR pin (LCD_WR = GPIO 47)
					-1,          // RD pin (not used for 8080 interface)
					TFT_D0, TFT_D1, TFT_D2, TFT_D3, TFT_D4, TFT_D5, TFT_D6, TFT_D7  // 8 data pins
				);
			}

			if (gfx == nullptr && bus != nullptr) {
				Serial.println("Creating ST7796 display object...");
				// Rotation: 0=Portrait, 1=Landscape, 2=Portrait inverted, 3=Landscape inverted
				tft = new Arduino_ST7796(bus, TFT_RST, 1 /* rotation = landscape */, true /* IPS */);
			}

			// Initialize backlight first - GPIO 45 according to WT32-SC01 Plus documentation
		#ifdef TFT_BL
			if (TFT_BL >= 0 && TFT_BL < 48) {  // ESP32-S3 has GPIOs 0-48
				pinMode(TFT_BL, OUTPUT);
				digitalWrite(TFT_BL, LOW);  // Start with backlight off
				ledcSetup(BACKLIGHT_PWM_CHANNEL, BACKLIGHT_PWM_FREQ, BACKLIGHT_PWM_RES);
				ledcAttachPin(TFT_BL, BACKLIGHT_PWM_CHANNEL);
				backlightPwmReady = true;
				delay(10);
				Serial.print("Backlight pin configured: GPIO ");
				Serial.println(TFT_BL);
			}
		#endif

			// Initialize display with error handling
			if (tft != nullptr && bus != nullptr) {
				Serial.println("Initializing display (8-bit parallel interface)...");
				Serial.flush();

				// Small delay before initialization to ensure everything is ready
				delay(100);

				// Initialize 8-bit parallel interface
				// Note: Arduino_ESP32PAR8 uses I80 bus internally
				bool busOk = bus->begin();
				if (!busOk) {
					Serial.println("ERROR: Failed to initialize data bus!");
					Serial.println("This may indicate 'no free i80 bus slot' error");
					displayEnabled = false;
					return;
				}
				Serial.println("Data bus initialized successfully");
				Serial.flush();

				// Initialize display
				bool displayOk = tft->begin();
				if (!displayOk) {
					Serial.println("ERROR: Failed to initialize display!");
					displayEnabled = false;
					return;
				}
				Serial.println("Display controller initialized");
				Serial.flush();

				delay(300);  // Give display time to stabilize

				// Try to create canvas framebuffer in PSRAM for double-buffering
				#ifdef BOARD_HAS_PSRAM
				if (psramFound()) {
					Serial.printf("PSRAM found: %d bytes free\n", ESP.getFreePsram());
					canvas = new Arduino_Canvas(tft->width(), tft->height(), tft);
					if (canvas->begin()) {
						gfx = canvas;
						Serial.println("Canvas framebuffer created in PSRAM (double-buffering enabled)");
					} else {
						delete canvas;
						canvas = nullptr;
						gfx = tft;
						Serial.println("Canvas creation failed, using direct rendering");
					}
				} else {
					gfx = tft;
					Serial.println("No PSRAM available, using direct rendering");
				}
				#else
				gfx = tft;
				Serial.println("PSRAM not enabled, using direct rendering");
				#endif

				// Turn on backlight after display is ready
		#ifdef TFT_BL
				if (TFT_BL >= 0 && TFT_BL < 48) {
					applyBacklight();
					delay(100);
					Serial.println("Backlight enabled");
				}
		#endif

				gfx->fillScreen(BLACK);
				delay(100);

				// Show loading screen with logo
				showLoadingScreen();

				Serial.println("Display initialized successfully!");
				Serial.println("Loading screen displayed");
			} else {
				Serial.println("ERROR: Display or bus object is null!");
				displayEnabled = false;
			}
		}


	}

	void initializeTouch() {
		Serial.print("\n");
		Serial.println("========== TOUCH INITIALIZATION START ==========");
		Serial.print("Millis: ");
		Serial.println(millis());
		Serial.flush();
		delay(100);

		screenLog("TOUCH: Initializing...");

		// Initialize I2C for FT6336U touch controller
		Serial.println("Step 1: Setting up I2C...");
		Serial.flush();
		delay(50);

		Wire.begin(TOUCH_SDA, TOUCH_SCL);

		Serial.print("Step 2: I2C begin() called with SDA=");
		Serial.print(TOUCH_SDA);
		Serial.print(" SCL=");
		Serial.println(TOUCH_SCL);
		Serial.flush();
		delay(50);

		screenLog("TOUCH: I2C begin SDA=" + String(TOUCH_SDA) + " SCL=" + String(TOUCH_SCL));

		Wire.setClock(400000);
		Serial.println("Step 3: I2C clock set to 400kHz");
		Serial.flush();
		delay(200);

		screenLog("TOUCH: I2C 400kHz configured");

		// Scan for FT6336U at address 0x38
		Serial.print("Step 4: Scanning I2C for FT6336U at address 0x");
		Serial.println(TOUCH_ADDRESS, HEX);
		Serial.flush();
		delay(50);

		screenLog("TOUCH: Scanning for FT6336U at 0x38...");

		Wire.beginTransmission(TOUCH_ADDRESS);
		uint8_t error = Wire.endTransmission();

		Serial.print("Step 5: I2C transmission result: ");
		Serial.println(error);
		Serial.flush();
		delay(50);

		if (error == 0) {
			touchInitialized = true;
			Serial.println("SUCCESS: FT6336U FOUND at 0x38!");
			Serial.flush();
			delay(100);

			screenLog("TOUCH: SUCCESS - FT6336U found!");
		} else {
			Serial.println("ERROR: FT6336U NOT FOUND at address 0x38");
			Serial.print("I2C Error code: ");
			Serial.println(error);
			Serial.flush();
			delay(100);

			screenLog("TOUCH: ERROR - not found at 0x38 (code " + String(error) + ")");

			// Try to scan all I2C addresses to find what's there
			Serial.println("Scanning ALL I2C addresses 0x01-0x7E...");
			Serial.flush();
			delay(50);

			screenLog("TOUCH: Scanning all addresses...");

			bool found_any = false;
			String found_devices = "";
			for (uint8_t i = 1; i < 127; i++) {
				Wire.beginTransmission(i);
				if (Wire.endTransmission() == 0) {
					Serial.print("  Found device at 0x");
					if (i < 0x10) Serial.print("0");
					Serial.println(i, HEX);
					Serial.flush();

					if (found_devices.length() > 0) found_devices += ", ";
					found_devices += "0x";
					if (i < 0x10) found_devices += "0";
					found_devices += String(i, HEX);

					found_any = true;
				}
			}

			if (found_any) {
				screenLog("TOUCH: Found devices at: " + found_devices);
			} else {
				screenLog("TOUCH: No I2C devices found!");
			}
			Serial.flush();
			delay(100);

			touchInitialized = false;
		}
		Serial.println("========== TOUCH INITIALIZATION END ==========\n");
		Serial.flush();
		delay(100);

		screenLog("TOUCH: Init complete");
	}

	// Called when new data is coming from computer
	void read() {
		if (!hasReceivedData) {
			hasReceivedData = true;
			if (displayEnabled && gfx != nullptr) {
				gfx->fillScreen(BLACK);
			}
			// Debug: First data received
			debugLog("[SHCustomProtocol.read()] First data packet received from SimHub!");
		}

		// BLOCO 1: Telemetria Básica (índices 0-4)
		speed = String(FlowSerialReadStringUntil(';').toInt());
		gear = FlowSerialReadStringUntil(';');
		rpmPercent = FlowSerialReadStringUntil(';').toInt();
		rpmRedLineSetting = FlowSerialReadStringUntil(';').toInt();
		String rpmsStr = FlowSerialReadStringUntil(';');
		currentRpms = rpmsStr.toInt();  // [4] Armazena RPM atual

		// BLOCO 2: Cronometragem (índices 5-10)
		currentLapTime = FlowSerialReadStringUntil(';');
		lastLapTime = FlowSerialReadStringUntil(';');
		bestLapTime = FlowSerialReadStringUntil(';');
		sessionBestLiveDeltaSeconds = FlowSerialReadStringUntil(';');
		sessionBestLiveDeltaProgressSeconds = FlowSerialReadStringUntil(';');
		String lapInvalidatedStr = FlowSerialReadStringUntil(';');
		lapInvalidated = lapInvalidatedStr;

		// BLOCO 3: Física e Pneus (índices 11-24)
		// Pressão dos pneus
		tyrePressureFrontLeft = FlowSerialReadStringUntil(';');
		tyrePressureFrontRight = FlowSerialReadStringUntil(';');
		tyrePressureRearLeft = FlowSerialReadStringUntil(';');
		tyrePressureRearRight = FlowSerialReadStringUntil(';');
		// Temperatura dos pneus
		tyreTemperatureFrontLeft = FlowSerialReadStringUntil(';');
		tyreTemperatureFrontRight = FlowSerialReadStringUntil(';');
		tyreTemperatureRearLeft = FlowSerialReadStringUntil(';');
		tyreTemperatureRearRight = FlowSerialReadStringUntil(';');
		// Temperatura dos freios
		brakeTemperatureFrontLeft = FlowSerialReadStringUntil(';');
		brakeTemperatureFrontRight = FlowSerialReadStringUntil(';');
		brakeTemperatureRearLeft = FlowSerialReadStringUntil(';');
		brakeTemperatureRearRight = FlowSerialReadStringUntil(';');
		// Motor
		oilTemperature = FlowSerialReadStringUntil(';');
		waterTemperature = FlowSerialReadStringUntil(';');

		// BLOCO 4: Eletrônica (índices 25-31)
		tcLevel = FlowSerialReadStringUntil(';');
		tcActive = FlowSerialReadStringUntil(';');
		absLevel = FlowSerialReadStringUntil(';');
		absActive = FlowSerialReadStringUntil(';');
		tcCut = FlowSerialReadStringUntil(';');  // [29] TCCut (ex: 0-12 in ACC)
		brakeBias = FlowSerialReadStringUntil(';');    // [30] BrakeBias (e.g., 68.0)
		brake = FlowSerialReadStringUntil(';');        // [31] Brake pedal (0-100)

		// BLOCO 5: Estratégia (índices 32-41)
		position = FlowSerialReadStringUntil(';');
		opponentsCount = FlowSerialReadStringUntil(';');
		driverAheadGap = FlowSerialReadStringUntil(';');
		driverBehindGap = FlowSerialReadStringUntil(';');
		fuelRemainingLaps = FlowSerialReadStringUntil(';');
		fuelLitersPerLap = FlowSerialReadStringUntil(';');
		sessionTimeLeft = FlowSerialReadStringUntil(';');
		currentFlag = FlowSerialReadStringUntil(';');
		currentFlag.trim();
		currentPenalties = FlowSerialReadStringUntil(';');
		cutTrackWarnings = FlowSerialReadStringUntil(';');

		// BLOCO 6: Mensagens e Alertas (índices 42-43)
		alertMessage = FlowSerialReadStringUntil(';');
		popupMessage = FlowSerialReadStringUntil(';');
		alertMessage.trim();
		popupMessage.trim();

		// BLOCO 7: Dados para Arduino LEDs (índices 44-47)
		rpmPercent2 = FlowSerialReadStringUntil(';');
		spotterLeft = FlowSerialReadStringUntil(';');
		spotterRight = FlowSerialReadStringUntil(';');
		absActive2 = FlowSerialReadStringUntil(';');

		// BLOCO 8: Desgaste e Ambiente (índices 48-61)
		tyreWearFrontLeft = FlowSerialReadStringUntil(';');
		tyreWearFrontRight = FlowSerialReadStringUntil(';');
		tyreWearRearLeft = FlowSerialReadStringUntil(';');
		tyreWearRearRight = FlowSerialReadStringUntil(';');
		sector1Time = FlowSerialReadStringUntil(';');
		sector2Time = FlowSerialReadStringUntil(';');
		sector3Time = FlowSerialReadStringUntil(';');
		airTemperature = FlowSerialReadStringUntil(';');
		roadTemperature = FlowSerialReadStringUntil(';');
		shiftLightTrigger = FlowSerialReadStringUntil(';');
		drsAvailable = FlowSerialReadStringUntil(';');
		drsActive = FlowSerialReadStringUntil(';');
		kersLevel = FlowSerialReadStringUntil(';');
		turboBoost = FlowSerialReadStringUntil(';');  // [61]

		// BLOCO 9: Dados 499P (índices 62-67)
		ersDeployMode = FlowSerialReadStringUntil(';');
		arbFront = FlowSerialReadStringUntil(';');
		arbRear = FlowSerialReadStringUntil(';');
		brkMigration = FlowSerialReadStringUntil(';');
		headWind = FlowSerialReadStringUntil(';');
		rearBrakeBias = FlowSerialReadStringUntil(';');  // [67]

		// BLOCO 10: Track Map (índices 68-71)
		trackPositionPercent = FlowSerialReadStringUntil(';');
		aheadTrackPosition = FlowSerialReadStringUntil(';');
		behindTrackPosition = FlowSerialReadStringUntil(';');
		trackId = FlowSerialReadStringUntil(';');  // Último campo (índice 71)
		trackId.trim();

		// Validate brakeBias (should be between 0-100)
		brakeBias.trim();
		float brakeBiasVal2 = brakeBias.toFloat();
		if (brakeBiasVal2 < 0 || brakeBiasVal2 > 100 || brakeBias.length() == 0) {
			brakeBias = "60.0";  // Default to 60.0 if invalid
		}
	}

	// Called once per arduino loop, timing can't be predicted,
	// but it's called between each command sent to the arduino
	void loop() {
		// Limpa pop-up vindo do UART após o tempo definido
		if (popupFromUart && popupFromUartUntil > 0 && millis() > popupFromUartUntil) {
			popupMessage = "";
			popupFromUart = false;
			popupFromUartUntil = 0;
		}
		// Initialize touch right after loading screen is shown (before SimHub data arrives)
		if (!touchInitAttempted && loadingScreenShown) {
			touchInitAttempted = true;
			initializeTouch();
		}

		// Check for touch input to change pages
		if (touchInitialized && hasReceivedData) {
			TouchPoint touch = readTouch();
			// Rising-edge detection: only act on new touch (finger down), not held/phantom touch.
			// A stuck phantom touch that never changes state fires exactly once, then stops.
			bool isNewTouch = touch.touched && !prevTouched;
			prevTouched = touch.touched;
			if (isNewTouch && (millis() - lastTouchTime) > TOUCH_DEBOUNCE_MS) {
				lastTouchTime = millis();

				// Display is landscape (rotation=1). The FT6336U reports in portrait coordinates:
				//   touch.x = portrait X axis → maps to display Y (0-319)
				//   touch.y = portrait Y axis → maps to display X (0-479)
				// So left/right split must use touch.y, not touch.x.
				Serial.printf("[TOUCH] x=%d y=%d (display_x≈touch.y, threshold=%d)\n",
					touch.x, touch.y, SCREEN_WIDTH / 2);

				if (touch.y < SCREEN_WIDTH / 2) {
					// Left half of display → previous page
					pagePrevExternal();  // includes buzzer + fillScreen + resetDrawCache
				} else {
					// Right half of display → next page
					pageNextExternal();  // includes buzzer + fillScreen + resetDrawCache
				}
			}
		}

		// Detect page changes (for cases other than touch)
		if (currentPage != lastPage) {
			resetDrawCache();
			lastPage = currentPage;
		}

		// DRS rising-edge detection: beep once when DRS becomes available
		if (prevDrsAvailable == "0" && drsAvailable == "1") {
			buzzerBeep(150);
		}
		prevDrsAvailable = drsAvailable;

		if (!hasReceivedData) {
			// Show loading animation on LEDs while waiting for SimHub
			#ifdef INCLUDE_RGB_LEDS_NEOPIXELBUS
			updateLoadingAnimation();
			#endif
			// Still render UART popups from ButtonBox even before SimHub connects
			if (popupFromUart && popupMessage.length() > 0) {
				drawAlert();
			}
			return;
		}

		// Check if we need full redraw after alert expired
		if (needsFullRedraw) {
			gfx->fillScreen(BLACK);
			resetDrawCache();  // Clear all caches
			needsFullRedraw = false;
		}

		// Draw page-specific content
		switch (currentPage) {
			case PAGE_RACE:
				drawRacePageContent();
				break;
			case PAGE_TIMING:
				drawTimingPageContent();
				break;
			case PAGE_TELEMETRY:
				drawTelemetryPageContent();
				break;
			case PAGE_ADVANCED:
				drawAdvancedTelemetryPage();
				break;
			case PAGE_RELATIVE:
				drawRelativePageContent();
				break;
			case PAGE_LAPS:
				drawLapsPageContent();
				break;
			case PAGE_MAP:
				drawMapPageContent();
				break;
			case PAGE_499P:
				draw499PPageContent();
				break;
		}

		// Draw alerts (flags, penalties, etc.) on top of everything
		drawAlert();

		// Draw page indicator at bottom
		drawPageIndicator();

		// Update LED strip with current telemetry data
		#ifdef INCLUDE_RGB_LEDS_NEOPIXELBUS
		updateCustomLEDs(
			rpmPercent,
			rpmRedLineSetting,
			currentFlag,
			spotterLeft,
			spotterRight,
			drsAvailable,
			drsActive,
			alertMessage,
			shiftLightTrigger == "1",
			tcActive,
			absActive
		);
		#endif

		// Flush canvas to display (double-buffered rendering)
		if (canvas) canvas->flush();
	}

	void drawPageIndicator() {
		// Draw small page indicator dots at bottom center (8 pages)
		// Positioned in dedicated padding area at bottom
		int dotRadius = 2;
		int dotSpacing = 8;
		int totalWidth = (8 - 1) * dotSpacing + (dotRadius * 2);
		int startX = (SCREEN_WIDTH - totalWidth) / 2;  // Center horizontally
		int startY = SCREEN_HEIGHT + 41;  // 8px from bottom margin (colado na borda)

		for (int i = 0; i < 8; i++) {
			uint16_t color = (i == currentPage) ? WHITE : RGB565(100, 100, 100);
			gfx->fillCircle(startX + (i * dotSpacing), startY, dotRadius, color);
		}
	}

	void drawStatusBar() {
		if (!gfx) return;

		// Clear the entire top row to avoid any residual text/artifacts
		// gfx->fillRect(0, ROW[0], SCREEN_WIDTH, CELL_HEIGHT, BLACK);

		// Build list of items to show (max 5)
		struct StatusItem {
			String value;
			String label;
			String cacheKey;
			uint16_t color;
		};
		StatusItem items[5];
		int itemCount = 0;

		// 1. Position (always first)
		items[itemCount++] = {
			position,
			"POS",
			"statusPos",
			YELLOW
		};

		// 2. Fuel (always second)
		float fuelLaps = fuelRemainingLaps.toFloat();
		String fuelDisplay = String(fuelLaps, 1) + "L";  // Format with 1 decimal place + " L"
		items[itemCount++] = {
			fuelDisplay,
			"FUEL",
			"statusFuel",
			fuelLaps < 3.0f ? RED : WHITE
		};

		// 3. KERS if hybrid (priority)
		int kersVal = kersLevel.toInt();
		bool hasKers = (kersVal > 0);
		if (hasKers && itemCount < 5) {
			items[itemCount++] = {
				kersLevel,
				"KERS",
				"statusKers",
				kersVal < 20 ? RED : GREEN
			};
		}

		// 4. Oil if critical OR space available
		int oilTempVal = oilTemperature.toInt();
		bool oilCritical = (oilTempVal > 110);
		if (itemCount < 5 && (oilCritical || !hasKers)) {
			items[itemCount++] = {
				oilTemperature,
				"OIL",
				"statusOil",
				oilCritical ? RED : ORANGE
			};
		}

		// 5. Fill remaining slots (turbo > water > gap)
		if (itemCount < 5) {
			// Try turbo first - only if string is not empty AND value is meaningful
			String turboTrimmed = turboBoost;
			turboTrimmed.trim();
			float turboVal = turboTrimmed.toFloat();
			if (turboTrimmed.length() > 0 && turboVal > 0.1) {
				items[itemCount++] = {
					turboBoost,
					"TURBO",
					"statusTurbo",
					CYAN
				};
			}
			// Then water (only if turbo not shown)
			else if (!oilCritical) {
				int waterTempVal = waterTemperature.toInt();
				if (waterTempVal > 0) {
					items[itemCount++] = {
						waterTemperature,
						"WATER",
						"statusWater",
						waterTempVal > 100 ? RED : CYAN
					};
				}
			}
		}

		// 6. Last slot: gap if space
		if (itemCount < 5 && driverAheadGap.length() > 0 && driverAheadGap != "--") {
			items[itemCount++] = {
				driverAheadGap,
				"GAP",
				"statusGap",
				WHITE
			};
		}

		// Clear unused columns if itemCount < 5
		// for (int i = itemCount; i < 5; i++) {
		// 	gfx->fillRect(COL[i], ROW[0], CELL_WIDTH, CELL_HEIGHT, BLACK);
		// }

		// Draw only collected items (itemCount is accurate)
		for (int i = 0; i < itemCount; i++) {
			drawCell(COL[i], ROW[0], items[i].value, items[i].cacheKey, items[i].label, "center", items[i].color);
		}
	}

	void drawRacePageContent() {
		// Reset cursor and text state to prevent drawing artifacts
		gfx->setCursor(0, 0);
		gfx->setTextColor(WHITE, BLACK);

		// Original dashboard content - LAYOUT CLÁSSICO
		// drawRpmMeter(0, 0, SCREEN_WIDTH, CELL_HEIGHT);
		drawStatusBar();

		// this takes 2 cells in height, hence CELL_HEIGHT is the half point
		drawGear(COL[2] + HALF_CELL_WIDTH, ROW[1] + CELL_HEIGHT);

		// First+Second Column (Lap times)
		drawCell(COL[0], ROW[1], bestLapTime, "bestLapTime", "Best Lap", "left");
		drawCell(COL[0], ROW[2], lastLapTime, "lastLapTime", "Last Lap", "left");
		drawCell(COL[0], ROW[3], currentLapTime, "currenLapTime", "Current Lap", "left", lapInvalidated == "True" ? RED : WHITE);

		// Third Column (speed) - ROW[3] para dar mais espaço ao gear
		drawCell(COL[2], ROW[3], speed, "speed", "Speed", "center");

		// Fourth+Fifth Column (delta)
		drawCell(SCREEN_WIDTH, ROW[1], sessionBestLiveDeltaSeconds, "sessionBestLiveDeltaSeconds", "Delta", "right", sessionBestLiveDeltaSeconds.indexOf('-') >= 0 ? GREEN : RED);
		drawCell(SCREEN_WIDTH, ROW[2], sessionBestLiveDeltaProgressSeconds, "sessionBestLiveDeltaProgressSeconds", "Delta P", "right", sessionBestLiveDeltaProgressSeconds.indexOf('-') >= 0 ? GREEN : RED);

		// Bottom row (TC, ABS, BB)
		// If TCCut is active (non-zero), show CUT indicator; otherwise show TC level
		if (tcCut != "0") {
			drawCell(COL[0], ROW[4], String("CUT"), "tcCut", "TC", "center", YELLOW);
		} else {
			drawCell(COL[0], ROW[4], tcLevel, "tcLevel", "TC", "center", YELLOW);
		}
		drawCell(COL[1], ROW[4], absLevel, "absLevel", "ABS", "center", BLUE);
		drawCell(COL[2], ROW[4], brakeBias, "brakeBias", "BB", "center", MAGENTA);

		// Tyre pressure
		drawCell(COL[3], ROW[3], tyrePressureFrontLeft, "tyrePressureFrontLeft", "FL", "center", CYAN);
		drawCell(COL[4], ROW[3], tyrePressureFrontRight, "tyrePressureFrontRight", "FR", "center", CYAN);
		drawCell(COL[3], ROW[4], tyrePressureRearLeft, "tyrePressureRearLeft", "RL", "center", CYAN);
		drawCell(COL[4], ROW[4], tyrePressureRearRight, "tyrePressureRearRight", "RR", "center", CYAN);
	}

	void drawTimingPageContent() {
		// ── Palette ──
		const uint16_t GOLD     = RGB565(220, 172, 0);
		const uint16_t GOLD_DIM = RGB565(120, 96, 0);
		const uint16_t TEAL     = RGB565(0, 200, 220);
		const uint16_t TEAL_DIM = RGB565(0, 100, 115);
		const uint16_t LIME     = RGB565(60, 230, 80);
		const uint16_t LIME_DIM = RGB565(30, 120, 45);
		const uint16_t BG_HDR   = RGB565(6, 8, 22);
		const uint16_t BG_C1    = RGB565(16, 13, 4);
		const uint16_t BG_C2    = RGB565(4, 14, 18);
		const uint16_t BG_C3    = RGB565(4, 16, 6);
		const uint16_t SEP      = RGB565(22, 20, 30);

		// ── Layout constants (physical height = SCREEN_HEIGHT+48 = 320) ──
		// Header 0..45 (46px) | C1 46..136 (90px) | C2 137..227 (90px) | C3 228..319 (91px)
		const int FH   = SCREEN_HEIGHT + 48; // 320
		const int C1_Y = 46,  C1_H = 90;
		const int C2_Y = 137, C2_H = 90;
		const int C3_Y = 228, C3_H = FH - C3_Y; // 91

		// ── Static frame: draw only once per page switch ──
		if (!timingFrameDrawn) {
			gfx->fillScreen(BLACK);

			// Header
			gfx->fillRect(0, 0, SCREEN_WIDTH, 46, BG_HDR);
			gfx->fillRect(0, 44, SCREEN_WIDTH, 2, GOLD);
			gfx->setTextColor(WHITE); gfx->setTextSize(3);
			gfx->setCursor(14, 9); gfx->print("TIMING");

			// Card 1 — BEST LAP
			gfx->fillRect(0, C1_Y, SCREEN_WIDTH, C1_H, BG_C1);
			gfx->fillRect(0, C1_Y, 5, C1_H, GOLD);
			gfx->setTextColor(GOLD_DIM); gfx->setTextSize(1);
			gfx->setCursor(14, C1_Y + 5); gfx->print("BEST LAP");
			gfx->fillRect(0, C1_Y + C1_H - 1, SCREEN_WIDTH, 2, SEP);

			// Card 2 — LAST LAP
			gfx->fillRect(0, C2_Y, SCREEN_WIDTH, C2_H, BG_C2);
			gfx->fillRect(0, C2_Y, 5, C2_H, TEAL);
			gfx->setTextColor(TEAL_DIM); gfx->setTextSize(1);
			gfx->setCursor(14, C2_Y + 5); gfx->print("LAST LAP");
			gfx->fillRect(0, C2_Y + C2_H - 1, SCREEN_WIDTH, 2, SEP);

			// Card 3 — CURRENT LAP
			gfx->fillRect(0, C3_Y, SCREEN_WIDTH, C3_H, BG_C3);
			gfx->fillRect(0, C3_Y, 5, C3_H, LIME);
			gfx->setTextColor(LIME_DIM); gfx->setTextSize(1);
			gfx->setCursor(14, C3_Y + 5); gfx->print("CURRENT LAP");

			timingFrameDrawn = true;
		}

		// ── Delta + POS (header right side) ──
		bool deltaIsNeg = sessionBestLiveDeltaSeconds.startsWith("-");
		uint16_t deltaCol = deltaIsNeg ? LIME : RED;
		if (prevData["t_delta"] != sessionBestLiveDeltaSeconds || prevData["t_delta_c"] != (deltaIsNeg ? "n" : "p")) {
			gfx->fillRect(SCREEN_WIDTH - 200, 2, 198, 42, BG_HDR);
			if (position.length() > 0 && position != "0") {
				gfx->setTextColor(RGB565(140, 140, 160)); gfx->setTextSize(1);
				gfx->setCursor(SCREEN_WIDTH - 52, 6); gfx->print("POS");
				gfx->setTextColor(WHITE); gfx->setTextSize(2);
				gfx->setCursor(SCREEN_WIDTH - 54, 18); gfx->print("P"); gfx->print(position);
			}
			int dxLabel = (position.length() > 0 && position != "0") ? SCREEN_WIDTH - 185 : SCREEN_WIDTH - 110;
			gfx->setTextColor(RGB565(120, 120, 140)); gfx->setTextSize(1);
			gfx->setCursor(dxLabel, 6); gfx->print("DELTA");
			gfx->setTextColor(deltaCol); gfx->setTextSize(2);
			gfx->setCursor(dxLabel - 4, 18); gfx->print(sessionBestLiveDeltaSeconds);
			prevData["t_delta"] = sessionBestLiveDeltaSeconds;
			prevData["t_delta_c"] = deltaIsNeg ? "n" : "p";
			prevData["t_pos"] = position;
		}

		// ── Best lap ── textSize 5 = 40px, centred vertically in 90px card ──
		if (prevData["t_best"] != bestLapTime) {
			gfx->fillRect(6, C1_Y + 16, SCREEN_WIDTH - 7, 64, BG_C1);
			gfx->setTextColor(GOLD); gfx->setTextSize(5);
			gfx->setCursor(14, C1_Y + 22); gfx->print(bestLapTime);
			prevData["t_best"] = bestLapTime;
		}

		// ── Last lap ── textSize 5
		if (prevData["t_last"] != lastLapTime) {
			gfx->fillRect(6, C2_Y + 16, SCREEN_WIDTH - 7, 64, BG_C2);
			gfx->setTextColor(TEAL); gfx->setTextSize(5);
			gfx->setCursor(14, C2_Y + 22); gfx->print(lastLapTime);
			prevData["t_last"] = lastLapTime;
		}

		// ── Current lap ── textSize 5
		if (prevData["t_cur"] != currentLapTime) {
			gfx->fillRect(6, C3_Y + 16, SCREEN_WIDTH - 7, 64, BG_C3);
			gfx->setTextColor(LIME); gfx->setTextSize(5);
			gfx->setCursor(14, C3_Y + 22); gfx->print(currentLapTime);
			prevData["t_cur"] = currentLapTime;
		}
	}

	void drawTelemetryPageContent() {
		const uint16_t ORNG   = RGB565(230, 130,  0);
		const uint16_t ORNG_D = RGB565( 80,  45,  0);
		const uint16_t YLW    = RGB565(230, 210,  0);
		const uint16_t YLW_D  = RGB565( 90,  82,  0);
		const uint16_t BLU    = RGB565( 60, 140, 240);
		const uint16_t BLU_D  = RGB565( 18,  45,  90);
		const uint16_t RD     = RGB565(220,  50,  50);
		const uint16_t RD_D   = RGB565( 80,  15,  15);
		const uint16_t TCL    = RGB565(  0, 200, 210);
		const uint16_t TCL_D  = RGB565(  0,  70,  80);
		const uint16_t BG_HDR = RGB565(  6,   8,  22);
		const uint16_t BG_SPD = RGB565( 18,   9,   0);
		const uint16_t BG_TC  = RGB565( 18,  14,   0);
		const uint16_t BG_ABS = RGB565(  5,  10,  36);
		const uint16_t BG_BRK = RGB565( 20,   5,   5);
		const uint16_t BG_TYR = RGB565(  0,  10,  12);
		const uint16_t SEP    = RGB565( 22,  20,  30);

		if (!telemFrameDrawn) {
			gfx->fillScreen(BLACK);

			// Header
			gfx->fillRect(0, 0, SCREEN_WIDTH, 46, BG_HDR);
			gfx->fillRect(0, 44, SCREEN_WIDTH, 2, ORNG);
			gfx->setTextColor(WHITE); gfx->setTextSize(3);
			gfx->setCursor(14, 9); gfx->print("TELEMETRY");
			gfx->setTextColor(RGB565(120, 120, 140)); gfx->setTextSize(1);
			gfx->setCursor(SCREEN_WIDTH - 92, 8); gfx->print("GEAR");

			// Layout: FH=320 | Header 0..45 | Speed/TC/ABS 46..134 | Brake 135..188 | Tyres 189..319
			const int FH=320, SPD_Y=46, SPD_H=88, BRK_Y=135, BRK_H=53, TYR_Y=189;
			const int TC_X=302, TC_H=43; // TC occupies upper half of SPD section, ABS lower

			// Card SPEED (x=0..TC_X-2, y=SPD_Y..SPD_Y+SPD_H)
			gfx->fillRect(0, SPD_Y, TC_X - 2, SPD_H, BG_SPD);
			gfx->fillRect(0, SPD_Y, 5, SPD_H, ORNG);
			gfx->setTextColor(ORNG_D); gfx->setTextSize(1);
			gfx->setCursor(14, SPD_Y + 5); gfx->print("SPEED");

			// Card TC (x=TC_X..479, y=SPD_Y..SPD_Y+TC_H)
			gfx->fillRect(TC_X, SPD_Y, SCREEN_WIDTH - TC_X, TC_H, BG_TC);
			gfx->fillRect(TC_X, SPD_Y, 5, TC_H, YLW);
			gfx->setTextColor(YLW_D); gfx->setTextSize(1);
			gfx->setCursor(TC_X + 14, SPD_Y + 5); gfx->print("TC");

			// Card ABS (x=TC_X..479, y=SPD_Y+TC_H+1..SPD_Y+SPD_H)
			int ABS_Y = SPD_Y + TC_H + 1;
			gfx->fillRect(TC_X, ABS_Y, SCREEN_WIDTH - TC_X, SPD_H - TC_H - 1, BG_ABS);
			gfx->fillRect(TC_X, ABS_Y, 5, SPD_H - TC_H - 1, BLU);
			gfx->setTextColor(BLU_D); gfx->setTextSize(1);
			gfx->setCursor(TC_X + 14, ABS_Y + 5); gfx->print("ABS");

			gfx->drawLine(TC_X - 1, SPD_Y, TC_X - 1, SPD_Y + SPD_H, SEP);
			gfx->fillRect(0, SPD_Y + SPD_H, SCREEN_WIDTH, 1, SEP);

			// Card BRAKE (full width, y=BRK_Y..BRK_Y+BRK_H)
			gfx->fillRect(0, BRK_Y, SCREEN_WIDTH, BRK_H, BG_BRK);
			gfx->fillRect(0, BRK_Y, 5, BRK_H, RD);
			gfx->setTextColor(RD_D); gfx->setTextSize(1);
			gfx->setCursor(14, BRK_Y + 5); gfx->print("BRAKE");

			gfx->fillRect(0, BRK_Y + BRK_H, SCREEN_WIDTH, 1, SEP);

			// 4 tyre columns (y=TYR_Y..319, each 120px wide)
			const int TYR_H_LOCAL = FH - TYR_Y;
			const char* tyreLabels[4] = {"FL","FR","RL","RR"};
			for (uint8_t c = 0; c < 4; c++) {
				int cx = c * 120;
				gfx->fillRect(cx, TYR_Y, 119, TYR_H_LOCAL, BG_TYR);
				gfx->fillRect(cx, TYR_Y, 5, TYR_H_LOCAL, TCL);
				if (c > 0) gfx->drawLine(cx, TYR_Y, cx, FH - 1, SEP);
				gfx->setTextColor(TCL_D); gfx->setTextSize(1);
				gfx->setCursor(14 + cx, TYR_Y + 5); gfx->print(tyreLabels[c]);
				gfx->setCursor(14 + cx, TYR_Y + 68); gfx->print("T:");
			}
			telemFrameDrawn = true;
		}

		// Layout constants (delta renders)
		const int SPD_Y=46, SPD_H=88, BRK_Y=135, BRK_H=53, TYR_Y=189;
		const int TC_X=302, TC_H=43;
		const int ABS_Y = SPD_Y + TC_H + 1;

		// Gear (header)
		if (prevData["v_gr"] != gear) {
			gfx->fillRect(SCREEN_WIDTH - 90, 18, 84, 26, BG_HDR);
			gfx->setTextColor(WHITE); gfx->setTextSize(3);
			gfx->setCursor(SCREEN_WIDTH - 88, 18); gfx->print(gear);
			prevData["v_gr"] = gear;
		}
		// Speed
		if (prevData["v_sp"] != speed) {
			gfx->fillRect(6, SPD_Y + 22, TC_X - 8, 56, BG_SPD);
			gfx->setTextColor(ORNG); gfx->setTextSize(6);
			gfx->setCursor(14, SPD_Y + 22); gfx->print(speed);
			prevData["v_sp"] = speed;
		}
		// TC
		if (prevData["v_tc"] != tcLevel) {
			gfx->fillRect(TC_X + 6, SPD_Y + 14, SCREEN_WIDTH - TC_X - 8, 22, BG_TC);
			gfx->setTextColor(tcActive == "1" ? WHITE : YLW); gfx->setTextSize(3);
			gfx->setCursor(TC_X + 14, SPD_Y + 14); gfx->print(tcLevel);
			prevData["v_tc"] = tcLevel;
		}
		// ABS
		if (prevData["v_ab"] != absLevel) {
			gfx->fillRect(TC_X + 6, ABS_Y + 14, SCREEN_WIDTH - TC_X - 8, 22, BG_ABS);
			gfx->setTextColor(absActive == "1" ? WHITE : BLU); gfx->setTextSize(3);
			gfx->setCursor(TC_X + 14, ABS_Y + 14); gfx->print(absLevel);
			prevData["v_ab"] = absLevel;
		}
		// Brake
		String brakeStr = brake + "%";
		if (prevData["v_bk"] != brakeStr) {
			gfx->fillRect(6, BRK_Y + 16, SCREEN_WIDTH - 7, 32, BG_BRK);
			gfx->setTextColor(RD); gfx->setTextSize(4);
			gfx->setCursor(14, BRK_Y + 16); gfx->print(brakeStr);
			prevData["v_bk"] = brakeStr;
		}
		// Tyres — pressure + temperature per column
		String tyreP[4] = {tyrePressureFrontLeft, tyrePressureFrontRight, tyrePressureRearLeft, tyrePressureRearRight};
		String tyreT[4] = {tyreTemperatureFrontLeft, tyreTemperatureFrontRight, tyreTemperatureRearLeft, tyreTemperatureRearRight};
		const char* kP[4] = {"v_tpfl","v_tpfr","v_tprl","v_tprr"};
		const char* kT[4] = {"v_ttfl","v_ttfr","v_ttrl","v_ttrr"};
		for (uint8_t c = 0; c < 4; c++) {
			int cx = c * 120;
			if (prevData[kP[c]] != tyreP[c]) {
				gfx->fillRect(6 + cx, TYR_Y + 18, 112, 24, BG_TYR);
				gfx->setTextColor(TCL); gfx->setTextSize(3);
				gfx->setCursor(14 + cx, TYR_Y + 18); gfx->print(tyreP[c]);
				prevData[kP[c]] = tyreP[c];
			}
			if (prevData[kT[c]] != tyreT[c]) {
				gfx->fillRect(6 + cx, TYR_Y + 80, 112, 24, BG_TYR);
				gfx->setTextColor(RGB565(100, 180, 200)); gfx->setTextSize(3);
				gfx->setCursor(14 + cx, TYR_Y + 80); gfx->print(tyreT[c]);
				prevData[kT[c]] = tyreT[c];
			}
		}
	}

	void drawAdvancedTelemetryPage() {
		const uint16_t WHT    = WHITE;
		const uint16_t WHT_D  = RGB565( 70,  70,  75);
		const uint16_t BLU    = RGB565( 60, 140, 240);
		const uint16_t BLU_D  = RGB565( 18,  45,  90);
		const uint16_t RD     = RGB565(220,  50,  50);
		const uint16_t ORNG   = RGB565(230, 140,   0);
		const uint16_t ORNG_D = RGB565( 90,  55,   0);
		const uint16_t GRN    = RGB565( 60, 220,  60);
		const uint16_t GRN_D  = RGB565( 20,  80,  20);
		const uint16_t MGT    = RGB565(200,   0, 200);
		const uint16_t MGT_D  = RGB565( 70,   0,  70);
		const uint16_t BG_HDR = RGB565(  6,   8,  22);
		const uint16_t BG_W   = RGB565(  8,   8,  10);
		const uint16_t BG_TW  = RGB565(  8,   5,   0);
		const uint16_t BG_BLU = RGB565(  4,   8,  20);
		const uint16_t BG_RD  = RGB565( 16,   4,   4);
		const uint16_t BG_GRN = RGB565(  4,  16,   4);
		const uint16_t BG_O   = RGB565( 18,   9,   0);
		const uint16_t BG_MGT = RGB565( 14,   0,  14);
		const uint16_t SEP    = RGB565( 22,  20,  30);

		if (!advFrameDrawn) {
			gfx->fillScreen(BLACK);
			gfx->fillRect(0, 0, SCREEN_WIDTH, 46, BG_HDR);
			gfx->fillRect(0, 44, SCREEN_WIDTH, 2, MGT);
			gfx->setTextColor(WHITE); gfx->setTextSize(3);
			gfx->setCursor(14, 9); gfx->print("ADVANCED");

			// Layout: FH=320 | Header 0..45 | OIL/WATER 46..118 | TYRE WEAR 119..181 | AIR/ROAD/DRS/TURBO 182..244 | KERS 245..319
			const int FH=320, OW_Y=46, OW_H=73, WR_Y=119, WR_H=63, R3_Y=182, R3_H=63, KR_Y=245;

			// Row 1: OIL (left 240) | WATER (right 240), y=OW_Y..OW_Y+OW_H
			gfx->fillRect(0,   OW_Y, 239, OW_H, BG_W);
			gfx->fillRect(0,   OW_Y,   5, OW_H, WHT);
			gfx->setTextColor(WHT_D); gfx->setTextSize(1);
			gfx->setCursor(14, OW_Y + 5); gfx->print("OIL TEMP");
			gfx->fillRect(241, OW_Y, 239, OW_H, BG_W);
			gfx->fillRect(241, OW_Y,   5, OW_H, BLU);
			gfx->setTextColor(BLU_D); gfx->setTextSize(1);
			gfx->setCursor(255, OW_Y + 5); gfx->print("WATER TEMP");
			gfx->drawLine(240, OW_Y, 240, OW_Y + OW_H, SEP);
			gfx->fillRect(0, OW_Y + OW_H, SCREEN_WIDTH, 1, SEP);

			// Row 2: TYRE WEAR, y=WR_Y..WR_Y+WR_H
			gfx->fillRect(0, WR_Y, SCREEN_WIDTH, WR_H, BG_TW);
			gfx->fillRect(0, WR_Y,   5, WR_H, ORNG);
			gfx->setTextColor(ORNG_D); gfx->setTextSize(1);
			gfx->setCursor(14, WR_Y + 5); gfx->print("TYRE WEAR");
			gfx->fillRect(0, WR_Y + WR_H, SCREEN_WIDTH, 1, SEP);

			// Row 3: AIR | ROAD | DRS | TURBO, y=R3_Y..R3_Y+R3_H
			const uint16_t r3acc[4] = {BLU, RD, GRN, ORNG};
			const uint16_t r3bg[4]  = {BG_BLU, BG_RD, BG_GRN, BG_O};
			for (uint8_t c = 0; c < 4; c++) {
				int cx = c * 120;
				gfx->fillRect(cx, R3_Y, 119, R3_H, r3bg[c]);
				gfx->fillRect(cx, R3_Y,   5, R3_H, r3acc[c]);
				if (c > 0) gfx->drawLine(cx, R3_Y, cx, R3_Y + R3_H, SEP);
			}
			gfx->setTextColor(BLU_D);  gfx->setTextSize(1); gfx->setCursor(  14, R3_Y + 5); gfx->print("AIR");
			gfx->setTextColor(RGB565(80,15,15)); gfx->setCursor( 134, R3_Y + 5); gfx->print("ROAD");
			gfx->setTextColor(GRN_D);  gfx->setCursor( 254, R3_Y + 5); gfx->print("DRS");
			gfx->setTextColor(ORNG_D); gfx->setCursor( 374, R3_Y + 5); gfx->print("TURBO");
			gfx->fillRect(0, R3_Y + R3_H, SCREEN_WIDTH, 1, SEP);

			// Row 4: KERS bar, y=KR_Y..319
			const int KERS_H = FH - KR_Y;
			gfx->fillRect(0, KR_Y, SCREEN_WIDTH, KERS_H, BG_MGT);
			gfx->fillRect(0, KR_Y,   5, KERS_H, MGT);
			gfx->setTextColor(MGT_D); gfx->setTextSize(1);
			gfx->setCursor(14, KR_Y + 5); gfx->print("KERS / BATTERY");
			advFrameDrawn = true;
		}

		// Layout delta constants
		const int OW_Y=46, OW_H=73, WR_Y=119, WR_H=63, R3_Y=182, R3_H=63, KR_Y=245;

		// Oil temp
		String oilStr = oilTemperature + "C";
		if (prevData["a_oil"] != oilStr) {
			gfx->fillRect(6, OW_Y + 18, 232, 48, BG_W);
			gfx->setTextColor(WHT); gfx->setTextSize(4);
			gfx->setCursor(14, OW_Y + 20); gfx->print(oilStr);
			prevData["a_oil"] = oilStr;
		}
		// Water temp
		String watStr = waterTemperature + "C";
		if (prevData["a_wat"] != watStr) {
			gfx->fillRect(247, OW_Y + 18, 230, 48, BG_W);
			gfx->setTextColor(BLU); gfx->setTextSize(4);
			gfx->setCursor(255, OW_Y + 20); gfx->print(watStr);
			prevData["a_wat"] = watStr;
		}
		// Tyre wear (4 cols, textSize 3)
		String wearFL = tyreWearFrontLeft, wearFR = tyreWearFrontRight;
		String wearRL = tyreWearRearLeft,  wearRR = tyreWearRearRight;
		String wearStr = wearFL + " " + wearFR + " " + wearRL + " " + wearRR;
		if (prevData["a_wr"] != wearStr) {
			gfx->fillRect(6, WR_Y + 18, SCREEN_WIDTH - 7, 40, BG_TW);
			gfx->setTextColor(ORNG); gfx->setTextSize(3);
			gfx->setCursor(14, WR_Y + 20); gfx->print("FL:" + wearFL + " FR:" + wearFR + " RL:" + wearRL + " RR:" + wearRR);
			prevData["a_wr"] = wearStr;
		}
		// Air temp
		String airStr = airTemperature + "C";
		if (prevData["a_air"] != airStr) {
			gfx->fillRect(6, R3_Y + 18, 112, 40, BG_BLU);
			gfx->setTextColor(BLU); gfx->setTextSize(3);
			gfx->setCursor(14, R3_Y + 20); gfx->print(airStr);
			prevData["a_air"] = airStr;
		}
		// Road temp
		String rdStr = roadTemperature + "C";
		if (prevData["a_rd"] != rdStr) {
			gfx->fillRect(126, R3_Y + 18, 112, 40, BG_RD);
			gfx->setTextColor(RD); gfx->setTextSize(3);
			gfx->setCursor(134, R3_Y + 20); gfx->print(rdStr);
			prevData["a_rd"] = rdStr;
		}
		// DRS
		String drsStr = drsActive == "1" ? "OPEN" : (drsAvailable == "1" ? "AVAIL" : "OFF");
		uint16_t drsCol = drsActive == "1" ? BLU : (drsAvailable == "1" ? GRN : RGB565(80, 80, 80));
		if (prevData["a_drs"] != drsStr) {
			gfx->fillRect(246, R3_Y + 18, 112, 40, BG_GRN);
			gfx->setTextColor(drsCol); gfx->setTextSize(3);
			gfx->setCursor(254, R3_Y + 20); gfx->print(drsStr);
			prevData["a_drs"] = drsStr;
		}
		// Turbo
		if (prevData["a_trb"] != turboBoost) {
			gfx->fillRect(366, R3_Y + 18, 112, 40, BG_O);
			gfx->setTextColor(ORNG); gfx->setTextSize(3);
			gfx->setCursor(374, R3_Y + 20); gfx->print(turboBoost);
			prevData["a_trb"] = turboBoost;
		}
		// KERS bar
		if (prevData["a_kers"] != kersLevel) {
			int kv = kersLevel.toInt();
			gfx->fillRect(6, KR_Y + 5, SCREEN_WIDTH - 12, 24, BG_MGT);
			gfx->setTextColor(WHITE); gfx->setTextSize(3);
			gfx->setCursor(11, KR_Y + 5); gfx->print(String(kv) + "%");
			int barW = (kv * (SCREEN_WIDTH - 22)) / 100;
			const int BAR_Y = KR_Y + 34, BAR_H = 320 - KR_Y - 34 - 4;
			gfx->fillRect(11, BAR_Y, SCREEN_WIDTH - 22, BAR_H, RGB565(30, 0, 30));
			if (barW > 0) gfx->fillRect(11, BAR_Y, barW, BAR_H, MGT);
			prevData["a_kers"] = kersLevel;
		}
	}

	void drawLapsPageContent() {
		const uint16_t GOLD   = RGB565(220, 172,   0);
		const uint16_t GOLD_D = RGB565(100,  78,   0);
		const uint16_t TEAL   = RGB565(  0, 200, 220);
		const uint16_t TEAL_D = RGB565(  0,  80,  95);
		const uint16_t LIME   = RGB565( 60, 230,  80);
		const uint16_t LIME_D = RGB565( 24,  95,  32);
		const uint16_t ORNG   = RGB565(230, 140,   0);
		const uint16_t ORNG_D = RGB565( 90,  55,   0);
		const uint16_t BG_HDR = RGB565(  6,   8,  22);
		const uint16_t BG_G   = RGB565( 16,  12,   0);
		const uint16_t BG_T   = RGB565(  0,  12,  14);
		const uint16_t BG_L   = RGB565(  4,  16,   6);
		const uint16_t BG_S   = RGB565(  6,   6,  18);
		const uint16_t BG_W   = RGB565( 10,   7,   0);
		const uint16_t SEP    = RGB565( 22,  20,  30);
		const uint16_t sCols[3] = {GOLD, TEAL, LIME};

		if (!lapsFrameDrawn) {
			gfx->fillScreen(BLACK);
			gfx->fillRect(0, 0, SCREEN_WIDTH, 46, BG_HDR);
			gfx->fillRect(0, 44, SCREEN_WIDTH, 2, GOLD);
			gfx->setTextColor(WHITE); gfx->setTextSize(3);
			gfx->setCursor(14, 9); gfx->print("LAPS/SECTORS");

			// Layout: FH=320 | Header 0..45 | BEST/LAST 46..128 | CURRENT 129..188 | SECTORS 189..257 | TYRE WEAR 258..319
			const int FH=320, BL_Y=46, BL_H=83, CL_Y=129, CL_H=59, SC_Y=189, SC_H=68, WR_Y=258;

			// BEST (left 240) | LAST (right 240), y=BL_Y..BL_Y+BL_H
			gfx->fillRect(0,   BL_Y, 239, BL_H, BG_G);
			gfx->fillRect(0,   BL_Y,   5, BL_H, GOLD);
			gfx->setTextColor(GOLD_D); gfx->setTextSize(1);
			gfx->setCursor(14, BL_Y + 5); gfx->print("BEST LAP");
			gfx->fillRect(241, BL_Y, 239, BL_H, BG_T);
			gfx->fillRect(241, BL_Y,   5, BL_H, TEAL);
			gfx->setTextColor(TEAL_D); gfx->setTextSize(1);
			gfx->setCursor(255, BL_Y + 5); gfx->print("LAST LAP");
			gfx->drawLine(240, BL_Y, 240, BL_Y + BL_H, SEP);
			gfx->fillRect(0, BL_Y + BL_H, SCREEN_WIDTH, 1, SEP);

			// CURRENT LAP, y=CL_Y..CL_Y+CL_H
			gfx->fillRect(0, CL_Y, SCREEN_WIDTH, CL_H, BG_L);
			gfx->fillRect(0, CL_Y,   5, CL_H, LIME);
			gfx->setTextColor(LIME_D); gfx->setTextSize(1);
			gfx->setCursor(14, CL_Y + 5); gfx->print("CURRENT LAP");
			gfx->fillRect(0, CL_Y + CL_H, SCREEN_WIDTH, 1, SEP);

			// Sectors 3 cols 160px each, y=SC_Y..SC_Y+SC_H
			const char* sLabels[3] = {"S1","S2","S3"};
			for (uint8_t c = 0; c < 3; c++) {
				int cx = c * 160;
				gfx->fillRect(cx, SC_Y, 159, SC_H, BG_S);
				gfx->fillRect(cx, SC_Y,   5, SC_H, sCols[c]);
				if (c > 0) gfx->drawLine(cx, SC_Y, cx, SC_Y + SC_H, SEP);
				gfx->setTextColor(RGB565(60, 60, 80)); gfx->setTextSize(1);
				gfx->setCursor(14 + cx, SC_Y + 5); gfx->print(sLabels[c]);
			}
			gfx->fillRect(0, SC_Y + SC_H, SCREEN_WIDTH, 1, SEP);

			// Wear strip, y=WR_Y..319
			const int WEAR_H = FH - WR_Y;
			gfx->fillRect(0, WR_Y, SCREEN_WIDTH, WEAR_H, BG_W);
			gfx->fillRect(0, WR_Y,   5, WEAR_H, ORNG);
			gfx->setTextColor(ORNG_D); gfx->setTextSize(1);
			gfx->setCursor(14, WR_Y + 5); gfx->print("TYRE WEAR");
			lapsFrameDrawn = true;
		}

		// Layout delta constants
		const int BL_Y=46, BL_H=83, CL_Y=129, CL_H=59, SC_Y=189, SC_H=68, WR_Y=258;

		// Best lap
		if (prevData["l_best"] != bestLapTime) {
			gfx->fillRect(6, BL_Y + 18, 232, 56, BG_G);
			gfx->setTextColor(GOLD); gfx->setTextSize(4);
			gfx->setCursor(14, BL_Y + 20); gfx->print(bestLapTime);
			prevData["l_best"] = bestLapTime;
		}
		// Last lap
		if (prevData["l_last"] != lastLapTime) {
			gfx->fillRect(247, BL_Y + 18, 230, 56, BG_T);
			gfx->setTextColor(TEAL); gfx->setTextSize(4);
			gfx->setCursor(255, BL_Y + 20); gfx->print(lastLapTime);
			prevData["l_last"] = lastLapTime;
		}
		// Current lap
		if (prevData["l_cur"] != currentLapTime) {
			gfx->fillRect(6, CL_Y + 18, SCREEN_WIDTH - 7, 36, BG_L);
			uint16_t curCol = (lapInvalidated == "True") ? RGB565(220, 60, 60) : LIME;
			gfx->setTextColor(curCol); gfx->setTextSize(4);
			gfx->setCursor(14, CL_Y + 18); gfx->print(currentLapTime);
			prevData["l_cur"] = currentLapTime;
		}
		// Sectors
		String sVals[3] = {sector1Time, sector2Time, sector3Time};
		const char* sKeys[3] = {"l_s1","l_s2","l_s3"};
		for (uint8_t c = 0; c < 3; c++) {
			if (prevData[sKeys[c]] != sVals[c]) {
				int cx = c * 160;
				gfx->fillRect(6 + cx, SC_Y + 18, 152, 40, BG_S);
				gfx->setTextColor(sCols[c]); gfx->setTextSize(3);
				gfx->setCursor(14 + cx, SC_Y + 20); gfx->print(sVals[c]);
				prevData[sKeys[c]] = sVals[c];
			}
		}
		// Wear strip
		String wearStr = "FL:" + tyreWearFrontLeft + " FR:" + tyreWearFrontRight + " RL:" + tyreWearRearLeft + " RR:" + tyreWearRearRight;
		if (prevData["l_wear"] != wearStr) {
			gfx->fillRect(6, WR_Y + 18, SCREEN_WIDTH - 7, 36, BG_W);
			gfx->setTextColor(ORNG); gfx->setTextSize(3);
			gfx->setCursor(14, WR_Y + 20); gfx->print(wearStr);
			prevData["l_wear"] = wearStr;
		}
	}

	void drawRelativePageContent() {
		const uint16_t WHT    = WHITE;
		const uint16_t WHT_D  = RGB565( 80,  80,  90);
		const uint16_t MGT    = RGB565(200,  60, 220);
		const uint16_t MGT_D  = RGB565( 75,  22,  82);
		const uint16_t ORNG   = RGB565(230, 140,   0);
		const uint16_t ORNG_D = RGB565( 90,  55,   0);
		const uint16_t GRN    = RGB565( 60, 230,  80);
		const uint16_t GRN_D  = RGB565( 24, 100,  32);
		const uint16_t RD     = RGB565(220,  50,  50);
		const uint16_t TCL    = RGB565(  0, 200, 220);
		const uint16_t TCL_D  = RGB565(  0,  80,  95);
		const uint16_t BG_HDR = RGB565(  6,   8,  22);
		const uint16_t BG_P   = RGB565( 10,  10,  14);
		const uint16_t BG_GA  = RGB565( 14,   0,  16);
		const uint16_t BG_GB  = RGB565( 18,  10,   0);
		const uint16_t BG_FL  = RGB565(  4,  16,   6);
		const uint16_t BG_FLR = RGB565( 16,   4,   4);
		const uint16_t BG_DRS = RGB565(  4,  14,  16);
		const uint16_t BG_BTM = RGB565(  0,   9,  12);
		const uint16_t SEP    = RGB565( 22,  20,  30);

		if (!stratFrameDrawn) {
			gfx->fillScreen(BLACK);
			gfx->fillRect(0, 0, SCREEN_WIDTH, 46, BG_HDR);
			gfx->fillRect(0, 44, SCREEN_WIDTH, 2, WHT);
			gfx->setTextColor(WHITE); gfx->setTextSize(3);
			gfx->setCursor(14, 9); gfx->print("STRATEGY");

			// Layout: FH=320 | Header 0..45 | POSITION 46..128 | GAP AHD/BHD 129..197 | FUEL LAPS/DRS 198..257 | BTM 258..319
			const int FH=320, POS_Y=46, POS_H=83, GAP_Y=129, GAP_H=69, ROW3_Y=198, ROW3_H=59, BTM_Y=258;

			// POSITION full width, y=POS_Y..POS_Y+POS_H
			gfx->fillRect(0, POS_Y, SCREEN_WIDTH, POS_H, BG_P);
			gfx->fillRect(0, POS_Y,   5, POS_H, WHT);
			gfx->setTextColor(WHT_D); gfx->setTextSize(1);
			gfx->setCursor(14, POS_Y + 5); gfx->print("POSITION");
			gfx->fillRect(0, POS_Y + POS_H, SCREEN_WIDTH, 1, SEP);

			// GAP AHEAD (left 240) | GAP BEHIND (right 240), y=GAP_Y..GAP_Y+GAP_H
			gfx->fillRect(0,   GAP_Y, 239, GAP_H, BG_GA);
			gfx->fillRect(0,   GAP_Y,   5, GAP_H, MGT);
			gfx->setTextColor(MGT_D); gfx->setTextSize(1);
			gfx->setCursor(14, GAP_Y + 5); gfx->print("GAP AHEAD");
			gfx->fillRect(241, GAP_Y, 239, GAP_H, BG_GB);
			gfx->fillRect(241, GAP_Y,   5, GAP_H, ORNG);
			gfx->setTextColor(ORNG_D); gfx->setTextSize(1);
			gfx->setCursor(255, GAP_Y + 5); gfx->print("GAP BEHIND");
			gfx->drawLine(240, GAP_Y, 240, GAP_Y + GAP_H, SEP);
			gfx->fillRect(0, GAP_Y + GAP_H, SCREEN_WIDTH, 1, SEP);

			// FUEL (left) | DRS (right), y=ROW3_Y..ROW3_Y+ROW3_H
			gfx->fillRect(0,   ROW3_Y, 239, ROW3_H, BG_FL);
			gfx->fillRect(0,   ROW3_Y,   5, ROW3_H, GRN);
			gfx->setTextColor(GRN_D); gfx->setTextSize(1);
			gfx->setCursor(14, ROW3_Y + 5); gfx->print("FUEL LAPS");
			gfx->fillRect(241, ROW3_Y, 239, ROW3_H, BG_DRS);
			gfx->fillRect(241, ROW3_Y,   5, ROW3_H, TCL);
			gfx->setTextColor(TCL_D); gfx->setTextSize(1);
			gfx->setCursor(255, ROW3_Y + 5); gfx->print("DRS");
			gfx->drawLine(240, ROW3_Y, 240, ROW3_Y + ROW3_H, SEP);
			gfx->fillRect(0, ROW3_Y + ROW3_H, SCREEN_WIDTH, 1, SEP);

			// Bottom strip: FUEL/LAP + TIME LEFT, y=BTM_Y..319
			const int BTM_H = FH - BTM_Y;
			gfx->fillRect(0, BTM_Y, SCREEN_WIDTH, BTM_H, BG_BTM);
			gfx->fillRect(0, BTM_Y,   5, BTM_H, TCL);
			gfx->setTextColor(TCL_D); gfx->setTextSize(1);
			gfx->setCursor(14, BTM_Y + 5); gfx->print("FUEL/LAP");
			gfx->setCursor(255, BTM_Y + 5); gfx->print("TIME LEFT");
			stratFrameDrawn = true;
		}

		// Layout delta constants
		const int POS_Y=46, POS_H=83, GAP_Y=129, GAP_H=69, ROW3_Y=198, ROW3_H=59, BTM_Y=258;

		// Position
		String posStr = "P" + position + " / " + opponentsCount;
		if (prevData["s_pos"] != posStr) {
			gfx->fillRect(6, POS_Y + 22, SCREEN_WIDTH - 7, 56, BG_P);
			gfx->setTextColor(WHITE); gfx->setTextSize(5);
			gfx->setCursor(14, POS_Y + 24); gfx->print(posStr);
			prevData["s_pos"] = posStr;
		}
		// Gap ahead
		if (prevData["s_ga"] != driverAheadGap) {
			gfx->fillRect(6, GAP_Y + 22, 232, 42, BG_GA);
			gfx->setTextColor(MGT); gfx->setTextSize(4);
			gfx->setCursor(14, GAP_Y + 24); gfx->print(driverAheadGap);
			prevData["s_ga"] = driverAheadGap;
		}
		// Gap behind
		if (prevData["s_gb"] != driverBehindGap) {
			gfx->fillRect(247, GAP_Y + 22, 230, 42, BG_GB);
			gfx->setTextColor(ORNG); gfx->setTextSize(4);
			gfx->setCursor(255, GAP_Y + 24); gfx->print(driverBehindGap);
			prevData["s_gb"] = driverBehindGap;
		}
		// Fuel laps (color changes when low)
		float fuelF = fuelRemainingLaps.toFloat();
		uint16_t fuelCol = fuelF < 2.0f ? RD : (fuelF < 4.0f ? ORNG : GRN);
		uint16_t fuelBg  = fuelF < 2.0f ? BG_FLR : BG_FL;
		String fuelKey = fuelRemainingLaps + (fuelF < 2.0f ? "r" : "g");
		if (prevData["s_fl"] != fuelKey) {
			gfx->fillRect(0, ROW3_Y, 239, ROW3_H, fuelBg);
			gfx->fillRect(0, ROW3_Y,   5, ROW3_H, fuelCol);
			gfx->setTextColor(GRN_D); gfx->setTextSize(1);
			gfx->setCursor(14, ROW3_Y + 5); gfx->print("FUEL LAPS");
			gfx->fillRect(6, ROW3_Y + 22, 232, 34, fuelBg);
			gfx->setTextColor(fuelCol); gfx->setTextSize(3);
			gfx->setCursor(14, ROW3_Y + 24); gfx->print(fuelRemainingLaps);
			prevData["s_fl"] = fuelKey;
		}
		// DRS
		String drsStr = drsActive == "1" ? "OPEN" : (drsAvailable == "1" ? "AVAIL" : "CLOSED");
		uint16_t drsCol = drsActive == "1" ? RGB565(60, 160, 255) : (drsAvailable == "1" ? GRN : RGB565(80,80,90));
		if (prevData["s_drs"] != drsStr) {
			gfx->fillRect(247, ROW3_Y + 22, 230, 34, BG_DRS);
			gfx->setTextColor(drsCol); gfx->setTextSize(3);
			gfx->setCursor(255, ROW3_Y + 24); gfx->print(drsStr);
			prevData["s_drs"] = drsStr;
		}
		// Bottom: fuel/lap
		if (prevData["s_fpl"] != fuelLitersPerLap) {
			gfx->fillRect(6, BTM_Y + 18, 238, 40, BG_BTM);
			gfx->setTextColor(TCL); gfx->setTextSize(3);
			gfx->setCursor(14, BTM_Y + 20); gfx->print(fuelLitersPerLap + "L");
			prevData["s_fpl"] = fuelLitersPerLap;
		}
		// Bottom: time left
		if (prevData["s_tl"] != sessionTimeLeft) {
			gfx->fillRect(247, BTM_Y + 18, 230, 40, BG_BTM);
			gfx->setTextColor(TCL); gfx->setTextSize(3);
			gfx->setCursor(255, BTM_Y + 20); gfx->print(sessionTimeLeft);
			prevData["s_tl"] = sessionTimeLeft;
		}
	}

	// ── Track Map Helpers ──────────────────────────────────────
	const TrackMapEntry* findTrackMap(const String& tid) {
		String lower = tid;
		lower.toLowerCase();
		for (uint8_t i = 0; i < TRACK_MAP_COUNT; i++) {
			if (lower.indexOf(TRACK_MAP_TABLE[i].id) >= 0) {
				return &TRACK_MAP_TABLE[i];
			}
		}
		return nullptr;
	}

	void drawTrackPolyline(const int16_t* points, uint8_t count, int ox, int oy, uint16_t color) {
		for (uint8_t i = 0; i < count; i++) {
			uint8_t next = (i + 1) % count;
			int x0 = (int16_t)pgm_read_word(&points[i * 2])     + ox;
			int y0 = (int16_t)pgm_read_word(&points[i * 2 + 1]) + oy;
			int x1 = (int16_t)pgm_read_word(&points[next * 2])     + ox;
			int y1 = (int16_t)pgm_read_word(&points[next * 2 + 1]) + oy;
			gfx->drawLine(x0, y0, x1, y1, color);
		}
	}

	void getPositionOnTrack(const int16_t* points, uint8_t count, float trackPos, int ox, int oy, int& outX, int& outY) {
		float idx = trackPos * count;
		int i0 = (int)idx % count;
		int i1 = (i0 + 1) % count;
		float frac = idx - (int)idx;
		int x0 = (int16_t)pgm_read_word(&points[i0 * 2]);
		int y0 = (int16_t)pgm_read_word(&points[i0 * 2 + 1]);
		int x1 = (int16_t)pgm_read_word(&points[i1 * 2]);
		int y1 = (int16_t)pgm_read_word(&points[i1 * 2 + 1]);
		outX = (int)(x0 + frac * (x1 - x0)) + ox;
		outY = (int)(y0 + frac * (y1 - y0)) + oy;
	}

	void drawMapPageContent() {
		const uint16_t CYN    = RGB565(  0, 200, 220);
		const uint16_t YLW    = RGB565(230, 210,   0);
		const uint16_t GRN    = RGB565( 60, 230,  80);
		const uint16_t RD     = RGB565(220,  50,  50);
		const uint16_t MGT    = RGB565(200,  60, 220);
		const uint16_t BLU    = RGB565( 60, 140, 240);
		const uint16_t ORNG   = RGB565(230, 150,   0);
		const uint16_t BG_HDR = RGB565(  6,   8,  22);
		const uint16_t BG_PAN = RGB565(  8,   8,  12);
		const uint16_t DIM    = RGB565(120, 120, 130);
		const uint16_t SEP    = RGB565( 22,  20,  30);
		const int PANEL_X     = 280;

		if (!mapFrameDrawn) {
			gfx->fillScreen(BLACK);
			gfx->fillRect(0, 0, SCREEN_WIDTH, 42, BG_HDR);
			gfx->fillRect(0, 41, SCREEN_WIDTH, 2, CYN);
			gfx->setTextColor(CYN); gfx->setTextSize(2);
			gfx->setCursor(10, 10); gfx->print("MAP");
			// Right panel bg + separator (fills to physical bottom)
			const int MAP_PAN_H = 320 - 43;
			gfx->fillRect(PANEL_X - 5, 43, SCREEN_WIDTH - PANEL_X + 5, MAP_PAN_H, BG_PAN);
			gfx->drawLine(PANEL_X - 7, 43, PANEL_X - 7, 319, SEP);
			mapFrameDrawn = true;
		}

		// Header: track name
		if (prevData["m_trk"] != trackId) {
			gfx->fillRect(55, 4, 180, 32, BG_HDR);
			String dt = trackId; if (dt.length() > 17) dt = dt.substring(0, 17);
			gfx->setTextColor(WHITE); gfx->setTextSize(1);
			gfx->setCursor(55, 13); gfx->print(dt);
			prevData["m_trk"] = trackId;
		}
		// Header: position
		String hPos = "P" + position + "/" + opponentsCount;
		if (prevData["m_hpos"] != hPos) {
			gfx->fillRect(PANEL_X, 4, SCREEN_WIDTH - PANEL_X, 36, BG_HDR);
			gfx->setTextColor(YLW); gfx->setTextSize(2);
			gfx->setCursor(PANEL_X, 6); gfx->print(hPos);
			prevData["m_hpos"] = hPos;
		}

		// Map area: redraw when track or positions change
		String posKey = trackPositionPercent + "|" + aheadTrackPosition + "|" + behindTrackPosition;
		if (!mapTrackDrawn || mapLastTrackId != trackId || prevData["m_pk"] != posKey) {
			gfx->fillRect(0, 43, PANEL_X - 8, (SCREEN_HEIGHT + 48) - 43, BLACK);
			const TrackMapEntry* tmap = findTrackMap(trackId);
			float myPos = trackPositionPercent.toFloat();
			float aPos  = aheadTrackPosition.toFloat();
			float bPos  = behindTrackPosition.toFloat();
			int dx, dy;
			if (tmap) {
				drawTrackPolyline(tmap->points, tmap->numPoints, 25, 52, RGB565(80, 80, 80));
				int sfX = (int16_t)pgm_read_word(&tmap->points[0]) + 25;
				int sfY = (int16_t)pgm_read_word(&tmap->points[1]) + 52;
				gfx->drawLine(sfX-4, sfY-4, sfX+4, sfY-4, WHITE);
				gfx->drawLine(sfX-4, sfY+4, sfX+4, sfY+4, WHITE);
				if (aPos > 0.001f) {
					getPositionOnTrack(tmap->points, tmap->numPoints, aPos, 25, 52, dx, dy);
					gfx->fillCircle(dx, dy, 4, RD);
					gfx->setTextColor(RD); gfx->setTextSize(1); gfx->setCursor(dx+6, dy-3); gfx->print("A");
				}
				if (bPos > 0.001f) {
					getPositionOnTrack(tmap->points, tmap->numPoints, bPos, 25, 52, dx, dy);
					gfx->fillCircle(dx, dy, 4, BLU);
					gfx->setTextColor(BLU); gfx->setTextSize(1); gfx->setCursor(dx+6, dy-3); gfx->print("B");
				}
				if (myPos > 0.001f || hasReceivedData) {
					getPositionOnTrack(tmap->points, tmap->numPoints, myPos, 25, 52, dx, dy);
					gfx->fillCircle(dx, dy, 5, GRN); gfx->drawCircle(dx, dy, 6, WHITE);
				}
			} else {
				const int mCX=133, mCY=141, mRX=108, mRY=68;
				gfx->drawEllipse(mCX, mCY, mRX, mRY, RGB565(60,60,60));
				gfx->drawEllipse(mCX, mCY, mRX-1, mRY-1, RGB565(80,80,80));
				if (aPos > 0.001f) { float a=aPos*TWO_PI-HALF_PI; int x=mCX+(int)(mRX*cos(a)),y=mCY+(int)(mRY*sin(a)); gfx->fillCircle(x,y,4,RD); }
				if (bPos > 0.001f) { float a=bPos*TWO_PI-HALF_PI; int x=mCX+(int)(mRX*cos(a)),y=mCY+(int)(mRY*sin(a)); gfx->fillCircle(x,y,4,BLU); }
				if (myPos > 0.001f || hasReceivedData) { float a=myPos*TWO_PI-HALF_PI; int x=mCX+(int)(mRX*cos(a)),y=mCY+(int)(mRY*sin(a)); gfx->fillCircle(x,y,5,GRN); gfx->drawCircle(x,y,6,WHITE); }
			}
			mapLastTrackId = trackId; mapTrackDrawn = true;
			prevData["m_pk"] = posKey;
		}

		// Right panel: delta rendering of each row (label + value redrawn together)
		// Rows spaced 27px apart starting at y=43 → fills FH=320
		// Row DELTA (y=43)
		if (prevData["m_dt"] != sessionBestLiveDeltaSeconds) {
			gfx->fillRect(PANEL_X, 43, SCREEN_WIDTH - PANEL_X, 26, BG_PAN);
			bool neg = sessionBestLiveDeltaSeconds.indexOf('-') >= 0;
			gfx->setTextColor(DIM); gfx->setTextSize(1); gfx->setCursor(PANEL_X, 47); gfx->print("DELTA");
			gfx->setTextColor(neg ? GRN : RD); gfx->setTextSize(2); gfx->setCursor(PANEL_X+50, 43); gfx->print(sessionBestLiveDeltaSeconds);
			prevData["m_dt"] = sessionBestLiveDeltaSeconds;
		}
		// Row GAP AHEAD (y=70)
		if (prevData["m_ga"] != driverAheadGap) {
			gfx->fillRect(PANEL_X, 70, SCREEN_WIDTH - PANEL_X, 26, BG_PAN);
			gfx->setTextColor(DIM); gfx->setTextSize(1); gfx->setCursor(PANEL_X, 74); gfx->print("GAP+");
			gfx->setTextColor(MGT); gfx->setTextSize(2); gfx->setCursor(PANEL_X+40, 70); gfx->print(driverAheadGap);
			prevData["m_ga"] = driverAheadGap;
		}
		// Row GAP BEHIND (y=97)
		if (prevData["m_gb"] != driverBehindGap) {
			gfx->fillRect(PANEL_X, 97, SCREEN_WIDTH - PANEL_X, 26, BG_PAN);
			gfx->setTextColor(DIM); gfx->setTextSize(1); gfx->setCursor(PANEL_X, 101); gfx->print("GAP-");
			gfx->setTextColor(ORNG); gfx->setTextSize(2); gfx->setCursor(PANEL_X+40, 97); gfx->print(driverBehindGap);
			prevData["m_gb"] = driverBehindGap;
		}
		// Row SPD + GEAR (y=124)
		String spdGear = speed + "|" + gear;
		if (prevData["m_sg"] != spdGear) {
			gfx->fillRect(PANEL_X, 124, SCREEN_WIDTH - PANEL_X, 26, BG_PAN);
			gfx->setTextColor(DIM); gfx->setTextSize(1); gfx->setCursor(PANEL_X, 128); gfx->print("SPD");
			gfx->setTextColor(WHITE); gfx->setTextSize(2); gfx->setCursor(PANEL_X+26, 124); gfx->print(speed);
			gfx->setTextColor(DIM); gfx->setTextSize(1); gfx->setCursor(PANEL_X+105, 128); gfx->print("G");
			gfx->setTextColor(WHITE); gfx->setTextSize(2); gfx->setCursor(PANEL_X+115, 124); gfx->print(gear);
			prevData["m_sg"] = spdGear;
		}
		// Row BRAKE BIAS (y=151)
		if (prevData["m_bb"] != brakeBias) {
			gfx->fillRect(PANEL_X, 151, SCREEN_WIDTH - PANEL_X, 26, BG_PAN);
			gfx->setTextColor(DIM); gfx->setTextSize(1); gfx->setCursor(PANEL_X, 155); gfx->print("BB");
			gfx->setTextColor(MGT); gfx->setTextSize(2); gfx->setCursor(PANEL_X+20, 151); gfx->print(brakeBias);
			prevData["m_bb"] = brakeBias;
		}
		// Row TC + ABS (y=178)
		String tcabs = tcLevel + "|" + absLevel;
		if (prevData["m_tcabs"] != tcabs) {
			gfx->fillRect(PANEL_X, 178, SCREEN_WIDTH - PANEL_X, 26, BG_PAN);
			gfx->setTextColor(DIM); gfx->setTextSize(1); gfx->setCursor(PANEL_X, 182); gfx->print("TC");
			gfx->setTextColor(YLW); gfx->setTextSize(2); gfx->setCursor(PANEL_X+20, 178); gfx->print(tcLevel);
			gfx->setTextColor(DIM); gfx->setTextSize(1); gfx->setCursor(PANEL_X+80, 182); gfx->print("ABS");
			gfx->setTextColor(BLU); gfx->setTextSize(2); gfx->setCursor(PANEL_X+108, 178); gfx->print(absLevel);
			prevData["m_tcabs"] = tcabs;
		}
		// Row LAP TIME (y=205)
		if (prevData["m_lap"] != currentLapTime) {
			gfx->fillRect(PANEL_X, 205, SCREEN_WIDTH - PANEL_X, 26, BG_PAN);
			gfx->setTextColor(DIM); gfx->setTextSize(1); gfx->setCursor(PANEL_X, 209); gfx->print("LAP");
			gfx->setTextColor(lapInvalidated == "True" ? RD : WHITE); gfx->setTextSize(1);
			gfx->setCursor(PANEL_X+24, 209); gfx->print(currentLapTime);
			prevData["m_lap"] = currentLapTime;
		}
		// Row FUEL (y=232)
		if (prevData["m_fmap"] != fuelRemainingLaps) {
			gfx->fillRect(PANEL_X, 232, SCREEN_WIDTH - PANEL_X, 26, BG_PAN);
			float fv = fuelRemainingLaps.toFloat();
			gfx->setTextColor(DIM); gfx->setTextSize(1); gfx->setCursor(PANEL_X, 236); gfx->print("FUEL");
			gfx->setTextColor(fv < 3.0f ? RD : GRN); gfx->setTextSize(2);
			gfx->setCursor(PANEL_X+30, 232); gfx->print(fuelRemainingLaps);
			prevData["m_fmap"] = fuelRemainingLaps;
		}
		// Row KERS (y=259)
		if (prevData["m_kers"] != kersLevel) {
			gfx->fillRect(PANEL_X, 259, SCREEN_WIDTH - PANEL_X, 26, BG_PAN);
			int kv2 = kersLevel.toInt();
			gfx->setTextColor(DIM); gfx->setTextSize(1); gfx->setCursor(PANEL_X, 263); gfx->print("ERS");
			gfx->setTextColor(kv2 > 20 ? GRN : RD); gfx->setTextSize(2);
			gfx->setCursor(PANEL_X+30, 259); gfx->print(kersLevel + "%");
			prevData["m_kers"] = kersLevel;
		}
	}

	void idle() {
	}

	// ============================================================
	// PAGE_499P: Ferrari 499P-inspired dashboard
	// Layout: 480×272
	// Delta rendering — static frame drawn once, values only on change
	// ============================================================
	void draw499PPageContent() {
		if (!canUseDisplay()) return;

		const int W      = SCREEN_WIDTH;   // 480
		const int H      = 320;            // physical height FH
		const int TOP_H  = 40;
		const int BOT_H  = 46;
		const int BOT_Y  = H - BOT_H;     // 274
		const int MID_Y  = TOP_H;          // 40
		const int MID_H  = BOT_Y - MID_Y; // 234

		// Colours
		const uint16_t BG_DARK  = RGB565( 10,  10,  10);
		const uint16_t BG_HDR   = RGB565( 22,  22,  28);
		const uint16_t BG_BOT   = RGB565( 18,  18,  24);
		const uint16_t BG_TC    = RGB565( 28,   6,   6);
		const uint16_t LBL      = RGB565(130, 130, 140);
		const uint16_t WHT      = WHITE;
		const uint16_t RD       = RGB565(220,  30,  30);
		const uint16_t GRN      = RGB565(  0, 200,  80);
		const uint16_t YLW      = RGB565(230, 210,   0);
		const uint16_t CYN      = RGB565(  0, 200, 220);
		const uint16_t MGT      = MAGENTA;
		const uint16_t SEP      = RGB565( 55,  55,  65);

		// Column x-positions (total 480px):
		// TC1[0..49] | SPD[50..189] | GEAR/ERS[190..309] | FUEL[310..429] | TC2[430..479]
		const int TC_W  = 50;
		const int SL_X  = TC_W;       const int SL_W = 140;
		const int CC_X  = SL_X+SL_W;  const int CC_W = 120;
		const int SR_X  = CC_X+CC_W;  const int SR_W = 120;
		const int TC2_X = W - TC_W;
		const int TYRE_Y = MID_Y + 130; // horizontal divider y = 170

		// ── Static frame (drawn once) ─────────────────────────────
		if (!p499FrameDrawn) {
			gfx->fillScreen(BG_DARK);

			// Top bar
			gfx->fillRect(0, 0, W, TOP_H, BG_HDR);
			gfx->drawLine(0, TOP_H, W, TOP_H, SEP);
			gfx->setTextColor(LBL); gfx->setTextSize(1);
			gfx->setCursor(4,      5); gfx->print("LAP");
			gfx->setCursor(W-62,   5); gfx->print("FUEL");

			// TC columns
			gfx->fillRect(0,     MID_Y, TC_W, MID_H, BG_TC);
			gfx->fillRect(TC2_X, MID_Y, TC_W, MID_H, BG_TC);
			gfx->drawRect(0,     MID_Y, TC_W, MID_H, RGB565(80,30,30));
			gfx->drawRect(TC2_X, MID_Y, TC_W, MID_H, RGB565(80,30,30));
			gfx->setTextColor(LBL); gfx->setTextSize(1);
			gfx->setCursor(6,        MID_Y+4); gfx->print("TC 1");
			gfx->setCursor(TC2_X+6, MID_Y+4); gfx->print("TC 2");

			// Column separators
			gfx->drawLine(SL_X,  MID_Y, SL_X,  BOT_Y, SEP);
			gfx->drawLine(CC_X,  MID_Y, CC_X,  BOT_Y, SEP);
			gfx->drawLine(SR_X,  MID_Y, SR_X,  BOT_Y, SEP);
			gfx->drawLine(TC2_X, MID_Y, TC2_X, BOT_Y, SEP);

			// Left col label
			gfx->setTextColor(LBL); gfx->setTextSize(1);
			gfx->setCursor(SL_X+5, MID_Y+4); gfx->print("SPEED km/h");

			// Centre col labels
			gfx->setCursor(CC_X+5, MID_Y+4); gfx->print("ERS");

			// Right col labels
			gfx->setCursor(SR_X+5, MID_Y+4);  gfx->print("FUEL/LAP");
			gfx->setCursor(SR_X+5, MID_Y+50); gfx->print("LAPS LEFT");

			// Horizontal divider (tyre area)
			gfx->drawLine(SL_X, TYRE_Y, TC2_X, TYRE_Y, SEP);

			// Tyre corner labels
			int h2 = SL_W/2;
			gfx->setTextColor(LBL);
			gfx->setCursor(SL_X+3,    TYRE_Y+4); gfx->print("FL");
			gfx->setCursor(SL_X+h2+3, TYRE_Y+4); gfx->print("FR");
			gfx->setCursor(SL_X+3,    TYRE_Y+48); gfx->print("RL");
			gfx->setCursor(SL_X+h2+3, TYRE_Y+48); gfx->print("RR");

			// Centre lower: ARB labels
			gfx->setCursor(CC_X+4,  TYRE_Y+4); gfx->print("ARB F");
			gfx->setCursor(CC_X+64, TYRE_Y+4); gfx->print("ARB R");
			gfx->setCursor(CC_X+30, TYRE_Y+36); gfx->print("POS");

			// Right lower: brake temp labels
			gfx->setCursor(SR_X+3,  TYRE_Y+4); gfx->print("BFL");
			gfx->setCursor(SR_X+63, TYRE_Y+4); gfx->print("BFR");
			gfx->setCursor(SR_X+3,  TYRE_Y+48); gfx->print("BRL");
			gfx->setCursor(SR_X+63, TYRE_Y+48); gfx->print("BRR");

			// Bottom bar
			gfx->fillRect(0, BOT_Y, W, BOT_H, BG_BOT);
			gfx->drawLine(0, BOT_Y, W, BOT_Y, SEP);
			const char* bLbls[] = {"H WIND","ENG MAP","BRK BIAS","BRK MIG","SOC %","REAR %"};
			int bCW = W/6;
			for (int i = 0; i < 6; i++) {
				int cx = i*bCW;
				if (i>0) gfx->drawLine(cx, BOT_Y, cx, H, SEP);
				gfx->setTextColor(LBL); gfx->setTextSize(1);
				gfx->setCursor(cx+3, BOT_Y+3); gfx->print(bLbls[i]);
			}

			p499FrameDrawn = true;
		}

		// ── helpers ──────────────────────────────────────────────
		int16_t bx, by; uint16_t bw, bh;
		int bCW = W/6;
		int h2 = SL_W/2;
		float fuelVal = fuelRemainingLaps.toFloat();
		int kv = kersLevel.toInt();

		// ── Top bar ───────────────────────────────────────────────
		// Lap time
		if (prevData["p_lap"] != currentLapTime) {
			gfx->fillRect(28, 2, 140, 28, BG_HDR);
			gfx->setTextColor(WHT); gfx->setTextSize(2);
			gfx->setCursor(30, 9); gfx->print(currentLapTime);
			prevData["p_lap"] = currentLapTime;
		}
		// Delta (centred)
		if (prevData["p_dt"] != sessionBestLiveDeltaSeconds) {
			gfx->fillRect(190, 2, 100, 28, BG_HDR);
			bool neg = sessionBestLiveDeltaSeconds.indexOf('-') >= 0;
			gfx->setTextColor(neg ? GRN : RD); gfx->setTextSize(2);
			gfx->getTextBounds(sessionBestLiveDeltaSeconds, 0,0,&bx,&by,&bw,&bh);
			gfx->setCursor((W-bw)/2, 9); gfx->print(sessionBestLiveDeltaSeconds);
			prevData["p_dt"] = sessionBestLiveDeltaSeconds;
		}
		// Fuel
		if (prevData["p_fl"] != fuelRemainingLaps) {
			gfx->fillRect(W-60, 2, 58, 28, BG_HDR);
			gfx->setTextColor(fuelVal<3.0f ? RD : WHT); gfx->setTextSize(2);
			gfx->setCursor(W-58, 9); gfx->print(fuelRemainingLaps);
			prevData["p_fl"] = fuelRemainingLaps;
		}

		// ── TC1 (left edge) ──────────────────────────────────────
		if (prevData["p_tc1"] != tcLevel) {
			gfx->fillRect(2, MID_Y+20, TC_W-4, MID_H-22, BG_TC);
			gfx->setTextColor(RD); gfx->setTextSize(4);
			gfx->setCursor(8, MID_Y+24); gfx->print(tcLevel);
			prevData["p_tc1"] = tcLevel;
		}
		// ── TC2 (right edge) ─────────────────────────────────────
		if (prevData["p_tc2"] != tcCut) {
			gfx->fillRect(TC2_X+2, MID_Y+20, TC_W-4, MID_H-22, BG_TC);
			gfx->setTextColor(RD); gfx->setTextSize(4);
			gfx->setCursor(TC2_X+6, MID_Y+24); gfx->print(tcCut);
			prevData["p_tc2"] = tcCut;
		}

		// ── SPEED (left column) ──────────────────────────────────
		if (prevData["p_spd"] != speed) {
			gfx->fillRect(SL_X+2, MID_Y+18, SL_W-4, TYRE_Y-MID_Y-20, BG_DARK);
			gfx->setTextColor(WHT); gfx->setTextSize(5);
			gfx->getTextBounds(speed,0,0,&bx,&by,&bw,&bh);
			gfx->setCursor(SL_X+(SL_W-bw)/2, MID_Y+22);
			gfx->print(speed);
			prevData["p_spd"] = speed;
		}

		// ── GEAR (centre, large) ─────────────────────────────────
		if (prevData["p_gr"] != gear) {
			gfx->fillRect(CC_X+2, MID_Y+22, CC_W-4, TYRE_Y-MID_Y-24, BG_DARK);
			gfx->setTextColor(YLW); gfx->setTextSize(7);
			gfx->getTextBounds(gear,0,0,&bx,&by,&bw,&bh);
			gfx->setCursor(CC_X+(CC_W-bw)/2, MID_Y+26);
			gfx->print(gear);
			prevData["p_gr"] = gear;
		}

		// ── ERS mode (above gear) ────────────────────────────────
		if (prevData["p_ers"] != ersDeployMode) {
			gfx->fillRect(CC_X+2, MID_Y+16, CC_W-4, 18, BG_DARK);
			String em = ersDeployMode; em.toUpperCase();
			uint16_t ec = WHT;
			if (em.indexOf("ATTACK")>=0||em.indexOf("HOTLAP")>=0) ec=RD;
			else if (em.indexOf("QUAL")>=0) ec=MGT;
			else if (em=="NONE"||em=="--") ec=LBL;
			gfx->setTextColor(ec); gfx->setTextSize(1);
			gfx->setCursor(CC_X+5, MID_Y+18); gfx->print(ersDeployMode);
			prevData["p_ers"] = ersDeployMode;
		}

		// ── KERS vertical bar (right side of centre col) ─────────
		if (prevData["p_kv"] != kersLevel) {
			const int bX=CC_X+CC_W-18, bTop=MID_Y+22, bBarW=12, bBarH=TYRE_Y-bTop-4;
			uint16_t kc = kv>50 ? GRN : (kv>20 ? YLW : RD);
			int fillH = (kv*(bBarH-2))/100;
			gfx->drawRect(bX, bTop, bBarW, bBarH, LBL);
			gfx->fillRect(bX+1, bTop+1, bBarW-2, bBarH-2, BG_DARK);
			if (fillH>0) gfx->fillRect(bX+1, bTop+bBarH-1-fillH, bBarW-2, fillH, kc);
			prevData["p_kv"] = kersLevel;
		}

		// ── FUEL/LAP (right column) ──────────────────────────────
		if (prevData["p_fpl"] != fuelLitersPerLap) {
			gfx->fillRect(SR_X+2, MID_Y+14, SR_W-4, 30, BG_DARK);
			gfx->setTextColor(WHT); gfx->setTextSize(3);
			gfx->setCursor(SR_X+5, MID_Y+16); gfx->print(fuelLitersPerLap);
			prevData["p_fpl"] = fuelLitersPerLap;
		}
		// LAPS LEFT
		if (prevData["p_frl"] != fuelRemainingLaps) {
			gfx->fillRect(SR_X+2, MID_Y+58, SR_W-4, 30, BG_DARK);
			gfx->setTextColor(fuelVal<3.0f ? RD : WHT); gfx->setTextSize(3);
			gfx->setCursor(SR_X+5, MID_Y+60); gfx->print(fuelRemainingLaps);
			prevData["p_frl"] = fuelRemainingLaps;
		}

		// ── Tyre pressures + temps ───────────────────────────────
		struct { int cx; int cy; const String* p; const String* t; const char* k; } tyres[4] = {
			{SL_X+3,    TYRE_Y+14, &tyrePressureFrontLeft,  &tyreTemperatureFrontLeft,  "p_tfl"},
			{SL_X+h2+3, TYRE_Y+14, &tyrePressureFrontRight, &tyreTemperatureFrontRight, "p_tfr"},
			{SL_X+3,    TYRE_Y+58, &tyrePressureRearLeft,   &tyreTemperatureRearLeft,   "p_trl"},
			{SL_X+h2+3, TYRE_Y+58, &tyrePressureRearRight,  &tyreTemperatureRearRight,  "p_trr"},
		};
		for (auto& tr : tyres) {
			String pv = *tr.p + "|" + *tr.t;
			if (prevData[tr.k] != pv) {
				gfx->fillRect(tr.cx, tr.cy, h2-4, 32, BG_DARK);
				gfx->setTextColor(CYN); gfx->setTextSize(2);
				gfx->setCursor(tr.cx, tr.cy); gfx->print(*tr.p);
				gfx->setTextColor(LBL); gfx->setTextSize(1);
				gfx->setCursor(tr.cx, tr.cy+20); gfx->print(*tr.t+"C");
				prevData[tr.k] = pv;
			}
		}

		// ── ARB F / ARB R ────────────────────────────────────────
		if (prevData["p_arbf"] != arbFront) {
			gfx->fillRect(CC_X+2,  TYRE_Y+14, 58, 18, BG_DARK);
			gfx->setTextColor(WHT); gfx->setTextSize(2);
			gfx->setCursor(CC_X+4, TYRE_Y+14); gfx->print(arbFront);
			prevData["p_arbf"] = arbFront;
		}
		if (prevData["p_arbr"] != arbRear) {
			gfx->fillRect(CC_X+62, TYRE_Y+14, 56, 18, BG_DARK);
			gfx->setTextColor(WHT); gfx->setTextSize(2);
			gfx->setCursor(CC_X+64, TYRE_Y+14); gfx->print(arbRear);
			prevData["p_arbr"] = arbRear;
		}
		// ── POSITION ─────────────────────────────────────────────
		if (prevData["p_pos"] != position) {
			gfx->fillRect(CC_X+2, TYRE_Y+44, CC_W-4, 22, BG_DARK);
			gfx->setTextColor(YLW); gfx->setTextSize(2);
			String posStr = "P"+position;
			gfx->getTextBounds(posStr,0,0,&bx,&by,&bw,&bh);
			gfx->setCursor(CC_X+(CC_W-bw)/2, TYRE_Y+46); gfx->print(posStr);
			prevData["p_pos"] = position;
		}
		// ── GAP AHD / BHD ────────────────────────────────────────
		if (prevData["p_gap"] != driverAheadGap+"|"+driverBehindGap) {
			gfx->fillRect(CC_X+2, TYRE_Y+68, CC_W-4, 24, BG_DARK);
			gfx->setTextSize(1);
			gfx->setCursor(CC_X+4, TYRE_Y+70);
			gfx->setTextColor(LBL); gfx->print("AHD ");
			gfx->setTextColor(GRN); gfx->print(driverAheadGap);
			gfx->setCursor(CC_X+4, TYRE_Y+80);
			gfx->setTextColor(LBL); gfx->print("BHD ");
			gfx->setTextColor(RD);  gfx->print(driverBehindGap);
			prevData["p_gap"] = driverAheadGap+"|"+driverBehindGap;
		}

		// ── Brake temps ──────────────────────────────────────────
		struct { int cx; int cy; const String* v; const char* k; } brkTemps[4] = {
			{SR_X+3,  TYRE_Y+14, &brakeTemperatureFrontLeft,  "p_bfl"},
			{SR_X+63, TYRE_Y+14, &brakeTemperatureFrontRight, "p_bfr"},
			{SR_X+3,  TYRE_Y+58, &brakeTemperatureRearLeft,   "p_brl"},
			{SR_X+63, TYRE_Y+58, &brakeTemperatureRearRight,  "p_brr"},
		};
		for (auto& bt : brkTemps) {
			if (prevData[bt.k] != *bt.v) {
				gfx->fillRect(bt.cx, bt.cy, 56, 22, BG_DARK);
				gfx->setTextColor(WHT); gfx->setTextSize(2);
				gfx->setCursor(bt.cx, bt.cy); gfx->print(*bt.v);
				prevData[bt.k] = *bt.v;
			}
		}
		// ── Tyre wear FL/FR ───────────────────────────────────────
		if (prevData["p_wr"] != tyreWearFrontLeft+"|"+tyreWearFrontRight) {
			gfx->fillRect(SL_X+2, TYRE_Y+92, SL_W-4, 10, BG_DARK);
			gfx->setTextSize(1);
			gfx->setCursor(SL_X+3, TYRE_Y+92);
			gfx->setTextColor(LBL); gfx->print("WR:");
			gfx->setTextColor(CYN); gfx->print(tyreWearFrontLeft+"%");
			gfx->setCursor(SL_X+h2+3, TYRE_Y+92);
			gfx->setTextColor(LBL); gfx->print("WR:");
			gfx->setTextColor(CYN); gfx->print(tyreWearFrontRight+"%");
			prevData["p_wr"] = tyreWearFrontLeft+"|"+tyreWearFrontRight;
		}
		// ── Oil / Water temp ──────────────────────────────────────
		if (prevData["p_therm"] != oilTemperature+"|"+waterTemperature) {
			gfx->fillRect(SR_X+2, TYRE_Y+82, SR_W-4, 12, BG_DARK);
			gfx->setTextSize(1);
			gfx->setCursor(SR_X+3, TYRE_Y+84);
			gfx->setTextColor(LBL); gfx->print("OL:");
			gfx->setTextColor(YLW); gfx->print(oilTemperature);
			gfx->setCursor(SR_X+63, TYRE_Y+84);
			gfx->setTextColor(LBL); gfx->print("HW:");
			gfx->setTextColor(CYN); gfx->print(waterTemperature);
			prevData["p_therm"] = oilTemperature+"|"+waterTemperature;
		}

		// ── Bottom bar values ─────────────────────────────────────
		String bVals[6] = {headWind, popupMessage, brakeBias, brkMigration, kersLevel, rearBrakeBias};
		uint16_t bClrs[6] = {WHT, WHT, MGT, WHT, kv>20?GRN:RD, WHT};
		const char* bKeys[6] = {"p_b0","p_b1","p_b2","p_b3","p_b4","p_b5"};
		for (int i = 0; i < 6; i++) {
			if (prevData[bKeys[i]] != bVals[i]) {
				int cx = i*bCW;
				gfx->fillRect(cx+1, BOT_Y+18, bCW-2, BOT_H-20, BG_BOT);
				uint16_t vc = (bVals[i]=="--"||bVals[i]=="None") ? LBL : bClrs[i];
				gfx->setTextColor(vc); gfx->setTextSize(2);
				gfx->setCursor(cx+3, BOT_Y+18); gfx->print(bVals[i]);
				prevData[bKeys[i]] = bVals[i];
			}
		}
	}

	void drawGear(int32_t x, int32_t y)
	{
		if (!canUseDisplay()) return;
		// draw gear only when it changes
		if (gear != prev_gear)
		{
			gfx->setTextColor(YELLOW, BLACK);
			auto fontSize = 10;
			drawCentreCentreString(gear, x, y, fontSize, gfx, 1 * PIXEL_PER_MM, 0.5 * PIXEL_PER_MM);
			prev_gear = gear;
		}
	}

	boolean isDrawGearRpmRedRec()
	{
		if (rpmPercent >= rpmRedLineSetting)
		{
			return true;
		}
		return false;
	}

	void drawRpmMeter(int32_t x, int32_t y, int width, int height)
	{
		if (!canUseDisplay()) return;
		int meterWidth = (width * rpmPercent) / 100;

		int yPlusOne = y + 1;
		int innerWidth = width - meterWidth - 1;
		int innerHeight = height - 4;

		if (prev_rpmPercent > rpmPercent)
		{
			gfx->fillRect(meterWidth, yPlusOne, innerWidth, innerHeight, BLACK); // clear the part after the current rect width
		}

		if (rpmPercent >= rpmRedLineSetting)
		{
			gfx->fillRect(x, yPlusOne, meterWidth - 2, innerHeight, RED);
		}
		else if (rpmPercent >= rpmRedLineSetting - 5)
		{
			gfx->fillRect(x, yPlusOne, meterWidth - 2, innerHeight, ORANGE);
		}
		else
		{
			gfx->fillRect(x, yPlusOne, meterWidth - 2, innerHeight, GREEN);
		}

		// draw the frame only if it's not there
		if (prev_rpmPercent == 50) gfx->drawRect(x, y, width, height-2, WHITE);

		prev_rpmPercent = rpmPercent;
	}

	void drawCell(int32_t x, int32_t y, String data, String id, String name = "Data", String align = "center", int32_t color = WHITE, int fontSize = 3)
	{
		if (cellTitleHeight == 0) {
			gfx->setTextSize(2);
			int16_t x1 = 0;
			int16_t y1 = 0;
			uint16_t width = 0;
			uint16_t height = 0;
			gfx->getTextBounds(name, 0, 0, &x1, &y1, &width, &height);
			cellTitleHeight = height;
		}
		const static int hPadding = 5;
		const static int vPadding = 4;
		const static int titleAreaHeight = cellTitleHeight + 8;

		gfx->setTextColor(color, BLACK);

		const bool dataChanged =  (prevData[id] != data);
		const bool colorChanged =  (prevColor[id] != color);

		if (dataChanged) {

			if (align == "left")
			{

				if (colorChanged) gfx->drawRoundRect(x, y, CELL_WIDTH * 2 - 1, CELL_HEIGHT - 2, 4, color);		// Rectangle
				if (colorChanged) drawString(name, x + hPadding, y + vPadding, 2, gfx);						// Title
				drawString(data, x + hPadding, y + titleAreaHeight, fontSize, gfx); // Data
			}
			else if (align == "right")
			{
				if (colorChanged) gfx->drawRoundRect(x - (CELL_WIDTH * 2), y, CELL_WIDTH * 2 - 1, CELL_HEIGHT - 2, 5, color); // Rectangle
				if (colorChanged) drawRightString(name, x - hPadding, y + vPadding, 2, gfx);						// Title
				drawRightString(data, x - hPadding, y + titleAreaHeight, fontSize, gfx);	  // Data
			}
			else // "center"
			{
				if (colorChanged) gfx->drawRoundRect(x, y, CELL_WIDTH - 2, CELL_HEIGHT - 2, 5, color);	 // Rectangle
				if (colorChanged) drawCentreString(name, x + HALF_CELL_WIDTH, y + vPadding, 2, gfx);			 // Title
				drawCentreString(data, x + HALF_CELL_WIDTH, y + titleAreaHeight, fontSize, gfx); // Data
			}

			// Clean the previous data if it was wider
			if (prevData[id].length() > data.length())
			{
				// variables where we will store the results of getTextBounds
				int16_t x1 = 0;
				int16_t y1 = 0;
				uint16_t width = 0;
				uint16_t height = 0;

				auto dataY = y + titleAreaHeight;
				// calculate the size of the rectangle to "clear"
				gfx->getTextBounds(prevData[id], x, dataY, &x1, &y1, &width, &height);

				// depending on the datum of our text, we need to adjust the coordinates, because our text
				//  has different boundaries
				if (align == "left")
				{
					clearTextArea(x + hPadding, dataY, width, height, Datum::left_top, gfx);
				}
				else if (align == "right")
				{
					clearTextArea(x - hPadding, dataY, width, height, Datum::right_top, gfx);
				}
				else
				{
					clearTextArea(x + HALF_CELL_WIDTH, dataY, width, height, Datum::center_top, gfx);
				}
			}

			prevData[id] = data;
			prevColor[id] = color;
		}

	}

	// Helper function to validate alert strings - prevents garbage data display
	bool isValidAlertString(const String &str) {
		String normalized = str;
		normalized.trim();
		String upper = normalized;
		upper.toUpperCase();

		// Check for valid alert strings only
		if (upper == "ENGINE OFF" || upper == "PIT LIMITER" || upper == "YELLOW FLAG" ||
			upper == "BLUE FLAG" || upper == "LOW FUEL" || upper == "BLACK FLAG" ||
			upper == "MEATBALL" || upper == "SLOW CAR" || upper == "GREEN FLAG" ||
			upper == "FINISHED" || upper == "RED FLAG") {
			return true;
		}
		// Accept control-change popups even without values
		// Examples: "BIAS", "TC LEVEL", "ABS LEVEL", "MAP"
		if (upper == "BIAS" || upper.startsWith("BIAS") ||
			upper.indexOf("TC LEVEL") >= 0 ||
			upper.indexOf("ABS LEVEL") >= 0 ||
			upper.indexOf("MAP") >= 0) {
			return true;
		}
		// Check for pop-up messages (contain colon for labels)
		if (upper.indexOf(':') > 0) {
			return true;
		}
		return false;
	}

	// Remove "LEVEL" suffix from alert text for cleaner display
	String cleanAlertText(const String &text) {
		String result = text;
		String upper = text;
		upper.toUpperCase();

		// Remove " LEVEL" if present (case insensitive)
		// int idx = upper.lastIndexOf(" LEVEL");
		// if (idx >= 0) {
		// 	result = result.substring(0, idx);
		// }

		if (result.indexOf('LEVEL') >= 0) {
			// Replace ": " with "\n" to put value on new line
			result.replace(" LEVEL", "");
		}

		if (result.indexOf('FLAG') >= 0) {
			// Replace ": " with "\n" to put value on new line
			result.replace(" FLAG", "");
		}

		// // Format "TC: 3" or "ABS: 5" as "TC\n3" or "ABS\n5" (label on first line, value on second)
		// // This improves readability with large text
		if (result.indexOf(':') >= 0) {
			// Replace ": " with "\n" to put value on new line
			result.replace(": ", "\n\n");
		}

		return result;
	}

	// Draw alerts/flags in the center of the screen
	void drawAlert() {
		if (!canUseDisplay()) return;

		// Check if we should show an alert
		bool shouldShowAlert = false;
		String alertText = "";
		uint16_t bgColor = BLACK;
		uint16_t textColor = WHITE;
		String alertNormalized = alertMessage;  // keep raw for display, normalized for checks
		alertNormalized.trim();
		String alertUpper = alertNormalized;
		alertUpper.toUpperCase();
		String popupNormalized = popupMessage;
		popupNormalized.trim();
		String popupUpper = popupNormalized;
		popupUpper.toUpperCase();

		// PRIORIDADE 1: Alertas críticos do SimHub (alertMessage)
		// Only show if it's a real alert (not empty, not "NORMAL", not "NONE", not "0")
		// Additional safety: check if string contains only valid characters (alphanumeric, space, colon, etc)
		if (alertUpper.length() > 0 &&
			alertUpper != "NORMAL" &&
			alertUpper != "NONE" &&
			alertUpper != "0" &&
			isValidAlertString(alertUpper)) {
			shouldShowAlert = true;
			alertStartTime = millis();
			alertText = cleanAlertText(alertNormalized);

			if (alertUpper.indexOf("ENGINE OFF") >= 0) {
				bgColor = RGB565(20, 20, 20);
				textColor = RED;
			} else if (alertUpper.indexOf("PIT LIMITER") >= 0) {
				bgColor = ORANGE;
				textColor = BLACK;
			} else if (alertUpper.indexOf("YELLOW FLAG") >= 0) {
				bgColor = YELLOW;
				textColor = BLACK;
			} else if (alertUpper.indexOf("BLUE FLAG") >= 0) {
				bgColor = BLUE;
				textColor = WHITE;
			} else if (alertUpper.indexOf("GREEN FLAG") >= 0 || alertUpper.indexOf("GREEN") >= 0) {
				bgColor = GREEN;  // Pure green RGB565
				textColor = BLACK;
			} else if (alertUpper.indexOf("LOW FUEL") >= 0) {
				bgColor = RED;
				textColor = YELLOW;
			} else {
				bgColor = MAGENTA;
				textColor = WHITE;
			}
		}

		// PRIORIDADE 2: Pop-up temporário (popupMessage) - mensagens de mudanças menores
		// Only show if SimHub popup passes validation (must contain ':' like "BIAS: 54.0")
		// OR if it came from the ButtonBox via UART (popupFromUart bypasses validation)
		if (popupUpper.length() > 0 &&
			popupUpper != "NORMAL" &&
			popupUpper != "NONE" &&
			popupUpper != "0" &&
			(popupFromUart || isValidAlertString(popupUpper))) {
			// Override any existing alert for control changes (ABS/TC/BIAS/MAP)
			// Popups are short-lived and should take precedence over flags
			shouldShowAlert = true;
			alertStartTime = millis();
			alertText = cleanAlertText(popupNormalized);

			// Color for popup based on popup content (not alert!)
			if (popupUpper.indexOf("ENGINE OFF") >= 0) {
				bgColor = RGB565(20, 20, 20);
				textColor = RED;
			} else if (popupUpper.indexOf("PIT LIMITER") >= 0) {
				bgColor = ORANGE;
				textColor = BLACK;
			} else if (popupUpper.indexOf("YELLOW FLAG") >= 0) {
				bgColor = YELLOW;
				textColor = BLACK;
			} else if (popupUpper.indexOf("BLUE FLAG") >= 0) {
				bgColor = BLUE;
				textColor = WHITE;
			} else if (popupUpper.indexOf("GREEN FLAG") >= 0 || popupUpper.indexOf("GREEN") >= 0) {
				bgColor = GREEN;  // Pure green RGB565
				textColor = BLACK;
			} else if (popupUpper.indexOf("LOW FUEL") >= 0) {
				bgColor = RED;
				textColor = YELLOW;
			} else {
				bgColor = RGB565(50, 50, 100);  // Dark blue background
				textColor = YELLOW;
			}
		}

		// Check for flag changes - TODAS AS BANDEIRAS SUPORTADAS
		// Only process if flag is not "None" and not empty
		String flagValue = currentFlag;
		flagValue.trim();
		if (flagValue != prevFlag && flagValue != "None" && flagValue != "" && alertText.length() == 0) {
			shouldShowAlert = true;
			alertStartTime = millis();
			prevFlag = flagValue;

			// 🔵 Blue Flag
			if (flagValue.equalsIgnoreCase("Blue")) {
				alertText = "BLUE FLAG";
				bgColor = BLUE;
				textColor = WHITE;
			}
			// 🟡 Yellow Flag
			else if (flagValue.equalsIgnoreCase("Yellow")) {
				alertText = "YELLOW FLAG";
				bgColor = YELLOW;
				textColor = BLACK;
			}
			// ⚫ Black Flag
			else if (flagValue.equalsIgnoreCase("Black")) {
				alertText = "BLACK FLAG";
				bgColor = RGB565(20, 20, 20);
				textColor = WHITE;
			}
			// 🟠 Orange Flag (Meatball)
			else if (flagValue.equalsIgnoreCase("Orange")) {
				alertText = "MEATBALL";
				bgColor = ORANGE;
				textColor = BLACK;
			}
			// ⚪ White Flag (Slow Car)
			else if (flagValue.equalsIgnoreCase("White")) {
				alertText = "SLOW CAR";
				bgColor = WHITE;
				textColor = BLACK;
			}
			// 🟢 Green Flag
			else if (flagValue.equalsIgnoreCase("Green")) {
				alertText = "GREEN FLAG";
				// bgColor = BLUE;
				bgColor = GREEN;  // Pure green RGB565 (R=0, G=31, B=0)
				textColor = BLACK;
			}
			// 🏁 Checkered Flag (Finished)
			else if (flagValue.equalsIgnoreCase("Checkered")) {
				alertText = "FINISHED";
				bgColor = WHITE;
				textColor = BLACK;
			}
			// 🔴 Red Flag
			else if (flagValue.equalsIgnoreCase("Red")) {
				alertText = "RED FLAG";
				bgColor = RED;
				textColor = WHITE;
			}
		}

		// Check for penalty changes (fallback)
		if (currentPenalties != prevPenalties && currentPenalties.toInt() > 0 && alertText.length() == 0) {
			shouldShowAlert = true;
			alertStartTime = millis();
			prevPenalties = currentPenalties;
			alertText = "PENALTY: " + currentPenalties;
			bgColor = RGB565(200, 0, 0);  // Dark red
			textColor = WHITE;
		}

		// Check if alert should still be displayed (3 second duration)
		// If a new alert was triggered, reset the timer
		unsigned long elapsedTime = millis() - alertStartTime;
		bool showingNow = (elapsedTime < ALERT_DURATION_MS) && (alertText.length() > 0);

		if (showingNow) {
            if (alertText.length() > 0) {
                // ... (código anterior de contagem de linhas igual) ...
                int lineCount = 1;
                for (int i = 0; i < alertText.length(); i++) {
                    if (alertText[i] == '\n') lineCount++;
                }

                int lineHeight = 50;
                int totalTextHeight = lineHeight * lineCount;

                // --- AJUSTE DE POSICIONAMENTO ---

                // 1. Defina a altura da sua barra de telemetria (chutei 50px pela foto)
                int bottomBarHeight = 0;

                // 2. A altura disponível para o alerta é a tela inteira MENOS a barra
                int availableScreenHeight = SCREEN_HEIGHT - bottomBarHeight;

                // 3. Ajustei a altura do box para ser menor que a área disponível
                // (280 era muito grande, deixei 260 para ter respiro em cima e embaixo)
                int alertHeight = 260;

                int alertWidth = SCREEN_WIDTH;
                int alertX = 0;

                // 4. O cálculo do Y agora é baseado na availableScreenHeight
                int alertY = ((availableScreenHeight - alertHeight) / 2)+30;

                // --- FIM DO AJUSTE ---

                // Desenha o fundo
                gfx->fillRect(alertX, alertY, alertWidth, alertHeight, bgColor);

                gfx->setTextColor(textColor);
                gfx->setTextSize(6); // Mantive grande

                int16_t x1, y1;
                uint16_t w, h;

                // Centraliza o texto dentro do novo box menor
                // Adicionei um ajuste (-5) para correção visual da fonte
                int currentY = alertY + (alertHeight - totalTextHeight) / 2 - 5;

                String remainingText = alertText;

                while (remainingText.length() > 0) {
                    // ... (lógica de quebra de linha igual ao anterior) ...
                    int newlinePos = remainingText.indexOf('\n');
                    String line;
                    if (newlinePos >= 0) {
                        line = remainingText.substring(0, newlinePos);
                        remainingText = remainingText.substring(newlinePos + 1);
                    } else {
                        line = remainingText;
                        remainingText = "";
                    }

                    gfx->getTextBounds(line, 0, 0, &x1, &y1, &w, &h);
                    int centerX = (SCREEN_WIDTH - w) / 2;

                    // --- EFEITO NEGRITO (FAUX BOLD) ---
                    // Imprime 3 vezes com leve deslocamento para engrossar a letra

                    // 1. Camada mais grossa (deslocada 2px)
                    gfx->setCursor(centerX + 2, currentY);
                    gfx->print(line);

                    // 2. Camada intermediária (deslocada 1px)
                    gfx->setCursor(centerX + 1, currentY);
                    gfx->print(line);

                    // 3. Camada principal (posição original)
                    gfx->setCursor(centerX, currentY);
                    gfx->print(line);
                    // ----------------------------------

                    currentY += lineHeight;
                }

                alertWasShowing = true;
            }
        } else if (alertWasShowing) {
			// Alert just expired - set flag to trigger full redraw in next draw() cycle
			alertWasShowing = false;
			needsFullRedraw = true;
		}
	}
};
#endif
