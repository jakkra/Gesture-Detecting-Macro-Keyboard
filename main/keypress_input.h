#pragma once

#include <stdbool.h>

typedef enum keypad_switch_t {
    KEYPAD_SWITCH_1,
    KEYPAD_SWITCH_2,
    KEYPAD_SWITCH_3,
    KEYPAD_SWITCH_4,
    KEYPAD_SWITCH_5,
    KEYPAD_SWITCH_6,
    KEYPAD_SWITCH_LAST = KEYPAD_SWITCH_6
} keypad_switch_t;

typedef void keypress_callback(keypad_switch_t key, bool longpress);

void keypress_input_init(keypress_callback* callback);