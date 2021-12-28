#include "display.h"
#include <esp_err.h>
#include <esp_log.h>
#include <u8g2.h>

#include "driver/gpio.h"
#include "driver/i2c.h"

#define ACK_CHECK_EN   0x1
#define ACK_CHECK_DIS  0x0

static const char* TAG = "display";

static const unsigned int I2C_TIMEOUT_MS = 1000;

typedef struct display_config_t {
    i2c_port_t port;
    gpio_num_t sda;
    gpio_num_t scl;
    gpio_num_t rst;
} display_config_t;

static uint8_t u8g2_i2c_byte_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
static uint8_t u8g2_gpio_and_delay_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

static i2c_cmd_handle_t handle_i2c;
static display_config_t display_config;
static u8g2_t u8g2;


esp_err_t display_init(i2c_port_t port, gpio_num_t  sda, gpio_num_t scl, gpio_num_t rst) {
    display_config.port = port;
    display_config.sda = sda;
    display_config.scl = scl;
    display_config.rst = rst;

    ESP_LOGD(TAG, "Resetting OLED Display\n");
    gpio_config_t io_conf;
    io_conf.pin_bit_mask = 1ULL << (GPIO_NUM_16);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = 1;
    io_conf.pull_up_en = 0;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

#ifdef CONFIG_KEYBOARD_V2
    u8g2_Setup_ssd1306_i2c_128x64_noname_f
#else
    u8g2_Setup_ssd1306_i2c_128x32_univision_f
#endif
        (&u8g2,
        U8G2_R0,
        u8g2_i2c_byte_cb,
        u8g2_gpio_and_delay_cb);

    u8g2_SetI2CAddress(&u8g2, 0x78); // 0x3C << 1
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFlipMode(&u8g2, 1);

    return ESP_OK;
}

esp_err_t display_draw_text(char* line1, char* line2)
{
    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
    int8_t max_char_height = u8g2_GetMaxCharHeight(&u8g2);
    u8g2_DrawStr(&u8g2, 1, 1 * max_char_height, line1);
    u8g2_DrawStr(&u8g2, 1, 2 * max_char_height, line2);
    u8g2_SendBuffer(&u8g2);

    return ESP_OK;
}

esp_err_t display_clear(void)
{
    u8g2_ClearBuffer(&u8g2);
    u8g2_SendBuffer(&u8g2);

    return ESP_OK;
}

static uint8_t u8g2_i2c_byte_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    ESP_LOGD(TAG, "i2c_cb: Received a msg: %d, arg_int: %d, arg_ptr: %p", msg, arg_int, arg_ptr);

    switch(msg) {
        case U8X8_MSG_BYTE_SET_DC:
        case U8X8_MSG_BYTE_INIT:
            break;
        case U8X8_MSG_BYTE_SEND:
        {
            uint8_t* data_ptr = (uint8_t*)arg_ptr;
            ESP_LOG_BUFFER_HEXDUMP(TAG, data_ptr, arg_int, ESP_LOG_VERBOSE);

            while(arg_int > 0) {
               ESP_ERROR_CHECK(i2c_master_write_byte(handle_i2c, *data_ptr, ACK_CHECK_EN));
               data_ptr++;
               arg_int--;
            }
            break;
        }
        case U8X8_MSG_BYTE_START_TRANSFER:
        {
            uint8_t i2c_address = u8x8_GetI2CAddress(u8x8);
            handle_i2c = i2c_cmd_link_create();
            ESP_LOGD(TAG, "Start I2C transfer to %02X.", i2c_address>>1);
            ESP_ERROR_CHECK(i2c_master_start(handle_i2c));
            ESP_ERROR_CHECK(i2c_master_write_byte(handle_i2c, i2c_address | I2C_MASTER_WRITE, ACK_CHECK_EN));
            break;
        }
        case U8X8_MSG_BYTE_END_TRANSFER:
        {
            ESP_LOGD(TAG, "End I2C transfer.");
            ESP_ERROR_CHECK(i2c_master_stop(handle_i2c));
            ESP_ERROR_CHECK(i2c_master_cmd_begin(display_config.port, handle_i2c, I2C_TIMEOUT_MS / portTICK_RATE_MS));
            i2c_cmd_link_delete(handle_i2c);
            break;
        }
    }
    return 0;
}

static uint8_t u8g2_gpio_and_delay_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    ESP_LOGD(TAG, "gpio_and_delay_cb: Received a msg: %d, arg_int: %d, arg_ptr: %p", msg, arg_int, arg_ptr);

    switch(msg) {
        case U8X8_MSG_GPIO_RESET:
            gpio_set_level(display_config.rst, arg_int);
            break;
        case U8X8_MSG_GPIO_AND_DELAY_INIT:
        case U8X8_MSG_GPIO_CS:
            break;
        case U8X8_MSG_GPIO_I2C_CLOCK:
            gpio_set_level(display_config.scl, arg_int);
            break;
        case U8X8_MSG_GPIO_I2C_DATA:
            gpio_set_level(display_config.sda, arg_int);
            break;
        case U8X8_MSG_DELAY_MILLI:
            vTaskDelay(pdMS_TO_TICKS(arg_int));
            break;
    }
    return 0;
}