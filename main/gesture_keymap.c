#include "gesture_keymap.h"
#include <string.h>
#include "esp_err.h"

esp_err_t gesture_keymap_get_keys(gesture_label_t gesture, key_mask_t* special_key_mask, uint8_t* keys, uint8_t* num_keys) {
    *special_key_mask = LEFT_CONTROL_KEY_MASK | LEFT_ALT_KEY_MASK;

    // This should match hotkey_handler.ahl
    switch (gesture) {
        case LABEL_LINE_DOWN_GESTURE:
            keys[0] = HID_KEY_K;
            break;
        case LABEL_LINE_HORIZONTAL_GESTURE:
            keys[0] = HID_KEY_L;
            break;
        case LABEL_V_GESTURE:
            keys[0] = HID_KEY_M;
            break;
        case LABEL_ARROW_UP:
            keys[0] = HID_KEY_P;
            break;
        case LABEL_C_GESTURE:
            keys[0] = HID_KEY_N;
            break;
        case LABEL_ARROW_RIGHT:
            keys[0] = HID_KEY_O;
            break;
        case LABEL_S_GESTURE:
            keys[0] = HID_KEY_Q;
            break;
        default:
            return ESP_FAIL;
    }
    *num_keys = 1;
    
    return ESP_OK;
}
