idf_component_register(
    SRCS "esp32_s3_touch_lcd_3_5.cpp" "bsp_board.cpp"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "priv_include"
    REQUIRES driver esp_lcd esp_lcd_st7796 esp_lcd_touch_ft6336
             esp_lvgl_port lvgl esp_codec_dev esp32-camera
             fatfs sdmmc esp_wifi esp_netif
    PRIV_REQUIRES bsp_extra XPowersLib sensorlib esp_io_expander_tca9554
                  button nvs_flash esp_timer esp_event)


# Keep hardware initialization separate from the APP audio-session layer.
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
