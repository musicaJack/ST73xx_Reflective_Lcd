#include "st7306_driver.hpp"
#include "st73xx_font_cn.hpp"
#include "st73xx_font.hpp"  // 添加内置ASCII字体支持
#include "spi_config.hpp"
#include "pico/stdlib.h"
#include <cstdio>

// 字库数据在Flash中的地址
const uint8_t *font_data = (const uint8_t *)st73xx_font_cn::DEFAULT_FONT_ADDRESS;

// ==================== 混合字体渲染器 ====================
class HybridFontRenderer {
private:
    st73xx_font_cn::FontManager<st7306::ST7306Driver> chinese_font_mgr_;
    bool chinese_font_initialized_;
    
public:
    HybridFontRenderer() : chinese_font_initialized_(false) {}
    
    // 初始化字体
    bool initialize(const uint8_t* chinese_font_data) {
        chinese_font_initialized_ = chinese_font_mgr_.initialize(chinese_font_data);
        return chinese_font_initialized_;
    }
    
    // 判断字符是否为ASCII
    static bool is_ascii(uint32_t char_code) {
        return char_code <= 0x7F;
    }
    
    // 判断字符是否为ASCII（字符串版本）
    static bool is_ascii_char(char c) {
        return static_cast<unsigned char>(c) <= 0x7F;
    }
    
    // 绘制单个字符（智能选择字体）
    void draw_char(st7306::ST7306Driver& display, int x, int y, uint32_t char_code, bool color) {
        if (is_ascii(char_code)) {
            // 使用内置ASCII字体
            draw_ascii_char(display, x, y, static_cast<char>(char_code), color);
        } else {
            // 使用中文字体
            if (chinese_font_initialized_) {
                chinese_font_mgr_.draw_char(display, x, y, char_code, color);
            }
        }
    }
    
    // 绘制ASCII字符（使用内置字体）
    void draw_ascii_char(st7306::ST7306Driver& display, int x, int y, char c, bool color) {
        const uint8_t* char_data = font::get_char_data(c);
        for (int row = 0; row < font::FONT_HEIGHT; ++row) {
            uint8_t row_data = char_data[row];
            for (int col = 0; col < font::FONT_WIDTH; ++col) {
                if (row_data & (0x80 >> col)) {
                    display.drawPixel(x + col, y + row, color);
                }
            }
        }
    }
    
    // 绘制字符串（智能选择字体）
    void draw_string(st7306::ST7306Driver& display, int x, int y, const char* str, bool color) {
        int current_x = x;
        const char* ptr = str;
        
        while (*ptr) {
            if ((*ptr & 0x80) == 0) {
                // ASCII字符
                draw_ascii_char(display, current_x, y, *ptr, color);
                current_x += font::FONT_WIDTH;
                ptr++;
            } else {
                // UTF-8字符，需要解码
                uint32_t char_code = decode_utf8(ptr);
                if (char_code != 0) {
                    if (is_ascii(char_code)) {
                        // 解码后的ASCII字符
                        draw_ascii_char(display, current_x, y, static_cast<char>(char_code), color);
                        current_x += font::FONT_WIDTH;
                    } else {
                        // 中文字符
                        if (chinese_font_initialized_) {
                            chinese_font_mgr_.draw_char(display, current_x, y, char_code, color);
                            current_x += 16; // 中文字符宽度
                        }
                    }
                }
            }
        }
    }
    
    // UTF-8解码
    static uint32_t decode_utf8(const char*& str) {
        unsigned char c = static_cast<unsigned char>(*str);
        if (c < 0x80) {
            // 单字节ASCII
            str++;
            return c;
        } else if ((c & 0xE0) == 0xC0) {
            // 2字节UTF-8
            if (str[1] == 0) return 0;
            uint32_t code = ((c & 0x1F) << 6) | (str[1] & 0x3F);
            str += 2;
            return code;
        } else if ((c & 0xF0) == 0xE0) {
            // 3字节UTF-8
            if (str[1] == 0 || str[2] == 0) return 0;
            uint32_t code = ((c & 0x0F) << 12) | ((str[1] & 0x3F) << 6) | (str[2] & 0x3F);
            str += 3;
            return code;
        } else if ((c & 0xF8) == 0xF0) {
            // 4字节UTF-8
            if (str[1] == 0 || str[2] == 0 || str[3] == 0) return 0;
            uint32_t code = ((c & 0x07) << 18) | ((str[1] & 0x3F) << 12) | 
                           ((str[2] & 0x3F) << 6) | (str[3] & 0x3F);
            str += 4;
            return code;
        } else {
            // 无效UTF-8
            str++;
            return 0;
        }
    }
    
    // 获取字符串宽度
    int get_string_width(const char* str) {
        int width = 0;
        const char* ptr = str;
        
        while (*ptr) {
            if ((*ptr & 0x80) == 0) {
                // ASCII字符
                width += font::FONT_WIDTH;
                ptr++;
            } else {
                // UTF-8字符
                uint32_t char_code = decode_utf8(ptr);
                if (char_code != 0) {
                    if (is_ascii(char_code)) {
                        width += font::FONT_WIDTH;
                    } else {
                        width += 16; // 中文字符宽度
                    }
                }
            }
        }
        return width;
    }
    
    // 验证字体
    bool verify_font() const {
        return chinese_font_initialized_ && chinese_font_mgr_.verify_font();
    }
    
    // 获取字体信息
    uint16_t get_font_version() const {
        return chinese_font_mgr_.get_font_version();
    }
    
    uint16_t get_total_chars() const {
        return chinese_font_mgr_.get_total_chars();
    }
};

// ==================== 基本使用示例 ====================
void example_basic_usage(st7306::ST7306Driver& lcd) {
    printf("=== 基本使用示例 ===\n");
    
    // 1. 创建混合字体渲染器
    HybridFontRenderer font_renderer;
    
    // 2. 初始化字体数据
    if (!font_renderer.initialize(font_data)) {
        printf("字体初始化失败!\n");
        return;
    }
    
    // 3. 绘制字符串（混合字体）
    lcd.clear();
    font_renderer.draw_string(lcd, 10, 10, "Hello World", true);
    font_renderer.draw_string(lcd, 10, 30, "你好世界", true);
    font_renderer.draw_string(lcd, 10, 50, "ABC123!@#", true);
    font_renderer.draw_string(lcd, 10, 70, "中英文混合", true);
    lcd.display();
    
    printf("基本使用示例完成。\n");
    sleep_ms(3000);
}

// ==================== 高级使用示例 ====================
void example_advanced_usage(st7306::ST7306Driver& lcd) {
    printf("=== 高级使用示例 ===\n");
    
    HybridFontRenderer font_renderer;
    font_renderer.initialize(font_data);
    
    // 绘制单个字符
    lcd.clear();
    font_renderer.draw_char(lcd, 10, 10, 'A', true);        // ASCII
    font_renderer.draw_char(lcd, 30, 10, '1', true);        // ASCII
    font_renderer.draw_char(lcd, 50, 10, 0x4E2D, true);     // "中"
    font_renderer.draw_char(lcd, 70, 10, 0x6587, true);     // "文"
    font_renderer.draw_char(lcd, 90, 10, '!', true);        // ASCII
    lcd.display();
    
    printf("高级使用示例完成。\n");
    sleep_ms(3000);
}

// ==================== 字体信息示例 ====================
void example_font_info() {
    printf("=== 字体信息示例 ===\n");
    
    HybridFontRenderer font_renderer;
    font_renderer.initialize(font_data);
    
    // 获取字体信息
    printf("字体版本: %d\n", font_renderer.get_font_version());
    printf("字符总数: %d\n", font_renderer.get_total_chars());
    printf("字体验证: %s\n", font_renderer.verify_font() ? "通过" : "失败");
    
    printf("字体信息示例完成。\n");
}

// ==================== 混合内容示例 ====================
void example_mixed_content(st7306::ST7306Driver& lcd) {
    printf("=== 混合内容示例 ===\n");
    
    HybridFontRenderer font_renderer;
    font_renderer.initialize(font_data);
    
    // 显示混合内容
    lcd.clear();
    
    // 多行文本
    font_renderer.draw_string(lcd, 1, 1, "Temperature: 25°C", true);
    font_renderer.draw_string(lcd, 1, 20, "温度: 25°C", true);
    font_renderer.draw_string(lcd, 1, 40, "Status: OK", true);
    font_renderer.draw_string(lcd, 1, 60, "状态: 正常", true);
    font_renderer.draw_string(lcd, 1, 80, "Time: 12:34", true);
    font_renderer.draw_string(lcd, 1, 100, "时间: 12:34", true);
    font_renderer.draw_string(lcd, 1, 120, "System: Ready", true);
    font_renderer.draw_string(lcd, 1, 140, "系统: 就绪", true);
    
    lcd.display();
    
    printf("混合内容示例完成。\n");
    sleep_ms(5000);
}

// ==================== 错误处理示例 ====================
void example_error_handling() {
    printf("=== 错误处理示例 ===\n");
    
    // 1. 处理字体初始化失败
    HybridFontRenderer font_renderer;
    
    // 尝试使用无效的字体数据
    const uint8_t* invalid_font_data = nullptr;
    if (!font_renderer.initialize(invalid_font_data)) {
        printf("字体初始化失败（预期结果）。\n");
    }
    
    // 2. 处理显示驱动错误
    st7306::ST7306Driver lcd(PIN_DC, PIN_RST, PIN_CS, PIN_SCLK, PIN_SDIN);
    lcd.initialize();
    
    // 即使字体未初始化，绘制函数也不会崩溃
    font_renderer.draw_string(lcd, 10, 10, "Test", true);
    printf("绘制函数优雅地处理了缺失的字体。\n");
    
    printf("错误处理示例完成。\n");
}

// ==================== 字符类型测试 ====================
void test_character_types(st7306::ST7306Driver& lcd) {
    printf("=== 字符类型测试 ===\n");
    
    HybridFontRenderer font_renderer;
    font_renderer.initialize(font_data);
    
    // 测试ASCII字符
    printf("测试ASCII字符...\n");
    lcd.clear();
    font_renderer.draw_string(lcd, 10, 10, "ABCDEFGHIJK", true);
    lcd.display();
    sleep_ms(2000);
    
    // 测试数字
    printf("测试数字...\n");
    lcd.clear();
    font_renderer.draw_string(lcd, 10, 10, "0123456789", true);
    lcd.display();
    sleep_ms(2000);
    
    // 测试符号
    printf("测试符号...\n");
    lcd.clear();
    font_renderer.draw_string(lcd, 10, 10, "!@#$%^&*()", true);
    lcd.display();
    sleep_ms(2000);
    
    // 测试中文字符
    printf("测试中文字符...\n");
    lcd.clear();
    font_renderer.draw_string(lcd, 10, 10, "中文字符测试", true);
    lcd.display();
    sleep_ms(2000);
    
    // 测试混合字符
    printf("测试混合字符...\n");
    lcd.clear();
    font_renderer.draw_string(lcd, 1, 1, "英文: English", true);
    font_renderer.draw_string(lcd, 1, 20, "中文: 中文字符测试", true);
    font_renderer.draw_string(lcd, 1, 40, "字母: A,B,C", true);
    font_renderer.draw_string(lcd, 1, 60, "符号: $#@!%^&*", true);
    font_renderer.draw_string(lcd, 1, 80, "数字: 1,2,3", true);
    lcd.display();
    sleep_ms(5000);
    lcd.clear();
    
    printf("字符类型测试完成。\n");
}

// ==================== 字体对比测试 ====================
void test_font_comparison(st7306::ST7306Driver& lcd) {
    printf("=== 字体对比测试 ===\n");
    
    HybridFontRenderer hybrid_renderer;
    hybrid_renderer.initialize(font_data);
    
    // 对比ASCII字符渲染效果
    printf("对比ASCII字符渲染效果...\n");
    lcd.clear();
    
    // 使用混合渲染器（ASCII用内置字体）
    hybrid_renderer.draw_string(lcd, 10, 10, "ASCII Test: ABC123", true);
    hybrid_renderer.draw_string(lcd, 10, 30, "Mixed: Hello世界", true);
    hybrid_renderer.draw_string(lcd, 10, 50, "Symbols: !@#$%^&*", true);
    hybrid_renderer.draw_string(lcd, 10, 70, "Numbers: 0123456789", true);
    
    lcd.display();
    sleep_ms(5000);
    
    printf("字体对比测试完成。\n");
}

// ==================== 主函数 ====================
int main() {
    // 初始化所有标准库功能，包括UART
    stdio_init_all();
    
    // 等待UART稳定
    sleep_ms(1000);
    
    printf("\n\n=== ST73xx 混合字体渲染 API 综合测试程序 ===\n");
    printf("UART初始化成功\n");
    
    // 创建混合字体渲染器
    HybridFontRenderer font_renderer;
    
    // 验证字体
    printf("验证字体...\n");
    if (!font_renderer.initialize(font_data)) {
        printf("字体初始化失败! 请检查font16.bin是否正确加载。\n");
        printf("确保font16.bin已加载到Flash地址 0x%08X\n", st73xx_font_cn::DEFAULT_FONT_ADDRESS);
        while (true) {
            sleep_ms(1000);
            printf("等待字体文件加载...\n");
        }
        return -1;
    }
    
    // 打印字体信息
    printf("字体初始化成功!\n");
    printf("字体验证: %s\n", font_renderer.verify_font() ? "通过" : "失败");
    printf("字体版本: %d\n", font_renderer.get_font_version());
    printf("字符总数: %d\n", font_renderer.get_total_chars());
    
    // 初始化ST7306显示模块
    printf("初始化ST7306显示...\n");
    st7306::ST7306Driver lcd(PIN_DC, PIN_RST, PIN_CS, PIN_SCLK, PIN_SDIN);
    lcd.initialize();
    printf("显示初始化成功。\n");
    
    // 设置显示模式
    lcd.setDisplayMode(st7306::DisplayMode::Day);
    lcd.displayOn(true);
    printf("显示模式设置为日间模式。\n");
    
    // 清屏
    lcd.clear();
    lcd.display();
    printf("显示已清屏。\n");
    sleep_ms(1000);
    
    // 运行各种示例
    printf("\n开始运行示例程序...\n");
    
    // 1. 基本使用示例
    example_basic_usage(lcd);
    
    // 2. 高级使用示例
    example_advanced_usage(lcd);
    
    // 3. 字体信息示例
    example_font_info();
    
    // 4. 混合内容示例
    example_mixed_content(lcd);
    
    // 5. 错误处理示例
    example_error_handling();
    
    // 6. 字符类型测试
    test_character_types(lcd);
    
    // 7. 字体对比测试
    test_font_comparison(lcd);
    
    // 最终显示
    printf("测试完成!\n");
    lcd.clear();
    font_renderer.draw_string(lcd, 10, 50, "测试完成", true);
    font_renderer.draw_string(lcd, 10, 70, "Test Complete", true);
    font_renderer.draw_string(lcd, 10, 90, "混合字体渲染成功", true);
    lcd.display();
    
    printf("混合字体测试程序成功完成!\n");
    printf("程序将继续运行...\n");
    
    // 保持显示
    while (true) {
        sleep_ms(1000);
        printf("程序运行中...\n");
    }
    
    return 0;
} 