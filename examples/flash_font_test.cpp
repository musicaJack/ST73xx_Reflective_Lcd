// Flash字体测试示例
// 演示如何使用FlashFontCache访问存储在Flash中的点阵字库

#include <stdio.h>
#include "pico/stdlib.h"
#include "flash_font_cache.hpp"

using namespace st73xx_font;

// 默认字体数据Flash地址
#define FLASH_FONT_ADDRESS 0x10100000

int main() {
    // 初始化stdio
    stdio_init_all();
    
    // 等待USB连接
    printf("等待USB连接...\n");
    sleep_ms(3000);
    
    printf("=== Flash字体测试开始 ===\n");
    
    // 获取FlashFontCache实例
    FlashFontCache& cache = FlashFontCache::get_instance();
    
    // 设置Flash字体数据地址
    const uint8_t* font_data = reinterpret_cast<const uint8_t*>(FLASH_FONT_ADDRESS);
    
    // 初始化字体缓存（假设是16x16字体）
    printf("初始化Flash字体缓存...\n");
    if (!cache.initialize(font_data, 16)) {
        printf("[ERROR] 字体缓存初始化失败！\n");
        while (1) {
            sleep_ms(1000);
            printf("[ERROR] 字体缓存初始化失败，请检查Flash地址 0x%08X\n", FLASH_FONT_ADDRESS);
        }
    }
    
    printf("[SUCCESS] 字体缓存初始化成功！\n");
    
    // 验证字体文件头
    printf("\n验证字体文件头...\n");
    if (!cache.verify_font_header()) {
        printf("[ERROR] 字体文件头验证失败！\n");
        printf("可能原因：\n");
        printf("1. 字体文件未正确烧录到Flash\n");
        printf("2. Flash地址不正确\n");
        printf("3. 字体文件格式不正确\n");
        while (1) {
            sleep_ms(1000);
            printf("[ERROR] 字体文件头验证失败\n");
        }
    }
    
    printf("[SUCCESS] 字体文件头验证通过！\n");
    
    // 显示字体信息
    cache.print_font_info();
    
    // 显示Unicode范围信息
    cache.print_unicode_ranges();
    
    // 测试ASCII字符
    printf("\n=== 测试ASCII字符 ===\n");
    char ascii_chars[] = {'A', 'B', 'C', '1', '2', '3', '@', '#', '$'};
    int ascii_count = sizeof(ascii_chars) / sizeof(ascii_chars[0]);
    
    for (int i = 0; i < ascii_count; i++) {
        printf("测试字符 '%c' (0x%02X):\n", ascii_chars[i], ascii_chars[i]);
        
        if (cache.is_char_supported(ascii_chars[i])) {
            printf("  字符受支持 ✓\n");
            cache.print_char_bitmap(ascii_chars[i]);
        } else {
            printf("  字符不受支持 ✗\n");
        }
        
        sleep_ms(1000); // 1秒延迟
    }
    
    // 测试中文字符
    printf("\n=== 测试中文字符 ===\n");
    uint16_t chinese_chars[] = {
        0x4E00, // "一"
        0x4E2D, // "中"
        0x6587, // "文"
        0x5B57, // "字"
        0x4F53  // "体"
    };
    int chinese_count = sizeof(chinese_chars) / sizeof(chinese_chars[0]);
    
    for (int i = 0; i < chinese_count; i++) {
        printf("测试中文字符 0x%04X:\n", chinese_chars[i]);
        
        if (cache.is_char_supported(chinese_chars[i])) {
            printf("  字符受支持 ✓\n");
            cache.print_char_bitmap(chinese_chars[i]);
        } else {
            printf("  字符不受支持 ✗\n");
        }
        
        sleep_ms(2000); // 2秒延迟，便于观察
    }
    
    // 测试不支持的字符
    printf("\n=== 测试不支持的字符 ===\n");
    uint16_t unsupported_chars[] = {0x0001, 0xFFFF, 0x1234};
    int unsupported_count = sizeof(unsupported_chars) / sizeof(unsupported_chars[0]);
    
    for (int i = 0; i < unsupported_count; i++) {
        printf("测试字符 0x%04X (应该不受支持):\n", unsupported_chars[i]);
        
        if (cache.is_char_supported(unsupported_chars[i])) {
            printf("  字符受支持 (意外!) ✓\n");
            cache.print_char_bitmap(unsupported_chars[i]);
        } else {
            printf("  字符不受支持 (预期) ✗\n");
            // 仍然尝试获取位图（应该返回空格字符）
            auto bitmap = cache.get_char_bitmap(unsupported_chars[i]);
            if (!bitmap.empty()) {
                printf("  返回默认字符位图 (空格)\n");
            }
        }
        
        sleep_ms(1000);
    }
    
    printf("\n=== 所有测试完成 ===\n");
    printf("进入循环模式，每10秒显示一次状态信息\n");
    
    // 主循环
    uint32_t counter = 0;
    while (1) {
        sleep_ms(10000); // 10秒间隔
        counter++;
        
        printf("\n[INFO] 运行计数: %d\n", counter);
        printf("Flash地址: 0x%08X\n", FLASH_FONT_ADDRESS);
        printf("字体大小: %dx%d\n", cache.get_font_size(), cache.get_font_size());
        
        // 每5次循环测试一个字符
        if (counter % 5 == 0) {
            printf("定期测试 - 显示字符 'A':\n");
            cache.print_char_bitmap('A');
        }
        
        // 每10次循环显示字体信息
        if (counter % 10 == 0) {
            cache.print_font_info();
        }
    }
    
    return 0;
} 