/**
 * Original implementation and testing by moriusz: https://github.com/moriusz
 * Adapted for ESP32-S3 WT32-SC01 Plus
*/

#include <typeinfo>
#include <NeoPixelBusLg.h>
#include <string>

/****************************
 *
 * Configuration Starts here
 *
 ************************** */

// Number of LEDs in your strip
#define LED_COUNT 21
// Set to 1 to reverse LED order (right to left)
#define RIGHTTOLEFT 0
// Set to 1 to light up all LEDs in red at startup (test mode)
#define TEST_MODE 1

// LED BRIGHTNESS NANNY
//  Think about why you want to go higher than this?
//  is your power supply ready? are your eyes ready?, is your heat dissipation ready?
//  https://learn.adafruit.com/sipping-power-with-neopixels/insights
//  remember, if you don't have an external power supply, your board or USB may not be able
//  to provide enough power.
// luminance goes from 0-255, UPDATE AT YOUR OWN RISK
#define LUMINANCE_LIMIT 150


// The color order that your LED strip uses
// https://github.com/Makuna/NeoPixelBus/wiki/Neo-Features
#define colorSpec NeoGrbFeature // A three-element color in the order of Green, Red, and then Blue. This is used for SK6812(grb), WS2811, and WS2812.
//#define colorSpec NeoRgbFeature //A three-element color in the order of Red, Green, and then Blue. Some older pixels used this.
//#define colorSpec NeoBgrFeature //A three-element color in the order of Blue, Red, and then Green.


// Identify your LED model or protocol
// Ws2812x << default for this library, no changes required; WS2812a, WS2812b, WS2812c, etc The most compatible
// Sk6812
// Apa106
// 400kbps << old slower speed standard that started this all
// .. or any of these but inverted.. example: Ws2812xInverted
//
// Then replace Ws2812x in the methods below with your LED Model/protocol


// We use different methods for each type of board based on available features and their limitations
#ifdef ESP32
//****** ESP32 / ESP32-S3 ******
// ESP32-S3 does not support RMT method, so we use BitBang
// https://github.com/Makuna/NeoPixelBus/wiki/ESP32-NeoMethods

#ifdef CONFIG_IDF_TARGET_ESP32S3
//******
// BitBang for ESP32-S3
// Uses CPU but works on all pins below GPIO32
// For WT32-SC01 Plus, available GPIOs are: 10, 11 (12, 13, 14, 21 are used by display)
//******
#define method NeoEsp32BitBangWs2812xMethod
// Use GPIO 10 (EXT_IO1) or GPIO 11 (EXT_IO2) - GPIO 10 recommended
#define DATA_PIN 10
#else
//******
// RMT for ESP32 (non-S3)
// little CPU Usage and low memory but many interrupts run for it and requires hardware buffer
// Supports all pins below GPIO34
//******
#define method NeoEsp32Rmt0Ws2812xMethod
#define DATA_PIN 8
#endif

#else

//****** ESP8266 ******
// There are other methods, but We're picking the most convenient ones here.
//  Feel free to investigate the others, understand their drawbacks and use them if you want
// https://github.com/Makuna/NeoPixelBus/wiki/ESP8266-NeoMethods

// UART
// FASTER;
// Only GPIO2 ("D4" in nodemcu, d1Mini and others, but verify)
#define method NeoEsp8266Uart1Ws2812xMethod

// IF using UART, this will be ignored and only GPIO2 will be used
#define DATA_PIN 2
#endif

// Initial color to fill the strip before SimHub connects to the device
// R, G, B format from 0-255..
//  Be aware that (255, 255, 255) may consume a lot of current
//  more than your device can provide, which can damage it. Start with lower numbers
//  ex: (50, 0, 0) is red and (100, 0, 0) is still red, just brighter
//  See this: https://learn.adafruit.com/adafruit-neopixel-uberguide/powering-neopixels#estimating-power-requirements-2894486
//
// note: that this color is not limited by the luminance limit
auto initialColor = RgbColor(120, 0, 0);


/*************************
 *
 * Configuration ends here
 *
 ********************** */

// Instantiate an LED Strip
NeoPixelBusLg<colorSpec, method, NeoGammaTableMethod> neoLedStrip(LED_COUNT, DATA_PIN);


/**
 * Initialization function: prepares the strip and other related things
 */
void neoPixelBusBegin()
{
    neoLedStrip.Begin();
    neoLedStrip.Show();

    if (TEST_MODE)
    {
        for (int i = 0; i < LED_COUNT; i++)
        {
            neoLedStrip.SetPixelColor(i, initialColor);
        }
        neoLedStrip.Show();
    }
    neoLedStrip.SetLuminance(LUMINANCE_LIMIT);
}

void neoPixelBusRead()
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint16_t b1;
    uint16_t b2;
    uint8_t j;
    int mode = 1;
    mode = FlowSerialTimedRead();
    while (mode > 0)
    {
        // Read all
        if (mode == 1)
        {
            for (j = 0; j < LED_COUNT; j++)
            {
                r = FlowSerialTimedRead();
                g = FlowSerialTimedRead();
                b = FlowSerialTimedRead();

                if (RIGHTTOLEFT == 1)
                {
                    neoLedStrip.SetPixelColor(LED_COUNT - j - 1, RgbColor(r, g, b));
                }
                else
                {
                    neoLedStrip.SetPixelColor(j, RgbColor(r, g, b));
                }
            }
        }

        // partial led data
        else if (mode == 2)
        {
            int startled = FlowSerialTimedRead();
            int numleds = FlowSerialTimedRead();

            for (j = startled; j < startled + numleds; j++)
            {
                r = FlowSerialTimedRead();
                g = FlowSerialTimedRead();
                b = FlowSerialTimedRead();

                if (RIGHTTOLEFT == 1)
                {
                    neoLedStrip.SetPixelColor(LED_COUNT - j - 1, RgbColor(r, g, b));
                }
                else
                {
                    neoLedStrip.SetPixelColor(j, RgbColor(r, g, b));
                }
            }
        }

        // repeated led data
        else if (mode == 3)
        {
            int startled = FlowSerialTimedRead();
            int numleds = FlowSerialTimedRead();

            r = FlowSerialTimedRead();
            g = FlowSerialTimedRead();
            b = FlowSerialTimedRead();

            for (j = startled; j < startled + numleds; j++)
            {
                if (RIGHTTOLEFT == 1)
                {
                    neoLedStrip.SetPixelColor(LED_COUNT - j - 1, RgbColor(r, g, b));
                }
                else
                {
                    neoLedStrip.SetPixelColor(j, RgbColor(r, g, b));
                }
            }
        }

        mode = FlowSerialTimedRead();
    }
}

void neoPixelBusShow() {
    if (LED_COUNT > 0 && neoLedStrip.IsDirty()) {
        neoLedStrip.Show();
    }
}

int neoPixelBusCount() {
    return LED_COUNT;
}

/*************************
 * Custom LED Logic for WT32-SC01 Plus
 * 21 LEDs total:
 * - LEDs 0-2 (1-3): Left side - Flags/Alerts/Spotter Left
 * - LEDs 3-17 (4-18): Center - RPM meter (15 LEDs) with DRS indication
 * - LEDs 18-20 (19-21): Right side - Spotter Right/Warnings
 *************************/

// LED position definitions (0-indexed)
#define LED_LEFT_1 0
#define LED_LEFT_2 1
#define LED_LEFT_3 2
#define LED_RPM_START 3
#define LED_RPM_END 17
#define LED_RPM_COUNT 15
#define LED_RIGHT_1 18
#define LED_RIGHT_2 19
#define LED_RIGHT_3 20

// Color definitions for flags and alerts
#define COLOR_FLAG_GREEN RgbColor(0, 255, 0)
#define COLOR_FLAG_YELLOW RgbColor(255, 255, 0)
#define COLOR_FLAG_RED RgbColor(255, 0, 0)
#define COLOR_FLAG_BLUE RgbColor(0, 0, 255)
#define COLOR_FLAG_WHITE RgbColor(255, 255, 255)
#define COLOR_FLAG_CHECKERED RgbColor(200, 200, 200)
#define COLOR_FLAG_BLACK RgbColor(50, 50, 50)
#define COLOR_ALERT_CRITICAL RgbColor(255, 0, 0)
#define COLOR_ALERT_WARNING RgbColor(255, 100, 0)
#define COLOR_SPOTTER RgbColor(255, 0, 255)  // Magenta for spotter
#define COLOR_DRS_AVAILABLE RgbColor(0, 255, 0)  // Green when DRS available
#define COLOR_DRS_ACTIVE RgbColor(0, 200, 255)   // Cyan when DRS active
#define COLOR_RPM_LOW RgbColor(0, 255, 0)        // Green for low RPM
#define COLOR_RPM_MID RgbColor(255, 255, 0)      // Yellow for mid RPM
#define COLOR_RPM_HIGH RgbColor(255, 50, 0)      // Orange for high RPM
#define COLOR_RPM_REDLINE RgbColor(255, 0, 0)    // Red for redline
#define COLOR_OFF RgbColor(0, 0, 0)

/**
 * Update LEDs based on telemetry data
 * Called from SHCustomProtocol with telemetry values
 */
void updateCustomLEDs(
    int rpmPercent,              // RPM percentage (0-100)
    int rpmRedLine,              // RPM redline setting (0-100)
    String currentFlag,          // Current flag state
    String spotterLeft,          // Spotter left (0/1)
    String spotterRight,         // Spotter right (0/1)
    String drsAvailable,         // DRS available (0/1)
    String drsActive,            // DRS active (0/1)
    String alertMessage,         // Critical alert message
    bool shiftLightTrigger       // Shift light trigger
) {
    // Clear all LEDs first
    for (int i = 0; i < LED_COUNT; i++) {
        neoLedStrip.SetPixelColor(i, COLOR_OFF);
    }

    // === LEFT SIDE LEDs (0-2): FLAGS & ALERTS ===
    RgbColor flagColor = COLOR_OFF;
    bool flagBlink = false;
    
    // Determine flag color
    if (currentFlag == "Green" || currentFlag == "GREEN") {
        flagColor = COLOR_FLAG_GREEN;
    } else if (currentFlag == "Yellow" || currentFlag == "YELLOW") {
        flagColor = COLOR_FLAG_YELLOW;
        flagBlink = true;
    } else if (currentFlag == "Red" || currentFlag == "RED") {
        flagColor = COLOR_FLAG_RED;
    } else if (currentFlag == "Blue" || currentFlag == "BLUE") {
        flagColor = COLOR_FLAG_BLUE;
    } else if (currentFlag == "White" || currentFlag == "WHITE") {
        flagColor = COLOR_FLAG_WHITE;
    } else if (currentFlag == "Checkered" || currentFlag == "CHECKERED") {
        flagColor = COLOR_FLAG_CHECKERED;
        flagBlink = true;
    } else if (currentFlag == "Black" || currentFlag == "BLACK") {
        flagColor = COLOR_FLAG_BLACK;
    }
    
    // Apply flag color to left LEDs
    if (flagColor.R > 0 || flagColor.G > 0 || flagColor.B > 0) {
        // For blinking flags (yellow, checkered), use millis() to create blink effect
        bool show = true;
        if (flagBlink) {
            show = (millis() / 500) % 2 == 0;  // Blink every 500ms
        }
        
        if (show) {
            neoLedStrip.SetPixelColor(LED_LEFT_1, flagColor);
            neoLedStrip.SetPixelColor(LED_LEFT_2, flagColor);
            neoLedStrip.SetPixelColor(LED_LEFT_3, flagColor);
        }
    }
    
    // Override with spotter left if car detected
    if (spotterLeft == "1") {
        neoLedStrip.SetPixelColor(LED_LEFT_1, COLOR_SPOTTER);
        neoLedStrip.SetPixelColor(LED_LEFT_2, COLOR_SPOTTER);
        neoLedStrip.SetPixelColor(LED_LEFT_3, COLOR_SPOTTER);
    }
    
    // Override with critical alerts (highest priority)
    if (alertMessage != "" && alertMessage != "NORMAL" && alertMessage != "None") {
        bool alertBlink = (millis() / 250) % 2 == 0;  // Fast blink for alerts
        if (alertBlink) {
            neoLedStrip.SetPixelColor(LED_LEFT_1, COLOR_ALERT_CRITICAL);
            neoLedStrip.SetPixelColor(LED_LEFT_2, COLOR_ALERT_CRITICAL);
            neoLedStrip.SetPixelColor(LED_LEFT_3, COLOR_ALERT_CRITICAL);
        }
    }

    // === CENTER LEDs (3-17): RPM METER with DRS ===
    int numLedsToLight = 0;
    
    // Calculate how many LEDs to light based on RPM
    if (rpmPercent > 0) {
        numLedsToLight = (rpmPercent * LED_RPM_COUNT) / 100;
        if (numLedsToLight > LED_RPM_COUNT) numLedsToLight = LED_RPM_COUNT;
    }
    
    // Determine if DRS is available or active
    bool hasDRS = (drsAvailable == "1" || drsActive == "1");
    RgbColor drsColor = (drsActive == "1") ? COLOR_DRS_ACTIVE : COLOR_DRS_AVAILABLE;
    
    // Light up RPM LEDs with progressive colors
    for (int i = 0; i < LED_RPM_COUNT; i++) {
        if (i < numLedsToLight) {
            int ledIndex = LED_RPM_START + i;
            RgbColor ledColor;
            
            // Calculate RPM segment percentage
            float segmentPercent = ((float)(i + 1) / LED_RPM_COUNT) * 100.0;
            
            // Choose color based on RPM segment
            if (hasDRS) {
                // If DRS available/active, show DRS color
                ledColor = drsColor;
            } else if (segmentPercent < 60) {
                // Low RPM - Green
                ledColor = COLOR_RPM_LOW;
            } else if (segmentPercent < 80) {
                // Mid RPM - Yellow
                ledColor = COLOR_RPM_MID;
            } else if (segmentPercent < rpmRedLine) {
                // High RPM - Orange
                ledColor = COLOR_RPM_HIGH;
            } else {
                // Redline - Red (flashing if shift light triggered)
                if (shiftLightTrigger && (millis() / 100) % 2 == 0) {
                    ledColor = COLOR_OFF;  // Flash off
                } else {
                    ledColor = COLOR_RPM_REDLINE;
                }
            }
            
            neoLedStrip.SetPixelColor(ledIndex, ledColor);
        }
    }

    // === RIGHT SIDE LEDs (18-20): FLAGS, ALERTS & SPOTTER RIGHT ===
    // Apply flag color to right LEDs (same as left side)
    if (flagColor.R > 0 || flagColor.G > 0 || flagColor.B > 0) {
        bool show = true;
        if (flagBlink) {
            show = (millis() / 500) % 2 == 0;  // Blink every 500ms
        }
        
        if (show) {
            neoLedStrip.SetPixelColor(LED_RIGHT_1, flagColor);
            neoLedStrip.SetPixelColor(LED_RIGHT_2, flagColor);
            neoLedStrip.SetPixelColor(LED_RIGHT_3, flagColor);
        }
    }
    
    // Show shift light on right LEDs if triggered (lower priority than flags)
    if (shiftLightTrigger) {
        bool shiftBlink = (millis() / 100) % 2 == 0;
        if (shiftBlink) {
            neoLedStrip.SetPixelColor(LED_RIGHT_1, COLOR_RPM_REDLINE);
            neoLedStrip.SetPixelColor(LED_RIGHT_2, COLOR_RPM_REDLINE);
            neoLedStrip.SetPixelColor(LED_RIGHT_3, COLOR_RPM_REDLINE);
        }
    }
    
    // Override with spotter right if car detected (higher priority)
    if (spotterRight == "1") {
        neoLedStrip.SetPixelColor(LED_RIGHT_1, COLOR_SPOTTER);
        neoLedStrip.SetPixelColor(LED_RIGHT_2, COLOR_SPOTTER);
        neoLedStrip.SetPixelColor(LED_RIGHT_3, COLOR_SPOTTER);
    }
    
    // Override with critical alerts (highest priority - same as left side)
    if (alertMessage != "" && alertMessage != "NORMAL" && alertMessage != "None") {
        bool alertBlink = (millis() / 250) % 2 == 0;  // Fast blink for alerts
        if (alertBlink) {
            neoLedStrip.SetPixelColor(LED_RIGHT_1, COLOR_ALERT_CRITICAL);
            neoLedStrip.SetPixelColor(LED_RIGHT_2, COLOR_ALERT_CRITICAL);
            neoLedStrip.SetPixelColor(LED_RIGHT_3, COLOR_ALERT_CRITICAL);
        }
    }

    // Update the strip
    neoPixelBusShow();
}

