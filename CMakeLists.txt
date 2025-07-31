cmake_minimum_required(VERSION 3.13)

# Pull in Raspberry Pi Pico SDK (must be defined before project)
# Adjust the path if your SDK is installed elsewhere
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Force the PICO_BOARD to pico_w BEFORE pico_sdk_init()
set(PICO_BOARD pico_w CACHE STRING "Target board" FORCE)

project(ST7305_Display C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()


# Add executable for ST7305
add_executable(ST7305_Display
    examples/st7305_demo.cpp
    src/st73xx/st7305_driver.cpp
    src/fonts/st73xx_font.cpp
    src/st73xx/st73xx_ui.cpp
)

# Add executable for ST7306
add_executable(ST7306_Display
    examples/st7306_demo.cpp
    src/st73xx/st7306_driver.cpp
    src/fonts/st73xx_font.cpp
    src/st73xx/st73xx_ui.cpp
)

# Add executable for Maze Game
add_executable(MazeGame
    examples/maze_game.cpp
    src/st73xx/st7306_driver.cpp
    src/fonts/st73xx_font.cpp
    src/st73xx/st73xx_ui.cpp
    src/joystick/joystick.cpp
)

# Add executable for WiFi Analog Clock
add_executable(AnalogClockWiFi
    examples/analog_clock_wifi.cpp
    src/st73xx/st7306_driver.cpp
    src/fonts/st73xx_font.cpp
    src/st73xx/st73xx_ui.cpp
)

# Add include directories
target_include_directories(ST7305_Display PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/include/st73xx
    ${CMAKE_CURRENT_LIST_DIR}/include/fonts
)

target_include_directories(ST7306_Display PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/include/st73xx
    ${CMAKE_CURRENT_LIST_DIR}/include/fonts
)

target_include_directories(MazeGame PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/include/st73xx
    ${CMAKE_CURRENT_LIST_DIR}/include/fonts
    ${CMAKE_CURRENT_LIST_DIR}/include/joystick
)

target_include_directories(AnalogClockWiFi PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/include/st73xx
    ${CMAKE_CURRENT_LIST_DIR}/include/fonts
)

# Link libraries
target_link_libraries(ST7305_Display PUBLIC
    pico_stdlib
    hardware_spi
    hardware_gpio
    pico_stdio_usb
)

target_link_libraries(ST7306_Display PUBLIC
    pico_stdlib
    hardware_spi
    hardware_gpio
    pico_stdio_usb
)

target_link_libraries(MazeGame PUBLIC
    pico_stdlib
    hardware_spi
    hardware_gpio
    hardware_i2c                  # 添加 I2C 库用于joystick
    pico_stdio_usb
)

target_link_libraries(AnalogClockWiFi PUBLIC
    pico_stdlib
    hardware_spi
    hardware_gpio
    hardware_rtc
    hardware_clocks
    pico_cyw43_arch_lwip_poll
    pico_stdio_usb
    pico_stdio_uart
)





# Enable usb output, disable uart output
pico_enable_stdio_usb(ST7305_Display 1)
pico_enable_stdio_uart(ST7305_Display 1)

pico_enable_stdio_usb(ST7306_Display 1)
pico_enable_stdio_uart(ST7306_Display 1)

pico_enable_stdio_usb(MazeGame 1)
pico_enable_stdio_uart(MazeGame 1)

pico_enable_stdio_usb(AnalogClockWiFi 1)
pico_enable_stdio_uart(AnalogClockWiFi 1)




# Create map/bin/hex/uf2 files
pico_add_extra_outputs(ST7305_Display)
pico_add_extra_outputs(ST7306_Display)
pico_add_extra_outputs(MazeGame)
pico_add_extra_outputs(AnalogClockWiFi)

