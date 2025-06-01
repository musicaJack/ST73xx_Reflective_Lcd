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

# Link libraries
target_link_libraries(ST7305_Display PUBLIC # 或者 PRIVATE 如果这些库仅此目标使用
    pico_stdlib
    hardware_spi
    hardware_gpio                 # 添加 GPIO 库
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

# Enable usb output, disable uart output
pico_enable_stdio_usb(ST7305_Display 1)
pico_enable_stdio_uart(ST7305_Display 0)

pico_enable_stdio_usb(ST7306_Display 1)
pico_enable_stdio_uart(ST7306_Display 0)

pico_enable_stdio_usb(MazeGame 1)
pico_enable_stdio_uart(MazeGame 0)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(ST7305_Display)
pico_add_extra_outputs(ST7306_Display)
pico_add_extra_outputs(MazeGame)
