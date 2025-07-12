#pragma once
#include "button/button.hpp"
#include "button_config.hpp"
#include <cstdint>

namespace button {

// === 按键参数配置 ===
// 使用 button_config.hpp 中的统一配置

// 长按触发时间（毫秒）
#define BUTTON_LONG_PRESS_MS_DEFAULT     BUTTON_LONG_PRESS_MS

// 双击间隔时间（毫秒）
#define BUTTON_DOUBLE_PRESS_MS_DEFAULT   BUTTON_DOUBLE_PRESS_MS

// 防抖时间（毫秒）- 在ButtonController中定义，这里作为参考
#define BUTTON_DEBOUNCE_MS_DEFAULT       BUTTON_DEBOUNCE_TIME

// 高级按键事件类型
enum class ButtonLogicEvent {
    NONE,
    SHORT_PRESS,
    LONG_PRESS,
    DOUBLE_PRESS,
    COMBO_PRESS // 组合键（两键同时短按）
};

class ButtonEventHandler {
public:
    ButtonEventHandler(ButtonController* controller, uint32_t long_press_ms = BUTTON_LONG_PRESS_MS_DEFAULT, uint32_t double_press_ms = BUTTON_DOUBLE_PRESS_MS_DEFAULT);
    void update();
    ButtonLogicEvent get_key1_event() const;
    ButtonLogicEvent get_key2_event() const;
    ButtonLogicEvent get_combo_event() const; // 检查组合键事件
    void reset();
private:
    ButtonController* controller_;
    uint32_t long_press_ms_;
    uint32_t double_press_ms_;
    // 状态记录
    uint32_t key1_press_time_;
    uint32_t key2_press_time_;
    uint32_t key1_last_release_time_;
    uint32_t key2_last_release_time_;
    bool key1_long_press_handled_;
    bool key2_long_press_handled_;
    bool key1_double_pending_;
    bool key2_double_pending_;
    // 组合键
    bool combo_press_handled_;
    uint32_t combo_press_time_;
    // 上一帧状态
    bool last_key1_pressed_;
    bool last_key2_pressed_;
    // 事件缓存
    mutable ButtonLogicEvent key1_event_;
    mutable ButtonLogicEvent key2_event_;
    mutable ButtonLogicEvent combo_event_;
    // 延迟单击判定
    bool key1_single_pending_ = false;
    uint32_t key1_single_pending_time_ = 0;
    bool key2_single_pending_ = false;
    uint32_t key2_single_pending_time_ = 0;
};

} // namespace button 