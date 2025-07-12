
#ifndef _MY_JOYSTICK_H_
#define _MY_JOYSTICK_H_

#include <hardware/i2c.h>
#include <pico/stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define JOYSTICK_ADDR                        0x63
#define JOYSTICK_ADC_VALUE_12BITS_REG        0x00
#define JOYSTICK_ADC_VALUE_8BITS_REG         0x10
#define JOYSTICK_BUTTON_REG                  0x20
#define JOYSTICK_RGB_REG                     0x30
#define JOYSTICK_ADC_VALUE_CAL_REG           0x40
#define JOYSTICK_OFFSET_ADC_VALUE_12BITS_REG 0x50
#define JOYSTICK_OFFSET_ADC_VALUE_8BITS_REG  0x60
#define JOYSTICK_FIRMWARE_VERSION_REG        0xFE
#define JOYSTICK_BOOTLOADER_VERSION_REG      0xFC
#define JOYSTICK_I2C_ADDRESS_REG             0xFF

typedef enum { ADC_8BIT_RESULT = 0, ADC_16BIT_RESULT } adc_mode_t;

/**
 * @brief Joystick control API
 */
class Joystick {
public:
    /**
     * @brief Joystick initialization
     * @param i2c_port I2C port
     * @param addr I2C address
     * @param sda_pin SDA Pin
     * @param scl_pin SCL Pin
     * @param speed I2C clock
     * @return 1 success, 0 false
     */
    bool begin(i2c_inst_t *i2c_port, uint8_t addr = JOYSTICK_ADDR, uint sda_pin = 21, uint scl_pin = 22,
               uint32_t speed = 400000UL);

    /**
     * @brief Set Joystick I2C address
     * @param addr I2C address
     * @return 1 success, 0 false
     */
    uint8_t set_i2c_address(uint8_t addr);

    /**
     * @brief Get Joystick I2C address
     * @return I2C address
     */
    uint8_t get_i2c_address(void);

    /**
     * @brief Get Joystick firmware version
     * @return firmware version
     */
    uint8_t get_firmware_version(void);

    /**
     * @brief Get Joystick bootloader version
     * @return bootloader version
     */
    uint8_t get_bootloader_version(void);

    /**
     * @brief Get Joystick x-axis ADC value
     * @param adc_bits ADC_8BIT_RESULT or ADC_16BIT_RESULT
     * @return x-axis ADC value
     */
    uint16_t get_joy_adc_value_x(adc_mode_t adc_bits);

    /**
     * @brief Get Joystick y-axis ADC value
     * @param adc_bits ADC_8BIT_RESULT or ADC_16BIT_RESULT
     * @return y-axis ADC value
     */
    uint16_t get_joy_adc_value_y(adc_mode_t adc_bits);

    /**
     * @brief Get Joystick button value
     * @return 0 press, 1 no press
     */
    uint8_t get_button_value(void);

    /**
     * @brief Set Joystick RGB color
     * @param color rgb color
     */
    void set_rgb_color(uint32_t color);

    /**
     * @brief Get Joystick RGB color
     * @return rgb color
     */
    uint32_t get_rgb_color(void);

    /**
     * @brief Get Joystick mapped calibration values
     * @param x_neg_min pointer of x-axis negative minimum value
     * @param x_neg_max pointer of x-axis negative maximum value
     * @param x_pos_min pointer of x-axis positive minimum value
     * @param x_pos_max pointer of x-axis positive maximum value
     * @param y_neg_min pointer of y-axis negative minimum value
     * @param y_neg_max pointer of y-axis negative maximum value
     * @param y_pos_min pointer of y-axis positive minimum value
     * @param y_pos_max pointer of y-axis positive maximum value
     */
    void get_joy_adc_value_cal(uint16_t *x_neg_min, uint16_t *x_neg_max, uint16_t *x_pos_min, uint16_t *x_pos_max,
                               uint16_t *y_neg_min, uint16_t *y_neg_max, uint16_t *y_pos_min, uint16_t *y_pos_max);

    /**
     * @brief Set Joystick mapped calibration values
     * @param x_neg_min x-axis negative minimum value
     * @param x_neg_max x-axis negative maximum value
     * @param x_pos_min x-axis positive minimum value
     * @param x_pos_max x-axis positive maximum value
     * @param y_neg_min y-axis negative minimum value
     * @param y_neg_max y-axis negative maximum value
     * @param y_pos_min y-axis positive minimum value
     * @param y_pos_max y-axis positive maximum value
     */
    void set_joy_adc_value_cal(uint16_t x_neg_min, uint16_t x_neg_max, uint16_t x_pos_min, uint16_t x_pos_max,
                               uint16_t y_neg_min, uint16_t y_neg_max, uint16_t y_pos_min, uint16_t y_pos_max);

    /**
     * @brief Get Joystick x-axis 12bits mapped value
     * @return x-axis 12bits mapped value
     */
    int16_t get_joy_adc_12bits_offset_value_x(void);

    /**
     * @brief Get Joystick y-axis 12bits mapped value
     * @return y-axis 12bits mapped value
     */
    int16_t get_joy_adc_12bits_offset_value_y(void);

    /**
     * @brief Get Joystick x-axis 8bits mapped value
     * @return x-axis 8bits mapped value
     */
    int8_t get_joy_adc_8bits_offset_value_x(void);

    /**
     * @brief Get Joystick y-axis 8bits mapped value
     * @return y-axis 8bits mapped value
     */
    int8_t get_joy_adc_8bits_offset_value_y(void);

    /**
     * @brief Get Joystick x-axis and y-axis 16bits ADC values
     * @param adc_x pointer of x-axis ADC value
     * @param adc_y pointer of y-axis ADC value
     */
    void get_joy_adc_16bits_value_xy(uint16_t *adc_x, uint16_t *adc_y);

    /**
     * @brief Get Joystick x-axis and y-axis 8bits ADC values
     * @param adc_x pointer of x-axis ADC value
     * @param adc_y pointer of y-axis ADC value
     */
    void get_joy_adc_8bits_value_xy(uint8_t *adc_x, uint8_t *adc_y);

private:
    i2c_inst_t *_i2c_port;
    uint8_t _addr;
    uint _scl_pin;
    uint _sda_pin;
    uint32_t _speed;
};

#endif 