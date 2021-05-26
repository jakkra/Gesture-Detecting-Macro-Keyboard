#include "menu.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "esp_log.h"
#include "tf_gesture_predictor.h"

#define IP_STR_MAX_LEN  15
#define MAC_STR_MAX_LEN  6 * 2
#define WORK_QUEUE_SIZE 5

static const char* TAG = "menu";

typedef enum page_t {
    PAGE_CRYPTO,
    PAGE_GESTURE,
    PAGE_CONNECTION,
    PAGE_END
} page_t;

typedef enum work_t {
    WORK_REDRAW
} work_t;

typedef struct menu_data_t {
    struct gesture {
        gesture_prediction_t prediction;
    } gesture;
    struct crypto {
        double bitcoinPrice;
        double dogePrice;
        double bitcoinChange24h;
        double dogeChange24h;
    } crypto;
    struct connection {
        bool wifi_connected;
        bool ble_connected;
        char ip[IP_STR_MAX_LEN + 1];
        char addr[MAC_STR_MAX_LEN + 1];
    } connection;
    page_t current_page;
} menu_data_t;

static void render_current_page(void);
static void work(void* arg);

static menu_data_t data;
static xQueueHandle work_queue = NULL;

static bool initialized = false;

esp_err_t menu_init(i2c_port_t port, gpio_num_t  sda, gpio_num_t scl){
    esp_err_t ret = ESP_OK;
    memset(&data, 0, sizeof(data));

    ret = display_init(port, sda, scl);
    if (ret == ESP_OK) {
        work_queue = xQueueCreate(WORK_QUEUE_SIZE, sizeof(work_t));
        assert(work_queue != NULL);
        xTaskCreate(work, "menu_work", 4096, NULL, 10, NULL);
        work_t* task = malloc(sizeof(work_t*));
        xQueueSend(work_queue, (void*)&task, (TickType_t)0);
        initialized = true;
    } else {
        ESP_LOGE(TAG, "Failed init display");
    }

    return ret;
}

esp_err_t menu_draw_crypto(double bitcoinPrice, double bitcoinChange24h, double dogePrice, double dogeChange24h) {
    esp_err_t ret = ESP_OK;
    assert(initialized);
    data.crypto.bitcoinPrice = bitcoinPrice;
    data.crypto.bitcoinChange24h = bitcoinChange24h;
    data.crypto.dogePrice = dogePrice;
    data.crypto.dogeChange24h = dogeChange24h;
    if (data.current_page == PAGE_CRYPTO) {
        work_t* task = malloc(sizeof(work_t));
        *task = WORK_REDRAW;
        xQueueSend(work_queue, (void*)&task, (TickType_t)0);
    }
    return ret;
}

esp_err_t menu_draw_gestures(gesture_prediction_t* prediction) {
    esp_err_t ret = ESP_OK;
    assert(initialized);
    memcpy(&data.gesture.prediction, prediction, sizeof(gesture_prediction_t));
    if (data.current_page == PAGE_GESTURE) {
        work_t* task = malloc(sizeof(work_t));
        *task = WORK_REDRAW;
        xQueueSend(work_queue, (void*)&task, (TickType_t)0);
    }
    return ret;
}

esp_err_t menu_draw_connection_status(char* wifi_ip, char* ble_addr) {
    esp_err_t ret = ESP_OK;
    assert(initialized);
    strncpy(data.connection.ip, wifi_ip, IP_STR_MAX_LEN);
    strncpy(data.connection.addr, ble_addr, MAC_STR_MAX_LEN);
    if (data.current_page == PAGE_CONNECTION) {
        work_t* task = malloc(sizeof(work_t));
        *task = WORK_REDRAW;
        xQueueSend(work_queue, (void*)&task, (TickType_t)0);
    }
    return ret;
}

esp_err_t menu_next_page(void){
    esp_err_t ret = ESP_OK;
    assert(initialized);
    data.current_page = (page_t)((data.current_page + 1) % PAGE_END);
    work_t* task = malloc(sizeof(work_t));
    *task = WORK_REDRAW;
    xQueueSend(work_queue, (void*)&task, (TickType_t)0);
    return ret;
}

static void work(void* arg)
{
    work_t* work_type;

    for (;;) {
        if (xQueueReceive(work_queue, &work_type, portMAX_DELAY)) {
            free(work_type);
            render_current_page();
        }
    }
}

static void render_crypto(void) {
    char line1[100];
    char line2[100];
    memset(line1, 0, sizeof(line1));
    memset(line2, 0, sizeof(line2));

    display_clear();
    snprintf(line1, sizeof(line1), "BTC: %.0f (%.2f%%)", data.crypto.bitcoinPrice, data.crypto.bitcoinChange24h);

    snprintf(line2, sizeof(line2), "DOGE: %.2f (%.2f%%)", data.crypto.dogePrice, data.crypto.dogeChange24h);
    display_draw_text(line1, line2);
}

static void render_gesture(void) {
    char line1[100];
    char line2[100];
    memset(line1, 0, sizeof(line1));
    memset(line2, 0, sizeof(line2));

    display_clear();
    snprintf(line1, sizeof(line1), "Probability: %f", data.gesture.prediction.probability);
    snprintf(line2, sizeof(line2), "%s", tf_gesture_predictor_get_name(data.gesture.prediction.label));
    display_draw_text(line1, line2);
}

static void render_connection(void) {
    char line1[100];
    char line2[100];
    memset(line1, 0, sizeof(line1));
    memset(line2, 0, sizeof(line2));

    snprintf(line1, sizeof(line1), "IP: %s", data.connection.ip);
    snprintf(line2, sizeof(line2), "BLE: %s", data.connection.addr);
    display_clear();
    display_draw_text(line1, line2);
}

static void render_current_page(void) {
    switch (data.current_page) {
        case PAGE_CRYPTO:
        {
            render_crypto();
            break;
        }
        case PAGE_GESTURE:
        {
            render_gesture();
            break;
        }
        case PAGE_CONNECTION:
        {
            render_connection();
            break;
        }
        default:
            assert(false);
    }
}