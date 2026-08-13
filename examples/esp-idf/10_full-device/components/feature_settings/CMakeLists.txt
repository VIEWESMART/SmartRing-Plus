idf_component_register(
    SRCS "feature_settings.c"
    INCLUDE_DIRS "include"
    REQUIRES ui_theme ui_shell smartring_plus wifi_service weather_service lvgl esp_wifi
)
