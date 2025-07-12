#include "st7305_driver.hpp"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "pico_display_gfx.hpp"
#include "st73xx_font.hpp"
#include "gfx_colors.hpp"
#include "spi_config.hpp"
#include <cstdio>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// 风车动画参数配置
namespace windmill_config {
    // 风车外观配置
    constexpr int NUM_BLADES = 3;         // 叶片数量
    
    // 时间配置（单位：秒）
    constexpr int TOTAL_DURATION = 30;    // 总演示时长（秒）
    constexpr int FPS = 30;              // 每秒帧数
    constexpr int TOTAL_FRAMES = TOTAL_DURATION * FPS; // 总帧数
    constexpr int ACCEL_FRAMES = TOTAL_FRAMES / 2;     // 加速阶段帧数
    constexpr int DECEL_FRAMES = TOTAL_FRAMES / 2;     // 减速阶段帧数
    
    // 转速配置
    constexpr float MAX_RPM = 6000.0f;     // 最高转速（转/分钟）
    constexpr float MIN_RPM = 1000.0f;      // 最低转速（转/分钟）
    
    // 速度配置（单位：毫秒）
    constexpr int MAX_DELAY = static_cast<int>(60000.0f / (MIN_RPM * NUM_BLADES));    // 最慢速度（最大延时）
    constexpr int MIN_DELAY = static_cast<int>(60000.0f / (MAX_RPM * NUM_BLADES));    // 最快速度（最小延时）
    
    // 风车外观配置
    constexpr int BLADE_LENGTH = 60;      // 叶片长度
    constexpr int HUB_RADIUS = 10;        // 中心圆半径
    constexpr int BLADE_WIDTH = 6;        // 叶片宽度
    constexpr int TOTAL_ROTATIONS = 3;    // 总旋转圈数
}

// 文字内容
const char* lines[] = {
    "Satellites whisper, Pixels dance.",
    "Pico brings them both to life.",
    "Tiny circuits hum a cosmic tune,",
    "while LEDs paint the void in bloom.",
    "A microcontroller's quiet might,",
    "turns stardust into blinking light.",
    "Through silicon veins, electrons race,",
    "crafting dreams in this small space.",
    "The universe fits in RAM's embrace,",
    "as Pico charts its stellar chase."
};

const int NUM_LINES = sizeof(lines) / sizeof(lines[0]);

// 自动分割字符串为多行，保证每行不超过max_chars个字符
std::vector<std::string> wrapText(const char* text, int max_chars) {
    std::vector<std::string> lines;
    std::string str(text);
    size_t start = 0;
    while (start < str.length()) {
        size_t len = std::min((size_t)max_chars, str.length() - start);
        lines.push_back(str.substr(start, len));
        start += len;
    }
    return lines;
}

// 用圆弧和直线组合的水滴状叶片，并填充内部为黑色
void drawFanBlade(pico_gfx::PicoDisplayGFX<st7305::ST7305Driver>& gfx, int cx, int cy, float angle, int length, int width, uint16_t color) {
    float root_radius = width * 0.6f; // 根部圆弧半径
    float tip_radius = width * 1.2f;  // 外缘圆弧半径
    float blade_span = M_PI / 2.2;    // 叶片张开角度
    int arc_steps = 24;
    std::vector<std::pair<int, int>> outline;
    // 根部圆弧
    float root_start = angle - blade_span/2;
    float root_end   = angle + blade_span/2;
    for (int i = 0; i <= arc_steps; ++i) {
        float t = (float)i / arc_steps;
        float a = root_start + t * (root_end - root_start);
        int x = cx + root_radius * cos(a);
        int y = cy + root_radius * sin(a);
        outline.push_back({x, y});
    }
    // 外缘圆弧（反向）
    float tip_start = root_end;
    float tip_end   = root_start;
    float tip_cx = cx + length * cos(angle);
    float tip_cy = cy + length * sin(angle);
    for (int i = 0; i <= arc_steps; ++i) {
        float t = (float)i / arc_steps;
        float a = tip_start + t * (tip_end - tip_start);
        int x = tip_cx + tip_radius * cos(a);
        int y = tip_cy + tip_radius * sin(a);
        outline.push_back({x, y});
    }
    // 闭合轮廓
    outline.push_back(outline.front());
    // 填充叶片内部
    // 简单扫描线填充法：遍历outline包围的最小矩形区域
    int min_x = outline[0].first, max_x = outline[0].first;
    int min_y = outline[0].second, max_y = outline[0].second;
    for (const auto& p : outline) {
        if (p.first < min_x) min_x = p.first;
        if (p.first > max_x) max_x = p.first;
        if (p.second < min_y) min_y = p.second;
        if (p.second > max_y) max_y = p.second;
    }
    for (int y = min_y; y <= max_y; ++y) {
        std::vector<int> nodes;
        size_t n = outline.size();
        for (size_t i = 0, j = n - 1; i < n; j = i++) {
            int x0 = outline[i].first, y0 = outline[i].second;
            int x1 = outline[j].first, y1 = outline[j].second;
            if ((y0 < y && y1 >= y) || (y1 < y && y0 >= y)) {
                int x = x0 + (y - y0) * (x1 - x0) / (y1 - y0);
                nodes.push_back(x);
            }
        }
        std::sort(nodes.begin(), nodes.end());
        for (size_t k = 1; k < nodes.size(); k += 2) {
            if (nodes[k-1] < nodes[k]) {
                gfx.drawLine(nodes[k-1], y, nodes[k], y, color);
            }
        }
    }
    // 画轮廓
    for (size_t i = 1; i < outline.size(); ++i) {
        gfx.drawLine(outline[i-1].first, outline[i-1].second, outline[i].first, outline[i].second, color);
    }
}

int main() {
    stdio_init_all();

    st7305::ST7305Driver RF_lcd(PIN_DC, PIN_RST, PIN_CS, PIN_SCLK, PIN_SDIN);
    pico_gfx::PicoDisplayGFX<st7305::ST7305Driver> gfx(RF_lcd, st7305::ST7305Driver::LCD_WIDTH, st7305::ST7305Driver::LCD_HEIGHT);

    printf("Initializing ST7305 display...\n");
    RF_lcd.initialize();
    printf("Display initialized.\n");

    const int rotation = 0;
    gfx.setRotation(rotation);
    RF_lcd.setRotation(rotation);

    // // 演示1：显示诗歌
    // printf("Displaying poem...\n");
    // RF_lcd.clearDisplay();
    
    // // 计算每行最多能显示多少字符
    // int max_chars_per_line = (gfx.width() - 10) / font::FONT_WIDTH; // 减去左右各5像素的边距
    // std::vector<std::string> wrapped_lines;
    // for (int i = 0; i < NUM_LINES; i++) {
    //     std::vector<std::string> temp = wrapText(lines[i], max_chars_per_line);
    //     wrapped_lines.insert(wrapped_lines.end(), temp.begin(), temp.end());
    // }
    // int total_lines = wrapped_lines.size();
    // int line_height = font::FONT_HEIGHT + 2;
    
    // // 计算总显示高度
    // int total_display_height = total_lines * line_height;
    // // 计算起始Y坐标，确保有足够的空间显示所有文字
    // int start_y = 5; // 从顶部5像素开始
    
    // // 左对齐显示每一行
    // for (int i = 0; i < total_lines; i++) {
    //     int x = 5; // 左侧固定5像素边距
    //     int y = start_y + (i * line_height);
    //     // 确保不会超出显示区域
    //     if (y + font::FONT_HEIGHT <= gfx.height() - 5) { // 底部保留5像素边距
    //         RF_lcd.drawString(x, y, wrapped_lines[i].c_str(), BLACK);
    //     }
    // }
    // RF_lcd.display();
    // sleep_ms(5000); // 显示5秒

    // // 演示2：显示棋盘
    // printf("Displaying checkerboard pattern...\n");
    // RF_lcd.clearDisplay();
    // for (uint16_t y = 0; y < st7305::ST7305Driver::LCD_HEIGHT; y++) {
    //     for (uint16_t x = 0; x < st7305::ST7305Driver::LCD_WIDTH; x++) {
    //         bool color = ((x / 8) + (y / 8)) % 2 == 0;
    //         RF_lcd.drawPixel(x, y, color);
    //     }
    // }
    // RF_lcd.display();
    // sleep_ms(2000); // 显示2秒

    // 演示3：动态风车
    printf("Displaying windmill animation...\n");
    RF_lcd.clearDisplay();
    
    // 风车参数
    const int center_x = gfx.width() / 2;
    const int center_y = gfx.height() / 2;
    
    // 动画循环
    float current_angle = 0.0f;
    for (int frame = 0; frame < windmill_config::TOTAL_FRAMES; frame++) {
        RF_lcd.clearDisplay();
        // 匀速加速
        float rpm = windmill_config::MAX_RPM * (float)frame / windmill_config::TOTAL_FRAMES;
        if (rpm < 0) rpm = 0;
        int current_delay = (rpm > 0) ? static_cast<int>(60000.0f / (rpm * windmill_config::NUM_BLADES)) : 20;
        // 显示转速信息
        char rpm_text[32];
        char frame_text[32];
        snprintf(rpm_text, sizeof(rpm_text), "RPM: %.1f/%.1f", rpm, windmill_config::MAX_RPM);
        snprintf(frame_text, sizeof(frame_text), "Frame: %d/%d", frame + 1, windmill_config::TOTAL_FRAMES);
        RF_lcd.drawString(5, 5, rpm_text, BLACK);
        RF_lcd.drawString(5, 5 + font::FONT_HEIGHT + 2, frame_text, BLACK);
        // 计算本帧角度增量
        float delta_angle = rpm * 360.0f * (1.0f / windmill_config::FPS) / 60.0f;
        current_angle += delta_angle;
        // 绘制风车
        gfx.drawFilledCircle(center_x, center_y, windmill_config::HUB_RADIUS, BLACK);
        for (int i = 0; i < windmill_config::NUM_BLADES; i++) {
            float angle = (current_angle + i * (360.0f / windmill_config::NUM_BLADES)) * M_PI / 180.0f;
            drawFanBlade(gfx, center_x, center_y, angle, windmill_config::BLADE_LENGTH, windmill_config::BLADE_WIDTH, BLACK);
        }
        RF_lcd.display();
        sleep_ms(current_delay);
    }
    
    sleep_ms(1000);  // 暂停1秒

        // 结束演示
    printf("Demo end.\n");
    RF_lcd.clearDisplay();
    
    // 显示结束信息
    const char* end_text = "DEMO END.";
    int end_text_len = strlen(end_text);
    int end_x = (gfx.width() - end_text_len * font::FONT_WIDTH) / 2;
    int end_y = (gfx.height() - font::FONT_HEIGHT) / 2;
    RF_lcd.drawString(end_x, end_y, end_text, BLACK);
    RF_lcd.display();
    sleep_ms(2000);
    return 0;
}