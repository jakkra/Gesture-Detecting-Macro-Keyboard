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
#include "keymap_config.h"
#include "hid_dev.h"
#include "menu.h"
#include "keypress_input.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "crypto.h"
#include "key_backlight.h"

#include "horizontal.h"
#include "vertical.h"
#include "v.h"

#define SDA_PIN GPIO_NUM_25
#define SCL_PIN GPIO_NUM_26
#define I2C_PORT I2C_NUM_0

#define CENTER_GESTURE      1 // Need to match if the model is trained with centered data or not.
#define PRINT_GESTURE_DATA  0

static const char* TAG = "main";

static void sendKeysFromGesture(gesture_label_t prediction);
static void touch_bar_event_callback(touch_bar_state state, int16_t raw_value);
static void switch_pressed_callback(keypad_switch_t key, bool longpress);
static void init_wifi(void);
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void periodic_update_thread(void* arg);
static void ble_hid_connection_callback(ble_hid_connection event, esp_bd_addr_t* addr);
static void runPrintTrainData(void);


bool wifi_connected;
esp_ip4_addr_t ip_addr;
esp_bd_addr_t connected_ble_addr;

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
  key_backlight_init();
  keypress_input_init();

  i2c_config_t i2c_config;
  i2c_config.mode = I2C_MODE_MASTER;
  i2c_config.sda_io_num = SDA_PIN;
  i2c_config.sda_pullup_en = GPIO_PULLUP_ENABLE;
  i2c_config.scl_io_num = SCL_PIN;
  i2c_config.scl_pullup_en = GPIO_PULLUP_ENABLE;
  i2c_config.master.clk_speed = 100000;

  ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &i2c_config));
  ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0));

  ret = ESP_FAIL;
  for (int retries = 0; retries < 5 && ret != ESP_OK; retries++) {
    ret = touch_sensors_init(I2C_PORT, &touch_bar_event_callback);
  }

  // Holding SWITCH_6 down when booting => enter training mode.
  if (keypress_input_read(KEYPAD_SWITCH_6)) {
      printf("Entering training mode\n");
      runPrintTrainData();
      // Never returns
      assert(false);
  }

  menu_init(I2C_PORT, SDA_PIN, SCL_PIN);
  ble_hid_init(ble_hid_connection_callback);
  init_wifi();

  ESP_ERROR_CHECK(tf_gesture_predictor_init());
  
  xTaskCreate(periodic_update_thread, "periodic_update_thread", 4096, NULL, 10, NULL);
  int64_t start = esp_timer_get_time();
  tf_gesture_predictor_run(v_shape, sizeof(v_shape), &prediction, PRINT_GESTURE_DATA);
  printf("Prediction took %dms\n", (int)(esp_timer_get_time() - start) / 1000);

  keypress_input_set_callback(switch_pressed_callback);

  while (true) {
    in_matrix = touch_sensors_touchpad_fetch();
    if (in_matrix) {
      tf_gesture_predictor_run(in_matrix, 28 * 28 * sizeof(float), &prediction, PRINT_GESTURE_DATA);

      if (prediction.probability > 0.95f) {
        sendKeysFromGesture(prediction.label);
      }
      menu_draw_gestures(&prediction);
      printf("Prediction: %s, prob: %f\n", getNameOfPrediction(prediction.label),  prediction.probability);
    } else {
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
}

static void runPrintTrainData(void) {
  while (true) {
    if (!touch_sensors_touchpad_print_raw()) {
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
}

static void sendKeysFromGesture(gesture_label_t prediction)
{
  esp_err_t err;
  key_mask_t key_mask;
  uint8_t num_keys;
  uint8_t keys[GESTURE_MAP_MAX_KEYS];

  err = keymap_config_gesture_get_keys(prediction, &key_mask, keys, &num_keys);
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
    case TOUCH_BAR_TOUCHED_IDLE:
    case TOUCH_BAR_TOUCH_END:
      break;
    case TOUCH_BAR_MOVING_UP:
      key = HID_CONSUMER_VOLUME_UP;
      break;
    case TOUCH_BAR_MOVING_DOWN:
      key = HID_CONSUMER_VOLUME_DOWN;
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

static void switch_pressed_callback(keypad_switch_t key, bool longpress)
{
  esp_err_t err = ESP_FAIL;
  key_mask_t key_mask;
  uint8_t num_keys;
  uint8_t keys[GESTURE_MAP_MAX_KEYS];

  switch (key) {
    case KEYPAD_SWITCH_1:
    case KEYPAD_SWITCH_2:
    case KEYPAD_SWITCH_3:
    case KEYPAD_SWITCH_4:
    case KEYPAD_SWITCH_5:
      err = keymap_config_switch_get_keys(key, longpress, &key_mask, keys, &num_keys);
      break;
    case KEYPAD_SWITCH_6:
      if (!longpress) {
        menu_next_page();
      } else {
        ble_hid_set_pairable(true);
      }
      break;
    default:
      return;
  }

  if (err == ESP_OK) {
    if (ble_hid_request_access(250) == ESP_OK) {
      ble_hid_send_key(key_mask, keys, num_keys);
      vTaskDelay(pdMS_TO_TICKS(30));
      ble_hid_send_key(0, NULL, 0);
      ble_hid_give_access();
    }
  }
}

static void init_wifi(void)
{
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,  IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_ESP_WIFI_SSID,
            .password = CONFIG_ESP_WIFI_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "WiFi Started");
}

static void refresh_menu_connection_data(void) {
  char ip[50];
  char addr[50];
  memset(ip, 0, sizeof(ip));
  memset(addr, 0, sizeof(addr));
  snprintf(ip, sizeof(ip), IPSTR, IP2STR(&ip_addr));

  snprintf(addr, sizeof(addr), "%08x%04x",\
                (connected_ble_addr[0] << 24) + (connected_ble_addr[1] << 16) + (connected_ble_addr[2] << 8) + connected_ble_addr[3],
                (connected_ble_addr[4] << 8) + connected_ble_addr[5]);
  menu_draw_connection_status(ip, addr);
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Failed to connect WiFi");
        wifi_connected = false;
        memset(&ip_addr, 0, sizeof(ip_addr));
        refresh_menu_connection_data();
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        wifi_connected = true;
        ip_addr = event->ip_info.ip;
        refresh_menu_connection_data();
    }
}

static void periodic_update_thread(void* arg) {
  double btc, doge, btc_change, doge_change;
  for (;;) {
    crypto_get_price("bitcoin", &btc, &btc_change);
    crypto_get_price("dogecoin", &doge, &doge_change);
    menu_draw_crypto(btc, btc_change, doge, doge_change);
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

static void ble_hid_connection_callback(ble_hid_connection event, esp_bd_addr_t* addr) {
  switch (event) {
    case BLE_HID_CONNECTED:
      memcpy(connected_ble_addr, addr, sizeof(esp_bd_addr_t));
      refresh_menu_connection_data();
      break;
    case BLE_HID_DISCONNECTED:
      memset(connected_ble_addr, 0, sizeof(esp_bd_addr_t));
      refresh_menu_connection_data();
      break;
    default:
      break;
  }
}