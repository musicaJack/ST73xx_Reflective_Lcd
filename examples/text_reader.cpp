#include "st7306_driver.hpp"
#include "joystick.hpp"
#include "st73xx_font.hpp"
#include "pico/stdlib.h"
#include <string>
#include <vector>
#include <cmath>

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

// 文本内容
const std::vector<std::string> text_content = {
    "THE LITTLE PRINCE",
    "",
    "Antoine De Saint-Exupery",
    "",
    "Antoine de Saint-Exupery, who was a French author, journalist and pilot wrote",
    "The Little Prince in 1943, one year before his death.",
    "",
    "The Little Prince appears to be a simple children's tale,",
    "some would say that it is actually a profound and deeply moving tale,",
    "written in riddles and laced with philosophy and poetic metaphor.",
    "",
    "Once when I was six years old I saw a magnificent picture in a book, called True Stories from",
    "Nature, about the primeval forest. It was a picture of a boa constrictor in the act of swallowing an",
    "animal. Here is a copy of the drawing.",
    "",
    "In the book it said: \"Boa constrictors swallow their prey whole, without chewing it. After that they",
    "are not able to move, and they sleep through the six months that they need for digestion.\" I",
    "pondered deeply, then, over the adventures of the jungle. And after some work with a coloured",
    "pencil I succeeded in making my first drawing. My Drawing Number One. It looked like this:",
    "",
    "I showed my masterpiece to the grown-ups, and asked them whether the drawing frightened them.",
    "But they answered: \"Frighten? Why should any one be frightened by a hat?\" My drawing was not",
    "a picture of a hat. It was a picture of a boa constrictor digesting an elephant. But since the grown-",
    "ups were not able to understand it, I made another drawing: I drew the inside of the boa",
    "constrictor, so that the grown-ups could see it clearly. They always need to have things explained.",
    "My Drawing Number Two looked like this: ",
    "The grown-ups' response, this time, was to advise me to lay aside my drawings of boa constrictors, "
    "whether from the inside or the outside. They told me that real constrictors in the wild do not eat "
    "anything except monkeys and little children; as a rule, they are too large to be eaten. Furthermore, "
    "these drawings upset them to such a degree that they totally lost their appetite. I was lucky enough "
    "to have a relative who was a tax collector. When he came to visit us, my parents would be sure to ask "
    "him to take a look at my Drawing Number Two. He would always say, \"That's one of the best drawings "
    "I've ever seen! You must have a great deal of talent. Draw me quickly something else…\" "
    "I was then very small. I had to get to my room at night, and lock the door so that I should not be "
    "disturbed. I had to sleep on the floor. I had to draw again and again. To those grown-ups who "
    "might read this book, I offer my excuses. The drawings, they are quite good drawings, but the "
    "words, that is to say my words, are not very good words.",
    "Now I had to draw a sheep. That was difficult.",
    "First I drew four good ones, but the grown-ups said that they were not sheep at all. They "
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

    void initialize_hardware() {
        display_.initialize();
        display_.clear();
        display_.displayOn(true);
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
        int current_page = 0;
        show_static_page(current_page);
        while (true) {
            int16_t x = joystick_.get_joy_adc_12bits_offset_value_x();
            int16_t y = joystick_.get_joy_adc_12bits_offset_value_y();
            int direction = determine_joystick_direction(x, y);
            if (direction == 1) { // 上
                if (current_page > 0) {
                    current_page--;
                    show_static_page(current_page);
                } else {
                    show_static_page(current_page, "已到首页");
                }
                wait_joystick_center();
            } else if (direction == 2) { // 下
                if (current_page < total_pages_ - 1) {
                    current_page++;
                    show_static_page(current_page);
                } else {
                    show_static_page(current_page, "已到末页");
                }
                wait_joystick_center();
            }
            sleep_ms(30);
        }
    }

    TextReader() : 
        display_(PIN_DC, PIN_RST, PIN_CS, PIN_SCLK, PIN_SDIN),
        current_page_(0),
        filename_("The Little Prince.txt") {
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