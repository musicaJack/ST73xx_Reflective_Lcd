#ifndef _JS16TMR_JOYSTICK_DIRECT_H_
#define _JS16TMR_JOYSTICK_DIRECT_H_

#include <hardware/adc.h>
#include <pico/stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

// JS16TMR摇杆引脚配置
#define JS16TMR_JOYSTICK_PIN_X     26          // X轴ADC引脚 (GP26, ADC0)
#define JS16TMR_JOYSTICK_PIN_Y     27          // Y轴ADC引脚 (GP27, ADC1)  
#define JS16TMR_JOYSTICK_PIN_SW    22          // 开关引脚 (GP22)

// ADC配置
#define JS16TMR_JOYSTICK_ADC_X_CHANNEL  0      // ADC通道0 (GP26)
#define JS16TMR_JOYSTICK_ADC_Y_CHANNEL  1      // ADC通道1 (GP27)

// JS16TMR摇杆操作参数配置
#define JS16TMR_JOYSTICK_THRESHOLD      1800   // 操作检测阈值
#define JS16TMR_JOYSTICK_LOOP_DELAY_MS  20     // 循环延迟时间（毫秒）
#define JS16TMR_JOYSTICK_DEADZONE       1000   // 摇杆死区阈值

// LED配置
#define JS16TMR_JOYSTICK_LED_PIN        25      // PICO主板LED引脚 (GP25)

// ADC模式枚举定义
typedef enum { ADC_8BIT_RESULT = 0, ADC_16BIT_RESULT } adc_mode_t;

/**
 * @brief JS16TMR直接连接摇杆控制类
 * @description 使用RP2040内置ADC直接读取JS16TMR摇杆的模拟信号
 * 硬件连接：
 * - X轴: GP26 (ADC0)
 * - Y轴: GP27 (ADC1) 
 * - 开关: GP22 (数字输入，内部上拉)
 */
class JS16TMRJoystickDirect {
public:
    /**
     * @brief 摇杆初始化
     * @return true 成功, false 失败
     */
    bool begin();

    // 实现与 Joystick 类相同的接口
    uint16_t get_joy_adc_value_x(adc_mode_t adc_bits);
    uint16_t get_joy_adc_value_y(adc_mode_t adc_bits);
    uint8_t get_button_value(void);
    int16_t get_joy_adc_12bits_offset_value_x(void);
    int16_t get_joy_adc_12bits_offset_value_y(void);
    void get_joy_adc_16bits_value_xy(uint16_t *adc_x, uint16_t *adc_y);
    void get_joy_adc_8bits_value_xy(uint8_t *adc_x, uint8_t *adc_y);
    
    // LED控制方法
    void set_led(bool state);
    void update_led_from_joystick();

private:
    uint16_t _center_x;
    uint16_t _center_y;
    bool _calibrated;
    
    // TMR高频滤波器
    uint16_t _filter_x[4];  // 减少滤波器长度，提高响应速度
    uint16_t _filter_y[4];
    uint8_t _filter_index;
    bool _filter_initialized;
    
    // 滞回控制
    int16_t _last_stable_x;
    int16_t _last_stable_y;
    bool _hysteresis_initialized;

    // 辅助函数
    void calibrate_center(void);
    uint16_t read_adc_channel(uint channel);
    uint16_t apply_filter(uint16_t new_value, uint16_t* filter_buffer);
};

#endif
