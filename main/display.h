#pragma once

#include <esp_err.h>
#include <driver/gpio.h>
#include <driver/i2c.h>

esp_err_t display_init(i2c_port_t port, gpio_num_t  sda, gpio_num_t scl, gpio_num_t rst);
esp_err_t display_draw_text(char* line1, char* line2, char* line3, char* line4);
esp_err_t display_clear(void);
esp_err_t display_draw_animation(void);
