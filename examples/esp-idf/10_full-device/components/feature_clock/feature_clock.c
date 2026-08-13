#include "feature_clock.h"

#include <stdio.h>
#include <time.h>

#include "ui_shell.h"
#include "ui_theme.h"

static lv_obj_t *s_scr;
static lv_obj_t *s_time_lb;
static lv_obj_t *s_date_lb;
static lv_timer_t *s_timer;

static const char *week_en[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

static void tick_cb(lv_timer_t *t)
{
    (void)t;
    time_t now = time(NULL);
    struct tm ti;
    localtime_r(&now, &ti);

    char tbuf[16];
    snprintf(tbuf, sizeof(tbuf), "%02d:%02d:%02d", ti.tm_hour, ti.tm_min, ti.tm_sec);
    lv_label_set_text(s_time_lb, tbuf);

    char dbuf[32];
    snprintf(dbuf, sizeof(dbuf), "%04d-%02d-%02d  %s",
             ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday,
             week_en[ti.tm_wday]);
    lv_label_set_text(s_date_lb, dbuf);
}

lv_obj_t *feature_clock_create_screen(void)
{
    s_scr = lv_obj_create(NULL);
    ui_theme_apply_screen(s_scr);
    ui_shell_add_back_button(s_scr, "Clock");

    s_time_lb = lv_label_create(s_scr);
    lv_obj_set_style_text_font(s_time_lb, UI_FONT_HUGE, 0);
    lv_obj_set_style_text_color(s_time_lb, UI_COLOR_TEXT, 0);
    lv_obj_align(s_time_lb, LV_ALIGN_CENTER, 0, -20);

    s_date_lb = lv_label_create(s_scr);
    lv_obj_set_style_text_font(s_date_lb, UI_FONT_MD, 0);
    lv_obj_set_style_text_color(s_date_lb, UI_COLOR_MUTED, 0);
    lv_obj_align(s_date_lb, LV_ALIGN_CENTER, 0, 40);

    s_timer = lv_timer_create(tick_cb, 1000, NULL);
    tick_cb(NULL);
    return s_scr;
}

void feature_clock_destroy(void)
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
