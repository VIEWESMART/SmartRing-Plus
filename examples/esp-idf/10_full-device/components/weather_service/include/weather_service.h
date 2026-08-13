#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char city[32];
    char weather[32];
    char temperature[16];
    bool valid;
    bool loading;
} weather_data_t;

esp_err_t weather_service_init(void);
void weather_service_set_api_key(const char *key);
void weather_service_set_city(const char *city);
const char *weather_service_get_api_key(void);
const char *weather_service_get_city(void);
esp_err_t weather_service_fetch(void);
void weather_service_get_data(weather_data_t *out);

#ifdef __cplusplus
}
#endif
