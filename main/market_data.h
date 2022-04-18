#pragma once
#include "esp_err.h"

esp_err_t market_data_get_crypto(const char* name, double* pOutPriceUsd, double* pOutChange24h);
esp_err_t market_data_get_stock(const char* name, double* pOutPrice, double* pOutChange24h);
