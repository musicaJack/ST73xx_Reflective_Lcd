#ifndef SPI_CONFIG_HPP
#define SPI_CONFIG_HPP

// SPI和硬件引脚定义
#define SPI_PORT spi0
#define PIN_DC   20
#define PIN_RST  15
#define PIN_CS   17
#define PIN_SCLK 18
#define PIN_SDIN 19

// SPI时钟频率配置 (40MHz)
#define SPI_FREQUENCY 40000000

// I2C配置（用于摇杆）
#define JOYSTICK_ADDR 0x63
#define PIN_SDA 6
#define PIN_SCL 7
#define I2C_FREQUENCY 100000

// LED颜色定义
#define JOYSTICK_LED_OFF   0x00000000
#define JOYSTICK_LED_RED   0x00FF0000
#define JOYSTICK_LED_GREEN 0x0000FF00
#define JOYSTICK_LED_BLUE  0x000000FF

// 死区阈值
#define JOY_DEADZONE 1000

#endif // SPI_CONFIG_HPP 