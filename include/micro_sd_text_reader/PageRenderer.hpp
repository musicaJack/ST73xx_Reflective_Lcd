#pragma once
#include <string>
#include <vector>

namespace micro_sd_text_reader {

class PageRenderer {
public:
    PageRenderer(void* display, void* font_manager); // 先用void*，后续主程序适配具体类型

    void draw_header(const std::string& filename);
    void draw_footer(int current_page, int total_pages, const std::string& tip = "");
    void show_static_page(int page, const std::vector<std::string>& lines, int total_pages, const std::string& filename, const std::string& tip = "");
    void display_error_screen(const std::string& error_msg);

private:
    void* display_;
    void* font_manager_;
};

} // namespace micro_sd_text_reader 