// font_verifier.cpp - 字体验证工具
// 用于验证烧录到Pico W Flash中的font16.bin字库文件
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"

// Flash基址 - 可通过CMake定义覆盖
#ifndef FLASH_BASE_ADDR
#define FLASH_BASE_ADDR 0x10100000
#endif

// 串口配置
#define UART_ID uart0
#define BAUD_RATE 115200
#define UART_TX_PIN 0
#define UART_RX_PIN 1

// 调试输出函数
static void debug_print(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    fflush(stdout);
    
    // 同时输出到串口
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    uart_puts(UART_ID, buffer);
}

// 简单的串口输出函数
static void uart_print(const char* str) {
    uart_puts(UART_ID, str);
}

// 初始化串口
void init_uart() {
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    
    // 等待串口稳定
    sleep_ms(100);
    
    uart_print("\r\n\r\n");
    uart_print("========================================\r\n");
    uart_print("字体验证工具启动\r\n");
    uart_print("========================================\r\n");
}

// 验证字体文件头
bool verify_font_header(const uint8_t* font_data) {
    uint16_t version = *(uint16_t*)font_data;
    uint16_t total_chars = *(uint16_t*)(font_data + 2);
    
    debug_print("[INFO] 字体文件头验证:\r\n");
    debug_print("  版本号: %d\r\n", version);
    debug_print("  字符总数: %d\r\n", total_chars);
    
    // 验证版本号
    if (version != 1) {
        debug_print("[ERROR] 不支持的字体版本: %d\r\n", version);
        return false;
    }
    
    // 验证字符数量（ASCII 95 + 全角标点 64 + 全角字符 240 + 中文 20902 = 21301）
    uint32_t expected_chars = 95 + 64 + 240 + 20902;
    if (total_chars != expected_chars) {
        debug_print("[ERROR] 字符数量不正确: %d (期望: %d)\r\n", total_chars, expected_chars);
        debug_print("[INFO] 字符分布: ASCII=%d, 全角标点=%d, 全角字符=%d, 中文=%d\r\n", 
                   95, 64, 240, 20902);
        return false;
    }
    
    debug_print("[SUCCESS] 字体文件头验证通过\r\n");
    debug_print("[INFO] 字符分布: ASCII=%d, 全角标点=%d, 全角字符=%d, 中文=%d\r\n", 
               95, 64, 240, 20902);
    return true;
}

// 获取字符点阵数据的函数
const uint8_t* get_char_bitmap(const uint8_t* font_data, uint32_t char_code) {
    // 计算字符在数据中的偏移
    uint32_t char_offset;
    
    if (char_code >= 0x20 && char_code <= 0x7E) {
        // ASCII字符 (0x20-0x7E) - 包含空格
        char_offset = char_code - 0x20;
        debug_print("[DEBUG] ASCII字符 0x%02X ('%c') 偏移: %d\r\n", char_code, (char)char_code, char_offset);
    } else if (char_code >= 0x3000 && char_code <= 0x303F) {
        // 全角标点符号 (0x3000-0x303F)
        char_offset = 95 + (char_code - 0x3000);
        debug_print("[DEBUG] 全角标点 0x%04X 偏移: %d\r\n", char_code, char_offset);
    } else if (char_code >= 0xFF00 && char_code <= 0xFFEF) {
        // 全角字符 (0xFF00-0xFFEF)
        char_offset = 95 + 64 + (char_code - 0xFF00);
        debug_print("[DEBUG] 全角字符 0x%04X 偏移: %d\r\n", char_code, char_offset);
    } else if (char_code >= 0x4E00 && char_code <= 0x9FA5) {
        // 中文字符 (0x4E00-0x9FA5)
        char_offset = 95 + 64 + 240 + (char_code - 0x4E00);
        debug_print("[DEBUG] 中文字符 0x%04X 偏移: %d\r\n", char_code, char_offset);
    } else {
        // 不支持的字符，返回空格
        char_offset = 0; // 空格字符
        debug_print("[ERROR] 不支持的字符编码: 0x%04X，使用空格代替\r\n", char_code);
    }
    
    // 返回字符点阵数据指针
    // 文件头4字节 + 字符偏移 × 32字节
    return font_data + 4 + char_offset * 32;
}

// 显示字符点阵数据
void display_char_bitmap(const uint8_t* font_data, uint32_t char_code) {
    // 使用统一的字符偏移计算函数
    const uint8_t* bitmap = get_char_bitmap(font_data, char_code);
    
    // 显示点阵
    debug_print("字符 0x%04X 点阵数据:\r\n", char_code);
    for (int row = 0; row < 16; row++) {
        uint16_t row_data = (bitmap[row * 2] << 8) | bitmap[row * 2 + 1];
        debug_print("  %02d: ", row);
        
        for (int col = 0; col < 16; col++) {
            char pixel = (row_data & (0x8000 >> col)) ? '#' : '.';
            printf("%c", pixel);
            uart_putc(UART_ID, pixel);
        }
        printf(" (0x%04X)\r\n", row_data);
        uart_print(" (0x");
        char hex[5];
        snprintf(hex, sizeof(hex), "%04X", row_data);
        uart_print(hex);
        uart_print(")\r\n");
    }
    uart_print("\r\n");
}

int main() {
    // 初始化标准输入输出
    stdio_init_all();
    
    // 初始化串口
    init_uart();
    
    // 等待串口枚举
    debug_print("[INFO] 等待串口枚举...\r\n");
    sleep_ms(3000);
    
    debug_print("[INFO] 字体验证工具启动\r\n");
    debug_print("[INFO] Flash基址: 0x%08X\r\n", FLASH_BASE_ADDR);
    
    // 获取字体数据指针
    const uint8_t* font_data = (const uint8_t*)(FLASH_BASE_ADDR);
    
    // 检查内存访问是否正常
    debug_print("[DEBUG] 检查内存访问...\r\n");
    debug_print("[DEBUG] 前4字节: %02X %02X %02X %02X\r\n", 
               font_data[0], font_data[1], font_data[2], font_data[3]);
    
    // 验证字体文件头
    if (!verify_font_header(font_data)) {
        debug_print("[ERROR] 字体文件头验证失败\r\n");
        debug_print("[ERROR] 请确保font16.bin已正确烧录到Flash地址0x10100000\r\n");
        while (1) { 
            sleep_ms(1000); 
            uart_print("[ERROR] 字体文件头验证失败，请检查烧录\r\n");
        }
    }
    
    // 显示前100行原始数据
    debug_print("[DEBUG] 字体数据前10行 (每行16字节):\r\n");
    for (int row = 0; row < 10; ++row) {
        debug_print("[%02d] ", row);
        for (int col = 0; col < 16; ++col) {
            int idx = row * 16 + col;
            printf("%02X ", font_data[idx]);
            char hex[4];
            snprintf(hex, sizeof(hex), "%02X ", font_data[idx]);
            uart_print(hex);
        }
        printf("\r\n");
        uart_print("\r\n");
    }
    uart_print("\r\n");
    
    // 测试显示一些字符
    debug_print("[INFO] 测试字符显示:\r\n");
    
    // ASCII字符测试
    display_char_bitmap(font_data, ' ');  // 空格
    display_char_bitmap(font_data, 'A');
    display_char_bitmap(font_data, '1');
    display_char_bitmap(font_data, '!');
    
    // 全角标点测试
    display_char_bitmap(font_data, 0x3000); // 全角空格
    display_char_bitmap(font_data, 0x3001); // 全角顿号
    display_char_bitmap(font_data, 0x3002); // 全角句号
    
    // 全角字符测试
    display_char_bitmap(font_data, 0xFF01); // 全角感叹号
    display_char_bitmap(font_data, 0xFF0C); // 全角逗号
    display_char_bitmap(font_data, 0xFF1A); // 全角冒号
    
    // 中文字符测试
    display_char_bitmap(font_data, 0x4E00); // 第一个汉字
    display_char_bitmap(font_data, 0x4E2D); // "中"
    display_char_bitmap(font_data, 0x6587); // "文"
    
    debug_print("[INFO] 字体验证完成，进入循环模式\r\n");
    debug_print("[INFO] 按Ctrl+C退出\r\n");
    
    // 主循环
    uint32_t counter = 0;
    while (1) {
        sleep_ms(5000);
        counter++;
        debug_print("[INFO] 运行中... 计数: %d\r\n", counter);
        
        // 每10次循环显示一次内存状态
        if (counter % 10 == 0) {
            debug_print("[DEBUG] 内存状态检查:\r\n");
            debug_print("  字体数据地址: 0x%08X\r\n", (uint32_t)font_data);
            debug_print("  前4字节: %02X %02X %02X %02X\r\n", 
                       font_data[0], font_data[1], font_data[2], font_data[3]);
        }
    }
    
    return 0;
}