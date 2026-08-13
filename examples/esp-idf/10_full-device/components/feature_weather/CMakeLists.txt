idf_component_register(
    SRCS "feature_weather.c"
    INCLUDE_DIRS "include"
    REQUIRES ui_theme ui_shell weather_service lvgl
)
