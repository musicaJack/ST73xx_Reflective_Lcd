#include "js16tmr_joystick/js16tmr_joystick_direct.hpp"
#include "hardware/adc.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <cstdlib>

bool JS16TMRJoystickDirect::begin() {
    _calibrated = false;
    _filter_index = 0;
    _filter_initialized = false;
    _hysteresis_initialized = false;
    _last_stable_x = 0;
    _last_stable_y = 0;
    
    // 初始化TMR高频滤波器缓冲区
    for (int i = 0; i < 4; i++) {
        _filter_x[i] = 0;
        _filter_y[i] = 0;
    }
    
    // 初始化ADC
    adc_init();
    
    // 配置ADC引脚
    adc_gpio_init(JS16TMR_JOYSTICK_PIN_X);  // GP26 -> ADC0
    adc_gpio_init(JS16TMR_JOYSTICK_PIN_Y);  // GP27 -> ADC1
    
    // 初始化数字开关引脚
    gpio_init(JS16TMR_JOYSTICK_PIN_SW);
    gpio_set_dir(JS16TMR_JOYSTICK_PIN_SW, GPIO_IN);
    gpio_pull_up(JS16TMR_JOYSTICK_PIN_SW);  // 内部上拉，按下时为低电平
    
    // 初始化LED引脚
    gpio_init(JS16TMR_JOYSTICK_LED_PIN);
    gpio_set_dir(JS16TMR_JOYSTICK_LED_PIN, GPIO_OUT);
    gpio_put(JS16TMR_JOYSTICK_LED_PIN, 0);  // 初始状态关闭LED
    
    // 等待ADC稳定
    sleep_ms(100);
    
    // 预热ADC，丢弃前几次读数
    for (int i = 0; i < 5; i++) {
        adc_select_input(JS16TMR_JOYSTICK_ADC_X_CHANNEL);
        adc_read();
        adc_select_input(JS16TMR_JOYSTICK_ADC_Y_CHANNEL);
        adc_read();
        sleep_ms(10);
    }
    
    printf("JS16TMR摇杆直接连接初始化成功\n");
    printf("X轴引脚: GP%d (ADC%d)\n", JS16TMR_JOYSTICK_PIN_X, JS16TMR_JOYSTICK_ADC_X_CHANNEL);
    printf("Y轴引脚: GP%d (ADC%d)\n", JS16TMR_JOYSTICK_PIN_Y, JS16TMR_JOYSTICK_ADC_Y_CHANNEL);
    printf("开关引脚: GP%d\n", JS16TMR_JOYSTICK_PIN_SW);
    printf("LED引脚: GP%d\n", JS16TMR_JOYSTICK_LED_PIN);
    
    // 校准中心点
    calibrate_center();
    
    return true;
}

uint16_t JS16TMRJoystickDirect::read_adc_channel(uint channel) {
    // 选择ADC通道
    adc_select_input(channel);
    
    // JS16TMR摇杆优化：适中的采样次数和延迟
    uint32_t sum = 0;
    const int samples = 4;  // JS16TMR摇杆适中的采样次数
    
    for (int i = 0; i < samples; i++) {
        sum += adc_read();
        sleep_us(100);  // 适中的延迟，提高稳定性
    }
    
    return sum / samples;
}

uint16_t JS16TMRJoystickDirect::get_joy_adc_value_x(adc_mode_t adc_bits) {
    uint16_t value = read_adc_channel(JS16TMR_JOYSTICK_ADC_X_CHANNEL);
    
    // 应用移动平均滤波
    value = apply_filter(value, _filter_x);
    
    if (adc_bits == ADC_8BIT_RESULT) {
        value = value >> 4; // 转换为 8 位 (0-255)
    }
    return value;
}

uint16_t JS16TMRJoystickDirect::get_joy_adc_value_y(adc_mode_t adc_bits) {
    uint16_t value = read_adc_channel(JS16TMR_JOYSTICK_ADC_Y_CHANNEL);
    
    // 应用移动平均滤波
    value = apply_filter(value, _filter_y);
    
    if (adc_bits == ADC_8BIT_RESULT) {
        value = value >> 4; // 转换为 8 位 (0-255)
    }
    return value;
}

uint8_t JS16TMRJoystickDirect::get_button_value(void) {
    // 读取数字开关引脚，低电平表示按下
    return gpio_get(JS16TMR_JOYSTICK_PIN_SW);
}

int16_t JS16TMRJoystickDirect::get_joy_adc_12bits_offset_value_x(void) {
    if (!_calibrated) {
        calibrate_center();
    }
    uint16_t value = get_joy_adc_value_x(ADC_16BIT_RESULT);
    int16_t offset = (int16_t)(value - _center_x);
    
    // 应用滞回控制
    if (!_hysteresis_initialized) {
        _last_stable_x = offset;
        _hysteresis_initialized = true;
        return offset;
    }
    
    // JS16TMR摇杆配置：适中的滞回阈值
    const int16_t hysteresis_threshold = 20;
    if (abs(offset - _last_stable_x) > hysteresis_threshold) {
        _last_stable_x = offset;
    }
    
    return _last_stable_x;
}

int16_t JS16TMRJoystickDirect::get_joy_adc_12bits_offset_value_y(void) {
    if (!_calibrated) {
        calibrate_center();
    }
    uint16_t value = get_joy_adc_value_y(ADC_16BIT_RESULT);
    int16_t offset = (int16_t)(value - _center_y);
    
    // 应用滞回控制
    if (!_hysteresis_initialized) {
        _last_stable_y = offset;
        return offset;
    }
    
    // JS16TMR摇杆配置：适中的滞回阈值
    const int16_t hysteresis_threshold = 20;
    if (abs(offset - _last_stable_y) > hysteresis_threshold) {
        _last_stable_y = offset;
    }
    
    return _last_stable_y;
}

void JS16TMRJoystickDirect::get_joy_adc_16bits_value_xy(uint16_t *adc_x, uint16_t *adc_y) {
    *adc_x = get_joy_adc_value_x(ADC_16BIT_RESULT);
    *adc_y = get_joy_adc_value_y(ADC_16BIT_RESULT);
}

void JS16TMRJoystickDirect::get_joy_adc_8bits_value_xy(uint8_t *adc_x, uint8_t *adc_y) {
    *adc_x = get_joy_adc_value_x(ADC_8BIT_RESULT);
    *adc_y = get_joy_adc_value_y(ADC_8BIT_RESULT);
}

void JS16TMRJoystickDirect::calibrate_center(void) {
    printf("正在校准JS16TMR摇杆中心点...\n");
    printf("请确保摇杆处于中心位置，不要移动...\n");
    
    uint32_t sum_x = 0, sum_y = 0;
    const int samples = 20;  // JS16TMR摇杆适中的样本数量
    
    // 读取多个样本并取平均值
    for (int i = 0; i < samples; i++) {
        sum_x += get_joy_adc_value_x(ADC_16BIT_RESULT);
        sum_y += get_joy_adc_value_y(ADC_16BIT_RESULT);
        sleep_ms(20);  // 适中的延迟时间
        
        // 每10次显示进度
        if ((i + 1) % 10 == 0) {
            printf("校准进度: %d/%d\n", i + 1, samples);
        }
    }
    
    _center_x = sum_x / samples;
    _center_y = sum_y / samples;
    _calibrated = true;
    
    printf("校准完成 - 中心点: X=%d, Y=%d\n", _center_x, _center_y);
    printf("请移动摇杆测试...\n");
}

void JS16TMRJoystickDirect::set_led(bool state) {
    gpio_put(JS16TMR_JOYSTICK_LED_PIN, state ? 1 : 0);
}

void JS16TMRJoystickDirect::update_led_from_joystick() {
    // 获取摇杆偏移值
    int16_t x_offset = get_joy_adc_12bits_offset_value_x();
    int16_t y_offset = get_joy_adc_12bits_offset_value_y();
    bool button_pressed = (get_button_value() == 0);  // 0表示按下
    
    // JS16TMR摇杆配置：适中的死区设置
    const int16_t deadzone = 200;  // JS16TMR摇杆适中的死区阈值
    bool joystick_active = (abs(x_offset) > deadzone) || 
                          (abs(y_offset) > deadzone) || 
                          button_pressed;
    
    // 根据摇杆状态控制LED
    set_led(joystick_active);
}

uint16_t JS16TMRJoystickDirect::apply_filter(uint16_t new_value, uint16_t* filter_buffer) {
    // 更新滤波器缓冲区
    filter_buffer[_filter_index] = new_value;
    _filter_index = (_filter_index + 1) % 4;
    
    // 如果滤波器未初始化，用当前值填充所有缓冲区
    if (!_filter_initialized) {
        for (int i = 0; i < 4; i++) {
            filter_buffer[i] = new_value;
        }
        _filter_initialized = true;
    }
    
    // TMR高频优化：简单移动平均，减少计算延迟
    uint32_t sum = 0;
    for (int i = 0; i < 4; i++) {
        sum += filter_buffer[i];
    }
    
    return sum / 4;
}
