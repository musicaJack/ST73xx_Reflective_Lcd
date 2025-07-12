#include "st73xx_font_cn.hpp"
#include <cstdio>

namespace st73xx_font_cn {

// 工具函数：打印字体信息
void print_font_info(const IFontDataSource* font_source) {
    if (!font_source) {
        printf("Font source is null\n");
        return;
    }
    
    printf("Font Information:\n");
    printf("  Version: %d\n", font_source->get_version());
    printf("  Total characters: %d\n", font_source->get_total_chars());
    printf("  Header verification: %s\n", font_source->verify_header() ? "PASSED" : "FAILED");
    printf("  Character size: %dx%d pixels\n", FontConfig::CHAR_WIDTH, FontConfig::CHAR_HEIGHT);
    printf("  Bytes per character: %d\n", FontConfig::BYTES_PER_CHAR);
}

// 工具函数：验证字符是否支持
bool is_supported_char(uint32_t char_code) {
    return (char_code >= 0x20 && char_code <= 0x7E) ||      // ASCII
           (char_code >= 0x3000 && char_code <= 0x303F) ||  // 全角标点
           (char_code >= 0xFF00 && char_code <= 0xFFEF) ||  // 全角字符
           (char_code >= 0x4E00 && char_code <= 0x9FA5);    // 中文
}

// 工具函数：获取字符类型描述
const char* get_char_type_name(uint32_t char_code) {
    if (char_code >= 0x20 && char_code <= 0x7E) {
        return "ASCII";
    } else if (char_code >= 0x3000 && char_code <= 0x303F) {
        return "Full-width punctuation";
    } else if (char_code >= 0xFF00 && char_code <= 0xFFEF) {
        return "Full-width character";
    } else if (char_code >= 0x4E00 && char_code <= 0x9FA5) {
        return "Chinese";
    } else {
        return "Unsupported";
    }
}

// 工具函数：测试字符渲染
template<typename DisplayDriver>
void test_char_rendering(FontManager<DisplayDriver>& font_mgr, DisplayDriver& display, 
                        const char* test_chars, int x, int y, bool color) {
    printf("Testing character rendering: %s\n", test_chars);
    
    // 清屏并显示测试字符串
    display.clear();
    font_mgr.draw_string(display, x, y, test_chars, color);
    display.display();
    
    // 打印字符信息
    const char* str = test_chars;
    while (*str) {
        uint32_t char_code = 0;
        
        if ((*str & 0x80) == 0) {
            char_code = *str;
            str++;
        } else if ((*str & 0xE0) == 0xC0) {
            char_code = ((*str & 0x1F) << 6) | (*(str + 1) & 0x3F);
            str += 2;
        } else if ((*str & 0xF0) == 0xE0) {
            char_code = ((*str & 0x0F) << 12) | 
                       ((*(str + 1) & 0x3F) << 6) | 
                       (*(str + 2) & 0x3F);
            str += 3;
        } else {
            str++;
            continue;
        }
        
        printf("  Character 0x%04X (%s): %s\n", 
               char_code, 
               get_char_type_name(char_code),
               is_supported_char(char_code) ? "Supported" : "Not supported");
    }
}

// 显式实例化模板函数（如果需要）
// 这里可以根据需要添加特定显示驱动的实例化

} // namespace st73xx_font_cn 