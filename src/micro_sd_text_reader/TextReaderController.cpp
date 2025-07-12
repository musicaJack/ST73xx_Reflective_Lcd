#include "micro_sd_text_reader/TextReaderController.hpp"
#include <cstdio>
#include "pico/stdlib.h"
#include "button/button_event.hpp"

namespace micro_sd_text_reader {

TextReaderController::TextReaderController(
    st7306::ST7306Driver* display,
    button::ButtonController* button_controller,
    button::ButtonEventHandler* button_event_handler,
    hybrid_font::FontManager<st7306::ST7306Driver>* font_manager,
    MicroSDManager* sd_manager,
    const std::string& filename)
    : display_(display), button_controller_(button_controller), button_event_handler_(button_event_handler), font_manager_(font_manager), sd_manager_(sd_manager),
      filename_(filename), current_page_(0), total_pages_(0), current_mode_(st7306::DisplayMode::Day), sd_ready_(false), file_size_(0)
{
    page_manager_ = std::make_unique<PageManager>(
        filename_,
        [this](const std::string& path, const char* mode) { return sd_manager_->open_file(path, mode); },
        [this](const std::string& s) { return font_manager_->get_string_width(s); }
    );
    page_renderer_ = std::make_unique<PageRenderer>(display_, font_manager_);
    // int total = page_manager_->precompute_pages();
    // printf("[DEBUG] 分页预处理完成，总页数: %d\n", total);
}

TextReaderController::~TextReaderController() {}

void TextReaderController::run() {
    update_display();
    int loop_count = 0;
    printf("[TEXT_READER] 进入阅读模式，长按KEY1可返回主菜单\n");
    while (true) {
        // 更新按键事件
        button_event_handler_->update();
        
        // 获取按键事件
        auto key1_event = button_event_handler_->get_key1_event();
        auto key2_event = button_event_handler_->get_key2_event();
        auto combo_event = button_event_handler_->get_combo_event();
        
        // 调试：打印事件状态
        if (key1_event != button::ButtonLogicEvent::NONE || key2_event != button::ButtonLogicEvent::NONE) {
            printf("[TEXT_READER] 事件检测: KEY1=%d, KEY2=%d\n", 
                   static_cast<int>(key1_event), static_cast<int>(key2_event));
        }
        
        // 处理长按事件（优先级最高）
        if (key1_event == button::ButtonLogicEvent::LONG_PRESS) {
            printf("[TEXT_READER] 检测到KEY1长按事件，准备返回主菜单\n");
            // 重置按键事件处理器状态，避免状态残留
            button_event_handler_->reset();
            // 返回主菜单，run()直接return
            return;
        }
        
        // 处理组合键事件（双键同时按下）
        if (combo_event == button::ButtonLogicEvent::COMBO_PRESS) {
            printf("[TEXT_READER] 检测到组合键，切换显示模式\n");
            toggle_mode();
            button_event_handler_->reset();
            continue;
        }
        
        // 处理双击事件
        if (key1_event == button::ButtonLogicEvent::DOUBLE_PRESS) {
            printf("[TEXT_READER] KEY1双击事件\n");
            // 可以在这里添加双击功能
            button_event_handler_->reset();
            continue;
        }
        
        if (key2_event == button::ButtonLogicEvent::DOUBLE_PRESS) {
            printf("[TEXT_READER] KEY2双击事件\n");
            // 可以在这里添加双击功能
            button_event_handler_->reset();
            continue;
        }
        
        // 处理短按事件
        if (key1_event == button::ButtonLogicEvent::SHORT_PRESS) {
            printf("[TEXT_READER] KEY1短按 - 上一页\n");
            prev_page();
            button_event_handler_->reset();
        }
        
        if (key2_event == button::ButtonLogicEvent::SHORT_PRESS) {
            printf("[TEXT_READER] KEY2短按 - 下一页\n");
            next_page();
            button_event_handler_->reset();
        }
        
        sleep_ms(30);
    }
}

void TextReaderController::next_page() {
    if (current_page_ < page_manager_->total_pages() - 1) {
        current_page_++;
        update_display();
    }
}

void TextReaderController::prev_page() {
    if (current_page_ > 0) {
        current_page_--;
        update_display();
    }
}

void TextReaderController::jump_to_page(int page) {
    if (page >= 0 && page < page_manager_->total_pages()) {
        current_page_ = page;
        update_display();
    }
}

void TextReaderController::toggle_mode() {
    if (current_mode_ == st7306::DisplayMode::Day) {
        current_mode_ = st7306::DisplayMode::Night;
    } else {
        current_mode_ = st7306::DisplayMode::Day;
    }
    display_->setDisplayMode(current_mode_);
    update_display();
}

void TextReaderController::reload_file() {
    page_manager_ = std::make_unique<PageManager>(
        filename_,
        [this](const std::string& path, const char* mode) { return sd_manager_->open_file(path, mode); },
        [this](const std::string& s) { return font_manager_->get_string_width(s); }
    );
    current_page_ = 0;
    update_display();
}

int TextReaderController::get_current_page() const { return current_page_; }
int TextReaderController::get_total_pages() const { return page_manager_->total_pages(); }
std::string TextReaderController::get_filename() const { return filename_; }

int TextReaderController::precompute_pages() {
    return page_manager_->precompute_pages();
}

void TextReaderController::update_display() {
    std::vector<std::string> content;
    if (page_manager_->load_page_content(current_page_, content)) {
        page_renderer_->show_static_page(current_page_, content, page_manager_->total_pages(), filename_, "");
    } else {
        show_error("页面加载失败");
    }
}

void TextReaderController::show_error(const std::string& msg) {
    page_renderer_->display_error_screen(msg);
}

} // namespace micro_sd_text_reader 