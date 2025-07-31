#include <stdio.h>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <queue>
#include <random>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "st7306_driver.hpp"
#include "pico_display_gfx.hpp"
#include "joystick.hpp"
#include "joystick/joystick_config.hpp"
#include "spi_config.hpp"

// ==================== 游戏难度设置 ====================
// 难度级别定义（1-5级）
#define DIFFICULTY_LEVEL 5  // 当前难度级别，改为1级便于调试

// 各难度级别的最大步数限制
struct DifficultySettings {
    int max_steps;
    int maze_size;
    const char* name;
};

const DifficultySettings DIFFICULTY_CONFIGS[5] = {
    {5,  11, "Level 1 (Easy)"},    // 1级：5步，小迷宫
    {8,  15, "Level 2 (Normal)"},  // 2级：8步，中等迷宫
    {12, 19, "Level 3 (Medium)"},  // 3级：12步，中等迷宫
    {15, 23, "Level 4 (Hard)"},    // 4级：15步，大迷宫
    {20, 27, "Level 5 (Expert)"}   // 5级：20步，最大迷宫
};

// 当前难度配置
const DifficultySettings& CURRENT_DIFFICULTY = DIFFICULTY_CONFIGS[DIFFICULTY_LEVEL - 1];

// ==================== 显示参数 ====================
// 屏幕尺寸
#define SCREEN_WIDTH  300
#define SCREEN_HEIGHT 400

// UI布局
#define UI_HEIGHT     60    // 顶部UI区域高度（增加以适应更大屏幕）
#define BOTTOM_UI_HEIGHT 50 // 底部UI区域高度（增加以适应更大屏幕）
#define MAZE_AREA_Y   UI_HEIGHT  // 迷宫区域起始Y坐标
#define MAZE_AREA_HEIGHT (SCREEN_HEIGHT - UI_HEIGHT - BOTTOM_UI_HEIGHT)  // 迷宫区域高度

// 迷宫参数（使用最大尺寸定义数组）
#define MAX_MAZE_SIZE 27    // 最大迷宫尺寸（支持所有难度级别）
#define MAZE_WIDTH    (getMazeSize())   // 当前迷宫宽度
#define MAZE_HEIGHT   (getMazeSize())   // 当前迷宫高度

// 计算格子大小以适应屏幕
#define CELL_SIZE     ((MAZE_AREA_HEIGHT - 40) / getMazeSize())  // 每个格子的像素大小（增加边距）
#define MAZE_OFFSET_X ((SCREEN_WIDTH - (getMazeSize() * getCellSize())) / 2)  // 迷宫居中X偏移
#define MAZE_OFFSET_Y (MAZE_AREA_Y + 20)  // 迷宫Y偏移（增加上边距）

// 辅助函数声明
inline int getMazeSize() { return CURRENT_DIFFICULTY.maze_size; }
inline int getCellSize() { return (MAZE_AREA_HEIGHT - 40) / getMazeSize(); }

// 方向常量
#define DIR_NONE  0
#define DIR_UP    1
#define DIR_DOWN  2
#define DIR_LEFT  3
#define DIR_RIGHT 4
#define DIR_MID   5

// 游戏状态
enum GameState {
    GAME_INIT,
    GAME_INPUT_PATH,
    GAME_RUNNING,
    GAME_WIN,
    GAME_LOSE
};

// 迷宫单元格类型
enum CellType {
    WALL = 0,
    PATH = 1,
    START = 2,
    END = 3,
    PLAYER = 4
};

// 位置结构
struct Position {
    int x, y;
    Position(int x = 0, int y = 0) : x(x), y(y) {}
    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
    bool operator!=(const Position& other) const {
        return !(*this == other);
    }
};

class MazeGame {
private:
    st7306::ST7306Driver& display;
    pico_gfx::PicoDisplayGFX<st7306::ST7306Driver>& gfx;
    Joystick& joystick;
    
    // 迷宫数据（使用最大尺寸）
    uint8_t maze[MAX_MAZE_SIZE][MAX_MAZE_SIZE];
    Position start_pos;
    Position end_pos;
    Position player_pos;
    
    // 游戏状态
    GameState game_state;
    std::vector<int> planned_moves;
    int current_move_index;
    
    // 输入状态
    int last_direction;
    absolute_time_t last_input_time;
    bool button_pressed;
    
    // LED闪烁控制
    bool led_flashing;
    absolute_time_t flash_start_time;
    uint32_t flash_color;
    int flash_duration_ms;
    bool flash_state;
    absolute_time_t last_flash_toggle;
    
    // 路径填充动画
    bool path_filling;
    absolute_time_t path_fill_start_time;
    int path_fill_index;
    
public:
    MazeGame(st7306::ST7306Driver& disp, pico_gfx::PicoDisplayGFX<st7306::ST7306Driver>& graphics, Joystick& joy)
        : display(disp), gfx(graphics), joystick(joy), game_state(GAME_INIT), 
          current_move_index(0), last_direction(DIR_NONE), button_pressed(false),
          led_flashing(false), flash_duration_ms(0), flash_state(false),
          path_filling(false), path_fill_index(0) {
        last_input_time = get_absolute_time();
        flash_start_time = get_absolute_time();
        last_flash_toggle = get_absolute_time();
    }
    
    // 初始化游戏
    void init() {
        // 初始化随机种子
        srand(time_us_32());
        
        generateMaze();
        game_state = GAME_INPUT_PATH;
        planned_moves.clear();
        current_move_index = 0;
        player_pos = start_pos;
        
        display.clearDisplay();
        drawMaze();
        drawUI();
        display.display();
        
        // 遵循标准LED规则：初始化后LED关闭，等待操作
        joystick.set_rgb_color(JOYSTICK_LED_OFF);
        printf("Maze game initialized. Use joystick to plan your path, then press MID to start!\n");
    }
    
    // 生成迷宫（确保能在指定步数内完成且至少需要5步）
    void generateMaze() {
        int max_attempts = 30;  // 增加尝试次数以适应新的最小步数要求
        
        for (int attempt = 0; attempt < max_attempts; attempt++) {
            generateBasicMaze();
            
            // 验证迷宫是否满足步数要求（5步到最大步数之间）
            if (validateMazeSteps()) {
                printf("Generated valid maze in %d attempts\n", attempt + 1);
                return;
            }
            
            printf("Attempt %d: Maze doesn't meet step requirements, regenerating...\n", attempt + 1);
        }
        
        // 如果多次尝试都失败，生成一个更智能的迷宫
        printf("Failed to generate random maze after %d attempts, creating optimized maze\n", max_attempts);
        generateOptimizedMaze();
    }
    
    // 生成基础迷宫（使用递归回溯算法）
    void generateBasicMaze() {
        int maze_size = getMazeSize();
        
        // 初始化迷宫为全墙
        for (int y = 0; y < maze_size; y++) {
            for (int x = 0; x < maze_size; x++) {
                maze[y][x] = WALL;
            }
        }
        
        // 设置起点和终点（随机化）
        setupStartAndEndPositions();
        
        // 使用递归回溯算法生成迷宫
        generateMazeRecursiveBacktrack();
        
        // 确保起点和终点是通路
        maze[start_pos.y][start_pos.x] = PATH;
        maze[end_pos.y][end_pos.x] = PATH;
        
        // 确保出入口路径畅通（支持起点和终点都在边界，并保证连通性）
        ensureExitPath();
        
        // 创建统一的边界厚度效果
        createUniformBorderThickness();
        
        // 添加入口开口
        addEntranceExits();
        
        // 调试输出迷宫状态
        printf("Generated maze %dx%d using Recursive Backtrack algorithm\n", maze_size, maze_size);
        printf("Start: (%d, %d), End: (%d, %d)\n", start_pos.x, start_pos.y, end_pos.x, end_pos.y);
        
        // 调试连通性（仅在需要时启用）
        #ifdef DEBUG_CONNECTIVITY
        debugMazeConnectivity();
        #endif
    }
    
    // 设置起点和终点位置（随机化 - 都在边界上）
    void setupStartAndEndPositions() {
        int maze_size = getMazeSize();
        
        // 随机选择起点位置（在边界上）
        int start_side = rand() % 4;  // 0=上边界, 1=右边界, 2=下边界, 3=左边界
        
        switch (start_side) {
            case 0: // 上边界
                start_pos.x = 1 + 2 * (rand() % ((maze_size - 2) / 2));
                start_pos.y = 0;
                break;
            case 1: // 右边界
                start_pos.x = maze_size - 1;
                start_pos.y = 1 + 2 * (rand() % ((maze_size - 2) / 2));
                break;
            case 2: // 下边界
                start_pos.x = 1 + 2 * (rand() % ((maze_size - 2) / 2));
                start_pos.y = maze_size - 1;
                break;
            case 3: // 左边界
                start_pos.x = 0;
                start_pos.y = 1 + 2 * (rand() % ((maze_size - 2) / 2));
                break;
        }
        
        // 随机选择终点位置，确保与起点有足够距离且在不同边界
        Position temp_end;
        int min_distance = maze_size / 3;  // 最小距离要求
        int attempts = 0;
        
        do {
            // 随机选择边界位置作为终点（避免与起点在同一边界）
            int end_side;
            do {
                end_side = rand() % 4;  // 0=上边界, 1=右边界, 2=下边界, 3=左边界
            } while (end_side == start_side && attempts < 20);  // 尽量避免同一边界，但不要无限循环
            
            switch (end_side) {
                case 0: // 上边界
                    temp_end.x = 1 + 2 * (rand() % ((maze_size - 2) / 2));
                    temp_end.y = 0;
                    break;
                case 1: // 右边界
                    temp_end.x = maze_size - 1;
                    temp_end.y = 1 + 2 * (rand() % ((maze_size - 2) / 2));
                    break;
                case 2: // 下边界
                    temp_end.x = 1 + 2 * (rand() % ((maze_size - 2) / 2));
                    temp_end.y = maze_size - 1;
                    break;
                case 3: // 左边界
                    temp_end.x = 0;
                    temp_end.y = 1 + 2 * (rand() % ((maze_size - 2) / 2));
                    break;
            }
            
            // 计算曼哈顿距离
            int distance = abs(temp_end.x - start_pos.x) + abs(temp_end.y - start_pos.y);
            
            if (distance >= min_distance) {
                end_pos = temp_end;
                break;
            }
            
            attempts++;
        } while (attempts < 50);  // 最多尝试50次
        
        // 如果50次尝试都没找到合适位置，使用对角线边界位置
        if (attempts >= 50) {
            if (start_side == 0 || start_side == 3) {
                // 起点在上边界或左边界，终点放右下边界
                end_pos = Position(maze_size - 1, maze_size - 1);
        } else {
                // 起点在右边界或下边界，终点放左上边界
                end_pos = Position(0, 0);
            }
        }
        
        printf("Random boundary positions - Start: (%d, %d) on side %d, End: (%d, %d)\n", 
               start_pos.x, start_pos.y, start_side, end_pos.x, end_pos.y);
    }
    
    // 确保出入口路径畅通（支持起点和终点都在边界，确保精确到达出口）
    void ensureExitPath() {
        int maze_size = getMazeSize();
        
        // 确保起点和终点都是通路
        maze[start_pos.y][start_pos.x] = PATH;
        maze[end_pos.y][end_pos.x] = PATH;
        
        // 为起点创建通向内部的路径
        Position start_inner = start_pos;
        if (start_pos.x == 0) {
            // 左边界：从起点向右创建通路到内部
            for (int x = start_pos.x + 1; x <= start_pos.x + 3 && x < maze_size - 1; x++) {
                maze[start_pos.y][x] = PATH;
                start_inner.x = x;
            }
            printf("Created entrance path from left boundary\n");
        }
        else if (start_pos.x == maze_size - 1) {
            // 右边界：从起点向左创建通路到内部
            for (int x = start_pos.x - 1; x >= start_pos.x - 3 && x > 0; x--) {
                maze[start_pos.y][x] = PATH;
                start_inner.x = x;
            }
            printf("Created entrance path from right boundary\n");
        }
        else if (start_pos.y == 0) {
            // 上边界：从起点向下创建通路到内部
            for (int y = start_pos.y + 1; y <= start_pos.y + 3 && y < maze_size - 1; y++) {
                maze[y][start_pos.x] = PATH;
                start_inner.y = y;
            }
            printf("Created entrance path from top boundary\n");
        }
        else if (start_pos.y == maze_size - 1) {
            // 下边界：从起点向上创建通路到内部
            for (int y = start_pos.y - 1; y >= start_pos.y - 3 && y > 0; y--) {
                maze[y][start_pos.x] = PATH;
                start_inner.y = y;
            }
            printf("Created entrance path from bottom boundary\n");
        }
        
        // 为终点创建通向内部的路径，确保能精确到达终点
        Position end_inner = end_pos;
        if (end_pos.x == 0) {
            // 左边界：从终点向右创建通路到内部
            for (int x = end_pos.x + 1; x <= end_pos.x + 3 && x < maze_size - 1; x++) {
                maze[end_pos.y][x] = PATH;
                end_inner.x = x;
            }
            // 确保从内部能直接到达边界出口
            maze[end_pos.y][end_pos.x + 1] = PATH;
            printf("Created exit path from left boundary with direct access\n");
        }
        else if (end_pos.x == maze_size - 1) {
            // 右边界：从终点向左创建通路到内部
            for (int x = end_pos.x - 1; x >= end_pos.x - 3 && x > 0; x--) {
                maze[end_pos.y][x] = PATH;
                end_inner.x = x;
            }
            // 确保从内部能直接到达边界出口
            maze[end_pos.y][end_pos.x - 1] = PATH;
            printf("Created exit path from right boundary with direct access\n");
        }
        else if (end_pos.y == 0) {
            // 上边界：从终点向下创建通路到内部
            for (int y = end_pos.y + 1; y <= end_pos.y + 3 && y < maze_size - 1; y++) {
                maze[y][end_pos.x] = PATH;
                end_inner.y = y;
            }
            // 确保从内部能直接到达边界出口
            maze[end_pos.y + 1][end_pos.x] = PATH;
            printf("Created exit path from top boundary with direct access\n");
        }
        else if (end_pos.y == maze_size - 1) {
            // 下边界：从终点向上创建通路到内部
            for (int y = end_pos.y - 1; y >= end_pos.y - 3 && y > 0; y--) {
                maze[y][end_pos.x] = PATH;
                end_inner.y = y;
            }
            // 确保从内部能直接到达边界出口
            maze[end_pos.y - 1][end_pos.x] = PATH;
            printf("Created exit path from bottom boundary with direct access\n");
        }
        
        // 创建起点和终点之间的基本连通路径（确保能到达精确位置）
        createBasicConnectivity(start_inner, end_inner);
        
        // 额外确保终点位置可达
        ensureExactExitAccess();
    }
    
    // 确保能精确到达出口位置
    void ensureExactExitAccess() {
        int maze_size = getMazeSize();
        
        // 确保终点位置本身是通路
        maze[end_pos.y][end_pos.x] = PATH;
        
        // 确保至少有一个相邻位置是通路，这样玩家可以"走到"出口
        bool has_access = false;
        int directions[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
        
        for (int i = 0; i < 4; i++) {
            int nx = end_pos.x + directions[i][0];
            int ny = end_pos.y + directions[i][1];
            
            if (nx >= 0 && nx < maze_size && ny >= 0 && ny < maze_size) {
                if (maze[ny][nx] == PATH) {
                    has_access = true;
                    break;
                }
            }
        }
        
        // 如果没有相邻通路，强制创建一个
        if (!has_access) {
            for (int i = 0; i < 4; i++) {
                int nx = end_pos.x + directions[i][0];
                int ny = end_pos.y + directions[i][1];
                
                if (nx > 0 && nx < maze_size - 1 && ny > 0 && ny < maze_size - 1) {
                    maze[ny][nx] = PATH;
                    printf("Forced access to exact exit at (%d, %d) from (%d, %d)\n", 
                           end_pos.x, end_pos.y, nx, ny);
                    break;
                }
            }
        }
        
        printf("Ensured exact access to exit at (%d, %d)\n", end_pos.x, end_pos.y);
    }
    
    // 创建起点和终点之间的基本连通性（确保精确到达终点）
    void createBasicConnectivity(Position start_inner, Position end_inner) {
        printf("Creating basic connectivity from (%d,%d) to (%d,%d)\n", 
               start_inner.x, start_inner.y, end_inner.x, end_inner.y);
        
        Position current = start_inner;
        maze[current.y][current.x] = PATH;
        
        // 创建一条简单的L型路径确保连通性
        // 先水平移动
        while (current.x != end_inner.x) {
            if (current.x < end_inner.x) {
                current.x++;
            } else {
                current.x--;
            }
            maze[current.y][current.x] = PATH;
        }
        
        // 再垂直移动
        while (current.y != end_inner.y) {
            if (current.y < end_inner.y) {
                current.y++;
            } else {
                current.y--;
            }
            maze[current.y][current.x] = PATH;
        }
        
        // 确保终点内部位置是通路
        maze[end_inner.y][end_inner.x] = PATH;
        
        // 从内部位置到精确终点位置的连接
        Position exact_end = end_pos;
        current = end_inner;
        
        // 创建从内部位置到精确终点的路径
        while (current.x != exact_end.x || current.y != exact_end.y) {
            if (current.x != exact_end.x) {
                if (current.x < exact_end.x) {
                    current.x++;
                } else {
                    current.x--;
                }
            } else if (current.y != exact_end.y) {
                if (current.y < exact_end.y) {
                    current.y++;
                } else {
                    current.y--;
                }
            }
            maze[current.y][current.x] = PATH;
        }
        
        // 确保精确终点位置是通路
        maze[exact_end.y][exact_end.x] = PATH;
        
        printf("Basic connectivity path created with exact exit access\n");
    }
    
    // 创建统一的边界厚度效果
    void createUniformBorderThickness() {
        int maze_size = getMazeSize();
        
        // 为所有边界位置创建向内的"厚度"效果
        // 这样整个迷宫边框看起来更对称
        
        // 处理上边界和下边界
        for (int x = 1; x < maze_size - 1; x += 2) {  // 只处理奇数位置
            // 上边界
            if (!(Position(x, 0) == start_pos || Position(x, 0) == end_pos)) {
                // 如果不是起点或终点，创建一个小的向内凹陷
                if (1 < maze_size - 1) {
                    maze[1][x] = PATH;  // 向内一格
                }
            }
            
            // 下边界
            if (!(Position(x, maze_size - 1) == start_pos || Position(x, maze_size - 1) == end_pos)) {
                // 如果不是起点或终点，创建一个小的向内凹陷
                if (maze_size - 2 > 0) {
                    maze[maze_size - 2][x] = PATH;  // 向内一格
                }
            }
        }
        
        // 处理左边界和右边界
        for (int y = 1; y < maze_size - 1; y += 2) {  // 只处理奇数位置
            // 左边界
            if (!(Position(0, y) == start_pos || Position(0, y) == end_pos)) {
                // 如果不是起点或终点，创建一个小的向内凹陷
                if (1 < maze_size - 1) {
                    maze[y][1] = PATH;  // 向内一格
                }
            }
            
            // 右边界
            if (!(Position(maze_size - 1, y) == start_pos || Position(maze_size - 1, y) == end_pos)) {
                // 如果不是起点或终点，创建一个小的向内凹陷
                if (maze_size - 2 > 0) {
                    maze[y][maze_size - 2] = PATH;  // 向内一格
                }
            }
        }
        
        printf("Created uniform border thickness for visual symmetry\n");
    }
    
    // 递归回溯算法生成迷宫
    void generateMazeRecursiveBacktrack() {
        int maze_size = getMazeSize();
        
        // 创建访问标记数组
        bool visited[MAX_MAZE_SIZE][MAX_MAZE_SIZE];
        for (int y = 0; y < maze_size; y++) {
            for (int x = 0; x < maze_size; x++) {
                visited[y][x] = false;
            }
        }
        
        // 从起点开始递归生成
        recursiveBacktrackDFS(start_pos, visited);
        
        // 确保有足够的连通性，添加一些额外的路径
        addExtraConnections();
    }
    
    // 递归回溯深度优先搜索
    void recursiveBacktrackDFS(Position current, bool visited[MAX_MAZE_SIZE][MAX_MAZE_SIZE]) {
        int maze_size = getMazeSize();
        
        // 标记当前位置为已访问和通路
        visited[current.y][current.x] = true;
        maze[current.y][current.x] = PATH;
        
        // 获取所有可能的方向（随机顺序）
        std::vector<int> directions = {0, 1, 2, 3}; // 上、右、下、左
        
        // 随机打乱方向顺序
        for (int i = directions.size() - 1; i > 0; i--) {
            int j = rand() % (i + 1);
            std::swap(directions[i], directions[j]);
        }
        
        // 尝试每个方向
        for (int dir : directions) {
            Position next = getNextPosition(current, dir, 2); // 跳过一个格子（墙壁）
            
            // 检查下一个位置是否有效且未访问
            if (isValidPosition(next) && !visited[next.y][next.x]) {
                // 打通中间的墙壁
                Position wall = getNextPosition(current, dir, 1);
                maze[wall.y][wall.x] = PATH;
                
                // 递归访问下一个位置
                recursiveBacktrackDFS(next, visited);
            }
        }
    }
    
    // 获取指定方向和距离的下一个位置
    Position getNextPosition(Position current, int direction, int distance) {
        Position next = current;
        
        switch (direction) {
            case 0: next.y -= distance; break; // 上
            case 1: next.x += distance; break; // 右
            case 2: next.y += distance; break; // 下
            case 3: next.x -= distance; break; // 左
        }
        
        return next;
    }
    
    // 检查位置是否有效
    bool isValidPosition(Position pos) {
        int maze_size = getMazeSize();
        return pos.x > 0 && pos.x < maze_size - 1 && 
               pos.y > 0 && pos.y < maze_size - 1;
    }
    
    // 添加额外的连接以增加路径选择（倾向于创建分支）
    void addExtraConnections() {
        int maze_size = getMazeSize();
        int num_connections = maze_size / 6; // 增加连接数量
        
        for (int i = 0; i < num_connections; i++) {
            // 随机选择一个墙壁位置
            int x = 2 + (rand() % (maze_size - 4));
            int y = 2 + (rand() % (maze_size - 4));
            
            // 确保选择的是墙壁，且周围有足够的通路
            if (maze[y][x] == WALL && countAdjacentPaths(Position(x, y)) >= 2) {
                // 50%概率打通这个墙壁，创建额外的连接（增加概率）
                if (rand() % 100 < 50) {
                    maze[y][x] = PATH;
                    printf("Added extra connection at (%d, %d) to create potential branch\n", x, y);
                }
            }
        }
        
        // 专门尝试创建一些分支点
        for (int i = 0; i < 5; i++) {
            int x = 3 + (rand() % (maze_size - 6));
            int y = 3 + (rand() % (maze_size - 6));
            
            // 如果这个位置是通路，尝试在周围创建更多连接
            if (maze[y][x] == PATH) {
                int adjacent_count = countAdjacentPaths(Position(x, y));
                
                // 如果已经有2个相邻通路，尝试添加第三个
                if (adjacent_count == 2) {
                    int directions[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
                    
                    for (int j = 0; j < 4; j++) {
                        int nx = x + directions[j][0];
                        int ny = y + directions[j][1];
                        
                        if (nx > 0 && nx < maze_size - 1 && ny > 0 && ny < maze_size - 1) {
                            if (maze[ny][nx] == WALL) {
                                // 30%概率添加这个连接，创建分支
                                if (rand() % 100 < 30) {
                                    maze[ny][nx] = PATH;
                                    printf("Enhanced potential branch at (%d, %d)\n", x, y);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    // 计算位置周围的通路数量
    int countAdjacentPaths(Position pos) {
        int maze_size = getMazeSize();
        int path_count = 0;
        int directions[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
        
        for (int i = 0; i < 4; i++) {
            int nx = pos.x + directions[i][0];
            int ny = pos.y + directions[i][1];
            
            if (nx >= 0 && nx < maze_size && ny >= 0 && ny < maze_size) {
                if (maze[ny][nx] == PATH) {
                    path_count++;
                }
            }
        }
        
        return path_count;
    }
    
    // 检查位置周围是否有通路（保留原有函数以兼容其他代码）
    bool hasAdjacentPath(Position pos) {
        return countAdjacentPaths(pos) > 0;
    }
    
    // 创建基础路径确保连通性（备用方案）
    void createBasicPath() {
        // 创建一条从起点到终点的简单路径
        Position current = start_pos;
        maze[current.y][current.x] = PATH;
        
        // 向右移动到接近终点的X坐标
        while (current.x < end_pos.x) {
            current.x++;
            maze[current.y][current.x] = PATH;
        }
        
        // 向下移动到终点的Y坐标
        while (current.y < end_pos.y) {
            current.y++;
            maze[current.y][current.x] = PATH;
        }
        
        // 确保终点可达
        maze[end_pos.y][end_pos.x] = PATH;
    }
    
    // 生成优化迷宫（智能备用方案）
    void generateOptimizedMaze() {
        int maze_size = getMazeSize();
        int attempts = 0;
        const int max_attempts = 10;
        
        while (attempts < max_attempts) {
        // 初始化迷宫为全墙
        for (int y = 0; y < maze_size; y++) {
            for (int x = 0; x < maze_size; x++) {
                maze[y][x] = WALL;
            }
        }
        
            // 重新设置起点和终点（确保有足够距离）
            setupStartAndEndPositions();
        
        // 创建一个更复杂的备用迷宫
        createComplexBackupMaze();
        
        // 确保起点和终点是通路
        maze[start_pos.y][start_pos.x] = PATH;
        maze[end_pos.y][end_pos.x] = PATH;
        
            // 确保出入口路径畅通
            ensureExitPath();
            
            // 创建统一的边界厚度效果
            createUniformBorderThickness();
            
            // 验证是否满足步数要求
            if (validateMazeSteps()) {
                printf("Generated valid optimized maze in %d attempts\n", attempts + 1);
        addEntranceExits();
                return;
            }
            
            attempts++;
            printf("Optimized maze attempt %d failed validation, retrying...\n", attempts);
        }
        
        // 如果所有尝试都失败，创建一个保证满足要求的简单迷宫
        printf("All optimized attempts failed, creating guaranteed valid maze\n");
        createGuaranteedValidMaze();
    }
    
    // 创建复杂的备用迷宫
    void createComplexBackupMaze() {
        int maze_size = getMazeSize();
        
        // 创建基础路径
        createBasicPath();
        
        // 添加一些分支路径
        for (int i = 0; i < 3; i++) {
            // 在主路径上随机选择分支点
            int branch_x = 1 + (rand() % (end_pos.x - 1));
            int branch_y = 1 + (rand() % (end_pos.y - 1));
            
            // 确保分支点在主路径上
            if (maze[branch_y][branch_x] == PATH) {
                // 随机选择分支方向
                int branch_dir = rand() % 4;
                createBranch(Position(branch_x, branch_y), branch_dir, 2 + rand() % 3);
            }
        }
        
        // 添加一些随机的小房间
        for (int i = 0; i < 2; i++) {
            int room_x = 2 + (rand() % (maze_size - 4));
            int room_y = 2 + (rand() % (maze_size - 4));
            createSmallRoom(Position(room_x, room_y));
        }
    }
    
    // 创建分支路径
    void createBranch(Position start, int direction, int length) {
        int maze_size = getMazeSize();
        Position current = start;
        
        for (int i = 0; i < length; i++) {
            switch (direction) {
                case 0: current.y--; break; // 上
                case 1: current.y++; break; // 下
                case 2: current.x--; break; // 左
                case 3: current.x++; break; // 右
            }
            
            // 检查边界
            if (current.x > 0 && current.x < maze_size - 1 && 
                current.y > 0 && current.y < maze_size - 1) {
                maze[current.y][current.x] = PATH;
            } else {
                break;
            }
        }
    }
    
    // 创建小房间
    void createSmallRoom(Position center) {
        int maze_size = getMazeSize();
        
        // 创建2x2的小房间
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int x = center.x + dx;
                int y = center.y + dy;
                
                if (x > 0 && x < maze_size - 1 && y > 0 && y < maze_size - 1) {
                    maze[y][x] = PATH;
                }
            }
        }
    }
    
    // 验证迷宫是否能在指定步数内完成（使用严格的胜利条件）
    bool validateMazeSteps() {
        // 使用BFS找到最短路径
        std::queue<Position> queue;
        int maze_size = getMazeSize();
        int steps[MAX_MAZE_SIZE][MAX_MAZE_SIZE];
        
        // 初始化步数数组
        for (int y = 0; y < maze_size; y++) {
            for (int x = 0; x < maze_size; x++) {
                steps[y][x] = -1;
            }
        }
        
        queue.push(start_pos);
        steps[start_pos.y][start_pos.x] = 0;
        
        int directions[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
        bool found_path = false;
        int min_steps = -1;
        
        while (!queue.empty()) {
            Position current = queue.front();
            queue.pop();
            
            // 检查是否正好到达终点（使用严格的胜利条件）
            if (current == end_pos) {
                min_steps = steps[current.y][current.x];
                found_path = true;
                printf("Found exact path to exit: %d steps\n", min_steps);
                break;
            }
            
            // 尝试四个方向的移动（走到墙壁）
            for (int dir = 0; dir < 4; dir++) {
                Position next_pos = calculateMoveToWall(current, dir + 1);  // dir+1 因为DIR_UP=1
                
                if (!(next_pos == current) && steps[next_pos.y][next_pos.x] == -1) {
                    steps[next_pos.y][next_pos.x] = steps[current.y][current.x] + 1;
                    queue.push(next_pos);
                }
            }
        }
        
        if (!found_path) {
            printf("No exact path found from start (%d,%d) to end (%d,%d)!\n", 
                   start_pos.x, start_pos.y, end_pos.x, end_pos.y);
            return false;
        }
        
        printf("Maze requires minimum %d steps (limit: %d, minimum required: 5)\n", 
               min_steps, CURRENT_DIFFICULTY.max_steps);
        
        // 检查是否满足最小步数要求（至少5步）和最大步数限制
        if (min_steps < 5) {
            printf("Maze too easy! Requires only %d steps (minimum: 5)\n", min_steps);
            return false;
        }
        
        if (min_steps > CURRENT_DIFFICULTY.max_steps) {
            printf("Maze too hard! Requires %d steps (maximum: %d)\n", 
                   min_steps, CURRENT_DIFFICULTY.max_steps);
            return false;
        }
        
        // 验证分支数量
        int branch_count = countPathBranches();
        printf("Found %d branches in the maze\n", branch_count);
        
        if (branch_count < 2) {
            printf("Not enough branches! Found %d branches (minimum: 2)\n", branch_count);
            return false;
        }
        
        return true;
    }
    
    // 计算迷宫中的分支数量
    int countPathBranches() {
        int maze_size = getMazeSize();
        int branch_count = 0;
        
        // 遍历所有通路位置，检查是否为分支点
        for (int y = 1; y < maze_size - 1; y++) {
            for (int x = 1; x < maze_size - 1; x++) {
                if (maze[y][x] == PATH) {
                    // 计算该位置周围的通路数量
                    int adjacent_paths = 0;
                    int directions[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
                    
                    for (int i = 0; i < 4; i++) {
                        int nx = x + directions[i][0];
                        int ny = y + directions[i][1];
                        
                        if (nx >= 0 && nx < maze_size && ny >= 0 && ny < maze_size) {
                            if (maze[ny][nx] == PATH) {
                                adjacent_paths++;
                            }
                        }
                    }
                    
                    // 如果有3个或更多相邻的通路，则认为是分支点
                    if (adjacent_paths >= 3) {
                        branch_count++;
                        printf("Branch found at (%d, %d) with %d adjacent paths\n", 
                               x, y, adjacent_paths);
                    }
                }
            }
        }
        
        return branch_count;
    }
    
    // 绘制迷宫
    void drawMaze() {
        int maze_size = getMazeSize();
        int cell_size = getCellSize();
        int offset_x = (SCREEN_WIDTH - (maze_size * cell_size)) / 2;
        
        // 如果正在进行路径填充或游戏已结束，计算所有已填充的位置
        std::vector<Position> filled_positions;
        if ((path_filling && game_state == GAME_RUNNING) || 
            game_state == GAME_WIN || game_state == GAME_LOSE) {
            filled_positions = calculateFilledPositions();
        }
        
        for (int y = 0; y < maze_size; y++) {
            for (int x = 0; x < maze_size; x++) {
                int screen_x = offset_x + x * cell_size;
                int screen_y = MAZE_OFFSET_Y + y * cell_size;
                
                uint8_t color = st7306::ST7306Driver::COLOR_WHITE;
                
                if (maze[y][x] == WALL) {
                    color = st7306::ST7306Driver::COLOR_BLACK;
                } else if (Position(x, y) == start_pos) {
                    // 入口用深灰色标识
                    color = st7306::ST7306Driver::COLOR_GRAY2;
                } else if (Position(x, y) == end_pos) {
                    // 出口用中灰色标识
                    color = st7306::ST7306Driver::COLOR_GRAY1;
                } else if ((path_filling && game_state == GAME_RUNNING) || 
                          game_state == GAME_WIN || game_state == GAME_LOSE) {
                    // 路径填充动画或游戏结束后：检查当前位置是否在已填充的路径上
                    bool is_filled = false;
                    for (const Position& filled_pos : filled_positions) {
                        if (Position(x, y) == filled_pos) {
                            is_filled = true;
                            break;
                        }
                    }
                    
                    if (is_filled) {
                        // 用中灰色填充整个路径轨迹
                        color = st7306::ST7306Driver::COLOR_GRAY1;
                    }
                } else if (Position(x, y) == player_pos && game_state == GAME_RUNNING) {
                    // 玩家位置用黑色（在路径填充完成后显示）
                    color = st7306::ST7306Driver::COLOR_BLACK;
                }
                
                // 绘制格子
                for (int dy = 0; dy < cell_size; dy++) {
                    for (int dx = 0; dx < cell_size; dx++) {
                        if (screen_x + dx < st7306::ST7306Driver::LCD_WIDTH && 
                            screen_y + dy < st7306::ST7306Driver::LCD_HEIGHT) {
                            display.drawPixelGray(screen_x + dx, screen_y + dy, color);
                        }
                    }
                }
            }
        }
        
        // 绘制入口和出口标识文字
        if (cell_size >= 8) {  // 只有当格子足够大时才显示文字
            int start_screen_x = offset_x + start_pos.x * cell_size;
            int start_screen_y = MAZE_OFFSET_Y + start_pos.y * cell_size;
            int end_screen_x = offset_x + end_pos.x * cell_size;
            int end_screen_y = MAZE_OFFSET_Y + end_pos.y * cell_size;
            
            // 在入口和出口格子中央显示标识
            display.drawString(start_screen_x + 1, start_screen_y + 1, "S", false);  // Start
            display.drawString(end_screen_x + 1, end_screen_y + 1, "E", false);      // End
        }
    }
    
    // 计算已填充的所有位置（包括路径上的所有中间位置）
    std::vector<Position> calculateFilledPositions() {
        std::vector<Position> filled_positions;
        Position current_pos = start_pos;
        
        // 添加起点
        filled_positions.push_back(current_pos);
        
        // 确定要填充的步数
        int steps_to_fill = path_fill_index;
        if (game_state == GAME_WIN || game_state == GAME_LOSE) {
            // 游戏结束时显示完整路径
            steps_to_fill = (int)planned_moves.size();
        }
        
        // 对于每一步已完成的移动，添加路径上的所有位置
        for (int step = 0; step < steps_to_fill && step < (int)planned_moves.size(); step++) {
            Position target_pos = calculateMoveToWall(current_pos, planned_moves[step]);
            
            // 添加从当前位置到目标位置的所有中间位置
            std::vector<Position> step_positions = getPositionsBetween(current_pos, target_pos);
            for (const Position& pos : step_positions) {
                filled_positions.push_back(pos);
            }
            
            current_pos = target_pos;
        }
        
        return filled_positions;
    }
    
    // 获取两个位置之间的所有位置（包括起点和终点）
    std::vector<Position> getPositionsBetween(Position start, Position end) {
        std::vector<Position> positions;
        
        if (start == end) {
            positions.push_back(start);
            return positions;
        }
        
        // 确定移动方向
        int dx = 0, dy = 0;
        if (end.x > start.x) dx = 1;
        else if (end.x < start.x) dx = -1;
        if (end.y > start.y) dy = 1;
        else if (end.y < start.y) dy = -1;
        
        // 添加路径上的所有位置
        Position current = start;
        while (true) {
            positions.push_back(current);
            
            if (current == end) break;
            
            current.x += dx;
            current.y += dy;
        }
        
        return positions;
    }
    
    // 绘制UI信息
    void drawUI() {
        // 顶部状态栏 - 显示难度和步数信息
        display.drawString(10, 10, CURRENT_DIFFICULTY.name, true);
        
        // 显示步数限制和当前步数
        char steps_info[50];
        snprintf(steps_info, sizeof(steps_info), "Steps: %d/%d", 
                (int)planned_moves.size(), CURRENT_DIFFICULTY.max_steps);
        display.drawString(10, 30, steps_info, true);
        
        // 底部UI区域的Y坐标（根据新的屏幕高度调整）
        int bottom_y1 = SCREEN_HEIGHT - 40;  // 倒数第二行
        int bottom_y2 = SCREEN_HEIGHT - 20;  // 最底行
        
        // 根据游戏状态显示不同信息
        switch (game_state) {
            case GAME_INPUT_PATH:
                // 在屏幕底部显示操作提示
                display.drawString(10, bottom_y1, "Plan path with joystick", true);
                display.drawString(10, bottom_y2, "Press MID to start", true);
                
                // 显示已规划的路径（简化显示）
                if (!planned_moves.empty()) {
                    std::string path_str = "Path: ";
                    
                    // 计算并显示路径预览
                    Position current_pos = start_pos;
                    for (size_t i = 0; i < planned_moves.size() && i < 8; i++) { // 增加到8步显示
                        Position next_pos = calculateMoveToWall(current_pos, planned_moves[i]);
                        
                        switch (planned_moves[i]) {
                            case DIR_UP: path_str += "↑"; break;
                            case DIR_DOWN: path_str += "↓"; break;
                            case DIR_LEFT: path_str += "←"; break;
                            case DIR_RIGHT: path_str += "→"; break;
                        }
                        
                        // 如果没有移动（撞墙），添加X标记
                        if (next_pos == current_pos) {
                            path_str += "X";
                        }
                        
                        current_pos = next_pos;
                    }
                    
                    if (planned_moves.size() > 8) {
                        path_str += "...";
                    }
                    
                    // 检查是否超过步数限制
                    if ((int)planned_moves.size() > CURRENT_DIFFICULTY.max_steps) {
                        path_str += " (TOO MANY!)";
                    }
                    
                    display.drawString(150, 30, path_str.c_str(), true);
                }
                break;
                
            case GAME_RUNNING:
                // 显示当前步骤进度
                if (current_move_index < planned_moves.size()) {
                    char progress[30];
                    snprintf(progress, sizeof(progress), "Step %d/%d", 
                            current_move_index + 1, (int)planned_moves.size());
                    display.drawString(10, bottom_y2, progress, true);
                }
                break;
                
            case GAME_WIN:
                display.drawString(120, bottom_y1, "YOU WIN!", true);
                display.drawString(80, bottom_y2, "Press MID for new game", true);
                break;
                
            case GAME_LOSE:
                display.drawString(120, bottom_y1, "YOU LOST!", true);
                display.drawString(80, bottom_y2, "Press MID for new game", true);
                break;
        }
    }
    
    // 处理joystick输入
    int getJoystickDirection() {
        uint16_t adc_x, adc_y;
        joystick.get_joy_adc_16bits_value_xy(&adc_x, &adc_y);
        
        // 使用偏移值获取更准确的方向检测
        int16_t offset_x = joystick.get_joy_adc_12bits_offset_value_x();
        int16_t offset_y = joystick.get_joy_adc_12bits_offset_value_y();
        
        int16_t abs_x = abs(offset_x);
        int16_t abs_y = abs(offset_y);
        
        if (abs_x < JOYSTICK_THRESHOLD && abs_y < JOYSTICK_THRESHOLD) {
            return DIR_NONE;
        }
        
        if (abs_y > abs_x * JOYSTICK_DIRECTION_RATIO) {
            return (offset_y < 0) ? DIR_UP : DIR_DOWN;
        }
        
        if (abs_x > abs_y * JOYSTICK_DIRECTION_RATIO) {
            return (offset_x < 0) ? DIR_LEFT : DIR_RIGHT;
        }
        
        return DIR_NONE;
    }
    
    // 更新游戏状态
    void update() {
        // 首先更新LED闪烁状态
        updateLedFlash();
        
        uint8_t button_state = joystick.get_button_value();
        int current_direction = getJoystickDirection();
        absolute_time_t current_time = get_absolute_time();
        
        // 操作检测变量
        static bool is_active = false;
        static bool last_operation_detected = false;
        bool operation_detected = false;
        
        // 按钮按下检测
        if (button_state == 0 && !button_pressed) {
            button_pressed = true;
            operation_detected = true;
            handleButtonPress();
        } else if (button_state == 1) {
            button_pressed = false;
        }
        
        // 方向输入检测（带防抖）
        if (current_direction != DIR_NONE && current_direction != last_direction) {
            if (absolute_time_diff_us(last_input_time, current_time) > 300000) { // 300ms防抖
                operation_detected = true;
                handleDirectionInput(current_direction);
                last_direction = current_direction;
                last_input_time = current_time;
            }
        } else if (current_direction == DIR_NONE) {
            last_direction = DIR_NONE;
        }
        
        // LED控制逻辑 - 遵循标准规则（但在闪烁期间暂停）
        if (!led_flashing) {
            if (operation_detected && !is_active) {
                // 有操作时，立即点亮蓝色LED
                is_active = true;
                joystick.set_rgb_color(JOYSTICK_LED_BLUE);
            } else if (!operation_detected && is_active && current_direction == DIR_NONE && button_state == 1) {
                // 操作结束且joystick回到中心时，关闭LED
                is_active = false;
                joystick.set_rgb_color(JOYSTICK_LED_OFF);
            }
        }
        
        // 游戏运行状态更新
        if (game_state == GAME_RUNNING) {
            updateGameRunning();
            updatePathFilling();
        }
        
        last_operation_detected = operation_detected;
    }
    
    // 处理按钮按下
    void handleButtonPress() {
        switch (game_state) {
            case GAME_INPUT_PATH:
                if (!planned_moves.empty() && (int)planned_moves.size() <= CURRENT_DIFFICULTY.max_steps) {
                    startGame();
                } else if (planned_moves.empty()) {
                    printf("Please plan a path first!\n");
                } else {
                    printf("Too many steps! Maximum allowed: %d\n", CURRENT_DIFFICULTY.max_steps);
                }
                break;
                
            case GAME_WIN:
            case GAME_LOSE:
                init(); // 重新开始游戏
                break;
        }
    }
    
    // 处理方向输入
    void handleDirectionInput(int direction) {
        if (game_state == GAME_INPUT_PATH) {
            // 检查是否已达到步数限制
            if ((int)planned_moves.size() >= CURRENT_DIFFICULTY.max_steps) {
                printf("Cannot add more moves! Maximum steps reached: %d\n", CURRENT_DIFFICULTY.max_steps);
                return;
            }
            
            planned_moves.push_back(direction);
            
            // 遵循标准LED规则：操作时蓝色LED亮起
            joystick.set_rgb_color(JOYSTICK_LED_BLUE);
            
            printf("Added move: %s (Total: %d/%d)\n", 
                   directionToString(direction).c_str(), 
                   (int)planned_moves.size(),
                   CURRENT_DIFFICULTY.max_steps);
            
            // 重新绘制UI
            display.clearDisplay();
            drawMaze();
            drawUI();
            display.display();
        }
    }
    
    // 开始游戏
    void startGame() {
        // 检查步数是否超过限制
        if ((int)planned_moves.size() > CURRENT_DIFFICULTY.max_steps) {
            printf("Cannot start game! Too many steps: %d (max: %d)\n", 
                   (int)planned_moves.size(), CURRENT_DIFFICULTY.max_steps);
            return;
        }
        
        // 检查是否有规划的路径
        if (planned_moves.empty()) {
            printf("Cannot start game! No path planned.\n");
            return;
        }
        
        game_state = GAME_RUNNING;
        current_move_index = 0;
        player_pos = start_pos;
        
        // 启动路径填充动画
        path_filling = true;
        path_fill_start_time = get_absolute_time();
        path_fill_index = 0;
        
        // 遵循标准LED规则：不在游戏状态变化时设置特定颜色
        printf("Game started! Following planned path (%d steps)...\n", (int)planned_moves.size());
        
        display.clearDisplay();
        drawMaze();
        drawUI();
        display.display();
    }
    
    // 更新游戏运行状态
    void updateGameRunning() {
        // 游戏运行状态现在由路径填充动画处理
        // 这里可以添加其他游戏运行时的逻辑
    }
    
    // 执行移动
    void executeMove(int direction) {
        Position new_pos = player_pos;
        
        switch (direction) {
            case DIR_UP: new_pos.y--; break;
            case DIR_DOWN: new_pos.y++; break;
            case DIR_LEFT: new_pos.x--; break;
            case DIR_RIGHT: new_pos.x++; break;
        }
        
        int maze_size = getMazeSize();
        
        // 检查移动是否有效
        if (new_pos.x >= 0 && new_pos.x < maze_size && 
            new_pos.y >= 0 && new_pos.y < maze_size && 
            maze[new_pos.y][new_pos.x] == PATH) {
            player_pos = new_pos;
            // 每走一步，蓝灯闪一下
            singleLedFlash(JOYSTICK_LED_BLUE);
            printf("Moved %s to (%d, %d)\n", 
                   directionToString(direction).c_str(), 
                   player_pos.x, player_pos.y);
        } else {
            printf("Invalid move %s - hit wall!\n", directionToString(direction).c_str());
            game_state = GAME_LOSE;
            // 撞墙失败：红灯闪三秒
            startLedFlash(JOYSTICK_LED_RED, 3000);
        }
    }
    
    // 方向转字符串
    std::string directionToString(int direction) {
        switch (direction) {
            case DIR_UP: return "UP";
            case DIR_DOWN: return "DOWN";
            case DIR_LEFT: return "LEFT";
            case DIR_RIGHT: return "RIGHT";
            default: return "NONE";
        }
    }
    
    // 计算从指定位置沿着某个方向能走到的最远位置
    Position calculateMoveToWall(const Position& start_pos, int direction) {
        Position current_pos = start_pos;
        Position next_pos = current_pos;
        int maze_size = getMazeSize();
        
        while (true) {
            // 计算下一个位置
            switch (direction) {
                case DIR_UP: next_pos.y = current_pos.y - 1; break;
                case DIR_DOWN: next_pos.y = current_pos.y + 1; break;
                case DIR_LEFT: next_pos.x = current_pos.x - 1; break;
                case DIR_RIGHT: next_pos.x = current_pos.x + 1; break;
                default: return current_pos;
            }
            
            // 检查边界和墙壁
            if (next_pos.x < 0 || next_pos.x >= maze_size || 
                next_pos.y < 0 || next_pos.y >= maze_size || 
                maze[next_pos.y][next_pos.x] == WALL) {
                break;
            }
            
            current_pos = next_pos;
        }
        
        return current_pos;
    }
    
    // 开始LED闪烁
    void startLedFlash(uint32_t color, int duration_ms) {
        led_flashing = true;
        flash_color = color;
        flash_duration_ms = duration_ms;
        flash_state = true;
        flash_start_time = get_absolute_time();
        last_flash_toggle = get_absolute_time();
        joystick.set_rgb_color(color);
    }
    
    // 单次LED闪烁（用于每步移动）- 改为非阻塞版本
    void singleLedFlash(uint32_t color) {
        // 使用短暂的LED闪烁而不阻塞主循环
        startLedFlash(color, 200);  // 200ms短闪烁
    }
    
    // 更新LED闪烁状态
    void updateLedFlash() {
        if (!led_flashing) return;
        
        absolute_time_t current_time = get_absolute_time();
        
        // 检查闪烁是否结束
        if (absolute_time_diff_us(flash_start_time, current_time) > flash_duration_ms * 1000) {
            led_flashing = false;
            joystick.set_rgb_color(JOYSTICK_LED_OFF);
            return;
        }
        
        // 每500ms切换一次LED状态（闪烁频率）
        if (absolute_time_diff_us(last_flash_toggle, current_time) > 500000) {
            flash_state = !flash_state;
            last_flash_toggle = current_time;
            
            if (flash_state) {
                joystick.set_rgb_color(flash_color);
            } else {
                joystick.set_rgb_color(JOYSTICK_LED_OFF);
            }
        }
    }
    
    // 更新路径填充动画
    void updatePathFilling() {
        if (!path_filling) return;
        
        absolute_time_t current_time = get_absolute_time();
        
        // 调整填充速度：每1秒填充一步路径
        int elapsed_time_ms = absolute_time_diff_us(path_fill_start_time, current_time) / 1000;
        int target_fill_index = elapsed_time_ms / 1000;  // 1000ms per step
        
        if (target_fill_index > path_fill_index && path_fill_index < (int)planned_moves.size()) {
            path_fill_index = target_fill_index;
            
            // 限制填充索引不超过路径长度
            if (path_fill_index > (int)planned_moves.size()) {
                path_fill_index = (int)planned_moves.size();
            }
            
            // 重新绘制
            display.clearDisplay();
            drawMaze();
            drawUI();
            display.display();
            
            // 每填充一步，播放一个短暂的蓝色LED闪烁
            if (path_fill_index <= (int)planned_moves.size()) {
                singleLedFlash(JOYSTICK_LED_BLUE);
                printf("Path filled up to step %d/%d\n", path_fill_index, (int)planned_moves.size());
            }
        }
        
        // 检查路径填充是否完成
        if (path_fill_index >= (int)planned_moves.size()) {
            path_filling = false;
            
            // 计算最终位置
            Position final_pos = start_pos;
            Position current_pos = start_pos;
            
            for (int direction : planned_moves) {
                Position next_pos = calculateMoveToWall(current_pos, direction);
                if (!(next_pos == current_pos)) {
                    current_pos = next_pos;
                }
            }
            final_pos = current_pos;
            
            // 检查是否到达出口或出口附近
            bool reached_exit = checkWinCondition(final_pos);
            
            if (reached_exit) {
                game_state = GAME_WIN;
                // 走出迷宫：绿灯闪三秒
                startLedFlash(JOYSTICK_LED_GREEN, 3000);
                printf("Congratulations! You won! Final position: (%d, %d)\n", final_pos.x, final_pos.y);
            } else {
                game_state = GAME_LOSE;
                // 未走出迷宫：红灯闪三秒
                startLedFlash(JOYSTICK_LED_RED, 3000);
                printf("Game over! You didn't reach the exit. Final position: (%d, %d), Exit: (%d, %d)\n", 
                       final_pos.x, final_pos.y, end_pos.x, end_pos.y);
            }
            
            // 重新绘制一次，显示完整路径和游戏结果
            display.clearDisplay();
            drawMaze();
            drawUI();
            display.display();
            
            printf("Game finished. Press MID button to start a new game.\n");
        }
    }
    
    // 检查胜利条件（只有正好到达出口才算胜利）
    bool checkWinCondition(Position final_pos) {
        // 只有正好到达终点位置才算胜利
        if (final_pos == end_pos) {
            return true;
        }
        
        // 不再接受出口附近的位置，必须精确到达出口
        return false;
    }
    
    // 添加入口开口（现在起点和终点都在边界上，不需要额外开口）
    void addEntranceExits() {
        // 由于起点和终点现在都在边界上，并且 ensureExitPath() 已经处理了
        // 从边界到内部的连通性，这里不需要额外的开口处理
        printf("Entrance and exit paths handled by ensureExitPath()\n");
    }
    
    // 创建保证满足步数要求的迷宫（确保连通性和最小步数）
    void createGuaranteedValidMaze() {
        int maze_size = getMazeSize();
        int attempts = 0;
        const int max_attempts = 10;
        
        while (attempts < max_attempts) {
            // 初始化迷宫为全墙
            for (int y = 0; y < maze_size; y++) {
                for (int x = 0; x < maze_size; x++) {
                    maze[y][x] = WALL;
                }
            }
            
            // 重新设置起点和终点，确保有足够距离
            setupStartAndEndPositions();
            
            // 确保起点和终点是通路
            maze[start_pos.y][start_pos.x] = PATH;
            maze[end_pos.y][end_pos.x] = PATH;
            
            // 首先确保基本的出入口路径
            ensureExitPath();
            
            // 在基本连通路径的基础上添加一些曲折，增加步数
            addComplexityToPath();
            
            // 验证是否满足步数要求
            if (validateMazeSteps()) {
                // 创建统一的边界厚度效果
                createUniformBorderThickness();
                
                // 添加入口开口
                addEntranceExits();
                
                printf("Created guaranteed valid maze with connectivity and minimum 5 steps in %d attempts\n", attempts + 1);
                return;
            }
            
            attempts++;
            printf("Guaranteed maze attempt %d failed validation, retrying...\n", attempts);
        }
        
        // 如果所有尝试都失败，创建一个强制满足要求的迷宫
        printf("All guaranteed attempts failed, creating forced valid maze\n");
        createForcedValidMaze();
    }
    
    // 创建强制满足5步要求的迷宫（包含分支，确保精确到达出口）
    void createForcedValidMaze() {
        int maze_size = getMazeSize();
        
        // 初始化迷宫为全墙
        for (int y = 0; y < maze_size; y++) {
            for (int x = 0; x < maze_size; x++) {
                maze[y][x] = WALL;
            }
        }
        
        // 设置固定的起点和终点，确保有足够距离
        start_pos = Position(0, maze_size / 2);  // 左边界中间
        end_pos = Position(maze_size - 1, maze_size / 2);  // 右边界中间
        
        // 创建一条保证至少5步的曲折路径，确保能到达精确出口
        Position current = start_pos;
        maze[current.y][current.x] = PATH;
        
        // 第1步：向右走一段
        int step1_x = maze_size / 4;
        while (current.x < step1_x) {
            current.x++;
            maze[current.y][current.x] = PATH;
        }
        
        // 第2步：向上走
        int step2_y = current.y - 3;
        if (step2_y < 1) step2_y = 1;
        while (current.y > step2_y) {
            current.y--;
            maze[current.y][current.x] = PATH;
        }
        
        // 第3步：向右走
        int step3_x = current.x + maze_size / 4;
        if (step3_x >= maze_size - 1) step3_x = maze_size - 2;
        while (current.x < step3_x) {
            current.x++;
            maze[current.y][current.x] = PATH;
        }
        
        // 第4步：向下走回到终点Y坐标
        while (current.y < end_pos.y) {
            current.y++;
            maze[current.y][current.x] = PATH;
        }
        
        // 第5步：向右走到精确的终点位置
        while (current.x < end_pos.x) {
            current.x++;
            maze[current.y][current.x] = PATH;
        }
        
        // 确保起点和终点是通路
        maze[start_pos.y][start_pos.x] = PATH;
        maze[end_pos.y][end_pos.x] = PATH;
        
        // 强制创建分支点
        createForcedBranches();
        
        // 确保出入口路径畅通（包括精确出口访问）
        ensureExitPath();
        
        // 创建统一的边界厚度效果
        createUniformBorderThickness();
        
        // 添加入口开口
        addEntranceExits();
        
        printf("Created forced valid maze with guaranteed 5+ steps, branches, and exact exit access\n");
    }
    
    // 强制创建分支点（用于保底方案）
    void createForcedBranches() {
        int maze_size = getMazeSize();
        
        // 在固定位置创建分支点，确保至少有2个
        int branch_positions[3][2] = {
            {maze_size / 4, maze_size / 2 - 2},      // 左侧分支
            {maze_size / 2, maze_size / 2 + 2},      // 中间分支
            {maze_size * 3 / 4, maze_size / 2 - 1}   // 右侧分支
        };
        
        for (int i = 0; i < 3; i++) {
            int center_x = branch_positions[i][0];
            int center_y = branch_positions[i][1];
            
            // 确保位置有效
            if (center_x > 1 && center_x < maze_size - 2 && 
                center_y > 1 && center_y < maze_size - 2) {
                
                // 创建十字型分支
                maze[center_y][center_x] = PATH;
                
                // 上下分支
                maze[center_y - 1][center_x] = PATH;
                maze[center_y + 1][center_x] = PATH;
                
                // 左右分支
                maze[center_y][center_x - 1] = PATH;
                maze[center_y][center_x + 1] = PATH;
                
                // 延长分支
                if (center_y - 2 > 0) maze[center_y - 2][center_x] = PATH;
                if (center_y + 2 < maze_size - 1) maze[center_y + 2][center_x] = PATH;
                if (center_x - 2 > 0) maze[center_y][center_x - 2] = PATH;
                if (center_x + 2 < maze_size - 1) maze[center_y][center_x + 2] = PATH;
                
                printf("Created forced branch at (%d, %d)\n", center_x, center_y);
            }
        }
    }
    
    // 在基本路径上添加复杂性（确保至少2个分支）
    void addComplexityToPath() {
        int maze_size = getMazeSize();
        
        // 在迷宫中间区域添加一些额外的路径段，增加步数
        int mid_x = maze_size / 2;
        int mid_y = maze_size / 2;
        
        // 创建一个中间的小区域作为主要分支点
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int x = mid_x + dx;
                int y = mid_y + dy;
                if (x > 0 && x < maze_size - 1 && y > 0 && y < maze_size - 1) {
                    maze[y][x] = PATH;
                }
            }
        }
        
        // 创建一些曲折的路径段，强制增加步数
        // 在起点和终点之间创建一些"绕路"
        Position start_inner = start_pos;
        Position end_inner = end_pos;
        
        // 调整内部位置
        if (start_pos.x == 0) start_inner.x = 3;
        else if (start_pos.x == maze_size - 1) start_inner.x = maze_size - 4;
        if (start_pos.y == 0) start_inner.y = 3;
        else if (start_pos.y == maze_size - 1) start_inner.y = maze_size - 4;
        
        if (end_pos.x == 0) end_inner.x = 3;
        else if (end_pos.x == maze_size - 1) end_inner.x = maze_size - 4;
        if (end_pos.y == 0) end_inner.y = 3;
        else if (end_pos.y == maze_size - 1) end_inner.y = maze_size - 4;
        
        // 创建一条曲折的路径
        Position current = start_inner;
        
        // 先向中间区域移动
        while (abs(current.x - mid_x) > 1 || abs(current.y - mid_y) > 1) {
            if (current.x < mid_x) {
                current.x++;
            } else if (current.x > mid_x) {
                current.x--;
            } else if (current.y < mid_y) {
                current.y++;
            } else if (current.y > mid_y) {
                current.y--;
            }
            // 添加边界检查
            if (current.x >= 0 && current.x < maze_size && current.y >= 0 && current.y < maze_size) {
                maze[current.y][current.x] = PATH;
            }
        }
        
        // 从中间区域到终点
        while (current.x != end_inner.x || current.y != end_inner.y) {
            if (current.x != end_inner.x) {
                if (current.x < end_inner.x) {
                    current.x++;
                } else {
                    current.x--;
                }
            } else if (current.y != end_inner.y) {
                if (current.y < end_inner.y) {
                    current.y++;
                } else {
                    current.y--;
                }
            }
            // 添加边界检查
            if (current.x >= 0 && current.x < maze_size && current.y >= 0 && current.y < maze_size) {
                maze[current.y][current.x] = PATH;
            }
        }
        
        // 专门创建分支点
        createBranchPoints();
        
        printf("Added enhanced complexity with guaranteed branches\n");
    }
    
    // 专门创建分支点
    void createBranchPoints() {
        int maze_size = getMazeSize();
        int created_branches = 0;
        int attempts = 0;
        const int max_attempts = 20;
        
        while (created_branches < 3 && attempts < max_attempts) {  // 尝试创建3个分支点
            // 随机选择一个位置作为分支中心
            int center_x = 3 + (rand() % (maze_size - 6));
            int center_y = 3 + (rand() % (maze_size - 6));
            
            // 确保中心点是通路
            maze[center_y][center_x] = PATH;
            
            // 在四个方向创建分支
            int directions[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};  // 上下左右
            int branches_created = 0;
            
            for (int i = 0; i < 4; i++) {
                int dx = directions[i][0];
                int dy = directions[i][1];
                
                // 创建长度为2-4的分支
                int branch_length = 2 + (rand() % 3);
                bool can_create_branch = true;
                
                // 检查是否可以创建这个分支
                for (int j = 1; j <= branch_length; j++) {
                    int nx = center_x + dx * j;
                    int ny = center_y + dy * j;
                    
                    if (nx <= 0 || nx >= maze_size - 1 || ny <= 0 || ny >= maze_size - 1) {
                        can_create_branch = false;
                        break;
                    }
                }
                
                // 创建分支
                if (can_create_branch) {
                    for (int j = 1; j <= branch_length; j++) {
                        int nx = center_x + dx * j;
                        int ny = center_y + dy * j;
                        // 添加边界检查以避免数组越界
                        if (nx >= 0 && nx < MAX_MAZE_SIZE && ny >= 0 && ny < MAX_MAZE_SIZE) {
                            maze[ny][nx] = PATH;
                        }
                    }
                    branches_created++;
                }
            }
            
            // 如果成功创建了至少3个方向的分支，则认为是一个有效的分支点
            if (branches_created >= 3) {
                created_branches++;
                printf("Created branch point %d at (%d, %d) with %d branches\n", 
                       created_branches, center_x, center_y, branches_created);
            }
            
            attempts++;
        }
        
        // 如果还是没有足够的分支，创建一些简单的T型分支
        if (created_branches < 2) {
            createSimpleBranches();
        }
        
        printf("Total branch points created: %d\n", created_branches);
    }
    
    // 创建简单的T型分支
    void createSimpleBranches() {
        int maze_size = getMazeSize();
        
        // 在迷宫的1/4和3/4位置创建T型分支
        int positions[2][2] = {
            {maze_size / 4, maze_size / 2},
            {maze_size * 3 / 4, maze_size / 2}
        };
        
        for (int i = 0; i < 2; i++) {
            int center_x = positions[i][0];
            int center_y = positions[i][1];
            
            // 确保位置有效
            if (center_x > 2 && center_x < maze_size - 3 && 
                center_y > 2 && center_y < maze_size - 3) {
                
                // 创建T型分支：中心点 + 上下左右各2格
                maze[center_y][center_x] = PATH;
                
                // 上下分支（添加边界检查）
                if (center_y - 1 >= 0) maze[center_y - 1][center_x] = PATH;
                if (center_y - 2 >= 0) maze[center_y - 2][center_x] = PATH;
                if (center_y + 1 < maze_size) maze[center_y + 1][center_x] = PATH;
                if (center_y + 2 < maze_size) maze[center_y + 2][center_x] = PATH;
                
                // 左右分支（添加边界检查）
                if (center_x - 1 >= 0) maze[center_y][center_x - 1] = PATH;
                if (center_x - 2 >= 0) maze[center_y][center_x - 2] = PATH;
                if (center_x + 1 < maze_size) maze[center_y][center_x + 1] = PATH;
                if (center_x + 2 < maze_size) maze[center_y][center_x + 2] = PATH;
                
                printf("Created simple T-branch at (%d, %d)\n", center_x, center_y);
            }
        }
    }
    
    // 调试函数：检查并输出迷宫连通性信息
    void debugMazeConnectivity() {
        int maze_size = getMazeSize();
        printf("\n=== Maze Connectivity Debug ===\n");
        printf("Maze size: %dx%d\n", maze_size, maze_size);
        printf("Start position: (%d, %d)\n", start_pos.x, start_pos.y);
        printf("End position: (%d, %d)\n", end_pos.x, end_pos.y);
        
        // 检查起点和终点是否为通路
        printf("Start is PATH: %s\n", maze[start_pos.y][start_pos.x] == PATH ? "YES" : "NO");
        printf("End is PATH: %s\n", maze[end_pos.y][end_pos.x] == PATH ? "YES" : "NO");
        
        // 统计通路数量
        int path_count = 0;
        for (int y = 0; y < maze_size; y++) {
            for (int x = 0; x < maze_size; x++) {
                if (maze[y][x] == PATH) {
                    path_count++;
                }
            }
        }
        printf("Total PATH cells: %d\n", path_count);
        
        // 检查起点周围的连通性
        printf("Start neighbors: ");
        int start_neighbors = 0;
        int directions[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
        for (int i = 0; i < 4; i++) {
            int nx = start_pos.x + directions[i][0];
            int ny = start_pos.y + directions[i][1];
            if (nx >= 0 && nx < maze_size && ny >= 0 && ny < maze_size) {
                if (maze[ny][nx] == PATH) {
                    start_neighbors++;
                    printf("(%d,%d) ", nx, ny);
                }
            }
        }
        printf("(total: %d)\n", start_neighbors);
        
        // 检查终点周围的连通性
        printf("End neighbors: ");
        int end_neighbors = 0;
        for (int i = 0; i < 4; i++) {
            int nx = end_pos.x + directions[i][0];
            int ny = end_pos.y + directions[i][1];
            if (nx >= 0 && nx < maze_size && ny >= 0 && ny < maze_size) {
                if (maze[ny][nx] == PATH) {
                    end_neighbors++;
                    printf("(%d,%d) ", nx, ny);
                }
            }
        }
        printf("(total: %d)\n", end_neighbors);
        printf("=== End Debug ===\n\n");
    }
};

int main() {
    stdio_init_all();
    printf("Maze Game Starting...\n");
    
    // 初始化显示屏
    st7306::ST7306Driver display(PIN_DC, PIN_RST, PIN_CS, PIN_SCLK, PIN_SDIN);
    pico_gfx::PicoDisplayGFX<st7306::ST7306Driver> gfx(
        display, 
        st7306::ST7306Driver::LCD_WIDTH, 
        st7306::ST7306Driver::LCD_HEIGHT
    );
    
    printf("Initializing ST7306 display...\n");
    display.initialize();
    display.clearDisplay();
    display.display();
    
    // 初始化joystick
    Joystick joystick;
    printf("Initializing joystick...\n");
    if (joystick.begin(JOYSTICK_I2C_PORT, JOYSTICK_I2C_ADDR, 
                      JOYSTICK_I2C_SDA_PIN, JOYSTICK_I2C_SCL_PIN, 
                      JOYSTICK_I2C_SPEED)) {
        printf("Joystick initialization successful!\n");
        // 遵循标准LED规则：初始化成功时绿色LED亮1秒，然后关闭
        joystick.set_rgb_color(JOYSTICK_LED_GREEN);
        sleep_ms(1000);
        joystick.set_rgb_color(JOYSTICK_LED_OFF);
    } else {
        printf("Joystick initialization failed!\n");
        return -1;
    }
    
    // 创建游戏实例
    MazeGame game(display, gfx, joystick);
    game.init();
    
    // 游戏主循环
    while (true) {
        game.update();
        sleep_ms(20);
    }
    
    return 0;
} 