#include "feature_weather.h"

#include "ui_shell.h"
#include "ui_theme.h"
#include "weather_service.h"

static lv_obj_t *s_scr;
static lv_obj_t *s_city_lb;
static lv_obj_t *s_temp_lb;
static lv_obj_t *s_text_lb;
static lv_timer_t *s_timer;

static void refresh_ui(void)
{
    weather_data_t d;
    weather_service_get_data(&d);
    lv_label_set_text(s_city_lb, d.city);
    lv_label_set_text(s_temp_lb, d.temperature);
    lv_label_set_text(s_text_lb, d.loading ? "Loading..." : d.weather);
}

static void poll_cb(lv_timer_t *t)
{
    (void)t;
    refresh_ui();
}

static void on_refresh_clicked(lv_event_t *e)
{
    (void)e;
    weather_service_fetch();
    refresh_ui();
}

lv_obj_t *feature_weather_create_screen(void)
{
    s_scr = lv_obj_create(NULL);
    ui_theme_apply_screen(s_scr);
    ui_shell_add_back_button(s_scr, "Weather");

    s_city_lb = lv_label_create(s_scr);
    lv_obj_set_style_text_font(s_city_lb, UI_FONT_LG, 0);
    lv_obj_set_style_text_color(s_city_lb, UI_COLOR_TEXT, 0);
    lv_obj_align(s_city_lb, LV_ALIGN_CENTER, 0, -50);

    s_temp_lb = lv_label_create(s_scr);
    lv_obj_set_style_text_font(s_temp_lb, UI_FONT_HUGE, 0);
    lv_obj_set_style_text_color(s_temp_lb, UI_COLOR_ACCENT, 0);
    lv_obj_align(s_temp_lb, LV_ALIGN_CENTER, 0, 10);

    s_text_lb = lv_label_create(s_scr);
    lv_obj_set_style_text_font(s_text_lb, UI_FONT_MD, 0);
    lv_obj_set_style_text_color(s_text_lb, UI_COLOR_MUTED, 0);
    lv_obj_align(s_text_lb, LV_ALIGN_CENTER, 0, 70);

    lv_obj_t *btn = lv_button_create(s_scr);
    lv_obj_set_size(btn, 100, 36);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_set_style_bg_color(btn, UI_COLOR_ACCENT, 0);
    lv_obj_add_event_cb(btn, on_refresh_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(btn);
    lv_label_set_text(bl, "Refresh");
    lv_obj_set_style_text_color(bl, UI_COLOR_TEXT, 0);
    lv_obj_center(bl);

    weather_service_fetch();
    s_timer = lv_timer_create(poll_cb, 1000, NULL);
    refresh_ui();
    return s_scr;
}

void feature_weather_destroy(void)
{
    if (s_timer) {
        lv_timer_delete(s_timer);
        s_timer = NULL;
    }
    if (s_scr) {
        lv_obj_delete(s_scr);
        s_scr = NULL;
    }
}
