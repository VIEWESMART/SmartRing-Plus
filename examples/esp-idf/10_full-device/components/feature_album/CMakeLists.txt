idf_component_register(
    SRCS "feature_album.c"
    INCLUDE_DIRS "include"
    REQUIRES ui_theme ui_shell smartring_plus lvgl esp_lv_decoder esp_lvgl_adapter fatfs
)
