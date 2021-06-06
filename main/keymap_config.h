#pragma once

#include "esp_err.h"
#include "hid_dev.h"
#include "gesture_model_tflite.h"
#include "keypress_input.h"

#define GESTURE_MAP_MAX_KEYS 6 // Comes from max in HID

esp_err_t keymap_config_gesture_get_keys(gesture_label_t gesture, key_mask_t* special_key_mask, uint8_t* keys, uint8_t* num_keys);
esp_err_t keymap_config_switch_get_keys(keypad_switch_t key, bool longpress, key_mask_t* special_key_mask, uint8_t* keys, uint8_t* num_keys);
