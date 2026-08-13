/*
 * 录音：写入 /sdcard/Recordings 下的 .wav，列表与回放
 * 格式：48kHz 立体声 16bit PCM，最长 10s 或手动停止
 */
#include "feature_recorder.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <stdlib.h>

#include "bsp/board_config.h"
#include "bsp/smartring_plus.h"
#include "esp_codec_dev.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ui_shell.h"
#include "ui_theme.h"

static const char *TAG = "feat_rec";
#define REC_DIR         BOARD_SD_MOUNT_POINT "/Recordings"
#define REC_MAX_SEC     10
#define REC_BYTES_MAX   ((size_t)BOARD_AUDIO_SAMPLE_RATE * 2 * sizeof(int16_t) * REC_MAX_SEC)
#define CHUNK_BYTES     (BOARD_AUDIO_SAMPLE_RATE * 2 * sizeof(int16_t) / 10)

static lv_obj_t *s_scr;
static lv_obj_t *s_list;
static lv_obj_t *s_status_lb;
static lv_timer_t *s_poll_timer;

static volatile bool s_recording;
static volatile bool s_playing;
static volatile int s_progress;
static TaskHandle_t s_task;
static FILE *s_wav_fp;
static char s_last_path[128];

#pragma pack(push, 1)
typedef struct {
    char riff[4];
    uint32_t size;
    char wave[4];
    char fmt[4];
    uint32_t fmt_size;
    uint16_t audio_format;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits;
    char data[4];
    uint32_t data_size;
} wav_hdr_t;
#pragma pack(pop)

static void wav_write_header_placeholder(FILE *fp)
{
    wav_hdr_t h = {
        .riff = {'R', 'I', 'F', 'F'},
        .size = 36,
        .wave = {'W', 'A', 'V', 'E'},
        .fmt = {'f', 'm', 't', ' '},
        .fmt_size = 16,
        .audio_format = 1,
        .channels = 2,
        .sample_rate = BOARD_AUDIO_SAMPLE_RATE,
        .byte_rate = BOARD_AUDIO_SAMPLE_RATE * 2 * 2,
        .block_align = 4,
        .bits = 16,
        .data = {'d', 'a', 't', 'a'},
        .data_size = 0,
    };
    fwrite(&h, 1, sizeof(h), fp);
}

static void wav_patch_size(FILE *fp, uint32_t data_bytes)
{
    uint32_t riff_size = data_bytes + 36;
    fseek(fp, 4, SEEK_SET);
    fwrite(&riff_size, 1, 4, fp);
    fseek(fp, 40, SEEK_SET);
    fwrite(&data_bytes, 1, 4, fp);
    fseek(fp, 0, SEEK_END);
}

static bool ensure_rec_dir(void)
{
    struct stat st;
    if (stat(REC_DIR, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return true;
        }
        ESP_LOGE(TAG, "%s exists but is not a directory", REC_DIR);
        return false;
    }
    if (mkdir(REC_DIR, 0775) != 0) {
        ESP_LOGE(TAG, "mkdir %s failed: errno=%d (%s)", REC_DIR, errno, strerror(errno));
        return false;
    }
    ESP_LOGI(TAG, "created %s", REC_DIR);
    return true;
}

static void make_rec_path(char *out, size_t len)
{
    time_t now = time(NULL);
    struct tm ti;
    localtime_r(&now, &ti);
    snprintf(out, len, REC_DIR "/rec_%04d%02d%02d_%02d%02d%02d.wav",
             ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday,
             ti.tm_hour, ti.tm_min, ti.tm_sec);
}

static void record_task(void *arg)
{
    (void)arg;

    if (!bsp_sd_is_mounted()) {
        ESP_LOGE(TAG, "SD not mounted");
        s_recording = false;
        s_task = NULL;
        vTaskDelete(NULL);
        return;
    }
    if (!ensure_rec_dir()) {
        s_recording = false;
        s_task = NULL;
        vTaskDelete(NULL);
        return;
    }

    make_rec_path(s_last_path, sizeof(s_last_path));
    s_wav_fp = fopen(s_last_path, "wb");
    if (!s_wav_fp) {
        ESP_LOGE(TAG, "fopen %s failed: errno=%d (%s)", s_last_path, errno, strerror(errno));
        s_recording = false;
        s_task = NULL;
        vTaskDelete(NULL);
        return;
    }
    wav_write_header_placeholder(s_wav_fp);

    uint8_t *chunk = malloc(CHUNK_BYTES);
    if (!chunk) {
        fclose(s_wav_fp);
        s_wav_fp = NULL;
        s_recording = false;
        s_task = NULL;
        vTaskDelete(NULL);
        return;
    }

    size_t total = 0;
    while (s_recording && total < REC_BYTES_MAX) {
        size_t want = CHUNK_BYTES;
        if (total + want > REC_BYTES_MAX) {
            want = REC_BYTES_MAX - total;
        }
        if (bsp_audio_read(chunk, want, 500) != ESP_OK) {
            ESP_LOGW(TAG, "audio read failed at %u bytes", (unsigned)total);
            break;
        }
        size_t wr = fwrite(chunk, 1, want, s_wav_fp);
        if (wr != want) {
            ESP_LOGE(TAG, "fwrite short: %u/%u errno=%d", (unsigned)wr, (unsigned)want, errno);
            break;
        }
        total += want;
        s_progress = (int)(total * 100 / REC_BYTES_MAX);
    }
    free(chunk);

    wav_patch_size(s_wav_fp, (uint32_t)total);
    fflush(s_wav_fp);
    fclose(s_wav_fp);
    s_wav_fp = NULL;
    s_recording = false;
    s_progress = 100;
    s_task = NULL;
    ESP_LOGI(TAG, "recorded: %s (%u bytes)", s_last_path, (unsigned)total);
    vTaskDelete(NULL);
}

static void play_task(void *arg)
{
    const char *path = (const char *)arg;
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        s_playing = false;
        s_task = NULL;
        vTaskDelete(NULL);
        return;
    }
    fseek(fp, 44, SEEK_SET);

    uint8_t *chunk = malloc(CHUNK_BYTES);
    if (!chunk) {
        fclose(fp);
        s_playing = false;
        s_task = NULL;
        vTaskDelete(NULL);
        return;
    }

    fseek(fp, 0, SEEK_END);
    long data_len = ftell(fp) - 44;
    fseek(fp, 44, SEEK_SET);

    bsp_audio_set_mute(false);
    long done = 0;
    while (s_playing && done < data_len) {
        size_t want = CHUNK_BYTES;
        if (done + (long)want > data_len) {
            want = (size_t)(data_len - done);
        }
        size_t rd = fread(chunk, 1, want, fp);
        if (rd == 0) {
            break;
        }
        bsp_audio_write(chunk, rd);
        done += (long)rd;
        s_progress = data_len > 0 ? (int)(done * 100 / data_len) : 100;
    }
    bsp_audio_set_mute(true);
    free(chunk);
    fclose(fp);
    s_playing = false;
    s_progress = 100;
    s_task = NULL;
    vTaskDelete(NULL);
}

static void rebuild_list(void);

static void on_record_clicked(lv_event_t *e)
{
    (void)e;
    if (s_recording || s_playing || s_task) {
        return;
    }
    s_progress = 0;
    s_recording = true;
    xTaskCreate(record_task, "rec", 4096, NULL, 10, &s_task);
}

static void on_stop_clicked(lv_event_t *e)
{
    (void)e;
    s_recording = false;
}

static void on_entry_clicked(lv_event_t *e)
{
    const char *name = (const char *)lv_event_get_user_data(e);
    if (s_recording || s_playing || s_task || !name) {
        return;
    }
    static char path[160];
    snprintf(path, sizeof(path), REC_DIR "/%s", name);
    s_progress = 0;
    s_playing = true;
    xTaskCreate(play_task, "play", 4096, path, 10, &s_task);
}

static void rebuild_list(void)
{
    lv_obj_clean(s_list);
    ensure_rec_dir();

    DIR *d = opendir(REC_DIR);
    if (!d) {
        lv_list_add_text(s_list, "(Cannot open folder)");
        return;
    }
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        /* FAT VFS 上 d_type 常为 DT_UNKNOWN，不能只认 DT_REG */
        if (ent->d_type != DT_REG && ent->d_type != DT_UNKNOWN) {
            continue;
        }
        const char *n = ent->d_name;
        if (n[0] == '.') {
            continue;
        }
        size_t ln = strlen(n);
        if (ln < 5 || strcasecmp(n + ln - 4, ".wav") != 0) {
            continue;
        }
        lv_obj_t *btn = lv_list_add_button(s_list, LV_SYMBOL_AUDIO, n);
        char *dup = strdup(n);
        if (dup) {
            lv_obj_add_event_cb(btn, on_entry_clicked, LV_EVENT_CLICKED, dup);
        }
    }
    closedir(d);
}

static void poll_cb(lv_timer_t *t)
{
    (void)t;
    static bool was_busy;
    if (s_recording) {
        lv_label_set_text_fmt(s_status_lb, "Recording %d%%", s_progress);
        was_busy = true;
    } else if (s_playing) {
        lv_label_set_text_fmt(s_status_lb, "Playing %d%%", s_progress);
        was_busy = true;
    } else {
        lv_label_set_text(s_status_lb, "Ready");
        if (was_busy) {
            rebuild_list();
            was_busy = false;
        }
    }
}

lv_obj_t *feature_recorder_create_screen(void)
{
    esp_codec_dev_handle_t codec = bsp_audio_get_codec();
    if (codec) {
        esp_codec_dev_set_in_gain(codec, 36.0f);
        esp_codec_dev_set_out_vol(codec, 75);
    }

    s_scr = lv_obj_create(NULL);
    ui_theme_apply_screen(s_scr);
    ui_shell_add_back_button(s_scr, "Record");

    s_status_lb = lv_label_create(s_scr);
    lv_obj_set_style_text_color(s_status_lb, UI_COLOR_MUTED, 0);
    lv_obj_align(s_status_lb, LV_ALIGN_TOP_MID, 0, 68);

    lv_obj_t *row = lv_obj_create(s_scr);
    lv_obj_set_size(row, 200, 40);
    lv_obj_align(row, LV_ALIGN_TOP_MID, 0, 92);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *rec_btn = lv_button_create(row);
    lv_obj_set_style_bg_color(rec_btn, UI_COLOR_WARN, 0);
    lv_obj_add_event_cb(rec_btn, on_record_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_t *rl = lv_label_create(rec_btn);
    lv_label_set_text(rl, "Rec");

    lv_obj_t *stop_btn = lv_button_create(row);
    lv_obj_add_event_cb(stop_btn, on_stop_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_t *sl = lv_label_create(stop_btn);
    lv_label_set_text(sl, "Stop");

    s_list = lv_list_create(s_scr);
    lv_obj_set_size(s_list, 230, 150);
    lv_obj_align(s_list, LV_ALIGN_BOTTOM_MID, 0, -30);

    rebuild_list();
    s_poll_timer = lv_timer_create(poll_cb, 200, NULL);
    return s_scr;
}

void feature_recorder_destroy(void)
{
    s_recording = false;
    s_playing = false;
    if (s_poll_timer) {
        lv_timer_delete(s_poll_timer);
        s_poll_timer = NULL;
    }
    if (s_scr) {
        lv_obj_delete(s_scr);
        s_scr = NULL;
    }
}
