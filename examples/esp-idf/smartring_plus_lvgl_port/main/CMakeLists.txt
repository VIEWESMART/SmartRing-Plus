set(ESPBAUD  "2000000")
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS ".")

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format" "-Wno-deprecated-declarations" "-Wno-stringop-truncation")

set_source_files_properties(
    PROPERTIES COMPILE_OPTIONS
    -DLV_LVGL_H_INCLUDE_SIMPLE)