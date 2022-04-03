#include <string.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
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
#include "market_data.h"
#include "key_backlight.h"
#include "sdkconfig.h"

#include "v.h"

#define I2C_PORT I2C_NUM_0

#define PRINT_GESTURE_DATA    0
#define SWITCH_UPDATE_RATE_MS 20

static const char* TAG = "main";

static void sendKeysFromGesture(gesture_label_t prediction);
static void touch_bar_event_callback(touch_bar_state state, int16_t raw_value);
static void keys_scanned_callback(key_info_t* keys, int number_of_keys);
static void init_wifi(void);
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void periodic_update_thread(void* arg);
static void ble_hid_connection_callback(ble_hid_connection event, esp_bd_addr_t* addr);
static void runPrintTrainData(void);
static void disable_pairing_cb(TimerHandle_t xTimer);


static bool wifi_connected;
static esp_ip4_addr_t ip_addr;
static esp_bd_addr_t connected_ble_addr;
static  TimerHandle_t pairing_timer;

void app_main(void) {
  esp_err_t ret;
  float* in_matrix;
  gesture_prediction_t prediction;
  i2c_config_t i2c_config;

  ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  key_backlight_init();
  keypress_input_init(SWITCH_UPDATE_RATE_MS);

  i2c_config.mode = I2C_MODE_MASTER;
  i2c_config.sda_io_num = CONFIG_I2C_SDA_PIN;
  i2c_config.sda_pullup_en = GPIO_PULLUP_ENABLE;
  i2c_config.scl_io_num = CONFIG_I2C_SCL_PIN;
  i2c_config.scl_pullup_en = GPIO_PULLUP_ENABLE;
  i2c_config.master.clk_speed = 100000;

  ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &i2c_config));
  ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0));

  ret = ESP_FAIL;
  for (int retries = 0; retries < 5 && ret != ESP_OK; retries++) {
    ret = touch_sensors_init(I2C_PORT, &touch_bar_event_callback);
  }

  // Holding SWITCH_MODE down when booting => enter training mode.
  if (keypress_input_read(KEYPAD_SWITCH_MODE)) {
      printf("Entering training mode\n");
      runPrintTrainData();
      // Never returns
      assert(false);
  }

  menu_init(I2C_PORT, CONFIG_I2C_SDA_PIN, CONFIG_I2C_SCL_PIN, CONFIG_OLED_RST_PIN);

  ble_hid_init(ble_hid_connection_callback);
  pairing_timer = xTimerCreate("BLE Pair Timeout", pdMS_TO_TICKS(30000), pdFALSE, (void*)0, disable_pairing_cb);
  init_wifi();

  ESP_ERROR_CHECK(tf_gesture_predictor_init());
  
  xTaskCreate(periodic_update_thread, "periodic_update_thread", 5000, NULL, 10, NULL);
  int64_t start = esp_timer_get_time();
  tf_gesture_predictor_run(v_shape, sizeof(v_shape), &prediction, PRINT_GESTURE_DATA);
  printf("Prediction took %dms\n", (int)(esp_timer_get_time() - start) / 1000);

  keypress_input_set_callback(keys_scanned_callback);

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

static void disable_pairing_cb(TimerHandle_t xTimer)
{
  ble_hid_set_pairable(false);
  key_backlight_set_mode(KEY_BACKLIGHT_RAINBOW);
  menu_set_page(PAGE_GESTURE);
  ESP_LOGD(TAG, "Disabled pairing after timeout");
}

static char* keypad_switch_to_name(keypad_switch_t switch_num)
{
  switch (switch_num) {
    case KEYPAD_SWITCH_1:
      return "KEYPAD_SWITCH_1";
    case KEYPAD_SWITCH_2:
      return "KEYPAD_SWITCH_2";
    case KEYPAD_SWITCH_3:
      return "KEYPAD_SWITCH_3";
    case KEYPAD_SWITCH_4:
      return "KEYPAD_SWITCH_4";
    case KEYPAD_SWITCH_5:
      return "KEYPAD_SWITCH_5";
    case KEYPAD_SWITCH_6:
      return "KEYPAD_SWITCH_6";
    case KEYPAD_SWITCH_7:
      return "KEYPAD_SWITCH_7";
    case KEYPAD_SWITCH_8:
      return "KEYPAD_SWITCH_8";
    case KEYPAD_SWITCH_9:
      return "KEYPAD_SWITCH_9";
    case KEYPAD_SWITCH_10:
      return "KEYPAD_SWITCH_10";
    case KEYPAD_SWITCH_MODE:
      return "KEYPAD_SWITCH_MODE";
    default:
      return "INVALID_SWITCH";
  }
}

static void keys_scanned_callback(key_info_t* switches, int number_of_keys)
{
  esp_err_t err = ESP_FAIL;
  key_mask_t key_mask;
  bool longpress;
  uint8_t num_keys;
  uint8_t keys[GESTURE_MAP_MAX_KEYS];

  for (int i = 0; i < number_of_keys; i++) {
    if (((switches[i].state != KEYPAD_SWITCH_STATE_LONG_PRESSED) && (switches[i].state != KEYPAD_SWITCH_STATE_SHORT_PRESSED))) {
      continue;
    }

    longpress = switches[i].state == KEYPAD_SWITCH_STATE_LONG_PRESSED;

    printf("%s %d => longpress: %d\n", keypad_switch_to_name(switches[i].key), switches[i].state, longpress);

    switch (switches[i].key) {
      case KEYPAD_SWITCH_1:
      case KEYPAD_SWITCH_2:
      case KEYPAD_SWITCH_3:
      case KEYPAD_SWITCH_4:
      case KEYPAD_SWITCH_5:
      case KEYPAD_SWITCH_6:
      case KEYPAD_SWITCH_7:
      case KEYPAD_SWITCH_8:
      case KEYPAD_SWITCH_9:
      case KEYPAD_SWITCH_10:
        err = keymap_config_switch_get_keys(switches[i].key, longpress, &key_mask, keys, &num_keys);
        break;
      case KEYPAD_SWITCH_MODE:
        if (!longpress) {
          menu_next_page();
        } else {
          xTimerStop(pairing_timer, portMAX_DELAY);
          xTimerStart(pairing_timer, portMAX_DELAY);
          ble_hid_set_pairable(true);
          menu_set_page(PAGE_PAIRING);
          key_backlight_set_mode(KEY_BACKLIGHT_BLINKING);
          ESP_LOGI(TAG, "BLE pairing enabled for 30s");
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
  menu_draw_connection_status(CONFIG_ESP_WIFI_SSID, wifi_connected, ip, addr);
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
  double btc, doge, omx = 0, nasdaq, btc_change, doge_change, omx_change = 0, nasdaq_change;
  for (;;) {
    if (wifi_connected) {
      market_data_get_crypto("bitcoin", &btc, &btc_change);
      market_data_get_crypto("dogecoin", &doge, &doge_change);
      market_data_get_stock("^OMX", &omx, &omx_change);
      market_data_get_stock("^IXIC", &nasdaq, &nasdaq_change);
      menu_draw_market_data(btc, btc_change, doge, doge_change, omx, omx_change, nasdaq, nasdaq_change);
    }
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