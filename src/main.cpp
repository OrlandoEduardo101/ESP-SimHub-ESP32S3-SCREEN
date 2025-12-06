#include <Arduino.h>
#include <WiFi.h>  // Needed for getUniqueId() even if WiFi is disabled
#include <EspSimHub.h>
#include <Arduino_GFX_Library.h>

#define DEVICE_NAME "ESP-SimHubDisplay"

// Display configuration - uncomment the one you're using
// For WT32-SC01 Plus (ST7796, 320x480 portrait, 480x320 landscape)
// Note: We're using USB-Serial/JTAG (not USB CDC) for better Windows compatibility
// Display is configured for landscape (horizontal) orientation
#define PIXEL_WIDTH 480   // Landscape width
#define PIXEL_HEIGHT 320  // Landscape height
#define SCREEN_WIDTH_MM 70 // approximate screen width in mm for WT32-SC01 Plus
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

#include <FlowSerialRead.h>
#include <SHCustomProtocol.h>

#ifdef INCLUDE_RGB_LEDS_NEOPIXELBUS
// Configure it here!
#include <NeoPixelBusLEDs.h>
#endif

SHCustomProtocol shCustomProtocol;
char loop_opt;
char xactionc;
unsigned long lastSerialActivity = 0;

void idle(bool critical);


// Don't change this
#define VERSION 'j'

#include <SHCommands.h>

void setup(void)
{
  // ESP32-S3 uses USB-Serial/JTAG by default (no special configuration needed)
  // This is the "USB JTAG/serial debug unit" that Windows recognizes automatically
  // Give it a moment to initialize, especially on Windows
  delay(1000);

  // Initialize WiFi (needed for getUniqueId() even if not using WiFi)
  // This allows us to get the MAC address for device identification
  WiFi.mode(WIFI_OFF); // Turn off WiFi but keep it available for MAC address

  // Initialize Serial - USB-Serial/JTAG needs Serial.begin() to be called
  Serial.begin(19200);
  delay(500); // Give Serial time to be ready

  // FlowSerialBegin initializes the ARQSerial library
  FlowSerialBegin(19200);

  shCustomProtocol.setup();
  arqserial.setIdleFunction(idle);

#ifdef INCLUDE_RGB_LEDS_NEOPIXELBUS
  neoPixelBusBegin();
#endif

#if INCLUDE_WIFI
#if DEBUG_TCP_BRIDGE
	Serial.begin(115200);
#endif
#endif

#if INCLUDE_WIFI
	ECrowneWifi::setup(&outgoingStream, &incomingStream, gfx);
#endif
}

void loop()
{
#if INCLUDE_WIFI
	ECrowneWifi::loop();
#endif

  shCustomProtocol.loop();

	if (FlowSerialAvailable() > 0) {
		int header = FlowSerialTimedRead();
		if (header == MESSAGE_HEADER)
		{
			lastSerialActivity = millis();
			// Read command
			loop_opt = FlowSerialTimedRead();
			switch(loop_opt) {
				case '1': Command_Hello(); break;
				case '0': Command_Features(); break;
				case '4': Command_RGBLEDSCount(); break;
				case '6': Command_RGBLEDSData(); break;
				case 'X': {
					String xaction = FlowSerialReadStringUntil(' ', '\n');
					if (xaction == F("list")) Command_ExpandedCommandsList();
					else if (xaction == F("mcutype")) Command_MCUType();
				}
				break;
				case 'N': Command_DeviceName(); break;
				case 'I': Command_UniqueId(); break;
				case '8': Command_SetBaudrate(); break;
				case 'J': Command_ButtonsCount(); break;
				case '2': Command_TM1638Count(); break;
				case 'B': Command_SimpleModulesCount(); break;
				case 'A': Command_Acq(); break;
				case 'G': Command_GearData(); break;
				case 'P': Command_CustomProtocolData(); break;
				default:
					break;
			}
		}
	}
}

void idle(bool critical) {
#if INCLUDE_WIFI
	yield();
	ECrowneWifi::flush();
#endif

	shCustomProtocol.idle();
}
