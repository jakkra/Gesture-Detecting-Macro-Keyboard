#pragma once
#include "esp_err.h"

esp_err_t crypto_get_price(const char* name, double* pOutPriceUsd, double* pOutChange24h);
