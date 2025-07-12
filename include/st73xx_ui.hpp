#ifndef ST73XX_UI_HPP
#define ST73XX_UI_HPP

#include "pico/stdlib.h"
#include <cstdint>

#define value_interchange(a, b) do { (a) ^= (b); (b) ^= (a); (a) ^= (b); } while(0)

class ST73XX_UI {
public:
    ST73XX_UI(int16_t w, int16_t h);
    virtual ~ST73XX_UI();

    // 纯虚函数，由子类 (PicoDisplayGFX) 实现
    virtual void writePoint(uint x, uint y, bool enabled) = 0;
    virtual void writePoint(uint x, uint y, uint16_t color) = 0; // uint16_t color 用于兼容，单色屏会转为bool

    // 绘图函数声明
    void drawPixel(int16_t x, int16_t y, bool enabled);
    void drawPixel(int16_t x, int16_t y, uint16_t color);

    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);

    void drawRectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void drawFilledRectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color); // fillRect -> drawFilledRectangle
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color); // Adafruit GFX name, for compatibility if used by drawChar etc.

    void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
    void drawFilledCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color); // fillCircle -> drawFilledCircle

    void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
    void drawFilledTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);

    void drawPolygon(const int16_t *x, const int16_t *y, uint8_t sides, uint16_t color); // Adjusted for common polygon passing
    void drawFilledPolygon(const int16_t *x, const int16_t *y, uint8_t sides, uint16_t color); // Adjusted

    void fillScreen(uint16_t color);

    // 文本相关 (Adafruit GFX 风格)
    void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y);
    // (setCursor, setTextSize, setTextColor etc. would go here if implementing full Adafruit_GFX text)

    void setRotation(uint8_t r);
    uint8_t getRotation(void) const;

    // Getter for display dimensions
    int16_t width() const;
    int16_t height() const;

    // 允许直接访问，或通过 width()/height()
    int16_t WIDTH;  ///< Display width as modified by current rotation
    int16_t HEIGHT; ///< Display height as modified by current rotation

protected:
    int16_t _width;  // Physical display width
    int16_t _height; // Physical display height
    uint8_t rotation_;
    // GFXFont *gfxFont;
};

#endif 