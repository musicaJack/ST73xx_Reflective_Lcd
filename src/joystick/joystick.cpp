
#include "joystick.hpp"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <cstring> // For memcpy

// Helper function for reading bytes from a specific register
static inline int reg_read(i2c_inst_t *i2c, uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t nbytes) {
    // Send the register address we want to read from
    int ret = i2c_write_blocking(i2c, addr, &reg, 1, true); // true to keep control of bus
    if (ret < 0) {
        return ret;
    }
    // Read the data from the register
    ret = i2c_read_blocking(i2c, addr, buf, nbytes, false); // false to release bus
    return ret;
}

// Helper function for writing bytes to a specific register
static inline int reg_write(i2c_inst_t *i2c, uint8_t addr, uint8_t reg, const uint8_t *buf, uint8_t nbytes) {
    uint8_t msg[nbytes + 1];
    // First byte is the register address
    msg[0] = reg;
    // Copy data bytes
    memcpy(msg + 1, buf, nbytes);
    // Write the register address followed by the data
    int ret = i2c_write_blocking(i2c, addr, msg, nbytes + 1, false); // false to release bus
    return ret;
}

bool Joystick::begin(i2c_inst_t *i2c_port, uint8_t addr, uint sda_pin, uint scl_pin, uint32_t speed)
{
    _i2c_port = i2c_port;
    _addr     = addr;
    _sda_pin  = sda_pin;
    _scl_pin  = scl_pin;
    _speed    = speed;

    // Initialize I2C port at requested speed
    i2c_init(_i2c_port, _speed);

    // Initialize I2C pins
    gpio_set_function(_sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(_scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(_sda_pin);
    gpio_pull_up(_scl_pin);

    sleep_ms(10);

    // Check device presence by trying to write nothing (just address)
    uint8_t dummy_byte; // A dummy byte to satisfy write function, won't be sent
    int ret = i2c_write_blocking(_i2c_port, _addr, &dummy_byte, 0, false);

    return (ret >= 0); // Returns PICO_ERROR_GENERIC (-1) if NACK received (no device)
}

uint16_t Joystick::get_joy_adc_value_x(adc_mode_t adc_bits)
{
    uint8_t data[2];
    uint16_t value = 0;
    int ret;

    if (adc_bits == ADC_16BIT_RESULT) {
        uint8_t reg = JOYSTICK_ADC_VALUE_12BITS_REG; // Reads X and Y together
        uint8_t temp_data[4];
        ret = reg_read(_i2c_port, _addr, reg, temp_data, 4);
        if (ret == 4) {
             memcpy(&value, &temp_data[0], 2); // Extract X value
        }
    } else if (adc_bits == ADC_8BIT_RESULT) {
        uint8_t reg = JOYSTICK_ADC_VALUE_8BITS_REG; // Reads X and Y together
        uint8_t temp_data[2];
        ret = reg_read(_i2c_port, _addr, reg, temp_data, 2);
        if (ret == 2) {
            value = temp_data[0]; // Extract X value
        }
    }
    return value;
}

void Joystick::get_joy_adc_16bits_value_xy(uint16_t *adc_x, uint16_t *adc_y)
{
    uint8_t data[4];
    uint8_t reg = JOYSTICK_ADC_VALUE_12BITS_REG;
    int ret = reg_read(_i2c_port, _addr, reg, data, 4);
    if (ret == 4) {
        memcpy(adc_x, &data[0], 2);
        memcpy(adc_y, &data[2], 2);
    } else {
        *adc_x = 0;
        *adc_y = 0;
    }
}

void Joystick::get_joy_adc_8bits_value_xy(uint8_t *adc_x, uint8_t *adc_y)
{
    uint8_t data[2];
    uint8_t reg = JOYSTICK_ADC_VALUE_8BITS_REG;
    int ret = reg_read(_i2c_port, _addr, reg, data, 2);
     if (ret == 2) {
        *adc_x = data[0];
        *adc_y = data[1];
    } else {
        *adc_x = 0;
        *adc_y = 0;
    }
}

uint16_t Joystick::get_joy_adc_value_y(adc_mode_t adc_bits)
{
    uint8_t data[2];
    uint16_t value = 0;
    int ret;

    if (adc_bits == ADC_16BIT_RESULT) {
        uint8_t reg = JOYSTICK_ADC_VALUE_12BITS_REG; // Reads X and Y together
        uint8_t temp_data[4];
        ret = reg_read(_i2c_port, _addr, reg, temp_data, 4);
        if (ret == 4) {
             memcpy(&value, &temp_data[2], 2); // Extract Y value
        }
    } else if (adc_bits == ADC_8BIT_RESULT) {
        uint8_t reg = JOYSTICK_ADC_VALUE_8BITS_REG; // Reads X and Y together
        uint8_t temp_data[2];
        ret = reg_read(_i2c_port, _addr, reg, temp_data, 2);
        if (ret == 2) {
            value = temp_data[1]; // Extract Y value
        }
    }
    return value;
}

int16_t Joystick::get_joy_adc_12bits_offset_value_x(void)
{
    int16_t value = 0;
    uint8_t reg = JOYSTICK_OFFSET_ADC_VALUE_12BITS_REG;
    reg_read(_i2c_port, _addr, reg, (uint8_t *)&value, 2);
    return value;
}

int16_t Joystick::get_joy_adc_12bits_offset_value_y(void)
{
    int16_t value = 0;
    uint8_t reg = JOYSTICK_OFFSET_ADC_VALUE_12BITS_REG + 2;
    reg_read(_i2c_port, _addr, reg, (uint8_t *)&value, 2);
    return value;
}

int8_t Joystick::get_joy_adc_8bits_offset_value_x(void)
{
    int8_t value = 0;
    uint8_t reg = JOYSTICK_OFFSET_ADC_VALUE_8BITS_REG;
    reg_read(_i2c_port, _addr, reg, (uint8_t *)&value, 1);
    return value;
}

int8_t Joystick::get_joy_adc_8bits_offset_value_y(void)
{
    int8_t value = 0;
    uint8_t reg = JOYSTICK_OFFSET_ADC_VALUE_8BITS_REG + 1;
    reg_read(_i2c_port, _addr, reg, (uint8_t *)&value, 1);
    return value;
}

void Joystick::set_joy_adc_value_cal(uint16_t x_neg_min, uint16_t x_neg_max, uint16_t x_pos_min,
                                 uint16_t x_pos_max, uint16_t y_neg_min, uint16_t y_neg_max,
                                 uint16_t y_pos_min, uint16_t y_pos_max)
{
    uint8_t data[16];
    memcpy(&data[0], (uint8_t *)&x_neg_min, 2);
    memcpy(&data[2], (uint8_t *)&x_neg_max, 2);
    memcpy(&data[4], (uint8_t *)&x_pos_min, 2);
    memcpy(&data[6], (uint8_t *)&x_pos_max, 2);
    memcpy(&data[8], (uint8_t *)&y_neg_min, 2);
    memcpy(&data[10], (uint8_t *)&y_neg_max, 2);
    memcpy(&data[12], (uint8_t *)&y_pos_min, 2);
    memcpy(&data[14], (uint8_t *)&y_pos_max, 2);

    reg_write(_i2c_port, _addr, JOYSTICK_ADC_VALUE_CAL_REG, data, 16);
}

void Joystick::get_joy_adc_value_cal(uint16_t *x_neg_min, uint16_t *x_neg_max, uint16_t *x_pos_min,
                                 uint16_t *x_pos_max, uint16_t *y_neg_min, uint16_t *y_neg_max,
                                 uint16_t *y_pos_min, uint16_t *y_pos_max)
{
    uint8_t data[16];
    int ret = reg_read(_i2c_port, _addr, JOYSTICK_ADC_VALUE_CAL_REG, data, 16);
    if (ret == 16) {
        memcpy((uint8_t *)x_neg_min, &data[0], 2);
        memcpy((uint8_t *)x_neg_max, &data[2], 2);
        memcpy((uint8_t *)x_pos_min, &data[4], 2);
        memcpy((uint8_t *)x_pos_max, &data[6], 2);
        memcpy((uint8_t *)y_neg_min, &data[8], 2);
        memcpy((uint8_t *)y_neg_max, &data[10], 2);
        memcpy((uint8_t *)y_pos_min, &data[12], 2);
        memcpy((uint8_t *)y_pos_max, &data[14], 2);
    } else {
        // Handle error by zeroing out the values
        memset(x_neg_min, 0, sizeof(uint16_t) * 8); // Zero out all params
    }
}

uint8_t Joystick::get_button_value(void)
{
    uint8_t data = 1; // Default to not pressed
    uint8_t reg = JOYSTICK_BUTTON_REG;
    reg_read(_i2c_port, _addr, reg, &data, 1);
    return data;
}

void Joystick::set_rgb_color(uint32_t color)
{
    // Color is sent as R, G, B, Brightness (4 bytes)
    reg_write(_i2c_port, _addr, JOYSTICK_RGB_REG, (uint8_t *)&color, 4);
}

uint32_t Joystick::get_rgb_color(void)
{
    uint32_t rgb_read_buff = 0;
    reg_read(_i2c_port, _addr, JOYSTICK_RGB_REG, (uint8_t *)&rgb_read_buff, 4);
    return rgb_read_buff;
}

uint8_t Joystick::get_firmware_version(void)
{
    uint8_t reg_value = 0;
    reg_read(_i2c_port, _addr, JOYSTICK_FIRMWARE_VERSION_REG, &reg_value, 1);
    return reg_value;
}

uint8_t Joystick::get_bootloader_version(void)
{
    uint8_t reg_value = 0;
    reg_read(_i2c_port, _addr, JOYSTICK_BOOTLOADER_VERSION_REG, &reg_value, 1);
    return reg_value;
}

uint8_t Joystick::get_i2c_address(void)
{
    uint8_t reg_value = 0;
    reg_read(_i2c_port, _addr, JOYSTICK_I2C_ADDRESS_REG, &reg_value, 1);
    return reg_value;
}

uint8_t Joystick::set_i2c_address(uint8_t new_addr)
{
    int ret = reg_write(_i2c_port, _addr, JOYSTICK_I2C_ADDRESS_REG, &new_addr, 1);
    if (ret > 0) {
        _addr = new_addr;
        return 1;
    }
    return 0;
} 