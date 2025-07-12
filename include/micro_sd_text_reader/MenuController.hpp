#pragma once
#include <vector>
#include <string>
#include "MenuItem.hpp"
#include "st7306_driver.hpp"
#include "button/button.hpp"
#include "hybrid_font_renderer.hpp"

namespace micro_sd_text_reader {

class MenuController {
public:
    MenuController(st7306::ST7306Driver* display, button::ButtonController* button_controller, hybrid_font::FontManager<st7306::ST7306Driver>* font_manager);
    bool initialize();
    void run(); // 菜单主循环
    std::string get_selected_file() const; // 获取选中的文件路径
    const std::vector<MenuItem>& get_menu_items() const { return menu_items_; }
private:
    std::vector<MenuItem> menu_items_;
    int selected_index_;
    std::string current_dir_;
    st7306::ST7306Driver* display_;
    button::ButtonController* button_controller_;
    hybrid_font::FontManager<st7306::ST7306Driver>* font_manager_;
};

} // namespace micro_sd_text_reader 