/*
 * 音乐播放器：扫描 /sdcard/Music 下的 .mp3，esp_audio_simple_dec 解码
 * ES8311 固定 48kHz：非 48k 源做简易重采样（重复/丢弃采样）
 *
 * UI：MP3 播放器样式 — 默认显示播放页（曲名/进度/运输键），曲库列表默认收起。
 */
#include "feature_music.h"

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>

#include "bsp/board_config.h"
#include "bsp/smartring_plus.h"
#include "esp_audio_simple_dec.h"
#include "esp_audio_simple_dec_default.h"
#include "esp_audio_dec_default.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ui_shell.h"
#include "ui_theme.h"

static const char *TAG = "feat_music";
#define MUSIC_DIR       BOARD_SD_MOUNT_POINT "/Music"
#define READ_SZ         4096
#define OUT_SZ_INIT     8192
#define RS_MAX_RATIO    6
#define MAX_TRACKS      64

static lv_obj_t *s_scr;
static lv_obj_t *s_player;       /* 播放主界面容器 */
static lv_obj_t *s_title_lb;
static lv_obj_t *s_status_lb;
static lv_obj_t *s_bar;
static lv_obj_t *s_play_btn;
static lv_obj_t *s_play_icon;
static lv_obj_t *s_lib_panel;    /* 曲库浮层，默认隐藏 */
static lv_obj_t *s_list;
static lv_timer_t *s_poll;

static char *s_tracks[MAX_TRACKS];
static int s_track_count;
static int s_track_idx = -1;

static volatile bool s_playing;  /* 播放任务存活 */
static volatile bool s_paused;
static volatile bool s_want_auto_next; /* 当前曲允许播完自动下一首 */
static volatile int s_pending_next;    /* >=0：UI 侧待启动的下一曲索引 */
static volatile int s_progress;
static TaskHandle_t s_task;
static char s_play_path[192];

static bool s_codec_registered;

static void scan_mp3(const char *dir, int depth);
static void rebuild_playlist(void);
static void rebuild_list_ui(void);
static void update_transport_ui(void);
static void start_track(int idx);
static void stop_playback(void);
static void wait_task_done(void);

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
        int src_i = (in_rate == BOARD_AUDIO_SAMPLE_RATE)
                    ? i
                    : (i * in_rate / BOARD_AUDIO_SAMPLE_RATE);
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

static void play_task(void *arg)
{
    const char *path = (const char *)arg;
    ESP_LOGI(TAG, "play: %s", path);
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        ESP_LOGE(TAG, "fopen failed: %s", path);
        s_playing = false;
        s_paused = false;
        s_task = NULL;
        vTaskDelete(NULL);
        return;
    }

    if (!s_codec_registered) {
        esp_audio_dec_register_default();
        esp_audio_simple_dec_register_default();
        s_codec_registered = true;
    }

    esp_audio_simple_dec_cfg_t cfg = {
        .dec_type = ESP_AUDIO_SIMPLE_DEC_TYPE_MP3,
    };
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
        ESP_LOGE(TAG, "oom for decode buffers");
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
    bool info_logged = false;
    size_t total_pcm = 0;
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
            .buffer = in_buf,
            .len = (uint32_t)rd,
            .eos = (rd < READ_SZ),
            .consumed = 0,
        };
        esp_audio_simple_dec_out_t out = {
            .buffer = out_buf,
            .len = out_cap,
        };

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
                    ESP_LOGE(TAG, "realloc out_buf %u failed", (unsigned)need);
                    finished_ok = false;
                    s_playing = false;
                    break;
                }
                out_buf = nb;
                out_cap = need;
                size_t need_frames = (out_cap / sizeof(int16_t) + 8) * RS_MAX_RATIO;
                if (need_frames > rs_cap_frames) {
                    int16_t *nrs = realloc(rs_buf, need_frames * 2 * sizeof(int16_t));
                    if (!nrs) {
                        ESP_LOGE(TAG, "realloc rs_buf failed");
                        finished_ok = false;
                        s_playing = false;
                        break;
                    }
                    rs_buf = nrs;
                    rs_cap_frames = need_frames;
                }
                continue;
            }
            if (er != ESP_AUDIO_ERR_OK) {
                ESP_LOGE(TAG, "decode err %d at pos %ld", (int)er, pos);
                finished_ok = false;
                s_playing = false;
                break;
            }
            if (raw.consumed == 0) {
                ESP_LOGW(TAG, "decode consumed=0, stop feed loop");
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
                    if (!info_logged) {
                        ESP_LOGI(TAG, "audio: %d Hz / %d ch / %d bit",
                                 sample_rate, channels, (int)info.bits_per_sample);
                        info_logged = true;
                    }
                }
                int bps = (channels > 0) ? channels : 2;
                int in_samples = (int)out.decoded_size / (bps * (int)sizeof(int16_t));
                int out_samples = 0;
                resample_to_48k_stereo((const int16_t *)out_buf, in_samples, sample_rate, channels,
                                       rs_buf, (int)rs_cap_frames, &out_samples);
                if (out_samples > 0) {
                    size_t pcm_bytes = (size_t)out_samples * 2 * sizeof(int16_t);
                    if (bsp_audio_write(rs_buf, pcm_bytes) != ESP_OK) {
                        ESP_LOGE(TAG, "bsp_audio_write failed");
                        finished_ok = false;
                        s_playing = false;
                        break;
                    }
                    total_pcm += pcm_bytes;
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
    esp_audio_simple_dec_close(dec);
    free(in_buf);
    free(out_buf);
    free(rs_buf);
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
    ESP_LOGI(TAG, "播放结束, pcm=%u bytes%s", (unsigned)total_pcm,
             do_next ? " -> next" : "");
    vTaskDelete(NULL);
}

static void free_playlist(void)
{
    for (int i = 0; i < s_track_count; i++) {
        free(s_tracks[i]);
        s_tracks[i] = NULL;
    }
    s_track_count = 0;
    s_track_idx = -1;
}

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

static void rebuild_playlist(void)
{
    free_playlist();
    struct stat st;
    if (stat(MUSIC_DIR, &st) != 0) {
        return;
    }
    scan_mp3(MUSIC_DIR, 0);
}

static const char *track_basename(int idx)
{
    if (idx < 0 || idx >= s_track_count || !s_tracks[idx]) {
        return "No track";
    }
    const char *base = strrchr(s_tracks[idx], '/');
    return base ? base + 1 : s_tracks[idx];
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
    xTaskCreate(play_task, "mp3", 8192, s_play_path, 10, &s_task);
    update_transport_ui();
}

static void update_transport_ui(void)
{
    if (!s_play_icon) {
        return;
    }
    if (s_playing && !s_paused) {
        lv_label_set_text(s_play_icon, LV_SYMBOL_PAUSE);
    } else {
        lv_label_set_text(s_play_icon, LV_SYMBOL_PLAY);
    }
    if (s_title_lb && s_track_idx >= 0) {
        lv_label_set_text(s_title_lb, track_basename(s_track_idx));
    }
}

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
        const char *name = track_basename(i);
        const char *icon = (i == s_track_idx) ? LV_SYMBOL_AUDIO : LV_SYMBOL_FILE;
        lv_obj_t *btn = lv_list_add_button(s_list, icon, name);
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
        if (s_status_lb) {
            lv_label_set_text(s_status_lb, "No tracks");
        }
        return;
    }
    if (s_playing && s_task) {
        s_paused = !s_paused;
        update_transport_ui();
        return;
    }
    int idx = (s_track_idx >= 0) ? s_track_idx : 0;
    start_track(idx);
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

    /* 自然播完后自动下一曲 */
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

static lv_obj_t *make_ctrl_btn(lv_obj_t *parent, const char *symbol, lv_event_cb_t cb, lv_coord_t size)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, size, size);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x1E2830), 0);
    lv_obj_set_style_bg_color(btn, UI_COLOR_ACCENT, LV_STATE_PRESSED);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lb = lv_label_create(btn);
    lv_label_set_text(lb, symbol);
    lv_obj_set_style_text_font(lb, UI_FONT_LG, 0);
    lv_obj_center(lb);
    return btn;
}

lv_obj_t *feature_music_create_screen(void)
{
    esp_codec_dev_handle_t codec = bsp_audio_get_codec();
    if (codec) {
        esp_codec_dev_set_out_vol(codec, 75);
    }

    rebuild_playlist();
    s_pending_next = -1;
    s_want_auto_next = false;

    s_scr = lv_obj_create(NULL);
    ui_theme_apply_screen(s_scr);
    ui_shell_add_back_button(s_scr, "Music");

    /* -------- 播放主界面（默认可见） -------- */
    s_player = lv_obj_create(s_scr);
    lv_obj_set_size(s_player, 240, 248);
    lv_obj_align(s_player, LV_ALIGN_CENTER, 0, 22);
    lv_obj_set_style_bg_opa(s_player, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_player, 0, 0);
    lv_obj_set_style_pad_all(s_player, 0, 0);
    lv_obj_remove_flag(s_player, LV_OBJ_FLAG_SCROLLABLE);

    /* 碟片占位 */
    lv_obj_t *disc = lv_obj_create(s_player);
    lv_obj_set_size(disc, 72, 72);
    lv_obj_align(disc, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_set_style_radius(disc, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(disc, lv_color_hex(0x243040), 0);
    lv_obj_set_style_border_color(disc, UI_COLOR_ACCENT, 0);
    lv_obj_set_style_border_width(disc, 2, 0);
    lv_obj_remove_flag(disc, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *disc_lb = lv_label_create(disc);
    lv_label_set_text(disc_lb, LV_SYMBOL_AUDIO);
    lv_obj_set_style_text_font(disc_lb, UI_FONT_XL, 0);
    lv_obj_set_style_text_color(disc_lb, UI_COLOR_ACCENT, 0);
    lv_obj_center(disc_lb);

    s_title_lb = lv_label_create(s_player);
    lv_obj_set_width(s_title_lb, 220);
    lv_obj_set_style_text_align(s_title_lb, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(s_title_lb, UI_FONT_MD, 0);
    lv_obj_set_style_text_color(s_title_lb, UI_COLOR_TEXT, 0);
    lv_label_set_long_mode(s_title_lb, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text(s_title_lb, s_track_count > 0 ? track_basename(0) : "No track");
    lv_obj_align(s_title_lb, LV_ALIGN_TOP_MID, 0, 86);

    s_status_lb = lv_label_create(s_player);
    lv_obj_set_style_text_color(s_status_lb, UI_COLOR_MUTED, 0);
    lv_obj_set_style_text_font(s_status_lb, UI_FONT_SM, 0);
    lv_label_set_text(s_status_lb, s_track_count > 0 ? "Ready" : "Put MP3 in /Music");
    lv_obj_align(s_status_lb, LV_ALIGN_TOP_MID, 0, 110);

    s_bar = lv_bar_create(s_player);
    lv_obj_set_size(s_bar, 200, 6);
    lv_obj_align(s_bar, LV_ALIGN_TOP_MID, 0, 132);
    lv_bar_set_range(s_bar, 0, 100);
    lv_bar_set_value(s_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(s_bar, lv_color_hex(0x2A3540), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_bar, UI_COLOR_ACCENT, LV_PART_INDICATOR);

    /* 运输键：上一曲 / 播放暂停 / 下一曲 */
    lv_obj_t *row = lv_obj_create(s_player);
    lv_obj_set_size(row, 220, 52);
    lv_obj_align(row, LV_ALIGN_TOP_MID, 0, 150);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    make_ctrl_btn(row, LV_SYMBOL_PREV, on_prev_clicked, 44);
    s_play_btn = make_ctrl_btn(row, LV_SYMBOL_PLAY, on_play_pause_clicked, 52);
    lv_obj_set_style_bg_color(s_play_btn, UI_COLOR_ACCENT, 0);
    s_play_icon = lv_obj_get_child(s_play_btn, 0);
    make_ctrl_btn(row, LV_SYMBOL_NEXT, on_next_clicked, 44);

    /* 曲库入口 */
    lv_obj_t *lib_btn = lv_button_create(s_player);
    lv_obj_set_size(lib_btn, 120, 32);
    lv_obj_align(lib_btn, LV_ALIGN_TOP_MID, 0, 210);
    lv_obj_set_style_radius(lib_btn, 16, 0);
    lv_obj_set_style_bg_color(lib_btn, lv_color_hex(0x1E2830), 0);
    lv_obj_add_event_cb(lib_btn, on_library_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lib_lb = lv_label_create(lib_btn);
    lv_label_set_text(lib_lb, LV_SYMBOL_LIST "  Library");
    lv_obj_set_style_text_font(lib_lb, UI_FONT_SM, 0);
    lv_obj_center(lib_lb);

    /* -------- 曲库浮层（默认隐藏） -------- */
    s_lib_panel = lv_obj_create(s_scr);
    lv_obj_set_size(s_lib_panel, 240, 230);
    lv_obj_align(s_lib_panel, LV_ALIGN_CENTER, 0, 18);
    lv_obj_set_style_bg_color(s_lib_panel, UI_COLOR_BG, 0);
    lv_obj_set_style_border_width(s_lib_panel, 0, 0);
    lv_obj_set_style_pad_all(s_lib_panel, 4, 0);
    lv_obj_add_flag(s_lib_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_lib_panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lib_hdr = lv_obj_create(s_lib_panel);
    lv_obj_set_size(lib_hdr, 230, 36);
    lv_obj_align(lib_hdr, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(lib_hdr, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(lib_hdr, 0, 0);
    lv_obj_set_style_pad_all(lib_hdr, 0, 0);
    lv_obj_remove_flag(lib_hdr, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *lib_title = lv_label_create(lib_hdr);
    lv_label_set_text(lib_title, "Playlist");
    lv_obj_set_style_text_font(lib_title, UI_FONT_MD, 0);
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
    lv_obj_set_size(s_list, 230, 180);
    lv_obj_align(s_list, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(s_list, lv_color_hex(0x161C22), 0);
    lv_obj_set_style_border_width(s_list, 0, 0);

    if (s_track_count > 0) {
        s_track_idx = 0;
    }
    update_transport_ui();
    s_poll = lv_timer_create(poll_cb, 200, NULL);
    return s_scr;
}

void feature_music_destroy(void)
{
    stop_playback();
    if (s_poll) {
        lv_timer_delete(s_poll);
        s_poll = NULL;
    }
    free_playlist();
    s_player = NULL;
    s_lib_panel = NULL;
    s_list = NULL;
    s_title_lb = NULL;
    s_status_lb = NULL;
    s_bar = NULL;
    s_play_btn = NULL;
    s_play_icon = NULL;
    if (s_scr) {
        lv_obj_delete(s_scr);
        s_scr = NULL;
    }
}
