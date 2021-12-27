#pragma once

#include "display.h"
#include "tf_gesture_predictor.h"
esp_err_t menu_init(i2c_port_t port, gpio_num_t  sda, gpio_num_t scl, gpio_num_t rst);

esp_err_t menu_draw_crypto(double bitcoinPrice, double bitcoinChange24h, double dogePrice, double dogeChange24h);
esp_err_t menu_draw_gestures(gesture_prediction_t* prediction);
esp_err_t menu_draw_connection_status(char* wifi_ip, char* ble_addr);
esp_err_t menu_next_page(void);