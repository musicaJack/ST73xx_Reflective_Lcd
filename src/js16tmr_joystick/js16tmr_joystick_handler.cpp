/**
 * @file js16tmr_joystick_handler.cpp
 * @brief JS16TMR摇杆处理器实现
 */

#include "js16tmr_joystick/js16tmr_joystick_handler.hpp"
#include "js16tmr_joystick/js16tmr_joystick_direct.hpp"
#include <cstdio>
#include <string>
#include <cmath>

namespace js16tmr {

JS16TMRJoystickHandler::JS16TMRJoystickHandler() 
    : is_initialized_(false)
    , js16tmr_joystick_direct_(nullptr)
    , deadzone_(20)
    , direction_ratio_(1.5f)
    , rotation_(JoystickRotation::ROTATION_0)
    , last_direction_(JoystickDirection::CENTER)
    , last_button_state_(false) {
}

JS16TMRJoystickHandler::~JS16TMRJoystickHandler() = default;

bool JS16TMRJoystickHandler::initialize(JS16TMRJoystickDirect* joystick) {
    if (is_initialized_) {
        return true;
    }
    
    if (!joystick) {
        printf("错误: JS16TMR摇杆直接连接实例为空\n");
        return false;
    }
    
    js16tmr_joystick_direct_ = joystick;
    
    printf("初始化JS16TMR摇杆直接连接处理器...\n");
    printf("死区设置: %d\n", deadzone_);
    printf("方向比例: %.2f\n", direction_ratio_);
    printf("旋转角度: %s\n", getRotationString(rotation_).c_str());
    
    is_initialized_ = true;
    printf("JS16TMR摇杆直接连接处理器初始化完成\n");
    
    return true;
}

void JS16TMRJoystickHandler::update() {
    if (!is_initialized_) return;
    
    int16_t x, y;
    bool button;
    
    if (js16tmr_joystick_direct_) {
        // 使用JS16TMR摇杆直接连接
        x = js16tmr_joystick_direct_->get_joy_adc_12bits_offset_value_x();
        y = js16tmr_joystick_direct_->get_joy_adc_12bits_offset_value_y();
        button = (js16tmr_joystick_direct_->get_button_value() == 0); // 0表示按下，1表示释放
        
        // 更新LED状态
        js16tmr_joystick_direct_->update_led_from_joystick();
    } else {
        return;
    }
    
    // 处理方向
    JoystickDirection current_direction = processDirection(x, y);
    
    // 应用旋转
    current_direction = applyRotation(current_direction);
    
    // 更新状态（静默更新，不输出日志）
    last_direction_ = current_direction;
    last_button_state_ = button;
}

JoystickDirection JS16TMRJoystickHandler::processDirection(int16_t x, int16_t y) {
    // 应用死区
    if (abs(x) < deadzone_ && abs(y) < deadzone_) {
        return JoystickDirection::CENTER;
    }
    
    // 如果按钮按下，使用更大的死区来防止误触发
    if (last_button_state_) {
        int16_t button_deadzone = deadzone_ * 2;  // 按钮按下时使用2倍死区（从3倍减少）
        if (abs(x) < button_deadzone && abs(y) < button_deadzone) {
            return JoystickDirection::CENTER;
        }
    }
    
    // 计算方向角度
    float angle = atan2(y, x) * 180.0f / 3.14159f;
    if (angle < 0) angle += 360.0f;
    
    // 确定主要方向
    if (angle >= 315.0f || angle < 45.0f) {
        return JoystickDirection::RIGHT;
    } else if (angle >= 45.0f && angle < 135.0f) {
        return JoystickDirection::DOWN;
    } else if (angle >= 135.0f && angle < 225.0f) {
        return JoystickDirection::LEFT;
    } else {
        return JoystickDirection::UP;
    }
}

JoystickDirection JS16TMRJoystickHandler::getCurrentDirection() const {
    return last_direction_;
}

bool JS16TMRJoystickHandler::isButtonPressed() const {
    return last_button_state_;
}

bool JS16TMRJoystickHandler::isButtonJustPressed() const {
    // 这里需要跟踪按钮状态变化
    return last_button_state_;
}

bool JS16TMRJoystickHandler::isButtonJustReleased() const {
    // 这里需要跟踪按钮状态变化
    return !last_button_state_;
}

void JS16TMRJoystickHandler::setDeadzone(int16_t deadzone) {
    deadzone_ = deadzone;
    printf("设置JS16TMR摇杆死区: %d\n", deadzone_);
}

void JS16TMRJoystickHandler::setDirectionRatio(float ratio) {
    direction_ratio_ = ratio;
    printf("设置JS16TMR方向比例: %.2f\n", direction_ratio_);
}

void JS16TMRJoystickHandler::setRotation(JoystickRotation rotation) {
    rotation_ = rotation;
    printf("设置JS16TMR摇杆旋转: %s\n", getRotationString(rotation_).c_str());
}

int16_t JS16TMRJoystickHandler::getDeadzone() const {
    return deadzone_;
}

float JS16TMRJoystickHandler::getDirectionRatio() const {
    return direction_ratio_;
}

JoystickRotation JS16TMRJoystickHandler::getRotation() const {
    return rotation_;
}

std::string JS16TMRJoystickHandler::getDirectionString(JoystickDirection direction) {
    switch (direction) {
        case JoystickDirection::UP:     return "UP";
        case JoystickDirection::DOWN:   return "DOWN";
        case JoystickDirection::LEFT:   return "LEFT";
        case JoystickDirection::RIGHT:  return "RIGHT";
        case JoystickDirection::CENTER: return "CENTER";
        default:                        return "UNKNOWN";
    }
}

std::string JS16TMRJoystickHandler::getRotationString(JoystickRotation rotation) {
    switch (rotation) {
        case JoystickRotation::ROTATION_0:   return "0°";
        case JoystickRotation::ROTATION_90:  return "90°";
        case JoystickRotation::ROTATION_180: return "180°";
        case JoystickRotation::ROTATION_270: return "270°";
        default:                             return "UNKNOWN";
    }
}

JoystickDirection JS16TMRJoystickHandler::applyRotation(JoystickDirection original_direction) const {
    if (rotation_ == JoystickRotation::ROTATION_0) {
        return original_direction;
    }
    
    // 定义旋转映射表
    static const JoystickDirection rotation_maps[4][5] = {
        // ROTATION_0: 原始方向
        {JoystickDirection::CENTER, JoystickDirection::UP, JoystickDirection::DOWN, JoystickDirection::LEFT, JoystickDirection::RIGHT},
        // ROTATION_90: 顺时针旋转90度 (UP->RIGHT, RIGHT->DOWN, DOWN->LEFT, LEFT->UP)
        {JoystickDirection::CENTER, JoystickDirection::RIGHT, JoystickDirection::LEFT, JoystickDirection::UP, JoystickDirection::DOWN},
        // ROTATION_180: 旋转180度 (UP->DOWN, DOWN->UP, LEFT->RIGHT, RIGHT->LEFT)
        {JoystickDirection::CENTER, JoystickDirection::DOWN, JoystickDirection::UP, JoystickDirection::RIGHT, JoystickDirection::LEFT},
        // ROTATION_270: 逆时针旋转90度 (UP->LEFT, LEFT->DOWN, DOWN->RIGHT, RIGHT->UP)
        {JoystickDirection::CENTER, JoystickDirection::LEFT, JoystickDirection::RIGHT, JoystickDirection::DOWN, JoystickDirection::UP}
    };
    
    // 获取原始方向的索引
    int original_index = static_cast<int>(original_direction);
    int rotation_index = static_cast<int>(rotation_);
    
    // 返回旋转后的方向
    return rotation_maps[rotation_index][original_index];
}

} // namespace js16tmr
