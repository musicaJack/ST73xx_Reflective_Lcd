/**
 * @file button_config.hpp
 * @brief 按键配置参数统一管理
 * @version 1.0.0
 */

#ifndef BUTTON_CONFIG_HPP
#define BUTTON_CONFIG_HPP

// === 按键硬件配置 ===

// 按键引脚定义
#define BUTTON_KEY1_PIN         2       // 按键1引脚 (GPIO2) - 上一页/返回
#define BUTTON_KEY2_PIN         3       // 按键2引脚 (GPIO3) - 下一页/确认

// === 按键时间参数配置 ===

// 防抖时间（毫秒）- 防止按键抖动
#define BUTTON_DEBOUNCE_TIME    50

// 长按触发时间（毫秒）- 按键持续按下超过此时间触发长按事件
#define BUTTON_LONG_PRESS_MS    400

// 双击间隔时间（毫秒）- 两次按键之间的最大间隔时间
#define BUTTON_DOUBLE_PRESS_MS  300

// 单击延迟时间（毫秒）- 等待双击判定的时间，与双击间隔相同
#define BUTTON_SINGLE_PENDING_MS BUTTON_DOUBLE_PRESS_MS

// === 按键行为配置 ===

// 按键按下时的GPIO电平（0=低电平，1=高电平）
#define BUTTON_PRESSED_LEVEL    0       // 按键按下时为低电平

// 按键释放时的GPIO电平
#define BUTTON_RELEASED_LEVEL   1       // 按键释放时为高电平

// === 调试配置 ===

// 按键事件调试开关（0=关闭，1=开启）
#define BUTTON_DEBUG_ENABLED    1       // 临时开启调试日志

// 调试日志级别
#define BUTTON_DEBUG_LEVEL      1       // 1=仅重要事件，2=详细事件，3=所有事件

// === 向后兼容性定义 ===

// 引脚别名
#define KEY1_PIN               BUTTON_KEY1_PIN
#define KEY2_PIN               BUTTON_KEY2_PIN

// 时间参数别名
#define BUTTON_LONG_PRESS_MS_DEFAULT     BUTTON_LONG_PRESS_MS
#define BUTTON_DOUBLE_PRESS_MS_DEFAULT   BUTTON_DOUBLE_PRESS_MS
#define BUTTON_DEBOUNCE_MS_DEFAULT       BUTTON_DEBOUNCE_TIME

// === 配置说明 ===
/*
按键参数调优指南：

1. 防抖时间 (BUTTON_DEBOUNCE_TIME)
   - 默认: 50ms
   - 范围: 20-100ms
   - 作用: 防止按键机械抖动导致的误触发

2. 长按时间 (BUTTON_LONG_PRESS_MS)
   - 默认: 400ms
   - 范围: 200-1000ms
   - 作用: 区分短按和长按操作

3. 双击间隔 (BUTTON_DOUBLE_PRESS_MS)
   - 默认: 300ms
   - 范围: 200-500ms
   - 作用: 判定双击操作的时间窗口

4. 单击延迟 (BUTTON_SINGLE_PENDING_MS)
   - 默认: 300ms (与双击间隔相同)
   - 作用: 等待双击判定的延迟时间

调优建议：
- 如果长按不灵敏：减少 BUTTON_LONG_PRESS_MS
- 如果双击不灵敏：增加 BUTTON_DOUBLE_PRESS_MS
- 如果按键误触发：增加 BUTTON_DEBOUNCE_TIME

调试配置说明：

1. 调试开关 (BUTTON_DEBUG_ENABLED)
   - 0: 关闭所有调试日志（默认）
   - 1: 开启调试日志

2. 调试级别 (BUTTON_DEBUG_LEVEL)
   - 1: 仅显示重要事件（长按、双击、组合键）
   - 2: 显示详细事件（包括短按、状态变化）
   - 3: 显示所有事件（包括计时过程）

使用方法：
- 正常使用：保持 BUTTON_DEBUG_ENABLED = 0
- 调试按键：设置 BUTTON_DEBUG_ENABLED = 1, BUTTON_DEBUG_LEVEL = 1
- 详细调试：设置 BUTTON_DEBUG_ENABLED = 1, BUTTON_DEBUG_LEVEL = 2 或 3
*/

#endif // BUTTON_CONFIG_HPP 