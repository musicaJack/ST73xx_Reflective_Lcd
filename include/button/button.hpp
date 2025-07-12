/**
 * @file button.hpp
 * @brief 双按键控制类 - 提供简洁的API接口
 * @version 1.0.0
 */

#ifndef BUTTON_HPP
#define BUTTON_HPP

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdint.h>

namespace button {

/**
 * @brief 按键状态枚举
 */
enum class ButtonState {
    RELEASED = 0,  // 释放状态
    PRESSED = 1    // 按下状态
};

/**
 * @brief 按键事件枚举
 */
enum class ButtonEvent {
    NONE = 0,      // 无事件
    PRESS = 1,     // 按下事件
    RELEASE = 2    // 释放事件
};

/**
 * @brief 双按键控制类
 * 
 * 提供简洁的API接口用于检测两个按键的状态变化
 * 支持防抖和事件检测
 */
class ButtonController {
private:
    uint8_t key1_pin_;           // 按键1引脚
    uint8_t key2_pin_;           // 按键2引脚
    uint8_t led_pin_;            // LED指示引脚（可选）
    
    ButtonState key1_state_;     // 按键1当前状态
    ButtonState key2_state_;     // 按键2当前状态
    ButtonState key1_last_state_; // 按键1上次状态
    ButtonState key2_last_state_; // 按键2上次状态
    
    uint32_t debounce_time_ms_;  // 防抖时间（毫秒）
    uint32_t key1_last_change_;  // 按键1最后状态变化时间
    uint32_t key2_last_change_;  // 按键2最后状态变化时间
    
    bool led_enabled_;           // LED指示是否启用

public:
    /**
     * @brief 构造函数
     * @param key1_pin 按键1引脚号
     * @param key2_pin 按键2引脚号
     * @param led_pin LED指示引脚号（可选，设为255禁用）
     * @param debounce_ms 防抖时间（毫秒）
     */
    ButtonController(uint8_t key1_pin = 2, uint8_t key2_pin = 3, 
                     uint8_t led_pin = 25, uint32_t debounce_ms = 50);
    
    /**
     * @brief 析构函数
     */
    ~ButtonController() = default;
    
    /**
     * @brief 初始化按键控制器
     * @return true 初始化成功，false 初始化失败
     */
    bool initialize();
    
    /**
     * @brief 更新按键状态（需要在主循环中调用）
     */
    void update();
    
    /**
     * @brief 获取按键1当前状态
     * @return 按键状态
     */
    ButtonState get_key1_state() const { return key1_state_; }
    
    /**
     * @brief 获取按键2当前状态
     * @return 按键状态
     */
    ButtonState get_key2_state() const { return key2_state_; }
    
    /**
     * @brief 检查按键1是否按下
     * @return true 按下，false 释放
     */
    bool is_key1_pressed() const { return key1_state_ == ButtonState::PRESSED; }
    
    /**
     * @brief 检查按键2是否按下
     * @return true 按下，false 释放
     */
    bool is_key2_pressed() const { return key2_state_ == ButtonState::PRESSED; }
    
    /**
     * @brief 获取按键1事件
     * @return 按键事件
     */
    ButtonEvent get_key1_event() const;
    
    /**
     * @brief 获取按键2事件
     * @return 按键事件
     */
    ButtonEvent get_key2_event() const;
    
    /**
     * @brief 检查按键1是否刚按下（边缘检测）
     * @return true 刚按下，false 其他情况
     */
    bool is_key1_just_pressed() const;
    
    /**
     * @brief 检查按键2是否刚按下（边缘检测）
     * @return true 刚按下，false 其他情况
     */
    bool is_key2_just_pressed() const;
    
    /**
     * @brief 检查按键1是否刚释放（边缘检测）
     * @return true 刚释放，false 其他情况
     */
    bool is_key1_just_released() const;
    
    /**
     * @brief 检查按键2是否刚释放（边缘检测）
     * @return true 刚释放，false 其他情况
     */
    bool is_key2_just_released() const;
    
    /**
     * @brief 设置LED状态
     * @param state true 点亮，false 熄灭
     */
    void set_led(bool state);
    
    /**
     * @brief 切换LED状态
     */
    void toggle_led();
    
    /**
     * @brief 启用/禁用LED指示
     * @param enabled true 启用，false 禁用
     */
    void enable_led(bool enabled) { led_enabled_ = enabled; }
    
    /**
     * @brief 打印按键状态信息（调试用）
     */
    void print_status() const;
    
    /**
     * @brief 获取按键状态字符串
     * @param state 按键状态
     * @return 状态字符串
     */
    static const char* state_to_string(ButtonState state);
    
    /**
     * @brief 获取按键事件字符串
     * @param event 按键事件
     * @return 事件字符串
     */
    static const char* event_to_string(ButtonEvent event);

private:
    /**
     * @brief 读取GPIO状态并转换为按键状态
     * @param pin 引脚号
     * @return 按键状态
     */
    ButtonState read_button_state(uint8_t pin) const;
    
    /**
     * @brief 检查是否超过防抖时间
     * @param last_change_time 最后状态变化时间
     * @return true 超过防抖时间，false 未超过
     */
    bool is_debounce_ready(uint32_t last_change_time) const;
};

} // namespace button

#endif // BUTTON_HPP 