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
#include "menu.h"
#include "keypress_input.h"
#include "driver/rmt.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "led_strip.h"
#include "crypto.h"

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
static void switch_pressed_callback(keypad_switch_t key);
static void init_led_strip(void);
static void init_wifi(void);
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void periodic_update_thread(void* arg);


static led_strip_t *strip;
bool wifi_connected;

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
  //touch_sensors_init(I2C_PORT, &touch_bar_event_callback);
  menu_init(I2C_PORT, SDA_PIN, SCL_PIN);
#ifdef TRAINING
  runPrintTrainData();
  // Never returns
  assert(false);
#endif
  ble_hid_init();
  init_wifi();

  ESP_ERROR_CHECK(tf_gesture_predictor_init());
  
  xTaskCreate(periodic_update_thread, "periodic_update_thread", 4096, NULL, 10, NULL);

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
      menu_draw_gestures(&prediction);
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
      menu_next_page();
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

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Failed to connect WiFi");
        wifi_connected = false;
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        wifi_connected = true;
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

static void periodic_update_thread(void* arg) {
  double btc, doge, btc_change, doge_change;
  for (;;) {
    crypto_get_price("bitcoin", &btc, &btc_change);
    crypto_get_price("dogecoin", &doge, &doge_change);
    printf("Bitcoin price: %f, change %f, doge %f, change %f\n", btc, doge, btc_change, doge_change);
    menu_draw_crypto(btc, btc_change, doge, doge_change);
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}