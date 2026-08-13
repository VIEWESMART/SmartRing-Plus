/*
 * 07_recorder — 录 5 秒 WAV 到 /sdcard/Recordings/hello.wav 并回放
 *
 * 验证：点 Record 后状态 recording 5s... → playing... → done；
 *       串口 TAG=recorder 出现 recorded ... hello.wav；电脑可播放该文件。
 * 准备：FAT32 卡；麦克风靠近声源。录音在独立任务里做，避免卡住 LVGL。
 *
 * Author : Ayang
 * Company: SHENZHEN VIEWE TECHNOLOGY CO.,LTD
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "bsp/smartring_plus.h"
#include "esp_codec_dev.h"
#include "esp_log.h"
#include "esp_lv_adapter.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "recorder";
#define REC_DIR       BOARD_SD_MOUNT_POINT "/Recordings"
#define REC_PATH      REC_DIR "/hello.wav"
#define REC_SEC       5
#define CHUNK_BYTES   (BOARD_AUDIO_SAMPLE_RATE * 2 * sizeof(int16_t) / 10)
#define REC_BYTES_MAX ((size_t)BOARD_AUDIO_SAMPLE_RATE * 2 * sizeof(int16_t) * REC_SEC)

static lv_obj_t *s_status_lb;
static volatile bool s_busy;

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

static void ui_status(const char *text)
{
    if (esp_lv_adapter_lock(200) != ESP_OK) {
        return;
    }
    lv_label_set_text(s_status_lb, text);
    esp_lv_adapter_unlock();
}

/* PCM WAV 文件头：44 字节；录完后用 wav_patch_size 回填 data 长度 */
static void wav_write_header(FILE *fp)
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
        .byte_rate = BOARD_AUDIO_SAMPLE_RATE * 4,
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
}

static bool ensure_dir(void)
{
    struct stat st;
    if (stat(REC_DIR, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return mkdir(REC_DIR, 0775) == 0;
}

/* 跳过 44 字节头，把 PCM 数据送 codec 播放 */
static void play_wav(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        ui_status("open wav failed");
        return;
    }
    fseek(fp, 0, SEEK_END);
    long data_len = ftell(fp) - 44;
    fseek(fp, 44, SEEK_SET);
    uint8_t *chunk = malloc(CHUNK_BYTES);
    if (!chunk) {
        fclose(fp);
        return;
    }
    ui_status("playing...");
    bsp_audio_set_mute(false);
    long done = 0;
    while (done < data_len) {
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
    }
    bsp_audio_set_mute(true);
    free(chunk);
    fclose(fp);
    ui_status("done");
}

/* 录音 + 回放放在独立任务，避免阻塞 LVGL 刷新导致界面卡住 */
static void rec_task(void *arg)
{
    (void)arg;
    if (!bsp_sd_is_mounted() || !ensure_dir()) {
        ui_status("SD / dir error");
        s_busy = false;
        vTaskDelete(NULL);
        return;
    }

    FILE *fp = fopen(REC_PATH, "wb");
    if (!fp) {
        ESP_LOGE(TAG, "fopen %s errno=%d", REC_PATH, errno);
        ui_status("write failed");
        s_busy = false;
        vTaskDelete(NULL);
        return;
    }
    wav_write_header(fp);
    uint8_t *chunk = malloc(CHUNK_BYTES);
    if (!chunk) {
        fclose(fp);
        s_busy = false;
        vTaskDelete(NULL);
        return;
    }

    ui_status("recording 5s...");
    size_t total = 0;
    while (total < REC_BYTES_MAX) {
        size_t want = CHUNK_BYTES;
        if (total + want > REC_BYTES_MAX) {
            want = REC_BYTES_MAX - total;
        }
        if (bsp_audio_read(chunk, want, 500) != ESP_OK) {
            break;
        }
        fwrite(chunk, 1, want, fp);
        total += want;
    }
    free(chunk);
    wav_patch_size(fp, (uint32_t)total);
    fclose(fp);
    ESP_LOGI(TAG, "recorded %s (%u bytes)", REC_PATH, (unsigned)total);

    play_wav(REC_PATH);
    s_busy = false;
    vTaskDelete(NULL);
}

static void on_record(lv_event_t *e)
{
    (void)e;
    if (s_busy) {
        return;
    }
    s_busy = true;
    xTaskCreate(rec_task, "rec", 4096, NULL, 10, NULL);
}

void app_main(void)
{
    ESP_ERROR_CHECK(bsp_display_init());
    if (bsp_sd_init() != ESP_OK) {
        ESP_LOGW(TAG, "SD not mounted");
    }
    ESP_ERROR_CHECK(bsp_audio_init());

    esp_codec_dev_handle_t codec = bsp_audio_get_codec();
    if (codec) {
        esp_codec_dev_set_in_gain(codec, 36.0f); /* 麦克风增益，太小会几乎无声 */
        esp_codec_dev_set_out_vol(codec, 75);
    }

    ESP_ERROR_CHECK(esp_lv_adapter_lock(-1));
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), 0);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Recorder");
    lv_obj_set_style_text_color(title, lv_color_hex(0x4DA3FF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 36);

    s_status_lb = lv_label_create(scr);
    lv_label_set_text(s_status_lb, bsp_sd_is_mounted() ? "tap to record 5s" : "insert SD card");
    lv_obj_set_style_text_color(s_status_lb, lv_color_hex(0xE8EEF4), 0);
    lv_obj_set_width(s_status_lb, 260);
    lv_obj_set_style_text_align(s_status_lb, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(s_status_lb, LV_ALIGN_CENTER, 0, -8);

    lv_obj_t *btn = lv_button_create(scr);
    lv_obj_set_size(btn, 160, 48);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -56);
    lv_obj_add_event_cb(btn, on_record, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(btn);
    lv_label_set_text(bl, "Record");
    lv_obj_center(bl);
    esp_lv_adapter_unlock();
}
