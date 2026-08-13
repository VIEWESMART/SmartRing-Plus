/*
 * IMU 功能页：水平仪气泡 + 加速度/倾斜读数 + 水平校准
 *
 * Author : Ayang
 * Company: SHENZHEN VIEWE TECHNOLOGY CO.,LTD
 */
#include "feature_imu.h"

#include <math.h>
#include <stdio.h>

#include "bsp/smartring_plus.h"
#include "esp_log.h"
#include "ui_shell.h"
#include "ui_theme.h"

static const char *TAG = "feat_imu";

#define LEVEL_SIZE      140
#define BUBBLE_SIZE     18
#define TILT_SCALE      12.0f   /* m/s² → 像素，约 ±1.2g 铺满半径 */
#define LEVEL_RADIUS    ((LEVEL_SIZE - BUBBLE_SIZE) / 2)

static lv_obj_t *s_scr;
static lv_obj_t *s_level;
static lv_obj_t *s_bubble;
static lv_obj_t *s_accel_lb;
static lv_obj_t *s_tilt_lb;
static lv_obj_t *s_status_lb;
static lv_obj_t *s_calib_btn_lb;
static lv_timer_t *s_timer;
static bool s_ready;

static const char *calib_text(bsp_imu_calib_status_t st)
{
    switch (st) {
    case BSP_IMU_CALIB_RUNNING: return "Calibrating... keep flat";
    case BSP_IMU_CALIB_PASSED:  return "Calibrated OK";
    case BSP_IMU_CALIB_FAILED:  return "Calib failed - retry";
    case BSP_IMU_CALIB_IDLE:
    default:                    return "Not calibrated";
    }
}

static void place_bubble(float gx, float gy)
{
    if (!s_bubble || !s_level) {
        return;
    }
    float px = gx * TILT_SCALE;
    float py = gy * TILT_SCALE;
    float r2 = (float)(LEVEL_RADIUS * LEVEL_RADIUS);
    float d2 = px * px + py * py;
    if (d2 > r2 && d2 > 0.001f) {
        float s = LEVEL_RADIUS / sqrtf(d2);
        px *= s;
        py *= s;
    }
    lv_obj_set_pos(s_bubble,
                   (LEVEL_SIZE - BUBBLE_SIZE) / 2 + (int)px,
                   (LEVEL_SIZE - BUBBLE_SIZE) / 2 + (int)py);
}

static void tick_cb(lv_timer_t *t)
{
    (void)t;
    if (!s_ready) {
        return;
    }

    float ax, ay, az, gx, gy;
    bsp_imu_get_accel(&ax, &ay, &az);
    bsp_imu_get_tilt(&gx, &gy);
    place_bubble(gx, gy);

    char abuf[64];
    snprintf(abuf, sizeof(abuf), "A  %.2f  %.2f  %.2f", ax, ay, az);
    lv_label_set_text(s_accel_lb, abuf);

    char tbuf[48];
    snprintf(tbuf, sizeof(tbuf), "Tilt  %.2f  %.2f", gx, gy);
    lv_label_set_text(s_tilt_lb, tbuf);

    bsp_imu_calib_status_t st = bsp_imu_calib_poll();
    if (st == BSP_IMU_CALIB_RUNNING) {
        float std_g = 0, tilt_deg = 0;
        uint32_t elapsed = 0;
        bsp_imu_calib_get_info(&std_g, &tilt_deg, &elapsed);
        char sbuf[72];
        snprintf(sbuf, sizeof(sbuf), "Calib %lus  std %.3fg  %.1fdeg",
                 (unsigned long)(elapsed / 1000U), std_g, tilt_deg);
        lv_label_set_text(s_status_lb, sbuf);
        lv_obj_set_style_text_color(s_status_lb, UI_COLOR_ACCENT, 0);
    } else {
        lv_label_set_text(s_status_lb, calib_text(st));
        if (st == BSP_IMU_CALIB_PASSED) {
            lv_obj_set_style_text_color(s_status_lb, UI_COLOR_OK, 0);
        } else if (st == BSP_IMU_CALIB_FAILED) {
            lv_obj_set_style_text_color(s_status_lb, UI_COLOR_WARN, 0);
        } else {
            lv_obj_set_style_text_color(s_status_lb, UI_COLOR_MUTED, 0);
        }
    }
}

static void on_calib_clicked(lv_event_t *e)
{
    (void)e;
    ESP_LOGI(TAG, "start IMU level calibration");
    bsp_imu_calib_start();
    if (s_calib_btn_lb) {
        lv_label_set_text(s_calib_btn_lb, "Calibrating...");
    }
}

lv_obj_t *feature_imu_create_screen(void)
{
    s_ready = false;
    s_scr = lv_obj_create(NULL);
    ui_theme_apply_screen(s_scr);
    ui_shell_add_back_button(s_scr, "IMU");

    /* 水平仪外圈 */
    s_level = lv_obj_create(s_scr);
    lv_obj_set_size(s_level, LEVEL_SIZE, LEVEL_SIZE);
    lv_obj_align(s_level, LV_ALIGN_CENTER, 0, -28);
    lv_obj_set_style_radius(s_level, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_level, UI_COLOR_SURFACE, 0);
    lv_obj_set_style_bg_opa(s_level, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_level, 2, 0);
    lv_obj_set_style_border_color(s_level, UI_COLOR_ACCENT, 0);
    lv_obj_set_style_pad_all(s_level, 0, 0);
    lv_obj_remove_flag(s_level, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    /* 中心十字参考点 */
    lv_obj_t *cross = lv_obj_create(s_level);
    lv_obj_set_size(cross, 8, 8);
    lv_obj_center(cross);
    lv_obj_set_style_radius(cross, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(cross, UI_COLOR_MUTED, 0);
    lv_obj_set_style_bg_opa(cross, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(cross, 0, 0);
    lv_obj_remove_flag(cross, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    /* 气泡 */
    s_bubble = lv_obj_create(s_level);
    lv_obj_set_size(s_bubble, BUBBLE_SIZE, BUBBLE_SIZE);
    lv_obj_set_style_radius(s_bubble, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_bubble, UI_COLOR_ACCENT, 0);
    lv_obj_set_style_bg_opa(s_bubble, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_bubble, 0, 0);
    lv_obj_set_style_pad_all(s_bubble, 0, 0);
    lv_obj_remove_flag(s_bubble, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    place_bubble(0, 0);

    s_accel_lb = lv_label_create(s_scr);
    lv_obj_set_style_text_font(s_accel_lb, UI_FONT_SM, 0);
    lv_obj_set_style_text_color(s_accel_lb, UI_COLOR_TEXT, 0);
    lv_label_set_text(s_accel_lb, "A  --  --  --");
    lv_obj_align(s_accel_lb, LV_ALIGN_CENTER, 0, 62);

    s_tilt_lb = lv_label_create(s_scr);
    lv_obj_set_style_text_font(s_tilt_lb, UI_FONT_SM, 0);
    lv_obj_set_style_text_color(s_tilt_lb, UI_COLOR_MUTED, 0);
    lv_label_set_text(s_tilt_lb, "Tilt  --  --");
    lv_obj_align(s_tilt_lb, LV_ALIGN_CENTER, 0, 82);

    s_status_lb = lv_label_create(s_scr);
    lv_obj_set_style_text_font(s_status_lb, UI_FONT_SM, 0);
    lv_obj_set_style_text_color(s_status_lb, UI_COLOR_MUTED, 0);
    lv_label_set_text(s_status_lb, calib_text(BSP_IMU_CALIB_IDLE));
    lv_obj_set_style_text_align(s_status_lb, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_status_lb, UI_SAFE_W - 20);
    lv_obj_align(s_status_lb, LV_ALIGN_CENTER, 0, 104);

    lv_obj_t *btn = lv_button_create(s_scr);
    lv_obj_set_size(btn, 140, 36);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -28);
    lv_obj_set_style_radius(btn, 18, 0);
    lv_obj_set_style_bg_color(btn, UI_COLOR_SURFACE_HI, 0);
    lv_obj_add_event_cb(btn, on_calib_clicked, LV_EVENT_CLICKED, NULL);
    s_calib_btn_lb = lv_label_create(btn);
    lv_label_set_text(s_calib_btn_lb, "Calibrate");
    lv_obj_set_style_text_font(s_calib_btn_lb, UI_FONT_MD, 0);
    lv_obj_center(s_calib_btn_lb);

    s_ready = true;
    s_timer = lv_timer_create(tick_cb, 100, NULL);
    tick_cb(NULL);
    return s_scr;
}

void feature_imu_destroy(void)
{
    s_ready = false;
    if (s_timer) {
        lv_timer_delete(s_timer);
        s_timer = NULL;
    }
    s_level = NULL;
    s_bubble = NULL;
    s_accel_lb = NULL;
    s_tilt_lb = NULL;
    s_status_lb = NULL;
    s_calib_btn_lb = NULL;
    if (s_scr) {
        lv_obj_delete(s_scr);
        s_scr = NULL;
    }
}
