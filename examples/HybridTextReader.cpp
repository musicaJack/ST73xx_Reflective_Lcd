#include "st7306_driver.hpp"
#include "joystick.hpp"
#include "hybrid_font_system.hpp"
#include "hybrid_font_renderer.hpp"
#include "spi_config.hpp"
#include "pico/stdlib.h"
#include <string>
#include <vector>
#include <cmath>

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

// 混合文本内容（中英日文混合）
const std::vector<std::string> text_content = {
    "小王子 The Little Prince 星の王子さま",
    "",
    "=== 中文版 ===",
    "",
    "从前，有一个小王子住在一颗比他身体大不了多少的小行星上。",
    "",
    "他需要一只绵羊来吃那些威胁要占领他小小世界的猴面包树。",
    "",
    "于是他踏上了寻找的旅程。",
    "",
    "在路上，他拜访了许多星球，遇到了很多奇怪的人。",
    "",
    "但是没有人能给他真正需要的东西。",
    "",
    "最后，他来到了地球，在沙漠中遇到了一个坠机的飞行员。",
    "",
    "飞行员帮助他理解了生活中真正重要的东西。",
    "",
    "小王子明白了世界上最美的东西是看不见也摸不着的。",
    "",
    "它们必须用心去感受。",
    "",
    "=== English Version ===",
    "",
    "Once upon a time, there was a little prince who lived on a planet scarcely bigger than himself.",
    "",
    "He needed a sheep to eat the baobab trees that threatened to overrun his tiny world.",
    "",
    "So he set out on a journey to find one.",
    "",
    "Along the way, he visited many planets and met many strange people.",
    "",
    "But none of them could give him what he really needed.",
    "",
    "Finally, he came to Earth, where he met a pilot who had crashed in the desert.",
    "",
    "The pilot helped him understand what was truly important in life.",
    "",
    "The little prince learned that the most beautiful things cannot be seen or touched.",
    "",
    "They must be felt with the heart.",
    "",
    "=== 日本語版 ===",
    "",
    "昔々、自分の体よりも少し大きい小さな星に住む王子様がいました。",
    "",
    "彼は小さな世界を占領しようとするバオバブの木を食べてくれる羊を必要としていました。",
    "",
    "そこで彼は羊を探す旅に出ました。",
    "",
    "道中、彼は多くの星を訪れ、たくさんの変わった人々に出会いました。",
    "",
    "しかし、誰も彼が本当に必要としているものを与えることはできませんでした。",
    "",
    "最後に、彼は地球にやって来て、砂漠で墜落したパイロットに出会いました。",
    "",
    "パイロットは彼が人生で本当に大切なものを理解する手助けをしました。",
    "",
    "王子様は世界で最も美しいものは目に見えず、触れることもできないということを学びました。",
    "",
    "それらは心で感じなければならないのです。",
    "",
    "=== 结束 The End おわり ==="
};

class HybridTextReader {
private:
    st7306::ST7306Driver display_;
    Joystick joystick_;
    hybrid_font::FontManager<st7306::ST7306Driver> font_manager_;
    
    int current_page_;
    int total_pages_;
    std::string filename_;
    st7306::DisplayMode current_mode_;
    

    
    // 方向判断
    int determine_joystick_direction(int16_t x, int16_t y) {
        int16_t abs_x = std::abs(x);
        int16_t abs_y = std::abs(y);
        if (abs_y > abs_x * 1.2 && abs_y > JOY_DEADZONE) {
            return (y < 0) ? 1 : 2;  // 1=up, 2=down
        }
        if (abs_x > abs_y * 1.2 && abs_x > JOY_DEADZONE) {
            return (x < 0) ? 3 : 4;  // 3=left, 4=right
        }
        return 0;  // center
    }

    // 等待摇杆回中
    void wait_joystick_center() {
        while (true) {
            int16_t x = joystick_.get_joy_adc_12bits_offset_value_x();
            int16_t y = joystick_.get_joy_adc_12bits_offset_value_y();
            if (std::abs(x) < JOY_DEADZONE && std::abs(y) < JOY_DEADZONE) break;
            sleep_ms(10);
        }
    }

    // 显示模式切换
    void toggleDisplayMode() {
        switch (current_mode_) {
            case st7306::DisplayMode::Day:
                current_mode_ = st7306::DisplayMode::Night;
                break;
            case st7306::DisplayMode::Night:
                current_mode_ = st7306::DisplayMode::Day;
                break;
        }
        display_.setDisplayMode(current_mode_);
        
        std::string mode_tip;
        switch (current_mode_) {
            case st7306::DisplayMode::Day:
                mode_tip = "日间模式";
                break;
            case st7306::DisplayMode::Night:
                mode_tip = "夜间模式";
                break;
        }
        show_static_page(current_page_, mode_tip);
        
        // 延迟后清除提示信息
        sleep_ms(1500);  // 显示1.5秒
        show_static_page(current_page_, "");  // 清除提示
    }

    void initialize_hardware() {
        printf("Initializing ST7306 display...\n");
        display_.initialize();
        printf("Display initialized.\n");
        
        // 严格遵循st7306_demo.cpp的初始化顺序 - 只调用clearDisplay
        display_.clearDisplay();
        
        joystick_.begin(i2c1, JOYSTICK_ADDR, PIN_SDA, PIN_SCL, I2C_FREQUENCY);
        
        // 初始化混合字体系统
        if (!font_manager_.initialize()) {
            printf("[ERROR] 混合字体系统初始化失败\n");
            joystick_.set_rgb_color(JOYSTICK_LED_RED);
            sleep_ms(2000);
        } else {
            printf("[SUCCESS] 混合字体系统初始化成功\n");
            font_manager_.print_status();
            joystick_.set_rgb_color(JOYSTICK_LED_GREEN);
            sleep_ms(1000);
        }
        
        joystick_.set_rgb_color(JOYSTICK_LED_OFF);
    }

    void draw_header() {
        // 在留白区域内绘制标题
        display_.drawString(SIDE_MARGIN, SIDE_MARGIN, filename_, true);
        // 绘制分割线，只在显示区域内
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
    
    // 智能换行：将文本分割成适合显示宽度的行（支持中英文混合）
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
                    // 这个词放不下，需要换行
                    if (!current_line.empty()) {
                        lines.push_back(current_line);
                        current_line = word;
                    } else {
                        // 当前行为空但词太长，强制放入
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
        for (const std::string& original_line : text_content) {
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
        for (const std::string& original_line : text_content) {
            std::vector<std::string> wrapped_lines = wrap_text_lines(original_line, display_width);
            for (const std::string& wrapped_line : wrapped_lines) {
                all_display_lines.push_back(wrapped_line);
            }
        }
        
        // 计算总页数
        int total_pages = (all_display_lines.size() + max_lines_per_page - 1) / max_lines_per_page;
        return total_pages > 0 ? total_pages : 1;
    }

public:
    void run() {
        current_page_ = 0;
        
        // 计算页面参数 - 使用新的智能换行算法
        total_pages_ = calculate_total_pages_by_simulation();
        
        // 计算实际每页行数
        int content_height = (LCD_HEIGHT - BOTTOM_MARGIN - 32) - (TOP_MARGIN + 16);
        int max_lines_per_page = content_height / LINE_HEIGHT;
        
        printf("[INFO] 显示配置: 屏幕留白 %d 像素，显示区域 %dx%d 像素\n", 
               SCREEN_MARGIN, DISPLAY_WIDTH, DISPLAY_HEIGHT);
        printf("[INFO] 页面配置: 每页最多 %d 行，总共 %d 页\n", max_lines_per_page, total_pages_);
        
        show_static_page(current_page_);
        
        while (true) {
            int16_t x = joystick_.get_joy_adc_12bits_offset_value_x();
            int16_t y = joystick_.get_joy_adc_12bits_offset_value_y();
            bool button_pressed = joystick_.get_button_value();
            
            int direction = determine_joystick_direction(x, y);
            
            if (direction == 1) { // 上
                if (current_page_ > 0) {
                    current_page_--;
                    show_static_page(current_page_);
                } else {
                    show_static_page(current_page_, "已到首页");
                }
                wait_joystick_center();
            } else if (direction == 2) { // 下
                if (current_page_ < total_pages_ - 1) {
                    current_page_++;
                    show_static_page(current_page_);
                } else {
                    show_static_page(current_page_, "已到末页");
                }
                wait_joystick_center();
            }
            
            // 按键切换显示模式 - 与text_reader.cpp保持一致的逻辑
            static bool last_button_state = false;
            if (button_pressed && !last_button_state) {
                toggleDisplayMode();
                wait_joystick_center();
            }
            last_button_state = button_pressed;
            
            sleep_ms(30);  // 与text_reader.cpp保持一致
        }
    }

    HybridTextReader() : 
        display_(PIN_DC, PIN_RST, PIN_CS, PIN_SCLK, PIN_SDIN),
        font_manager_(),
        current_page_(0),
        filename_("小王子 The Little Prince 星の王子さま"),
        current_mode_(st7306::DisplayMode::Day) {
        
        initialize_hardware();
    }
};

int main() {
    stdio_init_all();
    
    printf("=== 混合字体文本阅读器启动 ===\n");
    printf("支持中英日文混合显示\n");
    printf("ASCII字符使用8x16字体，中文/日文字符使用16x16字体\n");
    
    HybridTextReader reader;
    reader.run();
    
    return 0;
} 