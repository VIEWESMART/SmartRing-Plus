/*
 * 06_music — 播放 /sdcard/Music 下的 .mp3
 *
 * 界面：上一曲 / 暂停(播放) / 下一曲；Library 打开曲库（默认关闭）。
 * 验证：串口 TAG=music 出现 found N track(s)；点播放后 play [i] 路径，扬声器出声。
 * 准备：FAT32 卡，根目录 Music 文件夹放入至少一个 .mp3。
 *
 * ES8311 固定 48 kHz；非 48k 源在 play_task 里做简易重采样。
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
#include "esp_audio_dec_default.h"
#include "esp_audio_simple_dec.h"
#include "esp_audio_simple_dec_default.h"
#include "esp_codec_dev.h"
#include "esp_log.h"
#include "esp_lv_adapter.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "music";
#define MUSIC_DIR    BOARD_SD_MOUNT_POINT "/Music"
#define READ_SZ      4096
#define OUT_SZ_INIT  8192
#define RS_MAX_RATIO 6
#define MAX_TRACKS   64

static char *s_tracks[MAX_TRACKS];
static int s_track_count;
static int s_track_idx = -1;
static char s_play_path[192];

static volatile bool s_playing;
static volatile bool s_paused;
static volatile bool s_want_auto_next;
static volatile int s_pending_next = -1;
static volatile int s_progress;
static TaskHandle_t s_task;

static lv_obj_t *s_player;
static lv_obj_t *s_lib_panel;
static lv_obj_t *s_list;
static lv_obj_t *s_title_lb;
static lv_obj_t *s_status_lb;
static lv_obj_t *s_bar;
static lv_obj_t *s_play_icon;

static void start_track(int idx);
static void rebuild_list_ui(void);
static void update_transport_ui(void);

/* 把任意采样率 PCM 转成板载 codec 需要的 48 kHz 立体声（最近邻，够听即可） */
static void resample_to_48k_stereo(const int16_t *in, int in_samples, int in_rate, int channels,
                                   int16_t *out, int out_cap_frames, int *out_samples)
{
    if (in_rate <= 0) {
        in_rate = BOARD_AUDIO_SAMPLE_RATE;
    }
    int max_out = in_samples * BOARD_AUDIO_SAMPLE_RATE / in_rate + 8;
    if (max_out > out_cap_frames) {
        max_out = out_cap_frames;
    }
    int out_n = 0;
    for (int i = 0; i < max_out; i++) {
        int src_i = (in_rate == BOARD_AUDIO_SAMPLE_RATE) ? i : (i * in_rate / BOARD_AUDIO_SAMPLE_RATE);
        if (src_i >= in_samples) {
            break;
        }
        int16_t l, r;
        if (channels <= 1) {
            l = r = in[src_i];
        } else {
            l = in[src_i * 2];
            r = in[src_i * 2 + 1];
        }
        out[out_n * 2] = l;
        out[out_n * 2 + 1] = r;
        out_n++;
    }
    *out_samples = out_n;
}

/* 递归扫一层子目录，收集 .mp3 路径到 s_tracks[] */
static void scan_mp3(const char *dir, int depth)
{
    DIR *d = opendir(dir);
    if (!d) {
        return;
    }
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL && s_track_count < MAX_TRACKS) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", dir, ent->d_name);
        struct stat st;
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode) && depth < 1) {
            scan_mp3(path, depth + 1);
            continue;
        }
        size_t ln = strlen(ent->d_name);
        if (ln < 5 || strcasecmp(ent->d_name + ln - 4, ".mp3") != 0) {
            continue;
        }
        if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
            continue;
        }
        char *dup = strdup(path);
        if (!dup) {
            continue;
        }
        s_tracks[s_track_count++] = dup;
    }
    closedir(d);
}

static const char *track_basename(int idx)
{
    if (idx < 0 || idx >= s_track_count || !s_tracks[idx]) {
        return "No track";
    }
    const char *base = strrchr(s_tracks[idx], '/');
    return base ? base + 1 : s_tracks[idx];
}

/* 解码任务：读文件 → MP3 解码 → 重采样 → bsp_audio_write；播完可自动下一首 */
static void play_task(void *arg)
{
    const char *path = (const char *)arg;
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        ESP_LOGE(TAG, "fopen failed: %s", path);
        s_playing = false;
        s_paused = false;
        s_task = NULL;
        vTaskDelete(NULL);
        return;
    }

    static bool registered;
    if (!registered) {
        esp_audio_dec_register_default();
        esp_audio_simple_dec_register_default();
        registered = true;
    }

    esp_audio_simple_dec_cfg_t cfg = { .dec_type = ESP_AUDIO_SIMPLE_DEC_TYPE_MP3 };
    esp_audio_simple_dec_handle_t dec = NULL;
    if (esp_audio_simple_dec_open(&cfg, &dec) != ESP_AUDIO_ERR_OK) {
        ESP_LOGE(TAG, "simple_dec_open failed");
        fclose(fp);
        s_playing = false;
        s_paused = false;
        s_task = NULL;
        vTaskDelete(NULL);
        return;
    }

    fseek(fp, 0, SEEK_END);
    long total = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    uint32_t out_cap = OUT_SZ_INIT;
    uint8_t *in_buf = malloc(READ_SZ);
    uint8_t *out_buf = malloc(out_cap);
    size_t rs_cap_frames = (out_cap / sizeof(int16_t) + 8) * RS_MAX_RATIO;
    int16_t *rs_buf = malloc(rs_cap_frames * 2 * sizeof(int16_t));
    if (!in_buf || !out_buf || !rs_buf) {
        ESP_LOGE(TAG, "OOM");
        free(in_buf);
        free(out_buf);
        free(rs_buf);
        esp_audio_simple_dec_close(dec);
        fclose(fp);
        s_playing = false;
        s_paused = false;
        s_task = NULL;
        vTaskDelete(NULL);
        return;
    }

    int sample_rate = BOARD_AUDIO_SAMPLE_RATE;
    int channels = 2;
    bool finished_ok = true;
    bsp_audio_set_mute(false);

    while (s_playing) {
        while (s_playing && s_paused) {
            bsp_audio_set_mute(true);
            vTaskDelay(pdMS_TO_TICKS(40));
        }
        if (!s_playing) {
            break;
        }
        bsp_audio_set_mute(false);

        int rd = (int)fread(in_buf, 1, READ_SZ, fp);
        if (rd <= 0) {
            break;
        }
        long pos = ftell(fp);
        s_progress = total > 0 ? (int)(pos * 100 / total) : 100;

        esp_audio_simple_dec_raw_t raw = {
            .buffer = in_buf, .len = (uint32_t)rd, .eos = (rd < READ_SZ), .consumed = 0,
        };
        esp_audio_simple_dec_out_t out = { .buffer = out_buf, .len = out_cap };

        while (raw.len > 0 && s_playing) {
            while (s_playing && s_paused) {
                bsp_audio_set_mute(true);
                vTaskDelay(pdMS_TO_TICKS(40));
            }
            if (!s_playing) {
                break;
            }
            bsp_audio_set_mute(false);

            out.buffer = out_buf;
            out.len = out_cap;
            out.decoded_size = 0;
            out.needed_size = 0;
            esp_audio_err_t er = esp_audio_simple_dec_process(dec, &raw, &out);
            if (er == ESP_AUDIO_ERR_BUFF_NOT_ENOUGH) {
                uint32_t need = out.needed_size > 0 ? out.needed_size : (out_cap * 2);
                uint8_t *nb = realloc(out_buf, need);
                if (!nb) {
                    finished_ok = false;
                    s_playing = false;
                    break;
                }
                out_buf = nb;
                out_cap = need;
                continue;
            }
            if (er != ESP_AUDIO_ERR_OK || raw.consumed == 0) {
                if (er != ESP_AUDIO_ERR_OK) {
                    finished_ok = false;
                }
                break;
            }
            if (out.decoded_size > 0) {
                esp_audio_simple_dec_info_t info = {0};
                if (esp_audio_simple_dec_get_info(dec, &info) == ESP_AUDIO_ERR_OK) {
                    if (info.sample_rate > 0) {
                        sample_rate = (int)info.sample_rate;
                    }
                    if (info.channel > 0) {
                        channels = (int)info.channel;
                    }
                }
                int bps = (channels > 0) ? channels : 2;
                int in_samples = (int)out.decoded_size / (bps * (int)sizeof(int16_t));
                int out_samples = 0;
                resample_to_48k_stereo((const int16_t *)out_buf, in_samples, sample_rate, channels,
                                       rs_buf, (int)rs_cap_frames, &out_samples);
                if (out_samples > 0) {
                    if (bsp_audio_write(rs_buf, (size_t)out_samples * 2 * sizeof(int16_t)) != ESP_OK) {
                        finished_ok = false;
                        s_playing = false;
                        break;
                    }
                }
            }
            raw.len -= raw.consumed;
            raw.buffer += raw.consumed;
        }
        if (raw.eos) {
            break;
        }
    }

    bsp_audio_set_mute(true);
    free(in_buf);
    free(out_buf);
    free(rs_buf);
    esp_audio_simple_dec_close(dec);
    fclose(fp);

    bool do_next = finished_ok && s_want_auto_next && s_playing && s_track_count > 0;
    int next_idx = do_next ? (s_track_idx + 1) % s_track_count : -1;
    s_playing = false;
    s_paused = false;
    s_progress = do_next ? 0 : 100;
    s_task = NULL;
    if (do_next) {
        s_pending_next = next_idx;
    }
    ESP_LOGI(TAG, "playback finished%s", do_next ? " -> next" : "");
    vTaskDelete(NULL);
}

static void wait_task_done(void)
{
    for (int i = 0; i < 100 && s_task; i++) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

static void stop_playback(void)
{
    s_want_auto_next = false;
    s_pending_next = -1;
    s_paused = false;
    s_playing = false;
    wait_task_done();
}

/* 停掉当前解码任务后再开新任务，避免两路音频叠在一起 */
static void start_track(int idx)
{
    if (idx < 0 || idx >= s_track_count || !s_tracks[idx]) {
        return;
    }
    stop_playback();
    s_track_idx = idx;
    strlcpy(s_play_path, s_tracks[idx], sizeof(s_play_path));
    s_progress = 0;
    s_paused = false;
    s_pending_next = -1;
    s_want_auto_next = true;
    s_playing = true;
    if (s_title_lb) {
        lv_label_set_text(s_title_lb, track_basename(idx));
    }
    ESP_LOGI(TAG, "play [%d] %s", idx, s_play_path);
    xTaskCreate(play_task, "mp3", 8192, s_play_path, 10, &s_task);
    update_transport_ui();
}

static void update_transport_ui(void)
{
    if (!s_play_icon) {
        return;
    }
    lv_label_set_text(s_play_icon, (s_playing && !s_paused) ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
    if (s_title_lb && s_track_idx >= 0) {
        lv_label_set_text(s_title_lb, track_basename(s_track_idx));
    }
}

/* 曲库与播放页互斥显示；默认隐藏曲库 */
static void set_library_visible(bool show)
{
    if (!s_lib_panel || !s_player) {
        return;
    }
    if (show) {
        lv_obj_remove_flag(s_lib_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_player, LV_OBJ_FLAG_HIDDEN);
        rebuild_list_ui();
    } else {
        lv_obj_add_flag(s_lib_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(s_player, LV_OBJ_FLAG_HIDDEN);
    }
}

static void on_list_item_clicked(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    set_library_visible(false);
    start_track(idx);
}

static void rebuild_list_ui(void)
{
    if (!s_list) {
        return;
    }
    lv_obj_clean(s_list);
    if (s_track_count == 0) {
        lv_list_add_text(s_list, "(No MP3 in /Music)");
        return;
    }
    for (int i = 0; i < s_track_count; i++) {
        const char *icon = (i == s_track_idx) ? LV_SYMBOL_AUDIO : LV_SYMBOL_FILE;
        lv_obj_t *btn = lv_list_add_button(s_list, icon, track_basename(i));
        lv_obj_add_event_cb(btn, on_list_item_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }
}

static void on_prev_clicked(lv_event_t *e)
{
    (void)e;
    if (s_track_count <= 0) {
        return;
    }
    int idx = (s_track_idx < 0) ? 0 : (s_track_idx + s_track_count - 1) % s_track_count;
    start_track(idx);
}

static void on_next_clicked(lv_event_t *e)
{
    (void)e;
    if (s_track_count <= 0) {
        return;
    }
    int idx = (s_track_idx < 0) ? 0 : (s_track_idx + 1) % s_track_count;
    start_track(idx);
}

static void on_play_pause_clicked(lv_event_t *e)
{
    (void)e;
    if (s_track_count <= 0) {
        return;
    }
    if (s_playing && s_task) {
        s_paused = !s_paused;
        update_transport_ui();
        return;
    }
    start_track(s_track_idx >= 0 ? s_track_idx : 0);
}

static void on_library_clicked(lv_event_t *e)
{
    (void)e;
    set_library_visible(true);
}

static void on_library_close(lv_event_t *e)
{
    (void)e;
    set_library_visible(false);
}

static void poll_cb(lv_timer_t *t)
{
    (void)t;
    static bool was_playing;
    static int last_prog = -1;

    if (!s_playing && !s_task && s_pending_next >= 0) {
        int idx = s_pending_next;
        s_pending_next = -1;
        start_track(idx);
        return;
    }

    if (s_bar && s_progress != last_prog) {
        lv_bar_set_value(s_bar, s_progress, LV_ANIM_OFF);
        last_prog = s_progress;
    }

    if (s_status_lb) {
        if (s_playing && s_paused) {
            lv_label_set_text(s_status_lb, "Paused");
        } else if (s_playing) {
            lv_label_set_text_fmt(s_status_lb, "%d%%", s_progress);
        } else if (s_track_count <= 0) {
            lv_label_set_text(s_status_lb, "Put MP3 in /Music");
        } else {
            lv_label_set_text_fmt(s_status_lb, "%d / %d",
                                  s_track_idx >= 0 ? s_track_idx + 1 : 0, s_track_count);
        }
    }

    if (was_playing != (s_playing && !s_paused)) {
        update_transport_ui();
        was_playing = (s_playing && !s_paused);
    }
}

static lv_obj_t *make_ctrl_btn(lv_obj_t *parent, const char *symbol, lv_event_cb_t cb, int32_t size)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, size, size);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x1E2830), 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lb = lv_label_create(btn);
    lv_label_set_text(lb, symbol);
    lv_obj_set_style_text_font(lb, &lv_font_montserrat_20, 0);
    lv_obj_center(lb);
    return btn;
}

void app_main(void)
{
    ESP_ERROR_CHECK(bsp_display_init());
    if (bsp_sd_init() != ESP_OK) {
        ESP_LOGE(TAG, "SD mount failed");
    }
    ESP_ERROR_CHECK(bsp_audio_init());

    esp_codec_dev_handle_t codec = bsp_audio_get_codec();
    if (codec) {
        esp_codec_dev_set_out_vol(codec, 75); /* 0~100，过大可能破音 */
    }

    if (bsp_sd_is_mounted()) {
        scan_mp3(MUSIC_DIR, 0);
    }
    if (s_track_count > 0) {
        s_track_idx = 0;
    }
    ESP_LOGI(TAG, "found %d track(s)", s_track_count);

    ESP_ERROR_CHECK(esp_lv_adapter_lock(-1));
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), 0);

    lv_obj_t *hdr = lv_label_create(scr);
    lv_label_set_text(hdr, "Music");
    lv_obj_set_style_text_color(hdr, lv_color_hex(0x4DA3FF), 0);
    lv_obj_set_style_text_font(hdr, &lv_font_montserrat_20, 0);
    lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, 28);

    /* -------- 播放页（默认可见） -------- */
    s_player = lv_obj_create(scr);
    lv_obj_set_size(s_player, 260, 260);
    lv_obj_align(s_player, LV_ALIGN_CENTER, 0, 18);
    lv_obj_set_style_bg_opa(s_player, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_player, 0, 0);
    lv_obj_set_style_pad_all(s_player, 0, 0);
    lv_obj_remove_flag(s_player, LV_OBJ_FLAG_SCROLLABLE);

    s_title_lb = lv_label_create(s_player);
    lv_obj_set_width(s_title_lb, 230);
    lv_obj_set_style_text_align(s_title_lb, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(s_title_lb, lv_color_hex(0xE8EEF4), 0);
    lv_obj_set_style_text_font(s_title_lb, &lv_font_montserrat_16, 0);
    lv_label_set_long_mode(s_title_lb, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text(s_title_lb, s_track_count > 0 ? track_basename(0) : "No track");
    lv_obj_align(s_title_lb, LV_ALIGN_TOP_MID, 0, 8);

    s_status_lb = lv_label_create(s_player);
    lv_obj_set_style_text_color(s_status_lb, lv_color_hex(0x8A94A0), 0);
    lv_obj_set_style_text_font(s_status_lb, &lv_font_montserrat_14, 0);
    lv_label_set_text(s_status_lb, s_track_count > 0 ? "Ready" : "Put MP3 in /Music");
    lv_obj_align(s_status_lb, LV_ALIGN_TOP_MID, 0, 36);

    s_bar = lv_bar_create(s_player);
    lv_obj_set_size(s_bar, 200, 6);
    lv_obj_align(s_bar, LV_ALIGN_TOP_MID, 0, 62);
    lv_bar_set_range(s_bar, 0, 100);
    lv_obj_set_style_bg_color(s_bar, lv_color_hex(0x2A3540), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_bar, lv_color_hex(0x4DA3FF), LV_PART_INDICATOR);

    lv_obj_t *row = lv_obj_create(s_player);
    lv_obj_set_size(row, 240, 56);
    lv_obj_align(row, LV_ALIGN_TOP_MID, 0, 86);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    make_ctrl_btn(row, LV_SYMBOL_PREV, on_prev_clicked, 48);
    lv_obj_t *play_btn = make_ctrl_btn(row, LV_SYMBOL_PLAY, on_play_pause_clicked, 56);
    lv_obj_set_style_bg_color(play_btn, lv_color_hex(0x4DA3FF), 0);
    s_play_icon = lv_obj_get_child(play_btn, 0);
    make_ctrl_btn(row, LV_SYMBOL_NEXT, on_next_clicked, 48);

    lv_obj_t *lib_btn = lv_button_create(s_player);
    lv_obj_set_size(lib_btn, 130, 34);
    lv_obj_align(lib_btn, LV_ALIGN_TOP_MID, 0, 160);
    lv_obj_set_style_radius(lib_btn, 16, 0);
    lv_obj_set_style_bg_color(lib_btn, lv_color_hex(0x1E2830), 0);
    lv_obj_add_event_cb(lib_btn, on_library_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lib_lb = lv_label_create(lib_btn);
    lv_label_set_text(lib_lb, LV_SYMBOL_LIST "  Library");
    lv_obj_set_style_text_font(lib_lb, &lv_font_montserrat_14, 0);
    lv_obj_center(lib_lb);

    /* -------- 曲库（默认隐藏） -------- */
    s_lib_panel = lv_obj_create(scr);
    lv_obj_set_size(s_lib_panel, 260, 250);
    lv_obj_align(s_lib_panel, LV_ALIGN_CENTER, 0, 16);
    lv_obj_set_style_bg_color(s_lib_panel, lv_color_hex(0x101418), 0);
    lv_obj_set_style_border_width(s_lib_panel, 0, 0);
    lv_obj_set_style_pad_all(s_lib_panel, 4, 0);
    lv_obj_add_flag(s_lib_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_lib_panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lib_hdr = lv_obj_create(s_lib_panel);
    lv_obj_set_size(lib_hdr, 250, 36);
    lv_obj_align(lib_hdr, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(lib_hdr, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(lib_hdr, 0, 0);
    lv_obj_set_style_pad_all(lib_hdr, 0, 0);
    lv_obj_remove_flag(lib_hdr, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *lib_title = lv_label_create(lib_hdr);
    lv_label_set_text(lib_title, "Playlist");
    lv_obj_set_style_text_color(lib_title, lv_color_hex(0xE8EEF4), 0);
    lv_obj_set_style_text_font(lib_title, &lv_font_montserrat_16, 0);
    lv_obj_align(lib_title, LV_ALIGN_LEFT_MID, 8, 0);

    lv_obj_t *close_btn = lv_button_create(lib_hdr);
    lv_obj_set_size(close_btn, 36, 28);
    lv_obj_align(close_btn, LV_ALIGN_RIGHT_MID, -4, 0);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0x1E2830), 0);
    lv_obj_add_event_cb(close_btn, on_library_close, LV_EVENT_CLICKED, NULL);
    lv_obj_t *cl = lv_label_create(close_btn);
    lv_label_set_text(cl, LV_SYMBOL_CLOSE);
    lv_obj_center(cl);

    s_list = lv_list_create(s_lib_panel);
    lv_obj_set_size(s_list, 250, 200);
    lv_obj_align(s_list, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(s_list, lv_color_hex(0x161C22), 0);
    lv_obj_set_style_border_width(s_list, 0, 0);

    update_transport_ui();
    lv_timer_create(poll_cb, 200, NULL);
    esp_lv_adapter_unlock();
}
