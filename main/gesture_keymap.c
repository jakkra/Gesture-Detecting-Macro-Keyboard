#include "gesture_keymap.h"
#include <string.h>
#include "esp_err.h"

esp_err_t gesture_keymap_get_keys(gesture_label_t gesture, key_mask_t* special_key_mask, uint8_t* keys, uint8_t* num_keys) {
    *special_key_mask = LEFT_CONTROL_KEY_MASK;

    switch (gesture) {
        case LABEL_LINE_DOWN_GESTURE:
            keys[0] = HID_KEY_F1;
            keys[1] = HID_KEY_LEFT_ALT;
            break;
        case LABEL_LINE_HORIZONTAL_GESTURE:
            keys[0] = HID_KEY_F2;
            keys[1] = HID_KEY_LEFT_ALT;
            break;
        case LABEL_O_GESTURE:
            keys[0] = HID_KEY_F3;
            keys[1] = HID_KEY_LEFT_ALT;
            break;
        case LABEL_V_GESTURE:
            keys[0] = HID_KEY_F4;
            keys[1] = HID_KEY_LEFT_ALT;
            break;
        default:
            return ESP_FAIL;
    }
    *num_keys = 2;
    
    return ESP_OK;
}
