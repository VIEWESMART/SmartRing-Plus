/*
 * 04_lvgl_port — 验证显示 + 触摸 + LVGL 适配链路
 *
 * 目的：bsp_display_init() 成功后运行官方 lv_demo_widgets()。
 * 验证：串口 TAG=lvgl_port 出现 start lv_demo_widgets()；屏上是官方 Demo
 *       （不是整机主页），触摸可点标签/滑块。
 * 圆屏 360×360 上官方 Demo 边缘可能被裁切，属正常现象。
 *
 * Author : Ayang
 * Company: SHENZHEN VIEWE TECHNOLOGY CO.,LTD
 */
#include "bsp/smartring_plus.h"
#include "esp_log.h"
#include "esp_lv_adapter.h"
#include "lv_demos.h"

static const char *TAG = "lvgl_port";

void app_main(void)
{
    ESP_ERROR_CHECK(bsp_display_init());
    ESP_LOGI(TAG, "display ready, start lv_demo_widgets()");

    /* Demo 会创建大量控件，必须在锁内调用 */
    ESP_ERROR_CHECK(esp_lv_adapter_lock(-1));
    lv_demo_widgets();
    esp_lv_adapter_unlock();
}
