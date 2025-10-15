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

# =============================================================================
# 项目配置变量
# =============================================================================

# 基础源文件
set(COMMON_SOURCES
    src/st73xx/st7305_driver.cpp
    src/st73xx/st7306_driver.cpp
    src/fonts/st73xx_font.cpp
    src/st73xx/st73xx_ui.cpp
)

# 基础包含目录
set(COMMON_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/include/st73xx
    ${CMAKE_CURRENT_LIST_DIR}/include/fonts
)

# 基础库
set(COMMON_LIBRARIES
    pico_stdlib
    hardware_spi
    hardware_gpio
    pico_stdio_usb
)

# 特殊库
set(JOYSTICK_LIBRARIES
    hardware_i2c
)

set(JS16TMR_JOYSTICK_LIBRARIES
    hardware_adc
)

set(WIFI_LIBRARIES
    hardware_rtc
    hardware_clocks
    pico_cyw43_arch_lwip_poll
    pico_stdio_uart
)

# =============================================================================
# 辅助函数
# =============================================================================

# 创建ST7305可执行文件的函数
function(create_st7305_target TARGET_NAME MAIN_SOURCE)
    add_executable(${TARGET_NAME}
        ${MAIN_SOURCE}
        src/st73xx/st7305_driver.cpp
        src/fonts/st73xx_font.cpp
        src/st73xx/st73xx_ui.cpp
    )
    
    target_include_directories(${TARGET_NAME} PRIVATE ${COMMON_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME} PUBLIC ${COMMON_LIBRARIES})
    
    pico_enable_stdio_usb(${TARGET_NAME} 1)
    pico_enable_stdio_uart(${TARGET_NAME} 1)
    pico_add_extra_outputs(${TARGET_NAME})
endfunction()

# 创建ST7306可执行文件的函数
function(create_st7306_target TARGET_NAME MAIN_SOURCE)
    add_executable(${TARGET_NAME}
        ${MAIN_SOURCE}
        src/st73xx/st7306_driver.cpp
        src/fonts/st73xx_font.cpp
        src/st73xx/st73xx_ui.cpp
    )
    
    target_include_directories(${TARGET_NAME} PRIVATE ${COMMON_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME} PUBLIC ${COMMON_LIBRARIES})
    
    pico_enable_stdio_usb(${TARGET_NAME} 1)
    pico_enable_stdio_uart(${TARGET_NAME} 1)
    pico_add_extra_outputs(${TARGET_NAME})
endfunction()

# 创建带特殊库的ST7306可执行文件的函数
function(create_st7306_target_with_libs TARGET_NAME MAIN_SOURCE EXTRA_LIBS)
    add_executable(${TARGET_NAME}
        ${MAIN_SOURCE}
        src/st73xx/st7306_driver.cpp
        src/fonts/st73xx_font.cpp
        src/st73xx/st73xx_ui.cpp
    )
    
    target_include_directories(${TARGET_NAME} PRIVATE ${COMMON_INCLUDE_DIRS})
    target_link_libraries(${TARGET_NAME} PUBLIC ${COMMON_LIBRARIES} ${EXTRA_LIBS})
    
    pico_enable_stdio_usb(${TARGET_NAME} 1)
    pico_enable_stdio_uart(${TARGET_NAME} 1)
    pico_add_extra_outputs(${TARGET_NAME})
endfunction()

# 创建带特殊包含目录的ST7306可执行文件的函数
function(create_st7306_target_with_includes TARGET_NAME MAIN_SOURCE EXTRA_INCLUDES EXTRA_SOURCES EXTRA_LIBS)
    add_executable(${TARGET_NAME}
        ${MAIN_SOURCE}
        src/st73xx/st7306_driver.cpp
        src/fonts/st73xx_font.cpp
        src/st73xx/st73xx_ui.cpp
        ${EXTRA_SOURCES}
    )
    
    target_include_directories(${TARGET_NAME} PRIVATE ${COMMON_INCLUDE_DIRS} ${EXTRA_INCLUDES})
    target_link_libraries(${TARGET_NAME} PUBLIC ${COMMON_LIBRARIES} ${EXTRA_LIBS})
    
    pico_enable_stdio_usb(${TARGET_NAME} 1)
    pico_enable_stdio_uart(${TARGET_NAME} 1)
    pico_add_extra_outputs(${TARGET_NAME})
endfunction()

# 创建带JS16TMR摇杆的ST7306可执行文件的函数
function(create_st7306_target_with_js16tmr TARGET_NAME MAIN_SOURCE)
    add_executable(${TARGET_NAME}
        ${MAIN_SOURCE}
        src/st73xx/st7306_driver.cpp
        src/fonts/st73xx_font.cpp
        src/st73xx/st73xx_ui.cpp
        src/js16tmr_joystick/js16tmr_joystick_direct.cpp
        src/js16tmr_joystick/js16tmr_joystick_handler.cpp
    )
    
    target_include_directories(${TARGET_NAME} PRIVATE 
        ${COMMON_INCLUDE_DIRS}
        ${CMAKE_CURRENT_LIST_DIR}/include/js16tmr_joystick
    )
    target_link_libraries(${TARGET_NAME} PUBLIC ${COMMON_LIBRARIES} ${JS16TMR_JOYSTICK_LIBRARIES})
    
    pico_enable_stdio_usb(${TARGET_NAME} 1)
    pico_enable_stdio_uart(${TARGET_NAME} 1)
    pico_add_extra_outputs(${TARGET_NAME})
endfunction()


# =============================================================================
# 创建可执行文件
# =============================================================================

# ST7305 目标
create_st7305_target(ST7305_Display examples/st7305_demo.cpp)

# ST7306 基础目标
create_st7306_target(ST7306_Display examples/st7306_demo.cpp)
create_st7306_target(st7306_fullscreen_text_demo examples/st7306_fullscreen_text_demo.cpp)


# 带特殊库的目标
create_st7306_target_with_includes(MazeGame 
    examples/maze_game.cpp 
    "${CMAKE_CURRENT_LIST_DIR}/include/joystick"
    "src/joystick/joystick.cpp"
    "${JOYSTICK_LIBRARIES}"
)

create_st7306_target_with_libs(AnalogClockWiFi 
    examples/analog_clock_wifi.cpp 
    "${WIFI_LIBRARIES}"
)

# JS16TMR贪吃蛇游戏
create_st7306_target_with_js16tmr(SnakeGameJS16TMR 
    examples/snake_game_js16tmr.cpp
)




