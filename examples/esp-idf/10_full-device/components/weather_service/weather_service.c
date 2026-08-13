/*
 * Amap weather API client
 * GET http://restapi.amap.com/v3/weather/weatherInfo?city={adcode}&key={key}&extensions=base
 *
 * Uses HTTP (not HTTPS): device cert-bundle fails signature check against
 * amap.com on this IDF build (see esp-x509-crt-bundle PK verify 0x4290).
 */
#include "weather_service.h"
#include "wifi_service.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "cJSON.h"

static const char *TAG = "weather_svc";

/* Amap Web service key (overridable in Settings) */
static char s_api_key[64] = "ff6e2a04a13aacea4437c43209562ff7";
/* Shenzhen city adcode */
static char s_city[32] = "440300";
static char s_city_label[32] = "Shenzhen";

static weather_data_t s_data;
static SemaphoreHandle_t s_lock;
static TaskHandle_t s_task;

typedef struct {
    char *buf;
    int len;
    int cap;
} http_resp_t;

static const char *weather_to_en(const char *cn)
{
    if (!cn || !cn[0]) {
        return "Unknown";
    }
    /* Common Amap weather strings → English UI */
    static const struct { const char *cn; const char *en; } map[] = {
        {"晴", "Sunny"}, {"多云", "Cloudy"}, {"阴", "Overcast"},
        {"小雨", "Light rain"}, {"中雨", "Rain"}, {"大雨", "Heavy rain"},
        {"暴雨", "Storm"}, {"雷阵雨", "Thunderstorm"}, {"阵雨", "Shower"},
        {"雨", "Rain"}, {"雪", "Snow"}, {"小雪", "Light snow"},
        {"中雪", "Snow"}, {"大雪", "Heavy snow"}, {"雾", "Fog"},
        {"霾", "Haze"}, {"沙尘暴", "Sandstorm"}, {"浮尘", "Dust"},
        {"扬沙", "Blowing sand"}, {"冻雨", "Sleet"}, {"雨夹雪", "Sleet"},
        {"热", "Hot"}, {"冷", "Cold"}, {"未知", "Unknown"},
    };
    for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); i++) {
        if (strcmp(cn, map[i].cn) == 0) {
            return map[i].en;
        }
        /* Partial match for compound phrases like "晴间多云" */
        if (strstr(cn, map[i].cn) != NULL) {
            return map[i].en;
        }
    }
    return cn; /* fallback: show original */
}

static void fill_mock(const char *reason)
{
    strlcpy(s_data.city, s_city_label, sizeof(s_data.city));
    if (reason && reason[0]) {
        strlcpy(s_data.weather, reason, sizeof(s_data.weather));
    } else if (s_api_key[0] == '\0') {
        strlcpy(s_data.weather, "Set Amap Key", sizeof(s_data.weather));
    } else if (!wifi_service_is_connected()) {
        strlcpy(s_data.weather, "Connect WiFi first", sizeof(s_data.weather));
    } else {
        strlcpy(s_data.weather, "Fetch failed", sizeof(s_data.weather));
    }
    strlcpy(s_data.temperature, "--°C", sizeof(s_data.temperature));
    s_data.valid = true;
    s_data.loading = false;
}

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    http_resp_t *r = (http_resp_t *)evt->user_data;
    if (evt->event_id == HTTP_EVENT_ON_DATA && r && evt->data && evt->data_len > 0) {
        if (r->len + evt->data_len < r->cap) {
            memcpy(r->buf + r->len, evt->data, (size_t)evt->data_len);
            r->len += evt->data_len;
            r->buf[r->len] = '\0';
        }
    }
    return ESP_OK;
}

static esp_err_t http_fetch(void)
{
    if (s_api_key[0] == '\0' || !wifi_service_is_connected()) {
        fill_mock(NULL);
        return ESP_ERR_INVALID_STATE;
    }

    char url[256];
    snprintf(url, sizeof(url),
             "http://restapi.amap.com/v3/weather/weatherInfo?city=%s&key=%s&extensions=base",
             s_city, s_api_key);
    ESP_LOGI(TAG, "GET %s", url);

    http_resp_t resp = {0};
    resp.cap = 2048;
    resp.buf = calloc(1, (size_t)resp.cap);
    if (!resp.buf) {
        fill_mock("Out of memory");
        return ESP_ERR_NO_MEM;
    }

    esp_http_client_config_t cfg = {
        .url = url,
        .timeout_ms = 15000,
        .event_handler = http_event_handler,
        .user_data = &resp,
        .method = HTTP_METHOD_GET,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) {
        free(resp.buf);
        fill_mock("HTTP init fail");
        return ESP_FAIL;
    }

    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK || status != 200 || resp.len <= 0) {
        ESP_LOGE(TAG, "HTTP failed: %s status=%d len=%d", esp_err_to_name(err), status, resp.len);
        free(resp.buf);
        fill_mock("HTTP error");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "response: %s", resp.buf);

    cJSON *root = cJSON_Parse(resp.buf);
    free(resp.buf);
    if (!root) {
        fill_mock("Bad JSON");
        return ESP_FAIL;
    }

    cJSON *status_j = cJSON_GetObjectItem(root, "status");
    cJSON *info = cJSON_GetObjectItem(root, "info");
    if (!cJSON_IsString(status_j) || strcmp(status_j->valuestring, "1") != 0) {
        const char *msg = (info && cJSON_IsString(info)) ? info->valuestring : "API error";
        ESP_LOGE(TAG, "Amap API error: %s", msg);
        /* Keep English-friendly short message */
        if (strstr(msg, "INVALID_USER_KEY") || strstr(msg, "USERKEY")) {
            fill_mock("Invalid Key");
        } else if (strstr(msg, "DAILY_QUERY") || strstr(msg, "CUQPS")) {
            fill_mock("Quota exceeded");
        } else {
            fill_mock("API error");
        }
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    cJSON *lives = cJSON_GetObjectItem(root, "lives");
    if (cJSON_IsArray(lives) && cJSON_GetArraySize(lives) > 0) {
        cJSON *live = cJSON_GetArrayItem(lives, 0);
        cJSON *city = cJSON_GetObjectItem(live, "city");
        cJSON *weather = cJSON_GetObjectItem(live, "weather");
        cJSON *temp = cJSON_GetObjectItem(live, "temperature");

        if (strcmp(s_city, "440300") == 0) {
            strlcpy(s_data.city, "Shenzhen", sizeof(s_data.city));
        } else if (city && cJSON_IsString(city)) {
            strlcpy(s_data.city, city->valuestring, sizeof(s_data.city));
        } else {
            strlcpy(s_data.city, s_city_label, sizeof(s_data.city));
        }

        if (weather && cJSON_IsString(weather)) {
            strlcpy(s_data.weather, weather_to_en(weather->valuestring), sizeof(s_data.weather));
        }
        if (temp && cJSON_IsString(temp)) {
            snprintf(s_data.temperature, sizeof(s_data.temperature), "%s°C", temp->valuestring);
        }
        s_data.valid = true;
        s_data.loading = false;
        ESP_LOGI(TAG, "%s %s %s", s_data.city, s_data.weather, s_data.temperature);
    } else {
        fill_mock("No data");
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    cJSON_Delete(root);
    return ESP_OK;
}

static void fetch_task(void *arg)
{
    (void)arg;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_data.loading = true;
    xSemaphoreGive(s_lock);

    http_fetch();

    s_task = NULL;
    vTaskDelete(NULL);
}

esp_err_t weather_service_init(void)
{
    s_lock = xSemaphoreCreateMutex();
    if (!s_lock) {
        return ESP_ERR_NO_MEM;
    }
    fill_mock("Connect WiFi first");
    return ESP_OK;
}

void weather_service_set_api_key(const char *key)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    if (key) {
        strlcpy(s_api_key, key, sizeof(s_api_key));
    } else {
        s_api_key[0] = '\0';
    }
    xSemaphoreGive(s_lock);
}

void weather_service_set_city(const char *city)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    if (city) {
        strlcpy(s_city, city, sizeof(s_city));
        strlcpy(s_city_label, city, sizeof(s_city_label));
        strlcpy(s_data.city, city, sizeof(s_data.city));
    }
    xSemaphoreGive(s_lock);
}

const char *weather_service_get_api_key(void)
{
    return s_api_key;
}

const char *weather_service_get_city(void)
{
    return s_city;
}

esp_err_t weather_service_fetch(void)
{
    if (s_task) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xTaskCreate(fetch_task, "weather", 8192, NULL, 5, &s_task) != pdPASS) {
        s_task = NULL;
        return ESP_FAIL;
    }
    return ESP_OK;
}

void weather_service_get_data(weather_data_t *out)
{
    if (!out) {
        return;
    }
    xSemaphoreTake(s_lock, portMAX_DELAY);
    *out = s_data;
    xSemaphoreGive(s_lock);
}
