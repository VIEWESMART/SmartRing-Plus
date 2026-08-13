#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UI_FEATURE_CLOCK = 0,
    UI_FEATURE_WEATHER,
    UI_FEATURE_RECORDER,
    UI_FEATURE_MUSIC,
    UI_FEATURE_IMU,
    UI_FEATURE_ALBUM,
    UI_FEATURE_SETTINGS,
    UI_FEATURE_COUNT
} ui_feature_id_t;

typedef lv_obj_t *(*ui_feature_create_fn)(void);
typedef void (*ui_feature_destroy_fn)(void);

void ui_shell_register_feature(ui_feature_id_t id, ui_feature_create_fn create, ui_feature_destroy_fn destroy);
void ui_shell_start(void);
void ui_shell_show_home(void);
void ui_shell_open_feature(ui_feature_id_t id);
lv_obj_t *ui_shell_get_home_screen(void);
lv_obj_t *ui_shell_add_back_button(lv_obj_t *parent, const char *title);

#ifdef __cplusplus
}
#endif
