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
#include <algorithm>

// 满屏文字显示配置
namespace fullscreen_config {
    constexpr int MARGIN = 5;                    // 屏幕边距
    constexpr int LINE_SPACING = 2;              // 行间距
    constexpr int TOTAL_LINE_HEIGHT = font::FONT_HEIGHT + LINE_SPACING; // 总行高
    
    // 计算最大字符数和行数
    constexpr int MAX_CHARS_PER_LINE = (st7306::ST7306Driver::LCD_WIDTH - 2 * MARGIN) / font::FONT_WIDTH;
    constexpr int MAX_LINES_21 = (st7306::ST7306Driver::LCD_HEIGHT - 2 * MARGIN) / TOTAL_LINE_HEIGHT;
    constexpr int MAX_LINES_22 = 22; // 支持22行显示
    constexpr int TOTAL_CHARS_21 = MAX_CHARS_PER_LINE * MAX_LINES_21;
    constexpr int TOTAL_CHARS_22 = MAX_CHARS_PER_LINE * MAX_LINES_22;
}

// 预定义的满屏文字内容（792个字符，22行）
const char* FULLSCREEN_TEXT[] = {
    // 第1行 (36字符)
    "012345678901234567890123456789012345",
    // 第2行 (36字符)
    "123456789012345678901234567890123456",
    // 第3行 (36字符)
    "234567890123456789012345678901234567",
    // 第4行 (36字符)
    "345678901234567890123456789012345678",
    // 第5行 (36字符)
    "456789012345678901234567890123456789",
    // 第6行 (36字符)
    "567890123456789012345678901234567890",
    // 第7行 (36字符)
    "678901234567890123456789012345678901",
    // 第8行 (36字符)
    "789012345678901234567890123456789012",
    // 第9行 (36字符)
    "890123456789012345678901234567890123",
    // 第10行 (36字符)
    "901234567890123456789012345678901234",
    // 第11行 (36字符)
    "101112131415161718192021222324252627",
    // 第12行 (36字符)
    "112233445566778899001122334455667788",
    // 第13行 (36字符)
    "121314151617181920212223242526272829",
    // 第14行 (36字符)
    "131415161718192021222324252627282930",
    // 第15行 (36字符)
    "141516171819202122232425262728293031",
    // 第16行 (36字符)
    "151617181920212223242526272829303132",
    // 第17行 (36字符)
    "161718192021222324252627282930313233",
    // 第18行 (36字符)
    "171819202122232425262728293031323334",
    // 第19行 (36字符)
    "181920212223242526272829303132333435",
    // 第20行 (36字符)
    "192021222324252627282930313233343536",
    // 第21行 (36字符)
    "202122232425262728293031323334353637",
    // 第22行 (36字符) - 新增行
    "212223242526272829303132333435363738"
};

// 字符计数器
int countTotalCharacters() {
    int total = 0;
    for (int i = 0; i < fullscreen_config::MAX_LINES_22; i++) {
        std::string line(FULLSCREEN_TEXT[i]);
        total += line.length();
    }
    return total;
}

// 显示满屏文字
void displayFullscreenText(st7306::ST7306Driver& lcd) {
    printf("Displaying fullscreen text demo...\n");
    printf("Screen resolution: %dx%d\n", st7306::ST7306Driver::LCD_WIDTH, st7306::ST7306Driver::LCD_HEIGHT);
    printf("Max chars per line: %d\n", fullscreen_config::MAX_CHARS_PER_LINE);
    printf("Testing lines: %d\n", fullscreen_config::MAX_LINES_22);
    printf("Expected total chars (22): %d\n", fullscreen_config::TOTAL_CHARS_22);
    
    // 清屏
    lcd.clearDisplay();
    
    // 显示统计信息
    int actual_chars = countTotalCharacters();
    printf("Actual characters to display: %d\n", actual_chars);
    
    // 逐行显示文字
    for (int line = 0; line < fullscreen_config::MAX_LINES_22; line++) {
        int y = fullscreen_config::MARGIN + line * fullscreen_config::TOTAL_LINE_HEIGHT;
        int x = fullscreen_config::MARGIN;
        
        // 检查是否超出屏幕边界（允许第22行稍微超出一点）
        if (y + font::FONT_HEIGHT <= st7306::ST7306Driver::LCD_HEIGHT) {
            std::string text(FULLSCREEN_TEXT[line]);
            // 截断超出屏幕宽度的文字
            if (text.length() > fullscreen_config::MAX_CHARS_PER_LINE) {
                text = text.substr(0, fullscreen_config::MAX_CHARS_PER_LINE);
            }
            lcd.drawString(x, y, text.c_str(), true); // true表示黑色文字
            printf("Line %2d: %s (%d chars)\n", line + 1, text.c_str(), (int)text.length());
        } else {
            printf("Line %2d: OVERFLOW! Y=%d, Max=%d\n", line + 1, y + font::FONT_HEIGHT, st7306::ST7306Driver::LCD_HEIGHT);
        }
    }
    
    // 刷新显示
    lcd.display();
    
    printf("Fullscreen text display completed!\n");
    printf("Total characters displayed: %d\n", actual_chars);
    printf("Display efficiency: %.1f%%\n", (float)actual_chars / fullscreen_config::TOTAL_CHARS_22 * 100.0f);
}

// 显示字符密度测试
void displayCharacterDensityTest(st7306::ST7306Driver& lcd) {
    printf("\n=== Character Density Test ===\n");
    lcd.clearDisplay();
    
    // 测试1: 显示所有可打印ASCII字符
    printf("Test 1: Displaying all printable ASCII characters\n");
    std::string ascii_chars;
    for (int i = 32; i <= 126; i++) {
        ascii_chars += (char)i;
    }
    
    // 计算能显示多少行ASCII字符
    int chars_per_line = fullscreen_config::MAX_CHARS_PER_LINE;
    int total_ascii_chars = ascii_chars.length();
    int ascii_lines = (total_ascii_chars + chars_per_line - 1) / chars_per_line;
    
    printf("ASCII characters: %d\n", total_ascii_chars);
    printf("Lines needed: %d\n", ascii_lines);
    
    for (int line = 0; line < ascii_lines && line < fullscreen_config::MAX_LINES_22; line++) {
        int y = fullscreen_config::MARGIN + line * fullscreen_config::TOTAL_LINE_HEIGHT;
        int x = fullscreen_config::MARGIN;
        
        std::string line_text = ascii_chars.substr(line * chars_per_line, chars_per_line);
        lcd.drawString(x, y, line_text.c_str(), true);
        printf("ASCII Line %d: %s\n", line + 1, line_text.c_str());
    }
    
    lcd.display();
    sleep_ms(3000);
    
    // 测试2: 显示数字网格
    printf("\nTest 2: Displaying number grid\n");
    lcd.clearDisplay();
    
    for (int line = 0; line < fullscreen_config::MAX_LINES_22; line++) {
        int y = fullscreen_config::MARGIN + line * fullscreen_config::TOTAL_LINE_HEIGHT;
        int x = fullscreen_config::MARGIN;
        
        std::string number_line;
        for (int col = 0; col < fullscreen_config::MAX_CHARS_PER_LINE; col++) {
            number_line += '0' + ((line * fullscreen_config::MAX_CHARS_PER_LINE + col) % 10);
        }
        
        lcd.drawString(x, y, number_line.c_str(), true);
        printf("Number Line %2d: %s\n", line + 1, number_line.c_str());
    }
    
    lcd.display();
    sleep_ms(3000);
}

// 显示灰度文字效果
void displayGrayTextEffect(st7306::ST7306Driver& lcd) {
    printf("\n=== Gray Text Effect Test ===\n");
    lcd.clearDisplay();
    
    // 使用不同灰度级别显示文字
    const char* test_text = "Gray Text Effect Demo";
    int text_width = strlen(test_text) * font::FONT_WIDTH;
    int start_x = (st7306::ST7306Driver::LCD_WIDTH - text_width) / 2;
    
    // 显示标题
    lcd.drawString(start_x, 20, test_text, true);
    
    // 显示灰度说明
    lcd.drawString(10, 60, "Level 0: White", true);
    lcd.drawString(10, 80, "Level 1: Light Gray", true);
    lcd.drawString(10, 100, "Level 2: Dark Gray", true);
    lcd.drawString(10, 120, "Level 3: Black", true);
    
    // 绘制灰度条
    for (int x = 10; x < 290; x++) {
        for (int y = 140; y < 160; y++) {
            uint8_t gray_level = (x - 10) * 3 / 280; // 0-3
            lcd.drawPixelGray(x, y, gray_level);
        }
    }
    
    lcd.display();
    sleep_ms(3000);
}

int main() {
    stdio_init_all();

    st7306::ST7306Driver RF_lcd(PIN_DC, PIN_RST, PIN_CS, PIN_SCLK, PIN_SDIN);
    pico_gfx::PicoDisplayGFX<st7306::ST7306Driver> gfx(RF_lcd, st7306::ST7306Driver::LCD_WIDTH, st7306::ST7306Driver::LCD_HEIGHT);

    printf("Initializing ST7306 display for fullscreen text demo...\n");
    RF_lcd.initialize();
    printf("Display initialized.\n");

    const int rotation = 0;
    gfx.setRotation(rotation);
    RF_lcd.setRotation(rotation);

    // 演示1: 满屏文字显示
    printf("\n=== Demo 1: Fullscreen Text Display ===\n");
    displayFullscreenText(RF_lcd);
    sleep_ms(60000); // 60秒暂停，让用户观察效果

    // 演示2: 字符密度测试
    printf("\n=== Demo 2: Character Density Test ===\n");
    displayCharacterDensityTest(RF_lcd);

    // 演示3: 灰度文字效果
    printf("\n=== Demo 3: Gray Text Effect ===\n");
    displayGrayTextEffect(RF_lcd);

    // 结束演示
    printf("\n=== Demo End ===\n");
    RF_lcd.clearDisplay();
    
    // 显示结束信息
    const char* end_text = "DEMO COMPLETE";
    int end_text_len = strlen(end_text);
    int end_x = (st7306::ST7306Driver::LCD_WIDTH - end_text_len * font::FONT_WIDTH) / 2;
    int end_y = (st7306::ST7306Driver::LCD_HEIGHT - font::FONT_HEIGHT) / 2;
    RF_lcd.drawString(end_x, end_y, end_text, true);
    RF_lcd.display();
    
    printf("Fullscreen text demo completed!\n");
    printf("Total characters demonstrated: %d\n", countTotalCharacters());
    printf("Maximum theoretical capacity (21): %d\n", fullscreen_config::TOTAL_CHARS_21);
    printf("Maximum theoretical capacity (22): %d\n", fullscreen_config::TOTAL_CHARS_22);
    
    sleep_ms(3000);
    return 0;
}
