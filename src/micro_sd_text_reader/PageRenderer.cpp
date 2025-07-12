#include "micro_sd_text_reader/PageRenderer.hpp"
#include "st7306_driver.hpp"
#include "hybrid_font_system.hpp"
#include "pico/stdlib.h"
#include <cstdio>
#include "hybrid_font_renderer.hpp"
// #include <thread> // 不再需要
// #include <chrono> // 不再需要

// 相关宏定义
#define LCD_WIDTH 300
#define LCD_HEIGHT 400
#define SCREEN_MARGIN 20
#define SIDE_MARGIN SCREEN_MARGIN
#define TOP_MARGIN SCREEN_MARGIN
#define BOTTOM_MARGIN SCREEN_MARGIN
#define DISPLAY_WIDTH (LCD_WIDTH - 2 * SCREEN_MARGIN)
#define LINE_HEIGHT 22
#define PARAGRAPH_SPACING 8
#define TITLE_CONTENT_SPACING 12
#define CONTENT_FOOTER_SPACING 20

namespace micro_sd_text_reader {

PageRenderer::PageRenderer(void* display, void* font_manager)
    : display_(display), font_manager_(font_manager) {}

void PageRenderer::draw_header(const std::string& filename) {
    auto* display = static_cast<st7306::ST7306Driver*>(display_);
    auto* font_manager = static_cast<hybrid_font::FontManager<st7306::ST7306Driver>*>(font_manager_);
    font_manager->draw_string(*display, SIDE_MARGIN, SIDE_MARGIN - 3, filename, true);
    int separator_y = SIDE_MARGIN + 12;
    for (int x = SIDE_MARGIN; x < LCD_WIDTH - SIDE_MARGIN; x++) {
        display->drawPixel(x, separator_y, true);
    }
}

void PageRenderer::draw_footer(int current_page, int total_pages, const std::string& tip) {
    auto* display = static_cast<st7306::ST7306Driver*>(display_);
    auto* font_manager = static_cast<hybrid_font::FontManager<st7306::ST7306Driver>*>(font_manager_);
    if (total_pages <= 0) return;
    std::string page_info = "Page " + std::to_string(current_page + 1) + "/" + std::to_string(total_pages);
    int text_width = font_manager->get_string_width(page_info);
    int footer_y = LCD_HEIGHT - BOTTOM_MARGIN - 12;
    if (footer_y > 0 && footer_y < LCD_HEIGHT) {
        font_manager->draw_string(*display, (LCD_WIDTH - text_width) / 2, footer_y, page_info, true);
    }
    if (!tip.empty() && tip.size() > 0) {
        int tip_width = font_manager->get_string_width(tip);
        int tip_y = footer_y - 16;
        if (tip_y > 0 && tip_y < LCD_HEIGHT) {
            font_manager->draw_string(*display, (LCD_WIDTH - tip_width) / 2, tip_y, tip, true);
        }
    }
}

void PageRenderer::show_static_page(int page, const std::vector<std::string>& lines, int total_pages, const std::string& filename, const std::string& tip) {
    auto* display = static_cast<st7306::ST7306Driver*>(display_);
    auto* font_manager = static_cast<hybrid_font::FontManager<st7306::ST7306Driver>*>(font_manager_);
    if (!display->is_initialized()) {
        printf("[ERROR] 显示屏未初始化，无法显示页面\n");
        return;
    }
    display->clearDisplay();
    draw_header(filename);
    int content_start_y = TOP_MARGIN + 16 + TITLE_CONTENT_SPACING;
    int content_end_y = LCD_HEIGHT - BOTTOM_MARGIN - CONTENT_FOOTER_SPACING;
    int content_height = content_end_y - content_start_y;
    int display_width = DISPLAY_WIDTH;
    int y = content_start_y;
    bool prev_line_empty = false;
    for (int i = 0; i < lines.size() && y < content_end_y - LINE_HEIGHT; i++) {
        const std::string& line_text = lines[i];
        bool current_line_empty = line_text.empty() || line_text.size() == 0;
        if (current_line_empty) {
            if (!prev_line_empty) {
                y += PARAGRAPH_SPACING;
                prev_line_empty = true;
            }
            continue;
        }
        font_manager->draw_string(*display, SIDE_MARGIN, y, line_text, true);
        y += LINE_HEIGHT;
        prev_line_empty = false;
        if (y > content_end_y - LINE_HEIGHT) {
            break;
        }
    }
    draw_footer(page, total_pages, tip);
    display->display();
}

void PageRenderer::display_error_screen(const std::string& error_msg) {
    auto* display = static_cast<st7306::ST7306Driver*>(display_);
    auto* font_manager = static_cast<hybrid_font::FontManager<st7306::ST7306Driver>*>(font_manager_);
    if (!display->is_initialized()) {
        printf("[ERROR] 显示屏未初始化，无法显示错误信息\n");
        return;
    }
    display->clearDisplay();
    draw_header("");
    int center_x = LCD_WIDTH / 2;
    int y = LCD_HEIGHT / 2 - 70;
    std::string error_title = "❌ 系统错误";
    int title_width = font_manager->get_string_width(error_title);
    font_manager->draw_string(*display, center_x - title_width/2, y, error_title, true);
    y += LINE_HEIGHT * 2;
    int box_width = 260;
    int box_height = 100;
    int box_x = center_x - box_width/2;
    int box_y = y;
    for (int x = box_x; x < box_x + box_width; x++) {
        display->drawPixel(x, box_y, true);
        display->drawPixel(x, box_y + box_height, true);
    }
    for (int y_line = box_y; y_line <= box_y + box_height; y_line++) {
        display->drawPixel(box_x, y_line, true);
        display->drawPixel(box_x + box_width, y_line, true);
    }
    y += 15;
    int max_msg_width = box_width - 20;
    // 这里只简单显示，不做智能换行
    int line_width = font_manager->get_string_width(error_msg);
    font_manager->draw_string(*display, center_x - line_width/2, y, error_msg, true);
    y += LINE_HEIGHT;
    y = box_y + box_height + 15;
    std::string suggestion = "请检查 SD 卡连接和格式";
    int sug_width = font_manager->get_string_width(suggestion);
    font_manager->draw_string(*display, center_x - sug_width/2, y, suggestion, true);
    y += LINE_HEIGHT;
    std::string suggestion2 = "Check SD card connection";
    int sug2_width = font_manager->get_string_width(suggestion2);
    font_manager->draw_string(*display, center_x - sug2_width/2, y, suggestion2, true);
    y = LCD_HEIGHT - BOTTOM_MARGIN - 30;
    std::string retry_msg = "程序将在 5 秒后结束";
    int retry_width = font_manager->get_string_width(retry_msg);
    font_manager->draw_string(*display, center_x - retry_width/2, y, retry_msg, true);
    display->display();
    for (int i = 5; i > 0; i--) {
        sleep_ms(1000);
        printf("[错误] %s - %d 秒后程序退出\n", error_msg.c_str(), i);
    }
}

} // namespace micro_sd_text_reader 