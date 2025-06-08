#include "st73xx_ui.hpp"
#include <cstdlib>
#include "gfx_colors.hpp"

#define ABS_DIFF(x, y) (((x) > (y))? ((x) - (y)) : ((y) - (x)))

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

ST73XX_UI::ST73XX_UI(int16_t w, int16_t h) : WIDTH(w), HEIGHT(h), _width(w), _height(h), rotation_(0) {}
ST73XX_UI::~ST73XX_UI() {}

void ST73XX_UI::writePoint(uint x, uint y, bool enabled) {
    // 需由子类实现
}

void ST73XX_UI::writePoint(uint x, uint y, uint16_t color) {
    // 需由子类实现
}

void ST73XX_UI::drawPixel(int16_t x, int16_t y, bool enabled) {
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
        writePoint(static_cast<uint>(tx), static_cast<uint>(ty), enabled);
    }
}

void ST73XX_UI::drawPixel(int16_t x, int16_t y, uint16_t color) {
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
        writePoint(static_cast<uint>(tx), static_cast<uint>(ty), color);
    }
}

void ST73XX_UI::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
    int16_t phy_x = x, phy_y = y;
    int16_t phy_w = w;
    bool horizontal_after_rotation = true;

    switch (rotation_) {
        case 0:
            break;
        case 1:
            phy_x = y;
            phy_y = _width - 1 - (x + w -1);
            phy_w = w;
            horizontal_after_rotation = false;
            value_interchange(phy_x, phy_y);
            phy_x = _width -1 -x;
            drawLine(x,y, x + w -1, y, color); return;
        case 2:
            phy_x = _width - 1 - (x + w - 1);
            phy_y = _height - 1 - y;
            break;
        case 3:
            drawLine(x,y, x + w -1, y, color); return;
    }
    if (rotation_ == 0 || rotation_ == 2) {
        for (int16_t i = 0; i < phy_w; i++) {
            writePoint(static_cast<uint>(phy_x + i), static_cast<uint>(phy_y), color);
        }
    } else {
        drawLine(x, y, x + w - 1, y, color);
    }
}

void ST73XX_UI::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
    if (rotation_ == 0 || rotation_ == 2) {
        int16_t phy_x = x, phy_y = y;
        if (rotation_ == 2) {
            phy_x = _width - 1 - x;
            phy_y = _height - 1 - (y + h - 1);
        }
         for (int16_t i = 0; i < h; i++) {
            writePoint(static_cast<uint>(phy_x), static_cast<uint>(phy_y + i), color);
        }
    } else {
        drawLine(x, y, x, y + h - 1, color);
    }
}

void ST73XX_UI::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    if ((x0 == x1) && (y0 == y1)) {
        drawPixel(x0, y0, color);
        return;
    }
    if (x0 == x1) {
        if (y0 > y1) value_interchange(y0, y1);
        drawFastVLine(x0,y0, y1-y0+1, color);
        return;
    }
    if (y0 == y1) {
        if (x0 > x1) value_interchange(x0, x1);
        drawFastHLine(x0,y0, x1-x0+1, color);
        return;
    }

    bool steep = ABS_DIFF(y1, y0) > ABS_DIFF(x1, x0);
    if (steep) {
        value_interchange(x0, y0);
        value_interchange(x1, y1);
    }
    if (x0 > x1) {
        value_interchange(x0, x1);
        value_interchange(y0, y1);
    }

    int16_t dx = x1 - x0;
    int16_t dy = ABS_DIFF(y1, y0);
    int16_t err = dx / 2;
    int16_t ystep = (y0 < y1) ? 1 : -1;
    int16_t y = y0;

    for (int16_t x = x0; x <= x1; x++) {
        if (steep) {
            drawPixel(y, x, color);
        } else {
            drawPixel(x, y, color);
        }
        err -= dy;
        if (err < 0) {
            y += ystep;
            err += dx;
        }
    }
}

void ST73XX_UI::drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
    drawLine(x0, y0, x1, y1, color);
    drawLine(x1, y1, x2, y2, color);
    drawLine(x2, y2, x0, y0, color);
}

void ST73XX_UI::drawFilledTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
    int16_t a, b, y, last;
    if (y0 > y1) { value_interchange(y0, y1); value_interchange(x0, x1); }
    if (y1 > y2) { value_interchange(y2, y1); value_interchange(x2, x1); }
    if (y0 > y1) { value_interchange(y0, y1); value_interchange(x0, x1); }

    if (y0 == y2) {
        a = b = x0;
        if (x1 < a) a = x1;
        else if (x1 > b) b = x1;
        if (x2 < a) a = x2;
        else if (x2 > b) b = x2;
        drawFastHLine(a, y0, b - a + 1, color);
        return;
    }

    int16_t dx01 = x1 - x0, dy01 = y1 - y0,
            dx02 = x2 - x0, dy02 = y2 - y0,
            dx12 = x2 - x1, dy12 = y2 - y1;
    int32_t sa = 0, sb = 0;

    if (y1 == y2) last = y1;
    else last = y1 - 1;

    for (y = y0; y <= last; y++) {
        a = x0 + sa / dy01;
        b = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;
        if (a > b) value_interchange(a, b);
        drawFastHLine(a, y, b - a + 1, color);
    }

    sa = dx12 * (y - y1);
    sb = dx02 * (y - y0);
    for (; y <= y2; y++) {
        a = x1 + sa / dy12;
        b = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;
        if (a > b) value_interchange(a, b);
        drawFastHLine(a, y, b - a + 1, color);
    }
}

void ST73XX_UI::drawRectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (w <= 0 || h <= 0) return;
    drawFastHLine(x, y, w, color);
    drawFastHLine(x, y + h - 1, w, color);
    drawFastVLine(x, y, h, color);
    drawFastVLine(x + w - 1, y, h, color);
}

void ST73XX_UI::drawFilledRectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    fillRect(x, y, w, h, color);
}

void ST73XX_UI::drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    if (r < 0) return;
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    drawPixel(x0, y0 + r, color);
    drawPixel(x0, y0 - r, color);
    drawPixel(x0 + r, y0, color);
    drawPixel(x0 - r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        drawPixel(x0 + x, y0 + y, color);
        drawPixel(x0 - x, y0 + y, color);
        drawPixel(x0 + x, y0 - y, color);
        drawPixel(x0 - x, y0 - y, color);
        drawPixel(x0 + y, y0 + x, color);
        drawPixel(x0 - y, y0 + x, color);
        drawPixel(x0 + y, y0 - x, color);
        drawPixel(x0 - y, y0 - x, color);
    }
}

void ST73XX_UI::drawFilledCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    if (r < 0) return;
    drawFastVLine(x0, y0 - r, 2 * r + 1, color);
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        drawFastVLine(x0 + x, y0 - y, 2 * y + 1, color);
        drawFastVLine(x0 + y, y0 - x, 2 * x + 1, color);
        drawFastVLine(x0 - x, y0 - y, 2 * y + 1, color);
        drawFastVLine(x0 - y, y0 - x, 2 * x + 1, color);
    }
}

void ST73XX_UI::drawPolygon(const int16_t *x, const int16_t *y, uint8_t sides, uint16_t color) {
    if (sides < 3) return;
    for (uint8_t i = 0; i < sides - 1; i++) {
        drawLine(x[i], y[i], x[i+1], y[i+1], color);
    }
    drawLine(x[sides-1], y[sides-1], x[0], y[0], color);
}

void ST73XX_UI::drawFilledPolygon(const int16_t *vx, const int16_t *vy, uint8_t sides, uint16_t color) {
    if (sides < 3) return;
    int16_t i, j, miny, maxy, x1, y1, x2, y2, ind1, ind2;
    miny = vy[0]; maxy = vy[0];
    for (i = 1; i < sides; i++) {
        if (vy[i] < miny) miny = vy[i];
        if (vy[i] > maxy) maxy = vy[i];
    }
    int16_t *nodeX = new int16_t[sides];
    for (int16_t y = miny; y <= maxy; y++) {
        int nodes = 0;
        j = sides - 1;
        for (i = 0; i < sides; i++) {
            y1 = vy[i]; y2 = vy[j];
            if (((y1 <= y) && (y2 > y)) || ((y2 <= y) && (y1 > y))) {
                x1 = vx[i]; x2 = vx[j];
                nodeX[nodes++] = (int16_t)(x1 + (float)(y - y1) / (y2 - y1) * (x2 - x1));
            }
            j = i;
        }
        for(i=0; i<nodes-1; ++i) {
            for(j=0; j<nodes-i-1; ++j) {
                if(nodeX[j] > nodeX[j+1]) {
                    value_interchange(nodeX[j], nodeX[j+1]);
                }
            }
        }
        for (i = 0; i < nodes; i += 2) {
            if (nodeX[i] >= WIDTH) break;
            if (nodeX[i+1] > 0) {
                if (nodeX[i] < 0) nodeX[i] = 0;
                if (nodeX[i+1] > WIDTH) nodeX[i+1] = WIDTH;
                drawFastHLine(nodeX[i], y, nodeX[i+1] - nodeX[i] + 1, color);
            }
        }
    }
    delete[] nodeX;
}

void ST73XX_UI::fillScreen(uint16_t color) {
    fillRect(0, 0, WIDTH, HEIGHT, color);
}

void ST73XX_UI::drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y) {
    if (c < 32 || c > 126) return;

    if (size_x == 0 || size_y == 0) return;

    if (color != bg) {
        fillRect(x, y, 5 * size_x, 7 * size_y, color);
    }
}

void ST73XX_UI::setRotation(uint8_t r) {
    rotation_ = r % 4;
    switch (rotation_) {
    case 0:
    case 2:
        WIDTH = _width;
        HEIGHT = _height;
        break;
    case 1:
    case 3:
        WIDTH = _height;
        HEIGHT = _width;
        break;
    }
}

uint8_t ST73XX_UI::getRotation(void) const {
    return rotation_;
}

int16_t ST73XX_UI::width() const {
    return WIDTH;
}

int16_t ST73XX_UI::height() const {
    return HEIGHT;
}

void ST73XX_UI::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    for (int16_t i = x; i < x + w; i++) {
        drawFastVLine(i, y, h, color);
    }
} 