#include "gesture_keymap.h"
#include <string.h>
#include "esp_err.h"

esp_err_t gesture_keymap_get_keys(gesture_label_t gesture, key_mask_t* special_key_mask, uint8_t* keys, uint8_t* num_keys) {
    *special_key_mask = LEFT_SHIFT_KEY_MASK;

    switch (gesture) {
        case LABEL_LINE_DOWN_GESTURE:
            keys[0] = HID_KEY_A;
            keys[1] = HID_KEY_A;
            keys[2] = HID_KEY_RETURN;
            break;
        case LABEL_LINE_HORIZONTAL_GESTURE:
            keys[0] = HID_KEY_A;
            keys[1] = HID_KEY_B;
            keys[2] = HID_KEY_RETURN;
            break;
        case LABEL_O_GESTURE:
            keys[0] = HID_KEY_A;
            keys[1] = HID_KEY_C;
            keys[2] = HID_KEY_RETURN;
            break;
        case LABEL_V_GESTURE:
            keys[0] = HID_KEY_A;
            keys[1] = HID_KEY_D;
            keys[2] = HID_KEY_RETURN;
            break;
        default:
            return ESP_FAIL;
    }
    *num_keys = 3;
    
    return ESP_OK;
}
