# smartring_plus_full_device — Author: Ayang / SHENZHEN VIEWE TECHNOLOGY CO.,LTD
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash smartring_plus ui_shell ui_theme
             feature_clock feature_weather feature_recorder feature_music
             feature_imu feature_album feature_settings
             wifi_service weather_service
)
