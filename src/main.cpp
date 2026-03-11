#include <Arduino.h>
#include <EspSimHub.h>
#include <Arduino_GFX_Library.h>
#include "esp_system.h"

#define DEVICE_NAME "ESP-SimHubDisplay"
#define PIXEL_WIDTH 480
#define PIXEL_HEIGHT 272
#define SCREEN_WIDTH_MM 95 // only the screen area with pixels, in mm. Can be approximate, used to calculate density
#define PIXEL_PER_MM (PIXEL_WIDTH / SCREEN_WIDTH_MM)

#define INCLUDE_WIFI false
// Less secure if you plan to commit or share your files, but saves a bunch of memory.
//  If you hardcode credentials the device will only work in your network
#define USE_HARDCODED_CREDENTIALS false

// Enable NeoPixelBus for WS2812B RGB LEDs (recommended method)
// Configure LED strip in src/NeoPixelBusLEDs.h
#define INCLUDE_RGB_LEDS_NEOPIXELBUS

#if INCLUDE_WIFI
#if USE_HARDCODED_CREDENTIALS
#define WIFI_SSID "Wifi NAME"
#define WIFI_PASSWORD "WiFi Password"
#endif

#define BRIDGE_PORT 10001 // Perle TruePort uses port 10,001 for the first serial routed to the client
#define DEBUG_TCP_BRIDGE false // emits extra events to Serial that show network communication, set to false to save memory and make faster

#include <TcpSerialBridge2.h>
#include <ECrowneWifi.h>
#include <FullLoopbackStream.h>
FullLoopbackStream outgoingStream;
FullLoopbackStream incomingStream;

#endif // INCLUDE_WIFI

// FlowSerial uses default Serial (USB CDC)

#include <FlowSerialRead.h>

#ifdef INCLUDE_RGB_LEDS_NEOPIXELBUS
// Configure it here!
#include <NeoPixelBusLEDs.h>
#endif

#include <SHCustomProtocol.h>

SHCustomProtocol shCustomProtocol;
char loop_opt;
char xactionc;
unsigned long lastSerialActivity = 0;

RTC_DATA_ATTR uint32_t bootCount = 0;

// Dummy debug port (no actual output)
Stream* DebugPort = nullptr;

// ==========================================
// UART0 — Bidirectional link to ButtonBox wheel
// Connected via WT32 Debug Header:
//   TXD0 = GPIO 43 (pin 3) → Wheel RX (GPIO 11)
//   RXD0 = GPIO 44 (pin 4) ← Wheel TX (GPIO 43)
// Debug logs go to Serial (USB CDC) only.
// ==========================================

HardwareSerial WheelSerial(0);  // UART0 on debug header pins 43/44

void handleButtonBoxUart();
void processButtonBoxLine(const String &line);

// Debug logging function - sends to USB CDC Serial only
// (WheelSerial carries wheel protocol data, not debug)
void debugLog(const String &msg) {
	Serial.println(msg);
	Serial.flush();
}

void idle(bool critical);

// Don't change this
#define VERSION 'j'

#include <SHCommands.h>

// Simple helper to mirror logs to both Serial (if alive) and the LCD terminal
void screenLog(const String &msg) {
	if (gfx) {
		terminalPrintln(msg, gfx);
	}
	Serial.println(msg);
	// NO delay - prevents watchdog timeout
}

const char* resetReasonStr(esp_reset_reason_t reason) {
	switch (reason) {
		case ESP_RST_POWERON: return "POWERON";
		case ESP_RST_EXT:     return "EXT";
		case ESP_RST_SW:      return "SW";
		case ESP_RST_PANIC:   return "PANIC";
		case ESP_RST_INT_WDT: return "INT_WDT";
		case ESP_RST_TASK_WDT:return "TASK_WDT";
		case ESP_RST_WDT:     return "WDT";
		case ESP_RST_DEEPSLEEP:return "DEEPSLEEP";
		case ESP_RST_BROWNOUT:return "BROWNOUT";
		case ESP_RST_SDIO:    return "SDIO";
		default:              return "UNKNOWN";
	}
}

void setup(void)
{
	// Configure GPIO0 with strong pull-up to prevent entering download mode
	// This is critical for WT32-SC01 Plus when SimHub opens the serial port
	pinMode(0, INPUT_PULLUP);

	// Small delay to stabilize GPIO0 before USB CDC initialization
	delay(100);

	// Initialize Serial FIRST with explicit begin()
	Serial.begin(115200);
	Serial.setDebugOutput(true);

	// Wait for USB CDC to enumerate - critical for USB CDC mode
	// This blocks until the host recognizes the device
	unsigned long start = millis();
	while (!Serial && (millis() - start) < 5000) {
		delay(10);
	}

	// Additional delay to stabilize
	delay(1000);

	// FLUSH any garbage
	while (Serial.available()) Serial.read();
	Serial.flush();
	delay(200);

	// ==========================================
	// Initialize UART0 for wheel communication via debug header
	// TX=GPIO43 (pin 3 debug) → Wheel RX
	// RX=GPIO44 (pin 4 debug) ← Wheel TX
	// ==========================================
	WheelSerial.begin(115200, SERIAL_8N1, 44, 43);  // RX=44, TX=43
	debugLog("\n========================================");
	debugLog("UART0 WheelSerial init TX=43 RX=44 (debug header, wheel RX=11)");
	debugLog("========================================\n");

	// Print immediate boot message
	Serial.print("\n\n========================================\n");
	Serial.print(">>> ESP32-S3 BOOT SEQUENCE START\n");
	Serial.print(">>> Millis: ");
	Serial.println(millis());
	Serial.print(">>> Setup() starting...\n");
	Serial.flush();
	delay(100);

	bootCount++;
	Serial.print(">>> bootCount = ");
	Serial.println(bootCount);
	Serial.flush();
	delay(100);

	screenLog("BOOT: start #" + String(bootCount));
	screenLog(String("Reset:") + resetReasonStr(esp_reset_reason()));
	screenLog("Heap=" + String(ESP.getFreeHeap()) + " PSRAM=" + String(ESP.getFreePsram()));

	// Bring up display and protocol so we can show logs on screen
	Serial.println(">>> About to call shCustomProtocol.setup()");
	debugLog(">>> About to call shCustomProtocol.setup()");
	Serial.flush();
	delay(100);

	shCustomProtocol.setup();

	Serial.println(">>> shCustomProtocol.setup() completed");
	debugLog(">>> shCustomProtocol.setup() completed");
	Serial.flush();
	delay(100);

	arqserial.setIdleFunction(idle);
	screenLog("Display init OK");
	debugLog("Display init OK");

	// Initialize NeoPixel LED strip
	#ifdef INCLUDE_RGB_LEDS_NEOPIXELBUS
	Serial.println(">>> [STEP 1] About to init NeoPixel LEDs...");
	Serial.flush();
	debugLog(">>> [STEP 1] About to init NeoPixel LEDs...");
	delay(100);

	Serial.println(">>> [STEP 2] Calling neoPixelBusBegin()...");
	Serial.flush();
	debugLog(">>> [STEP 2] Calling neoPixelBusBegin()...");
	delay(100);

	neoPixelBusBegin();

	Serial.println(">>> [STEP 3] neoPixelBusBegin() returned successfully");
	Serial.flush();
	debugLog(">>> [STEP 3] neoPixelBusBegin() returned successfully");
	screenLog("LEDs init OK");
	#endif

	Serial.println(">>> Setup complete!");
	debugLog(">>> Setup complete!");
	Serial.println("========================================\n");
	Serial.flush();
}

void loop()
{
  static bool waitingLogged = false;

  if (!waitingLogged && millis() > 500) {
    screenLog("Aguardando SimHub...");
    waitingLogged = true;
  }

#if INCLUDE_WIFI
  ECrowneWifi::loop();
#endif

  // Re-enabled: display updates
  shCustomProtocol.loop();
  yield(); // Feed watchdog

	if (FlowSerialAvailable() > 0) {
		int r = FlowSerialTimedRead();

		if (r == MESSAGE_HEADER)
		{
			lastSerialActivity = millis();
			loop_opt = FlowSerialTimedRead();

			// PROTECTION: If command byte is 0x01, this is actually an ARQ packet being re-read
			// Skip processing to avoid infinite loops
			if (loop_opt == 0x01) {
				yield();
				return;
			}

			yield(); // Feed watchdog

			switch(loop_opt) {
				case '1':
					Command_Hello();
					yield();
					break;
				case '0':
					Command_Features();
					break;
				case '4': Command_RGBLEDSCount(); break;
				case '6': Command_RGBLEDSData(); break;
				case 'X': {
					String xaction = FlowSerialReadStringUntil(' ', '\n');
					if (xaction == F("list")) Command_ExpandedCommandsList();
					else if (xaction == F("mcutype")) Command_MCUType();
					FlowSerialWrite(0x15);  // Send ACK
				}
				break;
			case 'N': Command_DeviceName(); FlowSerialWrite(0x15); break;
			case 'I': Command_UniqueId(); FlowSerialWrite(0x15); break;
			case '8': Command_SetBaudrate(); break;
			case 'J': Command_ButtonsCount(); break;
			case '2': Command_TM1638Count(); break;
			case 'B': Command_SimpleModulesCount(); break;
			case 'A': Command_Acq(); break;
			case 'G': Command_GearData(); break;
			case 'P':
				Command_CustomProtocolData();
				break;
			default:
				break;
			}
		}
	}

	// Processa mensagens do ButtonBox via UART
	handleButtonBoxUart();
}

void idle(bool critical) {
	yield(); // Feed watchdog
	shCustomProtocol.idle();  // Re-enabled: idle updates
}

// ================================
// UART ButtonBox -> WT32 Popups
// ================================
String buttonBoxLine;

void handleButtonBoxUart() {
	while (WheelSerial.available()) {
		char c = (char)WheelSerial.read();
		if (c == '\r') {
			continue;
		}
		if (c == '\n') {
			if (buttonBoxLine.length() > 0) {
				processButtonBoxLine(buttonBoxLine);
			}
			buttonBoxLine = "";
			continue;
		}
		if (buttonBoxLine.length() < 80) {
			buttonBoxLine += c;
		}
	}
}

void processButtonBoxLine(const String &line) {
	if (!line.startsWith("$")) {
		return;
	}
	int p1 = line.indexOf(':', 1);
	int p2 = (p1 >= 0) ? line.indexOf(':', p1 + 1) : -1;
	if (p1 < 0 || p2 < 0) {
		return;
	}
	String cat = line.substring(1, p1);
	String func = line.substring(p1 + 1, p2);
	String val = line.substring(p2 + 1);
	cat.trim();
	func.trim();
	val.trim();

	// UART handshake: Wheel -> WT32 ping, WT32 -> Wheel pong
	// Expected incoming: $BB:PING:<seq>
	if (cat == "BB" && func == "PING") {
		String pong = String("$WT:PONG:") + val + "\n";
		WheelSerial.print(pong);  // UART0 TX=GPIO43 (debug header pin 3)
		debugLog(String("[UART] RX ping seq=") + val + " -> TX pong via GPIO43");
		return;
	}

	String msg;
	if (cat == "MODE" && func == "ENC") {
		msg = String("ENC: ") + val;
	} else if (cat == "MFC" && func == "NAV") {
		msg = String("MFC: ") + val;
	} else if (cat == "MFC" && func == "ADJUST") {
		msg = String("ADJUST: ") + val;
	} else if (cat == "MFC" && func == "CONFIRM") {
		// Legacy compatibility
		if (val == "PAGE_NEXT") {
			shCustomProtocol.pageNextExternal();
			msg = "PAGE +";
		} else if (val == "PAGE_PREV") {
			shCustomProtocol.pagePrevExternal();
			msg = "PAGE -";
		} else {
			msg = String("MFC OK: ") + val;
		}
	} else if (cat == "BRIGHT" && func == "VAL") {
		int brightVal = val.toInt();
		if (brightVal >= 15 && brightVal <= 255) {
			shCustomProtocol.setBacklight(brightVal);
			shCustomProtocol.adjustLedLuminance(0);
			msg = String("BRIGHT ") + shCustomProtocol.getBacklightPercent() + "%";
		} else {
			msg = "BRIGHT: invalid";
		}
	} else if (cat == "PAGE" && func == "NEXT") {
		shCustomProtocol.pageNextExternal();
		msg = "";
	} else if (cat == "PAGE" && func == "PREV") {
		shCustomProtocol.pagePrevExternal();
		msg = "";
	} else if (cat == "BITE" && func == "VAL") {
		msg = String("BITE: ") + val + "%";
	} else if (cat == "CLUTCH" && func == "MODE") {
		msg = String("CLUTCH: ") + val;
	} else if (cat == "CALIB" && func == "START") {
		msg = String("CALIB: ") + val;
	} else if (cat == "CALIB" && func == "DONE") {
		msg = String("CALIB OK: ") + val;
	} else if (cat == "CALIB" && func == "INVALID") {
		msg = String("CALIB ERR: ") + val;
	} else if (cat == "SYS" && func == "BOOT") {
		msg = String("BOOT: ") + val;
	} else if (cat == "SYS" && func == "RESET") {
		msg = String("RESET: ") + val;
	} else if (cat == "MEDIA") {
		msg = String("MEDIA: ") + func;
	} else {
		msg = cat + ":" + func + ":" + val;
	}

	if (msg.length() > 0) {
		shCustomProtocol.showPopup(msg, 2000);
	}
}
