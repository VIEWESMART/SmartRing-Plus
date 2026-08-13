idf_component_register(
    SRCS "feature_music.c"
    INCLUDE_DIRS "include"
    REQUIRES ui_theme ui_shell smartring_plus lvgl fatfs
             esp_audio_codec
)
