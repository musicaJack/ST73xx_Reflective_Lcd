/**
 * @file spi_config.hpp
 * @brief SPI Configuration File
 * @version 1.0.0
 * 
 * Default wiring scheme (modify according to actual wiring):
 * GPIO10(SCK) -> SPI clock signal
 * GPIO11(MISO) -> Master In Slave Out
 * GPIO12(MOSI) -> Master Out Slave In  
 * GPIO13(CS) -> Chip Select
 * VCC -> 3.3V
 * GND -> GND
 */

#pragma once

#include "pico/stdlib.h"
#include "hardware/spi.h"

namespace MicroSD {

/**
 * @brief SPI Configuration Structure
 */
struct SPIConfig {
    spi_inst_t* spi_port = spi1;          // SPI port 1
    uint32_t clk_slow = 400 * 1000;       // Slow clock frequency (400KHz for initialization)
    uint32_t clk_fast = 40 * 1000 * 1000; // Fast clock frequency (40MHz for normal operation)
    uint pin_miso = 7;                     // MISO pin (GPIO7)
    uint pin_cs = 1;                       // CS pin (GPIO1)
    uint pin_sck = 6;                      // SCK pin (GPIO6)
    uint pin_mosi = 0;                     // MOSI pin (GPIO0)
    bool use_internal_pullup = true;       // Use internal pull-up resistors
};

} // namespace MicroSD 