#pragma once

#include "lvgl.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t feature_album_decoder_init(void);
lv_obj_t *feature_album_create_screen(void);
void feature_album_destroy(void);

#ifdef __cplusplus
}
#endif
