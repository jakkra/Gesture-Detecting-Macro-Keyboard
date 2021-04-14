#include "touchpad_sensor.h"
#include "Trill.h"
#include "driver/i2c.h"
#include <string.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))

// Input from Trill sensor is 1792 x 1792
#define MAX_TRILL_COORDINATE    1792
// That's a large matrix, to rounding it into a 28x28 matrix.
#define MAX_LOCAL_COORDINATE    28
#define DIVIDER_FACTOR          (MAX_TRILL_COORDINATE / MAX_LOCAL_COORDINATE) // 64

#define SDA_PIN GPIO_NUM_21
#define SCL_PIN GPIO_NUM_22

#define SAMPLE_DELAY 2

static float input[MAX_LOCAL_COORDINATE][MAX_LOCAL_COORDINATE];
static Trill trillSensor;

esp_err_t touchpad_sensor_init(void)
{
    int ret;
    i2c_config_t i2c_config;
    i2c_config.mode = I2C_MODE_MASTER;
    i2c_config.sda_io_num = SDA_PIN;
    i2c_config.sda_pullup_en = GPIO_PULLUP_ENABLE;
    i2c_config.scl_io_num = SCL_PIN;
    i2c_config.scl_pullup_en = GPIO_PULLUP_ENABLE;
    i2c_config.master.clk_speed = 100000;

    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &i2c_config));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0));

    ret = trillSensor.setup(Trill::TRILL_SQUARE, I2C_NUM_0);
    return ret == 0  ? ESP_OK : ESP_FAIL;
}

float* touchpad_sensor_fetch(void)
{
    bool touchActive = false;
    uint16_t input_x;
    uint16_t input_y;
    uint8_t x;
    uint8_t y;

    while (true) {
        trillSensor.read();
        if (trillSensor.getNumTouches() > 0 && trillSensor.getNumHorizontalTouches() > 0) {
            if (!touchActive) {
                memset(input, 0, sizeof(input));
            }
            input_x = MAX_TRILL_COORDINATE - trillSensor.touchLocation(0);
            input_y = trillSensor.touchHorizontalLocation(0);
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