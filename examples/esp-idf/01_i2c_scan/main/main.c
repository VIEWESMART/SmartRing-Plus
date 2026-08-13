/*
 * 01_i2c_scan — 扫描 SmartRing-Plus 共享 I2C 总线上的 7 位地址
 *
 * 目的：确认触摸 / codec / IMU / V2 PMIC 是否挂在总线上。
 * 验证：串口 TAG=i2c_scan，应看到 found 0x15（CST816）等；屏上标题为 I2C Scan。
 *
 * 要点：必须先 bsp_display_init()，它会创建共享 I2C（SDA=GPIO8, SCL=GPIO9）。
 *       改 LVGL 控件前要 esp_lv_adapter_lock(-1)，结束后 unlock。
 *
 * Author : Ayang
 * Company: SHENZHEN VIEWE TECHNOLOGY CO.,LTD
 */
#include <stdio.h>
#include <string.h>

#include "bsp/smartring_plus.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_lv_adapter.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "i2c_scan";

/* 板上已知外设地址，仅用于日志/界面提示，扫描本身不依赖这份表 */
static const char *known_dev(uint8_t addr)
{
    switch (addr) {
    case 0x15: return "CST816 touch";
    case 0x18: return "ES8311 codec";
    case 0x34: return "AXP2101 PMIC (V2)";
    case 0x6B: return "QMI8658A IMU";
    default:   return NULL;
    }
}

void app_main(void)
{
    /* 创建 I2C + 屏 + 触摸 + LVGL；后续扫描用同一条总线 */
    ESP_ERROR_CHECK(bsp_display_init());

    i2c_master_bus_handle_t bus = bsp_i2c_get_bus();
    if (bus == NULL) {
        ESP_LOGE(TAG, "I2C bus is NULL");
        return;
    }

    ESP_LOGI(TAG, "scan I2C (SDA=%d SCL=%d 400kHz) ...",
             BOARD_I2C_SDA_IO, BOARD_I2C_SCL_IO);

    uint8_t found[16];
    int n_found = 0;
    /* 保留地址 0x00~0x07 / 0x78~0x7F 不扫；probe 有 ACK 即认为该地址有设备 */
    for (uint8_t addr = 0x08; addr < 0x78 && n_found < 16; addr++) {
        if (i2c_master_probe(bus, addr, 50) == ESP_OK) {
            const char *name = known_dev(addr);
            ESP_LOGI(TAG, "  found 0x%02X%s%s", addr,
                     name ? "  " : "", name ? name : "");
            found[n_found++] = addr;
        }
    }
    ESP_LOGI(TAG, "done, %d device(s)", n_found);

    /* LVGL 不线程安全：本任务改界面必须加锁 */
    ESP_ERROR_CHECK(esp_lv_adapter_lock(-1));
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), 0);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "I2C Scan");
    lv_obj_set_style_text_color(title, lv_color_hex(0x4DA3FF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 36);

    lv_obj_t *body = lv_label_create(scr);
    lv_obj_set_style_text_color(body, lv_color_hex(0xE8EEF4), 0);
    lv_obj_set_style_text_font(body, &lv_font_montserrat_14, 0);
    lv_obj_set_width(body, 260);
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(body, LV_TEXT_ALIGN_CENTER, 0);

    char buf[384];
    int off = snprintf(buf, sizeof(buf), "%d device(s)\n\n", n_found);
    for (int i = 0; i < n_found && off < (int)sizeof(buf) - 48; i++) {
        const char *name = known_dev(found[i]);
        off += snprintf(buf + off, sizeof(buf) - (size_t)off, "0x%02X  %s\n",
                        found[i], name ? name : "unknown");
    }
    if (n_found == 0) {
        snprintf(buf, sizeof(buf), "no ACK on bus");
    }
    lv_label_set_text(body, buf);
    lv_obj_align(body, LV_ALIGN_CENTER, 0, 16);
    esp_lv_adapter_unlock();
}
