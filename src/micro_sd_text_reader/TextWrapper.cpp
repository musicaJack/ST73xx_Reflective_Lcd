#include "micro_sd_text_reader/TextWrapper.hpp"

namespace micro_sd_text_reader {

bool TextWrapper::is_chinese_char(const std::string& text, size_t pos) {
    if (pos >= text.size()) return false;
    unsigned char c = text[pos];
    return (c >= 0xE0 && c <= 0xEF);
}

int TextWrapper::get_utf8_char_length(const std::string& text, size_t pos) {
    if (pos >= text.size()) return 0;
    unsigned char c = text[pos];
    if (c < 0x80) return 1;
    else if (c < 0xE0) return 2;
    else if (c < 0xF0) return 3;
    else return 4;
}

std::vector<std::string> TextWrapper::wrap_text_lines(const std::string& text, int max_width, std::function<int(const std::string&)> get_string_width) {
    std::vector<std::string> lines;
    if (text.empty()) {
        lines.push_back("");
        return lines;
    }
    std::string current_line;
    size_t pos = 0;
    while (pos < text.size()) {
        if (is_chinese_char(text, pos)) {
            int char_len = get_utf8_char_length(text, pos);
            std::string chinese_char = text.substr(pos, char_len);
            std::string test_line = current_line + chinese_char;
            int test_width = get_string_width(test_line);
            if (test_width <= max_width) {
                current_line = test_line;
            } else {
                if (!current_line.empty()) {
                    lines.push_back(current_line);
                    current_line = chinese_char;
                } else {
                    current_line = chinese_char;
                }
            }
            pos += char_len;
        } else {
            size_t next_space = text.find(' ', pos);
            size_t word_end = (next_space == std::string::npos) ? text.size() : next_space;
            for (size_t i = pos; i < word_end; i++) {
                if (is_chinese_char(text, i)) {
                    word_end = i;
                    break;
                }
            }
            std::string word = text.substr(pos, word_end - pos);
            std::string test_line = current_line;
            if (!test_line.empty() && !word.empty() && word[0] != ' ') {
                test_line += " ";
            }
            test_line += word;
            int test_width = get_string_width(test_line);
            if (test_width <= max_width) {
                current_line = test_line;
            } else {
                if (!current_line.empty()) {
                    lines.push_back(current_line);
                    current_line = word;
                } else {
                    current_line = word;
                }
            }
            pos = word_end;
            if (pos < text.size() && text[pos] == ' ') {
                pos++;
            }
        }
    }
    if (!current_line.empty()) {
        lines.push_back(current_line);
    }
    return lines;
}

} // namespace micro_sd_text_reader 