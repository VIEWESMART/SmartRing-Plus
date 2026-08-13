/*
 * 02_battery — 电池电压 / 电量 / 充电状态
 *
 * 目的：演示 BSP 运行时区分 V1（GPIO1 ADC）与 V2（AXP2101 I2C 0x34）。
 * 验证：串口 TAG=battery，出现 PMIC=V1 ADC 或 PMIC=V2 AXP2101，以及
 *       bat: x.xxV nn% charging|on-battery；屏上标题 Battery。
 * 本例不含软关机（插 USB 时 GPIO47 无法掉电）。
 *
 * Author : Ayang
 * Company: SHENZHEN VIEWE TECHNOLOGY CO.,LTD
 */
#include <stdio.h>

#include "bsp/smartring_plus.h"
#include "esp_log.h"
#include "esp_lv_adapter.h"

static const char *TAG = "battery";

static lv_obj_t *s_pmic_lb;
static lv_obj_t *s_volt_lb;
static lv_obj_t *s_pct_lb;
static lv_obj_t *s_chg_lb;

static const char *pmic_name(bsp_pmic_type_t t)
{
    switch (t) {
    case BSP_PMIC_V2_AXP2101: return "V2 AXP2101";
    case BSP_PMIC_V1_ADC:     return "V1 ADC";
    default:                  return "unknown";
    }
}

/* 从 BSP 读一次电池数据并刷新标签；充电时文字变绿 */
static void refresh_ui(void)
{
    bsp_battery_data_t bat;
    bsp_battery_get_data(&bat);

    char buf[48];
    snprintf(buf, sizeof(buf), "%.2f V", (double)bat.voltage_v);
    lv_label_set_text(s_volt_lb, buf);

    snprintf(buf, sizeof(buf), "%d %%", bat.percent);
    lv_label_set_text(s_pct_lb, buf);

    lv_label_set_text(s_chg_lb, bat.charging ? "charging / USB" : "on battery");
    lv_obj_set_style_text_color(s_chg_lb,
                                bat.charging ? lv_color_hex(0x3DDC84)
                                             : lv_color_hex(0x8A94A0), 0);

    ESP_LOGI(TAG, "bat: %.2fV %d%% %s",
             (double)bat.voltage_v, bat.percent,
             bat.charging ? "charging" : "on-battery");
}

static void on_tick(lv_timer_t *t)
{
    (void)t;
    refresh_ui();
}

void app_main(void)
{
    /* V2 的 AXP2101 走共享 I2C，必须先初始化显示（内部会建 I2C） */
    ESP_ERROR_CHECK(bsp_display_init());
    ESP_ERROR_CHECK(bsp_battery_init());

    bsp_pmic_type_t pmic = bsp_battery_get_pmic_type();
    ESP_LOGI(TAG, "PMIC=%s  cell=%.1fV/%dmAh",
             pmic_name(pmic), (double)BOARD_BAT_NOMINAL_V, BOARD_BAT_CAPACITY_MAH);

    ESP_ERROR_CHECK(esp_lv_adapter_lock(-1));
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), 0);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Battery");
    lv_obj_set_style_text_color(title, lv_color_hex(0x4DA3FF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);

    s_pmic_lb = lv_label_create(scr);
    lv_label_set_text(s_pmic_lb, pmic_name(pmic));
    lv_obj_set_style_text_color(s_pmic_lb, lv_color_hex(0x8A94A0), 0);
    lv_obj_align(s_pmic_lb, LV_ALIGN_TOP_MID, 0, 72);

    s_volt_lb = lv_label_create(scr);
    lv_obj_set_style_text_font(s_volt_lb, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_volt_lb, lv_color_hex(0xE8EEF4), 0);
    lv_obj_align(s_volt_lb, LV_ALIGN_CENTER, 0, -20);

    s_pct_lb = lv_label_create(scr);
    lv_obj_set_style_text_font(s_pct_lb, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_pct_lb, lv_color_hex(0xE8EEF4), 0);
    lv_obj_align(s_pct_lb, LV_ALIGN_CENTER, 0, 16);

    s_chg_lb = lv_label_create(scr);
    lv_obj_align(s_chg_lb, LV_ALIGN_CENTER, 0, 52);

    refresh_ui();
    lv_timer_create(on_tick, 1000, NULL); /* 约 1 秒刷新一次，便于看插拔 USB */
    esp_lv_adapter_unlock();
}
