#ifndef __ARQSERIAL_H__
#define __ARQSERIAL_H__

// Set to 1 to emit a compact per-packet timing line on Serial, separating
// time spent waiting for the sender (idle_ms) from our own parse+ack cost
// (proc_ms). Diagnostic only — leave at 0 for normal builds.
#ifndef ARQ_TIMING_TRACE
#define ARQ_TIMING_TRACE 0
#endif
//#define TESTFAIL

#ifndef StreamRead
#define StreamRead Serial.read
#define StreamFlush Serial.flush
#define StreamWrite Serial.write
#define StreamPrint Serial.print
#define StreamAvailable Serial.available
#endif

// Debug stream declared in main.cpp (can be USB CDC or UART)
extern Stream* DebugPort;

// Raw transport mode, set by the TCP bridge while a client is attached to the
// raw port (see TcpSerialBridge2). ARQ exists to make an unreliable serial link
// reliable: 32-byte packets, CRC, per-packet acks. Over TCP that work is already
// done by TCP itself, and all the layer still contributes is a stop-and-wait
// round trip per packet — measured at ~92ms each over WiFi, ~17 of them per
// telemetry frame, which is the multi-second refresh. In raw mode the byte
// stream is consumed directly; every layer above this class is untouched,
// because the sender emits exactly the same bytes SimHub does (0x03 'P' ...),
// just without the ARQ framing around them.
// Defined here rather than in the firmware's .cpp: main_wheel.cpp includes this
// header too, and it has no TCP bridge to define them for it. This header is
// pulled in exactly once per firmware, so a definition here suits both builds.
volatile bool arqRawMode = false;

// Frame terminator, and a flag saying the raw reader just swallowed one.
//
// The parser reads a fixed number of fields, but the sender's field count is not
// under our control — a SimHub template can be edited at any time. If the two
// disagree, a blind read runs straight past the end of one frame and into the
// head of the next, and from then on every value lands in the wrong slot with no
// way back. Stopping at the terminator bounds the damage to the frame that is
// actually malformed: a short frame just leaves its trailing fields empty.
#define RAW_FRAME_TERMINATOR '~'
volatile bool rawFrameEnded = false;

// Raw reads still need a bound so a half-delivered frame cannot wedge the loop.
// Kept small on purpose: parsing only starts once a whole frame is buffered, so a
// field read that has to wait at all means something is already wrong. At 50ms a
// single bad parse cost 72 x 50ms and stalled the loop for seconds; 5ms bounds
// the same mistake to a third of a second.
#define ARQ_RAW_READ_TIMEOUT_MS 5

#include <Arduino.h>
#include <RingBuf.h>

const uint8_t crc_table_crc8[256] PROGMEM = { 0,213,127,170,254,43,129,84,41,252,86,131,215,2,168,125,82,135,45,248,172,121,211,6,123,174,4,209,133,80,250,47,164,113,219,14,90,143,37,240,141,88,242,39,115,166,12,217,246,35,137,92,8,221,119,162,223,10,160,117,33,244,94,139,157,72,226,55,99,182,28,201,180,97,203,30,74,159,53,224,207,26,176,101,49,228,78,155,230,51,153,76,24,205,103,178,57,236,70,147,199,18,184,109,16,197,111,186,238,59,145,68,107,190,20,193,149,64,234,63,66,151,61,232,188,105,195,22,239,58,144,69,17,196,110,187,198,19,185,108,56,237,71,146,189,104,194,23,67,150,60,233,148,65,235,62,106,191,21,192,75,158,52,225,181,96,202,31,98,183,29,200,156,73,227,54,25,204,102,179,231,50,152,77,48,229,79,154,206,27,177,100,114,167,13,216,140,89,243,38,91,142,36,241,165,112,218,15,32,245,95,138,222,11,161,116,9,220,118,163,247,34,136,93,214,3,169,124,40,253,87,130,255,42,128,85,1,212,126,171,132,81,251,46,122,175,5,208,173,120,210,7,83,134,44,249 };
#define updateCrc(currentCrc, value) pgm_read_byte(&crc_table_crc8[currentCrc ^ value]);

typedef void(*IdleFunction) (bool);

class ARQSerial
{
private:

#if ARQ_TIMING_TRACE
	unsigned long arqTraceLastAckMicros = 0;
#endif

	// Buffer must hold the maximum ARQ payload (length <= 32). Use 64 for safety to avoid overflow.
	byte partialdatabuffer[64];
	int Arq_LastValidPacket = 255;
	RingBuf<uint8_t, 32> DataBuffer;
	IdleFunction idleFunction = 0;

#ifdef TESTFAIL
	int testfailidx = 0;
	int testfailidx2 = 0;
#endif

	int Arq_TimedRead()
	{
		int c;
		unsigned long fsr_startMillis = millis();
		do {
			if (idleFunction != 0) idleFunction(true);
			c = StreamRead();
			if (c >= 0) {
#ifdef TESTFAIL
				testfailidx = (testfailidx + 1) % 5000;
				if (testfailidx == 500)
					return random(255);

				if (testfailidx == 1000)
					return -1;
#endif
				return c;
			}
		} while (millis() - fsr_startMillis < 100);
		return -1;
	}

	void ProcessIncomingData() {
		int packetID, length, header, res, i, crc, nextpacketid;
		byte currentCrc;

#if ARQ_TIMING_TRACE
		// Lightweight timing probe (one short line per packet, not per byte).
		// proc_ms = our own parse+ack cost. idle_ms = wall time from the last
		// ack until we *noticed* the next chunk.
		//
		// Read idle_ms with care: it is not "time the PC took". It also counts
		// any stretch where the bytes were already sitting in the stream and
		// nothing called ProcessIncomingData() — so a slow caller looks exactly
		// like a slow sender here. Before blaming the PC, confirm this path is
		// really being polled at the rate you assume.
		if (StreamAvailable() > 0 && arqTraceLastAckMicros != 0) {
			unsigned long idleUs = micros() - arqTraceLastAckMicros;
			Serial.print("[T] idle_ms=");
			Serial.println(idleUs / 1000.0, 1);
		}
		unsigned long procStartUs = micros();
#endif

		while (StreamAvailable() > 0) {
			if (DebugPort) {
				DebugPort->print("[DBG] StreamAvailable=");
				DebugPort->println(StreamAvailable());
				DebugPort->flush();
			}
			header = Arq_TimedRead();
			if (DebugPort) {
				DebugPort->print("[DBG] header=");
				DebugPort->println(header, HEX);
			}
			currentCrc = 0;

			if (header == 0x01) {
				byte failureReason = 0x00;
				if (DebugPort) DebugPort->println("[DBG] header 0x01 detected");

				header = Arq_TimedRead();
				if (DebugPort) {
					DebugPort->print("[DBG] second header=");
					DebugPort->println(header, HEX);
				}
				if (header != 0x01) {
					if (DebugPort) DebugPort->println("[DBG] second header not 0x01, abort");
					return;
				}

				// read id of packet
				packetID = Arq_TimedRead(); // ?
				if (DebugPort) {
					DebugPort->print("[DBG] packetID=");
					DebugPort->println(packetID);
				}
				if (packetID < 0) {
					failureReason = 0x01; // bad id
					SendNAcq(Arq_LastValidPacket, failureReason);
					continue;
				}

				// read length of data
				length = Arq_TimedRead(); // 1
				if (DebugPort) {
					DebugPort->print("[DBG] length=");
					DebugPort->println(length);
				}
				// validate length against both protocol max (32) and buffer size
				if (length <= 0 || length > 32 || length > (int)sizeof(partialdatabuffer)) {
					failureReason = 0x02; // bad length
					SendNAcq(Arq_LastValidPacket, failureReason);
					continue;
				}

				// read data
				for (i = 0; i < length && !failureReason; i++) {
					res = Arq_TimedRead(); // 3 49 16
					partialdatabuffer[i] = res;
					if (DebugPort) {
						DebugPort->print("[DBG] data["); DebugPort->print(i); DebugPort->print("]="); DebugPort->println(res, HEX);
					}
					if (res < 0) {
						failureReason = 0x05; // bad data
						SendNAcq(Arq_LastValidPacket, failureReason);
						continue;
					}
				}

				// read checksum
				crc = Arq_TimedRead(); // 106
				if (DebugPort) { DebugPort->print("[DBG] crc recv="); DebugPort->println(crc, HEX); }
				if (crc < 0) {
					failureReason = 0x03; // bad data b/c no checksum
					SendNAcq(Arq_LastValidPacket, failureReason);
					continue;
				}

				// generate checksum for received data and check it with received checksum
				currentCrc = updateCrc(currentCrc, packetID);
				currentCrc = updateCrc(currentCrc, length);
				for (i = 0; i < length; i++) {
					currentCrc = updateCrc(currentCrc, partialdatabuffer[i]);
				}

				if (DebugPort) { DebugPort->print("[DBG] crc calc="); DebugPort->println(currentCrc, HEX); }
				if (crc != currentCrc) {
					failureReason = 0x04; // bad data b/c checksum doesnt match
					SendNAcq(Arq_LastValidPacket, failureReason);
					continue;
				}

				// push valid data and set state for next packet
				nextpacketid = Arq_LastValidPacket > 127 ? 0 : Arq_LastValidPacket + 1;
				if (packetID == nextpacketid || packetID == 255) {
					for (i = 0; i < length; i++) {
						// save valid data to buffer
						DataBuffer.push(partialdatabuffer[i]);
					}
					// save valid packet id
					Arq_LastValidPacket = packetID;
				}
#ifdef TESTFAIL
				testfailidx = (testfailidx + 1) % 5000;
				if (testfailidx != 788) {
					SendAcq(packetID);
				}
#else
				if (DebugPort) { DebugPort->print("[DBG] SendAcq id="); DebugPort->println(packetID); }
				SendAcq(packetID);
#if ARQ_TIMING_TRACE
				{
					unsigned long nowUs = micros();
					Serial.print("[T] id=");
					Serial.print(packetID);
					Serial.print(" len=");
					Serial.print(length);
					Serial.print(" proc_ms=");
					Serial.println((nowUs - procStartUs) / 1000.0, 1);
					// Timestamp *after* the prints, not before: on USB CDC a
					// write blocks for up to tx_timeout_ms when no host is
					// draining the port, and billing that stall to the next
					// idle window makes the sender look slow when the probe
					// itself was the delay.
					arqTraceLastAckMicros = micros();
				}
#endif
#endif
			}
		}
	}

	void SendAcq(uint8_t packetId)
	{
		StreamWrite(0x03);
		StreamWrite(packetId);
		StreamFlush();
	}

	void SendNAcq(uint8_t lastKnownValidPacket, byte reason)
	{
		StreamWrite(0x04);
		StreamWrite(lastKnownValidPacket);
		StreamWrite(reason);
		StreamFlush();
	}

public:

	void setIdleFunction(IdleFunction function) {
		idleFunction = function;
	}

	void CustomPacketStart(byte packetType, uint8_t length) {
		StreamWrite(0x09);
		StreamWrite(packetType);
		StreamWrite(length);
	}

	void CustomPacketSendByte(byte data) {
		StreamWrite(data);
	}

	void CustomPacketEnd() {
		//Serial.write(0x00);
	}

	int read() {
		unsigned long fsr_startMillis = millis();

		if (arqRawMode) {
			// Stay closed for the rest of this frame. Reporting end-of-frame once is
			// not enough: the caller reads a fixed number of fields, so the reads
			// after the boundary would have consumed the head of the NEXT frame and
			// shifted every value in it by that many places. The flag is cleared by
			// loop() just before the next frame is parsed.
			if (rawFrameEnded) return -1;

			do {
				if (idleFunction != 0) idleFunction(false);
				int c = StreamRead();
				if (c == RAW_FRAME_TERMINATOR) {
					rawFrameEnded = true;
					return -1;
				}
				if (c >= 0) return c;
			} while (millis() - fsr_startMillis < ARQ_RAW_READ_TIMEOUT_MS);
			return -1;
		}

		do {
			if (idleFunction != 0) idleFunction(false);

			if (DataBuffer.size() > 0) {
				if (DebugPort) { DebugPort->print("[DBG] DataBuffer size="); DebugPort->println(DataBuffer.size()); }
			}

			if (DataBuffer.size() > 0) {
				uint8_t res = 0;
				DataBuffer.pop(res);
				return (int)res;
			}

			ProcessIncomingData();
		} while (millis() - fsr_startMillis < 400 || DataBuffer.size() > 0);

		//DebugPrintLn("Read timeout !");
		return -1;
	}

	int Available() {
		if (idleFunction != 0) idleFunction(false);
		if (arqRawMode) return rawFrameEnded ? 0 : StreamAvailable();
		if (DataBuffer.size() == 0) {
			ProcessIncomingData();
		}
		return DataBuffer.size();
	}

	void Write(byte data) {
		StreamWrite(0x08);
		StreamWrite(data);
		StreamFlush();
	}

	void Print(char data)
	{
		Write((byte)data);
	}

	void Print(const char str[]) {
		int len = strlen(str);
		StreamWrite(0x06);
		StreamWrite(len);
		StreamWrite(str);
		StreamWrite(0x20);
		StreamFlush();
	}

	void WriteString(String& data)
	{
		int len = data.length();
		StreamWrite(0x06);
		StreamWrite(len);
		StreamPrint(data);
		StreamWrite(0x20);
		StreamFlush();
	}

	void PrintString(const char str[]) {
		int len = strlen(str);
		StreamWrite(0x06);
		StreamWrite(len);
		StreamWrite(str);
		StreamWrite(0x20);
		StreamFlush();
	}

	void PrintLn(const char str[]) {
		int len = strlen(str);
		StreamWrite(0x06);
		StreamWrite(len + 1);
		StreamWrite(str);
		StreamWrite('\n');
		StreamWrite(0x20);
		StreamFlush();
	}

	void PrintLn(String& data)
	{
		StreamWrite(0x06);
		StreamWrite(data.length() + 1);
		StreamPrint(data);
		StreamPrint('\n');
		StreamWrite(0x20);
		StreamFlush();
	}

	void PrintLn() {
		Write('\n');
	}

	String ReadStringUntil(char terminator1, char terminator2) {
		String ret;
		int c = read();
		while (c >= 0 && c != terminator1 && c != terminator2)
		{
			ret += (char)c;
			c = read();
		}
		return ret;
	}

	void ReadStringUntil(char buffer[], char terminator) {
		int pos = 0;

		int c = read();
		while (c >= 0 && c != terminator)
		{
			buffer[pos] = (char)c;
			c = read();
			pos++;
		}
		buffer[pos] = 0;
		
	}

	String ReadStringUntil(char terminator1) {
		String ret;
		int c = read();
		while (c >= 0 && c != terminator1)
		{
			ret += (char)c;
			c = read();
		}
		return ret;
	}

	void DebugPrintLn(String& data)
	{
		StreamWrite(0x07);
		StreamWrite(data.length() + 1);
		StreamPrint(data);
		StreamPrint('\n');
		StreamWrite(0x20);
		StreamFlush();
	}

	void DebugPrint(char data)
	{
		StreamWrite(0x07);
		StreamWrite(1);
		StreamPrint(data);
		StreamWrite(0x20);
		StreamFlush();
	}

	void DebugPrintLn(const char str[]) {
		StreamWrite(0x07);
		StreamWrite((byte)(strlen(str) + 1));
		StreamPrint(str);
		StreamPrint('\n');
		StreamWrite(0x20);
		StreamFlush();
	}
};

#endif