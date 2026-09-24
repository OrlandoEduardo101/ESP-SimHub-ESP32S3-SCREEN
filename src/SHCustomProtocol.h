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
	uint8_t raw;      // TD_STATUS as read, kept for the page-change log
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
	String popupMessage = "";  // [43] Pop-up temporário vindo do SimHub (BIAS, TC LEVEL, etc.)
	String uartPopupMessage = "";  // Pop-up temporário vindo do menu MFC/UART
	unsigned long alertStartTime = 0;
	bool alertWasShowing = false;  // Track if alert was displayed to trigger clear
	bool needsFullRedraw = false;  // Flag to trigger full screen redraw after alert

	// Rendering gate (see loop()). redrawPending is raised whenever something
	// the screen shows can have changed; the heartbeat refreshes anyway so
	// time-based state (overlay expiry) still resolves without a flag of its own.
	bool redrawPending = true;

	// Overlay repaint latch. A full-screen overlay only needs painting when its
	// content changes: loop() stops drawing the page underneath while one is up,
	// so the pixels survive between frames. Repainting it every frame measured
	// 101ms per frame (3.8 fps) against 46us with no overlay.
	String paintedOverlayText = "";
	uint16_t paintedOverlayBg = 0;
	uint8_t paintedBlinkPhase = 0xFF;

	// Half-period of the alert blink. Blinking is affordable precisely because the
	// overlay is latched: this repaints a couple of times a second instead of on
	// every frame, which is what used to cost 101ms per frame.
	static const unsigned long ALERT_BLINK_MS = 350;
	unsigned long lastRedrawMs = 0;
	static const unsigned long REDRAW_HEARTBEAT_MS = 250;
	bool timingFrameDrawn = false; // Flag: timing page static frame already drawn
	bool telemFrameDrawn  = false;
	bool advFrameDrawn    = false;
	bool stratFrameDrawn  = false;
	bool lapsFrameDrawn   = false;
	bool mapFrameDrawn    = false;
	bool mapTrackDrawn    = false;
	String mapLastTrackId = "";
	bool p499FrameDrawn   = false;

	// Calibration/trim live panel. This is deliberately NOT built on the
	// overlay/popup system above (OverlayPriority/latchOverlay): that system
	// is expiry-driven, right for a 3s toast, wrong for "stay up for as long
	// as the user is turning a knob and watching the number." This takes over
	// the whole screen instead, above even critical SimHub alerts, for as
	// long as either flag below is true -- see the render-dispatch check
	// that skips both the page switch and drawAlert() while active.
	static const uint8_t CALIB_TRIM_MAX_CHANNELS = 4;
	bool calibPanelActive = false;      // between $CALIB:START and a few seconds after $CALIB:DONE
	bool calibShowingDone = false;      // true during the post-DONE summary hold
	unsigned long calibDoneUntil = 0;
	String calibChannelId[CALIB_TRIM_MAX_CHANNELS];
	String calibChannelSpan[CALIB_TRIM_MAX_CHANNELS];  // "<min>-<max>/<span>", verbatim from the wheel
	uint8_t calibChannelCount = 0;

	// $TRIM:LIVE has no START/DONE pair, so liveness is inferred: still
	// "active" as long as a LIVE line arrived recently. If the wheel leaves
	// trim mode without announcing it, the panel clears itself within
	// TRIM_LIVE_TIMEOUT_MS instead of being stuck up forever.
	static const unsigned long TRIM_LIVE_TIMEOUT_MS = 1500;
	bool trimPanelActive = false;
	unsigned long lastTrimLiveMs = 0;
	String trimContextLine = "";  // everything except the trailing two axis values, verbatim
	String trimAxisA = "0";
	String trimAxisB = "0";

	bool calibTrimFrameDrawn = false;  // static frame (labels/borders) painted once per activation
	static const unsigned long ALERT_DURATION_MS = 3000;  // Show alert for 3 seconds
	bool popupFromUart = false;
	unsigned long popupFromUartUntil = 0;

	// ERS/FUEL step popups (see showErsStepPopup()/showFuelStepPopup()) don't
	// carry fixed text: the wheel only tells us a button was pressed, and the
	// game takes a frame or two to react, so text baked in at press time would
	// show the value from *before* the press for most of the popup's life.
	// Kind + direction are all that's fixed; drawAlert() recomputes the text
	// from live telemetry on every redraw for as long as the window is open,
	// so it catches up the moment the game's own state actually changes.
	enum UartPopupKind : uint8_t {
		UART_POPUP_TEXT = 0, UART_POPUP_ERS, UART_POPUP_FUEL,
		UART_POPUP_MAP, UART_POPUP_ABS, UART_POPUP_TURBO, UART_POPUP_REGEN, UART_POPUP_SOC
	};
	UartPopupKind uartPopupKind = UART_POPUP_TEXT;
	bool uartPopupStepUp = true;
	String prevAlertText = "";  // Track previous alert text to avoid resetting timer on same alert
	enum OverlayPriority : uint8_t {
		OVERLAY_NONE = 0,
		OVERLAY_SIMHUB_POPUP = 1,     // transient SimHub popups (BIAS:, TC LEVEL:)
		OVERLAY_SIMHUB_CRITICAL = 2,  // ENGINE OFF, YELLOW FLAG, PIT LIMITER, ...
		OVERLAY_UART_POPUP = 3        // MFC menu feedback — wins while the user is
		                             // physically operating the selector. Without
		                             // this, a stuck "ENGINE OFF" (game not open)
		                             // hid the menu and calibration was blind.
	};
	String activeOverlayText = "";
	uint16_t activeOverlayBgColor = BLACK;
	uint16_t activeOverlayTextColor = WHITE;
	unsigned long activeOverlayUntil = 0;
	OverlayPriority activeOverlayPriority = OVERLAY_NONE;

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
	String regenLevel = "--";               // [72] Regen level (LMU Hypercar via NeoRed; "--" nos demais

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
		TouchPoint point = {0, 0, false, 0};
		if (!touchInitialized) return point;

		// The touch panel is the full 480x320 glass. SCREEN_HEIGHT is 272
		// (the drawable area above the status bar), so it must not be used
		// to bounds-check a touch.
		const int16_t TP_MAX_X = 320;   // portrait X -> display Y
		const int16_t TP_MAX_Y = 480;   // portrait Y -> display X

		Wire.beginTransmission(TOUCH_ADDRESS);
		Wire.write(0x02);  // TD_STATUS
		if (Wire.endTransmission() != 0) { touchRejected++; return point; }

		if (Wire.requestFrom(TOUCH_ADDRESS, 5) < 5) { touchRejected++; return point; }
		if (Wire.available() < 5)                   { touchRejected++; return point; }

		uint8_t status = Wire.read();
		uint8_t x_high = Wire.read();
		uint8_t x_low  = Wire.read();
		uint8_t y_high = Wire.read();
		uint8_t y_low  = Wire.read();

		// TD_STATUS's low nibble is the NUMBER of touch points (0..2), not a
		// flag. Testing `status & 0x01` turned any odd byte from a corrupted
		// read -- 0xFF, 0x35, anything -- into a valid touch, which is what a
		// phantom page change looks like. The high nibble is reserved and
		// reads 0 on a healthy part, so anything set there means noise.
		uint8_t nTouch = status & 0x0F;
		if (nTouch == 0) return point;                       // nothing on the glass
		if (nTouch > 2 || (status & 0xF0) != 0) {
			touchRejected++;
			touchLastBadStatus = status;
			return point;
		}

		// Bits 7:6 of the X high byte are the event: 00 press down, 10
		// contact. 01 is lift-up and 11 is reserved -- neither is a press.
		uint8_t evt = x_high >> 6;
		if (evt == 0x01 || evt == 0x03) return point;

		int16_t x = ((x_high & 0x0F) << 8) | x_low;
		int16_t y = ((y_high & 0x0F) << 8) | y_low;
		if (x < 0 || x >= TP_MAX_X || y < 0 || y >= TP_MAX_Y) {
			touchRejected++;
			touchLastBadStatus = status;
			return point;
		}

		point.x = x;
		point.y = y;
		point.raw = status;
		point.touched = true;
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
	// PERF_DIAG: temporary per-phase draw timing, read over TCP 10002 by
	// main.cpp. Remove once the hot phase is identified. gfx is the raw panel
	// here (no PSRAM -> canvas is nullptr), so every primitive is bus I/O.
	volatile uint32_t pdFrames = 0;
	volatile uint32_t pdPageUs = 0, pdAlertUs = 0, pdIndUs = 0, pdLedUs = 0;
	volatile uint32_t pdFrameMaxUs = 0;
	volatile uint32_t pdOverlayPaints = 0;   // times the overlay was actually repainted

	// PERF_DIAG: parsed values of a few well-spaced fields. Comparing these with
	// what SimHub shows proves alignment in one look — a value landing in the
	// wrong one of these is a field-offset bug, not a rendering bug.
	String getPrevAlertTextDiag() { return prevAlertText; }

	// Page-change forensics. The dash occasionally switches page on its own,
	// it is not reproducible on demand and has no known trigger, so rather
	// than trying to catch it live every change is recorded here with its
	// cause and read back over PERF_DIAG afterwards. touchRejected is the
	// telling one: if it climbs while driving, electrical noise is reaching
	// the touch controller and the old `status & 0x01` test would have been
	// turning some of it into page changes.
	volatile uint32_t pgTouch = 0, pgUartPage = 0, pgUartMfc = 0;
	volatile uint32_t touchRejected = 0;
	volatile uint8_t  touchLastBadStatus = 0;
	static const uint8_t PG_LOG_N = 8;
	String pgLog[PG_LOG_N];
	uint8_t pgLogIdx = 0;

	void notePageChange(const String &why) {
		pgLog[pgLogIdx] = String(millis() / 1000) + "s " + why;
		pgLogIdx = (pgLogIdx + 1) % PG_LOG_N;
	}

	String pageChangeDump() {
		String out = "page_src touch=" + String(pgTouch) +
		             " uart_page=" + String(pgUartPage) +
		             " uart_mfc=" + String(pgUartMfc) +
		             " touch_rejected=" + String(touchRejected) +
		             " last_bad=0x" + String(touchLastBadStatus, HEX) + "\n";
		out += "page changes (oldest first):";
		bool any = false;
		for (uint8_t i = 0; i < PG_LOG_N; i++) {
			String &e = pgLog[(pgLogIdx + i) % PG_LOG_N];
			if (e.length()) { out += "\n  " + e; any = true; }
		}
		if (!any) out += " none";
		return out;
	}

	String perfFieldDump() {
		return "speed=" + speed + " gear=" + gear + " sessTime=" + sessionTimeLeft +
		       " flag=" + currentFlag + " pen=" + currentPenalties +
		       " alert=" + alertMessage + " track=" + trackId +
		       " overlay=[" + activeOverlayText + "]" +
		       " map=" + String(findTrackMap(trackId) ? "FOUND" : "none");
	}
	void showPopup(const String &msg, uint32_t durationMs = 2000) {
		// Draw it on the next loop() instead of waiting for the 250ms heartbeat.
		redrawPending = true;
		uartPopupKind = UART_POPUP_TEXT;
		uartPopupMessage = msg;
		popupFromUart = true;
		popupFromUartUntil = millis() + durationMs;
	}

	// $ERS:STEP:UP / $ERS:STEP:DN — the wheel only fired a button; it has no
	// idea what the game actually did with it. uartPopupMessage is left blank
	// on purpose: drawAlert() builds the real text from ersDeployMode/kersLevel
	// fresh each redraw (see the comment on uartPopupKind).
	void showErsStepPopup(bool up, uint32_t durationMs = 3000) {
		redrawPending = true;
		uartPopupKind = UART_POPUP_ERS;
		uartPopupStepUp = up;
		uartPopupMessage = "";
		popupFromUart = true;
		popupFromUartUntil = millis() + durationMs;
	}

	// $FUEL:STEP:UP / $FUEL:STEP:DN — same reasoning as showErsStepPopup().
	void showFuelStepPopup(bool up, uint32_t durationMs = 3000) {
		redrawPending = true;
		uartPopupKind = UART_POPUP_FUEL;
		uartPopupStepUp = up;
		uartPopupMessage = "";
		popupFromUart = true;
		popupFromUartUntil = millis() + durationMs;
	}

	// Shared by the five show*StepPopup() wrappers below -- same
	// self-updating-text mechanism as showErsStepPopup()/showFuelStepPopup()
	// (left as their own functions above, unchanged, rather than folded into
	// this: they already shipped and there's no reason to touch a working
	// path while refactoring). Parameterized over which telemetry field each
	// one draws from instead of five near-identical copies.
	void showTelemetryStepPopup(UartPopupKind kind, bool up, uint32_t durationMs = 3000) {
		redrawPending = true;
		uartPopupKind = kind;
		uartPopupStepUp = up;
		uartPopupMessage = "";
		popupFromUart = true;
		popupFromUartUntil = millis() + durationMs;
	}
	// $MAP:STEP:UP/DN -- ersDeployMode already resolves to the right thing
	// per game (ACC's real ErsDeployMode first, then LMU's
	// LMU_NeoRedPlugin.Extended.VM_ELECTRIC_MOTOR_MAP, confirmed against
	// Haagel's own dashboard code to return the game's native label already
	// -- a number for Hypercar, literal text like "Safety-car" for GT3, no
	// per-class logic needed here). The generic [EngineMap] SimHub exposes
	// is a bare rFactor2-engine index (what showed as "1, 2, 3..." instead
	// of the real label) -- deliberately not used for this popup.
	void showMapStepPopup(bool up, uint32_t durationMs = 3000) { showTelemetryStepPopup(UART_POPUP_MAP, up, durationMs); }
	// $ABS:STEP:UP/DN -- absLevel, already a plain numbered field.
	void showAbsStepPopup(bool up, uint32_t durationMs = 3000) { showTelemetryStepPopup(UART_POPUP_ABS, up, durationMs); }
	// $TURBO:STEP:UP/DN -- turboBoost (bar), for AC1-style turbo cars where
	// this same wheel button is used to trim boost rather than an ERS map.
	void showTurboStepPopup(bool up, uint32_t durationMs = 3000) { showTelemetryStepPopup(UART_POPUP_TURBO, up, durationMs); }
	// $REGEN:STEP:UP/DN -- regenLevel, wired yesterday.
	void showRegenStepPopup(bool up, uint32_t durationMs = 3000) { showTelemetryStepPopup(UART_POPUP_REGEN, up, durationMs); }
	// $SOC:STEP:UP/DN -- kersLevel (battery state of charge, 0-100%). Read-
	// only in spirit (nothing to actually step), but takes the same UP/DN
	// shape as the others for dispatch consistency; the direction arrow is
	// just cosmetic here.
	void showSocStepPopup(bool up, uint32_t durationMs = 3000) { showTelemetryStepPopup(UART_POPUP_SOC, up, durationMs); }

	// $CALIB:START:HALL -- opens the panel immediately, before any live data
	// exists yet, so it's on screen for the whole calibration from the first
	// moment rather than only once numbers are available.
	void startCalibPanel() {
		redrawPending = true;
		calibTrimFrameDrawn = false;
		calibPanelActive = true;
		calibShowingDone = false;
		calibChannelCount = 0;
	}

	// $CALIB:INVALID -- calibration aborted; don't leave the panel stuck open
	// with stale numbers.
	void closeCalibPanel() {
		calibPanelActive = false;
		calibShowingDone = false;
		calibTrimFrameDrawn = false;
		needsFullRedraw = true;
		redrawPending = true;
	}

	// $CALIB:LIVE:A 1782-2070/288 B 1790-2065/275 -- pairs of (channel id,
	// span string) separated by spaces. Stored verbatim per channel: the wire
	// format already is the display format ("<min>-<max>/<span>"), nothing to
	// reconstruct. Malformed tail (a channel id with nothing after it) stops
	// parsing rather than showing a channel with a blank span.
	void handleCalibLive(const String &val) {
		redrawPending = true;
		calibPanelActive = true;
		calibShowingDone = false;
		calibChannelCount = 0;
		int pos = 0;
		int len = val.length();
		while (pos < len && calibChannelCount < CALIB_TRIM_MAX_CHANNELS) {
			while (pos < len && val[pos] == ' ') pos++;
			if (pos >= len) break;
			int idEnd = pos;
			while (idEnd < len && val[idEnd] != ' ') idEnd++;
			String id = val.substring(pos, idEnd);
			pos = idEnd;
			while (pos < len && val[pos] == ' ') pos++;
			if (pos >= len) break;
			int spanEnd = pos;
			while (spanEnd < len && val[spanEnd] != ' ') spanEnd++;
			calibChannelId[calibChannelCount] = id;
			calibChannelSpan[calibChannelCount] = val.substring(pos, spanEnd);
			calibChannelCount++;
			pos = spanEnd;
		}
	}

	// $CALIB:DONE:A288 B275 -- channel id immediately followed by the final
	// span, no separator, repeated per channel. Returns false if val doesn't
	// look like that shape at all (an un-updated wheel still sending the old
	// "$CALIB:DONE:HALL"), so the caller can fall back to the legacy plain
	// popup instead of showing a panel with garbage in it.
	bool handleCalibDone(const String &val) {
		String trimmed = val;
		trimmed.trim();
		int len = trimmed.length();
		if (len == 0) return false;

		String ids[CALIB_TRIM_MAX_CHANNELS];
		String spans[CALIB_TRIM_MAX_CHANNELS];
		uint8_t count = 0;
		int pos = 0;
		while (pos < len && count < CALIB_TRIM_MAX_CHANNELS) {
			while (pos < len && trimmed[pos] == ' ') pos++;
			if (pos >= len) break;
			if (!isAlpha(trimmed[pos])) return false;  // not this shape at all
			int idEnd = pos + 1;
			int numEnd = idEnd;
			while (numEnd < len && isDigit(trimmed[numEnd])) numEnd++;
			if (numEnd == idEnd) return false;  // letter with no digits -- not this shape
			ids[count] = trimmed.substring(pos, idEnd);
			spans[count] = trimmed.substring(idEnd, numEnd);
			count++;
			pos = numEnd;
		}
		if (count == 0) return false;

		redrawPending = true;
		calibPanelActive = true;
		calibShowingDone = true;
		calibDoneUntil = millis() + 4000;  // hold the final numbers on screen briefly, then resume normal drawing
		calibChannelCount = count;
		for (uint8_t i = 0; i < count; i++) {
			calibChannelId[i] = ids[i];
			calibChannelSpan[i] = spans[i];  // final span only -- DONE carries no min/max, just the result
		}
		return true;
	}

	// $TRIM:LIVE:A MIN +5% ax -127 -127 -- format may drift since the wheel
	// side is developed separately from this. Robust to that on purpose: the
	// last two space-separated tokens are always taken as the two live axis
	// values (what the user actually watches while turning the knob),
	// everything before them shown verbatim as context underneath, whatever
	// it happens to contain.
	void handleTrimLive(const String &val) {
		redrawPending = true;
		trimPanelActive = true;
		lastTrimLiveMs = millis();

		String v = val;
		v.trim();
		int lastSpace = v.lastIndexOf(' ');
		if (lastSpace < 0) {
			trimContextLine = v;
			trimAxisA = "";
			trimAxisB = "";
			return;
		}
		String b = v.substring(lastSpace + 1);
		String rest = v.substring(0, lastSpace);
		rest.trim();
		int secondLastSpace = rest.lastIndexOf(' ');
		if (secondLastSpace < 0) {
			trimAxisA = rest;
			trimContextLine = "";
		} else {
			trimAxisA = rest.substring(secondLastSpace + 1);
			trimContextLine = rest.substring(0, secondLastSpace);
		}
		trimAxisB = b;
	}

	void pageNextExternal(const char *src = "?") {
		notePageChange(String(src) + " PAGE+");
		buzzerBeep(80);
		nextPage();
	}

	void pagePrevExternal(const char *src = "?") {
		notePageChange(String(src) + " PAGE-");
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
		// Fresh telemetry: let loop() draw one frame for it.
		redrawPending = true;
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
		trackId = FlowSerialReadStringUntil(';');
		regenLevel = FlowSerialReadStringUntil(';');  // Último campo (índice 72)
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
			uartPopupMessage = "";
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

				pgTouch++;
				String tag = "TOUCH raw=0x" + String(touch.raw, HEX) +
				             " x=" + String(touch.x) + " y=" + String(touch.y);
				if (touch.y < SCREEN_WIDTH / 2) {
					// Left half of display → previous page
					pagePrevExternal(tag.c_str());  // buzzer + fillScreen + resetDrawCache
				} else {
					// Right half of display → next page
					pageNextExternal(tag.c_str());  // buzzer + fillScreen + resetDrawCache
				}
			}
		}

		// Detect page changes (for cases other than touch)
		if (currentPage != lastPage) {
			resetDrawCache();
			lastPage = currentPage;
			redrawPending = true;
		}

		// DRS rising-edge detection: beep once when DRS becomes available
		if (prevDrsAvailable == "0" && drsAvailable == "1") {
			buzzerBeep(150);
		}
		prevDrsAvailable = drsAvailable;

		// Calibration/trim panel -- checked before hasReceivedData, on purpose.
		// It used to sit below the hasReceivedData return further down, which
		// meant it could never draw at all with SimHub closed: calibrating and
		// doing trim adjustment are bench activities, done specifically
		// without SimHub running, which is exactly the hasReceivedData==false
		// case -- the one scenario the panel matters most in was the one it
		// couldn't reach. It consumes no telemetry, so there was never a real
		// reason for the dependency.
		//
		// Has its own render-gate check (same redrawPending/heartbeat the
		// telemetry path uses further down) rather than drawing unthrottled on
		// every idle loop() iteration: drawCalibTrimPanel()'s row content is
		// diffed against prevData, but its status line isn't, so calling it a
		// few thousand times a second while idle would still hammer the
		// display bus for nothing.
		if (calibPanelActive || trimPanelActive) {
			if (trimPanelActive && millis() - lastTrimLiveMs > TRIM_LIVE_TIMEOUT_MS) {
				trimPanelActive = false;
			}
			if (calibShowingDone && millis() > calibDoneUntil) {
				calibPanelActive = false;
				calibShowingDone = false;
			}
			if (!calibPanelActive && !trimPanelActive) {
				// Just closed on this exact tick. Clear right here instead of
				// leaving stale panel pixels up until whichever path renders
				// next -- if SimHub still isn't connected, the
				// hasReceivedData==false path below never touches the screen
				// at all, so that wait could be indefinite.
				calibTrimFrameDrawn = false;
				gfx->fillScreen(BLACK);
				needsFullRedraw = false;  // handled right here; don't do it again downstream
			} else {
				const unsigned long nowMs = millis();
				if (!redrawPending && !needsFullRedraw &&
				    (nowMs - lastRedrawMs) < REDRAW_HEARTBEAT_MS) {
					return;
				}
				redrawPending = false;
				lastRedrawMs = nowMs;
				drawCalibTrimPanel();
				return;
			}
		}

		if (!hasReceivedData) {
			// Show loading animation on LEDs while waiting for SimHub
			#ifdef INCLUDE_RGB_LEDS_NEOPIXELBUS
			updateLoadingAnimation();
			#endif
			// Still render UART popups from ButtonBox even before SimHub connects
			if (popupFromUart && uartPopupMessage.length() > 0) {
				drawAlert();
			}
			return;
		}

		// Rendering gate.
		//
		// Everything below ends in canvas->flush(), a full-framebuffer push over
		// the parallel bus, and main.cpp's loop() only polls the SimHub link
		// *after* this function returns. Redrawing on every iteration therefore
		// pinned the ARQ ack latency to one whole frame time (~105ms, measured
		// against an idle board over TCP with a 4-9ms network RTT). ARQ is
		// stop-and-wait, so a ~270-byte telemetry frame paid that once per
		// 32-byte packet — about 17 times, which is the multi-second refresh.
		//
		// Drawing only when something can actually have changed leaves the loop
		// free to service the link at full speed in between. The heartbeat is the
		// safety net for time-based state: drawAlert() uses millis() purely for
		// overlay expiry (no blinking), so 250ms granularity is invisible.
		{
			const unsigned long nowMs = millis();
			if (!redrawPending && !needsFullRedraw &&
			    (nowMs - lastRedrawMs) < REDRAW_HEARTBEAT_MS) {
				return;
			}
			redrawPending = false;
			lastRedrawMs = nowMs;
		}
		const uint32_t pdT0 = micros();

		// (calibration/trim panel dispatch now lives above, before the
		// hasReceivedData check -- see that block's comment for why)

		// Check if we need full redraw after alert expired
		if (needsFullRedraw) {
			gfx->fillScreen(BLACK);
			resetDrawCache();  // Clear all caches
			needsFullRedraw = false;
			paintedOverlayText = "";  // the overlay was wiped too, so force a repaint
		}

		// An overlay covers the whole dashboard, so drawing the page under it is
		// invisible work — and it was that repaint which forced drawAlert() to
		// redraw the overlay every frame just to avoid being erased. Skipping both
		// is what turns a permanent 101ms/frame cost into a one-off.
		const bool overlayUp = overlayIsShowing();

		// Drawing the page invalidates whatever overlay is on the panel: the box does
		// not cover the full screen, so an alert that expires and is immediately
		// relatched with the same text would keep its old pixels while telemetry is
		// painted over them. That is the leak where values showed through the flag.
		if (!overlayUp) paintedOverlayText = "";

		// Draw page-specific content
		if (!overlayUp)
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

		const uint32_t pdT1 = micros();

		// Draw alerts (flags, penalties, etc.) on top of everything
		drawAlert();
		const uint32_t pdT2 = micros();

		// Draw page indicator at bottom
		if (!overlayUp) drawPageIndicator();
		const uint32_t pdT3 = micros();

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

		const uint32_t pdT4 = micros();

		// Flush canvas to display (double-buffered rendering)
		if (canvas) canvas->flush();

		// PERF_DIAG accounting
		pdPageUs  += pdT1 - pdT0;
		pdAlertUs += pdT2 - pdT1;
		pdIndUs   += pdT3 - pdT2;
		pdLedUs   += pdT4 - pdT3;
		const uint32_t pdTotal = micros() - pdT0;
		if (pdTotal > pdFrameMaxUs) pdFrameMaxUs = pdTotal;
		pdFrames++;
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
		// Header 0..35 (36px) | C1 36..126 (90px) | C2 127..217 (90px) | C3 218..319 (102px)
		const int FH   = SCREEN_HEIGHT + 48; // 320
		const int C1_Y = 36,  C1_H = 90;
		const int C2_Y = 127, C2_H = 90;
		const int C3_Y = 218, C3_H = FH - C3_Y; // 102

		// ── Static frame: draw only once per page switch ──
		if (!timingFrameDrawn) {
			gfx->fillScreen(BLACK);

			// Header
			gfx->fillRect(0, 0, SCREEN_WIDTH, 36, BG_HDR);
			gfx->fillRect(0, 34, SCREEN_WIDTH, 2, GOLD);
			gfx->setTextColor(WHITE); gfx->setTextSize(2);
			gfx->setCursor(10, 8); gfx->print("TIMING");

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
			gfx->fillRect(SCREEN_WIDTH - 200, 2, 198, 30, BG_HDR);
			if (position.length() > 0 && position != "0") {
				gfx->setTextColor(RGB565(140, 140, 160)); gfx->setTextSize(1);
				gfx->setCursor(SCREEN_WIDTH - 52, 4); gfx->print("POS");
				gfx->setTextColor(WHITE); gfx->setTextSize(2);
				gfx->setCursor(SCREEN_WIDTH - 54, 16); gfx->print("P"); gfx->print(position);
			}
			int dxLabel = (position.length() > 0 && position != "0") ? SCREEN_WIDTH - 185 : SCREEN_WIDTH - 110;
			gfx->setTextColor(RGB565(120, 120, 140)); gfx->setTextSize(1);
			gfx->setCursor(dxLabel, 4); gfx->print("DELTA");
			gfx->setTextColor(deltaCol); gfx->setTextSize(2);
			gfx->setCursor(dxLabel - 4, 16); gfx->print(sessionBestLiveDeltaSeconds);
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
			gfx->fillRect(0, 0, SCREEN_WIDTH, 36, BG_HDR);
			gfx->fillRect(0, 34, SCREEN_WIDTH, 2, ORNG);
			gfx->setTextColor(WHITE); gfx->setTextSize(2);
			gfx->setCursor(10, 8); gfx->print("TELEMETRY");
			gfx->setTextColor(RGB565(120, 120, 140)); gfx->setTextSize(1);
			gfx->setCursor(SCREEN_WIDTH - 92, 4); gfx->print("GEAR");

			// Layout: FH=320 | Header 0..35 | Speed/TC/ABS 36..124 | Brake 125..178 | Tyres 179..319
			const int FH=320, SPD_Y=36, SPD_H=88, BRK_Y=125, BRK_H=53, TYR_Y=179;
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
		const int SPD_Y=36, SPD_H=88, BRK_Y=125, BRK_H=53, TYR_Y=179;
		const int TC_X=302, TC_H=43;
		const int ABS_Y = SPD_Y + TC_H + 1;

		// Gear (header)
		if (prevData["v_gr"] != gear) {
			gfx->fillRect(SCREEN_WIDTH - 90, 4, 84, 28, BG_HDR);
			gfx->setTextColor(WHITE); gfx->setTextSize(3);
			gfx->setCursor(SCREEN_WIDTH - 88, 8); gfx->print(gear);
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
			gfx->fillRect(0, 0, SCREEN_WIDTH, 36, BG_HDR);
			gfx->fillRect(0, 34, SCREEN_WIDTH, 2, MGT);
			gfx->setTextColor(WHITE); gfx->setTextSize(2);
			gfx->setCursor(10, 8); gfx->print("ADVANCED");

			// Layout: FH=320 | Header 0..35 | OIL/WATER 36..108 | TYRE WEAR 109..171 | AIR/ROAD/DRS/TURBO 172..234 | KERS 235..319
			const int FH=320, OW_Y=36, OW_H=73, WR_Y=109, WR_H=63, R3_Y=172, R3_H=63, KR_Y=235;

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
		const int OW_Y=36, OW_H=73, WR_Y=109, WR_H=63, R3_Y=172, R3_H=63, KR_Y=235;

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
			gfx->fillRect(0, 0, SCREEN_WIDTH, 36, BG_HDR);
			gfx->fillRect(0, 34, SCREEN_WIDTH, 2, GOLD);
			gfx->setTextColor(WHITE); gfx->setTextSize(2);
			gfx->setCursor(10, 8); gfx->print("LAPS/SECTORS");

			// Layout: FH=320 | Header 0..35 | BEST/LAST 36..118 | CURRENT 119..178 | SECTORS 179..247 | TYRE WEAR 248..319
			const int FH=320, BL_Y=36, BL_H=83, CL_Y=119, CL_H=59, SC_Y=179, SC_H=68, WR_Y=248;

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
		const int BL_Y=36, BL_H=83, CL_Y=119, CL_H=59, SC_Y=179, SC_H=68, WR_Y=248;

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
			gfx->fillRect(0, 0, SCREEN_WIDTH, 36, BG_HDR);
			gfx->fillRect(0, 34, SCREEN_WIDTH, 2, WHT);
			gfx->setTextColor(WHITE); gfx->setTextSize(2);
			gfx->setCursor(10, 8); gfx->print("STRATEGY");

			// Layout: FH=320 | Header 0..35 | POSITION 36..118 | GAP AHD/BHD 119..187 | FUEL LAPS/DRS 188..247 | BTM 248..319
			const int FH=320, POS_Y=36, POS_H=83, GAP_Y=119, GAP_H=69, ROW3_Y=188, ROW3_H=59, BTM_Y=248;

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
		const int POS_Y=36, POS_H=83, GAP_Y=119, GAP_H=69, ROW3_Y=188, ROW3_H=59, BTM_Y=248;

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
	// Shortest alias allowed to match as a bare substring. See findTrackMap().
	static const uint8_t TRACK_ALIAS_MIN_SUBSTRING = 6;

	// True when `needle` appears in `hay` as a whole word — i.e. not glued to a
	// letter or digit on either side. Separator characters differ per sim
	// ('_', '-', ' '), so anything non-alphanumeric counts as a boundary.
	static bool trackTokenMatch(const String& hay, const char* needle) {
		const int nlen = strlen(needle);
		if (nlen == 0) return false;
		int at = hay.indexOf(needle);
		while (at >= 0) {
			const bool leftOk  = (at == 0) || !isAlphaNumeric(hay[at - 1]);
			const bool rightOk = (at + nlen >= (int)hay.length()) || !isAlphaNumeric(hay[at + nlen]);
			if (leftOk && rightOk) return true;
			at = hay.indexOf(needle, at + 1);
		}
		return false;
	}

	// Resolve SimHub's TrackId to a stored map.
	//
	// The same circuit arrives under different names depending on the sim and the
	// mod ("spa", "spa_francorchamps", "ks_barcelona", "monza_1966"), so matching
	// has to be loose. But loose on its own is wrong: a bare substring pass makes
	// the alias "spa" match a track called "spain". Hence three passes, strictest
	// first — exact, then whole word, and only then substring.
	const TrackMapEntry* findTrackMap(const String& tid) {
		String lower = tid;
		lower.toLowerCase();
		lower.trim();
		if (lower.length() == 0) return nullptr;

		for (uint8_t i = 0; i < TRACK_MAP_COUNT; i++) {
			if (lower == TRACK_MAP_TABLE[i].id) return &TRACK_MAP_TABLE[i];
		}
		for (uint8_t i = 0; i < TRACK_MAP_COUNT; i++) {
			if (trackTokenMatch(lower, TRACK_MAP_TABLE[i].id)) return &TRACK_MAP_TABLE[i];
		}
		// Last resort, for ids that run words together ("spafrancorchamps"). Only
		// aliases long enough to be unmistakable take part: allowing short ones here
		// is what made "spa" match a circuit called "spain".
		for (uint8_t i = 0; i < TRACK_MAP_COUNT; i++) {
			if (strlen(TRACK_MAP_TABLE[i].id) < TRACK_ALIAS_MIN_SUBSTRING) continue;
			if (lower.indexOf(TRACK_MAP_TABLE[i].id) >= 0) return &TRACK_MAP_TABLE[i];
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
		const uint16_t BLU    = RGB565( 60, 140, 240);
		const uint16_t ORNG   = RGB565(230, 150,   0);
		const uint16_t BG_HDR = RGB565(  6,   8,  22);
		const uint16_t BG_PAN = RGB565(  8,   8,  12);
		const uint16_t DIM    = RGB565(120, 120, 130);
		const uint16_t SEP    = RGB565( 22,  20,  30);
		const int PANEL_X     = 272;
		const int PANEL_W     = SCREEN_WIDTH - PANEL_X;
		const int FH          = 320;
		const int HDR_H       = 36;
		const int BODY_Y      = HDR_H;
		const int BODY_H      = 308 - BODY_Y;
		const int R_DELTA_Y   = 36;
		const int R_DELTA_H   = 68;
		const int R_GAPA_Y    = 104;
		const int R_GAPA_H    = 54;
		const int R_GAPB_Y    = 158;
		const int R_GAPB_H    = 54;
		const int R_LAP_Y     = 212;
		const int R_LAP_H     = 54;
		const int R_BEST_Y    = 266;
		const int R_BEST_H    = 42;
		int16_t bx, by;
		uint16_t bw, bh;

		if (!mapFrameDrawn) {
			gfx->fillScreen(BLACK);
			gfx->fillRect(0, 0, SCREEN_WIDTH, HDR_H, BG_HDR);
			gfx->fillRect(0, HDR_H - 1, SCREEN_WIDTH, 2, CYN);
			gfx->setTextColor(CYN); gfx->setTextSize(2);
			gfx->setCursor(10, 8); gfx->print("MAP");

			// Right panel bg + separators
			gfx->fillRect(PANEL_X - 5, BODY_Y, SCREEN_WIDTH - PANEL_X + 5, BODY_H, BG_PAN);
			gfx->drawLine(PANEL_X - 7, BODY_Y, PANEL_X - 7, FH - 1, SEP);
			gfx->drawLine(PANEL_X, R_GAPA_Y, SCREEN_WIDTH - 1, R_GAPA_Y, SEP);
			gfx->drawLine(PANEL_X, R_GAPB_Y, SCREEN_WIDTH - 1, R_GAPB_Y, SEP);
			gfx->drawLine(PANEL_X, R_LAP_Y, SCREEN_WIDTH - 1, R_LAP_Y, SEP);
			gfx->drawLine(PANEL_X, R_BEST_Y, SCREEN_WIDTH - 1, R_BEST_Y, SEP);
			gfx->drawLine(PANEL_X, R_BEST_Y + R_BEST_H, SCREEN_WIDTH - 1, R_BEST_Y + R_BEST_H, SEP);

			// Remove keys that are no longer used on the MAP page.
			prevData.erase("m_sg");
			prevData.erase("m_bb");
			prevData.erase("m_tcabs");
			prevData.erase("m_fmap");
			prevData.erase("m_kers");
			prevData.erase("m_trk");
			prevData.erase("m_hpos");

			mapFrameDrawn = true;
		}

		// Header: track name
		if (prevData["m_hdr_trk"] != trackId) {
			gfx->fillRect(55, 2, PANEL_X - 58, HDR_H - 4, BG_HDR);
			String dt = trackId;
			if (dt.length() > 18) dt = dt.substring(0, 18);
			gfx->setTextColor(DIM); gfx->setTextSize(1);
			gfx->setCursor(55, 5); gfx->print("TRACK");
			gfx->setTextColor(WHITE); gfx->setTextSize(2);
			gfx->setCursor(55, 15); gfx->print(dt);
			prevData["m_hdr_trk"] = trackId;
		}
		// Header: position + session time
		String hPos = "P" + position + "/" + opponentsCount + "|" + sessionTimeLeft;
		if (prevData["m_hdr_pos"] != hPos) {
			gfx->fillRect(PANEL_X, 2, SCREEN_WIDTH - PANEL_X, HDR_H - 4, BG_HDR);
			gfx->setTextColor(YLW); gfx->setTextSize(2);
			gfx->setCursor(PANEL_X + 4, 5); gfx->print("P" + position + "/" + opponentsCount);
			gfx->setTextColor(DIM); gfx->setTextSize(1);
			gfx->setCursor(PANEL_X + 4, 25); gfx->print("SESSION ");
			gfx->print(sessionTimeLeft);
			prevData["m_hdr_pos"] = hPos;
		}

		// Map area: redraw when track or positions change
		String posKey = trackPositionPercent + "|" + aheadTrackPosition + "|" + behindTrackPosition;
		if (!mapTrackDrawn || mapLastTrackId != trackId || prevData["m_pk"] != posKey) {
			gfx->fillRect(0, BODY_Y, PANEL_X - 8, BODY_H, BLACK);
			const TrackMapEntry* tmap = findTrackMap(trackId);
			float myPos = trackPositionPercent.toFloat();
			float aPos  = aheadTrackPosition.toFloat();
			float bPos  = behindTrackPosition.toFloat();
			int dx, dy;
			if (tmap) {
				drawTrackPolyline(tmap->points, tmap->numPoints, 20, 50, RGB565(80, 80, 80));
				int sfX = (int16_t)pgm_read_word(&tmap->points[0]) + 20;
				int sfY = (int16_t)pgm_read_word(&tmap->points[1]) + 50;
				gfx->drawLine(sfX-4, sfY-4, sfX+4, sfY-4, WHITE);
				gfx->drawLine(sfX-4, sfY+4, sfX+4, sfY+4, WHITE);
				if (aPos > 0.001f) {
					getPositionOnTrack(tmap->points, tmap->numPoints, aPos, 20, 50, dx, dy);
					gfx->fillCircle(dx, dy, 4, RD);
					gfx->setTextColor(RD); gfx->setTextSize(1); gfx->setCursor(dx+6, dy-3); gfx->print("A");
				}
				if (bPos > 0.001f) {
					getPositionOnTrack(tmap->points, tmap->numPoints, bPos, 20, 50, dx, dy);
					gfx->fillCircle(dx, dy, 4, BLU);
					gfx->setTextColor(BLU); gfx->setTextSize(1); gfx->setCursor(dx+6, dy-3); gfx->print("B");
				}
				if (myPos > 0.001f || hasReceivedData) {
					getPositionOnTrack(tmap->points, tmap->numPoints, myPos, 20, 50, dx, dy);
					gfx->fillCircle(dx, dy, 5, GRN); gfx->drawCircle(dx, dy, 6, WHITE);
				}
			} else {
				// No stored map for this circuit, so positions go on a plain ring.
				// Label it: an unexplained oval reads as a broken minimap, when the
				// real message is "this TrackId is not in TRACK_MAP_TABLE yet". The
				// id shown is exactly the string to add as an alias.
				const int mCX=132, mCY=172, mRX=108, mRY=108;
				gfx->setTextColor(RGB565(120,120,130)); gfx->setTextSize(1);
				gfx->setCursor(20, 56); gfx->print("NO MAP FOR:");
				gfx->setTextColor(RGB565(200,200,60));
				gfx->setCursor(20, 68); gfx->print(trackId.substring(0, 28));
				gfx->drawEllipse(mCX, mCY, mRX, mRY, RGB565(60,60,60));
				gfx->drawEllipse(mCX, mCY, mRX-1, mRY-1, RGB565(80,80,80));
				if (aPos > 0.001f) { float a=aPos*TWO_PI-HALF_PI; int x=mCX+(int)(mRX*cos(a)),y=mCY+(int)(mRY*sin(a)); gfx->fillCircle(x,y,4,RD); }
				if (bPos > 0.001f) { float a=bPos*TWO_PI-HALF_PI; int x=mCX+(int)(mRX*cos(a)),y=mCY+(int)(mRY*sin(a)); gfx->fillCircle(x,y,4,BLU); }
				if (myPos > 0.001f || hasReceivedData) { float a=myPos*TWO_PI-HALF_PI; int x=mCX+(int)(mRX*cos(a)),y=mCY+(int)(mRY*sin(a)); gfx->fillCircle(x,y,5,GRN); gfx->drawCircle(x,y,6,WHITE); }
			}
			mapLastTrackId = trackId; mapTrackDrawn = true;
			prevData["m_pk"] = posKey;
		}

		// Right panel: focused MAP metrics
		// Row DELTA HERO (y=36..103)
		if (prevData["m_dt"] != sessionBestLiveDeltaSeconds) {
			gfx->fillRect(PANEL_X, R_DELTA_Y, PANEL_W, R_DELTA_H, BG_PAN);
			bool neg = sessionBestLiveDeltaSeconds.indexOf('-') >= 0;
			gfx->setTextColor(DIM); gfx->setTextSize(1);
			gfx->setCursor(PANEL_X + 6, R_DELTA_Y + 6); gfx->print("DELTA");
			gfx->setTextColor(neg ? GRN : RD); gfx->setTextSize(4);
			gfx->getTextBounds(sessionBestLiveDeltaSeconds, 0, 0, &bx, &by, &bw, &bh);
			gfx->setCursor(PANEL_X + (PANEL_W - (int)bw) / 2, R_DELTA_Y + 24);
			gfx->print(sessionBestLiveDeltaSeconds);
			prevData["m_dt"] = sessionBestLiveDeltaSeconds;
		}

		// Row GAP AHEAD (y=104..157)
		if (prevData["m_ga"] != driverAheadGap) {
			gfx->fillRect(PANEL_X, R_GAPA_Y, PANEL_W, R_GAPA_H, BG_PAN);
			gfx->setTextColor(DIM); gfx->setTextSize(1);
			gfx->setCursor(PANEL_X + 8, R_GAPA_Y + 6); gfx->print("GAP +");
			gfx->setTextColor(GRN); gfx->setTextSize(3);
			gfx->setCursor(PANEL_X + 8, R_GAPA_Y + 22); gfx->print(driverAheadGap);
			prevData["m_ga"] = driverAheadGap;
		}

		// Row GAP BEHIND (y=158..211)
		if (prevData["m_gb"] != driverBehindGap) {
			gfx->fillRect(PANEL_X, R_GAPB_Y, PANEL_W, R_GAPB_H, BG_PAN);
			gfx->setTextColor(DIM); gfx->setTextSize(1);
			gfx->setCursor(PANEL_X + 8, R_GAPB_Y + 6); gfx->print("GAP -");
			gfx->setTextColor(ORNG); gfx->setTextSize(3);
			gfx->setCursor(PANEL_X + 8, R_GAPB_Y + 22); gfx->print(driverBehindGap);
			prevData["m_gb"] = driverBehindGap;
		}

		// Row LAP TIME (y=212..265)
		if (prevData["m_lap"] != currentLapTime) {
			gfx->fillRect(PANEL_X, R_LAP_Y, PANEL_W, R_LAP_H, BG_PAN);
			gfx->setTextColor(DIM); gfx->setTextSize(1);
			gfx->setCursor(PANEL_X + 8, R_LAP_Y + 6); gfx->print("LAP");
			gfx->setTextColor(lapInvalidated == "True" ? RD : WHITE); gfx->setTextSize(3);
			gfx->setCursor(PANEL_X + 8, R_LAP_Y + 22); gfx->print(currentLapTime);
			prevData["m_lap"] = currentLapTime;
		}

		// Row BEST LAP (y=266..307)
		if (prevData["m_best"] != bestLapTime) {
			gfx->fillRect(PANEL_X, R_BEST_Y, PANEL_W, R_BEST_H, BG_PAN);
			gfx->setTextColor(DIM); gfx->setTextSize(1);
			gfx->setCursor(PANEL_X + 8, R_BEST_Y + 6); gfx->print("BEST");
			gfx->setTextColor(GRN); gfx->setTextSize(2);
			gfx->setCursor(PANEL_X + 8, R_BEST_Y + 18); gfx->print(bestLapTime);
			prevData["m_best"] = bestLapTime;
		}
	}

	void idle() {
	}

	// ── CALIBRATION / TRIM LIVE PANEL ────────────────────────────────
	// Takes the whole screen while calibPanelActive or trimPanelActive is
	// true -- see the render-dispatch check that skips the normal page
	// switch and drawAlert() while either is set, and the comment on the
	// state fields themselves for why this isn't built on the overlay
	// system.
	//
	// Span colour bands (calib only) are a judgement call, not a value from
	// the wheel: the user's own account of the failure mode was "a span
	// this narrow, around 300 counts, turns ADC noise into visible axis
	// oscillation" -- red below 400, yellow 400-800, green above, so the
	// number that matters is legible at a glance without doing the math
	// during the physical act of calibrating.
	void drawCalibTrimPanel() {
		if (!canUseDisplay()) return;

		const int W = SCREEN_WIDTH;
		const uint16_t BG  = BLACK;
		const uint16_t LBL = RGB565(140, 140, 150);
		const uint16_t ROW_BG = RGB565(20, 20, 24);

		auto spanColor = [](long span) -> uint16_t {
			if (span < 400) return RED;
			if (span < 800) return YELLOW;
			return GREEN;
		};
		// Pull the trailing "/<digits>" off a "<min>-<max>/<span>" string, or
		// parse a bare span number (the DONE format has no slash at all).
		auto extractSpan = [](const String &s) -> long {
			int slash = s.lastIndexOf('/');
			String num = slash >= 0 ? s.substring(slash + 1) : s;
			return num.toInt();
		};

		if (!calibTrimFrameDrawn) {
			gfx->fillScreen(BG);
			gfx->setTextColor(LBL);
			gfx->setTextSize(2);
			gfx->setCursor(10, 10);
			gfx->print(calibPanelActive ? "CALIBRACAO HALL" : "TRIM");
			calibTrimFrameDrawn = true;
			// Force every row below to repaint against the fresh background.
			prevData.erase("ctp_rows");
			prevData.erase("ctp_ctx");
			prevData.erase("ctp_axis");
		}

		if (calibPanelActive) {
			gfx->fillRect(0, 40, W, 26, BG);
			gfx->setTextColor(calibShowingDone ? GREEN : YELLOW);
			gfx->setTextSize(2);
			gfx->setCursor(10, 44);
			gfx->print(calibShowingDone ? "FINALIZADA - solte os paddles" : "Mova os paddles ate o fim, os dois sentidos");

			String rowsKey;
			for (uint8_t i = 0; i < calibChannelCount; i++) rowsKey += calibChannelId[i] + calibChannelSpan[i] + "|";
			if (prevData["ctp_rows"] != rowsKey) {
				gfx->fillRect(0, 80, W, 220, BG);
				int y = 90;
				for (uint8_t i = 0; i < calibChannelCount; i++) {
					long span = extractSpan(calibChannelSpan[i]);
					uint16_t col = spanColor(span);
					gfx->fillRect(10, y, W - 20, 46, ROW_BG);
					gfx->setTextColor(LBL); gfx->setTextSize(2);
					gfx->setCursor(20, y + 6);
					gfx->print("Canal "); gfx->print(calibChannelId[i]);
					gfx->setTextColor(col); gfx->setTextSize(3);
					gfx->setCursor(20, y + 24);
					gfx->print(calibChannelSpan[i]);
					if (!calibShowingDone) {
						// LIVE carries "<min>-<max>/<span>"; DONE carries only
						// the span, so the unit label only applies to LIVE.
						gfx->setTextColor(LBL); gfx->setTextSize(1);
						gfx->setCursor(W - 90, y + 30);
						gfx->print("counts");
					}
					y += 54;
				}
				prevData["ctp_rows"] = rowsKey;
			}
		} else if (trimPanelActive) {
			if (prevData["ctp_ctx"] != trimContextLine) {
				gfx->fillRect(0, 40, W, 30, BG);
				gfx->setTextColor(LBL); gfx->setTextSize(2);
				gfx->setCursor(10, 44);
				gfx->print(trimContextLine);
				prevData["ctp_ctx"] = trimContextLine;
			}

			String axisKey = trimAxisA + "|" + trimAxisB;
			if (prevData["ctp_axis"] != axisKey) {
				gfx->fillRect(0, 90, W, 180, BG);
				// Priorize legibilidade: the two live axis numbers are the
				// biggest text this display ever draws anywhere.
				gfx->setTextColor(WHITE); gfx->setTextSize(8);
				int16_t bx, by; uint16_t bw, bh;
				gfx->getTextBounds(trimAxisA, 0, 0, &bx, &by, &bw, &bh);
				gfx->setCursor((W / 2 - (int)bw) / 2, 100);
				gfx->print(trimAxisA);
				gfx->getTextBounds(trimAxisB, 0, 0, &bx, &by, &bw, &bh);
				gfx->setCursor(W / 2 + (W / 2 - (int)bw) / 2, 100);
				gfx->print(trimAxisB);
				gfx->setTextColor(LBL); gfx->setTextSize(1);
				gfx->setCursor(W / 4 - 10, 200);
				gfx->print("EIXO A");
				gfx->setCursor(3 * W / 4 - 10, 200);
				gfx->print("EIXO B");
				prevData["ctp_axis"] = axisKey;
			}
		}
	}

	// ── PAGE 8 — 499P ───────────────────────────────────────────────
	// Modelled on the real Ferrari 499P wheel dash: black background, small
	// grey labels above each value, a big white gear between two thin
	// vertical bars, temperatures in red, full-width energy bar along the
	// bottom. Scaled down from the car's own screen to 480x320.
	//
	// Layout rule the previous version broke: a label is painted once with
	// the static frame, and the value under it owns a band that starts
	// below the label and never reaches into the neighbouring cell. Before,
	// the gear's clear rect ate the bottom half of the ERS text, the speed's
	// ate its own "SPEED" label, and the ARB values (size 3 text in 32px
	// boxes) collided with the position readout and spilled into the column
	// next door.
	void draw499PPageContent() {
		if (!canUseDisplay()) return;

		const int W     = SCREEN_WIDTH;   // 480
		const int H     = 320;
		const int TOP_H = 42;
		const int BAR_H = 24;
		const int BAR_Y = H - BAR_H;      // 296
		const int MID_Y = TOP_H;          // 42

		const uint16_t BG  = RGB565(  0,   0,   0);
		const uint16_t LBL = RGB565(110, 110, 118);
		const uint16_t WHT = WHITE;
		const uint16_t RD  = RGB565(235,  45,  35);
		const uint16_t ORG = RGB565(255, 150,   0);
		const uint16_t GRN = RGB565(  0, 220,  90);
		const uint16_t YLW = RGB565(240, 215,   0);
		const uint16_t CYN = RGB565(  0, 200, 220);
		const uint16_t MGT = RGB565(230,  60, 220);
		const uint16_t SEP = RGB565( 40,  40,  46);
		const uint16_t DIM = RGB565( 18,  18,  20);

		int16_t bx, by; uint16_t bw, bh;
		int kv = kersLevel.toInt();
		kv = kv < 0 ? 0 : (kv > 100 ? 100 : kv);

		auto tyreCol = [&](int t) -> uint16_t {
			if (t > 105) return RD;
			if (t > 90)  return ORG;
			if (t > 60)  return CYN;
			return LBL;
		};
		auto brakeCol = [&](int t) -> uint16_t {
			if (t > 700) return RD;
			if (t > 500) return ORG;
			if (t > 200) return WHT;
			return LBL;
		};

		// One labelled value. The band (x, y, w, vh) belongs to this value
		// alone: it is cleared and repainted here and nowhere else. Text too
		// wide for its band shrinks a size instead of spilling over — the
		// NeoRed fields can return strings of unknown length.
		auto val = [&](const char* key, int x, int y, int w, int vh,
		               uint8_t size, uint16_t col, const String& v) {
			if (prevData[key] == v) return;
			gfx->fillRect(x, y, w, vh, BG);
			while (size > 1 && (int)v.length() * 6 * size > w) size--;
			gfx->setTextColor(col);
			gfx->setTextSize(size);
			gfx->setCursor(x, y);
			gfx->print(v);
			prevData[key] = v;
		};

		// ── static frame (labels only, painted once) ────────────────
		if (!p499FrameDrawn) {
			gfx->fillScreen(BG);
			gfx->setTextSize(1);
			gfx->setTextColor(LBL);

			gfx->setCursor(  6,   4); gfx->print("LAP");
			gfx->setCursor(176,   4); gfx->print("DELTA");
			gfx->setCursor(310,   4); gfx->print("FUEL");
			gfx->setCursor(404,   4); gfx->print("ENERGY");
			gfx->drawLine(0, TOP_H - 1, W, TOP_H - 1, SEP);

			gfx->setCursor(  6, MID_Y +   4); gfx->print("TC");
			gfx->setCursor( 82, MID_Y +   4); gfx->print("TC CUT");
			gfx->setCursor(  6, MID_Y +  50); gfx->print("FL");
			gfx->setCursor( 82, MID_Y +  50); gfx->print("FR");
			gfx->setCursor(  6, MID_Y + 108); gfx->print("RL");
			gfx->setCursor( 82, MID_Y + 108); gfx->print("RR");
			gfx->setCursor(  6, MID_Y + 166); gfx->print("WEAR F");
			gfx->setCursor( 82, MID_Y + 166); gfx->print("WEAR R");

			gfx->setCursor(158, MID_Y +   4); gfx->print("MAP");
			gfx->setCursor(158, MID_Y + 158); gfx->print("POS");
			gfx->setCursor(240, MID_Y + 158); gfx->print("REGEN");
			gfx->setCursor(158, MID_Y + 202); gfx->print("BR BIAS");
			gfx->setCursor(240, MID_Y + 202); gfx->print("BRK MIG");

			gfx->setCursor(334, MID_Y +   4); gfx->print("EN/LAP");
			gfx->setCursor(416, MID_Y +   4); gfx->print("ABS");
			gfx->setCursor(334, MID_Y +  50); gfx->print("BFL");
			gfx->setCursor(410, MID_Y +  50); gfx->print("BFR");
			gfx->setCursor(334, MID_Y + 108); gfx->print("BRL");
			gfx->setCursor(410, MID_Y + 108); gfx->print("BRR");
			gfx->setCursor(334, MID_Y + 166); gfx->print("OIL");
			gfx->setCursor(410, MID_Y + 166); gfx->print("WATER");

			p499FrameDrawn = true;
		}

		// ── top strip ───────────────────────────────────────────────
		val("p_lap", 6, 14, 164, 24, 3, WHT, currentLapTime);
		bool dNeg = sessionBestLiveDeltaSeconds.indexOf('-') >= 0;
		val("p_dt", 176, 14, 126, 24, 3, dNeg ? GRN : RD, sessionBestLiveDeltaSeconds);
		val("p_fu", 310, 14,  86, 24, 3,
		    fuelRemainingLaps.toFloat() < 3.0f ? RD : WHT, fuelRemainingLaps);
		val("p_soc", 404, 14, 74, 24, 3,
		    kv > 50 ? GRN : (kv > 20 ? YLW : RD), String(kv) + "%");

		// ── left column: electronics, tyres, wear ───────────────────
		val("p_tc",   6, MID_Y +  14, 70, 24, 3, tcActive.toInt() ? ORG : WHT, tcLevel);
		val("p_tcc", 82, MID_Y +  14, 64, 24, 3, WHT, tcCut);

		val("p_pfl",  6, MID_Y +  60, 70, 18, 2, WHT, tyrePressureFrontLeft);
		val("p_tfl",  6, MID_Y +  80, 70, 16, 2,
		    tyreCol(tyreTemperatureFrontLeft.toInt()), tyreTemperatureFrontLeft + "C");
		val("p_pfr", 82, MID_Y +  60, 64, 18, 2, WHT, tyrePressureFrontRight);
		val("p_tfr", 82, MID_Y +  80, 64, 16, 2,
		    tyreCol(tyreTemperatureFrontRight.toInt()), tyreTemperatureFrontRight + "C");
		val("p_prl",  6, MID_Y + 118, 70, 18, 2, WHT, tyrePressureRearLeft);
		val("p_trl",  6, MID_Y + 138, 70, 16, 2,
		    tyreCol(tyreTemperatureRearLeft.toInt()), tyreTemperatureRearLeft + "C");
		val("p_prr", 82, MID_Y + 118, 64, 18, 2, WHT, tyrePressureRearRight);
		val("p_trr", 82, MID_Y + 138, 64, 16, 2,
		    tyreCol(tyreTemperatureRearRight.toInt()), tyreTemperatureRearRight + "C");

		val("p_wf",   6, MID_Y + 176, 70, 18, 2, CYN, tyreWearFrontLeft + "%");
		val("p_wr",  82, MID_Y + 176, 64, 18, 2, CYN, tyreWearRearLeft + "%");

		// ── centre: motor map, gear between the two bars ────────────
		val("p_map", 158, MID_Y + 14, 80, 18, 2, WHT, ersDeployMode);

		if (prevData["p_gr"] != gear) {
			gfx->fillRect(190, MID_Y + 44, 80, 68, BG);
			gfx->setTextColor(WHT);
			gfx->setTextSize(8);
			gfx->getTextBounds(gear, 0, 0, &bx, &by, &bw, &bh);
			gfx->setCursor(190 + (80 - (int)bw) / 2, MID_Y + 46);
			gfx->print(gear);
			prevData["p_gr"] = gear;
		}

		// brake pedal (left bar) and energy (right bar) — both clear of the
		// gear's band, so neither can wipe the other
		if (prevData["p_bk"] != brake) {
			int bkv = brake.toInt();
			bkv = bkv < 0 ? 0 : (bkv > 100 ? 100 : bkv);
			const int x0 = 166, y0 = MID_Y + 44, wq = 12, hq = 104;
			gfx->drawRect(x0, y0, wq, hq, SEP);
			gfx->fillRect(x0 + 1, y0 + 1, wq - 2, hq - 2, DIM);
			int fh = (bkv * (hq - 2)) / 100;
			if (fh > 0) gfx->fillRect(x0 + 1, y0 + hq - 1 - fh, wq - 2, fh, RD);
			prevData["p_bk"] = brake;
		}
		if (prevData["p_kv"] != kersLevel) {
			const int x0 = 302, y0 = MID_Y + 44, wq = 12, hq = 104;
			gfx->drawRect(x0, y0, wq, hq, SEP);
			gfx->fillRect(x0 + 1, y0 + 1, wq - 2, hq - 2, DIM);
			int fh = (kv * (hq - 2)) / 100;
			if (fh > 0) gfx->fillRect(x0 + 1, y0 + hq - 1 - fh, wq - 2, fh,
			                          kv > 50 ? GRN : (kv > 20 ? YLW : RD));
			prevData["p_kv"] = kersLevel;
		}

		val("p_pos", 158, MID_Y + 168, 76, 24, 3, YLW,
		    position.length() ? ("P" + position) : "--");
		val("p_rgn", 240, MID_Y + 168, 84, 24, 3,
		    regenLevel == "--" ? LBL : GRN, regenLevel);
		val("p_bb",  158, MID_Y + 212, 76, 18, 2, MGT, brakeBias);
		val("p_bm",  240, MID_Y + 212, 84, 18, 2,
		    brkMigration == "--" ? LBL : WHT, brkMigration);

		// ── right column: energy per lap, brakes, fluids ────────────
		val("p_enl", 334, MID_Y +  14, 76, 24, 3, WHT, fuelLitersPerLap);
		val("p_abs", 416, MID_Y +  14, 58, 24, 3,
		    absActive.toInt() ? ORG : WHT, absLevel);

		val("p_bfl", 334, MID_Y +  60, 70, 18, 2,
		    brakeCol(brakeTemperatureFrontLeft.toInt()),  brakeTemperatureFrontLeft);
		val("p_bfr", 410, MID_Y +  60, 64, 18, 2,
		    brakeCol(brakeTemperatureFrontRight.toInt()), brakeTemperatureFrontRight);
		val("p_brl", 334, MID_Y + 118, 70, 18, 2,
		    brakeCol(brakeTemperatureRearLeft.toInt()),   brakeTemperatureRearLeft);
		val("p_brr", 410, MID_Y + 118, 64, 18, 2,
		    brakeCol(brakeTemperatureRearRight.toInt()),  brakeTemperatureRearRight);

		val("p_oil", 334, MID_Y + 176, 70, 18, 2, YLW, oilTemperature);
		val("p_wat", 410, MID_Y + 176, 64, 18, 2, CYN, waterTemperature);

		// ── bottom energy bar (red → yellow → green as it fills) ────
		String barKey = String(kv);
		if (prevData["p_bar"] != barKey) {
			const int y0 = BAR_Y + 4, h0 = 16, inner = W - 4;
			gfx->drawRect(0, y0 - 2, W, h0 + 4, SEP);
			int fillW = (kv * inner) / 100;
			int z30 = inner * 30 / 100, z60 = inner * 60 / 100;
			auto zone = [&](int a, int b, uint16_t c) {
				int e = b < fillW ? b : fillW;
				if (e > a) gfx->fillRect(2 + a, y0, e - a, h0, c);
			};
			zone(0,   z30,   RD);
			zone(z30, z60,   YLW);
			zone(z60, inner, GRN);
			if (fillW < inner)
				gfx->fillRect(2 + fillW, y0, inner - fillW, h0, DIM);
			prevData["p_bar"] = barKey;
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

	void setAlertColorsFromText(const String &upperText, uint16_t &bgColor, uint16_t &textColor, bool popupDefault) {
		if (upperText.indexOf("ENGINE OFF") >= 0) {
			bgColor = RGB565(20, 20, 20);
			textColor = RED;
		} else if (upperText.indexOf("PIT LIMITER") >= 0) {
			bgColor = ORANGE;
			textColor = BLACK;
		} else if (upperText.indexOf("YELLOW FLAG") >= 0) {
			bgColor = YELLOW;
			textColor = BLACK;
		} else if (upperText.indexOf("BLUE FLAG") >= 0) {
			bgColor = BLUE;
			textColor = WHITE;
		} else if (upperText.indexOf("GREEN FLAG") >= 0 || upperText.indexOf("GREEN") >= 0) {
			bgColor = GREEN;
			textColor = BLACK;
		} else if (upperText.indexOf("LOW FUEL") >= 0) {
			bgColor = RED;
			textColor = YELLOW;
		} else {
			bgColor = popupDefault ? RGB565(50, 50, 100) : MAGENTA;
			textColor = popupDefault ? YELLOW : WHITE;
		}
	}

	void clearActiveOverlay() {
		activeOverlayText = "";
		activeOverlayBgColor = BLACK;
		activeOverlayTextColor = WHITE;
		activeOverlayUntil = 0;
		activeOverlayPriority = OVERLAY_NONE;
	}

	void latchOverlay(const String &text, uint16_t bgColor, uint16_t textColor, uint32_t durationMs, OverlayPriority priority) {
		if (text.length() == 0 || durationMs == 0) {
			return;
		}

		unsigned long now = millis();
		bool overlayActive = activeOverlayText.length() > 0 && now < activeOverlayUntil;
		bool sameOverlay = activeOverlayText == text &&
			activeOverlayBgColor == bgColor &&
			activeOverlayTextColor == textColor &&
			activeOverlayPriority == priority;

		if (overlayActive) {
			if (priority < activeOverlayPriority) {
				return;
			}
			if (sameOverlay) {
				return;
			}
		}

		activeOverlayText = text;
		activeOverlayBgColor = bgColor;
		activeOverlayTextColor = textColor;
		activeOverlayUntil = now + durationMs;
		activeOverlayPriority = priority;
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
	// True while a full-screen overlay covers the dashboard. Mirrors the
	// showingNow condition inside drawAlert().
	bool overlayIsShowing() {
		return activeOverlayText.length() > 0 && millis() < activeOverlayUntil;
	}

	// Only alerts we actually know how to label reach the screen.
	//
	// Everything on the SimHub side is a text field, so any glitch upstream — a
	// truncated frame, a template edit — lands here as an arbitrary string and used
	// to be rendered full-screen as if it meant something. That is how a session
	// clock ("05:10:15") and a fuel figure ("PENALTY: 12.62") became alerts. The
	// set below is exactly what customProtocol-dashBoard.txt can emit plus what
	// this firmware raises itself; anything else is dropped rather than shown.
	//
	// Wheel/MFC popups are NOT filtered here — those come over the local UART with
	// text the user chose, and are trusted.
	static bool isKnownAlertText(const String& upper) {
		static const char* const EXACT[] = {
			"ENGINE OFF", "PIT LIMITER", "LOW FUEL", "FINISHED",
			"GREEN FLAG", "YELLOW FLAG", "BLUE FLAG", "WHITE FLAG", "RED FLAG",
			"BLACK FLAG", "MEATBALL", "SLOW CAR"
		};
		for (uint8_t i = 0; i < sizeof(EXACT) / sizeof(EXACT[0]); i++) {
			if (upper == EXACT[i]) return true;
		}
		// Labelled value alerts: the label is fixed, the number is not.
		static const char* const PREFIX[] = { "BIAS:", "TC LEVEL:", "ABS LEVEL:", "MAP:", "PENALTY:" };
		for (uint8_t i = 0; i < sizeof(PREFIX) / sizeof(PREFIX[0]); i++) {
			if (upper.startsWith(PREFIX[i])) return true;
		}
		return false;
	}

	void drawAlert() {
		if (!canUseDisplay()) return;

		unsigned long now = millis();
		String alertNormalized = alertMessage;  // keep raw for display, normalized for checks
		alertNormalized.trim();
		String alertUpper = alertNormalized;
		alertUpper.toUpperCase();

		bool hasCriticalSimhubAlert = false;

		// PRIORIDADE 1: Alertas críticos do SimHub (alertMessage)
		// Only show if it's a real alert (not empty, not "NORMAL", not "NONE", not "0")
		// Additional safety: check if string contains only valid characters (alphanumeric, space, colon, etc)
		if (alertUpper.length() > 0 &&
			alertUpper != "NORMAL" &&
			alertUpper != "NONE" &&
			alertUpper != "0" &&
			isKnownAlertText(alertUpper) &&
			isValidAlertString(alertUpper)) {
			// alertMessage is a level signal — the template re-derives it every frame
			// the underlying condition holds, not just when it starts. Latching on
			// every frame is what kept a Full Course Yellow relatched back-to-back for
			// entire laps (each 3s pulse renewed itself before it could expire, so the
			// dashboard stayed blocked the whole time) and made AMS2's green flag pop
			// up again at every marshal post: its raw signal dips to "normal" between
			// posts, the previous pulse had already expired in that gap, so the next
			// post looked like a brand new event.
			//
			// prevAlertText already existed for exactly this ("avoid resetting timer
			// on same alert") but was only ever cleared, never compared — the guard
			// was declared and never wired in. Wiring it in makes this edge-triggered:
			// latch once when the text changes, then stay quiet for as long as the
			// exact same text keeps coming back — see the "never reset" note below and
			// the expiry handler further down for why staying quiet has to survive
			// both the condition holding for a long time (FCY) and it blinking on and
			// off (a green light at every marshal post). The LED strip keeps
			// indicating the ongoing flag colour the whole time regardless — this only
			// stops the full-screen overlay from re-blocking the panel for a
			// condition already announced.
			if (alertUpper != prevAlertText) {
				String alertText = cleanAlertText(alertNormalized);
				uint16_t bgColor = BLACK;
				uint16_t textColor = WHITE;
				setAlertColorsFromText(alertUpper, bgColor, textColor, false);
				latchOverlay(alertText, bgColor, textColor, ALERT_DURATION_MS, OVERLAY_SIMHUB_CRITICAL);
				prevAlertText = alertUpper;
			}
			hasCriticalSimhubAlert = true;
		}
		// prevAlertText is deliberately never reset to "" anywhere (see the expiry
		// handler further down for why the obvious place to do that is wrong). It
		// only ever changes by being overwritten with a genuinely different alert
		// text above, which is what lets a later, distinct alert announce normally.

		// PRIORIDADE 2: Pop-up temporário do SimHub - mensagens de mudanças menores
		String simhubPopupNormalized = popupMessage;
		simhubPopupNormalized.trim();
		String simhubPopupUpper = simhubPopupNormalized;
		simhubPopupUpper.toUpperCase();
		if (simhubPopupUpper.length() > 0 &&
			simhubPopupUpper != "NORMAL" &&
			simhubPopupUpper != "NONE" &&
			simhubPopupUpper != "0" &&
			isKnownAlertText(simhubPopupUpper) &&
			isValidAlertString(simhubPopupUpper)) {
			String alertText = cleanAlertText(simhubPopupNormalized);
			uint16_t bgColor = BLACK;
			uint16_t textColor = WHITE;
			setAlertColorsFromText(simhubPopupUpper, bgColor, textColor, true);
			latchOverlay(alertText, bgColor, textColor, ALERT_DURATION_MS, OVERLAY_SIMHUB_POPUP);
		}

		// PRIORIDADE 3: Pop-up do menu MFC/UART
		//
		// UART_POPUP_ERS/FUEL are built here, fresh, every time this runs while
		// popupFromUartUntil hasn't passed -- not once when the button press
		// arrived. The wheel only knows it fired a button; the game takes a
		// frame or two to actually change ersDeployMode/kersLevel/fuel, so a
		// value captured at press time would show the pre-press state for most
		// of the popup's life. Recomputing here means the moment the real
		// telemetry frame changes (which raises redrawPending on its own), the
		// same popup instance shows the post-press value without waiting for a
		// new button press or reopening anything.
		String uartPopupNormalized;
		if (uartPopupKind == UART_POPUP_ERS) {
			uartPopupNormalized = String(uartPopupStepUp ? "ERS ^ " : "ERS v ") +
				ersDeployMode + " " + kersLevel + "%";
		} else if (uartPopupKind == UART_POPUP_FUEL) {
			uartPopupNormalized = String(uartPopupStepUp ? "FUEL ^ " : "FUEL v ") +
				fuelRemainingLaps + "L " + fuelLitersPerLap + "L/L";
		} else if (uartPopupKind == UART_POPUP_MAP) {
			// ersDeployMode already resolves to the right per-game label --
			// see showMapStepPopup()'s comment for why this isn't [EngineMap].
			uartPopupNormalized = String(uartPopupStepUp ? "MAP ^ " : "MAP v ") + ersDeployMode;
		} else if (uartPopupKind == UART_POPUP_ABS) {
			uartPopupNormalized = String(uartPopupStepUp ? "ABS ^ " : "ABS v ") + absLevel;
		} else if (uartPopupKind == UART_POPUP_TURBO) {
			uartPopupNormalized = String(uartPopupStepUp ? "TURBO ^ " : "TURBO v ") + turboBoost + " bar";
		} else if (uartPopupKind == UART_POPUP_REGEN) {
			uartPopupNormalized = String(uartPopupStepUp ? "REGEN ^ " : "REGEN v ") + regenLevel;
		} else if (uartPopupKind == UART_POPUP_SOC) {
			uartPopupNormalized = String("SOC ") + kersLevel + "%";
		} else {
			uartPopupNormalized = uartPopupMessage;
			uartPopupNormalized.trim();
		}
		if (popupFromUart && uartPopupNormalized.length() > 0 && popupFromUartUntil > now) {
			String uartPopupUpper = uartPopupNormalized;
			uartPopupUpper.toUpperCase();
			uint16_t bgColor = BLACK;
			uint16_t textColor = WHITE;
			setAlertColorsFromText(uartPopupUpper, bgColor, textColor, true);
			latchOverlay(
				cleanAlertText(uartPopupNormalized),
				bgColor,
				textColor,
				(uint32_t)(popupFromUartUntil - now),
				OVERLAY_UART_POPUP
			);
		}

		// Check for flag changes - TODAS AS BANDEIRAS SUPORTADAS
		// Only process if flag is not "None" and not empty
		String flagValue = currentFlag;
		flagValue.trim();
		if (!hasCriticalSimhubAlert && flagValue != prevFlag && flagValue != "None" && flagValue != "") {
			prevFlag = flagValue;
			String alertText = "";
			uint16_t bgColor = BLACK;
			uint16_t textColor = WHITE;

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

			latchOverlay(alertText, bgColor, textColor, ALERT_DURATION_MS, OVERLAY_SIMHUB_CRITICAL);
		}

		// Check for penalty changes (fallback)
		// A real penalty count is a small whole number. Requiring that rejects the
		// times and decimals that a misread frame drops into this field, which is
		// what produced "PENALTY: 05:10:15" with no penalty in the game at all.
		bool penaltyIsCount = currentPenalties.length() > 0 && currentPenalties.length() <= 3;
		for (uint16_t i = 0; penaltyIsCount && i < currentPenalties.length(); i++) {
			if (!isDigit(currentPenalties[i])) penaltyIsCount = false;
		}
		if (!hasCriticalSimhubAlert && penaltyIsCount &&
		    currentPenalties != prevPenalties && currentPenalties.toInt() > 0) {
			prevPenalties = currentPenalties;
			latchOverlay(
				"PENALTY: " + currentPenalties,
				RGB565(200, 0, 0),
				WHITE,
				ALERT_DURATION_MS,
				OVERLAY_SIMHUB_CRITICAL
			);
		}

		bool showingNow = activeOverlayText.length() > 0 && now < activeOverlayUntil;

		if (showingNow) {
			alertWasShowing = true;

			// Repaint only when the overlay actually changed. Nothing draws over it
			// in between any more, so the previous paint is still on the panel.
			// Only race alerts pulse. Menu feedback from the MFC and the transient
			// SimHub popups (BIAS:, TC LEVEL:) share this same overlay path, and
			// blinking them made the selector flicker while it was being turned —
			// a confirmation flash is wanted there, not a strobe.
			const bool blinkThis = (activeOverlayPriority == OVERLAY_SIMHUB_CRITICAL);
			const uint8_t blinkPhase = blinkThis ? (uint8_t)((now / ALERT_BLINK_MS) & 1) : 0;
			const bool overlayUnchanged = (activeOverlayText == paintedOverlayText) &&
			                              (activeOverlayBgColor == paintedOverlayBg) &&
			                              (blinkPhase == paintedBlinkPhase);
			// Phase 1 swaps foreground and background, so the alert pulses while
			// staying readable in both phases.
			const uint16_t bgNow = blinkPhase ? BLACK : activeOverlayBgColor;
			const uint16_t fgNow = blinkPhase ? activeOverlayBgColor : activeOverlayTextColor;

	            if (activeOverlayText.length() > 0 && !overlayUnchanged) {
	                paintedOverlayText = activeOverlayText;
	                paintedOverlayBg = activeOverlayBgColor;
	                paintedBlinkPhase = blinkPhase;
	                pdOverlayPaints++;

	                // Wipe the whole panel before drawing the box. The box is not
	                // full-screen, and loop() stops refreshing the page underneath
	                // while an overlay is up — so without this the telemetry drawn on
	                // the frame the alert arrived stays frozen around it, which reads
	                // as values leaking through the alert.
	                gfx->fillScreen(BLACK);
                // ... (código anterior de contagem de linhas igual) ...
                int lineCount = 1;
	                for (int i = 0; i < activeOverlayText.length(); i++) {
	                    if (activeOverlayText[i] == '\n') lineCount++;
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
	                gfx->fillRect(alertX, alertY, alertWidth, alertHeight, bgNow);

	                gfx->setTextColor(fgNow);
                gfx->setTextSize(6); // Mantive grande

                int16_t x1, y1;
                uint16_t w, h;

                // Centraliza o texto dentro do novo box menor
                // Adicionei um ajuste (-5) para correção visual da fonte
                int currentY = alertY + (alertHeight - totalTextHeight) / 2 - 5;

	                String remainingText = activeOverlayText;

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
	        } else {
			if (activeOverlayText.length() > 0 && now >= activeOverlayUntil) {
				clearActiveOverlay();
			}
			if (alertWasShowing) {
			// Alert just expired - set flag to trigger full redraw in next draw() cycle
			alertWasShowing = false;
			needsFullRedraw = true;
			paintedOverlayText = "";
			paintedBlinkPhase = 0xFF;
			// prevAlertText is deliberately NOT reset here. This is the second half of
			// the FCY/AMS2-green fix above: this block runs on every natural expiry —
			// including the very first pulse of a level-triggered alert that is still
			// physically true (FCY still yellow, still under the same flag). If this
			// cleared prevAlertText, the alertMessage block up top would see it as ""
			// on the next frame and re-latch immediately, giving a fresh 3s pulse —
			// which repeats every ALERT_DURATION_MS for as long as the condition
			// holds. That was the actual mechanism behind "FCY blinked for laps": the
			// comparison added above was already being reset out from under itself
			// right here, every single expiry, before it could do its job.
			}
		}
	}
};
#endif
