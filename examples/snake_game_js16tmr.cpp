#include <stdio.h>
#include <cstdlib>
#include <vector>
#include <random>
#include <algorithm>
#include "pico/stdlib.h"
#include "st7306_driver.hpp"
#include "pico_display_gfx.hpp"
#include "js16tmr_joystick/js16tmr_joystick_direct.hpp"
#include "js16tmr_joystick/js16tmr_joystick_handler.hpp"
#include "spi_config.hpp"

// ==================== 游戏设置 ====================
// 屏幕尺寸
#define SCREEN_WIDTH  300
#define SCREEN_HEIGHT 400

// 游戏区域设置
#define GAME_AREA_X     10
#define GAME_AREA_Y     50
#define GAME_AREA_WIDTH 280
#define GAME_AREA_HEIGHT 300

// 网格设置
#define GRID_SIZE 10  // 每个格子的大小
#define GRID_COLS (GAME_AREA_WIDTH / GRID_SIZE)
#define GRID_ROWS (GAME_AREA_HEIGHT / GRID_SIZE)

// 游戏状态
enum GameState {
    GAME_MENU,
    GAME_PLAYING,
    GAME_PAUSED,
    GAME_OVER
};

// 方向定义
enum Direction {
    DIR_NONE = -1,
    DIR_UP = 0,
    DIR_DOWN = 1,
    DIR_LEFT = 2,
    DIR_RIGHT = 3
};

// 位置结构
struct Position {
    int x, y;
    Position(int x = 0, int y = 0) : x(x), y(y) {}
    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
};

// ==================== 全局变量 ====================
st7306::ST7306Driver display(TFT_PIN_DC, TFT_PIN_RST, TFT_PIN_CS, TFT_PIN_SCK, TFT_PIN_MOSI);
pico_gfx::PicoDisplayGFX<st7306::ST7306Driver> gfx(display, SCREEN_WIDTH, SCREEN_HEIGHT);

// JS16TMR摇杆
JS16TMRJoystickDirect joystick;
js16tmr::JS16TMRJoystickHandler joystick_handler;

// 游戏变量
GameState game_state = GAME_MENU;
std::vector<Position> snake;
Direction current_direction = DIR_RIGHT;
Direction next_direction = DIR_RIGHT;
Position food;
int score = 0;
int high_score = 0;
bool button_pressed = false;
absolute_time_t last_move_time;
absolute_time_t last_input_time;

// 随机数生成器
std::random_device rd;
std::mt19937 gen(rd());

// ==================== 游戏函数 ====================

// 前向声明
void generateFood();

// 初始化游戏
void initGame() {
    printf("初始化贪吃蛇游戏...\n");
    
    // 初始化蛇身（从中心开始，长度为3）
    snake.clear();
    int center_x = GRID_COLS / 2;
    int center_y = GRID_ROWS / 2;
    
    snake.push_back(Position(center_x, center_y));
    snake.push_back(Position(center_x - 1, center_y));
    snake.push_back(Position(center_x - 2, center_y));
    
    current_direction = DIR_RIGHT;
    next_direction = DIR_RIGHT;
    score = 0;
    
    // 生成第一个食物
    generateFood();
    
    last_move_time = get_absolute_time();
    last_input_time = get_absolute_time();
    
    printf("游戏初始化完成，蛇长度: %d\n", (int)snake.size());
}

// 生成食物
void generateFood() {
    std::uniform_int_distribution<> dis_x(0, GRID_COLS - 1);
    std::uniform_int_distribution<> dis_y(0, GRID_ROWS - 1);
    
    do {
        food.x = dis_x(gen);
        food.y = dis_y(gen);
    } while (std::find(snake.begin(), snake.end(), food) != snake.end());
    
    printf("生成食物位置: (%d, %d)\n", food.x, food.y);
}

// 处理摇杆输入
Direction getJoystickDirection() {
    js16tmr::JoystickDirection direction = joystick_handler.getCurrentDirection();
    
    // 调试信息（只在方向改变时输出）
    static js16tmr::JoystickDirection last_direction = js16tmr::JoystickDirection::CENTER;
    if (direction != last_direction) {
        const char* dir_names[] = {"CENTER", "UP", "DOWN", "LEFT", "RIGHT"};
        printf("摇杆方向: %s\n", dir_names[(int)direction]);
        last_direction = direction;
    }
    
    switch (direction) {
        case js16tmr::JoystickDirection::UP:    return DIR_UP;
        case js16tmr::JoystickDirection::DOWN:  return DIR_DOWN;
        case js16tmr::JoystickDirection::LEFT:  return DIR_LEFT;
        case js16tmr::JoystickDirection::RIGHT: return DIR_RIGHT;
        default: return DIR_NONE;
    }
}

// 检查方向是否有效（不能直接反向）
bool isValidDirection(Direction new_dir) {
    if (new_dir == DIR_NONE) return false;
    
    // 检查是否与当前方向相反
    if ((current_direction == DIR_UP && new_dir == DIR_DOWN) ||
        (current_direction == DIR_DOWN && new_dir == DIR_UP) ||
        (current_direction == DIR_LEFT && new_dir == DIR_RIGHT) ||
        (current_direction == DIR_RIGHT && new_dir == DIR_LEFT)) {
        return false;
    }
    
    return true;
}

// 移动蛇
void moveSnake() {
    Position head = snake[0];
    Position new_head = head;
    
    // 根据方向计算新头部位置
    switch (next_direction) {
        case DIR_UP:    new_head.y--; break;
        case DIR_DOWN:  new_head.y++; break;
        case DIR_LEFT:  new_head.x--; break;
        case DIR_RIGHT: new_head.x++; break;
    }
    
    // 检查边界碰撞
    if (new_head.x < 0 || new_head.x >= GRID_COLS || 
        new_head.y < 0 || new_head.y >= GRID_ROWS) {
        printf("撞墙！游戏结束\n");
        game_state = GAME_OVER;
        if (score > high_score) {
            high_score = score;
        }
        return;
    }
    
    // 检查自身碰撞
    for (const Position& segment : snake) {
        if (new_head == segment) {
            printf("撞到自己！游戏结束\n");
            game_state = GAME_OVER;
            if (score > high_score) {
                high_score = score;
            }
            return;
        }
    }
    
    // 移动蛇身
    snake.insert(snake.begin(), new_head);
    current_direction = next_direction;
    
    // 检查是否吃到食物
    if (new_head == food) {
        printf("吃到食物！得分: %d\n", ++score);
        generateFood();
    } else {
        // 没吃到食物，移除尾部
        snake.pop_back();
    }
}

// 绘制游戏区域
void drawGameArea() {
    // 绘制边框（黑色）
    gfx.drawRectangle(GAME_AREA_X, GAME_AREA_Y, GAME_AREA_WIDTH, GAME_AREA_HEIGHT, st7306::ST7306Driver::COLOR_BLACK);
    
    // 绘制网格线（浅灰色，帮助定位）
    for (int x = GAME_AREA_X; x <= GAME_AREA_X + GAME_AREA_WIDTH; x += GRID_SIZE) {
        gfx.drawLine(x, GAME_AREA_Y, x, GAME_AREA_Y + GAME_AREA_HEIGHT, st7306::ST7306Driver::COLOR_GRAY1);
    }
    for (int y = GAME_AREA_Y; y <= GAME_AREA_Y + GAME_AREA_HEIGHT; y += GRID_SIZE) {
        gfx.drawLine(GAME_AREA_X, y, GAME_AREA_X + GAME_AREA_WIDTH, y, st7306::ST7306Driver::COLOR_GRAY1);
    }
}

// 绘制蛇
void drawSnake() {
    for (size_t i = 0; i < snake.size(); i++) {
        int screen_x = GAME_AREA_X + snake[i].x * GRID_SIZE;
        int screen_y = GAME_AREA_Y + snake[i].y * GRID_SIZE;
        
        if (i == 0) {
            // 蛇头用黑色方块
            gfx.fillRect(screen_x + 1, screen_y + 1, GRID_SIZE - 2, GRID_SIZE - 2, st7306::ST7306Driver::COLOR_BLACK);
        } else {
            // 蛇身用深灰色方块
            gfx.fillRect(screen_x + 1, screen_y + 1, GRID_SIZE - 2, GRID_SIZE - 2, st7306::ST7306Driver::COLOR_GRAY2);
        }
    }
    
    // 调试信息
    printf("绘制蛇: 长度=%d, 头部位置(%d,%d)\n", (int)snake.size(), snake[0].x, snake[0].y);
}

// 绘制食物
void drawFood() {
    int screen_x = GAME_AREA_X + food.x * GRID_SIZE;
    int screen_y = GAME_AREA_Y + food.y * GRID_SIZE;
    
    // 食物用黑色方块表示
    gfx.fillRect(screen_x + 2, screen_y + 2, GRID_SIZE - 4, GRID_SIZE - 4, st7306::ST7306Driver::COLOR_BLACK);
    
    // 调试信息
    printf("绘制食物: 位置(%d,%d), 屏幕坐标(%d,%d)\n", food.x, food.y, screen_x, screen_y);
}

// 绘制UI
void drawUI() {
    // 清屏
    display.clearDisplay();
    
    // 绘制标题
    display.drawString(10, 10, "Snake Game", true);
    
    // 绘制分数
    char score_text[32];
    snprintf(score_text, sizeof(score_text), "Score: %d", score);
    display.drawString(10, 25, score_text, true);
    
    if (high_score > 0) {
        char high_score_text[32];
        snprintf(high_score_text, sizeof(high_score_text), "High: %d", high_score);
        display.drawString(150, 25, high_score_text, true);
    }
    
    // 根据游戏状态绘制不同内容
    switch (game_state) {
        case GAME_MENU:
            display.drawString(10, 350, "Press MID to Start", true);
            break;
            
        case GAME_PLAYING:
            drawGameArea();
            drawSnake();
            drawFood();
            display.drawString(10, 370, "Playing...", true);
            break;
            
        case GAME_PAUSED:
            drawGameArea();
            drawSnake();
            drawFood();
            display.drawString(10, 370, "Paused - Press MID", true);
            break;
            
        case GAME_OVER:
            display.drawString(10, 350, "Game Over!", true);
            display.drawString(10, 365, "Press MID to Restart", true);
            break;
    }
    
    display.display();
}

// 处理按钮输入
void handleButtonPress() {
    switch (game_state) {
        case GAME_MENU:
            game_state = GAME_PLAYING;
            initGame();
            printf("游戏开始！\n");
            break;
            
        case GAME_PLAYING:
            game_state = GAME_PAUSED;
            printf("游戏暂停\n");
            break;
            
        case GAME_PAUSED:
            game_state = GAME_PLAYING;
            printf("游戏继续\n");
            break;
            
        case GAME_OVER:
            game_state = GAME_MENU;
            printf("返回主菜单\n");
            break;
    }
}

// 更新游戏逻辑
void updateGame() {
    absolute_time_t current_time = get_absolute_time();
    
    // 更新摇杆状态
    joystick_handler.update();
    
    // 处理摇杆输入
    Direction joystick_dir = getJoystickDirection();
    if (joystick_dir != DIR_NONE && isValidDirection(joystick_dir)) {
        next_direction = joystick_dir;
        last_input_time = current_time;
    }
    
    // 处理按钮输入
    bool current_button = !joystick.get_button_value();
    if (current_button && !button_pressed) {
        button_pressed = true;
        handleButtonPress();
    } else if (!current_button && button_pressed) {
        button_pressed = false;
    }
    
    // 游戏运行时的移动逻辑
    if (game_state == GAME_PLAYING) {
        // 根据分数调整移动速度（分数越高，速度越快）
        int move_interval = 200 - (score * 5);  // 基础200ms，每分减少5ms
        if (move_interval < 50) move_interval = 50;  // 最快50ms
        
        if (absolute_time_diff_us(last_move_time, current_time) >= move_interval * 1000) {
            moveSnake();
            last_move_time = current_time;
        }
    }
}

// 主循环
void gameLoop() {
    while (true) {
        updateGame();
        drawUI();
        sleep_ms(16);  // 约60FPS
    }
}

// ==================== 主函数 ====================
int main() {
    stdio_init_all();
    
    printf("JS16TMR贪吃蛇游戏启动...\n");
    
    // 初始化显示
    printf("初始化ST7306显示...\n");
    display.initialize();
    printf("ST7306显示初始化成功\n");
    
    // 初始化JS16TMR摇杆
    printf("初始化JS16TMR摇杆...\n");
    if (!joystick.begin()) {
        printf("JS16TMR摇杆初始化失败！\n");
        return -1;
    }
    printf("JS16TMR摇杆初始化成功\n");
    
    // 初始化摇杆处理器
    printf("初始化JS16TMR摇杆处理器...\n");
    if (!joystick_handler.initialize(&joystick)) {
        printf("JS16TMR摇杆处理器初始化失败！\n");
        return -1;
    }
    
    // 配置摇杆参数 - 再逆时针旋转90度（总共180度）
    joystick_handler.setDeadzone(200);
    joystick_handler.setDirectionRatio(1.0f);
    joystick_handler.setRotation(js16tmr::JoystickRotation::ROTATION_180);
    
    printf("JS16TMR摇杆配置完成 - 旋转180度\n");
    printf("贪吃蛇游戏初始化完成！\n");
    
    // 显示初始界面
    drawUI();
    
    // 开始游戏循环
    gameLoop();
    
    return 0;
}
