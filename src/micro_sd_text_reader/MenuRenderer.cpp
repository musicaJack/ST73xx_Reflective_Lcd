#include "micro_sd_text_reader/MenuRenderer.hpp"
#include "hybrid_font_renderer.hpp"
#include <cstdio>

namespace micro_sd_text_reader {

MenuRenderer::MenuRenderer(st7306::ST7306Driver* display, hybrid_font::FontManager<st7306::ST7306Driver>* font_manager)
    : display_(display), font_manager_(font_manager) {}

void MenuRenderer::draw_menu(const std::vector<MenuItem>& items, int selected_index, const std::string& current_dir) {
    if (!display_->is_initialized()) {
        printf("[MenuRenderer] Display not initialized!\n");
        return;
    }
    display_->clearDisplay();

    // =================== 优化后的font_manager实现 ===================
    int x = 20;
    int y_start = 40;
    int line_height = 28;
    int max_items = 10; // 最多显示10项，超出可做翻页
    int show_count = std::min((int)items.size(), max_items);
    for (int i = 0; i < show_count; ++i) {
        const auto& item = items[i];
        int y = y_start + i * line_height;
        bool selected = (i == selected_index);
        // 高亮选中项背景
        if (selected) {
            for (int dx = 0; dx < 260; ++dx) {
                for (int dy = 0; dy < line_height; ++dy) {
                    display_->drawPixelGray(x + dx, y - 4 + dy, st7306::ST7306Driver::COLOR_GRAY2);
                }
            }
        }
        // 绘制文件夹/文件图标和名称，确保方向和对齐正常
        std::string prefix = (item.type == MenuItemType::Directory) ? "[DIR] " : "      ";
        // 优化：确保字符串不反转、不乱码，左对齐
        font_manager_->draw_string(*display_, x, y, (prefix + item.name).c_str(), true);
    }
    // 绘制当前目录路径
    font_manager_->draw_string(*display_, 10, 10, current_dir.c_str(), true);
    // 底部操作提示
    font_manager_->draw_string(*display_, 10, 370, "上/下:选择  下长按:进入  上长按:返回", true);
    display_->display();
}

} // namespace micro_sd_text_reader 