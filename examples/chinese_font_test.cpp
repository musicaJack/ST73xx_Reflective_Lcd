#include "st7306_driver.hpp"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/uart.h"
#include "pico_display_gfx.hpp"
#include "st73xx_font.hpp"
#include "gfx_colors.hpp"
#include "spi_config.hpp"
#include <cstdio>
#include <cstring>

// 字库数据在Flash中的地址（根据README中的说明）
const uint8_t *font_data = (const uint8_t *)0x10100000;

// 获取字符点阵数据的函数（参考readme_from_fontMaker中的实现）
const uint8_t* get_char_bitmap(uint32_t char_code) {
    // 读取文件头信息
    uint16_t version = *(uint16_t*)font_data;
    uint16_t total_chars = *(uint16_t*)(font_data + 2);
    
    // 计算字符在数据中的偏移
    uint32_t char_offset;
    
    if (char_code >= 0x20 && char_code <= 0x7E) {
        // ASCII字符 (0x20-0x7E, 95个字符，包含空格)
        char_offset = char_code - 0x20;
    } else if (char_code >= 0x3000 && char_code <= 0x303F) {
        // 全角标点符号 (0x3000-0x303F, 64个字符)
        char_offset = 95 + (char_code - 0x3000);
    } else if (char_code >= 0xFF00 && char_code <= 0xFFEF) {
        // 全角字符 (0xFF00-0xFFEF, 240个字符)
        char_offset = 95 + 64 + (char_code - 0xFF00);
    } else if (char_code >= 0x4E00 && char_code <= 0x9FA5) {
        // 中文字符 (0x4E00-0x9FA5, 28,648个字符)
        char_offset = 95 + 64 + 240 + (char_code - 0x4E00);
    } else {
        // 不支持的字符，返回空格
        char_offset = 0; // 空格字符 (0x20)
    }
    
    // 返回字符点阵数据指针
    // 文件头4字节 + 字符偏移 × 32字节
    return font_data + 4 + char_offset * 32;
}

// 在LCD上显示字符的函数（参考readme_from_fontMaker中的实现）
void draw_char_16x16(st7306::ST7306Driver& lcd, int x, int y, uint32_t char_code, bool color) {
    const uint8_t *bitmap = get_char_bitmap(char_code);
    
    for (int row = 0; row < 16; row++) {
        uint16_t row_data = (bitmap[row * 2] << 8) | bitmap[row * 2 + 1];
        
        for (int col = 0; col < 16; col++) {
            if (row_data & (0x8000 >> col)) {
                // 绘制像素点
                lcd.drawPixel(x + col, y + row, color);
            }
        }
    }
}

// 显示字符串（参考readme_from_fontMaker中的实现）
void draw_string_16x16(st7306::ST7306Driver& lcd, int x, int y, const char *str, bool color) {
    int current_x = x;
    
    while (*str) {
        if ((*str & 0x80) == 0) {
            // ASCII字符
            draw_char_16x16(lcd, current_x, y, *str, color);
            current_x += 16;  // ASCII字符宽度为16像素
            str++;
        } else {
            // 中文字符（UTF-8编码）
            uint32_t char_code = 0;
            
            if ((*str & 0xE0) == 0xC0) {
                // 2字节UTF-8
                char_code = ((*str & 0x1F) << 6) | (*(str + 1) & 0x3F);
                str += 2;
            } else if ((*str & 0xF0) == 0xE0) {
                // 3字节UTF-8
                char_code = ((*str & 0x0F) << 12) | 
                           ((*(str + 1) & 0x3F) << 6) | 
                           (*(str + 2) & 0x3F);
                str += 3;
            } else {
                // 不支持的编码，跳过
                str++;
                continue;
            }
            
            draw_char_16x16(lcd, current_x, y, char_code, color);
            current_x += 16;  // 中文字符宽度为16像素
        }
    }
}

// 验证字库文件头
bool verify_font_header() {
    uint16_t version = *(uint16_t*)font_data;
    uint16_t total_chars = *(uint16_t*)(font_data + 2);
    
    printf("Font version: %d\n", version);
    printf("Total characters: %d\n", total_chars);
    
    // 检查版本号和字符数量是否合理（更新后的字库有21,301个字符）
    if (version != 1 || total_chars != 21301) {
        printf("Font header verification failed!\n");
        printf("Expected: version=1, total_chars=21301\n");
        printf("Got: version=%d, total_chars=%d\n", version, total_chars);
        return false;
    }
    
    printf("Font header verification passed!\n");
    return true;
}

// 打印字符点阵（用于调试，参考readme_from_fontMaker中的实现）
void print_char_bitmap(uint32_t char_code) {
    const uint8_t *bitmap = get_char_bitmap(char_code);
    
    printf("Character 0x%04X:\n", char_code);
    for (int row = 0; row < 16; row++) {
        uint16_t row_data = (bitmap[row * 2] << 8) | bitmap[row * 2 + 1];
        
        for (int col = 0; col < 16; col++) {
            printf("%c", (row_data & (0x8000 >> col)) ? '#' : '.');
        }
        printf("\n");
    }
    printf("\n");
}

// 测试不同字符类型
void test_character_types(st7306::ST7306Driver& lcd) {
    printf("Testing different character types...\n");
    
    // 测试ASCII字符
    printf("Testing ASCII characters...\n");
    lcd.clear();
    draw_string_16x16(lcd, 10, 10, "ABCDEFGHIJK", true);
    lcd.display();
    sleep_ms(2000);
    
    // 测试数字
    printf("Testing numbers...\n");
    lcd.clear();
    draw_string_16x16(lcd, 10, 10, "0123456789", true);
    lcd.display();
    sleep_ms(2000);
    
    // 测试符号
    printf("Testing symbols...\n");
    lcd.clear();
    draw_string_16x16(lcd, 10, 10, "!@#$%^&*()", true);
    lcd.display();
    sleep_ms(2000);
    
    // 测试中文字符
    printf("Testing Chinese characters...\n");
    lcd.clear();
    draw_string_16x16(lcd, 10, 10, "中文字符测试", true);
    lcd.display();
    sleep_ms(2000);
    
    // 测试混合字符
    printf("Testing mixed characters...\n");
    lcd.clear();
    draw_string_16x16(lcd, 1, 1, "英文: English", true);
    draw_string_16x16(lcd, 1, 20, "中文: 中文字符测试", true);
    draw_string_16x16(lcd, 1, 40, "字母: A,B,C", true);
    draw_string_16x16(lcd, 1, 60, "符号: $#@!%^&*", true);
    draw_string_16x16(lcd, 1, 80, "数字: 1,2,3", true);
    lcd.display();
    sleep_ms(10000);  
    lcd.clear();

    
}

int main() {
    // 初始化所有标准库功能，包括UART
    stdio_init_all();
    
    // 等待UART稳定
    sleep_ms(1000);
    
    printf("\n\n=== Chinese Font Test Program Starting ===\n");
    printf("UART initialized successfully\n");
    
    // 验证字库文件头
    printf("Verifying font header...\n");
    if (!verify_font_header()) {
        printf("Font verification failed! Please check if font16.bin is properly loaded.\n");
        printf("Make sure font16.bin is loaded to Flash address 0x10100000\n");
        while (true) {
            sleep_ms(1000);
            printf("Waiting for font file to be loaded...\n");
        }
        return -1;
    }
    
    // 初始化ST7306显示模块（参考st7306_demo.cpp的方式）
    printf("Initializing ST7306 display...\n");
    st7306::ST7306Driver lcd(PIN_DC, PIN_RST, PIN_CS, PIN_SCLK, PIN_SDIN);
    lcd.initialize();
    printf("Display initialized successfully.\n");
    
    // 设置显示模式（参考text_reader.cpp的方式）
    lcd.setDisplayMode(st7306::DisplayMode::Day);
    lcd.displayOn(true);
    printf("Display mode set to Day mode.\n");
    
    // 清屏
    lcd.clear();
    lcd.display();
    printf("Display cleared.\n");
    sleep_ms(1000);
    
    // 测试不同字符类型
    test_character_types(lcd);
    
    // 显示字符点阵调试输出
    printf("Character bitmap debug output:\n");
    print_char_bitmap('A');
    print_char_bitmap('1');
    print_char_bitmap(0x4E2D);  // "中"
    print_char_bitmap(0x6587);  // "文"
    print_char_bitmap(0x3002);  // "。"
    print_char_bitmap(0xFF01);  // "！"
    
    // 最终显示
    printf("Test completed!\n");
    //lcd.clear();
    //draw_string_16x16(lcd, 10, 50, "测试完成", true);
    //lcd.display();
    
    printf("Chinese font test program completed successfully!\n");
    printf("Program will continue running...\n");
    
    // 保持显示
    while (true) {
        sleep_ms(1000);
        printf("Program running...\n");
    }
    
    return 0;
} 