#pragma once

#include "display.h"
#include "tf_gesture_predictor.h"

typedef enum page_t {
    PAGE_CRYPTO,
    PAGE_GESTURE,
    PAGE_CONNECTION,
    PAGE_HIDDEN_FIRST,
    PAGE_PAIRING = PAGE_HIDDEN_FIRST,
    PAGE_END
} page_t;

esp_err_t menu_init(i2c_port_t port, gpio_num_t  sda, gpio_num_t scl, gpio_num_t rst);

esp_err_t menu_draw_market_data(double bitcoinPrice, double bitcoinChange24h, double dogePrice, double dogeChange24h, double omxPrice, double omxChange24h, double nasdaqPrice, double nasdaqChange24h);
esp_err_t menu_draw_gestures(gesture_prediction_t* prediction);
esp_err_t menu_draw_connection_status(char* ssid, bool connected, char* wifi_ip, char* ble_addr);
esp_err_t menu_next_page(void);
esp_err_t menu_set_page(page_t page);