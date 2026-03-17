# ESP-SimHub ESP32-S3 Display Dashboard

[Portuguese version (PT-BR)](README.pt-BR.md)
[Documentation index](docs/README.md)

A custom SimHub firmware for ESP32-S3 with TFT display support, specifically designed for the **WT32-SC01 Plus** development board. This firmware displays real-time racing telemetry data from SimHub on a 3.2" 480x320 landscape display.

## 📋 Overview

This project is a fork of [ESP-SimHub](https://github.com/eCrowneEng/ESP-SimHub) firmware, customized to work with ESP32-S3 boards featuring TFT displays. It provides a complete racing dashboard solution that connects to SimHub via USB serial communication.

## 📌 Project Lineage and Attribution

To make authorship and scope explicit:

### Work inherited from the original ESP-SimHub repository (eCrowneEng)

- Core SimHub communication model and command flow used by this firmware baseline
- Foundational serial protocol concepts used in ESP-SimHub ecosystem
- General project concept and compatibility approach for SimHub integration

### Work added in this fork (Orlando Eduardo Pereira)

- ESP32-S3 and WT32-SC01 Plus focused implementation and hardware adaptation
- 8-bit parallel ST7796 display integration and board-specific pin mapping
- Dashboard rendering updates, custom protocol extensions, and UI refinements
- Wheel integration work (UART interaction, button box and telemetry interaction flow)
- NeoPixel and front LED behavior enhancements for sim racing use cases
- Wiring, soldering, troubleshooting, and setup documentation for this hardware stack
- Ongoing maintenance, debugging fixes, and platform-specific improvements

If you use this fork, please keep both credits: the original ESP-SimHub project and this fork's implementation work.

## 🎯 Firmware Targets

This repository currently contains two active firmware targets:

### 1) Display Firmware (WT32-SC01 Plus)

- Source entrypoint: `src/main.cpp`
- PlatformIO env: `wt32-sc01-plus`
- Main role: render dashboard and receive SimHub custom protocol over USB serial
- Hardware: WT32-SC01 Plus (ST7796, 8-bit parallel)

### 2) Wheel Firmware (ESP32-S3 WROOM1)

- Source entrypoint: `src/main_wheel.cpp`
- PlatformIO env: `wroom1-n8r8-wheel`
- Main role: USB HID gamepad + matrix/encoders/hall sensors + UART integration with display
- Hardware: ESP32-S3 WROOM1 N8R8 + MCP23017 + PCA9685 + wheel controls

## ✅ Key Features

### Display Firmware

- Real-time telemetry dashboard (speed, RPM, gear, lap data, tire pressure, TC/ABS)
- 8-bit parallel ST7796 display pipeline for WT32-SC01 Plus
- Custom loading screen/logo support
- Optional NeoPixel/WS2812 indicators

### Wheel Firmware

- USB HID gamepad (buttons, axes, HAT) for games/SimHub controls
- Matrix scanning via MCP23017 and front LED effects via PCA9685
- Hall sensor clutch modes and calibration flow
- UART data exchange with WT32 display firmware

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

4. **Build and upload Display Firmware**
   ```bash
   pio run -e wt32-sc01-plus -t upload
   ```

5. **Build and upload Wheel Firmware**
   ```bash
   pio run -e wroom1-n8r8-wheel -t upload
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

## 📊 Dashboard Layout (Display Firmware)

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

### Project Structure (high-level)

```
ESP-SimHub-ESP32S3-SCREEN/
├── src/
│   ├── main.cpp              # Display firmware entrypoint (WT32)
│   ├── main_wheel.cpp        # Wheel firmware entrypoint (ESP32-S3 WROOM1)
│   ├── SHCustomProtocol.h    # Display and dashboard logic
│   ├── SHCommands.h          # SimHub command handlers
│   ├── NeoPixelBusLEDs.h     # RGB LED support
│   ├── logo_image.h          # Generated logo array (from convert_logo.py)
│   └── GFXHelpers.h          # Graphics helper functions
├── docs/
│   └── README.md             # Documentation index
├── img/
│   └── logo.png              # Your custom logo (PNG with transparency)
├── scripts/
│   ├── upload-mac.sh         # macOS helper scripts
│   └── ...
├── platformio.ini            # PlatformIO configuration
├── convert_logo.py           # Logo conversion script
└── customProtocol-dashBoard.txt  # SimHub custom protocol
```

### Building

```bash
# Build display firmware only
pio run -e wt32-sc01-plus

# Build wheel firmware only
pio run -e wroom1-n8r8-wheel

# Upload display firmware
pio run -e wt32-sc01-plus -t upload

# Upload wheel firmware
pio run -e wroom1-n8r8-wheel -t upload

# Monitor serial output
pio device monitor
```

### Configuration

Display firmware options in `src/main.cpp`:

- `PIXEL_WIDTH` / `PIXEL_HEIGHT`: Display resolution (480x320 for landscape)
- `INCLUDE_WIFI`: Enable WiFi bridge (default: false)
- `INCLUDE_RGB_LEDS_NEOPIXELBUS`: Enable NeoPixel LED support

Wheel firmware options are mainly in `src/main_wheel.cpp` and `platformio.ini` (`wroom1-n8r8-wheel` env).

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

## ☕ Support This Fork

If this fork saved you time or helped your build, you can support my work:

- PicPay: **@orlandoeduardo.pereira**
- Link: https://picpay.me/orlandoeduardo.pereira
- Link (PicPay): https://link.picpay.com/p/177377918669b9b8f2b4e25

PIX QR Code:

QR image path: `docs/img/pix.png`

![PicPay PIX QR Code](docs/img/pix.png)

---

**Note**: This firmware is specifically optimized for the WT32-SC01 Plus board. For other ESP32-S3 boards with different displays, you may need to adjust the pin configurations and display driver settings in `src/SHCustomProtocol.h`.
