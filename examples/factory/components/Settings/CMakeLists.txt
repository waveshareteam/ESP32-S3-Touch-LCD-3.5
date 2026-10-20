idf_component_register(
    SRCS "Settings.cpp" "service/settings_service.cpp" "assets/img_app_settings.c"
    INCLUDE_DIRS "." "service"
    REQUIRES brookesia_core lvgl esp32_s3_touch_lcd_3_5 network_service
    PRIV_REQUIRES nvs_flash esp_wifi esp_app_format esp_system esp_hw_support spi_flash esp_psram)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
