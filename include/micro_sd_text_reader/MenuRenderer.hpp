#pragma once
#include <vector>
#include <string>
#include "MenuItem.hpp"
#include "st7306_driver.hpp"
#include "hybrid_font_renderer.hpp"

namespace micro_sd_text_reader {

class MenuRenderer {
public:
    MenuRenderer(st7306::ST7306Driver* display, hybrid_font::FontManager<st7306::ST7306Driver>* font_manager);
    void draw_menu(const std::vector<MenuItem>& items, int selected_index, const std::string& current_dir);
private:
    st7306::ST7306Driver* display_;
    hybrid_font::FontManager<st7306::ST7306Driver>* font_manager_;
};

} // namespace micro_sd_text_reader 