#include "st7306_driver.hpp"
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
    constexpr int TOTAL_DURATION = 60;    // 总演示时长（秒）
    constexpr int FPS = 30;              // 每秒帧数
    constexpr int TOTAL_FRAMES = TOTAL_DURATION * FPS; // 总帧数
    constexpr int ACCEL_FRAMES = TOTAL_FRAMES / 2;     // 加速阶段帧数
    constexpr int DECEL_FRAMES = TOTAL_FRAMES / 2;     // 减速阶段帧数
    
    // 转速配置
    constexpr float MAX_RPM = 2000.0f;     // 最高转速（转/分钟）
    constexpr float MIN_RPM = 1000.0f;      // 最低转速（转/分钟）
    
    // 速度配置（单位：毫秒）
    constexpr int MAX_DELAY = static_cast<int>(60000.0f / (MIN_RPM * NUM_BLADES));    // 最慢速度（最大延时）
    constexpr int MIN_DELAY = static_cast<int>(60000.0f / (MAX_RPM * NUM_BLADES));    // 最快速度（最小延时）
    
    // 风车外观配置
    constexpr int BLADE_LENGTH = 100;      // 叶片长度（根据4.2寸屏幕调整）
    constexpr int HUB_RADIUS = 15;         // 中心圆半径
    constexpr int BLADE_WIDTH = 8;         // 叶片宽度
    constexpr int TOTAL_ROTATIONS = 3;     // 总旋转圈数
}

// 用圆弧和直线组合的水滴状叶片，并填充内部为黑色
void drawFanBlade(pico_gfx::PicoDisplayGFX<st7306::ST7306Driver>& gfx, int cx, int cy, float angle, int length, int width, uint16_t color) {
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

    st7306::ST7306Driver RF_lcd(PIN_DC, PIN_RST, PIN_CS, PIN_SCLK, PIN_SDIN);
    pico_gfx::PicoDisplayGFX<st7306::ST7306Driver> gfx(RF_lcd, st7306::ST7306Driver::LCD_WIDTH, st7306::ST7306Driver::LCD_HEIGHT);

    printf("Initializing ST7306 display...\n");
    RF_lcd.initialize();
    printf("Display initialized.\n");

    const int rotation = 0;
    gfx.setRotation(rotation);
    RF_lcd.setRotation(rotation);
   
      
    // 简单灰度条纹测试，只测试一小部分
    printf("Testing grayscale...\n");
    RF_lcd.clearDisplay();
    
    // Windows 95风格进度条
    const int bar_width = RF_lcd.LCD_WIDTH * 0.7;  // 进度条总宽度
    const int bar_height = 20;                     // 进度条高度
    const int bar_x = (RF_lcd.LCD_WIDTH - bar_width) / 2;  // 居中显示
    const int bar_y = (RF_lcd.LCD_HEIGHT * 0.7);           // 位于屏幕下方
    const int steps = 100;                         // 进度步骤数
    const int pattern_width = 64;                  // 条纹渐变周期总宽度
    int offset = 0;                                // 动画偏移量
    
    // 显示标题文本
    RF_lcd.drawString((RF_lcd.LCD_WIDTH - 19*8)/2, RF_lcd.LCD_HEIGHT/3, "System Starting Up...", true);
    
    // 手动绘制外框 (使用drawPixelGray替代drawRect)
    for (int x = bar_x; x < bar_x + bar_width; x++) {
        // 绘制顶部和底部边框
        RF_lcd.drawPixelGray(x, bar_y, 3);
        RF_lcd.drawPixelGray(x, bar_y + bar_height - 1, 3);
    }
    for (int y = bar_y; y < bar_y + bar_height; y++) {
        // 绘制左侧和右侧边框
        RF_lcd.drawPixelGray(bar_x, y, 3);
        RF_lcd.drawPixelGray(bar_x + bar_width - 1, y, 3);
    }
    
    RF_lcd.display();
    sleep_ms(500);  // 显示空进度条一段时间
    
    // 进度条动画 - 从左到右填充，内部条纹滚动
    for (int step = 0; step <= steps; step++) {
        // 更新动画偏移量（逐渐向左移动）
        offset = (offset + 2) % pattern_width;
        
        // 计算当前填充宽度
        int fill_width = (step * (bar_width - 6)) / steps;
        
        // 先清除整个进度条内部区域
        for (int y = bar_y + 3; y < bar_y + bar_height - 3; y++) {
            for (int x = bar_x + 3; x < bar_x + bar_width - 3; x++) {
                RF_lcd.drawPixelGray(x, y, 0); // 背景用黑色
            }
        }
        
        // 绘制已完成的进度部分（带有滚动灰度条纹，使用抖动技术）
        for (int y = bar_y + 3; y < bar_y + bar_height - 3; y++) {
            for (int x = bar_x + 3; x < bar_x + 3 + fill_width; x++) {
                // 使用余弦函数生成更平滑的渐变
                float position = (float)((x + offset) % pattern_width) / pattern_width;
                // 生成一个0到1的平滑波浪值
                float wave = (1.0f + cosf(position * 2.0f * M_PI)) / 2.0f;
                
                // 获取主灰度级别 (0-3)
                uint8_t base_level = (uint8_t)(wave * 3.0f);
                
                // 获取误差，用于抖动
                float error = (wave * 3.0f) - base_level;
                
                // 应用抖动模式 - 使用2x2的Bayer矩阵
                int bayer_x = x % 2;
                int bayer_y = y % 2;
                int bayer_index = bayer_y * 2 + bayer_x;
                
                // 阈值矩阵
                const float bayer_threshold[4] = {
                    0.0f,  0.5f,
                    0.75f, 0.25f
                };
                
                // 根据抖动阈值决定是否提升灰度级别
                uint8_t gray_level = base_level;
                if (error > bayer_threshold[bayer_index] && base_level < 3) {
                    gray_level = base_level + 1;
                }
                
                RF_lcd.drawPixelGray(x, y, gray_level);
            }
        }
        
        // 显示百分比
        char progress_text[16];
        snprintf(progress_text, sizeof(progress_text), "%d%%", step);
        
        // 清除旧文字区域
        for (int y = bar_y; y < bar_y + 20; y++) {
            for (int x = bar_x + bar_width + 5; x < bar_x + bar_width + 45; x++) {
                RF_lcd.drawPixelGray(x, y, 0); // 用黑色清除
            }
        }
        
        RF_lcd.drawString(bar_x + bar_width + 5, bar_y, progress_text, true);
        
        RF_lcd.display();
        sleep_ms(50);  // 控制进度速度
    }
    
    // 进度到100%后暂停1秒
    sleep_ms(1000);
    
    // 显示完成信息
    RF_lcd.clearDisplay();
    RF_lcd.drawString((RF_lcd.LCD_WIDTH - 14*8)/2, RF_lcd.LCD_HEIGHT/2 - 4, "Loading Complete", true);
    RF_lcd.display();
    sleep_ms(2000);
    
    // 演示：动态风车
    printf("Displaying windmill animation...\n");
    RF_lcd.clearDisplay();
    
    // 风车参数
    const int center_x = gfx.width() / 2;
    const int center_y = gfx.height() / 2;
    
    // 动画循环
    float current_angle = 0.0f;
    for (int frame = 0; frame < windmill_config::TOTAL_FRAMES / 5; frame++) { // 减少帧数以缩短演示时间
        RF_lcd.clearDisplay();
        // 匀速加速
        float rpm = windmill_config::MAX_RPM * (float)frame / (windmill_config::TOTAL_FRAMES / 5);
        if (rpm < 0) rpm = 0;
        int current_delay = (rpm > 0) ? static_cast<int>(60000.0f / (rpm * windmill_config::NUM_BLADES)) : 20;
        // 显示转速信息
        char rpm_text[32];
        char frame_text[32];
        snprintf(rpm_text, sizeof(rpm_text), "RPM: %.1f/%.1f", rpm, windmill_config::MAX_RPM);
        snprintf(frame_text, sizeof(frame_text), "Frame: %d/%d", frame + 1, windmill_config::TOTAL_FRAMES / 5);
        RF_lcd.drawString(5, 5, rpm_text, true);
        RF_lcd.drawString(5, 5 + font::FONT_HEIGHT + 2, frame_text, true);
        // 计算本帧角度增量
        float delta_angle = rpm * 360.0f * (1.0f / windmill_config::FPS) / 60.0f;
        current_angle += delta_angle;
        // 绘制风车
        gfx.drawFilledCircle(center_x, center_y, windmill_config::HUB_RADIUS, true);
        for (int i = 0; i < windmill_config::NUM_BLADES; i++) {
            float angle = (current_angle + i * (360.0f / windmill_config::NUM_BLADES)) * M_PI / 180.0f;
            drawFanBlade(gfx, center_x, center_y, angle, windmill_config::BLADE_LENGTH, windmill_config::BLADE_WIDTH, true);
        }
        RF_lcd.display();
        sleep_ms(current_delay);
    }
    
    sleep_ms(1000);  // 暂停1秒
    
    // 结束测试
    printf("Finishing tests...\n");
    RF_lcd.clearDisplay();
    RF_lcd.drawString((RF_lcd.LCD_WIDTH - 9*8)/2, RF_lcd.LCD_HEIGHT/2 - 4, "DEMO END", true);
    RF_lcd.display();
    sleep_ms(3000);
    printf("Demo complete\n");
    
    return 0;
} 