#pragma once

#include <cstdint>
#include <string>

// 前向声明
class JS16TMRJoystickDirect;

namespace js16tmr {

// 摇杆方向枚举
enum class JoystickDirection {
    CENTER,
    UP,
    DOWN,
    LEFT,
    RIGHT
};

// 旋转角度枚举
enum class JoystickRotation {
    ROTATION_0,    // 0度，原始方向
    ROTATION_90,   // 90度，顺时针旋转
    ROTATION_180,  // 180度，旋转
    ROTATION_270   // 270度，逆时针旋转
};

/**
 * @brief JS16TMR摇杆处理器类
 * 
 * 负责处理JS16TMR摇杆输入，包括方向检测、死区处理、方向旋转等
 */
class JS16TMRJoystickHandler {
public:
    JS16TMRJoystickHandler();
    ~JS16TMRJoystickHandler();

    // 禁用拷贝构造和赋值
    JS16TMRJoystickHandler(const JS16TMRJoystickHandler&) = delete;
    JS16TMRJoystickHandler& operator=(const JS16TMRJoystickHandler&) = delete;

    // 初始化和更新
    bool initialize(JS16TMRJoystickDirect* joystick);
    void update();

    // 方向处理
    JoystickDirection processDirection(int16_t x, int16_t y);
    JoystickDirection getCurrentDirection() const;

    // 按钮状态
    bool isButtonPressed() const;
    bool isButtonJustPressed() const;
    bool isButtonJustReleased() const;

    // 配置
    void setDeadzone(int16_t deadzone);
    void setDirectionRatio(float ratio);
    void setRotation(JoystickRotation rotation);
    int16_t getDeadzone() const;
    float getDirectionRatio() const;
    JoystickRotation getRotation() const;

    // 工具方法
    std::string getDirectionString(JoystickDirection direction);
    std::string getRotationString(JoystickRotation rotation);
    JoystickDirection applyRotation(JoystickDirection original_direction) const;

private:
    bool is_initialized_;
    JS16TMRJoystickDirect* js16tmr_joystick_direct_;
    int16_t deadzone_;
    float direction_ratio_;
    JoystickRotation rotation_;
    JoystickDirection last_direction_;
    bool last_button_state_;
};

} // namespace js16tmr
