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

# Add pico_fatfs library
add_subdirectory(lib/pico_fatfs)


# Add executable for ST7305
add_executable(ST7305_Display
    examples/st7305_demo.cpp
    src/st7305_driver.cpp
    src/fonts/st73xx_font.cpp
    src/st73xx_ui.cpp
)

# Add executable for ST7306
add_executable(ST7306_Display
    examples/st7306_demo.cpp
    src/st7306_driver.cpp
    src/fonts/st73xx_font.cpp
    src/st73xx_ui.cpp
)

# Add executable for Maze Game
add_executable(MazeGame
    examples/maze_game.cpp
    src/st7306_driver.cpp
    src/fonts/st73xx_font.cpp
    src/st73xx_ui.cpp
    src/joystick/joystick.cpp
)

# Add executable for WiFi Analog Clock
add_executable(AnalogClockWiFi
    examples/analog_clock_wifi.cpp
    src/st7306_driver.cpp
    src/fonts/st73xx_font.cpp
    src/st73xx_ui.cpp
)

# Add executable for Text Reader
add_executable(TextReader
    examples/text_reader.cpp
    src/st7306_driver.cpp
    src/fonts/st73xx_font.cpp
    src/st73xx_ui.cpp
    src/joystick/joystick.cpp
    src/flash_font_cache.cpp
    src/hybrid_font_system.cpp
)



# Add include directories
target_include_directories(ST7305_Display PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include # 包含 st7305_driver.hpp 和 custom_font.hpp
)

target_include_directories(ST7306_Display PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_include_directories(MazeGame PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/include/joystick
)

target_include_directories(AnalogClockWiFi PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_include_directories(TextReader PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/include/joystick
)



# Add executable for Hybrid Text Reader
add_executable(HybridTextReader
    examples/HybridTextReader.cpp
    src/st7306_driver.cpp
    src/fonts/st73xx_font.cpp
    src/st73xx_ui.cpp
    src/joystick/joystick.cpp
    src/flash_font_cache.cpp
    src/hybrid_font_system.cpp
)

target_include_directories(HybridTextReader PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/include/joystick
)

# Add executable for MicroSD Text Reader
add_executable(MicroSDTextReader
    examples/MicroSDTextReader.cpp
    src/micro_sd_text_reader/TextWrapper.cpp
    src/micro_sd_text_reader/PageManager.cpp
    src/micro_sd_text_reader/PageRenderer.cpp
    src/micro_sd_text_reader/MicroSDManager.cpp
    src/micro_sd_text_reader/TextReaderController.cpp
    src/micro_sd_text_reader/MenuController.cpp
    src/micro_sd_text_reader/MenuRenderer.cpp
    src/button/button_event.cpp
    src/st7306_driver.cpp
    src/button/button.cpp
    src/flash_font_cache.cpp
    src/hybrid_font_system.cpp
    src/microsd/rw_sd.cpp
    src/microsd/storage_device.cpp
    src/fonts/st73xx_font.cpp
    src/st73xx_ui.cpp
)

target_include_directories(MicroSDTextReader PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/include/button
    ${CMAKE_CURRENT_LIST_DIR}/include/microsd
    ${CMAKE_CURRENT_LIST_DIR}/include/micro_sd_text_reader
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

target_link_libraries(TextReader PUBLIC
    pico_stdlib
    hardware_spi
    hardware_gpio
    hardware_i2c
    pico_stdio_usb
)



target_link_libraries(HybridTextReader PUBLIC
    pico_stdlib
    hardware_spi
    hardware_gpio
    hardware_i2c
    pico_stdio_usb
)

target_link_libraries(MicroSDTextReader PUBLIC
    pico_stdlib
    hardware_spi
    hardware_gpio
    hardware_i2c
    pico_stdio_usb
    pico_fatfs
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

pico_enable_stdio_uart(TextReader 1)



pico_enable_stdio_usb(HybridTextReader 1)
pico_enable_stdio_uart(HybridTextReader 1)

pico_enable_stdio_usb(MicroSDTextReader 1)
pico_enable_stdio_uart(MicroSDTextReader 1)





# Create map/bin/hex/uf2 files
pico_add_extra_outputs(ST7305_Display)
pico_add_extra_outputs(ST7306_Display)
pico_add_extra_outputs(MazeGame)
pico_add_extra_outputs(AnalogClockWiFi)
pico_add_extra_outputs(TextReader)

pico_add_extra_outputs(HybridTextReader)
pico_add_extra_outputs(MicroSDTextReader)

