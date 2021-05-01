#include "keypress_input.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "ble_hid.h"

#define NUM_BUTTONS 6

#define KEY_1_PIN   23
#define KEY_2_PIN   32
#define KEY_3_PIN   33
#define KEY_4_PIN   36
#define KEY_5_PIN   39
#define KEY_6_PIN   34

#define DEBOUNCE_TIME_MS 50

typedef struct btn_state_t {
    uint32_t gpio_num;
    uint32_t last_press_ms;
    uint8_t key;
} btn_state_t;

static void configure_gpios(void);
static void button_task(void* arg);


static btn_state_t buttons[NUM_BUTTONS] = {
    { .gpio_num = KEY_1_PIN, .key = HID_KEY_1 },
    { .gpio_num = KEY_2_PIN, .key = HID_KEY_2 },
    { .gpio_num = KEY_3_PIN, .key = HID_KEY_3 },
    { .gpio_num = KEY_4_PIN, .key = HID_KEY_4 },
    { .gpio_num = KEY_5_PIN, .key = HID_KEY_5 },
    { .gpio_num = KEY_6_PIN, .key = HID_KEY_6 },
};

static xQueueHandle button_evt_queue = NULL;

void keypress_input_init(void) {
    configure_gpios();

    button_evt_queue = xQueueCreate(NUM_BUTTONS, sizeof(uint32_t));
    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);
}

static void button_task(void* arg)
{
    btn_state_t* button;
    uint32_t current_ms;

    for(;;) {
        if(xQueueReceive(button_evt_queue, &button, portMAX_DELAY)) {
            current_ms = xTaskGetTickCount();
            if (current_ms - button->last_press_ms > DEBOUNCE_TIME_MS) {
                button->last_press_ms = current_ms;
                printf("GPIO[%d] intr, PRESSED\n", button->gpio_num);
                if (ble_hid_request_access(500) == ESP_OK) {
                    ble_hid_send_key(0, &button->key, 1);
                    vTaskDelay(pdMS_TO_TICKS(20));
                    ble_hid_send_key(0, NULL, 0);
                    ble_hid_give_access();
                }
            }
        }
    }
}

static void IRAM_ATTR isr_button_pressed(void* arg)
{
    btn_state_t* button = (btn_state_t*)arg;
    xQueueSendFromISR(button_evt_queue, &button, NULL);
}

static void configure_gpios(void) {
    gpio_config_t io_conf;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = 1;
    io_conf.pull_up_en = 0;
    io_conf.intr_type = GPIO_INTR_POSEDGE;

    for (int i = 0; i < NUM_BUTTONS; i++) {
        io_conf.pin_bit_mask = 1ULL << (buttons[i].gpio_num);
        gpio_config(&io_conf);
    }

    gpio_install_isr_service(0);
    for (int i = 0; i < NUM_BUTTONS; i++) {
        gpio_isr_handler_add(buttons[i].gpio_num, isr_button_pressed, (void*)&buttons[i]);
    }
}