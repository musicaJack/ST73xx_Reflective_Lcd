#include <string>
#include "st73xx_font_stream.hpp"
#include "st73xx_font_bin.hpp"
#include "pico/stdlib.h"

namespace fontstream {

template<typename LCDDriver>
void stream_text(LCDDriver& drv, const std::string& text, int font_size, uint16_t color, int delay_ms) {
    int char_w = (font_size == 16) ? 16 : 24;
    int char_h = (font_size == 16) ? 16 : 24;
    int scr_w = LCDDriver::LCD_WIDTH;
    int scr_h = LCDDriver::LCD_HEIGHT;
    int x = 0, y = 0;
    uint8_t buf[72];
    for (char c : text) {
        if (c == '\n') {
            x = 0;
            y += char_h;
            if (y + char_h > scr_h) break;
            continue;
        }
        if (!fontbin::font_get_bitmap(c, font_size, buf)) continue;
        // 绘制点阵
        for (int row = 0; row < char_h; ++row) {
            for (int col = 0; col < char_w; ++col) {
                bool pixel = false;
                if (font_size == 16) {
                    pixel = (buf[row * 2 + (col / 8)] >> (7 - (col % 8))) & 0x1;
                } else {
                    pixel = (buf[row * 3 + (col / 8)] >> (7 - (col % 8))) & 0x1;
                }
                if (pixel) drv.drawPixel(x + col, y + row, color);
            }
        }
        x += char_w;
        if (x + char_w > scr_w) {
            x = 0;
            y += char_h;
            if (y + char_h > scr_h) break;
        }
        if (delay_ms > 0) sleep_ms(delay_ms);
    }
}

} // namespace fontstream

// 显式实例化（放在命名空间外部）
#include "st7305_driver.hpp"
#include "st7306_driver.hpp"
template void fontstream::stream_text<st7305::ST7305Driver>(st7305::ST7305Driver&, const std::string&, int, uint16_t, int);
template void fontstream::stream_text<st7306::ST7306Driver>(st7306::ST7306Driver&, const std::string&, int, uint16_t, int); 