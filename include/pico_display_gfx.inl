#ifndef PICO_DISPLAY_GFX_INL
#define PICO_DISPLAY_GFX_INL

namespace pico_gfx {

template<typename Driver>
PicoDisplayGFX<Driver>::PicoDisplayGFX(Driver& driver, int16_t w, int16_t h)
    : ST73XX_UI(w, h), driver_(driver) {}

template<typename Driver>
PicoDisplayGFX<Driver>::~PicoDisplayGFX() {}

template<typename Driver>
void PicoDisplayGFX<Driver>::writePoint(uint x, uint y, bool enabled) {
    // x, y 是由 ST73XX_UI::drawPixel 传递过来的，已经过 ST73XX_UI 内部的旋转处理。
    // 直接调用驱动的原始画点函数，在物理坐标 (x,y) 上画点。
    driver_.plotPixelRaw(x, y, enabled);
}

template<typename Driver>
void PicoDisplayGFX<Driver>::writePoint(uint x, uint y, uint16_t color) {
    // 对于单色屏幕，将 uint16_t 类型的颜色转换为 bool 类型。
    // 通常约定：0 为关闭/背景色，非0 为点亮/前景色。
    driver_.plotPixelRaw(x, y, (color != 0));
}

template<typename Driver>
void PicoDisplayGFX<Driver>::drawPixelGray(int16_t x, int16_t y, uint8_t gray) {
    if ((x >= 0) && (x < WIDTH) && (y >= 0) && (y < HEIGHT)) {
        int16_t tx = x, ty = y;
        switch (rotation_) {
        case 1:
            tx = y;
            ty = _width - 1 - x;
            break;
        case 2:
            tx = _width - 1 - x;
            ty = _height - 1 - y;
            break;
        case 3:
            tx = _height - 1 - y;
            ty = x;
            break;
        }
        // 确保灰度值在0-3范围内
        uint8_t level = gray & 0x03;
        driver_.plotPixelGrayRaw(static_cast<uint>(tx), static_cast<uint>(ty), level);
    }
}

} // namespace pico_gfx

#endif // PICO_DISPLAY_GFX_INL 