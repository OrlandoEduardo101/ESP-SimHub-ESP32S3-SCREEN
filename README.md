# ESP-SimHub ESP32-S3 Display Dashboard

A custom SimHub firmware for ESP32-S3 with TFT display support, specifically designed for the **WT32-SC01 Plus** development board. This firmware displays real-time racing telemetry data from SimHub on a 3.2" 480x320 landscape display.

## 📋 Overview

This project is a fork of [ESP-SimHub](https://github.com/eCrowneEng/ESP-SimHub) firmware, customized to work with ESP32-S3 boards featuring TFT displays. It provides a complete racing dashboard solution that connects to SimHub via USB serial communication.

### Features

- ✅ **Real-time telemetry display** - Speed, RPM, gear, lap times, tire pressure, TC/ABS levels
- ✅ **Custom loading screen** - Display your logo during initialization
- ✅ **8-bit parallel display interface** - Optimized for WT32-SC01 Plus ST7796 display
- ✅ **NeoPixel/WS2812B LED support** - Optional RGB LED strips for RPM and flag indicators
- ✅ **USB-Serial/JTAG communication** - Works out of the box on Windows without special drivers
- ✅ **Landscape orientation** - 480x320 display optimized for racing dashboards

## 🎯 Supported Hardware

### Primary Target Device

**WT32-SC01 Plus** - ESP32-S3 development board with integrated 3.2" TFT display
- **Display**: ST7796 controller, 320x480 portrait / 480x320 landscape
- **Interface**: 8-bit MCU (8080) parallel interface
- **MCU**: ESP32-S3 with 2MB PSRAM
- **Communication**: USB-Serial/JTAG (native, no drivers needed on Windows)

### Display Specifications

- **Resolution**: 480x320 pixels (landscape mode)
- **Controller**: ST7796
- **Interface**: 8-bit parallel (8080 MCU mode)
- **Backlight**: PWM controlled (GPIO 45)

## 🚀 Quick Start

### Prerequisites

1. **PlatformIO** - Install via VS Code extension, pip, or Homebrew
2. **Python 3** with Pillow library (for logo conversion)
3. **SimHub** - Racing simulator dashboard software
4. **USB cable** - For programming and communication

### Installation

1. **Clone this repository**
   ```bash
   git clone https://github.com/your-username/ESP-SimHub-ESP32S3-SCREEN.git
   cd ESP-SimHub-ESP32S3-SCREEN
   ```

2. **Install PlatformIO** (if not already installed)
   - VS Code: Install "PlatformIO IDE" extension
   - Or via pip: `pip install platformio`
   - Or via Homebrew: `brew install platformio`

3. **Convert your logo** (optional)
   ```bash
   python3 convert_logo.py
   ```
   Place your logo PNG file in `img/logo.png` before running this command.

4. **Build and upload**
   ```bash
   ./upload-mac.sh    # On macOS
   # Or use PlatformIO directly:
   pio run -e wt32-sc01-plus -t upload
   ```

### SimHub Configuration

1. **Open SimHub** → **Arduino** → **Single Arduino**
2. **Select your COM port** (e.g., COM11 on Windows, `/dev/cu.usbmodem*` on macOS)
3. **Click "Scan"** to detect the device
4. **Go to "Custom Protocol"** section
5. **Copy and paste** the content from `customProtocol-dashBoard.txt`

The custom protocol sends 22 data fields including:
- Speed, Gear, RPM percentage
- Current/Last/Best lap times
- Live delta times
- Tire pressures (FL, FR, RL, RR)
- TC/ABS levels and active status
- Brake bias and brake level
- Lap invalidation status

## 📊 Dashboard Layout

The dashboard displays information in a 5x5 grid layout:

### Top Row
- **RPM Meter**: Color-coded bar (Green → Orange → Red at redline)

### Second Row
- **Gear Display**: Large centered gear indicator
- **Best Lap Time**: Left aligned
- **Last Lap Time**: Left aligned
- **Delta**: Right aligned (Green for negative, Red for positive)
- **Delta Progress**: Right aligned

### Third Row
- **Current Lap Time**: Left aligned (Red if invalidated)
- **Speed**: Center aligned
- **Tire Pressures**: FL, FR, RL, RR in corners

### Bottom Row
- **TC Level**: Center aligned (Yellow)
- **ABS Level**: Center aligned (Blue)
- **Brake Bias**: Center aligned (Magenta)
- **Tire Pressures**: Rear tires

## 🛠️ Development

### Project Structure

```
ESP-SimHub-ESP32S3-SCREEN/
├── src/
│   ├── main.cpp              # Main firmware code
│   ├── SHCustomProtocol.h    # Display and dashboard logic
│   ├── SHCommands.h          # SimHub command handlers
│   ├── NeoPixelBusLEDs.h     # RGB LED support
│   ├── logo_image.h          # Generated logo array (from convert_logo.py)
│   └── GFXHelpers.h          # Graphics helper functions
├── img/
│   └── logo.png              # Your custom logo (PNG with transparency)
├── platformio.ini            # PlatformIO configuration
├── convert_logo.py           # Logo conversion script
├── upload-mac.sh             # Build and upload script for macOS
└── customProtocol-dashBoard.txt  # SimHub custom protocol
```

### Building

```bash
# Build only
pio run -e wt32-sc01-plus

# Build and upload
pio run -e wt32-sc01-plus -t upload

# Monitor serial output
pio device monitor
```

### Configuration

Key configuration options in `src/main.cpp`:

- `PIXEL_WIDTH` / `PIXEL_HEIGHT`: Display resolution (480x320 for landscape)
- `INCLUDE_WIFI`: Enable WiFi bridge (default: false)
- `INCLUDE_RGB_LEDS_NEOPIXELBUS`: Enable NeoPixel LED support

### Logo Customization

1. Place your PNG logo in `img/logo.png` (with transparency support)
2. Run the conversion script:
   ```bash
   python3 convert_logo.py
   ```
3. The script will:
   - Resize logo maintaining aspect ratio (max 75% of screen height)
   - Convert to RGB565 format
   - Preserve transparency (converts to black for transparent areas)
   - Generate `src/logo_image.h` with the image array

## 🔧 Troubleshooting

### SimHub Connection Issues

#### **"Unrecognized, Invalid version" Error - FIXED**

**Problem**: SimHub detected the device but rejected the connection with error "Invalid version (2d)".

**Root Cause**: The `Command_Hello()` function in `src/SHCommands.h` was sending an extra byte `0x01` before the VERSION character, causing protocol corruption:
```cpp
// ❌ WRONG - Was sending: 0x01 + 'j'
FlowSerialWrite(0x01);      // Extra byte!
FlowSerialPrint(VERSION);   // VERSION character
```

**Solution**: Removed the extra `0x01` byte. SimHub protocol expects only the VERSION character:
```cpp
// ✅ CORRECT - Now sends only: 'j'
FlowSerialPrint(VERSION);   // Only VERSION character
```

**Status**: ✅ Resolved - Device now connects and communicates correctly with SimHub

---

### Display Not Turning On

- Verify backlight pin (GPIO 45) is configured correctly
- Check display initialization logs in serial monitor
- Ensure 8-bit parallel interface is properly configured

### Upload Issues

- **macOS**: Use `./upload-mac.sh` script
- **Windows**: Use PlatformIO directly or Arduino IDE
- **External Programmer**: Use `wt32-sc01-plus-debug` environment with ZXACC-ESPDB V2

### Common Errors

- **"no free i80 bus slot"**: Display objects are created in `setup()` to avoid this
- **"USB Desconhecido"**: Using USB-Serial/JTAG instead of USB CDC resolves this
- **Display stretched**: Logo conversion script maintains aspect ratio automatically
- **ESP32 entering download mode on SimHub connection**: Fixed by adding GPIO0 INPUT_PULLUP to prevent DTR/RTS signals from triggering bootloader

## 📚 Additional Resources

- **Original ESP-SimHub**: https://github.com/eCrowneEng/ESP-SimHub
- **SimHub Discord**: https://discord.gg/pnAXf2p3RS
- **WT32-SC01 Plus Docs**: https://github.com/Cesarbautista10/WT32-SC01-Plus-ESP32
- **Arduino GFX Library**: https://github.com/moononournation/Arduino_GFX_Library

## 🤝 Contributing

This is a fork focused on ESP32-S3 display support. Contributions are welcome!

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## 📝 License

Based on ESP-SimHub firmware. Please refer to the original repository for license information.

## 🙏 Credits

- **ESP-SimHub**: Original firmware by eCrowneEng
- **SimHub**: Racing dashboard software
- **Arduino GFX Library**: Display graphics library
- **WT32-SC01 Plus Community**: Hardware documentation and examples

## 📧 Support

For issues and questions:
- Open an issue on GitHub
- Join the SimHub Discord server
- Check the troubleshooting section above

---

**Note**: This firmware is specifically optimized for the WT32-SC01 Plus board. For other ESP32-S3 boards with different displays, you may need to adjust the pin configurations and display driver settings in `src/SHCustomProtocol.h`.
