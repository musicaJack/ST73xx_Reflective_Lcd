#include "st7306_driver.hpp"
#include <cstring>
#include <cstdio>
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "st73xx_font.hpp"
#include "gfx_colors.hpp"

namespace st7306 {

ST7306Driver::ST7306Driver(uint dc_pin, uint res_pin, uint cs_pin, uint sclk_pin, uint sdin_pin) :
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

    // 初始化SPI - 速率40MHz
    spi_init(spi0, 40000000); // 40MHz
    spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(sclk_pin_, GPIO_FUNC_SPI);
    gpio_set_function(sdin_pin_, GPIO_FUNC_SPI);
}

ST7306Driver::~ST7306Driver() {
    delete[] display_buffer_;
}

void ST7306Driver::initialize() {
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

    initST7306();
    
    // 初始化显示缓冲区为白色
    fill(0x00);  // 填充为白色
    // 不在初始化时自动显示，让应用程序控制第一次显示时机
    
    // 设置初始化完成标志
    initialized_ = true;
}

void ST7306Driver::initST7306() {
    writeCommand(0xD6); // NVM Load Control
    writeData(0x17);
    writeData(0x02);

    writeCommand(0xD1); // Booster Enable
    writeData(0x01);

    // 完全匹配原厂代码ST7306_4p2_BW_DisplayDriver.cpp中的Initial_ST7305函数
    writeCommand(0xC0); // Gate Voltage Setting
    writeData(0x12); // VGH 17V
    writeData(0x0A); // VGL -10V

    // VLC=3.6V (12/-5)(delta Vp=0.6V)
    writeCommand(0xC1); // VSHP Setting (4.8V)
    writeData(115);    // VSHP1
    writeData(0x3E);   // VSHP2
    writeData(0x3C);   // VSHP3
    writeData(0x3C);   // VSHP4

    writeCommand(0xC2); // VSLP Setting (0.98V)
    writeData(0);      // VSLP1
    writeData(0x21);   // VSLP2
    writeData(0x23);   // VSLP3
    writeData(0x23);   // VSLP4

    writeCommand(0xC4); // VSHN Setting (-3.6V)
    writeData(50);     // VSHN1
    writeData(0x5C);   // VSHN2
    writeData(0x5A);   // VSHN3
    writeData(0x5A);   // VSHN4

    writeCommand(0xC5); // VSLN Setting (0.22V)
    writeData(50);     // VSLN1
    writeData(0x35);   // VSLN2
    writeData(0x37);   // VSLN3
    writeData(0x37);   // VSLN4

    writeCommand(0xD8); // OSC Setting
    writeData(0xA6);
    writeData(0xE9);

    writeCommand(0xB2); // Frame Rate Control
    writeData(0x12);   // HPM=32hz ; LPM=1hz

    // 添加B3命令 - Update Period Gate EQ Control in HPM
    writeCommand(0xB3);
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

    // 添加B4命令 - Update Period Gate EQ Control in LPM
    writeCommand(0xB4);
    writeData(0x05);
    writeData(0x46);
    writeData(0x77);
    writeData(0x77);
    writeData(0x77);
    writeData(0x77);
    writeData(0x76);
    writeData(0x45);

    // 添加Gate Timing Control
    writeCommand(0x62);
    writeData(0x32);
    writeData(0x03);
    writeData(0x1F);
    
    // Source EQ Enable
    writeCommand(0xB7);
    writeData(0x13);
    
    // Gate Line Setting
    writeCommand(0xB0);
    writeData(0x64); // 400行 = 100*4

    writeCommand(0x11); // Sleep out
    sleep_ms(120);

    writeCommand(0xC9); // Source Voltage Select
    writeData(0x00);   // VSHP1; VSLP1 ; VSHN1 ; VSLN1

    writeCommand(0x36); // Memory Data Access Control
    writeData(0x48);   // MX=1 ; DO=1

    writeCommand(0x3A); // Data Format Select
    writeData(0x11);   // 11: 3write for 24bit

    writeCommand(0xB9); // Gamma Mode Setting
    writeData(0x20);   // 20: Mono

    writeCommand(0xB8); // Panel Setting
    writeData(0x29);   // 1-Dot inversion, Frame inversion, One Line Interlace

    writeCommand(0x2A); // Column Address Setting
    writeData(0x05);
    writeData(0x36);

    writeCommand(0x2B); // Row Address Setting
    writeData(0x00);
    writeData(0xC7);   // 0xC7 = 199 (LCD_DATA_HEIGHT-1)

    writeCommand(0x35); // TE
    writeData(0x00);

    writeCommand(0xD0); // Auto power down
    writeData(0xFF);   // Auto power down ON

    writeCommand(0x38); // HPM:high Power Mode ON

    writeCommand(0x29); // Display ON

    writeCommand(0x20); // Display Inversion Off

    writeCommand(0xBB); // Enable Clear RAM
    writeData(0x4F);   // CLR=0 ; Enable Clear RAM,clear RAM to 0

    // 等待清除完成
    sleep_ms(10);
    
    // 重新设置地址窗口，确保显示正常
    writeCommand(0x2A); // Column Address Setting
    writeData(0x05);
    writeData(0x36);

    writeCommand(0x2B); // Row Address Setting  
    writeData(0x00);
    writeData(0xC7);

    hpm_mode_ = true;
    lpm_mode_ = false;
}

void ST7306Driver::writeCommand(uint8_t cmd) {
    gpio_put(dc_pin_, 0);
    gpio_put(cs_pin_, 0);
    spi_write_blocking(spi0, &cmd, 1);
    gpio_put(cs_pin_, 1);
}

void ST7306Driver::writeData(uint8_t data) {
    gpio_put(dc_pin_, 1);
    gpio_put(cs_pin_, 0);
    spi_write_blocking(spi0, &data, 1);
    gpio_put(cs_pin_, 1);
}

void ST7306Driver::writeData(const uint8_t* data, size_t len) {
    gpio_put(dc_pin_, 1);
    gpio_put(cs_pin_, 0);
    spi_write_blocking(spi0, data, len);
    gpio_put(cs_pin_, 1);
}

void ST7306Driver::clear() {
    memset(display_buffer_, 0x00, DISPLAY_BUFFER_LENGTH);
}

void ST7306Driver::fill(uint8_t data) {
    memset(display_buffer_, data, DISPLAY_BUFFER_LENGTH);
}

void ST7306Driver::writePoint(uint16_t x, uint16_t y, bool enabled) {
    // 将布尔值转换为灰度值：true -> COLOR_BLACK (0x03), false -> COLOR_WHITE (0x00)
    writePointGray(x, y, enabled ? COLOR_BLACK : COLOR_WHITE);
}

void ST7306Driver::display() {
    setAddress();
    
    // 按照原厂Arduino代码的写法传输数据
    writeData(display_buffer_, DISPLAY_BUFFER_LENGTH);
    
    // printf("Display buffer transmitted: %d bytes\n", DISPLAY_BUFFER_LENGTH); // 已注释，避免频繁输出
}

void ST7306Driver::setAddress() {
    // 完全按照原厂驱动代码中的address函数
    writeCommand(0x2A); // Column Address Setting S61~S182
    writeData(0x05);    // Start column address
    writeData(0x36);    // End column address 0x36 = 54

    writeCommand(0x2B); // Row Address Setting G1~G250  
    writeData(0x00);    // Start row address
    writeData(0xC7);    // End row address 0xC7 = 199

    writeCommand(0x2C); // write image data
}

void ST7306Driver::drawPixel(uint16_t x, uint16_t y, bool color) {
    uint16_t tx = x, ty = y;
    switch (rotation_) {
        case 1:
            tx = LCD_WIDTH - 1 - y;
            ty = x;
            break;
        case 2:
            tx = LCD_WIDTH - 1 - x;
            ty = LCD_HEIGHT - 1 - y;
            break;
        case 3:
            tx = y;
            ty = LCD_HEIGHT - 1 - x;
            break;
        default:
            break;
    }
    plotPixelRaw(tx, ty, color);
}

void ST7306Driver::plotPixelRaw(uint16_t x, uint16_t y, bool color) {
    writePoint(x, y, color);
}

void ST7306Driver::plotPixelGrayRaw(uint16_t x, uint16_t y, uint8_t gray_level) {
    // 确保灰度级别在正确范围内 (0-3)
    uint8_t level = gray_level & 0x03;
    writePointGray(x, y, level);
}

void ST7306Driver::displayOn(bool enabled) {
    writeCommand(enabled ? 0x29 : 0x28);
}

void ST7306Driver::displaySleep(bool enabled) {
    if (enabled) {
        if (lpm_mode_) {
            writeCommand(0x38); // HPM:high Power Mode ON
            sleep_ms(300);
        }
        writeCommand(0x10); // Sleep IN
        sleep_ms(100);
    } else {
        writeCommand(0x11); // Sleep OUT
        sleep_ms(100);
    }
}

void ST7306Driver::displayInversion(bool enabled) {
    writeCommand(enabled ? 0x21 : 0x20);
}

void ST7306Driver::lowPowerMode() {
    if (!lpm_mode_) {
        writeCommand(0x39); // LPM:Low Power Mode ON
        hpm_mode_ = false;
        lpm_mode_ = true;
    }
}

void ST7306Driver::highPowerMode() {
    if (!hpm_mode_) {
        writeCommand(0x38); // HPM:high Power Mode ON
        hpm_mode_ = true;
        lpm_mode_ = false;
    }
}

void ST7306Driver::clearDisplay() {
    clear();
}

void ST7306Driver::setRotation(int r) {
    rotation_ = r & 0x03;
}

int ST7306Driver::getRotation() const {
    return rotation_;
}

void ST7306Driver::display_on(bool enabled) {
    displayOn(enabled);
}

void ST7306Driver::display_sleep(bool enabled) {
    displaySleep(enabled);
}

void ST7306Driver::display_Inversion(bool enabled) {
    displayInversion(enabled);
}

void ST7306Driver::Low_Power_Mode() {
    lowPowerMode();
}

void ST7306Driver::High_Power_Mode() {
    highPowerMode();
}

void ST7306Driver::setFontLayout(FontLayout layout) {
    font_layout_ = layout;
}

uint8_t ST7306Driver::getCurrentFontWidth() const {
    return font::FONT_WIDTH;
}

void ST7306Driver::drawString(uint16_t x, uint16_t y, const char* str, bool color) {
    while (*str) {
        if (*str < 32 || *str > 126) {
            str++;
            continue;
        }
        
        drawChar(x, y, *str, color);
        
        switch (rotation_) {
            case 0: // 正常横排
                x += font::FONT_WIDTH;
                break;
            case 1: // 90度，竖排，字头朝上
                y += font::FONT_WIDTH;
                break;
            case 2: // 180度，横排反向
                x -= font::FONT_WIDTH;
                break;
            case 3: // 270度，竖排反向
                y -= font::FONT_WIDTH;
                break;
            default:
                x += font::FONT_WIDTH;
                break;
        }
        str++;
    }
}

void ST7306Driver::drawChar(uint16_t x, uint16_t y, char c, bool color) {
    // 只支持黑白显示模式
    const uint8_t* char_data = font::get_char_data(c);
    if (!char_data) return;

    for (int dy = 0; dy < font::FONT_HEIGHT; dy++) {
        for (int dx = 0; dx < font::FONT_WIDTH; dx++) {
            bool pixel = (char_data[dy] >> (7 - dx)) & 0x01;
            drawPixel(x + dx, y + dy, color ? pixel : !pixel);
        }
    }
}

void ST7306Driver::writePointGray(uint16_t x, uint16_t y, uint8_t color) {
    if(x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
    
    // 原厂驱动中的详细注释:
    // 像素数据结构为：
    // P0P2 P4P6
    // P1P3 P5P7
    // 对应一个byte数据的：
    // BIT7 BIT5 BIT3 BIT1
    // BIT6 BIT4 BIT2 BIT0
    
    uint real_x = x/2; // 0->0, 1->0, 2->1, 3->1
    uint real_y = y/2; // 0->0, 1->0, 2->1, 3->1
    uint write_byte_index = real_y*LCD_DATA_WIDTH+real_x;
    uint one_two = (y % 2 == 0)?0:1; // 0表示上行，1表示下行
    uint line_bit_1 = (x % 2)*4;     // 0或4
    uint line_bit_0 = (x % 2)*4 + 2; // 2或6
    uint8_t write_bit_1 = 7-(line_bit_1+one_two); // 7, 6, 3, 2
    uint8_t write_bit_0 = 7-(line_bit_0+one_two); // 5, 4, 1, 0

    // 分解2位灰度值
    bool data_bit0 = (color & 0x01) > 0;
    bool data_bit1 = (color & 0x02) > 0;
    
    // 写入第一位
    if (data_bit1) {
        // 将指定位置的 bit 置为 1
        display_buffer_[write_byte_index] |= (1 << write_bit_1);
    } else {
        // 将指定位置的 bit 置为 0
        display_buffer_[write_byte_index] &= ~(1 << write_bit_1);
    }

    // 写入第二位
    if (data_bit0) {
        // 将指定位置的 bit 置为 1
        display_buffer_[write_byte_index] |= (1 << write_bit_0);
    } else {
        // 将指定位置的 bit 置为 0
        display_buffer_[write_byte_index] &= ~(1 << write_bit_0);
    }
}

void ST7306Driver::drawPixelGray(uint16_t x, uint16_t y, uint8_t gray_level) {
    uint16_t tx = x, ty = y;
    switch (rotation_) {
        case 1:
            tx = LCD_WIDTH - 1 - y;
            ty = x;
            break;
        case 2:
            tx = LCD_WIDTH - 1 - x;
            ty = LCD_HEIGHT - 1 - y;
            break;
        case 3:
            tx = y;
            ty = LCD_HEIGHT - 1 - x;
            break;
        default:
            break;
    }
    plotPixelGrayRaw(tx, ty, gray_level);
}

uint16_t ST7306Driver::getStringWidth(std::string_view str) const {
    uint16_t width = 0;
    for (char c : str) {
        if (c >= 32 && c <= 126) {
            width += font::FONT_WIDTH;
        }
    }
    return width;
}

void ST7306Driver::drawString(uint16_t x, uint16_t y, std::string_view str, bool color) {
    drawString(x, y, str.data(), color);
}

void ST7306Driver::setDisplayMode(DisplayMode mode) {
    if (display_mode_ != mode) {
        display_mode_ = mode;
        updateDisplayMode();
    }
}

DisplayMode ST7306Driver::getDisplayMode() const {
    return display_mode_;
}

void ST7306Driver::updateDisplayMode() {
    switch (display_mode_) {
        case DisplayMode::Day:
            // 白底黑字模式
            writeCommand(0x21); // Display Inversion On
            writeCommand(0xB9); // Gamma Mode Setting
            writeData(0x20);   // Mono mode
            break;
            
        case DisplayMode::Night:
            // 黑底白字模式
            writeCommand(0x20); // Display Inversion Off
            writeCommand(0xB9); // Gamma Mode Setting
            writeData(0x20);   // Mono mode
            break;
    }
    // 不自动刷新显示，让调用者决定何时刷新
}

bool ST7306Driver::is_initialized() const {
    return initialized_;
}



} // namespace st7306 