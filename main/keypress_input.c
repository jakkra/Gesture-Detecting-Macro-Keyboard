#include "keypress_input.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "ble_hid.h"

#define NUM_KEY_COLS    2
#define NUM_KEY_ROWS    5


#define DEBOUNCE_TIME_MS 50
#define LONGPRESS_TIME_MS 2000

typedef struct btn_row_t {
    key_info_t buttons[NUM_KEY_COLS];
    uint32_t    gpio_num;
} btn_row_t;

static void button_task(void* arg);
static void read_all_switches(void);

static uint32_t columnPins[NUM_KEY_COLS] = {
    GPIO_NUM_25,
    GPIO_NUM_13,
};

static btn_row_t rows[NUM_KEY_ROWS] = {
    { .gpio_num = GPIO_NUM_2, .buttons = {{ .key = KEYPAD_SWITCH_1 }, { .key = KEYPAD_SWITCH_2 }} },
    { .gpio_num = GPIO_NUM_5, .buttons = {{ .key = KEYPAD_SWITCH_3 }, { .key = KEYPAD_SWITCH_4 }} },
    { .gpio_num = GPIO_NUM_17, .buttons = {{ .key = KEYPAD_SWITCH_5 }, { .key = KEYPAD_SWITCH_6 }} },
    { .gpio_num = GPIO_NUM_18, .buttons = {{ .key = KEYPAD_SWITCH_7 }, { .key = KEYPAD_SWITCH_8 }} },
    { .gpio_num = GPIO_NUM_23, .buttons = {{ .key = KEYPAD_SWITCH_9 }, { .key = KEYPAD_SWITCH_10 }} },
};

static btn_row_t mode_button = { .gpio_num = GPIO_NUM_27, .buttons = {{ .key = KEYPAD_SWITCH_MODE }} };

static key_scan_callback* keys_scanned_callback = NULL;
static int scan_interval_ms;

void keypress_input_init(int key_scan_interval_ms) {
    gpio_config_t io_conf_col;
    gpio_config_t io_conf_row;

    scan_interval_ms = key_scan_interval_ms;

    io_conf_col.mode = GPIO_MODE_OUTPUT;
    io_conf_col.pull_down_en = 1;
    io_conf_col.pull_up_en = 0;

    io_conf_row.mode = GPIO_MODE_INPUT;
    io_conf_row.pull_down_en = 1;
    io_conf_row.pull_up_en = 0;

    io_conf_row.pin_bit_mask = 1ULL << (mode_button.gpio_num);
    gpio_config(&io_conf_row);

    for (int i = 0; i < NUM_KEY_COLS; i++) {
        io_conf_col.pin_bit_mask = 1ULL << (columnPins[i]);
        gpio_config(&io_conf_col);
    }
    for (int i = 0; i < NUM_KEY_ROWS; i++) {
        io_conf_row.pin_bit_mask = 1ULL << (rows[i].gpio_num);
        gpio_config(&io_conf_row);
    }
}

void keypress_input_set_callback(key_scan_callback* callback) {
    keys_scanned_callback = callback;

    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);
}

int keypress_input_read(keypad_switch_t switch_num) {
    return gpio_get_level(mode_button.gpio_num);
    return 0;
}

static char* keypad_mode_to_name(keypad_switch_t switch_num)
{
    switch (switch_num) {
        case KEYPAD_SWITCH_STATE_RELEASED:
            return "KEYPAD_SWITCH_STATE_RELEASED";
        case KEYPAD_SWITCH_STATE_PRESSED:
            return "KEYPAD_SWITCH_STATE_PRESSED";
        case KEYPAD_SWITCH_STATE_SHORT_PRESSED:
            return "KEYPAD_SWITCH_STATE_SHORT_PRESSED";
        case KEYPAD_SWITCH_STATE_LONG_PRESSED:
            return "KEYPAD_SWITCH_STATE_LONG_PRESSED";
        default:
            return "INVALID_STATE";
    }
}

static void update_switch_state(key_info_t* key, bool pressed)
{
    uint32_t current_time = esp_timer_get_time() / 1000;

    switch (key->state) {
    case KEYPAD_SWITCH_STATE_RELEASED:
        if (pressed) {
            key->state = KEYPAD_SWITCH_STATE_PRESSED;
            key->last_press_ms = current_time;
        }
        break;
    case KEYPAD_SWITCH_STATE_PRESSED:
        if (!pressed) {
            if ((current_time - key->last_press_ms) < LONGPRESS_TIME_MS) {
                key->state = KEYPAD_SWITCH_STATE_SHORT_PRESSED;
            } else {
                key->state = KEYPAD_SWITCH_STATE_LONG_PRESSED;
            }
        }
        break;
    case KEYPAD_SWITCH_STATE_SHORT_PRESSED:
    case KEYPAD_SWITCH_STATE_LONG_PRESSED:
        if (!pressed) {
            key->state = KEYPAD_SWITCH_STATE_RELEASED;
            key->last_press_ms = 0;
        } else {
            key->state = KEYPAD_SWITCH_STATE_PRESSED; // We missed reading the release => go to pressed state
            key->last_press_ms = current_time;
        }
        break;
    default:
        assert(0);
        break;
    }
}

static void button_task(void* arg)
{
    key_info_t keys[KEYPAD_SWITCH_END];
    int index;

    for (;;) {
        index = 0;
        read_all_switches();
        for (int row = 0; row < NUM_KEY_ROWS; row++) {
            for (int col = 0; col < NUM_KEY_COLS; col++) {
                keys[index] = rows[row].buttons[col];
                index++;
            }
        }
        keys[index] = mode_button.buttons[0];
        keys_scanned_callback(keys, index + 1);
        vTaskDelay(pdMS_TO_TICKS(scan_interval_ms));
    }
}

static void reset_all_col(void) {
    for (int i = 0; i < NUM_KEY_COLS; i++) {
        gpio_set_level(columnPins[i], 0);
    }
}

static void read_all_switches(void) {
    int val;

    for (int i = 0; i < NUM_KEY_COLS; i++) {
        reset_all_col();
        gpio_set_level(columnPins[i], 1);
        for (int j = 0; j < NUM_KEY_ROWS; j++) {
            val = gpio_get_level(rows[j].gpio_num);
            update_switch_state(&rows[j].buttons[i], val);
        }
    }

    reset_all_col();
    update_switch_state(&mode_button.buttons[0], gpio_get_level(mode_button.gpio_num));
}
