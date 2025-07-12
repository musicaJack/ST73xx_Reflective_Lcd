#pragma once

#include <cstdio>

namespace st73xx_font_cn {

// FlashFontDataSource 实现
inline FlashFontDataSource::FlashFontDataSource(const uint8_t* font_data)
    : font_data_(font_data) {
}

inline void FlashFontDataSource::set_font_data(const uint8_t* font_data) {
    font_data_ = font_data;
}

inline const uint8_t* FlashFontDataSource::get_char_bitmap(uint32_t char_code) const {
    if (!font_data_) {
        return nullptr;
    }
    
    // 读取文件头信息
    uint16_t version = *(uint16_t*)font_data_;
    uint16_t total_chars = *(uint16_t*)(font_data_ + 2);
    
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
    return font_data_ + 4 + char_offset * FontConfig::BYTES_PER_CHAR;
}

inline bool FlashFontDataSource::verify_header() const {
    if (!font_data_) {
        return false;
    }
    
    uint16_t version = *(uint16_t*)font_data_;
    uint16_t total_chars = *(uint16_t*)(font_data_ + 2);
    
    // 检查版本号和字符数量是否合理
    return (version == 1 && total_chars == 21301);
}

inline uint16_t FlashFontDataSource::get_version() const {
    return font_data_ ? *(uint16_t*)font_data_ : 0;
}

inline uint16_t FlashFontDataSource::get_total_chars() const {
    return font_data_ ? *(uint16_t*)(font_data_ + 2) : 0;
}

// FontRenderer16x16 实现
template<typename DisplayDriver>
inline FontRenderer16x16<DisplayDriver>::FontRenderer16x16(const IFontDataSource* font_source)
    : font_source_(font_source) {
}

template<typename DisplayDriver>
inline void FontRenderer16x16<DisplayDriver>::set_font_source(const IFontDataSource* font_source) {
    font_source_ = font_source;
}

template<typename DisplayDriver>
inline const IFontDataSource* FontRenderer16x16<DisplayDriver>::get_font_source() const {
    return font_source_;
}

template<typename DisplayDriver>
inline void FontRenderer16x16<DisplayDriver>::draw_char(DisplayDriver& display, int x, int y, uint32_t char_code, bool color) {
    if (!font_source_) {
        return;
    }
    
    const uint8_t* bitmap = font_source_->get_char_bitmap(char_code);
    if (!bitmap) {
        return;
    }
    
    for (int row = 0; row < FontConfig::CHAR_HEIGHT; row++) {
        uint16_t row_data = (bitmap[row * 2] << 8) | bitmap[row * 2 + 1];
        
        for (int col = 0; col < FontConfig::CHAR_WIDTH; col++) {
            if (row_data & (0x8000 >> col)) {
                // 绘制像素点
                display.drawPixel(x + col, y + row, color);
            }
        }
    }
}

template<typename DisplayDriver>
inline uint32_t FontRenderer16x16<DisplayDriver>::decode_utf8_char(const char*& str) const {
    uint32_t char_code = 0;
    
    if ((*str & 0x80) == 0) {
        // ASCII字符
        char_code = *str;
        str++;
    } else if ((*str & 0xE0) == 0xC0) {
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
        return 0;
    }
    
    return char_code;
}

template<typename DisplayDriver>
inline void FontRenderer16x16<DisplayDriver>::draw_string(DisplayDriver& display, int x, int y, const char* str, bool color) {
    if (!font_source_ || !str) {
        return;
    }
    
    int current_x = x;
    
    while (*str) {
        uint32_t char_code = decode_utf8_char(str);
        if (char_code > 0) {
            draw_char(display, current_x, y, char_code, color);
            current_x += FontConfig::CHAR_WIDTH;  // 所有字符宽度为16像素
        }
    }
}

// FontManager 实现
template<typename DisplayDriver>
inline FontManager<DisplayDriver>::FontManager(const uint8_t* font_data)
    : renderer_(&font_source_), font_source_(font_data) {
}

template<typename DisplayDriver>
inline bool FontManager<DisplayDriver>::initialize(const uint8_t* font_data) {
    if (font_data) {
        font_source_.set_font_data(font_data);
    }
    return verify_font();
}

template<typename DisplayDriver>
inline bool FontManager<DisplayDriver>::verify_font() const {
    return font_source_.verify_header();
}

template<typename DisplayDriver>
inline void FontManager<DisplayDriver>::draw_char(DisplayDriver& display, int x, int y, uint32_t char_code, bool color) {
    renderer_.draw_char(display, x, y, char_code, color);
}

template<typename DisplayDriver>
inline void FontManager<DisplayDriver>::draw_string(DisplayDriver& display, int x, int y, const char* str, bool color) {
    renderer_.draw_string(display, x, y, str, color);
}

template<typename DisplayDriver>
inline uint16_t FontManager<DisplayDriver>::get_font_version() const {
    return font_source_.get_version();
}

template<typename DisplayDriver>
inline uint16_t FontManager<DisplayDriver>::get_total_chars() const {
    return font_source_.get_total_chars();
}

template<typename DisplayDriver>
inline void FontManager<DisplayDriver>::print_char_bitmap(uint32_t char_code) const {
    const uint8_t* bitmap = font_source_.get_char_bitmap(char_code);
    if (!bitmap) {
        printf("Character 0x%04X: No bitmap data\n", char_code);
        return;
    }
    
    printf("Character 0x%04X:\n", char_code);
    for (int row = 0; row < FontConfig::CHAR_HEIGHT; row++) {
        uint16_t row_data = (bitmap[row * 2] << 8) | bitmap[row * 2 + 1];
        
        for (int col = 0; col < FontConfig::CHAR_WIDTH; col++) {
            printf("%c", (row_data & (0x8000 >> col)) ? '#' : '.');
        }
        printf("\n");
    }
    printf("\n");
}

} // namespace st73xx_font_cn 