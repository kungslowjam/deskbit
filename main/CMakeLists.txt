idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS ".")

set_source_files_properties(
    ${LV_DEMOS_SOURCES}
    PROPERTIES COMPILE_OPTIONS
    -DLV_LVGL_H_INCLUDE_SIMPLE)
