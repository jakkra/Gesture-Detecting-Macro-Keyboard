#include "menu.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_err.h"
#include "esp_log.h"
#include "tf_gesture_predictor.h"

#define IP_STR_MAX_LEN      15
#define MAC_STR_MAX_LEN     6 * 2
#define SSID_MAX_LEN        20
#define WORK_QUEUE_SIZE     5
#define MAX_LINE_LENGTH     50

static const char* TAG = "menu";

typedef enum work_t {
    WORK_REDRAW,
    WORK_INIT_ANIMTATION,
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
        bool connected;
        char ssid[SSID_MAX_LEN + 1]
;    } connection;
    page_t current_page;
} menu_data_t;

static void render_current_page(void);
static void work(void* arg);
static void draw_startup_animation(void);
static void schedule_redraw(void);

static menu_data_t data;
static xQueueHandle work_queue = NULL;

static bool initialized = false;

esp_err_t menu_init(i2c_port_t port, gpio_num_t  sda, gpio_num_t scl, gpio_num_t rst){
    esp_err_t ret = ESP_OK;
    memset(&data, 0, sizeof(data));
    
    data.current_page = PAGE_CONNECTION;
    ret = display_init(port, sda, scl, rst);
    if (ret == ESP_OK) {
        work_queue = xQueueCreate(WORK_QUEUE_SIZE, sizeof(work_t));
        assert(work_queue != NULL);
        xTaskCreate(work, "menu_work", 4096, NULL, 10, NULL);
        work_t* task = malloc(sizeof(work_t));
        *task = WORK_INIT_ANIMTATION;
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
        schedule_redraw();
    }
    return ret;
}

esp_err_t menu_draw_gestures(gesture_prediction_t* prediction) {
    esp_err_t ret = ESP_OK;
    assert(initialized);
    memcpy(&data.gesture.prediction, prediction, sizeof(gesture_prediction_t));
    if (data.current_page == PAGE_GESTURE) {
        schedule_redraw();
    }
    return ret;
}

esp_err_t menu_draw_connection_status(char* ssid, bool connected, char* wifi_ip, char* ble_addr) {
    esp_err_t ret = ESP_OK;
    assert(initialized);
    strncpy(data.connection.ip, wifi_ip, IP_STR_MAX_LEN);
    strncpy(data.connection.addr, ble_addr, MAC_STR_MAX_LEN);
    strncpy(data.connection.ssid, ssid, SSID_MAX_LEN);
    data.connection.connected = connected;
    if (data.current_page == PAGE_CONNECTION) {
        schedule_redraw();
    }
    return ret;
}

esp_err_t menu_next_page(void){
    esp_err_t ret = ESP_OK;
    assert(initialized);
    data.current_page = (page_t)((data.current_page + 1) % PAGE_HIDDEN_FIRST);
    schedule_redraw();
    return ret;
}

esp_err_t menu_set_page(page_t page) {
    esp_err_t ret = ESP_OK;
    assert(initialized);
    data.current_page = page;
    schedule_redraw();
    return ret;
}

static void work(void* arg)
{
    work_t* work_type;

    for (;;) {
        if (xQueueReceive(work_queue, &work_type, portMAX_DELAY)) {
            switch (*work_type) {
                case WORK_INIT_ANIMTATION:
                    draw_startup_animation();
                    break;
                case WORK_REDRAW:
                    render_current_page();
                    break;
                default:
                    ESP_LOGE(TAG, "Unknown work type: %d\n", *work_type);
            }
            free(work_type);
        }
    }
}

static void draw_startup_animation(void) {
    display_clear();
    display_draw_animation();
    schedule_redraw();
}

static void schedule_redraw(void)
{
    work_t* task = malloc(sizeof(work_t));
    *task = WORK_REDRAW;
    xQueueSend(work_queue, (void*)&task, (TickType_t)0);
}

static void render_crypto(void) {
    char line1[MAX_LINE_LENGTH];
    char line2[MAX_LINE_LENGTH];
    memset(line1, 0, sizeof(line1));
    memset(line2, 0, sizeof(line2));

    display_clear();
    snprintf(line1, sizeof(line1), "BTC: %.0f (%.2f%%)", data.crypto.bitcoinPrice, data.crypto.bitcoinChange24h);

    snprintf(line2, sizeof(line2), "DOGE: %.2f (%.2f%%)", data.crypto.dogePrice, data.crypto.dogeChange24h);
    display_draw_text(line1, line2, "", "");
}

static void render_gesture(void) {
    char line1[MAX_LINE_LENGTH];
    char line2[MAX_LINE_LENGTH];
    memset(line1, 0, sizeof(line1));
    memset(line2, 0, sizeof(line2));

    display_clear();
    snprintf(line1, sizeof(line1), "Probability: %f", data.gesture.prediction.probability);
    snprintf(line2, sizeof(line2), "%s", tf_gesture_predictor_get_name(data.gesture.prediction.label));
    display_draw_text(line1, line2, "", "");
}

static void render_connection(void) {
    char line1[MAX_LINE_LENGTH];
    char line2[MAX_LINE_LENGTH];
    char line3[MAX_LINE_LENGTH];
    memset(line1, 0, sizeof(line1));
    memset(line2, 0, sizeof(line2));
    memset(line3, 0, sizeof(line3));

    snprintf(line1, sizeof(line1), "SSID: %s", data.connection.ssid);
    snprintf(line2, sizeof(line2), "IP: %s", data.connection.ip);
    snprintf(line3, sizeof(line3), "BLE: %s", data.connection.addr);
    display_clear();
    display_draw_text(line1, line2, line3, "");
}

static void render_pairing(void) {
    display_clear();
    display_draw_text("Pairing enabled...", "", "", "");
}

static void render_current_page(void) {
    printf("Render current page: %d\n", data.current_page);
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
        case PAGE_PAIRING:
        {
            render_pairing();
            break;
        }
        default:
            assert(false);
    }
}