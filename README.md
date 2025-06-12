# ST73xx Reflective LCD Driver Library

[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Raspberry%20Pi%20Pico%20W-red.svg)](https://www.raspberrypi.com/products/raspberry-pi-pico/)
[![Version](https://img.shields.io/badge/Version-3.0.0-green.svg)](https://github.com/musicaJack/ST7305_2.9_Reflective_Lcd/releases)

English | [中文](README.zh.md)

A comprehensive, high-performance display driver library for ST7305 and ST7306 reflective LCD displays on Raspberry Pi Pico/Pico W. This library provides a complete graphics framework with hardware abstraction, optimized rendering, WiFi connectivity, and easy-to-use APIs.
<p align="center">
  <img src="imgs/analog_clock.jpg" alt="affect1" width="300"/>
  <img src="imgs/maze_game.jpg" alt="affect2" width="300"/>
</p>
## ✨ Features

### Display Support
- **Multi-Device Support**: Full support for both ST7305 and ST7306 reflective LCD controllers
- **Graphics Framework**: Complete UI abstraction layer with Adafruit GFX-style API
- **Advanced Graphics**: Built-in drawing functions for shapes, lines, text, and complex animations
- **Template Design**: Modern C++ template-based architecture for type safety and performance
- **Hardware Abstraction**: Clean separation between hardware drivers and graphics rendering
- **Optimized SPI**: High-performance SPI communication with configurable parameters

### WiFi & Networking (NEW)
- **WiFi Connectivity**: Built-in WiFi support for Raspberry Pi Pico W
- **NTP Time Synchronization**: Automatic time sync with NTP servers
- **Network Status Display**: Real-time WiFi and sync status indicators
- **LED Feedback**: Visual indicators for connection and sync states
- **Robust Error Handling**: Comprehensive timeout and retry mechanisms

### Advanced Features
- **Font System**: Built-in font support with customizable layouts and sizes
- **Color Management**: Support for grayscale and monochrome rendering modes
- **Rotation Support**: Display rotation and coordinate transformation
- **Power Management**: Low-power and high-power modes for energy efficiency
- **Real-Time Clock**: Precision timekeeping with automatic NTP synchronization

## 🏗️ Project Architecture

The project follows a modern modular design with clear separation of concerns:

### Core Components

- **Hardware Drivers** (`st7305_driver.cpp`, `st7306_driver.cpp`): Low-level display controller implementations
- **UI Abstraction** (`st73xx_ui.cpp/hpp`): Hardware-agnostic graphics interface (Adafruit GFX-style)
- **Graphics Engine** (`pico_display_gfx.hpp/inl`): Template-based graphics rendering engine
- **WiFi NTP Clock** (`analog_clock_wifi.cpp`): Complete WiFi-enabled analog clock application
- **Font System** (`fonts/st73xx_font.cpp`): Comprehensive font rendering with layout options
- **Examples** (`examples/`): Comprehensive demo applications showcasing features

### Directory Structure

```
├── src/                           # Source code directory
│   ├── st7305_driver.cpp         # ST7305 controller driver
│   ├── st7306_driver.cpp         # ST7306 controller driver
│   ├── st73xx_ui.cpp             # UI abstraction layer
│   └── fonts/
│       └── st73xx_font.cpp       # Font data and rendering
├── include/                       # Header files directory
│   ├── st7305_driver.hpp         # ST7305 driver interface
│   ├── st7306_driver.hpp         # ST7306 driver interface
│   ├── st73xx_ui.hpp             # UI abstraction interface
│   ├── pico_display_gfx.hpp      # Template graphics engine
│   ├── pico_display_gfx.inl      # Template implementation
│   ├── st73xx_font.hpp           # Font system interface
│   └── gfx_colors.hpp            # Color definitions
├── examples/                      # Example applications
│   ├── st7305_demo.cpp           # ST7305 comprehensive demo
│   ├── st7306_demo.cpp           # ST7306 demonstration
│   └── analog_clock_wifi.cpp     # WiFi NTP Analog Clock (NEW)
├── lwipopts/                      # Network configuration (NEW)
│   └── lwipopts.h                # lwIP configuration for WiFi
├── build/                         # Build output directory
├── CMakeLists.txt                # CMake build configuration
└── build_pico.bat                # Windows build script
```

## 🚀 Quick Start

### Hardware Requirements

#### For Basic Display Usage
- Raspberry Pi Pico or Pico W
- ST7305 or ST7306 reflective LCD display
- Breadboard and jumper wires

#### For WiFi NTP Clock (Pico W Required)
- **Raspberry Pi Pico W** (WiFi capability required)
- ST7306 reflective LCD display (recommended for best results)
- Stable 3.3V power supply
- WiFi network access

### Hardware Connections

#### ST7305/ST7306 Display Connection
```
Raspberry Pi Pico W       ST7305/ST7306 Display
+---------------+         +-------------------+
|  GPIO18 (SCK) |-------->| SCK               |
|  GPIO19 (MOSI)|-------->| MOSI/SDA          |
|  GPIO17 (CS)  |-------->| CS                |
|  GPIO20 (DC)  |-------->| DC                |
|  GPIO15 (RST) |-------->| RST               |
|  3.3V         |-------->| VCC               |
|  GND          |-------->| GND               |
+---------------+         +-------------------+
```

### Software Setup

1. **Clone the repository:**
```bash
git clone https://github.com/musicaJack/ST7305_2.9_Reflective_Lcd.git
cd ST7305_2.9_Reflective_Lcd
```

2. **Configure WiFi settings (for NTP clock):**
   Edit `examples/analog_clock_wifi.cpp` and update:
   ```cpp
   #define WIFI_SSID "YOUR_WIFI_NETWORK"
   #define WIFI_PASS "YOUR_WIFI_PASSWORD"
   #define NTP_SERVER "182.92.12.11"  // or your preferred NTP server
   ```

3. **Build the project:**
```bash
# Windows
./build_pico.bat

# Linux/Mac
mkdir build && cd build
cmake ..
make
```

4. **Flash to Pico W:**
   - Hold BOOTSEL button and connect USB cable
   - Copy `AnalogClockWiFi.uf2` to the mounted RPI-RP2 drive
   - The Pico W will restart and begin WiFi connection

## 📱 WiFi NTP Analog Clock

The flagship application of this library is a sophisticated analog clock with WiFi connectivity and NTP synchronization.

### Features

- **Elegant Analog Clock**: Classic analog clock face with hour, minute, and second hands
- **WiFi Connectivity**: Automatic connection to your WiFi network
- **NTP Time Sync**: Precision time synchronization every 12 hours
- **Real-Time Display**: Shows current date (YYYY-MM-DD format) and day of week
- **Status Indicators**: 
  - Top-left: WiFi connection status
  - Top-right: NTP synchronization status
- **LED Feedback**: Onboard LED blinks during WiFi/NTP operations
- **Robust Operation**: Comprehensive error handling and retry mechanisms
- **Beijing Timezone**: Configured for UTC+8 (customizable)

### Usage

1. **Configure Network**: Update WiFi credentials in the source code
2. **Build & Flash**: Compile and upload to Pico W
3. **Monitor Connection**: Watch serial output for connection status
4. **Enjoy**: The clock will automatically connect and sync time

### Display Layout

```
┌─────────────────────────┐
│ WiFi              NTP   │  ← Status indicators
│                         │
│      2024-01-15         │  ← Date (YYYY-MM-DD)
│         MON             │  ← Day of week
│                         │
│         ●               │
│       ╱   ╲             │
│      ●     ●            │  ← Analog clock
│       ╲   ╱             │
│         ●               │
│                         │
└─────────────────────────┘
![Layout](imgs/analog_clock.jpg)

```

## 📝 API Usage Guide

### Basic Display Setup

```cpp
#include "st7305_driver.hpp"  // or "st7306_driver.hpp"
#include "pico_display_gfx.hpp"

// Hardware pin definitions
#define PIN_DC   20
#define PIN_RST  15
#define PIN_CS   17
#define PIN_SCLK 18
#define PIN_SDIN 19

// Initialize display driver
st7305::ST7305Driver display(PIN_DC, PIN_RST, PIN_CS, PIN_SCLK, PIN_SDIN);

// Create graphics context
pico_gfx::PicoDisplayGFX<st7305::ST7305Driver> gfx(
    display, 
    st7305::ST7305Driver::LCD_WIDTH, 
    st7305::ST7305Driver::LCD_HEIGHT
);

// Initialize
display.initialize();
```

### Graphics Operations

```cpp
// Clear and basic operations
display.clearDisplay();
gfx.fillScreen(WHITE);

// Drawing primitives
gfx.drawPixel(x, y, BLACK);
gfx.drawLine(10, 10, 100, 10, BLACK);
gfx.drawRectangle(20, 20, 50, 30, BLACK);
gfx.drawFilledRectangle(30, 30, 40, 20, BLACK);
gfx.drawCircle(60, 60, 15, BLACK);
gfx.drawFilledCircle(80, 80, 20, BLACK);

// Text rendering
display.drawString(10, 10, "Hello World!", BLACK);
display.drawChar(x, y, 'A', BLACK);

// Display rotation
gfx.setRotation(1);  // 0: 0°, 1: 90°, 2: 180°, 3: 270°
display.setRotation(1);

// Update display
display.display();
```

### WiFi NTP Clock Integration

```cpp
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/dns.h"

// WiFi configuration
#define WIFI_SSID "YourNetwork"
#define WIFI_PASS "YourPassword"
#define NTP_SERVER "pool.ntp.org"

// Initialize WiFi
bool connect_wifi() {
    if (cyw43_arch_init()) {
        return false;
    }
    
    cyw43_arch_enable_sta_mode();
    
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, 
                                          CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        return false;
    }
    
    return true;
}

// NTP synchronization
bool sync_ntp_time() {
    // Implementation details in examples/analog_clock_wifi.cpp
    return ntp_sync_success;
}
```

## 🔧 Build Targets

The project supports multiple build targets:

- **ST7305_Display**: ST7305 demonstration application
- **ST7306_Display**: ST7306 demonstration application  
- **AnalogClockWiFi**: WiFi NTP Analog Clock (requires Pico W)
- **MazeGame**: Interactive maze game demonstration

Each target includes comprehensive examples showcasing the respective features.

## 🆕 What's New in Version 3.0

### Major New Features

1. **WiFi NTP Clock**: Complete analog clock application with WiFi connectivity
2. **NTP Time Synchronization**: Automatic time sync with configurable intervals
3. **Network Configuration**: Comprehensive lwIP configuration with optimal settings
4. **LED Indicators**: Visual feedback for network operations
5. **Enhanced UI Layout**: Improved display layout with date/time information
6. **Robust Error Handling**: Advanced timeout and retry mechanisms

### UI Improvements

- **Clean Layout**: Removed unnecessary decorative elements for better clarity
- **Date Format**: Standardized YYYY-MM-DD date display format
- **Centered Information**: Better positioning of date and day-of-week display
- **Status Indicators**: Clear WiFi and NTP sync status at screen top
- **Optimized Spacing**: Adjusted text positioning to avoid clock overlap

### Network Enhancements

- **Optimized lwIP**: Comprehensive network stack configuration
- **DNS Resolution**: Support for both IP addresses and domain names
- **Connection Monitoring**: Real-time network status feedback
- **Power Management**: Efficient WiFi operation with sleep modes

### Performance Optimizations

- **Faster Rendering**: Optimized display update algorithms
- **Memory Efficiency**: Reduced RAM usage with better buffer management
- **Network Efficiency**: Minimized network traffic with smart sync intervals
- **Boot Speed**: Faster startup with optimized initialization sequence

## 🐛 Troubleshooting

### WiFi Connection Issues

1. **WiFi won't connect:**
   - Verify SSID and password are correct
   - Check WiFi network is 2.4GHz (Pico W doesn't support 5GHz)
   - Ensure network allows new device connections
   - Check power supply stability (WiFi requires stable 3.3V)

2. **NTP sync fails:**
   - Verify internet connectivity
   - Try different NTP server (e.g., "pool.ntp.org", "time.google.com")
   - Check firewall settings allow UDP port 123
   - Ensure DNS resolution is working

3. **Display issues:**
   - Verify all pin connections are secure
   - Check SPI configuration matches your wiring
   - Ensure adequate power supply for both Pico W and display

### Performance Issues

1. **Slow display updates:**
   - Optimize SPI clock speed in driver configuration
   - Reduce update frequency if not needed
   - Use partial display updates when possible

2. **Network timeouts:**
   - Increase timeout values in network configuration
   - Check network signal strength
   - Verify router supports the number of connected devices

## 🤝 Contributing

We welcome contributions! Please ensure your submissions:

1. Follow the existing code style and conventions
2. Include comprehensive documentation and comments
3. Add appropriate test coverage for WiFi/network features
4. Update relevant documentation and examples

### Development Setup

1. Install Pico SDK and required tools
2. Set up WiFi network for testing
3. Configure NTP server access
4. Test on actual Pico W hardware

## 📄 License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## 📧 Contact

For questions, suggestions, or support:

- **Email**: yinyue@beingdigital.cn
- **Issues**: [GitHub Issues](https://github.com/musicaJack/ST7305_2.9_Reflective_Lcd/issues)

## 🙏 Acknowledgments

Special thanks to:
- The Raspberry Pi Foundation for the excellent Pico W platform
- The lwIP project for the comprehensive TCP/IP stack
- All contributors who have helped improve this library
- The maker community for valuable feedback and suggestions

## 📸 Gallery

### WiFi NTP Analog Clock
A sophisticated timepiece with network connectivity, precision time synchronization, and elegant analog display.

### Key Features Showcase
- Real-time analog clock with smooth hand movement
- WiFi connectivity with visual status indicators  
- NTP time synchronization every 12 hours
- Date display in YYYY-MM-DD format
- Day of week indication
- LED feedback during network operations
- Robust error handling and recovery 

## Hardware Configuration

### SPI Pin Configuration
The SPI pin configuration is centralized in `include/spi_config.hpp`. You can modify these pins according to your hardware setup:

```cpp
#define SPI_PORT spi0
#define PIN_DC   20
#define PIN_RST  15
#define PIN_CS   17
#define PIN_SCLK 18
#define PIN_SDIN 19
```

## Function Categories

### Core Functions
These are the most commonly used functions that are well-tested and used in examples:

#### Display Control
- `clearDisplay()` - Clear the display buffer
- `display()` - Update the physical display
- `displayOn()` - Turn display on/off
- `displaySleep()` - Set display sleep mode
- `displayInversion()` - Set display inversion mode

#### Basic Drawing
- `drawPixel()` / `drawPixelGray()` - Draw a single pixel
- `drawLine()` - Draw a line
- `drawCircle()` / `drawFilledCircle()` - Draw circle
- `drawString()` / `drawChar()` - Draw text

### Extended Functions
These functions are available but not extensively used in current examples. They are provided for future expansion:

#### Advanced Shapes
- `drawTriangle()` / `drawFilledTriangle()` - Draw triangle
- `drawPolygon()` / `drawFilledPolygon()` - Draw polygon
- `drawRectangle()` / `drawFilledRectangle()` - Draw rectangle

### Example Categories

#### Basic Examples
- `st7305_demo.cpp` - Basic drawing demo for ST7305
- `st7306_demo.cpp` - Basic drawing demo for ST7306

#### Network Examples
- `analog_clock_wifi.cpp` - WiFi-enabled clock with NTP synchronization

#### Game Examples
- `maze_game.cpp` - Interactive maze game with joystick control 

## Maze Game Example

### Overview
A maze game based on ST7306 display and joystick controller. Players need to pre-plan the path and watch the character move along the planned path in the maze.

### Hardware Requirements
- Raspberry Pi Pico development board
- ST7306 reflective LCD display
- Joystick controller (I2C interface)

### Hardware Connection

#### ST7306 Display Connection
```
Raspberry Pi Pico         ST7306 Display
+---------------+         +---------------+
|  GPIO18 (SCK) |-------->| SCK          |
|  GPIO19 (MOSI)|-------->| MOSI         |
|  GPIO17 (CS)  |-------->| CS           |
|  GPIO20 (DC)  |-------->| DC           |
|  GPIO15 (RST) |-------->| RST          |
|  3.3V         |-------->| VCC          |
|  GND          |-------->| GND          |
+---------------+         +---------------+
```

#### Joystick Connection
```
Raspberry Pi Pico         Joystick
+---------------+         +---------------+
|  GPIO6 (SDA)  |-------->| SDA          |
|  GPIO7 (SCL)  |-------->| SCL          |
|  3.3V         |-------->| VCC          |
|  GND          |-------->| GND          |
+---------------+         +---------------+
```

### Game Rules
1. **Maze Generation**: A random maze is automatically generated at game start
2. **Path Planning**: Use joystick directions (up, down, left, right) to pre-plan the character's movement path
3. **Game Start**: Press the joystick middle button (MID key) to execute the planned path
4. **Movement Feedback**: The joystick's blue LED flashes once for each step
5. **Win Condition**: Character successfully reaches the maze exit, green LED flashes for 3 seconds
6. **Lose Condition**: Character hits a wall or runs out of path before reaching exit, red LED flashes for 3 seconds

### Operation Guide

#### Path Planning Phase
- **Up/Down/Left/Right**: Add corresponding movement command to path
- **MID Key**: Start executing the planned path (at least one step required)

#### Game Running Phase
- Character moves automatically along the planned path
- Each move executes every 500ms
- Screen displays current step being executed

#### Game End Phase
- **Win**: Displays "YOU WIN!", joystick LED turns green
- **Lose**: Displays "YOU LOST!", joystick LED turns red
- **MID Key**: Start a new game

### Display Elements
- **Black Squares**: Walls (impassable)
- **White Areas**: Paths (passable)
- **Light Gray**: Starting position
- **Dark Gray**: End position
- **Black Dot**: Current character position (during game)

### LED Indicators
- **Green**: Joystick initialization successful (on for 1 second)
- **Blue**: Operation detected (direction input or button press)
- **Blue Flash**: Flashes once per character step
- **Green Flash**: Success (3 seconds)
- **Red Flash**: Game over (3 seconds)
- **Off**: No operation or operation complete

### Technical Features
- **Maze Generation**: Recursive backtracking algorithm
- **Path Validation**: BFS algorithm ensures maze is solvable
- **Input Debounce**: 300ms debounce time
- **Visual Feedback**: LED color changes and screen display
- **Error Handling**: Automatic invalid move and boundary checks

### Troubleshooting
1. **No Display**: Check SPI connections and power
2. **Joystick Not Responding**: Check I2C connections and address
3. **Game Freeze**: Restart device, check serial output
4. **Path Detection Issues**: Adjust joystick threshold parameters 