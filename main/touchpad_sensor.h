#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <esp_err.h>
#include <driver/i2c.h>

typedef enum touch_bar_state {
    TOUCH_BAR_TOUCH_START,
    TOUCH_BAR_MOVING_UP,
    TOUCH_BAR_MOVING_DOWN,
    TOUCH_BAR_TOUCHED_IDLE,
    TOUCH_BAR_TOUCH_END
} touch_bar_state;

typedef void touch_bar_callback(touch_bar_state state, int16_t raw_value);

esp_err_t touch_sensors_init(i2c_port_t port, touch_bar_callback* touch_bar_callback);
float* touch_sensors_touchpad_fetch(void);
bool touch_sensors_touchpad_print_raw(void);

#ifdef __cplusplus
}
#endif
