/*
 * 08_album — 轮播 /sdcard/Photos 或 /sdcard/DCIM 下的 jpg/png
 *
 * 验证：串口 TAG=album 出现 found N image(s)；N>0 时全屏出图，约 3 秒切换，
 *       底部 1 / N。N=0 则提示 put jpg/png in /Photos。
 * 准备：FAT32 卡；图片不要过大（边长几百~一千像素较稳妥）。
 *
 * Author : Ayang
 * Company: SHENZHEN VIEWE TECHNOLOGY CO.,LTD
 */
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "bsp/smartring_plus.h"
#include "esp_log.h"
#include "esp_lv_adapter.h"
#include "esp_lv_decoder.h"
#include "src/draw/lv_image_decoder_private.h"

static const char *TAG = "album";
#define MAX_PHOTOS 32

static char s_paths[MAX_PHOTOS][128];
static int s_count;
static int s_index;
static lv_obj_t *s_img;
static lv_obj_t *s_hint;
static lv_draw_buf_t *s_draw_buf;
static lv_image_dsc_t s_img_dsc;
static esp_lv_decoder_handle_t s_decoder;

static bool is_image(const char *name)
{
    size_t n = strlen(name);
    if (n < 5) {
        return false;
    }
    const char *ext = name + n - 4;
    return strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".png") == 0 ||
           (n >= 5 && strcasecmp(name + n - 5, ".jpeg") == 0);
}

static void scan_dir(const char *dir)
{
    DIR *d = opendir(dir);
    if (!d) {
        return;
    }
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL && s_count < MAX_PHOTOS) {
        if (!is_image(ent->d_name)) {
            continue;
        }
        char full[512];
        int n = snprintf(full, sizeof(full), "%s/%s", dir, ent->d_name);
        if (n < 0 || (size_t)n >= sizeof(s_paths[0])) {
            continue;
        }
        struct stat st;
        if (stat(full, &st) != 0 || !S_ISREG(st.st_mode)) {
            continue;
        }
        strlcpy(s_paths[s_count], full, sizeof(s_paths[s_count]));
        s_count++;
    }
    closedir(d);
}

static void free_draw_buf(void)
{
    if (s_draw_buf) {
        lv_draw_buf_destroy(s_draw_buf);
        s_draw_buf = NULL;
    }
    memset(&s_img_dsc, 0, sizeof(s_img_dsc));
}

static bool load_image_to_buf(const char *lvgl_src)
{
    lv_image_decoder_dsc_t dec;
    memset(&dec, 0, sizeof(dec));
    lv_image_decoder_args_t args = { .no_cache = true, .stride_align = true };
    if (lv_image_decoder_open(&dec, lvgl_src, &args) != LV_RESULT_OK) {
        ESP_LOGE(TAG, "decoder open failed: %s", lvgl_src);
        return false;
    }
    lv_draw_buf_t *decoded = (lv_draw_buf_t *)dec.decoded;
    if (!decoded) {
        lv_image_decoder_close(&dec);
        return false;
    }
    lv_draw_buf_t *dup = lv_draw_buf_dup(decoded);
    lv_image_decoder_close(&dec);
    if (!dup) {
        return false;
    }
    free_draw_buf();
    s_draw_buf = dup;
    lv_draw_buf_to_image(s_draw_buf, &s_img_dsc);
    return true;
}

static void show_index(int idx)
{
    if (s_count <= 0) {
        return;
    }
    if (idx < 0) {
        idx = s_count - 1;
    }
    if (idx >= s_count) {
        idx = 0;
    }
    s_index = idx;

    const char *full = s_paths[s_index];
    const char *rel = full + strlen(BOARD_SD_MOUNT_POINT);
    if (rel[0] == '/') {
        rel++;
    }
    /* LVGL 文件源用 S: 前缀，路径相对挂载点（例如 S:Photos/a.jpg） */
    char src[160];
    snprintf(src, sizeof(src), "S:%s", rel);

    if (!load_image_to_buf(src)) {
        return;
    }
    lv_image_set_src(s_img, &s_img_dsc);
    if (s_hint) {
        char buf[48];
        snprintf(buf, sizeof(buf), "%d / %d", s_index + 1, s_count);
        lv_label_set_text(s_hint, buf);
    }
    ESP_LOGI(TAG, "show %s", src);
}

static void slide_cb(lv_timer_t *t)
{
    (void)t;
    if (s_count > 1) {
        show_index(s_index + 1);
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(bsp_display_init());
    if (bsp_sd_init() != ESP_OK) {
        ESP_LOGW(TAG, "SD not mounted");
    }
    ESP_ERROR_CHECK(esp_lv_decoder_init(&s_decoder)); /* 注册 JPG/PNG 解码器，才能用 S: 路径 */

    scan_dir(BOARD_SD_MOUNT_POINT "/Photos");
    scan_dir(BOARD_SD_MOUNT_POINT "/DCIM");
    ESP_LOGI(TAG, "found %d image(s)", s_count);

    ESP_ERROR_CHECK(esp_lv_adapter_lock(-1));
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), 0);

    s_img = lv_image_create(scr);
    lv_obj_set_size(s_img, BOARD_LCD_H_RES, BOARD_LCD_V_RES);
    lv_obj_center(s_img);
    lv_image_set_inner_align(s_img, LV_IMAGE_ALIGN_COVER);

    s_hint = lv_label_create(scr);
    lv_obj_set_style_text_color(s_hint, lv_color_hex(0xE8EEF4), 0);
    lv_obj_align(s_hint, LV_ALIGN_BOTTOM_MID, 0, -24);

    if (s_count <= 0) {
        lv_label_set_text(s_hint, "put jpg/png in /Photos");
        lv_obj_center(s_hint);
    } else {
        show_index(0);
        lv_timer_create(slide_cb, 3000, NULL); /* 约 3 秒一张 */
    }
    esp_lv_adapter_unlock();
}
