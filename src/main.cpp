#include <Arduino.h>
#include <EspSimHub.h>
#include <Arduino_GFX_Library.h>
#include <Preferences.h>
#include "esp_system.h"

#define DEVICE_NAME "ESP-SimHubDisplay"
#define PIXEL_WIDTH 480
#define PIXEL_HEIGHT 272
#define SCREEN_WIDTH_MM 95 // only the screen area with pixels, in mm. Can be approximate, used to calculate density
#define PIXEL_PER_MM (PIXEL_WIDTH / SCREEN_WIDTH_MM)

// Less secure if you plan to commit or share your files, but saves a bunch of memory.
//  If you hardcode credentials the device will only work in your network
#define USE_HARDCODED_CREDENTIALS false

// Enable NeoPixelBus for WS2812B RGB LEDs (recommended method)
// Configure LED strip in src/NeoPixelBusLEDs.h
#define INCLUDE_RGB_LEDS_NEOPIXELBUS

#if USE_HARDCODED_CREDENTIALS
#define WIFI_SSID "Wifi NAME"
#define WIFI_PASSWORD "WiFi Password"
#endif

#define BRIDGE_PORT 10001 // Perle TruePort uses port 10,001 for the first serial routed to the client

// Raw transport listener. A client here speaks the same bytes SimHub does
// (0x03 'P' + ';'-separated fields) but without the ARQ framing, so no
// per-packet acks and no stop-and-wait round trips. See ArqSerial.h.
#define RAW_BRIDGE_PORT 10002

// arqRawMode and rawFrameEnded are defined in ArqSerial.h (the wheel firmware
// includes that header too and needs them). Only the frame tally is ours.
volatile int rawFramesPending = 0;
volatile bool rawResyncNeeded = false;
volatile uint32_t rawOverflows = 0;
volatile uint32_t rawMaxChunk = 0;
volatile uint32_t rawMaxUsed = 0;
volatile uint32_t rawChunks = 0;
volatile uint32_t rawMaxGapMs = 0;
volatile uint32_t rawLongGaps = 0;
volatile uint32_t rawMultiFrameChunks = 0;
volatile uint32_t rawLastChunkMs = 0;

// How many frames may sit queued before we start skipping to the newest.
#define RAW_FRAME_BACKLOG 4
#define DEBUG_TCP_BRIDGE false // emits extra events to Serial that show network communication, set to false to save memory and make faster

// OTA firmware updates over the same opt-in WiFi link (only active while
// wifiTransportActive is true — see setup()). CHANGE THIS before relying on
// it: anyone on the WiFi network with this password can reflash the board.
// Must match platformio.ini's [env:wt32-sc01-plus-ota] upload_flags --auth=.
#define OTA_PASSWORD "changeme-simhub-wt32"
#include <ArduinoOTA.h>

// ==========================================
// Optional WiFi telemetry link (opt-in, default OFF)
// WiFi library code is always compiled in (flash cost only), but the radio
// and TCP bridge are only started when wifiTransportActive is true — loaded
// from NVS, default false. This keeps USB CDC as the default SimHub
// transport with zero behavior change unless the user explicitly enables
// WiFi (via the wheel's hidden gesture, see processButtonBoxLine()) and
// reboots. See docs/WIRELESS.md for the full design rationale.
// ==========================================
#include <TcpSerialBridge2.h>
#include <FullLoopbackStream.h>
// FullLoopbackStream defaults to a 64-byte ring buffer (LoopbackStream's
// DEFAULT_SIZE). A single custom-protocol telemetry frame from SimHub is
// ~270 bytes and arrives over TCP in one burst — unlike real serial, which
// paces bytes out at the configured baud rate, TCP hands the whole payload
// to handleData() at once. Past the first 64 bytes, every write silently
// failed and FullLoopbackStream::write() dropped the rest of the frame
// (LoopbackStream.h: "If the buffer overflows, the last bytes written are
// lost"), truncating the ARQ framing mid-message. That, not WiFi latency,
// was the real cause of the ~3 FPS ceiling and the high corrupted/retransmit
// counts in SimHub — WiFi.setSleep(false) alone did not fix it. Sized well
// above the largest known frame for headroom as the protocol grows; cost is
// ~1KB extra RAM per buffer, trivial against the ~280KB free.
FullLoopbackStream outgoingStream(1024);
// Sized for a couple of dozen whole frames (~300 bytes each). The virtual COM
// port delivers in bursts, so headroom here is what keeps a burst from being
// truncated mid-frame; measured 37 overflows in 8s at 4096 with a 20Hz sender.
// Headroom does not fix a sender that is permanently faster than the panel,
// though — for that the send rate has to come down.
FullLoopbackStream incomingStream(8192);
bool wifiTransportActive = false; // set from NVS in setup(), before ArqSerial is touched
Preferences wifiPrefs;

#include <ECrowneWifi.h>

// Runtime dispatch for the ARQ transport: USB Serial (default) or the WiFi
// loopback streams (opt-in). Must be defined before FlowSerialRead.h pulls
// in ArqSerial.h, since these override its Serial-default macros.
int wsStreamRead() {
	if (!wifiTransportActive) return Serial.read();
	int c = incomingStream.read();
	if (c == RAW_FRAME_TERMINATOR) __atomic_sub_fetch(&rawFramesPending, 1, __ATOMIC_RELAXED);
	return c;
}
int wsStreamAvailable() { return wifiTransportActive ? incomingStream.available() : Serial.available(); }
size_t wsStreamWrite(uint8_t b) { return wifiTransportActive ? outgoingStream.write(b) : Serial.write(b); }
size_t wsStreamWrite(const char* str) { return wifiTransportActive ? outgoingStream.write(str) : Serial.write(str); }
void wsStreamFlush() { if (wifiTransportActive) ECrowneWifi::flush(); else Serial.flush(); }
size_t wsStreamPrint(const String &s) { return wifiTransportActive ? outgoingStream.print(s) : Serial.print(s); }
size_t wsStreamPrint(const char* str) { return wifiTransportActive ? outgoingStream.print(str) : Serial.print(str); }
size_t wsStreamPrint(char c) { return wifiTransportActive ? outgoingStream.print(c) : Serial.print(c); }
void wsFlowSerialBegin(unsigned long baud) { if (!wifiTransportActive) Serial.begin(baud); }

#define StreamRead wsStreamRead
#define StreamAvailable wsStreamAvailable
#define StreamWrite wsStreamWrite
#define StreamFlush wsStreamFlush
#define StreamPrint wsStreamPrint
#define FlowSerialBegin wsFlowSerialBegin
#define FlowSerialFlush wsStreamFlush

#include <FlowSerialRead.h>

#ifdef INCLUDE_RGB_LEDS_NEOPIXELBUS
// Configure it here!
#include <NeoPixelBusLEDs.h>
#endif

#include <SHCustomProtocol.h>
#include <Buzzer.h>

SHCustomProtocol shCustomProtocol;
char loop_opt;
char xactionc;
unsigned long lastSerialActivity = 0;

RTC_DATA_ATTR uint32_t bootCount = 0;

// Dummy debug port (no actual output). The per-byte tracer behind this is
// far too slow to leave on while measuring timing — see ARQ_TIMING_TRACE in
// lib/EspSimHub/ArqSerial.h for the lightweight per-packet timing probe used
// to diagnose the WiFi throughput problem instead.
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

	// Initialize buzzer (GPIO 11, EXT_IO2) — active buzzer via NPN transistor
	buzzerInit();

	// Small delay to stabilize GPIO0 before USB CDC initialization
	delay(100);

	// Initialize Serial FIRST with explicit begin()
	Serial.begin(115200);
	Serial.setDebugOutput(true);

	// The wireless transport preference decides whether the USB CDC is this
	// session's data link or merely a debug console, so read it before the
	// first log line. In WiFi mode nothing on the PC holds the CDC open, its
	// ring buffer fills, and from then on every write blocks for tx_timeout_ms
	// (100ms by default, and flush() waits again) — enough setup logging to add
	// seconds to boot, and enough per-packet logging to swamp the ARQ timing if
	// a diagnostic build turns it back on. Zeroing it makes stray prints drop
	// instead of stall. Never do this on the USB path: there Serial *is* the
	// transport and blocking until the host drains it is the correct behaviour.
	wifiPrefs.begin("screen", false);
	wifiTransportActive = wifiPrefs.getBool("wifiEn", false);
	if (wifiTransportActive) {
		Serial.setTxTimeoutMs(0);
	}

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

	// ==========================================
	// Optional WiFi telemetry link (opt-in, default OFF, applied at boot only)
	// See the comment above the includes near the top of this file.
	// ==========================================
	// wifiTransportActive is read from NVS at the top of setup(), so that the
	// CDC tx timeout is already settled before anything logs.
	if (wifiTransportActive) {
		screenLog("WiFi mode: connecting...");
		debugLog(">>> WiFi enabled - starting ECrowneWifi::setup() (may block up to ~120s on first-time provisioning)");
		ECrowneWifi::setup(&outgoingStream, &incomingStream, gfx);
		// The ARQ protocol (lib/EspSimHub/ArqSerial.h) is a stop-and-wait
		// scheme tuned for USB's sub-ms round trips. WiFi modem sleep (the
		// default) makes the radio nap between packets, pushing RTT to
		// 40-120ms — the ARQ layer times out and retransmits before the ack
		// even arrives, tanking throughput (observed: ~2.8 FPS in SimHub,
		// ~2/3 of packets retransmitted). Disabling sleep trades a bit more
		// idle power draw for USB-like responsiveness.
		WiFi.setSleep(false);

		ArduinoOTA.setHostname("esp-simhub-display");
		ArduinoOTA.setPassword(OTA_PASSWORD);
		ArduinoOTA.onStart([]() {
			screenLog("OTA: update starting...");
		});
		ArduinoOTA.onEnd([]() {
			screenLog("OTA: update done, rebooting");
		});
		ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
			static uint8_t lastPct = 255;
			uint8_t pct = (total > 0) ? (uint8_t)((progress * 100) / total) : 0;
			if (pct != lastPct) {
				lastPct = pct;
				screenLog("OTA: " + String(pct) + "%");
			}
		});
		ArduinoOTA.onError([](ota_error_t error) {
			screenLog("OTA: error " + String((int)error));
		});
		ArduinoOTA.begin();
		debugLog(">>> ArduinoOTA ready (hostname=esp-simhub-display)");

		screenLog("WiFi mode: ready");
	}

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

	// Boot beep — confirms the buzzer is wired and firmware is running
	buzzerBeep(100);

	Serial.println(">>> Setup complete!");
	debugLog(">>> Setup complete!");
	Serial.println("========================================\n");
	Serial.flush();
}

// PERF_DIAG: a text dump of where loop() time goes, served on TCP 10002 so it
// can be read over WiFi without disturbing the SimHub link on 10001. Temporary
// — delete this block, the counters below and the pd* fields in
// SHCustomProtocol once the hot phase is known.
static WiFiServer perfServer(10004);
static bool perfServerStarted = false;
static uint32_t perfLoops = 0, perfLoopMaxUs = 0, perfUartUs = 0;
// perfShortFrames counts frames that ended before the parser had all its fields
// (sender emits fewer than the parser reads) — those show blank trailing values.
// perfFullFrames counts the normal case, where the parser got everything and the
// terminator was still waiting to be discarded.
static uint32_t perfShortFrames = 0, perfFullFrames = 0;
static unsigned long perfSinceMs = 0;

static void perfDiagHandle()
{
	if (!perfServerStarted) {
		perfServer.begin();
		perfServer.setNoDelay(true);
		perfServerStarted = true;
		perfSinceMs = millis();
	}
	WiFiClient c = perfServer.available();
	if (!c) return;

	const uint32_t f = shCustomProtocol.pdFrames ? shCustomProtocol.pdFrames : 1;
	const unsigned long win = millis() - perfSinceMs;
	c.printf("window_ms=%lu loops=%lu loop_max_us=%lu uart_us_total=%lu\n",
	         win, (unsigned long)perfLoops, (unsigned long)perfLoopMaxUs,
	         (unsigned long)perfUartUs);
	c.printf("frames=%lu  fps=%.1f  frame_max_us=%lu\n",
	         (unsigned long)shCustomProtocol.pdFrames,
	         win ? shCustomProtocol.pdFrames * 1000.0 / win : 0.0,
	         (unsigned long)shCustomProtocol.pdFrameMaxUs);
	c.printf("avg per frame (us): page=%lu alert=%lu indicator=%lu leds=%lu\n",
	         (unsigned long)(shCustomProtocol.pdPageUs / f),
	         (unsigned long)(shCustomProtocol.pdAlertUs / f),
	         (unsigned long)(shCustomProtocol.pdIndUs / f),
	         (unsigned long)(shCustomProtocol.pdLedUs / f));
	c.printf("canvas=%s raw_mode=%d overlay_paints=%lu short=%lu full=%lu overflow=%lu\n",
	         canvas ? "present" : "nullptr", arqRawMode ? 1 : 0,
	         (unsigned long)shCustomProtocol.pdOverlayPaints,
	         (unsigned long)perfShortFrames, (unsigned long)perfFullFrames,
	         (unsigned long)rawOverflows);
	c.printf("chunks=%lu max_chunk=%lu max_used=%lu\n",
	         (unsigned long)rawChunks, (unsigned long)rawMaxChunk, (unsigned long)rawMaxUsed);
	c.printf("gap_max_ms=%lu gaps_over_100ms=%lu multi_frame_chunks=%lu\n",
	         (unsigned long)rawMaxGapMs, (unsigned long)rawLongGaps,
	         (unsigned long)rawMultiFrameChunks);
	c.println(shCustomProtocol.perfFieldDump());
	c.print("prevAlertText=["); c.print(shCustomProtocol.getPrevAlertTextDiag()); c.println("]");
	c.println(shCustomProtocol.pageChangeDump());

	c.flush();
	c.stop();

	shCustomProtocol.pdFrames = 0;
	shCustomProtocol.pdPageUs = shCustomProtocol.pdAlertUs = 0;
	shCustomProtocol.pdIndUs = shCustomProtocol.pdLedUs = 0;
	shCustomProtocol.pdFrameMaxUs = 0;
	shCustomProtocol.pdOverlayPaints = 0;
	perfLoops = perfLoopMaxUs = perfUartUs = 0;
	perfShortFrames = perfFullFrames = 0;
	rawMaxChunk = rawMaxUsed = rawChunks = rawOverflows = 0;
	rawMaxGapMs = rawLongGaps = rawMultiFrameChunks = 0;
	perfSinceMs = millis();
}

void loop()
{
  static bool waitingLogged = false;
  const uint32_t perfT0 = micros();

  buzzerUpdate(); // Non-blocking buzzer timeout handler

  if (!waitingLogged && millis() > 500) {
    screenLog("Aguardando SimHub...");
    waitingLogged = true;
  }

  if (wifiTransportActive) {
    ECrowneWifi::loop();
    ArduinoOTA.handle();
    perfDiagHandle();
  }

  // Re-enabled: display updates
  shCustomProtocol.loop();
  yield(); // Feed watchdog

	if (arqRawMode) {
		// Raw transport: this stream carries telemetry frames and nothing else,
		// so there is no command byte to dispatch on and the 0x03/'P' header is
		// pure cost. Dropping it lets the sender be SimHub's stock custom
		// protocol template with nothing prepended, which matters because
		// SimHub's NCalc has no chr() — a header would have forced the whole
		// 72-field formula to be rewritten in JavaScript just to emit one byte.
		// Parse only once a WHOLE frame has arrived. Starting on a partial frame
		// makes the last fields time out empty and leaves the remainder in the
		// buffer, so every later frame is read one field out of step — which is
		// the scrambled layout, not a transport fault.
		int c;
		int guard = 0;

		// The buffer overflowed, so a frame in there is missing bytes. Everything up
		// to the next boundary is untrustworthy — bin it rather than parse a half
		// frame, which would put values in the wrong fields from here on.
		if (rawResyncNeeded) {
			rawResyncNeeded = false;
			// Empty the buffer and zero the tally — do not just skip a frame.
			//
			// On overflow the dropped bytes had already been counted, so the tally
			// says frames are waiting that are not there. Parsing then starts on an
			// empty buffer and every field burns the read timeout, which stalls the
			// loop long enough for more data to pile up and overflow again. That
			// feedback loop is what pinned this at 5 fps with the fields shifted.
			while (wsStreamRead() >= 0) { /* discard */ }
			__atomic_store_n(&rawFramesPending, 0, __ATOMIC_RELAXED);
		}

		// Only whole frames are worth parsing: starting on a partial one leaves the
		// trailing fields empty and the remainder in the buffer, which is what put
		// every value in the wrong cell.
		if (__atomic_load_n(&rawFramesPending, __ATOMIC_RELAXED) > 0) {

			// Genuinely behind the sender? Skip to the newest frame — the stale ones
			// would only be drawn and instantly replaced.
			//
			// The threshold is not 1. The virtual COM port delivers in bursts, so a
			// few frames arriving together is normal rather than a backlog, and
			// dropping all but the last threw away two thirds of them (6fps out of a
			// 20Hz sender the panel could easily have kept up with). Only a queue
			// deeper than the panel can absorb is worth discarding; the 4096-byte
			// buffer holds around a dozen frames, so this has room to spare.
			while (__atomic_load_n(&rawFramesPending, __ATOMIC_RELAXED) > RAW_FRAME_BACKLOG && guard++ < 32) {
				do { c = wsStreamRead(); } while (c >= 0 && c != RAW_FRAME_TERMINATOR);
			}

			lastSerialActivity = millis();
			rawFrameEnded = false;
			Command_CustomProtocolData();

			if (rawFrameEnded) {
				// The parser stopped ON the boundary: this frame carried fewer fields
				// than it wanted, so the tail came back empty and the terminator is
				// already consumed. Draining again here would eat the next frame.
				perfShortFrames++;
			} else {
				// The frame had more fields than the parser reads (or was clipped by an
				// overflow): bin the remainder so the next parse starts on a boundary.
				perfFullFrames++;
				do { c = wsStreamRead(); } while (c >= 0 && c != RAW_FRAME_TERMINATOR);
			}
		}
	}
	else if (FlowSerialAvailable() > 0) {
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
	const uint32_t perfUartT0 = micros();
	handleButtonBoxUart();
	perfUartUs += micros() - perfUartT0;

	const uint32_t perfEl = micros() - perfT0;
	if (perfEl > perfLoopMaxUs) perfLoopMaxUs = perfEl;
	perfLoops++;
}

void idle(bool critical) {
	yield(); // Feed watchdog
	shCustomProtocol.idle();  // Re-enabled: idle updates

	// Service the wheel's UART from here too.
	//
	// The ARQ layer parks in this hook while it waits on the PC: ARQSerial::read()
	// spins for up to 400ms per field, and SHCustomProtocol::read() pulls one
	// field per ';', so a single telemetry frame can hold loop() for seconds
	// (measured: loop_max 2.69s, of which the whole redraw was 6ms). Without
	// this call, handleButtonBoxUart() cannot run for that entire stretch, which
	// is exactly the MFC/menu lag — same root cause as the slow dashboard, not a
	// UART problem.
	//
	// This runs on the `critical` path too. That path is Arq_TimedRead(), which
	// waits up to 100ms for a single byte, so skipping it left the exact stall
	// we are trying to cover. The rate limit below is what makes it affordable:
	// at most 500 scans/s of an empty UART, a few microseconds each.
	//
	// Guards: rate limit, and block re-entry — processButtonBoxLine() can write
	// NVS or the filesystem, and must never be started twice.
	(void)critical;
	static bool inIdleUart = false;
	static unsigned long lastIdleUartMs = 0;
	const unsigned long nowMs = millis();
	if (inIdleUart || (nowMs - lastIdleUartMs) < 2) return;
	inIdleUart = true;
	lastIdleUartMs = nowMs;
	handleButtonBoxUart();
	inIdleUart = false;
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

	// Diagnostic for the "MFC popups never appear" report: everything from
	// dispatch through showPopup()/drawAlert()/latchOverlay() reads correct on
	// the screen side (priority 3 beats both SimHub priorities unconditionally
	// in latchOverlay(), and drawAlert() runs unconditionally past the redraw
	// gate whenever redrawPending is set, which showPopup() does). Nothing
	// here can inject test traffic onto WheelSerial to prove it end to end, so
	// this prints exactly what was parsed, on the screen's own USB CDC Serial
	// -- if this line never appears while the wheel is confirmed sending (per
	// its own debug monitor), the gap is upstream of this function entirely
	// (wiring, baud, pin mapping) rather than in the popup/overlay logic.
	debugLog("[BB-UART] cat=" + cat + " func=" + func + " val=" + val);

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
		buzzerBeep(60); // Feedback de confirmação do menu MFC
		// Legacy compatibility
		if (val == "PAGE_NEXT") {
			shCustomProtocol.pgUartMfc++;
			shCustomProtocol.pageNextExternal("UART:MFC");
			msg = "PAGE +";
		} else if (val == "PAGE_PREV") {
			shCustomProtocol.pgUartMfc++;
			shCustomProtocol.pagePrevExternal("UART:MFC");
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
		shCustomProtocol.pgUartPage++;
		shCustomProtocol.pageNextExternal("UART:PAGE");
		msg = "";
	} else if (cat == "PAGE" && func == "PREV") {
		shCustomProtocol.pgUartPage++;
		shCustomProtocol.pagePrevExternal("UART:PAGE");
		msg = "";
	} else if (cat == "BITE" && func == "VAL") {
		msg = String("BITE: ") + val + "%";
	} else if (cat == "CLUTCH" && func == "MODE") {
		msg = String("CLUTCH: ") + val;
	} else if (cat == "CALIB" && func == "START") {
		shCustomProtocol.startCalibPanel();
		msg = "";
	} else if (cat == "CALIB" && func == "LIVE") {
		shCustomProtocol.handleCalibLive(val);
		msg = "";
	} else if (cat == "CALIB" && func == "DONE") {
		// New wheel firmware sends "A288 B275" (final span per channel), shown
		// in the panel opened by CALIB:START/LIVE. An un-updated wheel still
		// sends the old plain "HALL" -- handleCalibDone() returns false for
		// anything that doesn't look like the new shape, and the legacy popup
		// below covers that case exactly as it always did.
		if (!shCustomProtocol.handleCalibDone(val)) {
			msg = String("CALIB OK: ") + val;
		} else {
			msg = "";
		}
	} else if (cat == "CALIB" && func == "INVALID") {
		shCustomProtocol.closeCalibPanel();
		msg = String("CALIB ERR: ") + val;
	} else if (cat == "TRIM" && func == "LIVE") {
		shCustomProtocol.handleTrimLive(val);
		msg = "";
	} else if (cat == "SYS" && func == "BOOT") {
		msg = String("BOOT: ") + val;
	} else if (cat == "SYS" && func == "RESET") {
		msg = String("RESET: ") + val;
	} else if (cat == "WIFI" && func == "TOGGLE") {
		bool newState = !wifiPrefs.getBool("wifiEn", false);
		wifiPrefs.putBool("wifiEn", newState);
		msg = newState ? "WIFI: ON (reboot)" : "WIFI: OFF (reboot)";
	} else if (cat == "WIFI" && func == "PORTAL") {
		wifiPrefs.putBool("wifiEn", true);
		ECrowneWifi::forgetCredentials();
		msg = "WIFI: RESET CFG (reboot)";
	} else if (cat == "BLE" && func == "STATE") {
		msg = String("BLE: ") + val;
	} else if (cat == "ERS" && func == "STEP") {
		// New protocol: the wheel only reports the button press. The real
		// ERS mode/battery comes from SimHub telemetry (ersDeployMode,
		// kersLevel), computed fresh on every redraw in drawAlert() -- see
		// showErsStepPopup()'s comment for why (game reaction lag).
		shCustomProtocol.showErsStepPopup(val == "UP");
		msg = "";
	} else if (cat == "FUEL" && func == "STEP") {
		shCustomProtocol.showFuelStepPopup(val == "UP");
		msg = "";
	} else if (cat == "MAP" && func == "STEP") {
		shCustomProtocol.showMapStepPopup(val == "UP");
		msg = "";
	} else if (cat == "ABS" && func == "STEP") {
		shCustomProtocol.showAbsStepPopup(val == "UP");
		msg = "";
	} else if (cat == "TURBO" && func == "STEP") {
		shCustomProtocol.showTurboStepPopup(val == "UP");
		msg = "";
	} else if (cat == "REGEN" && func == "STEP") {
		shCustomProtocol.showRegenStepPopup(val == "UP");
		msg = "";
	} else if (cat == "SOC" && func == "STEP") {
		shCustomProtocol.showSocStepPopup(val == "UP");
		msg = "";
	} else if (cat == "ERS" && func == "MODE") {
		// Legacy protocol from an un-updated wheel firmware: it invented this
		// label from a local counter that never tracked the game's real ERS
		// state. Shown as-is for compatibility, but it is not telemetry.
		msg = String("ERS: ") + val + " (wheel)";
	} else if (cat == "FUEL" && func == "VAL") {
		msg = String("FUEL: ") + val + "% (wheel)";
	} else if (cat == "MEDIA") {
		msg = String("MEDIA: ") + func;
	} else {
		msg = cat + ":" + func + ":" + val;
	}

	if (msg.length() > 0) {
		shCustomProtocol.showPopup(msg, 3000);
	}
}
