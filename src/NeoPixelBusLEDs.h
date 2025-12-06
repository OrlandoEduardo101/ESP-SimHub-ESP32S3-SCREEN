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
#define LED_COUNT 24
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

