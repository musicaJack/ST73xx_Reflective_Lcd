#pragma once
#include <string>
#include <vector>
#include <functional>

namespace micro_sd_text_reader {

class TextWrapper {
public:
    // 判断是否为中文字符（UTF-8编码）
    static bool is_chinese_char(const std::string& text, size_t pos);
    // 获取UTF-8字符的字节长度
    static int get_utf8_char_length(const std::string& text, size_t pos);
    // 智能换行：将文本分割成适合显示宽度的行（支持中英文混合）
    static std::vector<std::string> wrap_text_lines(const std::string& text, int max_width, std::function<int(const std::string&)> get_string_width);
};

} // namespace micro_sd_text_reader 