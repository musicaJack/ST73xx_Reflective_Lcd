#pragma once
#include <string>

namespace micro_sd_text_reader {

enum class MenuItemType {
    Directory,
    File
};

struct MenuItem {
    MenuItemType type;
    std::string name;   // 显示名
    std::string path;   // 完整路径
};

} // namespace micro_sd_text_reader 