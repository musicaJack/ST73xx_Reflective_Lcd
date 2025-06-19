#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <windows.h>
#include <chrono>
#include <thread>
#include <algorithm>
#include "font_cache.hpp"

// 字体文件头结构
struct FontHeader {
    uint16_t version;     // 版本号
    uint16_t char_count;  // 字符数量（ASCII可打印字符为94个）
};

// 字体缓存
FontCache& font_cache = FontCache::get_instance();

// 设置控制台窗口大小
void set_console_size(int width, int height) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SMALL_RECT windowSize = {0, 0, static_cast<SHORT>(width - 1), static_cast<SHORT>(height - 1)};
    SetConsoleWindowInfo(hConsole, TRUE, &windowSize);
    
    COORD bufferSize = {static_cast<SHORT>(width), static_cast<SHORT>(height)};
    SetConsoleScreenBufferSize(hConsole, bufferSize);
}

// 设置控制台光标位置
void set_cursor_position(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

// 绘制单个像素点
void draw_pixel(int x, int y, bool is_on) {
    set_cursor_position(x, y);
    std::cout << (is_on ? "█" : " ");
}

// 显示字符位图
void display_char_bitmap(const std::vector<uint8_t>& bitmap, int size) {
    int bytes_per_row = size / 8;
    for (int row = 0; row < size; row++) {
        for (int col = 0; col < size; col++) {
            int byte_index = row * bytes_per_row + col / 8;
            int bit_index = 7 - (col % 8);  // 从高位到低位
            bool is_on = (bitmap[byte_index] & (1 << bit_index)) != 0;
            draw_pixel(col * 2, row, is_on);  // 水平方向放大2倍
        }
    }
}

// 显示ASCII字符
void display_ascii_char(uint16_t unicode_code, int size) {
    // 获取字符的位图数据
    std::vector<uint8_t> bitmap = font_cache.get_char_bitmap(unicode_code, size);
    if (bitmap.empty()) {
        std::cout << "Failed to get bitmap for character: 0x" << std::hex << unicode_code << std::dec << std::endl;
        return;
    }
    
    // 显示字符位图
    display_char_bitmap(bitmap, size);
}

// 显示Unicode字符
void display_unicode_char(uint16_t unicode_code, int size) {
    // 获取字符的位图数据
    std::vector<uint8_t> bitmap = font_cache.get_char_bitmap(unicode_code, size);
    if (bitmap.empty()) {
        std::cout << "Failed to get bitmap for character code: 0x" << std::hex << unicode_code << std::dec << std::endl;
        return;
    }
    
    // 显示字符位图
    display_char_bitmap(bitmap, size);
}

// GBK到Unicode转换
uint16_t gbk_to_unicode(uint16_t gbk_code) {
    uint8_t high = (gbk_code >> 8) & 0xFF;
    uint8_t low = gbk_code & 0xFF;
    
    // 常用汉字的直接映射
    if (gbk_code == 0xD6D0) return 0x4E2D; // 中
    if (gbk_code == 0xC4E3) return 0x4F60; // 你
    if (gbk_code == 0xBAC3) return 0x597D; // 好
    if (gbk_code == 0xCED2) return 0x6211; // 我
    if (gbk_code == 0xC4E3) return 0x4F60; // 你
    if (gbk_code == 0xBAC3) return 0x597D; // 好
    if (gbk_code == 0xCED2) return 0x6211; // 我
    
    // 对于其他字符，使用简化的转换公式（可能不准确）
    if (high >= 0xB0 && high <= 0xF7 && low >= 0xA1 && low <= 0xFE) {
        uint16_t unicode = 0x4E00 + (high - 0xB0) * 94 + (low - 0xA1);
        return unicode;
    }
    
    return 0; // 无法转换
}

// 将输入字符串转换为Unicode码点
std::vector<uint16_t> input_to_codes(const std::string& input_str) {
    std::vector<uint16_t> codes;
    size_t pos = 0;
    
    std::cout << "开始解码输入字符串，长度: " << input_str.length() << std::endl;
    
    while (pos < input_str.length()) {
        unsigned char c = static_cast<unsigned char>(input_str[pos]);
        
        std::cout << "位置 " << pos << ": 0x" << std::hex << static_cast<int>(c) << std::dec << std::endl;
        
        // 首先尝试UTF-8解码
        if (c < 0x80) {
            // ASCII字符
            std::cout << "  ASCII字符: " << static_cast<char>(c) << std::endl;
            codes.push_back(c);
            pos++;
        } else if ((c & 0xE0) == 0xC0) {
            // UTF-8 2字节字符
            std::cout << "  尝试UTF-8 2字节解码" << std::endl;
            if (pos + 1 < input_str.length()) {
                unsigned char c2 = static_cast<unsigned char>(input_str[pos + 1]);
                std::cout << "    第二个字节: 0x" << std::hex << static_cast<int>(c2) << std::dec << std::endl;
                if ((c2 & 0xC0) == 0x80) {
                    uint16_t unicode = ((c & 0x1F) << 6) | (c2 & 0x3F);
                    std::cout << "    UTF-8解码成功: 0x" << std::hex << unicode << std::dec << std::endl;
                    codes.push_back(unicode);
                    pos += 2;
                    continue;
                }
            }
        } else if ((c & 0xF0) == 0xE0) {
            // UTF-8 3字节字符
            std::cout << "  尝试UTF-8 3字节解码" << std::endl;
            if (pos + 2 < input_str.length()) {
                unsigned char c2 = static_cast<unsigned char>(input_str[pos + 1]);
                unsigned char c3 = static_cast<unsigned char>(input_str[pos + 2]);
                std::cout << "    第二个字节: 0x" << std::hex << static_cast<int>(c2) << std::dec << std::endl;
                std::cout << "    第三个字节: 0x" << std::hex << static_cast<int>(c3) << std::dec << std::endl;
                if ((c2 & 0xC0) == 0x80 && (c3 & 0xC0) == 0x80) {
                    uint16_t unicode = ((c & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
                    std::cout << "    UTF-8解码成功: 0x" << std::hex << unicode << std::dec << std::endl;
                    codes.push_back(unicode);
                    pos += 3;
                    continue;
                }
            }
        }
        
        // 如果不是UTF-8，尝试GBK解码
        if (c >= 0x80) {
            std::cout << "  尝试GBK解码" << std::endl;
            
            if (pos + 1 < input_str.length()) {
                unsigned char c2 = static_cast<unsigned char>(input_str[pos + 1]);
                std::cout << "    第二个字节: 0x" << std::hex << static_cast<int>(c2) << std::dec << std::endl;
                
                // 检查是否是GBK编码范围
                if ((c >= 0x81 && c <= 0xFE) && (c2 >= 0x40 && c2 <= 0xFE)) {
                    std::cout << "    可能是GBK编码" << std::endl;
                    
                    uint16_t gbk_code = (c << 8) | c2;
                    uint16_t unicode = gbk_to_unicode(gbk_code);
                    
                    if (unicode > 0) {
                        std::cout << "    GBK转换成功: 0x" << std::hex << gbk_code << " -> 0x" << unicode << std::dec << std::endl;
                        codes.push_back(unicode);
                        pos += 2;
                        continue;
                    } else {
                        std::cout << "    GBK转换失败" << std::endl;
                        pos += 2;
                        continue;
                    }
                } else {
                    std::cout << "    不是GBK编码范围" << std::endl;
                }
            } else {
                std::cout << "    缺少第二个字节，可能是单字节GBK" << std::endl;
                // 单字节GBK，尝试直接转换
                if (c >= 0xA1 && c <= 0xFE) {
                    // 可能是GBK符号区
                    std::cout << "    可能是GBK符号区: 0x" << std::hex << static_cast<int>(c) << std::dec << std::endl;
                    codes.push_back(c);
                    pos++;
                    continue;
                }
            }
        }
        
        // 如果都不是，跳过当前字节
        std::cout << "  无法解码，跳过字节" << std::endl;
        pos++;
    }
    
    std::cout << "解码完成，共 " << codes.size() << " 个字符" << std::endl;
    return codes;
}

// 显示输入的文字
void display_input_text(const std::string& text, int size) {
    int max_chars_per_line = 16;
    int top_blank_rows = 4;
    int header_rows = 6;
    int char_display_start_y = header_rows + top_blank_rows;
    std::vector<uint16_t> codes = input_to_codes(text);
    int total_rows = (codes.size() + max_chars_per_line - 1) / max_chars_per_line;
    int char_display_height = total_rows * (size + 1);
    int char_display_end_y = char_display_start_y + char_display_height;
    int info_display_y = char_display_end_y + 1;
    int finish_display_y = info_display_y + 2;
    int window_width = max_chars_per_line * (size + 2) + 2;
    int window_height = finish_display_y + 2;
    set_console_size(window_width, window_height);
    system("cls");
    set_cursor_position(0, 0);
    std::cout << "输入的文字: " << text << std::endl;
    std::cout << "字符数量: " << text.length() << std::endl;
    std::cout << "================================================" << std::endl;
    std::cout << "输入字节: ";
    for (unsigned char c : text) {
        printf("%02X ", c);
    }
    std::cout << std::endl;
    std::cout << "转换后的Unicode码点: ";
    for (uint16_t code : codes) {
        printf("0x%04X ", code);
    }
    std::cout << std::endl;
    std::cout << "================================================" << std::endl;
    for (int row = char_display_start_y; row < window_height; row++) {
        set_cursor_position(0, row);
        std::cout << std::string(window_width, ' ');
    }
    for (size_t i = 0; i < codes.size(); i++) {
        uint16_t code = codes[i];
        int row = i / max_chars_per_line;
        int col = i % max_chars_per_line;
        int start_x = col * (size + 2);
        int start_y = row * (size + 1) + char_display_start_y;
        for (int y = 0; y < size; y++) {
            set_cursor_position(start_x, start_y + y);
            std::cout << std::string(size * 2, ' ');
        }
        if (code < 0x80) {
            display_ascii_char(code, size);
        } else {
            display_unicode_char(code, size);
        }
        set_cursor_position(0, info_display_y);
        std::cout << std::string(window_width, ' ');
        set_cursor_position(0, info_display_y);
        printf("当前字符: 0x%04X (位置: %zu/%zu)    ", code, i + 1, codes.size());
        Sleep(500);
    }
    set_cursor_position(0, finish_display_y);
    std::cout << "显示完成！按任意键继续..." << std::endl;
    std::cin.get();
}

// 将16进制字符串转换为Unicode码点
std::vector<uint16_t> hex_to_unicode_codes(const std::string& hex_input) {
    std::vector<uint16_t> codes;
    std::string input = hex_input;
    
    // 移除所有空格和逗号
    input.erase(std::remove(input.begin(), input.end(), ' '), input.end());
    input.erase(std::remove(input.begin(), input.end(), ','), input.end());
    
    // 检查是否以0x开头，如果是则移除
    if (input.substr(0, 2) == "0x" || input.substr(0, 2) == "0X") {
        input = input.substr(2);
    }
    
    // 每4个字符转换为一个Unicode码点
    for (size_t i = 0; i < input.length(); i += 4) {
        if (i + 3 < input.length()) {
            std::string hex_code = input.substr(i, 4);
            try {
                uint16_t unicode = std::stoul(hex_code, nullptr, 16);
                codes.push_back(unicode);
            } catch (const std::exception& e) {
                std::cout << "无法解析16进制码: " << hex_code << std::endl;
            }
        } else {
            // 处理不足4位的情况
            std::string hex_code = input.substr(i);
            if (hex_code.length() <= 4) {
                try {
                    uint16_t unicode = std::stoul(hex_code, nullptr, 16);
                    codes.push_back(unicode);
                } catch (const std::exception& e) {
                    std::cout << "无法解析16进制码: " << hex_code << std::endl;
                }
            }
            break;
        }
    }
    
    return codes;
}

// 显示16进制Unicode码点对应的字符
void display_hex_unicode_codes(const std::string& hex_input, int size) {
    std::vector<uint16_t> codes = hex_to_unicode_codes(hex_input);
    if (codes.empty()) {
        std::cout << "没有有效的Unicode码点！" << std::endl;
        return;
    }
    int max_chars_per_line = 16;
    int top_blank_rows = 4; // 头部信息后空几行即可
    int header_rows = 6;    // 头部信息行数
    int char_display_start_y = header_rows + top_blank_rows;
    int total_rows = (codes.size() + max_chars_per_line - 1) / max_chars_per_line;
    int char_display_height = total_rows * (size + 1);
    int char_display_end_y = char_display_start_y + char_display_height;
    int info_display_y = char_display_end_y + 1;  // 字符信息显示在字符区域下方1行
    int finish_display_y = info_display_y + 2;    // "显示完成"再往下2行
    int window_width = max_chars_per_line * (size + 2) + 2;
    int window_height = finish_display_y + 2; // 总高度=头部+空白+字符区+信息+底部
    set_console_size(window_width, window_height);
    system("cls");
    // 头部信息
    set_cursor_position(0, 0);
    std::cout << "输入的16进制Unicode码: " << hex_input << std::endl;
    std::cout << "转换后的Unicode码点: ";
    for (uint16_t code : codes) {
        printf("0x%04X ", code);
    }
    std::cout << std::endl;
    std::cout << "字符数量: " << codes.size() << std::endl;
    std::cout << "================================================" << std::endl;
    // 清空字符显示区域和信息显示区域
    for (int row = char_display_start_y; row < window_height; row++) {
        set_cursor_position(0, row);
        std::cout << std::string(window_width, ' ');
    }
    for (size_t i = 0; i < codes.size(); i++) {
        uint16_t code = codes[i];
        int row = i / max_chars_per_line;
        int col = i % max_chars_per_line;
        int start_x = col * (size + 2);
        int start_y = row * (size + 1) + char_display_start_y;
        for (int y = 0; y < size; y++) {
            set_cursor_position(start_x, start_y + y);
            std::cout << std::string(size * 2, ' ');
        }
        if (code < 0x80) {
            display_ascii_char(code, size);
        } else {
            display_unicode_char(code, size);
        }
        set_cursor_position(0, info_display_y);
        std::cout << std::string(window_width, ' ');
        set_cursor_position(0, info_display_y);
        printf("当前字符: 0x%04X (位置: %zu/%zu)    ", code, i + 1, codes.size());
        Sleep(500);
    }
    set_cursor_position(0, finish_display_y);
    std::cout << "显示完成！按任意键继续..." << std::endl;
    std::cin.get();
}

// 交互式输入模式
void interactive_input_mode(int size) {
    std::string input;
    
    while (true) {
        // 清屏
        system("cls");
        
        std::cout << "=== 字体显示工具 - 交互式输入模式 ===" << std::endl;
        std::cout << "字体大小: " << size << "x" << size << " 像素" << std::endl;
        std::cout << "=====================================" << std::endl;
        std::cout << "请选择输入模式:" << std::endl;
        std::cout << "1. 普通文本输入 (ASCII字符和中文)" << std::endl;
        std::cout << "2. 16进制Unicode码点输入" << std::endl;
        std::cout << "3. 退出程序" << std::endl;
        std::cout << "请选择 (1-3): ";
        
        std::string mode_choice;
        std::getline(std::cin, mode_choice);
        
        if (mode_choice == "3" || mode_choice == "quit" || mode_choice == "exit" || mode_choice == "退出") {
            std::cout << "程序退出..." << std::endl;
            break;
        }
        
        if (mode_choice == "1") {
            // 普通文本输入模式
            system("cls");
            std::cout << "=== 普通文本输入模式 ===" << std::endl;
            std::cout << "支持输入: ASCII字符和中文" << std::endl;
            std::cout << "输入 'back' 返回主菜单" << std::endl;
            std::cout << "=====================================" << std::endl;
            std::cout << "请输入要显示的文字: ";
            
            // 获取用户输入
            std::getline(std::cin, input);
            
            // 检查返回命令
            if (input == "back" || input == "返回") {
                continue;
            }
            
            // 检查空输入
            if (input.empty()) {
                std::cout << "输入为空，请重新输入！" << std::endl;
                Sleep(1000);
                continue;
            }
            
            // 检查输入是否完整（对于中文字符）
            bool need_more_input = false;
            for (size_t i = 0; i < input.length(); i++) {
                unsigned char c = static_cast<unsigned char>(input[i]);
                if (c >= 0x80) {
                    // 可能是多字节字符
                    if (i + 1 >= input.length()) {
                        // 缺少后续字节
                        need_more_input = true;
                        break;
                    }
                    unsigned char c2 = static_cast<unsigned char>(input[i + 1]);
                    if ((c >= 0x81 && c <= 0xFE) && (c2 >= 0x40 && c2 <= 0xFE)) {
                        // GBK字符，需要2个字节
                        i++; // 跳过下一个字节
                    } else if ((c & 0xF0) == 0xE0) {
                        // UTF-8 3字节字符
                        if (i + 2 >= input.length()) {
                            need_more_input = true;
                            break;
                        }
                        i += 2; // 跳过后续字节
                    }
                }
            }
            
            if (need_more_input) {
                std::cout << "输入不完整，请重新输入完整的中文字符！" << std::endl;
                Sleep(2000);
                continue;
            }
            
            // 显示输入的文字
            display_input_text(input, size);
            
        } else if (mode_choice == "2") {
            // 16进制Unicode码点输入模式
            system("cls");
            std::cout << "=== 16进制Unicode码点输入模式 ===" << std::endl;
            std::cout << "支持格式:" << std::endl;
            std::cout << "  - 4E2D (中文字符'中'的Unicode码点)" << std::endl;
            std::cout << "  - 0x4E2D (带0x前缀)" << std::endl;
            std::cout << "  - 4E2D 597D (多个码点，用空格分隔)" << std::endl;
            std::cout << "  - 4E2D,597D (多个码点，用逗号分隔)" << std::endl;
            std::cout << "输入 'back' 返回主菜单" << std::endl;
            std::cout << "=====================================" << std::endl;
            std::cout << "请输入16进制Unicode码点: ";
            
            // 获取用户输入
            std::getline(std::cin, input);
            
            // 检查返回命令
            if (input == "back" || input == "返回") {
                continue;
            }
            
            // 检查空输入
            if (input.empty()) {
                std::cout << "输入为空，请重新输入！" << std::endl;
                Sleep(1000);
                continue;
            }
            
            // 显示16进制Unicode码点对应的字符
            display_hex_unicode_codes(input, size);
            
        } else {
            std::cout << "无效选择，请重新输入！" << std::endl;
            Sleep(1000);
        }
    }
}

int main() {
    // 显示UTF-8编码提示
    std::cout << "================================================" << std::endl;
    std::cout << "重要提示：使用前请先执行 chcp 65001 命令" << std::endl;
    std::cout << "将终端设置为UTF-8字符编码，才能正确显示中文" << std::endl;
    std::cout << "================================================" << std::endl;
    
    // 检查当前控制台编码
    UINT current_cp = GetConsoleOutputCP();
    std::cout << "当前控制台输出编码: " << current_cp << std::endl;
    if (current_cp != 65001) {
        std::cout << "警告：当前编码不是UTF-8，请执行 chcp 65001" << std::endl;
    }
    std::cout << "按任意键继续..." << std::endl;
    std::cin.get();
    
    // 设置控制台编码为UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);  // 同时设置输入编码
    
    // 选择字体大小
    int size;
    std::cout << "请选择字体大小：\n";
    std::cout << "1. 16x16像素\n";
    std::cout << "2. 24x24像素\n";
    std::cout << "请选择（1或2）：";
    std::cin >> size;
    std::cin.ignore();  // 清除输入缓冲
    size = (size == 1) ? 16 : 24;
    font_cache.set_font_size(size);
    std::string font_file = "output/font" + std::to_string(size) + ".bin";
    if (!font_cache.load_font(font_file)) {
        std::cout << "Failed to load font file: " << font_file << std::endl;
        return 1;
    }
    std::cout << "字体文件加载成功！" << std::endl;
    // 直接进入交互式输入模式
    interactive_input_mode(size);
    return 0;
} 