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

#include "horizontal.h"
#include "vertical.h"
#include "v.h"

#define SDA_PIN GPIO_NUM_21
#define SCL_PIN GPIO_NUM_22
#define I2C_PORT I2C_NUM_0

static const char* TAG = "main";

//#define TRAINING
#ifdef TRAINING
static void runPrintTrainData(void);
#endif
static void sendKeysFromGesture(gesture_label_t prediction);
static void touch_bar_event_callback(touch_bar_state state, int16_t raw_value);
static void update_display(gesture_prediction_t prediction);

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
