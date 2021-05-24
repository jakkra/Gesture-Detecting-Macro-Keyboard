#include "touchpad_sensor.h"
#include "Trill.h"
#include "driver/i2c.h"
#include <string.h>
#include "esp_log.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))

// Input from Trill sensor is 1792 x 1792
#define MAX_TRILL_COORDINATE    1792
// That's a large matrix, to rounding it into a 28x28 matrix.
#define MAX_LOCAL_COORDINATE    28
#define DIVIDER_FACTOR          (MAX_TRILL_COORDINATE / MAX_LOCAL_COORDINATE) // 64

#define SDA_PIN GPIO_NUM_21
#define SCL_PIN GPIO_NUM_22

#define SAMPLE_DELAY 2
#define TRILL_BAR_READ_INTERVAL_MS 50
#define TRILL_BAR_MIN_INPUT_VAL 1408
#define TRILL_BAR_MIN_CHANGE 25

static const char* TAG = "touchpad_sensor";

static void touch_bar_task(void* params);

static float input[MAX_LOCAL_COORDINATE][MAX_LOCAL_COORDINATE];
static Trill trillSquare;
static Trill trillBar;
static touch_bar_callback* bar_event_callback;
static i2c_port_t i2c_port;

esp_err_t touch_sensors_init(touch_bar_callback* touch_bar_callback);
float* touch_sensors_touchpad_fetch(void);

esp_err_t touch_sensors_init(i2c_port_t port, touch_bar_callback* touch_bar_callback)
{
    int err;
    esp_err_t ret = ESP_OK;
    i2c_port = port;

    err = trillSquare.setup(Trill::TRILL_SQUARE, i2c_port);
    if (err) {
        ret = ESP_FAIL;
        ESP_LOGE(TAG, "Failed to init TRILL_SQUARE: %d", err);
    }
    err = trillBar.setup(Trill::TRILL_BAR, i2c_port);
    if (!err) {
        TaskHandle_t touch_task_handle;
        BaseType_t status = xTaskCreate(touch_bar_task, "touchbar_task", 2048, NULL, tskIDLE_PRIORITY, &touch_task_handle);
        assert(status == pdPASS);
    } else {
        ret = ESP_FAIL;
        ESP_LOGE(TAG, "Failed to init TRILL_BAR: %d", err);
    }
    bar_event_callback = touch_bar_callback;

    return ret == 0  ? ESP_OK : ESP_FAIL;
}

float* touch_sensors_touchpad_fetch(void)
{
    bool touchActive = false;
    uint16_t input_x;
    uint16_t input_y;
    uint8_t x;
    uint8_t y;

    while (true) {
        trillSquare.read();
        if (trillSquare.getNumTouches() > 0 && trillSquare.getNumHorizontalTouches() > 0) {
            if (!touchActive) {
                memset(input, 0, sizeof(input));
            }
            input_x = MAX_TRILL_COORDINATE - trillSquare.touchLocation(0);
            input_y = trillSquare.touchHorizontalLocation(0);
            x = MAX((input_x / DIVIDER_FACTOR) - 1, 0);
            y = MAX((input_y / DIVIDER_FACTOR) - 1, 0);
            input[x][y] = 1;

            touchActive = true;
        } else if(touchActive) {
            touchActive = false;
            return (float*)input;
        } else {
            return NULL;
        }
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_DELAY));
    }

    return NULL;
}



bool touch_sensors_touchpad_print_raw(void)
{
    bool touchActive = false;

    while (true) {
        trillSquare.read();
        if (trillSquare.getNumTouches() > 0 && trillSquare.getNumHorizontalTouches() > 0) {
            if (!touchActive) {
                printf("START:");
            }
            printf("%d", trillSquare.touchHorizontalLocation(0));
            printf(",");

            printf("%d", MAX_TRILL_COORDINATE - trillSquare.touchLocation(0));
            printf(",");
            touchActive = true;
        } else if (touchActive) {
            // Print a single line when touch goes off
            touchActive = false;
            printf("\n");
            return true;
        } else {
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_DELAY));
    }

    return false;
}

static void touch_bar_task(void* params)
{
    bool touchActive = false;
    int16_t input_val;
    int16_t prev_val;

    while (true) {
        trillBar.read();
        if (trillBar.getNumTouches() == 1) {
            input_val = MAX_TRILL_COORDINATE - trillBar.touchLocation(0);
            input_val = input_val + TRILL_BAR_MIN_INPUT_VAL; // Make input between 0 and max_input + min_input;
            if (!touchActive) {
                // Since sometimes if the Trill bar is dirty or similar it outputs random touches
                // This is to verify that there still is a touch after TRILL_BAR_READ_INTERVAL_MS
                bool check = true;
                int16_t prev_check_val = input_val;
                for (int i = 0; i < 10; i++) {
                    vTaskDelay(pdMS_TO_TICKS(TRILL_BAR_READ_INTERVAL_MS / 10));
                    trillBar.read();
                    if (trillBar.getNumTouches() == 0 || trillBar.touchLocation(0) == input_val) {
                        check = false;
                        break;
                    }
                }
                if (check) {
                    touchActive = true;
                    prev_val = input_val;
                }
                    bar_event_callback(TOUCH_BAR_TOUCH_START, input_val);
            } else {
                if (abs(input_val - prev_val) > TRILL_BAR_MIN_CHANGE) {
                    if (input_val > prev_val) {
                        bar_event_callback(TOUCH_BAR_MOVING_UP, input_val);
                    } else {
                        bar_event_callback(TOUCH_BAR_MOVING_DOWN, input_val);
                    }
                } else {
                    bar_event_callback(TOUCH_BAR_TOUCHED_IDLE, input_val);
                }
            }

            prev_val = input_val;
        } else if (touchActive) {
            touchActive = false;
            bar_event_callback(TOUCH_BAR_TOUCH_END, input_val);
        }
        vTaskDelay(pdMS_TO_TICKS(TRILL_BAR_READ_INTERVAL_MS));
    }

}
