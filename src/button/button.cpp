/**
 * @file button.cpp
 * @brief 双按键控制类实现
 * @version 1.0.0
 */

#include "button/button.hpp"
#include <stdio.h>

namespace button {

ButtonController::ButtonController(uint8_t key1_pin, uint8_t key2_pin, 
                                 uint8_t led_pin, uint32_t debounce_ms)
    : key1_pin_(key1_pin)
    , key2_pin_(key2_pin)
    , led_pin_(led_pin)
    , key1_state_(ButtonState::RELEASED)
    , key2_state_(ButtonState::RELEASED)
    , key1_last_state_(ButtonState::RELEASED)
    , key2_last_state_(ButtonState::RELEASED)
    , debounce_time_ms_(debounce_ms)
    , key1_last_change_(0)
    , key2_last_change_(0)
    , led_enabled_(led_pin != 255) {
}

bool ButtonController::initialize() {
    printf("[Button] 初始化按键控制器...\n");
    printf("[Button] KEY1: GP%d, KEY2: GP%d\n", key1_pin_, key2_pin_);
    
    // 初始化GPIO引脚
    gpio_init(key1_pin_);
    gpio_init(key2_pin_);
    
    // 设置为输入模式
    gpio_set_dir(key1_pin_, GPIO_IN);
    gpio_set_dir(key2_pin_, GPIO_IN);
    
    // 禁用内部上拉/下拉电阻（按键模块自带）
    gpio_disable_pulls(key1_pin_);
    gpio_disable_pulls(key2_pin_);
    
    // 打印初始GPIO状态
    printf("[Button] 初始GPIO状态: KEY1=%s, KEY2=%s\n", 
           gpio_get(key1_pin_) ? "HIGH" : "LOW",
           gpio_get(key2_pin_) ? "HIGH" : "LOW");
    
    // 初始化LED（如果启用）
    if (led_enabled_) {
        gpio_init(led_pin_);
        gpio_set_dir(led_pin_, GPIO_OUT);
        gpio_put(led_pin_, 0);  // 初始状态熄灭
        printf("[Button] LED: GP%d (已启用)\n", led_pin_);
    } else {
        printf("[Button] LED: 已禁用\n");
    }
    
    // 读取初始状态
    key1_state_ = read_button_state(key1_pin_);
    key2_state_ = read_button_state(key2_pin_);
    key1_last_state_ = key1_state_;
    key2_last_state_ = key2_state_;
    
    printf("[Button] 初始状态: KEY1=%s, KEY2=%s\n", 
           state_to_string(key1_state_), state_to_string(key2_state_));
    printf("[Button] 防抖时间: %lu ms\n", debounce_time_ms_);
    printf("[Button] 按键控制器初始化完成\n");
    
    return true;
}

void ButtonController::update() {
    // 读取当前GPIO状态
    ButtonState key1_current = read_button_state(key1_pin_);
    ButtonState key2_current = read_button_state(key2_pin_);
    
    // 检测按键1刚按下（从释放变为按下）
    if (key1_current == ButtonState::PRESSED && key1_last_state_ == ButtonState::RELEASED) {
        printf("[Button] KEY1 按下\n");
        key1_state_ = ButtonState::PRESSED;
    } else if (key1_current == ButtonState::RELEASED) {
        // 按键释放时重置状态
        key1_state_ = ButtonState::RELEASED;
    }
    
    // 检测按键2刚按下（从释放变为按下）
    if (key2_current == ButtonState::PRESSED && key2_last_state_ == ButtonState::RELEASED) {
        printf("[Button] KEY2 按下\n");
        key2_state_ = ButtonState::PRESSED;
    } else if (key2_current == ButtonState::RELEASED) {
        // 按键释放时重置状态
        key2_state_ = ButtonState::RELEASED;
    }
    
    // 更新上次状态
    key1_last_state_ = key1_current;
    key2_last_state_ = key2_current;
}

ButtonEvent ButtonController::get_key1_event() const {
    if (key1_state_ == ButtonState::PRESSED && key1_last_state_ == ButtonState::RELEASED) {
        return ButtonEvent::PRESS;
    } else if (key1_state_ == ButtonState::RELEASED && key1_last_state_ == ButtonState::PRESSED) {
        return ButtonEvent::RELEASE;
    }
    return ButtonEvent::NONE;
}

ButtonEvent ButtonController::get_key2_event() const {
    if (key2_state_ == ButtonState::PRESSED && key2_last_state_ == ButtonState::RELEASED) {
        return ButtonEvent::PRESS;
    } else if (key2_state_ == ButtonState::RELEASED && key2_last_state_ == ButtonState::PRESSED) {
        return ButtonEvent::RELEASE;
    }
    return ButtonEvent::NONE;
}

bool ButtonController::is_key1_just_pressed() const {
    // 简化逻辑：直接检查当前状态是否为按下
    return key1_state_ == ButtonState::PRESSED;
}

bool ButtonController::is_key2_just_pressed() const {
    // 简化逻辑：直接检查当前状态是否为按下
    return key2_state_ == ButtonState::PRESSED;
}

bool ButtonController::is_key1_just_released() const {
    return get_key1_event() == ButtonEvent::RELEASE;
}

bool ButtonController::is_key2_just_released() const {
    return get_key2_event() == ButtonEvent::RELEASE;
}

void ButtonController::set_led(bool state) {
    if (led_enabled_) {
        gpio_put(led_pin_, state);
    }
}

void ButtonController::toggle_led() {
    if (led_enabled_) {
        gpio_put(led_pin_, !gpio_get(led_pin_));
    }
}

void ButtonController::print_status() const {
    printf("[Button] 状态: KEY1=%s, KEY2=%s\n", 
           state_to_string(key1_state_), state_to_string(key2_state_));
    printf("[Button] 事件: KEY1=%s, KEY2=%s\n", 
           event_to_string(get_key1_event()), event_to_string(get_key2_event()));
}

const char* ButtonController::state_to_string(ButtonState state) {
    switch (state) {
        case ButtonState::RELEASED: return "RELEASED";
        case ButtonState::PRESSED: return "PRESSED";
        default: return "UNKNOWN";
    }
}

const char* ButtonController::event_to_string(ButtonEvent event) {
    switch (event) {
        case ButtonEvent::NONE: return "NONE";
        case ButtonEvent::PRESS: return "PRESS";
        case ButtonEvent::RELEASE: return "RELEASE";
        default: return "UNKNOWN";
    }
}

ButtonState ButtonController::read_button_state(uint8_t pin) const {
    // 读取GPIO状态（按下时为低电平）
    bool gpio_state = gpio_get(pin);  // 原始电平：1=高，0=低
    // 按键按下时GPIO为低，取反后为true（与您的代码逻辑一致）
    ButtonState button_state = !gpio_state ? ButtonState::PRESSED : ButtonState::RELEASED;
    
    return button_state;
}

bool ButtonController::is_debounce_ready(uint32_t last_change_time) const {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    return (current_time - last_change_time) >= debounce_time_ms_;
}

} // namespace button 