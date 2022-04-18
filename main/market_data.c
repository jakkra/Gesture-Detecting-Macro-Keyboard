#include "market_data.h"
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_http_client.h"

static const char *TAG = "market_data";

#define MAX_HTTP_RSP_LEN    1600
#define MAX_URL_LEN         150

static esp_err_t http_event_handler(esp_http_client_event_t *evt);
esp_err_t get_price(const char* url, double* pOutVal1, double* pOutVal2, char* needle1, char* needle2);

static uint8_t http_get_rsp_buf[MAX_HTTP_RSP_LEN];
static uint32_t payload_length;

#define JSON_PRICE_NEEDLE_CRYPTO "\"priceUsd\":\""
#define JSON_PERCENT_NEEDLE_CRYPTO "\"changePercent24Hr\":\""
#define JSON_PRICE_NEEDLE_YAHOO "\"regularMarketPrice\":"
#define JSON_PERCENT_NEEDLE_YAHOO "\"regularMarketChangePercent\":"

esp_err_t market_data_get_crypto(const char* name, double* pOutPriceUsd, double* pOutChange24h) {
    char url[MAX_URL_LEN];
    esp_err_t ret = ESP_FAIL;
    memset(url, 0, sizeof(url));

    snprintf(url, sizeof(url), "http://api.coincap.io/v2/assets/%s", name);
    ret = get_price(url, pOutPriceUsd, pOutChange24h, JSON_PRICE_NEEDLE_CRYPTO, JSON_PERCENT_NEEDLE_CRYPTO);

    return ret;
}

esp_err_t market_data_get_stock(const char* name, double* pOutPrice, double* pOutChange24h) {
    char url[MAX_URL_LEN];
    esp_err_t ret = ESP_FAIL;
    memset(url, 0, sizeof(url));

    snprintf(url, sizeof(url), "https://query1.finance.yahoo.com/v7/finance/quote?&corsDomain=finance.yahoo.com&symbols=%s", name);
    ret = get_price(url, pOutPrice, pOutChange24h, JSON_PRICE_NEEDLE_YAHOO, JSON_PERCENT_NEEDLE_YAHOO);

    return ret;
}

esp_err_t get_price(const char* url, double* pOutVal1, double* pOutVal2, char* needle1, char* needle2) {
    esp_err_t ret = ESP_FAIL;

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
    };
    payload_length = 0;
    memset(http_get_rsp_buf, 0, sizeof(http_get_rsp_buf));
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        uint32_t content_len = esp_http_client_get_content_length(client);
        uint32_t status_code = esp_http_client_get_status_code(client);

        if (status_code == 200 && content_len < MAX_HTTP_RSP_LEN && payload_length > 0) {
            char* pEnd;
            char* pVal1 = strstr((char*)http_get_rsp_buf, needle1);
            char* pBal2 = strstr((char*)http_get_rsp_buf, needle2);
            pVal1 += strlen(needle1);
            pBal2 += strlen(needle2);
            *pOutVal1 = strtod(pVal1, &pEnd);
            *pOutVal2 = strtod(pBal2, &pEnd);
            ret = ESP_OK;
        } else {
            ESP_LOGE(TAG, "Unexpected content length %d or status code %d", content_len, status_code);
        }
    }
    esp_http_client_cleanup(client);

    return ret;
}


static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
        case HTTP_EVENT_ON_CONNECTED:
        case HTTP_EVENT_HEADER_SENT:
        case HTTP_EVENT_ON_HEADER:
        case HTTP_EVENT_ON_FINISH:
        case HTTP_EVENT_DISCONNECTED:
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (payload_length + evt->data_len <= MAX_HTTP_RSP_LEN) {
                memcpy(&http_get_rsp_buf[payload_length], evt->data, evt->data_len);
                payload_length += evt->data_len;
            } else {
                ESP_LOGE(TAG, "HTTP_EVENT_ON_DATA payload won't fit buffer, discarding...");
                payload_length = 0;
            }
            break;
    }

    return ESP_OK;
}