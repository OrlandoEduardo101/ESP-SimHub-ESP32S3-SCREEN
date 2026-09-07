#include <TcpSerialBridge2.h>
#include <FullLoopbackStream.h>
#include <Arduino_GFX_Library.h>

TcpSerialBridge2 instance(BRIDGE_PORT, RAW_BRIDGE_PORT);

class ECrowneWifi {
    public:
        static void setup(FullLoopbackStream *outgoingStream, FullLoopbackStream *incomingStream, Arduino_GFX *gfx) {
            instance.setup(outgoingStream, incomingStream, gfx);
        }
        static void loop() {
            instance.loop();
        }
        static void flush() {
            instance.flush();
        }
        // Deletes the stored WiFi credentials file so the next call to
        // setup() opens the WiFiManager captive portal again. Used by the
        // opt-in wireless toggle's "reset WiFi config" gesture.
        static void forgetCredentials() {
#if USE_HARDCODED_CREDENTIALS
            // no-op: credentials are hardcoded at compile time, nothing to forget
#else
            if (FileFS.begin(true)) {
                FileFS.remove(CONFIG_FILENAME);
            }
#endif
        }
};

// NOTE: the ARQ transport macros (FlowSerialBegin/StreamRead/StreamAvailable/
// FlowSerialFlush/StreamFlush/StreamWrite/StreamPrint) that used to be
// hard-redirected here at compile time are now defined in src/main.cpp as
// small runtime-dispatching wrapper functions, so a single firmware build
// can choose USB Serial (default) or this WiFi bridge based on a persisted
// setting instead of a compile-time flag. Do not redefine them here again.