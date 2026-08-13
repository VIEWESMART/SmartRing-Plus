#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *feature_recorder_create_screen(void);
void feature_recorder_destroy(void);

#ifdef __cplusplus
}
#endif
