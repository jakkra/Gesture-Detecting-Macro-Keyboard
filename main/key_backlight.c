#include "key_backlight.h"
#include "led_strip.h"
#include "driver/rmt.h"
#include "sdkconfig.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define RMT_TX_CHANNEL RMT_CHANNEL_0
#define RAINBOW_SPEED_MS (25)


static const char* TAG = "key_backlight";

static void led_anim_thread(void* arg);
static void set_pixel(led_strip_t *strip, uint32_t index, uint32_t red, uint32_t green, uint32_t blue, uint32_t brightness);
void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b);

static led_strip_t *strip;
static key_backlight_mode_t led_mode;

void key_backlight_init(void)
{
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(CONFIG_RMT_TX_GPIO, RMT_TX_CHANNEL);
    // set counter clock to 40MHz
    config.clk_div = 2;

    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));

    // install ws2812 driver
    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(CONFIG_STRIP_LED_NUMBER, (led_strip_dev_t)config.channel);
    strip = led_strip_new_rmt_ws2812(&strip_config);
    if (!strip) {
        ESP_LOGE(TAG, "install WS2812 driver failed");
    }

    led_mode = KEY_BACKLIGHT_RAINBOW;

    ESP_ERROR_CHECK(strip->clear(strip, 100));

    ESP_LOGI(TAG, "LED Strip ready");

    xTaskCreate(led_anim_thread, "led_anim_thread", 2048, NULL, 10, NULL);
}

void key_backlight_set_mode(key_backlight_mode_t mode)
{
    led_mode = mode;
}

static void set_pixel(led_strip_t *strip, uint32_t index, uint32_t red, uint32_t green, uint32_t blue, uint32_t brightness)
{
    assert(strip != NULL);
    assert(index < CONFIG_STRIP_LED_NUMBER);
    assert(red <= 255);
    assert(green <= 255);
    assert(blue <= 255);
    assert(brightness <= 255);

    float bri_multiplier = (float)brightness / 255;
    ESP_ERROR_CHECK(strip->set_pixel(strip, index, (float)red * bri_multiplier, (float)green * bri_multiplier, (float)blue * bri_multiplier));
}

static void led_anim_thread(void* arg) {
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    uint16_t hue = 0;
    uint16_t start_rgb = 0;
    bool blink_on = false;

    while (true) {
        switch (led_mode) {
            case KEY_BACKLIGHT_RAINBOW:
            {
                for (int i = 0; i < CONFIG_STRIP_LED_NUMBER; i++) {
                    hue = (start_rgb + i * 50) % 360;
                    led_strip_hsv2rgb(hue, 100, 50, &red, &green, &blue);
                    ESP_ERROR_CHECK(strip->set_pixel(strip, i, red, green, blue));
                }
                ESP_ERROR_CHECK(strip->refresh(strip, 100));
                vTaskDelay(pdMS_TO_TICKS(RAINBOW_SPEED_MS));
                start_rgb += 2;
                break;
            }
            case KEY_BACKLIGHT_BLINKING:
            {
                for (int i = 0; i < CONFIG_STRIP_LED_NUMBER; i++) {
                    if (blink_on) {
                        ESP_ERROR_CHECK(strip->set_pixel(strip, i, 100, 100, 100));
                    } else {
                        ESP_ERROR_CHECK(strip->clear(strip, 100));
                    }
                    ESP_ERROR_CHECK(strip->refresh(strip, 100));
                }
                blink_on = !blink_on;
                vTaskDelay(pdMS_TO_TICKS(500));
                break;
            }
            case KEY_BACKLIGHT_OFF:
            {
                ESP_ERROR_CHECK(strip->clear(strip, 100));
                vTaskDelay(pdMS_TO_TICKS(1000));
                break;
            }
            default: 
                vTaskDelay(pdMS_TO_TICKS(1000));
                break;
        }
    }
}

/**
 * @brief Simple helper function, converting HSV color space to RGB color space
 *
 * Wiki: https://en.wikipedia.org/wiki/HSL_and_HSV
 *
 */
void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}
