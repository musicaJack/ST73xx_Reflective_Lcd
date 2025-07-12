#ifndef SPI_CONFIG_HPP
#define SPI_CONFIG_HPP

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "button_config.hpp"
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

// === 按键配置 ===
// 按键配置已移至 button_config.hpp，这里保持向后兼容性

// 向后兼容性定义
#define KEY1_PIN               BUTTON_KEY1_PIN
#define KEY2_PIN               BUTTON_KEY2_PIN

// === I2C 配置（用于摇杆） ===

// 摇杆 I2C 配置
#define JOYSTICK_I2C_ADDR       0x63
#define JOYSTICK_PIN_SDA        6
#define JOYSTICK_PIN_SCL        7
#define JOYSTICK_I2C_FREQUENCY  100000

// 摇杆 LED 颜色定义
#define JOYSTICK_LED_OFF        0x00000000
#define JOYSTICK_LED_RED        0x00FF0000
#define JOYSTICK_LED_GREEN      0x0000FF00
#define JOYSTICK_LED_BLUE       0x000000FF

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

// === MicroSD 卡 SPI 配置 ===

// MicroSD SPI 端口定义
#define SPI_PORT_MICROSD spi1  // MicroSD 使用 SPI1

// MicroSD 引脚定义 (SPI1 默认引脚映射)
#define MICROSD_PIN_MISO        11      // MISO引脚 (GPIO12) - 主入从出
#define MICROSD_PIN_CS          13      // CS引脚 (GPIO13)   - 片选
#define MICROSD_PIN_SCK         10      // SCK引脚 (GPIO10)  - 时钟
#define MICROSD_PIN_MOSI        12      // MOSI引脚 (GPIO11) - 主出从入

// MicroSD 频率定义
#define MICROSD_SPI_FREQ_SLOW_DEFAULT    (400 * 1000)       // 慢速时钟频率 (400KHz用于初始化)
#define MICROSD_SPI_FREQ_FAST_DEFAULT    (40 * 1000 * 1000) // 快速时钟频率 (40MHz用于正常操作)
#define MICROSD_SPI_FREQ_SLOW_COMPAT     (200 * 1000)       // 兼容性慢速频率
#define MICROSD_SPI_FREQ_FAST_COMPAT     (20 * 1000 * 1000) // 兼容性快速频率
#define MICROSD_SPI_FREQ_FAST_HIGH       (50 * 1000 * 1000) // 高速频率

// MicroSD 配置标志
#define MICROSD_USE_INTERNAL_PULLUP     true    // 默认使用内部上拉电阻

namespace MicroSD {

/**
 * @brief MicroSD 引脚配置结构体
 */
struct PinConfig {
    uint pin_miso = MICROSD_PIN_MISO;        // MISO引脚
    uint pin_cs = MICROSD_PIN_CS;            // CS引脚
    uint pin_sck = MICROSD_PIN_SCK;          // SCK引脚
    uint pin_mosi = MICROSD_PIN_MOSI;        // MOSI引脚
    bool use_internal_pullup = MICROSD_USE_INTERNAL_PULLUP;  // 使用内部上拉电阻
    
    // 引脚功能验证
    bool is_valid() const {
        return pin_miso <= 29 && pin_cs <= 29 && 
               pin_sck <= 29 && pin_mosi <= 29;
    }
    
    // 获取引脚描述
    std::string get_description() const {
        return "MISO:" + std::to_string(pin_miso) + 
               " CS:" + std::to_string(pin_cs) + 
               " SCK:" + std::to_string(pin_sck) + 
               " MOSI:" + std::to_string(pin_mosi);
    }
};

/**
 * @brief MicroSD SPI配置结构体
 */
struct SPIConfig {
    spi_inst_t* spi_port = SPI_PORT_MICROSD;          // SPI端口 (使用 SPI1)
    uint32_t clk_slow = MICROSD_SPI_FREQ_SLOW_DEFAULT;       // 慢速时钟频率
    uint32_t clk_fast = MICROSD_SPI_FREQ_FAST_DEFAULT;       // 快速时钟频率
    PinConfig pins;                       // 引脚配置
    
    // 验证配置
    bool is_valid() const {
        return spi_port != nullptr && pins.is_valid();
    }
    
    // 获取配置描述
    std::string get_description() const {
        return "SPI" + std::to_string(spi_port == spi0 ? 0 : 1) + 
               " Slow:" + std::to_string(clk_slow/1000) + "KHz" +
               " Fast:" + std::to_string(clk_fast/1000000) + "MHz" +
               " Pins:" + pins.get_description();
    }
};

/**
 * @brief MicroSD 预定义配置
 */
namespace Config {
    // 默认配置
    inline const SPIConfig DEFAULT = {
        .spi_port = SPI_PORT_MICROSD,
        .clk_slow = MICROSD_SPI_FREQ_SLOW_DEFAULT,
        .clk_fast = MICROSD_SPI_FREQ_FAST_DEFAULT,
        .pins = {MICROSD_PIN_MISO, MICROSD_PIN_CS, MICROSD_PIN_SCK, MICROSD_PIN_MOSI, MICROSD_USE_INTERNAL_PULLUP}
    };
    
    // 高速配置
    inline const SPIConfig HIGH_SPEED = {
        .spi_port = SPI_PORT_MICROSD,
        .clk_slow = MICROSD_SPI_FREQ_SLOW_DEFAULT,
        .clk_fast = MICROSD_SPI_FREQ_FAST_HIGH,
        .pins = {MICROSD_PIN_MISO, MICROSD_PIN_CS, MICROSD_PIN_SCK, MICROSD_PIN_MOSI, MICROSD_USE_INTERNAL_PULLUP}
    };
    
    // 兼容性配置 (较低频率)
    inline const SPIConfig COMPATIBLE = {
        .spi_port = SPI_PORT_MICROSD,
        .clk_slow = MICROSD_SPI_FREQ_SLOW_COMPAT,
        .clk_fast = MICROSD_SPI_FREQ_FAST_COMPAT,
        .pins = {MICROSD_PIN_MISO, MICROSD_PIN_CS, MICROSD_PIN_SCK, MICROSD_PIN_MOSI, MICROSD_USE_INTERNAL_PULLUP}
    };
}

} // namespace MicroSD

#endif // SPI_CONFIG_HPP 