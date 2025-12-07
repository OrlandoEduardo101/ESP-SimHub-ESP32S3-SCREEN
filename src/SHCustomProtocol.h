#ifndef __SHCUSTOMPROTOCOL_H__
#define __SHCUSTOMPROTOCOL_H__

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <map>
#include "logo_image.h"  // Logo image array

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
static const int CELL_HEIGHT = SCREEN_HEIGHT / ROWS;
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

	int cellTitleHeight = 0;
	bool hasReceivedData = false;
	bool displayEnabled = true;  // Display enabled for dashboard

	// Helper function to safely use display
	bool canUseDisplay() {
		return displayEnabled && gfx != nullptr;
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
		int logoY = (availableHeight - LOGO_HEIGHT) / 2;  // Center in available area

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
	}
public:
	void setup() {
		// Initialize display for dashboard
		displayEnabled = true;

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

	// Called when new data is coming from computer
	void read() {
		if (!hasReceivedData) {
			hasReceivedData = true;
			if (displayEnabled && gfx != nullptr) {
				gfx->fillScreen(BLACK);
			}
		}
		String full = "";

		speed = String(FlowSerialReadStringUntil(';').toInt());
		gear = FlowSerialReadStringUntil(';');
		rpmPercent = FlowSerialReadStringUntil(';').toInt();
		rpmRedLineSetting = FlowSerialReadStringUntil(';').toInt();
		currentLapTime = FlowSerialReadStringUntil(';');
		lastLapTime = FlowSerialReadStringUntil(';');
		bestLapTime = FlowSerialReadStringUntil(';');
		sessionBestLiveDeltaSeconds = FlowSerialReadStringUntil(';');
		sessionBestLiveDeltaProgressSeconds = FlowSerialReadStringUntil(';');
		tyrePressureFrontLeft  = FlowSerialReadStringUntil(';');
		tyrePressureFrontRight  = FlowSerialReadStringUntil(';');
		tyrePressureRearLeft  = FlowSerialReadStringUntil(';');
		tyrePressureRearRight  = FlowSerialReadStringUntil(';');
		tcLevel  = FlowSerialReadStringUntil(';');
		tcActive  = FlowSerialReadStringUntil(';');
		absLevel  = FlowSerialReadStringUntil(';');
		absActive  = FlowSerialReadStringUntil(';');
		isTCCutNull  = FlowSerialReadStringUntil(';');
		tcTcCut  = FlowSerialReadStringUntil(';');
		brakeBias  = FlowSerialReadStringUntil(';');
		brake  = FlowSerialReadStringUntil(';');
		lapInvalidated  = FlowSerialReadStringUntil(';');

		const String rest = FlowSerialReadStringUntil('\n');
	}

	// Called once per arduino loop, timing can't be predicted,
	// but it's called between each command sent to the arduino
	void loop() {
		if (!hasReceivedData) {
			return;
		}
		drawRpmMeter(0, 0, SCREEN_WIDTH, CELL_HEIGHT);
		// this takes 2 cells in height, hence CELL_HEIGHT is the half point
		drawGear(COL[2] + HALF_CELL_WIDTH, ROW[1] + CELL_HEIGHT);

		// First+Second Column (Lap times)
		drawCell(COL[0], ROW[1], bestLapTime, "bestLapTime", "Best Lap", "left");
		drawCell(COL[0], ROW[2], lastLapTime, "lastLapTime", "Last Lap", "left");
		drawCell(COL[0], ROW[3], currentLapTime, "currenLapTime", "Current Lap", "left", lapInvalidated == "True" ? RED : WHITE);

		// Third Column (speed)
		drawCell(COL[2], ROW[3], speed, "speed", "Speed", "center");

		// Fourth+Fifth Column (delta)
		drawCell(SCREEN_WIDTH, ROW[1], sessionBestLiveDeltaSeconds, "sessionBestLiveDeltaSeconds", "Delta", "right", sessionBestLiveDeltaSeconds.indexOf('-') >= 0 ? GREEN : RED);
		drawCell(SCREEN_WIDTH, ROW[2], sessionBestLiveDeltaProgressSeconds, "sessionBestLiveDeltaProgressSeconds", "Delta P", "right", sessionBestLiveDeltaProgressSeconds.indexOf('-') >= 0 ? GREEN : RED);


		// (TC, ABS, BB)
		if (isTCCutNull == "False") {
			drawCell(COL[0], ROW[4], tcTcCut, "tcTcCut", "TC TC2", "center", YELLOW);
		} else {
			drawCell(COL[0], ROW[4], tcLevel, "tcLevel", "TC", "center", YELLOW);
		}
		drawCell(COL[1], ROW[4], absLevel, "absLevel", "ABS", "center", BLUE);
		drawCell(COL[2], ROW[4], brakeBias, "brakeBias", "BB", "center", MAGENTA);

		// (tyre pressure)
		drawCell(COL[3], ROW[3], tyrePressureFrontLeft, "tyrePressureFrontLeft", "FL", "center", CYAN);
		drawCell(COL[4], ROW[3], tyrePressureFrontRight, "tyrePressureFrontRight", "FR", "center", CYAN);
		drawCell(COL[3], ROW[4], tyrePressureRearLeft, "tyrePressureRearLeft", "RL", "center", CYAN);
		drawCell(COL[4], ROW[4], tyrePressureRearRight, "tyrePressureRearRight", "RR", "center", CYAN);
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
};
#endif
