#pragma once

// I2C Configuration
#define JOYSTICK_I2C_PORT i2c1        // Use i2c1 bus
#define JOYSTICK_I2C_SDA_PIN 6        // I2C SDA pin
#define JOYSTICK_I2C_SCL_PIN 7        // I2C SCL pin
#define JOYSTICK_I2C_ADDR 0x63        // Joystick Unit I2C address
#define JOYSTICK_I2C_SPEED 100000     // 100 kHz

// LED color definitions
#define JOYSTICK_LED_OFF  0x000000    // Black (off)
#define JOYSTICK_LED_RED  0xFF0000    // Red
#define JOYSTICK_LED_GREEN 0x00FF00   // Green
#define JOYSTICK_LED_BLUE 0x0000FF    // Blue

// Operation detection thresholds
#define JOYSTICK_THRESHOLD 1800       // Increased joystick threshold to reduce false triggers
#define JOYSTICK_LOOP_DELAY_MS 20     // Loop delay time (milliseconds)
#define JOYSTICK_PRINT_INTERVAL_MS 250 // Repeat print interval (milliseconds)
#define JOYSTICK_DIRECTION_RATIO 1.5  // Direction determination ratio 