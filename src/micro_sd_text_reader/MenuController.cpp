#include "micro_sd_text_reader/MenuController.hpp"
#include "micro_sd_text_reader/MicroSDManager.hpp"
#include "micro_sd_text_reader/MenuRenderer.hpp"
#include "button/button_event.hpp"
#include "pico/stdlib.h"
#include <algorithm>

namespace micro_sd_text_reader {

MenuController::MenuController(st7306::ST7306Driver* display, button::ButtonController* button_controller, hybrid_font::FontManager<st7306::ST7306Driver>* font_manager)
    : selected_index_(0), current_dir_("/"), display_(display), button_controller_(button_controller), font_manager_(font_manager) {}

bool MenuController::initialize() {
    static MicroSDManager sd_manager;
    if (!sd_manager.is_ready()) {
        if (!sd_manager.initialize()) {
            return false;
        }
    }
    menu_items_.clear();
    auto result = sd_manager.list_directory(current_dir_);
    if (!result.is_ok()) {
        return false;
    }
    const auto& file_list = *result;
    for (const auto& file : file_list) {
        MenuItemType type = file.is_directory ? MenuItemType::Directory : MenuItemType::File;
        menu_items_.push_back(MenuItem{type, file.name, file.full_path});
    }
    std::sort(menu_items_.begin(), menu_items_.end(), [](const MenuItem& a, const MenuItem& b) {
        if (a.type != b.type) return a.type == MenuItemType::Directory;
        return a.name < b.name;
    });
    selected_index_ = 0;
    return true;
}

void MenuController::run() {
    MenuRenderer renderer(display_, font_manager_);
    button::ButtonEventHandler btn_handler(button_controller_);
    
    // 重置按键事件处理器状态，确保状态干净
    btn_handler.reset();
    
    bool running = true;
    printf("[MENU] 进入菜单模式，按键说明：KEY1=上移/回退，KEY2=下移/选中\n");
    
    while (running) {
        btn_handler.update();
        
        // 获取按键事件
        auto key1_event = btn_handler.get_key1_event();
        auto key2_event = btn_handler.get_key2_event();
        auto combo_event = btn_handler.get_combo_event();
        
        // 调试：打印事件状态
        if (key1_event != button::ButtonLogicEvent::NONE || key2_event != button::ButtonLogicEvent::NONE) {
            printf("[MENU] 事件检测: KEY1=%d, KEY2=%d\n", 
                   static_cast<int>(key1_event), static_cast<int>(key2_event));
        }
        
        // 处理组合键事件
        if (combo_event == button::ButtonLogicEvent::COMBO_PRESS) {
            printf("[MENU] 检测到组合键事件\n");
            btn_handler.reset();
            continue;
        }
        
        // 优先处理双击事件，防止短按被提前消费
        bool handled = false;
        
        // KEY2双击 = 选中
        if (key2_event == button::ButtonLogicEvent::DOUBLE_PRESS) {
            printf("[MENU] KEY2双击 - 选中项目\n");
            if (!menu_items_.empty()) {
                const auto& item = menu_items_[selected_index_];
                if (item.type == MenuItemType::Directory) {
                    printf("[MENU] 进入目录: %s\n", item.path.c_str());
                    current_dir_ = item.path;
                    initialize();
                } else if (item.type == MenuItemType::File) {
                    printf("[MENU] 选中文件: %s\n", item.path.c_str());
                    running = false;
                }
            }
            handled = true;
            btn_handler.reset();
        }
        
        // KEY1双击 = 回退
        if (key1_event == button::ButtonLogicEvent::DOUBLE_PRESS) {
            printf("[MENU] KEY1双击 - 回退\n");
            if (current_dir_ != "/") {
                size_t pos = current_dir_.find_last_of("/");
                if (pos != std::string::npos && pos > 0) {
                    current_dir_ = current_dir_.substr(0, pos);
                } else {
                    current_dir_ = "/";
                }
                printf("[MENU] 返回上级目录: %s\n", current_dir_.c_str());
                initialize();
            } else {
                printf("[MENU] 已在根目录，退出菜单\n");
                running = false;
            }
            handled = true;
            btn_handler.reset();
        }
        
        // 只有不是双击时才处理短按移动
        if (!handled) {
            // KEY1短按 = 上移
            if (key1_event == button::ButtonLogicEvent::SHORT_PRESS) {
                printf("[MENU] KEY1短按 - 上移选择\n");
                if (selected_index_ > 0) {
                    selected_index_--;
                    printf("[MENU] 选择索引: %d\n", selected_index_);
                }
                btn_handler.reset();
            }
            // KEY2短按 = 下移
            if (key2_event == button::ButtonLogicEvent::SHORT_PRESS) {
                printf("[MENU] KEY2短按 - 下移选择\n");
                if (selected_index_ < (int)menu_items_.size() - 1) {
                    selected_index_++;
                    printf("[MENU] 选择索引: %d\n", selected_index_);
                }
                btn_handler.reset();
            }
        }
        
        // 渲染菜单
        renderer.draw_menu(menu_items_, selected_index_, current_dir_);
        sleep_ms(30);
    }
    
    printf("[MENU] 菜单模式结束\n");
}

std::string MenuController::get_selected_file() const {
    if (menu_items_.empty() || selected_index_ < 0 || selected_index_ >= (int)menu_items_.size())
        return "";
    const auto& item = menu_items_[selected_index_];
    if (item.type == MenuItemType::File)
        return item.path;
    return "";
}

} // namespace micro_sd_text_reader 