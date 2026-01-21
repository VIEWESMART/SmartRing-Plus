idf_component_register(
    SRCS "smartring_plus.c" ${SRC_VER}
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "priv_include"
    PRIV_REQUIRES esp_lcd
)
