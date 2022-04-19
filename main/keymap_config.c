#include "keymap_config.h"
#include <string.h>
#include "esp_err.h"
#include "keypress_input.h"

esp_err_t keymap_config_gesture_get_keys(gesture_label_t gesture, key_mask_t* special_key_mask, uint8_t* keys, uint8_t* num_keys) {
    *special_key_mask = LEFT_CONTROL_KEY_MASK | LEFT_ALT_KEY_MASK | LEFT_SHIFT_KEY_MASK;

    switch (gesture) {
        case LABEL_LINE_HORIZONTAL_GESTURE:
            keys[0] = HID_KEY_1;
            break;
        case LABEL_LINE_DOWN_GESTURE:
            keys[0] = HID_KEY_2;
            break;
        case LABEL_ARROW_DOWN:
            keys[0] = HID_KEY_3;
            break;
        case LABEL_C_GESTURE:
            keys[0] = HID_KEY_4;
            break;
        case LABEL_ARROW_RIGHT:
            keys[0] = HID_KEY_5;
            break;
        case LABEL_ARROW_UP:
            keys[0] = HID_KEY_6;
            break;
        case LABEL_S_GESTURE:
            keys[0] = HID_KEY_7;
            break;
        default:
            return ESP_FAIL;
    }
    *num_keys = 1;

    return ESP_OK;
}

esp_err_t keymap_config_switch_get_keys(keypad_switch_t key, bool longpress, key_mask_t* special_key_mask, uint8_t* keys, uint8_t* num_keys) {
    *special_key_mask = LEFT_CONTROL_KEY_MASK | LEFT_ALT_KEY_MASK;

    switch (key) {
        case KEYPAD_SWITCH_1:
            keys[0] = HID_KEY_A;
            break;
        case KEYPAD_SWITCH_2:
            keys[0] = HID_KEY_B;
            break;
        case KEYPAD_SWITCH_3:
            keys[0] = HID_KEY_C;
            break;
        case KEYPAD_SWITCH_4:
            keys[0] = HID_KEY_D;
            break;
        case KEYPAD_SWITCH_5:
            keys[0] = HID_KEY_E;
            break;
        case KEYPAD_SWITCH_6:
            keys[0] = HID_KEY_F;
            break;
        case KEYPAD_SWITCH_7:
            keys[0] = HID_KEY_G;
            break;
        case KEYPAD_SWITCH_8:
            keys[0] = HID_KEY_H;
            break;
        case KEYPAD_SWITCH_9:
            keys[0] = HID_KEY_I;
            break;
        case KEYPAD_SWITCH_10:
            keys[0] = HID_KEY_J;
            break;
        default:
            return ESP_FAIL;
    }

    if (longpress) {
        keys[0] += KEYPAD_SWITCH_LAST;
    }

    *num_keys = 1;

    return ESP_OK;
}