#pragma once

#include "esp_err.h"
#include "hid_dev.h"
#include "gesture_model_tflite.h"

#define GESTURE_MAP_MAX_KEYS 6 // Comes from max in HID

esp_err_t gesture_keymap_get_keys(gesture_label_t gesture, key_mask_t* special_key_mask, uint8_t* keys, uint8_t* num_keys);
