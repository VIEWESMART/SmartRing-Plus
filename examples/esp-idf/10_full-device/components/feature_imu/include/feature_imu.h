#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *feature_imu_create_screen(void);
void feature_imu_destroy(void);

#ifdef __cplusplus
}
#endif
