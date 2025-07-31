#include "st7305_driver.hpp"
#include <cstring>
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "st73xx_font.hpp"
#include "gfx_colors.hpp"

namespace st7305 {

// 显示缓冲区大小计算
// 168/4=42 一行共42个byte的数据，上下两行共用一行的数据，所以总行数需要除2
// 384/2=192 所以共192行，一行42个byte数据，共192*42=8064byte
constexpr uint16_t LCD_WIDTH = 168;
constexpr uint16_t LCD_HEIGHT = 384;
constexpr uint16_t LCD_DATA_WIDTH = 42;  // 168/4
constexpr uint16_t LCD_DATA_HEIGHT = 192; // 384/2
constexpr uint32_t DISPLAY_BUFFER_LENGTH = 8064; // 192*42

// ST7305命令定义
namespace {
    constexpr uint8_t CMD_DISPLAY_ON = 0xAF;
    constexpr uint8_t CMD_DISPLAY_OFF = 0xAE;
    constexpr uint8_t CMD_SET_PAGE_ADDRESS = 0xB0;
    constexpr uint8_t CMD_SET_COLUMN_ADDRESS_LSB = 0x00;
    constexpr uint8_t CMD_SET_COLUMN_ADDRESS_MSB = 0x10;
    constexpr uint8_t CMD_SET_DISPLAY_START_LINE = 0x40;
    constexpr uint8_t CMD_SET_CONTRAST = 0x81;
    constexpr uint8_t CMD_SET_SEGMENT_REMAP = 0xA0;
    constexpr uint8_t CMD_SET_ENTIRE_DISPLAY = 0xA4;
    constexpr uint8_t CMD_SET_NORMAL_DISPLAY = 0xA6;
    constexpr uint8_t CMD_SET_INVERSE_DISPLAY = 0xA7;
    constexpr uint8_t CMD_SET_MULTIPLEX_RATIO = 0xA8;
    constexpr uint8_t CMD_SET_DUTY_CYCLE = 0xA9;
    constexpr uint8_t CMD_SET_DISPLAY_OFFSET = 0xD3;
    constexpr uint8_t CMD_SET_DISPLAY_CLOCK = 0xD5;
    constexpr uint8_t CMD_SET_PRECHARGE_PERIOD = 0xD9;
    constexpr uint8_t CMD_SET_VCOMH_DESELECT = 0xDB;
    constexpr uint8_t CMD_SET_LOW_POWER_MODE = 0xAD;
    constexpr uint8_t CMD_SET_HIGH_POWER_MODE = 0xAC;
}

ST7305Driver::ST7305Driver(uint dc_pin, uint res_pin, uint cs_pin, uint sclk_pin, uint sdin_pin) :
    dc_pin_(dc_pin),
    res_pin_(res_pin),
    cs_pin_(cs_pin),
    sclk_pin_(sclk_pin),
    sdin_pin_(sdin_pin),
    display_buffer_(new uint8_t[DISPLAY_BUFFER_LENGTH]),
    font_layout_(FontLayout::Vertical)
{
    // 初始化GPIO
    gpio_init(dc_pin_);
    gpio_init(res_pin_);
    gpio_init(cs_pin_);
    gpio_init(sclk_pin_);
    gpio_init(sdin_pin_);

    gpio_set_dir(dc_pin_, GPIO_OUT);
    gpio_set_dir(res_pin_, GPIO_OUT);
    gpio_set_dir(cs_pin_, GPIO_OUT);
    gpio_set_dir(sclk_pin_, GPIO_OUT);
    gpio_set_dir(sdin_pin_, GPIO_OUT);

    // 初始化SPI
    spi_init(spi0, 40000000); // 40MHz
    spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(sclk_pin_, GPIO_FUNC_SPI);
    gpio_set_function(sdin_pin_, GPIO_FUNC_SPI);
}

ST7305Driver::~ST7305Driver() {
    delete[] display_buffer_;
}

void ST7305Driver::initialize() {
    // 初始化引脚
    gpio_set_dir(dc_pin_, GPIO_OUT);
    gpio_set_dir(res_pin_, GPIO_OUT);
    gpio_set_dir(cs_pin_, GPIO_OUT);
    gpio_set_dir(sclk_pin_, GPIO_OUT);
    gpio_set_dir(sdin_pin_, GPIO_OUT);

    // 复位时序
    gpio_put(res_pin_, 1);
    sleep_ms(10);
    gpio_put(res_pin_, 0);
    sleep_ms(10);
    gpio_put(res_pin_, 1);
    sleep_ms(10);

    // 初始化SPI
    spi_init(spi0, 40000000); // 40MHz
    spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(sclk_pin_, GPIO_FUNC_SPI);
    gpio_set_function(sdin_pin_, GPIO_FUNC_SPI);

    // 初始化显示
    writeCommand(0xD6); // NVM Load Control
    writeData(0x13);
    writeData(0x02);

    writeCommand(0xD1); // Booster Enable
    writeData(0x01);

    writeCommand(0xC0); // Gate Voltage Setting
    writeData(0x12); // VGH 17V
    writeData(0x0A); // VGL -10V

    writeCommand(0xC1); // VSHP Setting
    writeData(115);    // VSHP1 (厂商值)
    writeData(0x3E);   // VSHP2
    writeData(0x3C);   // VSHP3
    writeData(0x3C);   // VSHP4

    writeCommand(0xC2); // VSLP Setting
    writeData(0);      // VSLP1 (厂商值)
    writeData(0x21);   // VSLP2
    writeData(0x23);   // VSLP3
    writeData(0x23);   // VSLP4

    writeCommand(0xC4); // VSHN Setting
    writeData(50);     // VSHN1 (厂商值)
    writeData(0x5C);   // VSHN2
    writeData(0x5A);   // VSHN3
    writeData(0x5A);   // VSHN4

    writeCommand(0xC5); // VSLN Setting
    writeData(50);     // VSLN1 (厂商值)
    writeData(0x35);   // VSLN2
    writeData(0x37);   // VSLN3
    writeData(0x37);   // VSLN4

    writeCommand(0xD8); // OSC Setting
    writeData(0x80);   // Enable OSC, HPM Frame Rate Max = 51hZ
    writeData(0xE9);

    writeCommand(0xB2); // Frame Rate Control
    writeData(0x12);   // HPM=51hz ; LPM=1hz

    writeCommand(0xB3); // Update Period Gate EQ Control in HPM
    writeData(0xE5);
    writeData(0xF6);
    writeData(0x17);
    writeData(0x77);
    writeData(0x77);
    writeData(0x77);
    writeData(0x77);
    writeData(0x77);
    writeData(0x77);
    writeData(0x71);

    writeCommand(0xB4); // Update Period Gate EQ Control in LPM
    writeData(0x05);   // LPM EQ Control
    writeData(0x46);
    writeData(0x77);
    writeData(0x77);
    writeData(0x77);
    writeData(0x77);
    writeData(0x76);
    writeData(0x45);

    writeCommand(0x62); // Gate Timing Control
    writeData(0x32);
    writeData(0x03);
    writeData(0x1F);

    writeCommand(0xB7); // Source EQ Enable
    writeData(0x13);

    writeCommand(0xB0); // Gate Line Setting
    writeData(0x60);   // 384 line = 96 * 4

    writeCommand(0x11); // Sleep out
    sleep_ms(120);     // 重要：需要120ms延时

    writeCommand(0xC9); // Source Voltage Select
    writeData(0x00);   // VSHP1; VSLP1 ; VSHN1 ; VSLN1

    writeCommand(0x36); // Memory Data Access Control
    writeData(0x48);   // MX=1 ; DO=1

    writeCommand(0x3A); // Data Format Select
    writeData(0x11);   // 10:4write for 24bit ; 11: 3write for 24bit

    writeCommand(0xB9); // Gamma Mode Setting
    writeData(0x20);   // 20: Mono 00:4GS

    writeCommand(0xB8); // Panel Setting
    writeData(0x29);   // Panel Setting: 0x29: 1-Dot inversion, Frame inversion, One Line Interlace

    // 设置显示区域（全屏，严格对应168x384像素）
    writeCommand(0x2A); // Column Address Setting
    writeData(0x17);   // 起始列高字节
    writeData(0x24);   // 起始列低字节 (0x24-0x17=14, 14*12=168)
    writeData(0x00);   // 结束列高字节
    writeData(0x00);   // 结束列低字节

    writeCommand(0x2B); // Row Address Setting
    writeData(0x00);   // 起始行高字节
    writeData(0xBF);   // 起始行低字节 (192*2=384)
    writeData(0x00);   // 结束行高字节
    writeData(0x00);   // 结束行低字节

    writeCommand(0x35); // TE
    writeData(0x00);   // TE off

    writeCommand(0xD0); // Auto power down
    writeData(0xFF);   // Auto power down ON

    writeCommand(0x38); // HPM:high Power Mode ON

    writeCommand(0x29); // Display ON

    writeCommand(0x20); // Display Inversion Off

    writeCommand(0xBB); // Enable Clear RAM
    writeData(0x4F);   // CLR=0 ; Enable Clear RAM,clear RAM to 0
}

void ST7305Driver::writeCommand(uint8_t cmd) {
    gpio_put(dc_pin_, 0);
    gpio_put(cs_pin_, 0);
    spi_write_blocking(spi0, &cmd, 1);
    gpio_put(cs_pin_, 1);
}

void ST7305Driver::writeData(uint8_t data) {
    gpio_put(dc_pin_, 1);
    gpio_put(cs_pin_, 0);
    spi_write_blocking(spi0, &data, 1);
    gpio_put(cs_pin_, 1);
}

void ST7305Driver::writeData(const uint8_t* data, size_t len) {
    gpio_put(dc_pin_, 1);
    gpio_put(cs_pin_, 0);
    spi_write_blocking(spi0, data, len);
    gpio_put(cs_pin_, 1);
}

void ST7305Driver::clear() {
    memset(display_buffer_, 0x00, DISPLAY_BUFFER_LENGTH);
}

void ST7305Driver::fill(uint8_t data) {
    memset(display_buffer_, data, DISPLAY_BUFFER_LENGTH);
}

void ST7305Driver::writePoint(uint16_t x, uint16_t y, bool enabled) {
    uint16_t tx = x, ty = y;
    switch (rotation_) {
        case 1:
            tx = y;
            ty = LCD_WIDTH - x - 1;
            break;
        case 2:
            tx = LCD_WIDTH - x - 1;
            ty = LCD_HEIGHT - y - 1;
            break;
        case 3:
            tx = LCD_HEIGHT - y - 1;
            ty = x;
            break;
        default:
            break;
    }
    if (tx >= LCD_WIDTH || ty >= LCD_HEIGHT) return;
    // 找到是哪一行的数据
    uint16_t real_x = tx/4;
    uint16_t real_y = ty/2;
    uint16_t write_byte_index = real_y*LCD_DATA_WIDTH+real_x;
    uint8_t one_two = (ty % 2 == 0)?0:1;
    uint8_t line_bit_4 = tx % 4;
    uint8_t write_bit = 7-(line_bit_4*2+one_two);

    if (enabled) {
        display_buffer_[write_byte_index] |= (1 << write_bit);
    } else {
        display_buffer_[write_byte_index] &= ~(1 << write_bit);
    }
}

void ST7305Driver::display() {
    // 设置列地址
    writeCommand(0x2A);
    writeData(0x17);
    writeData(0x24); // 0X24-0X17=14 // 14*4*3=168

    // 设置行地址
    writeCommand(0x2B);
    writeData(0x00);
    writeData(0xBF); // 192*2=384

    // 发送写数据命令
    writeCommand(0x2C);

    // 写入显示数据
    gpio_put(dc_pin_, 1);
    gpio_put(cs_pin_, 0);
    spi_write_blocking(spi0, display_buffer_, DISPLAY_BUFFER_LENGTH);
    gpio_put(cs_pin_, 1);
}

void ST7305Driver::drawPixel(uint16_t x, uint16_t y, bool color) {
    uint16_t tx = x, ty = y;
    switch (rotation_) {
        case 1: // 90 deg
            tx = LCD_WIDTH - 1 - y;
            ty = x;
            break;
        case 2: // 180 deg
            tx = LCD_WIDTH - 1 - x;
            ty = LCD_HEIGHT - 1 - y;
            break;
        case 3: // 270 deg
            tx = y;
            ty = LCD_HEIGHT - 1 - x;
            break;
        default: // 0 deg
            // tx = x; ty = y; // No change
            break;
    }
    // 调用新的 plotPixelRaw 方法
    plotPixelRaw(tx, ty, color);
}

void ST7305Driver::displayOn(bool on) {
    writeCommand(0x28); // Display OFF
    if (on) {
        writeCommand(0x29); // Display ON
    }
}

void ST7305Driver::displaySleep(bool enabled) {
    if (enabled) {
        writeCommand(0x10); // Sleep IN
    } else {
        writeCommand(0x11); // Sleep OUT
        sleep_ms(120); // 重要：需要120ms延时
    }
}

void ST7305Driver::displayInversion(bool enabled) {
    writeCommand(enabled ? CMD_SET_INVERSE_DISPLAY : CMD_SET_NORMAL_DISPLAY);
}

void ST7305Driver::lowPowerMode() {
    if (!lpm_mode_) {
        writeCommand(0xAD);  // Low Power Mode
        lpm_mode_ = true;
        hpm_mode_ = false;
    }
}

void ST7305Driver::highPowerMode() {
    if (!hpm_mode_) {
        writeCommand(0xAC);  // High Power Mode
        hpm_mode_ = true;
        lpm_mode_ = false;
    }
}

void ST7305Driver::setFontLayout(FontLayout layout) {
    font_layout_ = layout;
}

void ST7305Driver::drawChar(uint16_t x, uint16_t y, char c, bool color) {
    if (c < 32 || c > 126) {
        return;
    }
    // 使用get_char_data API获取字符数据
    const uint8_t* char_data = font::get_char_data(c);
    for (uint8_t row = 0; row < font::FONT_HEIGHT; row++) {
        uint8_t byte = char_data[row];
        for (uint8_t col = 0; col < font::FONT_WIDTH; col++) {
            bool pixel_is_set_in_font = (byte >> (7 - col)) & 0x01;
            drawPixel(x + col, y + row, (color == BLACK && pixel_is_set_in_font) ? BLACK : WHITE);
        }
    }
}

void ST7305Driver::drawString(uint16_t x, uint16_t y, std::string_view str, bool color) {
    for (char c : str) {
        if (c < 32 || c > 126) {
            continue;
        }
        switch (rotation_) {
            case 0: // 正常横排
                drawChar(x, y, c, color);
                x += font::FONT_WIDTH;
                break;
            case 1: // 90度，竖排，字头朝上
                drawChar(x, y, c, color);
                y += font::FONT_WIDTH;
                break;
            case 2: // 180度，横排反向
                drawChar(x, y, c, color);
                x -= font::FONT_WIDTH;
                break;
            case 3: // 270度，竖排反向
                drawChar(x, y, c, color);
                y -= font::FONT_WIDTH;
                break;
            default:
                drawChar(x, y, c, color);
                x += font::FONT_WIDTH;
                break;
        }
    }
}

uint16_t ST7305Driver::getStringWidth(std::string_view str) const {
    uint16_t width = 0;
    for (char c : str) {
        if (c >= 32 && c <= 126) {
            width += font::FONT_WIDTH;
        }
    }
    return width;
}

// 新增：清屏
void ST7305Driver::clearDisplay() {
    clear();
}

// 新增：设置旋转
void ST7305Driver::setRotation(int r) {
    rotation_ = r % 4;
}

// 新增：获取旋转
int ST7305Driver::getRotation() const {
    return rotation_;
}

// 新增：显示开关
void ST7305Driver::display_on(bool enabled) {
    displayOn(enabled);
}

// 新增：休眠
void ST7305Driver::display_sleep(bool enabled) {
    displaySleep(enabled);
}

// 新增：反显
void ST7305Driver::display_Inversion(bool enabled) {
    displayInversion(enabled);
}

// 新增：低功耗
void ST7305Driver::Low_Power_Mode() {
    lowPowerMode();
}

// 新增：高功耗
void ST7305Driver::High_Power_Mode() {
    highPowerMode();
}

// 新增：plotPixelRaw 方法实现
void ST7305Driver::plotPixelRaw(uint16_t x, uint16_t y, bool color) {
    // (x,y) 已经是物理坐标，直接写入缓冲区
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) return;

    uint16_t real_x = x/4;
    uint16_t real_y = y/2;
    uint32_t write_byte_index = real_y*LCD_DATA_WIDTH+real_x;
    uint8_t one_two = (y % 2 == 0)?0:1;
    uint8_t line_bit_4 = x % 4;

    uint8_t write_bit = 7-(line_bit_4*2+one_two);

    if (color) {
        display_buffer_[write_byte_index] |= (1 << write_bit);
    }
    else {
        display_buffer_[write_byte_index] &= ~(1 << write_bit);
    }
}

uint8_t ST7305Driver::getCurrentFontWidth() const {
    return font::FONT_WIDTH;
}

} // namespace st7305 