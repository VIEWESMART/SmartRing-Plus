/*
 * 相册：全屏轮播 /sdcard/Photos 或 /sdcard/DCIM 下的 jpg/png/jpeg
 * - 无按钮；上滑退出（异步，避免在事件里删屏导致 UI 残留）
 * - 先完整解码到内存再显示，减轻边解边刷的撕裂感
 */
#include "feature_album.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

#include "bsp/board_config.h"
#include "esp_lv_adapter.h"
#include "esp_lv_decoder.h"
#include "esp_log.h"
#include "src/draw/lv_image_decoder_private.h"
#include "ui_shell.h"
#include "ui_theme.h"

static const char *TAG = "feat_album";

#define MAX_PHOTOS          64
#define SWIPE_UP_THRESHOLD  70

static lv_obj_t *s_scr;
static lv_obj_t *s_img;
static lv_timer_t *s_slide_timer;
static esp_lv_decoder_handle_t s_decoder;

static char s_paths[MAX_PHOTOS][128];
static int s_count;
static int s_index;

static lv_draw_buf_t *s_draw_buf;
static lv_image_dsc_t s_img_dsc;

static lv_point_t s_press_pt;
static bool s_pressing;
static bool s_exit_pending;

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
    if (s_count >= MAX_PHOTOS) {
        return;
    }
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

static void build_catalog(void)
{
    s_count = 0;
    scan_dir(BOARD_SD_MOUNT_POINT "/Photos");
    scan_dir(BOARD_SD_MOUNT_POINT "/DCIM");
}

static void free_draw_buf(void)
{
    if (s_draw_buf) {
        lv_draw_buf_destroy(s_draw_buf);
        s_draw_buf = NULL;
    }
    memset(&s_img_dsc, 0, sizeof(s_img_dsc));
}

/**
 * 将文件完整解码到独立 draw_buf（关闭 decoder 后仍可用），再作为变量图源显示。
 */
static bool load_image_to_buf(const char *lvgl_src)
{
    lv_image_decoder_dsc_t dec;
    memset(&dec, 0, sizeof(dec));

    lv_image_decoder_args_t args = { 0 };
    args.no_cache = true;
    args.stride_align = true;

    if (lv_image_decoder_open(&dec, lvgl_src, &args) != LV_RESULT_OK) {
        ESP_LOGE(TAG, "decoder open failed: %s", lvgl_src);
        return false;
    }

    lv_draw_buf_t *decoded = (lv_draw_buf_t *)dec.decoded;
    if (!decoded) {
        lv_image_header_t *h = &dec.header;
        if (h->w == 0 || h->h == 0) {
            lv_image_decoder_close(&dec);
            ESP_LOGE(TAG, "invalid image header");
            return false;
        }
        decoded = lv_draw_buf_create(h->w, h->h,
                                     h->cf ? h->cf : LV_COLOR_FORMAT_RGB565,
                                     LV_STRIDE_AUTO);
        if (!decoded) {
            lv_image_decoder_close(&dec);
            ESP_LOGE(TAG, "alloc draw_buf failed");
            return false;
        }
        lv_area_t full = { 0, 0, (int32_t)h->w - 1, (int32_t)h->h - 1 };
        lv_area_t decoded_area = {
            .x1 = LV_COORD_MIN, .y1 = LV_COORD_MIN,
            .x2 = LV_COORD_MIN, .y2 = LV_COORD_MIN,
        };
        while (lv_image_decoder_get_area(&dec, &full, &decoded_area) == LV_RESULT_OK) {
            if (dec.decoded && dec.decoded->data) {
                const lv_draw_buf_t *chunk = dec.decoded;
                uint32_t px = lv_color_format_get_size(decoded->header.cf);
                for (int32_t y = decoded_area.y1; y <= decoded_area.y2; y++) {
                    uint8_t *dst = (uint8_t *)decoded->data +
                                   (size_t)y * decoded->header.stride +
                                   (size_t)decoded_area.x1 * px;
                    const uint8_t *srcp = (const uint8_t *)chunk->data +
                                          (size_t)(y - decoded_area.y1) * chunk->header.stride;
                    memcpy(dst, srcp, (size_t)(decoded_area.x2 - decoded_area.x1 + 1) * px);
                }
            }
        }
        lv_image_decoder_close(&dec);
        free_draw_buf();
        s_draw_buf = decoded;
    } else {
        lv_draw_buf_t *dup = lv_draw_buf_dup(decoded);
        lv_image_decoder_close(&dec);
        if (!dup) {
            ESP_LOGE(TAG, "dup draw_buf failed");
            return false;
        }
        free_draw_buf();
        s_draw_buf = dup;
    }

    lv_draw_buf_to_image(s_draw_buf, &s_img_dsc);
    return true;
}

static void show_index(int idx)
{
    if (!s_img || s_exit_pending) {
        return;
    }
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
    char src[160];
    snprintf(src, sizeof(src), "S:%s", rel);

    /* 先隐藏，解码完成后再显示，避免边解边刷 */
    lv_obj_add_flag(s_img, LV_OBJ_FLAG_HIDDEN);

    if (!load_image_to_buf(src)) {
        ESP_LOGW(TAG, "skip broken image: %s", src);
        lv_obj_remove_flag(s_img, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_image_set_src(s_img, &s_img_dsc);
    lv_obj_remove_flag(s_img, LV_OBJ_FLAG_HIDDEN);
    lv_obj_invalidate(s_scr);
    ESP_LOGI(TAG, "show [%d/%d] %s (%ux%u)", s_index + 1, s_count, src,
             (unsigned)s_img_dsc.header.w, (unsigned)s_img_dsc.header.h);
}

static void slide_cb(lv_timer_t *t)
{
    (void)t;
    if (!s_exit_pending && s_count > 1) {
        show_index(s_index + 1);
    }
}

static void exit_album_async(void *user_data)
{
    (void)user_data;
    if (!s_exit_pending) {
        return;
    }
    /* 事件已结束，此时再切主页并销毁相册，避免删当前事件对象造成残留 */
    ui_shell_show_home();
}

static void on_touch(lv_event_t *e)
{
    if (s_exit_pending) {
        return;
    }
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_indev_active();
    if (!indev) {
        return;
    }

    if (code == LV_EVENT_PRESSED) {
        lv_indev_get_point(indev, &s_press_pt);
        s_pressing = true;
    } else if (code == LV_EVENT_PRESSING) {
        if (s_pressing && s_slide_timer) {
            lv_point_t now;
            lv_indev_get_point(indev, &now);
            if ((s_press_pt.y - now.y) > 30) {
                lv_timer_pause(s_slide_timer);
            }
        }
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        if (!s_pressing) {
            return;
        }
        s_pressing = false;
        lv_point_t now;
        lv_indev_get_point(indev, &now);
        int dy = (int)now.y - (int)s_press_pt.y;
        int dx = (int)now.x - (int)s_press_pt.x;
        if (dy < -SWIPE_UP_THRESHOLD && abs(dy) > abs(dx)) {
            ESP_LOGI(TAG, "swipe up -> exit album");
            s_exit_pending = true;
            if (s_slide_timer) {
                lv_timer_pause(s_slide_timer);
            }
            lv_async_call(exit_album_async, NULL);
            return;
        }
        if (s_slide_timer) {
            lv_timer_resume(s_slide_timer);
            lv_timer_reset(s_slide_timer);
        }
    }
}

esp_err_t feature_album_decoder_init(void)
{
    if (s_decoder) {
        return ESP_OK;
    }
    esp_err_t err = esp_lv_decoder_init(&s_decoder);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_lv_decoder_init 失败");
    }
    return err;
}

lv_obj_t *feature_album_create_screen(void)
{
    feature_album_decoder_init();
    build_catalog();
    s_exit_pending = false;
    s_pressing = false;

    s_scr = lv_obj_create(NULL);
    ui_theme_apply_screen(s_scr);
    lv_obj_add_flag(s_scr, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_scr, on_touch, LV_EVENT_ALL, NULL);

    s_img = lv_image_create(s_scr);
    lv_obj_set_size(s_img, BOARD_LCD_H_RES, BOARD_LCD_V_RES);
    lv_obj_align(s_img, LV_ALIGN_CENTER, 0, 0);
    lv_image_set_inner_align(s_img, LV_IMAGE_ALIGN_COVER);
    lv_obj_remove_flag(s_img, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(s_img, LV_OBJ_FLAG_SCROLLABLE);

    if (s_count <= 0) {
        lv_obj_t *hint = lv_label_create(s_scr);
        lv_label_set_text(hint, "No photos\nSwipe up to exit");
        lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(hint, UI_COLOR_MUTED, 0);
        lv_obj_center(hint);
    } else {
        show_index(0);
        s_slide_timer = lv_timer_create(slide_cb, 3000, NULL);
    }
    return s_scr;
}

void feature_album_destroy(void)
{
    lv_async_call_cancel(exit_album_async, NULL);
    s_exit_pending = false;
    s_pressing = false;

    if (s_slide_timer) {
        lv_timer_delete(s_slide_timer);
        s_slide_timer = NULL;
    }
    if (s_img) {
        lv_image_set_src(s_img, NULL);
        s_img = NULL;
    }
    free_draw_buf();
    if (s_scr) {
        lv_obj_delete(s_scr);
        s_scr = NULL;
    }
}
