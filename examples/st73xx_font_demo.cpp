#include "st7306_driver.hpp"
#include "st73xx_font_bin.hpp"
#include "st73xx_font_stream.hpp"
#include "microsd/micro_sd.hpp"
#include "microsd/spi_config.hpp"
#include "pico/stdlib.h"
#include "spi_config.hpp"
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>

// 分页参数
constexpr int FONT_SIZE = 16;
constexpr int CHAR_HEIGHT = 16;
constexpr int CHAR_WIDTH = 16;
constexpr int LCD_W = st7306::ST7306Driver::LCD_WIDTH;
constexpr int LCD_H = st7306::ST7306Driver::LCD_HEIGHT;
constexpr int PAGE_LINES = LCD_H / CHAR_HEIGHT - 1; // 最后一行显示页码
constexpr int PAGE_DELAY_MS = 10000; // 10秒

// 按屏幕宽度自动分割一行
std::vector<std::string> split_line(const std::string& line, int max_chars) {
    std::vector<std::string> result;
    for (size_t i = 0; i < line.size(); i += max_chars) {
        result.push_back(line.substr(i, max_chars));
    }
    return result;
}

// 按页切分文本
std::vector<std::vector<std::string>> paginate_text(const std::string& text, int lines_per_page, int max_chars_per_line) {
    std::vector<std::vector<std::string>> pages;
    std::vector<std::string> lines;
    size_t pos = 0, prev = 0;
    while ((pos = text.find('\n', prev)) != std::string::npos) {
        std::string line = text.substr(prev, pos - prev);
        auto sublines = split_line(line, max_chars_per_line);
        lines.insert(lines.end(), sublines.begin(), sublines.end());
        prev = pos + 1;
    }
    // 处理最后一行
    if (prev < text.size()) {
        auto sublines = split_line(text.substr(prev), max_chars_per_line);
        lines.insert(lines.end(), sublines.begin(), sublines.end());
    }
    // 分页
    for (size_t i = 0; i < lines.size(); i += lines_per_page) {
        std::vector<std::string> page;
        for (int j = 0; j < lines_per_page && i + j < lines.size(); ++j) {
            page.push_back(lines[i + j]);
        }
        pages.push_back(page);
    }
    return pages;
}

int main() {
    stdio_init_all();
    printf("ST7306 字库流式文本 DEMO (SD卡/分页/自动翻页)\n");

    // 初始化LCD
    st7306::ST7306Driver lcd(PIN_DC, PIN_RST, PIN_CS, PIN_SCLK, PIN_SDIN);
    lcd.initialize();
    lcd.setRotation(0);
    lcd.clearDisplay();

    // 初始化SD卡
    MicroSD::SPIConfig sd_spi_cfg;
    MicroSD::SDCard sd_card(sd_spi_cfg);
    if (sd_card.initialize().is_error()) {
        printf("SD卡初始化失败\n");
        lcd.clearDisplay();
        fontstream::stream_text(lcd, "SD卡初始化失败", FONT_SIZE, st7306::ST7306Driver::COLOR_WHITE, 0);
        sleep_ms(3000);
        return 1;
    }

    // 加载字体
    if (!fontbin::font_load("/font/font16.bin", FONT_SIZE)) {
        printf("字体加载失败\n");
        lcd.clearDisplay();
        fontstream::stream_text(lcd, "字体加载失败", FONT_SIZE, st7306::ST7306Driver::COLOR_WHITE, 0);
        sleep_ms(3000);
        return 1;
    }

    // 读取文本
    auto txt_result = sd_card.read_file("/doc/DigitalFortress_en.txt");
    if (txt_result.is_error()) {
        printf("文本文件读取失败\n");
        lcd.clearDisplay();
        fontstream::stream_text(lcd, "文本文件读取失败", FONT_SIZE, st7306::ST7306Driver::COLOR_WHITE, 0);
        sleep_ms(3000);
        fontbin::font_unload(FONT_SIZE);
        return 1;
    }
    std::string text(txt_result->begin(), txt_result->end());

    // 分页处理
    int max_chars_per_line = LCD_W / CHAR_WIDTH;
    auto pages = paginate_text(text, PAGE_LINES, max_chars_per_line);
    int total_pages = pages.size();
    int page = 0;

    while (true) {
        lcd.clearDisplay();
        // 显示本页内容
        int y = 0;
        for (const auto& line : pages[page]) {
            fontstream::stream_text(lcd, line, FONT_SIZE, st7306::ST7306Driver::COLOR_WHITE, 0);
            y += CHAR_HEIGHT;
            lcd.clearDisplay(); // 逐行流式显示后清屏，防止重叠
            for (int i = 0; i <= &line - &pages[page][0]; ++i) {
                fontstream::stream_text(lcd, pages[page][i], FONT_SIZE, st7306::ST7306Driver::COLOR_WHITE, 0);
            }
        }
        // 显示页码
        char page_info[32];
        snprintf(page_info, sizeof(page_info), "第%d/%d页", page + 1, total_pages);
        fontstream::stream_text(lcd, page_info, FONT_SIZE, st7306::ST7306Driver::COLOR_WHITE, 0);
        // 等待10秒
        sleep_ms(PAGE_DELAY_MS);
        // 下一页
        page = (page + 1) % total_pages;
    }

    fontbin::font_unload(FONT_SIZE);
    printf("DEMO 结束\n");
    return 0;
} 