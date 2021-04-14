#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

esp_err_t touchpad_sensor_init(void);
float* touchpad_sensor_fetch(void);
bool touchpad_sensor_print_raw(void);

#ifdef __cplusplus
}
#endif
