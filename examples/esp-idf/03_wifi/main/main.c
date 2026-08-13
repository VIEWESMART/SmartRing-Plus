/*
 * 03_wifi — WiFi STA 连接示例
 *
 * 目的：连上 2.4 GHz 热点后在屏幕显示 IP。
 * 验证：串口 TAG=wifi，成功为 got IP x.x.x.x；失败为 connect failed。
 * SSID/密码：idf.py menuconfig → Example Configuration，不要写进源码。
 * 默认占位 myssid / mypassword，烧录前必须改成你的热点。
 *
 * Author : Ayang
 * Company: SHENZHEN VIEWE TECHNOLOGY CO.,LTD
 */
#include <stdio.h>
#include <string.h>

#include "bsp/smartring_plus.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_lv_adapter.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"

static const char *TAG = "wifi";

#define WIFI_OK_BIT   BIT0
#define WIFI_FAIL_BIT BIT1
#define WIFI_RETRY    5

static EventGroupHandle_t s_events;
static int s_retry;
static char s_ip[16] = "0.0.0.0";
static lv_obj_t *s_status_lb;
static lv_obj_t *s_ip_lb;

static void ui_set_status(const char *text, uint32_t color)
{
    if (esp_lv_adapter_lock(200) != ESP_OK) {
        return;
    }
    lv_label_set_text(s_status_lb, text);
    lv_obj_set_style_text_color(s_status_lb, lv_color_hex(color), 0);
    lv_label_set_text(s_ip_lb, s_ip);
    esp_lv_adapter_unlock();
}

/* STA 启动后发起连接；断开则最多重试 WIFI_RETRY 次 */
static void on_wifi(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    (void)base;
    (void)data;
    if (id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry < WIFI_RETRY) {
            s_retry++;
            ESP_LOGW(TAG, "retry %d/%d", s_retry, WIFI_RETRY);
            esp_wifi_connect();
        } else {
            xEventGroupSetBits(s_events, WIFI_FAIL_BIT);
        }
    }
}

static void on_ip(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    (void)base;
    if (id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)data;
        snprintf(s_ip, sizeof(s_ip), IPSTR, IP2STR(&ev->ip_info.ip));
        s_retry = 0;
        ESP_LOGI(TAG, "got IP %s", s_ip);
        xEventGroupSetBits(s_events, WIFI_OK_BIT);
    }
}

void app_main(void)
{
    /* WiFi 驱动把校准等数据存在 NVS，必须先初始化 */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(bsp_display_init());

    ESP_ERROR_CHECK(esp_lv_adapter_lock(-1));
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x101418), 0);

    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "WiFi");
    lv_obj_set_style_text_color(title, lv_color_hex(0x4DA3FF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 36);

    lv_obj_t *ssid_lb = lv_label_create(scr);
    lv_label_set_text(ssid_lb, CONFIG_EXAMPLE_WIFI_SSID);
    lv_obj_set_style_text_color(ssid_lb, lv_color_hex(0xE8EEF4), 0);
    lv_obj_set_width(ssid_lb, 260);
    lv_label_set_long_mode(ssid_lb, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(ssid_lb, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(ssid_lb, LV_ALIGN_CENTER, 0, -24);

    s_status_lb = lv_label_create(scr);
    lv_label_set_text(s_status_lb, "connecting...");
    lv_obj_set_style_text_color(s_status_lb, lv_color_hex(0xFFB020), 0);
    lv_obj_align(s_status_lb, LV_ALIGN_CENTER, 0, 12);

    s_ip_lb = lv_label_create(scr);
    lv_label_set_text(s_ip_lb, "");
    lv_obj_set_style_text_color(s_ip_lb, lv_color_hex(0x8A94A0), 0);
    lv_obj_align(s_ip_lb, LV_ALIGN_CENTER, 0, 44);
    esp_lv_adapter_unlock();

    s_events = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, on_wifi, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, on_ip, NULL));

    /* CONFIG_EXAMPLE_* 来自 Kconfig.projbuild / menuconfig */
    wifi_config_t wifi_cfg = { 0 };
    strlcpy((char *)wifi_cfg.sta.ssid, CONFIG_EXAMPLE_WIFI_SSID, sizeof(wifi_cfg.sta.ssid));
    strlcpy((char *)wifi_cfg.sta.password, CONFIG_EXAMPLE_WIFI_PASSWORD, sizeof(wifi_cfg.sta.password));
    wifi_cfg.sta.threshold.authmode = (CONFIG_EXAMPLE_WIFI_PASSWORD[0] == '\0')
                                      ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "connecting to \"%s\"", CONFIG_EXAMPLE_WIFI_SSID);

    /* 等到拿到 IP 或重试耗尽，最多 30 秒 */
    EventBits_t bits = xEventGroupWaitBits(s_events, WIFI_OK_BIT | WIFI_FAIL_BIT,
                                           pdFALSE, pdFALSE, pdMS_TO_TICKS(30000));
    if (bits & WIFI_OK_BIT) {
        ui_set_status("connected", 0x3DDC84);
    } else {
        snprintf(s_ip, sizeof(s_ip), "failed");
        ui_set_status("connect failed", 0xFF5A5A);
        ESP_LOGE(TAG, "connect failed (check SSID/password in menuconfig)");
    }
}
