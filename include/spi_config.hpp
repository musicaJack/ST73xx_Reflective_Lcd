#ifndef SPI_CONFIG_HPP
#define SPI_CONFIG_HPP

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <string>

// === 显示屏 SPI 配置 ===

// 显示屏 SPI 端口定义
#define SPI_PORT_TFT spi0  // 显示屏使用 SPI0

// 显示屏引脚定义 (SPI0 + 控制引脚)
#define TFT_PIN_DC          20      // DC引脚 (GPIO20)   - 数据/命令切换
#define TFT_PIN_RST         15      // RST引脚 (GPIO15)  - 复位
#define TFT_PIN_CS          17      // CS引脚 (GPIO17)   - 片选
#define TFT_PIN_SCK         18      // SCK引脚 (GPIO18)  - 时钟
#define TFT_PIN_MOSI        19      // MOSI引脚 (GPIO19) - 主出从入

// 显示屏频率定义
#define TFT_SPI_FREQUENCY   (40 * 1000 * 1000)  // 40MHz

// 向后兼容性定义
#define SPI_PORT        SPI_PORT_TFT
#define PIN_DC          TFT_PIN_DC
#define PIN_RST         TFT_PIN_RST
#define PIN_CS          TFT_PIN_CS
#define PIN_SCLK        TFT_PIN_SCK
#define PIN_SDIN        TFT_PIN_MOSI
#define SPI_FREQUENCY   TFT_SPI_FREQUENCY


// === I2C 配置（用于摇杆） ===

// 摇杆 I2C 配置
#define JOYSTICK_I2C_ADDR       0x63
#define JOYSTICK_PIN_SDA        6
#define JOYSTICK_PIN_SCL        7
#define JOYSTICK_I2C_FREQUENCY  100000

// 摇杆 LED 颜色定义 (避免与 joystick_config.hpp 重复)
#ifndef JOYSTICK_LED_OFF
#define JOYSTICK_LED_OFF        0x00000000
#endif
#ifndef JOYSTICK_LED_RED
#define JOYSTICK_LED_RED        0x00FF0000
#endif
#ifndef JOYSTICK_LED_GREEN
#define JOYSTICK_LED_GREEN      0x0000FF00
#endif
#ifndef JOYSTICK_LED_BLUE
#define JOYSTICK_LED_BLUE       0x000000FF
#endif

// 摇杆死区阈值
#define JOYSTICK_DEADZONE       1000

// 向后兼容性定义
#ifndef JOYSTICK_ADDR
#define JOYSTICK_ADDR       JOYSTICK_I2C_ADDR
#endif
#define PIN_SDA             JOYSTICK_PIN_SDA
#define PIN_SCL             JOYSTICK_PIN_SCL
#define I2C_FREQUENCY       JOYSTICK_I2C_FREQUENCY
#define JOY_DEADZONE        JOYSTICK_DEADZONE


#endif // SPI_CONFIG_HPP 