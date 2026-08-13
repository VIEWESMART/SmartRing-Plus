#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WIFI_SVC_SSID_LEN   33
#define WIFI_SVC_SCAN_MAX   16

typedef enum {
    WIFI_SVC_DISCONNECTED = 0,
    WIFI_SVC_SCANNING,
    WIFI_SVC_CONNECTING,
    WIFI_SVC_CONNECTED,
    WIFI_SVC_FAILED,
} wifi_svc_status_t;

typedef struct {
    char ssid[WIFI_SVC_SSID_LEN];
    int8_t rssi;
    uint8_t authmode; /* wifi_auth_mode_t */
} wifi_svc_ap_t;

esp_err_t wifi_service_init(void);

/** Start async scan; results ready when status leaves WIFI_SVC_SCANNING. */
esp_err_t wifi_service_scan_start(void);
bool wifi_service_scan_done(void);
int wifi_service_get_ap_count(void);
esp_err_t wifi_service_get_ap(int index, wifi_svc_ap_t *out);

esp_err_t wifi_service_connect(const char *ssid, const char *password);
void wifi_service_disconnect(void);

wifi_svc_status_t wifi_service_get_status(void);
bool wifi_service_is_connected(void);
const char *wifi_service_get_ip_str(void);
const char *wifi_service_get_ssid(void);

esp_err_t wifi_service_sync_time(void);
bool wifi_service_time_valid(void);

#ifdef __cplusplus
}
#endif
