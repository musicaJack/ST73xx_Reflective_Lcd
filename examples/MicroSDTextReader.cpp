/**
 * @file MicroSDTextReader.cpp
 * @brief MicroSD 文本阅读器 - 从 SD 卡读取文本并显示
 * @version 1.0.0
 */

#include "st7306_driver.hpp"
#include "hybrid_font_system.hpp"
#include "hybrid_font_renderer.hpp"
#include "rw_sd.hpp"
#include "spi_config.hpp"
#include "pico/stdlib.h"
#include <string>
#include <vector>
#include <cmath>
#include <sstream>

// 显示配置
#define LCD_WIDTH 300
#define LCD_HEIGHT 400
#define SCREEN_MARGIN 20      // 屏幕四周留白20像素
#define SIDE_MARGIN SCREEN_MARGIN
#define TOP_MARGIN SCREEN_MARGIN
#define BOTTOM_MARGIN SCREEN_MARGIN

// 实际显示区域
#define DISPLAY_WIDTH (LCD_WIDTH - 2 * SCREEN_MARGIN)   // 260像素
#define DISPLAY_HEIGHT (LCD_HEIGHT - 2 * SCREEN_MARGIN) // 360像素
#define LINE_HEIGHT 16

// 默认文本文件路径
#define TEXT_FILE_PATH "/the_little_prince.txt"

class MicroSDTextReader {
private:
    st7306::ST7306Driver display_;
    hybrid_font::FontManager<st7306::ST7306Driver> font_manager_;
    MicroSD::RWSD sd_;
    
    int current_page_;
    int total_pages_;
    std::string filename_;
    st7306::DisplayMode current_mode_;
    std::vector<std::string> text_content_;
    bool sd_ready_;
    


    void initialize_hardware() {
        printf("初始化 ST7306 显示屏...\n");
        display_.initialize();
        
        // 设置屏幕180度旋转
        display_.setRotation(2);  // 0=0度, 1=90度, 2=180度, 3=270度
        printf("屏幕已设置为180度旋转\n");
        
        printf("显示屏初始化完成.\n");
        
        // 清除显示屏
        display_.clearDisplay();
        
        // 初始化混合字体系统
        if (!font_manager_.initialize()) {
            printf("[ERROR] 混合字体系统初始化失败\n");
            sleep_ms(2000);
        } else {
            printf("[SUCCESS] 混合字体系统初始化成功\n");
            font_manager_.print_status();
            sleep_ms(1000);
        }
        
        printf("[INFO] 显示系统初始化完成 (180度旋转)\n");
    }

    bool initialize_microsd() {
        printf("\n===== 初始化 MicroSD 卡 =====\n");
        
        // 显示引脚配置信息
        printf("MicroSD 引脚配置:\n");
        printf("  MISO: GPIO %d\n", MICROSD_PIN_MISO);
        printf("  MOSI: GPIO %d\n", MICROSD_PIN_MOSI);
        printf("  SCK:  GPIO %d\n", MICROSD_PIN_SCK);
        printf("  CS:   GPIO %d\n", MICROSD_PIN_CS);
        printf("  SPI:  %s\n", (SPI_PORT_MICROSD == spi0) ? "spi0" : "spi1");
        
        // 完全按照 rwsd_demo.cpp 的方式初始化
        printf("开始初始化 SD 卡...\n");
        auto init_result = sd_.initialize();
        if (!init_result.is_ok()) {
            printf("[ERROR] SD卡初始化失败: %s\n", MicroSD::StorageDevice::get_error_description(init_result.error_code()).c_str());
            printf("可能的原因:\n");
            printf("  1. SD 卡未插入或接触不良\n");
            printf("  2. 引脚连接错误\n");
            printf("  3. SD 卡格式不支持（需要 FAT32）\n");
            printf("  4. 电源供应不稳定\n");
            return false;
        }
        
        printf("[SUCCESS] SD卡初始化成功!\n");
        
        // 显示状态信息 - 与 rwsd_demo.cpp 一致
        printf("%s", sd_.get_status_info().c_str());
        printf("%s", sd_.get_config_info().c_str());
        
        return true;
    }

    bool display_file_streaming() {
        printf("\n===== 基础文件操作 =====\n");
        
        // 列出根目录
        auto list_result = sd_.list_directory("/");
        if (list_result.is_ok()) {
            printf("根目录内容:\n");
            for (const auto& file : *list_result) {
                printf("  %s\t%s\t%lu字节\n", 
                    file.is_directory ? "[DIR]" : "[FILE]", 
                    file.name.c_str(), 
                    file.size);
            }
        } else {
            printf("列出目录失败: %s\n", MicroSD::StorageDevice::get_error_description(list_result.error_code()).c_str());
        }
        
        // 检查文件是否存在
        if (!sd_.file_exists(TEXT_FILE_PATH)) {
            printf("文件不存在: %s\n", TEXT_FILE_PATH);
            return false;
        }
        
        printf("\n===== 开始流式显示文件 =====\n");
        printf("文件: %s\n", TEXT_FILE_PATH);
        printf("显示模式: 滚动显示，避免内存溢出\n");
        
        // 显示初始界面
        filename_ = "小王子 - 流式阅读";
        display_loading_screen();
        
        // 进入流式显示模式
        return enter_streaming_display_mode();
    }
    
    bool enter_streaming_display_mode() {
        printf("\n===== 进入流式显示模式 =====\n");
        printf("每 15 秒滚动显示一段文本内容\n");
        
        // 打开文件
        auto file_handle = sd_.open_file(TEXT_FILE_PATH, "r");
        if (!file_handle.is_ok()) {
            printf("打开文件失败: %s\n", MicroSD::StorageDevice::get_error_description(file_handle.error_code()).c_str());
            return false;
        }
        
        auto& handle = *file_handle;
        
        const size_t BUFFER_SIZE = 512;
        std::string accumulated_text;
        std::vector<std::string> current_lines;  // 只保存当前屏幕的内容
        size_t total_bytes_read = 0;
        int display_count = 0;
        
        // 计算每屏可显示的行数
        int content_height = (LCD_HEIGHT - BOTTOM_MARGIN - 40) - (TOP_MARGIN + 20);
        int max_lines_per_screen = content_height / LINE_HEIGHT;
        
        printf("每屏显示 %d 行文本\n", max_lines_per_screen);
        
        while (true) {
            current_lines.clear();  // 清空当前行缓存
            
            // 读取足够显示一屏的内容
            while (current_lines.size() < max_lines_per_screen) {
                // 如果累积文本中没有足够的行，继续从文件读取
                while (true) {
                    size_t newline_pos = accumulated_text.find('\n');
                    if (newline_pos != std::string::npos) {
                        std::string line = accumulated_text.substr(0, newline_pos);
                        current_lines.push_back(line);
                        accumulated_text = accumulated_text.substr(newline_pos + 1);
                        break;
                    }
                    
                    // 没有完整行，从文件读取更多内容
                    auto read_result = handle.read(BUFFER_SIZE);
                    if (!read_result.is_ok()) {
                        printf("[ERROR] 读取文件失败\n");
                        handle.close();
                        return false;
                    }
                    
                    auto& data = *read_result;
                    if (data.empty()) {
                        // 文件结束
                        if (!accumulated_text.empty()) {
                            current_lines.push_back(accumulated_text);
                            accumulated_text.clear();
                        }
                        printf("\n[INFO] 文件读取完成，总计 %zu 字节\n", total_bytes_read);
                        
                        // 显示最后一屏并结束
                        if (!current_lines.empty()) {
                            display_current_screen(current_lines, ++display_count, true);
                            sleep_ms(15000);
                        }
                        
                        handle.close();
                        printf("文件显示完成，程序结束\n");
                        return true;
                    }
                    
                    std::string text_chunk(data.begin(), data.end());
                    accumulated_text += text_chunk;
                    total_bytes_read += data.size();
                }
            }
            
            // 显示当前屏幕
            display_current_screen(current_lines, ++display_count, false);
            
            printf("[显示] 第 %d 屏，已读取 %zu 字节\n", display_count, total_bytes_read);
            
            // 等待 15 秒后显示下一屏
            sleep_ms(15000);
        }
        
        handle.close();
        return true;
    }
    
    void display_current_screen(const std::vector<std::string>& lines, int screen_num, bool is_last) {
        display_.clear();
        draw_header();
        
        // 显示屏幕编号信息 - 右对齐
        std::string screen_info = "第 " + std::to_string(screen_num) + " 屏";
        if (is_last) {
            screen_info += " (完)";
        }
        int info_width = font_manager_.get_string_width(screen_info);
        font_manager_.draw_string(display_, LCD_WIDTH - SIDE_MARGIN - info_width, TOP_MARGIN + 16, screen_info, true);
        
        // 显示读取进度指示器 - 左侧
        std::string progress_indicator = is_last ? "● 读取完成" : "● 读取中...";
        font_manager_.draw_string(display_, SIDE_MARGIN, TOP_MARGIN + 16, progress_indicator, true);
        
        // 绘制分隔线
        int separator_y = TOP_MARGIN + 32;
        for (int x = SIDE_MARGIN; x < LCD_WIDTH - SIDE_MARGIN; x++) {
            display_.drawPixel(x, separator_y, true);
        }
        
        // 计算内容显示区域 - 参考HybridTextReader的布局
        int content_start_y = separator_y + 10;
        int content_end_y = LCD_HEIGHT - BOTTOM_MARGIN - 40;
        int content_height = content_end_y - content_start_y;
        int max_lines = content_height / LINE_HEIGHT;
        
        // 对传入的lines进行智能换行处理 - 参考HybridTextReader
        std::vector<std::string> all_display_lines;
        for (const std::string& original_line : lines) {
            std::vector<std::string> wrapped_lines = wrap_text_lines(original_line, DISPLAY_WIDTH);
            for (const std::string& wrapped_line : wrapped_lines) {
                all_display_lines.push_back(wrapped_line);
            }
        }
        
        // 显示处理后的文本 - 绝不截断
        int y = content_start_y;
        for (int i = 0; i < max_lines && i < all_display_lines.size(); i++) {
            const std::string& line_text = all_display_lines[i];
            
            // 只绘制非空行，并且确保字符串有效 - 完全参考HybridTextReader
            if (!line_text.empty() && line_text.size() > 0) {
                font_manager_.draw_string(display_, SIDE_MARGIN, y, line_text, true);
            }
            
            y += LINE_HEIGHT;
        }
        
        // 底部状态栏
        int status_y = LCD_HEIGHT - BOTTOM_MARGIN - 15;
        for (int x = SIDE_MARGIN; x < LCD_WIDTH - SIDE_MARGIN; x++) {
            display_.drawPixel(x, status_y - 5, true);
        }
        
        std::string status_text = is_last ? "阅读完成 - 15秒后重新开始" : "15秒后显示下一屏";
        int status_width = font_manager_.get_string_width(status_text);
        font_manager_.draw_string(display_, (LCD_WIDTH - status_width) / 2, status_y, status_text, true);
        
        display_.display();
    }
    
    void display_loading_screen() {
        display_.clear();
        draw_header();
        
        // 居中显示加载信息
        int center_x = LCD_WIDTH / 2;
        int y = LCD_HEIGHT / 2 - 50;
        
        // 显示加载动画风格的文本 - 完整显示不截断
        std::string loading_text = "正在读取 SD 卡文件...";
        int text_width = font_manager_.get_string_width(loading_text);
        font_manager_.draw_string(display_, center_x - text_width/2, y, loading_text, true);
        
        y += LINE_HEIGHT * 2;
        loading_text = "Reading file from SD Card";
        text_width = font_manager_.get_string_width(loading_text);
        font_manager_.draw_string(display_, center_x - text_width/2, y, loading_text, true);
        
        y += LINE_HEIGHT * 2;
        loading_text = "ファイルを読み込み中...";
        text_width = font_manager_.get_string_width(loading_text);
        font_manager_.draw_string(display_, center_x - text_width/2, y, loading_text, true);
        
        y += LINE_HEIGHT * 3;
        loading_text = "请稍候 Please wait しばらくお待ちください";
        text_width = font_manager_.get_string_width(loading_text);
        
        // 如果文本太长，分行显示而不截断
        if (text_width > DISPLAY_WIDTH) {
            std::string line1 = "请稍候 Please wait";
            std::string line2 = "しばらくお待ちください";
            
            int line1_width = font_manager_.get_string_width(line1);
            int line2_width = font_manager_.get_string_width(line2);
            
            font_manager_.draw_string(display_, center_x - line1_width/2, y, line1, true);
            y += LINE_HEIGHT;
            font_manager_.draw_string(display_, center_x - line2_width/2, y, line2, true);
            y += LINE_HEIGHT;
        } else {
            font_manager_.draw_string(display_, center_x - text_width/2, y, loading_text, true);
            y += LINE_HEIGHT * 2;
        }
        
        // 绘制简单的进度条框架
        int bar_width = 200;
        int bar_height = 8;
        int bar_x = center_x - bar_width/2;
        int bar_y = y + LINE_HEIGHT;
        
        // 绘制进度条边框
        for (int x = bar_x; x < bar_x + bar_width; x++) {
            display_.drawPixel(x, bar_y, true);
            display_.drawPixel(x, bar_y + bar_height, true);
        }
        for (int y_line = bar_y; y_line <= bar_y + bar_height; y_line++) {
            display_.drawPixel(bar_x, y_line, true);
            display_.drawPixel(bar_x + bar_width, y_line, true);
        }
        
        // 填充进度条内部动画效果
        int fill_width = 20;  // 小段填充表示加载中
        for (int x = bar_x + 1; x < bar_x + fill_width && x < bar_x + bar_width - 1; x++) {
            for (int y_fill = bar_y + 1; y_fill < bar_y + bar_height; y_fill++) {
                display_.drawPixel(x, y_fill, true);
            }
        }
        
        display_.display();
    }
    

    


    void draw_header() {
        // 显示主标题 - 参考HybridTextReader的简洁风格
        std::string main_title = "MicroSD 文本阅读器 (180°旋转)";
        font_manager_.draw_string(display_, SIDE_MARGIN, SIDE_MARGIN, main_title, true);
        
        // 绘制分割线，只在显示区域内 - 完全参考HybridTextReader
        for (int x = SIDE_MARGIN; x < LCD_WIDTH - SIDE_MARGIN; x++) {
            display_.drawPixel(x, SIDE_MARGIN + 12, true);
        }
    }

    // 页脚显示，支持提示
    void draw_footer(int current_page, const std::string& tip = "") {
        // 确保页码信息有效
        if (total_pages_ <= 0) return;
        
        std::string page_info = "Page " + std::to_string(current_page + 1) + "/" + std::to_string(total_pages_);
        int text_width = display_.getStringWidth(page_info);
        int footer_y = LCD_HEIGHT - BOTTOM_MARGIN - 12;
        
        // 确保页脚位置在有效范围内
        if (footer_y > 0 && footer_y < LCD_HEIGHT) {
            display_.drawString((LCD_WIDTH - text_width) / 2, footer_y, page_info, true);
        }
        
        if (!tip.empty() && tip.size() > 0) {
            int tip_width = display_.getStringWidth(tip);
            int tip_y = footer_y - 16;  // 在页码上方显示提示
            if (tip_y > 0 && tip_y < LCD_HEIGHT) {
                display_.drawString((LCD_WIDTH - tip_width) / 2, tip_y, tip, true);
            }
        }
    }

    // 判断是否为中文字符（UTF-8编码）
    bool is_chinese_char(const std::string& text, size_t pos) {
        if (pos >= text.size()) return false;
        unsigned char c = text[pos];
        // UTF-8中文字符的第一个字节通常在0xE0-0xEF范围内
        return (c >= 0xE0 && c <= 0xEF);
    }
    
    // 获取UTF-8字符的字节长度
    int get_utf8_char_length(const std::string& text, size_t pos) {
        if (pos >= text.size()) return 0;
        unsigned char c = text[pos];
        if (c < 0x80) return 1;        // ASCII字符
        else if (c < 0xE0) return 2;   // 2字节UTF-8
        else if (c < 0xF0) return 3;   // 3字节UTF-8（大部分中文）
        else return 4;                 // 4字节UTF-8
    }
    
    // 智能换行：将文本分割成适合显示宽度的行（支持中英文混合）- 完全参考HybridTextReader
    std::vector<std::string> wrap_text_lines(const std::string& text, int max_width) {
        std::vector<std::string> lines;
        
        if (text.empty()) {
            lines.push_back("");  // 空行用空字符串表示
            return lines;
        }
        
        std::string current_line;
        size_t pos = 0;
        
        while (pos < text.size()) {
            if (is_chinese_char(text, pos)) {
                // 处理中文字符：逐个字符添加
                int char_len = get_utf8_char_length(text, pos);
                std::string chinese_char = text.substr(pos, char_len);
                
                // 测试加上这个中文字符后的宽度
                std::string test_line = current_line + chinese_char;
                int test_width = font_manager_.get_string_width(test_line);
                
                if (test_width <= max_width) {
                    // 可以放在当前行
                    current_line = test_line;
                } else {
                    // 放不下，需要换行
                    if (!current_line.empty()) {
                        lines.push_back(current_line);
                        current_line = chinese_char;
                    } else {
                        // 当前行为空，强制放入
                        current_line = chinese_char;
                    }
                }
                
                pos += char_len;
            } else {
                // 处理英文单词：按空格分割
                size_t next_space = text.find(' ', pos);
                size_t word_end = (next_space == std::string::npos) ? text.size() : next_space;
                
                // 检查是否遇到中文字符
                for (size_t i = pos; i < word_end; i++) {
                    if (is_chinese_char(text, i)) {
                        word_end = i;
                        break;
                    }
                }
                
                std::string word = text.substr(pos, word_end - pos);
                
                // 计算加上这个英文单词后的行宽度
                std::string test_line = current_line;
                if (!test_line.empty() && !word.empty() && word[0] != ' ') {
                    test_line += " ";
                }
                test_line += word;
                
                int test_width = font_manager_.get_string_width(test_line);
                
                if (test_width <= max_width) {
                    // 这个词可以放在当前行
                    current_line = test_line;
                } else {
                    // 这个词放不下，需要换行 - 绝不截断
                    if (!current_line.empty()) {
                        lines.push_back(current_line);
                        current_line = word;
                    } else {
                        // 当前行为空但词太长，仍然完整放入 - 绝不截断
                        current_line = word;
                    }
                }
                
                // 移动到下一个位置
                pos = word_end;
                if (pos < text.size() && text[pos] == ' ') {
                    pos++;  // 跳过空格
                }
            }
        }
        
        // 添加最后一行
        if (!current_line.empty()) {
            lines.push_back(current_line);
        }
        
        return lines;
    }

    void show_static_page(int page, const std::string& tip = "") {
        display_.clear();
        draw_header();
        
        // 计算实际内容显示区域
        int content_start_y = TOP_MARGIN + 16;  // 标题下方开始
        int content_end_y = LCD_HEIGHT - BOTTOM_MARGIN - 32;  // 页脚上方结束
        int content_height = content_end_y - content_start_y;
        int max_lines = content_height / LINE_HEIGHT;
        
        // 计算显示宽度（减去左右留白）
        int display_width = DISPLAY_WIDTH;
        
        // 构建所有换行后的文本行
        std::vector<std::string> all_display_lines;
        for (const std::string& original_line : text_content_) {
            std::vector<std::string> wrapped_lines = wrap_text_lines(original_line, display_width);
            for (const std::string& wrapped_line : wrapped_lines) {
                all_display_lines.push_back(wrapped_line);
            }
        }
        
        // 计算当前页的起始行
        int start_line = page * max_lines;
        int y = content_start_y;
        
        // 绘制当前页的文本
        for (int i = 0; i < max_lines && (start_line + i) < all_display_lines.size(); i++) {
            const std::string& line_text = all_display_lines[start_line + i];
            
            // 只绘制非空行，并且确保字符串有效
            if (!line_text.empty() && line_text.size() > 0) {
                font_manager_.draw_string(display_, SIDE_MARGIN, y, line_text, true);
            }
            
            y += LINE_HEIGHT;
        }
        
        draw_footer(page, tip);
        display_.display();
    }

    int calculate_total_pages_by_simulation() {
        // 计算实际内容显示区域
        int content_start_y = TOP_MARGIN + 16;
        int content_end_y = LCD_HEIGHT - BOTTOM_MARGIN - 32;
        int content_height = content_end_y - content_start_y;
        int max_lines_per_page = content_height / LINE_HEIGHT;
        int display_width = DISPLAY_WIDTH;
        
        // 构建所有换行后的文本行
        std::vector<std::string> all_display_lines;
        for (const std::string& original_line : text_content_) {
            std::vector<std::string> wrapped_lines = wrap_text_lines(original_line, display_width);
            for (const std::string& wrapped_line : wrapped_lines) {
                all_display_lines.push_back(wrapped_line);
            }
        }
        
        // 计算总页数
        int total_pages = (all_display_lines.size() + max_lines_per_page - 1) / max_lines_per_page;
        return total_pages > 0 ? total_pages : 1;
    }
    

    
    void display_error_screen(const std::string& error_msg) {
        display_.clear();
        draw_header();
        
        // 居中显示错误图标和标题
        int center_x = LCD_WIDTH / 2;
        int y = LCD_HEIGHT / 2 - 70;
        
        std::string error_title = "❌ 系统错误";
        int title_width = font_manager_.get_string_width(error_title);
        font_manager_.draw_string(display_, center_x - title_width/2, y, error_title, true);
        
        y += LINE_HEIGHT * 2;
        
        // 错误框架
        int box_width = 260;
        int box_height = 100;
        int box_x = center_x - box_width/2;
        int box_y = y;
        
        // 绘制错误框
        for (int x = box_x; x < box_x + box_width; x++) {
            display_.drawPixel(x, box_y, true);
            display_.drawPixel(x, box_y + box_height, true);
        }
        for (int y_line = box_y; y_line <= box_y + box_height; y_line++) {
            display_.drawPixel(box_x, y_line, true);
            display_.drawPixel(box_x + box_width, y_line, true);
        }
        
        // 显示错误消息 - 完整显示不截断
        y += 15;
        int max_msg_width = box_width - 20;
        
        // 使用智能换行处理错误消息，而不是截断
        std::vector<std::string> error_lines = wrap_text_lines(error_msg, max_msg_width);
        
        for (const std::string& line : error_lines) {
            if (y > box_y + box_height - 20) break;  // 避免超出框框
            
            int line_width = font_manager_.get_string_width(line);
            font_manager_.draw_string(display_, center_x - line_width/2, y, line, true);
            y += LINE_HEIGHT;
        }
        
        // 建议信息
        y = box_y + box_height + 15;
        std::string suggestion = "请检查 SD 卡连接和格式";
        int sug_width = font_manager_.get_string_width(suggestion);
        font_manager_.draw_string(display_, center_x - sug_width/2, y, suggestion, true);
        
        y += LINE_HEIGHT;
        std::string suggestion2 = "Check SD card connection";
        int sug2_width = font_manager_.get_string_width(suggestion2);
        font_manager_.draw_string(display_, center_x - sug2_width/2, y, suggestion2, true);
        
        // 底部重试提示
        y = LCD_HEIGHT - BOTTOM_MARGIN - 30;
        std::string retry_msg = "程序将在 5 秒后结束";
        int retry_width = font_manager_.get_string_width(retry_msg);
        font_manager_.draw_string(display_, center_x - retry_width/2, y, retry_msg, true);
        
        display_.display();
        
        // 显示5秒后退出，不再无限循环
        for (int i = 5; i > 0; i--) {
            sleep_ms(1000);
            printf("[错误] %s - %d 秒后程序退出\n", error_msg.c_str(), i);
        }
    }

public:
    void run() {
        printf("\n===== 开始 MicroSD 专项测试 =====\n");
        
        // 初始化 MicroSD 卡
        sd_ready_ = initialize_microsd();
        
        if (!sd_ready_) {
            printf("[ERROR] SD 卡初始化失败，尝试重试最多3次...\n");
            // 有限次数重试 SD 卡初始化
            for (int retry = 1; retry <= 3; retry++) {
                printf("第 %d 次重试 SD 卡初始化...\n", retry);
                sleep_ms(2000);
                sd_ready_ = initialize_microsd();
                if (sd_ready_) {
                    printf("[SUCCESS] SD 卡重试初始化成功！\n");
                    break;
                }
                printf("第 %d 次重试失败\n", retry);
            }
        }
        
        // 流式显示文件内容
        printf("\n===== 测试流式文件显示 =====\n");
        bool display_success = display_file_streaming();
        
        // 最终测试报告
        printf("\n===== 最终测试报告 =====\n");
        printf("SD卡状态: %s\n", sd_ready_ ? "正常" : "异常");
        printf("文件显示: %s\n", display_success ? "成功完成" : "失败");
        
        if (sd_ready_ && display_success) {
            printf("[SUCCESS] MicroSD 流式阅读器运行成功！\n");
            printf("文件已完整显示，程序正常结束。\n");
        } else if (sd_ready_ && !display_success) {
            printf("[PARTIAL] SD卡初始化成功，但文件显示失败。\n");
            printf("请检查文件 '%s' 是否存在于SD卡根目录。\n", TEXT_FILE_PATH);
            // 显示错误信息
            display_error_screen("文件读取失败");
        } else {
            printf("[FAILED] SD卡初始化失败。\n");
            printf("请检查SD卡连接、格式和引脚配置。\n");
            // 显示错误信息
            display_error_screen("SD卡初始化失败");
        }
    }

    MicroSDTextReader() : 
        display_(PIN_DC, PIN_RST, PIN_CS, PIN_SCLK, PIN_SDIN),
        font_manager_(),
        sd_(),
        current_page_(0),
        filename_("小王子 - 流式阅读器"),
        current_mode_(st7306::DisplayMode::Day),
        sd_ready_(false) {
        
        // 初始化显示系统
        initialize_hardware();
    }
};

int main() {
    // 初始化串口
    stdio_init_all();
    sleep_ms(3000); // 延长等待时间确保串口连接稳定
    
    // 输出启动信息
    printf("\n\n===== 程序启动 =====\n");
    printf("MicroSD 流式文本阅读器\n");
    printf("目标文件: '%s'\n", TEXT_FILE_PATH);
    printf("功能特性: 内存优化流式读取显示\n");
    printf("显示方式: 每屏显示15秒，自动滚动\n");
    printf("输出方式: 屏幕显示 + 串口日志\n");
    printf("特点: 避免内存溢出，适合大文件\n");
    printf("===================================\n");
    
    // 系统信息
    printf("\n[INFO] 系统启动完成\n");
    printf("[INFO] 开始创建 MicroSDTextReader 对象...\n");
    
    MicroSDTextReader reader;
    printf("[INFO] 对象创建成功，开始运行测试...\n");
    reader.run();
    printf("[INFO] 测试运行完成\n");
    
    printf("[INFO] 程序即将退出\n");
    return 0;
} 