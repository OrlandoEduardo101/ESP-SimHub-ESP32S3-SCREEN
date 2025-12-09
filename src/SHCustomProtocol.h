#ifndef __SHCUSTOMPROTOCOL_H__
#define __SHCUSTOMPROTOCOL_H__

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <map>
#include "logo_image.h"  // Logo image array
#include <Wire.h>
#include <Wire.h>

// Forward declaration for screenLog
extern void screenLog(const String &msg);

// Forward declaration for debugLog
extern void debugLog(const String &msg);

// WT32-SC01 Plus - ST7796 via 8-bit MCU (8080) parallel interface (320x480)
// IMPORTANT: WT32-SC01 Plus uses 8-bit parallel interface, NOT SPI!
// Pinout according to official WT32-SC01 Plus documentation:
// https://github.com/Cesarbautista10/WT32-SC01-Plus-ESP32
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
Arduino_ST7796 *gfx = nullptr;

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
	int rpmRedLineSetting = 90;
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
	String tcLevel = "0";
	String tcActive = "0";
	String absLevel = "0";
	String absActive = "0";
	String isTCCutNull = "True";
	String tcTcCut = "0  0";
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
	static const unsigned long ALERT_DURATION_MS = 3000;  // Show alert for 3 seconds

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

	int cellTitleHeight = 0;
	bool hasReceivedData = false;
	bool displayEnabled = true;  // Display enabled for dashboard
	bool loadingScreenShown = false;  // Track if loading screen has been shown
	bool touchInitAttempted = false;  // Track if we've already tried to init touch
	
	// Multi-page dashboard variables
	enum DashboardPage { 
		PAGE_RACE = 0, 
		PAGE_TIMING = 1, 
		PAGE_TELEMETRY = 2,
		PAGE_ADVANCED = 3,       // NEW: Advanced telemetry (Motor, Wear, Env, DRS, KERS, Turbo)
		PAGE_RELATIVE = 4,       // NEW: Relative/Head-to-head
		PAGE_LAPS = 5,           // NEW: Laps/Sectors analysis
		PAGE_LEADERBOARD = 6     // NEW: Leaderboard (placeholder)
	};
	DashboardPage currentPage = PAGE_RACE;
	DashboardPage lastPage = PAGE_RACE;  // Track previous page to detect page changes
	unsigned long lastTouchTime = 0;
	static const unsigned long TOUCH_DEBOUNCE_MS = 500;  // Debounce time between page changes

	// Helper function to safely use display
	bool canUseDisplay() {
		return displayEnabled && gfx != nullptr;
	}
	
	// Reset draw cache when changing pages
	void resetDrawCache() {
		prev_gear = "";  // Force gear redraw
		prev_rpmPercent = -1;  // Force RPM redraw
		// Clear prevData map to force all cells to redraw
		prevData.clear();
		prevColor.clear();
	}
	
	// Navigate to next page
	void nextPage() {
		currentPage = (DashboardPage)((currentPage + 1) % 7);
		gfx->fillScreen(BLACK);
		resetDrawCache();
	}
	
	// Navigate to previous page
	void prevPage() {
		currentPage = (DashboardPage)((currentPage - 1 + 7) % 7);
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
				gfx = new Arduino_ST7796(bus, TFT_RST, 1 /* rotation = landscape */, true /* IPS */);
			}

			// Initialize backlight first - GPIO 45 according to WT32-SC01 Plus documentation
		#ifdef TFT_BL
			if (TFT_BL >= 0 && TFT_BL < 48) {  // ESP32-S3 has GPIOs 0-48
				pinMode(TFT_BL, OUTPUT);
				digitalWrite(TFT_BL, LOW);  // Start with backlight off
				delay(10);
				Serial.print("Backlight pin configured: GPIO ");
				Serial.println(TFT_BL);
			}
		#endif

			// Initialize display with error handling
			if (gfx != nullptr && bus != nullptr) {
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
				bool displayOk = gfx->begin();
				if (!displayOk) {
					Serial.println("ERROR: Failed to initialize display!");
					displayEnabled = false;
					return;
				}
				Serial.println("Display controller initialized");
				Serial.flush();

				delay(300);  // Give display time to stabilize

				// Turn on backlight after display is ready
		#ifdef TFT_BL
				if (TFT_BL >= 0 && TFT_BL < 48) {
					digitalWrite(TFT_BL, HIGH);
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
		String tyreTemperatureFrontLeft = FlowSerialReadStringUntil(';');
		String tyreTemperatureFrontRight = FlowSerialReadStringUntil(';');
		String tyreTemperatureRearLeft = FlowSerialReadStringUntil(';');
		String tyreTemperatureRearRight = FlowSerialReadStringUntil(';');
		// Temperatura dos freios
		String brakeTemperatureFrontLeft = FlowSerialReadStringUntil(';');
		String brakeTemperatureFrontRight = FlowSerialReadStringUntil(';');
		String brakeTemperatureRearLeft = FlowSerialReadStringUntil(';');
		String brakeTemperatureRearRight = FlowSerialReadStringUntil(';');
		// Motor
		String oilTemperature = FlowSerialReadStringUntil(';');
		String waterTemperature = FlowSerialReadStringUntil(';');

		// BLOCO 4: Eletrônica (índices 25-31)
		tcLevel = FlowSerialReadStringUntil(';');
		tcActive = FlowSerialReadStringUntil(';');
		absLevel = FlowSerialReadStringUntil(';');
		absActive = FlowSerialReadStringUntil(';');
		isTCCutNull = FlowSerialReadStringUntil(';');
		tcTcCut = FlowSerialReadStringUntil(';');
		brakeBias = FlowSerialReadStringUntil(';');
		brake = FlowSerialReadStringUntil(';');

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
		
		// Debug: Log flag value (raw)
		debugLog(String("[RAW] idx39 flag: '") + currentFlag + "'");
		debugLog(String("[SHCustomProtocol] Flag value received [39]: '") + currentFlag + 
				 String("' (len=") + currentFlag.length() + ")");

		// BLOCO 6: Mensagens e Alertas (índices 42-43)
		alertMessage = FlowSerialReadStringUntil(';');
		popupMessage = FlowSerialReadStringUntil(';');
		alertMessage.trim();
		popupMessage.trim();

		// Debug: window around flag/alert (indices 38-43)
		debugLog(String("[CHECK idx38-43] time=") + sessionTimeLeft +
		         " | flag=" + currentFlag +
		         " | pen=" + currentPenalties +
		         " | cut=" + cutTrackWarnings +
		         " | alert=" + alertMessage +
		         " | popup=" + popupMessage);
		
		// Debug: Log alert and popup raw values
		debugLog(String("[RAW] idx42 alert: '") + alertMessage + "'");
		debugLog(String("[SHCustomProtocol] Alert [42]: '") + alertMessage + 
				 String("' (len=") + alertMessage.length() + ")");
		debugLog(String("[SHCustomProtocol] Popup [43]: '") + popupMessage + 
				 String("' (len=") + popupMessage.length() + ")");

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
		turboBoost = FlowSerialReadStringUntil('\n');  // Último campo (índice 61)

		// Build raw packet log (indices 0-61)
		String rawPacket; rawPacket.reserve(512);
		auto appendField = [&](const String &field) { rawPacket += field; rawPacket += ';'; };
		appendField(speed);
		appendField(gear);
		appendField(String(rpmPercent));
		appendField(String(rpmRedLineSetting));
		appendField(rpmsStr);
		appendField(currentLapTime);
		appendField(lastLapTime);
		appendField(bestLapTime);
		appendField(sessionBestLiveDeltaSeconds);
		appendField(sessionBestLiveDeltaProgressSeconds);
		appendField(lapInvalidatedStr);
		appendField(tyrePressureFrontLeft);
		appendField(tyrePressureFrontRight);
		appendField(tyrePressureRearLeft);
		appendField(tyrePressureRearRight);
		appendField(tyreTemperatureFrontLeft);
		appendField(tyreTemperatureFrontRight);
		appendField(tyreTemperatureRearLeft);
		appendField(tyreTemperatureRearRight);
		appendField(brakeTemperatureFrontLeft);
		appendField(brakeTemperatureFrontRight);
		appendField(brakeTemperatureRearLeft);
		appendField(brakeTemperatureRearRight);
		appendField(oilTemperature);
		appendField(waterTemperature);
		appendField(tcLevel);
		appendField(tcActive);
		appendField(absLevel);
		appendField(absActive);
		appendField(isTCCutNull);
		appendField(tcTcCut);
		appendField(brakeBias);
		appendField(brake);
		appendField(position);
		appendField(opponentsCount);
		appendField(driverAheadGap);
		appendField(driverBehindGap);
		appendField(fuelRemainingLaps);
		appendField(fuelLitersPerLap);
		appendField(sessionTimeLeft);
		appendField(currentFlag);
		appendField(currentPenalties);
		appendField(cutTrackWarnings);
		appendField(alertMessage);
		appendField(popupMessage);
		appendField(rpmPercent2);
		appendField(spotterLeft);
		appendField(spotterRight);
		appendField(absActive2);
		appendField(tyreWearFrontLeft);
		appendField(tyreWearFrontRight);
		appendField(tyreWearRearLeft);
		appendField(tyreWearRearRight);
		appendField(sector1Time);
		appendField(sector2Time);
		appendField(sector3Time);
		appendField(airTemperature);
		appendField(roadTemperature);
		appendField(shiftLightTrigger);
		appendField(drsAvailable);
		appendField(drsActive);
		appendField(kersLevel);
		appendField(turboBoost);
		if (rawPacket.length() > 0) rawPacket.remove(rawPacket.length() - 1); // remove last ';'
		debugLog(String("[RAW] packet: ") + rawPacket);
		

		// Re-parse critical indices from rawPacket to avoid any misalignment
		String fields[62];
		int fieldIdx = 0;
		int last = 0;
		for (int i = 0; i <= rawPacket.length(); i++) {
			if (i == rawPacket.length() || rawPacket.charAt(i) == ';') {
				if (fieldIdx < 62) {
					fields[fieldIdx] = rawPacket.substring(last, i);
				}
				fieldIdx++;
				last = i + 1;
			}
		}
		// Only override if we got at least up to index 43
		if (fieldIdx > 43) {
			String sTime = fields[38];
			String sFlag = fields[39];
			String sPen = fields[40];
			String sCut = fields[41];
			String sAlert = fields[42];
			String sPopup = fields[43];
			sTime.trim(); sFlag.trim(); sPen.trim(); sCut.trim(); sAlert.trim(); sPopup.trim();
			sessionTimeLeft = sTime;
			currentFlag = sFlag;
			currentPenalties = sPen;
			cutTrackWarnings = sCut;
			alertMessage = sAlert;
			popupMessage = sPopup;
			debugLog(String("[REPARSE idx38-43] time=") + sessionTimeLeft +
			         " | flag=" + currentFlag +
			         " | pen=" + currentPenalties +
			         " | cut=" + cutTrackWarnings +
			         " | alert=" + alertMessage +
			         " | popup=" + popupMessage);
		}

		// Debug: Log complete packet received
		debugLog(String("[SHCustomProtocol] Telemetry packet complete - Speed: ") + speed + 
				 String(" | Alert: ") + (alertMessage != "NORMAL" ? alertMessage : "none") +
				 String(" | Flag: ") + currentFlag);
	}

	// Called once per arduino loop, timing can't be predicted,
	// but it's called between each command sent to the arduino
	void loop() {
		// Initialize touch right after loading screen is shown (before SimHub data arrives)
		if (!touchInitAttempted && loadingScreenShown) {
			touchInitAttempted = true;
			initializeTouch();
		}
		
		// Check for touch input to change pages
		if (touchInitialized && hasReceivedData) {
			TouchPoint touch = readTouch();
			if (touch.touched && (millis() - lastTouchTime) > TOUCH_DEBOUNCE_MS) {
				lastTouchTime = millis();
				
				// Left half = previous page, Right half = next page
				if (touch.x < SCREEN_WIDTH / 2) {
					// Previous page (left swipe)
					currentPage = (DashboardPage)((currentPage - 1 + 6) % 6);
					gfx->fillScreen(BLACK);  // Clear screen for new page
				} else {
					// Next page (right swipe)
					currentPage = (DashboardPage)((currentPage + 1) % 6);
					gfx->fillScreen(BLACK);  // Clear screen for new page
				}
				
				// Reset draw cache when page changes to force complete redraw
				if (currentPage != lastPage) {
					resetDrawCache();
					lastPage = currentPage;
				}
			}
		}
		
		// Detect page changes (for cases other than touch)
		if (currentPage != lastPage) {
			resetDrawCache();
			lastPage = currentPage;
		}
		
		if (!hasReceivedData) {
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
			case PAGE_LEADERBOARD:
				drawLeaderboardPageContent();
				break;
		}
		
		// Draw alerts (flags, penalties, etc.) on top of everything
		drawAlert();
		
		// Draw page indicator at bottom
		drawPageIndicator();
	}
	
	void drawPageIndicator() {
		// Draw small page indicator dots at bottom center (7 pages)
		// Positioned in dedicated padding area at bottom
		int dotRadius = 2;
		int dotSpacing = 8;
		int totalWidth = (7 - 1) * dotSpacing + (dotRadius * 2);
		int startX = (SCREEN_WIDTH - totalWidth) / 2;  // Center horizontally
		int startY = SCREEN_HEIGHT + 41;  // 8px from bottom margin (colado na borda)
		
		for (int i = 0; i < 7; i++) {
			uint16_t color = (i == currentPage) ? WHITE : RGB565(100, 100, 100);
			gfx->fillCircle(startX + (i * dotSpacing), startY, dotRadius, color);
		}
	}
	
	void drawRacePageContent() {
		// Original dashboard content
		drawRpmMeter(0, 0, SCREEN_WIDTH, CELL_HEIGHT);
		// this takes 2 cells in height, hence CELL_HEIGHT is the half point
		drawGear(COL[2] + HALF_CELL_WIDTH, ROW[1] + CELL_HEIGHT);

		// First Column (Lap times + Position)
		drawCell(COL[0], ROW[1], bestLapTime, "bestLapTime", "Best Lap", "left");
		drawCell(COL[0], ROW[2], lastLapTime, "lastLapTime", "Last Lap", "left");
		drawCell(COL[0], ROW[3], currentLapTime, "currenLapTime", "Current Lap", "left", lapInvalidated == "True" ? RED : WHITE);
		// NEW: Position
		drawCell(COL[0], ROW[4], position + "/" + opponentsCount, "position", "POS", "left", WHITE);

		// Second Column (Speed + Gaps)
		drawCell(COL[2], ROW[2], speed, "speed", "Speed", "center");
		// NEW: Gaps to adjacent drivers
		drawCell(COL[2], ROW[3], driverAheadGap, "driverAheadGap", "Gap+", "center", WHITE);
		drawCell(COL[2], ROW[4], driverBehindGap, "driverBehindGap", "Gap-", "center", WHITE);

		// Third Column (delta)
		drawCell(SCREEN_WIDTH, ROW[1], sessionBestLiveDeltaSeconds, "sessionBestLiveDeltaSeconds", "Delta", "right", sessionBestLiveDeltaSeconds.indexOf('-') >= 0 ? GREEN : RED);
		drawCell(SCREEN_WIDTH, ROW[2], sessionBestLiveDeltaProgressSeconds, "sessionBestLiveDeltaProgressSeconds", "Delta P", "right", sessionBestLiveDeltaProgressSeconds.indexOf('-') >= 0 ? GREEN : RED);
		
		// NEW: Combustível
		drawCell(SCREEN_WIDTH, ROW[3], fuelRemainingLaps, "fuelRemainingLaps", "Fuel L", "right", fuelRemainingLaps.toFloat() < 2 ? RED : WHITE);
		// NEW: DRS
		uint16_t drsColor = WHITE;
		if (drsActive == "1") drsColor = BLUE;
		else if (drsAvailable == "1") drsColor = GREEN;
		drawCell(SCREEN_WIDTH, ROW[4], drsActive == "1" ? "OPEN" : (drsAvailable == "1" ? "AVAIL" : "CLOSED"), "drsStatus", "DRS", "right", drsColor);

		// Bottom row (TC, ABS, BB)
		if (isTCCutNull == "False") {
			drawCell(COL[0], ROW[4], tcTcCut, "tcTcCut", "TC TC2", "center", YELLOW);
		} else {
			drawCell(COL[1], ROW[4], tcLevel, "tcLevel", "TC", "center", YELLOW);
		}
		drawCell(COL[1], ROW[4], absLevel, "absLevel", "ABS", "center", BLUE);
		drawCell(COL[2], ROW[4], brakeBias, "brakeBias", "BB", "center", MAGENTA);

		// Tyre pressure (unchanged)
		drawCell(COL[3], ROW[3], tyrePressureFrontLeft, "tyrePressureFrontLeft", "FL", "center", CYAN);
		drawCell(COL[4], ROW[3], tyrePressureFrontRight, "tyrePressureFrontRight", "FR", "center", CYAN);
		drawCell(COL[3], ROW[4], tyrePressureRearLeft, "tyrePressureRearLeft", "RL", "center", CYAN);
		drawCell(COL[4], ROW[4], tyrePressureRearRight, "tyrePressureRearRight", "RR", "center", CYAN);
	}
	
	void drawTimingPageContent() {
		// PAGE 2: Timing/Lap Analysis - with visual layout (full width)
		gfx->fillRect(0, 0, SCREEN_WIDTH, 40, RGB565(20, 20, 60));  // Header background
		gfx->setTextColor(YELLOW);
		gfx->setTextSize(3);
		gfx->setCursor(10, 10);
		gfx->print("TIMING");
		
		// Best lap box
		gfx->drawRect(5, 55, SCREEN_WIDTH - 10, 70, YELLOW);
		gfx->fillRect(5, 55, SCREEN_WIDTH - 10, 25, YELLOW);
		gfx->setTextColor(BLACK);
		gfx->setTextSize(1);
		gfx->setCursor(10, 62);
		gfx->print("BEST LAP");
		gfx->setTextColor(YELLOW);
		gfx->setTextSize(2);
		gfx->setCursor(20, 82);
		gfx->print(bestLapTime);
		
		// Last lap box
		gfx->drawRect(5, 135, SCREEN_WIDTH - 10, 70, CYAN);
		gfx->fillRect(5, 135, SCREEN_WIDTH - 10, 25, CYAN);
		gfx->setTextColor(BLACK);
		gfx->setTextSize(1);
		gfx->setCursor(10, 142);
		gfx->print("LAST LAP");
		gfx->setTextColor(CYAN);
		gfx->setTextSize(2);
		gfx->setCursor(20, 162);
		gfx->print(lastLapTime);
		
		// Current lap box
		gfx->drawRect(5, 215, SCREEN_WIDTH - 10, 55, GREEN);
		gfx->fillRect(5, 215, SCREEN_WIDTH - 10, 25, GREEN);
		gfx->setTextColor(BLACK);
		gfx->setTextSize(1);
		gfx->setCursor(10, 222);
		gfx->print("CURRENT LAP");
		gfx->setTextColor(GREEN);
		gfx->setTextSize(2);
		gfx->setCursor(20, 242);
		gfx->print(currentLapTime);
		
		// Delta indicator on the right
		gfx->drawRect(SCREEN_WIDTH - 75, 55, 70, 215, WHITE);
		gfx->fillRect(SCREEN_WIDTH - 75, 55, 70, 25, sessionBestLiveDeltaSeconds.indexOf('-') >= 0 ? GREEN : RED);
		gfx->setTextColor(BLACK);
		gfx->setTextSize(1);
		gfx->setCursor(SCREEN_WIDTH - 70, 62);
		gfx->print("DELTA");
		gfx->setTextColor(WHITE);
		gfx->setTextSize(1);
		gfx->setCursor(SCREEN_WIDTH - 70, 90);
		gfx->print(sessionBestLiveDeltaSeconds);
	}
	
	void drawTelemetryPageContent() {
		// PAGE 3: Telemetry/Vehicle Status - with visual boxes (full width)
		gfx->fillRect(0, 0, SCREEN_WIDTH, 40, RGB565(60, 20, 20));  // Header background
		gfx->setTextColor(MAGENTA);
		gfx->setTextSize(3);
		gfx->setCursor(10, 10);
		gfx->print("TELEMETRY");
		
		// Speed - Large display (left side)
		int speedBoxWidth = (SCREEN_WIDTH - 20) / 2;
		gfx->drawRect(5, 55, speedBoxWidth, 80, YELLOW);
		gfx->fillRect(5, 55, speedBoxWidth, 25, YELLOW);
		gfx->setTextColor(BLACK);
		gfx->setTextSize(1);
		gfx->setCursor(10, 62);
		gfx->print("SPEED");
		gfx->setTextColor(YELLOW);
		gfx->setTextSize(3);
		gfx->setCursor(20, 75);
		gfx->print(speed);
		
		// TC/ABS indicators side by side (right side)
		int tcBoxWidth = (SCREEN_WIDTH - 25) / 2;
		// TC Box
		gfx->drawRect(5 + speedBoxWidth + 5, 55, tcBoxWidth, 35, ORANGE);
		gfx->fillRect(5 + speedBoxWidth + 5, 55, tcBoxWidth, 20, ORANGE);
		gfx->setTextColor(BLACK);
		gfx->setTextSize(1);
		gfx->setCursor(10 + speedBoxWidth + 5, 62);
		gfx->print("TC");
		gfx->setTextColor(ORANGE);
		gfx->setTextSize(2);
		gfx->setCursor(20 + speedBoxWidth + 5, 70);
		gfx->print(tcLevel);
		
		// ABS Box
		gfx->drawRect(5 + speedBoxWidth + 5, 95, tcBoxWidth, 35, BLUE);
		gfx->fillRect(5 + speedBoxWidth + 5, 95, tcBoxWidth, 20, BLUE);
		gfx->setTextColor(BLACK);
		gfx->setTextSize(1);
		gfx->setCursor(10 + speedBoxWidth + 5, 102);
		gfx->print("ABS");
		gfx->setTextColor(BLUE);
		gfx->setTextSize(2);
		gfx->setCursor(20 + speedBoxWidth + 5, 110);
		gfx->print(absLevel);
		
		// Brake Box (full width)
		gfx->drawRect(5, 145, SCREEN_WIDTH - 10, 50, RED);
		gfx->fillRect(5, 145, SCREEN_WIDTH - 10, 25, RED);
		gfx->setTextColor(BLACK);
		gfx->setTextSize(1);
		gfx->setCursor(10, 152);
		gfx->print("BRAKE PRESSURE");
		gfx->setTextColor(RED);
		gfx->setTextSize(2);
		gfx->setCursor(30, 165);
		gfx->print(brake);
		gfx->print("%");
		
		// Tyre pressures grid (full width, split in 2)
		int tyreBoxWidth = (SCREEN_WIDTH - 15) / 2;
		gfx->drawRect(5, 205, tyreBoxWidth, 65, CYAN);
		gfx->fillRect(5, 205, tyreBoxWidth, 20, CYAN);
		gfx->setTextColor(BLACK);
		gfx->setTextSize(1);
		gfx->setCursor(10, 212);
		gfx->print("TYRE FL/RL");
		gfx->setTextColor(CYAN);
		gfx->setCursor(10, 230);
		gfx->print("FL: ");
		gfx->println(tyrePressureFrontLeft);
		gfx->setCursor(10, 245);
		gfx->print("RL: ");
		gfx->println(tyrePressureRearLeft);
		
		gfx->drawRect(5 + tyreBoxWidth + 5, 205, tyreBoxWidth, 65, CYAN);
		gfx->fillRect(5 + tyreBoxWidth + 5, 205, tyreBoxWidth, 20, CYAN);
		gfx->setTextColor(BLACK);
		gfx->setTextSize(1);
		gfx->setCursor(10 + tyreBoxWidth + 5, 212);
		gfx->print("TYRE FR/RR");
		gfx->setTextColor(CYAN);
		gfx->setCursor(10 + tyreBoxWidth + 5, 230);
		gfx->print("FR: ");
		gfx->println(tyrePressureFrontRight);
		gfx->setCursor(10 + tyreBoxWidth + 5, 245);
		gfx->print("RR: ");
		gfx->println(tyrePressureRearRight);
	}
	
	void drawAdvancedTelemetryPage() {
		// PAGE 3B: Advanced Telemetry - Motor, Wear, Environment, DRS, KERS, Turbo
		gfx->fillRect(0, 0, SCREEN_WIDTH, 40, RGB565(60, 20, 80));  // Header background (Purple)
		gfx->setTextColor(MAGENTA);
		gfx->setTextSize(3);
		gfx->setCursor(10, 10);
		gfx->print("ADVANCED");
		
		int boxHeight = 45;
		int boxWidth = (SCREEN_WIDTH - 15) / 2;
		int yPos = 55;
		
		// Row 1: Motor temperatures
		// Oil Temperature
		gfx->drawRect(5, yPos, boxWidth, boxHeight, WHITE);
		gfx->fillRect(5, yPos, boxWidth, 20, WHITE);
		gfx->setTextColor(BLACK);
		gfx->setTextSize(1);
		gfx->setCursor(10, yPos + 5);
		gfx->print("OIL TEMP");
		gfx->setTextColor(WHITE);
		gfx->setTextSize(2);
		gfx->setCursor(10, yPos + 25);
		gfx->print("--C");  // Placeholder - seria: oilTemperature + "C"
		
		// Water Temperature
		gfx->drawRect(5 + boxWidth + 5, yPos, boxWidth, boxHeight, WHITE);
		gfx->fillRect(5 + boxWidth + 5, yPos, boxWidth, 20, WHITE);
		gfx->setTextColor(BLACK);
		gfx->setTextSize(1);
		gfx->setCursor(10 + boxWidth + 5, yPos + 5);
		gfx->print("WATER TEMP");
		gfx->setTextColor(WHITE);
		gfx->setTextSize(2);
		gfx->setCursor(10 + boxWidth + 5, yPos + 25);
		gfx->print("--C");  // Placeholder
		
		yPos += boxHeight + 5;
		
		// Row 2: Tyre Wear
		gfx->drawRect(5, yPos, SCREEN_WIDTH - 10, boxHeight, RGB565(200, 100, 0));
		gfx->fillRect(5, yPos, SCREEN_WIDTH - 10, 20, RGB565(200, 100, 0));
		gfx->setTextColor(BLACK);
		gfx->setTextSize(1);
		gfx->setCursor(10, yPos + 5);
		gfx->print("TYRE WEAR | FL: 0% FR: 0% RL: 0% RR: 0%");  // Placeholder
		
		yPos += boxHeight + 5;
		
		// Row 3: Environment + DRS/KERS/Turbo
		// Air Temp
		gfx->drawRect(5, yPos, 60, boxHeight, RGB565(0, 100, 200));
		gfx->fillRect(5, yPos, 60, 20, RGB565(0, 100, 200));
		gfx->setTextColor(BLACK);
		gfx->setTextSize(1);
		gfx->setCursor(10, yPos + 5);
		gfx->print("AIR");
		gfx->setTextColor(RGB565(0, 150, 255));
		gfx->setTextSize(1);
		gfx->setCursor(10, yPos + 25);
		gfx->print("--C");
		
		// Road Temp
		gfx->drawRect(70, yPos, 60, boxHeight, RGB565(200, 0, 0));
		gfx->fillRect(70, yPos, 60, 20, RGB565(200, 0, 0));
		gfx->setTextColor(BLACK);
		gfx->setTextSize(1);
		gfx->setCursor(75, yPos + 5);
		gfx->print("ROAD");
		gfx->setTextColor(RED);
		gfx->setTextSize(1);
		gfx->setCursor(75, yPos + 25);
		gfx->print("--C");
		
		// DRS
		uint16_t drsBoxColor = drsActive == "1" ? BLUE : (drsAvailable == "1" ? GREEN : RGB565(50, 50, 50));
		gfx->drawRect(135, yPos, 60, boxHeight, drsBoxColor);
		gfx->fillRect(135, yPos, 60, 20, drsBoxColor);
		gfx->setTextColor(BLACK);
		gfx->setTextSize(1);
		gfx->setCursor(140, yPos + 5);
		gfx->print("DRS");
		gfx->setTextColor(drsBoxColor == BLUE ? BLUE : WHITE);
		gfx->setTextSize(1);
		gfx->setCursor(140, yPos + 25);
		gfx->print(drsActive == "1" ? "OPEN" : (drsAvailable == "1" ? "AVAIL" : "OFF"));
		
		// KERS
		uint16_t kersBarWidth = (String(kersLevel).toInt() * 40) / 100;  // 40px width for 100%
		gfx->drawRect(200, yPos, 60, boxHeight, MAGENTA);
		gfx->fillRect(200, yPos, 60, 20, MAGENTA);
		gfx->setTextColor(BLACK);
		gfx->setTextSize(1);
		gfx->setCursor(205, yPos + 5);
		gfx->print("KERS");
		gfx->fillRect(200, yPos + 25, kersBarWidth, 15, MAGENTA);
		
		// Turbo
		gfx->drawRect(265, yPos, 65, boxHeight, RGB565(255, 128, 0));
		gfx->fillRect(265, yPos, 65, 20, RGB565(255, 128, 0));
		gfx->setTextColor(BLACK);
		gfx->setTextSize(1);
		gfx->setCursor(270, yPos + 5);
		gfx->print("TURBO");
		gfx->setTextColor(RGB565(255, 200, 0));
		gfx->setTextSize(1);
		gfx->setCursor(270, yPos + 25);
		gfx->print("0.0B");  // Placeholder
	}
	
	void drawLapsPageContent() {
		// PAGE 5: Laps/Sectors Analysis - with nice layout
		gfx->fillRect(0, 0, SCREEN_WIDTH, 40, RGB565(40, 20, 60));  // Header background
		gfx->setTextColor(MAGENTA);
		gfx->setTextSize(2);
		gfx->setCursor(10, 12);
		gfx->print("LAPS/SECTORS");
		
		// Lap times summary - Three boxes spanning full width
		int boxWidth = (SCREEN_WIDTH - 20) / 3;  // 3 boxes with 5px spacing
		int boxHeight = 65;
		
		// Best lap
		gfx->drawRect(5, 50, boxWidth, boxHeight, YELLOW);
		gfx->fillRect(5, 50, boxWidth, 20, YELLOW);
		gfx->setTextColor(BLACK);
		gfx->setTextSize(1);
		gfx->setCursor(8, 57);
		gfx->print("BEST");
		gfx->setTextColor(YELLOW);
		gfx->setTextSize(1);
		gfx->setCursor(10, 72);
		gfx->print(bestLapTime);
		
		// Last lap
		gfx->drawRect(5 + boxWidth + 5, 50, boxWidth, boxHeight, CYAN);
		gfx->fillRect(5 + boxWidth + 5, 50, boxWidth, 20, CYAN);
		gfx->setTextColor(BLACK);
		gfx->setTextSize(1);
		gfx->setCursor(8 + boxWidth + 5, 57);
		gfx->print("LAST");
		gfx->setTextColor(CYAN);
		gfx->setTextSize(1);
		gfx->setCursor(10 + boxWidth + 5, 72);
		gfx->print(lastLapTime);
		
		// Current lap
		gfx->drawRect(5 + (boxWidth + 5) * 2, 50, boxWidth, boxHeight, GREEN);
		gfx->fillRect(5 + (boxWidth + 5) * 2, 50, boxWidth, 20, GREEN);
		gfx->setTextColor(BLACK);
		gfx->setTextSize(1);
		gfx->setCursor(8 + (boxWidth + 5) * 2, 57);
		gfx->print("NOW");
		gfx->setTextColor(GREEN);
		gfx->setTextSize(1);
		gfx->setCursor(10 + (boxWidth + 5) * 2, 72);
		gfx->print(currentLapTime);
		
		// Sector times - detailed box (full width) - AGORA COM DADOS REAIS
		gfx->drawRect(5, 125, SCREEN_WIDTH - 10, 120, WHITE);
		gfx->fillRect(5, 125, SCREEN_WIDTH - 10, 25, WHITE);
		gfx->setTextColor(BLACK);
		gfx->setTextSize(1);
		gfx->setCursor(10, 132);
		gfx->print("SECTOR TIMES");
		
		// Sector 1
		gfx->setTextColor(WHITE);
		gfx->setCursor(15, 160);
		gfx->print("S1");
		gfx->setCursor(45, 160);
		gfx->print(": " + sector1Time);
		
		// Sector 2
		gfx->setTextColor(WHITE);
		gfx->setCursor(15, 180);
		gfx->print("S2");
		gfx->setCursor(45, 180);
		gfx->print(": " + sector2Time);
		
		// Sector 3
		gfx->setTextColor(WHITE);
		gfx->setCursor(15, 200);
		gfx->print("S3");
		gfx->setCursor(45, 200);
		gfx->print(": " + sector3Time);
		
		// Lap invalid status
		if (lapInvalidated == "True") {
			gfx->setTextColor(RED);
			gfx->setCursor(220, 145);
			gfx->print("INVALID");
		}
		
		// Tyre wear bar (full width at bottom)
		gfx->drawRect(5, 260, SCREEN_WIDTH - 10, 20, RGB565(200, 100, 0));
		gfx->fillRect(5, 260, SCREEN_WIDTH - 10, 5, RGB565(200, 100, 0));
		gfx->setTextColor(RGB565(255, 200, 0));
		gfx->setTextSize(1);
		gfx->setCursor(10, 265);
		
		// Draw wear percentages
		String wearStr = "Wear - FL:" + tyreWearFrontLeft + "% FR:" + tyreWearFrontRight + 
		                 "% RL:" + tyreWearRearLeft + "% RR:" + tyreWearRearRight + "%";
		gfx->print(wearStr);
	}
	
	void drawRelativePageContent() {
		// PAGE 4: Relative/Head-to-Head Comparison (full width)
		gfx->fillRect(0, 0, SCREEN_WIDTH, 40, RGB565(60, 40, 20));  // Header background
		gfx->setTextColor(YELLOW);
		gfx->setTextSize(2);
		gfx->setCursor(10, 12);
		gfx->print("HEAD TO HEAD");
		
		// Driver ahead - Top box
		gfx->drawRect(5, 50, SCREEN_WIDTH - 10, 80, MAGENTA);
		gfx->fillRect(5, 50, SCREEN_WIDTH - 10, 25, MAGENTA);
		gfx->setTextColor(BLACK);
		gfx->setTextSize(1);
		gfx->setCursor(10, 58);
		gfx->print("DRIVER AHEAD");
		gfx->setTextColor(MAGENTA);
		gfx->setTextSize(1);
		gfx->setCursor(10, 80);
		gfx->print("Position: --");
		gfx->setCursor(10, 95);
		gfx->print("Gap: " + driverAheadGap + "s");
		gfx->setCursor(10, 110);
		gfx->print("Last: 01:35.74");
		
		// YOU - Middle box (highlighted)
		gfx->drawRect(5, 140, SCREEN_WIDTH - 10, 80, CYAN);
		gfx->fillRect(5, 140, SCREEN_WIDTH - 10, 25, CYAN);
		gfx->setTextColor(BLACK);
		gfx->setTextSize(2);
		gfx->setCursor(10, 145);
		gfx->print(">>> YOU <<<");
		gfx->setTextColor(CYAN);
		gfx->setTextSize(1);
		gfx->setCursor(10, 175);
		gfx->print("Position: " + position);
		gfx->setCursor(10, 190);
		gfx->print("Last: " + lastLapTime);
		gfx->setCursor(10, 205);
		gfx->print("Current: " + currentLapTime);
		
		// Driver behind - Bottom box
		gfx->drawRect(5, 230, SCREEN_WIDTH - 10, 35, ORANGE);
		gfx->fillRect(5, 230, SCREEN_WIDTH - 10, 20, ORANGE);
		gfx->setTextColor(BLACK);
		gfx->setTextSize(1);
		gfx->setCursor(10, 237);
		gfx->print("DRIVER BEHIND: Gap " + driverBehindGap + "s");
	}
	
	void drawLeaderboardPageContent() {
		// PAGE 6: Leaderboard - with nice styling
		gfx->fillRect(0, 0, SCREEN_WIDTH, 40, RGB565(20, 60, 20));  // Header background
		gfx->setTextColor(GREEN);
		gfx->setTextSize(2);
		gfx->setCursor(10, 12);
		gfx->print("LEADERBOARD");
		
		// Leaderboard box (full width)
		gfx->drawRect(5, 50, SCREEN_WIDTH - 10, 215, WHITE);
		gfx->fillRect(5, 50, SCREEN_WIDTH - 10, 25, WHITE);
		gfx->setTextColor(BLACK);
		gfx->setTextSize(1);
		gfx->setCursor(10, 57);
		gfx->print("POS  DRIVER          TIME       GAP");
		
		// Draw separator line
		gfx->drawLine(5, 75, SCREEN_WIDTH - 5, 75, RGB565(100, 100, 100));
		
		// Leaderboard entries - small font
		gfx->setTextSize(1);
		int yPos = 85;
		
		// 1st place (leading driver)
		gfx->setTextColor(YELLOW);
		gfx->setCursor(10, yPos);
		gfx->print("1");
		gfx->setCursor(30, yPos);
		gfx->print("Driver A");
		gfx->setCursor(130, yPos);
		gfx->print("01:34.23");
		gfx->setCursor(200, yPos);
		gfx->print("--:--");
		
		yPos += 18;
		
		// 2nd place
		gfx->setTextColor(WHITE);
		gfx->setCursor(10, yPos);
		gfx->print("2");
		gfx->setCursor(30, yPos);
		gfx->print("Driver B");
		gfx->setCursor(130, yPos);
		gfx->print("01:34.89");
		gfx->setCursor(200, yPos);
		gfx->print("+0.66s");
		
		yPos += 18;
		
		// 3rd place (YOU)
		gfx->setTextColor(CYAN);
		gfx->setCursor(10, yPos);
		gfx->print("3");
		gfx->setCursor(30, yPos);
		gfx->print(">>> YOU <<<");
		gfx->setCursor(130, yPos);
		gfx->print("01:35.12");
		gfx->setCursor(200, yPos);
		gfx->print("+0.89s");
		
		yPos += 18;
		
		// 4th place
		gfx->setTextColor(WHITE);
		gfx->setCursor(10, yPos);
		gfx->print("4");
		gfx->setCursor(30, yPos);
		gfx->print("Driver C");
		gfx->setCursor(130, yPos);
		gfx->print("01:35.67");
		gfx->setCursor(200, yPos);
		gfx->print("+1.44s");
		
		yPos += 18;
		
		// 5th place
		gfx->setTextColor(WHITE);
		gfx->setCursor(10, yPos);
		gfx->print("5");
		gfx->setCursor(30, yPos);
		gfx->print("Driver D");
		gfx->setCursor(130, yPos);
		gfx->print("01:36.01");
		gfx->setCursor(200, yPos);
		gfx->print("+1.78s");
		
		// Summary bar at bottom
		gfx->drawRect(5, 265, 250, 20, CYAN);
		gfx->fillRect(5, 265, 250, 20, CYAN);
		gfx->setTextColor(BLACK);
		gfx->setTextSize(1);
		gfx->setCursor(10, 270);
		gfx->print("Your Gap to Leader: +0.89s");
	}

	void idle() {
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
			} else if (alertUpper.indexOf("LOW FUEL") >= 0) {
				bgColor = RED;
				textColor = YELLOW;
			} else {
				bgColor = MAGENTA;
				textColor = WHITE;
			}
		}
		
		// PRIORIDADE 2: Pop-up temporário (popupMessage) - mensagens de mudanças menores
		// Only show if it's a real message (not empty, not "NORMAL", not "NONE", not "0")
		if (popupUpper.length() > 0 && 
			popupUpper != "NORMAL" && 
			popupUpper != "NONE" && 
			popupUpper != "0" && 
			isValidAlertString(popupUpper) &&
			alertText.length() == 0) {
			shouldShowAlert = true;
			alertStartTime = millis();
			alertText = cleanAlertText(popupNormalized);
			bgColor = RGB565(50, 50, 100);  // Dark blue background
			textColor = YELLOW;
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
				bgColor = GREEN;
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
