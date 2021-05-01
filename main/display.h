#pragma once

#include <esp_err.h>
#include <driver/gpio.h>
#include <driver/i2c.h>

esp_err_t display_init(i2c_port_t port, gpio_num_t  sda, gpio_num_t scl);
esp_err_t display_draw_text(char* text, uint8_t lune_num);
esp_err_t display_clear(void);
