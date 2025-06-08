#pragma once

#include <cstdint>
#include <cstring>
#include <string_view>
#include "pico/stdlib.h"

namespace st7306 {

// 字体点阵布局类型
enum class FontLayout {
    Horizontal, // 横向点阵：每列一个字节
    Vertical   // 竖向点阵：每行一个字节
};

class ST7306Driver {
public:
    // 颜色定义
    static constexpr uint8_t COLOR_WHITE = 0x00;
    static constexpr uint8_t COLOR_BLACK = 0x03;
    static constexpr uint8_t COLOR_GRAY1 = 0x01;
    static constexpr uint8_t COLOR_GRAY2 = 0x02;

    // 显示参数
    static constexpr uint16_t LCD_WIDTH = 300;
    static constexpr uint16_t LCD_HEIGHT = 400;
    static constexpr uint16_t LCD_DATA_WIDTH = 150;  // LCD_WIDTH / 2
    static constexpr uint16_t LCD_DATA_HEIGHT = 200; // LCD_HEIGHT / 2
    static constexpr uint32_t DISPLAY_BUFFER_LENGTH = LCD_DATA_WIDTH * LCD_DATA_HEIGHT;

    // 构造函数
    ST7306Driver(uint dc_pin, uint res_pin, uint cs_pin, uint sclk_pin, uint sdin_pin);
    ~ST7306Driver();

    // 初始化函数
    void initialize();
    void clear();
    void display();

    // 绘图函数
    void drawPixel(uint16_t x, uint16_t y, bool color);
    void drawPixelGray(uint16_t x, uint16_t y, uint8_t gray_level);
    void fill(uint8_t data);

    // 文本显示函数
    void drawChar(uint16_t x, uint16_t y, char c, bool color);
    void drawString(uint16_t x, uint16_t y, std::string_view str, bool color);
    void drawString(uint16_t x, uint16_t y, const char* str, bool color);
    uint16_t getStringWidth(std::string_view str) const;

    // 显示控制
    void displayOn(bool enabled);
    void displaySleep(bool enabled);
    void displayInversion(bool enabled);
    void lowPowerMode();
    void highPowerMode();

    // 新增接口
    void clearDisplay();
    void setRotation(int r);
    int getRotation() const;
    void display_on(bool enabled);
    void display_sleep(bool enabled);
    void display_Inversion(bool enabled);
    void Low_Power_Mode();
    void High_Power_Mode();

    void plotPixelRaw(uint16_t x, uint16_t y, bool color);
    void plotPixelGrayRaw(uint16_t x, uint16_t y, uint8_t gray_level);

    uint8_t getCurrentFontWidth() const;

    void setFontLayout(FontLayout layout);

private:
    void writeCommand(uint8_t cmd);
    void writeData(uint8_t data);
    void writeData(const uint8_t* data, size_t len);
    void writePoint(uint16_t x, uint16_t y, bool enabled);
    void writePointGray(uint16_t x, uint16_t y, uint8_t color);

    const uint dc_pin_;
    const uint res_pin_;
    const uint cs_pin_;
    const uint sclk_pin_;
    const uint sdin_pin_;
    uint8_t* display_buffer_;

    bool hpm_mode_ = false;
    bool lpm_mode_ = false;

    int rotation_ = 0; // 0:默认，1:90度，2:180度，3:270度

    FontLayout font_layout_ = FontLayout::Vertical;

    // 私有辅助函数
    void setAddress();
    void initST7306();
};

} // namespace st7306 