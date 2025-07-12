/**
 * @file simple_button_test.cpp
 * @brief 简单按键测试程序
 */

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "spi_config.hpp"
#include <stdio.h>

int main() {
    // 初始化串口
    stdio_init_all();
    
    // 等待一段时间让系统稳定，但不强制等待USB连接
    sleep_ms(1000);
    
    printf("\n=== 简单按键测试程序 ===\n");
    printf("按键引脚: KEY1=GP%d, KEY2=GP%d\n", BUTTON_KEY1_PIN, BUTTON_KEY2_PIN);
    printf("按下按键查看输出...\n\n");
    
    // 初始化GPIO
    gpio_init(BUTTON_KEY1_PIN);
    gpio_init(BUTTON_KEY2_PIN);
    gpio_set_dir(BUTTON_KEY1_PIN, GPIO_IN);
    gpio_set_dir(BUTTON_KEY2_PIN, GPIO_IN);
    
    // 尝试不同的上拉配置
    printf("测试1: 禁用上拉电阻\n");
    gpio_disable_pulls(BUTTON_KEY1_PIN);
    gpio_disable_pulls(BUTTON_KEY2_PIN);
    
    for (int i = 0; i < 10; i++) {
        bool key1_raw = gpio_get(BUTTON_KEY1_PIN);
        bool key2_raw = gpio_get(BUTTON_KEY2_PIN);
        printf("KEY1: %d, KEY2: %d\n", key1_raw, key2_raw);
        sleep_ms(500);
    }
    
    printf("\n测试2: 启用上拉电阻\n");
    gpio_pull_up(BUTTON_KEY1_PIN);
    gpio_pull_up(BUTTON_KEY2_PIN);
    
    for (int i = 0; i < 10; i++) {
        bool key1_raw = gpio_get(BUTTON_KEY1_PIN);
        bool key2_raw = gpio_get(BUTTON_KEY2_PIN);
        printf("KEY1: %d, KEY2: %d\n", key1_raw, key2_raw);
        sleep_ms(500);
    }
    
    printf("\n测试3: 启用下拉电阻\n");
    gpio_pull_down(BUTTON_KEY1_PIN);
    gpio_pull_down(BUTTON_KEY2_PIN);
    
    for (int i = 0; i < 10; i++) {
        bool key1_raw = gpio_get(BUTTON_KEY1_PIN);
        bool key2_raw = gpio_get(BUTTON_KEY2_PIN);
        printf("KEY1: %d, KEY2: %d\n", key1_raw, key2_raw);
        sleep_ms(500);
    }
    
    printf("\n测试完成！\n");
    printf("如果按键按下时电平变化，说明连接正常\n");
    printf("如果电平始终不变，请检查按键连接\n");
    
    while (true) {
        sleep_ms(1000);
    }
    
    return 0;
} 