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
    struct market {
        double bitcoinPrice;
        double dogePrice;
        double omxPrice;
        double nasdaqPrice;
        double bitcoinChange24h;
        double dogeChange24h;
        double omxChange24h;
        double nasdaqChange24h;
    } market;
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

esp_err_t menu_draw_market_data(double bitcoinPrice, double bitcoinChange24h, double dogePrice, double dogeChange24h, double omxPrice, double omxChange24h, double nasdaqPrice, double nasdaqChange24h) {
    esp_err_t ret = ESP_OK;
    assert(initialized);
    data.market.bitcoinPrice = bitcoinPrice;
    data.market.bitcoinChange24h = bitcoinChange24h;
    data.market.dogePrice = dogePrice;
    data.market.dogeChange24h = dogeChange24h;
    data.market.omxPrice = omxPrice;
    data.market.omxChange24h = omxChange24h;
    data.market.nasdaqPrice = nasdaqPrice;
    data.market.nasdaqChange24h = nasdaqChange24h;
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

static void render_market_data(void) {
    char line1[MAX_LINE_LENGTH];
    char line2[MAX_LINE_LENGTH];
    char line3[MAX_LINE_LENGTH];
    char line4[MAX_LINE_LENGTH];
    memset(line1, 0, sizeof(line1));
    memset(line2, 0, sizeof(line2));
    memset(line3, 0, sizeof(line3));
    memset(line4, 0, sizeof(line4));

    display_clear();
    snprintf(line1, sizeof(line1), "BTC: %.0f (%.2f%%)", data.market.bitcoinPrice, data.market.bitcoinChange24h);
    snprintf(line2, sizeof(line2), "DOGE: %.2f (%.2f%%)", data.market.dogePrice, data.market.dogeChange24h);
    snprintf(line3, sizeof(line3), "OMX: %.2f%%", data.market.omxChange24h);
    snprintf(line4, sizeof(line4), "NASDAQ: %.2f%%", data.market.nasdaqChange24h);

    display_draw_text(line1, line2, line3, line4);
}

static void render_gesture(void) {
    char line1[MAX_LINE_LENGTH];
    char line2[MAX_LINE_LENGTH];
    char line3[MAX_LINE_LENGTH];
    memset(line1, 0, sizeof(line1));
    memset(line2, 0, sizeof(line2));
    memset(line3, 0, sizeof(line3));

    display_clear();
    snprintf(line1, sizeof(line1), "%s", tf_gesture_predictor_get_name(data.gesture.prediction.label));
    snprintf(line2, sizeof(line2), "Probability: %f", data.gesture.prediction.probability);
    snprintf(line3, sizeof(line3), "Took %d ms", data.gesture.prediction.calc_time_ms);
    display_draw_text(line1, line2, line3, "");
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
            render_market_data();
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