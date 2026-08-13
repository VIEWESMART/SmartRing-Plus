/*
 * smartring_plus_full_device — SmartRing-Plus 整机演示主程序
 *
 * Author : Ayang
 * Company: SHENZHEN VIEWE TECHNOLOGY CO.,LTD
 * License: Apache-2.0
 */
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "bsp/smartring_plus.h"
#include "wifi_service.h"
#include "weather_service.h"
#include "ui_shell.h"

#include "feature_album.h"
#include "feature_clock.h"
#include "feature_imu.h"
#include "feature_music.h"
#include "feature_recorder.h"
#include "feature_settings.h"
#include "feature_weather.h"

static const char *TAG = "smartring_plus_full_device";

static void register_features(void)
{
    ui_shell_register_feature(UI_FEATURE_CLOCK, feature_clock_create_screen, feature_clock_destroy);
    ui_shell_register_feature(UI_FEATURE_WEATHER, feature_weather_create_screen, feature_weather_destroy);
    ui_shell_register_feature(UI_FEATURE_RECORDER, feature_recorder_create_screen, feature_recorder_destroy);
    ui_shell_register_feature(UI_FEATURE_MUSIC, feature_music_create_screen, feature_music_destroy);
    ui_shell_register_feature(UI_FEATURE_IMU, feature_imu_create_screen, feature_imu_destroy);
    ui_shell_register_feature(UI_FEATURE_ALBUM, feature_album_create_screen, feature_album_destroy);
    ui_shell_register_feature(UI_FEATURE_SETTINGS, feature_settings_create_screen, feature_settings_destroy);
}

void app_main(void)
{
    ESP_LOGI(TAG, "smartring_plus_full_device starting");

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(bsp_display_init());
    ESP_ERROR_CHECK(bsp_battery_init());
    ESP_LOGI(TAG, "PMIC path: %s",
             bsp_battery_get_pmic_type() == BSP_PMIC_V2_AXP2101 ? "V2_AXP2101" :
             bsp_battery_get_pmic_type() == BSP_PMIC_V1_ADC ? "V1_ADC" : "unknown");
    ESP_ERROR_CHECK(bsp_power_init());

    err = bsp_sd_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "SD 卡未挂载: %s", esp_err_to_name(err));
    }

    err = bsp_audio_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "音频初始化失败: %s", esp_err_to_name(err));
    }

    err = bsp_imu_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "IMU 初始化失败: %s", esp_err_to_name(err));
    }

    ESP_ERROR_CHECK(wifi_service_init());
    ESP_ERROR_CHECK(weather_service_init());
    ESP_ERROR_CHECK(feature_album_decoder_init());

    register_features();
    ui_shell_start();

    ESP_LOGI(TAG, "初始化完成");
}
