idf_component_register(
    SRCS "weather_service.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_http_client json wifi_service freertos
)
