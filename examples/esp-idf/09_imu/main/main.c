/*
 * 09_imu — QMI8658A 水平仪（气泡 + 三轴加速度）
 *
 * 验证：串口 TAG=imu；倾斜时气泡向“下滑”方向移动；底部加速度约有一轴 ±9.8。
 *       放平后点 Calibrate，约 2 秒出现 calibrated（或 calib failed 可再试）。
 * IMU 在共享 I2C 上，必须先 bsp_display_init()。
 *
 * Author : Ayang
 * Company: SHENZHEN VIEWE TECHNOLOGY CO.,LTD
 */
#include <math.h>
#include <stdio.h>

#include "bsp/smartring_plus.h"
#include "esp_log.h"
#include "esp_lv_adapter.h"

static const char *TAG = "imu";

#define LEVEL_SIZE   140
#define BUBBLE_SIZE  18
#define TILT_SCALE   12.0f
#define LEVEL_RADIUS ((LEVEL_SIZE - BUBBLE_SIZE) / 2)

static lv_obj_t *s_level;
static lv_obj_t *s_bubble;
static lv_obj_t *s_accel_lb;
static lv_obj_t *s_status_lb;
static lv_obj_t *s_calib_lb;

static const char *calib_text(bsp_imu_calib_status_t st)
{
    switch (st) {
    case BSP_IMU_CALIB_RUNNING: return "keep flat...";
    case BSP_IMU_CALIB_PASSED:  return "calibrated";
    case BSP_IMU_CALIB_FAILED:  return "calib failed";
    default:                    return "not calibrated";
    }
}

/* 把倾斜 gx/gy 映射到气泡坐标，超出圆盘则夹在圆周上 */
static void place_bubble(float gx, float gy)
{
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

static void on_tick(lv_timer_t *t)
{
    (void)t;
    float ax, ay, az, gx, gy;
    bsp_imu_get_accel(&ax, &ay, &az);
    bsp_imu_get_tilt(&gx, &gy);
    place_bubble(gx, gy);

    char buf[64];
    snprintf(buf, sizeof(buf), "%.2f  %.2f  %.2f", ax, ay, az);
    lv_label_set_text(s_accel_lb, buf);

    bsp_imu_calib_status_t st = bsp_imu_calib_poll(); /* 校准是非阻塞的，每帧轮询状态 */
    lv_label_set_text(s_status_lb, calib_text(st));
}

static void on_calib(lv_event_t *e)
{
    (void)e;
    ESP_LOGI(TAG, "start level calibration");
    bsp_imu_calib_start();
}

void app_main(void)
{
    ESP_ERROR_CHECK(bsp_display_init()); /* 先建 I2C，再初始化 IMU */
    esp_err_t imu_err = bsp_imu_init();
    if (imu_err != ESP_OK) {
        ESP_LOGE(TAG, "IMU init failed: %s", esp_err_to_name(imu_err));
    }

    ESP_ERROR_CHECK(esp_lv_adapter_lock(-1));
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), 0);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "IMU");
    lv_obj_set_style_text_color(title, lv_color_hex(0x4DA3FF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 24);

    s_level = lv_obj_create(scr);
    lv_obj_set_size(s_level, LEVEL_SIZE, LEVEL_SIZE);
    lv_obj_align(s_level, LV_ALIGN_CENTER, 0, -12);
    lv_obj_set_style_radius(s_level, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_level, lv_color_hex(0x1C2228), 0);
    lv_obj_set_style_border_width(s_level, 2, 0);
    lv_obj_set_style_border_color(s_level, lv_color_hex(0x4DA3FF), 0);
    lv_obj_set_style_pad_all(s_level, 0, 0);
    lv_obj_remove_flag(s_level, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *cross = lv_obj_create(s_level);
    lv_obj_set_size(cross, 8, 8);
    lv_obj_center(cross);
    lv_obj_set_style_radius(cross, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(cross, lv_color_hex(0x8A94A0), 0);
    lv_obj_set_style_border_width(cross, 0, 0);
    lv_obj_remove_flag(cross, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    s_bubble = lv_obj_create(s_level);
    lv_obj_set_size(s_bubble, BUBBLE_SIZE, BUBBLE_SIZE);
    lv_obj_set_style_radius(s_bubble, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_bubble, lv_color_hex(0x4DA3FF), 0);
    lv_obj_set_style_border_width(s_bubble, 0, 0);
    lv_obj_remove_flag(s_bubble, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    place_bubble(0, 0);

    s_accel_lb = lv_label_create(scr);
    lv_obj_set_style_text_color(s_accel_lb, lv_color_hex(0xE8EEF4), 0);
    lv_obj_set_style_text_font(s_accel_lb, &lv_font_montserrat_14, 0);
    lv_label_set_text(s_accel_lb, imu_err == ESP_OK ? "..." : "IMU init failed");
    lv_obj_align(s_accel_lb, LV_ALIGN_BOTTOM_MID, 0, -72);

    s_status_lb = lv_label_create(scr);
    lv_obj_set_style_text_color(s_status_lb, lv_color_hex(0x8A94A0), 0);
    lv_label_set_text(s_status_lb, calib_text(BSP_IMU_CALIB_IDLE));
    lv_obj_align(s_status_lb, LV_ALIGN_BOTTOM_MID, 0, -52);

    lv_obj_t *btn = lv_button_create(scr);
    lv_obj_set_size(btn, 140, 36);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -16);
    lv_obj_add_event_cb(btn, on_calib, LV_EVENT_CLICKED, NULL);
    s_calib_lb = lv_label_create(btn);
    lv_label_set_text(s_calib_lb, "Calibrate");
    lv_obj_center(s_calib_lb);

    if (imu_err == ESP_OK) {
        lv_timer_create(on_tick, 80, NULL); /* ~12.5 Hz 刷新气泡，足够流畅 */
    }
    esp_lv_adapter_unlock();
}
