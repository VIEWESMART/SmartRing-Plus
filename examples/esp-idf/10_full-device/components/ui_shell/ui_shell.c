/*
 * 主界面 Shell：圆形主页（对齐 Figma Home / 360 Round）、状态栏、低电量弹窗、功能导航
 * 设计稿: 
 */
#include "ui_shell.h"

#include <stdio.h>
#include <time.h>

#include "bsp/smartring_plus.h"
#include "esp_err.h"
#include "esp_lv_adapter.h"
#include "ui_theme.h"

static lv_obj_t *s_home;
static lv_obj_t *s_status_time;
static lv_obj_t *s_status_date;
static lv_obj_t *s_status_bat;
static lv_obj_t *s_low_batt_modal;
static ui_feature_id_t s_active_id = UI_FEATURE_COUNT;
static lv_timer_t *s_status_timer;
static bool s_low_batt_shown;

typedef struct {
    const char *icon;
    const char *label;
    ui_feature_id_t id;
    uint32_t tile_hex;
    lv_coord_t cx; /* 圆形按钮中心 */
    lv_coord_t cy;
} menu_item_t;

/*
 * hex 2-3-2：以圆心对齐，侧列内收 + 行距加大，标签落在圆屏安全区
 * 有效可视半径约 150（含表圈遮挡）
 */
static const menu_item_t s_menu[] = {
    { NULL,                "Clock",    UI_FEATURE_CLOCK,    0x2F8683, 142, 156 }, /* 自绘表盘，见 add_clock_glyph */
    { LV_SYMBOL_GPS,       "Weather",  UI_FEATURE_WEATHER,  0x4A7499, 218, 156 },
    { LV_SYMBOL_AUDIO,     "Record",   UI_FEATURE_RECORDER, 0x854646, 112, 224 },
    { LV_SYMBOL_PLAY,      "Music",    UI_FEATURE_MUSIC,    0x8A6744, 180, 224 },
    { LV_SYMBOL_SHUFFLE,   "IMU",      UI_FEATURE_IMU,      0x3D8B8B, 248, 224 },
    { LV_SYMBOL_IMAGE,     "Album",    UI_FEATURE_ALBUM,    0x8A5266, 142, 288 },
    { LV_SYMBOL_SETTINGS,  "Settings", UI_FEATURE_SETTINGS, 0x556575, 218, 288 },
};

static ui_feature_create_fn s_create_fns[UI_FEATURE_COUNT];
static ui_feature_destroy_fn s_destroy_fns[UI_FEATURE_COUNT];

static void destroy_active_feature(void)
{
    if (s_active_id < UI_FEATURE_COUNT && s_destroy_fns[s_active_id]) {
        s_destroy_fns[s_active_id]();
    }
    s_active_id = UI_FEATURE_COUNT;
}

static void on_back_clicked(lv_event_t *e)
{
    (void)e;
    ui_shell_show_home();
}

static void on_tile_clicked(lv_event_t *e)
{
    ui_feature_id_t id = (ui_feature_id_t)(intptr_t)lv_event_get_user_data(e);
    ui_shell_open_feature(id);
}

static void build_hero_clock(lv_obj_t *parent)
{
    /* 电量胶囊居中在时间上方；收紧高度给下方应用区留间距 */
    lv_obj_t *hero = lv_obj_create(parent);
    lv_obj_set_size(hero, 240, 100);
    lv_obj_set_pos(hero, 60, 22);
    lv_obj_set_style_bg_opa(hero, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hero, 0, 0);
    lv_obj_set_style_pad_all(hero, 0, 0);
    lv_obj_remove_flag(hero, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(hero, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    lv_obj_t *pill = lv_obj_create(hero);
    lv_obj_set_size(pill, 68, 22);
    lv_obj_align(pill, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_radius(pill, 20, 0);
    lv_obj_set_style_bg_color(pill, UI_COLOR_SURFACE, 0);
    lv_obj_set_style_bg_opa(pill, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(pill, 1, 0);
    lv_obj_set_style_border_color(pill, UI_COLOR_ACCENT, 0);
    lv_obj_set_style_pad_hor(pill, 8, 0);
    lv_obj_set_style_pad_ver(pill, 2, 0);
    lv_obj_remove_flag(pill, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *dot = lv_obj_create(pill);
    lv_obj_set_size(dot, 6, 6);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, UI_COLOR_ACCENT, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_align(dot, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_remove_flag(dot, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    s_status_bat = lv_label_create(pill);
    lv_obj_set_style_text_font(s_status_bat, UI_FONT_SM, 0);
    lv_obj_set_style_text_color(s_status_bat, UI_COLOR_TEXT, 0);
    lv_label_set_text(s_status_bat, "--%");
    lv_obj_align(s_status_bat, LV_ALIGN_RIGHT_MID, 0, 0);

    s_status_time = lv_label_create(hero);
    lv_obj_set_style_text_font(s_status_time, UI_FONT_HUGE, 0);
    lv_obj_set_style_text_color(s_status_time, UI_COLOR_TEXT, 0);
    lv_label_set_text(s_status_time, "--:--");
    lv_obj_align(s_status_time, LV_ALIGN_TOP_MID, 0, 24);

    s_status_date = lv_label_create(hero);
    lv_obj_set_style_text_font(s_status_date, UI_FONT_SM, 0);
    lv_obj_set_style_text_color(s_status_date, UI_COLOR_MUTED, 0);
    lv_label_set_text(s_status_date, "---");
    lv_obj_align(s_status_date, LV_ALIGN_TOP_MID, 0, 78);
}

/** 迷你表盘图标（接近 ⏰；LVGL 内置 Font Awesome 无时钟符号） */
static void add_clock_glyph(lv_obj_t *btn)
{
    lv_obj_t *face = lv_obj_create(btn);
    lv_obj_set_size(face, 20, 20);
    lv_obj_center(face);
    lv_obj_set_style_radius(face, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(face, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(face, 2, 0);
    lv_obj_set_style_border_color(face, UI_COLOR_TEXT, 0);
    lv_obj_set_style_pad_all(face, 0, 0);
    lv_obj_remove_flag(face, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *min_h = lv_obj_create(face);
    lv_obj_set_size(min_h, 2, 7);
    lv_obj_align(min_h, LV_ALIGN_CENTER, 0, -2);
    lv_obj_set_style_radius(min_h, 1, 0);
    lv_obj_set_style_bg_color(min_h, UI_COLOR_TEXT, 0);
    lv_obj_set_style_bg_opa(min_h, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(min_h, 0, 0);
    lv_obj_set_style_pad_all(min_h, 0, 0);
    lv_obj_remove_flag(min_h, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *hr_h = lv_obj_create(face);
    lv_obj_set_size(hr_h, 5, 2);
    lv_obj_align(hr_h, LV_ALIGN_CENTER, 2, 0);
    lv_obj_set_style_radius(hr_h, 1, 0);
    lv_obj_set_style_bg_color(hr_h, UI_COLOR_TEXT, 0);
    lv_obj_set_style_bg_opa(hr_h, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(hr_h, 0, 0);
    lv_obj_set_style_pad_all(hr_h, 0, 0);
    lv_obj_remove_flag(hr_h, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *dot = lv_obj_create(face);
    lv_obj_set_size(dot, 3, 3);
    lv_obj_center(dot);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, UI_COLOR_TEXT, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_set_style_pad_all(dot, 0, 0);
    lv_obj_remove_flag(dot, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
}

static void build_menu_hex(lv_obj_t *parent)
{
    const int disc = 44;

    for (int i = 0; i < (int)(sizeof(s_menu) / sizeof(s_menu[0])); i++) {
        const menu_item_t *m = &s_menu[i];

        lv_obj_t *btn = lv_button_create(parent);
        /* 固定正方形 + 全圆角，避免默认 padding/拉伸把按钮拉成椭圆 */
        lv_obj_set_style_pad_all(btn, 0, 0);
        lv_obj_set_style_min_width(btn, disc, 0);
        lv_obj_set_style_min_height(btn, disc, 0);
        lv_obj_set_size(btn, disc, disc);
        lv_obj_set_pos(btn, m->cx - disc / 2, m->cy - disc / 2);
        lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(m->tile_hex), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_add_event_cb(btn, on_tile_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)m->id);

        if (m->icon) {
            lv_obj_t *ic = lv_label_create(btn);
            lv_label_set_text(ic, m->icon);
            lv_obj_set_style_text_color(ic, UI_COLOR_TEXT, 0);
            lv_obj_center(ic);
        } else if (m->id == UI_FEATURE_CLOCK) {
            add_clock_glyph(btn);
        }

        /* 标签直接挂在屏幕上并按按钮中心对齐，避免被父容器裁切 */
        lv_obj_t *lb = lv_label_create(parent);
        lv_label_set_text(lb, m->label);
        lv_obj_set_style_text_font(lb, UI_FONT_SM, 0);
        lv_obj_set_style_text_color(lb, UI_COLOR_MUTED, 0);
        lv_obj_remove_flag(lb, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_update_layout(lb);
        lv_coord_t lw = lv_obj_get_width(lb);
        lv_coord_t lh = lv_obj_get_height(lb);
        lv_obj_set_pos(lb, m->cx - lw / 2, m->cy + disc / 2 + 6);
        (void)lh;
    }
}

static void ensure_low_batt_modal(lv_obj_t *parent)
{
    if (s_low_batt_modal) {
        return;
    }
    s_low_batt_modal = lv_obj_create(parent);
    lv_obj_set_size(s_low_batt_modal, 200, 120);
    lv_obj_center(s_low_batt_modal);
    lv_obj_set_style_bg_color(s_low_batt_modal, UI_COLOR_BG, 0);
    lv_obj_set_style_border_color(s_low_batt_modal, UI_COLOR_WARN, 0);
    lv_obj_set_style_border_width(s_low_batt_modal, 2, 0);
    lv_obj_set_style_radius(s_low_batt_modal, 16, 0);
    lv_obj_add_flag(s_low_batt_modal, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *lb = lv_label_create(s_low_batt_modal);
    lv_label_set_text(lb, LV_SYMBOL_WARNING " Low battery");
    lv_obj_set_style_text_color(lb, UI_COLOR_WARN, 0);
    lv_obj_set_style_text_font(lb, UI_FONT_LG, 0);
    lv_obj_set_style_text_align(lb, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(lb, 180);
    lv_obj_center(lb);
}

static void status_timer_cb(lv_timer_t *t)
{
    (void)t;
    time_t now = time(NULL);
    struct tm ti;
    localtime_r(&now, &ti);

    char tbuf[8];
    snprintf(tbuf, sizeof(tbuf), "%02d:%02d", ti.tm_hour, ti.tm_min);
    if (s_status_time) {
        lv_label_set_text(s_status_time, tbuf);
    }

    static const char *k_wday[] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };
    static const char *k_mon[] = {
        "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
        "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
    };
    /* 用 ASCII '-'，Montserrat 无 U+00B7 中间点会显示方块 */
    char dbuf[24];
    snprintf(dbuf, sizeof(dbuf), "%s - %s %d",
             k_wday[ti.tm_wday % 7], k_mon[ti.tm_mon % 12], ti.tm_mday);
    if (s_status_date) {
        lv_label_set_text(s_status_date, dbuf);
    }

    int pct = bsp_battery_get_percent();
    char bbuf[8];
    snprintf(bbuf, sizeof(bbuf), "%d%%", pct);
    if (s_status_bat) {
        lv_label_set_text(s_status_bat, bbuf);
    }

    bool charging = bsp_battery_is_charging();
    if (pct < UI_BATTERY_LOW_PCT && !charging) {
        if (!s_low_batt_shown && s_low_batt_modal) {
            lv_obj_remove_flag(s_low_batt_modal, LV_OBJ_FLAG_HIDDEN);
            s_low_batt_shown = true;
        }
    } else if (s_low_batt_modal) {
        lv_obj_add_flag(s_low_batt_modal, LV_OBJ_FLAG_HIDDEN);
        s_low_batt_shown = false;
    }
}

static lv_obj_t *build_home_screen(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    ui_theme_apply_screen(scr);

    /* 外圈淡色强调环（Figma Accent Ring） */
    lv_obj_t *ring = lv_obj_create(scr);
    lv_obj_set_size(ring, 348, 348);
    lv_obj_center(ring);
    lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(ring, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ring, 2, 0);
    lv_obj_set_style_border_color(ring, UI_COLOR_ACCENT, 0);
    lv_obj_set_style_border_opa(ring, LV_OPA_30, 0);
    lv_obj_remove_flag(ring, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    build_hero_clock(scr);
    build_menu_hex(scr);
    ensure_low_batt_modal(scr);
    return scr;
}

void ui_shell_register_feature(ui_feature_id_t id, ui_feature_create_fn create, ui_feature_destroy_fn destroy)
{
    if (id >= 0 && id < UI_FEATURE_COUNT) {
        s_create_fns[id] = create;
        s_destroy_fns[id] = destroy;
    }
}

void ui_shell_start(void)
{
    ESP_ERROR_CHECK(esp_lv_adapter_lock(-1));
    s_home = build_home_screen();
    lv_screen_load(s_home);
    if (!s_status_timer) {
        s_status_timer = lv_timer_create(status_timer_cb, 1000, NULL);
        status_timer_cb(NULL);
    }
    esp_lv_adapter_unlock();
}

void ui_shell_show_home(void)
{
    ESP_ERROR_CHECK(esp_lv_adapter_lock(-1));
    if (!s_home) {
        s_home = build_home_screen();
    }
    lv_screen_load(s_home);
    destroy_active_feature();
    lv_obj_invalidate(s_home);
    esp_lv_adapter_unlock();
}

void ui_shell_open_feature(ui_feature_id_t id)
{
    if (id < 0 || id >= UI_FEATURE_COUNT || !s_create_fns[id]) {
        return;
    }
    ESP_ERROR_CHECK(esp_lv_adapter_lock(-1));
    ui_feature_id_t prev = s_active_id;
    ui_feature_destroy_fn prev_destroy = NULL;
    if (prev < UI_FEATURE_COUNT) {
        prev_destroy = s_destroy_fns[prev];
        s_active_id = UI_FEATURE_COUNT;
    }
    lv_obj_t *scr = s_create_fns[id]();
    if (scr) {
        s_active_id = id;
        lv_screen_load(scr);
        if (prev_destroy) {
            prev_destroy();
        }
    } else if (prev_destroy) {
        if (!s_home) {
            s_home = build_home_screen();
        }
        lv_screen_load(s_home);
        prev_destroy();
    }
    esp_lv_adapter_unlock();
}

lv_obj_t *ui_shell_get_home_screen(void)
{
    return s_home;
}

lv_obj_t *ui_shell_add_back_button(lv_obj_t *parent, const char *title)
{
    lv_obj_t *hdr = lv_obj_create(parent);
    lv_obj_set_size(hdr, UI_SAFE_W, 40);
    lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_bg_opa(hdr, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_remove_flag(hdr, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *back = lv_button_create(hdr);
    lv_obj_set_size(back, 64, 32);
    lv_obj_align(back, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(back, UI_COLOR_MUTED, 0);
    lv_obj_add_event_cb(back, on_back_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(back);
    lv_label_set_text(bl, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_font(bl, UI_FONT_SM, 0);
    lv_obj_center(bl);

    if (title) {
        lv_obj_t *tl = lv_label_create(hdr);
        lv_label_set_text(tl, title);
        lv_obj_set_style_text_font(tl, UI_FONT_LG, 0);
        lv_obj_set_style_text_color(tl, UI_COLOR_TEXT, 0);
        lv_obj_align(tl, LV_ALIGN_CENTER, 0, 0);
    }
    return hdr;
}
