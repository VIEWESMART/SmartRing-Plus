/*
 * WiFi STA: scan, connect, SNTP time sync
 */
#include "wifi_service.h"

#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static const char *TAG = "wifi_svc";

#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1
#define WIFI_SCAN_DONE_BIT  BIT2
#define WIFI_MAX_RETRY      5

static EventGroupHandle_t s_wifi_events;
static SemaphoreHandle_t s_lock;
static wifi_svc_status_t s_status = WIFI_SVC_DISCONNECTED;
static int s_retry;
static char s_ip[16] = "0.0.0.0";
static char s_ssid[WIFI_SVC_SSID_LEN];
static bool s_time_valid;
static bool s_want_connect; /* only connect when user requested */

static wifi_svc_ap_t s_aps[WIFI_SVC_SCAN_MAX];
static int s_ap_count;
static bool s_scan_done = true;

/* Must NOT live on the system event task stack (only ~2.3KB). */
static wifi_ap_record_t s_scan_records[WIFI_SVC_SCAN_MAX];

static int ap_rssi_cmp(const void *a, const void *b)
{
    const wifi_svc_ap_t *aa = a, *bb = b;
    return (int)bb->rssi - (int)aa->rssi;
}

static void process_scan_results(void)
{
    uint16_t n = WIFI_SVC_SCAN_MAX;
    esp_err_t err = esp_wifi_scan_get_ap_records(&n, s_scan_records);

    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_ap_count = 0;
    if (err == ESP_OK) {
        for (uint16_t i = 0; i < n && s_ap_count < WIFI_SVC_SCAN_MAX; i++) {
            if (s_scan_records[i].ssid[0] == '\0') {
                continue;
            }
            /* Deduplicate by SSID, keep strongest */
            int exist = -1;
            for (int j = 0; j < s_ap_count; j++) {
                if (strcmp(s_aps[j].ssid, (const char *)s_scan_records[i].ssid) == 0) {
                    exist = j;
                    break;
                }
            }
            if (exist >= 0) {
                if (s_scan_records[i].rssi > s_aps[exist].rssi) {
                    s_aps[exist].rssi = s_scan_records[i].rssi;
                    s_aps[exist].authmode = (uint8_t)s_scan_records[i].authmode;
                }
                continue;
            }
            strlcpy(s_aps[s_ap_count].ssid, (const char *)s_scan_records[i].ssid,
                    sizeof(s_aps[s_ap_count].ssid));
            s_aps[s_ap_count].rssi = s_scan_records[i].rssi;
            s_aps[s_ap_count].authmode = (uint8_t)s_scan_records[i].authmode;
            s_ap_count++;
        }
        qsort(s_aps, (size_t)s_ap_count, sizeof(s_aps[0]), ap_rssi_cmp);
    }
    s_scan_done = true;
    if (s_status == WIFI_SVC_SCANNING) {
        s_status = WIFI_SVC_DISCONNECTED;
    }
    xSemaphoreGive(s_lock);
    xEventGroupSetBits(s_wifi_events, WIFI_SCAN_DONE_BIT);
    ESP_LOGI(TAG, "scan done, %d APs", s_ap_count);
}

static void on_wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    (void)base;
    (void)data;

    switch (id) {
    case WIFI_EVENT_STA_START:
        ESP_LOGI(TAG, "STA started");
        break;

    case WIFI_EVENT_SCAN_DONE:
        process_scan_results();
        break;

    case WIFI_EVENT_STA_DISCONNECTED:
        if (!s_want_connect) {
            s_status = WIFI_SVC_DISCONNECTED;
            break;
        }
        if (s_retry < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            s_retry++;
            s_status = WIFI_SVC_CONNECTING;
            ESP_LOGW(TAG, "reconnect (%d/%d)", s_retry, WIFI_MAX_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_events, WIFI_FAIL_BIT);
            s_status = WIFI_SVC_FAILED;
            s_want_connect = false;
            ESP_LOGE(TAG, "connect failed");
        }
        snprintf(s_ip, sizeof(s_ip), "0.0.0.0");
        break;

    case WIFI_EVENT_STA_CONNECTED:
        s_retry = 0;
        break;

    default:
        break;
    }
}

static void sntp_task(void *arg)
{
    (void)arg;
    wifi_service_sync_time();
    vTaskDelete(NULL);
}

static void on_ip_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    (void)base;
    if (id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)data;
        snprintf(s_ip, sizeof(s_ip), IPSTR, IP2STR(&ev->ip_info.ip));
        s_status = WIFI_SVC_CONNECTED;
        s_want_connect = false;
        xEventGroupSetBits(s_wifi_events, WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG, "connected, IP: %s SSID: %s", s_ip, s_ssid);
        xTaskCreate(sntp_task, "sntp", 3072, NULL, 5, NULL);
    }
}

esp_err_t wifi_service_init(void)
{
    s_wifi_events = xEventGroupCreate();
    s_lock = xSemaphoreCreateMutex();
    if (!s_wifi_events || !s_lock) {
        return ESP_ERR_NO_MEM;
    }

    ESP_ERROR_CHECK(esp_netif_init());
    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, on_wifi_event, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, on_ip_event, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    s_status = WIFI_SVC_DISCONNECTED;
    return ESP_OK;
}

esp_err_t wifi_service_scan_start(void)
{
    if (s_status == WIFI_SVC_CONNECTING) {
        return ESP_ERR_INVALID_STATE;
    }

    s_want_connect = false;
    esp_wifi_disconnect();

    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_ap_count = 0;
    s_scan_done = false;
    s_status = WIFI_SVC_SCANNING;
    xSemaphoreGive(s_lock);

    xEventGroupClearBits(s_wifi_events, WIFI_SCAN_DONE_BIT);

    wifi_scan_config_t scan = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = 100,
        .scan_time.active.max = 300,
    };
    esp_err_t err = esp_wifi_scan_start(&scan, false);
    if (err != ESP_OK) {
        xSemaphoreTake(s_lock, portMAX_DELAY);
        s_scan_done = true;
        s_status = WIFI_SVC_DISCONNECTED;
        xSemaphoreGive(s_lock);
        ESP_LOGE(TAG, "scan_start failed: %s", esp_err_to_name(err));
    }
    return err;
}

bool wifi_service_scan_done(void)
{
    return s_scan_done;
}

int wifi_service_get_ap_count(void)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    int n = s_ap_count;
    xSemaphoreGive(s_lock);
    return n;
}

esp_err_t wifi_service_get_ap(int index, wifi_svc_ap_t *out)
{
    if (!out) {
        return ESP_ERR_INVALID_ARG;
    }
    xSemaphoreTake(s_lock, portMAX_DELAY);
    if (index < 0 || index >= s_ap_count) {
        xSemaphoreGive(s_lock);
        return ESP_ERR_INVALID_ARG;
    }
    *out = s_aps[index];
    xSemaphoreGive(s_lock);
    return ESP_OK;
}

esp_err_t wifi_service_connect(const char *ssid, const char *password)
{
    if (!ssid || ssid[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }

    esp_wifi_scan_stop();

    s_retry = 0;
    s_want_connect = true;
    s_status = WIFI_SVC_CONNECTING;
    s_scan_done = true;
    strlcpy(s_ssid, ssid, sizeof(s_ssid));
    xEventGroupClearBits(s_wifi_events, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

    wifi_config_t cfg = {0};
    strlcpy((char *)cfg.sta.ssid, ssid, sizeof(cfg.sta.ssid));
    if (password && password[0]) {
        strlcpy((char *)cfg.sta.password, password, sizeof(cfg.sta.password));
        /* Minimum auth: accept WPA/WPA2/WPA3 */
        cfg.sta.threshold.authmode = WIFI_AUTH_WPA_PSK;
    } else {
        cfg.sta.threshold.authmode = WIFI_AUTH_OPEN;
    }
    cfg.sta.pmf_cfg.capable = true;
    cfg.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &cfg));
    esp_wifi_disconnect();
    esp_err_t err = esp_wifi_connect();
    ESP_LOGI(TAG, "connecting to '%s' (%s)", ssid, esp_err_to_name(err));
    return err;
}

void wifi_service_disconnect(void)
{
    s_want_connect = false;
    esp_wifi_disconnect();
    s_status = WIFI_SVC_DISCONNECTED;
    snprintf(s_ip, sizeof(s_ip), "0.0.0.0");
}

wifi_svc_status_t wifi_service_get_status(void)
{
    return s_status;
}

bool wifi_service_is_connected(void)
{
    return s_status == WIFI_SVC_CONNECTED;
}

const char *wifi_service_get_ip_str(void)
{
    return s_ip;
}

const char *wifi_service_get_ssid(void)
{
    return s_ssid;
}

esp_err_t wifi_service_sync_time(void)
{
    if (!wifi_service_is_connected()) {
        return ESP_ERR_INVALID_STATE;
    }

    setenv("TZ", "CST-8", 1);
    tzset();

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "ntp.aliyun.com");
    esp_sntp_init();

    for (int i = 0; i < 20; i++) {
        time_t now = 0;
        struct tm ti = {0};
        time(&now);
        localtime_r(&now, &ti);
        if (ti.tm_year > (2020 - 1900)) {
            s_time_valid = true;
            ESP_LOGI(TAG, "SNTP ok");
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    ESP_LOGW(TAG, "SNTP timeout");
    return ESP_ERR_TIMEOUT;
}

bool wifi_service_time_valid(void)
{
    return s_time_valid;
}
