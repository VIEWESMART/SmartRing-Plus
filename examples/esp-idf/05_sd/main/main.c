/*
 * 05_sd — 挂载 SD 卡，写入并读回 /sdcard/hello.txt
 *
 * 目的：验证 SDMMC 4-bit + FAT32。本板没有卡检测脚，没插卡与挂载失败无法区分。
 * 验证：串口 TAG=sd 出现 wrote /sdcard/hello.txt；屏上标题 SD Card 并列出根目录。
 *       把卡插到电脑应能看到 hello.txt。
 *
 * Author : Ayang
 * Company: SHENZHEN VIEWE TECHNOLOGY CO.,LTD
 */
#include <stdio.h>
#include <string.h>
#include <dirent.h>

#include "bsp/smartring_plus.h"
#include "esp_log.h"
#include "esp_lv_adapter.h"

static const char *TAG = "sd";
#define HELLO_PATH  BOARD_SD_MOUNT_POINT "/hello.txt"
#define HELLO_MSG   "Hello SmartRing-Plus!\n"

void app_main(void)
{
    ESP_ERROR_CHECK(bsp_display_init());

    char ui_msg[384];
    /* 挂载点 BOARD_SD_MOUNT_POINT 一般为 /sdcard；需 FAT32，不支持 exFAT/NTFS */
    esp_err_t err = bsp_sd_init();
    if (err != ESP_OK) {
        snprintf(ui_msg, sizeof(ui_msg),
                 "mount failed:\n%s\n\ninsert a FAT32 card",
                 esp_err_to_name(err));
        ESP_LOGE(TAG, "%s", ui_msg);
    } else {
        FILE *fp = fopen(HELLO_PATH, "w");
        if (!fp) {
            snprintf(ui_msg, sizeof(ui_msg), "fopen write failed");
            ESP_LOGE(TAG, "%s", ui_msg);
        } else {
            fputs(HELLO_MSG, fp);
            fclose(fp);
            ESP_LOGI(TAG, "wrote %s", HELLO_PATH);

            /* 立刻读回，确认写成功而不是只 fopen 没报错 */
            char readback[64] = {0};
            fp = fopen(HELLO_PATH, "r");
            if (fp) {
                size_t n = fread(readback, 1, sizeof(readback) - 1, fp);
                readback[n] = '\0';
                fclose(fp);
            }

            int off = snprintf(ui_msg, sizeof(ui_msg),
                               "wrote hello.txt\nread: %s\n\n/", BOARD_SD_MOUNT_POINT + 1);
            bsp_sd_entry_t *ents = NULL;
            size_t count = 0;
            if (bsp_sd_list(BOARD_SD_MOUNT_POINT, &ents, &count) == ESP_OK) {
                for (size_t i = 0; i < count && off < (int)sizeof(ui_msg) - 40; i++) {
                    off += snprintf(ui_msg + off, sizeof(ui_msg) - (size_t)off, "\n%s%s",
                                    ents[i].name,
                                    ents[i].type == BSP_SD_ENTRY_DIR ? "/" : "");
                }
                bsp_sd_free_list(ents, count);
            }
        }
    }

    ESP_ERROR_CHECK(esp_lv_adapter_lock(-1));
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), 0);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "SD Card");
    lv_obj_set_style_text_color(title, lv_color_hex(0x4DA3FF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 32);

    lv_obj_t *body = lv_label_create(scr);
    lv_obj_set_width(body, 260);
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(body, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(body, lv_color_hex(0xE8EEF4), 0);
    lv_obj_set_style_text_font(body, &lv_font_montserrat_14, 0);
    lv_label_set_text(body, ui_msg);
    lv_obj_align(body, LV_ALIGN_CENTER, 0, 12);
    esp_lv_adapter_unlock();
}
