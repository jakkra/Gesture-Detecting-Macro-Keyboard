#pragma once

#include "hid_dev.h"

void ble_hid_init(void);
void ble_hid_send_key(key_mask_t key_mask, uint8_t* keys, uint16_t num_keys);
void ble_hid_set_pairable(bool pairable);
