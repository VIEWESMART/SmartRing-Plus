/*
 * SmartRing_Plus Minimal Version
 * Only includes display driver, touch driver and LVGL runtime
 * Uses LVGL official example lv_demo_widgets()
 *
 * @file main.c
 * @author Ayang
 * @date 2026-01
 */

#include <stdio.h>
#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bsp/smartring_plus.h"
#include "lvgl.h"

#if LV_USE_DEMO_WIDGETS
#include "demos/lv_demos.h"
#endif

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "SmartRing_Plus 极简版本启动");

    /* 初始化显示和LVGL */
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = BSP_LCD_H_RES * CONFIG_BSP_LCD_DRAW_BUF_HEIGHT,
        .double_buffer = 0,
        .flags = {
            .buff_dma = true,
        }
    };
    bsp_display_start_with_config(&cfg);

    /* 打开背光 */
    bsp_display_backlight_on();

    /* 运行LVGL官方示例 */
    bsp_display_lock(0);
    
#if LV_USE_DEMO_WIDGETS
    /* 运行LVGL widgets演示 */
    lv_demo_widgets();
    ESP_LOGI(TAG, "LVGL widgets demo started");
#else
    /* 如果demo未启用，显示简单提示 */
    lv_obj_t * label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "LVGL Demo Widgets\nnot enabled\n\nPlease enable\nLV_USE_DEMO_WIDGETS\nin menuconfig");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    ESP_LOGW(TAG, "LV_USE_DEMO_WIDGETS is not enabled, please enable it in menuconfig");
#endif
    
    bsp_display_unlock();

    ESP_LOGI(TAG, "初始化完成，LVGL运行中...");

    /* 主循环 */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}