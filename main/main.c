#include <string.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "tf_gesture_predictor.h"
#include "touchpad_sensor.h"
#include "ble_hid.h"
#include "hid_dev.h"
#include "gesture_keymap.h"
#include "hid_dev.h"

#include "horizontal.h"
#include "vertical.h"
#include "v.h"


static const char* TAG = "main";

//#define TRAINING
#ifdef TRAINING
static void runPrintTrainData(void);
#endif
static void sendKeysFromGesture(gesture_label_t prediction);
static void touch_bar_event_callback(touch_bar_state state, int16_t raw_value);

void app_main(void) {
  esp_err_t ret;

  ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  gesture_prediction_t prediction;
  float* in_matrix;

  //ESP_ERROR_CHECK(touchpad_sensor_init());
  touch_sensors_init(&touch_bar_event_callback);

#ifdef TRAINING
  runPrintTrainData();
  // Never returns
  assert(false);
#endif
  ble_hid_init();

  ESP_ERROR_CHECK(tf_gesture_predictor_init());
  
  int64_t start = esp_timer_get_time();
  tf_gesture_predictor_run(vertical, sizeof(vertical), &prediction, true);
  printf("Prediction took %d\n", (int)(esp_timer_get_time() - start) / 1000);

  while (true) {
    in_matrix = touch_sensors_touchpad_fetch();
    if (in_matrix) {
      tf_gesture_predictor_run(in_matrix, 28 * 28 * sizeof(float), &prediction, true);

      if (prediction.probability > 0.95f) {
        sendKeysFromGesture(prediction.label);
      }
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
