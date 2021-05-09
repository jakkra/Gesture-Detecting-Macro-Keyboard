#pragma once

#include "hid_dev.h"
#include "esp_err.h"

typedef enum ble_hid_connection {
    BLE_HID_CONNECTED,
    BLE_HID_DISCONNECTED
} ble_hid_connection;

typedef void hid_connection_callback(ble_hid_connection event, esp_bd_addr_t* addr);

void ble_hid_init(hid_connection_callback* callback);
esp_err_t ble_hid_send_key(key_mask_t key_mask, uint8_t* keys, uint16_t num_keys);
esp_err_t ble_hid_send_consumer_key(consumer_cmd_t key, bool key_pressed);
void ble_hid_set_pairable(bool pairable);
esp_err_t ble_hid_request_access(uint32_t timeout_ms);
esp_err_t ble_hid_give_access(void);
esp_bd_addr_t* ble_hid_get_connected(void);