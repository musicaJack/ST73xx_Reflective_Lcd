#pragma once
#include <memory>
#include <string>
#include <vector>
#include "micro_sd_text_reader/PageManager.hpp"
#include "micro_sd_text_reader/PageRenderer.hpp"
#include "micro_sd_text_reader/MicroSDManager.hpp"
#include "hybrid_font_renderer.hpp"
#include "st7306_driver.hpp"
#include "button/button.hpp"
#include "button/button_event.hpp"

namespace micro_sd_text_reader {

class TextReaderController {
public:
    TextReaderController(
        st7306::ST7306Driver* display,
        button::ButtonController* button_controller,
        button::ButtonEventHandler* button_event_handler,
        hybrid_font::FontManager<st7306::ST7306Driver>* font_manager,
        MicroSDManager* sd_manager,
        const std::string& filename
    );
    ~TextReaderController();

    void run();
    void next_page();
    void prev_page();
    void jump_to_page(int page);
    void toggle_mode();
    void reload_file();
    int get_current_page() const;
    int get_total_pages() const;
    std::string get_filename() const;
    int precompute_pages();

private:
    st7306::ST7306Driver* display_;
    button::ButtonController* button_controller_;
    button::ButtonEventHandler* button_event_handler_;
    hybrid_font::FontManager<st7306::ST7306Driver>* font_manager_;
    MicroSDManager* sd_manager_;
    std::unique_ptr<PageManager> page_manager_;
    std::unique_ptr<PageRenderer> page_renderer_;
    std::string filename_;
    int current_page_;
    int total_pages_;
    st7306::DisplayMode current_mode_;
    bool sd_ready_;
    size_t file_size_;
    void update_display();
    void show_error(const std::string& msg);
};

} // namespace micro_sd_text_reader 