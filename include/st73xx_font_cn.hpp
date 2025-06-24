#pragma once

#include <cstdint>
#include <cstring>
#include "flash_font_cache.hpp"

namespace st73xx_font_cn {

// 字体配置常量
struct FontConfig {
    static constexpr int CHAR_WIDTH = 16;
    static constexpr int CHAR_HEIGHT = 16;
    static constexpr int BYTES_PER_CHAR = 32;  // 16x16 = 256 bits = 32 bytes
};

// 字体数据源接口
class IFontDataSource {
public:
    virtual ~IFontDataSource() = default;
    virtual const uint8_t* get_char_bitmap(uint32_t char_code) const = 0;
    virtual bool verify_header() const = 0;
    virtual uint16_t get_version() const = 0;
    virtual uint16_t get_total_chars() const = 0;
};

// Flash内存字体数据源（使用FlashFontCache）
class FlashFontDataSource : public IFontDataSource {
private:
    st73xx_font::FlashFontCache* font_cache_;
    mutable std::vector<uint8_t> temp_bitmap_;  // 临时存储位图数据
    
public:
    explicit FlashFontDataSource(const uint8_t* font_data = nullptr);
    
    const uint8_t* get_char_bitmap(uint32_t char_code) const override;
    bool verify_header() const override;
    uint16_t get_version() const override;
    uint16_t get_total_chars() const override;
    
    // 设置字体数据地址
    void set_font_data(const uint8_t* font_data);
    
    // 获取FlashFontCache实例
    st73xx_font::FlashFontCache* get_cache() const;
};

// 字体渲染器接口
template<typename DisplayDriver>
class IFontRenderer {
public:
    virtual ~IFontRenderer() = default;
    virtual void draw_char(DisplayDriver& display, int x, int y, uint32_t char_code, bool color) = 0;
    virtual void draw_string(DisplayDriver& display, int x, int y, const char* str, bool color) = 0;
};

// 16x16字体渲染器
template<typename DisplayDriver>
class FontRenderer16x16 : public IFontRenderer<DisplayDriver> {
private:
    const IFontDataSource* font_source_;
    
public:
    explicit FontRenderer16x16(const IFontDataSource* font_source = nullptr);
    
    void set_font_source(const IFontDataSource* font_source);
    const IFontDataSource* get_font_source() const;
    
    void draw_char(DisplayDriver& display, int x, int y, uint32_t char_code, bool color) override;
    void draw_string(DisplayDriver& display, int x, int y, const char* str, bool color) override;
    
private:
    uint32_t decode_utf8_char(const char*& str) const;
};

// 便捷的字体管理器类
template<typename DisplayDriver>
class FontManager {
private:
    FontRenderer16x16<DisplayDriver> renderer_;
    FlashFontDataSource font_source_;
    
public:
    FontManager(const uint8_t* font_data = nullptr);
    
    // 初始化字体
    bool initialize(const uint8_t* font_data = nullptr);
    
    // 验证字体
    bool verify_font() const;
    
    // 绘制函数
    void draw_char(DisplayDriver& display, int x, int y, uint32_t char_code, bool color);
    void draw_string(DisplayDriver& display, int x, int y, const char* str, bool color);
    
    // 获取字体信息
    uint16_t get_font_version() const;
    uint16_t get_total_chars() const;
    
    // 调试功能
    void print_char_bitmap(uint32_t char_code) const;
};

// 默认字体数据地址
constexpr uint32_t DEFAULT_FONT_ADDRESS = 0x10100000;

} // namespace st73xx_font_cn

// 包含实现文件
#include "st73xx_font_cn.inl" 