#include <string.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <driver/i2c.h>
#include "tf_gesture_predictor.h"
#include "touchpad_sensor.h"
#include "ble_hid.h"
#include "hid_dev.h"
#include "gesture_keymap.h"
#include "hid_dev.h"
#include "display.h"
#include "keypress_input.h"
#include "driver/rmt.h"
#include "led_strip.h"

#include "horizontal.h"
#include "vertical.h"
#include "v.h"

#define SDA_PIN GPIO_NUM_25
#define SCL_PIN GPIO_NUM_26
#define I2C_PORT I2C_NUM_0
#define RMT_TX_CHANNEL RMT_CHANNEL_0

static const char* TAG = "main";


//#define TRAINING
#ifdef TRAINING
static void runPrintTrainData(void);
#endif
static void sendKeysFromGesture(gesture_label_t prediction);
static void touch_bar_event_callback(touch_bar_state state, int16_t raw_value);
static void update_display(gesture_prediction_t prediction);
static void switch_pressed_callback(keypad_switch_t key);
static void init_led_strip(void);

static led_strip_t *strip;


void app_main(void) {
  esp_err_t ret;
  float* in_matrix;
  gesture_prediction_t prediction;

  ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  init_led_strip();
  vTaskDelay(pdMS_TO_TICKS(10000));
  keypress_input_init(switch_pressed_callback);

  i2c_config_t i2c_config;
  i2c_config.mode = I2C_MODE_MASTER;
  i2c_config.sda_io_num = SDA_PIN;
  i2c_config.sda_pullup_en = GPIO_PULLUP_ENABLE;
  i2c_config.scl_io_num = SCL_PIN;
  i2c_config.scl_pullup_en = GPIO_PULLUP_ENABLE;
  i2c_config.master.clk_speed = 100000;

  ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &i2c_config));
  ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0));

  ESP_ERROR_CHECK(touch_sensors_init(I2C_PORT, &touch_bar_event_callback));
  display_init(I2C_PORT, SDA_PIN, SCL_PIN);
  display_draw_text("Draw some gestures!", 1);
  display_draw_text("Next line test", 2);
  vTaskDelay(pdMS_TO_TICKS(1000));
  display_clear();
#ifdef TRAINING
  runPrintTrainData();
  // Never returns
  assert(false);
#endif
  ble_hid_init();

  ESP_ERROR_CHECK(tf_gesture_predictor_init());
  
  int64_t start = esp_timer_get_time();
  tf_gesture_predictor_run(vertical, sizeof(vertical), &prediction, false);
  printf("Prediction took %d\n", (int)(esp_timer_get_time() - start) / 1000);

  while (true) {
    in_matrix = touch_sensors_touchpad_fetch();
    if (in_matrix) {
      tf_gesture_predictor_run(in_matrix, 28 * 28 * sizeof(float), &prediction, false);

      if (prediction.probability > 0.95f) {
        sendKeysFromGesture(prediction.label);
      }
      update_display(prediction);
      printf("Prediction: %s, prob: %f\n", getNameOfPrediction(prediction.label),  prediction.probability);
    } else {
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
}

#ifdef TRAINING
static void runPrintTrainData(void) {
  while (true) {
    if (!touch_sensors_touchpad_print_raw()) {
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
}
#endif

static void sendKeysFromGesture(gesture_label_t prediction)
{
  esp_err_t err;
  key_mask_t key_mask;
  uint8_t num_keys;
  uint8_t keys[GESTURE_MAP_MAX_KEYS];

  err = gesture_keymap_get_keys(prediction, &key_mask, keys, &num_keys);
  if (err) {
    return;
  }
  if (ble_hid_request_access(50) == ESP_OK) {
    ble_hid_send_key(key_mask, keys, num_keys);
    
    vTaskDelay(pdMS_TO_TICKS(20));
    ble_hid_send_key(0, NULL, 0);
    ble_hid_give_access();
  }
}

static void update_display(gesture_prediction_t prediction)
{
  char text[100];
  memset(text, 0, sizeof(text));

  display_clear();
  snprintf(text, sizeof(text), "Probability: %f", prediction.probability);
  display_draw_text(text, 1);

  memset(text, 0, sizeof(text));
  snprintf(text, sizeof(text), "%s", getNameOfPrediction(prediction.label));
  display_draw_text(text, 2);
  vTaskDelay(pdMS_TO_TICKS(1000));
}

static void touch_bar_event_callback(touch_bar_state state, int16_t raw_value)
{
  consumer_cmd_t key = 0;

  switch (state) {
    case TOUCH_BAR_TOUCH_START:
      printf("TOUCH_BAR_TOUCH_START\n");
      break;
    case TOUCH_BAR_MOVING_UP:
      printf("TOUCH_BAR_MOVING_UP\n");
      key = HID_CONSUMER_VOLUME_UP;
      break;
    case TOUCH_BAR_MOVING_DOWN:
      printf("TOUCH_BAR_MOVING_DOWN\n");
      key = HID_CONSUMER_VOLUME_DOWN;
      break;
    case TOUCH_BAR_TOUCHED_IDLE:
      printf("TOUCH_BAR_TOUCHED_IDLE\n");
      break;
    case TOUCH_BAR_TOUCH_END:
      printf("TOUCH_BAR_END\n");
      break;
    default:
      break;
      ESP_LOGE(TAG, "Unknown touch bar state %d", state);
  }
  if (key != 0) {
    if (ble_hid_request_access(50) == ESP_OK) {
      ble_hid_send_consumer_key(key, true);
      ble_hid_send_consumer_key(key, false);
      ble_hid_give_access();
    }
  }
}

static void switch_pressed_callback(keypad_switch_t key)
{
  uint8_t buf;

  switch (key) {
    case KEYPAD_SWITCH_1:
      buf = HID_KEY_1;
      break;
    case KEYPAD_SWITCH_2:
      buf = HID_KEY_2;
      break;
    case KEYPAD_SWITCH_3:
      buf = HID_KEY_3;
      break;
    case KEYPAD_SWITCH_4:
      buf = HID_KEY_4;
      break;
    case KEYPAD_SWITCH_5:
      buf = HID_KEY_5;
      break;
    case KEYPAD_SWITCH_6:
      buf = HID_KEY_6;
      break;
    default:
      return;
  }

  if (ble_hid_request_access(250) == ESP_OK) {
    ble_hid_send_key(LEFT_CONTROL_KEY_MASK | LEFT_ALT_KEY_MASK, &buf, 1);
    vTaskDelay(pdMS_TO_TICKS(20));
    ble_hid_send_key(0, NULL, 0);
    ble_hid_give_access();
  }
}

void set_pixel(led_strip_t *strip, uint32_t index, uint32_t red, uint32_t green, uint32_t blue, uint32_t brightness)
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

static void init_led_strip(void)
{
  uint32_t r = 255, g = 255, b = 255, a = 255;
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

  ESP_ERROR_CHECK(strip->clear(strip, 100));
  for (uint16_t i = 0; i < CONFIG_STRIP_LED_NUMBER; i++) {
      set_pixel(strip, i, r, g, b, a);
  }
  ESP_ERROR_CHECK(strip->refresh(strip, 100));
  ESP_LOGI(TAG, "LED Strip ready");
}
