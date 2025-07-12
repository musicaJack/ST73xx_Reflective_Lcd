#include "st7306_driver.hpp"
#include "joystick.hpp"
#include "st73xx_font.hpp"
#include "pico/stdlib.h"
#include <string>
#include <vector>
#include <cmath>
// #include "the_little_prince_content.hpp"  // 暂时注释掉

// 显示配置
#define LCD_WIDTH 300
#define LCD_HEIGHT 400
#define SIDE_MARGIN 16
#define TOP_MARGIN 16
#define BOTTOM_MARGIN 16
#define CHAR_WIDTH 8
#define LINE_HEIGHT 16

// 硬件引脚定义
#define PIN_DC   20
#define PIN_RST  15
#define PIN_CS   17
#define PIN_SCLK 18
#define PIN_SDIN 19

// I2C配置
#define JOYSTICK_ADDR 0x63
#define PIN_SDA 6
#define PIN_SCL 7

// LED颜色定义（需与joystick固件一致）
#define JOYSTICK_LED_OFF   0x00000000
#define JOYSTICK_LED_RED   0x00FF0000
#define JOYSTICK_LED_GREEN 0x0000FF00
#define JOYSTICK_LED_BLUE  0x000000FF

// 死区阈值
constexpr int16_t JOY_DEADZONE = 1000;

// 临时文本内容（替代the_little_prince_content）
const std::vector<std::string> text_content = {
    "The Little Prince",
    "",
    "Once upon a time, there was a little prince who lived on a planet that was scarcely any bigger than himself.",
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
    "And so the little prince learned that the most beautiful things in the world cannot be seen or touched.",
    "",
    "They must be felt with the heart.",
    "",
    "The End."
};

const int CHARS_PER_LINE = (LCD_WIDTH - 2 * SIDE_MARGIN) / CHAR_WIDTH; // 33
const int LINES_PER_PAGE = (LCD_HEIGHT - TOP_MARGIN - BOTTOM_MARGIN) / LINE_HEIGHT; // 23

class TextReader {
private:
    st7306::ST7306Driver display_;
    Joystick joystick_;
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
        // 显示当前模式提示
        std::string mode_tip;
        switch (current_mode_) {
            case st7306::DisplayMode::Day:
                mode_tip = "切换到日间模式";
                break;
            case st7306::DisplayMode::Night:
                mode_tip = "切换到夜间模式";
                break;
        }
        show_static_page(current_page_, mode_tip);
    }

    void initialize_hardware() {
        display_.initialize();
        // 严格遵循st7306_demo.cpp的初始化顺序 - 只调用clearDisplay
        display_.clearDisplay();
        
        joystick_.begin(i2c1, JOYSTICK_ADDR, PIN_SDA, PIN_SCL, 100000);
        // 绿灯亮1秒表示初始化成功
        joystick_.set_rgb_color(JOYSTICK_LED_GREEN);
        sleep_ms(1000);
        joystick_.set_rgb_color(JOYSTICK_LED_OFF);
    }

    void draw_header() {
        display_.drawString(0, 0, filename_, true);
        for (int x = 0; x < LCD_WIDTH; x++) {
            display_.drawPixel(x, 12, true);
        }
    }

    // 页脚显示，支持提示
    void draw_footer(int current_page, const std::string& tip = "") {
        std::string page_info = "Page " + std::to_string(current_page + 1) + "/" + std::to_string(total_pages_);
        int text_width = display_.getStringWidth(page_info);
        display_.drawString((LCD_WIDTH - text_width) / 2, LCD_HEIGHT - 12, page_info, true);
        if (!tip.empty()) {
            int tip_width = display_.getStringWidth(tip);
            display_.drawString((LCD_WIDTH - tip_width) / 2, LCD_HEIGHT - 28, tip, true);
        }
    }

    // 辅助结构体：页起始位置
    struct PagePos { int line; int char_idx; };
    // 计算每一页的起始行和字符索引
    PagePos get_page_start_pos(int page) {
        int line = 0, char_idx = 0, lines_shown = 0;
        bool new_paragraph = true;
        while (page > 0 && line < text_content.size()) {
            const std::string& linetxt = text_content[line];
            if (linetxt.empty()) {
                lines_shown++;
                new_paragraph = true;
                if (lines_shown >= LINES_PER_PAGE) {
                    lines_shown = 0;
                    --page;
                }
                ++line;
                continue;
            }
            size_t pos = 0;
            int x = SIDE_MARGIN;
            if (new_paragraph) {
                x += 2 * CHAR_WIDTH;
                new_paragraph = false;
            }
            while (pos < linetxt.size()) {
                size_t next_space = linetxt.find(' ', pos);
                size_t word_end = (next_space == std::string::npos) ? linetxt.size() : next_space + 1;
                std::string word = linetxt.substr(pos, word_end - pos);
                int word_width = CHAR_WIDTH * word.size();
                bool is_punct = (word.size() == 1 && std::string(".,?!:;\"'").find(word[0]) != std::string::npos);
                int max_x = LCD_WIDTH - SIDE_MARGIN;
                if (is_punct) max_x += CHAR_WIDTH;
                if (x + word_width > max_x) {
                    x = SIDE_MARGIN;
                    lines_shown++;
                    if (lines_shown >= LINES_PER_PAGE) {
                        lines_shown = 0;
                        --page;
                        if (page == 0) return {line, (int)pos};
                    }
                }
                x += word_width;
                pos = word_end;
            }
            lines_shown++;
            if (lines_shown >= LINES_PER_PAGE) {
                lines_shown = 0;
                --page;
            }
            ++line;
        }
        return {line, 0};
    }

    // 整页静态显示（不流式）
    void show_static_page(int page, const std::string& tip = "") {
        display_.clear();
        draw_header();
        int y = TOP_MARGIN;
        int lines_shown = 0;
        bool new_paragraph = true;
        PagePos posinfo = get_page_start_pos(page);
        int line = posinfo.line;
        int char_idx = posinfo.char_idx;
        for (; line < text_content.size(); ++line) {
            const std::string& linetxt = text_content[line];
            if (linetxt.empty()) {
                y += LINE_HEIGHT;
                lines_shown++;
                new_paragraph = true;
                if (lines_shown >= LINES_PER_PAGE) break;
                continue;
            }
            size_t pos = char_idx;
            int x = SIDE_MARGIN;
            if (new_paragraph) {
                x += 2 * CHAR_WIDTH;
                new_paragraph = false;
            }
            while (pos < linetxt.size()) {
                size_t next_space = linetxt.find(' ', pos);
                size_t word_end = (next_space == std::string::npos) ? linetxt.size() : next_space + 1;
                std::string word = linetxt.substr(pos, word_end - pos);
                int word_width = display_.getStringWidth(word);
                bool is_punct = (word.size() == 1 && std::string(".,?!:;\"'").find(word[0]) != std::string::npos);
                int max_x = LCD_WIDTH - SIDE_MARGIN;
                if (is_punct) max_x += CHAR_WIDTH;
                if (x + word_width > max_x) {
                    x = SIDE_MARGIN;
                    y += LINE_HEIGHT;
                    lines_shown++;
                    if (lines_shown >= LINES_PER_PAGE) break;
                }
                display_.drawString(x, y, word, true);
                x += word_width;
                pos = word_end;
            }
            if (lines_shown >= LINES_PER_PAGE) break;
            x = SIDE_MARGIN;
            y += LINE_HEIGHT;
            lines_shown++;
            if (lines_shown >= LINES_PER_PAGE) break;
            char_idx = 0;
        }
        draw_footer(page, tip);
        display_.display();
    }

    // 统计实际总页数（用分页模拟法，保证和显示逻辑一致）
    int calculate_total_pages_by_simulation() {
        int page = 0;
        while (true) {
            PagePos pos = get_page_start_pos(page);
            if (pos.line >= text_content.size()) break;
            page++;
        }
        return page;
    }

public:
    void run() {
        current_page_ = 0;
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
            
            // 按键切换显示模式
            static bool last_button_state = false;
            if (button_pressed && !last_button_state) {
                toggleDisplayMode();
                wait_joystick_center();
            }
            last_button_state = button_pressed;
            
            sleep_ms(30);
        }
    }

    TextReader() : 
        display_(PIN_DC, PIN_RST, PIN_CS, PIN_SCLK, PIN_SDIN),
        current_page_(0),
        filename_("The Little Prince.txt"),
        current_mode_(st7306::DisplayMode::Day) {
        initialize_hardware();
        total_pages_ = calculate_total_pages_by_simulation();
    }
};

int main() {
    stdio_init_all();
    TextReader reader;
    reader.run();
    return 0;
} 