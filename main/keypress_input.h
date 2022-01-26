#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum keypad_switch_t {
    KEYPAD_SWITCH_1,
    KEYPAD_SWITCH_2,
    KEYPAD_SWITCH_3,
    KEYPAD_SWITCH_4,
    KEYPAD_SWITCH_5,
    KEYPAD_SWITCH_6,
    KEYPAD_SWITCH_7,
    KEYPAD_SWITCH_8,
    KEYPAD_SWITCH_9,
    KEYPAD_SWITCH_10,
    KEYPAD_SWITCH_MODE,
    KEYPAD_SWITCH_LAST = KEYPAD_SWITCH_MODE,
    KEYPAD_SWITCH_END,
} keypad_switch_t;

typedef enum keypad_switch_state_t {
    KEYPAD_SWITCH_STATE_RELEASED,
    KEYPAD_SWITCH_STATE_PRESSED,
    KEYPAD_SWITCH_STATE_SHORT_PRESSED,
    KEYPAD_SWITCH_STATE_LONG_PRESSED,
} keypad_switch_state_t;

typedef struct key_info_t {
    uint32_t                last_press_ms;
    keypad_switch_t         key;
    keypad_switch_state_t   state;
} key_info_t;

typedef void key_scan_callback(key_info_t* keys, int num_keys);

void keypress_input_init(int key_scan_interval_ms);
void keypress_input_set_callback(key_scan_callback* callback);
int keypress_input_read(keypad_switch_t switch_num);