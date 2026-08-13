#include "feature_settings.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "bsp/smartring_plus.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "ui_shell.h"
#include "ui_theme.h"
#include "wifi_service.h"
#include "weather_service.h"

static lv_obj_t *s_scr;
static lv_obj_t *s_key_ta;
static lv_obj_t *s_info_lb;
static lv_obj_t *s_bl_slider;
static lv_obj_t *s_wifi_status;
static lv_obj_t *s_wifi_list;
static lv_obj_t *s_scan_btn_lb;
static lv_timer_t *s_poll;

static lv_obj_t *s_wifi_modal;
static lv_obj_t *s_pass_ta;
static lv_obj_t *s_kb;
static char s_sel_ssid[WIFI_SVC_SSID_LEN];
static bool s_sel_open; /* open network, no password */

static void dismiss_modal_cb(lv_event_t *e);
static void shutdown_confirm_cb(lv_event_t *e);
static void close_wifi_modal(void);
static void rebuild_wifi_list(void);

static const char *status_str(wifi_svc_status_t st)
{
    switch (st) {
    case WIFI_SVC_SCANNING:   return "Scanning...";
    case WIFI_SVC_CONNECTING: return "Connecting...";
    case WIFI_SVC_CONNECTED:  return "Connected";
    case WIFI_SVC_FAILED:     return "Failed";
    default:                  return "Disconnected";
    }
}

static void refresh_info(void)
{
    bsp_battery_data_t bat;
    bsp_battery_get_data(&bat);
    int mv = (int)(bat.voltage_v * 1000 + 0.5f);
    char buf[320];
    wifi_svc_status_t st = wifi_service_get_status();
    snprintf(buf, sizeof(buf),
             "IDF: %s\nFlash: 16MB  PSRAM: 8MB\nFree heap: %lu KB\n"
             "Battery: %d mV  %d%%  %s\nWiFi: %s\nSSID: %s\nIP: %s",
             esp_get_idf_version(),
             (unsigned long)(esp_get_free_heap_size() / 1024),
             mv, bat.percent, bat.charging ? "Charging" : "On battery",
             status_str(st),
             st == WIFI_SVC_CONNECTED ? wifi_service_get_ssid() : "-",
             wifi_service_get_ip_str());
    if (s_info_lb) {
        lv_label_set_text(s_info_lb, buf);
    }
}

static void refresh_wifi_status(void)
{
    if (!s_wifi_status) {
        return;
    }
    wifi_svc_status_t st = wifi_service_get_status();
    char buf[96];
    if (st == WIFI_SVC_CONNECTED) {
        snprintf(buf, sizeof(buf), "%s  %s", wifi_service_get_ssid(), wifi_service_get_ip_str());
    } else {
        snprintf(buf, sizeof(buf), "%s", status_str(st));
    }
    lv_label_set_text(s_wifi_status, buf);

    if (s_scan_btn_lb) {
        lv_label_set_text(s_scan_btn_lb, st == WIFI_SVC_SCANNING ? "Scanning..." : "Scan WiFi");
    }
}

static void poll_cb(lv_timer_t *t)
{
    (void)t;
    static wifi_svc_status_t prev = WIFI_SVC_DISCONNECTED;
    static bool was_scanning;

    wifi_svc_status_t st = wifi_service_get_status();
    bool scanning = (st == WIFI_SVC_SCANNING) || !wifi_service_scan_done();

    if (was_scanning && wifi_service_scan_done()) {
        rebuild_wifi_list();
    }
    was_scanning = scanning;

    if (st != prev || st == WIFI_SVC_CONNECTING || st == WIFI_SVC_CONNECTED) {
        refresh_wifi_status();
        refresh_info();
        if (st == WIFI_SVC_CONNECTED) {
            close_wifi_modal();
        }
    }
    prev = st;
}

static void on_bl_changed(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target_obj(e);
    int v = lv_slider_get_value(slider);
    bsp_display_backlight_set((uint8_t)v);
}

static void on_scan_clicked(lv_event_t *e)
{
    (void)e;
    if (wifi_service_get_status() == WIFI_SVC_SCANNING) {
        return;
    }
    if (s_wifi_list) {
        lv_obj_clean(s_wifi_list);
        lv_list_add_text(s_wifi_list, "Scanning...");
    }
    wifi_service_scan_start();
    refresh_wifi_status();
}

static void on_wifi_connect_clicked(lv_event_t *e)
{
    (void)e;
    const char *pass = s_pass_ta ? lv_textarea_get_text(s_pass_ta) : "";
    if (!s_sel_open && (!pass || pass[0] == '\0')) {
        if (s_wifi_status) {
            lv_label_set_text(s_wifi_status, "Enter password");
        }
        return;
    }
    wifi_service_connect(s_sel_ssid, s_sel_open ? "" : pass);
    refresh_wifi_status();
}

static void on_wifi_cancel_clicked(lv_event_t *e)
{
    (void)e;
    close_wifi_modal();
}

static void close_wifi_modal(void)
{
    if (s_wifi_modal) {
        lv_obj_delete(s_wifi_modal);
        s_wifi_modal = NULL;
    }
    s_kb = NULL;
    s_pass_ta = NULL;
}

static void open_wifi_modal(const char *ssid, bool open_net)
{
    close_wifi_modal();
    strlcpy(s_sel_ssid, ssid, sizeof(s_sel_ssid));
    s_sel_open = open_net;

    s_wifi_modal = lv_obj_create(s_scr);
    lv_obj_set_size(s_wifi_modal, UI_SAFE_W, UI_SAFE_H);
    lv_obj_center(s_wifi_modal);
    lv_obj_set_style_bg_color(s_wifi_modal, UI_COLOR_BG, 0);
    lv_obj_set_style_border_width(s_wifi_modal, 0, 0);
    lv_obj_set_style_pad_all(s_wifi_modal, 8, 0);
    lv_obj_remove_flag(s_wifi_modal, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(s_wifi_modal);
    lv_label_set_text(title, "Connect");
    lv_obj_set_style_text_font(title, UI_FONT_MD, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    lv_obj_t *ssid_lb = lv_label_create(s_wifi_modal);
    lv_label_set_text(ssid_lb, ssid);
    lv_obj_set_width(ssid_lb, 220);
    lv_label_set_long_mode(ssid_lb, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_color(ssid_lb, UI_COLOR_ACCENT, 0);
    lv_obj_align(ssid_lb, LV_ALIGN_TOP_MID, 0, 28);

    if (!open_net) {
        s_pass_ta = lv_textarea_create(s_wifi_modal);
        lv_textarea_set_one_line(s_pass_ta, true);
        lv_textarea_set_password_mode(s_pass_ta, true);
        lv_textarea_set_placeholder_text(s_pass_ta, "Password");
        lv_textarea_set_max_length(s_pass_ta, 63);
        lv_obj_set_width(s_pass_ta, 220);
        lv_obj_align(s_pass_ta, LV_ALIGN_TOP_MID, 0, 52);
        lv_obj_add_state(s_pass_ta, LV_STATE_FOCUSED);
    } else {
        lv_obj_t *hint = lv_label_create(s_wifi_modal);
        lv_label_set_text(hint, "Open network");
        lv_obj_set_style_text_color(hint, UI_COLOR_MUTED, 0);
        lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 56);
    }

    lv_obj_t *row = lv_obj_create(s_wifi_modal);
    lv_obj_set_size(row, 220, 36);
    lv_obj_align(row, LV_ALIGN_TOP_MID, 0, open_net ? 84 : 92);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *ok = lv_button_create(row);
    lv_obj_set_size(ok, 90, 32);
    lv_obj_set_style_bg_color(ok, UI_COLOR_ACCENT, 0);
    lv_obj_add_event_cb(ok, on_wifi_connect_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_t *okl = lv_label_create(ok);
    lv_label_set_text(okl, "Connect");
    lv_obj_set_style_text_color(okl, UI_COLOR_TEXT, 0);
    lv_obj_center(okl);

    lv_obj_t *cancel = lv_button_create(row);
    lv_obj_set_size(cancel, 90, 32);
    lv_obj_set_style_bg_color(cancel, UI_COLOR_MUTED, 0);
    lv_obj_add_event_cb(cancel, on_wifi_cancel_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_t *cl = lv_label_create(cancel);
    lv_label_set_text(cl, "Cancel");
    lv_obj_set_style_text_color(cl, UI_COLOR_TEXT, 0);
    lv_obj_center(cl);

    if (!open_net) {
        s_kb = lv_keyboard_create(s_wifi_modal);
        lv_obj_set_size(s_kb, UI_SAFE_W - 8, 110);
        lv_obj_align(s_kb, LV_ALIGN_BOTTOM_MID, 0, -4);
        lv_keyboard_set_textarea(s_kb, s_pass_ta);
        lv_obj_set_style_text_font(s_kb, UI_FONT_SM, 0);
    }
}

static void on_ap_clicked(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    wifi_svc_ap_t ap;
    if (wifi_service_get_ap(idx, &ap) != ESP_OK) {
        return;
    }
    open_wifi_modal(ap.ssid, ap.authmode == WIFI_AUTH_OPEN);
}

static void rebuild_wifi_list(void)
{
    if (!s_wifi_list) {
        return;
    }
    lv_obj_clean(s_wifi_list);

    int n = wifi_service_get_ap_count();
    if (n <= 0) {
        lv_list_add_text(s_wifi_list, "(No networks)");
        return;
    }

    for (int i = 0; i < n; i++) {
        wifi_svc_ap_t ap;
        if (wifi_service_get_ap(i, &ap) != ESP_OK) {
            continue;
        }
        char label[48];
        bool open_net = (ap.authmode == WIFI_AUTH_OPEN);
        snprintf(label, sizeof(label), "%s  %ddBm%s", ap.ssid, (int)ap.rssi, open_net ? " *" : "");

        lv_obj_t *btn = lv_list_add_button(s_wifi_list, LV_SYMBOL_WIFI, label);
        lv_obj_add_event_cb(btn, on_ap_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }
}

static void on_key_save(lv_event_t *e)
{
    (void)e;
    weather_service_set_api_key(lv_textarea_get_text(s_key_ta));
}

static void on_shutdown(lv_event_t *e)
{
    (void)e;
    lv_obj_t *box = lv_obj_create(s_scr);
    lv_obj_set_size(box, 220, 120);
    lv_obj_center(box);
    lv_obj_set_style_bg_color(box, UI_COLOR_BG, 0);
    lv_obj_set_style_border_color(box, UI_COLOR_WARN, 0);
    lv_obj_set_style_border_width(box, 2, 0);

    lv_obj_t *tl = lv_label_create(box);
    lv_label_set_text(tl, "Power off?");
    lv_obj_set_style_text_color(tl, UI_COLOR_TEXT, 0);
    lv_obj_align(tl, LV_ALIGN_TOP_MID, 0, 16);

    lv_obj_t *ok = lv_button_create(box);
    lv_obj_set_size(ok, 72, 36);
    lv_obj_align(ok, LV_ALIGN_BOTTOM_LEFT, 16, -14);
    lv_obj_set_style_bg_color(ok, UI_COLOR_WARN, 0);
    lv_obj_add_event_cb(ok, shutdown_confirm_cb, LV_EVENT_CLICKED, box);
    lv_obj_t *ol = lv_label_create(ok);
    lv_label_set_text(ol, "Off");
    lv_obj_set_style_text_color(ol, UI_COLOR_TEXT, 0);
    lv_obj_center(ol);

    lv_obj_t *cancel = lv_button_create(box);
    lv_obj_set_size(cancel, 72, 36);
    lv_obj_align(cancel, LV_ALIGN_BOTTOM_RIGHT, -16, -14);
    lv_obj_add_event_cb(cancel, dismiss_modal_cb, LV_EVENT_CLICKED, box);
    lv_obj_t *cl = lv_label_create(cancel);
    lv_label_set_text(cl, "Cancel");
    lv_obj_set_style_text_color(cl, UI_COLOR_TEXT, 0);
    lv_obj_center(cl);
}

static void dismiss_modal_cb(lv_event_t *e)
{
    lv_obj_t *box = (lv_obj_t *)lv_event_get_user_data(e);
    lv_obj_delete(box);
}

static void shutdown_confirm_cb(lv_event_t *e)
{
    (void)e;
    bsp_power_shutdown();
}

static lv_obj_t *add_label_row(lv_obj_t *parent, const char *text)
{
    lv_obj_t *lb = lv_label_create(parent);
    lv_label_set_text(lb, text);
    lv_obj_set_style_text_font(lb, UI_FONT_SM, 0);
    lv_obj_set_style_text_color(lb, UI_COLOR_MUTED, 0);
    return lb;
}

lv_obj_t *feature_settings_create_screen(void)
{
    s_scr = lv_obj_create(NULL);
    ui_theme_apply_screen(s_scr);
    ui_shell_add_back_button(s_scr, "Settings");

    lv_obj_t *panel = lv_obj_create(s_scr);
    lv_obj_set_size(panel, UI_SAFE_W, UI_SAFE_H - 50);
    lv_obj_align(panel, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_pad_all(panel, 8, 0);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(panel, 6, 0);
    lv_obj_set_scrollbar_mode(panel, LV_SCROLLBAR_MODE_AUTO);

    add_label_row(panel, "Backlight");
    s_bl_slider = lv_slider_create(panel);
    lv_slider_set_range(s_bl_slider, 5, 100);
    lv_slider_set_value(s_bl_slider, 100, LV_ANIM_OFF);
    lv_obj_set_width(s_bl_slider, 200);
    lv_obj_add_event_cb(s_bl_slider, on_bl_changed, LV_EVENT_VALUE_CHANGED, NULL);

    add_label_row(panel, "WiFi");
    s_wifi_status = lv_label_create(panel);
    lv_obj_set_width(s_wifi_status, 220);
    lv_label_set_long_mode(s_wifi_status, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_font(s_wifi_status, UI_FONT_SM, 0);
    lv_obj_set_style_text_color(s_wifi_status, UI_COLOR_TEXT, 0);

    lv_obj_t *scan_btn = lv_button_create(panel);
    lv_obj_set_style_bg_color(scan_btn, UI_COLOR_ACCENT, 0);
    lv_obj_add_event_cb(scan_btn, on_scan_clicked, LV_EVENT_CLICKED, NULL);
    s_scan_btn_lb = lv_label_create(scan_btn);
    lv_label_set_text(s_scan_btn_lb, "Scan WiFi");
    lv_obj_set_style_text_color(s_scan_btn_lb, UI_COLOR_TEXT, 0);

    s_wifi_list = lv_list_create(panel);
    lv_obj_set_size(s_wifi_list, 220, 100);
    lv_list_add_text(s_wifi_list, "Tap Scan WiFi");

    add_label_row(panel, "Amap API Key");
    s_key_ta = lv_textarea_create(panel);
    lv_textarea_set_one_line(s_key_ta, true);
    lv_obj_set_width(s_key_ta, 200);
    lv_textarea_set_placeholder_text(s_key_ta, "Enter Key");
    lv_textarea_set_max_length(s_key_ta, 48);
    if (weather_service_get_api_key()[0]) {
        lv_textarea_set_text(s_key_ta, weather_service_get_api_key());
    }

    lv_obj_t *key_btn = lv_button_create(panel);
    lv_obj_set_style_bg_color(key_btn, UI_COLOR_ACCENT, 0);
    lv_obj_add_event_cb(key_btn, on_key_save, LV_EVENT_CLICKED, NULL);
    lv_obj_t *kbl = lv_label_create(key_btn);
    lv_label_set_text(kbl, "Save Key");
    lv_obj_set_style_text_color(kbl, UI_COLOR_TEXT, 0);

    s_info_lb = lv_label_create(panel);
    lv_obj_set_width(s_info_lb, 220);
    lv_obj_set_style_text_font(s_info_lb, UI_FONT_SM, 0);
    lv_obj_set_style_text_color(s_info_lb, UI_COLOR_TEXT, 0);

    lv_obj_t *pw_btn = lv_button_create(panel);
    lv_obj_set_style_bg_color(pw_btn, UI_COLOR_WARN, 0);
    lv_obj_add_event_cb(pw_btn, on_shutdown, LV_EVENT_CLICKED, NULL);
    lv_obj_t *pbl = lv_label_create(pw_btn);
    lv_label_set_text(pbl, "Soft Power Off");
    lv_obj_set_style_text_color(pbl, UI_COLOR_TEXT, 0);

    refresh_wifi_status();
    refresh_info();
    s_poll = lv_timer_create(poll_cb, 400, NULL);

    /* Auto-scan once when opening settings */
    wifi_service_scan_start();

    return s_scr;
}

void feature_settings_destroy(void)
{
    close_wifi_modal();
    if (s_poll) {
        lv_timer_delete(s_poll);
        s_poll = NULL;
    }
    if (s_scr) {
        lv_obj_delete(s_scr);
        s_scr = NULL;
    }
    s_wifi_list = NULL;
    s_wifi_status = NULL;
    s_scan_btn_lb = NULL;
    s_info_lb = NULL;
    s_key_ta = NULL;
}
